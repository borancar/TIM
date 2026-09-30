/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The heart balloon**: its setup and drive.
 *
 * The twenty-fifth module of the original's **code segment 172c**, image
 * 0x19942..0x199e8 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x2ed8..0x2ee6
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3336..0x3344. Connection points, 7 pairs.
 */
struct point8 g_heart_balloon_points_3336[7] = {
    { 0x00, 0x08 }, { 0x06, 0x00 }, { 0x1e, 0x00 }, { 0x24, 0x07 },
    { 0x24, 0x10 }, { 0x11, 0x23 }, { 0x00, 0x10 },
};

/* 190f:263d, image 0x1b72d - a setup: seven points from DGROUP 0x3336. */
void part_setup_heart_balloon(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_heart_balloon_points_3336;
    for (i = 0, si = part->points; i < 7; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}

/*
 * 190f:267e, image 0x1b76e - kind 33's drive, and it is `part_drive_balloon`
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
            return 1;
    } else if (p2->momentum + p2->momentum > p6)
        return 1;
    return 0;
}
