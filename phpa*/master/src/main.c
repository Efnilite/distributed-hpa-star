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

#define STB_DS_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include "../../../common/stb_ds.h"

static volatile bool running = true;

void signal_handler(int sig)
{
    running = false;
    printf("\nShutdown signal received\n");
}

void divide_clusters(int* clusters_per_worker, int connections_count)
{
    const Map map = parse_map("../data/sparse/scene_mp_2p_01");

    const size_t cluster_w = (size_t)ceil(map.w / (float)CLUSTER_SIZE);
    const size_t cluster_h = (size_t)ceil(map.h / (float)CLUSTER_SIZE);
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
        printf("Worker %d is getting %d clusters: %d-%d\n", worker, clusters_per_worker[worker], remaining_clusters,
               old);
    }
}

void send_cluster_assignments(int* worker_fds, int workers_count, int* clusters_per_worker)
{
    const Map map = parse_map("../data/sparse/scene_mp_2p_01");
    const size_t cluster_w = (size_t)ceil(map.w / (float)CLUSTER_SIZE);
    const size_t cluster_h = (size_t)ceil(map.h / (float)CLUSTER_SIZE);
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
            int clusters_per_worker[WORKERS_SIZE];
            divide_clusters(clusters_per_worker, WORKERS_SIZE);

            // Send cluster assignments to all workers
            send_cluster_assignments(worker_fds, WORKERS_SIZE, clusters_per_worker);
            break;
        }
    }


    Graph* graph;
    Cluster* clusters;
    preprocess(graph, clusters);

    // Close server from accepting more connections
    tcp_server_destroy(&server);

    // Main worker communication loop
    printf("Master entering main communication loop with %d workers\n", (int)arrlen(worker_fds));

    while (running && arrlen(worker_fds) > 0)
    {
        printf("Sending example tasks to workers...\n");

        // Example: Send tasks to all connected workers
        for (int i = 0; i < arrlen(worker_fds); i++)
        {
            TaskRequest task = {.task_id = i + 1,
                                .start_x = (float)(i * 5),
                                .start_y = 0.0f,
                                .goal_x = (float)(i * 5 + 10),
                                .goal_y = 10.0f,
                                .max_iterations = 10000};

            if (tcp_send_task_request(worker_fds[i], &task) < 0)
            {
                fprintf(stderr, "Failed to send task request to worker %d\n", i);
                close(worker_fds[i]);
                arrdel(worker_fds, i);
                i--;
                continue;
            }

            printf("Sent task request to worker %d (id=%u)\n", i, task.task_id);

            // Receive response from worker
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
                    printf("(%.2f, %.2f) ", response->path[j * 2], response->path[j * 2 + 1]);
                }
                if (response->path_length > 5)
                    printf("...");
                printf("\n");
            }

            // Cleanup
            tcp_taskresponse_free(&response);
        }

        // Exit after one round of tasks for demo purposes
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
