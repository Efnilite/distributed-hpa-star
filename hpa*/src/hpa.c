#include "hpa.h"
#include "a.h"
#include "graph_a.h"

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
static void get_inter_edges_side(const Map* map, Cluster* cluster_a, Cluster* cluster_b, const Vec2 local_start,
                                 const Vec2 direction, const Vec2 to_other_cluster, Graph* graph)
{
    assert(direction.x == 1 || direction.y == 1);
    assert(direction.x + direction.y == 1);

    Vec2 current =
        (Vec2){local_start.x + cluster_a->pos.x * CLUSTER_SIZE, local_start.y + cluster_a->pos.y * CLUSTER_SIZE};

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
    const size_t result_size = MIN(options_size, 3);
    Vec2 res_a[3];
    Vec2 res_b[3];
    if (result_size >= 3)
    {
        res_a[0] = options_a[0];
        res_a[1] = options_a[options_size / 2];
        res_a[2] = options_a[options_size - 1];

        res_b[0] = options_b[0];
        res_b[1] = options_b[options_size / 2];
        res_b[2] = options_b[options_size - 1];
    }
    else if (result_size == 2)
    {
        res_a[0] = options_a[0];
        res_a[1] = options_a[1];

        res_b[0] = options_b[0];
        res_b[1] = options_b[1];
    }
    else if (result_size == 1)
    {
        res_a[0] = options_a[0];

        res_b[0] = options_b[0];
    }

    for (size_t i = 0; i < result_size; ++i)
    {
        graph_add_node(graph, res_a[i]);
        graph_add_node(graph, res_b[i]);
        graph_add_edge(graph, res_a[i], res_b[i], 1.f);

        if (cluster_a->inter_edges_count < 12)
        {
            cluster_a->inter_edges[cluster_a->inter_edges_count++] = res_a[i];
        }
        if (cluster_b->inter_edges_count < 12)
        {
            cluster_b->inter_edges[cluster_b->inter_edges_count++] = res_b[i];
        }
    }
}

static void populate_edges(const Map* map, Cluster* clusters, Graph* graph)
{
    const size_t cluster_w = (size_t)ceil(map->w / (float)CLUSTER_SIZE);
    const size_t cluster_h = (size_t)ceil(map->h / (float)CLUSTER_SIZE);

    // horizontal edges
    for (int cy = 0; cy < cluster_h; ++cy)
    {
        for (int cx = 0; cx < cluster_w - 1; ++cx)
        {
            Cluster* ca = &clusters[cy * cluster_w + cx];
            Cluster* cb = &clusters[cy * cluster_w + cx + 1];

            get_inter_edges_side(map, ca, cb, (Vec2){CLUSTER_SIZE - 1, 0}, (Vec2){0, 1}, (Vec2){1, 0}, graph);
        }
    }

    // vertical edges
    for (int cy = 0; cy < cluster_h - 1; ++cy)
    {
        for (int cx = 0; cx < cluster_w; ++cx)
        {
            Cluster* ca = &clusters[cy * cluster_w + cx];
            Cluster* cb = &clusters[(cy + 1) * cluster_w + cx];

            get_inter_edges_side(map, ca, cb, (Vec2){0, CLUSTER_SIZE - 1}, (Vec2){1, 0}, (Vec2){0, 1}, graph);
        }
    }
}

