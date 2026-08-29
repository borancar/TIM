#include <stdio.h>
/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 14de**, image
 * 0x14de0..0x1c250. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x1405b
 *
 * Build the list of parts a level may use, and reset the machine's state around
 * it: the list head at DGROUP 0x50d7, the two pairs at 0x5179 and 0x521b, the
 * play area at 0x50af..0x50b5, and the two at 0x4ead.
 *
 * Parts 0 to 0x32 are all included except in three cases. **0x14, 0x29 and 0x31
 * are never included**, and are excluded by falling into a branch that leaves
 * the flag clear rather than by being tested against a list. And **0x20, 0x21
 * and 0x22 are conditional**, each on its own word - 0x4e7d, 0x4e81 and 0x4e7b -
 * which is what makes three of the parts appear only when the game says so.
 *
 * The three conditionals are written as three independent `if`s inside the same
 * branch rather than as a switch, so a part number that is not one of the three
 * reaches the end of them with its flag still clear and is left out too - which
 * cannot happen, because only those three get in there.
 *
 * The play area is 0x43,0x110 to -8,-8 - the negative pair being the origin
 * rather than a size, which is worth saying because it reads like a mistake.
 */
void build_part_list(void)
{
    int16_t si;

    DGU16(0x50d9) = 0;
    DGU16(0x50d7) = 0;
    DGU16(0x517b) = 0;
    DGU16(0x5179) = 0;
    DGU16(0x521d) = 0;
    DGU16(0x521b) = 0;

    for (si = 0; si < 0x33; si++) {
        int16_t wanted = 0;

        if (si == 0x20 || si == 0x21 || si == 0x22) {
            if (si == 0x20 && DGU16(0x4e7d) != 0)
                wanted = 1;
            if (si == 0x21 && DGU16(0x4e81) != 0)
                wanted = 1;
            if (si == 0x22 && DGU16(0x4e7b) != 0)
                wanted = 1;
        } else if (si != 0x14 && si != 0x29 && si != 0x31) {
            wanted = 1;
        }

        if (wanted != 0) {
            uint16_t rec = make_part((uint16_t)si);

            if (rec != 0)
                insert_sorted(rec, 0x50d7);
        }
    }

    DGU16(0x50d3) = 0x50d7;
    DGU16(0x50b1) = 0;
    DGU16(0x50af) = 0;
    DGU16(0x50b3) = 0x43;
    DGU16(0x50b5) = 0x110;
    DG16(0x50b9) = -8;
    DG16(0x50b7) = -8;
    DGU16(0x50bb) = 0x3e9;
    DGU16(0x4eaf) = 0;
    DGU16(0x4ead) = 0;

    recompute_kind_physics();
}

/*
 * 0x14236 .. 0x14c8x - the **part initialisers**, forty-two of forty-eight.
 *
 * Every one of these is the same four steps, and the table at DGROUP 0x2966
 * points each part at its own:
 *
 *   1. OR some bits into the part's flags at +6, +8 and +0x0a, if it has any;
 *   2. take four bytes per bitmap - `heap_calloc_far(count, 4)` - into +0x82;
 *   3. refuse, by answering 1, if that allocation failed;
 *   4. call the part's own setup in segment 0x172c, and answer 0.
 *
 * They are transcribed as the table they are, because that is what they are:
 * the only things that differ between them are three flag words and which setup
 * is called. Every value here was read out of the image at the address in the
 * first column, and `part_init` dispatches on that address - `call_part_init`
 * in io.c reaches it because the original arrives through a relocated far
 * pointer the port has no way to call.
 *
 * **Six of the forty-eight are not in here** and are written out separately:
 * 0x143fb, 0x1443d, 0x1449d, 0x14aa2, 0x14c48 and 0x14c62 do something else
 * with their allocation and are not this routine with different constants.
 */
