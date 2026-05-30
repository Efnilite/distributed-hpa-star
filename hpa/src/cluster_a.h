#ifndef A_H
#define A_H

#include "hpa.h"
#include "../../common/map.h"
#include "../../common/result.h"

/**
 * Runs A* on a cluster.
 * @param map The map.
 * @param cluster The cluster.
 * @param global_start The absolute starting location.
 * @param global_goal The absolute goal location.
 */
Vec2* cluster_a(const Map* map, const Cluster* cluster, Vec2 global_start, Vec2 global_goal);

#endif
