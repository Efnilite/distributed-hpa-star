// Variable-size bitset

#ifndef VBITSET_H
#define VBITSET_H

// #ifdef VBITSET_IMPLEMENTATION

#include <stdlib.h>

#define _bit_header(bitset)

#define bit_get_value(bitset, map, index) \
do { \
    const uint32_t page_idx = index >> 5;\
    const uint8_t in_page = 31 - index % 32;\
    const unsigned int v = map->coordinates[page_idx].v;\
\
    return (v & 1 << in_page) >> in_page; \
} while (0);

#define bit_free(bitset) \
    free(bitset);

// #endif
#endif
