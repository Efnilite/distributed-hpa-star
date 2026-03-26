#include "graph.h"

#include <stdlib.h>

Graph* graph_create(void)
{
    Graph* graph = malloc(sizeof(Graph));
    if (graph == NULL)
    {
        return NULL;
    }

    graph->nodes = NULL;
    graph->node_count = 0;
    return graph;
}

static void graph_free_edges(GraphEdge* edge)
{
    while (edge != NULL)
    {
        GraphEdge* next = edge->next;
        free(edge);
        edge = next;
    }
}

void graph_free(Graph* graph)
{
    if (graph == NULL)
    {
        return;
    }

    GraphNode* node = graph->nodes;
    while (node != NULL)
    {
        GraphNode* next = node->next;
        graph_free_edges(node->edges);
        free(node);
        node = next;
    }

    graph->nodes = NULL;
    graph->node_count = 0;
}

GraphNode* graph_find_node(const Graph* graph, const Vec2 pos)
{
    if (graph == NULL)
    {
        return NULL;
    }

    GraphNode* current = graph->nodes;
    while (current != NULL)
    {
        if (vec2_equal(current->pos, pos))
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

const GraphNode* graph_find_node_const(const Graph* graph, const Vec2 pos)
{
    return graph_find_node((Graph*)graph, pos);
}

bool graph_has_node(const Graph* graph, const Vec2 pos) { return graph_find_node_const(graph, pos) != NULL; }

bool graph_add_node(Graph* graph, const Vec2 pos)
{
    if (graph == NULL)
    {
        return false;
    }
    if (graph_find_node(graph, pos) != NULL)
    {
        return true;
    }

    GraphNode* node = malloc(sizeof(GraphNode));
    if (node == NULL)
    {
        return false;
    }

    node->pos = pos;
    node->edges = NULL;
    node->next = graph->nodes;
    graph->nodes = node;
    graph->node_count++;

    return true;
}

static GraphEdge* graph_find_edge(const GraphNode* from, const GraphNode* to)
{
    GraphEdge* edge = from->edges;
    while (edge != NULL)
    {
        if (edge->to == to)
        {
            return edge;
        }
        edge = edge->next;
    }

    return NULL;
}

static bool graph_add_or_update_edge(GraphNode* from, GraphNode* to, const float weight)
{
    GraphEdge* existing = graph_find_edge(from, to);
    if (existing != NULL)
    {
        existing->weight = weight;
        return true;
    }

    GraphEdge* edge = malloc(sizeof(GraphEdge));
    if (edge == NULL)
    {
        return false;
    }

    edge->to = to;
    edge->weight = weight;
    edge->next = from->edges;
    from->edges = edge;
    return true;
}

bool graph_add_edge(const Graph* graph, const Vec2 a, const Vec2 b, const float weight)
{
    if (graph == NULL)
    {
        return false;
    }

    GraphNode* na = graph_find_node(graph, a);
    GraphNode* nb = graph_find_node(graph, b);
    if (na == NULL || nb == NULL)
    {
        return false;
    }

    if (!graph_add_or_update_edge(na, nb, weight))
    {
        return false;
    }
    if (!graph_add_or_update_edge(nb, na, weight))
    {
        return false;
    }

    return true;
}

size_t graph_node_count(const Graph* graph)
{
    if (graph == NULL)
    {
        return 0;
    }

    return graph->node_count;
}

size_t graph_edge_count(const Graph* graph, const Vec2 pos)
{
    const GraphNode* node = graph_find_node_const(graph, pos);
    if (node == NULL)
    {
        return 0;
    }

    size_t count = 0;
    const GraphEdge* edge = node->edges;
    while (edge != NULL)
    {
        count++;
        edge = edge->next;
    }
    return count;
}

const GraphEdge* graph_neighbors(const Graph* graph, const Vec2 pos)
{
    const GraphNode* node = graph_find_node_const(graph, pos);
    if (node == NULL)
    {
        return NULL;
    }
    return node->edges;
}
