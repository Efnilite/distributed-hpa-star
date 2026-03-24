#include "hpa.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "../../common/stb_ds.h"

typedef struct cluster_t
{
    Vec2 pos;
    Vec2 inter_edges[12];
    size_t inter_edges_count;
} Cluster;

#define DEBUG
#define CLUSTER_XY_TO_IDX(x, y) (cx * CLUSTER_SIZE + (x) + (cy * CLUSTER_SIZE + (y)) * map->w)

#define MIN(a, b) (a) > (b) ? (b) : (a)

// returns the inter edges from one side of a cluster
static size_t get_inter_edges_side(
    const Map* map, const Vec2 cluster_a, const Vec2 cluster_b,
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

static void populate_edges(const Map* map, Cluster* clusters, Vec2* inter_edges)
{
    const size_t cluster_w = map->w / CLUSTER_SIZE;
    const size_t cluster_h = map->h / CLUSTER_SIZE;

    // horizontal edges
    for (int cy = 0; cy < cluster_h; ++cy)
    {
        for (int cx = 0; cx < cluster_w - 1; ++cx)
        {
            const Vec2 cluster_a = {cx, cy};
            const Vec2 cluster_b = {cx + 1, cy};
            Vec2 result_a[3];
            Vec2 result_b[3];
            const size_t count = get_inter_edges_side(
                map, cluster_a, cluster_b,
                (Vec2){CLUSTER_SIZE - 1, 0}, (Vec2){0, 1},
                result_a, result_b);

#ifdef DEBUG
            for (int i = 0; i < count; ++i)
            {
                assert(result_a[i].x < map->w);
                assert(result_a[i].y < map->h);
                assert(result_b[i].x < map->w);
                assert(result_b[i].y < map->h);
            }
#endif

            Cluster* ca = &clusters[cy * cluster_w + cx];
            Cluster* cb = &clusters[cy * cluster_w + cx + 1];

            for (size_t i = 0; i < count; ++i)
            {
                ca->inter_edges[ca->inter_edges_count++] = result_a[i];
                cb->inter_edges[cb->inter_edges_count++] = result_b[i];
#ifdef DEBUG
                arrpush(inter_edges, result_a[i]);
                arrpush(inter_edges, result_b[i]);
#endif
            }
        }
    }

    // vertical edges
    for (int cy = 0; cy < cluster_h - 1; ++cy)
    {
        for (int cx = 0; cx < cluster_w; ++cx)
        {
            const Vec2 cluster_a = {cx, cy};
            const Vec2 cluster_b = {cx, cy + 1};
            Vec2 result_a[3];
            Vec2 result_b[3];
            const size_t count = get_inter_edges_side(
                map, cluster_a, cluster_b,
                (Vec2){0, CLUSTER_SIZE - 1}, (Vec2){1, 0},
                result_a, result_b);

#ifdef DEBUG
            for (int i = 0; i < count; ++i)
            {
                assert(result_a[i].x < map->w);
                assert(result_a[i].y < map->h);
                assert(result_b[i].x < map->w);
                assert(result_b[i].y < map->h);
            }
#endif

            Cluster* ca = &clusters[cy * cluster_w + cx];
            Cluster* cb = &clusters[(cy + 1) * cluster_w + cx];

            for (size_t i = 0; i < count; ++i)
            {
                ca->inter_edges[ca->inter_edges_count++] = result_a[i];
                cb->inter_edges[cb->inter_edges_count++] = result_b[i];
#ifdef DEBUG
                arrpush(inter_edges, result_a[i]);
                arrpush(inter_edges, result_b[i]);
#endif
            }
        }
    }
}

Result hpa(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    const clock_t begin = clock();
    const size_t cluster_w = map->w / CLUSTER_SIZE;
    const size_t cluster_h = map->h / CLUSTER_SIZE;
    const size_t cluster_size = cluster_w * cluster_h;

    // 1. preprocess
    Vec2* inter_edges = NULL;
    Cluster* clusters = malloc(cluster_size * sizeof(Cluster));
    for (int cy = 0; cy < cluster_h; ++cy)
    {
        for (int cx = 0; cx < cluster_w; ++cx)
        {
            clusters[cy * cluster_w + cx].pos = (Vec2){(int16_t)cx, (int16_t)cy};
            clusters[cy * cluster_w + cx].inter_edges_count = 0;
        }
    }
    populate_edges(map, clusters, inter_edges);

    // 2. pathfind

    // 3. cleanup
    // free(clusters);

    return (Result){NULL, NULL, false, (double)(clock() - begin) / CLOCKS_PER_SEC, inter_edges};
}
