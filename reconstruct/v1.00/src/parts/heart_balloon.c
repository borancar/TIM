/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The heart balloon**: its setup and drive.
 *
 * The twenty-fifth module of the original's **code segment 172c**, image
 * 0x19942..0x199e8 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3336..0x3344
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3336..0x3344. Connection points, 7 pairs.
 */
struct point8 g_heart_balloon_points[7] = {
    { 0, 8 }, { 6, 0 }, { 30, 0 }, { 36, 7 },
    { 36, 16 }, { 17, 35 }, { 0, 16 },
};

/* 172c:2682, image 0x19942 - a setup: seven points from DGROUP 0x3336. */
void part_setup_heart_balloon(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_heart_balloon_points;
    for (i = 0, si = part->points; i < 7; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:26c3, image 0x19983 - kind 33's drive, and it is `part_drive_balloon`
 * again with nothing added: mode 1 steps +0x0e of what +0x66 points at, and
 * otherwise the driven part's 32-bit value at +0x3c - doubled unless the asker
 * is kind 3 - answers 1 when it is past the limit.
 */
uint16_t part_drive_heart_balloon(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    struct rope *rope = p2->rope[0];   /* [bp-2] */

    if (p4 == 1) {
        rope->v[0]++;
        return 0;
    }

    if (p1->kind == KIND_SEESAW) {
        if (p2->momentum > p6)
            goto yes;
        goto no;
    }
    if (p2->momentum + p2->momentum > p6) {
yes:
        return 1;
    }
no:
    return 0;
}