Result hpa(const Map* map, const Vec2 start, const Vec2 goal)
{
    const clock_t begin = clock();

    // 1. preprocess
    // create graph
    Graph* graph = graph_create();

    const size_t cluster_w = (size_t)ceil(map->w / (float)CLUSTER_SIZE);
    const size_t cluster_h = (size_t)ceil(map->h / (float)CLUSTER_SIZE);
    const size_t cluster_size = cluster_w * cluster_h;

    Cluster* clusters = malloc(sizeof(Cluster) * cluster_size);

    for (int cy = 0; cy < cluster_h; ++cy)
    {
        for (int cx = 0; cx < cluster_w; ++cx)
        {
            clusters[cy * cluster_w + cx].pos = (Vec2){(int16_t)cx, (int16_t)cy};
            clusters[cy * cluster_w + cx].inter_edges_count = 0;
        }
    }

    populate_edges(map, clusters, graph);
    printf("Populated edges\n");

    // find paths in cluster
    for (size_t i = 0; i < cluster_size; i++)
    {
        const Cluster* cluster = &clusters[i];

        for (size_t a_idx = 0; a_idx < cluster->inter_edges_count; ++a_idx)
        {
            for (size_t b_idx = a_idx + 1; b_idx < cluster->inter_edges_count; ++b_idx)
            {
                Vec2* path = cluster_a(map, cluster->inter_edges[a_idx], cluster->inter_edges[b_idx]);
                if (path == NULL)
                {
                    continue;
                }

                graph_add_edge(graph, cluster->inter_edges[a_idx], cluster->inter_edges[b_idx],
                               arrlen(path) - 1);

                arrfree(path);
            }
        }

        printf("Finalized cluster %d,%d\n", cluster->pos.x, cluster->pos.y);
    }

    // find paths from start and goal to their cluster's inter edges
    graph_add_node(graph, start);
    graph_add_node(graph, goal);

    const Cluster* start_cluster = &clusters[start.y / CLUSTER_SIZE * cluster_w + start.x / CLUSTER_SIZE];
    for (size_t i = 0; i < start_cluster->inter_edges_count; ++i)
    {
        Vec2* path = cluster_a(map, start, start_cluster->inter_edges[i]);
        if (path == NULL)
        {
            continue;
        }

        graph_add_edge(graph, start, start_cluster->inter_edges[i], arrlen(path) - 1);
        arrfree(path);
    }

    const Cluster* goal_cluster = &clusters[goal.y / CLUSTER_SIZE * cluster_w + goal.x / CLUSTER_SIZE];
    for (size_t i = 0; i < goal_cluster->inter_edges_count; ++i)
    {
        Vec2* path = cluster_a(map, goal, goal_cluster->inter_edges[i]);
        if (path == NULL)
        {
            continue;
        }

        graph_add_edge(graph, goal_cluster->inter_edges[i], goal, arrlen(path) - 1);
        arrfree(path);
    }
    printf("Finalized extremity cluster finding\n");

    // 2. calculate

    // if start and goal cluster are the same, just run A*
    if (start_cluster == goal_cluster)
    {
        Vec2* final_path = cluster_a(map, start, goal);
        printf("Found overall path due to start and goal cluster being the same\n");
        return (Result){NULL, final_path, final_path != NULL && arrlen(final_path) > 0,
                        (double)(clock() - begin) / CLOCKS_PER_SEC, graph};
    }

    // else run graph pathfinding
    Vec2* graph_path = graph_a(map, graph, start, goal);
    if (graph_path == NULL)
    {
        printf("Failed to find graph path\n");
        return (Result){NULL, NULL, false, (double)(clock() - begin) / CLOCKS_PER_SEC, graph};
    }

    printf("Found graph path\n");

    // 3. build final path
    Vec2* final_path = NULL;
    for (int i = 0; i < arrlen(graph_path) - 1; ++i)
    {
        Vec2* path = cluster_a(map, graph_path[i], graph_path[i + 1]);
        if (path == NULL)
        {
            continue;
        }

        // concat paths
        const int start_idx = i == 0 ? 0 : 1;
        for (int j = start_idx; j < arrlen(path); ++j)
        {
            arrput(final_path, path[j]);
        }
        arrfree(path);
    }

    // 4. cleanup
    free(clusters);
    arrfree(graph_path);

    // graph_free(graph);

    printf("Found overall path\n");
    return (Result){NULL, final_path, final_path != NULL && arrlen(final_path) > 0,
                    (double)(clock() - begin) / CLOCKS_PER_SEC, graph};
}
