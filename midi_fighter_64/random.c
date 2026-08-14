#include <stdint.h>
#include "random.h"

static uint16_t g_seed16 = 36243;

uint16_t random16(void)
{
    g_seed16 ^= (g_seed16 << 13);
    g_seed16 ^= (g_seed16 >> 9);
    g_seed16 ^= (g_seed16 << 7);
    return g_seed16;
}
