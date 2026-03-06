#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct coordinate_bit_set_t
{
    unsigned int v; // 0 = empty, 1 = wall
} CoordinateBitSet;

typedef struct map_t
{
    CoordinateBitSet* coordinates; // 0 = empty, 1 = wall
    uint16_t w;
    uint16_t h;
} Map;

bool map_is_wall(const Map* map, uint16_t x, uint16_t y);

void map_free(const Map* map);

#endif
