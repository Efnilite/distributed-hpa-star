#include "map.h"

bool map_is_wall(const Map *map, uint16_t x, uint16_t y)
{
    const uint32_t idx = x + (y * map->w);
    return map->coordinates[idx];
    // const uint32_t set = idx << 4;
    // return map->coordinates[set].v & 1;
}
