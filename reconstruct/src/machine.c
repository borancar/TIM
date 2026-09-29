/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The machine**: the geometry of parts and their outlines, what is under
 * the pointer and which cursor shows, ropes, belts and pulleys and the
 * tension between their ends, taking parts out and filing them, the lists
 * the machine is walked by, the shapes that are redrawn, the history each
 * part keeps, and `reset_machine`.
 *
 * One module of the original's **code segment 0000** (`_TEXT`), image
 * 0x03b17..0x080b9, and one translation unit: from 0x03ba9 to 0x080a5 its
 * routines reach each other with the bare `push cs / call` Borland writes
 * only for a routine defined earlier in the same file, which chains every
 * one of them together (`part_flip_options` calls `object_overlaps_any`,
 * `reset_machine` calls `pick_by_flag`). Its `_DATA` is 0x284a..0x286e, the
 * cursors' hot spots, which TLINK put after goals.c's. **Both ends are
 * ours**: `rotate_point` at 0x03b17 is in by subject, and so is the end at
 * 0x080b9, where nothing reaches back across 0x080b9..0x08136.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 * JUDGE: data 0x284a..0x286e
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * The DGROUP records only this file reads or writes. Each is laid over the
 * DGROUP byte array at the address its macro names, like the shared ones in
 * dgroup.h; they are declared here because nothing else uses them.
 */

/*
 * **The cursors' hot spots**, DGROUP 0x284a..0x286e, 0x24 bytes: x for the nine
 * cursors, then y. `select_cursor` reads both by cursor number and the run ends
 * at 0x286e.
 *
 * **Which half is which was measured, because it had been written down the
 * other way round.** `draw_cursor` draws the bitmap at `cursor_x - 0x5780,
 * cursor_y - 0x577e` and `set_cursor` files its first argument - this first
 * table - in 0x5780. Broken on `set_cursor` with the hourglass up: the bitmap
 * is 16 by 20 and the pair is 8 and 10, which is its centre only if the first
 * table is x. The eight tool cursors have x 0..7 here and y 0 below, a hot
 * spot on the top edge.
 */
struct machine_cursor_hotspots {
    int16_t   hot_x[9];           /* +0x00 [0x12] */
    int16_t   hot_y[9];           /* +0x12 [0x12] */
} PACKED;

struct machine_cursor_hotspots MACHINE_CURSOR_HOTSPOTS DGROUP_AT(0x284a) = {
    {
        0x0000, 0x0008, 0x0004, 0x0005, 0x0006, 0x0003, 0x0007, 0x0000,
        0x0003,
    },
    { 0x0000, 0x000a },
};

/*
 * 0x03b17
 *
 * Rotate a point about the origin, in place. Both coordinates are **near
 * pointers** into DGROUP, and the angle is the 16-bit one the cosine table is
 * built for.
 *
 *     x' = (x*cos - y*sin) >> 14
 *     y' = (x*sin + y*cos) >> 14
 *
 * The shift is 14 because the table holds 16384 for 1, so the products come
 * back scaled by 16384 and the shift is the divide. Each product is a full
 * 32-bit `mul16x16` and the sum and difference are done in 32 bits with
 * `sub`/`sbb` and `add`/`adc`, so nothing is truncated before the shift - and
 * the shift itself is arithmetic, through the runtime helper at 0x0be5f.
 *
 * Both new values are computed before either is stored, so the second uses the
 * **old** x. Storing x first would change y, and that is exactly the kind of
 * thing a rewrite gets wrong.
 */
void rotate_point(int16_t *px, int16_t *py, uint16_t angle)
{
    int16_t c = angle_cos(angle);
    int16_t s = angle_sin(angle);
    int32_t nx, ny;

    nx = mul16x16(px[0], c) - mul16x16(py[0], s);
    ny = mul16x16(px[0], s) + mul16x16(py[0], c);

    px[0] = (int16_t)(nx >> 14);
    py[0] = (int16_t)(ny >> 14);
}

/*
 * 0x03ba9
 *
 * Intersect two line segments, store the point, and answer whether it lies on
 * both of them.
 *
 * Each segment is four words - two points - and is turned into the usual
 * `a*x + b*y = c` form. **The two are built with opposite sign conventions**:
 * the first takes `y1-y2` and `x1-x2`, the second `y2-y1` and `x2-x1`. That is
 * not a slip; it is what makes the determinant below come out with the sign the
 * division wants, and transcribing it either way round consistently would be
 * wrong.
 *
 * The three constants are computed with the one-operand `imul`, which produces
 * a 32-bit product in DX:AX - and only **AX is kept**. So they are truncated to
 * sixteen bits and can wrap. The numerators are not: those go through
 * `mul16x16` and are divided in 32 bits.
 *
 * Parallel segments - a zero determinant - fall to a different test: if the
 * first segment's start also satisfies the second's equation the two are the
 * same line and the answer is that segment's far end, otherwise the origin.
 *
 * Finally the point has to lie within both segments in both axes, which is four
 * `value_between` calls, and any one of them failing answers 0.
 */
int16_t intersect_segments(register const int16_t *seg1,
                           register const int16_t *seg2, uint8_t *out)
{
    int16_t a1;                         /* [bp-2] */
    int16_t b1;                         /* [bp-4] */
    int16_t c1;                         /* [bp-6] */
    int16_t a2;                         /* [bp-8] */
    int16_t b2;                         /* [bp-0xa] */
    int16_t c2;                         /* [bp-0xc] */
    int16_t denom;                      /* [bp-0xe] */
    int16_t x;                          /* [bp-0x10] */
    int16_t y;                          /* [bp-0x12] */
    int32_t n;                          /* [bp-0x16] */

    a1 = seg1[1] - seg1[3];
    b1 = seg1[0] - seg1[2];
    c1 = (int16_t)(seg1[2] * a1) - (int16_t)(seg1[3] * b1);
    a2 = seg2[3] - seg2[1];
    b2 = seg2[2] - seg2[0];
    c2 = (int16_t)(seg2[0] * a2) - (int16_t)(seg2[1] * b2);
    denom = (int16_t)(a2 * b1) - (int16_t)(a1 * b2);

    if (denom != 0) {
        n = mul16x16(c2, b1) - mul16x16(c1, b2);
        x = n / denom;
        n = mul16x16(a1, c2) - mul16x16(a2, c1);
        y = n / denom;
    } else if ((int16_t)((int16_t)(seg1[0] * a2) + (int16_t)(seg1[1] * b2))) {
        x = 0;
        y = 0;
    } else {
        x = seg1[2];
        y = seg1[3];
    }

    ((int16_t *)out)[0] = x;
    ((int16_t *)out)[1] = y;

    if (!value_between(x, seg1[0], seg1[2]))
        return 0;
    if (!value_between(x, seg2[0], seg2[2]))
        return 0;
    if (!value_between(y, seg1[1], seg1[3]))
        return 0;
    if (!value_between(y, seg2[1], seg2[3]))
        return 0;
    return 1;
}

/*
 * 0x03d2e
 *
 * Step the second word of each pair in a four-word record one further from the
 * first: if `[+4]` is above `[+0]` it goes up, if below it goes down, and if
 * equal it is left alone. The same for `[+6]` against `[+2]`.
 *
 * The two words are compared by subtraction and the *difference* tested, not
 * the values, so this is transcribed as a difference rather than as a compare.
 * What the record is has not been established - a pair of coordinates and a
 * pair of limits would fit, but that is inference.
 */
void step_pair_apart(register int16_t *rec)
{
    int16_t d;

    d = rec[2] - rec[0];
    if (d > 0)
        rec[2]++;
    else if (d < 0)
        rec[2]--;

    d = rec[3] - rec[1];
    if (d > 0)
        rec[3]++;
    else if (d < 0)
        rec[3]--;
}

/*
 * 0x03d67
 *
 * Is `v` between `a` and `b`, whichever way round they are?
 *
 * The bounds are ordered with a **signed** compare, and the containment test
 * is then the unsigned-difference trick: `v - lo <= hi - lo` is true exactly
 * when v lies in the range, and false by wrapping when it is below `lo`. The
 * two compares are of different signedness in the original and are transcribed
 * that way.
 */
int16_t value_between(uint16_t v, uint16_t a, uint16_t b)
{
    if ((int16_t)a > (int16_t)b)
        return (uint16_t)(v - b) <= (uint16_t)(a - b) ? 1 : 0;
    return (uint16_t)(v - a) <= (uint16_t)(b - a) ? 1 : 0;
}

/*
 * 0x03da5
 *
 * The angle from one object's middle to another's, in the whole-turn-is-0x10000
 * space `atan2_long` works in.
 *
 * Both middles are the position at +0x1e/+0x20 plus half the extent at
 * +0x44/+0x46. The vertical difference is taken the other way round from the
 * horizontal - `b` minus `a` down, `a` minus `b` across - which is what turns
 * a screen's y-down into the angle's y-up.
 */
int16_t angle_between_centres(register struct part *a, register struct part *b)
{
    int16_t acx;                        /* [bp-2] */
    int16_t acy;                        /* [bp-4] */
    int16_t bcx;                        /* [bp-6] */
    int16_t bcy;                        /* [bp-8] */
    int16_t angle;                      /* [bp-0xa] */
    int32_t dx;                         /* [bp-0xe] */
    int32_t dy;                         /* [bp-0x12] */

    acx = a->pos[0].x + (a->size[0].width >> 1);
    acy = a->pos[0].y + (a->size[0].height >> 1);
    bcx = b->pos[0].x + (b->size[0].width >> 1);
    bcy = b->pos[0].y + (b->size[0].height >> 1);
    dx = (int16_t)(acx - bcx);
    dy = (int16_t)(bcy - acy);
    angle = atan2_long(dx, dy);
    return angle;
}

/*
 * 0x03e23
 *
 * Is an object overlapping anything else on the 0x3000 list?
 *
 * Two parts that are a kind 0x0c and a kind 0x2a in either order never count -
 * those two are meant to pass through each other - and neither does the object
 * itself or anything hidden.
 *
 * With bit 14 of +6 set on *both*, the boxes at +0x50 are enough. Otherwise the
 * boxes at +0x44 have to overlap first and then the outlines are tested
 * properly by `outlines_cross`, so a wide part with a thin shape does not stop
 * something passing through the gap.
 */
int16_t object_overlaps_any(register struct part *obj)
{
    register struct part *si;
    int16_t x0;                         /* [bp-2] */
    int16_t y0;                         /* [bp-4] */
    int16_t x1;                         /* [bp-6] */
    int16_t y1;                         /* [bp-8] */
    int16_t x2;                         /* [bp-0xa] */
    int16_t y2;                         /* [bp-0xc] */
    int16_t sx0;                        /* [bp-0xe] */
    int16_t sy0;                        /* [bp-0x10] */
    int16_t sx1;                        /* [bp-0x12] */
    int16_t sy1;                        /* [bp-0x14] */
    int16_t sx2;                        /* [bp-0x16] */
    int16_t sy2;                        /* [bp-0x18] */

    x0 = obj->pos[0].x;
    y0 = obj->pos[0].y;
    x1 = x0 + obj->set_size.width;
    y1 = y0 + obj->set_size.height;
    x2 = x0 + obj->size[0].width;
    y2 = y0 + obj->size[0].height;

    for (si = pick_by_flag(0x3000); si != PART_NONE;
         si = pick_for_record(si, 0x1000)) {
        if (obj->kind == KIND_POKEY && si->kind == KIND_MORT_THE_MOUSE)
            continue;
        if (si->kind == KIND_POKEY && obj->kind == KIND_MORT_THE_MOUSE)
            continue;
        if (si == obj)
            continue;
        if (si->flags_08 & 0x2000)
            continue;

        sx0 = si->pos[0].x;
        sy0 = si->pos[0].y;
        sx1 = sx0 + si->set_size.width;
        sy1 = sy0 + si->set_size.height;
        sx2 = sx0 + si->size[0].width;
        sy2 = sy0 + si->size[0].height;

        if (!(obj->flags_06 & 0x4000) || !(si->flags_06 & 0x4000)) {
            if (sx0 < x2 && sx2 > x0 && sy0 < y2 && sy2 > y0
                && outlines_cross(obj, si))
                return 1;
        } else if (sx0 < x1 && sx1 > x0 && sy0 < y1 && sy1 > y0)
            return 1;
    }
    return 0;
}

/*
 * 0x03f4d
 *
 * Do two parts' outlines actually cross?
 *
 * Each outline is the array of points at +0x82, `[0x80]` of them, two bytes to
 * a point and taken as offsets from the part's own position; the last wraps
 * back to the first. Every segment of the first is tested against every segment
 * of the second, both moved into the first segment's own frame so the
 * arithmetic stays small, and `step_pair_apart` nudges each pair before the
 * test.
 *
 * A crossing *at the far end of the first segment* does not count - that is the
 * corner two neighbouring segments share, and counting it would make every
 * outline cross itself. Anything else answers 1 at once.
 */
int16_t outlines_cross(struct part *a, struct part *b)
{
    register struct part_point *pa;
    register struct part_point *pb;
    int16_t i;                          /* [bp-2] */
    int16_t j;                          /* [bp-4] */
    int16_t ax1;                        /* [bp-6] */
    int16_t ax2;                        /* [bp-8] */
    int16_t fax;                        /* [bp-0xa] */
    int16_t ay1;                        /* [bp-0xc] */
    int16_t ay2;                        /* [bp-0xe] */
    int16_t fay;                        /* [bp-0x10] */
    int16_t ax0;                        /* [bp-0x12] */
    int16_t ay0;                        /* [bp-0x14] */
    int16_t bx1;                        /* [bp-0x16] */
    int16_t bx2;                        /* [bp-0x18] */
    int16_t fbx;                        /* [bp-0x1a] */
    int16_t by1;                        /* [bp-0x1c] */
    int16_t by2;                        /* [bp-0x1e] */
    int16_t fby;                        /* [bp-0x20] */
    int16_t bx0;                        /* [bp-0x22] */
    int16_t by0;                        /* [bp-0x24] */
    int16_t segA[4];                    /* [bp-0x2c] */
    int16_t segB[4];                    /* [bp-0x34] */
    int16_t out[2];                     /* [bp-0x38] */

#ifndef __TURBOC__
    /* Ours: the second outline's points are read only when it has any, which
       gcc cannot see. The original leaves them as the stack had them. */
    bx1 = bx2 = by1 = by2 = 0;
#endif
    ax0 = a->pos[0].x;
    ay0 = a->pos[0].y;
    bx0 = b->pos[0].x;
    by0 = b->pos[0].y;

    i = 1;
    if ((pa = POINTS(a->points_ptr)) != POINTS(0)) {
        fax = ax1 = ax0 + pa[0].x;
        fay = ay1 = ay0 + pa[0].y;
        ax2 = ax0 + pa[1].x;
        ay2 = ay0 + pa[1].y;
    }

    while (pa != POINTS(0)) {
        segA[0] = ax1 - ax1;
        segA[1] = ay1 - ay1;
        segA[2] = ax2 - ax1;
        segA[3] = ay2 - ay1;
        step_pair_apart(segA);

        j = 1;
        if ((pb = POINTS(b->points_ptr)) != POINTS(0)) {
            fbx = bx1 = bx0 + pb[0].x;
            fby = by1 = by0 + pb[0].y;
            bx2 = bx0 + pb[1].x;
            by2 = by0 + pb[1].y;
        }

        while (pb != POINTS(0)) {
            segB[0] = bx1 - ax1;
            segB[1] = by1 - ay1;
            segB[2] = bx2 - ax1;
            segB[3] = by2 - ay1;
            step_pair_apart(segB);

            if (intersect_segments(segA, segB, (uint8_t *)out)
                && (out[1] != segA[3] || out[0] != segA[2]))
                return 1;

            j++;
            if ((int16_t)b->point_count < j) {
                pb = POINTS(0);
            } else {
                pb++;
                bx1 = bx2;
                by1 = by2;
                if ((int16_t)b->point_count == j) {
                    bx2 = fbx;
                    by2 = fby;
                } else {
                    bx2 = bx0 + pb[1].x;
                    by2 = by0 + pb[1].y;
                }
            }
        }

        i++;
        if ((int16_t)a->point_count < i) {
            pa = POINTS(0);
        } else {
            pa++;
            ax1 = ax2;
            ay1 = ay2;
            if ((int16_t)a->point_count == i) {
                ax2 = fax;
                ay2 = fay;
            } else {
                ax2 = ax0 + pa[1].x;
                ay2 = ay0 + pa[1].y;
            }
        }
    }
    return 0;
}

/*
 * 0x04169
 *
 * **Turn a link end for end**: swap the two ends of the link record and of
 * every pulley hanging off it.
 *
 * The walk starts at the end named by `[rec+2]` plus twice the byte at
 * `[rec+0xa]`, indexing the pair of ends at +0x5a, and follows +0x5c for as
 * long as the part it lands on is **kind 7, the pulley** - which is the same
 * kind `mark_joined_shapes` singles out for passing a mark through its second
 * belt only. Each pulley has its own pair at +0x5a/+0x5c swapped, +0x5e and
 * +0x60 rewritten from them, its +0x6a/+0x6c swapped, and six pairs swapped in
 * the record at its +0x66.
 *
 * Then the link itself: +0x2 and +0x4 exchange, +0x6 and +0x8 take the new
 * values, and the two bytes at +0xa and +0xb do the same into +0xc and +0xd.
 * The four writes are not two swaps - +0x6 and +0x8 are *copies* of the
 * swapped pair, not participants - and are transcribed as written.
 *
 * `mark_part_shapes([rec], 3)` last, so what was drawn for the old direction
 * is re-filed for the new one.
 *
 * *The name is a reading.* Nothing says "reverse"; it is what swapping both
 * ends of a run of pulleys amounts to.
 */
void reverse_link_ends(struct belt *rec)
{
    register struct belt *si;
    register struct part *di;
    uint8_t b;                          /* [bp-1] */
    int16_t tp;                         /* [bp-4] */
    struct point8 pair;                 /* [bp-6] */
    dg_near_t t;                         /* [bp-8] */

    di = PART_PTR(PART_PTR(rec->end_a_ptr)->link_ptr[rec->slot_a]);
    while (di != PART_NONE && di->kind == KIND_PULLEY) {
        t = di->link_ptr[0];
        di->link_ptr[0] = di->link_ptr[1];
        di->link_ptr[1] = t;
        di->link_ptr[2] = di->link_ptr[0];
        di->link_ptr[3] = di->link_ptr[1];

        /* the pair, not the byte: one 16-bit move */
        pair = di->attach[0];
        di->attach[0] = di->attach[1];
        di->attach[1] = pair;

        si = BELT_PTR(di->belt_ptr[0]);
        tp = si->pt[0][0].x;
        si->pt[0][0].x = si->pt[0][1].x;
        si->pt[0][1].x = tp;
        tp = si->pt[0][0].y;
        si->pt[0][0].y = si->pt[0][1].y;
        si->pt[0][1].y = tp;
        tp = si->pt[1][0].x;
        si->pt[1][0].x = si->pt[1][1].x;
        si->pt[1][1].x = tp;
        tp = si->pt[1][0].y;
        si->pt[1][0].y = si->pt[1][1].y;
        si->pt[1][1].y = tp;
        tp = si->pt[2][0].x;
        si->pt[2][0].x = si->pt[2][1].x;
        si->pt[2][1].x = tp;
        tp = si->pt[2][0].y;
        si->pt[2][0].y = si->pt[2][1].y;
        si->pt[2][1].y = tp;

        di = PART_PTR(di->link_ptr[1]);
    }

    t = rec->end_a_ptr;
    rec->home_a_ptr = rec->end_a_ptr = rec->end_b_ptr;
    rec->home_b_ptr = rec->end_b_ptr = t;

    b = rec->slot_a;
    rec->home_slot_a = rec->slot_a = rec->slot_b;
    rec->home_slot_b = rec->slot_b = b;

    mark_part_shapes(PART_PTR(rec->owner_ptr), 3);
}

/*
 * 0x042a2
 *
 * **Is the pointer over this part**, and if it is over one of the part's link
 * ends instead, which link.
 *
 * The pointer is 0x5784 and 0x5782; the part's own box is its +0x2a and +0x2c
 * less the play area's origins at 0x4ea3 and 0x4ea1, extended by its size at
 * +0x44 and +0x46. `exclude` is a link the caller is already holding: if the
 * part is that link, or either of the two records at its +0x66 and +0x68, or
 * the one at its +0x54, the box is **grown by 0xb on every side** - a part you
 * are already attached to is easier to hit than one you are not.
 *
 * Answers the part, or 0 when the pointer is outside its box, or one of the
 * link records when the pointer is over an end rather than the body.
 *
 * The two end tests are skipped entirely while a part is being carried
 * (0x4e69 == 9) - you cannot grab an end with your hands full - and the second
 * of them is skipped for kind 7, the pulley.
 *
 * Where an end matches, the link is put the right way round before it is
 * answered: the first test swaps the link's +4 and +6 in place, the second
 * calls `reverse_link_ends`, which is the same idea done properly for a run of
 * pulleys.
 *
 * The height of the first end's box is not its width: `+0x46 >> 1` against
 * +0x58 decides between +0x58 and a flat 0xa, so a short part gets a taller
 * grab area than its own half-height. Transcribed as the branch it is.
 */
