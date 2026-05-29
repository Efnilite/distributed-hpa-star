#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#define XY_TO_IDX(x, y) ((x) + (y) * map->w)

/**
 * Get the current resident set size (RSS) memory usage in bytes.
 * Returns -1 if unable to read from /proc/self/status.
 */
long util_get_memory_usage(void);

long get_memory_usage(long exisiting);

#endif