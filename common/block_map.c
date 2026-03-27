#include "block_map.h"

#include <stdlib.h>

#include "stb_ds.h"

BlockMap* block_map_create(void)
{
    BlockMap* block_map = malloc(sizeof(BlockMap));
    if (block_map == NULL)
    {
        return NULL;
    }
    block_map->clusters = NULL;
    block_map->cluster_count = 0;

    return block_map;
}

void block_map_add(BlockMap* block_map, const BlockMapCluster cluster)
{
    arrput(block_map->clusters, cluster);
    block_map->cluster_count++;
}

int8_t block_map_get(const BlockMap* block_map, const Vec2 pos)
{
    if (pos.x < 0 || pos.y < 0)
    {
        return -1;
    }

    for (int i = 0; i < block_map->cluster_count; ++i)
    {
        const BlockMapCluster* cluster = &block_map->clusters[i];
        const Vec2 cluster_pos = cluster->pos;
        const Vec2 cluster_dimensions = cluster->dimensions;

        if (pos.x < cluster_pos.x || pos.y < cluster_pos.y
            || pos.x >= cluster_pos.x + cluster_dimensions.x
            || pos.y >= cluster_pos.y + cluster_dimensions.y)
        {
            continue;
        }

        return (int8_t)cluster->value;
    }

    return -1;
}

void block_map_free(BlockMap* block_map)
{
    arrfree(block_map->clusters);
    free(block_map);
}
