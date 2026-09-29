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
 * JUDGE: built-with -mm
 * JUDGE: data 0x3422..0x3432
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3422..0x3432. Connection points, 8 pairs.
 */
struct point8 PUMPKIN_POINTS_3422[8] DGROUP_AT(0x3422) = {
    { 0x00, 0x0f }, { 0x09, 0x09 }, { 0x1d, 0x09 }, { 0x26, 0x12 },
    { 0x26, 0x16 }, { 0x1b, 0x20 }, { 0x0b, 0x20 }, { 0x00, 0x16 },
};

/* 172c:35f4, image 0x1a8b4 - a setup: eight points from DGROUP 0x3422. */
void part_setup_pumpkin(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = PUMPKIN_POINTS_3422;
    for (i = 0, si = part->points; i < 8; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}
