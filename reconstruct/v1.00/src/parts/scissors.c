/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The scissors**: its hit, setup, step and flip, and the ropes they cut.
 *
 * The thirty-seventh module of the original's **code segment 172c**, image
 * 0x1aae4..0x1b0a5 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3472..0x34ca
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3472..0x3492. Connection points, 8 pairs a row, indexed [form].
 */
struct point8 g_scissors_points[2][8] = {
    {
        { 0, 4 }, { 10, 0 }, { 17, 11 }, { 39, 7 },
        { 39, 26 }, { 17, 20 }, { 11, 33 }, { 0, 31 },
    },
    {
        { 0, 8 }, { 9, 4 }, { 16, 14 }, { 39, 16 },
        { 39, 18 }, { 16, 20 }, { 9, 27 }, { 0, 24 },
    },
};

/*
 * DGROUP 0x3492..0x3496. **Which row of points, by form**: a near pointer each.
 */
struct point8 *g_scissors_point_table[2] = { g_scissors_points[0], g_scissors_points[1] };

/*
 * DGROUP 0x3496..0x34b6. Connection points, 8 pairs a row, indexed [form].
 */
struct point8 g_scissors_points_flipped[2][8] = {
    {
        { 0, 7 }, { 22, 11 }, { 29, 0 }, { 39, 4 },
        { 39, 31 }, { 28, 33 }, { 22, 20 }, { 0, 26 },
    },
    {
        { 0, 16 }, { 23, 14 }, { 30, 4 }, { 39, 8 },
        { 39, 24 }, { 30, 27 }, { 23, 30 }, { 0, 18 },
    },
};

/*
 * DGROUP 0x34b6..0x34ba. **Which row of points, by form**: a near pointer each.
 */
struct point8 *g_scissors_point_table_flipped[2] = { g_scissors_points_flipped[0], g_scissors_points_flipped[1] };

/*
 * DGROUP 0x34ba..0x34ca. **The scissors' blade**, a segment - its two end points -
 * once as it stands and once mirrored; `part_step_scissors` picks one by the
 * flip bit and `cut_ropes` cuts every rope that crosses it.
 */
struct point16 g_scissors_cut_line[2][2] = {
    { { 22, 15 }, { 39, 15 } },
    { { 0, 15 }, { 16, 15 } },
};

/*
 * 172c:3824, image 0x1aae4 - kind 37's hit test. Closing the scissors.
 *
 * Four of the eight faces set the thing that hit it going, and *which* four
 * depends on the mirror bit of the thing itself, not of the scissors: 1, 2, 4
 * and 5 mirrored, 0, 1, 5 and 6 not. One more face - 7 mirrored, 3 not - sets
 * the *scissors* going instead, and only when they were hit by a kind 4.
 *
 * It answers 1 whatever happened.
 */
uint16_t part_hit_scissors(struct part *part)
{
    struct part *other = part->contact;
    int16_t face = ((int16_t)part->contact_edge);

    if (other->state & STATE_FLIP_HORIZONTAL) {
        if (face == 1 || face == 2 || face == 4 || face == 5)
            other->direction = 1;
        else if (face == 7 && part->kind == KIND_BALLOON)
            part->direction = 1;
    } else {
        if (face == 0 || face == 1 || face == 5 || face == 6)
            other->direction = 1;
        else if (face == 3 && part->kind == KIND_BALLOON)
            part->direction = 1;
    }

    return 1;
}

/*
 * 172c:389b, image 0x1ab5b - a setup.
 *
 * Eight points, reached the way `part_setup_bellow` reaches its six: bit 4 of +8
 * picks between the **pointer arrays** at DGROUP 0x34b6 and 0x3492, and the
 * form at +0x0c indexes the one picked. The load is a word, so what is indexed
 * is an array of near pointers and not the points themselves.
 */