struct part *part_under_pointer(struct part *exclude, register struct part *part)
{
    int16_t y0;
    int16_t pr;                         /* [bp-2] */
    int16_t pb;                         /* [bp-4] */
    int16_t pl;                         /* [bp-6] */
    int16_t pt;                         /* [bp-8] */
    int16_t x0;                         /* [bp-0xa] */
    int16_t x1;                         /* [bp-0xc] */
    int16_t y1;                         /* [bp-0xe] */
    int16_t i;                          /* [bp-0x10] */
    int16_t oy;                         /* [bp-0x12] */
    int16_t ox;                         /* [bp-0x14] */
    struct rope *link;                  /* [bp-0x16] */
    struct belt *cur;                   /* [bp-0x18] */
    struct belt *e0;                    /* [bp-0x1a] */
    struct belt *e1;                    /* [bp-0x1c] */
    struct part *link_end;              /* [bp-0x1e] */
    struct part *e0_part;               /* [bp-0x20] */
    struct part *e1_part;               /* [bp-0x22] */

    pl = pr = DG5768.pointer_x;
    pt = pb = DG5768.pointer_y;
    ox = part->box[0].x - DG4E67.origin_x;
    oy = part->box[0].y - DG4E67.origin_y;
    x0 = ox;
    y0 = oy;
    x1 = x0 + part->size[0].width;
    y1 = y0 + part->size[0].height;

    if ((link = ROPE_PTR(part->rope_ptr)) != ROPE_NONE)
        link_end = PART_PTR(link->owner_ptr);
    else
        link_end = PART_NONE;
    if ((e0 = BELT_PTR(part->belt_ptr[0])) != BELT_NONE)
        e0_part = PART_PTR(e0->owner_ptr);
    else
        e0_part = PART_NONE;
    if ((e1 = BELT_PTR(part->belt_ptr[1])) != BELT_NONE)
        e1_part = PART_PTR(e1->owner_ptr);
    else
        e1_part = PART_NONE;

    if (exclude != PART_NONE
        && (exclude == part || exclude == link_end
            || exclude == e0_part || exclude == e1_part)) {
        x0 -= 0xb;
        y0 -= 0xb;
        x1 += 0xb;
        y1 += 0xb;
    }

    if (x0 < pl && x1 > pr && y0 < pt && y1 > pb) {
        if (link != ROPE_NONE && DG4E67.tool != 9) {
            x0 = ox + part->grab.x;
            y0 = oy + part->grab.y;
            x1 = x0 + part->grab_size;
            y1 = (part->size[0].height >> 1) < (int16_t)part->grab_size
                 ? y0 + 0xa : y0 + part->grab_size;
            if (PART_PTR(link->owner_ptr) == exclude) {
                x0 -= 0xb;
                y0 -= 0xb;
            }
            if (x0 < pl && x1 > pr && y0 < pt && y1 > pb) {
                if (PART_PTR(link->end_a_ptr) == part) {
                    link->end_a_ptr = link->end_b_ptr;
                    link->end_b_ptr = dg_near(dgroup, part);
                }
                return PART_PTR(link->owner_ptr);
            }
        }

        for (cur = e0, i = 0; i < 2; cur = e1, i++) {
            if (cur != BELT_NONE && DG4E67.tool != 9
                && part->kind != KIND_PULLEY) {
                x0 = ox + part->attach[i].x - 8;
                y0 = oy + part->attach[i].y - 4;
                x1 = x0 + 0x10;
                y1 = y0 + 8;
                if (PART_PTR(cur->owner_ptr) == exclude) {
                    x0 -= 0xb;
                    y0 -= 0xb;
                }
                if (x0 < pl && x1 > pr && y0 < pt && y1 > pb) {
                    if (PART_PTR(cur->end_a_ptr) == part)
                        reverse_link_ends(cur);
                    return PART_PTR(cur->owner_ptr);
                }
            }
        }
        return part;
    }
    return PART_NONE;
}

/*
 * 0x04500
 *
 * **What the pointer is on**, searched across every part on the screen.
 *
 * A part is offered first: if `rec` is given and `part_under_pointer` says the
 * pointer is on it, that is the answer and nothing is walked. That is what
 * makes dragging stick to what you already have hold of.
 *
 * Otherwise every part is tried, `pick_by_flag(0x3000)` first and
 * `pick_for_record(cur, 0x1000)` after. A hit whose +6 has bit 0x8000 clear
 * wins outright and ends the walk; one with it set is only *remembered*, in
 * `best`, and the walk goes on. So a part carrying that bit is the answer only
 * when nothing else was hit at all - it is the fallback, not a match.
 *
 * `part_under_pointer` is asked with `rec` as its exclude, and its answer is
 * discarded when the hit is the part itself, that part's +6 has the bit, and
 * `rec` is non-zero. Both of those tests exist twice over, once for the
 * equal-to-`cur` case and once for any other, and the second reads a +6 from a
 * pointer the first branch may have zeroed; it is transcribed as written.
 *
 * With nothing found and nothing remembered: 0 if a **belt** is being carried
 * - kind 0x0a at 0x50d5, which must land on a part and not on the background -
 * and otherwise `rec`, so a drag that wanders off everything keeps what it had.
 */
struct part *find_part_from(register struct part *rec)
{
    register struct part *si;
    struct part *cur;                   /* [bp-2] */
    struct part *best;                  /* [bp-4] */

    if (rec != PART_NONE && (si = part_under_pointer(rec, rec)) != PART_NONE)
        return si;

    best = PART_NONE;
    for (cur = pick_by_flag(0x3000); cur != PART_NONE;
         cur = pick_for_record(cur, 0x1000)) {
        si = part_under_pointer(rec, cur);
        if (si == cur && (cur->flags_06 & 0x8000) && rec != PART_NONE)
            si = PART_NONE;
        else if ((si->flags_06 & 0x8000) && rec != PART_NONE)
            si = PART_NONE;
        if (si != PART_NONE) {
            if (si->flags_06 & 0x8000)
                best = si;
            else
                return si;
        }
    }

    if (best != PART_NONE)
        return best;
    if (DG50D3.dragged_part_ptr != 0
        && PART_PTR(DG50D3.dragged_part_ptr)->kind == KIND_ROPE)
        return PART_NONE;
    return rec;
}

/*
 * 0x045b8
 *
 * **Where a belt end could attach**: the part under the pointer that will take
 * one, and which of its two ends, written through `out_end`.
 *
 * `find_part_from` finds the part; bit 4 of +8 is what says it can take a belt
 * at all, and without it the answer is 0. Bit 8 says it has *two* ends worth
 * choosing between, and then the nearer one wins - both distances are taken
 * along the **x axis only**, `abs(0x5784 - end)`, with the ends at +0x6a and
 * +0x6c from the part's own +0x1e. Without bit 8 end 0 is used and nothing is
 * measured.
 *
 * The comparison is `>=`, so a pointer exactly between the two ends picks the
 * second.
 *
 * Then whether that end is free. A pulley - kind 7 - has one socket at +0x5a
 * and is refused if it is taken; everything else is refused if the chosen
 * end's +0x66 pair is already occupied. Either way the answer becomes 0 while
 * `out_end` keeps the end that was chosen, which the caller does not read
 * unless the answer was non-zero.
 */
struct part *find_belt_anchor(register int16_t *out_end, struct part *rec)
{
    register struct part *si;
    int16_t e0;                         /* [bp-2] */
    int16_t e1;                         /* [bp-4] */

    if ((si = find_part_from(rec)) != PART_NONE) {
        if (si->flags_08 & 4) {
            if (si->flags_08 & 8) {
                e0 = e1 = si->pos[0].x - DG4E67.origin_x;
                e0 += si->attach[0].x;
                e1 += si->attach[1].x;
                if (abs((int16_t)(DG5768.pointer_x - e0))
                    < abs((int16_t)(DG5768.pointer_x - e1)))
                    *out_end = 0;
                else
                    *out_end = 1;
            } else
                *out_end = 0;

            if (si->kind == KIND_PULLEY) {
                if (si->link_ptr[0] != 0)
                    si = PART_NONE;
            } else if (si->belt_ptr[*out_end] != 0)
                si = PART_NONE;
        } else
            si = PART_NONE;
    }
    return si;
}

/*
 * 0x04652
 *
 * Put up the waiting cursor, remembering the one it replaces at DGROUP 0x4ec3
 * so `restore_cursor` can put it back. A cursor that is *already* the waiting
 * one is not remembered, which is what stops two of these in a row losing the
 * cursor the first one replaced.
 */
void wait_cursor(void)
{
    if (DG4E67.cursor != 1)
        DG4E67.saved_cursor = DG4E67.cursor;

    select_cursor(1);
}

/*
 * 0x0466e
 *
 * And put back whatever `wait_cursor` remembered.
 */
void restore_cursor(void)
{
    select_cursor(DG4E67.saved_cursor);
}

/*
 * 0x0467d
 *
 * Choose one of the game's cursors by number, and do nothing if it is already
 * the one showing - DGROUP 0x4ec5 remembers which.
 *
 * A number above 0x1a is refused by being turned into 0 rather than rejected,
 * and the first nine have a hot spot in the pair of tables at DGROUP 0x284a and
 * 0x285c; from nine up the hot spot is (0, 0). The bitmap itself is the entry
 * in the list at DGROUP 0x52f6, which the start-up loaded from "mouse.bmp".
 */
void select_cursor(register int16_t which)
{
    int16_t hot_x;
    int16_t hot_y;                      /* [bp-2] */

    if (which > 0x1a)
        which = 0;
    if (which != DG4E67.cursor) {
        DG4E67.cursor = which;
        if (which < 9) {
            hot_x = MACHINE_CURSOR_HOTSPOTS.hot_x[which];
            hot_y = MACHINE_CURSOR_HOTSPOTS.hot_y[which];
        } else
            hot_x = hot_y = 0;
        set_cursor(DG52ED.cursor_art[which],
                   hot_x, hot_y);
    }
}

/*
 * 0x046d8
 *
 * Which cursor the currently selected tool wants, as a number for
 * `select_cursor` above.
 *
 * A **jump table** on DGROUP 0x4e69, the selected tool, at CS:0x4736: the
 * index is the tool minus one and `ja` sends anything above eight - which
 * includes tool 0, since the subtraction wraps - to the default of 0. Nine
 * tools, eight of them a constant:
 *
 *     tool  1 -> 4      tool  5 -> 7
 *     tool  2 -> 5      tool  6 -> 7
 *     tool  3 -> 6      tool  7 -> 2
 *     tool  4 -> 6      tool  8 -> 3
 *
 * Tools 3 and 4 share an entry and so do 5 and 6; the table has nine slots and
 * seven distinct targets, which is why this is transcribed as the table rather
 * than as a formula.
 *
 * **Tool 9 is the one that asks a question**: it looks at the part being
 * dragged - the near pointer at DGROUP 0x50d5 - and answers by its kind at
 * +4, the same kind `draw_machine` switches on. A rope, kind 8, wants cursor
 * 8; a belt, kind 0x0a, wants cursor 9; anything else, including no part at
 * all, wants 0. The pointer is loaded twice, once for each comparison, and it
 * is transcribed that way.
 *
 * *The name is a reading.* What the nine tools are is not written down
 * anywhere here; that 0x4e69 selects one is from `region_cursor_playfield`,
 * which is the only caller, and from 0x4e69's other uses testing it for 7, 8
 * and 9.
 */
int16_t cursor_for_tool(void)
{
    int16_t r;

    switch (DG4E67.tool) {
    case 1:
        r = 4;
        break;
    case 2:
        r = 5;
        break;
    case 3:
    case 4:
        r = 6;
        break;
    case 5:
    case 6:
        r = 7;
        break;
    case 7:
        r = 2;
        break;
    case 8:
        r = 3;
        break;
    case 9:
        if (PART_PTR(DG50D3.dragged_part_ptr)->kind == KIND_BELT)
            r = 8;
        else if (PART_PTR(DG50D3.dragged_part_ptr)->kind == KIND_ROPE)
            r = 9;
        else
            r = 0;
        break;
    default:
        r = 0;
        break;
    }
    return r;
}

/*
 * 0x04748
 *
 * **Which of a part's two ends could move**, as a bitmask.
 *
 * A rope or a belt - kinds 8 and 0x0a - has no ends of its own to move and
 * answers 0 before anything else is looked at.
 *
 * Two of the four bits are read straight off the part: +8 bit 0x80 gives 1 and
 * bit 0x100 gives 2. The other two are *earned*, and only for a part whose +6
 * says it has that end at all - 0x400 for the first, 0x200 for the second:
 *
 *   - while a part is being carried, 0x4e69 == 9, the end is simply taken as
 *     available and nothing is tried;
 *   - otherwise the end is actually moved, by the kind's flip hook at +0x30
 *     with 1 or 2, `object_overlaps_any` is asked whether that put the part
 *     inside something, and the hook is called **again with the same
 *     argument** to put it back. The bit is set only if nothing was hit.
 *
 * So the answer is "where could this go", worked out by going there and
 * undoing it, twice per end. `+0x94` is refreshed from `+8` after each call
 * because the hook changes +8 and the two must not drift apart - and it is
 * done after the restoring call as well as the trying one, which is why there
 * are four of those assignments and not two.
 */
uint16_t part_flip_options(register struct part *part)
{
    uint16_t di;

    if (part->kind == KIND_BELT || part->kind == KIND_ROPE)
        return 0;

    di = 0;
    if (part->flags_08 & 0x80)
        di |= 1;
    if (part->flags_08 & 0x100)
        di |= 2;

    if (part->flags_06 & 0x400) {
        if (DG4E67.tool == 9)
            di |= 4;
        else {
            part_flip(part, 1);
            part->start_flags = part->flags_08;
            if (!object_overlaps_any(part))
                di |= 4;
            part_flip(part, 1);
            part->start_flags = part->flags_08;
        }
    }

    if (part->flags_06 & 0x200) {
        if (DG4E67.tool == 9)
            di |= 8;
        else {
            part_flip(part, 2);
            part->start_flags = part->flags_08;
            if (!object_overlaps_any(part))
                di |= 8;
            part_flip(part, 2);
            part->start_flags = part->flags_08;
        }
    }
    return di;
}

/*
 * 0x04830
 *
 * **Which handle of a part the pointer is on**, as a code: 1 to 6 for the six
 * grab handles, 7 for the body, 8 for the top-left corner, 0xa for nothing.
 *
 * `part_flip_options` is asked first and its answer kept at DGROUP 0x50bd,
 * because four of the six handles only exist if the corresponding end could
 * move - bits 1, 2, 4 and 8 gate the pairs 3/4, 5/6, 1 and 2.
 *
 * A rope and a belt are special-cased before the general box, and both look at
 * the *other* end of their link rather than at themselves: a rope through its
 * +0x54 record's +6, a belt through its +0x66 record's +4 with the end index
 * from that record's +0xb. Their boxes are not the same size either - the rope
 * end is 10 by 10 and the belt end 15 by 7.
 *
 * **The rope and belt branches subtract 0x4ea3 from the *y* coordinate**,
 * where every other place subtracts 0x4ea1. That is what the original does,
 * twice each, and it is transcribed as written rather than corrected. It
 * cannot be seen on level one, where both origins are -8; it would show as a
 * rope end whose grab box is offset vertically on a level whose window has
 * scrolled. Recorded here because a reader who "fixes" it will be changing
 * behaviour, not repairing it.
 *
 * Every handle box is 11 across from its anchor, and the anchors are the
 * corners and the midpoints of the part's own extent, the midpoints pulled
 * back by 6 so the handle straddles them.
 */
uint16_t part_handle_at_pointer(register struct part *part)
{
    int16_t x0;
    int16_t x_mid;                      /* [bp-2] */
    int16_t x_end;                      /* [bp-4] */
    int16_t y0;                         /* [bp-6] */
    int16_t y_mid;                      /* [bp-8] */
    int16_t y_end;                      /* [bp-0xa] */
    uint16_t idx;                       /* [bp-0xc] */
    struct part *rec;                   /* [bp-0xe] */
    struct belt *end;                   /* [bp-0x10] */

    DG50AF.flip_options = part_flip_options(part);

    if (part->kind == KIND_BELT) {
        rec = PART_PTR(ROPE_PTR(part->rope_ptr)->end_b_ptr);
        x0 = rec->box[0].x + rec->grab.x - DG4E67.origin_x;
        /* the original takes origin_x off a y here, and below */
        y0 = rec->box[0].y + rec->grab.y - DG4E67.origin_x;
        if (x0 - 11 <= DG5768.pointer_x && DG5768.pointer_x < x0
            && y0 - 11 <= DG5768.pointer_y && DG5768.pointer_y < y0)
            return 8;
        if (DG5768.pointer_x >= x0 && x0 + 10 > DG5768.pointer_x
            && DG5768.pointer_y >= y0 && y0 + 10 > DG5768.pointer_y)
            return 7;
    }

    if (part->kind == KIND_ROPE) {
        end = BELT_PTR(part->belt_ptr[0]);
        rec = PART_PTR(end->end_b_ptr);
        idx = end->slot_b;
        x0 = rec->box[0].x + rec->attach[idx].x - DG4E67.origin_x - 8;
        y0 = rec->box[0].y + rec->attach[idx].y - DG4E67.origin_x - 4;
        if (x0 - 11 <= DG5768.pointer_x && DG5768.pointer_x < x0
            && y0 - 11 <= DG5768.pointer_y && DG5768.pointer_y < y0)
            return 8;
        if (DG5768.pointer_x >= x0 && x0 + 15 > DG5768.pointer_x
            && DG5768.pointer_y >= y0 && y0 + 7 > DG5768.pointer_y)
            return 7;
    }

    x0 = part->box[0].x - DG4E67.origin_x;
    x_mid = x0 + (part->size[0].width >> 1) - 6;
    x_end = x0 + part->size[0].width;
    y0 = part->box[0].y - DG4E67.origin_y;
    y_mid = y0 + (part->size[0].height >> 1) - 6;
    y_end = y0 + part->size[0].height;

    if (x0 - 11 <= DG5768.pointer_x && DG5768.pointer_x < x0
        && y0 - 11 <= DG5768.pointer_y && DG5768.pointer_y < y0)
        return 8;

    if (DG50AF.flip_options & 1) {
        if (x0 - 11 <= DG5768.pointer_x && DG5768.pointer_x < x0
            && DG5768.pointer_y >= y_mid && y_mid + 11 > DG5768.pointer_y)
            return 3;
        if (DG5768.pointer_x > x_end && x_end + 11 > DG5768.pointer_x
            && DG5768.pointer_y >= y_mid && y_mid + 11 > DG5768.pointer_y)
            return 4;
    }
    if (DG50AF.flip_options & 2) {
        if (y0 - 11 <= DG5768.pointer_y && DG5768.pointer_y < y0
            && DG5768.pointer_x >= x_mid && x_mid + 11 > DG5768.pointer_x)
            return 5;
        if (DG5768.pointer_y > y_end && y_end + 11 > DG5768.pointer_y
            && DG5768.pointer_x >= x_mid && x_mid + 11 > DG5768.pointer_x)
            return 6;
    }
    if (DG50AF.flip_options & 4) {
        if (x0 - 11 <= DG5768.pointer_x && DG5768.pointer_x < x0
            && DG5768.pointer_y > y_end && y_end + 11 > DG5768.pointer_y)
            return 1;
    }
    if (DG50AF.flip_options & 8) {
        if (DG5768.pointer_x > x_end && x_end + 11 > DG5768.pointer_x
            && DG5768.pointer_y > y_end && y_end + 11 > DG5768.pointer_y)
            return 2;
    }
    if (DG5768.pointer_x >= x0 && DG5768.pointer_x < x_end
        && DG5768.pointer_y >= y0 && DG5768.pointer_y < y_end)
        return 7;
    return 0x0a;
}

/*
 * 0x04b53
 *
 * Are two points within 140 of each other in both axes?
 *
 * The absolute value is the branchless `cwd / xor ax,dx / sub ax,dx`: sign
 * extend into DX, exclusive-or, subtract. Both axes must pass; the first
 * failure answers 0 immediately.
 */
int16_t points_within_140(register const struct point16 *a,
                          register const struct point16 *b)
{
    if (abs((int16_t)(a->x - b->x)) > 0x8c)
        return 0;
    if (abs((int16_t)(a->y - b->y)) > 0x8c)
        return 0;
    return 1;
}

/*
 * 0x04b8f
 *
 * Are a rope's two ends close enough together to matter?
 *
 * The two parts it joins are at +4 and +6 of the rope. Either being zero means
 * that end is not attached to anything, and `find_part_from` is asked for a
 * part instead - which has to answer with one whose flags at +8 have bit 0 or
 * bit 1 set, or the whole thing is 0. With both ends in hand it is
 * `points_within_140` on the positions at +0x1e.
 */
