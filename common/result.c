#include "result.h"
#include "constants.h"
#include "graph.h"
#include "stb_ds.h"
#include "util.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

void result_visualize(const Map* map, const Result* result)
{
    const size_t size = (size_t)map->w * map->h;
    char* text = malloc(size);
    if (text == NULL)
    {
        perror("Failed to allocate visualization buffer");
        exit(EXIT_FAILURE);
    }

    for (uint16_t y = 0; y < map->h; ++y)
    {
        for (uint16_t x = 0; x < map->w; ++x)
        {
            text[XY_TO_IDX(x, y)] = map_is_wall(map, x, y) ? '@' : ' ';
        }
    }

    size_t visited = 0;
    if (result->visited != NULL)
    {
        for (int i = 0; i < size; ++i)
        {
            const ClosedNode m = result->visited[i];
            if (m.is_closed && text[i] != '@')
            {
                text[i] = 'o';
                visited++;
            }
        }
    }

    if (result->graph != NULL)
    {
        const GraphNode* node = result->graph->nodes;
        while (node != NULL)
        {
            if (text[XY_TO_IDX(node->pos.x, node->pos.y)] == '@')
            {
                fprintf(stderr, "Graph replaced wall at %d,%d", node->pos.x, node->pos.y);
            }

            text[XY_TO_IDX(node->pos.x, node->pos.y)] = '~';

            node = node->next;
        }
    }

    float cost = 0.0f;
    if (result->path != NULL && arrlen(result->path) > 0)
    {
        Vec2 previous = result->path[0];
        for (int i = 0; i < arrlen(result->path); ++i)
        {
            const Vec2 pos = result->path[i];
            if (text[XY_TO_IDX(pos.x, pos.y)] == '@')
            {
                fprintf(stderr, "Pathfinding replaced wall at %d,%d", pos.x, pos.y);
            }

            text[XY_TO_IDX(pos.x, pos.y)] = '*';
            if (previous.x != pos.x && previous.y != pos.y)
            {
                cost += M_SQRT2;
            }
            else
            {
                cost += 1.0f;
            }

            previous = pos;
        }
    }

    struct timeval time;
    gettimeofday(&time, NULL);
    char filename[50];
    if (snprintf(filename, 50, "result-%ld%d", time.tv_sec, getpid()) < 0)
    {
        perror("Failed to create filename");
        exit(EXIT_FAILURE);
    }

    FILE* file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Path Length: %ld\n", arrlen(result->path));
    fprintf(file, "Max Memory: %.2f MB\n", result->max_memory_bytes / (1024.0 * 1024.0));
    fprintf(file, "Worker Max Memory: %.2f MB\n", result->worker_max_memory_bytes / (1024.0 * 1024.0));
    fprintf(file, "Path Cost: %f\n", cost);
    if (result->visited != NULL)
    {
        fprintf(file, "Visited: %f%%\n", (double)visited / ((double)map->size * 1.0) * 100.0);
        free(result->visited);
    }

    if (result->graph != NULL)
    {
        fprintf(file, "L1 Nodes: %ld\n", graph_node_count(result->graph));

        graph_free(result->graph);
    }

    fprintf(file, "CPU Time: %f secs\n", result->cpu_secs);
    for (int y = 0; y < map->h; ++y)
    {
        fwrite(text + (size_t)y * map->w, 1, map->w, file);
        fputc('\n', file);
    }

    fflush(file);
    fclose(file);
    free(text);
}

#define XY_TO_IDX_C(x, y) ((x) + (y) * CLUSTER_SIZE)

static inline Vec2 global_vec_to_local_vec(Vec2 cluster_pos, Vec2 vec)
{
    return (Vec2){vec.x - cluster_pos.x * CLUSTER_SIZE, vec.y - cluster_pos.y * CLUSTER_SIZE};
}

void cluster_visualize(const VBitSet* coordinates, const Vec2 cluster_pos, const Vec2* path)
{
    const size_t size = coordinates->capacity;
    char* text = malloc(size);
    if (text == NULL)
    {
        perror("Failed to allocate visualization buffer");
        exit(EXIT_FAILURE);
    }

    for (size_t idx = 0; idx < size; ++idx)
    {
        text[idx] = vbitset_get(coordinates, idx) ? '@' : ' ';
    }

    float cost = 0.0f;
    if (path != NULL)
    {
        Vec2 previous = path[0];
        for (int i = 0; i < arrlen(path); ++i)
        {
            const Vec2 pos = global_vec_to_local_vec(cluster_pos, path[i]);
            if (text[XY_TO_IDX_C(pos.x, pos.y)] == '@')
            {
                fprintf(stderr, "Pathfinding replaced wall at %d,%d", pos.x, pos.y);
            }

            text[XY_TO_IDX_C(pos.x, pos.y)] = '*';
            if (previous.x != pos.x && previous.y != pos.y)
            {
                cost += M_SQRT2;
            }
            else
            {
                cost += 1.0f;
            }

            previous = pos;
        }
    }

    struct timeval time;
    gettimeofday(&time, NULL);
    char filename[50];
    if (snprintf(filename, 50, "result-%ld%d", time.tv_sec, getpid()) < 0)
    {
        perror("Failed to create filename");
        exit(EXIT_FAILURE);
    }

    FILE* file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    if (path)
    {
        fprintf(file, "Path Length: %ld\n", arrlen(path));
    }
    else
    {
        fprintf(file, "Path Length: 0\n");
    }
    fprintf(file, "Path Cost: %f\n", cost);
    for (int y = 0; y < CLUSTER_SIZE; ++y)
    {
        fwrite(text + (size_t)y * CLUSTER_SIZE, 1, CLUSTER_SIZE, file);
        fputc('\n', file);
    }

    fflush(file);
    fclose(file);
    free(text);
}
