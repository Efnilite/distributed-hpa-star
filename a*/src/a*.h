#ifndef A_H
#define A_H

#include "../../common/map.h"
#include "../../common/vec2.h"

#include <stdint.h>

Vec2* astar(const Map* map, int16_t sx, int16_t sy, int16_t gx, int16_t gy);

#endif
