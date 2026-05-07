#ifndef RLEBITSET_H
#define RLEBITSET_H

#include <stddef.h>
#include <stdint.h>

/**
 * A run-length encoded bitset struct for efficient storage of repetitive data.
 */
typedef struct
{
    unsigned int value;
    size_t count;
} RLEBitSetRun;

typedef struct rlebitset_t
{
    RLEBitSetRun* runs;
    size_t run_count;
    size_t run_capacity;
    size_t total_elements;
    size_t capacity;
    uint8_t element_size;
} RLEBitSet;

/**
 * Create a new vbitset with malloc and a default value.
 * @param capacity The amount of elements that this bitset can hold.
 * @param element_size The amount of bits used by one element.
 * @param default_value The default value to set all words to.
 */
RLEBitSet* rlebitset_create_with_default(size_t capacity, uint8_t element_size, int32_t default_value);

/**
 * Create a new vbitset with malloc.
 * @param capacity The amount of elements that this bitset can hold.
 * @param element_size The amount of bits used by one element.
 */
RLEBitSet* rlebitset_create(size_t capacity, uint8_t element_size);

/**
 * Gets a value from a vbitset.
 * @param bitset The vbitset.
 * @param index The index of the element in the bitset.
 * @returns The value of element at index, with 0 being the default value.
 */
unsigned int rlebitset_get(const RLEBitSet* bitset, size_t index);

/**
 * Set a value in a vbitset.
 * @param bitset The vbitset.
 * @param index The index of the value to set.
 * @param value The value to set at index.
 */
void rlebitset_set(RLEBitSet* bitset, size_t index, unsigned int value);

/**
 * Set a range of consecutive values in a vbitset.
 * @param bitset The vbitset.
 * @param start_index The starting index.
 * @param count The number of consecutive elements to set.
 * @param value The value to set for all elements in the range.
 */
void rlebitset_set_range(RLEBitSet* bitset, size_t start_index, size_t count, unsigned int value);

/**
 * Frees a vbitset from memory.
 * @param bitset The bitset.
 */
void rlebitset_free(RLEBitSet* bitset);

#endif
