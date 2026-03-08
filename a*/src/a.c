#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "a.h"
#include "../../common/mheap.h"
#include "../../common/result.h"
#include "../../common/stb_ds.h"
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

#define XY_TO_IDX(x, y) ((x) + (y) * map->w)
#define IDX_TO_XY(idx) (Vec2){((idx) % map->w), ((idx) / map->w)}

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

    ClosedNode* closed = calloc(size, sizeof(ClosedNode));

    uint16_t* scores = malloc(sizeof(uint16_t) * size);
    memset(scores, UINT16_MAX, sizeof(uint16_t) * size);
    scores[XY_TO_IDX(sx, sy)] = 0;

    uint32_t* came_from = calloc(size, sizeof(uint32_t));

    while (heap_size(&frontier) > 0)
    {
        FrontierNode* n = NULL;
        uint16_t* n_score = NULL;

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
                current = IDX_TO_XY(came_from[XY_TO_IDX(current.x, current.y)]);
                arrput(path, current);
            }

            free(n);
            heap_destroy(&frontier);
            free(scores);
            free(came_from);

            return (Result){
                closed, path, true,
                (double)(clock() - begin) / CLOCKS_PER_SEC
            };
        }

        const ClosedNode close_n = (ClosedNode){*n_score, true};
        closed[XY_TO_IDX(pos.x, pos.y)] = close_n;

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

        const uint16_t score = scores[XY_TO_IDX(pos.x, pos.y)];
        for (int i = 0; i < 8; ++i)
        {
            const Vec2 successor = successors[i];

            if (map_is_wall(map, successor.x, successor.y))
            {
                continue;
            }

            const uint16_t gn = score + NEIGHBOUR_COST;
            const uint16_t hn = (uint16_t)DISTANCE_FUNCTION(successor.x, successor.y, gx, gy);
            const uint16_t fn = gn + hn;

            const ClosedNode closed_data = closed[XY_TO_IDX(successor.x, successor.y)];
            if (closed_data.is_closed)
            {
                continue;
            }

            // only update if we found a better g-score
            const uint16_t old_g = scores[XY_TO_IDX(successor.x, successor.y)];
            if (gn >= old_g)
            {
                continue;
            }

            // update g-score and came_from
            scores[XY_TO_IDX(successor.x, successor.y)] = gn;
            came_from[XY_TO_IDX(successor.x, successor.y)] = XY_TO_IDX(pos.x, pos.y);

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
