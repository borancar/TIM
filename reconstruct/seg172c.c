#include <stdio.h>
#include <stdlib.h>
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

    /*
     * A seventh shape, and the fifth without its index: the same two tables
     * chosen by bit 4 of the flags at +8, but taken whole rather than indexed
     * by the part's form. A part with two shapes and only two.
     */
    {
        static const struct {
            uint16_t off, set, clear;
            uint8_t n;
        } two[4] = {
            { 0x08a1, 0x322a, 0x3222, 4 },
            { 0x0c1c, 0x325c, 0x3252, 5 },
            { 0x1a32, 0x32d2, 0x32c8, 5 },
            { 0x1d28, 0x3308, 0x32fc, 6 },
        };
        int32_t j;

        for (j = 0; j < 4; j++) {
            uint16_t si, tab;
            int32_t k;

            if (two[j].off != off)
                continue;

            tab = (DGU16((uint16_t)(part + 8)) & 0x10) ? two[j].set
                                                       : two[j].clear;
            si = DGU16((uint16_t)(part + 0x82));
            for (k = 0; k < two[j].n; k++) {
                DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
                DG8((uint16_t)(si + 4 * k + 1)) =
                    DG8((uint16_t)(tab + 2 * k + 1));
            }

            part_finish(0x5d1e, part);
            return;
        }
    }

    /*
     * 172c:10b6, image 0x18376 - two tables again, but chosen by the form at
     * +0x0c rather than by the flag at +8: zero takes 0x3274 and anything else
     * 0x3282. Those two sit right after 0x3266, which the 0x1075 copy above
     * uses, so all three are one array of seven-pair rows and this picks the
     * second or the third.
     */
    if (off == 0x10b6) {
        uint16_t tab = DGU16((uint16_t)(part + 0x0c)) == 0 ? 0x3274 : 0x3282;
        uint16_t di = DGU16((uint16_t)(part + 0x82));
        int32_t k;

        for (k = 0; k < 7; k++) {
            DG8((uint16_t)(di + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
            DG8((uint16_t)(di + 4 * k + 1)) = DG8((uint16_t)(tab + 2 * k + 1));
        }

        part_finish(0x5d1e, part);
        return;
    }

    /*
     * 172c:1105, image 0x183c5 - four slots computed rather than copied. Two
     * bytes are worked out first and then laid into the corners: (0,b), (a,b),
     * (a,c), (0,c). `a` is 0x54 for kind 0x37, 0x69 for kind 0x39 in form 8 -
     * which also makes `b` 0x0a rather than 0 - and otherwise one less than the
     * part's width; `c` is 1 for kind 0x39 in form 0 and otherwise one less
     * than its height. So the general case is "the part's own box", and the two
     * named kinds are exceptions carved out of it.
     *
     * Afterwards, and unlike every other setup, it goes on to set +0x6a to half
     * the width and +0x6b to zero.
     */
    if (off == 0x1105) {
        uint8_t a, b = 0, c;
        uint16_t di;

        if (DGU16((uint16_t)(part + 4)) == 0x37) {
            a = 0x54;
        } else if (DGU16((uint16_t)(part + 4)) == 0x39
                   && DGU16((uint16_t)(part + 0x0c)) == 8) {
            a = 0x69;
            b = 0x0a;
        } else {
            a = (uint8_t)(DG8((uint16_t)(part + 0x44)) - 1);
        }

        if (DGU16((uint16_t)(part + 4)) == 0x39
            && DGU16((uint16_t)(part + 0x0c)) == 0)
            c = 1;
        else
            c = (uint8_t)(DG8((uint16_t)(part + 0x46)) - 1);

        di = DGU16((uint16_t)(part + 0x82));
        DG8(di) = 0;                        DG8((uint16_t)(di + 1)) = b;
        di = (uint16_t)(di + 4);
        DG8(di) = a;                        DG8((uint16_t)(di + 1)) = b;
        di = (uint16_t)(di + 4);
        DG8(di) = a;                        DG8((uint16_t)(di + 1)) = c;
        di = (uint16_t)(di + 4);
        DG8(di) = 0;                        DG8((uint16_t)(di + 1)) = c;

        part_finish(0x5d1e, part);

        DG8((uint16_t)(part + 0x6a)) =
            (uint8_t)(DG16((uint16_t)(part + 0x44)) >> 1);
        DG8((uint16_t)(part + 0x6b)) = 0;
        return;
    }

    /*
     * 172c:1435 - two tables by the flag at +8, with three bytes beside them:
     * +0x56 depends on the flag, +0x57 and +0x58 do not. It is the fortieth
     * setup, and it is here rather than in a table because it sets a word as
     * well as bytes.
     */
    if (off == 0x1435) {
        int32_t on = (DGU16((uint16_t)(part + 8)) & 0x10) != 0;
        uint16_t tab = on ? 0x32ae : 0x32a4;
        uint16_t si;
        int32_t k;

        DG8((uint16_t)(part + 0x56)) = on ? 0x25 : 0x00;
        DG8((uint16_t)(part + 0x57)) = 0x0d;
        DGU16((uint16_t)(part + 0x58)) = 0x12;

        si = DGU16((uint16_t)(part + 0x82));
        for (k = 0; k < 5; k++) {
            DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
            DG8((uint16_t)(si + 4 * k + 1)) = DG8((uint16_t)(tab + 2 * k + 1));
        }

        part_finish(0x5d1e, part);
        return;
    }

    /*
     * 172c:2068, image 0x19328 - the only setup that looks at the rest of the
     * machine. It runs 172c:0001 for the slots, clears its own four links at
     * +0x5a, and then walks the list at DGROUP 0x521b for other parts of its
     * own kind, 0x0e, sitting exactly 0x20 away in one axis and level in the
     * other. Each one found goes in the link for the direction it lies in -
     * right, left, down, up - so a run of them ends up knowing its neighbours.
     */
    if (off == 0x2068) {
        uint16_t di;
        int32_t i;

        part_setup(0x0001, part);

        for (i = 0; i < 4; i++)
            DGU16((uint16_t)(part + 0x5a + 2 * i)) = 0;

        for (di = DGU16(0x521b); di != 0; di = DGU16(di)) {
            int16_t dx, dy;

            if (di == part)
                continue;
            if (DGU16((uint16_t)(di + 4)) != 0x0e)
                continue;

            dx = (int16_t)(DG16((uint16_t)(part + 0x8c))
                           - DG16((uint16_t)(di + 0x8c)));
            dy = (int16_t)(DG16((uint16_t)(part + 0x8e))
                           - DG16((uint16_t)(di + 0x8e)));

            if (dy == 0) {
                if (dx == 0x20)
                    DGU16((uint16_t)(part + 0x5a)) = di;
                else if (dx == -0x20)
                    DGU16((uint16_t)(part + 0x5c)) = di;
            } else if (dx == 0) {
                if (dy == 0x20)
                    DGU16((uint16_t)(part + 0x5e)) = di;
                else if (dy == -0x20)
                    DGU16((uint16_t)(part + 0x60)) = di;
            }
        }
        return;
    }

    /*
     * 172c:3de5, image 0x1b0a5 - no slots at all, and no finish. It only turns
     * the two part numbers at +0x62 and +0x64 into two bits of the form at
     * +0x0c, so a part that was read off disk with those links set comes out in
     * the form that matches them.
     */
    if (off == 0x3de5) {
        DGU16((uint16_t)(part + 0x0c)) = 0;
        if (DGU16((uint16_t)(part + 0x62)) != 0)
            DGU16((uint16_t)(part + 0x0c)) |= 1;
        if (DGU16((uint16_t)(part + 0x64)) != 0)
            DGU16((uint16_t)(part + 0x0c)) |= 2;
        return;
    }

    /*
     * The last six, each written out: the pattern has run out and these are
     * genuinely their own routines.
     *
     * They share the copy at the end - N pairs, two bytes into every four - and
     * differ in how the source table is chosen and what else is set first.
     */
    if (off == 0x065b) {
        /* Four tables: the flag at +8, and then whether the form is zero. */
        uint16_t tab;
        uint16_t si;
        int32_t k;

        if (DGU16((uint16_t)(part + 8)) & 0x10)
            tab = DGU16((uint16_t)(part + 0x0c)) == 0 ? 0x320a : 0x3216;
        else
            tab = DGU16((uint16_t)(part + 0x0c)) == 0 ? 0x31f2 : 0x31fe;

        si = DGU16((uint16_t)(part + 0x82));
        for (k = 0; k < 6; k++) {
            DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
            DG8((uint16_t)(si + 4 * k + 1)) = DG8((uint16_t)(tab + 2 * k + 1));
        }
        part_finish(0x5d1e, part);
        return;
    }

    if (off == 0x1556) {
        /*
         * The form decides the table by being under 4 rather than by equalling
         * anything, and the count at +0x80 is **raised to 4 for the angles and
         * then dropped to 1** - so the part has four connection points while
         * they are being measured and one afterwards.
         */
        uint16_t tab = DG16((uint16_t)(part + 0x0c)) < 4 ? 0x32b8 : 0x32c0;
        uint16_t si = DGU16((uint16_t)(part + 0x82));
        int32_t k;

        for (k = 0; k < 4; k++) {
            DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
            DG8((uint16_t)(si + 4 * k + 1)) = DG8((uint16_t)(tab + 2 * k + 1));
        }

        DGU16((uint16_t)(part + 0x80)) = 4;
        part_finish(0x5d1e, part);
        DGU16((uint16_t)(part + 0x80)) = 1;

        DGU16((uint16_t)(part + 0x0c)) =
            (uint16_t)(DGU16((uint16_t)(part + 0x0c)) & 4);
        if (DGU16((uint16_t)(part + 0x62)) != 0)
            DGU16((uint16_t)(part + 0x0c)) =
                (uint16_t)(DGU16((uint16_t)(part + 0x0c)) | 1);
        if (DGU16((uint16_t)(part + 0x64)) != 0)
            DGU16((uint16_t)(part + 0x0c)) =
                (uint16_t)(DGU16((uint16_t)(part + 0x0c)) | 2);
        return;
    }

    if (off == 0x1dfb) {
        /*
         * Four constant points, three bytes set first, and then the form at
         * +0x0c is masked to its low two bits and two more are put back from
         * whether +0x62 and +0x64 are set. The mask and the rebuild are how the
         * form's own bits survive being used as a table index elsewhere.
         */
        static const uint8_t xy[8] = { 0x15, 0, 0x47, 0, 0x47, 0x1f, 0x15, 0x1f };
        uint16_t si = DGU16((uint16_t)(part + 0x82));
        int32_t k;

        DG8((uint16_t)(part + 0x56)) = 0x38;
        DG8((uint16_t)(part + 0x57)) = 0x12;
        DGU16((uint16_t)(part + 0x58)) = 0x0c;

        for (k = 0; k < 4; k++) {
            DG8((uint16_t)(si + 4 * k)) = xy[2 * k];
            DG8((uint16_t)(si + 4 * k + 1)) = xy[2 * k + 1];
        }

        part_finish(0x5d1e, part);

        DGU16((uint16_t)(part + 0x0c)) =
            (uint16_t)(DGU16((uint16_t)(part + 0x0c)) & 3);
        if (DGU16((uint16_t)(part + 0x62)) != 0)
            DGU16((uint16_t)(part + 0x0c)) =
                (uint16_t)(DGU16((uint16_t)(part + 0x0c)) | 4);
        if (DGU16((uint16_t)(part + 0x64)) != 0)
            DGU16((uint16_t)(part + 0x0c)) =
                (uint16_t)(DGU16((uint16_t)(part + 0x0c)) | 8);
        return;
    }

    if (off == 0x23b1 || off == 0x2cce) {
        /* Two tables by the flag, with one or two grab bytes beside them. */
        int32_t on = (DGU16((uint16_t)(part + 8)) & 0x10) != 0;
        uint16_t tab, si;
        int32_t k, n;

        if (off == 0x23b1) {
            DG8((uint16_t)(part + 0x6a)) = on ? 0x2a : 0x12;
            DG8((uint16_t)(part + 0x6b)) = 0x12;
            tab = on ? 0x3322 : 0x3314;
            n = 7;
        } else {
            DG8((uint16_t)(part + 0x6a)) = on ? 0x10 : 0x4b;
            DG8((uint16_t)(part + 0x56)) = on ? 0x24 : 0x2f;
            DG8((uint16_t)(part + 0x6b)) = 0x2d;
            tab = on ? 0x33bc : 0x33aa;
            n = 9;
        }

        si = DGU16((uint16_t)(part + 0x82));
        for (k = 0; k < n; k++) {
            DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
            DG8((uint16_t)(si + 4 * k + 1)) = DG8((uint16_t)(tab + 2 * k + 1));
        }
        part_finish(0x5d1e, part);
        return;
    }

    if (off == 0x377b) {
        /* Four tables by the form, compared one at a time, 0 1 2 and anything. */
        uint16_t form = DGU16((uint16_t)(part + 0x0c));
        uint16_t tab = form == 0 ? 0x3432
                     : form == 1 ? 0x3442
                     : form == 2 ? 0x3452 : 0x3462;
        uint16_t si = DGU16((uint16_t)(part + 0x82));
        int32_t k;

        for (k = 0; k < 8; k++) {
            DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
            DG8((uint16_t)(si + 4 * k + 1)) = DG8((uint16_t)(tab + 2 * k + 1));
        }
        part_finish(0x5d1e, part);
        return;
    }

    /*
     * An eighth shape: two tables by the same flag, and with them the two bytes
     * of the part's grab box at +0x72 and +0x73 - the first depending on the
     * flag as well, the second not. A part whose two forms are different sizes.
     */
    {
        static const struct {
            uint16_t off, set, clear;
            uint8_t set72, clear72, b73, n;
        } sized[2] = {
            { 0x0b88, 0x3242, 0x3232, 0x3e, 0x01, 0x03, 8 },
            { 0x1261, 0x329a, 0x3290, 0x01, 0x2d, 0x0f, 5 },
        };
        int32_t j;

        for (j = 0; j < 2; j++) {
            uint16_t si, tab;
            int32_t k, on;

            if (sized[j].off != off)
                continue;

            on = (DGU16((uint16_t)(part + 8)) & 0x10) != 0;
            DG8((uint16_t)(part + 0x72)) = on ? sized[j].set72
                                              : sized[j].clear72;
            DG8((uint16_t)(part + 0x73)) = sized[j].b73;
            tab = on ? sized[j].set : sized[j].clear;

            si = DGU16((uint16_t)(part + 0x82));
            for (k = 0; k < sized[j].n; k++) {
                DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(tab + 2 * k));
                DG8((uint16_t)(si + 4 * k + 1)) =
                    DG8((uint16_t)(tab + 2 * k + 1));
            }

            part_finish(0x5d1e, part);
            return;
        }
    }

    /*
     * 172c:2b58 - no connection points, only the grab box, and both its bytes
     * come out of one table indexed by the part's form at +0x0c.
     */
    if (off == 0x2b58) {
        uint16_t form = DGU16((uint16_t)(part + 0x0c));

        DG8((uint16_t)(part + 0x6a)) = DG8((uint16_t)(0x339a + 4 * form));
        DG8((uint16_t)(part + 0x6b)) = DG8((uint16_t)(0x339c + 4 * form));
        return;
    }

    /*
     * 172c:1be9 - the same copy, but with a stride of four in the *source*
     * table and, before that, the part's bitmap count at +0x80 **overwritten**
     * with 8. The initialiser sized the slot array from the count the part
     * table gave; this decides the count is really eight and uses it.
     */
    if (off == 0x1be9) {
        uint16_t si;
        int32_t k;

        DGU16((uint16_t)(part + 0x80)) = 8;
        si = DGU16((uint16_t)(part + 0x82));

        for (k = 0; k < 8; k++) {
            DG8((uint16_t)(si + 4 * k)) = DG8((uint16_t)(0x32dc + 4 * k));
            DG8((uint16_t)(si + 4 * k + 1)) = DG8((uint16_t)(0x32de + 4 * k));
        }

        part_finish(0x5d1e, part);
        return;
    }

    /*
     * 172c:19db - no connection points at all, just the two bytes of the box
     * the part is grabbed by, and the second of them depends on bit 5 of the
     * flags at +8. It does not call the angle routine, because there are no
     * angles to work out.
     */
    if (off == 0x19db) {
        DG8((uint16_t)(part + 0x6a)) = 7;
        DG8((uint16_t)(part + 0x6b)) =
            (DGU16((uint16_t)(part + 8)) & 0x20) ? 0x0e : 0x01;
        return;
    }

    /* 172c:3030 - `push bp / mov bp,sp / pop bp / retf`, and nothing else. */
    if (off == 0x3030)
        return;

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

/*
 * NOT a transcription: reach one part's per-step or hit hook by its offset in
 * this segment, the same way `part_setup` reaches a setup. An offset with no
 * case yet aborts and names itself.
 */
uint16_t part_hook_172c(uint16_t off, uint16_t part)
{
    switch (off) {
    case 0x2f25: return part_hit_2f25(part);
    case 0x018e: return part_step_018e(part);
    case 0x0552: return part_hit_0552(part);
    case 0x057e: return part_step_057e(part);
    case 0x098a: return part_step_098a(part);
    case 0x0a5d: return part_step_0a5d(part);
    case 0x0ca3: return part_step_0ca3(part);
    case 0x11a6: return part_step_11a6(part);
    case 0x0c6c: return part_hit_0c6c(part);
    case 0x12c2: return part_step_12c2(part);
    case 0x13c9: return part_step_13c9(part);
    case 0x14d3: return part_hit_14d3(part);
    case 0x15ce: return part_step_15ce(part);
    case 0x20fc: return part_step_20fc(part);
    case 0x22ae: return part_step_22ae(part);
    case 0x2514: return part_hit_2514(part);
    case 0x2592: return part_step_2592(part);
    case 0x2f3e: return part_step_2f3e(part);
    case 0x3035: return part_step_3035(part);
    case 0x34d0: return part_step_34d0(part);
    case 0x3824: return part_hit_3824(part);
    case 0x3635: return part_step_3635(part);
    case 0x38fc: return part_step_38fc(part);
    case 0x3ebf: return part_hit_3ebf(part);
    case 0x3fae: return part_step_3fae(part);
    case 0x3fe8: return part_hit_3fe8(part);
    case 0x420f: return part_step_420f(part);
    case 0x1a82: return part_step_1a82(part);
    case 0x1d07: return part_hit_1d07(part);
    case 0x1d78: return part_step_1d78(part);
    case 0x1e5c: return part_step_1e5c(part);
    case 0x1c39: return part_hit_1c39(part);
    case 0x34b5: return part_hit_34b5(part);
    case 0x1c5f: return part_step_1c5f(part);
    case 0x27e2: return part_step_27e2(part);
    case 0x2b99: return part_step_2b99(part);
    case 0x49a1: return part_step_49a1(part);
    default: break;
    }

    {
        static char what[64];
        static int32_t survey = -1;

        if (survey < 0)
            survey = getenv("TIM_SURVEY_HOOKS") != NULL;
        if (survey) {
            fprintf(stderr, "HOOK 172c:%04x kind %u\n", off,
                    DGU16((uint16_t)(part + 4)));
            return 0;
        }

        snprintf(what, sizeof what, "the part hook at 172c:%04x", off);
        not_transcribed(what);
    }
    return 0;
}

/*
 * 172c:49a1, image 0x1bc61 - kind 40's step.
 *
 * A countdown at +0x9c: while it is running the part is "on", which it says in
 * the word at +0x12 and passes to whatever its rope is tied to - as 1, or -1
 * when bit 4 of its flags at +8 is set, which is the mirrored form. The other
 * end is only told if it is not already busy, bit 11 of its own +8.
 *
 * Being on also steps the form at +0x0c round the four frames, and a form that
 * has changed since the last one drawn is handed to `place_object_for_draw`.
 *
 * The original leaves AX as whatever fell out; nothing reads it.
 */
uint16_t part_step_49a1(uint16_t part)
{
    uint16_t di;

    DGU16((uint16_t)(part + 0x12)) = 0;

    if (DGU16((uint16_t)(part + 0x9c)) != 0) {
        DGU16((uint16_t)(part + 0x9c))--;
        if (DGU16((uint16_t)(part + 0x9c)) != 0)
            DGU16((uint16_t)(part + 0x12)) = 1;
    }

    di = rope_other_end(part);
    if (di != 0 && !(DGU16((uint16_t)(di + 8)) & 0x800)) {
        if (DGU16((uint16_t)(part + 0x12)) != 0)
            DGU16((uint16_t)(di + 0x12)) =
                (DGU16((uint16_t)(part + 8)) & 0x10) ? 0xffff : 1;
        else
            DGU16((uint16_t)(di + 0x12)) = 0;
    }

    if (DGU16((uint16_t)(part + 0x12)) != 0) {
        if (DGU16((uint16_t)(part + 0x0c)) == 3)
            DGU16((uint16_t)(part + 0x0c)) = 0;
        else
            DGU16((uint16_t)(part + 0x0c))++;
    }

    if (DGU16((uint16_t)(part + 0x0c)) != DGU16((uint16_t)(part + 0x0e)))
        place_object_for_draw(part);

    return 0;
}

/*
 * 172c:15ce, image 0x1888e - kind 21's step.
 *
 * It does not move: it marks itself done - bit 6 of +8 - and passes its own
 * +0x12 on to whatever is in its links 4 and 5. The first four links are
 * something else's; these two are the ones this kind wires up.
 */
uint16_t part_step_15ce(uint16_t part)
{
    int16_t dx;

    DGU16((uint16_t)(part + 8)) |= 0x40;

    for (dx = 4; dx < 6; dx++) {
        uint16_t di = DGU16((uint16_t)(part + 0x5a + 2 * dx));

        if (di != 0)
            DGU16((uint16_t)(di + 0x12)) = DGU16((uint16_t)(part + 0x12));
    }

    return 0;
}

/*
 * 172c:1c5f, image 0x18f1f - kind 15's step.
 *
 * A two-part animation. Below a count of 0x14 at +0x9c the form runs on every
 * step; past 0x16 it drops back to 0x0e and the count goes up by one, and at
 * 0x0b it wraps to zero - so the first eleven frames play once and then it
 * loops on 0x0e to 0x16 for as long as the count allows.
 */
uint16_t part_step_1c5f(uint16_t part)
{
    if (DG16((uint16_t)(part + 0x9c)) < 0x14)
        DGU16((uint16_t)(part + 0x0c))++;

    if (DG16((uint16_t)(part + 0x0c)) > 0x16) {
        DGU16((uint16_t)(part + 0x0c)) = 0x0e;
        DGU16((uint16_t)(part + 0x9c))++;
    } else if (DGU16((uint16_t)(part + 0x0c)) == 0x0b) {
        DGU16((uint16_t)(part + 0x0c)) = 0;
    }

    if (DGU16((uint16_t)(part + 0x0c)) != DGU16((uint16_t)(part + 0x0e)))
        place_object_for_draw(part);

    return 0;
}

/*
 * 172c:2b7e, image 0x19e3e - kind 29's hit test.
 *
 * The same do-nothing as `part_hit_1de0`, down to the unused local.
 */
uint16_t part_hit_2b7e(uint16_t part)
{
    (void)part;
    return 1;
}

/*
 * 172c:2b99, image 0x19e59 - kind 29's step.
 *
 * Only forms 0 and 2 move on, and only while +0x12 says it is on: the form
 * steps by one and its own setup runs again, because this kind's connection
 * points depend on the form.
 */
uint16_t part_step_2b99(uint16_t part)
{
    if (DGU16((uint16_t)(part + 0x12)) == 0)
        return 0;

    if (DGU16((uint16_t)(part + 0x0c)) != 0
        && DGU16((uint16_t)(part + 0x0c)) != 2)
        return 0;

    DGU16((uint16_t)(part + 0x0c))++;
    part_setup(0x2b58, part);
    place_object_for_draw(part);

    return 0;
}

/*
 * 172c:20fc, image 0x193bc - kind 14's step, and the two routines below it.
 *
 * Kind 14 is a gear. A gear that has been given a direction at +0x12 marks
 * itself done - bit 6 of +8 - and pushes that direction out along its first
 * four links; `spread_gear_signal` follows the chain and answers 1 if it ever
 * found a gear already turning the wrong way. A chain that disagrees with
 * itself is jammed, so the gear's own direction is thrown away, and either way
 * `settle_gear_signal` walks the chain again to turn every gear on it.
 */
uint16_t part_step_20fc(uint16_t part)
{
    uint16_t fp = dg_enter(4);
    uint16_t v04 = (uint16_t)(fp + 0);      /* [bp-4] */
    uint16_t v02 = (uint16_t)(fp + 2);      /* [bp-2] */
    uint16_t di = 0;

    if (DGU16((uint16_t)(part + 0x12)) == 0)
        goto out;

    DGU16((uint16_t)(part + 8)) |= 0x40;

    for (DGU16(v02) = 0; DG16(v02) < 4; DGU16(v02)++) {
        DGU16(v04) = DGU16((uint16_t)(part + 0x5a + 2 * DGU16(v02)));
        if (DGU16(v04) == 0)
            continue;

        di = spread_gear_signal(part, DGU16(v04), 2, di);
    }

    if (di != 0)
        DGU16((uint16_t)(part + 0x12)) = 0;

    settle_gear_signal(part, (int16_t)di);

out:
    dg_leave(4);
    return 0;
}

/*
 * 172c:105d, image 0x1941d
 *
 * Push one gear's direction on to the next, and answer whether the chain
 * disagrees with itself.
 *
 * `how` says how the two are joined: 1 is a rope, which carries the direction
 * unchanged, and 2 is a mesh, which reverses it. A gear that is not turning yet
 * takes the direction; one that is already turning is checked against it, and
 * a mismatch - the same direction through a mesh, or a different one through a
 * rope - is the jam this answers 1 for.
 *
 * From a gear, kind 0x0e, it goes on to that gear's own four links and its
 * rope, marking each as it goes so a ring of gears is walked once. `flag` is
 * carried through and comes back, so one answer covers the whole chain.
 */
uint16_t spread_gear_signal(uint16_t from, uint16_t to, int16_t how,
                            uint16_t flag)
{
    uint16_t fp = dg_enter(6);
    uint16_t v06 = (uint16_t)(fp + 0);      /* [bp-6] the next gear */
    uint16_t v04 = (uint16_t)(fp + 2);      /* [bp-4] how it is joined */
    uint16_t v02 = (uint16_t)(fp + 4);      /* [bp-2] */

    if (DGU16((uint16_t)(to + 0x12)) != 0) {
        if (how == 1
            && DGU16((uint16_t)(to + 0x12)) != DGU16((uint16_t)(from + 0x12)))
            flag = 1;
        else if (how == 2
                 && DGU16((uint16_t)(to + 0x12))
                    == DGU16((uint16_t)(from + 0x12)))
            flag = 1;
    } else {
        DGU16((uint16_t)(to + 0x12)) =
            (how == 1) ? DGU16((uint16_t)(from + 0x12))
                       : (uint16_t)(0 - DGU16((uint16_t)(from + 0x12)));
    }

    if (DGU16((uint16_t)(to + 4)) != 0x0e
        || (DGU16((uint16_t)(to + 8)) & 0x40))
        goto out;

    DGU16((uint16_t)(to + 8)) |= 0x40;

    for (DGU16(v02) = 0; DG16(v02) < 5; DGU16(v02)++) {
        if (DG16(v02) == 4) {
            DGU16(v06) = rope_other_end(to);
            DGU16(v04) = 1;
        } else {
            DGU16(v06) = DGU16((uint16_t)(to + 0x5a + 2 * DGU16(v02)));
            DGU16(v04) = 2;
        }

        if (DGU16(v06) == 0)
            continue;
        if (DGU16((uint16_t)(DGU16(v06) + 8)) & 0x800)
            continue;

        flag = spread_gear_signal(to, DGU16(v06), (int16_t)DGU16(v04), flag);
    }

out:
    dg_leave(6);
    return flag;
}

/*
 * 172c:1225, image 0x194e5
 *
 * Turn a chain of gears by one step. Each one's direction at +0x12 is added to
 * its form at +0x0c, which wraps round the four positions, and the direction is
 * then cleared so it has to be given again next step.
 *
 * The walk is the same five links `spread_gear_signal` uses, and with `clear`
 * set every gear reached has its direction thrown away first - which is how a
 * jammed chain comes to a stop rather than turning.
 */
void settle_gear_signal(uint16_t part, int16_t clear)
{
    uint16_t fp = dg_enter(2);
    uint16_t v02 = fp;                      /* [bp-2] */

    DGU16((uint16_t)(part + 0x0c)) =
        (uint16_t)(DGU16((uint16_t)(part + 0x0c))
                   + DGU16((uint16_t)(part + 0x12)));

    if (DG16((uint16_t)(part + 0x0c)) == -1)
        DGU16((uint16_t)(part + 0x0c)) = 3;
    else if (DG16((uint16_t)(part + 0x0c)) == 4)
        DGU16((uint16_t)(part + 0x0c)) = 0;

    DGU16((uint16_t)(part + 0x12)) = 0;

    for (DGU16(v02) = 0; DG16(v02) < 5; DGU16(v02)++) {
        uint16_t di = (DG16(v02) == 4)
                      ? rope_other_end(part)
                      : DGU16((uint16_t)(part + 0x5a + 2 * DGU16(v02)));

        if (di == 0)
            continue;
        if (DGU16((uint16_t)(di + 0x12)) == 0)
            continue;
        if (DGU16((uint16_t)(di + 8)) & 0x800)
            continue;

        if (clear != 0)
            DGU16((uint16_t)(di + 0x12)) = 0;

        if (DGU16((uint16_t)(di + 4)) == 0x0e)
            settle_gear_signal(di, clear);
    }

    dg_leave(2);
}

/*
 * 172c:3035, image 0x1a2f5 - kind 30's step.
 *
 * It reaches for whatever is passing: `link_nearby_objects` builds a chain
 * through +0x78 of everything within 0x20 either side, and this picks one of
 * them to hold at +0x62.
 *
 * Two questions are asked of each candidate. Kinds 0x1d, 0x19 and 0x2d in a
 * form other than zero *block* it - unless the mirror bits agree for 0x19, or
 * the form is 2 for 0x1d - and something is only taken hold of at all if
 * something else blocked. Anything else with bit 2 of +0x0a in form zero is a
 * candidate: it has to be moving towards this part, and to be within 0x30
 * across and no further down than across.
 *
 * What it held last step wins outright if it is still there; otherwise the
 * slowest candidate wins, which is what makes it settle on the thing it can
 * actually catch. Taking hold steps the held part's +0x9c and says this part
 * moved.
 */
uint16_t part_step_3035(uint16_t part)
{
    uint16_t fp = dg_enter(0x0e);
    uint16_t v0e = (uint16_t)(fp + 0);      /* [bp-0x0e] the one held */
    uint16_t v0c = (uint16_t)(fp + 2);      /* [bp-0x0c] the drop */
    uint16_t v0a = (uint16_t)(fp + 4);      /* [bp-0x0a] the reach */
    uint16_t v08 = (uint16_t)(fp + 6);      /* [bp-8]  this one will do */
    uint16_t v06 = (uint16_t)(fp + 8);      /* [bp-6]  the slowest so far */
    uint16_t v04 = (uint16_t)(fp + 0x0a);   /* [bp-4]  held it last step */
    uint16_t v02 = (uint16_t)(fp + 0x0c);   /* [bp-2]  something blocked */
    uint16_t di = part;
    uint16_t si;

    link_nearby_objects(di, 0x3000, -0x20, 0x20, 0, 0);

    DGU16(v0e) = 0;
    DGU16(v02) = 0;
    DGU16(v04) = 0;
    DGU16(v06) = 0x190;

    for (si = DGU16((uint16_t)(di + 0x78)); si != 0; ) {
        if ((DGU16((uint16_t)(si + 4)) == 0x1d
             || DGU16((uint16_t)(si + 4)) == 0x19
             || DGU16((uint16_t)(si + 4)) == 0x2d)
            && DGU16((uint16_t)(si + 0x0c)) != 0) {

            if (DGU16((uint16_t)(di + 8)) & 0x10) {
                if (DG16((uint16_t)(si + 0x7a)) > 0)
                    DGU16(v02) = 1;
            } else {
                if (DG16((uint16_t)(si + 0x7a)) < 0)
                    DGU16(v02) = 1;
            }

            if (DGU16((uint16_t)(si + 4)) == 0x19) {
                if (((DGU16((uint16_t)(si + 8))
                      ^ DGU16((uint16_t)(di + 8))) & 0x10) == 0)
                    DGU16(v02) = 0;
            } else if (DGU16((uint16_t)(si + 4)) == 0x1d
                       && DGU16((uint16_t)(si + 0x0c)) == 2) {
                DGU16(v02) = 0;
            }

            goto next;
        }

        if (!(DGU16((uint16_t)(si + 0x0a)) & 4))
            goto next;
        if (DGU16((uint16_t)(si + 0x0c)) != 0)
            goto next;
        if (DGU16(v04) != 0)
            goto next;

        DGU16(v08) = 0;

        if (DGU16((uint16_t)(di + 8)) & 0x10) {
            if (DG16((uint16_t)(si + 0x7a)) < 0)
                DGU16(v08) = 1;
        } else {
            if (DG16((uint16_t)(si + 0x7a)) > 0)
                DGU16(v08) = 1;
        }

        grab_distance(di, si, v0a, v0c);

        if (DG16(v0a) >= 0x30 || DG16(v0c) > DG16(v0a))
            DGU16(v08) = 0;

        if (DGU16(v08) == 0)
            goto next;

        if (DGU16((uint16_t)(di + 0x62)) == si) {
            DGU16(v0e) = si;
            DGU16(v04) = 1;
            goto next;
        }

        {
            int16_t speed = DG16((uint16_t)(si + 0x7a));
            int16_t best = DG16(v06);

            if (speed < 0)
                speed = (int16_t)-speed;
            if (best < 0)
                best = (int16_t)-best;

            if (speed < best) {
                DGU16(v06) = DGU16((uint16_t)(si + 0x7a));
                DGU16(v0e) = si;
            }
        }

    next:
        if (DGU16(v02) != 0 && DGU16(v04) != 0)
            si = 0;
        else
            si = DGU16((uint16_t)(si + 0x78));
    }

    if (DGU16(v02) == 0)
        DGU16(v0e) = 0;

    DGU16((uint16_t)(di + 0x62)) = DGU16(v0e);

    if (DGU16(v0e) != 0) {
        DGU16((uint16_t)(DGU16(v0e) + 0x9c))++;
        part_moved(di);
    }

    dg_leave(0x0e);
    return 0;
}

/*
 * 172c:31dc, image 0x1a49c
 *
 * How far one part is from another's grip, as two absolute distances written
 * through pointers.
 *
 * The grip is the part's own left edge, or its right edge when bit 4 of +8 is
 * clear, and eight down from its top; the other part's point is its position
 * plus the two bytes at +0x72 and +0x73, which is where that kind is held.
 */
void grab_distance(uint16_t a, uint16_t b, uint16_t out_x, uint16_t out_y)
{
    int16_t ax = DG16((uint16_t)(a + 0x1e));
    int16_t ay = (int16_t)(DG16((uint16_t)(a + 0x20)) + 8);
    int16_t bx = (int16_t)(DG16((uint16_t)(b + 0x1e))
                           + DG8((uint16_t)(b + 0x72)));
    int16_t by = (int16_t)(DG16((uint16_t)(b + 0x20))
                           + DG8((uint16_t)(b + 0x73)));
    int16_t dx, dy;

    if (!(DGU16((uint16_t)(a + 8)) & 0x10))
        ax = (int16_t)(ax + DG16((uint16_t)(a + 0x44)));

    dx = (int16_t)(ax - bx);
    if (dx < 0)
        dx = (int16_t)-dx;
    DG16(out_x) = dx;

    dy = (int16_t)(ay - by);
    if (dy < 0)
        dy = (int16_t)-dy;
    DG16(out_y) = dy;
}

/*
 * 172c:2592, image 0x19852 - kind 5's step.
 *
 * A crank. Its direction at +0x12 turns the handle round seven positions, up
 * or down, and the wrap is written as a remainder rather than a compare: one
 * past a multiple of seven goes back six, and a multiple of seven goes forward
 * six. So the seven frames cycle in either direction without a table.
 *
 * A crank whose rope reaches a gear that is not turning - kind 0x0e with its
 * last two forms equal - gives up before any of that: nothing is on the other
 * end to turn.
 *
 * Turning sets DGROUP 0x52d3 to 2, and the first frame of a turn plays sound 1.
 */
uint16_t part_step_2592(uint16_t part)
{
    if (DGU16((uint16_t)(part + 0x12)) != 0) {
        uint16_t di = rope_other_end(part);

        if (di != 0 && DGU16((uint16_t)(di + 4)) == 0x0e
            && DGU16((uint16_t)(di + 0x0e)) == DGU16((uint16_t)(di + 0x10)))
            DGU16((uint16_t)(part + 0x12)) = 0;
    }

    if (DGU16((uint16_t)(part + 0x12)) == 0)
        return 0;

    DGU16(0x52d3) = 2;

    if (DGU16((uint16_t)(part + 0x0c)) == DGU16((uint16_t)(part + 0x0e)))
        play_sound(1);

    if (DG16((uint16_t)(part + 0x12)) > 0) {
        if ((int16_t)(DG16((uint16_t)(part + 0x0c)) + 1) % 7 == 0)
            DG16((uint16_t)(part + 0x0c)) -= 6;
        else
            DGU16((uint16_t)(part + 0x0c))++;
    } else if (DG16((uint16_t)(part + 0x12)) < 0) {
        if (DG16((uint16_t)(part + 0x0c)) % 7 == 0)
            DG16((uint16_t)(part + 0x0c)) += 6;
        else
            DGU16((uint16_t)(part + 0x0c))--;
    }

    return 0;
}

/*
 * 172c:057e, image 0x1783e - kind 35's step.
 *
 * A swing. While its +0x12 says go and it has not reached form 9 it steps one
 * frame - the first one plays sound 3 - and runs its own setup again, because
 * its connection points move with the swing.
 *
 * Forms 2 and 3 are where it reaches something: a box in front of it, taken
 * from one of two tables by the mirror bit and indexed by the form, and
 * everything caught in it is dealt with by kind. Bit 12 of +6 means it can be
 * knocked along, and it is given a speed by its own mass; kind 0x0f breaks;
 * kind 6 is set going.
 */
uint16_t part_step_057e(uint16_t part)
{
    uint16_t di;

    if (DGU16((uint16_t)(part + 0x12)) != 0
        && DGU16((uint16_t)(part + 0x0c)) != 9) {

        if (DGU16((uint16_t)(part + 0x0c)) == 0)
            play_sound(3);

        DGU16((uint16_t)(part + 0x0c))++;
        part_setup(0x065b, part);
        place_object_for_draw(part);
    }

    if (DGU16((uint16_t)(part + 0x0c)) != 2
        && DGU16((uint16_t)(part + 0x0c)) != 3)
        return 0;

    if (DGU16((uint16_t)(part + 8)) & 0x10)
        link_objects_in_range(
            part, 0x3000, 0x30,
            DG16((uint16_t)(0x31e8 + 2 * DGU16((uint16_t)(part + 0x0c)))),
            0, 0x1f);
    else
        link_objects_in_range(
            part, 0x3000,
            DG16((uint16_t)(0x31e2 + 2 * DGU16((uint16_t)(part + 0x0c)))),
            0, 0, 0x1f);

    for (di = DGU16((uint16_t)(part + 0x78)); di != 0;
         di = DGU16((uint16_t)(di + 0x78))) {

        if (DGU16((uint16_t)(di + 6)) & 0x1000) {
            int16_t v = bounce_speed_for_mass(di);

            DG16((uint16_t)(di + 0x36)) =
                (DGU16((uint16_t)(part + 8)) & 0x10) ? v : (int16_t)-v;
        } else if (DGU16((uint16_t)(di + 4)) == 0x0f) {
            break_kind_15(di);
        } else if (DGU16((uint16_t)(di + 4)) == 6) {
            trigger_kind_6(di);
        }
    }

    return 0;
}

/*
 * 172c:06f9, image 0x179b9
 *
 * How fast a thing is thrown, by how heavy it is: the mass at DGROUP 0xea8 for
 * its kind, in seven steps from 0x1a00 for the lightest down to 0x0c00 for the
 * heaviest. Written as a ladder of compares rather than a table.
 */
int16_t bounce_speed_for_mass(uint16_t obj)
{
    int16_t m = DG16((uint16_t)(0x0ea8
                                + 0x3a * (int16_t)DG16((uint16_t)(obj + 4))));

    if (m < 0x0006) return 0x1a00;
    if (m < 0x000a) return 0x1800;
    if (m < 0x0015) return 0x1600;
    if (m < 0x0079) return 0x1400;
    if (m < 0x0097) return 0x1200;
    if (m < 0x00c9) return 0x1000;
    if (m < 0x0709) return 0x0e00;
    return 0x0c00;
}

/*
 * 172c:1c9e, image 0x18f5e
 *
 * Break a kind-15 part: form 0x0b is the broken one, and a part already at
 * 0x0b or past it is left alone. Breaking plays sound 0x0a and replaces the
 * connection points with three of its own - the broken shape has a different
 * outline from the whole one.
 */
void break_kind_15(uint16_t part)
{
    uint16_t di, a, b;

    if (DG16((uint16_t)(part + 0x0c)) >= 0x0b)
        return;

    DGU16((uint16_t)(part + 0x0c)) = 0x0b;
    place_object_for_draw(part);
    play_sound(0x0a);

    DGU16((uint16_t)(part + 0x80)) = 3;

    di = DGU16((uint16_t)(part + 0x82));
    a = (uint16_t)(di + 4);
    b = (uint16_t)(a + 4);

    DG8(di) = 8;
    DG8((uint16_t)(b + 1)) = 0x2f;
    DG8((uint16_t)(di + 1)) = 0x2f;
    DG8(a) = 0x18;
    DG8((uint16_t)(a + 1)) = 0x2c;
    DG8(b) = 0x27;
}

/*
 * 172c:2ffd, image 0x1a2bd
 *
 * Set a kind-6 part going, in the direction its mirror bit says. A part that
 * was not going already plays sound 0x0d, and either way its +0x96 is put back
 * to 0x64.
 */
void trigger_kind_6(uint16_t part)
{
    if (DGU16((uint16_t)(part + 0x12)) == 0)
        play_sound(0x0d);

    DGU16((uint16_t)(part + 0x12)) =
        (DGU16((uint16_t)(part + 8)) & 0x10) ? 0xffff : 1;

    DGU16((uint16_t)(part + 0x96)) = 0x64;
}

/*
 * 172c:0a5d, image 0x17d1d - kind 18's step. The cannon.
 *
 * It starts itself once its +0x9c has counted past 0x14, then plays its eleven
 * frames: 0 to 7 one per step, and 7 held until +0x9c has gone up three more.
 * Frame 8 is the bang, and frame 9 fires - `make_part` builds a kind 0x2b, puts
 * it on the list at DGROUP 0x5179 and gives it a position and a velocity, left
 * or right by the mirror bit at +8.
 *
 * The velocity is the pair at +0x36 and +0x38 - 0xd000 or 0x3000 across and
 * 0xf000 down - and the position is carried in sixteenths as well, shifted left
 * by nine into +0x16 and +0x1a, which is the same wind-up `reset_machine` does.
 *
 * A cannon that could not get the shot from the heap simply does not fire.
 */
uint16_t part_step_0a5d(uint16_t part)
{
    uint16_t si;

    if (DGU16((uint16_t)(part + 0x12)) == 0
        && DG16((uint16_t)(part + 0x9c)) > 0x14)
        DGU16((uint16_t)(part + 0x12)) = 1;

    if (DGU16((uint16_t)(part + 0x12)) == 0)
        return 0;
    if (DGU16((uint16_t)(part + 0x0c)) == 0x0b)
        return 0;

    if (DGU16((uint16_t)(part + 0x0c)) != 7) {
        DGU16((uint16_t)(part + 0x0c))++;
    } else {
        DGU16((uint16_t)(part + 0x9c))++;
        if (DG16((uint16_t)(part + 0x9c)) > 3)
            DGU16((uint16_t)(part + 0x0c))++;
    }

    place_object_for_draw(part);

    if (DGU16((uint16_t)(part + 0x0c)) == 8)
        play_sound(6);

    if (DGU16((uint16_t)(part + 0x0c)) != 9)
        return 0;

    si = make_part(0x2b);
    if (si == 0)
        return 0;

    insert_sorted(si, 0x5179);
    DGU16((uint16_t)(si + 6)) |= 0x10;

    if (DGU16((uint16_t)(part + 8)) & 0x10) {
        DG16((uint16_t)(si + 0x1e)) =
            (int16_t)(DG16((uint16_t)(part + 0x1e)) - 0x30);
        DG16((uint16_t)(si + 0x26)) =
            (int16_t)(DG16((uint16_t)(si + 0x1e)) + 0x18);
        DG16((uint16_t)(si + 0x22)) = DG16((uint16_t)(si + 0x26));
        DGU16((uint16_t)(si + 0x36)) = 0xd000;
    } else {
        DG16((uint16_t)(si + 0x1e)) =
            (int16_t)(DG16((uint16_t)(part + 0x1e)) + 0x61);
        DG16((uint16_t)(si + 0x26)) =
            (int16_t)(DG16((uint16_t)(si + 0x1e)) - 0x18);
        DG16((uint16_t)(si + 0x22)) = DG16((uint16_t)(si + 0x26));
        DGU16((uint16_t)(si + 0x36)) = 0x3000;
    }

    DG16((uint16_t)(si + 0x20)) =
        (int16_t)(DG16((uint16_t)(part + 0x20)) - 7);
    DG16((uint16_t)(si + 0x28)) =
        (int16_t)(DG16((uint16_t)(si + 0x20)) + 8);
    DG16((uint16_t)(si + 0x24)) = DG16((uint16_t)(si + 0x28));
    DGU16((uint16_t)(si + 0x38)) = 0xf000;

    clamp_record_pair(si);

    DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
    DG32((uint16_t)(si + 0x16)) =
        (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x16)), 9);

    DG32((uint16_t)(si + 0x1a)) = DG16((uint16_t)(si + 0x20));
    DG32((uint16_t)(si + 0x1a)) =
        (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x1a)), 9);

    place_object_for_draw(si);

    return 0;
}

