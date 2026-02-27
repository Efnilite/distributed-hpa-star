#ifndef VEC2_H
#define VEC2_H

#include <stdint.h>

typedef struct vec2_t
{
    uint16_t x;
    uint16_t y;
} Vec2;

float vec2_distance(Vec2 a, Vec2 b);

#endif
