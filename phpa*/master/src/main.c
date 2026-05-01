#include "../../common/tcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static volatile int running = 1;

void signal_handler(int sig)
{
    running = 0;
    printf("\nShutdown signal received\n");
}

int main(int argc, char const *argv[])
{
    signal(SIGINT, signal_handler);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [host]\n", argv[0]);
        fprintf(stderr, "Example: %s 9090 127.0.0.1\n", argv[0]);
        return 1;
    }

    uint16_t port = atoi(argv[1]);
    const char *host = (argc > 2) ? argv[2] : "127.0.0.1";

    // Create TCP server
    tcp_server *server = tcp_server_create(host, port, 10);
    if (!server) {
        fprintf(stderr, "Failed to create TCP server\n");
        return 1;
    }

    printf("Master process started, waiting for worker connections on %s:%u\n", host, port);

    // Main server loop
    while (running) {
        // Accept worker connection
        int client_fd = tcp_server_accept(server);
        if (client_fd < 0) {
            if (running) {
                fprintf(stderr, "Failed to accept connection\n");
            }
            continue;
        }

        printf("Worker connected (fd=%d)\n", client_fd);

        // Example: Create and send a task to the worker
        TaskRequest task = {
            .task_id = 1,
            .start_x = 0.0f,
            .start_y = 0.0f,
            .goal_x = 10.0f,
            .goal_y = 10.0f,
            .max_iterations = 10000
        };

        if (tcp_send_task_request(client_fd, &task) < 0) {
            fprintf(stderr, "Failed to send task request\n");
            close(client_fd);
            continue;
        }

        printf("Sent task request (id=%u) to worker\n", task.task_id);

        // Receive response from worker
        TaskResponse *response = NULL;
        if (tcp_recv_task_response(client_fd, &response) < 0) {
            fprintf(stderr, "Failed to receive task response\n");
            close(client_fd);
            continue;
        }

        printf("Received task response (id=%u, path_length=%u, status=%d)\n",
               response->task_id, response->path_length, response->status_code);

        if (response->path_length > 0 && response->path) {
            printf("Path waypoints: ");
            for (uint32_t i = 0; i < response->path_length && i < 5; i++) {
                printf("(%.2f, %.2f) ", response->path[i*2], response->path[i*2+1]);
            }
            if (response->path_length > 5) printf("...");
            printf("\n");
        }

        // Cleanup
        tcp_taskresponse_free(&response);
        close(client_fd);
    }

    tcp_server_destroy(&server);
    printf("Master shutdown complete\n");
    return 0;
}
