/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The dynamite plunger**: its hit, setup, step, flip and drive.
 *
 * The thirty-second module of the original's **code segment 172c**, image
 * 0x1a4ff..0x1a72f - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x2f70..0x2fc4
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x2f70..0x2f88. Outline points, 4 pairs a row, indexed [form].
 */
struct point8 g_plunger_outline[3][4] = {
    {
        { 103, 0 }, { 134, 0 }, { 127, 47 }, { 111, 47 },
    },
    {
        { 103, 5 }, { 134, 5 }, { 127, 47 }, { 111, 47 },
    },
    {
        { 103, 10 }, { 134, 10 }, { 127, 47 }, { 111, 47 },
    },
};

/*
 * DGROUP 0x2f88..0x2f8e. **Which row of points, by form**: a near pointer each.
 */
struct point8 *g_plunger_outline_table[3] = {
    g_plunger_outline[0], g_plunger_outline[1],
    g_plunger_outline[2],
};

/*
 * DGROUP 0x2f8e..0x2fa6. Outline points, 4 pairs a row, indexed [form].
 */
struct point8 g_plunger_outline_flipped[3][4] = {
    {
        { 0, 0 }, { 31, 0 }, { 24, 47 }, { 8, 47 },
    },
    {
        { 0, 5 }, { 31, 5 }, { 24, 47 }, { 8, 47 },
    },
    {
        { 0, 10 }, { 31, 10 }, { 24, 47 }, { 8, 47 },
    },
};

/*
 * DGROUP 0x2fa6..0x2fac. **Which row of points, by form**: a near pointer each.
 */
struct point8 *g_plunger_outline_table_flipped[3] = {
    g_plunger_outline_flipped[0], g_plunger_outline_flipped[1],
    g_plunger_outline_flipped[2],
};

/*
 * DGROUP 0x2fac..0x2fc4. The attach point, 3 points a row, indexed [flipped][form].
 */
struct point16 g_plunger_attach[2][3] = {
    {
        { 114, 0 }, { 114, 5 }, { 114, 10 },
    },
    {
        { 11, 0 }, { 11, 5 }, { 11, 10 },
    },
};

/*
 * 190f:31c8, image 0x1c2b8 - kind 22's hit test, the trigger for
 * `part_step_dynamite_plunger`.
 *
 * A touch on face 0 sets the part going outright. Any other face has to be
 * something landing on it: the arriving object's +0x38 must be positive, which
 * is the downward half of the velocity pair, its +0x88 must be under 0x800
 * once 0x800 is added - so between -0x800 and 0x800, a shallow angle - and it
 * must be above the part, its +0x20 plus +0x42 short of the part's +0x20 by
 * more than 0xc.
 *
 * It answers 1 either way, like every other hit test here.
 */
uint16_t part_hit_dynamite_plunger(struct part *part)
{
    struct part *di = part->contact;
    int16_t face = part->contact_edge;  /* [bp-2] */

    if (face == 0)
        di->direction = 1;
    else if (part->vel_y > 0
             && (int16_t)(part->contact_angle + 0x800) < 0x1000
             && part->pos[0].y + part->flip_size.height < di->pos[0].y + 12)
        di->direction = 1;

    return 1;
}

/*
 * 190f:3216, image 0x1c306 - a setup.
 *
 * Four points and a grab box, all three read out of tables indexed by the form
 * at +0x0c, and bit 4 of +8 picks which set of three tables. The points come
 * through a **pointer array** - the load is a word - and the two box bytes out
 * of four-byte rows, so the same form indexes two different strides.
 */
void part_setup_dynamite_plunger(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->state & STATE_FLIP_HORIZONTAL) {
        src = g_plunger_outline_table_flipped[part->form];
        part->attach[0].x = (uint8_t)g_plunger_attach[1][part->form].x;
        part->attach[0].y = (uint8_t)g_plunger_attach[1][part->form].y;
    } else {
        src = g_plunger_outline_table[part->form];
        part->attach[0].x = (uint8_t)g_plunger_attach[0][part->form].x;
        part->attach[0].y = (uint8_t)g_plunger_attach[0][part->form].y;
    }

    for (i = 0, dst = part->points; i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 190f:32a9, image 0x1c399 - kind 22's step. **It makes a new part.**
 *
 * On the first frame of its three - +0x0c at 1 - it calls `make_part` for a
 * kind-0x29 part, files it on the list at 0x521b, and puts it half a part to
 * the left of itself, or 0x60 to the right of that when bit 4 of the flags
 * says it is mirrored. The new part's 32-bit position at +0x16 and +0x1a is
 * the 16-bit one shifted left nine, which is the fixed point the physics uses.
 *
 * `make_part` answering zero is a full heap and is simply skipped; the part
 * still steps its own form.
 *
 * The form then walks 1, 2 and stops, rebuilt each time through the setup at
 * 172c:3294, called directly.
 */
void part_step_dynamite_plunger(struct part *part)
{
    struct part *si;

    if (part->direction != 0) {
        if (part->form == 1) {
            play_sound(8);

            if ((si = make_part(KIND_BLAST)) == NULL)
                goto done;

            insert_sorted(si, &g_placed_parts);
            si->traits |= TRAIT_SPAWNED;
            si->pos[0].x = part->pos[0].x - 16;
            si->pos[0].y = part->pos[0].y;

            if (part->state & STATE_FLIP_HORIZONTAL)
                si->pos[0].x += 96;

            si->fx = si->pos[0].x;
            si->fx <<= 9;
            si->fy = si->pos[0].y;
            si->fy <<= 9;

            place_object_for_draw(si);
        }
done:
        if (part->form != 2) {
            part->form++;
            part_setup_dynamite_plunger(part);
            place_object_for_draw(part);
        }
    }
}

/*
 * 190f:3362, image 0x1c452 - kind 22's flip: bit 4, its setup, three marks and
 * no draw.
 */
void part_flip_dynamite_plunger(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    part_setup_dynamite_plunger(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 190f:339a, image 0x1c48a - kind 22's drive, the same family as kind 31's at
 * 172c:2e4b: the mode masked to 0x8006 and then to 0x7fff, which leaves 2, 4
 * or 6 and drops the top bit, and the second test asking about the *0x8006*
 * value so a 4 with the top bit on takes neither arm.
 *
 * Mode 2 always answers yes and mode 4 answers yes once the form has reached
 * 2. Failing that, a plain 4 sets +0x12 going if it is not going already, and
 * answers 0 - so this is the drive that starts a kind 22 rather than reporting
 * on it.
 */
uint16_t part_drive_dynamite_plunger(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    struct rope *di = p2->rope[0];
    uint16_t low;                       /* cx */

    if (p4 == 1) {
        di->v[0]++;
        return 0;
    }

    p4 &= 0x8006;
    low = p4 & 0x7fff;

    if (low == 2)
        return 1;
    if (low == 4 && p2->form == 2)
        return 1;

    if (p4 == 4 && p2->direction == 0)
        p2->direction = 1;

    return 0;
}