/*
 * 172c:1a82, image 0x18d42 - kind 24's step. The fan.
 *
 * Four frames on a loop while it is on, the first playing sound 9, and DGROUP
 * 0x52cf set to 2. The blast is a box reaching 0x100 out in the direction the
 * mirror bit says and ten up, and everything in it is pushed.
 *
 * The push is not a constant: it is the fan's force times how much slower than
 * 0x100 the thing is already going, shifted down eight and then *divided by the
 * thing's own mass*. So a heavy object barely moves and one already at full
 * speed is not pushed at all.
 *
 * Two kinds answer differently. Kind 0x28 with bit 13 of +6 is switched on
 * rather than pushed, with its +0x9c set to 0x14 - but only below 0xc8, so a
 * fast one is left alone. Kind 0x2d is reset to its first frame after being
 * pushed.
 */
uint16_t part_step_1a82(uint16_t part)
{
    uint16_t fp = dg_enter(0x0a);
    uint16_t v0a = (uint16_t)(fp + 0);      /* [bp-0x0a] the product, low */
    uint16_t v08 = (uint16_t)(fp + 2);      /* [bp-8] the product, high */
    uint16_t v06 = (uint16_t)(fp + 4);      /* [bp-6] how much slower */
    uint16_t v04 = (uint16_t)(fp + 6);      /* [bp-4] the push */
    uint16_t v02 = (uint16_t)(fp + 8);      /* [bp-2] the force */
    uint16_t di = part;
    uint16_t si;

    if (DGU16((uint16_t)(di + 0x12)) == 0)
        goto out;

    DGU16(0x52cf) = 2;

    if (DGU16((uint16_t)(di + 0x0c)) == DGU16((uint16_t)(di + 0x0e)))
        play_sound(9);

    DGU16((uint16_t)(di + 0x0c))++;
    if (DGU16((uint16_t)(di + 0x0c)) == 4)
        DGU16((uint16_t)(di + 0x0c)) = 0;

    if (DGU16((uint16_t)(di + 8)) & 0x10) {
        link_nearby_objects(di, 0x3000, (int16_t)0xff00, 0, -10, 0);
        DGU16(v02) = 0xf000;
    } else {
        link_nearby_objects(di, 0x3000, 0, 0x100, -10, 0);
        DGU16(v02) = 0x1000;
    }

    for (si = DGU16((uint16_t)(di + 0x78)); si != 0;
         si = DGU16((uint16_t)(si + 0x78))) {

        int16_t speed, mass;
        int32_t p;

        if (DGU16((uint16_t)(si + 6)) & 0x2000) {
            if (DGU16((uint16_t)(si + 4)) != 0x28)
                continue;

            speed = DG16((uint16_t)(si + 0x7a));
            if (speed < 0)
                speed = (int16_t)-speed;
            if (speed >= 0xc8)
                continue;

            DGU16((uint16_t)(si + 0x12)) = 1;
            DGU16((uint16_t)(si + 0x9c)) = 0x14;
            continue;
        }

        speed = DG16((uint16_t)(si + 0x7a));
        if (speed < 0)
            speed = (int16_t)-speed;
        DG16(v06) = (int16_t)(0x100 - speed);

        p = (int32_t)mul16x16(DG16(v02), DG16(v06));
        p = long_shift_right(p, 8);
        DG16(v08) = (int16_t)(p >> 16);
        DG16(v0a) = (int16_t)p;

        mass = DG16((uint16_t)(0x0ea8
                               + 0x3a * (int16_t)DG16((uint16_t)(si + 4))));

        DG16(v04) = (int16_t)long_divide(
            ((int32_t)(uint16_t)DG16(v08) << 16) | (uint16_t)DG16(v0a),
            (int32_t)mass);

        DG16((uint16_t)(si + 0x36)) =
            (int16_t)(DG16((uint16_t)(si + 0x36)) + DG16(v04));

        clamp_record_pair(si);

        if (DGU16((uint16_t)(si + 4)) == 0x2d) {
            DGU16((uint16_t)(si + 0x9c)) = 0;
            DGU16((uint16_t)(si + 0x12)) = 0;
            DGU16((uint16_t)(si + 0x0c)) = 0;
        }
    }

    place_object_for_draw(di);

out:
    dg_leave(0x0a);
    return 0;
}

