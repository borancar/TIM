/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The Christmas tree**: its setup.
 *
 * The eleventh module of the original's **code segment 172c**, image
 * 0x18335..0x18376 - one module for each kind of part; part_ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3266..0x3274
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * DGROUP 0x3266..0x3274. Connection points, 7 pairs.
 */
struct point8 CHRISTMAS_TREE_POINTS_3266[7] DGROUP_AT(0x3266) = {
    { 0x00, 0x35 }, { 0x14, 0x00 }, { 0x27, 0x37 }, { 0x19, 0x3d },
    { 0x19, 0x48 }, { 0x10, 0x48 }, { 0x10, 0x3d },
};

/*
 * 172c:1075, image 0x18335 - a setup.
 *
 * Seven points from the one table at DGROUP 0x3266, with nothing to choose:
 * this part has a single shape.
 */
void part_setup_christmas_tree(struct part *part)
{
    struct part_point *dst;
    const struct point8 *src;
    int16_t i;

    src = CHRISTMAS_TREE_POINTS_3266;
    for (i = 0, dst = POINTS(part->points_ptr); i < 7; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}
