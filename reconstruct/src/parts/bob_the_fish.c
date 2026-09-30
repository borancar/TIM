/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Bob the fish**: its setup, hit and step, and the bowl breaking.
 *
 * The nineteenth module of the original's **code segment 172c**, image
 * 0x18ea9..0x18fc7 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x32dc..0x32fc
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x32dc..0x32fc. Connection points, 8 points.
 */
struct point16 g_bob_the_fish_points_32dc[8] = {
    { 0x0000, 0x0012 }, { 0x000b, 0x0000 }, { 0x0025, 0x0000 },
    { 0x002f, 0x0012 }, { 0x002f, 0x0023 }, { 0x0027, 0x002f },
    { 0x0008, 0x002f }, { 0x0000, 0x0022 },
};

/*
 * 190f:1bce, image 0x1acbe - a setup.
 *
 * Eight points out of a **four-byte-stride** table at DGROUP 0x32dc, x at +0
 * and y at +2 of each row - unlike the two-byte tables the other setups walk,
 * so the index is multiplied rather than the pointer advanced. It also sets
 * +0x80 to 8, which is the count the part carries.
 */
void part_setup_bob_the_fish(struct part *part)
{
    struct part_point *si;
    int16_t i;

    part->point_count = 8;

    for (i = 0, si = part->points; i < 8; i++, si++) {
        si->x = (uint8_t)g_bob_the_fish_points_32dc[i].x;
        si->y = (uint8_t)g_bob_the_fish_points_32dc[i].y;
    }

    part_finish_angles(part);
}

/*
 * 190f:1c1e, image 0x1ad0e - kind 15's hit test.
 *
 * A kind 15 already past form 0x0b is broken and the hit counts - answer 1.
 * One that is not gets broken by the hit and the hit does *not* count, so the
 * thing that broke it carries on through rather than bouncing off.
 */
uint16_t part_hit_bob_the_fish(struct part *part)
{
    struct part *other = part->contact;

    if (((int16_t)other->form) >= 0x0b)
        return 1;

    break_bob_the_fish(other);
    return 0;
}

/*
 * 190f:1c42, image 0x1ad32 - kind 15's step.
 *
 * A two-part animation. Below a count of 0x14 at +0x9c the form runs on every
 * step; past 0x16 it drops back to 0x0e and the count goes up by one, and at
 * 0x0b it wraps to zero - so the first eleven frames play once and then it
 * loops on 0x0e to 0x16 for as long as the count allows.
 */
void part_step_bob_the_fish(struct part *part)
{
    if (part->spin < 0x14)
        part->form++;

    if (((int16_t)part->form) > 0x16) {
        part->form = 0x0e;
        part->spin++;
    } else if (part->form == 0x0b) {
        part->form = 0;
    }

    if (part->form != part->form_prev)
        place_object_for_draw(part);
}

/*
 * 190f:1c81, image 0x1ad71
 *
 * Break bob the fish's bowl: form 0x0b is the broken one, and a part already at
 * 0x0b or past it is left alone. Breaking plays sound 0x0a and replaces the
 * connection points with three of its own - the broken shape has a different
 * outline from the whole one.
 */
void break_bob_the_fish(struct part *part)
{
    /* Three points, and the original keeps a cursor per point rather than
       indexing - `di`, `di + 4`, `di + 8` - and writes them out of order. */
    struct part_point *di;
    struct part_point *p1;              /* [bp-2] */
    struct part_point *p2;              /* [bp-4] */

    if (part->form < 0x0b) {
        part->form = 0x0b;
        place_object_for_draw(part);
        play_sound(0x0a);

        part->point_count = 3;

        di = part->points;
        p1 = di + 1;
        p2 = p1 + 1;

        di->x = 8;
        di->y = p2->y = 47;
        p1->x = 24;
        p1->y = 44;
        p2->x = 39;
    }
}
