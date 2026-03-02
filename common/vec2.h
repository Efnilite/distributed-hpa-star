#ifndef VEC2_H
#define VEC2_H

#include <stdint.h>

typedef struct vec2_t
{
    int16_t x;
    int16_t y;
} Vec2;

float vec2_distance(Vec2 a, Vec2 b);

float vec2_distance_a(int16_t ax, int16_t ay, int16_t bx, int16_t by);

#endif