/*
 * 172c:271f, image 0x1b9df
 *
 * The same ladder as `bounce_speed_for_mass`, one step longer at the light
 * end: 0x1c00 below a mass of 2, and the rest of the steps as before. The two
 * exist separately in the original and are kept separate here.
 */
int16_t push_speed_for_mass(uint16_t obj)
{
    int16_t m = DG16((uint16_t)(0x0ea8
                                + 0x3a * (int16_t)DG16((uint16_t)(obj + 4))));

    if (m < 0x0002) return 0x1c00;
    if (m < 0x0006) return 0x1a00;
    if (m < 0x000a) return 0x1800;
    if (m < 0x0015) return 0x1600;
    if (m < 0x0079) return 0x1400;
    if (m < 0x0097) return 0x1200;
    return 0x1000;
}

/*
 * 172c:277d, image 0x1ba3d
 *
 * Set going whatever is in the chain at +0x78, at a point `dx` from the part's
 * own position.
 *
 * The original dispatches on the kind through a jump table in its own code
 * segment - six kinds at 172c:4893 and six targets twelve bytes after them -
 * which is the compiler's `switch`, so the port writes it as one.
 *
 * Four of the six turn on only if the thing is within a window of the point,
 * and the window depends on the mirror bit: kind 0x10 at 0x36..0x3c or 0..8,
 * kind 0x25 at 0x19..0x25 or 0..0x0c, and kinds 0x19 and 0x16 the same but
 * only when `mode` is 1. The other two are handed to the routines that already
 * know what to do with them.
 */
