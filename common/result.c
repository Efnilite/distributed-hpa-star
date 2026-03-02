#include "result.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "stb_ds.h"

#define index (x + (y) * map->w)

void result_visualize(const Map* map, const Result* result)
{
    char text[map->w * map->h];
    for (int y = 0; y < map->w; ++y)
    {
        for (int x = 0; x < map->w; ++x)
        {
            if (map->coordinates[x + y * map->w])
            {
                text[x + y * map->w] = '@';
            }
            else
            {
                text[x + y * map->w] = ' ';
            }
        }
    }

    if (result->visited != NULL)
    {
        for (int i = 0; i < hmlen(result->visited); ++i)
        {
            const Vec2 pos = result->visited[i].key;
            text[pos.x + pos.y * map->w] = 'o';
        }
    }

    if (result->path != NULL)
    {
        for (int i = 0; i < arrlen(result->path); ++i)
        {
            const Vec2 pos = result->path[i];
            text[pos.x + pos.y * map->w] = '.';
        }
    }

    char filename[50];
    if (snprintf(filename, 50, "result-%ld", time(NULL)) < 0)
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

    fprintf(file, "Length: %ld\n", arrlen(result->path));
    fprintf(file, "Visited: %f%%\n", hmlen(result->visited) / (map->w * map->h * 1.0) * 100.0);
    fprintf(file, "CPU Time: %f secs\n", result->cpu_secs);
    for (int y = 0; y < map->h; ++y)
    {
        char line[map->w];
        strncpy(line, text + y * map->w, map->w);
        line[map->w] = '\0';
        fprintf(file, "%s\n", line);
    }

    fflush(file);
    fclose(file);
}
