#include <stdio.h>

#define STB_DS_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include "stb_ds.h"

#include "map.h"
#include "parser.h"
#include "result.h"

// #define A
#define HPA

#ifdef A
#include "../a*/src/a.h"
#define ALGORITHM a
#endif
#ifdef HPA
#include "../hpa*/src/hpa.h"
#define ALGORITHM hpa
#endif

int main()
{
    // Map map = parse_map("../data/ih/scene_test");
    // const Result result = ALGORITHM(&map, 3, 1, 3, 3);
    Map map = parse_map("../data/ih/scene_test_small");
    const Result result = ALGORITHM(&map, (Vec2){10, 10}, (Vec2){18, 18});
    // Map map = parse_map("../data/ih/scene_mp_2p_01");
    // const Result result = ALGORITHM(&map, (Vec2){260, 180}, (Vec2){1565, 1745});
    // Map map = parse_map("../../data/ih/scene_mp_2p_04");
    // const Result result = ALGORITHM(&map, 170, 170, 2000, 2600);

    if (!result.success)
    {
        printf("Failed to find path\n");
    }

    result_visualize(&map, &result);

    map_free(&map);

    return 0;
}
