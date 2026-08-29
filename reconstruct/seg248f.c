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
 * 0x252d0
 *
 * A thunk from this module into `free_bitmaps` in segment 1c25, which is a
 * `push`, an `lcall` and nothing else. It exists because the two are different
 * translation units and the call has to be far.
 */
void free_bitmaps_thunk(uint16_t list)
{
    free_bitmaps(list);
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
 * 0x2551a
 *
 * Read a 32-bit count of bytes from a file into a far destination, through a
 * bounce buffer, because the read below it takes a **near** buffer and a
 * 16-bit count.
 *
 * The buffer is as big as the near heap will give it: it asks for 0x4000 and
 * halves down to 0x800, then steps down by 0x100 at a time, and if even that
 * fails it falls back to 0x100 bytes of its own stack. So a machine with a full
 * heap reads in small pieces rather than failing.
 *
 * The destination is renormalised every 0x10000/buffer reads - which is what
 * the divide at the top is for - by adding a whole segment to the pointer and
 * starting the offset again, so a destination longer than 64 KB is written
 * without the offset ever wrapping.
 *
 * A short read ends it, whatever the count still says. A **near** routine.
 */
void read_far(uint16_t dst_off, uint16_t dst_seg,
              uint16_t count_lo, uint16_t count_hi, uint16_t file)
{
    uint16_t fp = dg_enter(0x10a);
    uint16_t fallback = fp;             /* [bp-0x10a], 0x100 bytes */

    uint16_t buf;                       /* [bp-6]   */
    int16_t si = 0x4000;
    int16_t per_segment;                /* [bp-8]   */
    int16_t left_in_segment;            /* [bp-0xa] */
    uint16_t walk_off, walk_seg;        /* [bp-4], [bp-2] */
    uint16_t ptr_off = dst_off, ptr_seg = dst_seg;
    uint32_t remaining = ((uint32_t)count_hi << 16) | count_lo;

    for (;;) {
        if (si == 0)
            break;
        buf = heap_malloc_far((uint16_t)si);
        if (buf != 0)
            break;
        if (si > 0x800)
            si = (int16_t)(si >> 1);
        else
            si = (int16_t)(si - 0x100);
    }

    if (si == 0) {
        buf = fallback;
        si = 0x100;
    }

    per_segment = (count_hi != 0)
                  ? (int16_t)long_divide(0x00010000L, si)
                  : 0;
    left_in_segment = per_segment;

    walk_seg = ptr_seg;
    walk_off = ptr_off;

    while (remaining != 0) {
        uint16_t want = (uint16_t)(((int32_t)si <= (int32_t)remaining)
                                   ? (uint16_t)si : (uint16_t)remaining);
        uint16_t got = game_fread(buf, 1, want, file);

        if (got == 0)
            break;

        far_copy(walk_off, walk_seg, buf, DGROUP_SEG, got);

        walk_off = (uint16_t)(walk_off + got);
        remaining -= got;

        if (per_segment != 0 && --left_in_segment == 0) {
            uint16_t fp2 = dg_enter(4);

            DGU16(fp2) = ptr_off;
            DGU16((uint16_t)(fp2 + 2)) = ptr_seg;
            huge_add_to(fp2, DGROUP_SEG, 0x00010000L);
            ptr_off = DGU16(fp2);
            ptr_seg = DGU16((uint16_t)(fp2 + 2));
            dg_leave(4);

            left_in_segment = per_segment;
            walk_seg = ptr_seg;
            walk_off = ptr_off;
        }
    }

    if (buf != 0 && buf != fallback)
        heap_free_far(buf);

    dg_leave(0x10a);
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