void trigger_things_at(uint16_t part, int16_t mode, int16_t dx)
{
    int16_t x = (int16_t)(DG16((uint16_t)(part + 0x1e)) + dx);
    uint16_t si;

    for (si = DGU16((uint16_t)(part + 0x78)); si != 0;
         si = DGU16((uint16_t)(si + 0x78))) {

        int16_t d = (int16_t)(x - DG16((uint16_t)(si + 0x1e)));
        int16_t mirrored = (DGU16((uint16_t)(si + 8)) & 0x10) != 0;

        switch (DGU16((uint16_t)(si + 4))) {
        case 0x10:
            if (mirrored ? (d >= 0x36 && d <= 0x3c) : (d >= 0 && d <= 8))
                DGU16((uint16_t)(si + 0x12)) = 1;
            break;

        case 0x06:
            trigger_kind_6(si);
            break;

        case 0x25:
            if (mirrored ? (d >= 0x19 && d <= 0x25) : (d >= 0 && d <= 0x0c))
                DGU16((uint16_t)(si + 0x12)) = 1;
            break;

        case 0x19:
            if (mode != 1)
                break;
            if (mirrored ? (d >= 0x0d && d <= 0x18) : (d >= 5 && d <= 0x10))
                DGU16((uint16_t)(si + 0x12)) = 1;
            break;

        case 0x16:
            if (mode != 1)
                break;
            if (mirrored ? (d >= 0 && d <= 0x1f) : (d >= 0x67 && d <= 0x87))
                DGU16((uint16_t)(si + 0x12)) = 1;
            break;

        case 0x0f:
            break_kind_15(si);
            break;

        default:
            break;
        }
    }
}

/*
 * 172c:0000, image 0x172bc
 *
 *
 * The module's first routine, and it does nothing at all: a frame and a `retf`.
 * It is here because the segment's own offset 0 has to be something.
 */
void seg172c_nothing(void)
{
}

/*
 * 172c:461a, image 0x1b8da
 *
 * Push a part's motion out along its belts, and answer whether anything
 * refused.
 *
 * Each of the two belts at +0x66 leads to another part, which
 * `select_field_2_or_4` names. The one the caller came *from* is skipped, which is
 * what stops the walk going back on itself. `belt_orientation` says how the
 * belt runs between them - which way round the tangent points are - and that,
 * or-ed with the caller's own flags, is handed on with the part.
 *
 * The handler is the far pointer at +0x36 of the *far* part's kind record, so
 * what happens next is that part's business and not this one's. A part already
 * marked with bit 9 of +8 answers 1 straight away, and the walk stops at the
 * first belt that answers anything at all.
 */
uint16_t drive_belts(uint16_t from, uint16_t part, uint16_t flags,
                     uint16_t a, uint16_t b, uint16_t c)
{
    uint16_t fp = dg_enter(0x10);
    uint16_t v10 = (uint16_t)(fp + 0x00);   /* [bp-0x10] the far part */
    uint16_t v0a = (uint16_t)(fp + 0x06);   /* [bp-0x0a] the far slot */
    uint16_t v08 = (uint16_t)(fp + 0x08);   /* [bp-8] the near slot */
    uint16_t v06 = (uint16_t)(fp + 0x0a);   /* [bp-6] which end */
    uint16_t v04 = (uint16_t)(fp + 0x0c);   /* [bp-4] the answer */
    uint16_t v02 = (uint16_t)(fp + 0x0e);   /* [bp-2] the belt */
    uint16_t di = part;
    uint16_t si;
    uint16_t answer;

    if (DGU16((uint16_t)(di + 8)) & 0x200) {
        answer = 1;
        goto out;
    }

    DGU16(v04) = 0;

    for (DGU16(v02) = 0; DG16(v02) < 2 && DGU16(v04) == 0; DGU16(v02)++) {
        uint16_t dir;

        si = DGU16((uint16_t)(di + 0x66 + 2 * DGU16(v02)));
        if (si == 0)
            continue;

        DGU16(v10) = (uint16_t)select_field_2_or_4((int16_t)di, si);
        if (DGU16(v10) == from)
            continue;

        if (DGU16((uint16_t)(si + 2)) == di) {
            DGU16(v06) = 0;
            DGU16(v08) = DG8((uint16_t)(si + 0x0a));
            DGU16(v0a) = DG8((uint16_t)(si + 0x0b));
        } else {
            DGU16(v06) = 1;
            DGU16(v08) = DG8((uint16_t)(si + 0x0b));
            DGU16(v0a) = DG8((uint16_t)(si + 0x0a));
        }

        if (DG16((uint16_t)(di + 0x12)) > 0)
            dir = (DGU16(v08) == 0) ? 0 : 1;
        else
            dir = (DGU16(v08) == 0) ? 1 : 0;

        DGU16(v04) = (uint16_t)belt_orientation(si, (int16_t)DGU16(v06),
                                                (int16_t)dir);
        DGU16(v04) |= flags;

        DGU16(v04) = part_drive(DGU16(v10), di, DGU16(v10), DGU16(v0a),
                                DGU16(v04), a, b, c);
    }

    answer = DGU16(v04);

out:
    dg_leave(0x10);
    return answer;
}

/*
 * NOT a transcription: reach one part's drive hook by its offset in this
 * segment. An offset with no case yet aborts and names itself.
 */
uint16_t part_drive_172c(uint16_t off, uint16_t p1, uint16_t p2, uint16_t p3,
                         uint16_t p4, uint16_t p5, uint16_t p6, uint16_t p7)
{
    switch (off) {
    case 0x11d2: return part_drive_11d2(p1, p2, p3, p4, p5, p6, p7);
    case 0x2c19: return part_drive_2c19(p1, p2, p3, p4, p5, p6, p7);
    default: break;
    }

    {
        static char what[64];

        (void)p1; (void)p2; (void)p3; (void)p4; (void)p5; (void)p6; (void)p7;
        snprintf(what, sizeof what, "the part drive at 172c:%04x", off);
        not_transcribed(what);
    }
    return 0;
}

/*
 * 172c:11d2, image 0x18492 - kind 57's drive hook.
 *
 * Flags of exactly 1 is the counting pass `part_drive_2c19` also recognises:
 * the belt's +0x0e goes up and the answer is 0, so the walk carries on.
 *
 * Otherwise it is a contest of momentum. The part's own at +0x3c - the long
 * the record doc calls speed, weight times how fast it is going - is measured
 * against the momentum the drive arrived with, and the part refuses when its
 * own is the greater, which ends the caller's walk. Driven straight from a
 * kind 3 - the motor - the part's momentum counts once; through anything else
 * it counts *twice*, so the same drive that turns a thing directly can fail to
 * turn it at one more remove.
 */
uint16_t part_drive_11d2(uint16_t from, uint16_t part, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t lo,
                         uint16_t hi)
{
    uint32_t mine;

    (void)p3; (void)p5;

    if (flags == 1) {
        DGU16((uint16_t)(DGU16((uint16_t)(part + 0x66)) + 0x0e))++;
        return 0;
    }

    mine = (uint32_t)DGU16((uint16_t)(part + 0x3c))
           | ((uint32_t)DGU16((uint16_t)(part + 0x3e)) << 16);

    if (DGU16((uint16_t)(from + 4)) != 3)
        mine += mine;

    return (int32_t)mine > (int32_t)((uint32_t)lo | ((uint32_t)hi << 16))
           ? 1 : 0;
}

/*
 * 172c:2c19, image 0x19ed9 - kind 29's drive hook.
 *
 * The arguments are the seven `drive_belts` hands over; this one uses only the
 * part at +8 and the flags at +0x0c.
 *
 * Flags of exactly 1 means "count how many belts reach here": the belt's +0x0e
 * goes up and the answer is 0, so the walk carries on.
 *
 * Otherwise only bits 1, 2 and 15 of the flags are kept. A belt running that
 * way over a part already going - or bit 1 on its own - refuses, which is what
 * stops the drive: it answers 1 and the caller's walk ends. Bit 2 on a part
 * that is *not* going starts it instead, with sound 0x11, and answers 0.
 */
uint16_t part_drive_2c19(uint16_t p1, uint16_t si, uint16_t p3,
                         uint16_t flags, uint16_t p5, uint16_t p6, uint16_t p7)
{
    uint16_t belt = DGU16((uint16_t)(si + 0x66));
    uint16_t kept;

    (void)p1; (void)p3; (void)p5; (void)p6; (void)p7;

    if (flags == 1) {
        DGU16((uint16_t)(belt + 0x0e))++;
        return 0;
    }

    flags &= 0x8006;
    kept = (uint16_t)(flags & 0x7fff);

    if (kept == 2)
        return 1;
    if (kept == 4 && DGU16((uint16_t)(si + 0x12)) != 0)
        return 1;

    if (flags == 4 && DGU16((uint16_t)(si + 0x12)) == 0) {
        play_sound(0x11);
        DGU16((uint16_t)(si + 0x12)) = 1;
    }

    return 0;
}

/*
 * 172c:420f, image 0x1b4cf - kind 3's step. The motor.
 *
 * Nothing happens unless +0x12 says it is on. Then it marks itself done - bit
 * 6 of +8 - and either turns freely, when bit 10 of +8 is set, or asks its
 * belts first: `drive_belts` twice, once with 0x8000 in the flags and once
 * without, and an answer from the first means the belt is being held, which
 * sets bit 9 of +8 and stops it turning. Otherwise the second call goes out
 * anyway and the form steps by the direction.
 *
 * A form that has changed runs `part_setup_40f0` - the motor's connection
 * points move with it - and forms 0 and 2 play sound 0x12. Then it looks for
 * what its shaft is over: `link_objects_crossing_segments` gives it the
 * candidates and each is given a speed from `push_speed_for_mass`, signed by
 * which side of the shaft's middle it lies. Each one is then dropped 0x10 and
 * lifted 0x10 through `resolve_collisions` to settle it, with the motor hidden
 * for the second of the two so it does not collide with itself.
 *
 * Finally, at form 0 with the last form non-zero, it reaches out once more:
 * `link_objects_in_range` over a box 0x4a to 0x4f across and 2 down, and
 * `trigger_things_at` sets going whatever is there.
 */
uint16_t part_step_420f(uint16_t part)
{
    uint16_t fp = dg_enter(0x0a);
    uint16_t v0a = (uint16_t)(fp + 0x00);   /* [bp-0x0a] the position, 32-bit */
    uint16_t v06 = (uint16_t)(fp + 0x04);   /* [bp-6] the speed */
    uint16_t v04 = (uint16_t)(fp + 0x06);   /* [bp-4] the other's middle */
    uint16_t v02 = (uint16_t)(fp + 0x08);   /* [bp-2] the shaft's middle */
    uint16_t si = part;
    uint16_t di;

    if (DGU16((uint16_t)(si + 0x12)) == 0)
        goto tail;

    DGU16((uint16_t)(si + 8)) |= 0x40;

    if (DGU16((uint16_t)(si + 8)) & 0x400) {
        DGU16((uint16_t)(si + 0x0c)) =
            (uint16_t)(DGU16((uint16_t)(si + 0x0c))
                       + DGU16((uint16_t)(si + 0x12)));
    } else if (drive_belts(0, si, 0x8000, 0x3e8,
                           DGU16((uint16_t)(si + 0x3c)),
                           DGU16((uint16_t)(si + 0x3e))) != 0) {
        DGU16((uint16_t)(si + 8)) |= 0x200;
    } else {
        drive_belts(0, si, 0, 0x3e8,
                    DGU16((uint16_t)(si + 0x3c)),
                    DGU16((uint16_t)(si + 0x3e)));
        DGU16((uint16_t)(si + 0x0c)) =
            (uint16_t)(DGU16((uint16_t)(si + 0x0c))
                       + DGU16((uint16_t)(si + 0x12)));
    }

    if (DGU16((uint16_t)(si + 0x0c)) == DGU16((uint16_t)(si + 0x0e)))
        goto clear;

    part_setup_40f0(si);

    if (DGU16((uint16_t)(si + 0x0e)) == 0
        || DGU16((uint16_t)(si + 0x0e)) == 2)
        play_sound(0x12);

    place_object_for_draw(si);

    DG16(v02) = (int16_t)(DG16((uint16_t)(si + 0x1e))
                          + (DG16((uint16_t)(si + 0x44)) >> 1));

    link_objects_crossing(si, 0x1000,
                          (uint16_t)(0x3542 + 8 * DGU16((uint16_t)(si + 0x0c))));

    for (di = DGU16((uint16_t)(si + 0x78)); di != 0;
         di = DGU16((uint16_t)(di + 0x78))) {

        DG16(v04) = (int16_t)(DG16((uint16_t)(di + 0x1e))
                              + (DG16((uint16_t)(di + 0x44)) >> 1));
        DG16(v06) = push_speed_for_mass(di);

        if (DG16((uint16_t)(si + 0x12)) == -1) {
            if (DG16(v04) < DG16(v02)) {
                DG16((uint16_t)(di + 0x38)) = DG16(v06);
                DG16((uint16_t)(di + 0x36)) =
                    (int16_t)-(int16_t)(DG16(v06) >> 2);
            } else {
                DG16((uint16_t)(di + 0x38)) = (int16_t)-DG16(v06);
                DG16((uint16_t)(di + 0x36)) = (int16_t)(DG16(v06) >> 2);
            }
        } else if (DG16((uint16_t)(si + 0x12)) == 1) {
            if (DG16(v04) < DG16(v02)) {
                DG16((uint16_t)(di + 0x38)) = (int16_t)-DG16(v06);
                DG16((uint16_t)(di + 0x36)) =
                    (int16_t)-(int16_t)(DG16(v06) >> 2);
            } else {
                DG16((uint16_t)(di + 0x38)) = DG16(v06);
                DG16((uint16_t)(di + 0x36)) = (int16_t)(DG16(v06) >> 2);
            }
        }

        mark_part_shapes(di, 3);

        if (DG16((uint16_t)(di + 0x38)) < 0) {
            DG16((uint16_t)(di + 0x24)) =
                (int16_t)(DG16((uint16_t)(di + 0x20)) - 0x10);
            resolve_collisions(di);

            DG16((uint16_t)(di + 0x24)) =
                (int16_t)(DG16((uint16_t)(di + 0x20)) + 0x10);
            DGU16((uint16_t)(si + 8)) |= 0x2000;
            resolve_collisions(di);
            DGU16((uint16_t)(si + 8)) &= 0xdfff;

            DG16((uint16_t)(di + 0x24)) = DG16((uint16_t)(di + 0x20));

            DG32(v0a) = DG16((uint16_t)(di + 0x20));
            DG32((uint16_t)(di + 0x1a)) =
                (int32_t)long_shift_left((uint32_t)DG32(v0a), 9);
        } else {
            DG16((uint16_t)(di + 0x24)) =
                (int16_t)(DG16((uint16_t)(di + 0x20)) + 0x10);
            resolve_collisions(di);

            DG16((uint16_t)(di + 0x24)) =
                (int16_t)(DG16((uint16_t)(di + 0x20)) - 0x10);
            DGU16((uint16_t)(si + 8)) |= 0x2000;
            resolve_collisions(di);
            DGU16((uint16_t)(si + 8)) &= 0xdfff;

            DG16((uint16_t)(di + 0x24)) = DG16((uint16_t)(di + 0x20));

            DG32(v0a) = DG16((uint16_t)(di + 0x20));
            DG32((uint16_t)(di + 0x1a)) =
                (int32_t)(long_shift_left((uint32_t)(DG32(v0a) + 1), 9) - 1);
        }
    }

clear:
    DGU16((uint16_t)(si + 0x12)) = 0;
    DGU16((uint16_t)(si + 0x3e)) = 0;
    DGU16((uint16_t)(si + 0x3c)) = 0;

tail:
    /*
     * The two ends of the stroke reach out, in opposite pairs: at form 0 the
     * near side of the shaft is at 0x4a..0x4f across and level, and the far
     * side at 0..6 and 0x20..0x24 down; at form 2 the two swap over. Each box
     * is followed by a `trigger_things_at` for the point it was measured from.
     */
    if (DGU16((uint16_t)(si + 0x0e)) == 0) {
        if (DGU16((uint16_t)(si + 0x10)) == 0)
            goto out;

        link_objects_in_range(si, 0x2000, 0x4a, 0x4f, -2, 2);
        trigger_things_at(si, 0, 0x4a);

        link_objects_in_range(si, 0x2000, 0, 6, 0x20, 0x24);
        trigger_things_at(si, 1, 0);
        goto out;
    }

    if (DGU16((uint16_t)(si + 0x0e)) != 2)
        goto out;
    if (DGU16((uint16_t)(si + 0x10)) == 2)
        goto out;

    link_objects_in_range(si, 0x2000, 0x4a, 0x4f, 0x20, 0x24);
    trigger_things_at(si, 1, 0x4a);

    link_objects_in_range(si, 0x2000, 0, 6, -2, 2);
    trigger_things_at(si, 0, 0);

out:
    dg_leave(0x0a);
    return 0;
}

