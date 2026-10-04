/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * Borland's **long arithmetic** helpers the game's code still calls by name.
 *
 * The huge-pointer family that lived here - `F_PADA@`, `F_PSBA@`, `F_PADD@`,
 * `F_PCMP@` and the post-add, image 0x0bd0d and 0x0be82..0x0bfa0 - went on
 * 2026-09-27. They are what Turbo C++ emits for `huge` pointer arithmetic
 * and links from CM.LIB by itself: the game's source says `p += n` on a
 * `huge` pointer and the compiler calls them, and on the host the same line
 * is plain pointer arithmetic. Nothing in the port called them any more.
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x0bcf6
 *
 * A 32-bit multiply, DX:AX times CX:BX, answered in DX:AX - Borland's `__LMUL`,
 * and the **second copy of it in the image**. The other is at 0x0c16e and is
 * the same instructions; the linker pulled the routine in twice because two of
 * its library modules wanted it, and neither call site can tell.
 */
uint32_t long_multiply_2(uint32_t a, uint32_t b)
{
    return a * b;
}


/*
 * 0x0be62
 *
 * A **signed** 32-bit right shift, DX:AX by CL, answering DX:AX. Borland's, and
 * a far routine.
 *
 * Under sixteen it shifts each half and then rotates the bits that fell out of
 * the high half into the top of the low one, which is what the `neg cl / add
 * cl,0x10` is for. Sixteen or more it moves the high half down into AX, sign
 * extends with `cwd`, and shifts what is left.
 *
 * A count of zero takes the first path and ends up shifting BX by sixteen,
 * which on this machine is zero, so nothing is rotated in and the value comes
 * back unchanged.
 */
int32_t long_shift_right(int32_t v, uint8_t count)
{
    if (count >= 32)
        return v < 0 ? -1 : 0;

    return (int32_t)(v >> count);
}

