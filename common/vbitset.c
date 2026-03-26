#include "vbitset.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VBitSet* vbitset_create_with_default(const size_t capacity, const uint8_t element_size, const int32_t default_value)
{
    // uses uint32 so max size is 32
    assert(element_size <= 32);

    const size_t elements_per_word = 32 / element_size;
    const size_t words = capacity / elements_per_word + 1;
    const size_t size = sizeof(VBitSet) + words * sizeof(uint32_t);

    VBitSet* bitset = malloc(size);
    if (bitset == NULL)
    {
        perror("Failed to malloc bitset");
        return NULL;
    }
    bitset->ptr = (uint32_t*)(bitset + 1);
    memset(bitset->ptr, default_value, words * sizeof(uint32_t));
    bitset->capacity = capacity;
    bitset->element_size = element_size;
    bitset->elements_per_word = 32 / element_size;
    bitset->unused_word_bits = 32 % element_size;

    return bitset;
}

VBitSet* vbitset_create(const size_t capacity, const uint8_t element_size)
{
    return vbitset_create_with_default(capacity, element_size, 0);
}

unsigned int vbitset_get(const VBitSet* bitset, const size_t index)
{
    const size_t size = bitset->element_size;
    // get index accounting for unused bits in word
    const size_t element_index = index / bitset->elements_per_word * bitset->unused_word_bits + size * index;
    const size_t word_index = element_index >> 5;
    const uint8_t in_page = 31 - element_index % 32;
    const uint32_t word = bitset->ptr[word_index];

    unsigned int value = 0;
    for (unsigned int i = 0; i < size; ++i)
    {
        const uint32_t bit = (word & (uint32_t)1 << (in_page - i)) >> (in_page - i);
        value |= bit << i;
    }
    return value;
}

void vbitset_set(const VBitSet* bitset, const size_t index, const unsigned int value)
{
    const size_t size = bitset->element_size;
    // get index accounting for unused bits in word
    const size_t element_index = index / bitset->elements_per_word * bitset->unused_word_bits + size * index;
    const size_t word_index = element_index >> 5;
    const uint8_t in_page = 31 - element_index % 32;
    uint32_t word = bitset->ptr[word_index];

    for (unsigned int i = 0; i < bitset->element_size; ++i)
    {
        const uint32_t value_bit = 1 & value >> i;
        word = word & ~((uint32_t)1 << (in_page - i)) | (uint32_t)value_bit << (in_page - i);
    }

    bitset->ptr[word_index] = word;
}

void vbitset_free(VBitSet* bitset) { free(bitset); }
