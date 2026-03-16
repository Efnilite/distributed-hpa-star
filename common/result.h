#ifndef RESULT_H
#define RESULT_H

#include <stdbool.h>
#include <stdint.h>

#include "map.h"
#include "vec2.h"
#include "graph.h"

typedef struct closed_node_t
{
    uint16_t estimated_score;
    bool is_closed;
} ClosedNode;

typedef struct result_t
{
    ClosedNode* visited;
    Vec2* path;
    bool success;
    double cpu_secs;
    Graph* graph;
} Result;

void result_visualize(const Map* map, const Result* result);

#endif
