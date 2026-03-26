#ifndef HPA_H
#define HPA_H

#include "../../common/map.h"
#include "../../common/result.h"

#define CLUSTER_SIZE 100

Result hpa(const Map* map, Vec2 start, Vec2 goal);

#endif
