#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../../common/constants.h"
#include "../../../common/map.h"
#include "../../../common/parser.h"
#include "../../../common/result.h"
#include "../../../common/tcp.h"
#include "../../../common/vec2.h"
#include "../../common/hpa.h"
#include "../../common/graph_a.h"
#include "preprocess.h"

#define STB_DS_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include "../../../common/stb_ds.h"

static volatile bool running = true;

void signal_handler(int sig)
{
    running = false;
    printf("\nShutdown signal received\n");
}

void divide_clusters(const Map* map, int* clusters_per_worker, int connections_count)
{
    const size_t cluster_w = (size_t)ceil(map->w / (float)CLUSTER_SIZE);
    const size_t cluster_h = (size_t)ceil(map->h / (float)CLUSTER_SIZE);
    const size_t cluster_size = cluster_w * cluster_h;
    const size_t clusters_per_worker_base = (size_t)ceil(cluster_size / (float)connections_count);

    int remaining_clusters = cluster_size;
    for (size_t worker = 0; worker < connections_count; worker++)
    {
        int old = remaining_clusters;
        clusters_per_worker[worker] = 0;
        for (size_t i = 0; i < clusters_per_worker_base; i++)
        {
            if (remaining_clusters <= 0)
            {
                break;
            }

            clusters_per_worker[worker] = clusters_per_worker[worker] + 1;

            remaining_clusters--;
        }
        printf("Worker %ld is getting %d clusters: %d-%d\n", worker, clusters_per_worker[worker], remaining_clusters,
               old);
    }
}

void send_cluster_assignments(const Map* map, int* worker_fds, int workers_count, int* clusters_per_worker)
{
    const size_t cluster_w = (size_t)ceil(map->w / (float)CLUSTER_SIZE);
    const size_t cluster_h = (size_t)ceil(map->h / (float)CLUSTER_SIZE);
    const size_t cluster_size = cluster_w * cluster_h;

    // Track which cluster index we're at for each worker
    int cluster_offset = 0;

    for (int worker = 0; worker < workers_count; worker++)
    {
        int num_clusters = clusters_per_worker[worker];

        // Calculate payload size: 4 bytes for count + 4 bytes per cluster (2 bytes x, 2 bytes y)
        uint32_t payload_size = sizeof(uint32_t) + (num_clusters * sizeof(int16_t) * 2);
        void* payload = malloc(payload_size);

        if (!payload)
        {
            fprintf(stderr, "Failed to allocate memory for cluster assignment payload\n");
            continue;
        }

        // Pack the data: first 4 bytes = cluster count, then cluster positions
        uint32_t* count_ptr = (uint32_t*)payload;
        *count_ptr = num_clusters;

        int16_t* positions = (int16_t*)(payload + sizeof(uint32_t));

        // Extract cluster positions from linear cluster index
        for (int i = 0; i < num_clusters; i++)
        {
            size_t cluster_idx = cluster_offset + i;
            int16_t cluster_x = (int16_t)(cluster_idx % cluster_w);
            int16_t cluster_y = (int16_t)(cluster_idx / cluster_w);

            positions[i * 2] = cluster_x;
            positions[i * 2 + 1] = cluster_y;
        }

        // Send the message to worker
        if (tcp_send_message(worker_fds[worker], MSG_CLUSTER_ASSIGNMENT, payload, payload_size) < 0)
        {
            fprintf(stderr, "Failed to send cluster assignment to worker %d\n", worker);
        }
        else
        {
            printf("Sent cluster assignment to worker %d: %d clusters (indices %d-%d)\n", worker, num_clusters,
                   cluster_offset, cluster_offset + num_clusters - 1);
            for (int i = 0; i < num_clusters && i < 5; i++)
            {
                printf("  Cluster %d: pos (%d, %d)\n", cluster_offset + i, positions[i * 2], positions[i * 2 + 1]);
            }
            if (num_clusters > 5)
                printf("  ... and %d more clusters\n", num_clusters - 5);
        }

        cluster_offset += num_clusters;
        free(payload);
    }
}

/**
 * Cluster path segment with start and goal within cluster
 */
typedef struct {
    Vec2 cluster_pos;      // cluster grid position
    Vec2 start_in_cluster; // start coordinates within this cluster
    Vec2 goal_in_cluster;  // goal coordinates within this cluster
} ClusterPathSegment;

/**
 * Extract clusters from a graph_a path with per-cluster start/goal coordinates
 * @param path The Vec2* path returned by graph_a
 * @param path_length The number of waypoints in the path
 * @param global_start The global start position
 * @param global_goal The global goal position
 * @return ClusterPathSegment* array with cluster info and per-cluster start/goal, caller must free with arrfree
 */
