#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../../common/map.h"
#include "../../../common/parser.h"
#include "../../../common/tcp.h"
#include "../../common/hpa.h"
#include "worker_a.h"

#define STB_DS_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include "../../../common/stb_ds.h"

static volatile bool running = true;

void signal_handler(int sig)
{
    running = false;
    printf("\nShutdown signal received\n");
}

void cluster_free(WorkerCluster* cluster)
{
    if (cluster && cluster->bits)
    {
        vbitset_free(cluster->bits);
        cluster->bits = NULL;
    }
}

/**
 * Create a WorkerCluster from the global map data
 * Extracts the obstacle bitset for the specified cluster
 */
WorkerCluster* worker_cluster_create(const Map* map, int16_t cluster_x, int16_t cluster_y)
{
    WorkerCluster* cluster = (WorkerCluster*)malloc(sizeof(WorkerCluster));
    if (!cluster)
    {
        perror("malloc");
        return NULL;
    }

    cluster->pos = (Vec2){cluster_x, cluster_y};

    // Create a bitset for this cluster's obstacle map
    size_t cluster_area = CLUSTER_SIZE * CLUSTER_SIZE;
    cluster->bits = vbitset_create(cluster_area, 1); // 1 bit per cell (0=free, 1=wall)
    if (!cluster->bits)
    {
        fprintf(stderr, "Failed to create bitset for cluster (%d, %d)\n", cluster_x, cluster_y);
        free(cluster);
        return NULL;
    }

    // Extract cluster-specific obstacle data from the global map
    uint16_t cluster_start_x = cluster_x * CLUSTER_SIZE;
    uint16_t cluster_start_y = cluster_y * CLUSTER_SIZE;

    for (uint16_t local_y = 0; local_y < CLUSTER_SIZE; local_y++)
    {
        for (uint16_t local_x = 0; local_x < CLUSTER_SIZE; local_x++)
        {
            uint16_t global_x = cluster_start_x + local_x;
            uint16_t global_y = cluster_start_y + local_y;

            // Check if position is within map bounds
            if (global_x < map->w && global_y < map->h)
            {
                // Copy wall data from global map to cluster bits
                if (map_is_wall(map, global_x, global_y))
                {
                    size_t idx = xy_to_idx_cluster_a(local_x, local_y);
                    vbitset_set(cluster->bits, idx, 1);
                }
            }
            else
            {
                // Out of bounds cells are treated as walls
                size_t idx = xy_to_idx_cluster_a(local_x, local_y);
                vbitset_set(cluster->bits, idx, 1);
            }
        }
    }

    return cluster;
}

/**
 * Find a cluster by position from the assigned cluster list
 */
WorkerCluster* find_cluster_by_position(WorkerCluster* clusters, uint32_t cluster_count, int16_t cluster_x, int16_t cluster_y)
{
    for (uint32_t i = 0; i < cluster_count; i++)
    {
        if (clusters[i].pos.x == cluster_x && clusters[i].pos.y == cluster_y)
        {
            return &clusters[i];
        }
    }
    return NULL;
}

/**
 * Free all clusters
 */
void clusters_free(WorkerCluster* clusters, uint32_t count)
{
    if (!clusters)
        return;
    for (uint32_t i = 0; i < count; i++)
    {
        cluster_free(&clusters[i]);
    }
    free(clusters);
}

