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
