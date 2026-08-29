#include <stdio.h>
/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 172c**, image
 * 0x172c0..0x1c250. Functions are in address order and each carries the image
 * offset it was read from.
 *
 * This is where the parts live: each of the machine's fifty-odd components has
 * its own setup routine here, reached from the initialiser table in seg14de.c
 * through a far pointer the loader relocates.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * The **part setups**: 14 of the 39, as the table they are.
 *
 * Each writes a list of byte pairs into the four-bytes-per-bitmap array the
 * initialiser allocated at +0x82 - an x at +0 and a y at +1 of every slot, the
 * other two bytes left for the angle - and then calls one routine with the
 * part. Those pairs are the part's **connection points**: where a rod or a rope
 * may attach to each of its bitmaps.
 *
 * A byte is either a constant or the part's own extent plus one: `w` and `h`
 * below are the bytes at +0x44 and +0x46, and a setup that says (0,0),
 * (w-1,0), (w-1,h-1), (0,h-1) is naming the four corners of whatever size the
 * part turns out to be. Both kinds are in the same table because they are the
 * same routine written twice, once with numbers and once with the extent.
 *
 * Every value here was read out of the image at the offset in the first column.
 */
#define PS_K 0                  /* a constant */
#define PS_W 1                  /* the part's width at +0x44, plus the addend */
#define PS_H 2                  /* its height at +0x46, plus the addend */

static const struct {
    uint16_t off;
    uint16_t finish;
    uint8_t  n;
    uint8_t  kind[2 * 16];
    int8_t   add[2 * 16];
} part_setups[14] = {
    { 0x0001, 0x05d1e,  8,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 8, 0, 23, 0, 31, 8, 31, 23, 23, 31, 8, 31, 0, 23, 0, 8 } },
    { 0x0065, 0x05d1e,  8,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 7, 0, 15, 0, 22, 8, 22, 15, 14, 22, 8, 22, 0, 15, 0, 8 } },
    { 0x00c9, 0x05d1e,  8,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 3, 0, 11, 0, 14, 4, 14, 10, 11, 14, 3, 14, 0, 10, 0, 4 } },
    { 0x07b2, 0x05d1e,  6,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 19, 10, 40, 25, 40, 36, 20, 27, 47, 8, 47 } },
    { 0x0950, 0x05d1e,  3,
      { 0, 0, 0, 0, 0, 0 },
      { 8, 31, 14, 22, 21, 31 } },
    { 0x0f70, 0x05d1e, 12,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 24, 19, 0, 25, 0, 45, 24, 45, 63, 43, 63, 43, 26, 34, 16, 11, 16, 2, 26, 2, 63, 0, 63 } },
    { 0x24d0, 0x05d1e,  4,
      { 0, 0, 1, 0, 1, 2, 0, 2 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 0x295d, 0x05d1e,  4,
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 31, 0, 31, 31, 0, 31 } },
    { 0x2ee1, 0x05d1e,  4,
      { 0, 0, 1, 0, 1, 2, 0, 2 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 0x346f, 0x05d1e,  5,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 6, 12, 0, 23, 6, 23, 10, 0, 10 } },
    { 0x3737, 0x05d1e,  4,
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 4, 0, 10, 0, 14, 51, 0, 51 } },
    { 0x3f72, 0x05d1e,  4,
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 11, 47, 11, 47, 27, 0, 27 } },
    { 0x48ab, 0x05d1e,  4,
      { 0, 0, 1, 0, 1, 2, 0, 2 },
      { 0, 0, -1, 0, -1, -1, 0, -1 } },
    { 0x496f, 0x05d1e,  3,
      { 0, 0, 0, 0, 0, 0 },
      { 8, 47, 18, 17, 28, 47 } },
};

/*
 * NOT a transcription: reach one part's setup by its offset in this segment.
 *
 * The original arrives by `lcall` through a relocated far pointer, which the
 * port cannot do, so the offset is dispatched here the same way the region and
 * timer handlers are. An offset with no row yet aborts and names itself.
 */
