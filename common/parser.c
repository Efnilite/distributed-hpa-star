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

    bool* map = malloc(w * h);
    if (map == NULL)
    {
        perror("Failed to malloc map");
        exit(EXIT_FAILURE);
    }

    // lines
    for (size_t line = 0; line < h; line++)
    {
        fgets(buff, LINE_LENGTH, file);
        const uint16_t buff_len = strlen(buff);
        if (buff_len == LINE_LENGTH)
        {
            perror("Line length too long");
            exit(EXIT_FAILURE);
        }

        // const uint8_t max = sizeof(unsigned int) * 8;
        for (size_t idx = 0; idx < buff_len; idx++)
        {
            // unsigned int v = 0;
            // for (size_t i = idx; i < max; i++)
            // {
            //     if (buff[i] == '@')
            //     {

            //     }
            //     else
            //     {
            //     }
            // }
            map[line * w + idx] = buff[idx] == '@';
        }
    }

    return (Map){
        .w = w, .h = h, .coordinates = map
    };
}
