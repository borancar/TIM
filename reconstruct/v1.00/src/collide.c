/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Collisions**: the do-nothing part hooks, the swept boxes and angles of a
 * moving part and the one it may hit, `resolve_collisions`, and the two edge
 * walks that find where they touch.
 *
 * The first game module of the original's **code segment 0000** (`_TEXT`),
 * image 0x00297..0x00f86, right after C0M's startup; split out of machine.c
 * on 2026-09-27. `step_moving_object` (0x01216) reaches `resolve_collisions`
 * through TLINK's `nop / push cs / call`, so a module ends between them;
 * `step_machine` (0x00f86) calls nothing here, and whether it is this
 * module's last routine or the next one's first is not settled. Its `_DATA`
 * is the quadrant steps at DGROUP 0x258c, just before machine_draw.c's, and
 * its `_BSS` the collision state at 0x53fc. Functions are in address order and
 * each carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 * JUDGE: data 0x258c..0x259c
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The four quadrants' unit steps**, DGROUP 0x258c..0x259c, 0x10 bytes: `dx` is 0, -1, 0, 1
 * and `dy` is -1, 0, 1, 0 in the image, and `find_edge_contact` and its
 * reversed twin index both by the quadrant. Eight words, up to 0x259c.
 */
struct machine_quadrant_steps {
    int16_t   dx[4];              /* +0x00 [8] */
    int16_t   dy[4];              /* +0x08 [8] */
} PACKED;

struct machine_quadrant_steps g_machine_quadrant_steps = { { 0, -1, 0, 1 }, { -1, 0, 1, 0 } };

/*
 * **Whether the moving part's swept box and the other part's box overlap.**
 * The original writes the four comparisons out inline at each of the four
 * places it tests, and **not identically**: three compare strictly and the
 * fourth does not. As macros, which is how the same code four times over is
 * most likely to have been written, and with the difference kept visible.
 * 0x5408..0x540e are the other part's bounds and 0x5410..0x541e the swept
 * box's, filled by `compute_other_bounds` and `compute_swept_bounds`.
 */
#define BOXES_MEET_STRICT                                               \
    (g_collision.other_left < g_collision.swept_right                             \
     && g_collision.other_right > g_collision.swept_left                          \
     && g_collision.other_top < g_collision.swept_bottom                          \
     && g_collision.other_bottom > g_collision.swept_top)
#define BOXES_MEET                                                      \
    (g_collision.other_left <= g_collision.swept_right                            \
     && g_collision.other_right >= g_collision.swept_left                         \
     && g_collision.other_top <= g_collision.swept_bottom                         \
     && g_collision.other_bottom >= g_collision.swept_top)

/* **This module's `_BSS`**: the collision state the routines below share. */
struct collision g_collision;

/*
 * 0x00297
 *
 * A part hook that agrees to everything: it answers 1 and does nothing else.
 * The six routines from here to 0x002b5 are the kind table's do-nothing
 * entries - a kind that has no opinion about one of the hooks points at one of
 * these rather than leaving the slot empty, so every dispatch through the table
 * is a real call.
 */
uint16_t part_hook_yes(struct part *part)
{
    (void)part;
    return 1;
}

/* 0x002a1 */
void part_hook_none_2a1(struct part *part)
{
    (void)part;
}

/* 0x002a6 */
void part_hook_none_2a6(struct part *part)
{
    (void)part;
}

/* 0x002ab */
void part_hook_none_2ab(struct part *part)
{
    (void)part;
}

/* 0x002b0 */
void part_hook_none_2b0(struct part *part)
{
    (void)part;
}

/*
 * 0x002b5
 *
 * The other half of the pair: answers 0.
 */
uint16_t part_hook_no(struct part *part)
{
    (void)part;
    return 0;
}

/*
 * 0x002be
 *
 * Subtract two fields of the structure that DGROUP 0x5400 points at from two
 * words beside it. What the structure is has not been established; only the
 * two fields it touches, at +0x22 and +0x24, and the fact that the pointer is
 * a **** one - a DGROUP offset dereferenced as `[bx + 0x22]`, which is why
 * DGROUP has to be memory rather than a set of named globals.
 *
 * The pointer is re-read from DGROUP for the second field, exactly as here.
 */
