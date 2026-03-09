#include "map.h"
#include "util.h"
#include "vbitset.h"

#include <stdlib.h>

bool map_is_wall(const Map* map, const uint16_t x, const uint16_t y)
{
    const uint32_t idx = XY_TO_IDX(x, y);
    return vbitset_get(map->coordinates, idx);
}

void map_free(Map* map)
{
    vbitset_free(map->coordinates);
    // free(map);
}
