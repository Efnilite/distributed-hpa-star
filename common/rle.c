#include "rle.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RLEBitSet* rlebitset_create_with_default(const size_t capacity, const uint8_t element_size, const int32_t default_value)
{
    // element size must fit in at least 1 bit
    assert(element_size > 0 && element_size <= 32);

    RLEBitSet* bitset = malloc(sizeof(RLEBitSet));
    if (bitset == NULL)
    {
        perror("Failed to malloc bitset");
        return NULL;
    }

    bitset->capacity = capacity;
    bitset->element_size = element_size;
    bitset->total_elements = capacity;
    bitset->run_capacity = 64; // start with initial capacity
    bitset->run_count = 0;

    bitset->runs = malloc(bitset->run_capacity * sizeof(RLEBitSetRun));
    if (bitset->runs == NULL)
    {
        perror("Failed to malloc runs array");
        free(bitset);
        return NULL;
    }

    // Initialize with a single run of the default value spanning the entire capacity
    if (capacity > 0)
    {
        bitset->runs[0].value = (unsigned int)default_value & ((1U << element_size) - 1);
        bitset->runs[0].count = capacity;
        bitset->run_count = 1;
    }

    return bitset;
}

RLEBitSet* rlebitset_create(const size_t capacity, const uint8_t element_size)
{
    return rlebitset_create_with_default(capacity, element_size, 0);
}

unsigned int rlebitset_get(const RLEBitSet* bitset, const size_t index)
{
    if (index >= bitset->total_elements) {
        return 1;
    }
    assert(index < bitset->total_elements);

    size_t current_pos = 0;
    for (size_t i = 0; i < bitset->run_count; ++i)
    {
        const size_t run_end = current_pos + bitset->runs[i].count;
        if (index < run_end)
        {
            return bitset->runs[i].value;
        }
        current_pos = run_end;
    }

    // Should not reach here if index is valid
    return 0;
}

void rlebitset_set(RLEBitSet* bitset, const size_t index, const unsigned int value)
{
    assert(index < bitset->total_elements);

    const unsigned int masked_value = value & ((1U << bitset->element_size) - 1);

    // Find the run containing this index
    size_t current_pos = 0;
    for (size_t i = 0; i < bitset->run_count; ++i)
    {
        const size_t run_end = current_pos + bitset->runs[i].count;
        if (index < run_end)
        {
            // Index is in this run
            if (bitset->runs[i].value == masked_value)
            {
                // Value already matches, nothing to do
                return;
            }

            const size_t offset_in_run = index - current_pos;
            const size_t before_count = offset_in_run;
            const size_t after_count = bitset->runs[i].count - offset_in_run - 1;

            // Calculate how many new runs we'll need
            int new_runs_needed = 0;
            if (before_count > 0) new_runs_needed++;
            new_runs_needed++; // the changed element
            if (after_count > 0) new_runs_needed++;
            int net_new_runs = new_runs_needed - 1; // we're replacing 1 run

            // Expand array if needed
            if (bitset->run_count + net_new_runs > bitset->run_capacity)
            {
                bitset->run_capacity = (bitset->run_count + net_new_runs) * 2;
                RLEBitSetRun* new_runs = realloc(bitset->runs, bitset->run_capacity * sizeof(RLEBitSetRun));
                if (new_runs == NULL)
                {
                    perror("Failed to realloc runs array");
                    return;
                }
                bitset->runs = new_runs;
            }

            const unsigned int original_value = bitset->runs[i].value;

            // Shift runs after position i to make space
            if (net_new_runs > 0)
            {
                memmove(&bitset->runs[i + new_runs_needed], &bitset->runs[i + 1],
                        (bitset->run_count - i - 1) * sizeof(RLEBitSetRun));
            }

            size_t write_pos = i;

            // Write "before" run if needed
            if (before_count > 0)
            {
                bitset->runs[write_pos].value = original_value;
                bitset->runs[write_pos].count = before_count;
                write_pos++;
            }

            // Write the single changed element
            bitset->runs[write_pos].value = masked_value;
            bitset->runs[write_pos].count = 1;
            write_pos++;

            // Write "after" run if needed
            if (after_count > 0)
            {
                bitset->runs[write_pos].value = original_value;
                bitset->runs[write_pos].count = after_count;
                write_pos++;
            }

            bitset->run_count += net_new_runs;
            return;
        }
        current_pos = run_end;
    }
}