void compute_moved(void)
{
    g_collision.moved_x = (int16_t)(g_collision.cur_x - g_collision.list->pos[1].x);
    g_collision.moved_y = (int16_t)(g_collision.cur_y - g_collision.list->pos[1].y);
}

/*
 * 0x002dd
 *
 * Build the **swept** bounding box of the object at DGROUP 0x5400: the union
 * of where it is and where it was, which is what a dirty-rectangle redraw has
 * to repaint.
 *
 * The current box comes from the same fields `compute_other_bounds` uses -
 * +0x1e/+0x20 for the corner and +0x44/+0x46 for the extents - into
 * 0x5420/0x541c and 0x541e/0x541a, with the centre at 0x5418/0x5416. Then
 * `compute_moved` fills 0x5414 and 0x5402 with how far the object has moved, from
 * the previous position at +0x22/+0x24.
 *
 * The box is then stretched both ways: the near edges at 0x5412/0x5410 move
 * back to the old position if that was further back, and the far edges are
 * pushed out by the **absolute** movement - the branchless
 * `cwd / xor / sub` again.
 *
 * As in 0x00386, the pointer at 0x5400 is re-read before every field.
 */
void compute_swept_bounds(void)
{
    g_collision.swept_left = g_collision.cur_x = g_collision.list->pos[0].x;
    g_collision.swept_top = g_collision.cur_y = g_collision.list->pos[0].y;
    g_collision.swept_right = g_collision.cur_x + g_collision.list->size[0].width;
    g_collision.swept_bottom = g_collision.cur_y + g_collision.list->size[0].height;
    g_collision.mid_x = g_collision.cur_x + (g_collision.list->size[0].width >> 1);
    g_collision.mid_y = g_collision.cur_y + (g_collision.list->size[0].height >> 1);

    compute_moved();

    if (g_collision.list->pos[1].x < g_collision.cur_x)
        g_collision.swept_left = g_collision.list->pos[1].x;
    if (g_collision.list->pos[1].y < g_collision.cur_y)
        g_collision.swept_top = g_collision.list->pos[1].y;

    g_collision.swept_right += abs(g_collision.moved_x);
    g_collision.swept_bottom += abs(g_collision.moved_y);
}

/*
 * 0x00386
 *
 * Derive a rectangle and its centre from the structure that DGROUP 0x53fe
 * points at, into six words at DGROUP 0x5404..0x540e:
 *
 *     0x540e = left    = [+0x1e]        0x540a = top     = [+0x20]
 *     0x540c = right   = left + [+0x44] 0x5408 = bottom  = top + [+0x46]
 *     0x5406 = mid x   = left + [+0x44]/2
 *     0x5404 = mid y   = top  + [+0x46]/2
 *
 * so +0x44 and +0x46 are a width and a height. The halving is an **arithmetic**
 * shift, so a negative extent rounds toward negative infinity rather than
 * toward zero - which is not the same as dividing by two in C, and is why it
 * is written as a shift here.
 *
 * The pointer is re-read from DGROUP before every field, six times over. That
 * is what the original does and it is transcribed that way; it matters if
 * anything else can change 0x53fe in between.
 */
void compute_other_bounds(void)
{
    g_collision.other_left = g_collision.other->pos[0].x;
    g_collision.other_top = g_collision.other->pos[0].y;
    g_collision.other_right = (int16_t)(g_collision.other_left + g_collision.other->size[0].width);
    g_collision.other_bottom = (int16_t)(g_collision.other_top + g_collision.other->size[0].height);
    g_collision.other_mid_x = (int16_t)(g_collision.other_left
                             + (int16_t)(g_collision.other->size[0].width >> 1));
    g_collision.other_mid_y = (int16_t)(g_collision.other_top
                             + (int16_t)(g_collision.other->size[0].height >> 1));
}

