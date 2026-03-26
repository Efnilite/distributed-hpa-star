#ifndef HPA_H
#define HPA_H

#include "../../common/map.h"
#include "../../common/result.h"

#define CLUSTER_SIZE 100

_Static_assert(CLUSTER_SIZE >= 3, "Cluster size must be at least 3");

typedef struct cluster_t
{
    Vec2 pos;
    Vec2 inter_edges[12];
    size_t inter_edges_count;
} Cluster;

Result hpa(const Map* map, Vec2 start, Vec2 goal);

#endif