int16_t rope_ends_close(struct rope *rope)
{
    register struct part *si;
    register struct part *di;

    if ((si = PART_PTR(rope->end_a_ptr)) == PART_NONE) {
        if ((si = find_part_from(PART_NONE)) == PART_NONE)
            return 0;
        if ((si->flags_08 & 2) || !(si->flags_08 & 1))
            return 0;
        return 1;
    }
    /* an empty branch and an `else`: the image's `je` over a `jmp` is the
       shape Borland gives exactly that, and not the `if (!di)` it means */
    if ((di = PART_PTR(rope->end_b_ptr)) != PART_NONE) {
    } else {
        if ((di = find_part_from(PART_NONE)) == PART_NONE)
            return 0;
        if ((di->flags_08 & 2) || !(di->flags_08 & 1))
            return 0;
    }
    return points_within_140(&si->pos[0], &di->pos[0]);
}

/*
 * 0x04c0d
 *
 * **The angle from one part to another**, in the sixteen-bit turn this code
 * works in - `atan2_long` answers it and 0x4000 is a quarter. `aim_link_at_bisector` uses
 * it twice to bisect.
 *
 * Three ways of deciding what "the other" is, and they are not
 * interchangeable:
 *
 * **No other part at all** and it is the *pointer* that is aimed at: the
 * position at DGROUP 0x5784 and 0x5782 plus the view's origin at 0x4ea3 and
 * 0x4ea1. So a chain being dragged points at the mouse, and the same routine
 * does it.
 *
 * **Another kind-7 part** and it is that part's own position, plainly.
 *
 * **Anything else** and the point aimed at is offset by the two bytes at that
 * part's +0x6a and +0x6b for the slot this one occupies - `match_field_5a_5c`
 * says which slot - so a chain hangs from where it is attached rather than from
 * the middle of what it is attached to. Those are the same four bytes
 * `aim_link_at_bisector` writes, which is what makes the two routines a pair: one decides
 * where a link points, the other where the next one hangs from.
 *
 * The differences are sign-extended to longs before the divide, because a part
 * can be further away than a word holds once the view's origin is in it.
 */
uint16_t angle_between_parts(register struct part *part,
                             register struct part *other)
{
    uint16_t angle;                     /* [bp-2] */
    int16_t slot;                       /* [bp-4] */
    int32_t dx;                         /* [bp-8] */
    int32_t dy;                         /* [bp-0xc] */

    if (other == PART_NONE) {
        dx = (int16_t)(part->pos[0].x - (DG5768.pointer_x + DG4E67.origin_x));
        dy = (int16_t)(part->pos[0].y - (DG5768.pointer_y + DG4E67.origin_y));
    } else if (other->kind == KIND_PULLEY) {
        dx = (int16_t)(part->pos[0].x - other->pos[0].x);
        dy = (int16_t)(part->pos[0].y - other->pos[0].y);
    } else {
        slot = match_field_5a_5c(part, other);
        dx = (int16_t)(part->pos[0].x - (other->pos[0].x + other->attach[slot].x));
        dy = (int16_t)(part->pos[0].y - (other->pos[0].y + other->attach[slot].y));
    }
    angle = atan2_long(dx, dy);
    return angle;
}

/*
 * 0x04cc8
 *
 * Re-tension a part and the pulleys at either end of it.
 *
 * `aim_link_at_bisector` is what a pulley needs after anything has moved, and this is the
 * three places it has to happen at once: the part itself if it is a pulley -
 * kind 7 - and then whatever sits in its +0x5a and +0x5c slots, each only if
 * it is also a pulley.
 *
 * The part's own call is **just the one call**; the two neighbours get the
 * marking as well - `mark_part_shapes` with 3 and `mark_needs_refile` with 2 -
 * because their shapes have moved and the part's own caller is expected to
 * have marked it already. The asymmetry is the original's.
 *
 * Both slots are read into locals *before* any of the calls, so a callee that
 * rewrites +0x5a or +0x5c cannot change which parts this one goes on to visit.
 */
void retension_pulleys(struct part *part)
{
    struct part *di = PART_PTR(part->link_ptr[0]);
    struct part *other = PART_PTR(part->link_ptr[1]);

    if (part->kind == KIND_PULLEY)
        aim_link_at_bisector(part);

    if (di != PART_NONE && di->kind == KIND_PULLEY) {
        aim_link_at_bisector(di);
        mark_part_shapes(di, 3);
        mark_needs_refile(di, 2);
    }

    if (other != PART_NONE && other->kind == KIND_PULLEY) {
        aim_link_at_bisector(other);
        mark_part_shapes(other, 3);
        mark_needs_refile(other, 2);
    }
}

/*
 * 0x04d4c
 *
 * **Point a chain link along the bisector of its two neighbours.** Called on a
 * kind-7 part after a neighbour has been spliced out, and always followed by
 * `mark_part_shapes(part, 3)`, which is what makes the new shape draw.
 *
 * The angle to each neighbour comes from `angle_between_parts`, and 0x2000 is added to
 * both - a quarter turn, added twice, so it cancels in the difference and only
 * moves where the quadrant boundaries fall. **The halving is of the difference,
 * not of the sum**, which is what makes it a bisector on a circle rather than
 * an average: `mid` starts from whichever end the short way round begins at,
 * and `d >= 0x8000` - unsigned, so "more than half a turn" - is the test for
 * which that is.
 *
 * The quadrant is the top two bits of the result, and it decides everything
 * below: two bytes at +0x6a..+0x6d get 6 or 0x0a, and the other two get 0 and
 * 0x0f. Which pair is which comes from the quadrant, and which way round from
 * `d` again - so a link that bends one way and a link that bends the other are
 * given mirrored values from the same code.
 *
 * **The jump table at cs:0x4e5d has four entries and two bodies**: quadrants 0
 * and 2 share one, 1 and 3 the other. That is a `switch` the compiler expanded,
 * not four cases - and inside each body the quadrant is tested again to
 * separate the pair. Transcribed as the two bodies it is, with the tests kept.
 *
 * The quadrant itself is left at +0x0c and mirrored to +0x90, the same pairing
 * `break_second_attachment` copies.
 */
void aim_link_at_bisector(register struct part *part)
{
    int16_t quad;
    uint16_t a1;                        /* [bp-2] */
    uint16_t a2;                        /* [bp-4] */
    uint16_t mid;                       /* [bp-6] */
    uint16_t d;                         /* [bp-8] */
    int16_t form;                       /* [bp-0xa] */
    struct part *after;                 /* [bp-0xc] */
    struct part *before;                /* [bp-0xe] */

    if ((after = PART_PTR(part->link_ptr[1])) != PART_NONE) {
        before = PART_PTR(part->link_ptr[0]);
        a1 = angle_between_parts(part, after);
        a1 += 0x2000;
        a2 = angle_between_parts(part, before);
        a2 += 0x2000;
        if ((d = a2 - a1) < 0x8000)
            mid = a1 + (d >> 1);
        else
            mid = a2 + ((uint16_t)(0 - d) >> 1);

        quad = (mid >> 14) & 3;
        switch (quad) {
        case 0:
        case 2:
            part->attach[0].y = part->attach[1].y = quad == 0 ? 0x0a : 6;
            if ((quad == 0 && d < 0x8000) || (quad == 2 && d >= 0x8000)) {
                part->attach[0].x = 0;
                part->attach[1].x = 15;
            } else {
                part->attach[1].x = 0;
                part->attach[0].x = 15;
            }
            break;
        case 1:
        case 3:
            part->attach[0].x = part->attach[1].x = quad == 1 ? 6 : 0x0a;
            if ((quad == 1 && d < 0x8000) || (quad == 3 && d >= 0x8000)) {
                part->attach[0].y = 0;
                part->attach[1].y = 15;
            } else {
                part->attach[1].y = 0;
                part->attach[0].y = 15;
            }
            break;
        }

        form = quad;
        part->start_form = part->form = form;
    }
}

/*
 * 0x04e65
 *
 * Work out the two endpoints of the link between a pair of objects, and a
 * second pair of endpoints offset from them.
 *
 * The two objects are named by the words at +4 and +6 of the link record. Each
 * contributes a position at +0x2a/+0x2c plus a **byte** offset at +0x56/+0x57,
 * zero extended - so the offsets are 0..255 and never negative.
 *
 * Then the dominant axis decides how the object's size at +0x58 is spread. If
 * the link is more vertical than horizontal the offsets go across the x axis
 * and the halves along y; otherwise the other way round. Either way one
 * endpoint gets the whole size and the other half of it, which is what draws a
 * band between the two objects rather than a line between their corners.
 *
 * The comparison is between the two **absolute** differences, each the
 * branchless `cwd / xor / sub`.
 */
void compute_link_endpoints(register struct rope *link)
{
    /*
     * **An end can be 0, and the original reads it anyway.** After the first
     * click of a rope the far end is still empty, and `mark_needs_refile` in
     * state 0x1000 comes here before `rope_ends_close` asks about it. The
     * listing loads both ends at 0x04e70/0x04e73 and reads `[bx+0x56]` and
     * `[bx+0x2a]` through them with no test, so an empty end reads DS:0 - the
     * Borland banner - and the endpoint is those bytes until the second click
     * fills the end. `PART_PTR(0)` is DS:0, so the same read is written here.
     */
    struct part *a;
    int16_t a_dx1;                      /* [bp-2] */
    int16_t a_dy1;                      /* [bp-4] */
    int16_t a_dx2;                      /* [bp-6] */
    int16_t a_dy2;                      /* [bp-8] */
    int16_t b_dx1;                      /* [bp-0xa] */
    int16_t b_dy1;                      /* [bp-0xc] */
    int16_t b_dx2;                      /* [bp-0xe] */
    int16_t b_dy2;                      /* [bp-0x10] */
    struct part *b;                     /* [bp-0x12] */

    a = PART_PTR(link->end_a_ptr);
    b = PART_PTR(link->end_b_ptr);
    link->pt[0][0].x = a->box[0].x + a->grab.x;
    link->pt[0][0].y = a->box[0].y + a->grab.y;
    link->pt[0][1].x = b->box[0].x + b->grab.x;
    link->pt[0][1].y = b->box[0].y + b->grab.y;

    if (abs((int16_t)(link->pt[0][0].x - link->pt[0][1].x))
        < abs((int16_t)(link->pt[0][0].y - link->pt[0][1].y))) {
        a_dx1 = b_dx1 = 0;
        a_dx2 = a->grab_size;
        a_dy1 = a_dy2 = a_dx2 >> 1;
        b_dx2 = b->grab_size;
        b_dy1 = b_dy2 = b_dx2 >> 1;
    } else {
        a_dy1 = b_dy1 = 0;
        a_dy2 = a->grab_size;
        a_dx1 = a_dx2 = a_dy2 >> 1;
        b_dy2 = b->grab_size;
        b_dx1 = b_dx2 = b_dy2 >> 1;
    }

    link->pt[0][2].x = link->pt[0][0].x + a_dx2;
    link->pt[0][2].y = link->pt[0][0].y + a_dy2;
    link->pt[0][3].x = link->pt[0][1].x + b_dx2;
    link->pt[0][3].y = link->pt[0][1].y + b_dy2;
    link->pt[0][0].x += a_dx1;
    link->pt[0][0].y += a_dy1;
    link->pt[0][1].x += b_dx1;
    link->pt[0][1].y += b_dy1;
}

/*
 * 0x04f7f
 *
 * Recompute a link's endpoint coordinates from the objects it joins, and then
 * set the rest lengths those endpoints imply.
 *
 * The endpoints land in the +0x14 array `belt_orientation` and
 * `link_end_distance` read as generation zero - +0x14/+0x16 for the first end,
 * +0x18/+0x1a for the second. Each is the object's position at +0x2a/+0x2c
 * plus the zero-extended byte pair at +0x6a/+0x6b. A link with no object at +2
 * does nothing at all; one with nothing at +4 still gets its first end.
 *
 * Then it follows the chain at +0x5a for as long as the objects on it are type
 * 7, writing both of each one's endpoints into the point array at +0x66. Those
 * are built from +0x1e/+0x20 rather than +0x2a/+0x2c - a type 7 object does
 * not carry its position in the same place.
 *
 * Finally the rest lengths. `link_end_distance` is called with generation 3,
 * which is not one of the three it names and so falls to the +0x14 array - the
 * endpoints just written. The two lengths go to +0x96 and +0x9c on the object
 * the link names at +0, which are the newest slots of the chains
 * `shift_state_history` ages and `link_slack` measures against. So this is
 * where a link learns how long it is meant to be.
 *
 * The global at 0x4e6b holding 0x2000 skips the rest lengths entirely, leaving
 * whatever they were.
 */
void refresh_link_geometry(register struct belt *link)
{
    register struct part *a;            /* the first end, then the chain */
    uint16_t idx;                       /* [bp-2] */
    uint16_t k;                         /* [bp-4] */
    int16_t j;                          /* [bp-6] */
    struct part *b;                     /* [bp-8] */

    if ((a = PART_PTR(link->end_a_ptr)) != PART_NONE) {
        idx = link->slot_a;
        link->pt[0][0].x = a->box[0].x + a->attach[idx].x;
        link->pt[0][0].y = a->box[0].y + a->attach[idx].y;
        if ((b = PART_PTR(link->end_b_ptr)) != PART_NONE) {
            k = link->slot_b;
            link->pt[0][1].x = b->box[0].x + b->attach[k].x;
            link->pt[0][1].y = b->box[0].y + b->attach[k].y;
        }

        for (a = PART_PTR(a->link_ptr[idx]);
             a != PART_NONE && a->kind == KIND_PULLEY;
             a = PART_PTR(a->link_ptr[0]))
            for (j = 0; j < 2; j++) {
                BELT_PTR(a->belt_ptr[0])->pt[0][j].x = a->pos[0].x + a->attach[j].x;
                BELT_PTR(a->belt_ptr[0])->pt[0][j].y = a->pos[0].y + a->attach[j].y;
            }

        if (DG4E67.state != 0x2000) {
            PART_PTR(link->owner_ptr)->word_96 = link_end_distance(link, 3, 0);
            PART_PTR(link->owner_ptr)->spin = link_end_distance(link, 3, 1);
        }
    }
}

/*
 * 0x050a6
 *
 * **Re-home the carried part** onto whatever it is now near, and let go of
 * whatever it was on before.
 *
 * The old host and slot are saved first and +0x62 is cleared, so the search
 * cannot find the part still attached to where it was.
 * `link_nearby_objects(part, 0x2000, -8, 8, -8, 8)` fills the +0x78 chain with
 * candidates - a margin of 8 in every direction - and the chain is walked
 * once.
 *
 * The walk stops at the **first** of three things, by zeroing `si` rather than
 * breaking: the old host again, which keeps the slot it already had; or a part
 * whose +0xa has bit 2 and a free +0x62, taking slot 0; or the same with a
 * free +0x64, taking slot 1. Anything else is skipped and the chain followed
 * through +0x78.
 *
 * Detaching and attaching are separate and both conditional. The old host is
 * only cleared if there was one *and* it is not the one just chosen - so
 * landing back where you started does nothing at all. Each of the two writes
 * the slot at `(slot + 4) * 2` from +0x5a, runs the host's setup hook, and
 * copies its +0xc into +0x90.
 */
void rehome_carried_part(void)
{
    register struct part *si;
    register struct part *di = PART_NONE;
    uint8_t old_slot;                   /* [bp-1] */
    uint8_t slot;                       /* [bp-2] */
    struct part *old;                   /* [bp-4] */

#ifndef __TURBOC__
    /* Ours: read only when a host was found, and set with it, which gcc
       cannot see. The original leaves it as the stack had it. */
    slot = 0;
#endif
    old = PART_PTR(PART_PTR(DG50D3.dragged_part_ptr)->link_ptr[4]);
    old_slot = PART_PTR(DG50D3.dragged_part_ptr)->host_slot;
    PART_PTR(DG50D3.dragged_part_ptr)->link_ptr[4] = 0;
    link_nearby_objects(PART_PTR(DG50D3.dragged_part_ptr), 0x2000, -8, 8, -8, 8);

    si = PART_PTR(PART_PTR(DG50D3.dragged_part_ptr)->next_linked_ptr);
    while (si != PART_NONE) {
        if (si == old) {
            di = old;
            slot = old_slot;
            si = PART_NONE;
        } else if (si->flags_0a & 2) {
            if (si->link_ptr[4] == 0) {
                di = si;
                slot = 0;
                si = PART_NONE;
            } else if (si->link_ptr[5] == 0) {
                di = si;
                slot = 1;
                si = PART_NONE;
            }
        }
        if (si != PART_NONE)
            si = PART_PTR(si->next_linked_ptr);
    }

    if (old != PART_NONE && di != old) {
        old->link_ptr[PART_PTR(DG50D3.dragged_part_ptr)->host_slot + 4] = 0;
        PART_PTR(DG50D3.dragged_part_ptr)->link_ptr[4] = 0;
        PART_KINDS[old->kind].setup(old);
        old->start_form = old->form;
    }
    if (di != PART_NONE) {
        di->link_ptr[slot + 4] = DG50D3.dragged_part_ptr;
        PART_PTR(DG50D3.dragged_part_ptr)->link_ptr[4] = dg_near(dgroup, di);
        PART_PTR(DG50D3.dragged_part_ptr)->host_slot = slot;
        PART_KINDS[di->kind].setup(di);
        di->start_form = di->form;
    }
}

/*
 * 0x051cb
 *
 * **Break the second kind of attachment**, the one at +0x62 and slots 4 and 5
 * of the +0x5a array - not the ropes and belts the rest of the removal chain
 * deals with.
 *
 * Bit 1 of +0x0a says which end of it this part is, and the two halves are
 * mirror images. **Set**: others hang off this one, so slots 4 and 5 are walked,
 * each one found is cleared here and its +0x62 - the back-pointer - cleared
 * there. **Clear**: this part hangs off another, so the one at +0x62 is found
 * and *this* part's entry in *its* array is cleared, at the slot the byte at
 * +0x7e names, plus four.
 *
 * That +0x7e is what makes the second half possible at all: a part hanging off
 * another remembers which of the other's slots it is in, so it can take itself
 * out without searching.
 *
 * Every part touched is then handed to its kind's own routine through the table
 * at DGROUP 0xed0, indexed by kind times 0x3a - its setup hook - and afterwards +0x0c is copied to
 * +0x90. Both halves do that copy, and both do it to the part at the *far* end
 * rather than to the one they were given.
 */
void break_second_attachment(register struct part *part)
{
    struct part *other;
    int16_t i;                          /* [bp-2] */

    if (part->flags_0a & 2) {
        for (i = 4; i < 6; i++)
            if ((other = PART_PTR(part->link_ptr[i])) != PART_NONE) {
                part->link_ptr[i] = 0;
                other->link_ptr[4] = 0;
                PART_KINDS[other->kind].setup(other);
            }
        PART_KINDS[part->kind].setup(part);
        part->start_form = part->form;
    } else if ((other = PART_PTR(part->link_ptr[4])) != PART_NONE) {
        other->link_ptr[part->host_slot + 4] = 0;
        part->link_ptr[4] = 0;
        PART_KINDS[part->kind].setup(part);
        PART_KINDS[other->kind].setup(other);
        other->start_form = other->form;
    }
}

/*
 * 0x0527f
 *
 * **Untie a rope from both the parts it joins**, before the rope itself goes.
 * `remove_all_parts` calls it for kind 8, which is the kind `draw_machine`
 * already names a rope, and the record at the part's +0x54 holds one part at +4
 * and another at +6 - so the name is read off the shape, not guessed from the
 * caller.
 *
 * Each end is let go the same way: bit 1 of its flags at +8 is cleared, the
 * result copied to +0x94, and its own +0x54 - the link back to this rope -
 * zeroed. Then the rope's reference to it is zeroed too, so neither end can be
 * reached from the other afterwards.
 *
 * **The two ends are written out twice rather than looped**, because there are
 * exactly two and they are separate fields, not an array. Transcribed the same
 * way: a loop here would be inventing a structure the original does not have.
 *
 * The copy to +0x94 is the flags being mirrored, and both ends get it - so
 * whatever reads +0x94 is reading what the flags were left as, not what they
 * were when something last drew.
 *
 * Then, unless bit 11 of the part's own +6 is set, the generic removal runs on
 * top. So a rope is not a special case *instead* of the ordinary one; it is a
 * special case *before* it.
 */
void untie_rope(struct part *part)
{
    register struct part *a;
    register struct part *b;
    struct rope *rope;                  /* [bp-2] */

    if ((rope = ROPE_PTR(part->rope_ptr)) != ROPE_NONE) {
        if ((a = PART_PTR(rope->end_a_ptr)) != PART_NONE) {
            a->flags_08 &= 0xfffd;
            a->start_flags = a->flags_08;
            a->rope_ptr = 0;
            rope->end_a_ptr = 0;
        }
        if ((b = PART_PTR(rope->end_b_ptr)) != PART_NONE) {
            b->flags_08 &= 0xfffd;
            b->start_flags = b->flags_08;
            b->rope_ptr = 0;
            rope->end_b_ptr = 0;
        }
        if (!(part->flags_06 & 0x800))
            detach_part_to_bin(part);
    }
}