int main(int argc, char const* argv[])
{
    signal(SIGINT, signal_handler);

    // Load map once at startup
    Map map = parse_map("../data/sparse/scene_mp_2p_01");
    printf("Loaded map: %u x %u\n", map.w, map.h);

    // Worker clusters storage
    WorkerCluster* clusters = NULL;
    uint32_t cluster_count = 0;
    int clusters_initialized = 0;

    // connection

    const char* master_host = "127.0.0.1";
    uint16_t master_port = 9090;

    // Connect to master server
    tcp_client* client = tcp_client_create(master_host, master_port);
    if (!client)
    {
        fprintf(stderr, "Failed to connect to master at %s:%u\n", master_host, master_port);
        map_free(&map);
        return 1;
    }

    printf("Worker process connected to master at %s:%u\n", master_host, master_port);

    // Worker main loop - receive tasks and send responses
    while (running && tcp_client_is_connected(client))
    {
        ClusterAssignment* ca = NULL;
        TaskRequest* task = NULL;
        int socket_fd = tcp_client_get_socket_fd(client);

        // Wait for cluster assignment from master (only once)
        if (!clusters_initialized)
        {
            if (tcp_recv_cluster_assignment(socket_fd, &ca) < 0)
            {
                fprintf(stderr, "Waiting to receive cluster assignment\n");
                if (running)
                {
                    sleep(1);
                    continue;
                }
                else
                {
                    break;
                }
            }

            printf("Received cluster assignment: count=%u\n", ca->count);

            // Initialize clusters from the assignment
            cluster_count = ca->count;
            clusters = (WorkerCluster*)malloc(cluster_count * sizeof(WorkerCluster));
            if (!clusters)
            {
                fprintf(stderr, "Failed to allocate memory for clusters\n");
                tcp_clusterassignment_free(&ca);
                break;
            }

            for (uint32_t i = 0; i < cluster_count; i++)
            {
                int16_t cluster_x = ca->positions[i * 2];
                int16_t cluster_y = ca->positions[i * 2 + 1];

                WorkerCluster* new_cluster = worker_cluster_create(&map, cluster_x, cluster_y);
                if (!new_cluster)
                {
                    fprintf(stderr, "Failed to create cluster (%d, %d)\n", cluster_x, cluster_y);
                    clusters_free(clusters, i);
                    tcp_clusterassignment_free(&ca);
                    break;
                }
                clusters[i] = *new_cluster;
                free(new_cluster);
                printf("Initialized cluster %u: pos=(%d, %d)\n", i, cluster_x, cluster_y);
            }

            tcp_clusterassignment_free(&ca);
            clusters_initialized = 1;
            map_free(&map);
        }

        // Wait for task from master
        if (tcp_recv_task_request(socket_fd, &task) < 0)
        {
            fprintf(stderr, "Waiting to receive task request\n");
            if (running)
            {
                sleep(1);
                continue;
            }
            else
            {
                break;
            }
        }

        printf("Received task (id=%u): start=(%.2f, %.2f) goal=(%.2f, %.2f)\n", task->task_id, task->start_x,
               task->start_y, task->goal_x, task->goal_y);

        Vec2 start = (Vec2){task->start_x, task->start_y};
        Vec2 goal = (Vec2){task->goal_x, task->goal_y};
        Vec2 cluster_pos = global_vec_to_cluster_pos(start);
        
        // Find the correct cluster for this task
        WorkerCluster* cluster = find_cluster_by_position(clusters, cluster_count, (int16_t)cluster_pos.x, (int16_t)cluster_pos.y);
        if (!cluster)
        {
            fprintf(stderr, "ERROR: Cluster (%d, %d) not assigned to this worker\n", (int16_t)cluster_pos.x, (int16_t)cluster_pos.y);
            TaskResponse response = {
                .task_id = task->task_id, .path_length = 0, .path = NULL, .iterations_used = 0, .status_code = 2}; // status_code=2 for cluster not found
            if (tcp_send_task_response(socket_fd, &response) < 0)
            {
                fprintf(stderr, "Failed to send error response\n");
            }
            tcp_taskrequest_free(&task);
            continue;
        }

        Vec2* result = worker_a(cluster, start, goal);
        if (result == NULL)
        {
            TaskResponse response = {
                .task_id = task->task_id, .path_length = 0, .path = NULL, .iterations_used = 0, .status_code = 1};
            if (tcp_send_task_response(socket_fd, &response) < 0)
            {
                fprintf(stderr, "Failed to send failed task response\n");
            }
            else
            {
                printf("Sent failed task response (id=%u, path_length=%u)\n", response.task_id, response.path_length);
            }
            tcp_taskrequest_free(&task);
            continue;
        }

        TaskResponse response = {.task_id = task->task_id,
                                 .path_length = arrlen(result),
                                 .path = (float*)result,
                                 .iterations_used = 0,
                                 .status_code = 0};

        // Send response back to master
        if (tcp_send_task_response(socket_fd, &response) < 0)
        {
            fprintf(stderr, "Failed to send task response\n");
        }
        else
        {
            printf("Sent task response (id=%u, path_length=%u)\n", response.task_id, response.path_length);
        }

        // Cleanup
        if (response.path)
        {
            free(response.path);
        }
        tcp_taskrequest_free(&task);
    }

    // Cleanup
    if (clusters)
    {
        clusters_free(clusters, cluster_count);
    }
    tcp_client_destroy(&client);
    map_free(&map);
    printf("Worker shutdown complete\n");
    return 0;
}