/*
 * 0x003df
 *
 * Are two angles on the same side of a reference direction?
 *
 * The angle argued about is compared with the one at DGROUP 0x5424, and only
 * when the flag at 0x53fc is set and both fall in the quadrant recorded at
 * 0x5422 - `angle_to_quadrant` decides that.
 *
 * The trick is the rotation: both angles are shifted by 0x2000 (an eighth of a
 * turn) and accepted if both then land in 0..0x4000, and failing that by
 * 0xa000 and tested the same way. That picks whichever half-turn window holds
 * them both, so the comparison that follows can be a plain one against the
 * window's middle at 0x2000. Whichever rotation succeeded is the one the final
 * tests use.
 *
 * Then: equal angles, or either landing exactly on the middle, or both below
 * it, or both above it, all answer yes.
 */
int16_t angles_same_side(int16_t angle)
{
    int16_t quadrant;                   /* [bp-2] */
    int16_t ok;                         /* [bp-4] */
    register int16_t si, di;

    if (g_collision.contact == 0)
        return 0;
    quadrant = angle_to_quadrant(angle);
    if (quadrant != g_collision.contact_quadrant)
        return 0;

    ok = 0;
    si = angle + 0x2000;
    di = g_collision.contact_angle + 0x2000;
    if (si >= 0 && si <= 0x4000 && di >= 0 && di <= 0x4000)
        ok = 1;
    else {
        si = angle + 0xA000;
        di = g_collision.contact_angle + 0xA000;
        if (si >= 0 && si <= 0x4000 && di >= 0 && di <= 0x4000)
            ok = 1;
    }

    if (ok) {
        if (angle == g_collision.contact_angle)
            return 1;
        if (si == 0x2000 || di == 0x2000)
            return 1;
        if (si < 0x2000 && di < 0x2000)
            return 1;
        if (si > 0x2000 && di > 0x2000)
            return 1;
    }
    return 0;
}

/*
 * 0x004ab
 *
 * Answer the angle `atan2_long` gives for two differences taken across an
 * object's +0x1e and +0x22 fields.
 *
 * Both differences are sign-extended to 32 bits before the call, so the caller
 * only ever passes whole longs. The pairing is worth stating because it is not
 * symmetric: the first argument is `+0x22 - +0x1e` and the second is
 * `+0x20 - +0x24`, so one runs one way and the other the opposite.
 *
 * What the four words mean is **not established**. `shift_state_history` ages a
 * 32-bit chain whose generations are at +0x1e and +0x22, which would make +0x1e
 * and +0x20 the halves of one long and +0x22 and +0x24 the halves of the
 * previous one - and this routine reads them as four separate words. Either
 * this is reading the halves deliberately or the chain is not what it appears;
 * nothing here settles which, so the name says only what is computed.
 */
int16_t object_delta_angle(register struct part *obj)
{
    return atan2_long((int16_t)(obj->pos[1].x - obj->pos[0].x),
                      (int16_t)(obj->pos[0].y - obj->pos[1].y));
}

/*
 * 0x004d1
 *
 * Reduce a 16-bit angle to one of four directions. Two exact values are
 * answered directly - 0x2000 gives 0 and 0xa000 gives 2 - and everything else
 * is rotated by an eighth of a turn and shifted down to its top two bits.
 *
 * The shift is **arithmetic** (`sar`), not logical, and then masked to two
 * bits, so the sign makes no difference to the result; it is transcribed as
 * written rather than simplified to a logical shift.
 */
int16_t angle_to_quadrant(register int16_t angle)
{
    if (angle == 0x2000)
        return 0;
    if (angle == (int16_t)0xA000)
        return 2;
    angle += 0x2000;
    angle >>= 14;
    angle &= 3;
    return angle;
}

/*
 * 0x004fd
 *
 * Decide which side of a range a value falls on, and set one of two flag bytes
 * accordingly - or both, when the value is inside the range.
 *
 * `range` is a record whose bounds are at +0 and +4; `out` is a part's
 * contact, whose flags are the bytes at +2 and +3. The containment test is `value_between` at
 * 0x03d67, which handles either ordering, so the side test below has to handle
 * both orderings too - and it does, by asking which bound is the lower one
 * first. All four compares here are **signed**.
 */
void set_side_flags(register const int16_t *range, int16_t v,
                    register struct part_contact *out)
{
    if (value_between(v, range[0], range[2])) {
        out->no_nudge_plus = 1;
        out->no_nudge_minus = 1;
    } else if (range[0] < range[2]) {
        if (range[0] > v)
            out->no_nudge_minus = 1;
        else
            out->no_nudge_plus = 1;
    } else {
        if (range[2] > v)
            out->no_nudge_plus = 1;
        else
            out->no_nudge_minus = 1;
    }
}

