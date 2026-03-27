#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stdint.h>

#include "block_map.h"

/**
 * A struct for map info.
 */
typedef struct map_t
{
    uint16_t w;
    uint16_t h;
    size_t size;
    BlockMap* coordinates; // 0 = empty, 1 = wall
} Map;

/**
 * Checks whether a coordinate is a wall.
 * @param map The map.
 * @param x
 * @param y
 * @returns True if the coordinate is a wall, false if not.
 */
bool map_is_wall(const Map* map, uint16_t x, uint16_t y);

/**
 * Frees a map.
 * @param map The map.
 */
void map_free(const Map* map);

#endif
