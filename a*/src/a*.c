#include "a*.h"

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../common/mheap.h"
#include "../../common/result.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

typedef struct node_t
{
    Vec2 pos;
    float estimated_score;
} Node;

static int frontier_compare(void* a, void* b)
{
    const Node* node_a = a;
    const Node* node_b = b;
    if (node_a->estimated_score < node_b->estimated_score)
    {
        return -1;
    }
    if (node_a->estimated_score > node_b->estimated_score)
    {
        return 1;
    }
    return 0;
}

Result astar(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    heap frontier;
    heap_create(&frontier, map->w + map->h, frontier_compare);

    struct
    {
        Vec2 key;
        float value;
    }* score = NULL;
    hmdefault(score, FLT_MAX);
    const Vec2 start_pos = (Vec2){sx, sy};
    hmput(score, start_pos, 0);

    struct
    {
        Vec2 key;
        float value;
    }* estimated_score = NULL;
    hmdefault(estimated_score, FLT_MAX);

    struct
    {
        Vec2 key;
        bool value;
    }* closed = NULL;
    hmdefault(closed, false);

    struct
    {
        Vec2 key;
        Vec2 value;
    }* source = NULL;
    const Vec2 start_vec = (Vec2){-1, -1};
    hmdefault(source, start_vec);

    Node* start_node = malloc(sizeof(Node));
    if (start_node == NULL)
    {
        perror("Failed node malloc");
        exit(EXIT_FAILURE);
    }
    start_node->pos = start_pos;
    start_node->estimated_score = vec2_distance_a(sx, sy, gx, gy);

    float start_estimate = start_node->estimated_score;
    heap_insert(&frontier, start_node, &start_estimate);

    while (heap_size(&frontier) > 0)
    {
        Node* current = NULL;
        float* _ = NULL;
        if (heap_delmin(&frontier, (void**)&current, (void**)&_) == 0 || current == NULL)
        {
            break;
        }

        const Vec2 pos = current->pos;

        if (hmget(closed, pos))
        {
            free(current);
            continue;
        }

        hmput(closed, pos, true);

        if (pos.x == gx && pos.y == gy)
        {
            Vec2* path = NULL;
            Vec2 node_pos = pos;
            arrput(path, pos);
            while (true)
            {
                arrput(path, node_pos);
                node_pos = hmget(source, node_pos);
                if (node_pos.x == -1 || node_pos.y == -1)
                {
                    break;
                }
            }

            Node* temp_node;
            while (heap_delmin(&frontier, (void**)&temp_node, (void**)&_) != 0 && temp_node != NULL)
            {
                free(temp_node);
            }
            heap_destroy(&frontier);
            hmfree(score);
            hmfree(estimated_score);
            // hmfree(closed);
            hmfree(source);
            free(current);

            return (Result){
                closed,
                path,
                true
            };
        }

        const Vec2 neighbours[] = {
            {(int16_t)(pos.x - 1), pos.y},
            {pos.x, (int16_t)(pos.y - 1)},
            {(int16_t)(pos.x + 1), pos.y},
            {pos.x, (int16_t)(pos.y + 1)},
        };

        for (int i = 0; i < 4; ++i)
        {
            const Vec2 neighbour = neighbours[i];

            if (hmget(closed, neighbour))
            {
                continue;
            }

            if (map_is_wall(map, neighbour.x, neighbour.y))
            {
                continue;
            }

            const float temp_score = hmget(score, pos) + 1;

            if (temp_score >= hmget(score, neighbour))
            {
                continue;
            }

            hmput(source, neighbour, pos);
            hmput(score, neighbour, temp_score);

            float estimate = temp_score + vec2_distance_a(neighbour.x, neighbour.y, gx, gy);

            Node* node = malloc(sizeof(Node));
            if (node == NULL)
            {
                perror("Failed node malloc");
                exit(EXIT_FAILURE);
            }
            node->pos = neighbour;
            node->estimated_score = estimate;

            heap_insert(&frontier, node, &estimate);
        }

        free(current);
    }

    heap_destroy(&frontier);
    hmfree(score);
    hmfree(estimated_score);
    // hmfree(closed);
    hmfree(source);

    return (Result){closed, NULL, false};
}
