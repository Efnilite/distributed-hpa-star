#include "a.h"
#include "../../common/graph.h"
#include "../../common/mheap.h"
#include "../../common/util.h"
#include "hpa.h"

#include <stdlib.h>
#include <time.h>

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

Vec2* graph_a(const Graph* graph, const Vec2 start, const Vec2 goal)
{
    const size_t size = graph->node_count;

    heap frontier;
    heap_create(&frontier, CLUSTER_SIZE, frontier_compare);

    FrontierNode* startn = malloc(sizeof(FrontierNode));
    *startn = (FrontierNode){
        .pos = start,
        .estimated_score = (uint16_t)vec2_distance_chebyshev(start, goal),
    };
    heap_insert(&frontier, startn, &startn->estimated_score);

    while (heap_size(&frontier) > 0)
    {
        FrontierNode* n = NULL;
        uint16_t* _n_score = NULL;

        if (!heap_delmin(&frontier, (void**)&n, (void**)&_n_score) || n == NULL || _n_score == NULL)
        {
            continue;
        }

        const Vec2 pos = n->pos;
        const GraphNode* node = graph_find_node_const(graph, pos);
        if (vec2_equal(pos, goal))
        {
            // reconstruct path
            Vec2* path = NULL;
            Vec2 current = pos;
            const Vec2 start_pos = start;
            arrput(path, current);
            while (current.x != start_pos.x || current.y != start_pos.y)
            {
                arrput(path, current);
            }

            free(n);
            heap_destroy(&frontier);

            return NULL;
        }

        vbitset_set(closed, pos_idx, true);

        const Vec2 successors[] = SUCCESSORS(pos.x, pos.y);
        const uint16_t score = scores[pos_idx];
        GraphEdge* to_successor = node->edges;
        while (to_successor != NULL)
        {
            const GraphNode* successor = to_successor->to;

            const uint16_t gn = score + NEIGHBOUR_COST;
            const uint16_t hn = (uint16_t)vec2_distance_chebyshev(to_successor, goal);
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
            node->pos = to_successor;
            node->estimated_score = fn;

            heap_insert(&frontier, node, &node->estimated_score);

            to_successor = to_successor->next;
        }

        free(n);
    }

    heap_destroy(&frontier);

    return NULL;
}
