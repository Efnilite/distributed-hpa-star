#include "parser.h"
#include "map.h"
#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "vbitset.h"

/**
 * Helper function to try opening a map file from a given directory
 */
static FILE* try_open_map(const char* dir, const char* filename)
{
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir, filename);
    return fopen(full_path, "r");
}

Map parse_map(const char* file_name)
{
    {
        printf("Current directory contents:\n");
        struct dirent* de; 
        DIR* dr = opendir(".");

        if (dr == NULL)
        {
            printf("Could not open current directory");
            return (Map){.w = 0, .h = 0, .size = 0, .coordinates = NULL};
        }

        while ((de = readdir(dr)) != NULL)
            printf("%s\n", de->d_name);

        closedir(dr);
        printf("\n");
    }

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

    return (Map){.w = w, .h = h, .size = w * h, .coordinates = map};
}

int parse_map_dimensions(const char* file_name, uint16_t* out_width, uint16_t* out_height)
{
    if (!file_name || !out_width || !out_height)
    {
        return -1;
    }

    FILE* file = fopen(file_name, "r");
    if (file == NULL)
    {
        perror("Failed to open map file");
        return -1;
    }

    char buff[LINE_LENGTH];

    // skip type
    if (fgets(buff, LINE_LENGTH, file) == NULL)
        goto error;

    // height
    if (fgets(buff, LINE_LENGTH, file) == NULL)
        goto error;
    *out_height = (uint16_t)atoi(buff + 7);

    // width
    if (fgets(buff, LINE_LENGTH, file) == NULL)
        goto error;
    *out_width = (uint16_t)atoi(buff + 6);

    fclose(file);
    return 0;

error:
    fclose(file);
    return -1;
}

VBitSet* parse_map_for_cluster(const char* file_name, uint16_t map_width, uint16_t map_height,
        int16_t cluster_x, int16_t cluster_y)
{
    if (!file_name || CLUSTER_SIZE == 0)
    {
        return NULL;
    }

    FILE* file = fopen(file_name, "r");
    if (file == NULL)
    {
        perror("Failed to open map file for cluster parsing");
        return NULL;
    }

    char buff[LINE_LENGTH];

    // Skip header (type, height, width, "map")
    if (fgets(buff, LINE_LENGTH, file) == NULL)
        goto error;
    if (fgets(buff, LINE_LENGTH, file) == NULL)
        goto error;
    if (fgets(buff, LINE_LENGTH, file) == NULL)
        goto error;
    if (fgets(buff, LINE_LENGTH, file) == NULL)
        goto error;

    // Create bitset for this cluster
    size_t cluster_area = CLUSTER_SIZE * CLUSTER_SIZE;
    VBitSet* cluster_bits = vbitset_create(cluster_area, 1);
    if (cluster_bits == NULL)
    {
        fprintf(stderr, "Failed to create bitset for cluster (%d, %d)\n", cluster_x, cluster_y);
        goto error;
    }

    // Calculate cluster bounds in global coordinates
    uint16_t cluster_start_x = (uint16_t)(cluster_x * CLUSTER_SIZE);
    uint16_t cluster_start_y = (uint16_t)(cluster_y * CLUSTER_SIZE);
    uint16_t cluster_end_x = cluster_start_x + CLUSTER_SIZE;
    uint16_t cluster_end_y = cluster_start_y + CLUSTER_SIZE;

    // Clamp to map bounds
    if (cluster_end_x > map_width)
        cluster_end_x = map_width;
    if (cluster_end_y > map_height)
        cluster_end_y = map_height;

    // Read map data line by line
    uint32_t current_y = 0;
    while (fgets(buff, LINE_LENGTH, file) != NULL && current_y < map_height)
    {
        // Check if this line is within the cluster's y-range
        if (current_y >= cluster_start_y && current_y < cluster_end_y)
        {
            uint16_t local_y = current_y - cluster_start_y;

            // Process characters in this line that fall within the cluster's x-range
            for (uint32_t char_idx = 0; char_idx < map_width && buff[char_idx] != '\n' && buff[char_idx] != '\0';
                 char_idx++)
            {
                if (char_idx >= cluster_start_x && char_idx < cluster_end_x)
                {
                    uint16_t local_x = (uint16_t)(char_idx - cluster_start_x);
                    size_t idx = local_x + local_y * CLUSTER_SIZE;

                    // Set bit to 1 if it's a wall ('#'), 0 if empty ('.')
                    if (buff[char_idx] != '.')
                    {
                        vbitset_set(cluster_bits, idx, 1);
                    }
                }
            }
        }

        current_y++;
    }

    // Handle out-of-bounds cells as walls
    for (uint16_t local_y = 0; local_y < CLUSTER_SIZE; local_y++)
    {
        for (uint16_t local_x = 0; local_x < CLUSTER_SIZE; local_x++)
        {
            uint16_t global_x = cluster_start_x + local_x;
            uint16_t global_y = cluster_start_y + local_y;

            if (global_x >= map_width || global_y >= map_height)
            {
                size_t idx = local_x + local_y * CLUSTER_SIZE;
                vbitset_set(cluster_bits, idx, 1);
            }
        }
    }

    fclose(file);
    return cluster_bits;

error:
    fclose(file);
    return NULL;
}
