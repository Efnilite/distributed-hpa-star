#ifndef RESULT_H
#define RESULT_H

#include <stdbool.h>

#include "map.h"
#include "vec2.h"

typedef struct result_t
{
    struct
    {
        Vec2 key;
        bool value;
    } * visited;
    Vec2* path;
    bool success;
    double cpu_secs;
} Result;

void result_visualize(const Map* map, const Result* result);

#endif