/*
 * 172c:38fc, image 0x1abbc - kind 37's step. The scissors.
 *
 * They cut once: only in form 0, and only while +0x12 says go. The line they
 * cut along is one of two in DGROUP - 0x34c2 mirrored, 0x34ba not - and
 * `cut_belts` does the work. Then the form steps, its own setup runs again
 * because the shape has changed, and sound 0x10 plays.
 */
uint16_t part_step_38fc(uint16_t part)
{
    if (DGU16((uint16_t)(part + 0x12)) == 0)
        return 0;
    if (DGU16((uint16_t)(part + 0x0c)) != 0)
        return 0;

    cut_belts(part, (DGU16((uint16_t)(part + 8)) & 0x10) ? 0x34c2 : 0x34ba);

    DGU16((uint16_t)(part + 0x0c))++;
    part_setup(0x389b, part);
    place_object_for_draw(part);
    play_sound(0x10);

    return 0;
}

/*
 * 172c:3970, image 0x1ac30
 *
 * Cut every belt that crosses a line.
 *
 * The list at DGROUP 0x521b is walked for belts - kind 0x0a - and each one's
 * lengths in turn: from the part the record names at +2, along the chain of
 * pulleys, to the part at +4. Each length is turned into two points in the
 * scissors' own frame and handed to `intersect_segments`.
 *
 * A cut makes three parts: two kind-0x31 anchors, both at the point the cut
 * fell, and a kind-0x0a to carry the second half of the belt. If any of the
 * three cannot be had the ones already made are given back and nothing is cut -
 * so a machine that runs out of memory keeps its belt whole rather than losing
 * half of it.
 *
 * The rethreading is the fiddly part. The old belt record keeps the near half
 * and ends at the first anchor; the new one takes the second anchor and the old
 * far end, and the pulley the cut length was heading for is pointed at it. A
 * pulley - kind 7 - is repointed through its own +0x68 and +0x5c rather than
 * through the slot the walk was using, because a pulley's two links are not
 * interchangeable.
 *
 * Both anchors get their positions wound into sixteenths, both belts have their
 * geometry refreshed with the machine forced into state 0x1000, and the walk
 * ends: a belt is only cut once per pass.
 */
void cut_belts(uint16_t part, uint16_t line)
{
    uint16_t fp = dg_enter(0x26);
    uint16_t newbelt = (uint16_t)(fp + 0x00);   /* [bp-0x26] */
    uint16_t belt    = (uint16_t)(fp + 0x02);   /* [bp-0x24] */
    uint16_t endB    = (uint16_t)(fp + 0x04);   /* [bp-0x22] */
    uint16_t carrier = (uint16_t)(fp + 0x06);   /* [bp-0x1e] */
    uint16_t anchorB = (uint16_t)(fp + 0x08);   /* [bp-0x1c] */
    uint16_t next    = (uint16_t)(fp + 0x0c);   /* [bp-0x1a] */
    uint16_t prev    = (uint16_t)(fp + 0x0e);   /* [bp-0x18] */
    uint16_t endA    = (uint16_t)(fp + 0x10);   /* [bp-0x20] */
    uint16_t rec     = (uint16_t)(fp + 0x12);   /* [bp-0x16] */
    uint16_t seg     = (uint16_t)(fp + 0x14);   /* [bp-0x14], four words */
    uint16_t at      = (uint16_t)(fp + 0x1c);   /* [bp-0x0c], two words */
    uint16_t saved   = (uint16_t)(fp + 0x20);   /* [bp-8] */
    uint16_t slotB   = (uint16_t)(fp + 0x22);   /* [bp-6] */
    uint16_t slotA   = (uint16_t)(fp + 0x24);   /* [bp-4] */
    uint16_t i       = (uint16_t)(fp + 0x26 - 2); /* [bp-2] */
    uint16_t di;
    int16_t k;

    for (DGU16(rec) = DGU16(0x521b); DGU16(rec) != 0;
         DGU16(rec) = DGU16(DGU16(rec))) {

        if (DGU16((uint16_t)(DGU16(rec) + 4)) != 0x0a)
            continue;

        DGU16(belt) = DGU16((uint16_t)(DGU16(rec) + 0x66));
        DGU16(endA) = DGU16((uint16_t)(DGU16(belt) + 2));
        DGU16(prev) = DGU16(endA);
        DGU16(endB) = DGU16((uint16_t)(DGU16(belt) + 4));
        DG16(slotA) = (int16_t)DG8((uint16_t)(DGU16(belt) + 0x0a));
        DG16(slotB) = 0;
        DGU16(next) = DGU16((uint16_t)(DGU16(prev) + 0x5a
                                       + 2 * DGU16(slotA)));

        while (DGU16(prev) != 0 && DGU16(next) != 0) {
            if (DGU16(prev) != DGU16(endA))
                DG16(slotA) = 1;

            DG16(seg) = (int16_t)(
                DG16((uint16_t)(DGU16(prev) + 0x2a))
                + DG8((uint16_t)(DGU16(prev) + 0x6a + 2 * DGU16(slotA)))
                - DG16((uint16_t)(part + 0x1e)));
            DG16((uint16_t)(seg + 2)) = (int16_t)(
                DG16((uint16_t)(DGU16(prev) + 0x2c))
                + DG8((uint16_t)(DGU16(prev) + 0x6b + 2 * DGU16(slotA)))
                - DG16((uint16_t)(part + 0x20)));

            if (DGU16(next) == DGU16(endB))
                DG16(slotB) = (int16_t)DG8((uint16_t)(DGU16(belt) + 0x0b));

            DG16((uint16_t)(seg + 4)) = (int16_t)(
                DG16((uint16_t)(DGU16(next) + 0x2a))
                + DG8((uint16_t)(DGU16(next) + 0x6a + 2 * DGU16(slotB)))
                - DG16((uint16_t)(part + 0x1e)));
            DG16((uint16_t)(seg + 6)) = (int16_t)(
                DG16((uint16_t)(DGU16(next) + 0x2c))
                + DG8((uint16_t)(DGU16(next) + 0x6b + 2 * DGU16(slotB)))
                - DG16((uint16_t)(part + 0x20)));

            if (intersect_segments(line, seg, at) == 0) {
                if (DGU16(next) == DGU16(endB)) {
                    DGU16(next) = 0;
                    DGU16(prev) = 0;
                } else {
                    DGU16(prev) = DGU16(next);
                    DGU16(next) = DGU16((uint16_t)(DGU16(next) + 0x5a));
                }
                continue;
            }

            DG16(saved) = DG16(0x4e6b);
            DG16(0x4e6b) = 0x1000;
            mark_belt_shapes(DGU16(DGU16(belt)), 3);
            DG16(0x4e6b) = DG16(saved);

            di = make_part(0x31);
            if (di == 0)
                goto out;

            DGU16(anchorB) = make_part(0x31);
            if (DGU16(anchorB) == 0) {
                free_part(di);
                goto out;
            }

            DGU16(carrier) = make_part(0x0a);
            if (DGU16(carrier) == 0) {
                free_part(DGU16(anchorB));
                free_part(di);
                goto out;
            }

            insert_sorted(di, 0x5179);
            DGU16((uint16_t)(di + 6)) |= 0x10;
            DG16((uint16_t)(di + 0x1e)) =
                (int16_t)(DG16(at) + DG16((uint16_t)(part + 0x1e)));
            DG16((uint16_t)(di + 0x20)) =
                (int16_t)(DG16((uint16_t)(at + 2))
                          + DG16((uint16_t)(part + 0x20)));

            insert_sorted(DGU16(anchorB), 0x5179);
            DGU16((uint16_t)(DGU16(anchorB) + 6)) |= 0x10;
            DG16((uint16_t)(DGU16(anchorB) + 0x20)) =
                DG16((uint16_t)(di + 0x20));
            DG16((uint16_t)(DGU16(anchorB) + 0x1e)) =
                DG16((uint16_t)(di + 0x1e));

            insert_sorted(DGU16(carrier), 0x521b);
            DGU16((uint16_t)(DGU16(carrier) + 6)) |= 0x10;

            DGU16(newbelt) = DGU16((uint16_t)(DGU16(carrier) + 0x66));
            DGU16((uint16_t)(DGU16(newbelt) + 2)) = DGU16(anchorB);
            DGU16((uint16_t)(DGU16(newbelt) + 4)) = DGU16(endB);
            DG8((uint16_t)(DGU16(newbelt) + 0x0a)) = 0;
            DG8((uint16_t)(DGU16(newbelt) + 0x0b)) =
                DG8((uint16_t)(DGU16(belt) + 0x0b));

            DGU16((uint16_t)(DGU16(anchorB) + 0x5a)) = DGU16(next);
            DGU16((uint16_t)(DGU16(anchorB) + 0x66)) = DGU16(newbelt);

            if (DGU16((uint16_t)(DGU16(next) + 4)) == 7) {
                DGU16((uint16_t)(DGU16(next) + 0x68)) = DGU16(newbelt);
                DGU16((uint16_t)(DGU16(next) + 0x5c)) = DGU16(anchorB);
            } else {
                DGU16((uint16_t)(DGU16(next) + 0x66 + 2 * DGU16(slotB))) =
                    DGU16(newbelt);
                DGU16((uint16_t)(DGU16(next) + 0x5a + 2 * DGU16(slotB))) =
                    DGU16(anchorB);
            }

            DGU16((uint16_t)(DGU16(endB) + 0x66
                             + 2 * DG8((uint16_t)(DGU16(newbelt) + 0x0b)))) =
                DGU16(newbelt);

            DGU16((uint16_t)(DGU16(belt) + 4)) = di;
            DG8((uint16_t)(DGU16(belt) + 0x0b)) = 0;
            DGU16((uint16_t)(di + 0x5a)) = DGU16(prev);
            DGU16((uint16_t)(di + 0x66)) = DGU16(belt);

            if (DGU16((uint16_t)(DGU16(prev) + 4)) == 7)
                DGU16((uint16_t)(DGU16(prev) + 0x5a)) = di;
            else
                DGU16((uint16_t)(DGU16(prev) + 0x5a + 2 * DGU16(slotA))) = di;

            DG16((uint16_t)(di + 0x22)) = DG16((uint16_t)(di + 0x1e));
            DG16((uint16_t)(di + 0x26)) = DG16((uint16_t)(di + 0x1e));
            DG32((uint16_t)(di + 0x16)) = DG16((uint16_t)(di + 0x1e));
            DG32((uint16_t)(di + 0x16)) =
                (int32_t)long_shift_left(
                    (uint32_t)DG32((uint16_t)(di + 0x16)), 9);

            DG16((uint16_t)(di + 0x24)) = DG16((uint16_t)(di + 0x20));
            DG16((uint16_t)(di + 0x28)) = DG16((uint16_t)(di + 0x20));
            DG32((uint16_t)(di + 0x1a)) = DG16((uint16_t)(di + 0x20));
            DG32((uint16_t)(di + 0x1a)) =
                (int32_t)long_shift_left(
                    (uint32_t)DG32((uint16_t)(di + 0x1a)), 9);

            place_object_for_draw(di);

            DG16((uint16_t)(DGU16(anchorB) + 0x22)) =
                DG16((uint16_t)(DGU16(anchorB) + 0x1e));
            DG16((uint16_t)(DGU16(anchorB) + 0x26)) =
                DG16((uint16_t)(DGU16(anchorB) + 0x1e));
            DG32((uint16_t)(DGU16(anchorB) + 0x16)) =
                DG16((uint16_t)(DGU16(anchorB) + 0x1e));
            DG32((uint16_t)(DGU16(anchorB) + 0x16)) =
                (int32_t)long_shift_left(
                    (uint32_t)DG32((uint16_t)(DGU16(anchorB) + 0x16)), 9);

            DG16((uint16_t)(DGU16(anchorB) + 0x24)) =
                DG16((uint16_t)(DGU16(anchorB) + 0x20));
            DG16((uint16_t)(DGU16(anchorB) + 0x28)) =
                DG16((uint16_t)(DGU16(anchorB) + 0x20));
            DG32((uint16_t)(DGU16(anchorB) + 0x1a)) =
                DG16((uint16_t)(DGU16(anchorB) + 0x20));
            DG32((uint16_t)(DGU16(anchorB) + 0x1a)) =
                (int32_t)long_shift_left(
                    (uint32_t)DG32((uint16_t)(DGU16(anchorB) + 0x1a)), 9);

            place_object_for_draw(DGU16(anchorB));

            DG16(0x4e6b) = 0x1000;

            refresh_link_geometry(DGU16(belt));
            for (k = 0; k < 2; k++) {
                DG16((uint16_t)(DGU16(belt) + 0x1c + 4 * k)) =
                    DG16((uint16_t)(DGU16(belt) + 0x14 + 4 * k));
                DG16((uint16_t)(DGU16(belt) + 0x1e + 4 * k)) =
                    DG16((uint16_t)(DGU16(belt) + 0x16 + 4 * k));
                DG16((uint16_t)(DGU16(belt) + 0x24 + 4 * k)) =
                    DG16((uint16_t)(DGU16(belt) + 0x14 + 4 * k));
                DG16((uint16_t)(DGU16(belt) + 0x26 + 4 * k)) =
                    DG16((uint16_t)(DGU16(belt) + 0x16 + 4 * k));
            }

            refresh_link_geometry(DGU16(newbelt));
            for (k = 0; k < 2; k++) {
                DG16((uint16_t)(DGU16(newbelt) + 0x1c + 4 * k)) =
                    DG16((uint16_t)(DGU16(newbelt) + 0x14 + 4 * k));
                DG16((uint16_t)(DGU16(newbelt) + 0x1e + 4 * k)) =
                    DG16((uint16_t)(DGU16(newbelt) + 0x16 + 4 * k));
                DG16((uint16_t)(DGU16(newbelt) + 0x24 + 4 * k)) =
                    DG16((uint16_t)(DGU16(newbelt) + 0x14 + 4 * k));
                DG16((uint16_t)(DGU16(newbelt) + 0x26 + 4 * k)) =
                    DG16((uint16_t)(DGU16(newbelt) + 0x16 + 4 * k));
            }

            DG16(0x4e6b) = DG16(saved);

            DGU16(next) = 0;
            DGU16(prev) = 0;
        }
    }

out:
    (void)i;
    dg_leave(0x26);
}

/*
 * 172c:016e, image 0x1742e - kind 4's hit test.
 *
 * Only one kind of arrival counts: 0x14. That sets the part at the object's
 * +0x84 going at +0x12, and anything else touching it does nothing. The answer
 * is 1 either way.
 */
uint16_t part_hit_016e(uint16_t part)
{
    if (DGU16((uint16_t)(part + 4)) == 0x14)
        DGU16((uint16_t)(DGU16((uint16_t)(part + 0x84)) + 0x12)) = 1;

    return 1;
}

/*
 * 172c:018e, image 0x1744e - kind 4's step.
 *
 * A part that hands its belt over to something else and then disappears. At
 * form 6 it registers its shapes one last time and hides itself - bit 13 of
 * +8 - and that is the end of it.
 *
 * Before then, and only while +0x12 is exactly 1, it makes a kind-0x31 anchor,
 * puts it on the list at DGROUP 0x5179, and moves its belt across: the anchor
 * takes the belt at +0x66 and the link at +0x5a, the part on the far side of
 * that link is pointed back at the anchor through whichever of its own two
 * links matched - `match_field_5a_5c` - and the belt record's own end, +2 or
 * +4, is repointed too. The anchor lands on the belt's tangent point for that
 * end, carried in sixteenths the usual way, and this part lets go of both.
 *
 * Either way the form steps on, and the first step plays sound 0x0e.
 */
uint16_t part_step_018e(uint16_t part)
{
    uint16_t fp = dg_enter(6);
    uint16_t belt = fp;                     /* [bp-6] */
    uint16_t link = (uint16_t)(fp + 2);     /* [bp-4] */
    uint16_t k = (uint16_t)(fp + 4);        /* [bp-2] */
    uint16_t di = part;
    uint16_t si;

    if (DGU16((uint16_t)(di + 0x12)) == 0)
        goto out;

    DGU16((uint16_t)(di + 8)) |= 0x40;

    if (DGU16((uint16_t)(di + 0x0c)) == 6) {
        mark_part_shapes(di, 3);
        DGU16((uint16_t)(di + 8)) |= 0x2000;
        goto out;
    }

    if (DGU16((uint16_t)(di + 0x12)) != 1)
        goto step;

    DGU16(belt) = DGU16((uint16_t)(di + 0x66));
    if (DGU16(belt) == 0)
        goto step;

    si = make_part(0x31);
    if (si == 0)
        goto step;

    insert_sorted(si, 0x5179);
    DGU16((uint16_t)(si + 6)) |= 0x10;

    DGU16((uint16_t)(si + 0x66)) = DGU16(belt);
    DGU16((uint16_t)(si + 0x5a)) = DGU16((uint16_t)(di + 0x5a));
    DGU16(link) = DGU16((uint16_t)(si + 0x5a));

    DG16(k) = match_field_5a_5c((int16_t)di, DGU16(link));
    if (DG16(k) != -1)
        DGU16((uint16_t)(DGU16(link) + 0x5a + 2 * DGU16(k))) = si;

    if (DGU16((uint16_t)(DGU16(belt) + 2)) == di) {
        DGU16((uint16_t)(DGU16(belt) + 2)) = si;
        DG16((uint16_t)(si + 0x1e)) = DG16((uint16_t)(DGU16(belt) + 0x14));
        DG16((uint16_t)(si + 0x20)) = DG16((uint16_t)(DGU16(belt) + 0x16));
    } else {
        DGU16((uint16_t)(DGU16(belt) + 4)) = si;
        DG16((uint16_t)(si + 0x1e)) = DG16((uint16_t)(DGU16(belt) + 0x18));
        DG16((uint16_t)(si + 0x20)) = DG16((uint16_t)(DGU16(belt) + 0x1a));
    }

    DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
    DG32((uint16_t)(si + 0x16)) =
        (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x16)), 9);

    DG32((uint16_t)(si + 0x1a)) = DG16((uint16_t)(si + 0x20));
    DG32((uint16_t)(si + 0x1a)) =
        (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x1a)), 9);

    place_object_for_draw(si);

    DGU16((uint16_t)(di + 0x66)) = 0;
    DGU16((uint16_t)(di + 0x5a)) = 0;

