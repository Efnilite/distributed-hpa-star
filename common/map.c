#include "map.h"

#include <stdlib.h>

bool map_is_wall(const Map* map, const uint16_t x, const uint16_t y)
{
    if (x >= map->w || y >= map->h)
    {
        return true;
    }

    const uint32_t idx = x + y * map->w;
    const uint32_t page = idx >> 5;
    const uint8_t in_page_idx = idx % 32;
    return (map->coordinates[page].v & 1 << in_page_idx) >> in_page_idx;
}

void map_free(const Map* map)
{
    free(map->coordinates);
}
