#include "vec2.h"

#include <math.h>
#include <stdlib.h>

float vec2_distance(const Vec2 a, const Vec2 b)
{
    const int16_t dx = (int16_t)(a.x - b.x);
    const int16_t dy = (int16_t)(a.y - b.y);
    return sqrtf((float)(dx * dx + dy * dy));
}

int16_t vec2_distance_manhattan(const int16_t ax, const int16_t ay, const int16_t bx, const int16_t by)
{
    const int16_t dx = (int16_t)(ax - bx);
    const int16_t dy = (int16_t)(ay - by);
    return (int16_t)(abs(dx) + abs(dy));
}

float vec2_distance_euclidean(const int16_t ax, const int16_t ay, const int16_t bx, const int16_t by)
{
    const int16_t dx = (int16_t)(ax - bx);
    const int16_t dy = (int16_t)(ay - by);
    return sqrtf((float)(dx * dx + dy * dy));
}

int16_t vec2_distance_chebyshev(const int16_t ax, const int16_t ay, const int16_t bx, const int16_t by)
{
    const int16_t dx = (int16_t)abs(ax - bx);
    const int16_t dy = (int16_t)abs(ay - by);
    return dx > dy ? dx : dy;
}