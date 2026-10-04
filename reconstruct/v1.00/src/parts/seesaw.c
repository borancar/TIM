/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The seesaw**: its hit, setup, flip, step and drive, the ropes it drives.
 *
 * The fortieth module of the original's **code segment 172c**, image
 * 0x1b2a8..0x1ba3d - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x34ca..0x355a
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x34ca..0x34e2. The two attach points, 3 points a row, indexed [attach][form].
 */
struct point16 g_seesaw_attach[2][3] = {
    {
        { 5, 27 }, { 4, 2 }, { 6, 3 },
    },
    {
        { 73, 3 }, { 75, 2 }, { 74, 27 },
    },
};

/*
 * DGROUP 0x34e2..0x3542. Connection points, 8 points a row, indexed [form].
 */
struct point16 g_seesaw_points[3][8] = {
    {
        { 0, 32 }, { 79, 3 }, { 79, 8 },
        { 44, 21 }, { 44, 34 }, { 36, 34 },
        { 36, 24 }, { 0, 36 },
    },
    {
        { 0, 17 }, { 79, 17 }, { 79, 21 },
        { 44, 21 }, { 44, 34 }, { 36, 34 },
        { 36, 21 }, { 0, 21 },
    },
    {
        { 0, 3 }, { 79, 32 }, { 79, 36 },
        { 44, 24 }, { 44, 34 }, { 36, 34 },
        { 36, 21 }, { 0, 8 },
    },
};

/*
 * DGROUP 0x3542..0x355a. The seesaw's shaft by form, a segment - its two end
 * points - which `part_step_seesaw` hands `link_objects_crossing`.
 */
struct point16 g_seesaw_shaft_line[3][2] = {
    { { 0, 32 }, { 79, 3 } },
    { { 0, 17 }, { 79, 17 } },
    { { 0, 3 }, { 79, 32 } },
};

/*
 * 172c:3fe8, image 0x1b2a8 - kind 3's hit test. Standing on the motor.
 *
 * A motor whose rope is held - bit 9 of +8 - answers 1 at once and does
 * nothing: it cannot be turned by being stood on.
 *
 * Otherwise the face that was touched, +0x8a of the thing that hit, decides.
 * Faces 0, 2 and 6 can turn it; anything else is a plain hit. Face 0 is the
 * top and is split by where along it the contact fell: past 0x2c is one end,
 * up to 0x24 the other, and between them nothing. Faces 2 and 6 are the sides
 * and turn it by which form it is in.
 *
 * Turning is not done here: `queue_part` asks for the motor to be stepped, and
 * only if the queue took it does the direction go across, with the asking
 * part's priority. A motor already on the queue at a better priority makes this
 * a plain hit instead - and the thing's own contact is cleared when the turn
 * was taken, so it does not also bounce.
 */
uint16_t part_hit_seesaw(struct part *part)
{
    struct part *si = part->contact;
    int16_t face;                       /* [bp-2] */
    int16_t along;                      /* [bp-4] */
    int16_t dir;                        /* [bp-6] */
    int16_t plain;                      /* [bp-8] */

    if (si->state & STATE_HELD)
        return 1;

    face = part->contact_edge;

    if (face == 0 || face == 2 || face == 6)
        plain = 0;
    else
        plain = 1;

    switch (face) {
    case 0:
        along = part->pos[0].x + (part->size[0].width >> 1) - si->pos[0].x;

        if (along >= 44) {
            if (si->form == 2)
                plain = 1;
            else
                dir = 1;
        } else if (along <= 36) {
            if (si->form == 0)
                plain = 1;
            else
                dir = -1;
        } else {
            plain = 1;
        }
        break;
    case 2:
        if (si->form == 0)
            plain = 1;
        else
            dir = -1;
        break;
    case 6:
        if (si->form == 2)
            plain = 1;
        else
            dir = 1;
        break;
    }

    if (plain == 0) {
        if (queue_part(part, part->contact) != 0) {
            si->direction = dir;
            si->momentum = part->momentum;
            part->contact = 0;
        } else {
            plain = 1;
        }
    }

    return plain;
}

/*
 * 172c:40f0, image 0x1b3b0
 *
 * A part with **three forms**, and the word at +0x0c says which. Its four
 * bytes at +0x6a..+0x6d - the box it is grabbed by - come out of one table
 * indexed by that word, and its eight connection points out of one of three
 * others, chosen by the same word with a `switch`.
 *
 * Its tables are `point16` rather than `point8` - four bytes an entry with
 * the coordinate at +0 and +2 - which the image settles: every other byte in
 * all five of them is zero. The setup reads each with a byte move, so it takes
 * the low half of each word and the top halves are never looked at.
 *
 * A form other than 0, 1 or 2 leaves the point untouched rather than defaulting
 * to one of them: the `jmp` at the end of the switch goes to the loop's own
 * increment.
 */
