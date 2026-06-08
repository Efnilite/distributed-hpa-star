#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stddef.h>

#define CLUSTER_SIZE 150
#define INTER_EDGES_PER_CLUSTER (CLUSTER_SIZE / 2 - 1)

_Static_assert(CLUSTER_SIZE >= 3, "Cluster size must be at least 3");
_Static_assert(INTER_EDGES_PER_CLUSTER >= 1, "Inter edges per cluster must be at least 1");

#define WORKERS_SIZE 9

#define MAP_FILE "/home/efy/Projects/bep/data/sparse/scene_mp_2p_01"
#define START (Vec2){260, 180}
#define GOAL (Vec2){1565, 1745}

// #define MAP_FILE "/home/efy/Projects/bep/data/dense/maze"
// #define START (Vec2){1, 1}
// #define GOAL (Vec2){999, 999}

#endif
