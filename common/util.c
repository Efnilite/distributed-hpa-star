#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long util_get_memory_usage(void)
{
    FILE* file = fopen("/proc/self/status", "r");
    if (file == NULL)
    {
        return -1;
    }

    char line[256];
    long rss_kb = -1;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Look for VmRSS line: "VmRSS:     12345 kB"
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            // Parse the value in KB
            if (sscanf(line, "VmRSS: %ld", &rss_kb) == 1)
            {
                // Convert KB to bytes
                fclose(file);
                return rss_kb * 1024;
            }
        }
    }

    fclose(file);
    return -1;
}

long get_memory_usage(long existing) {
    long current_memory = util_get_memory_usage();
    // If unable to read current memory (returns -1), return existing max
    if (current_memory < 0)
        return existing;
    // Return the maximum of current and existing
    if (current_memory > existing)
        return current_memory;
    return existing;
}   