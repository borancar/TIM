/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Kind 64's handlers**, new in 1.11 - by what they do, the pinball
 * bumper on the title screen: whatever strikes it is sent straight back out
 * from its centre at one speed, and it lights for a frame with a sound. The
 * name waits for its icon. Its record in `g_part_kinds` names these.
 *
 * In 1.11, image 0x1dead.. in the part kinds' code segment. **Both ends are
 * ours**: each kind in 1.00 is a module of its own, and so is this one until
 * the far calls between its routines and its neighbours' say otherwise.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x310c..0x311c: the eight connection points `part_setup_kind_64` copies -
 * an octagon inside its 0x28-square box.
 */
struct point8 g_kind_64_points[8] = {
    { 0, 13 }, { 13, 0 }, { 27, 0 }, { 37, 13 },
    { 37, 25 }, { 25, 37 }, { 13, 37 }, { 0, 27 },
};

/*
 * 0x1dead
 *
 * **Something struck it.** The part in contact is marked (`traits2` bit
 * 0x80, which only this sets) and told to light (`direction` 1), and this
 * part - the one moving - is sent off at 0x1c00 along the line from the
 * other's centre to its own. Its speed is measured first - the larger
 * velocity component plus three-eighths of the smaller, at least 0x1c00 -
 * and never used: the send-off speed is the constant.
 */
uint16_t part_hit_kind_64(register struct part *part)
{
    int16_t angle;                      /* [bp-2] */
    int16_t speed;                      /* [bp-4], written and never read */
    struct part *other;

    other = part->contact;
    other->traits2 |= 0x80;
    speed = abs(part->vel_y) > abs(part->vel_x)
            ? (abs(part->vel_x) >> 2) + (abs(part->vel_x) >> 3) + abs(part->vel_y)
            : (abs(part->vel_y) >> 2) + (abs(part->vel_y) >> 3) + abs(part->vel_x);
    speed = speed > 0x1c00 ? speed : 0x1c00;
#ifndef __TURBOC__
    (void)speed;
#endif
    angle = angle_between_centres(other, part);
    set_vector_from_angle(part, angle, 0x1c00);
    other->direction = 1;
    return 0;
}

/* 0x1df70 - a setup: eight points from DGROUP 0x310c. */
void part_setup_kind_64(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_kind_64_points;
    for (i = 0, si = part->points; i < 8; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}

/*
 * 0x1dfb1
 *
 * Light for one frame after a strike: `direction` is the request `hit`
 * leaves, and it becomes form 1 and sound 0x1a; the frame after, form 0.
 */
void part_step_kind_64(register struct part *part)
{
    if (part->direction != 0) {
        play_sound(0x1a);
        part->form = 1;
        part->direction = 0;
    } else {
        if (part->form == 0)
            return;
        part->form = 0;
    }
    place_object_for_draw(part);
}
