#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "a.h"
#include "../../common/mheap.h"
#include "../../common/result.h"
#include "../../common/stb_ds.h"
#include "../../common/util.h"
#include "../../common/vec2.h"

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

#define SUCCESSORS(x, y) \
            { \
                {(int16_t)(x - 1), y}, \
                {x, (int16_t)(y - 1)}, \
                {(int16_t)(x + 1), y}, \
                {x, (int16_t)(y + 1)}, \
                \
                {(int16_t)(x + 1), (int16_t)(y + 1)}, \
                {(int16_t)(x + 1), (int16_t)(y - 1)}, \
                {(int16_t)(x - 1), (int16_t)(y + 1)}, \
                {(int16_t)(x - 1), (int16_t)(y - 1)},\
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

Result astar(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    const clock_t begin = clock();
    const size_t size = map->w * map->h;

    heap frontier;
    heap_create(&frontier, map->w + map->h, frontier_compare);

    FrontierNode* start = malloc(sizeof(FrontierNode));
    *start = (FrontierNode){
        .pos = {sx, sy},
        .estimated_score = (uint16_t)DISTANCE_FUNCTION(sx, sy, gx, gy),
    };
    heap_insert(&frontier, start, &start->estimated_score);

    bool* closed = calloc(size, sizeof(bool));

    uint16_t* scores = malloc(sizeof(uint16_t) * size);
    memset(scores, UINT16_MAX, sizeof(uint16_t) * size);
    scores[XY_TO_IDX(sx, sy)] = 0;

    uint8_t* came_from = calloc(size, sizeof(uint8_t));

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
        if (pos.x == gx && pos.y == gy)
        {
            // reconstruct path
            Vec2* path = NULL;
            Vec2 current = pos;
            const Vec2 start_pos = (Vec2){sx, sy};
            arrput(path, current);
            while (current.x != start_pos.x || current.y != start_pos.y)
            {
                const Vec2 predecessors[] = SUCCESSORS(current.x, current.y);
                const uint8_t came_from_idx = came_from[XY_TO_IDX(current.x, current.y)];
                const Vec2 reverse = predecessors[came_from_idx];
                current = (Vec2){
                    (int16_t)(current.x + (current.x - reverse.x)),
                    (int16_t)(current.y + (current.y - reverse.y))
                };
                arrput(path, current);
            }

            free(n);
            heap_destroy(&frontier);
            free(scores);
            free(came_from);
            free(closed);

            return (Result){
                NULL, path, true,
                (double)(clock() - begin) / CLOCKS_PER_SEC
            };
        }

        closed[pos_idx] = true;

        const Vec2 successors[] = SUCCESSORS(pos.x, pos.y);
        const uint16_t score = scores[pos_idx];
        for (int i = 0; i < 8; ++i)
        {
            const Vec2 successor = successors[i];
            const uint32_t successor_idx = XY_TO_IDX(successor.x, successor.y);

            if (map_is_wall(map, successor.x, successor.y))
            {
                continue;
            }

            const uint16_t gn = score + NEIGHBOUR_COST;
            const uint16_t hn = (uint16_t)DISTANCE_FUNCTION(successor.x, successor.y, gx, gy);
            const uint16_t fn = gn + hn;

            if (closed[successor_idx])
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
            came_from[successor_idx] = i;

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
    free(closed);
    free(scores);
    free(came_from);

    return (Result){
        NULL, NULL, false,
        (double)(clock() - begin) / CLOCKS_PER_SEC
    };
}
