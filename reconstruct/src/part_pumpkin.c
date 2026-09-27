/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The pumpkin**: its setup.
 *
 * The thirty-fourth module of the original's **code segment 172c**, image
 * 0x1a8b4..0x1a8f5 - one module for each kind of part; part_ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/* 172c:35f4, image 0x1a8b4 - a setup: eight points from DGROUP 0x3422. */
void part_setup_pumpkin(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = PARTSHAPES.s_3422;
    for (i = 0, si = POINTS(part->points_ptr); i < 8; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}