/*
 * 0x052f5
 *
 * **Take a part off the belts it runs on**, and there are two slots, so the
 * whole body runs twice - +0x66 and +0x68.
 *
 * A belt record has *two* ends and each end knows which slot of its own part it
 * sits in: the first end's part is at +2 with its slot index in the byte at
 * +0xa, the second's at +4 with its index at +0xb. So letting an end go means
 * clearing three things - the part's slot, the belt's reference to the part, and
 * the pair of words at that part's +0x5a - and the two ends are not symmetrical
 * enough to share code, which is why the original writes them out separately.
 *
 * **`how` decides whether the first end is let go at all.** With it non-zero -
 * `remove_all_parts`, removing the belt outright - both ends go. With it zero -
 * `detach_part_to_bin`, taking some other part off - only the second end does, and the
 * first is left attached to whatever it was on.
 *
 * The first end walks a chain: while the next record is kind 7, four words at
 * its +0x5a and the word at +0x68 are cleared and the walk goes on through
 * +0x5a. So a run of kind-7 records hanging off a belt end is cleared with it,
 * and the walk stops at the first thing that is not one.
 *
 * The second end does one step instead of a walk, and only when `how` is zero:
 * `match_field_5a_5c` says which of the pair to clear. So the chain is followed
 * when the belt is going and a single link is cut when it is not, which is the
 * same asymmetry `how` sets up above.
 *
 * Both slots end by calling `detach_part_to_bin` on the part unless bit 11 of its +6 is
 * set - the same guard, and the same fall-through into the common path, that
 * `untie_rope` has.
 */
void detach_belt(struct part *part, uint16_t how)
{
    register struct belt *belt;
    register struct part *next;
    int16_t i;                          /* [bp-2] */
    uint16_t slot;                      /* [bp-4] */
    struct part *a;                     /* [bp-6] */
    struct part *b;                     /* [bp-8] */
    struct part *after;                 /* [bp-0xa] */

    for (i = 0; i < 2; i++) {
        if ((belt = BELT_PTR(part->belt_ptr[i])) != BELT_NONE) {
            if (how != 0 && (a = PART_PTR(belt->end_a_ptr)) != PART_NONE) {
                belt->home_a_ptr = belt->end_a_ptr = 0;
                slot = belt->slot_a;
                a->belt_ptr[slot] = 0;
                next = PART_PTR(a->link_ptr[slot]);
                a->link_ptr[slot] = a->link_ptr[slot + 2] = 0;
                while (next != PART_NONE && next->kind == KIND_PULLEY) {
                    after = PART_PTR(next->link_ptr[0]);
                    /* the outer loop's own counter: a pulley on the first
                       belt leaves it at 4, and the second is not looked at */
                    for (i = 0; i < 4; i++)
                        next->link_ptr[i] = 0;
                    next->belt_ptr[1] = 0;
                    next = after;
                }
            }
            if ((b = PART_PTR(belt->end_b_ptr)) != PART_NONE) {
                slot = belt->slot_b;
                b->belt_ptr[slot] = 0;
                belt->home_b_ptr = belt->end_b_ptr = 0;
                next = PART_PTR(b->link_ptr[slot]);
                b->link_ptr[slot] = b->link_ptr[slot + 2] = 0;
                if (next != PART_NONE && how == 0) {
                    slot = match_field_5a_5c(b, next);
                    next->link_ptr[slot] = next->link_ptr[slot + 2] = 0;
                }
            }
            if (!(part->flags_06 & 0x800))
                detach_part_to_bin(part);
        }
    }
}

/*
 * 0x05457
 *
 * **Discard a part - but only really in freeform mode.** Every path through
 * `finish_part_removal` ends here, and the rope and belt paths call it on what they
 * detached as well.
 *
 * The free is behind DGROUP 0x4e67, the freeform flag. In a level the part is
 * *not* unlinked and *not* freed: `detach_part_to_bin` has already put it in the bin at
 * 0x50d7, and that is where it stays, because a level's parts are the ones the
 * level came with and the player will want them back. In freeform the player
 * makes parts, so there they are unlinked and handed to `free_part`.
 *
 * So "removing a part" means two different things depending on the mode, and
 * this one word is the whole of the difference. Nothing above here knows about
 * it.
 *
 * Then, if the part was the current one at 0x50d5, that is forgotten - which is
 * why `remove_all_parts` clearing the same word after the call is belt and
 * braces rather than the only thing doing it.
 */
void discard_part(struct part *part)
{
    if (DG4E67.freeform != 0) {
        unlink_part(part);
        free_part(part);
    }

    if (part == PART_PTR(DG50D3.dragged_part_ptr))
        DG50D3.dragged_part_ptr = 0;
}

/*
 * 0x05482
 *
 * **Finish taking a part out**, on whatever DGROUP 0x50d5 points at. It takes
 * no argument, which is why `remove_all_parts` sets that word and clears it
 * again around the call.
 *
 * **It requires bit 11 of +6 to be set and leaves at once otherwise** - and that
 * is the bit `detach_part_to_bin` sets. So the two are a sequence and not alternatives:
 * detach first, which marks the part, then this. The guards in `untie_rope` and
 * `detach_belt` test the same bit the other way, to avoid detaching a part that
 * has already been through it.
 *
 * Three kinds of work, and which one depends on the part's kind at +4.
 *
 * A part of kind 7 is **spliced out of a chain rather than removed from it**.
 * Its two neighbours are at +0x5a and +0x5c; `match_field_5a_5c` asks each which
 * of its own slots points back, and each is then pointed at the other - so the
 * chain closes over the gap. Both writes are done twice, to `slot` and to
 * `slot + 2`, which is the pair that field is. Then any neighbour that is itself
 * kind 7 is told to rebuild, its four link words are cleared and its +0x68 with
 * them.
 *
 * A part that is not kind 7 or 0x0a has its two belt slots emptied - each with
 * `detach_belt(belt, 1)`, the outright form - and a rope, if it has one and is
 * not one, is untied first.
 *
 * Every path ends at `sub_05457` on the part itself, and the belt and rope paths
 * call it on what they detached as well. So that is what actually disposes of
 * one, and everything above it is about leaving the things it was attached to in
 * a consistent state first.
 */
void finish_part_removal(void)
{
    register int16_t i;
    register struct part *next;
    int16_t a;                          /* [bp-2] */
    int16_t b;                          /* [bp-4] */
    struct part *r;                     /* [bp-6] */
    struct part *belt;                  /* [bp-8] */
    struct part *other;                 /* [bp-0xa] */
    struct rope *rope;                  /* [bp-0xc] */
    struct belt *slot;                  /* [bp-0xe] */

    if (DG50D3.dragged_part_ptr != 0
        && (PART_PTR(DG50D3.dragged_part_ptr)->flags_06 & 0x800)) {
        if (PART_PTR(DG50D3.dragged_part_ptr)->flags_0a & 3)
            break_second_attachment(PART_PTR(DG50D3.dragged_part_ptr));

        rope = ROPE_PTR(PART_PTR(DG50D3.dragged_part_ptr)->rope_ptr);
        if (PART_PTR(DG50D3.dragged_part_ptr)->kind != KIND_BELT
            && rope != ROPE_NONE) {
            r = PART_PTR(rope->owner_ptr);
            untie_rope(r);
            discard_part(r);
        }

        if (PART_PTR(DG50D3.dragged_part_ptr)->kind == KIND_PULLEY) {
            if ((next = PART_PTR(PART_PTR(DG50D3.dragged_part_ptr)->link_ptr[0]))
                != PART_NONE) {
                a = match_field_5a_5c(PART_PTR(DG50D3.dragged_part_ptr), next);
                other = PART_PTR(PART_PTR(DG50D3.dragged_part_ptr)->link_ptr[1]);
                b = match_field_5a_5c(PART_PTR(DG50D3.dragged_part_ptr), other);
                next->link_ptr[a] = next->link_ptr[a + 2] = dg_near(dgroup, other);
                other->link_ptr[b] = other->link_ptr[b + 2] = dg_near(dgroup, next);
                if (next->kind == KIND_PULLEY) {
                    aim_link_at_bisector(next);
                    mark_part_shapes(next, 3);
                }
                if (other->kind == KIND_PULLEY) {
                    aim_link_at_bisector(other);
                    mark_part_shapes(other, 3);
                }
                mark_needs_refile(PART_PTR(BELT_PTR(PART_PTR(DG50D3.dragged_part_ptr)
                                                    ->belt_ptr[1])->owner_ptr), 2);
                for (i = 0; i < 4; i++)
                    PART_PTR(DG50D3.dragged_part_ptr)->link_ptr[i] = 0;
                PART_PTR(DG50D3.dragged_part_ptr)->belt_ptr[1] = 0;
            }
        } else if (PART_PTR(DG50D3.dragged_part_ptr)->kind != KIND_ROPE) {
            for (i = 0; i < 2; i++)
                if ((slot = BELT_PTR(PART_PTR(DG50D3.dragged_part_ptr)->belt_ptr[i]))
                    != BELT_NONE) {
                    belt = PART_PTR(slot->owner_ptr);
                    detach_belt(belt, 1);
                    discard_part(belt);
                }
        }
        discard_part(PART_PTR(DG50D3.dragged_part_ptr));
    }
}

/*
 * 0x05628
 *
 * Take a part out of the doubly linked list it is on: whatever its `prev_ptr`
 * names has its `next` word set to this part's `next_ptr`, and the next part
 * - if there is one - has its `prev_ptr` pointed back past it. Nothing is
 * written into the part itself, so it still points at both of its old
 * neighbours when this returns.
 *
 * For the first part on a list `prev_ptr` is the list's **head word** -
 * 0x50d7, 0x5179, 0x521b or `bin_list_ptr` - which `insert_sorted` files
 * there; the head is read as a part whose only field is `next_ptr`, which
 * is the original's own model: one `mov` for both.
 */
void unlink_part(struct part *part)
{
    PART_PTR(part->prev_ptr)->next_ptr = part->next_ptr;

    if (part->next_ptr != 0)
        PART_PTR(part->next_ptr)->prev_ptr = part->prev_ptr;
}

/*
 * 0x05646
 *
 * Insert a record into a doubly-linked list, threaded through the words at +0
 * (next) and +2 (previous). The walk holds a pointer to the *link cell* rather
 * than to a node - so it starts at the head variable itself and the insertion
 * is the same three assignments wherever it lands.
 *
 * **It sorts for exactly two lists and prepends for every other**, which the
 * name does not say and which is the more important half:
 *
 *   0x50d7 - ordered on the word at +0x20 of the kind entry (table 0xec6)
 *   0x5179 - ordered on the word at +0x02 of the kind entry (table 0xea8)
 *   anything else - inserted at the front, comparing nothing
 *
 * The machine's own parts are on **0x521b**, so they are prepended: a machine
 * read front to back comes out back to front, which is measurable in the file
 * a save produces. `read_list` in game.c said the opposite for a while,
 * because this header led with the word "sorted".
 *
 * The record's own key is computed once, before the walk; the 0x5179 case
 * recomputes both sides from the other table rather than reusing it.
 */
void insert_sorted(register struct part *rec, struct part *head)
{
    struct part *di;
    int16_t stop;
    int16_t kind;                       /* [bp-2] */
    int16_t kind2;                      /* [bp-4] */
    int16_t prio;                       /* [bp-6] */
    int16_t prio2;                      /* [bp-8] */

    kind = rec->kind;
    prio = PART_KINDS[kind].priority;
    stop = 0;
    di = head;
    while (!stop) {
        if (di->next_ptr == 0)
            stop = 1;
        else {
            kind2 = PART_PTR(di->next_ptr)->kind;
            prio2 = PART_KINDS[kind2].priority;
            if (head == &DG50D3.parts_bin)
                stop = prio < prio2;
            else if (head == &DG5179.moving_parts)
                stop = PART_KINDS[kind].weight < PART_KINDS[kind2].weight;
            else
                stop = 1;
        }
        if (!stop)
            di = PART_PTR(di->next_ptr);
    }

    rec->next_ptr = di->next_ptr;
    rec->prev_ptr = dg_near(dgroup, di);
    di->next_ptr = dg_near(dgroup, rec);
    if (rec->next_ptr != 0)
        PART_PTR(rec->next_ptr)->prev_ptr = dg_near(dgroup, rec);
}

/*
 * 0x05704
 *
 * **Detach a part from everything holding it, and put it back in the bin.**
 * The common path: `remove_all_parts` sends every kind but a rope and a belt
 * straight here, and `untie_rope` finishes by coming here too.
 *
 * **The detaching is skipped on two screens.** If the round's state at DGROUP
 * 0x4e69 is 8 or 7 *and* the screen's at 0x4e6b is 0x1000, everything below the
 * first branch is jumped over and only the last three lines run. Both
 * conditions, not either: the same round state on another screen still detaches.
 *
 * What it detaches from is two different things. A rope, if the part has one at
 * +0x54 and is not itself a rope - and it is the rope *record's* +2 that goes
 * to `untie_rope`, not this part, because that routine wants the rope. And up
 * to two belts, from the slots at +0x66 and +0x68, each holding a record whose
 * first word is the belt. Kinds 0x0a and 7 skip the belt loop, which is a belt
 * and whatever 7 is not looking for belts of their own.
 *
 * Then three things that always happen: bits 12 and 13 of +6 are cleared and
 * bit 11 set, the part is unlinked from wherever it was, and it is inserted
 * into the list at DGROUP 0x50d7. That list is the parts bin - the same word
 * `save_machine` zeroes so a dragged part is written down - so "removing" a
 * part is moving it back to where unused parts live, not destroying it.
 */
void detach_part_to_bin(register struct part *part)
{
    int16_t i;

    if (!((DG4E67.tool == 8 || DG4E67.tool == 7) && DG4E67.state == 0x1000)) {
        if (part->rope_ptr != 0 && part->kind != KIND_BELT)
            untie_rope(PART_PTR(ROPE_PTR(part->rope_ptr)->owner_ptr));
        if (part->kind != KIND_ROPE && part->kind != KIND_PULLEY)
            for (i = 0; i < 2; i++)
                if (part->belt_ptr[i] != 0)
                    detach_belt(PART_PTR(BELT_PTR(part->belt_ptr[i])->owner_ptr), 0);
    }
    part->flags_06 = (part->flags_06 & 0xcfff) | 0x800;
    unlink_part(part);
    insert_sorted(part, &DG50D3.parts_bin);
}

/*
 * 0x0578c
 *
 * Move a part between the two sorted lists, and mend the bin cursor if that
 * emptied the node it was sitting on.
 *
 * The part is unlinked, then bit 0x4000 of +6 picks which list it belongs in:
 * set means 0x521b and flag 0x2000, clear means 0x5179 and flag 0x1000. Both
 * arms clear 0x0800 first - `and 0xf7ff` - so the three bits are a state and
 * not an accumulation.
 *
 * The tail is the part worth reading. If the bin cursor at 0x50d3 is not the
 * head sentinel and the node it names has become empty, the cursor steps back
 * to that node's +2. Only one step: a run of empty nodes would leave it on the
 * second of them, and the original does not loop.
 */
void refile_part_list(register struct part *part)
{
    unlink_part(part);
    if (part->flags_06 & 0x4000) {
        part->flags_06 = (part->flags_06 & 0xf7ff) | 0x2000;
        insert_sorted(part, &DG521B.placed_parts);
    } else {
        part->flags_06 = (part->flags_06 & 0xf7ff) | 0x1000;
        insert_sorted(part, &DG5179.moving_parts);
    }
    if (PART_PTR(DG50D3.bin_list_ptr) != &DG50D3.parts_bin
        && PART_PTR(DG50D3.bin_list_ptr)->next_ptr == 0)
        DG50D3.bin_list_ptr = PART_PTR(DG50D3.bin_list_ptr)->prev_ptr;
}

/*
 * 0x057e6
 *
 * **Take out every part the player put there**, which is what "restart level"
 * asks for. Bit 15 of a part's +6 protects it: those are stepped over with
 * `pick_for_record` and left alone, so the level's own furniture survives and
 * only what was added goes.
 *
 * **A part that is taken out restarts the walk.** The removal path ends by
 * calling `pick_by_flag(0x3000)` again rather than walking on from where it
 * was, because taking a part out relinks the list under it - `pick_for_record`
 * would then be walking from a record that is no longer in it. The skip path,
 * which changes nothing, walks on normally. That asymmetry is the whole shape
 * of the loop and it is not an accident of the disassembly.
 *
 * Three ways out by kind, and the kinds are the ones `draw_machine` already
 * names: 8 is a rope and 0x0a is a belt, each with its own routine because each
 * is attached to two other parts rather than standing on its own; everything
 * else goes through one. Then the part is made the *current* one at DGROUP
 * 0x50d5 for the length of one call and put back to zero - the same word the
 * dragged part uses, borrowed to say "this one" to a routine that takes no
 * argument.
 */
void remove_all_parts(void)
{
    register struct part *si;

    for (si = pick_by_flag(0x3000); si != PART_NONE; ) {
        if (!(si->flags_06 & 0x8000)) {
            if (si->kind == KIND_BELT)
                untie_rope(si);
            else if (si->kind == KIND_ROPE)
                detach_belt(si, 1);
            else
                detach_part_to_bin(si);
            DG50D3.dragged_part_ptr = dg_near(dgroup, si);
            finish_part_removal();
            DG50D3.dragged_part_ptr = 0;
            si = pick_by_flag(0x3000);
        } else
            si = pick_for_record(si, 0x1000);
    }
}

/*
 * 0x05855
 *
 * Step through the **parts bin** by *kind*, and answer the record you land on.
 *
 * The bin is the doubly-linked list whose head word is DGROUP 0x50d7 - `next`
 * at +0, `prev` at +2, the kind at +4 - and 0x50d3 holds that head's own
 * address, which is why the two directions start differently: forwards takes
 * `[[0x50d3]]`, the first entry, and backwards takes `[0x50d3]` itself, the
 * head cell, and uses it as the sentinel the walk stops on.
 *
 * A step is a **run of equal kinds**, not one entry: the inner loop takes the
 * kind it starts on and skips every neighbour sharing it, so the bin's three
 * tools are three steps apart however many of each the level holds. That is
 * what makes this the right routine behind a region that offers one icon per
 * kind.
 *
 * The sign of `index` picks the direction and the two halves are not mirror
 * images. Forwards ends with one step back along `prev`, so it answers the
 * *last* record of the group it stopped after; backwards has no such step and
 * answers the record it stopped on. Transcribed as written rather than
 * folded together, because that asymmetry is the routine.
 *
 * The `or si,si / je` guard inside the forward run is dead - the test it jumps
 * to has just made it - and is kept because it is there.
 *
 * *The name is a reading*: what the caller means by the index is not written
 * down, only that region 4 passes its own +4 and that 0x50d7 is the bin.
 */
dg_near_t bin_part_at_index(int16_t index)
{
    register dg_near_t si;
    uint16_t di;
    int16_t n;

    if (index < 0) {
        n = 0;
        si = DG50D3.bin_list_ptr;
        while (n != index) {
            di = PART_PTR(si)->kind;
            while (PART_PTR(si) != &DG50D3.parts_bin && PART_PTR(si)->kind == di)
                if (PART_PTR(si) != &DG50D3.parts_bin)
                    si = PART_PTR(si)->prev_ptr;
            n--;
        }
    } else {
        n = 0;
        si = PART_PTR(DG50D3.bin_list_ptr)->next_ptr;
        while (n != index) {
            di = PART_PTR(si)->kind;
            while (si != 0 && PART_PTR(si)->kind == di)
                if (si != 0)
                    si = PART_PTR(si)->next_ptr;
            n++;
        }
        if (si != 0)
            si = PART_PTR(si)->prev_ptr;
    }
    return si;
}

/*
 * 0x058bb
 *
 * How far the parts bin can be scrolled forward - the position of its last
 * page, as a value for the cursor at DGROUP 0x50d3.
 *
 * It finds it by doing it: step five kind-groups at a time with
 * `bin_part_at_index` until the step answers nothing, keeping the last
 * position that worked, then **put 0x50d3 back where it was** and answer the
 * one it reached. The cursor is moved for real during the search and restored
 * afterwards, because the step routine reads it rather than taking it as an
 * argument - there is no way to ask the question without moving.
 *
 * Five is the page: the same five `bin_scroll_back` and `bin_scroll_forward`
 * move by, so the answer is a position those two can actually land on.
 */
dg_near_t bin_scroll_end(void)
{
    register dg_near_t si;
    dg_near_t saved;                     /* [bp-2] */
    dg_near_t last;                      /* [bp-4] */

    saved = DG50D3.bin_list_ptr;
    while (si = bin_part_at_index(5), si)
        DG50D3.bin_list_ptr = si;
    last = DG50D3.bin_list_ptr;
    DG50D3.bin_list_ptr = saved;
    return last;
}

/*
 * 0x058f3
 *
 * Say that a part and everything joined to it needs re-filing: the byte at
 * +0x14 is the countdown `step_and_draw_machine` reads, and this sets it on
 * the part and on the parts at the other end of its rope and its belts.
 *
 * Kind 0x31 does not take the mark itself - it draws nothing - and kind 7, the
 * pulley, passes it only through its second belt and stops there.
 *
 * What the rest do depends on the machine's state at DGROUP 0x4e6b. In 0x1000
 * a rope's endpoints are recomputed first and the far end is only marked if the
 * two are close enough to matter; otherwise it is marked outright. In 0x2000 a
 * belt is only marked if it was not already, and its geometry is refreshed;
 * outside that state both belts are marked and refreshed unconditionally.
 */
