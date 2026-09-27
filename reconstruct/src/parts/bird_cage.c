/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The bird cage**: its setup and drive.
 *
 * The tenth module of the original's **code segment 172c**, image
 * 0x18230..0x18335 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 172c:0f70, image 0x18230 - a setup.
 *
 * Twelve points, the longest outline of the fourteen: a head on a pair of
 * legs, 45 wide and 63 tall.
 */
void part_setup_bird_cage(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 0;
    si->y = 24;
    si++;
    si->x = 19;
    si->y = 0;
    si++;
    si->x = 25;
    si->y = 0;
    si++;
    si->x = 45;
    si->y = 24;
    si++;
    si->x = 45;
    si->y = 63;
    si++;
    si->x = 43;
    si->y = 63;
    si++;
    si->x = 43;
    si->y = 26;
    si++;
    si->x = 34;
    si->y = 16;
    si++;
    si->x = 11;
    si->y = 16;
    si++;
    si->x = 2;
    si->y = 26;
    si++;
    si->x = 2;
    si->y = 63;
    si++;
    si->x = 0;
    si->y = 63;

    part_finish_angles(part);
}

/*
 * 172c:0ffc, image 0x182bc - kind 11's drive.
 *
 * `part_drive_02cd` with a tail. Mode 1 steps +0x0e of what +0x66 points at;
 * otherwise the driven part's 32-bit value at +0x3c - doubled unless the asker
 * is kind 3 - is compared against the limit in the sixth and seventh
 * arguments, and past it the answer is 1.
 *
 * What is new is what happens when it is **not** past: in mode 2 the part is
 * lifted 0x14, its +0x12 stepped, and it is redrawn. So this drive moves the
 * thing it was asked about.
 */
uint16_t part_drive_0ffc(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    struct belt *belt = BELT_PTR(p2->belt_ptr[0]);   /* [bp-2] */

    if (p4 == 1) {
        belt->v[0]++;
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
    if (p4 == 2) {
        p2->pos[0].y -= 0x14;
        p2->direction++;
        place_object_for_draw(p2);
    }

    return 0;
}
