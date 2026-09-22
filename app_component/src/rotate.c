#include "rotate.h"

u32 rot_l_4( u32 x )
{
    x &= WIDTH_MASK;
    return ( ( x << 1U ) | ( x >> ( WIDTH - 1U ) ) ) & WIDTH_MASK;
}

u32 rot_r_4( u32 x )
{
    x &= WIDTH_MASK;
    return ( ( x >> 1U ) | ( x << ( WIDTH - 1U ) ) ) & WIDTH_MASK;
}
