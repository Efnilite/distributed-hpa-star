#ifndef BLOCK_MAP_H
#define BLOCK_MAP_H

#include <stddef.h>

#include "vec2.h"

typedef struct block_map_cluster_t
{
    Vec2 pos;
    Vec2 dimensions;
    bool value;
} BlockMapCluster;

typedef struct block_map_t
{
    BlockMapCluster* clusters;
    size_t cluster_count;
} BlockMap;

/**
 * @return A new block map.
 */
BlockMap* block_map_create(void);

/**
 * Adds a cluster to the block map.
 * @param block_map The block map.
 * @param cluster The cluster to add.
 */
void block_map_add(BlockMap* block_map, BlockMapCluster cluster);

/**
 * Gets a value in the block map.
 * @param block_map The block map.
 * @param pos The global position.
 * @return -1 if pos is out of bounds, 0 if empty, 1 if wall
 */
int8_t block_map_get(const BlockMap* block_map, Vec2 pos);

/**
 * Frees a block map.
 * @param block_map The block map.
 */
void block_map_free(BlockMap* block_map);

#endif
