#ifndef PARSER_H
#define PARSER_H

#include "map.h"

#define LINE_LENGTH 4096

Map parse_map(const char* file_name);

#endif
