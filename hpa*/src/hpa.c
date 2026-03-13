#include "hpa.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "../../common/stb_ds.h"

typedef struct cluster_t
{
    Vec2 pos;
    Vec2* inter_edges;
} Cluster;

#define CLUSTER_XY_TO_IDX(x, y) (cx * CLUSTER_SIZE + (x) + (cy * CLUSTER_SIZE + (y)) * map->w)

#define MIN(a, b) (a) > (b) ? (b) : (a)

size_t get_inter_edges_side(const Map* map, const Vec2 cluster,
                           const Vec2 start, const Vec2 direction, const Vec2 to_other_cluster,
                           Vec2* result)
{
    assert(direction.x == 1 || direction.y == 1);

    const int cx = cluster.x;
    const int cy = cluster.y;

    Vec2 current = start;

    Vec2 options[CLUSTER_SIZE];
    size_t options_size = 0;
    for (int step = 0; step < CLUSTER_SIZE; ++step)
    {
        if (vbitset_get(map->coordinates, CLUSTER_XY_TO_IDX(current.x, current.y)))
        {
            current.x += direction.x;
            current.y += direction.y;
            continue;
        }

        const ssize_t symm = CLUSTER_XY_TO_IDX(to_other_cluster.x + current.x, to_other_cluster.y + current.y);
        if (symm < 0 || symm > map->size)
        {
            continue;
        }
        if (vbitset_get(map->coordinates, symm))
        {
            continue;
        }

        options[options_size] = current;
        options_size++;
    }

    const size_t result_size = MIN(options_size, 3);
    if (result_size >= 3)
    {
        result[0] = options[0];
        result[1] = options[result_size / 2];
        result[2] = options[result_size];
    }
    else if (result_size == 2)
    {
        result[0] = options[0];
        result[1] = options[1];
    }
    else if (result_size == 1)
    {
        result[0] = options[0];
    }
    return result_size;
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
            Vec2* inter_edges = NULL;

            get_inter_edges(map, cx, cy, inter_edges);

            clusters[cy * cluster_w + cx] = (Cluster){
                .pos = (Vec2){(int16_t)cx, (int16_t)cy},
                .inter_edges = inter_edges,
            };
        }
    }

    for (int i = 0; i < cluster_size; ++i)
    {
        if (vbitset_get(clusters[i].bitset, 0))
        {
            printf("adf");
        }
    }

    // cleanup
    for (int i = 0; i < cluster_size; ++i)
    {
        vbitset_free(clusters[i].bitset);
    }

    return (Result){NULL, NULL, false, (double)(clock() - begin) / CLOCKS_PER_SEC};
}
