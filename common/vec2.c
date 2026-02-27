#include "vec2.h"

#include <math.h>

float vec2_distance(const Vec2 a, const Vec2 b)
{
    const uint16_t dx = a.x - b.x;
    const uint16_t dy = a.y - b.y;
    return sqrtf((float)(dx * dx + dy * dy));
}
