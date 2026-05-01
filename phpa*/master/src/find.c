#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../../../common/parser.h"
#include "../../../common/stb_ds.h"
#include "../../common/cluster_a.h"
#include "../../common/graph_a.h"

#define VEC_TO_CLUSTER(vec) (vec.y / CLUSTER_SIZE * cluster_w + vec.x / CLUSTER_SIZE)

Result find(Graph* graph, Cluster* clusters, const Vec2 start, const Vec2 goal)
{

    Vec2* graph_path = graph_a(graph, start, goal);
    if (graph_path == NULL)
    {
        printf("Failed to find graph path\n");
        return (Result){NULL, NULL, false, (double)(clock() - calc_begin) / CLOCKS_PER_SEC, graph};
    }

    printf("Found graph path - %fs\n", (double)(clock() - calc_begin) / CLOCKS_PER_SEC);
}