ClusterPathSegment* extract_clusters_from_path(const Vec2* path, size_t path_length, Vec2 global_start, Vec2 global_goal)
{
    ClusterPathSegment* segments = NULL;
    
    if (!path || path_length == 0)
    {
        return NULL;
    }

    // Group waypoints by cluster
    for (size_t i = 0; i < path_length; i++)
    {
        int16_t cluster_x = (int16_t)(path[i].x / CLUSTER_SIZE);
        int16_t cluster_y = (int16_t)(path[i].y / CLUSTER_SIZE);
        Vec2 cluster_pos = (Vec2){cluster_x, cluster_y};

        // Check if this cluster is already in our list
        int found = -1;
        for (size_t j = 0; j < arrlen(segments); j++)
        {
            if (vec2_equal(segments[j].cluster_pos, cluster_pos))
            {
                found = (int)j;
                break;
            }
        }

        if (found == -1)
        {
            // New cluster - initialize segment
            ClusterPathSegment seg = {
                .cluster_pos = cluster_pos,
                .start_in_cluster = path[i],
                .goal_in_cluster = path[i]
            };
            arrput(segments, seg);
        }
        else
        {
            // Update goal for existing cluster
            segments[found].goal_in_cluster = path[i];
        }
    }

    // Adjust start/goal to use global start/goal where appropriate
    if (arrlen(segments) > 0)
    {
        // First cluster uses global start if it's in that cluster
        if ((int16_t)(global_start.x / CLUSTER_SIZE) == segments[0].cluster_pos.x &&
            (int16_t)(global_start.y / CLUSTER_SIZE) == segments[0].cluster_pos.y)
        {
            segments[0].start_in_cluster = global_start;
        }

        // Last cluster uses global goal if it's in that cluster
        size_t last = arrlen(segments) - 1;
        if ((int16_t)(global_goal.x / CLUSTER_SIZE) == segments[last].cluster_pos.x &&
            (int16_t)(global_goal.y / CLUSTER_SIZE) == segments[last].cluster_pos.y)
        {
            segments[last].goal_in_cluster = global_goal;
        }
    }

    return segments;
}

/**
 * Determine which worker owns a given cluster based on cluster grid position
 * @param cluster_pos The cluster grid position
 * @param map The map to calculate total clusters
 * @param clusters_per_worker Array of cluster counts per worker
 * @param workers_count Number of workers
 * @return The worker index that owns this cluster, or -1 if not found
 */
int get_worker_for_cluster(Vec2 cluster_pos, const Map* map, int* clusters_per_worker, int workers_count)
{
    const size_t cluster_w = (size_t)ceil(map->w / (float)CLUSTER_SIZE);
    
    // Convert cluster grid position to linear index
    size_t cluster_idx = (size_t)cluster_pos.y * cluster_w + (size_t)cluster_pos.x;
    
    // Find which worker owns this cluster
    size_t offset = 0;
    for (int worker = 0; worker < workers_count; worker++)
    {
        if (cluster_idx < offset + clusters_per_worker[worker])
        {
            return worker;
        }
        offset += clusters_per_worker[worker];
    }
    
    return -1; // Cluster not assigned to any worker
}

/**
 * Send pathfinding packets for clusters found in a graph_a path to the correct workers
 * @param worker_fds Array of worker file descriptors
 * @param workers_count Number of workers
 * @param map The map for cluster calculations
 * @param clusters_per_worker Array of cluster counts per worker
 * @param graph_path The path returned by graph_a
 * @param path_length Number of waypoints in the path
 * @param global_start The global start position
 * @param global_goal The global goal position
 */
void send_cluster_pathfinding_packets(int* worker_fds, int workers_count, const Map* map, 
                                      int* clusters_per_worker, const Vec2* graph_path, 
                                      size_t path_length, Vec2 global_start, Vec2 global_goal)
{
    if (!graph_path || path_length == 0)
    {
        printf("No path found, skipping cluster pathfinding packets\n");
        return;
    }

    // Extract unique clusters from the path with per-cluster start/goal
    ClusterPathSegment* segments = extract_clusters_from_path(graph_path, path_length, global_start, global_goal);
    
    if (!segments || arrlen(segments) == 0)
    {
        printf("No clusters extracted from path\n");
        if (segments)
            arrfree(segments);
        return;
    }

    printf("Extracted %ld clusters from graph_a path\n", arrlen(segments));
    printf("Sending cluster pathfinding packets to correct workers...\n");

    // Send a task request to the correct worker for each cluster
    uint32_t task_id = 1;
    for (size_t i = 0; i < arrlen(segments); i++)
    {
        int worker = get_worker_for_cluster(segments[i].cluster_pos, map, clusters_per_worker, workers_count);
        
        if (worker < 0 || worker >= workers_count)
        {
            fprintf(stderr, "ERROR: No worker assigned to cluster (%d, %d)\n", 
                   segments[i].cluster_pos.x, segments[i].cluster_pos.y);
            continue;
        }

        TaskRequest task = {
            .task_id = task_id++,
            .start_x = (float)segments[i].start_in_cluster.x,
            .start_y = (float)segments[i].start_in_cluster.y,
            .goal_x = (float)segments[i].goal_in_cluster.x,
            .goal_y = (float)segments[i].goal_in_cluster.y,
        };

        if (tcp_send_task_request(worker_fds[worker], &task) < 0)
        {
            fprintf(stderr, "Failed to send pathfinding packet to worker %d for cluster (%d, %d)\n", 
                   worker, segments[i].cluster_pos.x, segments[i].cluster_pos.y);
            continue;
        }

        printf("  Sent task %u to worker %d for cluster (%d, %d) start=(%d, %d) goal=(%d, %d)\n", 
               task.task_id, worker, segments[i].cluster_pos.x, segments[i].cluster_pos.y,
               task.start_x, task.start_y, task.goal_x, task.goal_y);
    }

    printf("Sent %u total pathfinding packets\n", task_id - 1);
    arrfree(segments);
}

