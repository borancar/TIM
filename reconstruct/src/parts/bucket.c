/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The bucket**: its hit, setup and drive.
 *
 * The fifth module of the original's **code segment 172c**, image
 * 0x17a23..0x17b27 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 190f:0757, image 0x19847 - kind 17's hit test, and **it answers 0 to refuse
 * the hit**, which almost none of the others do.
 *
 * The refusal is a band: the arriving object must be coming down - +0x38
 * positive - and its centre, +0x22 plus half its width at +0x44, must lie
 * between the struck part's +0x22 plus 4 and that plus 0x1c. Inside the band
 * the answer is 0 and outside it 1, so the part is solid everywhere except
 * across a 0x1c-wide mouth that something falling can drop through.
 */
uint16_t part_hit_bucket(struct part *part)
{
    int16_t lo;                         /* cx */
    int16_t mid;                        /* di */
    int16_t hi;                         /* [bp-2] */
    struct part *other;                 /* [bp-4] */

    other = part->contact;
    lo = other->pos[1].x + 4;
    hi = lo + 0x1c;
    mid = part->pos[1].x + (part->size[0].width >> 1);

    if (part->vel_y > 0 && mid > lo && mid < hi)
        return 0;

    return 1;
}

/*
 * 190f:07a2, image 0x19892 - a setup.
 *
 * Six points, and not a regular shape: the two at y 47 sit below the four
 * that make the body, so this outline has a foot.
 */
void part_setup_bucket(struct part *part)
{
    struct part_point *si = part->points;

    si->x = 0;
    si->y = 19;
    si++;
    si->x = 10;
    si->y = 40;
    si++;
    si->x = 25;
    si->y = 40;
    si++;
    si->x = 36;
    si->y = 20;
    si++;
    si->x = 27;
    si->y = 47;
    si++;
    si->x = 8;
    si->y = 47;

    part_finish_angles(part);
}

/*
 * 190f:07f2, image 0x198e2 - kind 17's drive hook.
 *
 * The same routine as 172c:11d2 below, and not merely alike: the 0x65 bytes at
 * the two addresses are **identical**, so the source had one function and the
 * compiler emitted it twice, once per kind that names it.
 *
 * It is transcribed twice here for the same reason it exists twice there. Each
 * address is its own entry in a kind record and its own thing to prove against
 * the original, and a shared C function would carry one provenance comment for
 * two addresses - so a verifier run naming 172c:0802 would be checking
 * something that, as far as the file is concerned, is at 172c:11d2.
 */
uint16_t part_drive_bucket(struct part *from, struct part *part, uint16_t p3,
                         uint16_t flags, uint16_t p5, int32_t momentum)
{
    struct rope *rope = part->rope[0];   /* [bp-2] */

    if (flags == 1) {
        rope->v[0]++;
        return 0;
    }

    if (from->kind == KIND_SEESAW) {
        if (part->momentum > momentum)
            return 1;
    } else if (part->momentum + part->momentum > momentum)
        return 1;
    return 0;
}