step:
    if (DGU16((uint16_t)(di + 0x0c)) == 0)
        play_sound(0x0e);

    DGU16((uint16_t)(di + 0x0c))++;
    place_object_for_draw(di);

out:
    dg_leave(6);
    return 0;
}

/*
 * 172c:34d0, image 0x1a790 - kind 42's step. The mouse.
 *
 * It runs when it is startled and then stops. The countdown at +0x96 is how
 * many steps of running are left; each one flips the form between 0 and 1 and
 * moves it three or four pixels the way its mirror bit points - four on the
 * odd frame and three on the even, which is what makes the gait uneven.
 *
 * With the countdown spent it waits for a touch - bit 0 of +6 - and then looks
 * for a kind-0x0c anywhere in a box 0x80 either side and 8 below,
 * `link_nearby_objects` building the candidates. The slowest one it finds
 * decides which way it runs: something moving right sends it left and clears
 * the mirror bit, anything else sends it right. Five steps of running, and a
 * form of 1 to start.
 *
 * A form that has changed is carried into the sixteenths at +0x16 and drawn.
 */
uint16_t part_step_34d0(uint16_t part)
{
    uint16_t fp = dg_enter(4);
    uint16_t best = fp;                     /* [bp-4] the step, then... */
    uint16_t slowest = (uint16_t)(fp + 2);  /* [bp-2] */
    uint16_t si = part;
    uint16_t di;

    if (DG16((uint16_t)(si + 0x96)) != 0) {
        DGU16((uint16_t)(si + 0x96))--;
        DGU16((uint16_t)(si + 0x0c)) ^= 1;

        DG16(best) = (DGU16((uint16_t)(si + 0x0c)) != 0) ? 4 : 3;

        if (DGU16((uint16_t)(si + 8)) & 0x10)
            DG16((uint16_t)(si + 0x1e)) += DG16(best);
        else
            DG16((uint16_t)(si + 0x1e)) -= DG16(best);

        goto draw;
    }

    if (!(DGU16((uint16_t)(si + 6)) & 1))
        goto draw;

    link_nearby_objects(si, 0x1000, (int16_t)0xff80, 0x80, -8, 8);

    DG16(slowest) = 0x190;

    for (di = DGU16((uint16_t)(si + 0x78)); di != 0;
         di = DGU16((uint16_t)(di + 0x78))) {

        int16_t a, b;

        if (DGU16((uint16_t)(di + 4)) != 0x0c)
            continue;

        a = DG16((uint16_t)(di + 0x7a));
        if (a < 0)
            a = (int16_t)-a;
        b = DG16(slowest);
        if (b < 0)
            b = (int16_t)-b;

        if (a < b)
            DG16(slowest) = DG16((uint16_t)(di + 0x7a));
    }

    if (DG16(slowest) == 0x190)
        goto draw;

    DGU16((uint16_t)(si + 0x0c)) = 1;
    DGU16((uint16_t)(si + 0x96)) = 5;

    if (DG16(slowest) > 0) {
        DGU16((uint16_t)(si + 8)) &= 0xffef;
        DG16((uint16_t)(si + 0x1e)) -= 3;
    } else {
        DGU16((uint16_t)(si + 8)) |= 0x10;
        DG16((uint16_t)(si + 0x1e)) += 3;
    }

draw:
    if (DGU16((uint16_t)(si + 0x0c)) != DGU16((uint16_t)(si + 0x0e))) {
        DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
        DG32((uint16_t)(si + 0x16)) =
            (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x16)), 9);
        place_object_for_draw(si);
    }

    dg_leave(4);
    return 0;
}

/*
 * 172c:0ca3, image 0x17f63 - kind 12's step. The cat.
 *
 * It walks in jumps of 0x20, and every jump is checked before it is kept: the
 * cat is moved, `object_overlaps_any` asked whether that put it inside
 * something, and if it did the move is undone by *twice* the step - a jump the
 * other way - and checked again. If that fails too it goes back where it was
 * and sits down, form 0; if the second try worked it turns round, flipping bit
 * 4 of +8.
 *
 * Sitting still it looks for what is near: `link_nearby_objects` over a box
 * that reaches 0xf0 the way it faces and 0x110 the other, and each candidate
 * gets a range at which the cat will react - a mouse, kind 0x2a, inside a small
 * box is caught outright, hidden and sounded; a kind 0x0f is 0x124 away if it
 * is past form 0x0b and 0x60 otherwise; everything else is out of reach. The
 * first thing moving slower than its range sets the cat off.
 *
 * The two `+0x96` counters are the settling time: twelve steps of standing
 * before it will move again, and four steps of the tail flicking - form 1
 * through 9 - before it settles.
 */
uint16_t part_step_0ca3(uint16_t part)
{
    uint16_t fp = dg_enter(0x0c);
    uint16_t range = fp;                    /* [bp-0x0c] */
    uint16_t step  = (uint16_t)(fp + 0x02); /* [bp-0x0a] */
    uint16_t busy  = (uint16_t)(fp + 0x04); /* [bp-8] */
    uint16_t still = (uint16_t)(fp + 0x06); /* [bp-6] */
    uint16_t dy    = (uint16_t)(fp + 0x08); /* [bp-4] */
    uint16_t dx    = (uint16_t)(fp + 0x0a); /* [bp-2] */
    uint16_t si = part;
    uint16_t di;
    int16_t t;

    DG16(dy) = (int16_t)(DG16((uint16_t)(si + 0x20))
                         - DG16((uint16_t)(si + 0x28)));
    t = DG16(dy);
    if (t < 0)
        t = (int16_t)-t;
    DG16(still) = (t <= 1) ? 1 : 0;

    if (DGU16((uint16_t)(si + 8)) & 0x20) {
        if (DGU16((uint16_t)(si + 6)) & 2) {
            DGU16((uint16_t)(si + 8)) &= 0xffdf;
            DGU16((uint16_t)(si + 0x0c)) = 0;
        }
        goto draw;
    }

    if (DG16(still) == 0 || DG16((uint16_t)(si + 0x0c)) >= 2) {
        if (DGU16((uint16_t)(si + 0x0c)) == 1) {
            DGU16((uint16_t)(si + 0x96))++;
            if (DG16((uint16_t)(si + 0x96)) <= 0x0c)
                goto draw;

            DG16(step) = (DGU16((uint16_t)(si + 8)) & 0x10)
                         ? 0x20 : (int16_t)0xffe0;
            DGU16((uint16_t)(si + 0x96)) = 0;
            DG16((uint16_t)(si + 0x1e)) += DG16(step);
            place_object_for_draw(si);

            if (object_overlaps_any(si) != 0) {
                DG16((uint16_t)(si + 0x1e)) -= (int16_t)(DG16(step) * 2);
                place_object_for_draw(si);

                if (object_overlaps_any(si) != 0) {
                    DG16((uint16_t)(si + 0x1e)) += DG16(step);
                    place_object_for_draw(si);
                    DGU16((uint16_t)(si + 0x0c)) = 0;
                } else {
                    DGU16((uint16_t)(si + 0x0c)) = 2;
                    DGU16((uint16_t)(si + 8)) ^= 0x10;
                }
            } else {
                DGU16((uint16_t)(si + 0x0c)) = 2;
            }

            DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
            DG32((uint16_t)(si + 0x16)) =
                (int32_t)long_shift_left(
                    (uint32_t)DG32((uint16_t)(si + 0x16)), 9);
            goto draw;
        }

        if (DGU16((uint16_t)(si + 0x0c)) != 0) {
            DG16(busy) = 1;
            DGU16((uint16_t)(si + 0x0c))++;
            if (DGU16((uint16_t)(si + 0x0c)) == 0x0a)
                DGU16((uint16_t)(si + 0x0c)) = 0;
        } else {
            DG16(busy) = 0;
        }

        if (DGU16((uint16_t)(si + 0x0c)) != 0)
            goto draw;
    } else {
        if (DG16((uint16_t)(si + 0x96)) > 4) {
            DGU16((uint16_t)(si + 8)) |= 0x20;
            DGU16((uint16_t)(si + 0x0c)) = 1;
            DGU16((uint16_t)(si + 0x96)) = 0;
        } else {
            DGU16((uint16_t)(si + 0x96))++;
        }
        goto draw;
    }

    if (DGU16((uint16_t)(si + 8)) & 0x10)
        link_nearby_objects(si, 0x3000, 0, 0xf0, 0, 0);
    else
        link_nearby_objects(si, 0x3000, (int16_t)0xff10, 0, 0, 0);

    for (di = DGU16((uint16_t)(si + 0x78)); di != 0;
         di = DGU16((uint16_t)(di + 0x78))) {

        if (DGU16((uint16_t)(di + 4)) == 0x0f) {
            DG16(range) = (DG16((uint16_t)(di + 0x0c)) >= 0x0b)
                          ? 0x124 : 0x60;
        } else if (DGU16((uint16_t)(di + 4)) == 0x2a) {
            DG16(dx) = (int16_t)(DG16((uint16_t)(di + 0x1e))
                                 - DG16((uint16_t)(si + 0x1e)) + 0x10);
            DG16(dy) = (int16_t)(DG16((uint16_t)(di + 0x20))
                                 - DG16((uint16_t)(si + 0x20)));

            if (DG16(dx) > 0 && DG16(dx) < 0x38
                && DG16(dy) > 0 && DG16(dy) < 0x28) {
                mark_part_shapes(di, 3);
                DGU16((uint16_t)(di + 8)) |= 0x2000;
                play_sound(0x0d);
                DG16(range) = -1;
            } else {
                DG16(range) = (DG16(busy) != 0) ? 0xc0 : 0x80;
            }
        } else {
            DG16(range) = -1;
        }

        t = DG16((uint16_t)(di + 0x7a));
        if (t < 0)
            t = (int16_t)-t;
        if (t >= DG16(range))
            continue;

        DG16(step) = (DGU16((uint16_t)(si + 8)) & 0x10)
                     ? 0x20 : (int16_t)0xffe0;
        DGU16((uint16_t)(si + 0x96)) = 0;
        DG16((uint16_t)(si + 0x1e)) += DG16(step);
        place_object_for_draw(si);

        if (object_overlaps_any(si) != 0) {
            DG16((uint16_t)(si + 0x1e)) -= DG16(step);
            place_object_for_draw(si);
            DGU16((uint16_t)(si + 0x0c)) = 0;
        } else {
            DGU16((uint16_t)(si + 0x0c)) = 2;
        }

        di = 0;
        DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
        DG32((uint16_t)(si + 0x16)) =
            (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x16)), 9);
        break;
    }

draw:
    if (DGU16((uint16_t)(si + 0x0c)) != DGU16((uint16_t)(si + 0x0e)))
        place_object_for_draw(si);

    dg_leave(0x0c);
    return 0;
}

/*
 * 172c:11a6, image 0x18466 - kind 57's step.
 *
 * Four lines: in form 1, and only once something has given it a sideways
 * velocity at +0x36, it goes to form 3 and plays sound 3. Nothing else happens
 * to it at all.
 */
uint16_t part_step_11a6(uint16_t part)
{
    if (DGU16((uint16_t)(part + 0x0c)) != 1)
        return 0;
    if (DGU16((uint16_t)(part + 0x36)) == 0)
        return 0;

    DGU16((uint16_t)(part + 0x0c)) = 3;
    place_object_for_draw(part);
    play_sound(3);

    return 0;
}

/*
 * 172c:2514, image 0x197d4 - kind 5's hit test.
 *
 * A crank being turned pushes whatever is standing on it sideways at 0x1000,
 * building up to that speed rather than snapping to it: the speed is added and
 * then clamped, so a thing already going faster is left alone.
 *
 * Which way depends on the direction the thing hit carries at its +0x12 and on
 * the crank's own +0x8a: at 0 a positive direction pushes right, at 2 the two
 * are the other way round, and at anything else nothing happens at all. It
 * always answers 1 - the hit counts either way.
 */
uint16_t part_hit_2514(uint16_t part)
{
    uint16_t di = DGU16((uint16_t)(part + 0x84));
    int16_t cx = DG16((uint16_t)(di + 0x12));
    const int16_t v = 0x1000;

    if (DGU16((uint16_t)(part + 0x8a)) == 0) {
        if (cx > 0) {
            DG16((uint16_t)(part + 0x36)) += v;
            if (DG16((uint16_t)(part + 0x36)) > v)
                DG16((uint16_t)(part + 0x36)) = v;
        } else if (cx < 0) {
            DG16((uint16_t)(part + 0x36)) -= v;
            if (DG16((uint16_t)(part + 0x36)) < v)
                DG16((uint16_t)(part + 0x36)) = (int16_t)-v;
        }
    } else if (DGU16((uint16_t)(part + 0x8a)) == 2) {
        if (cx < 0) {
            DG16((uint16_t)(part + 0x36)) += v;
            if (DG16((uint16_t)(part + 0x36)) > v)
                DG16((uint16_t)(part + 0x36)) = v;
        } else if (cx > 0) {
            DG16((uint16_t)(part + 0x36)) -= v;
            if (DG16((uint16_t)(part + 0x36)) < v)
                DG16((uint16_t)(part + 0x36)) = (int16_t)-v;
        }
    }

    return 1;
}

/*
 * 172c:3fe8, image 0x1b2a8 - kind 3's hit test. Standing on the motor.
 *
 * A motor whose belt is held - bit 9 of +8 - answers 1 at once and does
 * nothing: it cannot be turned by being stood on.
 *
 * Otherwise the face that was touched, +0x8a of the thing that hit, decides.
 * Faces 0, 2 and 6 can turn it; anything else is a plain hit. Face 0 is the
 * top and is split by where along it the contact fell: past 0x2c is one end,
 * up to 0x24 the other, and between them nothing. Faces 2 and 6 are the sides
 * and turn it by which form it is in.
 *
 * Turning is not done here: `queue_part` asks for the motor to be stepped, and
 * only if the queue took it does the direction go across, with the asking
 * part's priority. A motor already on the queue at a better priority makes this
 * a plain hit instead - and the thing's own contact is cleared when the turn
 * was taken, so it does not also bounce.
 */
uint16_t part_hit_3fe8(uint16_t part)
{
    uint16_t fp = dg_enter(8);
    uint16_t plain = fp;                    /* [bp-8] */
    uint16_t dir = (uint16_t)(fp + 2);      /* [bp-6] */
    uint16_t along = (uint16_t)(fp + 4);    /* [bp-4] */
    uint16_t face = (uint16_t)(fp + 6);     /* [bp-2] */
    uint16_t di = part;
    uint16_t si = DGU16((uint16_t)(di + 0x84));
    uint16_t answer;

    if (DGU16((uint16_t)(si + 8)) & 0x200) {
        answer = 1;
        goto out;
    }

    DGU16(face) = DGU16((uint16_t)(di + 0x8a));

    DG16(plain) = (DGU16(face) == 0 || DGU16(face) == 2 || DGU16(face) == 6)
                  ? 0 : 1;

    if (DGU16(face) == 0) {
        DG16(along) = (int16_t)(DG16((uint16_t)(di + 0x1e))
                                + (DG16((uint16_t)(di + 0x44)) >> 1)
                                - DG16((uint16_t)(si + 0x1e)));

        if (DG16(along) >= 0x2c) {
            if (DGU16((uint16_t)(si + 0x0c)) == 2)
                DG16(plain) = 1;
            else
                DG16(dir) = 1;
        } else if (DG16(along) <= 0x24) {
            if (DGU16((uint16_t)(si + 0x0c)) == 0)
                DG16(plain) = 1;
            else
                DG16(dir) = -1;
        } else {
            DG16(plain) = 1;
        }
    } else if (DGU16(face) == 2) {
        if (DGU16((uint16_t)(si + 0x0c)) == 0)
            DG16(plain) = 1;
        else
            DG16(dir) = -1;
    } else if (DGU16(face) == 6) {
        if (DGU16((uint16_t)(si + 0x0c)) == 2)
            DG16(plain) = 1;
        else
            DG16(dir) = 1;
    }

    if (DG16(plain) == 0) {
        if (queue_part(di, DGU16((uint16_t)(di + 0x84))) != 0) {
            DG16((uint16_t)(si + 0x12)) = DG16(dir);
            DGU16((uint16_t)(si + 0x3e)) = DGU16((uint16_t)(di + 0x3e));
            DGU16((uint16_t)(si + 0x3c)) = DGU16((uint16_t)(di + 0x3c));
            DGU16((uint16_t)(di + 0x84)) = 0;
        } else {
            DG16(plain) = 1;
        }
    }

    answer = DGU16(plain);

out:
    dg_leave(8);
    return answer;
}

