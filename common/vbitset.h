#ifndef VBITSET_H
#define VBITSET_H

#include <stddef.h>
#include <stdint.h>

/**
 * A variable element-size fixed-capacity bitset struct.
 */
typedef struct vbitset_t
{
    uint32_t* ptr;
    size_t capacity;
    uint8_t element_size;
    uint8_t elements_per_word; // the amount of elements per word
    uint8_t unused_word_bits; // the amount of bits unused in a word
} VBitSet;

/**
 * Create a new vbitset with malloc.
 * @param capacity The amount of elements that this bitset can hold.
 * @param element_size The amount of bits used by one element.
 */
VBitSet* vbitset_create(size_t capacity, uint8_t element_size);

/**
 * Gets a value from a vbitset.
 * @param bitset The vbitset.
 * @param index The index of the element in the bitset.
 * @returns The value of element at index, with 0 being the default value.
 */
uint8_t vbitset_get(const VBitSet* bitset, size_t index);

/**
 * Set a value in a vbitset.
 * @param bitset The vbitset.
 * @param index The index of the value to set.
 * @param value The value to set at index.
 */
void vbitset_set(const VBitSet* bitset, size_t index, uint8_t value);

/**
 * Frees a vbitset from memory.
 * @param bitset The bitset.
 */
void vbitset_free(VBitSet* bitset);

#endif
