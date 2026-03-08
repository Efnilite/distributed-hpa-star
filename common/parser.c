#include "parser.h"
#include "map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Map parse_map(const char* file_name)
{
    FILE* file = fopen(file_name, "r");
    if (file == NULL)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    char buff[LINE_LENGTH];
    // skip type
    fgets(buff, LINE_LENGTH, file);

    // height
    fgets(buff, LINE_LENGTH, file);
    const uint16_t h = atoi(buff + 7);

    // width
    fgets(buff, LINE_LENGTH, file);
    const uint16_t w = atoi(buff + 6);

    // skip map
    fgets(buff, LINE_LENGTH, file);

    CoordinateBitSet* map = calloc((w * h + 31) >> 5, sizeof(CoordinateBitSet));
    if (map == NULL)
    {
        perror("Failed to malloc map");
        exit(EXIT_FAILURE);
    }

    fgets(buff, LINE_LENGTH, file);
    uint32_t line_idx = 0;
    uint32_t page = 0;
    while (!feof(file))
    {
        unsigned int v = 0;
        int i = 31;
        while (i >= 0)
        {
            if (line_idx >= strlen(buff))
            {
                break;
            }

            if (buff[line_idx] == '\n')
            {
                fgets(buff, LINE_LENGTH, file);
                line_idx = 0;
            }

            if (buff[line_idx] == '@')
            {
                v |= 1 << i;
            }

            i--;
            line_idx++;
        }

        map[page] = (CoordinateBitSet){v};
        page++;
    }

    fclose(file);

    return (Map){
        .w = w, .h = h, .coordinates = map
    };
}
