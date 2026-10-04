/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The boxing glove**: its hit, step, setup and flip, and the speed it gives by mass.
 *
 * The fourth module of the original's **code segment 172c**, image
 * 0x17812..0x17a23 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x31e6..0x3222
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x31e6..0x31f2. How far the boxing glove reaches, by form: -32 -82 0 80 130 0.
 * `part_step_boxing_glove` indexes it from +1 and from -2.
 */
int16_t g_boxing_glove_reach[6] = { -32, -82, 0, 80, 130, 0 };

/*
 * DGROUP 0x31f2..0x3222. Connection points, 6 pairs a row, indexed [flipped][form != 0].
 */
struct point8 g_boxing_glove_points[2][2][6] = {
    {
        {
            { 0, 12 }, { 16, 0 }, { 47, 5 }, { 47, 20 },
            { 26, 21 }, { 9, 27 },
        },
        {
            { 5, 21 }, { 17, 10 }, { 47, 5 }, { 47, 20 },
            { 26, 21 }, { 9, 27 },
        },
    },
    {
        {
            { 38, 27 }, { 21, 21 }, { 0, 20 }, { 0, 5 },
            { 31, 0 }, { 47, 12 },
        },
        {
            { 38, 27 }, { 21, 21 }, { 0, 20 }, { 0, 5 },
            { 30, 10 }, { 43, 21 },
        },
    },
};

/*
 * 172c:0552, image 0x17812 - kind 35's hit test.
 *
 * Being hit on face 2 sets the thing that hit it going; any other face does
 * nothing. It answers 1 either way, so the hit still counts.
 */
uint16_t part_hit_boxing_glove(struct part *part)
{
    struct part *di = part->contact;
    int16_t edge = part->contact_edge;          /* [bp-2] */

    if (edge == 2)
        di->direction = 1;

    return 1;
}

/*
 * 172c:057e, image 0x1783e - kind 35's step.
 *
 * A swing. While its +0x12 says go and it has not reached form 9 it steps one
 * frame - the first one plays sound 3 - and runs its own setup again, because
 * its connection points move with the swing.
 *
 * Forms 2 and 3 are where it reaches something: a box in front of it, taken
 * from one of two tables by the mirror bit and indexed by the form, and
 * everything caught in it is dealt with by kind. Bit 12 of +6 means it can be
 * knocked along, and it is given a speed by its own mass; kind 0x0f breaks;
 * kind 6 is set going.
 */
void part_step_boxing_glove(struct part *part)
{
    struct part *di;
    int16_t v;                          /* [bp-2] */

    if (part->direction != 0 && part->form != 9) {
        if (part->form == 0)
            play_sound(3);

        part->form++;
        part_setup_boxing_glove(part);
        place_object_for_draw(part);
    }

    if (part->form == 2 || part->form == 3) {
        /* Forms 2 and 3 only: the reach is `glove_reach` at 0x31e6, whose
           first two words face left and the next ones right. */
        if (part->state & STATE_FLIP_HORIZONTAL)
            link_objects_in_range(part, (TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST), 48,
                                  (g_boxing_glove_reach + 1)[part->form], 0, 31);
        else
            link_objects_in_range(part, (TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST),
                                  (g_boxing_glove_reach - 2)[part->form], 0, 0, 31);

        for (di = part->next_linked; di != NULL;
             di = di->next_linked) {
            if (di->traits & TRAIT_IN_MOVING_LIST) {
                v = bounce_speed_for_mass(di);
                di->vel_x = (part->state & STATE_FLIP_HORIZONTAL) ? v : 0 - v;
            } else {
                switch (di->kind) {
                case KIND_BOB_THE_FISH:
                    break_bob_the_fish(di);
                    break;
                case KIND_MOUSE_CAGE:
                    trigger_mouse_cage(di);
                    break;
                }
            }
        }
    }
}

/*
 * 172c:065b, image 0x1791b - six slots, from one of four tables. The flag at
 * +8 bit 4 picks the pair and the form at +0x0c picks within it, so the four
 * sit as 0x31f2, 0x31fe, 0x320a, 0x3216 - twelve bytes apart, six pairs each.
 *
 * This and the setups below it share the copy at the end - N pairs, two bytes
 * into every four - and differ only in how the source table is chosen and what
 * else is set first.
 */
void part_setup_boxing_glove(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    /* Four tables: the flag at +8, and then whether the form is zero. */
    if (part->state & STATE_FLIP_HORIZONTAL) {
        if (part->form == 0)
            src = g_boxing_glove_points[1][0];
        else
            src = g_boxing_glove_points[1][1];
    } else {
        if (part->form == 0)
            src = g_boxing_glove_points[0][0];
        else
            src = g_boxing_glove_points[0][1];
    }

    for (i = 0, dst = part->points; i < 6; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:06c6, image 0x17986 - kind 35's flip.
 *
 * Byte for byte the same routine as `part_flip_bellow` above with a different
 * setup behind it - checked as bytes, not assumed: of the twenty flips in this
 * segment only four are this shape and the other sixteen are not, so the
 * family is real but small.
 */
void part_flip_boxing_glove(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;

    part_setup_boxing_glove(part);

    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:06f9, image 0x179b9
 *
 * How fast a thing is thrown, by how heavy it is: the mass at DGROUP 0xea8 for
 * its kind, in seven steps from 0x1a00 for the lightest down to 0x0c00 for the
 * heaviest. Written as a ladder of compares rather than a table.
 */
int16_t bounce_speed_for_mass(struct part *obj)
{
    int16_t m;                          /* si */
    int16_t r;                          /* cx */

    m = g_part_kinds[obj->kind].weight;

    if (m < 0x0006)
        r = 0x1a00;
    else if (m < 0x000a)
        r = 0x1800;
    else if (m < 0x0015)
        r = 0x1600;
    else if (m < 0x0079)
        r = 0x1400;
    else if (m < 0x0097)
        r = 0x1200;
    else if (m < 0x00c9)
        r = 0x1000;
    else if (m < 0x0709)
        r = 0x0e00;
    else
        r = 0x0c00;

    return r;
}
