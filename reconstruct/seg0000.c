/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file: it is
 * derived from someone else's executable.
 *
 * This file corresponds to the original's **code segment 0000**, image
 * 0x00000..0x0dff0. The binary is Borland C++ large model, so each translation
 * unit is its own code segment and this file mirrors one of them. Functions
 * are in address order and each carries the image offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * NOT a transcription: the absolute value the compiler emits inline - `cwd;
 * xor ax,dx; sub ax,dx` - wherever the original takes one. There is no routine
 * at any address to point at; it is written once here because several
 * transcribed routines below need it.
 */
static int16_t abs16(int16_t v)
{
    return (int16_t)(v < 0 ? (uint16_t)-(uint16_t)v : (uint16_t)v);
}

/*
 * 0x002be
 *
 * Subtract two fields of the structure that DGROUP 0x5400 points at from two
 * words beside it. What the structure is has not been established; only the
 * two fields it touches, at +0x22 and +0x24, and the fact that the pointer is
 * a **near** one - a DGROUP offset dereferenced as `[bx + 0x22]`, which is why
 * DGROUP has to be memory rather than a set of named globals.
 *
 * The pointer is re-read from DGROUP for the second field, exactly as here.
 */
void sub_002be(void)
{
    word_5414 = (int16_t)(word_5420 - DG16(ptr_5400 + 0x22));
    word_5402 = (int16_t)(word_541c - DG16(ptr_5400 + 0x24));
}

/*
 * 0x002dd
 *
 * Build the **swept** bounding box of the object at DGROUP 0x5400: the union
 * of where it is and where it was, which is what a dirty-rectangle redraw has
 * to repaint.
 *
 * The current box comes from the same fields `compute_bounds_53fe` uses -
 * +0x1e/+0x20 for the corner and +0x44/+0x46 for the extents - into
 * 0x5420/0x541c and 0x541e/0x541a, with the centre at 0x5418/0x5416. Then
 * `sub_002be` fills 0x5414 and 0x5402 with how far the object has moved, from
 * the previous position at +0x22/+0x24.
 *
 * The box is then stretched both ways: the near edges at 0x5412/0x5410 move
 * back to the old position if that was further back, and the far edges are
 * pushed out by the **absolute** movement - the branchless
 * `cwd / xor / sub` again.
 *
 * As in 0x00386, the pointer at 0x5400 is re-read before every field.
 */