/*
 * 0x00556
 *
 * Resolve one object against everything it could be touching, and answer
 * whether anything was.
 *
 * The object goes into the global at 0x5400 - the two sweeps read it from
 * there rather than taking it as an argument - and an object with no edge list
 * at +0x82 is answered 0 immediately.
 *
 * Its existing contact, the link at +0x84, is retried first: that object's
 * angle is remembered at 0x5424 with its quadrant at 0x5422, and the two side
 * bytes at +2 and +3 of the link are cleared so the sweeps can set them afresh.
 * `chain_contains` rejects a partner already reachable through the chain, which
 * is what stops a contact being resolved twice from both ends.
 *
 * Then every object the 0x3000/0x1000 walk reaches is tried the same way,
 * skipping the object itself, the one already handled, anything without edges
 * or carrying bit 0x2000 at +8, and one specific pairing - type 0xc against
 * type 0x2a - that is excluded by name.
 *
 * Each candidate gets both sweeps, `find_edge_contact` and then
 * `find_edge_contact_reversed`, each behind its own box test. **The box tests
 * are not all the same**: the one before the reversed sweep inside the walk
 * compares non-strictly where the other three are strict, so a pair whose boxes
 * exactly abut is swept one way and not the other. After any hit the angle at
 * 0x5426 is recomputed, because the object has moved.
 *
 * With nothing found the link's first word is cleared. With something found,
 * bit 0 at +6 is set if `angles_same_side` agrees about the link's angle - the
 * flag `integrate_object` reads to decide whether gravity applies.
 */
int16_t resolve_collisions(struct part *obj)
{
    register struct part_contact *c;
    register int16_t hit;

    g_collision.list = obj;
    if (g_collision.list->points == 0)
        return 0;

    c = (struct part_contact *)&g_collision.list->contact;
    if ((g_collision.contact = g_collision.list->contact) != 0) {
        g_collision.contact_angle = c->angle;
        g_collision.contact_quadrant = angle_to_quadrant(g_collision.contact_angle);
    }
    c->no_nudge_plus = c->no_nudge_minus = 0;

    g_collision.travel_angle = object_delta_angle(g_collision.list);
    compute_swept_bounds();

    hit = 0;
    if (g_collision.contact != 0
        && !chain_contains(g_collision.list, g_collision.contact)) {
        g_collision.other = g_collision.contact;
        if (g_collision.other->points != 0
            && !(g_collision.other->state & STATE_GONE)) {
            compute_other_bounds();

            if (BOXES_MEET_STRICT && find_edge_contact(0)) {
                hit = 1;
                g_collision.travel_angle = object_delta_angle(g_collision.list);
            }
            if (BOXES_MEET_STRICT && find_edge_contact_reversed(0)) {
                hit = 1;
                g_collision.travel_angle = object_delta_angle(g_collision.list);
            }
        }
    }

    for (g_collision.other = (pick_by_flag((TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST)));
         g_collision.other != 0;
         g_collision.other = (pick_for_record(g_collision.other,
                                                    TRAIT_IN_MOVING_LIST))) {
        if (!chain_contains(g_collision.list, g_collision.other)
            && g_collision.list != g_collision.other
            && g_collision.contact != g_collision.other
            && g_collision.other->points != 0
            && !(g_collision.other->state & STATE_GONE)
            && !(g_collision.list->kind == 0xc
                 && g_collision.other->kind == 0x2a)) {
            compute_other_bounds();

            if (BOXES_MEET_STRICT && find_edge_contact(0)) {
                hit = 1;
                g_collision.travel_angle = object_delta_angle(g_collision.list);
            }
            if (BOXES_MEET && find_edge_contact_reversed(0)) {
                hit = 1;
                g_collision.travel_angle = object_delta_angle(g_collision.list);
            }
        }
    }

    if (!hit)
        c->part = 0;
    else if (angles_same_side(c->angle))
        g_collision.list->traits |= TRAIT_ON_SURFACE;

    return hit;
}

