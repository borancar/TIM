/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Geometry**: rotating a point, crossing two segments, the angle between
 * two parts, and whether a part's box or outline meets another's.
 *
 * The original's **code segment 0000** (`_TEXT`), image 0x03b17..0x04169,
 * split out of machine.c on 2026-09-27. **Both ends of this file are ours**,
 * for the reason links.c gives: nothing here names `_DATA`, and no call
 * reaches back across either end.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include <stdlib.h>
#include "tim.h"
#include "io.h"
#include "dgroup.h"

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
