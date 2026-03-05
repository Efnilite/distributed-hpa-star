#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "../../common/mheap.h"
#include "../../common/result.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

#include "a.h"

typedef struct frontier_node_t
{
    Vec2 pos;
    float estimated_score;
} FrontierNode;

static int frontier_compare(void* a, void* b)
{
    const FrontierNode* node_a = a;
    const FrontierNode* node_b = b;
    if (node_a->estimated_score < node_b->estimated_score)
    {
        return -1;
    }
    if (node_a->estimated_score > node_b->estimated_score)
    {
        return 1;
    }
    return 0;
}

typedef struct closed_t
{
    Vec2 key;
    bool is_closed;
    float estimated_score;
} CloseData;

typedef struct score_t
{
    Vec2 key;
    float score;
    Vec2 source;
} ScoreData;

Result astar(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    const clock_t begin = clock();

    heap frontier;
    heap_create(&frontier, map->w + map->h, frontier_compare);

    struct closed_t
    {
        Vec2 key;
        bool is_closed;
        float estimated_score;
    };

    CloseData* closed = NULL;
    ScoreData* scores = NULL;
    {
        const CloseData close_def = (CloseData){0, 0, false, 0};
        hmdefaults(closed, close_def);

        const ScoreData score_def = (ScoreData){0, 0, FLT_MAX, -1, -1};
        hmdefaults(scores, score_def);
    }

    struct
    {
        Vec2 key;
        bool value;
    }* visited = NULL;

    FrontierNode start = {
        .pos = {sx, sy},
        .estimated_score = vec2_distance_euclidean(sx, sy, gx, gy),
    };
    heap_insert(&frontier, &start, &start.estimated_score);
    ScoreData start_score = (ScoreData){start.pos, 0.f, -1, -1};
    hmputs(scores, start_score);

    while (heap_size(&frontier) > 0)
    {
        FrontierNode* n = NULL;
        float* n_score = NULL;

        if (!heap_delmin(&frontier, &n, &n_score) || n == NULL || n_score == NULL)
        {
            continue;
        }

        const Vec2 pos = n->pos;
        if (pos.x == gx && pos.y == gy)
        {
            // Reconstruct path by walking came_from from goal to start
            Vec2* path = NULL;
            Vec2 current = pos;
            arrput(path, current);
            while (!(current.x == sx && current.y == sy))
            {
                current = hmgets(scores, current).source;
                arrput(path, current);
            }

            return (Result){
                visited, path, true,
                (double)(clock() - begin) / CLOCKS_PER_SEC
            };
        }

        CloseData close_n = (CloseData){pos, true, *n_score};
        hmputs(closed, close_n);
        hmput(visited, pos, true);

        const Vec2 successors[] = {
            {(int16_t)(pos.x - 1), pos.y},
            {pos.x, (int16_t)(pos.y - 1)},
            {(int16_t)(pos.x + 1), pos.y},
            {pos.x, (int16_t)(pos.y + 1)},

            {(int16_t)(pos.x + 1), (int16_t)(pos.y + 1)},
            {(int16_t)(pos.x + 1), (int16_t)(pos.y - 1)},
            {(int16_t)(pos.x - 1), (int16_t)(pos.y + 1)},
            {(int16_t)(pos.x - 1), (int16_t)(pos.y - 1)},
        };

        const float score = hmgets(scores, pos).score;
        for (int i = 0; i < 8; ++i)
        {
            const Vec2 successor = successors[i];

            if (map_is_wall(map, successor.x, successor.y))
            {
                continue;
            }

            const float gn = score + (i < 4 ? 5.f : 7.f);
            const float hn = vec2_distance_euclidean(successor.x, successor.y, gx, gy);
            float fn = gn + hn;

            const CloseData closed_data = hmgets(closed, successor);
            if (closed_data.is_closed && fn >= closed_data.estimated_score)
            {
                continue;
            }

            // Only update if we found a better g-score
            const ScoreData old_score = hmgets(scores, successor);
            if (gn >= old_score.score)
            {
                continue;
            }

            ScoreData new_score ={
                successor, gn, pos
            };
            hmputs(scores, new_score);

            // mark successor open
            FrontierNode* node = malloc(sizeof(FrontierNode));
            if (node == NULL)
            {
                perror("Failed node malloc");
                exit(EXIT_FAILURE);
            }
            node->pos = successor;
            node->estimated_score = fn;

            heap_insert(&frontier, node, &fn);
        }
    }

    return (Result){
        visited, NULL, false,
        (double)(clock() - begin) / CLOCKS_PER_SEC
    };
}
