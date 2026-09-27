/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Detecting a PCjr, and clipping a polygon's outline to the clip box.**
 *
 * The original's **code segment 1c25**, image 0x20be0..0x21088, split out
 * of engine.c on 2026-09-27. Both routines are hand-written - no frames,
 * register answers - so **no C compiler judges this file**; it is the host's
 * transcription, and the byte-exact source is TASM's to make (not written
 * yet). Neither has data of its own.
 *
 * **Possibly two modules.** Its end is proven: `install_keyboard` calls
 * `detect_pcjr` through TLINK's `nop / push cs / call`, so the keyboard
 * driver after it is another module. Its start is the first byte after
 * `blit_scaled_b`, a C routine. Nothing calls between the two routines here,
 * so nothing says whether they shared a file.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x20be0
 *
 * Ask whether this is a PCjr, and remember the answer at DGROUP 0x38ac.
 *
 * The test is the ROM: the model byte at F000:FFFE being 0xff and the byte at
 * F000:C000 being 0x21. Answers the flag, sign-extended - and it is only ever
 * **set**, never cleared, so asking twice cannot unset it.
 *
 * Both addresses are ordinary memory as far as the port is concerned: the
 * verifier seeds all of it, ROM included.
 */
int16_t detect_pcjr(void)
{
    if (*MK_FP(0xf000, 0xfffe) == 0xff
        && *MK_FP(0xf000, 0xc000) == 0x21)
        VMDS.is_pcjr = 1;

    return (int16_t)(int8_t)VMDS.is_pcjr;
}

/*
 * 172c:39b7, image 0x20c07
 *
 * Clip the polygon against the window, in two passes: left and right into the
 * working arrays at 0x398c and dg_near(dgroup, VMDS.work_y), then top and bottom back into 0x393c and
 * 0x3964. Sutherland and Hodgman's, and the count at 0x3a2c is rewritten after
 * each pass.
 *
 * Each pass walks the edges with an outcode for the previous point and one for
 * this one, and there are four cases: both inside emits this point, both
 * outside on the *same* side emits nothing, and the two crossing cases emit the
 * intersection - and, when this point is the one inside, the point after it.
 * An edge that leaves through one side and comes back through the other emits
 * both intersections and no vertex, which is how a polygon wider than the
 * window keeps its shape.
 *
 * The intersection is `y0 + (y1 - y0) * (edge - x0) / (x1 - x0)`, computed with
 * `imul` and `idiv` so the product is 32 bits before the divide - the
 * coordinates are large enough that a 16-bit product would wrap.
 *
 * A polygon left with one point or none is not clipped a second time: the first
 * pass's answer is copied back and that is that.
 */
