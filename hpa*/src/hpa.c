#include "hpa.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "../../common/stb_ds.h"

typedef struct cluster_t
{
    Vec2 pos;
    Vec2 inter_edges[12];
} Cluster;

#define CLUSTER_XY_TO_IDX(x, y) (cx * CLUSTER_SIZE + (x) + (cy * CLUSTER_SIZE + (y)) * map->w)

#define MIN(a, b) (a) > (b) ? (b) : (a)

// returns the inter edges from one side of a cluster
size_t get_inter_edges_side(const Map* map, const Vec2 cluster_a, const Vec2 cluster_b,
                            const Vec2 start, const Vec2 direction,
                            Vec2* result_a, Vec2* result_b)
{
    assert(direction.x == 1 || direction.y == 1);
    assert(direction.x + direction.y == 1);

    const int cx = cluster_a.x;
    const int cy = cluster_a.y;

    const int to_bx = cluster_b.x - cluster_a.x;
    const int to_by = cluster_b.y - cluster_a.y;

    Vec2 current = start;

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

        const int16_t bx = current.x + to_bx;
        const int16_t by = current.y + to_by;
        const ssize_t symm = CLUSTER_XY_TO_IDX(bx, by);
        if (symm < 0 || symm > map->size)
        {
            continue;
        }
        if (vbitset_get(map->coordinates, symm))
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
        result_a[0] = options_a[0];
        result_a[1] = options_a[options_size / 2];
        result_a[2] = options_a[options_size - 1];

        result_b[0] = options_b[0];
        result_b[1] = options_b[options_size / 2];
        result_b[2] = options_b[options_size - 1];
    }
    else if (result_size == 2)
    {
        result_a[0] = options_a[0];
        result_a[1] = options_a[1];

        result_b[0] = options_b[0];
        result_b[1] = options_b[1];
    }
    else if (result_size == 1)
    {
        result_a[0] = options_a[0];

        result_b[0] = options_b[0];
    }
    return result_size;
}

// returns all inter edges of a cluster
void get_inter_edges(const Map* map, const Vec2 cluster, Vec2 inter_edges[12])
{
    size_t edges = 0;
    {
        Vec2 top_edges[3];
        const size_t count = get_inter_edges_side(map, cluster,
                                                  (Vec2){0, 0}, (Vec2){1, 0}, (Vec2){0, -1},
                                                  top_edges);
        edges += count;
        memcpy(top_edges, inter_edges, count * sizeof(Vec2));
    }

    {
        Vec2 left_edges[3];
        const size_t count = get_inter_edges_side(map, cluster,
                                                  (Vec2){0, 0}, (Vec2){0, 1}, (Vec2){-1, 0},
                                                  left_edges);
        edges += count;
        memcpy(left_edges, inter_edges + edges, count * sizeof(Vec2));
    }

    {
        Vec2 right_edges[3];
        const size_t count = get_inter_edges_side(map, cluster,
                                                  (Vec2){CLUSTER_SIZE, 0}, (Vec2){0, 1}, (Vec2){1, 0},
                                                  right_edges);
        edges += count;
        memcpy(right_edges, inter_edges + edges, count * sizeof(Vec2));
    }

    {
        Vec2 bottom_edges[3];
        const size_t count = get_inter_edges_side(map, cluster,
                                                  (Vec2){0, CLUSTER_SIZE}, (Vec2){1, 0}, (Vec2){0, 1},
                                                  bottom_edges);
        edges += count;
        memcpy(bottom_edges, inter_edges + edges, count * sizeof(Vec2));
    }
}

Result hpa(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    const clock_t begin = clock();

    // preprocess

    const size_t cluster_w = map->w / CLUSTER_SIZE;
    const size_t cluster_h = map->h / CLUSTER_SIZE;
    const size_t cluster_size = cluster_w * cluster_h;
    Cluster clusters[cluster_size];
    for (int cy = 0; cy < cluster_h; ++cy)
    {
        for (int cx = 0; cx < cluster_w; ++cx)
        {
            const Vec2 pos = (Vec2){(int16_t)cx, (int16_t)cy};

            Vec2 inter_edges[12];
            get_inter_edges(map, pos, inter_edges);

            Cluster cluster;
            cluster.pos = pos;
            memcpy(cluster.inter_edges, inter_edges, 12 * sizeof(Vec2));

            clusters[cy * cluster_w + cx] = cluster;
        }
    }

    return (Result){NULL, NULL, false, (double)(clock() - begin) / CLOCKS_PER_SEC, NULL};
}
