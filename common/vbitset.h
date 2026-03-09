#ifndef VBITSET_H
#define VBITSET_H

#include <stddef.h>
#include <stdint.h>

typedef struct vbitset_t
{
    uint32_t* ptr;
    uint8_t element_size;
    size_t capacity;
} VBitSet;

VBitSet* vbitset_create(size_t capacity, uint8_t element_size);

uint8_t vbitset_get(const VBitSet* bitset, size_t index);

void vbitset_set(const VBitSet* bitset, size_t index, uint8_t value);

void vbitset_free(VBitSet* bitset);

#endif
