/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The monkey**: its hit, setup, step, flip and drive.
 *
 * The twenty-ninth module of the original's **code segment 172c**, image
 * 0x19f43..0x1a1a1 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x33aa..0x33ce
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x33aa..0x33ce. Connection points, 9 pairs a row, indexed [flipped].
 */
struct point8 g_monkey_points[2][9] = {
    {
        { 17, 42 }, { 33, 12 }, { 40, 13 }, { 42, 37 },
        { 66, 60 }, { 66, 68 }, { 56, 78 }, { 23, 67 },
        { 0, 76 },
    },
    {
        { 49, 37 }, { 51, 13 }, { 58, 12 }, { 74, 42 },
        { 91, 76 }, { 68, 67 }, { 35, 78 }, { 25, 68 },
        { 25, 60 },
    },
};

/*
 * 172c:2c83, image 0x19f43 - kind 31's hit test, the third way into
 * `part_step_monkey`'s timer after its own drive and its belt.
 *
 * It only fires when the timer at +0x96 is already at rest, and only on faces
 * 0, 1 and 2 - a strike from behind does nothing. Then the timer is loaded
 * with 0x1c, whatever the part was doing is cleared, and the form is put at
 * the head of one of the step's two animation loops: 5 for a part sitting at
 * 0, and 9 for one anywhere else.
 *
 * The `jmp` to the next instruction at 0x19f86 is the compiler leaving a
 * return path in that nothing needed.
 */
uint16_t part_hit_monkey(struct part *part)
{
    struct part *other = part->contact;
    int16_t  face = ((int16_t)part->contact_edge);

    if (other->kind_state == 0 && face < 3) {
        other->kind_state = 0x1c;
        other->direction = 0;

        if (other->form == 0)
            other->form = 5;
        else
            other->form = 9;
    }

    return 1;
}

/*
 * 172c:2cce, image 0x19f8e - a setup.
 *
 * A part that is a different size each way round: bit 4 of +8 chooses both the
 * grab box's width at +0x6a and the box origin at +0x56 *and* which of the two
 * point tables to walk, 0x33bc or 0x33aa. The height at +0x6b, the other
 * origin byte at +0x57 and the count at +0x58 are the same either way.
 *
 * Nine points, two bytes each at the source and four at the destination.
 */
void part_setup_monkey(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->state & STATE_FLIP_HORIZONTAL) {
        part->attach[0].x = 16;
        part->grab.x = 36;
        src = g_monkey_points[1];
    } else {
        part->attach[0].x = 75;
        part->grab.x = 47;
        src = g_monkey_points[0];
    }

    part->attach[0].y = 45;
    part->grab.y = 60;
    part->grab_size = 9;

    for (i = 0, dst = part->points; i < 9; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:2d40, image 0x1a000 - kind 31's step.
 *
 * Whatever is on the other end of its belt is told what this part is doing -
 * +0x12 copied straight across - unless that end is already busy, bit 11 of
 * its +8. `part_step_windmill` below does the same thing for kind 40.
 *
 * The rest is a timer at +0x96 and a form at +0x0c. While the timer runs the
 * form steps, and **it steps round two different loops**: past 9 it goes back
 * to 5, and past 0xd back to 9, so the animation has a short cycle and a long
 * one and which it is on depends on where it started. From 9 up it also stamps
 * 0x35 into the byte at +0x6b.
 *
 * When the timer reaches zero the part either fires - a sound, 0x52d1 set, and
 * +0x12 becoming 1 or -1 by bit 4 of the flags, which is the mirrored form -
 * or simply stops, and which of those depends on the form being past 8.
 *
 * With the timer already at zero the form creeps up by one a step, wrapping 4
 * back to 1 rather than going on, and only a form that differs from the one
 * last drawn at +0x0e is redrawn.
 *
 * The original leaves AX as whatever fell out; nothing reads it.
 */
void part_step_monkey(struct part *part)
{
    struct part *di;

    if ((di = belt_other_end(part)) != NULL && !(di->state & STATE_SELF_DRIVEN))
        di->direction = part->direction;

    if (part->kind_state != 0) {
        part->kind_state--;

        if (part->kind_state == 0) {
            if (part->form > 8) {
                play_sound(2);
                g_sound_request_02 = 2;
                if (part->state & STATE_FLIP_HORIZONTAL)
                    part->direction = -1;
                else
                    part->direction = 1;
                part->form = 1;
            } else {
                part->form = 0;
            }
        } else {
            if (part->form >= 9)
                part->attach[0].y = 53;

            part->form++;

            if (part->form == 9)
                part->form = 5;
            else if (part->form == 0x0d)
                part->form = 9;
        }

        place_object_for_draw(part);
    } else {
        if (part->direction != 0) {
            part->attach[0].y = 53;

            if (part->form != 4)
                part->form++;
            else
                part->form = 1;
        }

        if (part->form != part->form_prev) {
            place_object_for_draw(part);
            g_sound_request_02 = 2;
        }
    }
}

/*
 * 172c:2e0c, image 0x1a0cc - kind 31's flip, the four-redraw shape.
 */
void part_flip_monkey(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    part_setup_monkey(part);
    place_object_for_draw(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:2e4b, image 0x1a10b - kind 31's drive, the other half of
 * `part_step_monkey`.
 *
 * Mode 1 steps the word at +0x0e of what +0x66 points at and answers 0, the
 * same as kind 4's drive at 172c:02cd does.
 *
 * Otherwise the mode is **masked to 0x8006 and then to 0x7fff**, which leaves
 * 2, 4 or 6 and throws the top bit away, and the two masks are not the same
 * question: the second test below asks about the *0x8006* value, so a 4 with
 * the 0x8000 bit still on takes neither arm.
 *
 * Mode 2 always answers yes. Mode 4 answers yes if the part is already going -
 * +0x12 not zero - or its form has reached 9. Failing that it *acts*: a form
 * between 5 and 8 gets four added, and anything else fires, with the sound and
 * 0x52d1 and +0x12 set to 1 or -1 by bit 4 of the flags, exactly as the step
 * does when its timer runs out. Those all answer 0.
 *
 * The `+0x12 != 0` test at 0x1a15e cannot be reached with +0x12 set, because
 * mode 4 has already answered yes in that case. Transcribed anyway.
 */
uint16_t part_drive_monkey(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    uint16_t kept;                      /* [bp-2] */
    struct rope *rope;                  /* [bp-4] */

    rope = p2->rope[0];

    if (p4 == 1) {
        rope->v[0]++;
        return 0;
    }

    p4 &= 0x8006;
    kept = p4 & 0x7fff;

    if (kept == 2)
        goto yes;
    if (kept == 4 && (p2->direction != 0 || p2->form >= 9)) {
yes:
        return 1;
    }

    if (p4 == 4 && p2->direction == 0) {
        if (p2->form >= 5 && p2->form <= 8) {
            p2->form += 4;
        } else {
            play_sound(2);
            g_sound_request_02 = 2;
            if (p2->state & STATE_FLIP_HORIZONTAL)
                p2->direction = -1;
            else
                p2->direction = 1;
        }
    }

    return 0;
}
