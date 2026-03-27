#include "map.h"

#include <stdlib.h>

bool map_is_wall(const Map* map, const uint16_t x, const uint16_t y)
{
    const int8_t result = block_map_get(map->coordinates, (Vec2){x, y});
    if (!result)
    {
        return false;
    }
    return true;
}

void map_free(const Map* map)
{
    block_map_free(map->coordinates);
}
