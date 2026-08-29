/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * Borland's **huge-pointer arithmetic**, image 0x0be82..0x0bfa0 or so. Hand-
 * written assembly with register arguments, called through `lcall 0,...` from
 * the game's own code.
 *
 * Unlike the near heap in borland_heap.c, this is **not** a runtime detail the
 * port could replace. A huge pointer is kept *normalised* - the offset is
 * always 0..15 and everything above it lives in the segment - and the game
 * stores those pointers, compares them, and hands them back to the runtime. Two
 * far pointers that address the same byte are equal only if both are
 * normalised the same way, so the exact arithmetic here is part of the data
 * format and not an implementation choice.
 *
 * The family has a shape worth knowing before reading any of it. Each operation
 * exists twice, as an add and a subtract sharing one body, and the second entry
 * begins `pop es / push cs / push es`: the caller made a **near** call, so that
 * turns its two-byte return address into the four-byte one the `retf` at the
 * end expects. Only the entries the game reaches are transcribed.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x0be82
 *
 * `*var += delta`, normalised, answering the new value. `DX:AX` is the address
 * *of* the pointer variable and `CX:BX` the signed 32-bit delta.
 *
 * A negative delta is negated and sent to the subtract half, which is the same
 * code with the signs turned round - so this one routine is both, and the near
 * entry at 0x0bec3 is the other way in.
 *
 * The normalisation is the last four instructions of either half and is worth
 * reading slowly: the low nibble of the summed offset is kept, everything above
 * it is shifted down by four and added to the segment. `add dh,ch` is an
 * **8-bit** add into the high byte of the segment and wraps there; it is
 * transcribed as such.
 */
uint32_t huge_add_to(uint16_t var_off, uint16_t var_seg, int32_t delta)
{
    uint8_t *var = FAR_PTR(var_seg, var_off);
    uint16_t seg = *(uint16_t *)(var + 2);
    uint16_t off = *(uint16_t *)var;
    uint16_t lo  = (uint16_t)delta;
    uint16_t hi  = (uint16_t)((uint32_t)delta >> 16);
    uint16_t keep;

    if ((int16_t)hi < 0) {
        uint32_t neg = (uint32_t)(-(int32_t)((uint32_t)hi << 16 | lo));

        lo = (uint16_t)neg;
        hi = (uint16_t)(neg >> 16);

        if ((uint32_t)off < (uint32_t)lo)
            seg = (uint16_t)(seg - 0x1000);
        off = (uint16_t)(off - lo);

        seg = (uint16_t)(seg - (uint16_t)((hi & 0xff) << 12));
    } else {
        if ((uint32_t)off + lo > 0xffff)
            seg = (uint16_t)(seg + 0x1000);
        off = (uint16_t)(off + lo);

        seg = (uint16_t)((seg & 0x00ff)
                         | (uint16_t)(((seg >> 8)
                                       + (uint8_t)((hi & 0xff) << 4)) << 8));
    }

    keep = (uint16_t)(off & 0xf);
    seg = (uint16_t)(seg + (off >> 4));

    *(uint16_t *)var = keep;
    *(uint16_t *)(var + 2) = seg;

    return ((uint32_t)seg << 16) | keep;
}

/*
 * 0x0bf0a
 *
 * `p + delta`, normalised, without a variable to write back to: the pointer
 * arrives in `DX:AX` and the signed 32-bit delta in `CX:BX`, and the answer
 * comes back in `DX:AX`.
 *
 * The same two halves and the same normalisation as 0x0be82. The near entry
 * that subtracts is 0x0bf36.
 */
uint32_t huge_add(uint16_t off, uint16_t seg, int32_t delta)
{
    uint16_t lo = (uint16_t)delta;
    uint16_t hi = (uint16_t)((uint32_t)delta >> 16);
    uint16_t keep;

    if ((int16_t)hi < 0) {
        uint32_t neg = (uint32_t)(-(int32_t)((uint32_t)hi << 16 | lo));

        lo = (uint16_t)neg;
        hi = (uint16_t)(neg >> 16);

        if ((uint32_t)off < (uint32_t)lo)
            seg = (uint16_t)(seg - 0x1000);
        off = (uint16_t)(off - lo);

        seg = (uint16_t)(seg - (uint16_t)((hi & 0xff) << 12));
    } else {
        if ((uint32_t)off + lo > 0xffff)
            seg = (uint16_t)(seg + 0x1000);
        off = (uint16_t)(off + lo);

        seg = (uint16_t)((seg & 0x00ff)
                         | (uint16_t)(((seg >> 8)
                                       + (uint8_t)((hi & 0xff) << 4)) << 8));
    }

    keep = (uint16_t)(off & 0xf);
    seg = (uint16_t)(seg + (off >> 4));

    return ((uint32_t)seg << 16) | keep;
}

/*
 * 0x0bf6a
 *
 * `(*var)++` by an unsigned 16-bit amount, answering the value the variable
 * **had**. The variable is at `ES:BX` and the amount in `AX`.
 *
 * The incoming `DX` is not read - it is overwritten by `mov dx,ax` before
 * anything looks at it - so this entry takes a 16-bit step however wide the
 * caller's type is. That is not an oversight to correct: 0x0bf0a is the entry
 * for a 32-bit one.
 *
 * The old value comes out of two `xchg`s rather than a saved copy, which is why
 * there is no spare register in the routine at all.
 */
uint32_t huge_post_add(uint16_t var_off, uint16_t var_seg, uint16_t inc)
{
    uint8_t *var = FAR_PTR(var_seg, var_off);
    uint16_t old_off = *(uint16_t *)var;
    uint16_t old_seg = *(uint16_t *)(var + 2);
    uint32_t sum = (uint32_t)inc + old_off;
    uint16_t seg = (uint16_t)((uint16_t)sum >> 4);

    seg = (uint16_t)(seg + old_seg);
    if (sum > 0xffff)
        seg = (uint16_t)(seg + 0x1000);

    *(uint16_t *)var = (uint16_t)(sum & 0xf);
    *(uint16_t *)(var + 2) = seg;

    return ((uint32_t)old_seg << 16) | old_off;
}
