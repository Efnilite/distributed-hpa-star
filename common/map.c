#include "map.h"

#include <stdlib.h>

#define XY_TO_IDX(x, y) ((x) + (y) * map->w)

bool map_is_wall(const Map* map, const uint16_t x, const uint16_t y)
{
    const uint32_t idx = XY_TO_IDX(x, y);
    const uint32_t page_idx = idx >> 5;
    const uint8_t in_page = idx % 32;
    const unsigned int v = map->coordinates[page_idx].v;

    return (v & 1 << in_page) >> in_page;
}

void map_free(const Map* map)
{
    free(map->coordinates);
}
