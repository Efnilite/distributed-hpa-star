#ifndef A_H
#define A_H

#include <math.h>
#include <stdlib.h>

#include "../../../common/constants.h"
#include "../../../common/vbitset.h"
#include "../../../common/vec2.h"

typedef struct worker_cluster_t
{
    Vec2 pos;
    VBitSet* bits;
} WorkerCluster;

static inline uint32_t xy_to_idx_cluster_a(uint16_t x, uint16_t y)
{
    return x + y * CLUSTER_SIZE;
}

static inline Vec2 global_vec_to_local_vec(const WorkerCluster* cluster, Vec2 vec)
{
    return (Vec2){vec.x - cluster->pos.x * CLUSTER_SIZE, vec.y - cluster->pos.y * CLUSTER_SIZE};
}

static inline Vec2 local_vec_to_global_vec(const WorkerCluster* cluster, Vec2 vec)
{
    return (Vec2){vec.x + cluster->pos.x * CLUSTER_SIZE, vec.y + cluster->pos.y * CLUSTER_SIZE};
}

static inline Vec2 global_vec_to_cluster_pos(Vec2 vec)
{
    return (Vec2){(int16_t)floor(vec.x / (float)CLUSTER_SIZE), (int16_t)floor(vec.y / (float)CLUSTER_SIZE)};
}

/**
 * Runs A* on a cluster.
 * @param cluster The cluster.
 * @param global_start The absolute starting location.
 * @param global_goal The absolute goal location.
 */
Vec2* worker_a(const WorkerCluster* cluster, Vec2 global_start, Vec2 global_goal);

#endif