void mark_needs_refile(register struct part *part, int16_t n)
{
    register struct belt *si;
    int16_t i;                          /* [bp-2] */
    struct rope *rope;                  /* [bp-4] */

    if (part->kind != KIND_ANCHOR)
        part->redraw_count = n;

    if (part->kind == KIND_PULLEY) {
        if ((si = BELT_PTR(part->belt_ptr[1])) != BELT_NONE)
            PART_PTR(si->owner_ptr)->redraw_count = n;
    } else {
        if ((rope = ROPE_PTR(part->rope_ptr)) != ROPE_NONE) {
            if (DG4E67.state == 0x1000) {
                compute_link_endpoints(rope);
                if (rope_ends_close(rope))
                    PART_PTR(rope->owner_ptr)->redraw_count = n;
            } else
                PART_PTR(rope->owner_ptr)->redraw_count = n;
        }

        if (DG4E67.state == 0x2000) {
            if ((si = BELT_PTR(part->belt_ptr[0])) != BELT_NONE
                && !PART_PTR(si->owner_ptr)->redraw_count) {
                PART_PTR(si->owner_ptr)->redraw_count = n;
                refresh_link_geometry(si);
            }
            if ((si = BELT_PTR(part->belt_ptr[1])) != BELT_NONE
                && !PART_PTR(si->owner_ptr)->redraw_count) {
                PART_PTR(si->owner_ptr)->redraw_count = n;
                refresh_link_geometry(si);
            }
        } else
            for (i = 0; i < 2; i++)
                if ((si = BELT_PTR(part->belt_ptr[i])) != BELT_NONE) {
                    PART_PTR(si->owner_ptr)->redraw_count = n;
                    refresh_link_geometry(si);
                }
    }
}

/*
 * 0x059e4
 *
 * Copy a part, and give the copy its own of whatever the original only points
 * at.
 *
 * The fields are copied one at a time rather than as a block, and the ones
 * left out are as much of the transcription as the ones copied: the position
 * at +0x1e, the histories, the chain links and the shape list are all left at
 * the zeros `calloc_far` gives, so a copy starts nowhere and on no list
 * until the caller puts it somewhere.
 *
 * Three things are pointed at rather than held, and each is allocated afresh:
 * a rope's sub-object at +0x54 for kind 8, a belt's at +0x66 for kinds 7 and
 * 0x0a, and the connection points at +0x82 - as many as the kind's record says
 * at +0x1e, four bytes each, copied two words at a time. Each new block is
 * pointed back at the copy.
 *
 * **Any allocation failing frees the whole copy and answers zero**, and it does
 * it by a `jmp` back to one place that sets the flag - so a half-built copy
 * never escapes.
 */
struct part *clone_part(register struct part *part)
{
    register struct part *si;
    int16_t i;                          /* [bp-2] */
    uint16_t failed;                    /* [bp-4] */
    struct part_point *dst_pt;          /* [bp-6] */
    const struct part_point *src_pt;    /* [bp-8] */

    failed = 0;
    if ((si = (struct part *)(void *)calloc_far(1, sizeof(struct part))) == NULL) {
#ifndef __TURBOC__
        /* Ours: the host's refusal is NULL, and `free_part` below tests
           for offset 0, which is what the original's refusal already is */
        si = PART_NONE;
#endif
give_up:
        failed = 1;
    } else {
        si->kind = part->kind;
        si->flags_06 = part->flags_06;
        si->flags_08 = part->flags_08;
        si->flags_0a = part->flags_0a;
        si->form = part->form;
        si->form_prev = part->form_prev;
        si->form_prev2 = part->form_prev2;
        si->direction = part->direction;
        si->mirror_size = part->mirror_size;
        si->size[0] = part->size[0];
        si->set_size = part->set_size;

        if (si->kind == KIND_BELT) {
            if ((si->rope_ptr = dg_near(dgroup, calloc_far(1, sizeof(struct rope)))) == 0)
                goto give_up;
            ROPE_PTR(si->rope_ptr)->owner_ptr = dg_near(dgroup, si);
        }
        si->grab = part->grab;
        si->grab_size = part->grab_size;
        if (si->kind == KIND_ROPE || si->kind == KIND_PULLEY) {
            if ((si->belt_ptr[0] = dg_near(dgroup, calloc_far(1, sizeof(struct belt)))) == 0)
                goto give_up;
            BELT_PTR(si->belt_ptr[0])->owner_ptr = dg_near(dgroup, si);
        }
        si->attach[0] = part->attach[0];
        si->attach[1] = part->attach[1];

        if ((si->point_count = PART_KINDS[part->kind].point_count) != 0) {
            src_pt = POINTS(part->points_ptr);
            dst_pt = POINTS(si->points_ptr
                            = dg_near(dgroup, calloc_far(si->point_count, 4)));
            if (dst_pt == POINTS_NONE)
                goto give_up;
            for (i = 0; (int16_t)si->point_count > i; i++, dst_pt++, src_pt++)
                *dst_pt = *src_pt;
        }
        si->start_form = part->start_form;
        si->start_direction = part->start_direction;
        si->start_flags = part->start_flags;
    }

    if (failed) {
        free_part(si);
        return PART_NONE;
    }
    return si;
}

/*
 * 0x05b65
 *
 * Pick the first of three words that is both non-zero and enabled by its bit
 * in the argument: 0x2000 selects DGROUP 0x521b, 0x1000 selects 0x5179, and
 * 0x0800 selects 0x50d7. If none qualifies the answer is 0.
 *
 * The order is the priority, and each test is "the slot is filled **and** the
 * caller asked for it" - a slot that is empty is skipped even when its bit is
 * set.
 */
struct part *pick_by_flag(uint16_t flags)
{
    if (DG521B.placed_parts.next_ptr != 0 && (flags & 0x2000))
        return PART_PTR(DG521B.placed_parts.next_ptr);
    if (DG5179.moving_parts.next_ptr != 0 && (flags & 0x1000))
        return PART_PTR(DG5179.moving_parts.next_ptr);
    if (DG50D3.parts_bin.next_ptr != 0 && (flags & 0x0800))
        return PART_PTR(DG50D3.parts_bin.next_ptr);
    return PART_NONE;
}

/*
 * 0x05ba7
 *
 * Choose a value for a record: its own word at +0 if that is set, otherwise
 * one of the shared slots, chosen by the record's flag word at +6 together
 * with the caller's flags.
 *
 * The 0x2000 case defers to `pick_by_flag` at 0x05b65 with the caller's flags,
 * so the record decides *whether* to look and the caller decides *which* slot.
 * The 0x1000 case does not defer: it requires the caller's 0x800 as well, and
 * reads DGROUP 0x50d7 directly - the same slot `pick_by_flag`'s third case
 * reads, reached by a different route.
 */
struct part *pick_for_record(struct part *rec, uint16_t flags)
{
    if (rec->next_ptr != 0)
        return PART_PTR(rec->next_ptr);

    if ((int16_t)rec->flags_06 & 0x2000)
        return pick_by_flag(flags);

    if (((int16_t)rec->flags_06 & 0x1000) && (flags & 0x800))
        return PART_PTR(DG50D3.parts_bin.next_ptr);

    return PART_NONE;
}

/*
 * 0x05be4
 *
 * Work out where an object should be drawn - the pair at +0x2a and +0x2c -
 * from where it *is*, at +0x1e and +0x20, plus the hotspot its kind defines.
 *
 * The hotspot is two **signed** bytes in the array at +0x18 of the material
 * record, indexed by the object's +0xc; the original sign-extends each with
 * `cbw`, so a hotspot can pull the drawing up and left of the position as well
 * as down and right. A record with no such array leaves the position alone.
 *
 * Bits 0x10 and 0x20 of the object's +8 flip it horizontally and vertically,
 * and each axis is flipped independently. A flipped axis measures the hotspot
 * from the far edge instead: the object's own span at +0x40 or +0x42, less the
 * hotspot, less the extent at +0x44 or +0x46. Those extents are why
 * `set_object_extent` is called first - the flip cannot be computed without
 * them, and it is called before the record's array is even looked at.
 */
void place_object_for_draw(register struct part *obj)
{
    const struct point8 *hot;
    int16_t type;                       /* [bp-2] */
    uint16_t idx;                       /* [bp-4] */
    int16_t flags;                      /* [bp-6] */
    const struct part_kind *rec;        /* [bp-8] */

    type = obj->kind;
    rec = &PART_KINDS[type];
    obj->box[0].x = obj->pos[0].x;
    obj->box[0].y = obj->pos[0].y;
    idx = obj->form;
    flags = obj->flags_08;
    set_object_extent(obj);

    if ((hot = POINT_TABLE(rec->hotspots_ptr)) != POINT_TABLE(0)) {
        hot += idx;
        if (flags & 0x10)
            obj->box[0].x += obj->mirror_size.width - (int8_t)hot->x
                             - obj->size[0].width;
        else
            obj->box[0].x += (int8_t)hot->x;
        if (flags & 0x20)
            obj->box[0].y += obj->mirror_size.height - (int8_t)hot->y
                             - obj->size[0].height;
        else
            obj->box[0].y += (int8_t)hot->y;
    }
}

/*
 * 0x05c77
 *
 * Set an object's extent - the pair at +0x44 and +0x46 - from wherever its
 * kind keeps that information. There are five answers and they are tried in
 * order.
 *
 * Types 8 and 10 have no extent at all and get zeros. An object with bit 0x40
 * at +6 carries its own, already sized, at +0x50 and +0x52. Everything else
 * looks it up in the 0x3a-byte material record at DGROUP 0xea6 + 0x3a * type,
 * indexed by the object's +0xc.
 *
 * The record offers two different shapes and the first one present wins. +0x1a
 * is an array of four-byte pairs read directly, so the extent is two words
 * sitting next to each other. +0x14 is an array of *pointers*, and the extent
 * is then at +6 and +8 of whatever each one names - one more indirection, and
 * a different offset within the target. A record with neither gets zeros.
 *
 * The original recomputes `+0x1a + 4 * +0xc` for each of the two words rather
 * than keeping it; that is the compiler, not a second read of anything that
 * could have changed.
 */
void set_object_extent(register struct part *obj)
{
    const struct part_kind *rec;
    struct bitmap *target;
    int16_t type;                       /* [bp-2] */

    if (obj->kind == KIND_BELT || obj->kind == KIND_ROPE)
        obj->size[0].width = obj->size[0].height = 0;
    else if (obj->flags_06 & 0x40) {
        obj->size[0].width = obj->set_size.width;
        obj->size[0].height = obj->set_size.height;
    } else {
        type = obj->kind;
        rec = &PART_KINDS[type];
        if (rec->sizes_ptr != 0) {
            obj->size[0].width = POINT16_TABLE(rec->sizes_ptr)[obj->form].x;
            obj->size[0].height = POINT16_TABLE(rec->sizes_ptr)[obj->form].y;
        } else if (rec->bitmaps != 0) {
            target = rec->bitmaps[obj->form];
            obj->size[0].width = target->width;
            obj->size[0].height = target->height;
        } else
            obj->size[0].width = obj->size[0].height = 0;
    }
}

/*
 * 0x05d1e
 *
 * Finish a part's connection points: give each one the *angle* to the next.
 *
 * The setups above leave an x and a y in the first two bytes of each
 * four-byte slot. This walks them in a ring - each to the one after it, and the
 * last back to the first - and puts `0xc000 - atan2(dy, dx)` in the slot's word
 * at +2. That is a quarter turn minus the angle, which is the game's angles
 * measured the other way round.
 *
 * The pair is handed to `step_pair_apart` before the angle is taken, which is
 * what stops two points at the same place from asking for the angle of a
 * zero-length line.
 *
 * The loop runs `count - 1` times and the last pair is done afterwards rather
 * than by wrapping the index, which is why the tail repeats the body.
 */
void part_finish_angles(register struct part *part)
{
    register struct part_point *si;
    int16_t n;                          /* [bp-2] */
    int16_t dx;                         /* [bp-4] */
    int16_t dy;                         /* [bp-6] */
    int16_t pair[4];                    /* [bp-0xe]: x0, y0, x1, y1 */

    for (n = 1, si = POINTS(part->points_ptr); (int16_t)part->point_count > n;
         n++, si++) {
        pair[0] = si->x;
        pair[1] = si->y;
        pair[2] = si[1].x;
        pair[3] = si[1].y;
        step_pair_apart(pair);
        dx = pair[2] - pair[0];
        dy = pair[3] - pair[1];
        si->angle = 0xc000 - atan2_long(dx, dy);
    }

    pair[0] = si->x;
    pair[1] = si->y;
    pair[2] = POINTS(part->points_ptr)->x;
    pair[3] = POINTS(part->points_ptr)->y;
    step_pair_apart(pair);
    dx = pair[2] - pair[0];
    dy = pair[3] - pair[1];
    si->angle = 0xc000 - atan2_long(dx, dy);
}

/*
 * 0x05dfc
 *
 * **Every shape on the drawn list back on the free list**: walk the list at
 * DGROUP 0x4e52 to its last node, point that at the free list at 0x4e4e, make
 * the whole list the free list's head, and empty 0x4e52. The far-pointer
 * twin of `splice_list_4e58_onto_4e56`.
 *
 * **Nothing in the image calls it**: no near call from segment 0000, and no
 * far call to it anywhere. It is transcribed because the module holds it.
 * The name is ours.
 */
void free_all_shapes(void)
{
    struct shape far *next;             /* [bp-4] */
    struct shape far *q;                /* [bp-8] */

    if (DG4E4E.shapes) {
        q = DG4E4E.shapes;
        next = q->next;
        while (next) {
            q = next;
            next = next->next;
        }
        q->next = DG4E4E.shape_free;
        DG4E4E.shape_free = DG4E4E.shapes;
        DG4E4E.shapes = 0;
    }
}

/*
 * 0x05e70
 *
 * Register the shapes for everything a part is joined to.
 *
 * A pulley, kind 7, only has its second belt done. A rope, kind 8, and a belt,
 * kind 0x0a, are not asked at all - they are the things being registered, not
 * the things that hold them. Everything else does its rope, unless the machine
 * is in state 0x2000, and then both of its belts.
 *
 * Each belt is reached through the *part* its record names at +0, not through
 * this one, so the shapes come out in the belt's own terms.
 */
void mark_joined_shapes(register struct part *part, uint16_t mode)
{
    struct belt *di;
    struct rope *rope;                  /* [bp-2] */

    if (part->kind == KIND_PULLEY) {
        if ((di = BELT_PTR(part->belt_ptr[1])) != BELT_NONE)
            mark_belt_shapes(PART_PTR(di->owner_ptr), mode);
    } else if (part->kind != KIND_BELT && part->kind != KIND_ROPE) {
        if (DG4E67.state != 0x2000
            && (rope = ROPE_PTR(part->rope_ptr)) != ROPE_NONE)
            add_sub_object_shapes(PART_PTR(rope->owner_ptr), mode);
        if ((di = BELT_PTR(part->belt_ptr[0])) != BELT_NONE)
            mark_belt_shapes(PART_PTR(di->owner_ptr), mode);
        if ((di = BELT_PTR(part->belt_ptr[1])) != BELT_NONE)
            mark_belt_shapes(PART_PTR(di->owner_ptr), mode);
    }
}

/*
 * 0x05ef6
 *
 * Add shape records for the point pairs held by an object's sub-object at
 * +0x54, choosing which generation by the caller's mask.
 *
 * Bit 0 asks for the pairs at +0x28/+0x2c and +0x30/+0x34; bit 1 for those at
 * +0x18/+0x1c and +0x20/+0x24. Those are exactly the generations
 * `shift_state_history` ages on a type 8 object's +0x54 sub-object - four
 * 32-bit chains at +8, +0xc, +0x10 and +0x14 whose generations are 0x10 apart -
 * so bit 0 selects two steps ago and bit 1 one step ago. The two routines agree
 * about that layout, which is worth stating because the offsets alone look
 * arbitrary.
 *
 * Both bits may be set, giving four shapes. Every one is added with flags 4 and
 * zero width; only the `which` argument differs, 1 for the older generation and
 * 2 for the newer.
 */
void add_sub_object_shapes(struct part *obj, int16_t mask)
{
    struct rope *sub = ROPE_PTR(obj->rope_ptr);

    if ((mask & 1) != 0) {
        alloc_shape((const uint8_t *)&sub->pt[2][0], (const uint8_t *)&sub->pt[2][1], 4, 1, 0);
        alloc_shape((const uint8_t *)&sub->pt[2][2], (const uint8_t *)&sub->pt[2][3], 4, 1, 0);
    }

    if ((mask & 2) != 0) {
        alloc_shape((const uint8_t *)&sub->pt[1][0], (const uint8_t *)&sub->pt[1][1], 4, 2, 0);
        alloc_shape((const uint8_t *)&sub->pt[1][2], (const uint8_t *)&sub->pt[1][3], 4, 2, 0);
    }
}

/*
 * 0x05f87
 *
 * Register the rectangles a belt covers, so what it drew can be erased again.
 *
 * The belt is the record at the part's +0x66. Two shapes come out of each
 * length: the line itself, given to `alloc_shape` as its two endpoints and the
 * slack `link_slack` measured, and a 16 by 16 box at each of the two points the
 * belt is fastened at. The mode's bit 0 does the first side of the belt and
 * bit 1 the second, and they are written out separately rather than looped
 * because each takes a different pair of fields.
 *
 * Which fields depends on the part at the far end: a pulley, kind 7, keeps the
 * tangent points in its own belt record, and anything else keeps them in this
 * one.
 *
 * The whole thing is written twice. In state 0x2000 - DGROUP 0x4e6b - only the
 * two ends of this belt are done, because that state moves one part at a time.
 * Outside it the chain of pulleys is walked to its end, so a belt over three
 * wheels registers every length of itself.
 */
void mark_belt_shapes(struct part *part, uint16_t mode)
{
    register struct belt *si;
    register int16_t di;
    int16_t slack;                      /* [bp-2] */
    int16_t corner[2];                  /* [bp-6] */
    int16_t box[2];                     /* [bp-0xa]: 0x10 square */
    struct point16 *near_pt;            /* [bp-0xc] */
    struct point16 *far_pt;             /* [bp-0xe] */
    struct part *near_part;             /* [bp-0x10] */
    struct part *far_part;              /* [bp-0x12] */

    si = BELT_PTR(part->belt_ptr[0]);
    box[0] = 0x10;
    box[1] = 0x10;

    if (DG4E67.state == 0x2000) {
        near_part = PART_PTR(si->end_a_ptr);
        far_part = PART_PTR(near_part->link_ptr[si->slot_a]);
        if (mode & 1) {
            far_pt = far_part->kind == KIND_PULLEY
                     ? &BELT_PTR(far_part->belt_ptr[0])->pt[2][0] : &si->pt[2][1];
            slack = link_slack(near_part, si, 1);
            alloc_shape((const uint8_t *)&si->pt[2][0], (const uint8_t *)far_pt,
                        4, 1, slack);
            for (di = 0; di < 2; di++) {
                corner[0] = si->pt[2][di].x - 8;
                corner[1] = si->pt[2][di].y - 8;
                alloc_shape((const uint8_t *)corner, (const uint8_t *)box, 1, 1, 0);
            }
        }
        if (mode & 2) {
            far_pt = far_part->kind == KIND_PULLEY
                     ? &BELT_PTR(far_part->belt_ptr[0])->pt[1][0] : &si->pt[1][1];
            slack = link_slack(near_part, si, 2);
            alloc_shape((const uint8_t *)&si->pt[1][0], (const uint8_t *)far_pt,
                        4, 2, slack);
            for (di = 0; di < 2; di++) {
                corner[0] = si->pt[2][di].x - 8;
                corner[1] = si->pt[2][di].y - 8;
                alloc_shape((const uint8_t *)corner, (const uint8_t *)box, 1, 2, 0);
            }
        }

        if (PART_PTR(si->end_b_ptr) != far_part) {
            far_part = PART_PTR(si->end_b_ptr);
            near_part = PART_PTR(far_part->link_ptr[si->slot_b]);
            if (mode & 1) {
                near_pt = near_part->kind == KIND_PULLEY
                          ? &BELT_PTR(near_part->belt_ptr[0])->pt[2][1] : &si->pt[2][0];
                slack = link_slack(near_part, si, 1);
                alloc_shape((const uint8_t *)near_pt, (const uint8_t *)&si->pt[2][1],
                            4, 1, slack);
                for (di = 0; di < 2; di++) {
                    corner[0] = si->pt[2][di].x - 8;
                    corner[1] = si->pt[2][di].y - 8;
                    alloc_shape((const uint8_t *)corner, (const uint8_t *)box, 1, 1, 0);
                }
            }
            if (mode & 2) {
                /* the far end's kind decides, and the near end's belt is read */
                near_pt = far_part->kind == KIND_PULLEY
                          ? &BELT_PTR(near_part->belt_ptr[0])->pt[1][1] : &si->pt[1][0];
                slack = link_slack(near_part, si, 2);
                alloc_shape((const uint8_t *)near_pt, (const uint8_t *)&si->pt[1][1],
                            4, 2, slack);
                for (di = 0; di < 2; di++) {
                    corner[0] = si->pt[2][di].x - 8;
                    corner[1] = si->pt[2][di].y - 8;
                    alloc_shape((const uint8_t *)corner, (const uint8_t *)box, 1, 2, 0);
                }
            }
        }
    } else {
        if (mode & 1) {
            near_part = PART_PTR(si->end_a_ptr);
            far_part = PART_PTR(near_part->link_ptr[si->slot_a]);
            while (near_part != PART_NONE && far_part != PART_NONE) {
                near_pt = near_part->kind == KIND_PULLEY
                          ? &BELT_PTR(near_part->belt_ptr[0])->pt[2][1] : &si->pt[2][0];
                far_pt = far_part->kind == KIND_PULLEY
                         ? &BELT_PTR(far_part->belt_ptr[0])->pt[2][0] : &si->pt[2][1];
                slack = link_slack(near_part, si, 1);
                alloc_shape((const uint8_t *)near_pt, (const uint8_t *)far_pt,
                            4, 1, slack);
                near_part = far_part;
                if (near_part->kind != KIND_PULLEY)
                    far_part = PART_NONE;
                else
                    far_part = PART_PTR(far_part->link_ptr[0]);
            }
            for (di = 0; di < 2; di++) {
                corner[0] = si->pt[2][di].x - 8;
                corner[1] = si->pt[2][di].y - 8;
                alloc_shape((const uint8_t *)corner, (const uint8_t *)box, 1, 1, 0);
            }
        }
        if (mode & 2) {
            near_part = PART_PTR(si->end_a_ptr);
            far_part = PART_PTR(near_part->link_ptr[si->slot_a]);
            while (near_part != PART_NONE && far_part != PART_NONE) {
                near_pt = near_part->kind == KIND_PULLEY
                          ? &BELT_PTR(near_part->belt_ptr[0])->pt[1][1] : &si->pt[1][0];
                far_pt = far_part->kind == KIND_PULLEY
                         ? &BELT_PTR(far_part->belt_ptr[0])->pt[1][0] : &si->pt[1][1];
                slack = link_slack(near_part, si, 2);
                alloc_shape((const uint8_t *)near_pt, (const uint8_t *)far_pt,
                            4, 2, slack);
                near_part = far_part;
                if (near_part->kind != KIND_PULLEY)
                    far_part = PART_NONE;
                else
                    far_part = PART_PTR(far_part->link_ptr[0]);
            }
            for (di = 0; di < 2; di++) {
                corner[0] = si->pt[1][di].x - 8;
                corner[1] = si->pt[1][di].y - 8;
                alloc_shape((const uint8_t *)corner, (const uint8_t *)box, 1, 2, 0);
            }
        }
    }
}

