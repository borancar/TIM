/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The platforms**: the setup and settle the brick and wooden platforms and the pipe share.
 *
 * The forty-second module of the original's **code segment 172c**, image
 * 0x1bb6b..0x1bc2f - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 172c:48ab, image 0x1bb6b - a setup.
 *
 * The bounding rectangle **inset by one**: (0,0), (W-1,0), (W-1,H-1),
 * (0,H-1). The subtraction is `add al, 0xff` in the original, which is the
 * same byte and is transcribed as the -1 it is.
 */
void part_setup_platform(struct part *part)
{
    struct part_point *si;

    si = part->points;
    si->x = si->y = 0;
    si++;
    si->x = part->size[0].width - 1;
    si->y = 0;
    si++;
    si->x = part->size[0].width - 1;
    si->y = part->size[0].height - 1;
    si++;
    si->x = 0;
    si->y = part->size[0].height - 1;

    part_finish_angles(part);
}

/*
 * 172c:48f7, image 0x1bbb7 - the settle shared by kinds 1, 46 and 48.
 *
 * **Which edge was dragged decides which way it is squared off.** DGROUP
 * 0x4e69 is the handle being dragged; 0x8003 is taken off it and the four
 * values that leaves index a jump table at cs:0x4967, whose four entries are
 * only two: 0 and 1 pin the height at 0x10, 2 and 3 pin the width. Anything
 * else falls through untouched.
 *
 * Then the dragged size becomes the real size, and three connection points -
 * +0x82 plus 4, plus 8 and plus 0x0c - take the width and height **less one**,
 * because a point sits inside the edge rather than on it.
 */
void part_settle_platform(struct part *part)
{
    /* Three cursors four bytes apart - points 1, 2 and 3 of the part's own
       table. */
    struct part_point *p1;              /* dx */
    struct part_point *p2;              /* di */
    struct part_point *p3;              /* [bp-2] */

    switch (g_tool) {
    case 0x8003:
    case 0x8004:
        part->set_size.height = 0x10;
        break;
    case 0x8005:
    case 0x8006:
        part->set_size.width = 0x10;
        break;
    }

    part->size[0].width = part->set_size.width;
    part->size[0].height = part->set_size.height;

    p1 = part->points + 1;
    p2 = p1 + 1;
    p3 = p2 + 1;

    p1->x = p2->x = part->size[0].width - 1;
    p2->y = p3->y = part->size[0].height - 1;
}