int main(int argc, char const* argv[])
{
    signal(SIGINT, signal_handler);

    uint16_t port = 9090;
    const char* host = "127.0.0.1";

    // Create TCP server
    tcp_server* server = tcp_server_create(host, port, 10);
    if (!server)
    {
        fprintf(stderr, "Failed to create TCP server\n");
        return 1;
    }

    // Store worker connections
    int* worker_fds = NULL;
    Map map = parse_map("../data/sparse/scene_mp_2p_01");
    int clusters_per_worker[WORKERS_SIZE] = {0};

    // Accept worker connections until we reach WORKERS_SIZE
    while (running && arrlen(worker_fds) < WORKERS_SIZE)
    {
        // Accept worker connection
        int client_fd = tcp_server_accept(server);
        if (client_fd < 0)
        {
            if (running)
            {
                fprintf(stderr, "Failed to accept connection\n");
            }
            continue;
        }

        printf("Worker %d connected (fd=%d)\n", (int)arrlen(worker_fds), client_fd);
        arrput(worker_fds, client_fd);

        if (arrlen(worker_fds) == WORKERS_SIZE)
        {
            printf("All %d workers connected. Running divide_clusters...\n", WORKERS_SIZE);

            // Calculate cluster distribution
            divide_clusters(&map, clusters_per_worker, WORKERS_SIZE);

            // Send cluster assignments to all workers
            send_cluster_assignments(&map, worker_fds, WORKERS_SIZE, clusters_per_worker);
            break;
        }
    }


    Graph* graph = NULL;
    Cluster* clusters = NULL;
    
    // start/end goal
    Vec2 start = (Vec2){260, 180};
    Vec2 goal = (Vec2){1565, 1745};
    
    preprocess(&map, &graph, clusters, start, goal);

    // Close server from accepting more connections
    tcp_server_destroy(&server);

    // Main worker communication loop
    printf("Master entering main communication loop with %d workers\n", (int)arrlen(worker_fds));

    while (running && arrlen(worker_fds) > 0)
    {
        printf("\nStarting pathfinding with graph_a...\n");

        printf("Computing graph-level path from (%d, %d) to (%d, %d)\n", 
               start.x, start.y, goal.x, goal.y);

        // Call graph_a to find graph path
        Vec2* graph_path = graph_a(&map, graph, start, goal);
        
        if (graph_path != NULL)
        {
            printf("Graph_a found path with %ld waypoints\n", arrlen(graph_path));
            for (size_t i = 0; i < arrlen(graph_path) && i < 5; i++)
            {
                printf("  Waypoint %ld: (%d, %d)\n", i, graph_path[i].x, graph_path[i].y);
            }
            if (arrlen(graph_path) > 5)
                printf("  ... and %ld more waypoints\n", arrlen(graph_path) - 5);

            // Send pathfinding packets for all clusters found by graph_a to the correct workers
            send_cluster_pathfinding_packets(worker_fds, WORKERS_SIZE, &map, clusters_per_worker, graph_path, arrlen(graph_path), start, goal);

            // Receive responses from workers
            printf("\nWaiting for responses from workers...\n");
            for (int i = 0; i < arrlen(worker_fds); i++)
            {
                TaskResponse* response = NULL;
                if (tcp_recv_task_response(worker_fds[i], &response) < 0)
                {
                    fprintf(stderr, "Failed to receive task response from worker %d\n", i);
                    close(worker_fds[i]);
                    arrdel(worker_fds, i);
                    i--;
                    continue;
                }

                printf("Received task response from worker %d (id=%u, path_length=%u, status=%d)\n", i, response->task_id,
                       response->path_length, response->status_code);

                if (response->path_length > 0 && response->path)
                {
                    printf("  Path waypoints: ");
                    for (uint32_t j = 0; j < response->path_length && j < 5; j++)
                    {
                        printf("(%d, %d) ", response->path[j].x, response->path[j].y);
                    }
                    if (response->path_length > 5)
                        printf("...");
                    printf("\n");
                }

                // Cleanup
                tcp_taskresponse_free(&response);
            }

            arrfree(graph_path);
        }
        else
        {
            printf("graph_a returned no path\n");
        }

        // Exit after one round for demo purposes
        break;
    }

    // Cleanup: close all worker connections
    for (int i = 0; i < arrlen(worker_fds); i++)
    {
        close(worker_fds[i]);
    }
    arrfree(worker_fds);

    tcp_server_destroy(&server);
    printf("Master shutdown complete\n");
    return 0;
}
