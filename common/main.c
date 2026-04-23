#include <stdio.h>

#define STB_DS_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include "stb_ds.h"

#include "map.h"
#include "parser.h"
#include "result.h"

// options

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

int main(void)
{
#ifdef TEST
    const Map map = parse_map("../../data/ih/scene_test_small");
    const Result result = ALGORITHM(&map, (Vec2){1, 1}, (Vec2){18, 18});
#else
    const Map map = parse_map("../../data/ih/scene_test_small");
    const Result result = ALGORITHM(&map, (Vec2){1, 1}, (Vec2){18, 18});
    // const Map map = parse_map("../../data/ih/scene_mp_2p_01");
    // const Result result = ALGORITHM(&map, (Vec2){260, 180}, (Vec2){1565, 1745});
    // const Map map = parse_map("../../data/ih/scene_mp_2p_04");
    // const Result result = ALGORITHM(&map, (Vec2){170, 170}, (Vec2){2000, 2600});
#endif

    if (!result.success)
    {
        printf("Failed to find path\n");
    }

    result_visualize(&map, &result);

    map_free(&map);

    return result.success ? 0 : 1;
}
