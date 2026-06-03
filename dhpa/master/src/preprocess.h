#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "../../../common/vec2.h"
#include "../../../common/map.h"
#include "../../../common/graph.h"
#include "../../common/hpa.h"

void preprocess(const MapDimensions* dimensions, Graph** graph, const Vec2 start, const Vec2 goal);

#endif 