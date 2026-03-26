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

GraphNode* graph_add_node(Graph* graph, const Vec2 pos)
{
    if (graph == NULL)
    {
        return NULL;
    }
    if (graph_find_node(graph, pos) != NULL)
    {
        return NULL;
    }

    GraphNode* node = malloc(sizeof(GraphNode));
    if (node == NULL)
    {
        return NULL;
    }

    node->pos = pos;
    node->edges = NULL;
    node->next = graph->nodes;
    graph->nodes = node;
    graph->node_count++;

    return node;
}

static bool graph_add_edge_(GraphNode* from, GraphNode* to, const float weight)
{
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

bool graph_add_edge_from_nodes(GraphNode* na, GraphNode* nb, const float weight)
{
    if (na == NULL || nb == NULL)
    {
        return false;
    }

    if (!graph_add_edge_(na, nb, weight))
    {
        return false;
    }
    if (!graph_add_edge_(nb, na, weight))
    {
        return false;
    }

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

    if (!graph_add_edge_(na, nb, weight))
    {
        return false;
    }
    if (!graph_add_edge_(nb, na, weight))
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