void rlebitset_set_range(RLEBitSet* bitset, size_t start_index, size_t count, unsigned int value)
{
    assert(start_index < bitset->total_elements);
    assert(start_index + count <= bitset->total_elements);

    if (count == 0)
        return;

    const size_t end_index = start_index + count - 1;
    const unsigned int masked_value = value & ((1U << bitset->element_size) - 1);

    // Find the run containing start_index
    size_t current_pos = 0;
    size_t start_run_idx = 0;
    for (size_t i = 0; i < bitset->run_count; ++i)
    {
        const size_t run_end = current_pos + bitset->runs[i].count;
        if (start_index < run_end)
        {
            start_run_idx = i;
            break;
        }
        current_pos = run_end;
    }

    // Find the run containing end_index
    current_pos = 0;
    size_t end_run_idx = 0;
    for (size_t i = 0; i < bitset->run_count; ++i)
    {
        const size_t run_end = current_pos + bitset->runs[i].count;
        if (end_index < run_end)
        {
            end_run_idx = i;
            break;
        }
        current_pos = run_end;
    }

    // Calculate the position where the range starts within its run
    current_pos = 0;
    for (size_t i = 0; i < start_run_idx; ++i)
        current_pos += bitset->runs[i].count;
    const size_t start_offset_in_run = start_index - current_pos;

    // Calculate the position where the range ends within its run
    current_pos = 0;
    for (size_t i = 0; i < end_run_idx; ++i)
        current_pos += bitset->runs[i].count;
    const size_t end_offset_in_run = end_index - current_pos;

    const unsigned int start_run_original_value = bitset->runs[start_run_idx].value;
    const unsigned int end_run_original_value = bitset->runs[end_run_idx].value;
    const size_t start_run_before_count = start_offset_in_run;
    const size_t end_run_after_count = bitset->runs[end_run_idx].count - end_offset_in_run - 1;

    // Calculate how many runs we'll have after the operation
    int runs_to_remove = end_run_idx - start_run_idx + 1;
    int runs_to_insert = 1; // the new range run
    if (start_run_before_count > 0) runs_to_insert++;
    if (end_run_after_count > 0) runs_to_insert++;
    int net_change = runs_to_insert - runs_to_remove;

    // Expand array if needed
    if ((int)bitset->run_count + net_change > (int)bitset->run_capacity)
    {
        bitset->run_capacity = (bitset->run_count + net_change) * 2;
        RLEBitSetRun* new_runs = realloc(bitset->runs, bitset->run_capacity * sizeof(RLEBitSetRun));
        if (new_runs == NULL)
        {
            perror("Failed to realloc runs array");
            return;
        }
        bitset->runs = new_runs;
    }

    // Shift runs after end_run_idx to make space (or remove if negative)
    if (net_change != 0)
    {
        memmove(&bitset->runs[start_run_idx + runs_to_insert],
                &bitset->runs[end_run_idx + 1],
                (bitset->run_count - end_run_idx - 1) * sizeof(RLEBitSetRun));
    }

    size_t write_pos = start_run_idx;

    // Write "before" run if needed
    if (start_run_before_count > 0)
    {
        bitset->runs[write_pos].value = start_run_original_value;
        bitset->runs[write_pos].count = start_run_before_count;
        write_pos++;
    }

    // Write the range run
    bitset->runs[write_pos].value = masked_value;
    bitset->runs[write_pos].count = count;
    write_pos++;

    // Write "after" run if needed
    if (end_run_after_count > 0)
    {
        bitset->runs[write_pos].value = end_run_original_value;
        bitset->runs[write_pos].count = end_run_after_count;
        write_pos++;
    }

    bitset->run_count = write_pos + (bitset->run_count - end_run_idx - 1);
}

void rlebitset_free(RLEBitSet* bitset)
{
    if (bitset != NULL)
    {
        free(bitset->runs);
        free(bitset);
    }
}
