#include "a*.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../common/mheap.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

typedef struct node_t
{
    Vec2 pos;
    float score;
    float estimated_score;
    struct node_t* source;
} Node;

static int frontier_compare(const Node* a, const Node* b)
{
    return (int)(a->estimated_score - b->estimated_score);
}

Vec2* astar(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    heap frontier;
    heap_create(&frontier, 1, (void*)frontier_compare);

    Node* start = malloc(sizeof(Node));
    if (start == NULL)
    {
        perror("Failed node malloc");
        exit(EXIT_FAILURE);
    }
    start->pos.x = sx;
    start->pos.y = sy;
    start->score = 0;
    start->estimated_score = vec2_distance_a(sx, sy, gx, gy);
    start->source = NULL;

    int start_score = INT_MAX;
    heap_insert(&frontier, start, &start_score);

    while (heap_size(&frontier) > 0)
    {
        Node* current = NULL;
        float* _ = NULL;
        if (heap_delmin(&frontier, (void**)&current, (void**)&_) == 0 || current == NULL)
        {
            break;
        }

        const Vec2 pos = current->pos;
        if (pos.x == gx && pos.y == gy)
        {
            Vec2* path = NULL;
            const Node* node = current;
            arrput(path, pos);
            while (node->source != NULL)
            {
                arrput(path, node->pos);
                node = node->source;
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
            {pos.x - 1, pos.y},
            {pos.x, pos.y - 1},
            {pos.x + 1, pos.y},
            {pos.x, pos.y + 1},
        };

        for (int i = 0; i < 4; ++i)
        {
            const Vec2 neighbour = neighbours[i];

            const float temp_score = current->score + 1;
            if (map_is_wall(map, neighbour.x, neighbour.y))
            {
                continue;
            }

            float estimated_score = temp_score + vec2_distance_a(neighbour.x, neighbour.y, gx, gy);

            Node* neighbor_node = malloc(sizeof(Node));
            if (neighbor_node == NULL)
            {
                perror("Failed node malloc");
                exit(EXIT_FAILURE);
            }
            neighbor_node->pos = neighbour;
            neighbor_node->score = temp_score;
            neighbor_node->estimated_score = estimated_score;
            neighbor_node->source = current;

            heap_insert(&frontier, neighbor_node, &estimated_score);
        }
    }

    heap_destroy(&frontier);

    return NULL;
}