void compute_swept_bounds_5400(void)
{
    int16_t d;

    DG16(0x5420) = DG16(DGU16(0x5400) + 0x1E);
    DG16(0x5412) = DG16(0x5420);
    DG16(0x541C) = DG16(DGU16(0x5400) + 0x20);
    DG16(0x5410) = DG16(0x541C);

    DG16(0x541E) = (int16_t)(DG16(0x5420) + DG16(DGU16(0x5400) + 0x44));
    DG16(0x541A) = (int16_t)(DG16(0x541C) + DG16(DGU16(0x5400) + 0x46));

    DG16(0x5418) = (int16_t)(DG16(0x5420)
                             + (int16_t)(DG16(DGU16(0x5400) + 0x44) >> 1));
    DG16(0x5416) = (int16_t)(DG16(0x541C)
                             + (int16_t)(DG16(DGU16(0x5400) + 0x46) >> 1));

    sub_002be();

    if (DG16(DGU16(0x5400) + 0x22) < DG16(0x5420))
        DG16(0x5412) = DG16(DGU16(0x5400) + 0x22);
    if (DG16(DGU16(0x5400) + 0x24) < DG16(0x541C))
        DG16(0x5410) = DG16(DGU16(0x5400) + 0x24);

    d = DG16(0x5414);
    if (d < 0)
        d = (int16_t)-d;
    DG16(0x541E) = (int16_t)(DG16(0x541E) + d);

    d = DG16(0x5402);
    if (d < 0)
        d = (int16_t)-d;
    DG16(0x541A) = (int16_t)(DG16(0x541A) + d);
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
void compute_bounds_53fe(void)
{
    DG16(0x540E) = DG16(DGU16(0x53FE) + 0x1E);
    DG16(0x540A) = DG16(DGU16(0x53FE) + 0x20);
    DG16(0x540C) = (int16_t)(DG16(0x540E) + DG16(DGU16(0x53FE) + 0x44));
    DG16(0x5408) = (int16_t)(DG16(0x540A) + DG16(DGU16(0x53FE) + 0x46));
    DG16(0x5406) = (int16_t)(DG16(0x540E)
                             + (int16_t)(DG16(DGU16(0x53FE) + 0x44) >> 1));
    DG16(0x5404) = (int16_t)(DG16(0x540A)
                             + (int16_t)(DG16(DGU16(0x53FE) + 0x46) >> 1));
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
    int16_t si, di, ok = 0;

    if (DG16(0x53FC) == 0)
        return 0;
    if (angle_to_quadrant(angle) != DG16(0x5422))
        return 0;

    si = (int16_t)(angle + 0x2000);
    di = (int16_t)(DG16(0x5424) + 0x2000);
    if (si >= 0 && si <= 0x4000 && di >= 0 && di <= 0x4000) {
        ok = 1;
    } else {
        si = (int16_t)(angle + 0xA000);
        di = (int16_t)(DG16(0x5424) + 0xA000);
        if (si >= 0 && si <= 0x4000 && di >= 0 && di <= 0x4000)
            ok = 1;
    }

    if (ok == 0)
        return 0;
    if (angle == DG16(0x5424))
        return 1;
    if (si == 0x2000 || di == 0x2000)
        return 1;
    if (si < 0x2000 && di < 0x2000)
        return 1;
    if (si > 0x2000 && di > 0x2000)
        return 1;
    return 0;
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
int16_t angle_to_quadrant(int16_t angle)
{
    if ((uint16_t)angle == 0x2000)
        return 0;
    if ((uint16_t)angle == 0xA000)
        return 2;
    return (int16_t)(((int16_t)(angle + 0x2000) >> 14) & 3);
}

/*
 * 0x004fd
 *
 * Decide which side of a range a value falls on, and set one of two flag bytes
 * accordingly - or both, when the value is inside the range.
 *
 * `range` is a record whose bounds are at +0 and +4; `out` is a record whose
 * flags are the bytes at +2 and +3. The containment test is `value_between` at
 * 0x03d67, which handles either ordering, so the side test below has to handle
 * both orderings too - and it does, by asking which bound is the lower one
 * first. All four compares here are **signed**.
 */
void set_side_flags(uint16_t range, int16_t v, uint16_t out)
{
    if (value_between((uint16_t)v, DGU16(range), DGU16(range + 4))) {
        DG8(out + 2) = 1;
        DG8(out + 3) = 1;
        return;
    }

    if (DG16(range) >= DG16(range + 4)) {
        if (DG16(range + 4) <= v)
            DG8(out + 3) = 1;
        else
            DG8(out + 2) = 1;
    } else {
        if (DG16(range) <= v)
            DG8(out + 2) = 1;
        else
            DG8(out + 3) = 1;
    }
}

/*
 * 0x0144e
 *
 * Step the counter at DGROUP 0x4e87, wrapping 0x2a00 back to 0x1c00. What it
 * counts is not established; the range is 0x1c00..0x29ff.
 *
 * **The wrap is unverified.** It needs 10,752 calls to reach, and over the two
 * intro screens the routine is called 428 times with the counter never above
 * 0x1ab. The branch is transcribed from the disassembly and has never been
 * run against the original.
 */
void step_word_4e87(void)
{
    word_4e87++;
    if (word_4e87 == 0x2a00)
        word_4e87 = 0x1c00;
}

/*
 * 0x02ac0
 *
 * Recompute the gravity and the velocity limit for **every kind** - all 0x3a
 * of them - from two settings at DGROUP 0x50b3 and 0x50b5. This is what a
 * change of speed or gravity in the game's controls has to run.
 *
 * Both settings are first bent by a piecewise rule: the one at 0x50b5 is
 * quartered and incremented below 0x8c, doubled above 0x116, and left alone
 * between; the one at 0x50b3 is halved below 0x46 and multiplied by sixteen
 * otherwise. Neither is a smooth curve and both are transcribed as the three
 * cases they are.
 *
 * The gravity is then a ratio, computed in 32 bits through `mul16x16` and the
 * runtime's long divide, and **which way round** depends on the comparison: a
 * kind heavier than the setting gets `s - base*s/v`, a lighter one gets
 * `v*s/base - s`, and an equal one gets zero. So the sign falls out of the
 * order rather than from a negation.
 *
 * Two kinds are special-cased by index: 0x14 and 0x2b take a fixed limit of
 * 0x3000, and 0x14 also has its gravity forced back to zero - after it has
 * just been computed.
 */
void recompute_kind_physics(void)
{
    int16_t s = DG16(0x50B5);
    int16_t base;
    int16_t i;

    if (s < 0x8C)
        s = (int16_t)((s >> 2) + 1);
    else if (s > 0x116)
        s = (int16_t)(s << 1);

    base = DG16(0x50B3);
    if (base < 0x46)
        base = (int16_t)(base >> 1);
    else
        base = (int16_t)(base << 4);

    for (i = 0; i < 0x3A; i++) {
        uint16_t entry = (uint16_t)(0xEA6 + i * 0x3A);
        int16_t v = DG16(entry);
        int16_t g;

        if (v == base) {
            g = 0;
        } else if (v > base) {
            int32_t q = (int32_t)mul16x16(base, s) / (int32_t)v;
            g = (int16_t)(s - (int16_t)q);
        } else {
            int32_t q = (int32_t)mul16x16(v, s) / (int32_t)base;
            g = (int16_t)((int16_t)q - s);
        }
        DG16(entry + 8) = g;

        if (i == 0x14 || i == 0x2B) {
            DG16(entry + 0x0A) = 0x3000;
            if (i == 0x14)
                DG16(entry + 8) = 0;
        } else {
            DG16(entry + 0x0A) = (int16_t)(0x2600 - base);
        }
    }
}

/*
 * 0x02bcc
 *
 * Clamp the two signed words at +0x36 and +0x38 of a record to plus or minus
 * a limit that depends on the record's kind.
 *
 * The kind is the word at +4, and it indexes a table of 0x3a-byte entries
 * starting at DGROUP 0xea6; the limit is the word at +0x0a of that entry. The
 * negative bound is computed as `0 - limit` each time rather than kept, so a
 * limit of 0 pins the field to 0.
 *
 * The entry address is recomputed into BX before every single access in the
 * original - six times - and that is transcribed rather than hoisted.
 */
void clamp_record_pair(uint16_t rec)
{
    uint16_t entry = (uint16_t)(0xEA6 + (uint16_t)(DG16(rec + 4) * 0x3A));

    if (DG16(rec + 0x38) > DG16(entry + 0x0A))
        DG16(rec + 0x38) = DG16(entry + 0x0A);
    else if (DG16(rec + 0x38) < (int16_t)(0 - DG16(entry + 0x0A)))
        DG16(rec + 0x38) = (int16_t)(0 - DG16(entry + 0x0A));

    if (DG16(rec + 0x36) > DG16(entry + 0x0A))
        DG16(rec + 0x36) = DG16(entry + 0x0A);
    else if (DG16(rec + 0x36) < (int16_t)(0 - DG16(entry + 0x0A)))
        DG16(rec + 0x36) = (int16_t)(0 - DG16(entry + 0x0A));
}

/*
 * 0x02c39
 *
 * Apply the kind's gravity to a record's vertical velocity, clamp both axes,
 * and work out a speed.
 *
 * The gravity is the word at +8 of the kind entry, added to the velocity at
 * +0x38; `clamp_record_pair` then holds both axes inside the kind's limit.
 *
 * The speed is **Manhattan**, not Euclidean: the absolute values of the two
 * velocities are added - each with the branchless `cwd / xor / sub` - and the
 * sum multiplied by the record's own scale at +0x3a. The 32-bit product is
 * stored across +0x3c and +0x3e, low half first, so that pair is a long.
 */
void apply_gravity_and_speed(uint16_t rec)
{
    uint16_t entry = (uint16_t)(0xEA6 + (uint16_t)(DG16(rec + 4) * 0x3A));
    int16_t vx, vy;
    uint32_t speed;

    DG16(rec + 0x38) = (int16_t)(DG16(rec + 0x38) + DG16(entry + 8));
    clamp_record_pair(rec);

    vx = DG16(rec + 0x36);
    if (vx < 0)
        vx = (int16_t)-vx;
    vy = DG16(rec + 0x38);
    if (vy < 0)
        vy = (int16_t)-vy;

    speed = mul16x16((int16_t)(vx + vy), DG16(rec + 0x3A));
    DG16(rec + 0x3E) = (int16_t)(speed >> 16);
    DG16(rec + 0x3C) = (int16_t)speed;
}

/*
 * 0x03a61
 *
 * Is `node` on the chain hanging off `rec`? Only records whose type word at
 * +4 is 0x11 have such a chain; anything else answers no without looking.
 * The chain is linked through the word at +0x78, by **near** pointer.
 */
int16_t chain_contains(uint16_t rec, uint16_t node)
{
    uint16_t p;

    if (DG16(rec + 4) != 0x11)
        return 0;

    p = DGU16(rec + 0x78);
    while (p != 0) {
        if (p == node)
            return 1;
        p = DGU16(p + 0x78);
    }
    return 0;
}

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
void rotate_point(uint16_t px, uint16_t py, uint16_t angle)
{
    int16_t c = angle_cos(angle);
    int16_t s = angle_sin(angle);
    int32_t nx, ny;

    nx = (int32_t)mul16x16(DG16(px), c) - (int32_t)mul16x16(DG16(py), s);
    ny = (int32_t)mul16x16(DG16(px), s) + (int32_t)mul16x16(DG16(py), c);

    DG16(px) = (int16_t)(nx >> 14);
    DG16(py) = (int16_t)(ny >> 14);
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
int16_t intersect_segments(uint16_t seg1, uint16_t seg2, uint16_t out)
{
    int16_t a1 = (int16_t)(DG16(seg1 + 2) - DG16(seg1 + 6));
    int16_t b1 = (int16_t)(DG16(seg1) - DG16(seg1 + 4));
    int16_t c1 = (int16_t)((int16_t)(DG16(seg1 + 4) * a1)
                           - (int16_t)(DG16(seg1 + 6) * b1));

    int16_t a2 = (int16_t)(DG16(seg2 + 6) - DG16(seg2 + 2));
    int16_t b2 = (int16_t)(DG16(seg2 + 4) - DG16(seg2));
    int16_t c2 = (int16_t)((int16_t)(DG16(seg2) * a2)
                           - (int16_t)(DG16(seg2 + 2) * b2));

    int16_t denom = (int16_t)((int16_t)(a2 * b1) - (int16_t)(a1 * b2));
    int16_t x, y;

    if (denom != 0) {
        int32_t nx = (int32_t)mul16x16(c2, b1) - (int32_t)mul16x16(c1, b2);
        int32_t ny = (int32_t)mul16x16(a1, c2) - (int32_t)mul16x16(a2, c1);

        x = (int16_t)(nx / denom);
        y = (int16_t)(ny / denom);
    } else {
        int16_t t = (int16_t)((int16_t)(DG16(seg1) * a2)
                              + (int16_t)(DG16(seg1 + 2) * b2));
        if (t != 0) {
            x = 0;
            y = 0;
        } else {
            x = DG16(seg1 + 4);
            y = DG16(seg1 + 6);
        }
    }

    DG16(out) = x;
    DG16(out + 2) = y;

    if (!value_between((uint16_t)x, DGU16(seg1), DGU16(seg1 + 4)))
        return 0;
    if (!value_between((uint16_t)x, DGU16(seg2), DGU16(seg2 + 4)))
        return 0;
    if (!value_between((uint16_t)y, DGU16(seg1 + 2), DGU16(seg1 + 6)))
        return 0;
    if (!value_between((uint16_t)y, DGU16(seg2 + 2), DGU16(seg2 + 6)))
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
void step_pair_apart(uint16_t rec)
{
    int16_t d = (int16_t)(DG16(rec + 4) - DG16(rec));

    if (d > 0)
        DG16(rec + 4)++;
    else if (d < 0)
        DG16(rec + 4)--;

    d = (int16_t)(DG16(rec + 6) - DG16(rec + 2));
    if (d > 0)
        DG16(rec + 6)++;
    else if (d < 0)
        DG16(rec + 6)--;
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
 * 0x04b53
 *
 * Are two points within 140 of each other in both axes?
 *
 * The absolute value is the branchless `cwd / xor ax,dx / sub ax,dx`: sign
 * extend into DX, exclusive-or, subtract. Both axes must pass; the first
 * failure answers 0 immediately.
 */
int16_t points_within_140(uint16_t a, uint16_t b)
{
    int16_t d = (int16_t)(DG16(a) - DG16(b));

    if (d < 0)
        d = (int16_t)-d;
    if (d > 0x8C)
        return 0;

    d = (int16_t)(DG16(a + 2) - DG16(b + 2));
    if (d < 0)
        d = (int16_t)-d;
    if (d > 0x8C)
        return 0;

    return 1;
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
void compute_link_endpoints(uint16_t link)
{
    uint16_t a = DGU16(link + 4);
    uint16_t b = DGU16(link + 6);
    int16_t dx, dy;
    int16_t a_dx1, a_dy1, a_dx2, a_dy2;
    int16_t b_dx1, b_dy1, b_dx2, b_dy2;

    DG16(link + 8)   = (int16_t)(DG16(a + 0x2A) + DG8(a + 0x56));
    DG16(link + 0xA) = (int16_t)(DG16(a + 0x2C) + DG8(a + 0x57));
    DG16(link + 0xC) = (int16_t)(DG16(b + 0x2A) + DG8(b + 0x56));
    DG16(link + 0xE) = (int16_t)(DG16(b + 0x2C) + DG8(b + 0x57));

    dx = (int16_t)(DG16(link + 8) - DG16(link + 0xC));
    if (dx < 0)
        dx = (int16_t)-dx;
    dy = (int16_t)(DG16(link + 0xA) - DG16(link + 0xE));
    if (dy < 0)
        dy = (int16_t)-dy;

    if (dx < dy) {
        b_dx1 = 0;
        a_dx1 = 0;
        a_dx2 = DG16(a + 0x58);
        a_dy2 = a_dy1 = (int16_t)(a_dx2 >> 1);
        b_dx2 = DG16(b + 0x58);
        b_dy2 = b_dy1 = (int16_t)(b_dx2 >> 1);
    } else {
        b_dy1 = 0;
        a_dy1 = 0;
        a_dy2 = DG16(a + 0x58);
        a_dx2 = a_dx1 = (int16_t)(a_dy2 >> 1);
        b_dy2 = DG16(b + 0x58);
        b_dx2 = b_dx1 = (int16_t)(b_dy2 >> 1);
    }

    DG16(link + 0x10) = (int16_t)(DG16(link + 8) + a_dx2);
    DG16(link + 0x12) = (int16_t)(DG16(link + 0xA) + a_dy2);
    DG16(link + 0x14) = (int16_t)(DG16(link + 0xC) + b_dx2);
    DG16(link + 0x16) = (int16_t)(DG16(link + 0xE) + b_dy2);

    DG16(link + 8)   = (int16_t)(DG16(link + 8) + a_dx1);
    DG16(link + 0xA) = (int16_t)(DG16(link + 0xA) + a_dy1);
    DG16(link + 0xC) = (int16_t)(DG16(link + 0xC) + b_dx1);
    DG16(link + 0xE) = (int16_t)(DG16(link + 0xE) + b_dy1);
}

/*
 * 0x04f7f
 *
 * Recompute a link's endpoint coordinates from the objects it joins, and then
 * set the rest lengths those endpoints imply.
 *
 * The endpoints land in the +0x14 array `compare_link_ends` and
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
void refresh_link_geometry(uint16_t link)
{
    uint16_t a, b, chain, holder, pt;
    int16_t idx, j;

    a = DGU16(link + 2);
    if (a == 0)
        return;

    idx = DG8(link + 0xa);
    DG16(link + 0x14) = (int16_t)(DG16(a + 0x2a) + DG8(a + 0x6a + 2 * idx));
    DG16(link + 0x16) = (int16_t)(DG16(a + 0x2c) + DG8(a + 0x6b + 2 * idx));

    b = DGU16(link + 4);
    if (b != 0) {
        int16_t k = DG8(link + 0xb);

        DG16(link + 0x18) = (int16_t)(DG16(b + 0x2a) + DG8(b + 0x6a + 2 * k));
        DG16(link + 0x1a) = (int16_t)(DG16(b + 0x2c) + DG8(b + 0x6b + 2 * k));
    }

    chain = DGU16(a + 0x5a + 2 * idx);
    while (chain != 0 && DG16(chain + 4) == 7) {
        for (j = 0; j < 2; j++) {
            pt = (uint16_t)(DGU16(chain + 0x66) + 4 * j);
            DG16(pt + 0x14) = (int16_t)(DG16(chain + 0x1e)
                                        + DG8(chain + 0x6a + 2 * j));
            DG16(pt + 0x16) = (int16_t)(DG16(chain + 0x20)
                                        + DG8(chain + 0x6b + 2 * j));
        }
        chain = DGU16(chain + 0x5a);
    }

    if (DGU16(0x4e6b) == 0x2000)
        return;

    holder = DGU16(link);
    DG16(holder + 0x96) = link_end_distance(link, 3, 0);
    holder = DGU16(link);
    DG16(holder + 0x9c) = link_end_distance(link, 3, 1);
}

/*
 * 0x05646
 *
 * Insert a record into a **sorted doubly-linked list**, threaded through the
 * words at +0 (next) and +2 (previous). The walk holds a pointer to the *link
 * cell* rather than to a node - so it starts at the head variable itself and
 * the insertion is the same three assignments wherever it lands.
 *
 * The sort key depends on which list, and the two heads it knows are the same
 * two `pick_by_flag` reads:
 *
 *   0x50d7 - ordered on the word at +0x20 of the kind entry (table 0xec6)
 *   0x5179 - ordered on the word at +0x02 of the kind entry (table 0xea8)
 *
 * Any other head inserts at the front without comparing anything.
 *
 * The record's own key is computed once, before the walk; the 0x5179 case
 * recomputes both sides from the other table rather than reusing it.
 */
void insert_sorted(uint16_t rec, uint16_t head)
{
    int16_t kind = DG16(rec + 4);
    int16_t prio = DG16((uint16_t)(kind * 0x3A + 0xEC6));
    uint16_t di = head;
    int16_t stop = 0;

    for (;;) {
        if (stop)
            break;

        if (DGU16(di) == 0) {
            stop = 1;
        } else {
            uint16_t next = DGU16(di);
            int16_t kind2 = DG16(next + 4);

            if (head == 0x50D7) {
                stop = (prio < DG16((uint16_t)(kind2 * 0x3A + 0xEC6))) ? 1 : 0;
            } else if (head == 0x5179) {
                stop = (DG16((uint16_t)(kind * 0x3A + 0xEA8))
                        < DG16((uint16_t)(kind2 * 0x3A + 0xEA8))) ? 1 : 0;
            } else {
                stop = 1;
            }
        }

        if (stop == 0)
            di = DGU16(di);
    }

    DGU16(rec) = DGU16(di);
    DGU16(rec + 2) = di;
    DGU16(di) = rec;
    if (DGU16(rec) != 0)
        DGU16(DGU16(rec) + 2) = rec;
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
int16_t pick_by_flag(uint16_t flags)
{
    if (DG16(0x521B) != 0 && (flags & 0x2000))
        return DG16(0x521B);
    if (DG16(0x5179) != 0 && (flags & 0x1000))
        return DG16(0x5179);
    if (DG16(0x50D7) != 0 && (flags & 0x0800))
        return DG16(0x50D7);
    return 0;
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
int16_t pick_for_record(uint16_t rec, uint16_t flags)
{
    if (DG16(rec) != 0)
        return DG16(rec);

    if (DG16(rec + 6) & 0x2000)
        return pick_by_flag(flags);

    if ((DG16(rec + 6) & 0x1000) && (flags & 0x800))
        return DG16(0x50D7);

    return 0;
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
void add_record_shapes(uint16_t rec, uint16_t which)
{
    if (which & 1)
        alloc_shape((uint16_t)(rec + 0x32), (uint16_t)(rec + 0x4C), 1, 1, 0);
    if (which & 2)
        alloc_shape((uint16_t)(rec + 0x2E), (uint16_t)(rec + 0x48), 1, 2, 0);
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
void alloc_shape(uint16_t pt1, uint16_t pt2, uint8_t flags, uint8_t which,
                 int16_t width)
{
    uint16_t off = DGU16(0x4E4E), seg = DGU16(0x4E50);

    /* Pop from the free list, push onto the used list. */
    DGU16(0x4E50) = FARU16(seg, off + 2);
    DGU16(0x4E4E) = FARU16(seg, off);
    FARU16(seg, off + 2) = DGU16(0x4E54);
    FARU16(seg, off) = DGU16(0x4E52);
    DGU16(0x4E54) = seg;
    DGU16(0x4E52) = off;

    if ((uint16_t)(off | seg) == 0)
        return;

    FAR8(seg, off + 4) = flags;
    FAR8(seg, off + 5) = which;
    FAR16(seg, off + 6) = DG16(pt1);
    FAR16(seg, off + 8) = DG16(pt1 + 2);
    FAR16(seg, off + 0x0A) = DG16(pt2);
    FAR16(seg, off + 0x0C) = DG16(pt2 + 2);
    FAR16(seg, off + 0x0E) = width;

    if (which == 1) {
        FAR16(seg, off + 6) -= DG16(0x4E9B);
        FAR16(seg, off + 8) -= DG16(0x4E99);
        if (flags & 4) {
            FAR16(seg, off + 0x0A) -= DG16(0x4E9B);
            FAR16(seg, off + 0x0C) -= DG16(0x4E99);
        }
    } else {
        FAR16(seg, off + 6) -= DG16(0x4E9F);
        FAR16(seg, off + 8) -= DG16(0x4E9D);
        if (flags & 4) {
            FAR16(seg, off + 0x0A) -= DG16(0x4E9F);
            FAR16(seg, off + 0x0C) -= DG16(0x4E9D);
        }
    }

    if (FAR8(seg, off + 4) & 4) {
        int16_t hi;

        if (FAR16(seg, off + 6) < FAR16(seg, off + 0x0A)) {
            FAR16(seg, off + 0x10) = FAR16(seg, off + 6);
            hi = FAR16(seg, off + 0x0A);
        } else {
            FAR16(seg, off + 0x10) = FAR16(seg, off + 0x0A);
            hi = FAR16(seg, off + 6);
        }
        FAR16(seg, off + 0x12) = hi;

        if (FAR16(seg, off + 8) < FAR16(seg, off + 0x0C)) {
            FAR16(seg, off + 0x14) = FAR16(seg, off + 8);
            hi = FAR16(seg, off + 0x0C);
        } else {
            FAR16(seg, off + 0x14) = FAR16(seg, off + 0x0C);
            hi = FAR16(seg, off + 8);
        }
        FAR16(seg, off + 0x16) = hi;
        FAR16(seg, off + 0x16) += (int16_t)(FAR16(seg, off + 0x0E) >> 1);
    } else {
        FAR16(seg, off + 0x10) = FAR16(seg, off + 6);
        FAR16(seg, off + 0x14) = FAR16(seg, off + 8);
        FAR16(seg, off + 0x12) = (int16_t)(FAR16(seg, off + 0x10)
                                           + FAR16(seg, off + 0x0A));
        FAR16(seg, off + 0x16) = (int16_t)(FAR16(seg, off + 0x14)
                                           + FAR16(seg, off + 0x0C));
    }
}

/*
 * 0x06de9
 *
 * Classify how a link's two endpoints sit against the endpoints they connect
 * to, and answer a small bit code.
 *
 * A link carries two connected objects, at +2 and +4, and a byte index into
 * each, at +0xa and +0xb. `end` selects which of the two is looked at first
 * and swaps the pair; everything below is written in terms of that choice and
 * its opposite. Endpoint coordinates live in an array of two-word points at
 * +0x14 - x at +0x14, y at +0x16, four bytes apart - so `+ 4 * which` picks an
 * endpoint and the two offsets pick its axis.
 *
 * Each object's byte index selects a word from a table at +0x5a. If the second
 * object *is* what the first's table names, the link is its own partner on
 * both sides; otherwise each table entry's +0x66 names the partner. Either way
 * the partner of the near end is indexed by the far one and vice versa.
 *
 * The original computes those two indices as `1 - end` and `1 - opposite`,
 * which are just `opposite` and `end` again - it was written twice in the
 * source and the compiler did not fold it. Folded here.
 *
 * One x comparison sets bit 3 or bit 4, then two y comparisons choose 1, 2 or
 * 4. `reversed` flips the sense of both y comparisons, and not quite
 * symmetrically: the unreversed side tests strictly greater where the reversed
 * side tests greater-or-equal, so a tie goes to 4 one way and 2 the other.
 */
int16_t compare_link_ends(uint16_t link, int16_t end, int16_t reversed)
{
    int16_t opposite = 1 - end;
    uint16_t obj_a, obj_b, p_obj, q_obj;
    int16_t idx_a, idx_b, bits;
    uint16_t ent_a, ent_b;

    if (end != 0) {
        obj_a = DGU16(link + 4);
        idx_a = DG8(link + 0xb);
        obj_b = DGU16(link + 2);
        idx_b = DG8(link + 0xa);
    } else {
        obj_a = DGU16(link + 2);
        idx_a = DG8(link + 0xa);
        obj_b = DGU16(link + 4);
        idx_b = DG8(link + 0xb);
    }

    ent_a = DGU16(obj_a + 2 * idx_a + 0x5a);
    ent_b = DGU16(obj_b + 2 * idx_b + 0x5a);

    if (obj_b == ent_a) {
        p_obj = link;
        q_obj = link;
    } else {
        p_obj = DGU16(ent_a + 0x66);
        q_obj = DGU16(ent_b + 0x66);
    }

    if (DG16(link + 0x14 + 4 * opposite) > DG16(q_obj + 0x14 + 4 * end))
        bits = 8;
    else
        bits = 0x10;

    if (reversed == 0) {
        if (DG16(link + 0x16 + 4 * end) > DG16(p_obj + 0x16 + 4 * opposite))
            return 1;
        if (DG16(link + 0x16 + 4 * opposite) > DG16(q_obj + 0x16 + 4 * end))
            return (int16_t)(2 | bits);
        return (int16_t)(4 | bits);
    }

    if (DG16(link + 0x16 + 4 * end) < DG16(p_obj + 0x16 + 4 * opposite))
        return 1;
    if (DG16(link + 0x16 + 4 * opposite) >= DG16(q_obj + 0x16 + 4 * end))
        return (int16_t)(2 | bits);
    return (int16_t)(4 | bits);
}

/*
 * 0x06f43
 *
 * Say which of two fields of a structure matches a value: 0 for the field at
 * +0x5a, 1 for the one at +0x5c, and -1 for neither. The structure is reached
 * by a **near** pointer - a DGROUP offset - so it is indexed off DGROUP here.
 */
int16_t match_field_5a_5c(int16_t value, uint16_t obj)
{
    if (DG16(obj + 0x5A) == value)
        return 0;
    if (DG16(obj + 0x5C) == value)
        return 1;
    return -1;
}

/*
 * 0x06f68
 *
 * Given a record reached by a **near** pointer, answer the word at +4 if the
 * word at +2 matches, and the word at +2 itself if it does not. A null record
 * answers 0.
 *
 * Both the "matched" and "did not match" paths funnel through one `jmp` to the
 * epilogue, which is why the disassembly has three jumps to reach two results.
 */
int16_t select_field_2_or_4(int16_t key, uint16_t rec)
{
    if (rec == 0)
        return 0;
    if (DG16(rec + 2) == key)
        return DG16(rec + 4);
    return DG16(rec + 2);
}

/*
 * 0x06f8e
 *
 * Measure how far a link's endpoint is from the endpoint it joins, in
 * whichever coordinate array `mode` names.
 *
 * The partner is found the same way `compare_link_ends` finds it: the byte
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
int16_t link_end_distance(uint16_t link, int16_t gen, int16_t end)
{
    uint16_t partner, ent;
    int16_t near_i, far_i, base, dx, dy;

    if (end == 0) {
        near_i = 0;
        ent = DGU16(DGU16(link + 2) + 2 * DG8(link + 0xa) + 0x5a);
        if (DGU16(link + 4) == ent) {
            partner = link;
            far_i = 1;
        } else {
            partner = DGU16(ent + 0x66);
            far_i = 0;
        }
    } else {
        near_i = 1;
        ent = DGU16(DGU16(link + 4) + 2 * DG8(link + 0xb) + 0x5a);
        if (DGU16(link + 2) == ent) {
            partner = link;
            far_i = 0;
        } else {
            partner = DGU16(ent + 0x66);
            far_i = 1;
        }
    }

    if (partner == 0)
        return 0;

    if (gen == 1)
        base = 0x24;
    else if (gen == 2)
        base = 0x1c;
    else
        base = 0x14;

    dx = abs16((int16_t)(DG16(link + base + 4 * near_i)
                         - DG16(partner + base + 4 * far_i)));
    dy = abs16((int16_t)(DG16(link + base + 2 + 4 * near_i)
                         - DG16(partner + base + 2 + 4 * far_i)));

    if (dy > dx)
        return (int16_t)(dy + (dx >> 2) + (dx >> 3));
    return (int16_t)(dx + (dy >> 2) + (dy >> 3));
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
int16_t link_slack(uint16_t obj, uint16_t link, int16_t gen)
{
    uint16_t base, ent, holder;
    int16_t rest;

    if (DG16(obj + 4) == 7)
        base = obj;
    else
        base = (uint16_t)(obj + 2 * DG8(link + 0xa));
    ent = DGU16(base + 0x5a);

    holder = DGU16(link);

    if (DGU16(link + 2) == obj) {
        if (gen == 1)
            rest = DG16(holder + 0x9a);
        else if (gen == 2)
            rest = DG16(holder + 0x98);
        else
            rest = DG16(holder + 0x96);
        return (int16_t)(rest - link_end_distance(link, gen, 0));
    }

    if (ent != 0 && DGU16(link + 4) == ent) {
        if (gen == 1)
            rest = DG16(holder + 0xa0);
        else if (gen == 2)
            rest = DG16(holder + 0x9e);
        else
            rest = DG16(holder + 0x9c);
        return (int16_t)(rest - link_end_distance(link, gen, 1));
    }

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
void set_vector_from_angle(uint16_t obj, uint16_t angle, int16_t mag)
{
    int32_t sn = (int32_t)mul16x16(mag, angle_sin(angle));
    int32_t cs = (int32_t)mul16x16(mag, angle_cos(angle));

    DG16(obj + 0x36) = (int16_t)(sn >> 14);
    DG16(obj + 0x38) = (int16_t)(cs >> 14);
}

/*
 * 0x07283
 *
 * Recompute a record's velocity from how far it has moved, then clamp it.
 *
 * The position is at +0x1e and +0x20 - the same pair `compute_bounds_53fe`
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
void update_velocity(uint16_t rec, uint8_t shift_x, uint8_t shift_y,
                     uint16_t which)
{
    if (which & 1) {
        DG16(rec + 0x36) = (int16_t)(DG16(rec + 0x1E) - DG16(rec + 0x22));
        DG16(rec + 0x36) = (int16_t)((uint16_t)DG16(rec + 0x36)
                                     << (uint8_t)(9 - shift_x));
    }
    if (which & 2) {
        DG16(rec + 0x38) = (int16_t)(DG16(rec + 0x20) - DG16(rec + 0x24));
        DG16(rec + 0x38) = (int16_t)((uint16_t)DG16(rec + 0x38)
                                     << (uint8_t)(9 - shift_y));
    }
    clamp_record_pair(rec);
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
int16_t link_endpoint_gap(uint16_t link, uint16_t obj,
                          uint16_t out_dx, uint16_t out_dy)
{
    uint16_t self, other, pt;
    int16_t idx, facing, x1, y1, x2, y2, adx, ady;

    if (DGU16(link + 2) == obj) {
        self = obj;
        idx = DG8(link + 0xa);
    } else {
        self = DGU16(link + 4);
        idx = DG8(link + 0xb);
    }

    x1 = (int16_t)(DG16(self + 0x2a) + DG8(self + 0x6a + 2 * idx));
    y1 = (int16_t)(DG16(self + 0x2c) + DG8(self + 0x6b + 2 * idx));

    other = DGU16(self + 0x5a + 2 * idx);
    facing = match_field_5a_5c((int16_t)self, other);

    if (DG16(other + 4) == 7) {
        pt = (uint16_t)(DGU16(other + 0x66) + 4 * (1 - facing));
        x2 = DG16(pt + 0x14);
        y2 = DG16(pt + 0x16);
    } else {
        x2 = (int16_t)(DG16(other + 0x2a) + DG8(other + 0x6a + 2 * facing));
        y2 = (int16_t)(DG16(other + 0x2c) + DG8(other + 0x6b + 2 * facing));
    }

    DG16(out_dx) = (int16_t)(x1 - x2);
    DG16(out_dy) = (int16_t)(y1 - y2);

    adx = abs16(DG16(out_dx));
    ady = abs16(DG16(out_dy));

    if (ady > adx)
        return (int16_t)((adx >> 2) + (adx >> 3) + ady);
    return (int16_t)((ady >> 2) + (ady >> 3) + adx);
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
    uint16_t last, next;

    if (DGU16(0x4E58) == 0)
        return;

    last = DGU16(0x4E58);
    next = DGU16(last);
    while (next != 0) {
        last = next;
        next = DGU16(next);
    }

    DGU16(last) = DGU16(0x4E56);
    DGU16(0x4E56) = DGU16(0x4E58);
    DGU16(0x4E58) = 0;
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
    int16_t obj;

    if (DG16(0x50d5) != 0)
        shift_state_history(DGU16(0x50d5));

    obj = pick_by_flag(0x3000);
    while (obj != 0) {
        if (obj != DG16(0x50d5))
            shift_state_history((uint16_t)obj);
        obj = pick_for_record((uint16_t)obj, 0x1000);
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
void shift_state_history(uint16_t obj)
{
    uint16_t sub;

    DG32(obj + 0x26) = DG32(obj + 0x22);
    DG32(obj + 0x22) = DG32(obj + 0x1e);
    DG32(obj + 0x32) = DG32(obj + 0x2e);
    DG32(obj + 0x2e) = DG32(obj + 0x2a);
    DG32(obj + 0x4c) = DG32(obj + 0x48);
    DG32(obj + 0x48) = DG32(obj + 0x44);
    DG16(obj + 0x10) = DG16(obj + 0xe);
    DG16(obj + 0xe) = DG16(obj + 0xc);

    if (DG16(obj + 4) == 8 && DGU16(0x4e6b) == 0x1000) {
        sub = DGU16(obj + 0x54);
        DG32(sub + 0x28) = DG32(sub + 0x18);
        DG32(sub + 0x18) = DG32(sub + 0x08);
        DG32(sub + 0x2c) = DG32(sub + 0x1c);
        DG32(sub + 0x1c) = DG32(sub + 0x0c);
        DG32(sub + 0x30) = DG32(sub + 0x20);
        DG32(sub + 0x20) = DG32(sub + 0x10);
        DG32(sub + 0x34) = DG32(sub + 0x24);
        DG32(sub + 0x24) = DG32(sub + 0x14);
    }

    if (DG16(obj + 4) == 0xa || DG16(obj + 4) == 7) {
        sub = DGU16(obj + 0x66);
        DG16(sub + 0x12) = DG16(sub + 0x10);
        DG16(sub + 0x10) = DG16(sub + 0xe);
        DG32(sub + 0x24) = DG32(sub + 0x1c);
        DG32(sub + 0x1c) = DG32(sub + 0x14);
        DG32(sub + 0x28) = DG32(sub + 0x20);
        DG32(sub + 0x20) = DG32(sub + 0x18);
    }

    DG16(obj + 0x9a) = DG16(obj + 0x98);
    DG16(obj + 0x98) = DG16(obj + 0x96);
    DG16(obj + 0xa0) = DG16(obj + 0x9e);
    DG16(obj + 0x9e) = DG16(obj + 0x9c);
}

/*
 * 0x0811b
 *
 * A one-call forwarder to `clear_flag_2d44`, in the same segment, reached from
 * 48 sites. Whatever the flag means, this is how most of the program clears
 * it; the sibling at 0x08125 is how it is set again, guarded by 0x52f2.
 */
void clear_flag_2d44_thunk(void)
{
    clear_flag_2d44();
}

/*
 * 0x08136
 *
 * Advance the button state for one frame. Three states live in DGROUP 0x5774
 * and the previous frame's is kept at 0x286e:
 *
 *   0  not pressed
 *   1  held
 *   2  the frame of a change
 *
 * It waits for the frame first, takes the two flag bits, and then walks the
 * state machine. The last test - a 2 while the previous frame was also 2
 * becomes a 1 - is what stops "changed" lasting two frames in a row.
 *
 * The third branch is `cmp [0x5774],0 / je`, so anything that is not zero
 * becomes 1: the state is normalised, not merely tested.
 */
void update_button_state(void)
{
    int16_t prev;

    wait_and_latch_frame();
    prev = DG16(0x5774);

    if (flag_bit_48ea(0))
        DG16(0x5774) = 1;
    if (flag_bit_48ea(1))
        DG16(0x5772) = 2;

    if (prev == 2 && DG16(0x286E) != 1) {
        DG16(0x5774) = 2;
    } else if (DG16(0x5774) == 1 && DG16(0x286E) == 0) {
        DG16(0x5774) = 2;
    } else if (DG16(0x5774) != 0) {
        DG16(0x5774) = 1;
    } else {
        DG16(0x5774) = 0;
    }

    if (DG16(0x5774) == 2 && DG16(0x286E) == 2)
        DG16(0x5774) = 1;

    DG16(0x286E) = DG16(0x5774);
}

/*
 * 0x081cc
 *
 * Present the frame. Three paths, chosen by two DGROUP flags: an optional
 * call to the routine at 0x0e34a first, then either a hook at 0x0b078 or the
 * driver's page flip. Called from sixteen places.
 */
void present_frame(uint16_t wait_retrace)
{
    if (present_hook_a != 0)
        sub_0e34a(1);

    if (present_hook_b != 0)
        sub_0b078();
    else
        vm_show_page(wait_retrace);
}

/*
 * 0x082c3
 *
 * Set the clip box: to the saved rectangle at DGROUP 0x52d7..0x52dd when the
 * mode word at 0x4e6b is any of seven values, and to a fixed one otherwise.
 *
 * The seven are single bits - 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000,
 * 0x8000 - but they are compared **for equality**, one at a time, not tested
 * as a mask, so a word with two of them set matches none. Transcribed as seven
 * compares rather than folded into a mask test.
 *
 * The saved rectangle is stored in descending order - 0x52dd is the left edge
 * and 0x52d7 the bottom - which is worth saying because it looks like a
 * transcription error otherwise.
 *
 * The fixed box is 0x110,0x48 to 0x20f,0xe7: 256 wide by 160 tall.
 */
void set_clip_for_mode(void)
{
    uint16_t mode = DGU16(0x4E6B);

    if (mode == 0x2000 || mode == 0x1000 || mode == 0x200 || mode == 0x8000
        || mode == 0x4000 || mode == 0x800 || mode == 0x400) {
        clip_left = DG16(0x52DD);
        clip_right = DG16(0x52DB);
        clip_top = DG16(0x52D9);
        clip_bottom = DG16(0x52D7);
    } else {
        clip_left = 0x110;
        clip_right = 0x20F;
        clip_top = 0x48;
        clip_bottom = 0xE7;
    }
}

/*
 * 0x0834b
 *
 * Set the clipping box to the whole visible screen: 0,0 to 639,399. The
 * bottom is 0x18f, which is the blanking line the CRTC is programmed with -
 * so the clip box is the *visible* 400 rows, not the 480 the mode scans.
 */
void set_clip_full_screen(void)
{
    clip_left = 0;
    clip_top = 0;
    clip_right = 0x27F;
    clip_bottom = 0x18F;
}

/*
 * 0x08f77
 *
 * Program the CRTC to blank after `lines` scan lines. The count is ten bits
 * and the hardware spreads it over three registers: the low eight in Start
 * Vertical Blank, bit 8 in Overflow bit 3, bit 9 in Maximum Scan Line bit 5.
 * Overflow and Maximum Scan Line are read back first so the other timing bits
 * the BIOS put there survive.
 *
 * Called with 0x1d6 (470) for the Sierra logo and 0x18f (399) for the game's
 * own 640x400 screens. Vertical Display End is never touched, so the CRTC goes
 * on scanning 480 lines and simply blanks the tail - which is what lets two
 * 640x400 pages fit in one 64 KB plane.
 */
void vm_set_display_lines(uint16_t lines)
{
    uint8_t v;

    io_out8(PORT_CRTC_INDEX, 0x15);
    io_out8(PORT_CRTC_DATA, (uint8_t)(lines & 0xFF));

    io_out8(PORT_CRTC_INDEX, 0x07);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xF7) | (((lines >> 8) & 1) << 3));
    io_out8(PORT_CRTC_DATA, v);

    io_out8(PORT_CRTC_INDEX, 0x09);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xDF) | (((lines >> 8) & 2) << 4));
    io_out8(PORT_CRTC_DATA, v);
}

/*
 * 0x098e0 (the scan below is the loop this routine has three inlined copies of)
 *
 * Walk each list once, from its head to either a null entry or a match.
 */
static void scan_entry_list(int16_t idx, uint16_t want_off, uint16_t want_seg,
                            uint16_t *off, uint16_t *seg)
{
    *seg = DGU16((uint16_t)(idx * 0x1c) + 0x54a9);
    *off = DGU16((uint16_t)(idx * 0x1c) + 0x54a7);

    for (;;) {
        uint8_t *p = FAR_PTR(*seg, *off);
        uint16_t e_off = *(uint16_t *)p;
        uint16_t e_seg = *(uint16_t *)(p + 2);

        if ((e_off | e_seg) == 0)
            return;
        if (e_seg == want_seg && e_off == want_off)
            return;
        *off = (uint16_t)(*off + 8);
    }
}

/*
 * 0x098e0
 *
 * Find which record owns a far pointer, and answer whether one does.
 *
 * The pointer looked for is not an argument - it is the pair of globals at
 * 0x5482 and 0x5484. Records are 0x1c bytes from 0x54a7, and each one's first
 * field is a far pointer to a list of eight-byte entries, a key pointer at +0
 * and a value pointer at +4, ending at an all-zero key. 0x5480 holds the
 * record last used and 0x547e the highest valid index.
 *
 * The search starts at 0x5480 - or at 1 if that is zero, so record 0 is never
 * where a search begins - and then spirals outward one index at a time,
 * forward first and backward second, re-reading both globals on every pass.
 * That is a locality bet: the pointer being asked about is usually in the
 * record that was just used.
 *
 * Each step scans a whole list, so a step can stop on a null entry rather than
 * a match; the outer loop re-checks and keeps going while either direction has
 * indices left. Whichever record was scanned last is the one reported, so the
 * final check decides between a real hit and having simply run out.
 *
 * On success the caller's block takes the record index, the entry's value
 * pointer, and two zeroed 32-bit fields at +6 and +0xa.
 */
int16_t find_entry_for_pointer(uint16_t out)
{
    uint16_t want_off = DGU16(0x5482);
    uint16_t want_seg = DGU16(0x5484);
    uint16_t off, seg;
    int16_t idx, fwd, back;
    uint8_t *p;

    idx = DG16(0x5480);
    if (idx == 0)
        idx = 1;
    scan_entry_list(idx, want_off, want_seg, &off, &seg);

    fwd = (int16_t)(DG16(0x5480) + 1);
    back = (int16_t)(DG16(0x5480) - 1);

    for (;;) {
        p = FAR_PTR(seg, off);
        if (*(uint16_t *)(p + 2) == want_seg && *(uint16_t *)p == want_off)
            break;
        if (back <= 0 && fwd > DG16(0x547e))
            break;

        if (fwd <= DG16(0x547e)) {
            idx = fwd++;
            scan_entry_list(idx, want_off, want_seg, &off, &seg);
        }

        p = FAR_PTR(seg, off);
        if (*(uint16_t *)(p + 2) == want_seg && *(uint16_t *)p == want_off)
            continue;
        if (back <= 0)
            continue;
        idx = back--;
        scan_entry_list(idx, want_off, want_seg, &off, &seg);
    }

    p = FAR_PTR(seg, off);
    if (*(uint16_t *)(p + 2) != want_seg || *(uint16_t *)p != want_off)
        return 0;

    DG16(out) = idx;
    DG16(out + 2) = *(int16_t *)(p + 4);
    DG16(out + 4) = *(int16_t *)(p + 6);
    DG32(out + 6) = 0;
    DG32(out + 0xa) = 0;
    return 1;
}

/*
 * 0x0a7a3
 *
 * Set the word at DGROUP 0x2d44 to zero, and nothing else.
 *
 * What the flag governs is **not established**. Its counterpart at 0x0a78e
 * sets it to 1 and then redraws through 0xacc3, passing the word at 0x38a4 -
 * which is inside the video driver's data block at 0x3890 - so the pair reads
 * like suspending and resuming something on screen. That is inference from the
 * shape of the two routines, not something measured, and the name says only
 * what the code does.
 */
void clear_flag_2d44(void)
{
    DG16(0x2d44) = 0;
}

/*
 * 0x0aaca
 *
 * Wait for the frame, then latch the input state for the frame about to be
 * drawn and clear the accumulators.
 *
 * The wait is the spin on `frame_pending` that the INT 08h handler releases -
 * and it is guarded, so when DGROUP 0x44ee is clear the routine does not wait
 * at all. That spin measured at 64% of all basic block executions under an
 * emulator paced on the host clock; see STATUS.md.
 *
 * The pair at 0x5782/0x5784 is filled either from `read_pair_4740` or from the
 * two words at 0x576c/0x576e, and the pair at 0x5768/0x576a is moved into
 * 0x5772/0x5774 and zeroed - accumulated since the last frame, then handed
 * over and reset, which is what a frame boundary looks like.
 */
void wait_and_latch_frame(void)
{
    if (DG8(0x44EE) != 0) {
        while (frame_pending())
            ;
    }

    if (DG16(0x2D42) != 0) {
        read_pair_4740(0x5784, 0x5782);
    } else {
        DG16(0x5784) = DG16(0x576E);
        DG16(0x5782) = DG16(0x576C);
    }

    DG16(0x5774) = DG16(0x576A);
    DG16(0x5772) = DG16(0x5768);
    DG16(0x5768) = 0;
    DG16(0x576A) = 0;
    frame_flag = 0;
}

/*
 * 0x0b078
 *
 * NOT TRANSCRIBED YET. Reached from the frame-presentation routine at 0x081cc
 * when DGROUP 0x52f2 is set. The address is known and the body is not read, so
 * it aborts rather than doing nothing: a silent no-op here would be a missing
 * frame that looks like a blitter fault.
 */
void sub_0b078(void)
{
    not_transcribed("0x0b078");
}

/*
 * 0x0b429
 *
 * Find the entry in the two-slot table at DGROUP 0x56e6 whose top bits match,
 * and claim it. The slots are 0x20 bytes apart, so the second is at 0x5706 -
 * and those are exactly the two words the initialisation below fills with the
 * driver's back and front pages, once, the first time through.
 *
 * The match is on bits **0xa800** only, not on the whole word, so a slot
 * matches a page that differs from it in the low bits. Answers the slot's
 * DGROUP offset, or 0 if neither matched.
 *
 * A `want` of 0 means "the page currently being drawn into".
 */
uint16_t claim_page_slot(uint16_t want)
{
    uint16_t si;
    int16_t i;

    if (DG16(0x2D46) != 0) {
        DGU16(0x56E6) = vga_page_back;
        DGU16(0x5706) = vga_page_front;
        DG16(0x2D46) = 0;
    }

    if (want == 0)
        want = vga_page_back;

    si = 0x56E6;
    for (i = 0; i < 2; i++, si = (uint16_t)(si + 0x20)) {
        if ((want & 0xA800) == (DGU16(si) & 0xA800)) {
            DGU16(si) = want;
            return si;
        }
    }
    return 0;
}

/*
 * 0x0b47f
 *
 * Save the driver's drawing state, or put it back: a non-zero argument saves,
 * zero restores. The state is the clip box, whether clipping is on, and the
 * two page segments - seven values, kept at DGROUP 0x5726..0x5732.
 *
 * `clip_enabled` is a byte and is saved **zero-extended into a word**, then
 * restored as a byte, so the high half of 0x5726 is always zero. Transcribed
 * with the same widths rather than made symmetrical.
 */
void save_or_restore_draw_state(int16_t save)
{
    if (save != 0) {
        DGU16(0x5726) = clip_enabled;
        DG16(0x5728) = clip_left;
        DG16(0x572A) = clip_right;
        DG16(0x572C) = clip_top;
        DG16(0x572E) = clip_bottom;
        DGU16(0x5732) = vga_page_dst;
        DGU16(0x5730) = vga_page_src;
    } else {
        clip_enabled = DG8(0x5726);
        clip_left = DG16(0x5728);
        clip_right = DG16(0x572A);
        clip_top = DG16(0x572C);
        clip_bottom = DG16(0x572E);
        vga_page_dst = DGU16(0x5732);
        vga_page_src = DGU16(0x5730);
    }
}

/*
 * 0x0b4e2
 *
 * Non-zero while `frame_flag` is still clear. The original is
 * `neg ax / sbb ax,ax / inc ax`, which is Borland's idiom for `ax = (ax == 0)`.
 *
 * The caller at 0x0aaca spins on this waiting for the INT 08h handler to set
 * the flag; that spin was 64% of all basic block executions under an emulator
 * paced on the host clock. See STATUS.md.
 */
int16_t frame_pending(void)
{
    return (int16_t)(frame_flag == 0);
}
/*
 * 0x0b4f1
 *
 * Clear the input state: two eight-byte blocks at DGROUP 0x5742, then the two
 * accumulators at 0x5768/0x576a and the two latched values at 0x5772/0x5774 -
 * the same four words `wait_and_latch_frame` moves and zeroes each frame.
 *
 * The word at 0x5752 is **saved, set to 2, and put back**. It sits immediately
 * after the sixteen bytes being cleared, so it is not being protected from the
 * loop; it is a guard held across the clear, which only makes sense if
 * something asynchronous - the INT 08h handler, which writes these very words -
 * reads it.
 */
void reset_input_state(void)
{
    int16_t saved = DG16(0x5752);
    uint16_t si = 0x5742;
    int16_t n = 2;

    DG16(0x5752) = 2;

    while (n != 0) {
        DG16(si) = 0;
        DG16(si + 2) = 0;
        DG16(si + 4) = 0;
        DG16(si + 6) = 0;
        si = (uint16_t)(si + 8);
        n--;
    }

    DG16(0x5768) = 0;
    DG16(0x576A) = 0;
    DG16(0x5772) = 0;
    DG16(0x5774) = 0;

    DG16(0x5752) = saved;
}