/*
 * 0x007af
 *
 * Sweep one object's edges against another's and record the first contact.
 *
 * Two edge lists are walked, the outer one belonging to the object at DGROUP
 * 0x53fe and the inner to the one at 0x5400. An edge is four bytes - two
 * unsigned byte coordinates, then a word angle at +2 - and each list wraps: the
 * last edge's far end is the first edge's near end, which is why the first
 * vertex is kept aside at the top and restored on the final pass.
 *
 * The angle at +2 is what prunes the work. An edge pair is only considered when
 * the two angles lie on the right sides of the outer edge's, tested by adding
 * 0x8000 and looking at the sign - a half-turn rotation that turns "within this
 * arc" into "non-negative". The value 0x8000 itself is admitted explicitly,
 * because negating it does not change it and it would otherwise read as
 * negative. With no motion at all - both 0x5414 and 0x5402 zero - nothing can
 * touch and the pair is skipped.
 *
 * The test itself is two segments. The first runs from the inner edge's start,
 * expressed relative to the outer edge's start, to that point plus the motion;
 * the second runs from the origin along the outer edge. `step_pair_apart`
 * adjusts the second before `intersect_segments` is asked. An intersection
 * exactly at the second segment's far end does not count - that is the shared
 * vertex with the next edge, and counting it would report every corner twice.
 *
 * With a non-zero argument the routine stops at the first contact and answers
 * 1, which is how a caller asks "would this move touch anything" without
 * disturbing either object.
 *
 * Otherwise the contact is resolved. The outer edge's quadrant, from
 * `angle_to_quadrant`, indexes the two nudge tables at DGROUP 0x258c and 0x2594
 * which shift the second segment off the surface, and then `angles_same_side`
 * chooses between two ways of placing the object:
 *
 *   - not the same side: intersect again and move the object by the difference
 *     between the new crossing and the old, or - if the nudged segments no
 *     longer meet at all - snap it back to +0x22/+0x24.
 *   - the same side: solve the outer edge's line for the y at the object's own
 *     x and correct only the vertical position. This is the one place the
 *     routine divides, and it divides by the negated run, so a horizontal edge
 *     would divide by zero; that case is the one routed to the snap-back.
 *
 * Either way the object is re-placed, its swept bounds recomputed, and its
 * flags at +6 rewritten: bits 1 and 2 cleared, then bit 1 set when either
 * object's +8 has the top bit or the outer object's +6 has 0x4000, and bit 2
 * set otherwise. The contact is written into the link at +0x84 - the other
 * object, the angle, and the edge index - and `set_side_flags` finishes it.
 */
