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

void divide_clusters(const MapDimensions map, int* clusters_per_worker)
{
    for (size_t worker = 0; worker < WORKERS_SIZE; worker++)
    {
        clusters_per_worker[worker] = 0;
    }

    // alternatingly assign clusters
    for (size_t cluster_idx = 0; cluster_idx < map.clusters_size; cluster_idx++)
    {
        size_t worker = cluster_idx % WORKERS_SIZE;
        clusters_per_worker[worker]++;
    }

    for (size_t worker = 0; worker < WORKERS_SIZE; worker++)
    {
        printf("Worker %ld is getting %d clusters\n", worker, clusters_per_worker[worker]);
    }
}

void send_cluster_assignments(const MapDimensions map, int* worker_fds, int* clusters_per_worker)
{
    typedef struct
    {
        int16_t* positions;
        int count;
    } WorkerClusters;

    WorkerClusters* worker_clusters = (WorkerClusters*)malloc(WORKERS_SIZE * sizeof(WorkerClusters));
    for (int i = 0; i < WORKERS_SIZE; i++)
    {
        worker_clusters[i].positions = (int16_t*)malloc(clusters_per_worker[i] * 2 * sizeof(int16_t));
        worker_clusters[i].count = 0;
    }

    // assign clusters in alternating manner
    for (size_t cluster_idx = 0; cluster_idx < map.clusters_size; cluster_idx++)
    {
        int worker = cluster_idx % WORKERS_SIZE;
        int16_t cluster_x = (int16_t)(cluster_idx % map.clusters_w);
        int16_t cluster_y = (int16_t)(cluster_idx / map.clusters_w);

        int pos_idx = worker_clusters[worker].count * 2;
        worker_clusters[worker].positions[pos_idx] = cluster_x;
        worker_clusters[worker].positions[pos_idx + 1] = cluster_y;
        worker_clusters[worker].count++;
    }

    for (int worker = 0; worker < WORKERS_SIZE; worker++)
    {
        int num_clusters = clusters_per_worker[worker];
        uint32_t payload_size = sizeof(uint16_t) + sizeof(uint32_t) + (num_clusters * sizeof(int16_t) * 2);
        void* payload = malloc(payload_size);

        if (!payload)
        {
            fprintf(stderr, "Failed to allocate memory for cluster assignment payload\n");
            continue;
        }

        // first 2 bytes = worker_id, next 4 bytes = cluster count, then cluster positions
        uint16_t* worker_id_ptr = (uint16_t*)payload;
        *worker_id_ptr = (uint16_t)worker;

        uint32_t* count_ptr = (uint32_t*)(payload + sizeof(uint16_t));
        *count_ptr = num_clusters;

        int16_t* positions = (int16_t*)(payload + sizeof(uint16_t) + sizeof(uint32_t));
        memcpy(positions, worker_clusters[worker].positions, num_clusters * sizeof(int16_t) * 2);

        if (tcp_send_message(worker_fds[worker], MSG_CLUSTER_ASSIGNMENT, payload, payload_size) < 0)
        {
            fprintf(stderr, "Failed to send cluster assignment to worker %d\n", worker);
        }
        else
        {
            printf("Sent cluster assignment to worker %d: %d clusters\n", worker, num_clusters);
        }

        free(payload);
    }

    for (int i = 0; i < WORKERS_SIZE; i++)
    {
        free(worker_clusters[i].positions);
    }
    free(worker_clusters);
}

/**
 * Cluster path segment with start and goal within cluster
 */
typedef struct
{
    Vec2 cluster_pos;
    Vec2 start_in_cluster;
    Vec2 goal_in_cluster;
} ClusterPathSegment;

/**
 * Extract clusters from a graph path with per-cluster start/goal coordinates
 * @param path The path
 * @param path_length The number of waypoints in the path
 * @param global_start The global start position
 * @param global_goal The global goal position
 * @return ClusterPathSegment*
 */
