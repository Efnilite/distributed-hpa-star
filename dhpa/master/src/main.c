#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include "../../../common/constants.h"
#include "../../../common/map.h"
#include "../../../common/parser.h"
#include "../../../common/result.h"
#include "../../../common/tcp.h"
#include "../../../common/util.h"
#include "../../../common/vec2.h"
#include "../../common/graph_a.h"
#include "../../common/hpa.h"
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

void divide_clusters(const MapDimensions map, int* clusters_per_worker, int connections_count)
{
    const size_t clusters_per_worker_base = (size_t)ceil(map.clusters_size / (float)connections_count);

    int remaining_clusters = map.clusters_size;
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

void send_cluster_assignments(const MapDimensions map, int* worker_fds, int workers_count, int* clusters_per_worker)
{
    // Track which cluster index we're at for each worker
    int cluster_offset = 0;

    for (int worker = 0; worker < workers_count; worker++)
    {
        int num_clusters = clusters_per_worker[worker];

        // Calculate payload size: 2 bytes for worker_id + 4 bytes for count + 4 bytes per cluster (2 bytes x, 2 bytes y)
        uint32_t payload_size = sizeof(uint16_t) + sizeof(uint32_t) + (num_clusters * sizeof(int16_t) * 2);
        void* payload = malloc(payload_size);

        if (!payload)
        {
            fprintf(stderr, "Failed to allocate memory for cluster assignment payload\n");
            continue;
        }

        // Pack the data: first 2 bytes = worker_id, next 4 bytes = cluster count, then cluster positions
        uint16_t* worker_id_ptr = (uint16_t*)payload;
        *worker_id_ptr = (uint16_t)worker;

        uint32_t* count_ptr = (uint32_t*)(payload + sizeof(uint16_t));
        *count_ptr = num_clusters;

        int16_t* positions = (int16_t*)(payload + sizeof(uint16_t) + sizeof(uint32_t));

        // Extract cluster positions from linear cluster index
        for (int i = 0; i < num_clusters; i++)
        {
            size_t cluster_idx = cluster_offset + i;
            int16_t cluster_x = (int16_t)(cluster_idx % map.clusters_w);
            int16_t cluster_y = (int16_t)(cluster_idx / map.clusters_w);

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
            printf("Sent cluster assignment to worker %d (worker_id=%u): %d clusters (indices %d-%d)\n", worker,
                   (uint16_t)worker, num_clusters, cluster_offset, cluster_offset + num_clusters - 1);
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
typedef struct
{
    Vec2 cluster_pos; // cluster grid position
    Vec2 start_in_cluster; // start coordinates within this cluster
    Vec2 goal_in_cluster; // goal coordinates within this cluster
} ClusterPathSegment;

/**
 * Extract clusters from a graph_a path with per-cluster start/goal coordinates
 * @param path The Vec2* path returned by graph_a
 * @param path_length The number of waypoints in the path
 * @param global_start The global start position
 * @param global_goal The global goal position
 * @return ClusterPathSegment* array with cluster info and per-cluster start/goal, caller must free with arrfree
 */
ClusterPathSegment* extract_clusters_from_path(const Vec2* path, size_t path_length, Vec2 global_start,
                                               Vec2 global_goal)
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
                .cluster_pos = cluster_pos, .start_in_cluster = path[i], .goal_in_cluster = path[i]};
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
int get_worker_for_cluster(Vec2 cluster_pos, const MapDimensions map, int* clusters_per_worker, int workers_count)
{
    const size_t cluster_w = (size_t)ceil(map.w / (float)CLUSTER_SIZE);

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
 * @return Number of packets successfully sent
 */
uint32_t send_cluster_pathfinding_packets(int* worker_fds, int workers_count, const MapDimensions map,
                                          int* clusters_per_worker, const Vec2* graph_path, size_t path_length,
                                          Vec2 global_start, Vec2 global_goal)
{
    if (!graph_path || path_length == 0)
    {
        printf("No path found, skipping cluster pathfinding packets\n");
        return 0;
    }

    // Extract unique clusters from the path with per-cluster start/goal
    ClusterPathSegment* segments = extract_clusters_from_path(graph_path, path_length, global_start, global_goal);

    if (!segments || arrlen(segments) == 0)
    {
        printf("No clusters extracted from path\n");
        if (segments)
            arrfree(segments);
        return 0;
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
            fprintf(stderr, "ERROR: No worker assigned to cluster (%d, %d)\n", segments[i].cluster_pos.x,
                    segments[i].cluster_pos.y);
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
            fprintf(stderr, "Failed to send pathfinding packet to worker %d for cluster (%d, %d)\n", worker,
                    segments[i].cluster_pos.x, segments[i].cluster_pos.y);
            continue;
        }

        printf("  Sent task %u to worker %d for cluster (%d, %d) start=(%d, %d) goal=(%d, %d)\n", task.task_id, worker,
               segments[i].cluster_pos.x, segments[i].cluster_pos.y, task.start_x, task.start_y, task.goal_x,
               task.goal_y);
    }

    uint32_t packets_sent = task_id - 1;
    printf("Sent %u total pathfinding packets\n", packets_sent);
    arrfree(segments);

    return packets_sent;
}

int main(int argc, char const* argv[])
{
    signal(SIGINT, signal_handler);

    const char* host = getenv("MASTER_HOST");
    if (!host)
    {
        host = "127.0.0.1";
    }
    // if (strcmp(host, "0.0.0.0") == 0)
    // {
    //     fprintf(stderr, "Cannot have 0.0.0.0 as host\n");
    //     return 1;
    // }

    const char* port_str = getenv("MASTER_PORT");
    uint16_t port = 9090;
    if (port_str)
    {
        port = (uint16_t)atoi(port_str);
    }

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

    Graph* graph = NULL;

    // start/end goal
    Vec2 start = (Vec2){260, 180};
    Vec2 goal = (Vec2){1565, 1745};

    preprocess(&map, &graph, start, goal);

    MapDimensions dimensions = (MapDimensions){.w = map.w,
                                               .h = map.h,
                                               .clusters_w = (size_t)ceil(map.w / (float)CLUSTER_SIZE),
                                               .clusters_h = (size_t)ceil(map.h / (float)CLUSTER_SIZE),
                                               .clusters_size = (size_t)ceil(map.w / (float)CLUSTER_SIZE) *
                                                   (size_t)ceil(map.h / (float)CLUSTER_SIZE)};
    int clusters_per_worker[WORKERS_SIZE] = {0};

    long max_memory = 0;
    max_memory = get_memory_usage(max_memory);

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
            divide_clusters(dimensions, clusters_per_worker, WORKERS_SIZE);

            // Send cluster assignments to all workers
            send_cluster_assignments(dimensions, worker_fds, WORKERS_SIZE, clusters_per_worker);
            break;
        }
    }

    max_memory = get_memory_usage(max_memory);

    // Close server from accepting more connections
    tcp_server_destroy(&server);
    max_memory = get_memory_usage(max_memory);
    clock_t time = clock();

    // Main worker communication loop
    printf("Master entering main communication loop with %d workers\n", (int)arrlen(worker_fds));

    printf("\nStarting pathfinding with graph_a...\n");

    printf("Computing graph-level path from (%d, %d) to (%d, %d)\n", start.x, start.y, goal.x, goal.y);

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
        uint32_t packets_sent = send_cluster_pathfinding_packets(
            worker_fds, WORKERS_SIZE, dimensions, clusters_per_worker, graph_path, arrlen(graph_path), start, goal);
        max_memory = get_memory_usage(max_memory);

        arrfree(graph_path);

        // Receive responses from all workers using select()
        printf("\nWaiting for responses from all workers...\n");
        printf("Expecting %u responses\n", packets_sent);

        uint32_t responses_received = 0;
        fd_set read_fds;
        int max_fd = -1;

        // Find max fd for select
        for (int i = 0; i < arrlen(worker_fds); i++)
        {
            if (worker_fds[i] > max_fd)
                max_fd = worker_fds[i];
        }

        // Use a map to store responses by task_id to handle out-of-order packets
        // Key: task_id, Value: TaskResponse*
        typedef struct
        {
            uint32_t task_id;
            TaskResponse* response;
        } TaskResponseEntry;

        TaskResponseEntry* responses_map = NULL;

        // Keep receiving until we get all responses
        while (responses_received < packets_sent)
        {
            FD_ZERO(&read_fds);
            for (int i = 0; i < arrlen(worker_fds); i++)
            {
                if (worker_fds[i] >= 0)
                    FD_SET(worker_fds[i], &read_fds);
            }

            struct timeval tv = {.tv_sec = 5, .tv_usec = 0}; // 5 second timeout
            int select_result = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

            if (select_result < 0)
            {
                perror("select");
                break;
            }
            else if (select_result == 0)
            {
                printf("Timeout waiting for responses. Received %u/%u responses.\n", responses_received, packets_sent);
                break;
            }

            // Check which sockets have data ready
            for (int i = 0; i < arrlen(worker_fds); i++)
            {
                if (worker_fds[i] >= 0 && FD_ISSET(worker_fds[i], &read_fds))
                {
                    TaskResponse* response = NULL;
                    if (tcp_recv_task_response(worker_fds[i], &response) < 0)
                    {
                        fprintf(stderr, "Failed to receive task response from worker %d or connection closed\n", i);
                        close(worker_fds[i]);
                        worker_fds[i] = -1;
                        continue;
                    }

                    responses_received++;
                    printf("Received response %u/%u from worker %d (task_id=%u, path_length=%u, status=%d)\n",
                           responses_received, packets_sent, i, response->task_id, response->path_length,
                           response->status_code);

                    if (response->path_length > 0 && response->path)
                    {
                        printf("  Path waypoints: ");
                        for (uint32_t j = 0; j < response->path_length && j < 5; j++)
                        {
                            printf("(%d, %d) ", response->path[j].x, response->path[j].y);
                        }
                        if (response->path_length > 5)
                            printf("... and %u more", response->path_length - 5);
                        printf("\n");
                    }

                    // Store response by task_id for later ordering
                    TaskResponseEntry entry = {.task_id = response->task_id, .response = response};
                    arrput(responses_map, entry);
                    max_memory = get_memory_usage(max_memory);
                }
            }
        }

        printf("Finished receiving responses. Total: %u/%u\n", responses_received, packets_sent);

        // Compile final path by processing responses in task_id order
        Vec2* result = NULL;
        max_memory = get_memory_usage(max_memory);
        WorkerResult workers[WORKERS_SIZE];
        for (size_t i = 0; i < WORKERS_SIZE; i++)
        {
            workers[i] = (WorkerResult) {
                .cpu_time = 0,
                .max_memory_bytes = 0
            };
        }
        

        // Sort responses by task_id if not already in order
        for (uint32_t task_id = 1; task_id <= packets_sent; task_id++)
        {
            for (size_t i = 0; i < arrlen(responses_map); i++)
            {
                if (responses_map[i].task_id == task_id)
                {
                    TaskResponse* resp = responses_map[i].response;

                    WorkerResult existing = workers[resp->worker_id];
                    existing.cpu_time += resp->cpu_time;
                    if (resp->max_memory_bytes > existing.max_memory_bytes) {
                        existing.max_memory_bytes = resp->max_memory_bytes;
                    }
                    
                    if (resp->path_length > 0 && resp->path)
                    {
                        for (uint32_t j = 0; j < resp->path_length; j++)
                        {
                            arrpush(result, resp->path[j]);
                        }
                        printf("Added %u points from task_id %u\n", resp->path_length, task_id);
                    }
                    break;
                }
            }
        }

        max_memory = get_memory_usage(max_memory);

        result_visualize(&map,
                         &(Result){.success = true,
                                   .graph = graph,
                                   .path = result,
                                   .max_memory_bytes = max_memory,
                                   .cpu_secs = (double)(clock() - time) / CLOCKS_PER_SEC,
                                   .visited = NULL,
                                   .workers = workers});

        // Cleanup responses
        for (size_t i = 0; i < arrlen(responses_map); i++)
        {
            tcp_taskresponse_free(&responses_map[i].response);
        }
        arrfree(responses_map);
        arrfree(result);
    }
    else
    {
        printf("graph_a returned no path\n");
    }


    // Cleanup: close all worker connections
    for (int i = 0; i < arrlen(worker_fds); i++)
    {
        tcp_send_shutdown(worker_fds[i]);
        close(worker_fds[i]);
    }
    arrfree(worker_fds);

    tcp_server_destroy(&server);
    printf("Master shutdown complete\n");
    return 0;
}
