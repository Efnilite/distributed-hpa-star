#include "hpa.h"

#include <stdio.h>
#include <time.h>

typedef struct cluster_t
{
    Vec2 pos;
    VBitSet* bitset;
} Cluster;

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
            VBitSet* bitset = vbitset_create(CLUSTER_SIZE * CLUSTER_SIZE, 1);
            for (int x = 0; x < CLUSTER_SIZE; ++x)
            {
                for (int y = 0; y < CLUSTER_SIZE; ++y)
                {
                    const size_t map_index = cx * CLUSTER_SIZE + x + (cy * CLUSTER_SIZE + y) * map->w;
                    const uint8_t val = vbitset_get(map->coordinates, map_index);

                    vbitset_set(bitset, x + CLUSTER_SIZE * y, val);
                }
            }

            clusters[cy * cluster_w + cx] = (Cluster){
                .pos = (Vec2){(int16_t)cx, (int16_t)cy},
                .bitset = bitset,
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