static const struct {
    uint32_t at;                /* an image address: it does not fit in 16 */
    uint16_t flags6;
    uint16_t flags8;
    uint16_t flags10;
    uint16_t setup;
} part_inits[42] = {
    /*    at       +6      +8      +0a     setup */
    { 0x14236, 0x0000, 0x0000, 0x0000, 0x0001 },
    { 0x14267, 0x0040, 0x0180, 0x0000, 0x48ab },
    { 0x142a1, 0x0600, 0x0080, 0x0000, 0x2728 },
    { 0x142e6, 0x0400, 0x000c, 0x0000, 0x40f0 },
    { 0x14320, 0x0020, 0x0004, 0x0000, 0x012d },
    { 0x14361, 0x0000, 0x0081, 0x0000, 0x24d0 },
    { 0x143b3, 0x0400, 0x0801, 0x0000, 0x2ee1 },
    { 0x1446c, 0x0000, 0x0000, 0x0000, 0x0001 },
    { 0x144cb, 0x0020, 0x0004, 0x0000, 0x0f70 },
    { 0x1450c, 0x0400, 0x8000, 0x0000, 0x0c1c },
    { 0x14547, 0x0400, 0x1001, 0x0000, 0x295d },
    { 0x1458f, 0x0000, 0x0001, 0x0000, 0x0001 },
    { 0x145d1, 0x0000, 0x1000, 0x0000, 0x1be9 },
    { 0x14607, 0x0400, 0x0000, 0x0000, 0x0371 },
    { 0x1463d, 0x0020, 0x0004, 0x0000, 0x07b2 },
    { 0x1467e, 0x0400, 0x1000, 0x0004, 0x0b88 },
    { 0x146bd, 0x0420, 0x1000, 0x0004, 0x1261 },
    { 0x146fc, 0x0000, 0x0000, 0x0000, 0x08a1 },
    { 0x1472d, 0x0200, 0x1000, 0x0002, 0x1556 },
    { 0x1476c, 0x0400, 0x1004, 0x0000, 0x3294 },
    { 0x147a7, 0x0200, 0x0004, 0x0000, 0x19db },
    { 0x147c5, 0x0400, 0x1000, 0x0001, 0x1a32 },
    { 0x14804, 0x0400, 0x0000, 0x0000, 0x1d28 },
    { 0x1483a, 0x0000, 0x1001, 0x0002, 0x1dfb },
    { 0x14874, 0x0400, 0x1004, 0x0000, 0x23b1 },
    { 0x148af, 0x0000, 0x0000, 0x0000, 0x00c9 },
    { 0x148e0, 0x0200, 0x1004, 0x0000, 0x2b58 },
    { 0x148ff, 0x0400, 0x0000, 0x0000, 0x3030 },
    { 0x14919, 0x0400, 0x1805, 0x0000, 0x2cce },
    { 0x14954, 0x0000, 0x0000, 0x0000, 0x35f4 },
    { 0x14985, 0x0020, 0x0004, 0x0000, 0x2682 },
    { 0x149c6, 0x0000, 0x0000, 0x0000, 0x1075 },
    { 0x149f7, 0x0400, 0x0000, 0x0000, 0x065b },
    { 0x14a2d, 0x0000, 0x1000, 0x0004, 0x3737 },
    { 0x14a67, 0x0400, 0x1000, 0x0000, 0x389b },
    { 0x14ab9, 0x0000, 0x1000, 0x0000, 0x3f72 },
    { 0x14aef, 0x0400, 0x0801, 0x0000, 0x496f },
    { 0x14b37, 0x0400, 0x8000, 0x0000, 0x346f },
    { 0x14b72, 0x0000, 0x0000, 0x0000, 0x0065 },
    { 0x14ba3, 0x0000, 0x0000, 0x0000, 0x00c9 },
    { 0x14bd4, 0x0020, 0x1000, 0x0004, 0x0950 },
    { 0x14c12, 0x0600, 0x0000, 0x0000, 0x377b },
};

