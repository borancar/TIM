/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The balls**: the setups the bowling ball and the basketball, the cannon ball, and the baseball and the tennis ball share.
 *
 * The first module of the original's **code segment 172c**, image
 * 0x172c0..0x173ed - one module for each kind of part; part_ball.c says how
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
 * 172c:0001, image 0x172c1 - a setup.
 *
 * Eight points round a 32 by 32 part, written straight out.
 */
void part_setup_0001(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 8;
    si->y = 0;
    si++;
    si->x = 23;
    si->y = 0;
    si++;
    si->x = 31;
    si->y = 8;
    si++;
    si->x = 31;
    si->y = 23;
    si++;
    si->x = 23;
    si->y = 31;
    si++;
    si->x = 8;
    si->y = 31;
    si++;
    si->x = 0;
    si->y = 23;
    si++;
    si->x = 0;
    si->y = 8;

    part_finish_angles(part);
}

/*
 * 172c:0065, image 0x17325 - a setup.
 *
 * The same eight-cornered shape on a 23 by 23 part.
 */
void part_setup_cannon_ball(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 7;
    si->y = 0;
    si++;
    si->x = 15;
    si->y = 0;
    si++;
    si->x = 22;
    si->y = 8;
    si++;
    si->x = 22;
    si->y = 15;
    si++;
    si->x = 14;
    si->y = 22;
    si++;
    si->x = 8;
    si->y = 22;
    si++;
    si->x = 0;
    si->y = 15;
    si++;
    si->x = 0;
    si->y = 8;

    part_finish_angles(part);
}

/*
 * 172c:00c9, image 0x17389 - a setup.
 *
 * The same again, smaller still - 15 by 15.
 */
void part_setup_00c9(struct part *part)
{
    struct part_point *si = POINTS(part->points_ptr);

    si->x = 3;
    si->y = 0;
    si++;
    si->x = 11;
    si->y = 0;
    si++;
    si->x = 14;
    si->y = 4;
    si++;
    si->x = 14;
    si->y = 10;
    si++;
    si->x = 11;
    si->y = 14;
    si++;
    si->x = 3;
    si->y = 14;
    si++;
    si->x = 0;
    si->y = 10;
    si++;
    si->x = 0;
    si->y = 4;

    part_finish_angles(part);
}
