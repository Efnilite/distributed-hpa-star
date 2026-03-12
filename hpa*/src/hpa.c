#include "hpa.h"

#include <time.h>

Result hpa(const Map* map, const int16_t sx, const int16_t sy, const int16_t gx, const int16_t gy)
{
    const clock_t begin = clock();

    return (Result){NULL, NULL, false, (double)(clock() - begin) / CLOCKS_PER_SEC};
}