int16_t find_edge_contact(int16_t test_only)
{
    int16_t hit;                        /* [bp-2] */
    int16_t same;                       /* [bp-4] */
    int16_t i;                          /* [bp-6] */
    int16_t j;                          /* [bp-8] */
    int16_t a_ang;                      /* [bp-0xa] */
    int16_t b_ang;                      /* [bp-0xc] */
    int16_t d;                          /* [bp-0xe] */
    int16_t x0;                         /* [bp-0x10] */
    int16_t x1;                         /* [bp-0x12] */
    int16_t fx0;                        /* [bp-0x14] */
    int16_t y0;                         /* [bp-0x16] */
    int16_t y1;                         /* [bp-0x18] */
    int16_t fy0;                        /* [bp-0x1a] */
    int16_t tx;                         /* [bp-0x1c] */
    int16_t ty;                         /* [bp-0x1e] */
    int16_t quad;                       /* [bp-0x20] */
    int16_t p;                          /* [bp-0x22] */
    int16_t q;                          /* [bp-0x24] */
    int16_t r;                          /* [bp-0x26] */
    int16_t c;                          /* [bp-0x28] */
    int16_t run;                        /* [bp-0x2a] */
    struct part_contact *cp;            /* [bp-0x2c] */
    int16_t seg2[4];                    /* [bp-0x34] */
    int16_t seg1[4];                    /* [bp-0x3c] */
    int16_t out[2];                     /* [bp-0x40] */
    register struct part_point *si;
    register struct part_point *di;

    hit = 0;
    i = 1;
    si = g_collision.other->points;
    fx0 = x0 = g_collision.other_left + si[0].x;
    fy0 = y0 = g_collision.other_top + si[0].y;
    x1 = g_collision.other_left + si[1].x;
    y1 = g_collision.other_top + si[1].y;
    a_ang = si[0].angle;

    while (si) {
        quad = angle_to_quadrant(a_ang);
        d = g_collision.travel_angle - a_ang + 0x4000;
        if (d > 0) {
            di = g_collision.list->points;
            b_ang = di->angle;
            di++;
            j = 1;

            while (di) {
                d = b_ang - a_ang + 0x8000;
                if (d >= 0 || d == (int16_t)0x8000) {
                    d = di->angle - a_ang + 0x8000;
                    if (d <= 0 && (g_collision.moved_x || g_collision.moved_y)) {
                        seg1[0] = g_collision.list->pos[1].x
                                  + di->x - x0;
                        seg1[1] = g_collision.list->pos[1].y
                                  + di->y - y0;
                        tx = seg1[2] = seg1[0] + g_collision.moved_x;
                        ty = seg1[3] = seg1[1] + g_collision.moved_y;

                        seg2[0] = 0;
                        seg2[1] = 0;
                        seg2[2] = x1 - x0;
                        seg2[3] = y1 - y0;
                        step_pair_apart(seg2);

                        if (intersect_segments(seg1, seg2, (uint8_t *)out)
                            && !(out[1] == seg2[3] && out[0] == seg2[2])) {
                            if (test_only)
                                return 1;

                            seg2[0] = g_machine_quadrant_steps.dx[quad];
                            seg2[1] = g_machine_quadrant_steps.dy[quad];
                            seg2[2] += g_machine_quadrant_steps.dx[quad];
                            seg2[3] += g_machine_quadrant_steps.dy[quad];

                            same = angles_same_side(a_ang);
                            if (same == 0) {
                                if (!intersect_segments(seg1, seg2, (uint8_t *)out)) {
                                    g_collision.list->pos[0].x =
                                        g_collision.list->pos[1].x;
                                    g_collision.list->pos[0].y =
                                        g_collision.list->pos[1].y;
                                } else {
                                    g_collision.list->pos[0].x += out[0] - tx;
                                    g_collision.list->pos[0].y += out[1] - ty;
                                }
                            } else {
                                p = seg1[2];
                                q = seg2[3] - seg2[1];
                                r = seg2[2] - seg2[0];
                                c = q * seg2[0] - r * seg2[1];
                                run = 0 - r;
                                if (run != 0) {
                                    out[1] = (int16_t)(c - (int16_t)(q * p)) / run;
                                    g_collision.list->pos[0].y += out[1] - ty;
                                } else {
                                    g_collision.list->pos[0].x =
                                        g_collision.list->pos[1].x;
                                    g_collision.list->pos[0].y =
                                        g_collision.list->pos[1].y;
                                }
                            }

                            place_object_for_draw(g_collision.list);
                            compute_swept_bounds();

                            g_collision.list->traits &= ~(TRAIT_HIT_FIXED | TRAIT_HIT_MOVING);
                            if ((g_collision.list->state
                                 | g_collision.other->state) & STATE_SOLID
                                || g_collision.other->traits & TRAIT_STATIC)
                                g_collision.list->traits |= TRAIT_HIT_FIXED;
                            else
                                g_collision.list->traits |= TRAIT_HIT_MOVING;

                            cp = (struct part_contact *)
                                 &g_collision.list->contact;
                            cp->part = g_collision.other;
                            cp->angle = a_ang;
                            cp->edge = i - 1;
                            set_side_flags(seg2, g_collision.mid_x - x0, cp);
                            hit = 1;
                        }
                    }
                }

                j++;
                if ((int16_t)g_collision.list->point_count < j)
                    di = 0;
                else {
                    b_ang = di->angle;
                    if ((int16_t)g_collision.list->point_count == j)
                        di = g_collision.list->points;
                    else
                        di++;
                }
            }
        }

        i++;
        if ((int16_t)g_collision.other->point_count < i)
            si = 0;
        else {
            si++;
            x0 = x1;
            y0 = y1;
            a_ang = si[0].angle;
            if ((int16_t)g_collision.other->point_count == i) {
                x1 = fx0;
                y1 = fy0;
            } else {
                x1 = g_collision.other_left + si[1].x;
                y1 = g_collision.other_top + si[1].y;
            }
        }
    }
    return hit;
}

