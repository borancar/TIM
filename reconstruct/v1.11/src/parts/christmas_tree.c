/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The Christmas tree**: its setup.
 *
 * The eleventh module of the original's **code segment 172c**, image
 * 0x18335..0x18376 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x2e08..0x2e16
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x2e08..0x2e16. Outline points, 7 pairs.
 */
struct point8 g_christmas_tree_outline[7] = {
    { 0, 53 }, { 20, 0 }, { 39, 55 }, { 25, 61 },
    { 25, 72 }, { 16, 72 }, { 16, 61 },
};

/*
 * 190f:107c, image 0x1a16c - a setup.
 *
 * Seven points from the one table at DGROUP 0x3266, with nothing to choose:
 * this part has a single shape.
 */
void part_setup_christmas_tree(struct part *part)
{
    struct part_point *dst;
    const struct point8 *src;
    int16_t i;

    src = g_christmas_tree_outline;
    for (i = 0, dst = part->points; i < 7; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}
