/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Kind 53's handlers**, new in 1.11 - by what they do, a pin: a balloon
 * that meets it at one of three angles pops. The name waits for its icon.
 *
 * In 1.11, image 0x1e2ce.. in the part kinds' code segment. **Both ends are
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

/*
 * DGROUP 0x3148..0x314e: the three points `part_setup_kind_53` copies - a spike.
 */
struct point8 g_kind_53_points[3] = {
    { 0, 0 }, { 12, 0 }, { 6, 16 },
};

/*
 * 0x1e2ce
 *
 * A balloon that met it at 0x5e00, 0x8000 or 0x9c90 - on the point - is
 * told to pop. Anything else bounces off as off any other part.
 */
uint16_t part_hit_kind_53(register struct part *part)
{
    int16_t angle;                      /* dx */

    if (part->kind == KIND_BALLOON) {
        angle = part->contact_angle;
        if (angle == 0x5e00 || angle == (int16_t)0x8000 || angle == (int16_t)0x9c90)
            part->direction = 1;
    }
    return 1;
}

/* 0x1e2fc - a setup: three points from DGROUP 0x3148. */
void part_setup_kind_53(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_kind_53_points;
    for (i = 0, si = part->points; i < 3; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}
