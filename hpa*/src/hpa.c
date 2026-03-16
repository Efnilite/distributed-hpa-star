#include "hpa.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "../../common/graph.h"
#include "../../common/stb_ds.h"
#include "../../common/util.h"

#define MIN(a, b) (a) > (b) ? (b) : (a)

// returns the inter edges from one side of a cluster
void get_inter_edges_side(const Map* map, const Vec2 cluster, const Vec2 local_start, const Vec2 direction,
                          const Vec2 to_other_cluster, Graph* graph)
{
    assert(direction.x == 1 || direction.y == 1);
    assert(direction.x + direction.y == 1);

    const int cx = cluster.x;
    const int cy = cluster.y;

    Vec2 current = (Vec2){local_start.x + cluster.x * CLUSTER_SIZE, local_start.y + cluster.y * CLUSTER_SIZE};

    // find all options along the edge
    Vec2 options_a[CLUSTER_SIZE];
    Vec2 options_b[CLUSTER_SIZE];
    size_t options_size = 0;
    for (int step = 0; step < CLUSTER_SIZE; ++step)
    {
        if (vbitset_get(map->coordinates, XY_TO_IDX(current.x, current.y)))
        {
            current.x += direction.x;
            current.y += direction.y;
            continue;
        }

        // make sure symmetric node is also valid
        const Vec2 other = (Vec2){current.x + to_other_cluster.x, current.y + to_other_cluster.y};
        if (other.x < 0 || other.x >= map->w || other.y < 0 || other.y >= map->h)
        {
            continue;
        }
        if (vbitset_get(map->coordinates, XY_TO_IDX(other.x, other.y)))
        {
            continue;
        }

        options_a[options_size] = current;
        options_b[options_size] = other;
        options_size++;
        current.x += direction.x;
        current.y += direction.y;
    }

    // select from options
    switch (MIN(options_size, 3))
    {
    case 3:
        graph_add_node(graph, options_a[0]);
        graph_add_node(graph, options_a[options_size / 2]);
        graph_add_node(graph, options_a[options_size - 1]);

        graph_add_node(graph, options_b[0]);
        graph_add_node(graph, options_b[options_size / 2]);
        graph_add_node(graph, options_b[options_size - 1]);

        graph_add_edge(graph, options_a[0], options_b[0], 1.f);
        graph_add_edge(graph, options_a[options_size / 2], options_b[options_size / 2], 1.f);
        graph_add_edge(graph, options_a[options_size - 1], options_b[options_size - 1], 1.f);
        break;
    case 2:
        graph_add_node(graph, options_a[0]);
        graph_add_node(graph, options_a[1]);

        graph_add_node(graph, options_b[0]);
        graph_add_node(graph, options_b[1]);

        graph_add_edge(graph, options_a[0], options_b[0], 1.f);
        graph_add_edge(graph, options_a[1], options_b[1], 1.f);
        break;
    case 1:
        graph_add_node(graph, options_a[0]);

        graph_add_node(graph, options_b[0]);

        graph_add_edge(graph, options_a[0], options_b[0], 1.f);
        break;
    default:
        break;
    }
}

Result hpa(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    const clock_t begin = clock();

    // create abstract graph
    Graph* graph = graph_create();

    const size_t cluster_w = (size_t)ceil(map->w / (float)CLUSTER_SIZE);
    const size_t cluster_h = (size_t)ceil(map->h / (float)CLUSTER_SIZE);
    const size_t cluster_size = cluster_w * cluster_h;
    for (int cy = 0; cy < cluster_h; ++cy)
    {
        for (int cx = 0; cx < cluster_w; ++cx)
        {
            const Vec2 pos = (Vec2){(int16_t)cx, (int16_t)cy};

            get_inter_edges_side(map, pos, (Vec2){0, 0}, (Vec2){1, 0}, (Vec2){0, -1}, graph);
            get_inter_edges_side(map, pos, (Vec2){0, 0}, (Vec2){0, 1}, (Vec2){-1, 0}, graph);
            get_inter_edges_side(map, pos, (Vec2){CLUSTER_SIZE, 0}, (Vec2){0, 1}, (Vec2){1, 0}, graph);
            get_inter_edges_side(map, pos, (Vec2){0, CLUSTER_SIZE}, (Vec2){1, 0}, (Vec2){0, 1}, graph);
        }
    }

    // cleanup

    // graph_free(graph);

    return (Result){NULL, NULL, false, (double)(clock() - begin) / CLOCKS_PER_SEC, graph};
}