/*
 * 0x00b6c
 *
 * The other half of the sweep in 0x007af: the same contact search with the two
 * objects exchanged, so the object at DGROUP 0x5400 is the one being moved
 * against the edges of the one at 0x53fe.
 *
 * It is a near-mirror and the differences are all deliberate:
 *
 *   - every angle is taken half a turn round, `+ 0x8000`, before it is used;
 *   - the outer list is 0x5400's and the inner 0x53fe's, the reverse of before,
 *     and the coordinates come from 0x5420/0x541c rather than 0x540e/0x540a;
 *   - the first segment runs from the *moved* point back to the original, not
 *     out from the original;
 *   - the two nudge tables at 0x258c and 0x2594 are **negated**;
 *   - the corrections **subtract** where the other routine adds.
 *
 * Every one of those follows from the swap. Transcribing this by copying the
 * other routine and changing what looked different would be exactly the wrong
 * method here, because several of the differences are a sign hidden inside an
 * addressing mode.
 *
 * The contact is recorded differently too. Rather than calling
 * `set_side_flags`, this writes the link at +0x84 itself and then sets one of
 * the two bytes at +0x86 and +0x87 directly, choosing between them on two
 * comparisons: whether the outer edge runs right-to-left, and whether the
 * crossing falls beyond `0x5418 - x0`. The two questions swap which byte is
 * set, which is how a contact on the far side of an edge is told from one on
 * the near side.
 *
 * Two locals are written from the inner edge's bytes and never read again.
 * They are on the stack, so nothing can observe them, and they are left out.
 */
