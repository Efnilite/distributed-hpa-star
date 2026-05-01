#ifndef HPA_H
#define HPA_H

#include "../../common/constants.h"
#include "../../common/map.h"
#include "../../common/result.h"

typedef struct cluster_t
{
    Vec2 inter_edges[4 * INTER_EDGES_PER_CLUSTER];
    Vec2 pos;
    uint8_t inter_edges_count;
} Cluster;

Result hpa(const Map* map, Vec2 start, Vec2 goal);

#endif
