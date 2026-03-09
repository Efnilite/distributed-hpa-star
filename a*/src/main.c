#include <stdio.h>
#include <stdlib.h>

#define STB_DS_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include "../../common/stb_ds.h"

#include "a.h"
#include "../../common/map.h"
#include "../../common/parser.h"

int main()
{
    // const Map map = parse_map("../../data/ih/scene_test");
    // Result result = astar(&map, 3, 1, 3, 3);
    // const Map map = parse_map("../../data/ih/scene_test_small");
    // Result result = astar(&map, 10, 10, 18, 18);
    const Map map = parse_map("../../data/ih/scene_mp_2p_01");
    const Result result = astar(&map, 260, 180, 1565, 1745);
    // const Map map = parse_map("../../data/ih/scene_mp_2p_04");
    // const Result result = astar(&map, 170, 170, 2000, 2600);

    if (!result.success)
    {
        printf("Failed to find path\n");
    }

    result_visualize(&map, &result);

    free(result.visited);
    map_free(&map);

    return 0;
}
