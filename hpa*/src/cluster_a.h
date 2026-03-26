#ifndef A_H
#define A_H

#include "../../common/map.h"
#include "../../common/result.h"

/**
 * Runs A* on a cluster.
 * @param map The map.
 * @param start The starting location.
 * @param goal The goal location.
 */
Vec2* cluster_a(const Map* map, Vec2 start, Vec2 goal);

#endif
