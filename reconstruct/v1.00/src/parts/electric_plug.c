/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The electric plug**: its hit, setup, step and flip.
 *
 * The fifteenth module of the original's **code segment 172c**, image
 * 0x18793..0x18909 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x32b8..0x32c8
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x32b8..0x32c0. Connection points, 4 pairs.
 */
struct point8 g_electric_plug_points_32b8[4] = {
    { 0x08, 0x08 }, { 0x0f, 0x08 }, { 0x0f, 0x09 }, { 0x08, 0x09 },
};

/*
 * DGROUP 0x32c0..0x32c8. Connection points, 4 pairs.
 */
struct point8 g_electric_plug_points_32c0[4] = {
    { 0x0f, 0x18 }, { 0x08, 0x18 }, { 0x08, 0x17 }, { 0x0f, 0x17 },
};

/*
 * 172c:14d3, image 0x18793 - kind 21's hit test. The see-saw.
 *
 * Which way it tips comes from the angle at +0x88 of the thing that hit it,
 * turned a quarter and then read as a sign: the high bit of `angle + 0x4000`.
 * Below form 4 and pushed one way it goes up by four; at form 4 or above and
 * pushed the other it comes down by four, and either move runs its own setup
 * again and plays sound 0x11.
 *
 * Then it is *on* whenever its form is not the one at +0x90 - which is where a
 * see-saw's rest position is kept - and a kind 0x14 that hit it has its
 * sideways velocity stepped down by one. It answers 0, so the hit does not
 * count as a landing.
 */
uint16_t part_hit_electric_plug(struct part *part)
{
    struct part *si = part->contact;
    uint16_t turned;                    /* [bp-2] */

    turned = part->contact_angle;
    turned += 0x4000;

    if (si->form < 4) {
        if (!(turned & 0x8000)) {
            si->form += 4;
            part_setup_electric_plug(si);
            play_sound(0x11);
        }
    } else if (turned & 0x8000) {
        si->form -= 4;
        part_setup_electric_plug(si);
        play_sound(0x11);
    }

    if (si->form != si->start_form)
        si->direction = 1;
    else
        si->direction = 0;

    if (part->kind == KIND_BULLET)
        part->vel_x--;

    return 0;
}

/*
 * 172c:1556, image 0x18816 - a setup.
 *
 * The tail is the part worth reading twice. +0x80 is set to 4 for the duration
 * of `part_finish` and put back to 1 afterwards, so the finish sees four
 * points on a part that carries one; and then +0x0c keeps only bit 2 and takes
 * bits 0 and 1 from whichever of +0x62 and +0x64 is linked.
 *
 * A routine transcribed from this address stopped at the finish and had none
 * of that. It was never called - it duplicated this body, which already ran -
 * and comparing the two is how the truncation was found. Read a routine to its
 * `retf`, not to its loop.
 */
void part_setup_electric_plug(struct part *part)
{
    /*
     * The form decides the table by being under 4 rather than by equalling
     * anything, and the count at +0x80 is **raised to 4 for the angles and
     * then dropped to 1** - so the part has four connection points while
     * they are being measured and one afterwards.
     */
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->form < 4)
        src = g_electric_plug_points_32b8;
    else
        src = g_electric_plug_points_32c0;

    for (i = 0, dst = part->points; i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part->point_count = 4;
    part_finish_angles(part);
    part->point_count = 1;

    part->form &= 4;
    if (part->link[4] != 0)
        part->form |= 1;
    if (part->link[5] != 0)
        part->form |= 2;
}

/*
 * 172c:15ce, image 0x1888e - kind 21's step.
 *
 * It does not move: it marks itself done - bit 6 of +8 - and passes its own
 * +0x12 on to whatever is in its links 4 and 5. The first four links are
 * something else's; these two are the ones this kind wires up.
 */
void part_step_electric_plug(struct part *part)
{
    int16_t i;                          /* dx */
    struct part *di;

    part->state |= STATE_STEPPED;

    for (i = 4; i < 6; i++)
        if ((di = part->link[i]) != NULL)
            di->direction = part->direction;
}

/*
 * 172c:15fc, image 0x188bc - kind 21's flip, and **it is not a flip of bit 4
 * at all**. The form at +0x0c is swung between 4 and 0 - four or more goes to
 * zero, anything else to four - and copied into +0x90 before the setup runs.
 *
 * So this part has two forms held in the form word rather than in the flags,
 * and reading the family's name onto it would have got it wrong.
 */
void part_flip_electric_plug(struct part *part)
{
    if (part->form < 4)
        part->form = 4;
    else
        part->form = 0;

    part->start_form = part->form;

    part_setup_electric_plug(part);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
