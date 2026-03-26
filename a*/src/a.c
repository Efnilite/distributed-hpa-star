#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../common/mheap.h"
#include "../../common/result.h"
#include "../../common/stb_ds.h"
#include "../../common/util.h"
#include "../../common/vec2.h"
#include "a.h"

#include <assert.h>

// #define EUCLIDEAN
#define OCTILE
// #define MANHATTAN

#ifdef EUCLIDEAN
#define DISTANCE_FUNCTION vec2_distance_euclidean
#define NEIGHBOUR_COST ((i < 4) ? 5 : 7)
#endif
#ifdef OCTILE
#define DISTANCE_FUNCTION vec2_distance_chebyshev
#define NEIGHBOUR_COST ((i < 4) ? 5 : 7)
#endif
#ifdef MANHATTAN
#define DISTANCE_FUNCTION vec2_distance_manhattan
#define NEIGHBOUR_COST 1
#endif

#define SUCCESSORS(x, y)                                                                                               \
    {                                                                                                                  \
        {(int16_t)(x - 1), y},                                                                                         \
        {x, (int16_t)(y - 1)},                                                                                         \
        {(int16_t)(x + 1), y},                                                                                         \
        {x, (int16_t)(y + 1)},                                                                                         \
                                                                                                                       \
        {(int16_t)(x + 1), (int16_t)(y + 1)},                                                                          \
        {(int16_t)(x + 1), (int16_t)(y - 1)},                                                                          \
        {(int16_t)(x - 1), (int16_t)(y + 1)},                                                                          \
        {(int16_t)(x - 1), (int16_t)(y - 1)},                                                                          \
    }

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

Result a(const Map* map, const Vec2 start, const Vec2 goal)
{
    assert(start.x > 0 && start.x < map->w && start.y > 0 && start.y < map->h);
    assert(goal.x > 0 && goal.x < map->w && goal.y > 0 && goal.y < map->h);

    const clock_t begin = clock();
    const size_t size = map->w * map->h;

    heap frontier;
    heap_create(&frontier, map->w + map->h, frontier_compare);

    FrontierNode* startn = malloc(sizeof(FrontierNode));
    *startn = (FrontierNode){
        .pos = start,
        .estimated_score = (uint16_t)DISTANCE_FUNCTION(start, goal),
    };
    heap_insert(&frontier, startn, &startn->estimated_score);

    VBitSet* closed = vbitset_create(size, 1);
    VBitSet* came_from = vbitset_create(size, 3);

    uint16_t* scores = malloc(sizeof(uint16_t) * size);
    memset(scores, UINT16_MAX, sizeof(uint16_t) * size);
    scores[XY_TO_IDX(start.x, start.y)] = 0;

    while (heap_size(&frontier) > 0)
    {
        FrontierNode* n = NULL;
        uint16_t* _n_score = NULL;

        if (!heap_delmin(&frontier, (void**)&n, (void**)&_n_score) || n == NULL || _n_score == NULL)
        {
            continue;
        }

        const Vec2 pos = n->pos;
        const uint32_t pos_idx = XY_TO_IDX(pos.x, pos.y);
        if (vec2_equal(pos, goal))
        {
            // reconstruct path
            Vec2* path = NULL;
            Vec2 current = pos;
            const Vec2 start_pos = start;
            arrput(path, current);
            while (current.x != start_pos.x || current.y != start_pos.y)
            {
                const Vec2 predecessors[] = SUCCESSORS(current.x, current.y);
                const uint8_t came_from_idx = vbitset_get(came_from, XY_TO_IDX(current.x, current.y));

                const Vec2 reverse = predecessors[came_from_idx];
                current = (Vec2){(int16_t)(current.x + (current.x - reverse.x)),
                                 (int16_t)(current.y + (current.y - reverse.y))};
                arrput(path, current);
            }

            free(n);
            heap_destroy(&frontier);
            free(scores);
            vbitset_free(came_from);
            vbitset_free(closed);

            return (Result){NULL, path, true, (double)(clock() - begin) / CLOCKS_PER_SEC};
        }

        vbitset_set(closed, pos_idx, true);

        const Vec2 successors[] = SUCCESSORS(pos.x, pos.y);
        const uint16_t score = scores[pos_idx];
        for (uint8_t i = 0; i < 8; ++i)
        {
            const Vec2 successor = successors[i];
            const uint32_t successor_idx = XY_TO_IDX(successor.x, successor.y);

            if (map_is_wall(map, successor.x, successor.y))
            {
                continue;
            }

            const uint16_t gn = score + NEIGHBOUR_COST;
            const uint16_t hn = (uint16_t)DISTANCE_FUNCTION(successor, goal);
            const uint16_t fn = gn + hn;

            if (vbitset_get(closed, successor_idx))
            {
                continue;
            }

            // only update if we found a better g-score
            const uint16_t old_g = scores[successor_idx];
            if (gn >= old_g)
            {
                continue;
            }

            // update g-score and came_from
            scores[successor_idx] = gn;
            vbitset_set(came_from, successor_idx, i);

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
    vbitset_free(closed);
    free(scores);
    vbitset_free(came_from);

    return (Result){NULL, NULL, false, (double)(clock() - begin) / CLOCKS_PER_SEC};
}
