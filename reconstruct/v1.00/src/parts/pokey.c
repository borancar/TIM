/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Pokey the cat**: its setup, hit, step and flip.
 *
 * The ninth module of the original's **code segment 172c**, image
 * 0x17edc..0x18230 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3252..0x3266
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3252..0x3266. Connection points, 5 pairs a row, indexed [flipped].
 */
struct point8 g_pokey_points[2][5] = {
    {
        { 0, 7 }, { 10, 0 }, { 36, 26 }, { 36, 37 },
        { 10, 40 },
    },
    {
        { 3, 26 }, { 29, 0 }, { 39, 10 }, { 29, 40 },
        { 3, 37 },
    },
};

/*
 * 172c:0c1c, image 0x17edc - a setup.
 *
 * Five points, on bit 4 of +8 again - 0x325c or 0x3252, ten bytes apart.
 */
void part_setup_pokey(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->state & STATE_FLIP_HORIZONTAL)
        si = g_pokey_points[1];
    else
        si = g_pokey_points[0];

    for (i = 0, di = part->points; i < 5; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:0c6c, image 0x17f2c - kind 12's hit test. Waking the cat.
 *
 * A cat in form 0 is put into form 1 with its counter cleared and mews - sound
 * 7. One already awake is left alone. It answers 1 either way.
 */
uint16_t part_hit_pokey(struct part *part)
{
    struct part *other = part->contact;

    if (other->form == 0) {
        other->form = 1;
        other->kind_state = 0;
        place_object_for_draw(other);
        play_sound(7);
    }

    return 1;
}

/*
 * 172c:0ca3, image 0x17f63 - kind 12's step. The cat.
 *
 * It walks in jumps of 0x20, and every jump is checked before it is kept: the
 * cat is moved, `object_overlaps_any` asked whether that put it inside
 * something, and if it did the move is undone by *twice* the step - a jump the
 * other way - and checked again. If that fails too it goes back where it was
 * and sits down, form 0; if the second try worked it turns round, flipping bit
 * 4 of +8.
 *
 * Sitting still it looks for what is near: `link_nearby_objects` over a box
 * that reaches 0xf0 the way it faces and 0x110 the other, and each candidate
 * gets a range at which the cat will react - a mouse, kind 0x2a, inside a small
 * box is caught outright, hidden and sounded; a kind 0x0f is 0x124 away if it
 * is past form 0x0b and 0x60 otherwise; everything else is out of reach. The
 * first thing moving slower than its range sets the cat off.
 *
 * The two `+0x96` counters are the settling time: twelve steps of standing
 * before it will move again, and four steps of the tail flicking - form 1
 * through 9 - before it settles.
 *
 * "Still" is what the first test asks - the vertical movement since the last
 * step, at +0x20 against +0x28, no more than one pixel - and a cat that is
 * still is the one that gets on with walking and looking. Reading that test the
 * other way round leaves the cat settling for ever instead, which is a
 * difference of six bytes on the first step and a trail of undrawn parts a
 * hundred and fifty frames later.
 */
void part_step_pokey(struct part *part)
{
    struct part *di;
    int16_t dx;                         /* [bp-2] */
    int16_t dy;                         /* [bp-4] */
    int16_t still;                      /* [bp-6] */
    int16_t busy;                       /* [bp-8] */
    int16_t step;                       /* [bp-0xa] */
    int16_t range;                      /* [bp-0xc] */

    dy = part->pos[0].y - part->pos[2].y;
    if (abs(dy) > 1)
        still = 0;
    else
        still = 1;

    if (part->state & STATE_FLIP_VERTICAL) {
        if (part->traits & TRAIT_HIT_FIXED) {
            part->state &= ~STATE_FLIP_VERTICAL;
            part->form = 0;
        }
    } else if (still == 0 && part->form < 2) {
        if (part->kind_state > 4) {
            part->state |= STATE_FLIP_VERTICAL;
            part->form = 1;
            part->kind_state = 0;
        } else {
            part->kind_state++;
        }
    } else if (part->form == 1) {
        part->kind_state++;
        if (part->kind_state > 0x0c) {
            if (part->state & STATE_FLIP_HORIZONTAL)
                step = 0x20;
            else
                step = -0x20;
            part->kind_state = 0;
            part->pos[0].x += step;
            place_object_for_draw(part);

            if (object_overlaps_any(part) != 0) {
                part->pos[0].x -= step * 2;
                place_object_for_draw(part);

                if (object_overlaps_any(part) != 0) {
                    part->pos[0].x += step;
                    place_object_for_draw(part);
                    part->form = 0;
                } else {
                    part->form = 2;
                    part->state ^= STATE_FLIP_HORIZONTAL;
                }
            } else {
                part->form = 2;
            }

            part->fx = part->pos[0].x;
            part->fx <<= 9;
        }
    } else {
        if (part->form != 0) {
            busy = 1;
            part->form++;
            if (part->form == 0x0a)
                part->form = 0;
        } else {
            busy = 0;
        }

        if (part->form == 0) {
            if (part->state & STATE_FLIP_HORIZONTAL)
                link_nearby_objects(part, (TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST), 0, 0xf0, 0, 0);
            else
                link_nearby_objects(part, (TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST), (int16_t)0xff10, 0, 0, 0);

            di = part->next_linked;
            while (di != NULL) {
                switch (di->kind) {
                case KIND_MORT_THE_MOUSE:
                    dx = di->pos[0].x - part->pos[0].x + 16;
                    dy = di->pos[0].y - part->pos[0].y;

                    if (dx > 0 && dx < 56 && dy > 0 && dy < 40) {
                        mark_part_shapes(di, 3);
                        di->state |= STATE_GONE;
                        play_sound(0x0d);
                        range = -1;
                    } else if (busy != 0) {
                        range = 0xc0;
                    } else {
                        range = 0x80;
                    }
                    break;
                case KIND_BOB_THE_FISH:
                    if (di->form >= 0x0b)
                        range = 0x124;
                    else
                        range = 0x60;
                    break;
                default:
                    range = -1;
                    break;
                }

                if (abs(di->link_dx) < range) {
                    if (part->state & STATE_FLIP_HORIZONTAL)
                        step = 0x20;
                    else
                        step = -0x20;
                    part->kind_state = 0;
                    part->pos[0].x += step;
                    place_object_for_draw(part);

                    if (object_overlaps_any(part) != 0) {
                        part->pos[0].x -= step;
                        place_object_for_draw(part);
                        part->form = 0;
                    } else {
                        part->form = 2;
                    }

                    di = NULL;
                    part->fx = part->pos[0].x;
                    part->fx <<= 9;
                } else {
                    di = di->next_linked;
                }
            }
        }
    }

    if (part->form != part->form_prev)
        place_object_for_draw(part);
}

/*
 * 172c:0f3d, image 0x181fd - kind 12's flip.
 *
 * Byte for byte the same routine as `part_flip_bellow` above with a different
 * setup behind it - checked as bytes, not assumed: of the twenty flips in this
 * segment only four are this shape and the other sixteen are not, so the
 * family is real but small.
 */
void part_flip_pokey(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;

    part_setup_pokey(part);

    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