void part_setup_seesaw(struct part *part)
{
    struct part_point *di;
    int16_t i;                          /* [bp-2] */

    part->attach[0].x = (uint8_t)g_seesaw_attach[0][part->form].x;
    part->attach[0].y = (uint8_t)g_seesaw_attach[0][part->form].y;
    part->attach[1].x = (uint8_t)g_seesaw_attach[1][part->form].x;
    part->attach[1].y = (uint8_t)g_seesaw_attach[1][part->form].y;

    for (i = 0, di = part->points; i < 8; i++, di++) {
        switch (part->form) {
        case 0:
            di->x = (uint8_t)g_seesaw_points[0][i].x;
            di->y = (uint8_t)g_seesaw_points[0][i].y;
            break;
        case 1:
            di->x = (uint8_t)g_seesaw_points[1][i].x;
            di->y = (uint8_t)g_seesaw_points[1][i].y;
            break;
        case 2:
            di->x = (uint8_t)g_seesaw_points[2][i].x;
            di->y = (uint8_t)g_seesaw_points[2][i].y;
            break;
        }
    }

    part_finish_angles(part);
}

/*
 * 172c:41bb, image 0x1b47b - kind 3's flip, the 0-or-2 form swing with all
 * four redraws behind it.
 */
void part_flip_seesaw(struct part *part)
{
    if (part->form != 0)
        part->form = 0;
    else
        part->form = 2;

    part->start_form = part->form;

    part_setup_seesaw(part);
    place_object_for_draw(part);
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:420f, image 0x1b4cf - kind 3's step. The motor.
 *
 * Nothing happens unless +0x12 says it is on. Then it marks itself done - bit
 * 6 of +8 - and either turns freely, when bit 10 of +8 is set, or asks its
 * ropes first: `drive_ropes` twice, once with 0x8000 in the flags and once
 * without, and an answer from the first means the rope is being held, which
 * sets bit 9 of +8 and stops it turning. Otherwise the second call goes out
 * anyway and the form steps by the direction.
 *
 * A form that has changed runs `part_setup_seesaw` - the motor's connection
 * points move with it - and forms 0 and 2 play sound 0x12. Then it looks for
 * what its shaft is over: `link_objects_crossing_segments` gives it the
 * candidates and each is given a speed from `push_speed_for_mass`, signed by
 * which side of the shaft's middle it lies. Each one is then dropped 0x10 and
 * lifted 0x10 through `resolve_collisions` to settle it, with the motor hidden
 * for the second of the two so it does not collide with itself.
 *
 * Finally, at form 0 with the last form non-zero, it reaches out once more:
 * `link_objects_in_range` over a box 0x4a to 0x4f across and 2 down, and
 * `trigger_things_at` sets going whatever is there.
 */
void part_step_seesaw(struct part *part)
{
    struct part *di;
    int16_t v02;                        /* [bp-2] the shaft's middle */
    int16_t v04;                        /* [bp-4] the other's middle */
    int16_t v06;                        /* [bp-6] the speed */
    int32_t fy;                         /* [bp-0xa] */

    if (part->direction != 0) {
        part->state |= STATE_STEPPED;

        if (part->state & STATE_TURNS_FREE)
            part->form += part->direction;
        else if (drive_ropes(NULL, part, 0x8000, 0x3e8, part->momentum) != 0)
            part->state |= STATE_HELD;
        else {
            drive_ropes(NULL, part, 0, 0x3e8, part->momentum);
            part->form += part->direction;
        }

        if (part->form != part->form_prev) {
            part_setup_seesaw(part);

            if (part->form_prev == 0 || part->form_prev == 2)
                play_sound(0x12);

            place_object_for_draw(part);

            v02 = part->pos[0].x + (part->size[0].width >> 1);

            link_objects_crossing(part, TRAIT_IN_MOVING_LIST, g_seesaw_shaft_line[part->form]);

            for (di = part->next_linked; di != NULL;
                 di = di->next_linked) {
                v04 = di->pos[0].x + (di->size[0].width >> 1);
                v06 = push_speed_for_mass(di);

                switch (part->direction) {
                case -1:
                    if (v04 < v02) {
                        di->vel_y = v06;
                        di->vel_x = 0 - (v06 >> 2);
                    } else {
                        di->vel_y = 0 - v06;
                        di->vel_x = v06 >> 2;
                    }
                    break;
                case 1:
                    if (v04 < v02) {
                        di->vel_y = 0 - v06;
                        di->vel_x = 0 - (v06 >> 2);
                    } else {
                        di->vel_y = v06;
                        di->vel_x = v06 >> 2;
                    }
                    break;
                }

                mark_part_shapes(di, 3);

                if (di->vel_y < 0) {
                    di->pos[1].y = di->pos[0].y - 16;
                    resolve_collisions(di);

                    di->pos[1].y = di->pos[0].y + 16;
                    part->state |= STATE_GONE;
                    resolve_collisions(di);
                    part->state &= ~STATE_GONE;

                    di->pos[1].y = di->pos[0].y;

                    fy = di->pos[0].y;
                    di->fy = fy << 9;
                } else {
                    di->pos[1].y = di->pos[0].y + 16;
                    resolve_collisions(di);

                    di->pos[1].y = di->pos[0].y - 16;
                    part->state |= STATE_GONE;
                    resolve_collisions(di);
                    part->state &= ~STATE_GONE;

                    di->pos[1].y = di->pos[0].y;

                    fy = di->pos[0].y;
                    di->fy = ((fy + 1) << 9) - 1;
                }
            }
        }

        part->direction = 0;
        part->momentum = 0;
    }

    /*
     * The two ends of the stroke reach out, in opposite pairs: at form 0 the
     * near side of the shaft is at 0x4a..0x4f across and level, and the far
     * side at 0..6 and 0x20..0x24 down; at form 2 the two swap over. Each box
     * is followed by a `trigger_things_at` for the point it was measured from.
     */
    if (part->form_prev == 0 && part->form_prev2 != 0) {
        link_objects_in_range(part, TRAIT_IN_PLACED_LIST, 74, 79, -2, 2);
        trigger_things_at(part, 0, 0x4a);

        link_objects_in_range(part, TRAIT_IN_PLACED_LIST, 0, 6, 32, 36);
        trigger_things_at(part, 1, 0);
    } else if (part->form_prev == 2 && part->form_prev2 != 2) {
        link_objects_in_range(part, TRAIT_IN_PLACED_LIST, 74, 79, 32, 36);
        trigger_things_at(part, 1, 0x4a);

        link_objects_in_range(part, TRAIT_IN_PLACED_LIST, 0, 6, -2, 2);
        trigger_things_at(part, 0, 0);
    }
}

/*
 * 172c:44fe, image 0x1b7be - kind 3's drive, and the longest of them.
 *
 * `p3` picks **which of a pair** of pointers at +0x66 to work through - it is
 * doubled and used as an index - so this kind has two ends and is driven at
 * each independently.
 *
 * The mode is masked to 0x8007 and then to 0x7fff; the 0x8000 bit is kept
 * apart and carried into `drive_ropes` and tested again afterwards, so it is
 * "ask, do not act". Away from mode 1, a non-zero counter at +0x0e of the
 * chosen pointer is decremented and the answer is 0 - unless the 0x8000 bit is
 * set, when it is left alone. That is the "already busy" path.
 *
 * Modes 2 and 4 are the two directions, and each asks whether the form at
 * +0x0c is already at the end it would be driven to: at that end `di` is set
 * and nothing is driven, otherwise +0x12 is loaded with 1 or -1 and
 * `drive_ropes` is asked to carry it. `p3` swaps which end counts, which is
 * what makes the two ends opposite.
 *
 * Afterwards the 0x8000 bit puts +0x12 back to what it was, and a `drive_ropes`
 * that answered nothing sets bit 10 of +8. Bit 9 is set when `di` is non-zero,
 * and bit 9 being set at the end is the answer 1.
 *
 * **A local is read before it is written.** [bp-2] is only set inside the mode
 * 2 and mode 4 arms, and a mode that is neither - anything but 1, 2 and 4 -
 * reaches `[si+0x12] = [bp-2]` with whatever was on the stack. The port cannot
 * reproduce an uninitialised DOS stack and does not try; `drive` here starts at
 * zero, which is the one value that is certainly wrong in the same way for
 * every run rather than differently each time. Recorded because it is a real
 * difference and not a transcription slip.
 */
uint16_t part_drive_seesaw(struct part *p1, struct part *p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, int32_t p6)
{
    int16_t  di;
    int16_t  drive;                     /* [bp-2] */
    int16_t  was;                       /* [bp-4] */
    uint16_t mode;                      /* [bp-6] */
    struct rope *chain;                 /* [bp-8] */

    chain = p2->rope[p3];
    p4 &= 0x8007;
    mode = p4 & 0x7fff;

    if (mode != 1 && chain->v[0] != 0) {
        if (!(p4 & 0x8000))
            chain->v[0]--;
        return 0;
    }

    was = p2->direction;
    di = 0;
#ifndef __TURBOC__
    drive = 0;      /* the original reads [bp-2] unset here: see above */
#endif

    if (mode == 4) {
        if (p3 == 0) {
            if (p2->form == 0)
                di = 1;
            else
                drive = -1;
        } else {
            if (p2->form == 2)
                di = 1;
            else
                drive = 1;
        }
    } else if (mode == 2) {
        if (p3 == 0) {
            if (p2->form == 2)
                di = 1;
            else
                drive = 1;
        } else {
            if (p2->form == 0)
                di = 1;
            else
                drive = -1;
        }
    }

    if (di == 0 && mode != 1) {
        p2->direction = drive;

        di = drive_ropes(p1, p2, p4 & 0x8000, p5, p6);

        if (p4 & 0x8000)
            p2->direction = was;
        else if (di == 0)
            p2->state |= STATE_TURNS_FREE;
    }

    if (di != 0)
        p2->state |= STATE_HELD;

    if (p2->state & STATE_HELD)
        return 1;

    if (p4 == 1)
        chain->v[0]++;

    return 0;
}

/*
 * 172c:461a, image 0x1b8da
 *
 * Push a part's motion out along its ropes, and answer whether anything
 * refused.
 *
 * Each of the two ropes at +0x66 leads to another part, which
 * `rope_other_end` names. The one the caller came *from* is skipped, which is
 * what stops the walk going back on itself. `rope_orientation` says how the
 * rope runs between them - which way round the tangent points are - and that,
 * or-ed with the caller's own flags, is handed on with the part.
 *
 * The handler is the far pointer at +0x36 of the *far* part's kind record, so
 * what happens next is that part's business and not this one's. A part already
 * marked with bit 9 of +8 answers 1 straight away, and the walk stops at the
 * first rope that answers anything at all.
 */
uint16_t drive_ropes(struct part *from, struct part *part, uint16_t flags,
                     uint16_t a, int32_t momentum)
{
    struct rope *si;                       /* the rope */
    int16_t  v02;                       /* [bp-2]    the rope */
    uint16_t v04;                       /* [bp-4]    the answer */
    int16_t  v06;                       /* [bp-6]    which end */
    uint16_t v08;                       /* [bp-8]    the near slot */
    uint16_t v0a;                       /* [bp-0xa]  the far slot */
    uint16_t v0c;                       /* [bp-0xc]  how the rope runs */
    int16_t  v0e;                       /* [bp-0xe]  which way */
    struct part *v10;                   /* [bp-0x10] the far part */

    if (part->state & STATE_HELD)
        return 1;

    v04 = 0;

    for (v02 = 0; v02 < 2 && v04 == 0; v02++) {
        if ((si = part->rope[v02]) != 0) {
            v10 = (rope_other_end(part, si));
            if (v10 != from) {
                if ((si->end_a) == part) {
                    v06 = 0;
                    v08 = si->slot_a;
                    v0a = si->slot_b;
                } else {
                    v06 = 1;
                    v08 = si->slot_b;
                    v0a = si->slot_a;
                }

                if (part->direction > 0) {
                    if (v08 == 0)
                        v0e = 0;
                    else
                        v0e = 1;
                } else {
                    if (v08 == 0)
                        v0e = 1;
                    else
                        v0e = 0;
                }

                v0c = rope_orientation(si, v06, v0e);
                v0c |= flags;

                v04 = g_part_kinds[v10->kind].drive(part, v10, v0a, v0c, a, momentum);
            }
        }
    }

    return v04;
}

/*
 * 172c:471f, image 0x1b9df
 *
 * The same ladder as `bounce_speed_for_mass`, one step longer at the light
 * end: 0x1c00 below a mass of 2, and the rest of the steps as before. The two
 * exist separately in the original and are kept separate here.
 */
int16_t push_speed_for_mass(struct part *obj)
{
    int16_t m;                          /* si */
    int16_t r;                          /* cx */

    m = g_part_kinds[obj->kind].weight;

    if (m < 0x0002)
        r = 0x1c00;
    else if (m < 0x0006)
        r = 0x1a00;
    else if (m < 0x000a)
        r = 0x1800;
    else if (m < 0x0015)
        r = 0x1600;
    else if (m < 0x0079)
        r = 0x1400;
    else if (m < 0x0097)
        r = 0x1200;
    else
        r = 0x1000;

    return r;
}
