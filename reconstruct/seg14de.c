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
