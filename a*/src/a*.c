#include "a*.h"

#include <limits.h>
#include <stdlib.h>

#include "../../common/mheap.h"
#include "../../common/vec2.h"

typedef struct node_t
{
    Vec2 pos;
    uint16_t score;
    float estimated_score;
    struct node_t* source;
} Node;

static int frontier_compare(const Node* a, const Node* b)
{
    return (int) (a->estimated_score - b->estimated_score);
}

int astar(const Map* map, const uint16_t sx, const uint16_t sy, const uint16_t gx, const uint16_t gy)
{
    heap frontier;
    heap_create(&frontier, 1, (void*)frontier_compare);

    Node start = {
        { sx, sy },
        0,
        vec2_distance_a(sx, sy, gx, gy),
    };
    int start_score = INT_MAX;
    heap_insert(&frontier, &start, &start_score);

    while (heap_size(&frontier) > 0)
    {
        Node* current = NULL;
        if (heap_delmin(&frontier, (void**)&current, NULL) == 0 || current == NULL)
        {
            break;
        }

        const Vec2 pos = current->pos;
        if (pos.x == gx && pos.y == gy)
        {
            return EXIT_SUCCESS;
        }

        const Vec2 neighbours[] = {
            {pos.x - 1, pos.y},
            {pos.x, pos.y - 1},
            {pos.x + 1, pos.y},
            {pos.x, pos.y + 1},
        };

        for (int i = 0; i < 4; ++i)
        {
            const Vec2 neighbour = neighbours[i];

            const int temp_score = current->score + 1;

            if (map_is_wall(map, neighbour.x, neighbour.y))
            {
                continue;
            }

            const float estimated_score = temp_score;


        }
    }

    heap_destroy(&frontier);

    return EXIT_FAILURE;
}
