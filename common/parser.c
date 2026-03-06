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

    const size_t num_pages = (w * h + 31) / 32;
    CoordinateBitSet* map = calloc(num_pages, sizeof(CoordinateBitSet));
    if (map == NULL)
    {
        perror("Failed to calloc map");
        exit(EXIT_FAILURE);
    }

    fgets(buff, LINE_LENGTH, file);
    size_t buff_len = strlen(buff);
    size_t buff_index = 0;
    size_t total_count = 0;

    for (size_t i = 0; i < num_pages; ++i)
    {
        unsigned int bit_set = 0;
        for (uint8_t bs_char = 0; bs_char < 32; ++bs_char)
        {
            if (total_count >= w * h)
            {
                break;
            }

            if (buff[buff_index] == '\n' || buff_index >= buff_len - 1)
            {
                fgets(buff, LINE_LENGTH, file);
                buff_len = strlen(buff);
                buff_index = 0;
                --bs_char;
                continue;
            }

            if (buff[buff_index] == '@')
            {
                bit_set |= 1 << bs_char;
            }

            buff_index++;
            total_count++;
        }

        map[i] = (CoordinateBitSet){bit_set};
    }

    fclose(file);

    return (Map){
        .coordinates = map,
        .w = w,
        .h = h
    };
}
