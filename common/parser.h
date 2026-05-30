#ifndef PARSER_H
#define PARSER_H

#include "map.h"

#define LINE_LENGTH 4096

/**
 * Parses a map file.
 * @param file_name The map file to parse.
 * @returns A Map struct.
 */
Map parse_map(const char* file_name);

/**
 * Parses a map file with automatic data directory selection.
 */
Map parse_map_auto(const char* base_filename);

#endif
