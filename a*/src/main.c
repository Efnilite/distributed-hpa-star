#include <stdio.h>

#include "a*.h"
#include "../../common/map.h"
#include "../../common/parser.h"

int main()
{
    const Map map = parse_map("../../data/ih/scene_test");

    astar(&map, 2, 2, 4, 4);

    return 0;
}
