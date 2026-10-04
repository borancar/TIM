/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The pumpkin**: its setup.
 *
 * The thirty-fourth module of the original's **code segment 172c**, image
 * 0x1a8b4..0x1a8f5 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x2fc4..0x2fd4
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x2fc4..0x2fd4. Connection points, 8 pairs.
 */
struct point8 g_pumpkin_points[8] = {
    { 0, 15 }, { 9, 9 }, { 29, 9 }, { 38, 18 },
    { 38, 22 }, { 27, 32 }, { 11, 32 }, { 0, 22 },
};

/* 190f:35bf, image 0x1c6af - a setup: eight points from DGROUP 0x3422. */
void part_setup_pumpkin(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_pumpkin_points;
    for (i = 0, si = part->points; i < 8; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}
