#ifndef VEC2_H
#define VEC2_H

#include <stdbool.h>
#include <stdint.h>

/**
 * A 2d int vector.
 */
typedef struct vec2_t
{
    int16_t x;
    int16_t y;
} Vec2;

static inline bool vec2_equal(Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }

/**
 * Get the Manhattan distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @returns The distance.
 */
int16_t vec2_distance_manhattan(Vec2 a, Vec2 b);

/**
 * Get the Euclidean distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @returns The distance.
 */
float vec2_distance_euclidean(Vec2 a, Vec2 b);

/**
 * Get the Chebyshev distance between two vectors.
 * @param a The first vector.
 * @param b The second vector.
 * @returns The distance.
 */
int16_t vec2_distance_chebyshev(Vec2 a, Vec2 b);

#endif
