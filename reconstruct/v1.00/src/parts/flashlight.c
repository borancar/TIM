/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The flashlight**: its hit, setup, step and flip.
 *
 * The twentieth module of the original's **code segment 172c**, image
 * 0x18fc7..0x190a0 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x32fc..0x3314
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x32fc..0x3314. Connection points, 6 pairs a row, indexed [flipped].
 */
struct point8 g_flashlight_points[2][6] = {
    {
        { 0x00, 0x04 }, { 0x17, 0x04 }, { 0x1f, 0x00 }, { 0x1f, 0x10 },
        { 0x17, 0x0c }, { 0x00, 0x0c },
    },
    {
        { 0x08, 0x04 }, { 0x1f, 0x04 }, { 0x1f, 0x0c }, { 0x08, 0x0c },
        { 0x00, 0x10 }, { 0x00, 0x00 },
    },
};

/*
 * 172c:1d07, image 0x18fc7 - kind 25's hit test.
 *
 * The hook is the *linked* thing's, run on whatever ran into it: `di` is the
 * kind 25 part at the hit object's +0x84 and `si` the object that arrived.
 *
 * Unless the arriving object is already spoken for - a non-zero +0x88 - the
 * kind 25 part is set going, which its step at 172c:1d78 then acts on. Either
 * way the answer is 1: the hit counts.
 */
uint16_t part_hit_flashlight(struct part *part)
{
    struct part *other = part->contact;

    if (((uint16_t)part->contact_angle) == 0)
        other->direction = 1;

    return 1;
}

/*
 * 172c:1d28, image 0x18fe8 - a setup.
 *
 * Six points, from DGROUP 0x3308 or 0x32fc as bit 4 of +8 says. Two bytes a
 * point at the source and four at the destination, as usual.
 */
void part_setup_flashlight(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->state & STATE_FLIP_HORIZONTAL)
        si = g_flashlight_points[1];
    else
        si = g_flashlight_points[0];

    for (i = 0, di = part->points; i < 6; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:1d78, image 0x19038 - kind 25's step.
 *
 * One move and then nothing: turned on in form 0 it steps to form 1, runs its
 * own setup again because the shape has changed, and plays sound 0x11. In any
 * other form it does nothing at all.
 */
void part_step_flashlight(struct part *part)
{
    if (part->direction != 0 && part->form == 0) {
        part->form++;
        part_setup_flashlight(part);
        place_object_for_draw(part);
        play_sound(0x11);
    }
}

/*
 * 172c:1da8, image 0x19068 - kind 25's flip, the three-mark shape.
 */
void part_flip_flashlight(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;

    part_setup_flashlight(part);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