void clip_polygon(void)
{
    int16_t si, di, bx;
    uint8_t cl, ch;
    int16_t n;

    di = 0;
    n = (int16_t)VMDS.palettes.clip_count;
    if (n <= 1)
        return;

    bx = (int16_t)((n - 1) * 2);

    cl = 0;
    if (VMDS.poly_x[bx >> 1] < VMDS.clip_left)
        cl |= 1;
    if (VMDS.poly_x[bx >> 1] > VMDS.clip_right)
        cl |= 2;

    for (si = 0; ; ) {
        ch = 0;
        if (VMDS.poly_x[si >> 1] < VMDS.clip_left)
            ch |= 1;
        if (VMDS.poly_x[si >> 1] > VMDS.clip_right)
            ch |= 2;

        if ((cl | ch) == 0) {
            VMDS.work_x[di >> 1] = VMDS.poly_x[si >> 1];
            VMDS.work_y[di >> 1] = VMDS.poly_y[si >> 1];
            di += 2;
        } else if ((cl & ch) != 0) {
            /* Both outside the same edge: nothing survives. */
        } else if (cl == 0) {
            /* Leaving: the crossing only. */
            int16_t edge = (ch & 1) ? VMDS.clip_left
                         : (ch & 2) ? VMDS.clip_right : 0;

            if (ch & 3) {
                VMDS.work_x[di >> 1] = edge;
                VMDS.work_y[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.poly_y[bx >> 1]
                              - VMDS.poly_y[si >> 1])
                    * (int32_t)(int16_t)(edge - VMDS.poly_x[si >> 1])
                    / (int32_t)(int16_t)(VMDS.poly_x[bx >> 1]
                                         - VMDS.poly_x[si >> 1])
                    + VMDS.poly_y[si >> 1]);
                di += 2;
            }
        } else if (ch == 0) {
            /* Arriving: the crossing, and then the point itself. */
            int16_t edge = (cl & 1) ? VMDS.clip_left
                         : (cl & 2) ? VMDS.clip_right : 0;

            if (cl & 3) {
                VMDS.work_x[di >> 1] = edge;
                VMDS.work_y[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.poly_y[si >> 1]
                              - VMDS.poly_y[bx >> 1])
                    * (int32_t)(int16_t)(edge - VMDS.poly_x[bx >> 1])
                    / (int32_t)(int16_t)(VMDS.poly_x[si >> 1]
                                         - VMDS.poly_x[bx >> 1])
                    + VMDS.poly_y[bx >> 1]);
                di += 2;
            }

            VMDS.work_x[di >> 1] = VMDS.poly_x[si >> 1];
            VMDS.work_y[di >> 1] = VMDS.poly_y[si >> 1];
            di += 2;
        } else {
            /* Out one side and in the other: both crossings, no vertex. */
            int16_t e1 = (cl & 1) ? VMDS.clip_left
                       : (cl & 2) ? VMDS.clip_right : 0;
            int16_t e2 = (ch & 1) ? VMDS.clip_left
                       : (ch & 2) ? VMDS.clip_right : 0;

            if (cl & 3) {
                VMDS.work_x[di >> 1] = e1;
                VMDS.work_y[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.poly_y[si >> 1]
                              - VMDS.poly_y[bx >> 1])
                    * (int32_t)(int16_t)(e1 - VMDS.poly_x[bx >> 1])
                    / (int32_t)(int16_t)(VMDS.poly_x[si >> 1]
                                         - VMDS.poly_x[bx >> 1])
                    + VMDS.poly_y[bx >> 1]);
                di += 2;
            }

            if (ch & 3) {
                VMDS.work_x[di >> 1] = e2;
                VMDS.work_y[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.poly_y[bx >> 1]
                              - VMDS.poly_y[si >> 1])
                    * (int32_t)(int16_t)(e2 - VMDS.poly_x[si >> 1])
                    / (int32_t)(int16_t)(VMDS.poly_x[bx >> 1]
                                         - VMDS.poly_x[si >> 1])
                    + VMDS.poly_y[si >> 1]);
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)VMDS.palettes.clip_count)
            break;
        si = (int16_t)(si * 2);
    }

    n = (int16_t)((uint16_t)di >> 1);
    VMDS.palettes.clip_count = (uint16_t)n;

    if (n <= 1) {
        int16_t i;

        for (i = 0; i < n; i++) {
            VMDS.poly_x[i] = ((uint16_t)VMDS.work_x[i]);
            VMDS.poly_y[i] = ((uint16_t)VMDS.work_y[i]);
        }
        return;
    }

    bx = (int16_t)((n - 1) * 2);
    di = 0;

    cl = 0;
    if (VMDS.work_y[bx >> 1] > VMDS.clip_bottom)
        cl |= 4;
    if (VMDS.work_y[bx >> 1] < VMDS.clip_top)
        cl |= 8;

    for (si = 0; ; ) {
        ch = 0;
        if (VMDS.work_y[si >> 1] > VMDS.clip_bottom)
            ch |= 4;
        if (VMDS.work_y[si >> 1] < VMDS.clip_top)
            ch |= 8;

        if ((cl | ch) == 0) {
            VMDS.poly_x[di >> 1] = VMDS.work_x[si >> 1];
            VMDS.poly_y[di >> 1] = VMDS.work_y[si >> 1];
            di += 2;
        } else if ((cl & ch) != 0) {
            /* nothing */
        } else if (cl == 0) {
            int16_t edge = (ch & 4) ? VMDS.clip_bottom
                         : (ch & 8) ? VMDS.clip_top : 0;

            if (ch & 12) {
                VMDS.poly_y[di >> 1] = edge;
                VMDS.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.work_x[bx >> 1]
                              - VMDS.work_x[si >> 1])
                    * (int32_t)(int16_t)(edge - VMDS.work_y[si >> 1])
                    / (int32_t)(int16_t)(VMDS.work_y[bx >> 1]
                                         - VMDS.work_y[si >> 1])
                    + VMDS.work_x[si >> 1]);
                di += 2;
            }
        } else if (ch == 0) {
            int16_t edge = (cl & 4) ? VMDS.clip_bottom
                         : (cl & 8) ? VMDS.clip_top : 0;

            if (cl & 12) {
                VMDS.poly_y[di >> 1] = edge;
                VMDS.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.work_x[si >> 1]
                              - VMDS.work_x[bx >> 1])
                    * (int32_t)(int16_t)(edge - VMDS.work_y[bx >> 1])
                    / (int32_t)(int16_t)(VMDS.work_y[si >> 1]
                                         - VMDS.work_y[bx >> 1])
                    + VMDS.work_x[bx >> 1]);
                di += 2;
            }

            VMDS.poly_x[di >> 1] = VMDS.work_x[si >> 1];
            VMDS.poly_y[di >> 1] = VMDS.work_y[si >> 1];
            di += 2;
        } else {
            int16_t e1 = (cl & 4) ? VMDS.clip_bottom
                       : (cl & 8) ? VMDS.clip_top : 0;
            int16_t e2 = (ch & 4) ? VMDS.clip_bottom
                       : (ch & 8) ? VMDS.clip_top : 0;

            if (cl & 12) {
                VMDS.poly_y[di >> 1] = e1;
                VMDS.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.work_x[si >> 1]
                              - VMDS.work_x[bx >> 1])
                    * (int32_t)(int16_t)(e1 - VMDS.work_y[bx >> 1])
                    / (int32_t)(int16_t)(VMDS.work_y[si >> 1]
                                         - VMDS.work_y[bx >> 1])
                    + VMDS.work_x[bx >> 1]);
                di += 2;
            }

            if (ch & 12) {
                VMDS.poly_y[di >> 1] = e2;
                VMDS.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.work_x[bx >> 1]
                              - VMDS.work_x[si >> 1])
                    * (int32_t)(int16_t)(e2 - VMDS.work_y[si >> 1])
                    / (int32_t)(int16_t)(VMDS.work_y[bx >> 1]
                                         - VMDS.work_y[si >> 1])
                    + VMDS.work_x[si >> 1]);
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)VMDS.palettes.clip_count)
            break;
        si = (int16_t)(si * 2);
    }

    VMDS.palettes.clip_count = (uint16_t)((uint16_t)di >> 1);
}
