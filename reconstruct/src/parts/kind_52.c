/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Kind 52's handlers**, new in 1.11 - a part that only has a shape: its
 * setup is all it has of its own. Its icon will say what it is.
 *
 * In 1.11, image 0x1dfe9.. in the part kinds' code segment. **Both ends are
 * ours**: each kind in 1.00 is a module of its own, and so is this one until
 * the far calls between its routines and its neighbours' say otherwise.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* DGROUP 0x311c: the six points `part_setup_kind_52` copies. */
struct point8 g_kind_52_points_311c[6] = {
    { 0x00, 0x04 }, { 0x07, 0x00 }, { 0x0f, 0x00 }, { 0x1b, 0x07 },
    { 0x1b, 0x11 }, { 0x00, 0x0f },
};

/* 0x1dfe9 - a setup: six points from DGROUP 0x311c. */
void part_setup_kind_52(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_kind_52_points_311c;
    for (i = 0, si = part->points; i < 6; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}