/*
 * 0x0642a
 *
 * Add one or both of a record's two shapes, selected by bits 0 and 1 of the
 * argument.
 *
 * The two calls differ only in which pair of points they take - +0x32 with
 * +0x4c, or +0x2e with +0x48 - and in the `which` byte they pass, 1 or 2, which
 * is what makes `alloc_shape` choose between the two origins. The flags byte is
 * 1 both times, so bit 2 is clear and both shapes are the single-point kind.
 */
void add_record_shapes(struct part *rec, uint16_t which)
{
    if (which & 1)
        alloc_shape((const uint8_t *)&rec->box[2],
                    (const uint8_t *)&rec->size[2], 1, 1, 0);
    if (which & 2)
        alloc_shape((const uint8_t *)&rec->box[1],
                    (const uint8_t *)&rec->size[1], 1, 2, 0);
}

/*
 * 0x0647f
 *
 * Register a part's shapes, by kind: a rope, kind 8, through
 * `add_sub_object_shapes`; a belt, kind 0x0a, through `mark_belt_shapes`;
 * everything else through `add_record_shapes`. Three lines and a dispatch, and
 * fifteen callers.
 */
void mark_part_shapes(register struct part *part, register uint16_t mode)
{
    if (part->kind == KIND_BELT)
        add_sub_object_shapes(part, mode);
    else if (part->kind == KIND_ROPE)
        mark_belt_shapes(part, mode);
    else
        add_record_shapes(part, mode);
}

/*
 * 0x064b4
 *
 * Take a node off the free list at DGROUP 0x4e4e, link it onto the list at
 * 0x4e52, and fill it in as a shape between two points.
 *
 * Both lists are **far** pointers and the node's own link is its first four
 * bytes, so the pop and the push are the same three moves through `les`. If
 * the free list was empty - both halves zero - the routine returns having
 * already done the pushing, which leaves the used list pointing at a null
 * node. That is what it does.
 *
 * The two points arrive as near pointers to word pairs and land at +6/+8 and
 * +0xa/+0xc. Both are then shifted by an origin: one pair when the byte
 * argument is 1 and a different pair otherwise, and the second point only when
 * bit 2 of the flags is set - so a shape with that bit uses two points and
 * without it only one.
 *
 * Finally the bounds at +0x10..+0x16. With bit 2 set they are the **ordered**
 * minimum and maximum of the two points, and the last one has half of the
 * width at +0xe added to it - only that one. Without bit 2 the second point is
 * treated as an extent and simply added to the first.
 *
 * The compiler reloads the node pointer with `les bx, [bp-4]` before every
 * single field access - thirty-odd times - which is transcribed as one local
 * because nothing can change it in between.
 */
void alloc_shape(const uint8_t *pt1, const uint8_t *pt2,
                 uint8_t flags, uint8_t which, int16_t width)
{
    struct shape far *n;                /* [bp-4] */

    n = DG4E4E.shape_free;
    /* Pop from the free list, push onto the used list - before the test for
       an empty list, as the original does it, which then reads and writes
       0000:0000. The host's null is C's and cannot be followed, so there the
       test comes first; ours. */
#ifndef __TURBOC__
    if (n == NULL)
        return;
#endif
    DG4E4E.shape_free = DG4E4E.shape_free->next;
    n->next = DG4E4E.shapes;
    DG4E4E.shapes = n;

    if (n) {
        n->flags = flags;
        n->replays = which;
        *(struct point16 far *)&n->x1 = *(const struct point16 *)pt1;
        *(struct point16 far *)&n->x2 = *(const struct point16 *)pt2;
        n->width = width;
        if (which == 1) {
            n->x1 -= DG4E67.origin_c_x;
            n->y1 -= DG4E67.origin_c_y;
            if (flags & 4) {
                n->x2 -= DG4E67.origin_c_x;
                n->y2 -= DG4E67.origin_c_y;
            }
        } else {
            n->x1 -= DG4E67.origin_b_x;
            n->y1 -= DG4E67.origin_b_y;
            if (flags & 4) {
                n->x2 -= DG4E67.origin_b_x;
                n->y2 -= DG4E67.origin_b_y;
            }
        }

        if (n->flags & 4) {
            if (n->x1 < n->x2) {
                n->left = n->x1;
                n->right = n->x2;
            } else {
                n->left = n->x2;
                n->right = n->x1;
            }
            if (n->y1 < n->y2) {
                n->top = n->y1;
                n->bottom = n->y2;
            } else {
                n->top = n->y2;
                n->bottom = n->y1;
            }
            n->bottom += n->width >> 1;
        } else {
            n->left = n->x1;
            n->top = n->y1;
            n->right = n->left + n->x2;
            n->bottom = n->top + n->y2;
        }
    }
}

/*
 * 0x06699
 *
 * Put back what was drawn over: walk the shape list at DGROUP 0x4e52, step each
 * record's `replays` at +5, and act on the ones that have run out.
 *
 * A record with bit 2 of +4 is a belt length and is redrawn by
 * `draw_belt_segment`; everything else is a rectangle filled in the background
 * colour, clipped against the window at 0x3894 first and with bit 0 of +4
 * deciding the second fill colour. A rectangle whose far edge is exactly on the
 * window's is pulled in by one, which is the original's own fencepost and not
 * an approximation of one.
 *
 * A record that is used up is unlinked and put back on the free list at 0x4e4e;
 * one that is not becomes the new predecessor, which is how the walk keeps the
 * single-linked list stitched.
 */
void replay_shapes(void)
{
    register int16_t si;
    register int16_t di;
    int16_t a;                          /* [bp-2] */
    int16_t b;                          /* [bp-4] */
    int16_t c;                          /* [bp-6] */
    struct shape far *cur;              /* [bp-0xa] */
    struct shape far *next;             /* [bp-0xe] */
    struct shape far *prev;             /* [bp-0x12] */

    set_clip_for_mode();
    VMDS.clip_enabled = 1;
    VMDS.fill_colour = VMDS.second_colour = DG52BD.fill_colour;
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    prev = 0;
    for (cur = DG4E4E.shapes; cur; cur = next) {
        next = cur->next;
        if (--cur->replays == 0) {
            si = cur->x1;
            di = cur->y1;
            a = cur->x2;
            b = cur->y2;
            c = cur->width;
            cursor_redraw_off_thunk();
            if (cur->flags & 4)
                draw_belt_segment(si, di, a, b, c);
            else {
                VMDS.fill_enabled = cur->flags & 1;
                if (di == VMDS.clip_bottom)
                    di--;
                if (si == VMDS.clip_right)
                    si--;
                if (si < VMDS.clip_right && (int16_t)(si + a) > VMDS.clip_left
                    && di < VMDS.clip_bottom && (int16_t)(di + b) > VMDS.clip_top)
                    fill_rect(si, di, a, b);
            }
            restore_cursor_following();
            if (prev)
                prev->next = next;
            else
                DG4E4E.shapes = next;
            cur->next = DG4E4E.shape_free;
            DG4E4E.shape_free = cur;
        } else
            prev = cur;
    }
}

/*
 * 0x06806
 *
 * Which parts have to be redrawn: walk the 0x3000 list and mark every one that
 * lies in a rectangle on the list at DGROUP 0x4e52.
 *
 * A part already marked - the byte at +0x14 - or hidden is skipped. A belt goes
 * to `belt_in_dirty_rect`, which has to walk its lengths. A rope has no box of
 * its own and gets one from the four corners its record keeps at +8 through
 * +0x16, taking the smaller of each pair as the origin - the same construction
 * `refile_overlapping_parts` makes, and skipped entirely unless its ends are
 * close and, in state 9 with one end being dragged, the pointer is still in the
 * play area. Everything else uses its own box at +0x2a and +0x44.
 */
void mark_parts_in_dirty_rects(void)
{
    register struct rope *si;
    register struct part *di;
    int16_t left;                       /* [bp-2] */
    int16_t top;                        /* [bp-4] */
    int16_t right;                      /* [bp-6] */
    int16_t bottom;                     /* [bp-8] */
    struct shape far *node;             /* [bp-0xc] */

    for (di = pick_by_flag(0x3000); di != PART_NONE;
         di = pick_for_record(di, 0x1000)) {
        if (!di->redraw_count && !(di->flags_08 & 0x2000)) {
            if (di->kind == KIND_ROPE) {
                belt_in_dirty_rect(di);
                continue;
            }

            if (di->kind == KIND_BELT) {
                si = ROPE_PTR(di->rope_ptr);
                if (!rope_ends_close(si))
                    continue;
                if (DG4E67.tool == 9
                    && (si->end_a_ptr == DG50D3.dragged_part_ptr
                        || si->end_b_ptr == DG50D3.dragged_part_ptr)
                    && !point_in_play_area())
                    continue;
                if (si->pt[0][0].x < si->pt[0][1].x) {
                    right = left = si->pt[0][0].x - DG4E67.origin_x;
                    right += si->pt[0][3].x - si->pt[0][0].x;
                } else {
                    right = left = si->pt[0][1].x - DG4E67.origin_x;
                    right += si->pt[0][2].x - si->pt[0][1].x;
                }
                /* origin_x on the y axis too, as the original has it */
                if (si->pt[0][0].y < si->pt[0][1].y) {
                    bottom = top = si->pt[0][0].y - DG4E67.origin_x;
                    bottom += si->pt[0][3].y - si->pt[0][0].y;
                } else {
                    bottom = top = si->pt[0][1].y - DG4E67.origin_x;
                    bottom += si->pt[0][2].y - si->pt[0][1].y;
                }
            } else {
                left = di->box[0].x - DG4E67.origin_x;
                top = di->box[0].y - DG4E67.origin_y;
                right = left + di->size[0].width;
                bottom = top + di->size[0].height;
            }

            node = DG4E4E.shapes;
            while (node) {
                if (node->left < right && node->right > left
                    && node->top < bottom && node->bottom > top) {
                    mark_needs_refile(di, 1);
                    node = 0;
                } else
                    node = node->next;
            }
        }
    }
}

/*
 * 0x06994
 *
 * A belt's version of the dirty-rectangle test: walk the belt from pulley to
 * pulley and mark the *part* if any length of it lies in a rectangle that has
 * to be redrawn.
 *
 * Each length runs between two of the belt's fastening points - the pairs of
 * bytes at +0x6a - and its box is the two points made into a rectangle, in
 * screen coordinates. `link_slack` adds half its slack to the bottom, because a
 * sagging belt reaches below the straight line between its ends.
 *
 * The rectangles are the far-pointer list at DGROUP 0x4e52; the first that
 * overlaps marks the part and ends the walk, which is what setting `si` to the
 * belt's far end does.
 */
void belt_in_dirty_rect(struct part *part)
{
    register struct part *si;
    register struct part *di;
    int16_t ax;                         /* [bp-2] */
    int16_t ay;                         /* [bp-4] */
    int16_t bx;                         /* [bp-6] */
    int16_t by;                         /* [bp-8] */
    int16_t left;                       /* [bp-0xa] */
    int16_t top;                        /* [bp-0xc] */
    int16_t right;                      /* [bp-0xe] */
    int16_t bottom;                     /* [bp-0x10] */
    int16_t slack;                      /* [bp-0x12] */
    uint16_t slotA;                     /* [bp-0x14] */
    uint16_t slotB;                     /* [bp-0x16] */
    struct part *endA;                  /* [bp-0x18] */
    struct part *endB;                  /* [bp-0x1a] */
    struct belt *belt;                  /* [bp-0x1c] */
    struct shape far *node;             /* [bp-0x20] */

    belt = BELT_PTR(part->belt_ptr[0]);
    di = endA = PART_PTR(belt->end_a_ptr);
    endB = PART_PTR(belt->end_b_ptr);
    slotA = belt->slot_a;
    slotB = 0;
    si = PART_PTR(di->link_ptr[slotA]);
    slack = link_slack(di, belt, 3);

    while (di != PART_NONE && si != PART_NONE) {
        if (di != endA) {
            slotA = 1;
            slack = 0;
        }
        ax = di->box[0].x + di->attach[slotA].x;
        ay = di->box[0].y + di->attach[slotA].y;
        if (si == endB) {
            slotB = belt->slot_b;
            slack = link_slack(di, belt, 3);
        }
        bx = si->box[0].x + si->attach[slotB].x;
        by = si->box[0].y + si->attach[slotB].y;

        if (ax < bx) {
            left = ax - DG4E67.origin_x;
            right = bx - DG4E67.origin_x;
        } else {
            left = bx - DG4E67.origin_x;
            right = ax - DG4E67.origin_x;
        }
        if (ay < by) {
            top = ay - DG4E67.origin_y;
            bottom = by - DG4E67.origin_y;
        } else {
            top = by - DG4E67.origin_y;
            bottom = ay - DG4E67.origin_y;
        }
        if (slack > 0)
            bottom += slack >> 1;

        node = DG4E4E.shapes;
        while (node) {
            if (node->left < right && node->right > left
                && node->top < bottom && node->bottom > top) {
                mark_needs_refile(part, 1);
                node = 0;
                si = endB;
            } else
                node = node->next;
        }

        if (si == endB)
            di = si = PART_NONE;
        else {
            di = si;
            si = PART_PTR(si->link_ptr[0]);
        }
    }
}

/*
 * 0x06b5b
 *
 * Re-file every part that overlaps one already in the display buckets.
 *
 * The six lists at DGROUP 0x50bf are the drawing order, and each is a tree
 * walked by the byte at +0x7f: equal to the level takes the child at +0x74,
 * anything else the one at +0x76. For each part on a level, every other part
 * whose box overlaps its box is handed to `link_record_into_buckets`, so
 * anything sitting under something about to be drawn is drawn too.
 *
 * Two bytes of the kind's record decide whether a part takes part at this
 * level at all: +0x1c and +0x1d, both compared unsigned, with 0xff meaning
 * "always" and a value of 2 or less meaning "at every level". A part being
 * dragged or hidden - bit 5 of +0x0a, or bit 13 of +8 - is skipped, and so are
 * kinds 0x0a and 0x31, which are the belt and the one that draws nothing.
 *
 * A rope, kind 8, has no box of its own: its extent is worked out from the
 * four corners its record holds at +8..+0x16, taking whichever of each pair is
 * the smaller as the origin. It is also skipped entirely unless its ends are
 * close - `rope_ends_close` - and, while the machine is in state 9 with one of
 * its ends being dragged, unless the pointer is still in the play area.
 */
void refile_overlapping_parts(void)
{
    register struct rope *si;
    register struct part *di;
    uint8_t level_n;                    /* [bp-1] the counter */
    uint8_t level;                      /* [bp-2] */
    int16_t x0;                         /* [bp-4] */
    int16_t y0;                         /* [bp-6] */
    int16_t x1;                         /* [bp-8] */
    int16_t y1;                         /* [bp-0xa] */
    int16_t dx0;                        /* [bp-0xc] */
    int16_t dy0;                        /* [bp-0xe] */
    int16_t dx1;                        /* [bp-0x10] */
    int16_t dy1;                        /* [bp-0x12] */
    struct part *walk;                  /* [bp-0x14] */
    const struct part_kind *rec;        /* [bp-0x16] */

    for (level_n = 6; level_n > 0; level_n--) {
        level = level_n - 1;
        walk = PART_PTR(DG50BF.layer_head_ptr[level]);
        while (walk != PART_NONE) {
            rec = &PART_KINDS[walk->kind];
            if ((rec->refile_level[0] == 0xff || rec->refile_level[0] >= level
                 || rec->refile_level[0] <= 2)
                && (rec->refile_level[0] == 0xff || rec->refile_level[1] >= level
                    || rec->refile_level[1] <= 2)) {
                x0 = walk->box[0].x;
                y0 = walk->box[0].y;
                x1 = x0 + walk->size[0].width;
                y1 = y0 + walk->size[0].height;

                for (di = pick_by_flag(0x3000); di != PART_NONE;
                     di = pick_for_record(di, 0x1000)) {
                    if ((di->flags_0a & 0x20) || (di->flags_08 & 0x2000))
                        continue;
                    if (di->kind == KIND_ROPE || di->kind == KIND_ANCHOR)
                        continue;
                    rec = &PART_KINDS[di->kind];
                    if (rec->refile_level[0] > level && rec->refile_level[0] != 0xff)
                        continue;
                    if (rec->refile_level[1] > level && rec->refile_level[1] != 0xff)
                        continue;

                    if (di->kind == KIND_BELT) {
                        si = ROPE_PTR(di->rope_ptr);
                        if (!rope_ends_close(si))
                            continue;
                        if (DG4E67.tool == 9
                            && (si->end_a_ptr == DG50D3.dragged_part_ptr
                                || si->end_b_ptr == DG50D3.dragged_part_ptr)
                            && !point_in_play_area())
                            continue;
                        if (si->pt[0][0].x < si->pt[0][1].x) {
                            dx1 = dx0 = si->pt[0][0].x;
                            dx1 += si->pt[0][3].x - si->pt[0][0].x;
                        } else {
                            dx1 = dx0 = si->pt[0][1].x;
                            dx1 += si->pt[0][2].x - si->pt[0][1].x;
                        }
                        if (si->pt[0][0].y < si->pt[0][1].y) {
                            dy1 = dy0 = si->pt[0][0].y;
                            dy1 += si->pt[0][3].y - si->pt[0][0].y;
                        } else {
                            dy1 = dy0 = si->pt[0][1].y;
                            dy1 += si->pt[0][2].y - si->pt[0][1].y;
                        }
                    } else {
                        dx0 = di->box[0].x;
                        dy0 = di->box[0].y;
                        dx1 = dx0 + di->size[0].width;
                        dy1 = dy0 + di->size[0].height;
                    }

                    if (dx0 < x1 && dx1 > x0 && dy0 < y1 && dy1 > y0)
                        link_record_into_buckets(di);
                }
            }
            if (walk->layer_slot == level)
                walk = PART_PTR(walk->layer_next_ptr[0]);
            else
                walk = PART_PTR(walk->layer_next_ptr[1]);
        }
    }
}

/*
 * 0x06d8e
 *
 * A part has moved: mark it and everything joined to it as needing re-filing,
 * register the shapes of what it is joined to, and - unless it is a kind 0x31
 * anchor, which draws nothing - its own as well. All three with a count of 1.
 */
