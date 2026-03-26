#ifndef GRAPH_H
#define GRAPH_H

#include "vec2.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct graph_node_t GraphNode;
typedef struct graph_edge_t GraphEdge;

typedef struct graph_t
{
    GraphNode* nodes;
    size_t node_count;
} Graph;

typedef struct graph_edge_t
{
    GraphNode* to;
    float weight;
    GraphEdge* next;
} GraphEdge;

struct graph_node_t
{
    Vec2 pos;
    GraphEdge* edges;
    GraphNode* next;
};

Graph* graph_create(void);
void graph_free(Graph* graph);

GraphNode* graph_find_node(const Graph* graph, Vec2 pos);
const GraphNode* graph_find_node_const(const Graph* graph, Vec2 pos);
bool graph_has_node(const Graph* graph, Vec2 pos);
bool graph_add_node(Graph* graph, Vec2 pos);

bool graph_add_edge(Graph* graph, Vec2 a, Vec2 b, float weight);

size_t graph_node_count(const Graph* graph);
size_t graph_edge_count(const Graph* graph, Vec2 pos);
const GraphEdge* graph_neighbors(const Graph* graph, Vec2 pos);

#endif