int16_t find_edge_contact_reversed(int16_t test_only)
{
    int16_t hit;                        /* [bp-2] */
    int16_t same;                       /* [bp-4] */
    int16_t i;                          /* [bp-6] */
    int16_t j;                          /* [bp-8] */
    int16_t a_ang;                      /* [bp-0xa] */
    int16_t b_ang;                      /* [bp-0xc] */
    int16_t d;                          /* [bp-0xe] */
    int16_t x0;                         /* [bp-0x10] */
    int16_t x1;                         /* [bp-0x12] */
    int16_t fx0;                        /* [bp-0x14] */
    int16_t y0;                         /* [bp-0x16] */
    int16_t y1;                         /* [bp-0x18] */
    int16_t fy0;                        /* [bp-0x1a] */
    int16_t sx;                         /* [bp-0x1c] */
    /* The point's own offsets, stored and never read again. */
    int16_t px;                         /* [bp-0x1e] */
    int16_t sy;                         /* [bp-0x20] */
    int16_t py;                         /* [bp-0x22] */
    int16_t v;                          /* [bp-0x24] */
    int16_t quad;                       /* [bp-0x26] */
    int16_t p;                          /* [bp-0x28] */
    int16_t q;                          /* [bp-0x2a] */
    int16_t r;                          /* [bp-0x2c] */
    int16_t c;                          /* [bp-0x2e] */
    int16_t run;                        /* [bp-0x30] */
    int16_t seg2[4];                    /* [bp-0x38] */
    int16_t seg1[4];                    /* [bp-0x40] */
    int16_t out[2];                     /* [bp-0x44] */
    register struct part_point *di;
    register struct part_point *si;

    hit = 0;
    i = 1;
    di = g_collision.list->points;
    fx0 = x0 = g_collision.cur_x + di[0].x;
    fy0 = y0 = g_collision.cur_y + di[0].y;
    x1 = g_collision.cur_x + di[1].x;
    y1 = g_collision.cur_y + di[1].y;
    a_ang = di[0].angle;

    while (di) {
        quad = angle_to_quadrant(a_ang + 0x8000);
        d = g_collision.travel_angle + 0x8000 - a_ang + 0x4000;
        if (d > 0) {
            si = g_collision.other->points;
            b_ang = si->angle;
            si++;
            j = 1;

            while (si) {
                d = b_ang - a_ang + 0x8000;
                if (d >= 0 || d == (int16_t)0x8000) {
                    d = si->angle - a_ang + 0x8000;
                    if (d <= 0 && (g_collision.moved_x || g_collision.moved_y)) {
                        px = si->x;
                        py = si->y;
                        sx = seg1[2] = g_collision.other->pos[0].x
                                       + si->x - x0;
                        sy = seg1[3] = g_collision.other->pos[0].y
                                       + si->y - y0;
                        seg1[0] = seg1[2] + g_collision.moved_x;
                        seg1[1] = seg1[3] + g_collision.moved_y;
#ifndef __TURBOC__
                        (void)px;   /* the dead stores above are the original's */
                        (void)py;
#endif

                        seg2[0] = 0;
                        seg2[1] = 0;
                        seg2[2] = x1 - x0;
                        seg2[3] = y1 - y0;
                        step_pair_apart(seg2);

                        if (intersect_segments(seg1, seg2, (uint8_t *)out)
                            && !(out[1] == seg2[3] && out[0] == seg2[2])) {
                            if (test_only)
                                return 1;

                            seg2[0] = 0 - g_machine_quadrant_steps.dx[quad];
                            seg2[1] = 0 - g_machine_quadrant_steps.dy[quad];
                            seg2[2] += 0 - g_machine_quadrant_steps.dx[quad];
                            seg2[3] += 0 - g_machine_quadrant_steps.dy[quad];

                            same = angles_same_side(a_ang + 0x8000);
                            if (same == 0) {
                                if (!intersect_segments(seg1, seg2, (uint8_t *)out)) {
                                    g_collision.list->pos[0].x =
                                        g_collision.list->pos[1].x;
                                    g_collision.list->pos[0].y =
                                        g_collision.list->pos[1].y;
                                } else {
                                    g_collision.list->pos[0].x -= out[0] - sx;
                                    g_collision.list->pos[0].y -= out[1] - sy;
                                }
                            } else {
                                p = seg1[2];
                                q = seg2[3] - seg2[1];
                                r = seg2[2] - seg2[0];
                                c = q * seg2[0] - r * seg2[1];
                                run = 0 - r;
                                if (run != 0) {
                                    out[1] = (int16_t)(c - (int16_t)(q * p)) / run;
                                    g_collision.list->pos[0].y -= out[1] - sy;
                                } else {
                                    g_collision.list->pos[0].x =
                                        g_collision.list->pos[1].x;
                                    g_collision.list->pos[0].y =
                                        g_collision.list->pos[1].y;
                                }
                            }

                            v = g_collision.mid_x - x0;

                            place_object_for_draw(g_collision.list);
                            compute_swept_bounds();

                            g_collision.list->traits &= ~(TRAIT_HIT_FIXED | TRAIT_HIT_MOVING);
                            if ((g_collision.list->state
                                 | g_collision.other->state) & STATE_SOLID
                                || g_collision.other->traits & TRAIT_STATIC)
                                g_collision.list->traits |= TRAIT_HIT_FIXED;
                            else
                                g_collision.list->traits |= TRAIT_HIT_MOVING;

                            g_collision.list->contact = g_collision.other;
                            g_collision.list->contact_angle = a_ang + 0x8000;

                            if (x0 > x1) {
                                if (v > out[0])
                                    g_collision.list->no_nudge_plus = 1;
                                else
                                    g_collision.list->no_nudge_minus = 1;
                            } else {
                                if (v > out[0])
                                    g_collision.list->no_nudge_minus = 1;
                                else
                                    g_collision.list->no_nudge_plus = 1;
                            }

                            g_collision.list->contact_edge = j - 1;
                            hit = 1;
                        }
                    }
                }

                j++;
                if ((int16_t)g_collision.other->point_count < j)
                    si = 0;
                else {
                    b_ang = si->angle;
                    if ((int16_t)g_collision.other->point_count == j)
                        si = g_collision.other->points;
                    else
                        si++;
                }
            }
        }

        i++;
        if ((int16_t)g_collision.list->point_count < i)
            di = 0;
        else {
            di++;
            x0 = x1;
            y0 = y1;
            a_ang = di[0].angle;
            if ((int16_t)g_collision.list->point_count == i) {
                x1 = fx0;
                y1 = fy0;
            } else {
                x1 = g_collision.cur_x + di[1].x;
                y1 = g_collision.cur_y + di[1].y;
            }
        }
    }
    return hit;
}
