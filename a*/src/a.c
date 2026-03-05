#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "../../common/mheap.h"
#include "../../common/result.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

#include "a*.h"

typedef struct frontier_node_t
{
    Vec2 pos;
    float estimated_score;
} FrontierNode;

static int frontier_compare(void* a, void* b)
{
    const FrontierNode* node_a = a;
    const FrontierNode* node_b = b;
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

    FrontierNode start = {
        .pos = {sx, sy},
        .estimated_score = vec2_distance_euclidean(sx, sy, gx, gy),
    };
    heap_insert(&frontier, &start, &start.estimated_score);

    struct closed_t
    {
        Vec2 key;
        bool is_closed;
        float estimated_score;
    };

    struct closed_t* closed = NULL;
    {
        const struct closed_t def = (struct closed_t){0, 0, false, 0};
        hmdefaults(closed, def);
    }

    struct
    {
        Vec2 key;
        float value;
    }* scores = NULL;
    hmdefault(scores, FLT_MAX);

    while (heap_size(&frontier) > 0)
    {
        FrontierNode* n = NULL;
        float* n_score = NULL;

        if (!heap_delmin(&frontier, &n, &n_score) || n == NULL || n_score == NULL)
        {
            continue;
        }

        const Vec2 pos = n->pos;
        if (pos.x == gx && pos.y == gy)
        {

            return (Result){
                NULL, NULL, true,
                (double)(clock() - begin) / CLOCKS_PER_SEC
            };
        }

        struct closed_t close_n = (struct closed_t){pos, true, *n_score};
        hmputs(closed, close_n);

        const Vec2 successors[] = {
            {(int16_t)(pos.x - 1), pos.y},
            {pos.x, (int16_t)(pos.y - 1)},
            {(int16_t)(pos.x + 1), pos.y},
            {pos.x, (int16_t)(pos.y + 1)},

            {(int16_t)(pos.x + 1), (int16_t)(pos.y + 1)},
            {(int16_t)(pos.x + 1), (int16_t)(pos.y - 1)},
            {(int16_t)(pos.x - 1), (int16_t)(pos.y + 1)},
            {(int16_t)(pos.x - 1), (int16_t)(pos.y - 1)},
        };

        const float score = hmget(scores, pos);
        for (int i = 0; i < 8; ++i)
        {
            const Vec2 successor = successors[i];

            if (map_is_wall(map, successor.x, successor.y))
            {
                continue;
            }

            const float gn = score + (i < 4 ? 5.f : 7.f);
            const float hn = vec2_distance_euclidean(successor.x, successor.y, gx, gy);
            float fn = gn + hn;

            const struct closed_t closed_data = hmgets(closed, successor);
            if (closed_data.is_closed && fn >= closed_data.estimated_score)
            {
                continue;
            }

            // mark successor open
            FrontierNode* node = malloc(sizeof(FrontierNode));
            if (node == NULL)
            {
                perror("Failed node malloc");
                exit(EXIT_FAILURE);
            }
            node->pos = successor;
            node->estimated_score = fn;

            heap_insert(&frontier, node, &fn);
        }
    }

    return (Result){
        NULL, NULL, false,
        (double)(clock() - begin) / CLOCKS_PER_SEC
    };
}