/*
 * 0x14236
 *
 * Run one part's initialiser, found by its own address. Answers 1 when the
 * per-bitmap slots could not be allocated, which is what makes `make_part`
 * throw the part away.
 */
uint16_t part_init(uint32_t at, uint16_t part)
{
    int32_t i;

    for (i = 0; i < 42; i++) {
        if (part_inits[i].at != at)
            continue;

        if (part_inits[i].flags6 != 0)
            DGU16((uint16_t)(part + 6)) =
                (uint16_t)(DGU16((uint16_t)(part + 6)) | part_inits[i].flags6);
        if (part_inits[i].flags8 != 0)
            DGU16((uint16_t)(part + 8)) =
                (uint16_t)(DGU16((uint16_t)(part + 8)) | part_inits[i].flags8);
        if (part_inits[i].flags10 != 0)
            DGU16((uint16_t)(part + 0x0a)) =
                (uint16_t)(DGU16((uint16_t)(part + 0x0a))
                           | part_inits[i].flags10);

        DGU16((uint16_t)(part + 0x82)) =
            heap_calloc_far(DGU16((uint16_t)(part + 0x80)), 4);

        if (DGU16((uint16_t)(part + 0x82)) == 0)
            return 1;

        part_setup(part_inits[i].setup, part);
        return 0;
    }

    return part_init_special(at, part);
}

/*
 * 0x14133
 *
 * Make one part: a 0xa2-byte record off the near heap, filled from the
 * sixteen-byte-per-part table at DGROUP 0x2966 and the bitmap list
 * `load_part_bitmap` left at 0xeba.
 *
 * The fields that come across are the part's kind at +6, its size at +0xa and
 * +0x50/+0x52, its extent at +0x44/+0x46, its bitmaps at +0x80 and a word at
 * +0x94. The two at +0x8c and +0x8e start at -1 rather than 0, which is what
 * "no link" looks like everywhere else in this game.
 *
 * Each part may also have an **init function** in the table, at +12 of its
 * entry, and a part that answers 1 from it is refused - the record is freed and
 * the answer is 0. The port dispatches that far pointer on its value, as it
 * does everywhere else it cannot call one.
 *
 * The heap is checked three times: before the allocation, after it, and at the
 * end.
 */
uint16_t make_part(uint16_t n)
{
    uint16_t si;
    uint16_t bx = (uint16_t)(n << 4);
    int16_t failed = 0;

    heap_check_or_hang();

    si = heap_calloc_far(1, 0xa2);
    if (si == 0) {
        failed = 1;
        goto done;
    }

    heap_check_or_hang();

    DGU16((uint16_t)(si + 4)) = n;
    DGU16((uint16_t)(si + 6)) = DGU16((uint16_t)(bx + 0x2966));
    DGU16((uint16_t)(si + 0x0a)) = DGU16((uint16_t)(bx + 0x2968));
    DGU16((uint16_t)(si + 0x50)) = DGU16((uint16_t)(bx + 0x296a));
    DGU16((uint16_t)(si + 0x52)) = DGU16((uint16_t)(bx + 0x296c));
    DGU16((uint16_t)(si + 0x44)) = DGU16((uint16_t)(bx + 0x296e));
    DGU16((uint16_t)(si + 0x46)) = DGU16((uint16_t)(bx + 0x2970));
    DGU16((uint16_t)(si + 0x80)) =
        DGU16((uint16_t)(n * 0x3a + 0x0ec4));
    DGU16((uint16_t)(si + 0x8c)) = 0xffff;
    DGU16((uint16_t)(si + 0x8e)) = 0xffff;
    DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(bx + 0x2972));

    if ((DGU16((uint16_t)(bx + 0x2972)) | DGU16((uint16_t)(bx + 0x2974))) != 0
        && call_part_init(DGU16((uint16_t)(bx + 0x2972)),
                          DGU16((uint16_t)(bx + 0x2974)), si) == 1) {
        failed = 1;
        goto done;
    }

    DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(si + 8));

    set_object_extent(si);

    DGU16((uint16_t)(si + 0x42)) = DGU16((uint16_t)(si + 0x46));
    DGU16((uint16_t)(si + 0x40)) = DGU16((uint16_t)(si + 0x44));

    heap_check_or_hang();

