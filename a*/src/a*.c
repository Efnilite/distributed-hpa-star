#include "a*.h"

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../common/mheap.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

typedef struct node_t
{
    Vec2 pos;
    float estimated_score;
} Node;

static int frontier_compare(const Node* a, const Node* b)
{
    if (a->estimated_score > b->estimated_score)
    {
        return -1;
    }
    return 1;
}

Vec2* astar(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    heap frontier;
    heap_create(&frontier, 1, (void*)frontier_compare);

    struct { Vec2 key; float value; } *score = NULL;
    hmdefault(score, FLT_MAX);
    const Vec2 start_pos = (Vec2){sx, sy};
    hmput(score, start_pos, vec2_distance_a(sx, sy, gx, gy));

    struct { Vec2 key; float value; } *estimated_score = NULL;
    hmdefault(estimated_score, FLT_MAX);

    struct { Vec2 key; bool value; } *been_in_frontier = NULL;
    hmdefault(been_in_frontier, false);

    struct { Vec2 key; Vec2 value; } *source = NULL;
    const Vec2 start_vec = (Vec2){-1, -1};
    hmdefault(source, start_vec);

    Node start_node = {
        start_pos,
        vec2_distance_a(sx, sy, gx, gy)
    };

    int start_score = INT_MAX;
    heap_insert(&frontier, &start_node, &start_score);

    while (heap_size(&frontier) > 0)
    {
        Node* current = NULL;
        float* _ = NULL;
        if (heap_delmin(&frontier, (void**)&current, (void**)&_) == 0 || current == NULL)
        {
            break;
        }

        const Vec2 pos = current->pos;
        printf("(%d, %d)\n", pos.x, pos.y);
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
            free(current);

            return path;
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

            const float temp_score = hmget(score, pos) + 1;

            if (temp_score >= hmget(score, neighbour))
            {
                continue;
            }

            if (map_is_wall(map, neighbour.x, neighbour.y))
            {
                continue;
            }

            hmput(source, neighbour, pos);
            hmput(score, neighbour, temp_score);

            float estimate = temp_score + vec2_distance_a(neighbour.x, neighbour.y, gx, gy);

            if (hmget(been_in_frontier, pos))
            {
                continue;
            }

            Node* node = malloc(sizeof(Node));
            node->pos = neighbour;
            node->estimated_score = estimate;

            heap_insert(&frontier, node, &estimate);
            hmput(been_in_frontier, pos, true);
        }
    }

    heap_destroy(&frontier);

    return NULL;
}
