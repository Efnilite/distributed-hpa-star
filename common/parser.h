#ifndef PARSER_H
#define PARSER_H

#include "map.h"
#include "vbitset.h"
#include <stdint.h>

#define LINE_LENGTH 4096

/**
 * Parses a map file.
 * @param file_name The map file to parse.
 * @returns A Map struct.
 */
Map parse_map(const char* file_name);

/**
 * Reads map dimensions.
 * @param file_name The map file to parse.
 * @param out_width Pointer to store map width.
 * @param out_height Pointer to store map height.
 * @returns 0 on success, -1 on failure.
 */
int parse_map_dimensions(const char* file_name, uint16_t* out_width, uint16_t* out_height);

/**
 * Parses map data for a specific cluster only.
 * @param file_name The map file to parse.
 * @param map_width The total map width.
 * @param map_height The total map height.
 * @param cluster_size The size of a cluster.
 * @param cluster_x The cluster column index.
 * @param cluster_y The cluster row index.
 * @returns A vbitset representing the cluster obstacles, or NULL on failure.
 */
VBitSet* parse_map_for_cluster(const char* file_name, uint16_t map_width, uint16_t map_height, 
        int16_t cluster_x, int16_t cluster_y);

#endif
