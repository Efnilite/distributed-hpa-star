#ifndef RESULT_H
#define RESULT_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "graph.h"
#include "constants.h"
#include "map.h"
#include "vec2.h"

/**
 * A closed node, used for A* performance measuring.
 */
typedef struct closed_node_t
{
    uint16_t estimated_score;
    bool is_closed;
} ClosedNode;


/**
 * A struct containing information about a worker.
 */
typedef struct worker_result_t {
    long max_memory_bytes;
    double cpu_time;
} WorkerResult;

/**
 * A struct containing information about a pathfinding result.
 */
typedef struct result_t
{
    ClosedNode* visited;
    Vec2* path;
    bool success;
    double cpu_secs;
    long max_memory_bytes;
    Graph* graph;
    WorkerResult* workers[WORKERS_SIZE];
} Result;

/**
 * Visualizes the result of the pathfinding attempt in a result file.
 * @param map The map.
 * @param result The struct containing result info.
 */
void result_visualize(Map* map, const Result* result);

/**
 * Visualizes the result of the pathfinding attempt in a cluster to a result file.
 * @param coordinates The cluster data.
 * @param path The path.
 */
void cluster_visualize(const VBitSet* coordinates, const Vec2 cluster_pos, const Vec2* path);

#endif
