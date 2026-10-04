/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Kind 54's handlers**, new in 1.11 - by what they do, a small walker:
 * it steps along a stride table, turns round at what it walks into, falls
 * and lands (squashed, form 0x1e, if it lands hard), and is what the
 * alligator and the vacuum eat and kind 58 catches. The name waits for its
 * icon.
 *
 * In 1.11, image 0x1e02a.. in the part kinds' code segment. **Both ends are
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
 * DGROUP 0x3128..0x313c: its five points, on its feet and squashed, indexed [form == 0x1e].
 */
struct point8 g_kind_54_outline[2][5] = {
    {
        { 0, 0 }, { 12, 0 }, { 12, 19 }, { 6, 23 },
        { 0, 19 },
    },
    {
        { 0, 16 }, { 12, 16 }, { 12, 19 }, { 6, 23 },
        { 0, 19 },
    },
};

/* DGROUP 0x313c: how far each walking form moves it. */
int16_t g_kind_54_strides[6] = { 1, 2, 2, 1, 2, 2 };

/*
 * 0x1e02a
 *
 * **A collision**, with the walker on either side of it. Falling (forms
 * 6..8) onto anything but a trampoline, from above, it lands - form 9, and
 * `spin` says it came down hard (over 0x1c00) - and landing flat it stops,
 * sideways too unless it is on a running conveyor. Walking, it turns round
 * at what it meets ahead of it.
 */
uint16_t part_hit_kind_54(register struct part *part)
{
    int16_t angle;                      /* [bp-2] */
    struct part *other;                 /* [bp-4] */
    register struct part *me;

    angle = part->contact_angle;
    if (part->kind == 0x36) {
        me = part;
        other = part->contact;
    } else {
        me = part->contact;
        other = part;
    }

    if (me->form >= 6 && me->form <= 8) {
        if (other->kind != KIND_TRAMPOLINE && ((angle + 0xc000) & 0x8000)) {
            me->form = 9;
            if (me->vel_y > 0x1c00)
                me->spin = 1;
            else
                me->spin = 0;
            if (angle == 0) {
                me->vel_y = 0;
                if (other->kind != KIND_CONVEYOR || other->direction == 0)
                    me->vel_x = 0;
            }
        }
        place_object_for_draw(me);
    } else if (me->form != 0x1e) {
        if (me->state & STATE_FLIP_HORIZONTAL) {
            if (angle < -0x2000) {
                me->state ^= STATE_FLIP_HORIZONTAL;
                place_object_for_draw(me);
            }
        } else if (angle > 0x2000) {
            me->state ^= STATE_FLIP_HORIZONTAL;
            place_object_for_draw(me);
        }
    }
    return 1;
}

/* 0x1e0e4 - a setup: five points, squashed (form 0x1e) or not. */
void part_setup_kind_54(struct part *part)
{
    register const struct point8 *src;
    register struct part_point *dst;
    int16_t i;

    if (part->form == 0x1e)
        src = g_kind_54_outline[1];
    else
        src = g_kind_54_outline[0];
    for (i = 0, dst = part->points; i < 5; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 0x1e133
 *
 * **Walking, falling, landing.** On its feet (forms 0..5) it starts falling
 * if it is moving and faster than 0x600 up or down (sound 0x16); otherwise
 * it steps - the stride for the form, the way it faces - unless it stands
 * on a running conveyor, and sets its fixed position from the new one.
 * Falling it cycles 6..8; landed (9 on) it plays out to 0x16, and then gets
 * up (form 0) unless it came down hard, in which case it goes on to 0x1e,
 * squashed, and takes that shape.
 */
void part_step_kind_54(register struct part *part)
{
    if (part->form < 6) {
        if (part->pos[0].y != part->pos[2].y && abs(part->vel_y) > 0x600) {
            part->form = 6;
            play_sound(0x16);
        } else {
            part->form++;
            if (part->form == 6)
                part->form = 0;
            /* Standing on nothing, `contact` is null, and the original
               reads DGROUP:0000 - Borland's copyright - which is never a
               conveyor, so it walks. */
            if (NEAR_ZERO(part->contact)->direction == 0
                || NEAR_ZERO(part->contact)->kind != KIND_CONVEYOR) {
                if (part->state & STATE_FLIP_HORIZONTAL)
                    part->pos[0].x -= g_kind_54_strides[part->form];
                else
                    part->pos[0].x += g_kind_54_strides[part->form];
                part->fx = part->pos[0].x;
                part->fx <<= 9;
                part->vel_x = 0;
            }
        }
        place_object_for_draw(part);
    } else if (part->form < 9) {
        part->form++;
        if (part->form == 9)
            part->form = 6;
        place_object_for_draw(part);
    } else if (part->form != 0x1e) {
        part->form++;
        if (part->form == 0x17 && part->spin == 0)
            part->form = 0;
        place_object_for_draw(part);
        if (part->form == 0x1e)
            part_setup_kind_54(part);
    }
}

/* 0x1e21b - turn it round, and redraw. */
void part_flip_kind_54(register struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    part_setup_kind_54(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
