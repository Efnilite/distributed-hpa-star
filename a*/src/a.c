#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "../../common/mheap.h"
#include "../../common/result.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

#include "a.h"

typedef struct frontier_node_t
{
    Vec2 pos;
    uint16_t estimated_score;
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

    FrontierNode* start = malloc(sizeof(FrontierNode));
    *start = (FrontierNode){
        .pos = {sx, sy},
        .estimated_score = (uint16_t)vec2_distance_euclidean(sx, sy, gx, gy),
    };
    heap_insert(&frontier, start, &start->estimated_score);

    struct closed_t
    {
        Vec2 key;
        bool is_closed;
        uint16_t estimated_score;
    };
    // struct closed_t closed[map->w * map->h];

    struct closed_t* closed = NULL;
    {
        const struct closed_t def = (struct closed_t){0, 0, false, 0};
        hmdefaults(closed, def);
    }

    struct
    {
        Vec2 key;
        uint16_t value;
    }* scores = NULL;
    hmdefault(scores, UINT16_MAX);
    hmput(scores, ((Vec2){sx, sy}), 0);

    struct
    {
        Vec2 key;
        Vec2 value;
    }* came_from = NULL;

    while (heap_size(&frontier) > 0)
    {
        FrontierNode* n = NULL;
        float* n_score = NULL;

        if (!heap_delmin(&frontier, (void**)&n, (void**)&n_score) || n == NULL || n_score == NULL)
        {
            continue;
        }

        const Vec2 pos = n->pos;

        if (pos.x == gx && pos.y == gy)
        {
            // reconstruct path
            Vec2* path = NULL;
            Vec2 current = pos;
            const Vec2 start_pos = (Vec2){sx, sy};
            arrput(path, current);
            while (current.x != start_pos.x || current.y != start_pos.y)
            {
                current = hmget(came_from, current);
                arrput(path, current);
            }

            heap_destroy(&frontier);
            hmfree(closed);
            hmfree(scores);
            hmfree(came_from);

            return (Result){
                NULL, path, true,
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

        const uint16_t score = hmget(scores, pos);
        for (int i = 0; i < 8; ++i)
        {
            const Vec2 successor = successors[i];

            if (map_is_wall(map, successor.x, successor.y))
            {
                continue;
            }

            const uint16_t gn = score + (i < 4 ? 5 : 7);
            const uint16_t hn = (uint16_t)vec2_distance_euclidean(successor.x, successor.y, gx, gy);
            const uint16_t fn = gn + hn;

            const struct closed_t closed_data = hmgets(closed, successor);
            if (closed_data.is_closed && fn >= closed_data.estimated_score)
            {
                continue;
            }

            // only update if we found a better g-score
            const uint16_t old_g = hmget(scores, successor);
            if (gn >= old_g)
            {
                continue;
            }

            // update g-score and came_from
            hmput(scores, successor, gn);
            hmput(came_from, successor, pos);

            FrontierNode* node = malloc(sizeof(FrontierNode));
            if (node == NULL)
            {
                perror("Failed node malloc");
                exit(EXIT_FAILURE);
            }
            node->pos = successor;
            node->estimated_score = fn;

            heap_insert(&frontier, node, &node->estimated_score);
        }

        free(n);
    }

    heap_destroy(&frontier);
    hmfree(closed);
    hmfree(scores);
    hmfree(came_from);

    return (Result){
        NULL, NULL, false,
        (double)(clock() - begin) / CLOCKS_PER_SEC
    };
}
