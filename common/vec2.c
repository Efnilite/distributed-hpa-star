#include "vec2.h"

#include <math.h>
#include <stdlib.h>

int16_t vec2_distance_manhattan(const Vec2 a, const Vec2 b)
{
    const int16_t dx = (int16_t)(a.x - b.x);
    const int16_t dy = (int16_t)(a.y - b.y);
    return (int16_t)(abs(dx) + abs(dy));
}

float vec2_distance_euclidean(const Vec2 a, const Vec2 b)
{
    const int16_t dx = (int16_t)(a.x - b.x);
    const int16_t dy = (int16_t)(a.y - b.y);
    return sqrtf((float)(dx * dx + dy * dy));
}

int16_t vec2_distance_chebyshev(const Vec2 a, const Vec2 b)
{
    const int16_t dx = (int16_t)abs(a.x - b.x);
    const int16_t dy = (int16_t)abs(a.y - b.y);
    return dx > dy ? dx : dy;
}