/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The pumpkin**: its setup.
 *
 * The thirty-fourth module of the original's **code segment 172c**, image
 * 0x1a8b4..0x1a8f5 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3422..0x3432
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3422..0x3432. Outline points, 8 pairs.
 */
struct point8 g_pumpkin_outline[8] = {
    { 0, 15 }, { 9, 9 }, { 29, 9 }, { 38, 18 },
    { 38, 22 }, { 27, 32 }, { 11, 32 }, { 0, 22 },
};

/* 172c:35f4, image 0x1a8b4 - a setup: eight points from DGROUP 0x3422. */
void part_setup_pumpkin(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_pumpkin_outline;
    for (i = 0, si = part->points; i < 8; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}
