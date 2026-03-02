#include <stdio.h>

#define STB_DS_IMPLEMENTATION

#include "a*.h"
#include "../../common/map.h"
#include "../../common/parser.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

int main()
{
    const Map map = parse_map("../../data/ih/scene_test_small");
    const Result result = astar(&map, 10, 10, 18, 18);

    if (!result.success)
    {
        printf("Failed to find path\n");
    }

    result_visualize(&map, &result);

    return 0;
}
