#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stdint.h>

#include "vbitset.h"

typedef struct map_t
{
    uint16_t w;
    uint16_t h;
    VBitSet* coordinates; // 0 = empty, 1 = wall
} Map;

bool map_is_wall(const Map* map, uint16_t x, uint16_t y);

void map_free(Map* map);

#endif
