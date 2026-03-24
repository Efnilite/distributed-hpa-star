#ifndef HPA_H
#define HPA_H

#include "../../common/map.h"
#include "../../common/result.h"

// enables A* compact mode to save memory
#define A_COMPACT_MODE
#define CLUSTER_SIZE 10

#include <stdint.h>

Result hpa(const Map* map, Vec2 start, Vec2 goal);

#endif
