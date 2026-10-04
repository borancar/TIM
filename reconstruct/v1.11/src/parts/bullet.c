/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The bullet**: the gun's shot: its hit, setup and step.
 *
 * The sixth module of the original's **code segment 172c**, image
 * 0x17b27..0x17c10 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x2dc4..0x2dd4
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x2dc4..0x2dd4. Connection points, 4 pairs a row, indexed [flipped].
 */
struct point8 g_bullet_points[2][4] = {
    {
        { 28, 0 }, { 39, 1 }, { 39, 5 }, { 28, 6 },
    },
    {
        { 0, 0 }, { 11, 1 }, { 11, 5 }, { 0, 6 },
    },
};

/*
 * 190f:0855, image 0x19945 - kind 20's hit test: three kinds get three
 * different answers and everything else is simply a hit.
 *
 * A kind-4 part - a balloon - has its +0x12 set, which is what a balloon does
 * when touched. A kind-0x13 bursts, through `burst_dynamite` and given the
 * **collision record**. A kind-0x15 has one taken off its +0x36, which is a
 * nudge left. The answer is 1 whichever happened.
 */
uint16_t part_hit_bullet(struct part *part)
{
    struct part *di = part->contact;

    if (part->kind == KIND_BALLOON)
        part->direction = 1;
    else if (part->kind == KIND_DYNAMITE)
        burst_dynamite(part);
    else if (part->kind == KIND_ELECTRIC_PLUG)
        di->vel_x--;

    return 1;
}

/*
 * 190f:088d, image 0x1997d - a setup.
 *
 * Four points, from DGROUP 0x322a or 0x3222 as bit 4 of +8 says. The
 * two tables are eight bytes apart, which is those four points.
 */
void part_setup_bullet(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->state & STATE_FLIP_HORIZONTAL)
        si = g_bullet_points[1];
    else
        si = g_bullet_points[0];

    for (i = 0, di = part->points; i < 4; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 190f:08dd, image 0x199cd - kind 20's step.
 *
 * Three forms and then gone. At form 2 it registers its shapes a last time and
 * hides itself with bit 13 of +8; at any form but 0 it simply steps on and
 * redraws.
 *
 * Form 0 is where it decides whether to start at all, and it decides on its
 * **sideways speed**: exactly 0x3000 or exactly 0xd000 - the same speed left
 * and right, since 0xd000 is -0x3000 - and it does nothing. Any other speed
 * starts it: the slot count at +0x80 is cleared, the form goes to 1, it is
 * redrawn and sound 0x0b plays. Two exact comparisons rather than a range, so
 * a speed one away from either starts it.
 */
void part_step_bullet(struct part *part)
{
    if (part->form == 2) {
        mark_part_shapes(part, 3);
        part->state |= STATE_GONE;
    } else if (part->form == 0) {
        if (part->vel_x != 0x3000 && part->vel_x != (int16_t)0xd000) {
            part->point_count = 0;
            part->form = 1;
            place_object_for_draw(part);
            play_sound(0x0b);
        }
    } else {
        part->form++;
        place_object_for_draw(part);
    }
}
