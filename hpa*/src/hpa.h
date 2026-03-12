#ifndef HPA_H
#define HPA_H

#include "../../common/map.h"
#include "../../common/result.h"

#define CLUSTER_SIZE 10

#include <stdint.h>

Result hpa(const Map* map, int16_t sx, int16_t sy, int16_t gx, int16_t gy);

#endif