ClusterPathSegment* extract_clusters_from_path(const Vec2* path, size_t path_length, Vec2 global_start,
                                               Vec2 global_goal)
{
    ClusterPathSegment* segments = NULL;

    if (!path || path_length == 0)
    {
        return NULL;
    }

    for (size_t i = 0; i < path_length; i++)
    {
        int16_t cluster_x = (int16_t)(path[i].x / CLUSTER_SIZE);
        int16_t cluster_y = (int16_t)(path[i].y / CLUSTER_SIZE);
        Vec2 cluster_pos = (Vec2){cluster_x, cluster_y};

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
            ClusterPathSegment seg = {
                .cluster_pos = cluster_pos, .start_in_cluster = path[i], .goal_in_cluster = path[i]};
            arrput(segments, seg);
        }
        else
        {
            segments[found].goal_in_cluster = path[i];
        }
    }

    if (arrlen(segments) > 0)
    {
        if ((int16_t)(global_start.x / CLUSTER_SIZE) == segments[0].cluster_pos.x &&
            (int16_t)(global_start.y / CLUSTER_SIZE) == segments[0].cluster_pos.y)
        {
            segments[0].start_in_cluster = global_start;
        }

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
 * Gets the worker that owns a given cluster based on cluster grid position
 * @param cluster_pos The cluster grid position
 * @param map The map to calculate total clusters
 * @return The worker index that owns this cluster
 */
int get_worker_for_cluster(Vec2 cluster_pos, const MapDimensions map)
{
    size_t cluster_idx = (size_t)cluster_pos.y * map.clusters_w + (size_t)cluster_pos.x;
    return cluster_idx % WORKERS_SIZE;
}

/**
 * Send pathfinding packets for clusters found in a graph path to the correct workers
 * @param worker_fds Array of worker file descriptors
 * @param workers_count Number of workers
 * @param map The map for cluster calculations
 * @param clusters_per_worker Array of cluster counts per worker
 * @param graph_path The path
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

    ClusterPathSegment* segments = extract_clusters_from_path(graph_path, path_length, global_start, global_goal);

    if (!segments || arrlen(segments) == 0)
    {
        printf("No clusters extracted from path\n");
        if (segments)
            arrfree(segments);
        return 0;
    }

    printf("Extracted %ld clusters from graph_a path\n", arrlen(segments));
    printf("Sending cluster pathfinding packets to correct workers\n");

    uint32_t task_id = 1;
    for (size_t i = 0; i < arrlen(segments); i++)
    {
        int worker = get_worker_for_cluster(segments[i].cluster_pos, map);

        if (worker < 0 || worker >= workers_count)
        {
            fprintf(stderr, "No worker assigned to cluster (%d, %d)\n", segments[i].cluster_pos.x,
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

        printf("Sent task %u to worker %d for cluster (%d, %d) start=(%d, %d) goal=(%d, %d)\n", task.task_id, worker,
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

    tcp_server* server = tcp_server_create(host, port, 10);
    if (!server)
    {
        fprintf(stderr, "Failed to create TCP server\n");
        return 1;
    }

    // store worker connections
    int* worker_fds = NULL;

    Graph* graph = NULL;

    // start/end goal
    Vec2 start = START;
    Vec2 goal = GOAL;

    uint16_t map_width = 0, map_height = 0;
    if (parse_map_dimensions(MAP_FILE, &map_width, &map_height) < 0)
    {
        fprintf(stderr, "Failed to parse map dimensions from %s\n", MAP_FILE);
        return 1;
    }

    MapDimensions dimensions = (MapDimensions){.w = map_width,
                                               .h = map_height,
                                               .clusters_w = (size_t)ceil(map_width / (float)CLUSTER_SIZE),
                                               .clusters_h = (size_t)ceil(map_height / (float)CLUSTER_SIZE),
                                               .clusters_size = (size_t)ceil(map_width / (float)CLUSTER_SIZE) *
                                                   (size_t)ceil(map_height / (float)CLUSTER_SIZE)};
    int clusters_per_worker[WORKERS_SIZE];

    long max_memory = 0;
    max_memory = get_memory_usage(max_memory);

    while (running && arrlen(worker_fds) < WORKERS_SIZE)
    {
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
            printf("All %d workers connected\n", WORKERS_SIZE);
            divide_clusters(dimensions, clusters_per_worker);
            send_cluster_assignments(dimensions, worker_fds, clusters_per_worker);
            break;
        }
    }

    preprocess(&dimensions, &graph, start, goal);

    max_memory = get_memory_usage(max_memory);

    tcp_server_destroy(&server);
    max_memory = get_memory_usage(max_memory);
    clock_t time = clock();

    printf("Computing graph-level path from (%d, %d) to (%d, %d)\n", start.x, start.y, goal.x, goal.y);

    Vec2* graph_path = graph_a(&dimensions, graph, start, goal);

    if (graph_path != NULL)
    {

        printf("Found graph path - %fs\n", (double)(clock() - time) / CLOCKS_PER_SEC);
        time = clock();

        uint32_t packets_sent = send_cluster_pathfinding_packets(
            worker_fds, WORKERS_SIZE, dimensions, clusters_per_worker, graph_path, arrlen(graph_path), start, goal);
        max_memory = get_memory_usage(max_memory);

        arrfree(graph_path);

        printf("Waiting for responses from all workers\n");
        printf("Expecting %u responses\n", packets_sent);

        uint32_t responses_received = 0;
        fd_set read_fds;
        int max_fd = -1;

        for (int i = 0; i < arrlen(worker_fds); i++)
        {
            if (worker_fds[i] > max_fd)
                max_fd = worker_fds[i];
        }

        typedef struct
        {
            uint32_t task_id;
            TaskResponse* response;
        } TaskResponseEntry;

        TaskResponseEntry* responses_map = NULL;

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
                printf("Timeout waiting for responses: received %u/%u.\n", responses_received, packets_sent);
                break;
            }

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

                    TaskResponseEntry entry = {.task_id = response->task_id, .response = response};
                    arrput(responses_map, entry);
                    max_memory = get_memory_usage(max_memory);
                }
            }
        }

        printf("Finished receiving responses. Total: %u/%u\n", responses_received, packets_sent);

        Vec2* result = NULL;
        max_memory = get_memory_usage(max_memory);
        WorkerResult* workers = malloc(WORKERS_SIZE * sizeof(WorkerResult));
        for (size_t i = 0; i < WORKERS_SIZE; i++)
        {
            workers[i] = (WorkerResult) {
                .cpu_time = 0,
                .max_memory_bytes = 0
            };
        }

        for (uint32_t task_id = 1; task_id <= packets_sent; task_id++)
        {
            for (size_t i = 0; i < arrlen(responses_map); i++)
            {
                if (responses_map[i].task_id == task_id)
                {
                    TaskResponse* resp = responses_map[i].response;

                    WorkerResult* existing = workers + resp->worker_id;
                    existing->cpu_time += resp->cpu_time;
                    if (resp->max_memory_bytes > existing->max_memory_bytes) {
                        existing->max_memory_bytes = resp->max_memory_bytes;
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
        printf("Found overall path - %fs\n", (double)(clock() - time) / CLOCKS_PER_SEC);

        max_memory = get_memory_usage(max_memory);
        printf("Max memory: %f MB\n", max_memory / (1024.0 * 1024.0));
        printf("Path length: %d\n", arrlen(result));

        for (size_t i = 0; i < WORKERS_SIZE; i++)
        {
            printf("Worker %d CPU time: %fs\n", i + 1, workers[i].cpu_time);
            printf("Worker %d Max memory: %f MB\n", i + 1, workers[i].max_memory_bytes / (1024.0 * 1024.0));
        }

        // result_visualize(NULL,
        //                  &(Result){.success = true,
        //                            .graph = graph,
        //                            .path = result,
        //                            .max_memory_bytes = max_memory,
        //                            .cpu_secs = (double)(clock() - time) / CLOCKS_PER_SEC,
        //                            .visited = NULL,
        //                            .workers = workers});

        free(workers);

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
