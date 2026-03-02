#include "vec2.h"

#include <math.h>

float vec2_distance(const Vec2 a, const Vec2 b)
{
    const uint16_t dx = a.x - b.x;
    const uint16_t dy = a.y - b.y;
    return sqrtf((float)(dx * dx + dy * dy));
}

float vec2_distance_a(const uint16_t ax, const uint16_t ay, const uint16_t bx, const uint16_t by)
{
    const int16_t dx = ax - bx;
    const int16_t dy = ay - by;
    return sqrtf((float)(dx * dx + dy * dy));
}