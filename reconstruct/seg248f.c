/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 248f**, image
 * 0x248f0..0x26190. The segment was found by looking at where the binary's own
 * far calls land: 140 of them carry the segment 0x248f, and nothing between
 * 0x248f0 and the sound module at 0x26190 is reached any other way. Functions
 * are in address order and each carries the image offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x24f72
 *
 * NOT TRANSCRIBED YET. The start-up calls it with "cp.bmp" and "gp_bord.bmp",
 * keeping the answers at DGROUP 0x52f4 and 0x4ecb.
 */
uint16_t sub_24f72(uint16_t name)
{
    (void)name;
    not_transcribed("0x24f72");
    return 0;
}

/*
 * 0x252b4
 *
 * Walk a null-terminated array of near pointers and write the same word into
 * each target's +4.
 *
 * The array is the second argument and the word the first, which is the order
 * the compiler pushed them and not the order it reads them.
 */
void set_field_4_of_each(uint16_t value, uint16_t list)
{
    while (DGU16(list) != 0) {
        DG16((uint16_t)(DGU16(list) + 4)) = (int16_t)value;
        list = (uint16_t)(list + 2);
    }
}

/*
 * 0x252e0
 *
 * Count the entries in a null-terminated array of words. A null array answers
 * 0 without looking at it, which is what the test before the loop is for.
 */
uint16_t count_list(uint16_t list)
{
    uint16_t n = 0;

    if (list == 0)
        return 0;

    while (DGU16((uint16_t)(list + 2 * n)) != 0)
        n++;

    return n;
}

/*
 * 0x25d96
 *
 * A far block move, destination first: words then a trailing byte, the odd
 * count carried out of `shr cx,1` in the carry flag.
 *
 * This is the third routine in the port that does this - `far_memcpy` at
 * 0x222c6 and `far_move` at 0x0bd2e are the others - and all three differ in
 * argument order or in which register holds the count. They are separate
 * routines in the original and stay separate here.
 */
void far_copy(uint16_t dst_off, uint16_t dst_seg, uint16_t src_off,
              uint16_t src_seg, uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        *FAR_PTR(dst_seg, (uint16_t)(dst_off + i)) =
            *FAR_PTR(src_seg, (uint16_t)(src_off + i));
}
