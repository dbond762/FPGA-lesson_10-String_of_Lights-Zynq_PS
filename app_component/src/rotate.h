#ifndef ROTATE_H
#define ROTATE_H

#include "xil_types.h"

#define WIDTH      4U
#define WIDTH_MASK ( ( 1U << WIDTH ) - 1U )   // 0xF

/* Циклічний зсув вліво на 1 біт в межах 4 біт: 1000 -> 0001 */
u32 rot_l_4( u32 x );

/* Циклічний зсув вправо на 1 біт в межах 4 біт: 0001 -> 1000 */
u32 rot_r_4( u32 x );

#endif // ROTATE_H