#include "vec2.h"

#include <math.h>

float vec2_distance(const Vec2 a, const Vec2 b)
{
    const int16_t dx = a.x - b.x;
    const int16_t dy = a.y - b.y;
    return sqrtf((float)(dx * dx + dy * dy));
}

float vec2_distance_a(const int16_t ax, const int16_t ay, const int16_t bx, const int16_t by)
{
    const int16_t dx = ax - bx;
    const int16_t dy = ay - by;
    return sqrtf((float)(dx * dx + dy * dy));
}