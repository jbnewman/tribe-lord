#include "random.h"

// Unsigned on purpose: libtonc's qran() overflows a signed int, which GCC is allowed to miscompile.
static unsigned int seed = 42;

void random_seed(unsigned int value)
{
    seed = value;
}

int random_range(int min, int max)
{
    seed = seed * 1664525u + 1013904223u;
    unsigned int value = (seed >> 16) & 0x7FFF;
    return min + (int)((value * (unsigned int)(max - min)) >> 15);
}
