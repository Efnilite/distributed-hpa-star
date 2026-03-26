#ifndef HPA_H
#define HPA_H

#include "../../common/map.h"
#include "../../common/result.h"

#define CLUSTER_SIZE 100
#define INTER_EDGES_PER_CLUSTER 2

_Static_assert(CLUSTER_SIZE >= 3, "Cluster size must be at least 3");
_Static_assert(CLUSTER_SIZE >= 1, "Inter edges per cluster must be at least 1");

typedef struct cluster_t
{
    Vec2 inter_edges[4 * INTER_EDGES_PER_CLUSTER];
    Vec2 pos;
    size_t inter_edges_count;
} Cluster;

Result hpa(const Map* map, Vec2 start, Vec2 goal);

#endif
