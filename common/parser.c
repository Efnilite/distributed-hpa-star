#include "parser.h"
#include "map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block_map.h"

Map parse_map(const char* file_name)
{
    FILE* file = fopen(file_name, "r");
    if (file == NULL)
    {
        perror("Failed to open map file");
        exit(EXIT_FAILURE);
    }

    char buff[LINE_LENGTH];
    // skip type
    (void)fgets(buff, LINE_LENGTH, file);

    // height
    (void)fgets(buff, LINE_LENGTH, file);
    const uint16_t h = atoi(buff + 7);

    // width
    (void)fgets(buff, LINE_LENGTH, file);
    const uint16_t w = atoi(buff + 6);

    // skip map
    (void)fgets(buff, LINE_LENGTH, file);

    BlockMap* map = block_map_create();
    if (map == NULL)
    {
        perror("Failed to malloc map");
        exit(EXIT_FAILURE);
    }

    (void)fgets(buff, LINE_LENGTH, file);
    int16_t x = 0;
    int16_t y = 0;
    while (!feof(file))
    {
        const char cluster_c = buff[x];
        int16_t cluster_w = 1;
        int16_t cluster_h = 1;

        while (true)
        {
            bool fail = false;
            for (size_t dy = 0; dy < cluster_h; dy++)
            {
                if (buff[(x + cluster_w) + (y + dy) * w] != cluster_c)
                {
                    fail = true;
                    break;
                }
            }
            for (size_t dx = 0; dx < cluster_w; dx++)
            {
                if (buff[(x + dx) + (y + cluster_h) * w] != cluster_c)
                {
                    fail = true;
                    break;
                }
            }

            if (fail)
            {
                break;
            }

            cluster_w++;
            cluster_h++;
        }

        block_map_add(map,
                      (BlockMapCluster){
                          .pos = (Vec2){x, y},
                          .dimensions = (Vec2){cluster_w, cluster_h},
                          .value = cluster_c == '@',
                      });
    }

    fclose(file);

    return (Map){.w = w, .h = h, .size = w * h, .coordinates = map};
}
