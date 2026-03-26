#include "graph_a.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hpa.h"
#include "../../common/graph.h"
#include "../../common/mheap.h"
#include "../../common/stb_ds.h"
#include "../../common/util.h"

typedef struct frontier_node_t
{
    Vec2 pos;
    const GraphNode* node;
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

static void heap_free_elements(void* key, void* value)
{
    free(key);
}

Vec2* graph_a(const Map* map, const Graph* graph, const Vec2 start, const Vec2 goal)
{
    const size_t size = map->size;

    heap frontier;
    heap_create(&frontier, CLUSTER_SIZE, frontier_compare);

    FrontierNode* startn = malloc(sizeof(FrontierNode));
    *startn = (FrontierNode){
        .pos = start,
        .node = graph_find_node(graph, start),
        .estimated_score = (uint16_t)vec2_distance_chebyshev(start, goal),
    };
    heap_insert(&frontier, startn, &startn->estimated_score);

    bool* closed = malloc(sizeof(bool) * size);
    memset(closed, false, sizeof(bool) * size);

    Vec2* came_from = malloc(sizeof(Vec2) * size);
    memset(came_from, 0, sizeof(Vec2) * size);

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
        const GraphNode* node = n->node;
        if (vec2_equal(pos, goal))
        {
            // reconstruct path
            Vec2* path = NULL;
            Vec2 current = pos;
            while (current.x != start.x || current.y != start.y)
            {
                arrput(path, current);
                current = came_from[XY_TO_IDX(current.x, current.y)];
            }
            arrput(path, current);

            free(n);
            heap_foreach(&frontier, heap_free_elements);
            heap_destroy(&frontier);
            free(scores);
            free(came_from);
            free(closed);

            return path;
        }

        const size_t pos_idx = XY_TO_IDX(pos.x, pos.y);
        closed[pos_idx] = true;

        const uint16_t score = scores[pos_idx];
        const GraphEdge* to_successor = node->edges;
        while (to_successor != NULL)
        {
            const GraphNode* successor = to_successor->to;
            const size_t successor_idx = XY_TO_IDX(successor->pos.x, successor->pos.y);

            const uint16_t gn = score + to_successor->weight;
            const uint16_t hn = (uint16_t)vec2_distance_chebyshev(successor->pos, goal);
            const uint16_t fn = gn + hn;

            if (closed[successor_idx])
            {
                to_successor = to_successor->next;
                continue;
            }

            // only update if we found a better g-score
            const uint16_t old_g = scores[successor_idx];
            if (gn >= old_g)
            {
                to_successor = to_successor->next;
                continue;
            }

            // update g-score and came_from
            scores[successor_idx] = gn;
            came_from[successor_idx] = pos;

            FrontierNode* fnode = malloc(sizeof(FrontierNode));
            if (fnode == NULL)
            {
                perror("Failed node malloc");
                exit(EXIT_FAILURE);
            }
            fnode->pos = successor->pos;
            fnode->node = successor;
            fnode->estimated_score = fn;

            heap_insert(&frontier, fnode, &fnode->estimated_score);

            to_successor = to_successor->next;
        }

        free(n);
    }

    heap_foreach(&frontier, heap_free_elements);
    heap_destroy(&frontier);
    free(scores);
    free(came_from);
    free(closed);

    return NULL;
}
