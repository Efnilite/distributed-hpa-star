#include "result.h"
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
        GraphNode* node = result->graph->nodes;
        while (node != NULL)
        {
            // if (text[XY_TO_IDX(node->pos.x, node->pos.y)] == '@')
            // {
            //     perror("Graph replaced wall");
            //     free(text);
            //     exit(EXIT_FAILURE);
            //     return;
            // }

            text[XY_TO_IDX(node->pos.x, node->pos.y)] = '~';

            node = node->next;
        }
    }

    float cost = 0.0f;
    if (result->path != NULL)
    {
        Vec2 previous = result->path[0];
        for (int i = 0; i < arrlen(result->path); ++i)
        {
            const Vec2 pos = result->path[i];
            if (text[XY_TO_IDX(pos.x, pos.y)] == '@')
            {
                perror("Pathfinding replaced wall");
                free(text);
                exit(EXIT_FAILURE);
                return;
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
