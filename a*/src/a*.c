#include "a*.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../common/mheap.h"
#include "../../common/result.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

#define EUCLIDEAN
// #define OCTILE
// #define MANHATTAN

#ifdef EUCLIDEAN
#define DISTANCE_FUNCTION vec2_distance_euclidean
#define NEIGHBOUR_COST ((i < 4) ? 5 : 7)
#endif
#ifdef OCTILE
#define DISTANCE_FUNCTION vec2_distance_chebyshev
#include <math.h>
#define NEIGHBOUR_COST ((i < 4) ? 5 : 7)
#endif
#ifdef MANHATTAN
#define DISTANCE_FUNCTION vec2_distance_manhattan
#define NEIGHBOUR_COST 1
#endif


typedef struct node_t
{
    Vec2 pos;
    uint16_t estimated_score;
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
    const clock_t begin = clock();

    heap frontier;
    heap_create(&frontier, map->w + map->h, frontier_compare);

    struct
    {
        Vec2 key;
        uint16_t value;
    }* score = NULL;
    hmdefault(score, UINT16_MAX);
    const Vec2 start_pos = (Vec2){sx, sy};
    hmput(score, start_pos, 0);

    struct
    {
        Vec2 key;
        uint16_t value;
    }* estimated_score = NULL;
    hmdefault(estimated_score, UINT16_MAX);

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
    start_node->estimated_score = DISTANCE_FUNCTION(sx, sy, gx, gy);

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
                true,
                (double)(clock() - begin) / CLOCKS_PER_SEC
            };
        }

        const Vec2 neighbours[] = {
            {(int16_t)(pos.x - 1), pos.y},
            {pos.x, (int16_t)(pos.y - 1)},
            {(int16_t)(pos.x + 1), pos.y},
            {pos.x, (int16_t)(pos.y + 1)},

            {(int16_t)(pos.x + 1), (int16_t)(pos.y + 1)},
            {(int16_t)(pos.x + 1), (int16_t)(pos.y - 1)},
            {(int16_t)(pos.x - 1), (int16_t)(pos.y + 1)},
            {(int16_t)(pos.x - 1), (int16_t)(pos.y - 1)},
        };

        for (int i = 0; i < 8; ++i)
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

            const uint16_t temp_score = hmget(score, pos) + NEIGHBOUR_COST;
            if (temp_score >= hmget(score, neighbour))
            {
                continue;
            }

            hmput(source, neighbour, pos);
            hmput(score, neighbour, temp_score);

            uint16_t estimate = temp_score + (uint16_t) DISTANCE_FUNCTION(neighbour.x, neighbour.y, gx, gy);

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

    return (Result){
        closed, NULL, false,
        (double)(clock() - begin) / CLOCKS_PER_SEC
    };
}
