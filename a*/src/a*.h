#ifndef A_H
#define A_H

#include "../../common/map.h"

#include <stdint.h>

int astar(const Map* map, uint16_t sx, uint16_t sy, uint16_t gx, uint16_t gy);

#endif