done:
    if (failed != 0) {
        if (si != 0)
            free_part(si);
        return 0;
    }

    return si;
}

/*
 * 0x14d95
 *
 * NOT TRANSCRIBED YET. Give one part record back.
 */
void free_part(uint16_t part)
{
    (void)part;
    not_transcribed("0x14d95, freeing a part");
}

/*
 * 0x15004
 *
 * NOT TRANSCRIBED YET. Called from the intro.
 */
void sub_15004(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    not_transcribed("0x15004");
}

/*
 * 0x151c8
 *
 * NOT TRANSCRIBED YET. Called from the intro.
 */
void sub_151c8(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    not_transcribed("0x151c8");
}

/*
 * 0x15f76
 *
 * NOT TRANSCRIBED YET. Called from the intro.
 */
void sub_15f76(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e)
{
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    not_transcribed("0x15f76");
}

/*
 * 0x16181
 *
 * NOT TRANSCRIBED YET. Called from the intro.
 */
void sub_16181(uint16_t a)
{
    (void)a;
    not_transcribed("0x16181");
}

/*
 * 0x166d6
 *
 * Clear six words at DGROUP 0x50bf. The loop counts *down* from 5 and tests
 * `jge`, so index 0 is cleared too - six entries, not five. What they hold is
 * not established.
 */
void clear_word_array_50bf(void)
{
    int16_t i = 5;

    do {
        word_array_50bf(i) = 0;
        i--;
    } while (i >= 0);
}

/*
 * 0x166ef
 *
 * Link a record into up to two buckets, and mark it linked.
 *
 * Which buckets is decided by two bytes in the record's kind entry - the same
 * 0x3a-byte table `clamp_record_pair` indexes, read here at +0x1c rather than
 * +0x0a - and a byte of 0xff means "not in this bucket". The bucket heads are
 * the six-word array at DGROUP 0x50bf, which is the array
 * `clear_word_array_50bf` zeroes; that the two routines agree about it is what
 * identifies it as a set of list heads.
 *
 * The insertion is at the head: the record's link at +0x74 (or +0x76 for the
 * second bucket) takes the old head and the head becomes the record. For the
 * first bucket only, the bucket number is also stored at +0x7f.
 *
 * One record is special - the one whose address is at DGROUP 0x50d5 always
 * goes into bucket 0 whatever its kind says.
 */
void link_record_into_buckets(uint16_t rec)
{
    int16_t kind = DG16(rec + 4);
    int16_t i;

    DG16(rec + 0x0A) |= 0x20;

    for (i = 0; i < 2; i++) {
        uint8_t slot = DG8((uint16_t)(0xEC2 + kind * 0x3A + i));

        if (slot == 0xFF)
            continue;
        if (rec == DGU16(0x50D5))
            slot = 0;

        DGU16((uint16_t)(rec + 0x74 + i * 2)) = DGU16(0x50BF + slot * 2);
        DGU16(0x50BF + slot * 2) = rec;
        if (i == 0)
            DG8(rec + 0x7F) = slot;
    }
}

/*
 * OURS: the six initialisers that are not the common one.
 *
 * 0x143fb, 0x1443d, 0x1449d, 0x14aa2, 0x14c48 and 0x14c62 allocate something
 * different and do not call into segment 0x172c at all. They are not
 * transcribed yet, and each aborts naming itself rather than being folded into
 * the table it does not belong in.
 */
uint16_t part_init_special(uint32_t at, uint16_t part)
{
    static char what[64];

    (void)part;
    snprintf(what, sizeof what, "the part initialiser at %#07lx",
             (unsigned long)at);
    not_transcribed(what);
    return 1;
}
