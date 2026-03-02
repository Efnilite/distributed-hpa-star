#include <stdio.h>

#define STB_DS_IMPLEMENTATION

#include "a*.h"
#include "../../common/map.h"
#include "../../common/parser.h"
#include "../../common/stb_ds.h"
#include "../../common/vec2.h"

int main()
{
    const Map map = parse_map("../../data/ih/scene_test");

    const Vec2* path = astar(&map, 1, 1, 3, 3);

    for (int i = arrlen(path); i > 0; --i)
    {
        const Vec2* vec = &path[i];
        printf("(%d, %d) ->", vec->x, vec->y);
    }
    printf("\n");

    return 0;
}