void part_moved(struct part *part)
{
    mark_needs_refile(part, 1);
    mark_joined_shapes(part, 1);

    if (part->kind != KIND_ANCHOR)
        mark_part_shapes(part, 1);
}

/*
 * 0x06dbf
 *
 * The part at the other end of a part's rope: the rope record at +0x54 names
 * both ends at +4 and +6, and this answers whichever is not the one asked
 * about. A part with no rope answers 0, and so does one whose rope names it at
 * neither end - the `xor ax, ax` is reached from both.
 */
struct part *rope_other_end(register struct part *part)
{
    register struct rope *si;

    if ((si = ROPE_PTR(part->rope_ptr)) != ROPE_NONE) {
        if (PART_PTR(si->end_a_ptr) == part)
            return PART_PTR(si->end_b_ptr);
        else
            return PART_PTR(si->end_a_ptr);
    }
    return PART_NONE;
}

/*
 * 0x06de9
 *
 * How a belt runs between two parts, as a small bit set.
 *
 * `which` says which end of the belt record to start from - +2 and its slot at
 * +0x0a, or +4 and +0x0b - and the other end follows. The two tangent points of
 * each end are at +0x14 and +0x16 of the belt records involved, four bytes to
 * the pair, and the answer compares them.
 *
 * Bit 3 or bit 4 says which of the two the near end is above; bits 1 and 2 say
 * the same for the far end, and an answer of 1 means the belt crosses itself,
 * which is the only one returned without the first bit or-ed in. `dir` turns
 * every comparison round, which is how the same routine serves a belt read from
 * either side.
 *
 * If the far end's neighbour is the near part itself the belt is a loop of two,
 * and both ends read from this record rather than from the neighbours'.
 */
int16_t belt_orientation(register dg_near_t belt, int16_t which, int16_t dir)
{
    int16_t di;
    uint16_t v02;                       /* [bp-2]  the near slot */
    uint16_t v04;                       /* [bp-4]  the far slot */
    uint16_t v06;                       /* [bp-6]  the near index */
    uint16_t v08;                       /* [bp-8]  the far index */
    int16_t v0a;                        /* [bp-0xa] the first bit */
    dg_near_t v0c;                       /* [bp-0xc] the near record */
    dg_near_t v0e;                       /* [bp-0xe] the far record */
    struct part *v10;                   /* [bp-0x10] the near part */
    dg_near_t v12;                       /* [bp-0x12] the far part */
    dg_near_t v14;                       /* [bp-0x14] beyond the near end */
    struct part *v16;                   /* [bp-0x16] beyond the far end */

    di = 1 - which;
    if (which) {
        v10 = PART_PTR(BELT_PTR(belt)->end_b_ptr);
        v02 = BELT_PTR(belt)->slot_b;
        v12 = BELT_PTR(belt)->end_a_ptr;
        v04 = BELT_PTR(belt)->slot_a;
    } else {
        v10 = PART_PTR(BELT_PTR(belt)->end_a_ptr);
        v02 = BELT_PTR(belt)->slot_a;
        v12 = BELT_PTR(belt)->end_b_ptr;
        v04 = BELT_PTR(belt)->slot_b;
    }
    v14 = v10->link_ptr[v02];
    v16 = PART_PTR(PART_PTR(v12)->link_ptr[v04]);

    if (v12 == v14) {
        v0c = v0e = belt;
        v06 = di;
        v08 = which;
    } else {
        v0c = PART_PTR(v14)->belt_ptr[0];
        v0e = v16->belt_ptr[0];
        v06 = 1 - which;
        v08 = 1 - di;
    }

    if (BELT_PTR(belt)->pt[0][di].x > BELT_PTR(v0e)->pt[0][v08].x)
        v0a = 8;
    else
        v0a = 0x10;

    if (dir == 0) {
        if (BELT_PTR(belt)->pt[0][which].y > BELT_PTR(v0c)->pt[0][v06].y)
            return 1;
        if (BELT_PTR(belt)->pt[0][di].y > BELT_PTR(v0e)->pt[0][v08].y)
            return 2 | v0a;
        return 4 | v0a;
    }
    if (BELT_PTR(belt)->pt[0][which].y < BELT_PTR(v0c)->pt[0][v06].y)
        return 1;
    /*
     * `jge`, not `jl`. The two halves are **not** mirror images: with the
     * direction set the first test is `<` and the second `>=`, where with
     * it clear both are `>`. Reading the second as the first one flipped
     * gives 2 where the original gives 4, so the answer loses bit 2 - and
     * bit 2 is what `tension_belt` reads to decide which way a lever is
     * driven. The lever then never turns, and on the credits screen the
     * gun it is tied to never fires. Written as `<` answering 4, which is
     * the same test in the order the image has its two returns.
     */
    if (BELT_PTR(belt)->pt[0][di].y < BELT_PTR(v0e)->pt[0][v08].y)
        return 4 | v0a;
    return 2 | v0a;
}

/*
 * 0x06f43
 *
 * Say which of two fields of a structure matches a value: 0 for the field at
 * +0x5a, 1 for the one at +0x5c, and -1 for neither. The structure is reached
 * by a **** pointer - a DGROUP offset - so it is indexed off DGROUP here.
 */
int16_t match_field_5a_5c(struct part *value, struct part *obj)
{
    if (PART_PTR(obj->link_ptr[0]) == value)
        return 0;
    if (PART_PTR(obj->link_ptr[1]) == value)
        return 1;
    return -1;
}

/*
 * 0x06f68
 *
 * Given a record reached by a **** pointer, answer the word at +4 if the
 * word at +2 matches, and the word at +2 itself if it does not. A null record
 * answers 0.
 *
 * Both the "matched" and "did not match" paths funnel through one `jmp` to the
 * epilogue, which is why the disassembly has three jumps to reach two results.
 */
dg_near_t select_field_2_or_4(struct part *key, register struct belt *rec)
{
    /* `or si,si` at 0x06f6f: no belt is an offset of 0, which as a pointer
       is BELT_NONE - DGROUP:0 - and never NULL. */
    if (rec != BELT_NONE) {
        if (PART_PTR(rec->end_a_ptr) == key)
            return rec->end_b_ptr;
        else
            return rec->end_a_ptr;
    }
    return 0;
}

/*
 * 0x06f8e
 *
 * Measure how far a link's endpoint is from the endpoint it joins, in
 * whichever coordinate array `mode` names.
 *
 * The partner is found the same way `belt_orientation` finds it: the byte
 * index at +0xa or +0xb selects a word from the object's table at +0x5a, and
 * either the link is its own partner or that entry's +0x66 names one. Which
 * index each side is read with is not symmetric - when the link partners
 * itself the two indices are opposites, and when the partner is a different
 * object both sides use the same index.
 *
 * A missing partner is distance zero rather than an error.
 *
 * `gen` picks which generation of the object's position history to measure -
 * 1 reads +0x24, 2 reads +0x1c, and anything else +0x14. Those are the three
 * slots `shift_state_history` ages, so 1 is two steps ago, 2 is one step ago
 * and 0 is now. Each slot is a two-word point, x then y, four bytes to an
 * endpoint.
 *
 * The result is the usual octagonal approximation to a hypotenuse - the larger
 * of |dx| and |dy| plus three eighths of the smaller, as `>> 2` plus `>> 3`.
 * It never divides and is within about six per cent of the true length.
 */
int16_t link_end_distance(register struct belt *link, int16_t gen, int16_t end)
{
    struct belt *partner;
    int16_t dx;                         /* [bp-2] */
    int16_t dy;                         /* [bp-4] */
    int16_t d;                          /* [bp-6] */
    uint16_t near_i;                    /* [bp-8] */
    uint16_t far_i;                     /* [bp-0xa] */
    struct part *ent;                   /* [bp-0xc] */
    struct belt *l;                     /* [bp-0xe] the same link */

    l = link;
    if (end == 0) {
        near_i = 0;
        ent = PART_PTR(PART_PTR(link->end_a_ptr)->link_ptr[link->slot_a]);
        if (PART_PTR(link->end_b_ptr) == ent) {
            partner = link;
            far_i = 1;
        } else {
            partner = BELT_PTR(ent->belt_ptr[0]);
            far_i = 0;
        }
    } else {
        near_i = 1;
        ent = PART_PTR(PART_PTR(link->end_b_ptr)->link_ptr[link->slot_b]);
        if (PART_PTR(link->end_a_ptr) == ent) {
            partner = link;
            far_i = 0;
        } else {
            partner = BELT_PTR(ent->belt_ptr[0]);
            far_i = 1;
        }
    }

    if (partner == BELT_NONE)
        return 0;

    if (gen == 1) {
        dx = abs((int16_t)(l->pt[2][near_i].x - partner->pt[2][far_i].x));
        dy = abs((int16_t)(l->pt[2][near_i].y - partner->pt[2][far_i].y));
    } else if (gen == 2) {
        dx = abs((int16_t)(l->pt[1][near_i].x - partner->pt[1][far_i].x));
        dy = abs((int16_t)(l->pt[1][near_i].y - partner->pt[1][far_i].y));
    } else {
        dx = abs((int16_t)(l->pt[0][near_i].x - partner->pt[0][far_i].x));
        dy = abs((int16_t)(l->pt[0][near_i].y - partner->pt[0][far_i].y));
    }
    d = dy > dx ? (dx >> 2) + (dx >> 3) + dy : (dy >> 2) + (dy >> 3) + dx;
    return d;
}

/*
 * 0x0713d
 *
 * How much slack a link has at one of its ends: the rest length the link was
 * given, less how far apart the two ends actually are.
 *
 * `gen` selects a generation of history and is passed straight through, so the
 * rest length and the distance are always read from the same step. 1 is two
 * steps ago, 2 is one step ago, anything else is now - the chains at +0x96 and
 * +0x9c that `shift_state_history` ages, read newest-first as +0x96, +0x98,
 * +0x9a.
 *
 * Which end is measured comes from matching the link's two objects. The object
 * at +2 is the first end and uses the +0x96 chain; the object at +4 is the
 * second end, uses +0x9c, and has to match what the +0x5a table names rather
 * than being compared directly. An object of type 7 is looked up at index 0
 * instead of at the link's own index. Matching neither end is zero slack, not
 * an error - and so is a null table entry.
 *
 * The rest lengths live on the object the link names at +0, which is neither
 * of the two ends.
 */
int16_t link_slack(struct part *obj, register struct belt *link, int16_t gen)
{
    register struct part *holder;
    int16_t d;                          /* [bp-2] */
    int16_t rest;                       /* [bp-4] */
    struct part *ent;                   /* [bp-6] */

    if (obj->kind == KIND_PULLEY)
        ent = PART_PTR(obj->link_ptr[0]);
    else
        ent = PART_PTR(obj->link_ptr[link->slot_a]);
    holder = PART_PTR(link->owner_ptr);

    if (PART_PTR(link->end_a_ptr) == obj) {
        if (gen == 1)
            rest = holder->word_96_prev2;
        else if (gen == 2)
            rest = holder->word_96_prev;
        else
            rest = holder->word_96;
        d = rest - link_end_distance(link, gen, 0);
    } else if (ent != PART_NONE && PART_PTR(link->end_b_ptr) == ent) {
        if (gen == 1)
            rest = holder->spin_prev2;
        else if (gen == 2)
            rest = holder->spin_prev;
        else
            rest = holder->spin;
        d = rest - link_end_distance(link, gen, 1);
    } else
        d = 0;
    return d;
}

/*
 * 0x07205
 *
 * **The direction of whatever a rope's other end is tied to**: `rope_other_end`
 * for the part, and that part's +0x12, or 0 when the rope has no other end.
 *
 * **Nothing in the image calls it**: no near call from segment 0000 and no far
 * call anywhere, so the descent map never reached it. It is transcribed
 * because the module holds it. The name is ours.
 */
int16_t other_end_direction(struct part *part)
{
    register struct part *si;

    if ((si = rope_other_end(part)) != PART_NONE)
        return si->direction;
    return 0;
}

/*
 * 0x07223
 *
 * Set an object's vector at +0x36/+0x38 from an angle and a magnitude.
 *
 * `angle_sin` goes to +0x36 and `angle_cos` to +0x38, which is the opposite
 * pairing to the usual (x, y) order - worth stating because the two calls are
 * three bytes apart in the image and easy to swap.
 *
 * Both tables are scaled so that 16384 stands for 1, so the products are
 * shifted right by 14 to come back to the caller's units. The original does
 * that through the runtime helper at 0x0be5f, which is the near entry that
 * fakes a far frame and falls into the `sar` body at 0x0be62 - an arithmetic
 * shift, so a negative component stays negative.
 *
 * AX happens to hold the second component on return; the routine is void and
 * no caller reads it.
 */
void set_vector_from_angle(struct part *obj, uint16_t angle, int16_t mag)
{
    int32_t sn = mul16x16(mag, angle_sin(angle));
    int32_t cs = mul16x16(mag, angle_cos(angle));

    obj->vel_x = (int16_t)(sn >> 14);
    obj->vel_y = (int16_t)(cs >> 14);
}

/*
 * 0x07283
 *
 * Recompute a record's velocity from how far it has moved, then clamp it.
 *
 * The position is at +0x1e and +0x20 - the same pair `compute_other_bounds`
 * reads as the left and top edges - and +0x22 and +0x24 hold where it was, so
 * the difference is the step taken. That difference is then shifted **left**
 * by `9 - shift`, which turns a whole-pixel step into the fixed-point velocity
 * the rest of the code works in: a smaller `shift` argument means a bigger
 * result.
 *
 * The two axes are independent, chosen by bits 0 and 1 of the last argument,
 * so either can be left alone. It finishes by calling `clamp_record_pair`,
 * which clamps exactly the two fields written here - which is what identifies
 * +0x36 and +0x38 as a velocity pair rather than anything else.
 */
void update_velocity(register struct part *rec, int16_t shift_x, int16_t shift_y,
                     register uint16_t which)
{
    if (which & 1) {
        rec->vel_x = rec->pos[0].x - rec->pos[1].x;
        SHL16_ASSIGN(rec->vel_x, 9 - shift_x);
    }
    if (which & 2) {
        rec->vel_y = rec->pos[0].y - rec->pos[1].y;
        SHL16_ASSIGN(rec->vel_y, 9 - shift_y);
    }
    clamp_record_pair(rec);
}

#ifndef __TURBOC__
/* ours: a call counter for reconstruct/devdump.c. Above this
   routine's comment, not between it and the routine: the
   provenance is the comment *directly* above a definition. */
int32_t dev_tension_belt_calls;
#endif

/*
 * 0x072c7
 *
 * Settle one part against the belt it hangs from, and answer whether the part
 * had to move.
 *
 * The belt at +0x66 has a length of slack at each end - +0x96 and +0x9c of the
 * part the belt record names at +0 - and `link_endpoint_gap` measures how far
 * apart the two ends actually are. The difference is how much the belt is
 * over-stretched at this end.
 *
 * There are three ways to take up the stretch, tried in order.
 *
 *  1. **Borrow from the other end.** If this end is long and the other short,
 *     the two are added and the surplus moved across, which costs nothing.
 *  2. **Pull the other part.** If the far part can be pulled - bit 12 of +6 -
 *     and this one is heavier, some of the stretch is given to it in
 *     proportion to the difference in mass, and the far part is settled
 *     recursively before the measurement is taken again.
 *  3. **Move this part.** The position is set along the line to the far end at
 *     exactly the slack's distance, and the velocity is recomputed.
 *
 * A fourth case sits between the second and the third: a far part of kind 0x31
 * is an anchor and cannot be pulled, so the belt is *unthreaded* from the
 * pulley instead - the pulley's link is spliced out, its own two links cleared,
 * and the slack recomputed from what is left. That is how a belt comes off a
 * wheel when it is pulled too hard.
 *
 * Afterwards, unless the part is standing still, the far part is told which
 * way the belt is now running: kind 3, the motor, is put on the move queue with
 * a direction that depends on which side of it the belt leaves; everything else
 * goes through its own drive hook.
 */
int16_t tension_belt(register struct part *part)
{
    struct part *di;
    int16_t slackA;                     /* [bp-2] */
    int16_t gapA;                       /* [bp-4] */
    int16_t dA;                         /* [bp-6] */
    int16_t dB;                         /* [bp-8] */
    int16_t slackB;                     /* [bp-0xa] */
    int16_t gapB;                       /* [bp-0xc] */
    int16_t k;                          /* [bp-0xe] */
    int16_t i;                          /* [bp-0x10] */
    int16_t dx1;                        /* [bp-0x12] */
    int16_t dy1;                        /* [bp-0x14] */
    int16_t nx;                         /* [bp-0x16] */
    int16_t ny;                         /* [bp-0x18] */
    uint8_t dx2[2];                     /* [bp-0x1a] */
    uint8_t dy2[2];                     /* [bp-0x1c] */
    int16_t end;                        /* [bp-0x1e] */
    uint16_t slot;                      /* [bp-0x20] */
    int16_t orient;                     /* [bp-0x22] */
    int16_t moving;                     /* [bp-0x24] */
    int16_t answer;                     /* [bp-0x26] */
    int16_t dir;                        /* [bp-0x28] */
    int16_t saved;                      /* [bp-0x2a] */
    int16_t give;                       /* [bp-0x2c] */
    int16_t pulley;                     /* [bp-0x2e] */
    /* One long: the original stores DX:AX across [bp-0x32] and [bp-0x30] and
       reads the pair straight back into a divide. */
    int32_t product;                    /* [bp-0x32] */
    dg_near_t other;                     /* [bp-0x34] a part, as the offset queue_part takes */
    struct part *pB;                    /* [bp-0x36] */
    struct part *pC;                    /* [bp-0x38] */
    dg_near_t belt;                      /* [bp-0x3a] as the offset belt_orientation takes */

#ifndef __TURBOC__
    dev_tension_belt_calls++;
#endif
    answer = 0;
    if (PART_PTR(part->link_ptr[0])->kind == KIND_PULLEY)
        pulley = 1;
    else
        pulley = 0;
    if (part->pos[0].y < part->pos[1].y)
        moving = 0;
    else if (part->pos[0].y > part->pos[1].y)
        moving = 1;
    else
        moving = -1;

    belt = part->belt_ptr[0];
    di = PART_PTR(BELT_PTR(belt)->owner_ptr);
    other = select_field_2_or_4(part, BELT_PTR(belt));
    if (PART_PTR(BELT_PTR(belt)->end_a_ptr) == part) {
        end = 0;
        slot = BELT_PTR(belt)->slot_b;
        slackA = di->word_96;
        slackB = di->spin;
    } else {
        end = 1;
        slot = BELT_PTR(belt)->slot_a;
        slackA = di->spin;
        slackB = di->word_96;
    }
    gapB = link_endpoint_gap(BELT_PTR(belt), PART_PTR(other), dx2, dy2);
    gapA = link_endpoint_gap(BELT_PTR(belt), part, (uint8_t *)&dx1, (uint8_t *)&dy1);
    dA = gapA - slackA;

    if (part->kind != KIND_ANCHOR) {
        dB = gapB - slackB;
        if (dA > 0 && dB < 0) {
            dA += dB;
            if (dA > 0)
                dB = 0;
            else {
                dB = dA;
                dA = 0;
            }
            if (PART_PTR(BELT_PTR(belt)->end_a_ptr) == part) {
                di->word_96 = slackA = gapA - dA;
                di->spin = slackB = gapB - dB;
            } else {
                di->spin = slackA = gapA - dA;
                di->word_96 = slackB = gapB - dB;
            }
        }
    }

    if (dA > 0 && (PART_PTR(other)->flags_06 & 0x1000) && PART_PTR(other)->kind != KIND_ANCHOR
        && part->kind != KIND_ANCHOR && part->weight > PART_PTR(other)->weight) {
        product = mul16x16(abs(dA), part->weight - PART_PTR(other)->weight);
        give = (product + part->weight) / part->weight;
        give = abs(slackB) > (give > 1 ? give : 1) ? (give > 1 ? give : 1)
                                                   : abs(slackB);
        if (give != 0) {
            if (PART_PTR(BELT_PTR(belt)->end_a_ptr) == part) {
                di->spin -= give;
                slackB = di->spin;
                tension_belt(PART_PTR(other));
                PART_PTR(other)->flags_06 &= 0xfff0;
                resolve_collisions(PART_PTR(other));
                gapB = link_endpoint_gap(BELT_PTR(belt), PART_PTR(other), dx2, dy2);
                dB = gapB - slackB;
                if (dB != 0) {
                    di->spin += dB;
                    give -= dB;
                }
                if (give != 0) {
                    di->word_96 += give;
                    slackA = di->word_96;
                    dA = gapA - slackA;
                }
            } else {
                di->word_96 -= give;
                slackB = di->word_96;
                tension_belt(PART_PTR(other));
                PART_PTR(other)->flags_06 &= 0xfff0;
                resolve_collisions(PART_PTR(other));
                gapB = link_endpoint_gap(BELT_PTR(belt), PART_PTR(other), dx2, dy2);
                dB = gapB - slackB;
                if (dB != 0) {
                    di->word_96 += dB;
                    give -= dB;
                }
                if (give != 0) {
                    di->spin += give;
                    slackA = di->spin;
                    dA = gapA - slackA;
                }
            }
        }
    }

    if (dA > 0) {
        if (PART_PTR(other)->kind == KIND_ANCHOR && part->kind != KIND_ANCHOR) {
            /* The far end is an anchor: take the belt off the pulley instead. */
            if (pulley) {
                if (BELT_PTR(belt)->end_a_ptr == other) {
                    di->word_96 -= dA;
                    if ((int16_t)di->word_96 < 0) {
                        dA += (int16_t)di->word_96;
                        saved = DG4E67.state;
                        DG4E67.state = 0x1000;
                        mark_belt_shapes(di, 3);
                        DG4E67.state = saved;
                        pB = PART_PTR(PART_PTR(other)->link_ptr[BELT_PTR(belt)->slot_a]);
                        pC = PART_PTR(pB->link_ptr[0]);
                        k = match_field_5a_5c(pB, pC);
                        PART_PTR(other)->link_ptr[BELT_PTR(belt)->slot_a] = dg_near(dgroup, pC);
                        pC->link_ptr[k] = other;
                        for (i = 0; i < 2; i++)
                            pB->link_ptr[i] = 0;
                        PART_PTR(BELT_PTR(belt)->owner_ptr)->word_96 = link_end_distance(BELT_PTR(belt), 3, 0);
                    }
                    di->spin += dA;
                } else {
                    di->spin -= dA;
                    if (di->spin < 0) {
                        dA += di->spin;
                        saved = DG4E67.state;
                        DG4E67.state = 0x1000;
                        mark_belt_shapes(di, 3);
                        DG4E67.state = saved;
                        pB = PART_PTR(PART_PTR(other)->link_ptr[BELT_PTR(belt)->slot_b]);
                        pC = PART_PTR(pB->link_ptr[1]);
                        k = match_field_5a_5c(pB, pC);
                        PART_PTR(other)->link_ptr[BELT_PTR(belt)->slot_b] = dg_near(dgroup, pC);
                        pC->link_ptr[k] = other;
                        for (i = 0; i < 2; i++)
                            pB->link_ptr[i] = 0;
                        PART_PTR(BELT_PTR(belt)->owner_ptr)->spin = link_end_distance(BELT_PTR(belt), 3, 1);
                    }
                    di->word_96 += dA;
                }
            }
        } else {
            answer = 1;
            product = mul16x16(dx1, slackA);
            nx = product / gapA;
            part->pos[0].x += nx - dx1;
            part->fx = part->pos[0].x;
            part->fx = (int32_t)((uint32_t)part->fx << 9);
            product = mul16x16(dy1, slackA);
            ny = product / gapA;
            part->pos[0].y += ny - dy1;
            part->fy = part->pos[0].y;
            part->fy = (int32_t)((uint32_t)part->fy << 9);
            place_object_for_draw(part);
            update_velocity(part, 0, 0, 1);
            if (part->pos[0].x == part->pos[1].x)
                part->vel_y = 0;

            if (part->kind != KIND_ANCHOR && moving != -1) {
                orient = belt_orientation(belt, end,
                                          moving == 0 ? 0 : 1);
                if (PART_PTR(other)->kind == 3) {
                    dir = 0;
                    if (orient & 4) {
                        if (slot == 0) {
                            if (PART_PTR(other)->form > 0)
                                dir = -1;
                        } else if (PART_PTR(other)->form < 2)
                            dir = 1;
                    } else {
                        if (slot == 0) {
                            if (PART_PTR(other)->form < 2)
                                dir = 1;
                        } else if (PART_PTR(other)->form > 0)
                            dir = -1;
                    }
                    if (queue_part(part, other)) {
                        PART_PTR(other)->direction = dir;
                        PART_PTR(other)->momentum = part->momentum;
                    }
                } else
                    part_drive(PART_PTR(other), part, PART_PTR(other), 0, orient,
                               PART_KINDS[part->kind].weight, part->momentum);
            }
        }
    }
    return answer;
}

