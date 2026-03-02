#ifndef VEC2_H
#define VEC2_H

#include <stdint.h>

typedef struct vec2_t
{
    int16_t x;
    int16_t y;
} Vec2;

float vec2_distance(Vec2 a, Vec2 b);

int16_t vec2_distance_manhattan(int16_t ax, int16_t ay, int16_t bx, int16_t by);

float vec2_distance_euclidean(int16_t ax, int16_t ay, int16_t bx, int16_t by);

int16_t vec2_distance_chebyshev(int16_t ax, int16_t ay, int16_t bx, int16_t by);

#endif
