#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stddef.h>

#define CLUSTER_SIZE 100
#define INTER_EDGES_PER_CLUSTER (CLUSTER_SIZE / 5)

_Static_assert(CLUSTER_SIZE >= 3, "Cluster size must be at least 3");
_Static_assert(INTER_EDGES_PER_CLUSTER >= 1, "Inter edges per cluster must be at least 1");

#define WORKERS_SIZE 4

#endif