/*
 * 0x07947
 *
 * Measure the gap a link has to close: the vector from one of its endpoints
 * to the endpoint it joins, written to the caller's two words, and the
 * approximate length of that vector as the result.
 *
 * The `obj` argument only decides which side of the link is read. If it is the
 * object at +2 the link's first index at +0xa is used; otherwise the object at
 * +4 and the index at +0xb are used and `obj` itself is ignored entirely - so
 * passing something that is neither still measures the second side.
 *
 * An endpoint is the object's position at +0x2a/+0x2c plus a signed... no,
 * an *unsigned* byte offset from the pair at +0x6a/+0x6b, two bytes to an
 * index. The offsets are zero-extended, so an endpoint is never left of or
 * above the object's own position.
 *
 * The far side comes from the object named at +0x5a, and `match_field_5a_5c`
 * says which of its ends faces back. Type 7 is the exception: that object's
 * endpoint is a point in the array at +0x66, indexed by the *opposite* end,
 * and read as full words rather than byte offsets.
 *
 * The length is the same octagonal approximation `link_end_distance` uses. The
 * original re-reads both deltas out of the caller's words rather than keeping
 * them in registers, which is what it does here too: if a caller passes the
 * same address for both, the second store lands on the first and the length is
 * measured from the aliased pair.
 */
int16_t link_endpoint_gap(struct belt *link, register struct part *obj,
                          uint8_t * out_dx, uint8_t * out_dy)
{
    struct part *other;
    uint16_t a;                         /* [bp-2] */
    uint16_t b;                         /* [bp-4] */
    int16_t d;                          /* [bp-6] */
    int16_t y1;                         /* [bp-8] */
    int16_t x1;                         /* [bp-0xa] */
    int16_t y2;                         /* [bp-0xc] */
    int16_t x2;                         /* [bp-0xe] */

    /* Two whole blocks, and the second turns the first's roles round: `obj`
       is the near end in the first and takes the far end in the second, and
       the two index slots swap with them. */
    if (PART_PTR(link->end_a_ptr) == obj) {
        a = link->slot_a;
        x1 = obj->box[0].x + obj->attach[a].x;
        y1 = obj->box[0].y + obj->attach[a].y;
        other = PART_PTR(obj->link_ptr[a]);
        b = match_field_5a_5c(obj, other);
        if (other->kind == KIND_PULLEY) {
            x2 = BELT_PTR(other->belt_ptr[0])->pt[0][1 - b].x;
            y2 = BELT_PTR(other->belt_ptr[0])->pt[0][1 - b].y;
        } else {
            x2 = other->box[0].x + other->attach[b].x;
            y2 = other->box[0].y + other->attach[b].y;
        }
    } else {
        other = PART_PTR(link->end_b_ptr);
        b = link->slot_b;
        x1 = other->box[0].x + other->attach[b].x;
        y1 = other->box[0].y + other->attach[b].y;
        obj = PART_PTR(other->link_ptr[b]);
        a = match_field_5a_5c(other, obj);
        if (obj->kind == KIND_PULLEY) {
            x2 = BELT_PTR(obj->belt_ptr[0])->pt[0][1 - a].x;
            y2 = BELT_PTR(obj->belt_ptr[0])->pt[0][1 - a].y;
        } else {
            x2 = obj->box[0].x + obj->attach[a].x;
            y2 = obj->box[0].y + obj->attach[a].y;
        }
    }

    *(int16_t *)out_dx = x1 - x2;
    *(int16_t *)out_dy = y1 - y2;
    d = abs(*(int16_t *)out_dy) > abs(*(int16_t *)out_dx)
        ? (abs(*(int16_t *)out_dx) >> 2) + (abs(*(int16_t *)out_dx) >> 3)
          + abs(*(int16_t *)out_dy)
        : (abs(*(int16_t *)out_dy) >> 2) + (abs(*(int16_t *)out_dy) >> 3)
          + abs(*(int16_t *)out_dx);
    return d;
}

/*
 * 0x07b3e
 *
 * Splice the whole of one list onto the front of another and empty the first.
 *
 * The list at DGROUP 0x4e58 is walked to its last node - the link is the first
 * word of each node - that node is pointed at the head of the list at DGROUP
 * 0x4e56, and 0x4e56 is then pointed at what 0x4e58 held. Returning a batch of
 * nodes to a free list in one move, by the shape of it, though the names are
 * not established.
 */
void splice_list_4e58_onto_4e56(void)
{
    register dg_near_t next;
    register dg_near_t last;

    if (DG4E4E.parts_queue_ptr != 0) {
        last = DG4E4E.parts_queue_ptr;
        next = QNODE_PTR(last)->next_ptr;
        while (next != 0) {
            last = next;
            next = QNODE_PTR(next)->next_ptr;
        }
        QNODE_PTR(last)->next_ptr = DG4E4E.parts_free_ptr;
        DG4E4E.parts_free_ptr = DG4E4E.parts_queue_ptr;
        DG4E4E.parts_queue_ptr = 0;
    }
}

#ifndef __TURBOC__
/* ours: a call counter for reconstruct/devdump.c. Above this
   routine's comment, not between it and the routine: the
   provenance is the comment *directly* above a definition. */
int32_t dev_queue_part_calls;
#endif

/*
 * 0x07b6f
 *
 * Put a part on the queue at DGROUP 0x4e58, in order of the priority the
 * *source* part carries at +0x3c and +0x3e - a 32-bit pair, compared high word
 * signed and low word unsigned, which is what `jg`/`jl` and `jae`/`ja` say.
 *
 * A part already on the queue at that priority or better is left where it is
 * and the answer is 0. Otherwise a node comes off the free list at 0x4e56 and
 * goes in at the right place: at the head if the queue is empty or the head is
 * lower, and otherwise after the last node that outranks it.
 *
 * The queue is what `step_machine` runs first, so this is how one part asks
 * another to move before the general passes begin.
 */
int16_t queue_part(struct part *src, dg_near_t part)
{
    register dg_near_t si;
    register dg_near_t di;
    int32_t key;                        /* [bp-4], high word at [bp-2] */

#ifndef __TURBOC__
    dev_queue_part_calls++;
#endif
    /* The pair is one `long`, and every test on it below is the original's
       high-word `jg`/`jl` and low-word `jae`/`ja` - a signed 32-bit compare. */
    key = src->momentum;

    for (si = DG4E4E.parts_queue_ptr; si != 0; si = QNODE_PTR(si)->next_ptr)
        if (QNODE_PTR(si)->part == part && QNODE_PTR(si)->momentum >= key)
            return 0;

    if (DG4E4E.parts_queue_ptr == 0
        || QNODE_PTR(DG4E4E.parts_queue_ptr)->momentum < key) {
        si = DG4E4E.parts_free_ptr;
        DG4E4E.parts_free_ptr = QNODE_PTR(DG4E4E.parts_free_ptr)->next_ptr;
        QNODE_PTR(si)->next_ptr = DG4E4E.parts_queue_ptr;
        DG4E4E.parts_queue_ptr = si;
    } else {
        di = DG4E4E.parts_queue_ptr;
        si = QNODE_PTR(DG4E4E.parts_queue_ptr)->next_ptr;
        while (si != 0 && QNODE_PTR(si)->momentum > key) {
            di = si;
            si = QNODE_PTR(si)->next_ptr;
        }
        si = DG4E4E.parts_free_ptr;
        DG4E4E.parts_free_ptr = QNODE_PTR(DG4E4E.parts_free_ptr)->next_ptr;
        QNODE_PTR(si)->next_ptr = QNODE_PTR(di)->next_ptr;
        QNODE_PTR(di)->next_ptr = si;
    }
    QNODE_PTR(si)->part = part;
    QNODE_PTR(si)->momentum = key;
    return 1;
}

/*
 * 0x07c3a
 *
 * Add the weight of everything a platform carries to the platform itself: the
 * chain `collect_carried` built at +0x78, one at a time.
 */
void add_carried_weight(struct part *obj)
{
    struct part *si;

    for (si = PART_PTR(obj->next_linked_ptr); si != PART_NONE;
         si = PART_PTR(si->next_linked_ptr))
        add_mass_capped(obj, si);
}

/*
 * 0x07c5b
 *
 * One thing's weight on to another's, capped at 0x7d00.
 *
 * The sum is made in 32 bits and compared against the cap as a 32-bit value -
 * high word signed, low word unsigned - so a pair of heavy things cannot wrap
 * round to a light one, which a 16-bit add would.
 */
void add_mass_capped(register struct part *obj, register struct part *other)
{
    int32_t total;                      /* [bp-4] */

    total = obj->weight;
    total += other->weight;
    if (total > 0x7d00)
        total = 0x7d00;
    obj->weight = total;
}

/*
 * 0x07ca2
 *
 * Age the state histories of everything the simulation is about to step.
 *
 * The object named by the global at 0x50d5 goes first, if there is one, and
 * then every object the 0x3000/0x1000 list walk reaches - `pick_by_flag` for
 * the head, `pick_for_record` for each one after. The list member equal to
 * 0x50d5 is skipped, because it was already done; without that test it would
 * be aged twice and lose a generation.
 */
void shift_all_histories(void)
{
    struct part *obj;

    if (DG50D3.dragged_part_ptr != 0)
        shift_state_history(PART_PTR(DG50D3.dragged_part_ptr));

    obj = pick_by_flag(0x3000);
    while (obj != PART_NONE) {
        if (obj != PART_PTR(DG50D3.dragged_part_ptr))
            shift_state_history(obj);
        obj = pick_for_record(obj, 0x1000);
    }
}

/*
 * 0x07ce3
 *
 * Age every tracked quantity on an object by one step: slot 2 takes slot 1,
 * slot 1 takes slot 0. Slot 0 is left alone - whatever runs the simulation
 * writes it afterwards - so the object keeps the last three values of each
 * quantity.
 *
 * The main object's histories are three 32-bit chains at +0x1e, +0x2a and
 * +0x44 and one 16-bit chain at +0xc, each generation four bytes on from the
 * last, plus two more 16-bit chains at +0x96 and +0x9c that are aged
 * unconditionally at the end.
 *
 * Two nested objects are aged as well, and which one depends on the type word
 * at +4. Type 8 reaches the object at +0x54 - but only while the global at
 * 0x4e6b holds 0x1000 - and ages four 32-bit chains at +8, +0xc, +0x10 and
 * +0x14, whose generations are 0x10 apart rather than 4. Types 7 and 10 reach
 * the object at +0x66 and age two 32-bit chains at +0x14 and +0x18 and one
 * 16-bit chain at +0xe.
 *
 * The two nested cases are not exclusive in the code: the type-8 test falls
 * through to the 7-or-10 test rather than returning, so a single pass could in
 * principle do both. No type satisfies both conditions, so it never does.
 */
void shift_state_history(register struct part *obj)
{
    struct rope *rope;
    struct belt *belt;

    /* one 32-bit move per generation, which is what a `struct point16` is */
    obj->pos[2] = obj->pos[1];
    obj->pos[1] = obj->pos[0];
    obj->box[2] = obj->box[1];
    obj->box[1] = obj->box[0];
    obj->size[2] = obj->size[1];
    obj->size[1] = obj->size[0];
    obj->form_prev2 = obj->form_prev;
    obj->form_prev = obj->form;

    if (obj->kind == KIND_BELT && DG4E67.state == 0x1000) {
        rope = ROPE_PTR(obj->rope_ptr);
        rope->pt[2][0] = rope->pt[1][0];
        rope->pt[1][0] = rope->pt[0][0];
        rope->pt[2][1] = rope->pt[1][1];
        rope->pt[1][1] = rope->pt[0][1];
        rope->pt[2][2] = rope->pt[1][2];
        rope->pt[1][2] = rope->pt[0][2];
        rope->pt[2][3] = rope->pt[1][3];
        rope->pt[1][3] = rope->pt[0][3];
    }
    if (obj->kind == KIND_ROPE || obj->kind == KIND_PULLEY) {
        belt = BELT_PTR(obj->belt_ptr[0]);
        belt->v[2] = belt->v[1];
        belt->v[1] = belt->v[0];
        /* one 32-bit move per point, which is what a `struct belt_end` is */
        belt->pt[2][0] = belt->pt[1][0];
        belt->pt[1][0] = belt->pt[0][0];
        belt->pt[2][1] = belt->pt[1][1];
        belt->pt[1][1] = belt->pt[0][1];
    }
    obj->word_96_prev2 = obj->word_96_prev;
    obj->word_96_prev = obj->word_96;
    obj->spin_prev2 = obj->spin_prev;
    obj->spin_prev = obj->spin;
}

/*
 * 0x07e45
 *
 * Put the machine back to its starting state - two passes over every part on
 * the 0x3000 list.
 *
 * The first pass either throws the part away or resets it. A part with bit 4
 * of its flags at +6 set is one that was added while the machine ran, so it is
 * unlinked and freed; every other one has the low nibble of those flags
 * cleared and is wound back: the position at +0x8c/+0x8e becomes the current,
 * the last and the one before that all at once, and the same position in
 * sixteenths - shifted left by nine - becomes the 32-bit pair at +0x16 and
 * +0x1a. The form at +0x90, the angle at +0x92 and the size at +0x44 are
 * restored the same way, the velocities at +0x36 and the two three-deep
 * histories at +0x96 and +0x9c are cleared, and the mass at +0x3a comes back
 * out of the kind's record. Everything but kind 0x0e also has its two links at
 * +0x5a copied back from the pair at +0x5e, which is where the file's originals
 * were kept. Then the kind's setup runs again, exactly as it did when the part
 * was read.
 *
 * The second pass exists because a rope or a belt joins two parts, so it can
 * only be rebuilt once both ends have been reset. Kind 8 recomputes its rope's
 * endpoints; kind 0x0a rebuilds its belt from the copies at +6, +8, +0x0c and
 * +0x0d, points both parts back at it, walks the chain of parts the belt runs
 * over so a kind 7 among them takes it as its second belt, and measures both
 * ends into the two histories.
 */
void reset_machine(void)
{
    register struct part *si;
    register struct belt *di;
    int16_t i;                          /* [bp-2] */
    struct part *next;                  /* [bp-4] */
    struct part *walk;                  /* [bp-6] */
    struct part *after;                 /* [bp-8] */

    for (si = pick_by_flag(0x3000); si != PART_NONE; si = next) {
        next = pick_for_record(si, 0x1000);
        if (si->flags_06 & 0x10) {
            unlink_part(si);
            free_part(si);
        } else {
            si->flags_06 &= 0xfff0;
            si->flags_08 = si->start_flags;
            si->pos[0].x = si->pos[1].x = si->pos[2].x = si->start_x;
            si->pos[0].y = si->pos[1].y = si->pos[2].y = si->start_y;
            si->fx = si->pos[0].x;
            si->fy = si->pos[0].y;
            si->fx = (int32_t)((uint32_t)si->fx << 9);
            si->fy = (int32_t)((uint32_t)si->fy << 9);
            si->form = si->start_form;
            si->form_prev2 = si->form_prev = si->form;
            set_object_extent(si);
            si->mirror_size = si->size[0];
            place_object_for_draw(si);
            si->box[2] = si->box[1] = si->box[0];
            si->size[2] = si->size[1] = si->size[0];
            si->weight = PART_KINDS[si->kind].weight;
            si->contact_ptr = 0;
            si->direction = si->start_direction;
            si->vel_x = si->vel_y = 0;
            si->word_96 = si->word_96_prev = si->word_96_prev2 = 0;
            si->spin = si->spin_prev = si->spin_prev2 = 0;
            if (si->kind != KIND_GEAR)
                for (i = 0; i < 2; i++)
                    si->link_ptr[i] = si->link_ptr[i + 2];
            PART_KINDS[si->kind].setup(si);
        }
    }

    for (si = pick_by_flag(0x3000); si != PART_NONE;
         si = pick_for_record(si, 0x1000)) {
        if (si->kind == KIND_BELT)
            compute_link_endpoints(ROPE_PTR(si->rope_ptr));
        else if (si->kind == KIND_ROPE) {
            di = BELT_PTR(si->belt_ptr[0]);
            di->end_a_ptr = di->home_a_ptr;
            di->end_b_ptr = di->home_b_ptr;
            di->slot_a = di->home_slot_a;
            di->slot_b = di->home_slot_b;
            PART_PTR(di->end_a_ptr)->belt_ptr[di->slot_a] = dg_near(dgroup, di);
            PART_PTR(di->end_b_ptr)->belt_ptr[di->slot_b] = dg_near(dgroup, di);

            walk = PART_PTR(di->end_a_ptr);
            after = PART_PTR(walk->link_ptr[di->slot_a]);
            while (walk != PART_NONE) {
                if (walk->kind == KIND_PULLEY)
                    walk->belt_ptr[1] = dg_near(dgroup, di);
                if (PART_PTR(di->end_b_ptr) == walk)
                    walk = PART_NONE;
                else {
                    walk = after;
                    after = PART_PTR(after->link_ptr[0]);
                }
            }

            refresh_link_geometry(di);
            si->word_96 = si->word_96_prev = si->word_96_prev2
                = link_end_distance(di, 3, 0);
            si->spin = si->spin_prev = si->spin_prev2
                = link_end_distance(di, 3, 1);
            di->v[0] = di->v[1] = di->v[2] = 0;
        }
    }
}
