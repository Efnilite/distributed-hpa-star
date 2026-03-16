#include "hpa.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "../../common/graph.h"
#include "../../common/stb_ds.h"

#define CLUSTER_XY_TO_IDX(x, y) (cx * CLUSTER_SIZE + (x) + (cy * CLUSTER_SIZE + (y)) * map->w)
#define XY_TO_CLUSTER_IDX(x, y) (((x) / CLUSTER_SIZE) + ((y) / CLUSTER_SIZE) * cluster_w)

#define MIN(a, b) (a) > (b) ? (b) : (a)

// returns the inter edges from one side of a cluster
size_t get_inter_edges_side(const Map* map, const Vec2 cluster, const Vec2 start, const Vec2 direction,
                            const Vec2 to_other_cluster, Graph* graph)
{
    assert(direction.x == 1 || direction.y == 1);
    assert(direction.x + direction.y == 1);

    const int cx = cluster.x;
    const int cy = cluster.y;

    Vec2 current = (Vec2){start.x + cx * CLUSTER_SIZE, start.y + cy * CLUSTER_SIZE};

    Vec2 options_a[CLUSTER_SIZE];
    Vec2 options_b[CLUSTER_SIZE];
    size_t options_size = 0;
    for (int step = 0; step < CLUSTER_SIZE; ++step)
    {
        if (vbitset_get(map->coordinates, CLUSTER_XY_TO_IDX(current.x, current.y)))
        {
            current.x += direction.x;
            current.y += direction.y;
            continue;
        }

        const int16_t bx = current.x + to_other_cluster.x;
        const int16_t by = current.y + to_other_cluster.y;

        if (bx >= map->w || by >= map->h)
        {
            continue;
        }
        if (vbitset_get(map->coordinates, CLUSTER_XY_TO_IDX(bx, by)))
        {
            continue;
        }

        options_a[options_size] = current;
        options_b[options_size] = (Vec2){bx, by};
        options_size++;
        current.x += direction.x;
        current.y += direction.y;
    }

    const size_t result_size = MIN(options_size, 3);
    if (result_size >= 3)
    {
        graph_add_node(graph, options_a[0]);
        graph_add_node(graph, options_a[result_size / 2]);
        graph_add_node(graph, options_a[result_size - 1]);

        graph_add_node(graph, options_b[0]);
        graph_add_node(graph, options_b[result_size / 2]);
        graph_add_node(graph, options_b[result_size - 1]);

        graph_add_edge(graph, options_a[0], options_b[0], 1.f);
        graph_add_edge(graph, options_a[result_size / 2], options_b[result_size / 2], 1.f);
        graph_add_edge(graph, options_a[result_size - 1], options_b[result_size - 1], 1.f);
    }
    else if (result_size == 2)
    {
        graph_add_node(graph, options_a[0]);
        graph_add_node(graph, options_a[1]);

        graph_add_node(graph, options_b[0]);
        graph_add_node(graph, options_b[1]);

        graph_add_edge(graph, options_a[0], options_b[0], 1.f);
        graph_add_edge(graph, options_a[1], options_b[1], 1.f);
    }
    else if (result_size == 1)
    {
        graph_add_node(graph, options_a[0]);

        graph_add_node(graph, options_b[0]);

        graph_add_edge(graph, options_a[0], options_b[0], 1.f);
    }
    return result_size;
}

Result hpa(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    const clock_t begin = clock();

    // preprocess
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
