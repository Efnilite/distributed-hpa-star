#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../../common/tcp.h"
#include "../../common/hpa.h"
#include "worker_a.h"

#define STB_DS_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include "../../../common/stb_ds.h"

static volatile int running = 1;

void signal_handler(int sig)
{
    running = 0;
    printf("\nShutdown signal received\n");
}

void cluster_free(WorkerCluster* cluster) { vbitset_free(cluster->bits); }

int main(int argc, char const* argv[])
{
    signal(SIGINT, signal_handler);

    // setup

    WorkerCluster* clusters;
    // todo init clusters

    // connection

    const char* master_host = "127.0.0.1";
    uint16_t master_port = 9090;

    // Connect to master server
    tcp_client* client = tcp_client_create(master_host, master_port);
    if (!client)
    {
        fprintf(stderr, "Failed to connect to master at %s:%u\n", master_host, master_port);
        return 1;
    }

    printf("Worker process connected to master at %s:%u\n", master_host, master_port);

    // Worker main loop - receive tasks and send responses
    while (running && tcp_client_is_connected(client))
    {
        TaskRequest* task = NULL;
        int socket_fd = tcp_client_get_socket_fd(client);

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
        WorkerCluster* cluster = NULL; // todo search clusters

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
            continue;
        }

        TaskResponse response = {.task_id = task->task_id,
                                 .path_length = arrlen(result),
                                 .path = result,
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

    tcp_client_destroy(&client);
    printf("Worker shutdown complete\n");
    return 0;
}
