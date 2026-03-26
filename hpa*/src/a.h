#ifndef A_H
#define A_H

#include "../../common/map.h"
#include "../../common/result.h"

#include <stdint.h>

/**
 * Runs A*.
 * @param map The map.
 * @param start The starting location.
 * @param goal The goal location.
 */
Result a(const Map* map, Vec2 start, Vec2 goal);

#endif