/*
 * 172c:0552, image 0x17812 - kind 35's hit test.
 *
 * Being hit on face 2 sets the thing that hit it going; any other face does
 * nothing. It answers 1 either way, so the hit still counts.
 */
uint16_t part_hit_0552(uint16_t part)
{
    uint16_t di = DGU16((uint16_t)(part + 0x84));

    if (DGU16((uint16_t)(part + 0x8a)) == 2)
        DGU16((uint16_t)(di + 0x12)) = 1;

    return 1;
}

/*
 * 172c:3824, image 0x1aae4 - kind 37's hit test. Closing the scissors.
 *
 * Four of the eight faces set the thing that hit it going, and *which* four
 * depends on the mirror bit of the thing itself, not of the scissors: 1, 2, 4
 * and 5 mirrored, 0, 1, 5 and 6 not. One more face - 7 mirrored, 3 not - sets
 * the *scissors* going instead, and only when they were hit by a kind 4.
 *
 * It answers 1 whatever happened.
 */
uint16_t part_hit_3824(uint16_t part)
{
    uint16_t di = DGU16((uint16_t)(part + 0x84));
    int16_t face = DG16((uint16_t)(part + 0x8a));

    if (DGU16((uint16_t)(di + 8)) & 0x10) {
        if (face == 1 || face == 2 || face == 4 || face == 5)
            DGU16((uint16_t)(di + 0x12)) = 1;
        else if (face == 7 && DGU16((uint16_t)(part + 4)) == 4)
            DGU16((uint16_t)(part + 0x12)) = 1;
    } else {
        if (face == 0 || face == 1 || face == 5 || face == 6)
            DGU16((uint16_t)(di + 0x12)) = 1;
        else if (face == 3 && DGU16((uint16_t)(part + 4)) == 4)
            DGU16((uint16_t)(part + 0x12)) = 1;
    }

    return 1;
}

/*
 * 172c:0c6c, image 0x17f2c - kind 12's hit test. Waking the cat.
 *
 * A cat in form 0 is put into form 1 with its counter cleared and mews - sound
 * 7. One already awake is left alone. It answers 1 either way.
 */
uint16_t part_hit_0c6c(uint16_t part)
{
    uint16_t si = DGU16((uint16_t)(part + 0x84));

    if (DGU16((uint16_t)(si + 0x0c)) == 0) {
        DGU16((uint16_t)(si + 0x0c)) = 1;
        DGU16((uint16_t)(si + 0x96)) = 0;
        place_object_for_draw(si);
        play_sound(7);
    }

    return 1;
}

/*
 * 172c:14d3, image 0x18793 - kind 21's hit test. The see-saw.
 *
 * Which way it tips comes from the angle at +0x88 of the thing that hit it,
 * turned a quarter and then read as a sign: the high bit of `angle + 0x4000`.
 * Below form 4 and pushed one way it goes up by four; at form 4 or above and
 * pushed the other it comes down by four, and either move runs its own setup
 * again and plays sound 0x11.
 *
 * Then it is *on* whenever its form is not the one at +0x90 - which is where a
 * see-saw's rest position is kept - and a kind 0x14 that hit it has its
 * sideways velocity stepped down by one. It answers 0, so the hit does not
 * count as a landing.
 */
uint16_t part_hit_14d3(uint16_t part)
{
    uint16_t di = part;
    uint16_t si = DGU16((uint16_t)(di + 0x84));
    uint16_t turned = (uint16_t)(DGU16((uint16_t)(di + 0x88)) + 0x4000);

    if (DG16((uint16_t)(si + 0x0c)) < 4) {
        if (!(turned & 0x8000)) {
            DG16((uint16_t)(si + 0x0c)) += 4;
            part_setup(0x1556, si);
            play_sound(0x11);
        }
    } else if (turned & 0x8000) {
        DG16((uint16_t)(si + 0x0c)) -= 4;
        part_setup(0x1556, si);
        play_sound(0x11);
    }

    DGU16((uint16_t)(si + 0x12)) =
        (DGU16((uint16_t)(si + 0x0c)) != DGU16((uint16_t)(si + 0x90))) ? 1 : 0;

    if (DGU16((uint16_t)(di + 4)) == 0x14)
        DGU16((uint16_t)(di + 0x36))--;

    return 0;
}

/*
 * 172c:1c39, image 0x18ef9 - kind 15's hit test.
 *
 * A kind 15 already past form 0x0b is broken and the hit counts - answer 1.
 * One that is not gets broken by the hit and the hit does *not* count, so the
 * thing that broke it carries on through rather than bouncing off.
 */
uint16_t part_hit_1c39(uint16_t part)
{
    uint16_t si = DGU16((uint16_t)(part + 0x84));

    if (DG16((uint16_t)(si + 0x0c)) >= 0x0b)
        return 1;

    break_kind_15(si);
    return 0;
}

/*
 * 172c:34b5, image 0x1a775 - kind 42's hit test.
 *
 * It reads the thing that hit it and does nothing with it: the mouse is solid
 * and that is all. Answers 1.
 */
uint16_t part_hit_34b5(uint16_t part)
{
    (void)DGU16((uint16_t)(part + 0x84));
    return 1;
}

/*
 * 172c:2f25, image 0x1a1e5 - kind 6's hit test. The mousetrap.
 *
 * Anything that touches a trap springs it, whatever it was: the hook is the
 * trap's, run on the object that arrived, and the trap itself is the one at
 * that object's +0x84. `trigger_kind_6` does the rest.
 */
uint16_t part_hit_2f25(uint16_t part)
{
    trigger_kind_6(DGU16((uint16_t)(part + 0x84)));
    return 1;
}

/*
 * 172c:2f3e, image 0x1a1fe - kind 6's step. The mousetrap.
 *
 * A trap that is not already going looks for a mouse - kind 0x0c - within
 * 0x10 either side, and the first one it finds sets it off. Going or not, it
 * passes its own state along its rope to whatever is not already busy.
 *
 * While it is going the form flips between two and the countdown at +0x96 runs
 * out; reaching zero switches it off again.
 */
uint16_t part_step_2f3e(uint16_t part)
{
    uint16_t si = part;
    uint16_t di;

    if (DGU16((uint16_t)(si + 0x12)) == 0) {
        link_nearby_objects(si, 0x1000, -0x10, 0x10, 0, 0);

        for (di = DGU16((uint16_t)(si + 0x78)); di != 0;
             di = DGU16((uint16_t)(di + 0x78))) {

            if (DGU16((uint16_t)(di + 4)) == 0x0c) {
                DGU16((uint16_t)(si + 0x12)) = 1;
                break;
            }
        }
    }

    di = rope_other_end(si);
    if (di != 0 && !(DGU16((uint16_t)(di + 8)) & 0x800))
        DGU16((uint16_t)(di + 0x12)) = DGU16((uint16_t)(si + 0x12));

    if (DGU16((uint16_t)(si + 0x12)) != 0) {
        DGU16((uint16_t)(si + 0x0c)) ^= 1;
        DGU16((uint16_t)(si + 0x96))--;
        if (DGU16((uint16_t)(si + 0x96)) == 0)
            DGU16((uint16_t)(si + 0x12)) = 0;
    }

    return 0;
}

/*
 * 172c:13c9, image 0x18689 - kind 50's step.
 *
 * It passes its own state down its rope - as 1 or -1 by its mirror bit while
 * it is on, and as 0 when it is off - and, while it is on, runs its three
 * frames backwards, wrapping -1 round to 2. The first frame of a turn plays
 * sound 0x0c and sets DGROUP 0x52cd to 2.
 */
uint16_t part_step_13c9(uint16_t part)
{
    uint16_t si = part;
    uint16_t di = rope_other_end(si);

    if (di != 0 && !(DGU16((uint16_t)(di + 8)) & 0x800)) {
        if (DGU16((uint16_t)(si + 0x12)) == 0)
            DGU16((uint16_t)(di + 0x12)) = 0;
        else
            DGU16((uint16_t)(di + 0x12)) =
                (DGU16((uint16_t)(si + 8)) & 0x10) ? 1 : 0xffff;
    }

    if (DGU16((uint16_t)(si + 0x12)) == 0)
        return 0;

    DGU16(0x52cd) = 2;

    if (DGU16((uint16_t)(si + 0x0c)) == DGU16((uint16_t)(si + 0x0e)))
        play_sound(0x0c);

    DGU16((uint16_t)(si + 0x0c))--;
    if (DG16((uint16_t)(si + 0x0c)) == -1)
        DGU16((uint16_t)(si + 0x0c)) = 2;

    return 0;
}

/*
 * 172c:1d07, image 0x18fc7 - kind 25's hit test.
 *
 * The hook is the *linked* thing's, run on whatever ran into it: `di` is the
 * kind 25 part at the hit object's +0x84 and `si` the object that arrived.
 *
 * Unless the arriving object is already spoken for - a non-zero +0x88 - the
 * kind 25 part is set going, which its step at 172c:1d78 then acts on. Either
 * way the answer is 1: the hit counts.
 */
uint16_t part_hit_1d07(uint16_t part)
{
    uint16_t si = part;
    uint16_t di = DGU16((uint16_t)(si + 0x84));

    if (DGU16((uint16_t)(si + 0x88)) == 0)
        DGU16((uint16_t)(di + 0x12)) = 1;

    return 1;
}

/*
 * 172c:1d78, image 0x19038 - kind 25's step.
 *
 * One move and then nothing: turned on in form 0 it steps to form 1, runs its
 * own setup again because the shape has changed, and plays sound 0x11. In any
 * other form it does nothing at all.
 */
uint16_t part_step_1d78(uint16_t part)
{
    if (DGU16((uint16_t)(part + 0x12)) == 0)
        return 0;
    if (DGU16((uint16_t)(part + 0x0c)) != 0)
        return 0;

    DGU16((uint16_t)(part + 0x0c))++;
    part_setup(0x1d28, part);
    place_object_for_draw(part);
    play_sound(0x11);

    return 0;
}

/*
 * 172c:1de0, image 0x190a0 - kind 26's hit test. The pulley wheel.
 *
 * Nothing happens. The original still loads the part at the object's +0x84 into
 * a local and then never reads it, which is a hook written from the same
 * template as the ones that do use it - so the wheel is touchable and is
 * unmoved by being touched.
 */
uint16_t part_hit_1de0(uint16_t part)
{
    (void)part;
    return 1;
}

/*
 * 172c:1e5c, image 0x1911c - kind 26's step. The pulley wheel.
 *
 * It stops if the gear its rope reaches is not turning - kind 0x0e with its
 * last two forms equal - and otherwise runs its four frames in the direction
 * its +0x12 says, wrapping within the low two bits so the form's other bits
 * survive the turn. The first frame plays sound 0x0c and sets DGROUP 0x52cd.
 *
 * Whatever happens it passes its own state on to links 4 and 5.
 */
uint16_t part_step_1e5c(uint16_t part)
{
    uint16_t si = part;
    int16_t i;

    if (DGU16((uint16_t)(si + 0x12)) != 0) {
        uint16_t di = rope_other_end(si);

        if (di != 0 && DGU16((uint16_t)(di + 4)) == 0x0e
            && DGU16((uint16_t)(di + 0x0e)) == DGU16((uint16_t)(di + 0x10)))
            DGU16((uint16_t)(si + 0x12)) = 0;
    }

    if (DGU16((uint16_t)(si + 0x12)) != 0) {
        DGU16(0x52cd) = 2;

        if (DGU16((uint16_t)(si + 0x0c)) == DGU16((uint16_t)(si + 0x0e)))
            play_sound(0x0c);

        if (DG16((uint16_t)(si + 0x12)) > 0) {
            if ((DGU16((uint16_t)(si + 0x0c)) & 3) == 3)
                DG16((uint16_t)(si + 0x0c)) -= 3;
            else
                DGU16((uint16_t)(si + 0x0c))++;
        } else {
            if ((DGU16((uint16_t)(si + 0x0c)) & 3) == 0)
                DG16((uint16_t)(si + 0x0c)) += 3;
            else
                DGU16((uint16_t)(si + 0x0c))--;
        }

        place_object_for_draw(si);
    }

    for (i = 4; i < 6; i++) {
        uint16_t di = DGU16((uint16_t)(si + 0x5a + 2 * i));

        if (di != 0)
            DGU16((uint16_t)(di + 0x12)) = DGU16((uint16_t)(si + 0x12));
    }

    return 0;
}

/*
 * 172c:3ebf, image 0x1b17f - kind 39's hit test.
 *
 * The hook belongs to the kind 39 part - `di`, at the arriving object's +0x84 -
 * and runs on whatever arrived, `si`.
 *
 * It only catches a thing that lands squarely on it: an object already spoken
 * for at +0x8a is refused, and so is one whose middle is more than 14 across
 * from the part's own middle, both by answering 1.
 *
 * Caught, the part is set going at +0x12 and the object is let go: a sideways
 * speed of 0x400 or more is halved, the link at +0x84 is dropped and the "in
 * contact" bit of +6 with it, and the sixteenths at +0x1a are rebuilt from the
 * whole pixels so the release leaves no fractional position behind.
 *
 * Form 3 - and only form 3 - throws it as well: the downward speed at +0x38
 * becomes minus its own size, less another 0x400, and `clamp_record_pair` holds
 * that to what the kind allows.
 */
uint16_t part_hit_3ebf(uint16_t part)
{
    uint16_t si = part;
    uint16_t di = DGU16((uint16_t)(si + 0x84));
    int16_t apart, v;
    int32_t q;

    if (DGU16((uint16_t)(si + 0x8a)) != 0)
        return 1;

    apart = (int16_t)((DG16((uint16_t)(si + 0x1e))
                       + (int16_t)(DG16((uint16_t)(si + 0x44)) >> 1))
                      - (DG16((uint16_t)(di + 0x1e))
                         + (int16_t)(DG16((uint16_t)(di + 0x44)) >> 1)));

    if (apart < -0x0e || apart > 0x0e)
        return 1;

    DGU16((uint16_t)(di + 0x12)) = 1;

    v = DG16((uint16_t)(si + 0x36));
    if ((v < 0 ? (int16_t)-v : v) >= 0x400)
        DG16((uint16_t)(si + 0x36)) = (int16_t)(v >> 1);

    DGU16((uint16_t)(si + 0x84)) = 0;
    DGU16((uint16_t)(si + 6)) &= 0xfffe;

    q = (int32_t)DG16((uint16_t)(si + 0x20)) << 9;
    DG16((uint16_t)(si + 0x1c)) = (int16_t)(q >> 16);
    DG16((uint16_t)(si + 0x1a)) = (int16_t)q;

    if (DGU16((uint16_t)(di + 0x0c)) == 3) {
        v = DG16((uint16_t)(si + 0x38));
        DG16((uint16_t)(si + 0x38)) =
            (int16_t)(-(v < 0 ? (int16_t)-v : v) - 0x400);
        clamp_record_pair(si);
    }

    return 0;
}

/*
 * 172c:3fae, image 0x1b26e - kind 39's step.
 *
 * Five frames once it is set going, the third playing sound 3, and reaching
 * the fifth wraps the form back to 0 and switches it off - so it plays through
 * and stops rather than looping.
 */
uint16_t part_step_3fae(uint16_t part)
{
    if (DGU16((uint16_t)(part + 0x12)) == 0)
        return 0;

    DGU16((uint16_t)(part + 0x0c))++;

    if (DGU16((uint16_t)(part + 0x0c)) == 3)
        play_sound(3);

    if (DGU16((uint16_t)(part + 0x0c)) == 5) {
        DGU16((uint16_t)(part + 0x0c)) = 0;
        DGU16((uint16_t)(part + 0x12)) = 0;
    }

    place_object_for_draw(part);

    return 0;
}

/*
 * 172c:27e2, image 0x19aa2 - kind 13's step. The conveyor.
 *
 * Above form 7 it is winding down: the form just runs on to 0x12 and stops
 * there. At 7 or below and switched on it steps the form by its direction -
 * reversed by the mirror bit - and wraps 8 back to 0 and -1 back to 7, counting
 * a lap at +0x96 each time. Six laps play sound 3 and put it into form 8, which
 * is where the winding-down starts.
 *
 * From form 8 up it reaches out over the belt: a box `0x3384 + 2 * form` wide
 * and 0x1f down, and everything in it is dealt with by kind. One that can be
 * knocked along gets a speed from `conveyor_speed_for_mass`, negative sideways
 * *and* negative downwards - which is what tips a thing off the end. The rest
 * go through a jump table of six kinds, and the four that are not
 * `break_kind_15` or `trigger_kind_6` are each given the middle of the conveyor
 * to compare themselves against.
 */