void part_setup(uint16_t off, uint16_t part)
{
    int32_t i;

    for (i = 0; i < 14; i++) {
        uint16_t si;
        int32_t k;

        if (part_setups[i].off != off)
            continue;

        si = DGU16((uint16_t)(part + 0x82));
        for (k = 0; k < 2 * part_setups[i].n; k++) {
            uint8_t v = (uint8_t)part_setups[i].add[k];

            if (part_setups[i].kind[k] == PS_W)
                v = (uint8_t)(DG8((uint16_t)(part + 0x44)) + v);
            else if (part_setups[i].kind[k] == PS_H)
                v = (uint8_t)(DG8((uint16_t)(part + 0x46)) + v);

            DG8((uint16_t)(si + 4 * (k / 2) + (k % 2))) = v;
        }

        part_finish(part_setups[i].finish, part);
        return;
    }

    /*
     * A fourth shape, and the plainest: copy N pairs straight out of a table
     * that is already in DGROUP, two bytes at a time into slots four apart.
     * These four are written as a loop in the original rather than unrolled,
     * which is the only reason they are not in the table above.
     */
    {
        static const struct { uint16_t off, tab; uint8_t n; } copies[4] = {
            { 0x012d, 0x3182, 8 },
            { 0x1075, 0x3266, 7 },
            { 0x2682, 0x3336, 7 },
            { 0x35f4, 0x3422, 8 },
        };
        int32_t j;

        for (j = 0; j < 4; j++) {
            uint16_t si, tab;
            int32_t k;

            if (copies[j].off != off)
                continue;

            si = DGU16((uint16_t)(part + 0x82));
            tab = copies[j].tab;
            for (k = 0; k < copies[j].n; k++) {
                DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
                DG8((uint16_t)(si + 4 * k + 1)) = DG8((uint16_t)(tab + 2 * k + 1));
            }

            part_finish(0x5d1e, part);
            return;
        }
    }

    /*
     * A fifth shape: the same copy, but from one of *two* tables, chosen by bit
     * 4 of the part's flags at +8 and then indexed by the word at +0x0c. That
     * is a part with two forms - the flag says which it is in - and each form
     * has its own connection points.
     */
    {
        static const struct {
            uint16_t off, set, clear;
            uint8_t n;
        } flagged[4] = {
            { 0x0371, 0x31e0, 0x31b6, 6 },
            { 0x2728, 0x338c, 0x3364, 4 },
            { 0x3294, 0x3404, 0x33e6, 4 },
            { 0x389b, 0x34b6, 0x3492, 8 },
        };
        int32_t j;

        for (j = 0; j < 4; j++) {
            uint16_t si, tab;
            int32_t k;

            if (flagged[j].off != off)
                continue;

            tab = (DGU16((uint16_t)(part + 8)) & 0x10)
                  ? flagged[j].set : flagged[j].clear;
            tab = DGU16((uint16_t)(tab
                                   + 2 * DGU16((uint16_t)(part + 0x0c))));

            si = DGU16((uint16_t)(part + 0x82));
            for (k = 0; k < flagged[j].n; k++) {
                DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
                DG8((uint16_t)(si + 4 * k + 1)) =
                    DG8((uint16_t)(tab + 2 * k + 1));
            }

            part_finish(0x5d1e, part);
            return;
        }
    }

    if (off == 0x40f0) {
        part_setup_40f0(part);
        return;
    }

    {
        static char what[64];

        snprintf(what, sizeof what, "the part setup at 172c:%04x", off);
        not_transcribed(what);
    }
}

/*
 * 172c:40f0, image 0x1b3b0
 *
 * A part with **three forms**, and the word at +0x0c says which. Its four
 * bytes at +0x6a..+0x6d - the box it is grabbed by - come out of one table
 * indexed by that word, and its eight connection points out of one of three
 * others, chosen by the same word with a `switch`.
 *
 * The three point tables are four bytes apart per entry rather than two, so the
 * pairs in them are interleaved with something this routine does not read.
 *
 * A form other than 0, 1 or 2 leaves the point untouched rather than defaulting
 * to one of them: the `jmp` at the end of the switch goes to the loop's own
 * increment.
 */
void part_setup_40f0(uint16_t part)
{
    uint16_t form = DGU16((uint16_t)(part + 0x0c));
    uint16_t di = DGU16((uint16_t)(part + 0x82));
    int32_t i;

    DG8((uint16_t)(part + 0x6a)) = DG8((uint16_t)(0x34ca + 4 * form));
    DG8((uint16_t)(part + 0x6b)) = DG8((uint16_t)(0x34cc + 4 * form));
    DG8((uint16_t)(part + 0x6c)) = DG8((uint16_t)(0x34d6 + 4 * form));
    DG8((uint16_t)(part + 0x6d)) = DG8((uint16_t)(0x34d8 + 4 * form));

    for (i = 0; i < 8; i++) {
        uint16_t tab;

        switch (form) {
        case 0:  tab = 0x34e2; break;
        case 1:  tab = 0x3502; break;
        case 2:  tab = 0x3522; break;
        default: di = (uint16_t)(di + 4); continue;
        }

        DG8(di) = DG8((uint16_t)(tab + 4 * i));
        DG8((uint16_t)(di + 1)) = DG8((uint16_t)(tab + 2 + 4 * i));
        di = (uint16_t)(di + 4);
    }

    part_finish(0x5d1e, part);
}

/*
 * NOT a transcription: reach the routine a setup ends by calling.
 *
 * All thirteen of the setups above end the same way, at 0x05d1e, so this could
 * be one call - it is a dispatcher anyway, because the setups that are not in
 * the table yet end at other addresses and this is where they will arrive.
 */
void part_finish(uint16_t off, uint16_t part)
{
    switch (off) {
    case 0x5d1e:
        part_finish_angles(part);
        return;
    default:
        break;
    }

    {
        static char what[64];

        snprintf(what, sizeof what, "the part finish at %#07x", off);
        (void)part;
        not_transcribed(what);
    }
}