void part_setup_scissors(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->state & STATE_FLIP_HORIZONTAL)
        src = g_scissors_point_table_flipped[part->form];
    else
        src = g_scissors_point_table[part->form];

    for (i = 0, dst = part->points; i < 8; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:38fc, image 0x1abbc - kind 37's step. The scissors.
 *
 * They cut once: only in form 0, and only while +0x12 says go. The line they
 * cut along is one of two in DGROUP - 0x34c2 mirrored, 0x34ba not - and
 * `cut_ropes` does the work. Then the form steps, its own setup runs again
 * because the shape has changed, and sound 0x10 plays.
 */
void part_step_scissors(struct part *part)
{
    if (part->direction != 0 && part->form == 0) {
        cut_ropes(part, (part->state & STATE_FLIP_HORIZONTAL) ? g_scissors_cut_line[1]
                                                : g_scissors_cut_line[0]);

        part->form++;
        part_setup_scissors(part);
        place_object_for_draw(part);
        play_sound(0x10);
    }
}

/*
 * 172c:3944, image 0x1ac04 - kind 37's flip: bit 4, its setup, two marks.
 */
void part_flip_scissors(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    part_setup_scissors(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:3970, image 0x1ac30
 *
 * Cut every rope that crosses a line.
 *
 * The list at DGROUP 0x521b is walked for ropes - kind 0x0a - and each one's
 * lengths in turn: from the part the record names at +2, along the chain of
 * pulleys, to the part at +4. Each length is turned into two points in the
 * scissors' own frame and handed to `intersect_segments`.
 *
 * A cut makes three parts: two kind-0x31 anchors, both at the point the cut
 * fell, and a kind-0x0a to carry the second half of the rope. If any of the
 * three cannot be had the ones already made are given back and nothing is cut -
 * so a machine that runs out of memory keeps its rope whole rather than losing
 * half of it.
 *
 * The rethreading is the fiddly part. The old rope record keeps the near half
 * and ends at the first anchor; the new one takes the second anchor and the old
 * far end, and the pulley the cut length was heading for is pointed at it. A
 * pulley - kind 7 - is repointed through its own +0x68 and +0x5c rather than
 * through the slot the walk was using, because a pulley's two links are not
 * interchangeable.
 *
 * Both anchors get their positions wound into sixteenths, both ropes have their
 * geometry refreshed with the machine forced into state 0x1000, and the walk
 * ends: a rope is only cut once per pass.
 */
void cut_ropes(struct part *part, const struct point16 *line)
{
    struct part *di;                    /* the near anchor */
    int16_t k;                          /* [bp-2] */
    int16_t slotA;                      /* [bp-4] */
    int16_t slotB;                      /* [bp-6] */
    int16_t saved;                      /* [bp-8] */
    struct point16 at;                      /* [bp-0xc] */
    struct point16 seg[2];                     /* [bp-0x14] */
    struct part *rec;                   /* [bp-0x16] */
    struct part *prev;                  /* [bp-0x18] */
    struct part *next;                  /* [bp-0x1a] */
    struct part *anchorB;               /* [bp-0x1c] */
    struct part *carrier;               /* [bp-0x1e] */
    struct part *endA;                  /* [bp-0x20] */
    struct part *endB;                  /* [bp-0x22] */
    struct rope *rope;                  /* [bp-0x24] */
    struct rope *newrope;               /* [bp-0x26] */

    for (rec = g_placed_parts.next; rec != NULL;
         rec = rec->next) {
        if (rec->kind != KIND_ROPE)
            continue;

        rope = rec->rope[0];
        prev = endA = rope->end_a;
        endB = rope->end_b;
        slotA = rope->slot_a;
        slotB = 0;
        next = prev->link[slotA];

        while (prev != NULL && next != NULL) {
            if (prev != endA)
                slotA = 1;

            seg[0].x = prev->box[0].x + prev->attach[slotA].x - part->pos[0].x;
            seg[0].y = prev->box[0].y + prev->attach[slotA].y - part->pos[0].y;

            if (next == endB)
                slotB = rope->slot_b;

            seg[1].x = next->box[0].x + next->attach[slotB].x - part->pos[0].x;
            seg[1].y = next->box[0].y + next->attach[slotB].y - part->pos[0].y;

            if (intersect_segments(line, seg, &at) != 0) {
                saved = g_round_state;
                g_round_state = 0x1000;
                mark_rope_shapes(rope->owner, 3);
                g_round_state = saved;

                if ((di = make_part(KIND_ANCHOR)) == NULL)
                    goto out;
                if ((anchorB = make_part(KIND_ANCHOR)) == NULL)
                    goto fail;
                if ((carrier = make_part(KIND_ROPE)) == NULL) {
                    free_part(anchorB);
fail:
                    free_part(di);
                    goto out;
                }

                insert_sorted(di, &g_moving_parts);
                di->traits |= TRAIT_SPAWNED;
                di->pos[0].x = at.x + part->pos[0].x;
                di->pos[0].y = at.y + part->pos[0].y;

                insert_sorted(anchorB, &g_moving_parts);
                anchorB->traits |= TRAIT_SPAWNED;
                anchorB->pos[0] = di->pos[0];

                insert_sorted(carrier, &g_placed_parts);
                carrier->traits |= TRAIT_SPAWNED;

                newrope = carrier->rope[0];
                newrope->end_a = anchorB;
                newrope->end_b = endB;
                newrope->slot_a = 0;
                newrope->slot_b = rope->slot_b;

                anchorB->link[0] = next;
                anchorB->rope[0] = newrope;

                if (next->kind == KIND_PULLEY) {
                    next->rope[1] = newrope;
                    next->link[1] = anchorB;
                } else {
                    next->rope[slotB] = newrope;
                    next->link[slotB] = anchorB;
                }

                endB->rope[newrope->slot_b] = newrope;

                rope->end_b = di;
                rope->slot_b = 0;
                di->link[0] = prev;
                di->rope[0] = rope;

                if (prev->kind == KIND_PULLEY)
                    prev->link[0] = di;
                else
                    prev->link[slotA] = di;

                di->pos[2].x = di->pos[1].x = di->pos[0].x;
                di->fx = di->pos[0].x;
                di->fx <<= 9;
                di->pos[2].y = di->pos[1].y = di->pos[0].y;
                di->fy = di->pos[0].y;
                di->fy <<= 9;
                place_object_for_draw(di);

                anchorB->pos[2].x = anchorB->pos[1].x = anchorB->pos[0].x;
                anchorB->fx = anchorB->pos[0].x;
                anchorB->fx <<= 9;
                anchorB->pos[2].y = anchorB->pos[1].y = anchorB->pos[0].y;
                anchorB->fy = anchorB->pos[0].y;
                anchorB->fy <<= 9;
                place_object_for_draw(anchorB);

                g_round_state = 0x1000;

                refresh_link_geometry(rope);
                for (k = 0; k < 2; k++)
                    rope->pt[2][k] = rope->pt[1][k] = rope->pt[0][k];

                refresh_link_geometry(newrope);
                for (k = 0; k < 2; k++)
                    newrope->pt[2][k] = newrope->pt[1][k] = newrope->pt[0][k];

                g_round_state = saved;
                prev = next = NULL;
            } else if (next == endB) {
                prev = next = NULL;
            } else {
                prev = next;
                next = next->link[0];
            }
        }
    }

out:
    ;
}
