#ifndef VEC2_H
#define VEC2_H

#include <stdint.h>

typedef struct vec2_t
{
    uint16_t x;
    uint16_t y;
} Vec2;

float vec2_distance(Vec2 a, Vec2 b);

float vec2_distance_a(uint16_t ax, uint16_t ay, uint16_t bx, uint16_t by);

#endif
