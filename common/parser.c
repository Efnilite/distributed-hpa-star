#include "parser.h"
#include "map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vbitset.h"

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

    VBitSet* map = vbitset_create(w * h, 1);
    if (map == NULL)
    {
        perror("Failed to malloc map");
        exit(EXIT_FAILURE);
    }

    (void)fgets(buff, LINE_LENGTH, file);
    uint32_t char_idx = 0;
    uint32_t line_idx = 0;
    while (!feof(file))
    {
        if (buff[line_idx] == '\n')
        {
            (void)fgets(buff, LINE_LENGTH, file);
            line_idx = 0;
        }

        if (buff[line_idx] != '.')
        {
            vbitset_set(map, char_idx, 1);
        }

        line_idx++;
        char_idx++;
    }

    fclose(file);

    return (Map){
        .w = w, .h = h, .size = w * h, .coordinates = map
    };
}
