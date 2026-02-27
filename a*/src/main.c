#include <stdio.h>

#include "a*.h"
#include "../../common/map.h"
#include "../../common/parser.h"

int main()
{
    const Map map = parse_map("../../data/ih/scene_mp_2p_01");

    astar(&map, 150, 150, 1650, 1790);

    return 0;
}