uint16_t part_step_27e2(uint16_t part)
{
    uint16_t fp = dg_enter(8);
    uint16_t mid = (uint16_t)(fp + 2);      /* [bp-6] */
    uint16_t push = (uint16_t)(fp + 4);     /* [bp-4] */
    uint16_t dir = (uint16_t)(fp + 6);      /* [bp-2] */
    uint16_t si = part;
    uint16_t di;

    if (DG16((uint16_t)(si + 0x0c)) > 7) {
        if (DGU16((uint16_t)(si + 0x0c)) != 0x12)
            DGU16((uint16_t)(si + 0x0c))++;
    } else if (DGU16((uint16_t)(si + 0x12)) != 0) {
        DG16(dir) = (DGU16((uint16_t)(si + 8)) & 0x10)
                    ? (int16_t)-DG16((uint16_t)(si + 0x12))
                    : DG16((uint16_t)(si + 0x12));

        DG16((uint16_t)(si + 0x0c)) += DG16(dir);

        if (DGU16((uint16_t)(si + 0x0c)) == 8) {
            DGU16((uint16_t)(si + 0x0c)) = 0;
            DGU16((uint16_t)(si + 0x96))++;
        } else if (DG16((uint16_t)(si + 0x0c)) == -1) {
            DGU16((uint16_t)(si + 0x0c)) = 7;
            DGU16((uint16_t)(si + 0x96))++;
        }

        if (DGU16((uint16_t)(si + 0x96)) == 6) {
            play_sound(3);
            DGU16((uint16_t)(si + 0x0c)) = 8;
        }
    }

    if (DG16((uint16_t)(si + 0x0c)) >= 8
        && DG16((uint16_t)(si + 0x0c)) <= 0x0a) {

        DG16(mid) = (int16_t)(DG16((uint16_t)(si + 0x1e))
                              + (DG16((uint16_t)(si + 0x44)) >> 1));

        link_objects_in_range(
            si, 0x3000, 0, 0x1f,
            DG16((uint16_t)(0x3384 + 2 * DGU16((uint16_t)(si + 0x0c)))), 0);

        for (di = DGU16((uint16_t)(si + 0x78)); di != 0;
             di = DGU16((uint16_t)(di + 0x78))) {

            if (DGU16((uint16_t)(di + 6)) & 0x1000) {
                DG16(push) = conveyor_speed_for_mass(di);

                DG16((uint16_t)(di + 0x36)) =
                    (DGU16((uint16_t)(si + 8)) & 0x10)
                    ? DG16(push) : (int16_t)-DG16(push);
                DG16((uint16_t)(di + 0x38)) = (int16_t)-DG16(push);
                continue;
            }

            switch (DGU16((uint16_t)(di + 4))) {
            case 0x0f: break_kind_15(di); break;
            case 0x06: trigger_kind_6(di); break;
            case 0x03: conveyor_nudge_3(di, DG16(mid)); break;
            case 0x10: conveyor_nudge_10(di, DG16(mid)); break;
            case 0x15: conveyor_nudge_15(di, DG16(mid)); break;
            case 0x25: conveyor_nudge_25(di, DG16(mid)); break;
            default: break;
            }
        }
    }

    if (DGU16((uint16_t)(si + 0x0c)) != DGU16((uint16_t)(si + 0x0e)))
        place_object_for_draw(si);

    dg_leave(8);
    return 0;
}

/*
 * 172c:29c6, image 0x19c86
 *
 * How fast the conveyor throws a thing, by its mass: nine steps from 0x1800
 * for the lightest down to 0x800 for the heaviest. The third of these ladders
 * in the module, and the slowest of them.
 */
int16_t conveyor_speed_for_mass(uint16_t obj)
{
    int16_t m = DG16((uint16_t)(0x0ea8
                                + 0x3a * (int16_t)DG16((uint16_t)(obj + 4))));

    if (m < 0x0002) return 0x1800;
    if (m < 0x0006) return 0x1600;
    if (m < 0x000a) return 0x1400;
    if (m < 0x0015) return 0x1200;
    if (m < 0x0079) return 0x1000;
    if (m < 0x0097) return 0x0e00;
    if (m < 0x00c9) return 0x0c00;
    if (m < 0x0709) return 0x0a00;
    return 0x0800;
}

/*
 * 172c:2a3a, image 0x19cfa - a kind-3 motor on the conveyor.
 *
 * Which way the conveyor turns it depends on the form it is in and on which
 * side of the conveyor's middle it sits: form 0 only turns one way, form 1
 * turns either, form 2 only the other.
 */
void conveyor_nudge_3(uint16_t obj, int16_t mid)
{
    if (DGU16((uint16_t)(obj + 0x0c)) == 0) {
        if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) + 0x24) > mid)
            DGU16((uint16_t)(obj + 0x12)) = 1;
    } else if (DGU16((uint16_t)(obj + 0x0c)) == 1) {
        if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) + 0x28) > mid)
            DGU16((uint16_t)(obj + 0x12)) = 1;
        else
            DGU16((uint16_t)(obj + 0x12)) = 0xffff;
    } else if (DGU16((uint16_t)(obj + 0x0c)) == 2) {
        if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) + 0x2c) < mid)
            DGU16((uint16_t)(obj + 0x12)) = 0xffff;
    }
}

/*
 * 172c:2a91, image 0x19d51 - a kind-0x10 on the conveyor.
 *
 * Only in form 0, and the offset it measures from and the direction of the
 * comparison both come from its mirror bit.
 */
void conveyor_nudge_10(uint16_t obj, int16_t mid)
{
    if (DGU16((uint16_t)(obj + 0x0c)) != 0)
        return;

    if (DGU16((uint16_t)(obj + 8)) & 0x10) {
        if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) + 0x0c) < mid)
            DGU16((uint16_t)(obj + 0x12)) = 1;
    } else {
        if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) + 0x2c) > mid)
            DGU16((uint16_t)(obj + 0x12)) = 1;
    }
}

/*
 * 172c:2acb, image 0x19d8b - a kind-0x15 see-saw on the conveyor.
 *
 * A see-saw already tipped one way and sitting between two and twenty pixels
 * of the conveyor's middle is tipped back - four off the form, its own setup
 * run again, and sound 0x11 - and then told whether it is at rest by comparing
 * the form with the one at +0x90.
 */
void conveyor_nudge_15(uint16_t obj, int16_t mid)
{
    if (DG16((uint16_t)(obj + 0x0c)) < 4)
        return;
    if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) - 2) >= mid)
        return;
    if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) + 0x14) <= mid)
        return;

    DG16((uint16_t)(obj + 0x0c)) -= 4;
    part_setup(0x1556, obj);
    play_sound(0x11);

    DGU16((uint16_t)(obj + 0x12)) =
        (DGU16((uint16_t)(obj + 0x0c)) != DGU16((uint16_t)(obj + 0x90)))
        ? 1 : 0;
}

/*
 * 172c:2b1e, image 0x19dde - a kind-0x25 on the conveyor.
 *
 * The same shape as the kind-0x10 nudge with different offsets: 0x12 mirrored
 * and 0x18 not.
 */
void conveyor_nudge_25(uint16_t obj, int16_t mid)
{
    if (DGU16((uint16_t)(obj + 0x0c)) != 0)
        return;

    if (DGU16((uint16_t)(obj + 8)) & 0x10) {
        if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) + 0x12) < mid)
            DGU16((uint16_t)(obj + 0x12)) = 1;
    } else {
        if ((int16_t)(DG16((uint16_t)(obj + 0x1e)) + 0x18) > mid)
            DGU16((uint16_t)(obj + 0x12)) = 1;
    }
}

/*
 * 172c:22ae, image 0x1956e - kind 27's step. The gun.
 *
 * Six frames once it is set going, the second playing sound 0x0b, and the
 * third fires: `make_part` builds a kind 0x14, `insert_sorted` puts it on the
 * list at DGROUP 0x5179, and it gets a position and a sideways velocity left
 * or right by the mirror bit - and, mirrored, its own setup at 172c:08a1 runs
 * as well, because the shot is a different shape that way round.
 *
 * A gun that could not get the shot from the heap simply does not fire.
 */
uint16_t part_step_22ae(uint16_t part)
{
    uint16_t di = part;
    uint16_t si;

    if (DGU16((uint16_t)(di + 0x12)) == 0)
        return 0;
    if (DGU16((uint16_t)(di + 0x0c)) == 6)
        return 0;

    DGU16((uint16_t)(di + 0x0c))++;
    place_object_for_draw(di);

    if (DGU16((uint16_t)(di + 0x0c)) == 2)
        play_sound(0x0b);

    if (DGU16((uint16_t)(di + 0x0c)) != 3)
        return 0;

    si = make_part(0x14);
    if (si == 0)
        return 0;

    insert_sorted(si, 0x5179);
    DGU16((uint16_t)(si + 6)) |= 0x10;

    if (DGU16((uint16_t)(di + 8)) & 0x10) {
        DGU16((uint16_t)(si + 8)) |= 0x10;
        part_setup(0x08a1, si);

        DG16((uint16_t)(si + 0x1e)) =
            (int16_t)(DG16((uint16_t)(di + 0x1e)) - 0x20);
        DG16((uint16_t)(si + 0x26)) =
            (int16_t)(DG16((uint16_t)(si + 0x1e)) + 0x18);
        DG16((uint16_t)(si + 0x22)) = DG16((uint16_t)(si + 0x26));
        DGU16((uint16_t)(si + 0x36)) = 0xd000;
    } else {
        DG16((uint16_t)(si + 0x1e)) =
            (int16_t)(DG16((uint16_t)(di + 0x1e)) + 0x24);
        DG16((uint16_t)(si + 0x26)) =
            (int16_t)(DG16((uint16_t)(si + 0x1e)) - 0x18);
        DG16((uint16_t)(si + 0x22)) = DG16((uint16_t)(si + 0x26));
        DGU16((uint16_t)(si + 0x36)) = 0x3000;
    }

    DG16((uint16_t)(si + 0x28)) =
        (int16_t)(DG16((uint16_t)(di + 0x20)) + 3);
    DG16((uint16_t)(si + 0x24)) = DG16((uint16_t)(si + 0x28));
    DG16((uint16_t)(si + 0x20)) = DG16((uint16_t)(si + 0x28));

    clamp_record_pair(si);

    DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
    DG32((uint16_t)(si + 0x16)) =
        (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x16)), 9);

    DG32((uint16_t)(si + 0x1a)) = DG16((uint16_t)(si + 0x20));
    DG32((uint16_t)(si + 0x1a)) =
        (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x1a)), 9);

    place_object_for_draw(si);

    return 0;
}

/*
 * 172c:098a, image 0x17c4a - kind 45's step. The paddle wheel.
 *
 * Once its +0x9c has counted past 0x14 it starts itself, and then runs four
 * frames on a loop - 5 wraps back to 1, so frame 0 is only ever the first one.
 *
 * Every step it reaches for the point at +0x72 of whatever is nearby, in a box
 * 9 to 0x12 across and ten up to five down, and switches each on. In the odd
 * frames it reaches again over the same box for two kinds by name: a kind 4 is
 * switched on, and a cat - kind 0x0c - in form 0 is woken with sound 7.
 */
uint16_t part_step_098a(uint16_t part)
{
    uint16_t di = part;
    uint16_t si;

    if (DGU16((uint16_t)(di + 0x12)) == 0
        && DG16((uint16_t)(di + 0x9c)) > 0x14)
        DGU16((uint16_t)(di + 0x12)) = 1;

    if (DGU16((uint16_t)(di + 0x12)) == 0)
        return 0;

    if (DGU16((uint16_t)(di + 0x0c)) == 5)
        DGU16((uint16_t)(di + 0x0c)) = 1;
    else
        DGU16((uint16_t)(di + 0x0c))++;

    place_object_for_draw(di);

    link_objects_at_point(di, 9, 0x12, -10, 5);

    for (si = DGU16((uint16_t)(di + 0x78)); si != 0;
         si = DGU16((uint16_t)(si + 0x78))) {

        if (DGU16((uint16_t)(si + 0x12)) == 0)
            DGU16((uint16_t)(si + 0x12)) = 1;
    }

    if (!(DGU16((uint16_t)(di + 0x0c)) & 1))
        return 0;

    link_objects_in_range(di, 0x1000, 9, 0x12, -10, 5);

    for (si = DGU16((uint16_t)(di + 0x78)); si != 0;
         si = DGU16((uint16_t)(si + 0x78))) {

        if (DGU16((uint16_t)(si + 4)) == 4) {
            DGU16((uint16_t)(si + 0x12)) = 1;
            continue;
        }

        if (DGU16((uint16_t)(si + 4)) != 0x0c)
            continue;
        if (DGU16((uint16_t)(si + 0x0c)) != 0)
            continue;

        DGU16((uint16_t)(si + 0x0c)) = 1;
        DGU16((uint16_t)(si + 0x96)) = 0;
        place_object_for_draw(si);
        play_sound(7);
    }

    return 0;
}

/*
 * 172c:12c2, image 0x18582 - kind 19's step. The balloon.
 *
 * It starts itself once its counter passes 0x14, then rises a frame at a time
 * until form 5, which is where it bursts.
 */
uint16_t part_step_12c2(uint16_t part)
{
    if (DGU16((uint16_t)(part + 0x12)) == 0
        && DG16((uint16_t)(part + 0x9c)) > 0x14)
        DGU16((uint16_t)(part + 0x12)) = 1;

    if (DGU16((uint16_t)(part + 0x12)) == 0)
        return 0;

    if (DGU16((uint16_t)(part + 0x0c)) == 5) {
        burst_kind_19(part);
        return 0;
    }

    DGU16((uint16_t)(part + 0x0c))++;
    place_object_for_draw(part);

    return 0;
}

/*
 * 172c:1328, image 0x185e8
 *
 * Burst it: the form goes to 5, a kind 0x29 - the shreds - is made and put on
 * the list at DGROUP 0x521b at a fixed offset up and to the left, sound 8
 * plays, and the balloon itself registers its shapes one last time and hides.
 *
 * A burst that could not get the shreds from the heap still hides the balloon,
 * because the `jmp` past the allocation lands *after* the form was set and
 * before the hiding - so a machine out of memory loses the shreds and not the
 * burst.
 */
void burst_kind_19(uint16_t part)
{
    uint16_t di = part;
    uint16_t si;

    DGU16((uint16_t)(di + 0x0c)) = 5;

    si = make_part(0x29);
    if (si != 0) {
        play_sound(8);

        insert_sorted(si, 0x521b);
        DGU16((uint16_t)(si + 6)) |= 0x10;

        DG16((uint16_t)(si + 0x1e)) =
            (int16_t)(DG16((uint16_t)(di + 0x1e)) - 0x0f);
        DG16((uint16_t)(si + 0x20)) =
            (int16_t)(DG16((uint16_t)(di + 0x20)) - 0x13);

        DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
        DG32((uint16_t)(si + 0x16)) =
            (int32_t)long_shift_left(
                (uint32_t)DG32((uint16_t)(si + 0x16)), 9);

        DG32((uint16_t)(si + 0x1a)) = DG16((uint16_t)(si + 0x20));
        DG32((uint16_t)(si + 0x1a)) =
            (int32_t)long_shift_left(
                (uint32_t)DG32((uint16_t)(si + 0x1a)), 9);

        place_object_for_draw(si);
    }

    mark_part_shapes(di, 3);
    DGU16((uint16_t)(di + 8)) |= 0x2000;
}

/*
 * 172c:3635, image 0x1a8f5 - kind 36's step. The kicker.
 *
 * It forgets whatever it was touching - +0x84 to zero - and starts itself once
 * its counter passes 0x14. Then it runs its frames, wrapping 0x0a back to 7 so
 * the last four loop; form 6 plays sound 0x0f, and from form 7 on it is
 * *lifting*, taking 0x400 off its own downward velocity every step.
 *
 * From form 7 it also reaches out, the same way the paddle wheel does: a point
 * match first, switching on everything it finds, and then in the odd frames a
 * box match for a kind 4 to switch on and a cat to wake.
 */
uint16_t part_step_3635(uint16_t part)
{
    uint16_t si = part;
    uint16_t di;

    DGU16((uint16_t)(si + 0x84)) = 0;

    if (DGU16((uint16_t)(si + 0x12)) == 0
        && DG16((uint16_t)(si + 0x9c)) > 0x14)
        DGU16((uint16_t)(si + 0x12)) = 1;

    if (DGU16((uint16_t)(si + 0x12)) == 0)
        return 0;

    DGU16((uint16_t)(si + 0x0c))++;
    if (DGU16((uint16_t)(si + 0x0c)) == 0x0a)
        DGU16((uint16_t)(si + 0x0c)) = 7;

    if (DGU16((uint16_t)(si + 0x0c)) == 6)
        play_sound(0x0f);

    if (DG16((uint16_t)(si + 0x0c)) >= 7) {
        DG16((uint16_t)(si + 0x38)) -= 0x400;
        clamp_record_pair(si);
    }

    place_object_for_draw(si);

    if (DG16((uint16_t)(si + 0x0c)) < 7)
        return 0;

    link_objects_at_point(si, -4, 0x12, 0x30, 0x51);

    for (di = DGU16((uint16_t)(si + 0x78)); di != 0;
         di = DGU16((uint16_t)(di + 0x78))) {

        if (DGU16((uint16_t)(di + 0x12)) == 0)
            DGU16((uint16_t)(di + 0x12)) = 1;
    }

    if (!(DGU16((uint16_t)(si + 0x0c)) & 1))
        return 0;

    link_objects_in_range(si, 0x1000, -4, 0x12, 0x30, 0x51);

    for (di = DGU16((uint16_t)(si + 0x78)); di != 0;
         di = DGU16((uint16_t)(di + 0x78))) {

        if (DGU16((uint16_t)(di + 4)) == 4) {
            DGU16((uint16_t)(di + 0x12)) = 1;
            continue;
        }

        if (DGU16((uint16_t)(di + 4)) != 0x0c)
            continue;
        if (DGU16((uint16_t)(di + 0x0c)) != 0)
            continue;

        DGU16((uint16_t)(di + 0x0c)) = 1;
        DGU16((uint16_t)(di + 0x96)) = 0;
        place_object_for_draw(di);
        play_sound(7);
    }

    return 0;
}
