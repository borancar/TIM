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
 * 0x00297
 *
 * A part hook that agrees to everything: it answers 1 and does nothing else.
 * The six routines from here to 0x002b5 are the kind table's do-nothing
 * entries - a kind that has no opinion about one of the hooks points at one of
 * these rather than leaving the slot empty, so every dispatch through the table
 * is a real call.
 */
uint16_t part_hook_yes(uint16_t part)
{
    (void)part;
    return 1;
}

/* 0x002a1 */
void part_hook_none_2a1(uint16_t part)
{
    (void)part;
}

/* 0x002a6 */
void part_hook_none_2a6(uint16_t part)
{
    (void)part;
}

/* 0x002ab */
void part_hook_none_2ab(uint16_t part)
{
    (void)part;
}

/* 0x002b0 */
void part_hook_none_2b0(uint16_t part)
{
    (void)part;
}

/*
 * 0x002b5
 *
 * The other half of the pair: answers 0.
 */
uint16_t part_hook_no(uint16_t part)
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
int16_t object_delta_angle(uint16_t obj)
{
    int32_t a = (int16_t)(DG16(obj + 0x22) - DG16(obj + 0x1e));
    int32_t b = (int16_t)(DG16(obj + 0x20) - DG16(obj + 0x24));

    return atan2_long((uint16_t)a, (uint16_t)(a >> 16),
                      (uint16_t)b, (uint16_t)(b >> 16));
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
 * NOT a transcription: the four-way box overlap the routine below tests before
 * each sweep. It is written out four times in the original, and **not
 * identically** - three copies compare strictly and the fourth does not. The
 * two helpers keep that difference visible instead of burying it in a repeated
 * block of four comparisons.
 *
 * 0x5408..0x540e are one object's bounds and 0x5410..0x541e the other's, filled
 * in by `compute_swept_bounds_5400` and `compute_bounds_53fe`.
 */
static int16_t boxes_meet_strict(void)
{
    return DG16(0x540e) < DG16(0x541e) && DG16(0x540c) > DG16(0x5412)
        && DG16(0x540a) < DG16(0x541a) && DG16(0x5408) > DG16(0x5410);
}

/*
 * NOT a transcription either: the non-strict fourth copy. Kept separate from
 * `boxes_meet_strict` above precisely because the difference is one character
 * per comparison in the original and easy to lose.
 */
static int16_t boxes_meet(void)
{
    return DG16(0x540e) <= DG16(0x541e) && DG16(0x540c) >= DG16(0x5412)
        && DG16(0x540a) <= DG16(0x541a) && DG16(0x5408) >= DG16(0x5410);
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
int16_t resolve_collisions(uint16_t obj)
{
    uint16_t link;
    int16_t hit = 0;

    DG16(0x5400) = (int16_t)obj;
    if (DG16(DGU16(0x5400) + 0x82) == 0)
        return 0;

    link = (uint16_t)(DGU16(0x5400) + 0x84);

    DG16(0x53fc) = DG16(DGU16(0x5400) + 0x84);
    if (DG16(0x53fc) != 0) {
        DG16(0x5424) = DG16(link + 4);
        DG16(0x5422) = angle_to_quadrant(DG16(0x5424));
    }

    DG8(link + 3) = 0;
    DG8(link + 2) = 0;

    DG16(0x5426) = object_delta_angle(DGU16(0x5400));
    compute_swept_bounds_5400();

    if (DG16(0x53fc) != 0
        && chain_contains(DGU16(0x5400), DGU16(0x53fc)) == 0) {
        DG16(0x53fe) = DG16(0x53fc);
        if (DG16(DGU16(0x53fe) + 0x82) != 0
            && (DG16(DGU16(0x53fe) + 8) & 0x2000) == 0) {
            compute_bounds_53fe();

            if (boxes_meet_strict() && find_edge_contact(0) != 0) {
                hit = 1;
                DG16(0x5426) = object_delta_angle(DGU16(0x5400));
            }
            if (boxes_meet_strict() && find_edge_contact_reversed(0) != 0) {
                hit = 1;
                DG16(0x5426) = object_delta_angle(DGU16(0x5400));
            }
        }
    }

    DG16(0x53fe) = pick_by_flag(0x3000);

    while (DG16(0x53fe) != 0) {
        if (chain_contains(DGU16(0x5400), DGU16(0x53fe)) == 0
            && DG16(0x5400) != DG16(0x53fe)
            && DG16(0x53fc) != DG16(0x53fe)
            && DG16(DGU16(0x53fe) + 0x82) != 0
            && (DG16(DGU16(0x53fe) + 8) & 0x2000) == 0
            && !(DG16(DGU16(0x5400) + 4) == 0xc
                 && DG16(DGU16(0x53fe) + 4) == 0x2a)) {
            compute_bounds_53fe();

            if (boxes_meet_strict() && find_edge_contact(0) != 0) {
                hit = 1;
                DG16(0x5426) = object_delta_angle(DGU16(0x5400));
            }
            if (boxes_meet() && find_edge_contact_reversed(0) != 0) {
                hit = 1;
                DG16(0x5426) = object_delta_angle(DGU16(0x5400));
            }
        }

        DG16(0x53fe) = pick_for_record(DGU16(0x53fe), 0x1000);
    }

    if (hit == 0)
        DG16(link) = 0;
    else if (angles_same_side(DG16(link + 4)) != 0)
        DG16(DGU16(0x5400) + 6) |= 1;

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
 *
 * The locals here are on the guest's stack because their addresses are passed
 * on; see `dg_enter` in dgroup.h for why the port needs a stack of its own.
 */
int16_t find_edge_contact(int16_t test_only)
{
    uint16_t fp   = dg_enter(0x40);
    uint16_t out  = fp;                   /* [bp-0x40] */
    uint16_t seg1 = (uint16_t)(fp + 4);   /* [bp-0x3c] */
    uint16_t seg2 = (uint16_t)(fp + 0xc); /* [bp-0x34] */

    uint16_t si, di, owner, link;
    int16_t hit = 0, i = 1, j;
    int16_t x0, y0, x1, y1, fx0, fy0;
    int16_t a_ang, b_ang, quad, d, same, tx, ty;

    si = DGU16(DGU16(0x53fe) + 0x82);
    x0 = (int16_t)(DG16(0x540e) + DG8(si));
    fx0 = x0;
    y0 = (int16_t)(DG16(0x540a) + DG8(si + 1));
    fy0 = y0;
    x1 = (int16_t)(DG16(0x540e) + DG8(si + 4));
    y1 = (int16_t)(DG16(0x540a) + DG8(si + 5));
    a_ang = DG16(si + 2);

    while (si != 0) {
        quad = angle_to_quadrant(a_ang);
        d = (int16_t)(DG16(0x5426) - a_ang + 0x4000);

        if (d > 0) {
            owner = DGU16(0x5400);
            di = DGU16(owner + 0x82);
            b_ang = DG16(di + 2);
            di = (uint16_t)(di + 4);
            j = 1;

            while (di != 0) {
                d = (int16_t)(b_ang - a_ang + 0x8000);
                if (d >= 0 || d == (int16_t)0x8000) {
                    d = (int16_t)(DG16(di + 2) - a_ang + 0x8000);
                    if (d <= 0
                        && (DG16(0x5414) != 0 || DG16(0x5402) != 0)) {
                        DG16(seg1) = (int16_t)(DG16(DGU16(0x5400) + 0x22)
                                               + DG8(di) - x0);
                        DG16(seg1 + 2) = (int16_t)(DG16(DGU16(0x5400) + 0x24)
                                                   + DG8(di + 1) - y0);
                        DG16(seg1 + 4) = (int16_t)(DG16(seg1) + DG16(0x5414));
                        tx = DG16(seg1 + 4);
                        DG16(seg1 + 6) = (int16_t)(DG16(seg1 + 2)
                                                   + DG16(0x5402));
                        ty = DG16(seg1 + 6);

                        DG16(seg2) = 0;
                        DG16(seg2 + 2) = 0;
                        DG16(seg2 + 4) = (int16_t)(x1 - x0);
                        DG16(seg2 + 6) = (int16_t)(y1 - y0);

                        step_pair_apart(seg2);

                        if (intersect_segments(seg1, seg2, out)
                            && !(DG16(out + 2) == DG16(seg2 + 6)
                                 && DG16(out) == DG16(seg2 + 4))) {
                            if (test_only != 0) {
                                dg_leave(0x40);
                                return 1;
                            }

                            DG16(seg2) = DG16(0x258c + 2 * quad);
                            DG16(seg2 + 2) = DG16(0x2594 + 2 * quad);
                            DG16(seg2 + 4) = (int16_t)(DG16(seg2 + 4)
                                                       + DG16(0x258c + 2 * quad));
                            DG16(seg2 + 6) = (int16_t)(DG16(seg2 + 6)
                                                       + DG16(0x2594 + 2 * quad));

                            same = angles_same_side(a_ang);
                            if (same == 0) {
                                if (!intersect_segments(seg1, seg2, out)) {
                                    DG16(DGU16(0x5400) + 0x1e) =
                                        DG16(DGU16(0x5400) + 0x22);
                                    DG16(DGU16(0x5400) + 0x20) =
                                        DG16(DGU16(0x5400) + 0x24);
                                } else {
                                    DG16(DGU16(0x5400) + 0x1e) = (int16_t)
                                        (DG16(DGU16(0x5400) + 0x1e)
                                         + (DG16(out) - tx));
                                    DG16(DGU16(0x5400) + 0x20) = (int16_t)
                                        (DG16(DGU16(0x5400) + 0x20)
                                         + (DG16(out + 2) - ty));
                                }
                            } else {
                                int16_t p = DG16(seg1 + 4);
                                int16_t q = (int16_t)(DG16(seg2 + 6)
                                                      - DG16(seg2 + 2));
                                int16_t r = (int16_t)(DG16(seg2 + 4)
                                                      - DG16(seg2));
                                int16_t c = (int16_t)
                                    ((int16_t)(q * DG16(seg2))
                                     - (int16_t)(r * DG16(seg2 + 2)));
                                int16_t run = (int16_t)(0 - r);

                                if (run != 0) {
                                    DG16(out + 2) = (int16_t)
                                        ((int16_t)(c - (int16_t)(q * p)) / run);
                                    DG16(DGU16(0x5400) + 0x20) = (int16_t)
                                        (DG16(DGU16(0x5400) + 0x20)
                                         + (DG16(out + 2) - ty));
                                } else {
                                    DG16(DGU16(0x5400) + 0x1e) =
                                        DG16(DGU16(0x5400) + 0x22);
                                    DG16(DGU16(0x5400) + 0x20) =
                                        DG16(DGU16(0x5400) + 0x24);
                                }
                            }

                            place_object_for_draw(DGU16(0x5400));
                            compute_swept_bounds_5400();

                            DG16(DGU16(0x5400) + 6) &= 0xfff9;
                            if (((DG16(DGU16(0x5400) + 8)
                                  | DG16(DGU16(0x53fe) + 8)) & 0x8000) != 0
                                || (DG16(DGU16(0x53fe) + 6) & 0x4000) != 0)
                                DG16(DGU16(0x5400) + 6) |= 2;
                            else
                                DG16(DGU16(0x5400) + 6) |= 4;

                            link = (uint16_t)(DGU16(0x5400) + 0x84);
                            DG16(link) = DG16(0x53fe);
                            DG16(link + 4) = a_ang;
                            DG16(link + 6) = (int16_t)(i - 1);
                            set_side_flags(seg2,
                                           (int16_t)(DG16(0x5418) - x0), link);
                            hit = 1;
                        }
                    }
                }

                j++;
                if (DG16(DGU16(0x5400) + 0x80) < j) {
                    di = 0;
                } else {
                    b_ang = DG16(di + 2);
                    if (DG16(DGU16(0x5400) + 0x80) == j)
                        di = DGU16(DGU16(0x5400) + 0x82);
                    else
                        di = (uint16_t)(di + 4);
                }
            }
        }

        i++;
        if (DG16(DGU16(0x53fe) + 0x80) < i) {
            si = 0;
        } else {
            si = (uint16_t)(si + 4);
            x0 = x1;
            y0 = y1;
            a_ang = DG16(si + 2);
            if (DG16(DGU16(0x53fe) + 0x80) == i) {
                x1 = fx0;
                y1 = fy0;
            } else {
                x1 = (int16_t)(DG16(0x540e) + DG8(si + 4));
                y1 = (int16_t)(DG16(0x540a) + DG8(si + 5));
            }
        }
    }

    dg_leave(0x40);
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
    uint16_t fp   = dg_enter(0x44);
    uint16_t out  = fp;                    /* [bp-0x44] */
    uint16_t seg1 = (uint16_t)(fp + 4);    /* [bp-0x40] */
    uint16_t seg2 = (uint16_t)(fp + 0xc);  /* [bp-0x38] */

    uint16_t si, di;
    int16_t hit = 0, i = 1, j;
    int16_t x0, y0, x1, y1, fx0, fy0;
    int16_t a_ang, b_ang, quad, d, same, sx, sy, v;

    di = DGU16(DGU16(0x5400) + 0x82);
    x0 = (int16_t)(DG16(0x5420) + DG8(di));
    fx0 = x0;
    y0 = (int16_t)(DG16(0x541c) + DG8(di + 1));
    fy0 = y0;
    x1 = (int16_t)(DG16(0x5420) + DG8(di + 4));
    y1 = (int16_t)(DG16(0x541c) + DG8(di + 5));
    a_ang = DG16(di + 2);

    while (di != 0) {
        quad = angle_to_quadrant((int16_t)(a_ang + 0x8000));
        d = (int16_t)(DG16(0x5426) + 0x8000 - a_ang + 0x4000);

        if (d > 0) {
            si = DGU16(DGU16(0x53fe) + 0x82);
            b_ang = DG16(si + 2);
            si = (uint16_t)(si + 4);
            j = 1;

            while (si != 0) {
                d = (int16_t)(b_ang - a_ang + 0x8000);
                if (d >= 0 || d == (int16_t)0x8000) {
                    d = (int16_t)(DG16(si + 2) - a_ang + 0x8000);
                    if (d <= 0
                        && (DG16(0x5414) != 0 || DG16(0x5402) != 0)) {
                        DG16(seg1 + 4) = (int16_t)(DG16(DGU16(0x53fe) + 0x1e)
                                                   + DG8(si) - x0);
                        sx = DG16(seg1 + 4);
                        DG16(seg1 + 6) = (int16_t)(DG16(DGU16(0x53fe) + 0x20)
                                                   + DG8(si + 1) - y0);
                        sy = DG16(seg1 + 6);
                        DG16(seg1) = (int16_t)(DG16(seg1 + 4) + DG16(0x5414));
                        DG16(seg1 + 2) = (int16_t)(DG16(seg1 + 6)
                                                   + DG16(0x5402));

                        DG16(seg2) = 0;
                        DG16(seg2 + 2) = 0;
                        DG16(seg2 + 4) = (int16_t)(x1 - x0);
                        DG16(seg2 + 6) = (int16_t)(y1 - y0);

                        step_pair_apart(seg2);

                        if (intersect_segments(seg1, seg2, out)
                            && !(DG16(out + 2) == DG16(seg2 + 6)
                                 && DG16(out) == DG16(seg2 + 4))) {
                            if (test_only != 0) {
                                dg_leave(0x44);
                                return 1;
                            }

                            DG16(seg2) = (int16_t)(0 - DG16(0x258c + 2 * quad));
                            DG16(seg2 + 2) = (int16_t)(0 - DG16(0x2594
                                                               + 2 * quad));
                            DG16(seg2 + 4) = (int16_t)(DG16(seg2 + 4)
                                                       - DG16(0x258c + 2 * quad));
                            DG16(seg2 + 6) = (int16_t)(DG16(seg2 + 6)
                                                       - DG16(0x2594 + 2 * quad));

                            same = angles_same_side((int16_t)(a_ang + 0x8000));
                            if (same == 0) {
                                if (!intersect_segments(seg1, seg2, out)) {
                                    DG16(DGU16(0x5400) + 0x1e) =
                                        DG16(DGU16(0x5400) + 0x22);
                                    DG16(DGU16(0x5400) + 0x20) =
                                        DG16(DGU16(0x5400) + 0x24);
                                } else {
                                    DG16(DGU16(0x5400) + 0x1e) = (int16_t)
                                        (DG16(DGU16(0x5400) + 0x1e)
                                         - (DG16(out) - sx));
                                    DG16(DGU16(0x5400) + 0x20) = (int16_t)
                                        (DG16(DGU16(0x5400) + 0x20)
                                         - (DG16(out + 2) - sy));
                                }
                            } else {
                                int16_t p = DG16(seg1 + 4);
                                int16_t q = (int16_t)(DG16(seg2 + 6)
                                                      - DG16(seg2 + 2));
                                int16_t r = (int16_t)(DG16(seg2 + 4)
                                                      - DG16(seg2));
                                int16_t c = (int16_t)
                                    ((int16_t)(q * DG16(seg2))
                                     - (int16_t)(r * DG16(seg2 + 2)));
                                int16_t run = (int16_t)(0 - r);

                                if (run != 0) {
                                    DG16(out + 2) = (int16_t)
                                        ((int16_t)(c - (int16_t)(q * p)) / run);
                                    DG16(DGU16(0x5400) + 0x20) = (int16_t)
                                        (DG16(DGU16(0x5400) + 0x20)
                                         - (DG16(out + 2) - sy));
                                } else {
                                    DG16(DGU16(0x5400) + 0x1e) =
                                        DG16(DGU16(0x5400) + 0x22);
                                    DG16(DGU16(0x5400) + 0x20) =
                                        DG16(DGU16(0x5400) + 0x24);
                                }
                            }

                            v = (int16_t)(DG16(0x5418) - x0);

                            place_object_for_draw(DGU16(0x5400));
                            compute_swept_bounds_5400();

                            DG16(DGU16(0x5400) + 6) &= 0xfff9;
                            if (((DG16(DGU16(0x5400) + 8)
                                  | DG16(DGU16(0x53fe) + 8)) & 0x8000) != 0
                                || (DG16(DGU16(0x53fe) + 6) & 0x4000) != 0)
                                DG16(DGU16(0x5400) + 6) |= 2;
                            else
                                DG16(DGU16(0x5400) + 6) |= 4;

                            DG16(DGU16(0x5400) + 0x84) = DG16(0x53fe);
                            DG16(DGU16(0x5400) + 0x88) =
                                (int16_t)(a_ang + 0x8000);

                            if (x0 > x1) {
                                if (v > DG16(out))
                                    DG8(DGU16(0x5400) + 0x86) = 1;
                                else
                                    DG8(DGU16(0x5400) + 0x87) = 1;
                            } else {
                                if (v > DG16(out))
                                    DG8(DGU16(0x5400) + 0x87) = 1;
                                else
                                    DG8(DGU16(0x5400) + 0x86) = 1;
                            }

                            DG16(DGU16(0x5400) + 0x8a) = (int16_t)(j - 1);
                            hit = 1;
                        }
                    }
                }

                j++;
                if (DG16(DGU16(0x53fe) + 0x80) < j) {
                    si = 0;
                } else {
                    b_ang = DG16(si + 2);
                    if (DG16(DGU16(0x53fe) + 0x80) == j)
                        si = DGU16(DGU16(0x53fe) + 0x82);
                    else
                        si = (uint16_t)(si + 4);
                }
            }
        }

        i++;
        if (DG16(DGU16(0x5400) + 0x80) < i) {
            di = 0;
        } else {
            di = (uint16_t)(di + 4);
            x0 = x1;
            y0 = y1;
            a_ang = DG16(di + 2);
            if (DG16(DGU16(0x5400) + 0x80) == i) {
                x1 = fx0;
                y1 = fy0;
            } else {
                x1 = (int16_t)(DG16(0x5420) + DG8(di + 4));
                y1 = (int16_t)(DG16(0x541c) + DG8(di + 5));
            }
        }
    }

    dg_leave(0x44);
    return hit;
}

/*
 * 0x00f86
 *
 * One step of the machine's physics, as a dozen passes over the same lists.
 *
 * The order matters and is the whole point: a pass finishes for every part
 * before the next begins, so a part never sees half of another part's step.
 *
 *  1. Clear bits 6 to 9 of the flags at +8 on everything - last step's answers.
 *  2. Run the step of every part on the list at DGROUP 0x4e58, which is the
 *     queue of parts something asked to move, then fold that list onto 0x4e56.
 *  3. Run it again for the parts on 0x521b with bit 11 set and neither bit 6
 *     nor bit 13, then for kind 0x0e, then for everything with none of bits 6,
 *     11 or 13. Three passes in a fixed order, so a conveyor moves before the
 *     things standing on it.
 *  4. Over the list at 0x5179 - the moving objects - apply gravity, reset the
 *     mass from the kind's record, and clear bit 4 of +0x0a.
 *  5. Four more passes over 0x5179 around kind 0x11, each pairing 0x03972 with
 *     a different follow-up.
 *  6. Collisions: an object with bit 1 of +6 asks the kind of whatever it is
 *     touching - the part at +0x84 - whether the hit counts, and answers by
 *     bouncing or sliding; bit 2 asks the same question and takes a third
 *     answer. Bit 3 or being hidden skips it.
 *  7. Finally, over the 0x3000 list, each part that has moved since last step
 *     is told so, and each that has not still copies its belts' positions
 *     forward.
 */
void step_machine(void)
{
    uint16_t fp = dg_enter(6);
    uint16_t v06 = (uint16_t)(fp + 0);      /* [bp-6] */
    uint16_t v04 = (uint16_t)(fp + 2);      /* [bp-4] */
    uint16_t v02 = (uint16_t)(fp + 4);      /* [bp-2] */
    uint16_t si, di;

    for (si = DGU16(0x521b); si != 0; si = DGU16(si))
        DGU16((uint16_t)(si + 8)) &= 0xf9bf;

    for (di = DGU16(0x4e58); di != 0; di = DGU16(di)) {
        si = DGU16((uint16_t)(di + 2));
        if (DGU16((uint16_t)(si + 8)) & 0x40)
            continue;
        part_step(si);
    }

    splice_list_4e58_onto_4e56();

    for (si = DGU16(0x521b); si != 0; si = DGU16(si)) {
        DGU16(v02) = DGU16((uint16_t)(si + 8));
        if (!(DGU16(v02) & 0x800))
            continue;
        if (DGU16(v02) & 0x2040)
            continue;
        part_step(si);
    }

    for (si = DGU16(0x521b); si != 0; si = DGU16(si)) {
        if (DGU16((uint16_t)(si + 4)) != 0x0e)
            continue;
        if (DGU16((uint16_t)(si + 8)) & 0x2040)
            continue;
        part_step(si);
    }

    for (si = DGU16(0x521b); si != 0; si = DGU16(si)) {
        DGU16(v02) = DGU16((uint16_t)(si + 8));
        if (DGU16(v02) & 0x2840)
            continue;
        part_step(si);
    }

    for (si = DGU16(0x5179); si != 0; si = DGU16(si)) {
        if (!(DGU16((uint16_t)(si + 8)) & 0x2000))
            apply_gravity_and_speed(si);

        DGU16((uint16_t)(si + 0x3a)) =
            DGU16((uint16_t)(0x0ea8
                             + 0x3a * (int16_t)DG16((uint16_t)(si + 4))));
        DGU16((uint16_t)(si + 0x0a)) &= 0xffef;
    }

    for (si = DGU16(0x5179); si != 0; si = DGU16(si))
        if (DGU16((uint16_t)(si + 4)) != 0x11)
            step_moving_object(si);

    for (si = DGU16(0x5179); si != 0; si = DGU16(si))
        if (DGU16((uint16_t)(si + 4)) == 0x11) {
            collect_carried(si);
            add_carried_weight(si);
        }

    for (si = DGU16(0x5179); si != 0; si = DGU16(si))
        if (DGU16((uint16_t)(si + 4)) == 0x11) {
            collect_carried(si);
            step_moving_object(si);
        }

    for (si = DGU16(0x5179); si != 0; si = DGU16(si))
        if (DGU16((uint16_t)(si + 4)) == 0x11) {
            collect_carried(si);
            carry_riders_along(si);
        }

    for (si = DGU16(0x5179); si != 0; si = DGU16(si)) {
        if (DGU16((uint16_t)(si + 6)) & 8)
            continue;
        if (DGU16((uint16_t)(si + 8)) & 0x2000)
            continue;

        if (DGU16((uint16_t)(si + 6)) & 2) {
            if (part_hit(DGU16((uint16_t)(DGU16((uint16_t)(si + 0x84)) + 4)),
                         si) == 0)
                continue;

            if (DGU16((uint16_t)(si + 6)) & 1)
                apply_contact_friction(si);
            else
                bounce_off_contact(si);
            continue;
        }

        if (DGU16((uint16_t)(si + 6)) & 4) {
            if (part_hit(DGU16((uint16_t)(DGU16((uint16_t)(si + 0x84)) + 4)),
                         si) != 0)
                bounce_pair(si);
        }
    }

    for (si = (uint16_t)pick_by_flag(0x3000); si != 0;
         si = (uint16_t)pick_for_record(si, 0x1000)) {

        if (DGU16((uint16_t)(si + 8)) & 0x2000)
            continue;

        if (DGU16((uint16_t)(si + 0x1e)) != DGU16((uint16_t)(si + 0x26))
            || DGU16((uint16_t)(si + 0x20)) != DGU16((uint16_t)(si + 0x28))
            || DGU16((uint16_t)(si + 0x0c)) != DGU16((uint16_t)(si + 0x10))) {
            part_moved(si);
            continue;
        }

        if (DGU16((uint16_t)(si + 0x1e)) == DGU16((uint16_t)(si + 0x22))
            && DGU16((uint16_t)(si + 0x20)) == DGU16((uint16_t)(si + 0x24))
            && DGU16((uint16_t)(si + 0x0c)) == DGU16((uint16_t)(si + 0x0e)))
            continue;

        for (DGU16(v04) = 0; DG16(v04) < 2; DGU16(v04)++) {
            DGU16(v06) = DGU16((uint16_t)(si + 0x66 + 2 * DGU16(v04)));
            if (DGU16(v06) == 0)
                continue;

            DG32((uint16_t)(DGU16(v06) + 0x14)) =
                DG32((uint16_t)(DGU16(v06) + 0x24));
            DG32((uint16_t)(DGU16(v06) + 0x18)) =
                DG32((uint16_t)(DGU16(v06) + 0x28));
        }
    }

    dg_leave(6);
}

/*
 * 0x01216
 *
 * One moving object's step: run its kind's own handler, integrate it, clear the
 * low nibble of its contact flags at +6, and settle it against whatever it hits.
 *
 * Then, if it hangs from a belt, `tension_belt` is asked whether that pulled it
 * somewhere. If it did, the contact flags are cleared again - the position it
 * was settled at is no longer where it is. If it did not, the contact record at
 * +0x84 is *saved and cleared* across a second `resolve_collisions`, and put
 * back only if that second pass found nothing: a part that the belt did not
 * move keeps the contact it already had, rather than losing it to a settle that
 * was only run to check.
 *
 * An object hidden - bit 13 of +8 - does none of it.
 */
void step_moving_object(uint16_t obj)
{
    uint16_t fp = dg_enter(6);
    uint16_t saved = fp;                    /* [bp-6], the contact's +0 */
    uint16_t b2 = (uint16_t)(fp + 2);       /* [bp-4], its +3 */
    uint16_t b1 = (uint16_t)(fp + 3);       /* [bp-3], its +2 */
    uint16_t pulled = (uint16_t)(fp + 4);   /* [bp-2] */
    uint16_t si = obj;
    uint16_t di;

    if (DGU16((uint16_t)(si + 8)) & 0x2000)
        goto out;

    part_step(si);
    integrate_object(si);

    DGU16((uint16_t)(si + 6)) &= 0xfff0;
    resolve_collisions(si);

    if (DGU16((uint16_t)(si + 0x66)) == 0)
        goto out;

    DG16(pulled) = tension_belt(si);

    if (DG16(pulled) != 0) {
        DGU16((uint16_t)(si + 6)) &= 0xfff0;
    } else {
        di = (uint16_t)(si + 0x84);

        DGU16(saved) = DGU16(di);
        DG8(b1) = DG8((uint16_t)(di + 2));
        DG8(b2) = DG8((uint16_t)(di + 3));
        DGU16(di) = 0;
    }

    resolve_collisions(si);

    if (DG16(pulled) != 0)
        goto out;

    di = (uint16_t)(si + 0x84);
    if (DGU16(di) != 0)
        goto out;

    DGU16(di) = DGU16(saved);
    DG8((uint16_t)(di + 2)) = DG8(b1);
    DG8((uint16_t)(di + 3)) = DG8(b2);

out:
    dg_leave(6);
}

/*
 * 0x03972
 *
 * Collect what a kind-0x11 platform is carrying, into the chain at +0x78, and
 * give each of them the platform's own velocity.
 *
 * Anything else does nothing: the first test is the kind, and there is no other
 * way in.
 *
 * A thing counts as carried in two ways. Either it is *already* resting on this
 * platform - its contact at +0x84 names it, it is moving downwards, and its
 * middle is within the platform's span - or its middle is within the span and
 * its underside sits between the platform's top and bottom, which is the case
 * for something that has just arrived. The two are separate tests and the
 * second is only reached when the first says no, so a thing already resting is
 * carried whatever its underside is doing.
 *
 * The span is the platform's own, four in from the left and 0x20 from there;
 * kind 0x0b is never carried.
 */
void collect_carried(uint16_t obj)
{
    uint16_t fp = dg_enter(0x0c);
    uint16_t their_bottom = (uint16_t)(fp + 0x00);  /* [bp-0x0c] */
    uint16_t their_mid = (uint16_t)(fp + 0x02);     /* [bp-0x0a] */
    uint16_t bottom = (uint16_t)(fp + 0x04);        /* [bp-8] */
    uint16_t top = (uint16_t)(fp + 0x06);           /* [bp-6] */
    uint16_t right = (uint16_t)(fp + 0x08);         /* [bp-4] */
    uint16_t left = (uint16_t)(fp + 0x0a);          /* [bp-2] */
    uint16_t di = obj;
    uint16_t si;

    if (DGU16((uint16_t)(di + 4)) != 0x11)
        goto out;

    DGU16((uint16_t)(di + 0x78)) = 0;

    DG16(left) = (int16_t)(DG16((uint16_t)(di + 0x22)) + 4);
    DG16(right) = (int16_t)(DG16(left) + 0x1c);
    DG16(top) = DG16((uint16_t)(di + 0x24));
    DG16(bottom) = (int16_t)(DG16(top) + DG16((uint16_t)(di + 0x46)));

    for (si = DGU16(0x5179); si != 0; si = DGU16(si)) {
        int16_t carried = 0;

        if (si == di)
            continue;
        if (DGU16((uint16_t)(si + 8)) & 0x2000)
            continue;
        if (DGU16((uint16_t)(si + 4)) == 0x0b)
            continue;

        DG16(their_mid) = (int16_t)(DG16((uint16_t)(si + 0x22))
                                    + (DG16((uint16_t)(si + 0x44)) >> 1));
        DG16(their_bottom) = (int16_t)(DG16((uint16_t)(si + 0x24))
                                       + DG16((uint16_t)(si + 0x46)));

        if (DGU16((uint16_t)(si + 0x84)) != 0
            && DGU16((uint16_t)(si + 0x84)) == di
            && DG16((uint16_t)(si + 0x38)) > 0
            && DG16(their_mid) > DG16(left)
            && DG16(their_mid) < DG16(right))
            carried = 1;

        if (carried == 0
            && DG16(their_mid) > DG16(left)
            && DG16(their_mid) < DG16(right)
            && (int16_t)(DG16(top) + 0x14) < DG16(their_bottom)
            && DG16(their_bottom) < DG16(bottom))
            carried = 1;

        if (carried == 0)
            continue;

        DGU16((uint16_t)(si + 0x78)) = DGU16((uint16_t)(di + 0x78));
        DGU16((uint16_t)(di + 0x78)) = si;
        DGU16((uint16_t)(si + 0x0a)) |= 0x10;

        DG16((uint16_t)(si + 0x38)) = DG16((uint16_t)(di + 0x38));
        DG16((uint16_t)(si + 0x36)) = DG16((uint16_t)(di + 0x36));
    }

out:
    dg_leave(0x0c);
}

/*
 * 0x03a8d
 *
 * Carry everything a platform holds along with it: whatever the platform
 * itself moved this step - its position at +0x1e/+0x20 against where it was at
 * +0x22/+0x24 - each thing in its chain at +0x78 moves the same way.
 *
 * A rider is *placed*, not stepped: the position moves, `place_object_for_draw`
 * refreshes the shape, and the sixteenths at +0x16/+0x1a are rebuilt from the
 * whole pixels rather than accumulated - so riding a platform leaves no
 * velocity of its own behind.
 */
void carry_riders_along(uint16_t obj)
{
    int16_t dx, dy;
    int32_t q;
    uint16_t si;

    if (DGU16((uint16_t)(obj + 4)) != 0x11)
        return;

    dx = (int16_t)(DG16((uint16_t)(obj + 0x1e)) - DG16((uint16_t)(obj + 0x22)));
    dy = (int16_t)(DG16((uint16_t)(obj + 0x20)) - DG16((uint16_t)(obj + 0x24)));
    if (dx == 0 && dy == 0)
        return;

    for (si = DGU16((uint16_t)(obj + 0x78)); si != 0;
         si = DGU16((uint16_t)(si + 0x78))) {
        DG16((uint16_t)(si + 0x1e)) += dx;
        DG16((uint16_t)(si + 0x20)) += dy;

        place_object_for_draw(si);

        q = (int32_t)DG16((uint16_t)(si + 0x1e)) << 9;
        DG16((uint16_t)(si + 0x18)) = (int16_t)(q >> 16);
        DG16((uint16_t)(si + 0x16)) = (int16_t)q;

        q = (int32_t)DG16((uint16_t)(si + 0x20)) << 9;
        DG16((uint16_t)(si + 0x1c)) = (int16_t)(q >> 16);
        DG16((uint16_t)(si + 0x1a)) = (int16_t)q;
    }
}

/*
 * 0x03009
 *
 * Play the impact sound if a kind-0 object hit hard enough: the two velocity
 * components at +0x36 and +0x38, each made positive and added, over 0x1000.
 * That is a sum of absolute values rather than a length, so a diagonal counts
 * for more than the same speed along one axis - which is the original's
 * arithmetic and not an approximation of anything.
 */
void sound_on_hard_impact(uint16_t obj)
{
    int16_t a, b;

    if (DGU16((uint16_t)(obj + 4)) != 0)
        return;

    a = DG16((uint16_t)(obj + 0x36));
    if (a < 0)
        a = (int16_t)-a;
    b = DG16((uint16_t)(obj + 0x38));
    if (b < 0)
        b = (int16_t)-b;

    if ((int16_t)(a + b) > 0x1000)
        play_sound(0x14);
}

/*
 * 0x03046
 *
 * A bounce off a surface, rather than a slide along one.
 *
 * The contact record at +0x84 names what was hit at +4 and the angle of the
 * face it hit; that angle is turned by a quarter turn or back by one, by
 * whether the two bytes at +2 and +3 are set, so a corner reflects the way the
 * face it belongs to does.
 *
 * The velocity is rotated into the surface's frame, the component along the
 * face is kept, and the component into it is scaled by the *smaller* of the two
 * kinds' bounciness at +4 of their records - so a hard thing on a soft one
 * bounces as the soft one says - negated, and clamped: over 0x40 loses 0x40 and
 * under -0x40 gains it, otherwise it goes to zero. Then the pair is rotated
 * back.
 *
 * The position is carried into sixteenths afterwards, and the rounding is not
 * symmetric: a positive velocity or a positive gravity at +8 of the kind's
 * record rounds the *other* way, `(v + 1) << 9 - 1` rather than `v << 9`.
 */
void bounce_off_contact(uint16_t obj)
{
    uint16_t fp = dg_enter(0x18);
    uint16_t their = (uint16_t)(fp + 0x00);  /* [bp-0x18] their kind record */
    uint16_t mine  = (uint16_t)(fp + 0x02);  /* [bp-0x16] my kind record */
    uint16_t hit   = (uint16_t)(fp + 0x04);  /* [bp-0x14] the contact */
    uint16_t what  = (uint16_t)(fp + 0x06);  /* [bp-0x12] what was hit */
    uint16_t plo   = (uint16_t)(fp + 0x08);  /* [bp-0x10] */
    uint16_t phi   = (uint16_t)(fp + 0x0a);  /* [bp-0x0e] */
    uint16_t qlo   = (uint16_t)(fp + 0x0c);  /* [bp-0x0c] */
    uint16_t bounce = (uint16_t)(fp + 0x10); /* [bp-8] */
    uint16_t t     = (uint16_t)(fp + 0x12);  /* [bp-6] */
    uint16_t vy    = (uint16_t)(fp + 0x14);  /* [bp-4] */
    uint16_t vx    = (uint16_t)(fp + 0x16);  /* [bp-2] */
    uint16_t si = obj;
    int16_t di;

    sound_on_hard_impact(si);

    DGU16(hit) = (uint16_t)(si + 0x84);
    DGU16(what) = DGU16(DGU16(hit));

    DGU16(mine) = (uint16_t)(0x0ea6
                             + 0x3a * (int16_t)DG16((uint16_t)(si + 4)));
    DGU16(their) = (uint16_t)(0x0ea6
                              + 0x3a * (int16_t)DG16(
                                  (uint16_t)(DGU16(what) + 4)));

    di = DG16((uint16_t)(DGU16(hit) + 4));

    if (di == 0 || di == (int16_t)0x8000) {
        if (DG8((uint16_t)(DGU16(hit) + 2)) == 0)
            di = (int16_t)(di + 0x1000);
        else if (DG8((uint16_t)(DGU16(hit) + 3)) == 0)
            di = (int16_t)(di - 0x1000);
    }

    DG16(vx) = DG16((uint16_t)(si + 0x36));
    DG16(vy) = DG16((uint16_t)(si + 0x38));

    rotate_point(vx, vy, (uint16_t)di);

    DG16(bounce) =
        (DG16((uint16_t)(DGU16(mine) + 4)) < DG16((uint16_t)(DGU16(their) + 4)))
        ? DG16((uint16_t)(DGU16(mine) + 4))
        : DG16((uint16_t)(DGU16(their) + 4));

    {
        int32_t p = (int32_t)mul16x16(DG16(vy), DG16(bounce));

        DG16(phi) = (int16_t)(p >> 16);
        DG16(plo) = (int16_t)p;

        DG16(vy) = (int16_t)long_shift_right(
            ((int32_t)(uint16_t)DG16(phi) << 16) | (uint16_t)DG16(plo), 8);
    }

    DG16(vy) = (int16_t)-DG16(vy);

    if (DG16(vy) < 0) {
        DG16(t) = (int16_t)(DG16(vy) + 0x40);
        DG16(vy) = (DG16(t) < 0) ? DG16(t) : 0;
    } else {
        DG16(t) = (int16_t)(DG16(vy) - 0x40);
        DG16(vy) = (DG16(t) > 0) ? DG16(t) : 0;
    }

    rotate_point(vx, vy, (uint16_t)(0 - di));

    DG16((uint16_t)(si + 0x36)) = DG16(vx);
    DG16((uint16_t)(si + 0x38)) = DG16(vy);

    clamp_record_pair(si);

    DG32(qlo) = DG16((uint16_t)(si + 0x1e));
    if (DG16(vx) >= 0)
        DG32((uint16_t)(si + 0x16)) =
            (int32_t)(long_shift_left((uint32_t)(DG32(qlo) + 1), 9) - 1);
    else
        DG32((uint16_t)(si + 0x16)) =
            (int32_t)long_shift_left((uint32_t)DG32(qlo), 9);

    DG32(plo) = DG16((uint16_t)(si + 0x20));
    if (DG16((uint16_t)(DGU16(mine) + 8)) >= 0)
        DG32((uint16_t)(si + 0x1a)) =
            (int32_t)(long_shift_left((uint32_t)(DG32(plo) + 1), 9) - 1);
    else
        DG32((uint16_t)(si + 0x1a)) =
            (int32_t)long_shift_left((uint32_t)DG32(plo), 9);

    dg_leave(0x18);
}

/*
 * 0x03201
 *
 * Two moving things hit each other: share the momentum out between them.
 * `bounce_off_contact` is the same event against something that cannot move.
 *
 * Both velocities are turned into the frame of the line between the two
 * middles - `angle_between_centres` less a quarter turn - so that "x" means
 * along that line and "y" across it. Only the x halves are exchanged, by the
 * usual two-body formula over the two weights the kind records keep at +2:
 *
 *     mine  = (m*u + 2*n*v - n*u) / (m + n)
 *     yours = (2*m*u + n*v - m*v) / (m + n)
 *
 * built as 32-bit sums so a heavy thing at speed cannot wrap. Then both are
 * turned back and **halved**, which is where the energy goes.
 *
 * After that, a nudge apart, and the condition for it is three separate ways
 * of saying "these two are going to stay stuck": both left slower than 0x100,
 * or bit 0 of my +6, or bit 4 of my +0xa. Whichever it is, the one on the left
 * is given at least 0x200 leftwards and the one on the right at least 0x200
 * rightwards - unless bit 4 of my +0xa says the other one is not to be pushed.
 *
 * The bounciness at +4 of the two kind records - the smaller of the two - is
 * worked out and **never used**. It is a dead store in the original and is
 * transcribed as one; `bounce_off_contact` uses the same value for what looks
 * like the job this one was meant to do with it.
 */
void bounce_pair(uint16_t obj)
{
    uint16_t fp = dg_enter(0x36);
    uint16_t theirKind = (uint16_t)(fp + 0x00);  /* [bp-0x36] */
    uint16_t myKind = (uint16_t)(fp + 0x02);     /* [bp-0x34] */
    uint16_t yLo   = (uint16_t)(fp + 0x04);      /* [bp-0x32], with -0x30 */
    uint16_t xLo   = (uint16_t)(fp + 0x08);      /* [bp-0x2e], with -0x2c */
    uint16_t mine_v = (uint16_t)(fp + 0x0c);     /* [bp-0x2a] m*v, low */
    uint16_t yours_u = (uint16_t)(fp + 0x10);    /* [bp-0x26] n*u, low */
    uint16_t yours_v = (uint16_t)(fp + 0x14);    /* [bp-0x22] n*v, low */
    uint16_t mine_u = (uint16_t)(fp + 0x18);     /* [bp-0x1e] m*u, low */
    uint16_t apart = (uint16_t)(fp + 0x1c);      /* [bp-0x1a] */
    uint16_t theirMid = (uint16_t)(fp + 0x1e);   /* [bp-0x18] */
    uint16_t myMid = (uint16_t)(fp + 0x20);      /* [bp-0x16] */
    uint16_t dvy   = (uint16_t)(fp + 0x22);      /* [bp-0x14] */
    uint16_t dvx   = (uint16_t)(fp + 0x24);      /* [bp-0x12] */
    uint16_t svy   = (uint16_t)(fp + 0x26);      /* [bp-0x10] */
    uint16_t svx   = (uint16_t)(fp + 0x28);      /* [bp-0x0e] */
    uint16_t total = (uint16_t)(fp + 0x2a);      /* [bp-0x0c], a long */
    uint16_t theirW = (uint16_t)(fp + 0x2e);     /* [bp-8] */
    uint16_t myW   = (uint16_t)(fp + 0x30);      /* [bp-6] */
    uint16_t bounce = (uint16_t)(fp + 0x32);     /* [bp-4], never read */
    uint16_t angle = (uint16_t)(fp + 0x34);      /* [bp-2] */
    uint16_t si = obj;
    uint16_t di;
    int32_t q;

    sound_on_hard_impact(si);

    di = DGU16((uint16_t)(si + 0x84));

    DGU16((uint16_t)(si + 6)) |= 8;
    DGU16((uint16_t)(di + 6)) |= 8;

    DGU16(myKind) = (uint16_t)(0x0ea6
        + 0x3a * (int16_t)DG16((uint16_t)(si + 4)));
    DGU16(theirKind) = (uint16_t)(0x0ea6
        + 0x3a * (int16_t)DG16((uint16_t)(di + 4)));

    DG16(bounce) = (DG16((uint16_t)(DGU16(myKind) + 4))
                    < DG16((uint16_t)(DGU16(theirKind) + 4)))
                   ? DG16((uint16_t)(DGU16(myKind) + 4))
                   : DG16((uint16_t)(DGU16(theirKind) + 4));

    DG16(myW) = DG16((uint16_t)(DGU16(myKind) + 2));
    DG16(theirW) = DG16((uint16_t)(DGU16(theirKind) + 2));

    DG16(svx) = DG16((uint16_t)(si + 0x36));
    DG16(svy) = DG16((uint16_t)(si + 0x38));
    DG16(dvx) = DG16((uint16_t)(di + 0x36));
    DG16(dvy) = DG16((uint16_t)(di + 0x38));

    DG16(angle) = (int16_t)(angle_between_centres(si, di) - 0x4000);

    rotate_point(svx, svy, DGU16(angle));
    rotate_point(dvx, dvy, DGU16(angle));

    DG32(total) = (int32_t)DG16(myW) + (int32_t)DG16(theirW);

    DG32(mine_u)  = (int32_t)mul16x16(DG16(myW), DG16(svx));
    DG32(yours_v) = (int32_t)mul16x16(DG16(theirW), DG16(dvx));
    DG32(yours_u) = (int32_t)mul16x16(DG16(theirW), DG16(svx));
    DG32(mine_v)  = (int32_t)mul16x16(DG16(myW), DG16(dvx));

    DG16(svx) = (int16_t)long_divide(
        DG32(mine_u) + DG32(yours_v) + DG32(yours_v) - DG32(yours_u),
        DG32(total));

    DG16(dvx) = (int16_t)long_divide(
        DG32(mine_u) + DG32(mine_u) + DG32(yours_v) - DG32(mine_v),
        DG32(total));

    rotate_point(svx, svy, (uint16_t)(int16_t)-DG16(angle));
    rotate_point(dvx, dvy, (uint16_t)(int16_t)-DG16(angle));

    DG16((uint16_t)(si + 0x36)) = (int16_t)(DG16(svx) >> 1);
    DG16((uint16_t)(si + 0x38)) = (int16_t)(DG16(svy) >> 1);
    DG16((uint16_t)(di + 0x36)) = (int16_t)(DG16(dvx) >> 1);
    DG16((uint16_t)(di + 0x38)) = (int16_t)(DG16(dvy) >> 1);

    DG16(apart) = 0;

    {
        int16_t a = DG16((uint16_t)(si + 0x36));
        int16_t b = DG16((uint16_t)(di + 0x36));

        if (a < 0)
            a = (int16_t)-a;
        if (b < 0)
            b = (int16_t)-b;
        if (a < 0x100 && b < 0x100)
            DG16(apart) = 1;
    }

    if (DGU16((uint16_t)(si + 6)) & 1)
        DG16(apart) = 1;
    if (DGU16((uint16_t)(si + 0x0a)) & 0x10)
        DG16(apart) = 1;

    if (DG16(apart) != 0) {
        DG16(myMid) = (int16_t)(DG16((uint16_t)(si + 0x1e))
            + (int16_t)(DG16((uint16_t)(si + 0x44)) >> 1));
        DG16(theirMid) = (int16_t)(DG16((uint16_t)(di + 0x1e))
            + (int16_t)(DG16((uint16_t)(di + 0x44)) >> 1));

        if (DG16(myMid) < DG16(theirMid)) {
            if (DG16((uint16_t)(si + 0x36)) > (int16_t)0xfe00)
                DG16((uint16_t)(si + 0x36)) = (int16_t)0xfe00;

            if (!(DGU16((uint16_t)(si + 0x0a)) & 0x10)
                && DG16((uint16_t)(di + 0x36)) < 0x200)
                DG16((uint16_t)(di + 0x36)) = 0x200;
        } else {
            if (DG16((uint16_t)(si + 0x36)) < 0x200)
                DG16((uint16_t)(si + 0x36)) = 0x200;

            if (!(DGU16((uint16_t)(si + 0x0a)) & 0x10)
                && DG16((uint16_t)(di + 0x36)) > (int16_t)0xfe00)
                DG16((uint16_t)(di + 0x36)) = (int16_t)0xfe00;
        }
    }

    clamp_record_pair(si);
    clamp_record_pair(di);

    /*
     * The sixteenths, for both, and the rounding is not symmetric - the same
     * asymmetry `bounce_off_contact` has. Across, it keys on the sign of the
     * thing's own speed; down, on the sign of its kind's gravity at +8.
     */
    DG32(xLo) = DG16((uint16_t)(si + 0x1e));
    q = (DG16((uint16_t)(si + 0x36)) < 0)
        ? (int32_t)long_shift_left((uint32_t)DG32(xLo), 9)
        : (int32_t)(long_shift_left((uint32_t)(DG32(xLo) + 1), 9) - 1);
    DG16((uint16_t)(si + 0x18)) = (int16_t)(q >> 16);
    DG16((uint16_t)(si + 0x16)) = (int16_t)q;

    DG32(yLo) = DG16((uint16_t)(si + 0x20));
    q = (DG16((uint16_t)(DGU16(myKind) + 8)) < 0)
        ? (int32_t)long_shift_left((uint32_t)DG32(yLo), 9)
        : (int32_t)(long_shift_left((uint32_t)(DG32(yLo) + 1), 9) - 1);
    DG16((uint16_t)(si + 0x1c)) = (int16_t)(q >> 16);
    DG16((uint16_t)(si + 0x1a)) = (int16_t)q;

    DG32(xLo) = DG16((uint16_t)(di + 0x1e));
    q = (DG16((uint16_t)(di + 0x36)) < 0)
        ? (int32_t)long_shift_left((uint32_t)DG32(xLo), 9)
        : (int32_t)(long_shift_left((uint32_t)(DG32(xLo) + 1), 9) - 1);
    DG16((uint16_t)(di + 0x18)) = (int16_t)(q >> 16);
    DG16((uint16_t)(di + 0x16)) = (int16_t)q;

    DG32(yLo) = DG16((uint16_t)(di + 0x20));
    q = (DG16((uint16_t)(DGU16(theirKind) + 8)) < 0)
        ? (int32_t)long_shift_left((uint32_t)DG32(yLo), 9)
        : (int32_t)(long_shift_left((uint32_t)(DG32(yLo) + 1), 9) - 1);
    DG16((uint16_t)(di + 0x1c)) = (int16_t)(q >> 16);
    DG16((uint16_t)(di + 0x1a)) = (int16_t)q;

    dg_leave(0x36);
}

/*
 * 0x06d8e
 *
 * A part has moved: mark it and everything joined to it as needing re-filing,
 * register the shapes of what it is joined to, and - unless it is a kind 0x31
 * anchor, which draws nothing - its own as well. All three with a count of 1.
 */
void part_moved(uint16_t part)
{
    mark_needs_refile(part, 1);
    mark_joined_shapes(part, 1);

    if (DGU16((uint16_t)(part + 4)) != 0x31)
        mark_part_shapes(part, 1);
}

/*
 * 0x07c3a
 *
 * Add the weight of everything a platform carries to the platform itself: the
 * chain `collect_carried` built at +0x78, one at a time.
 */
void add_carried_weight(uint16_t obj)
{
    uint16_t si;

    for (si = DGU16((uint16_t)(obj + 0x78)); si != 0;
         si = DGU16((uint16_t)(si + 0x78)))
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
void add_mass_capped(uint16_t obj, uint16_t other)
{
    int32_t total = (int32_t)DG16((uint16_t)(obj + 0x3a))
                    + (int32_t)DG16((uint16_t)(other + 0x3a));

    if (total > 0x7d00)
        total = 0x7d00;

    DG16((uint16_t)(obj + 0x3a)) = (int16_t)total;
}

/*
 * OURS: not a transcription. The original runs a part's per-step handler
 * through the far pointer at +0x26 of its kind's record, and asks whether a
 * hit counts through the one at +0x22 of the *other* part's kind. C cannot
 * call either, so both are dispatched by value.
 */
void part_step(uint16_t part)
{
    uint16_t bx = (uint16_t)((int16_t)DG16((uint16_t)(part + 4)) * 0x3a);

    call_part_hook(DGU16((uint16_t)(bx + 0x0ecc)),
                   DGU16((uint16_t)(bx + 0x0ece)), part, "step");
}

/*
 * OURS: not a transcription, the third of the by-value dispatches. A part's
 * drive hook is the far pointer at +0x36 of its kind's record, and it takes
 * seven arguments where the other two take one.
 */
uint16_t part_drive(uint16_t by, uint16_t p1, uint16_t p2, uint16_t p3,
                    uint16_t p4, uint16_t p5, uint16_t p6, uint16_t p7)
{
    uint16_t bx = (uint16_t)((int16_t)DG16((uint16_t)(by + 4)) * 0x3a);

    return call_part_drive(DGU16((uint16_t)(bx + 0x0edc)),
                           DGU16((uint16_t)(bx + 0x0ede)),
                           p1, p2, p3, p4, p5, p6, p7);
}

/*
 * OURS: not a transcription, the other half of the pair above.
 */
uint16_t part_hit(uint16_t kind, uint16_t part)
{
    uint16_t bx = (uint16_t)((int16_t)kind * 0x3a);

    return call_part_hook(DGU16((uint16_t)(bx + 0x0ec8)),
                          DGU16((uint16_t)(bx + 0x0eca)), part, "hit");
}

/*
 * 0x013e9
 *
 * Clear the machine down to nothing: reset every part, put the cursor back to
 * 0, take both pages' drawings off, and zero the handful of words the running
 * machine keeps - the one at 0x50d5, the animation phase at 0x4ea7, the four
 * at 0x52cd and the ten at 0x5458.
 */
void clear_machine(void)
{
    int16_t si;

    reset_machine();
    select_cursor(0);
    erase_both_pages();

    DGU16(0x50d5) = 0;
    DGU16(0x4ea7) = 0;
    DGU16(0x52cd) = 0;
    DGU16(0x52cf) = 0;
    DGU16(0x52d1) = 0;
    DGU16(0x52d3) = 0;

    for (si = 0; si < 10; si++)
        DGU16((uint16_t)(0x5458 + 2 * si)) = 0;
}

/*
 * 0x01431
 *
 * The other half of the pair: fold the list at 0x4e58 back onto 0x4e56, reset
 * the machine, and hand it to the two routines that follow.
 */
void restart_machine(void)
{
    splice_list_4e58_onto_4e56();
    reset_machine();
    show_cursor_again();
    stop_music_or_effect(0);
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
 * 0x026e8
 *
 * Clip to the **counter strip** and draw into the visible page.
 *
 * Full width, rows 0x1b to 0x45 - the band the three counters sit in - and
 * `vga_page_dst` set to 0xa000 rather than to whichever page is being built.
 * The counters are drawn straight onto the screen, outside the double
 * buffering, which is what lets them roll while the machine below them is
 * still being composed.
 *
 * The strip is 0x2b rows and a digit cell is 0x15, so two cells and a line:
 * enough for a digit and the one arriving behind it.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void set_clip_counter_strip(void)
{
    vga_page_dst = 0xa000;
    clip_enabled = 1;
    clip_left    = 0;
    clip_right   = 0x27f;
    clip_top     = 0x1b;
    clip_bottom  = 0x45;
}

/*
 * 0x0262b
 *
 * Draw a **four-digit counter** at `x`, scrolled by `y`, right digit first.
 *
 * The four digits come out of a five-digit string, and the fifth is why
 * `0x2710` is added first: 10000 forces `itoa` to fill all five places, so
 * `buf[1..4]` are the digits wanted with their leading zeros already there and
 * no zero-padding loop needed. `buf[0]` is the 1 that was added, and is never
 * drawn.
 *
 * Then a sentinel: `buf[5]`, the terminator, is overwritten with `'0'`. That
 * one byte is the whole of the leading-zero test below - it makes the digit to
 * the right of the units read as a zero, so the units are always drawn.
 *
 * `all` decides how much of the number gets redrawn:
 *
 *   set    every digit, which is what a full repaint wants
 *   clear  only while `buf[si]` is `'0'` - the units, then the tens if the
 *          units are zero, then the hundreds if the tens are too, and stop
 *
 * That reads like a mistake and is exactly right. The counter is stepped by
 * one, so the digits that move are the trailing zeros and the one above them:
 * 1300 going to 1299 changes three digits, 1301 going to 1300 changes one. The
 * test asks "did the digit to my right just wrap", and stops at the first that
 * did not.
 *
 * Stopping is done by setting the index to 0 and letting the shared `dec` take
 * it to -1, so the loop's own test ends it - and the x step happens on that
 * pass too, harmlessly, because nothing reads x again.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void draw_counter_word(int16_t value, int16_t x, int16_t y, int16_t all)
{
    uint16_t buf = dg_enter(8);
    int16_t  si;

    int_to_string((int16_t)(value + 0x2710), buf, 10);
    DG8(buf + 5) = '0';

    for (si = 5; si > 1; si--, x = (int16_t)(x - 0x20)) {
        if (all != 0 || DG8(buf + si) == '0')
            draw_odometer_digit((char)DG8(buf + si - 1), x, y);
        else
            si = 0;
    }

    dg_leave(8);
}

/*
 * 0x02686
 *
 * Draw a **six-digit counter** at `x`, scrolled by `y`. The 32-bit sibling of
 * `draw_counter_word`, and the same trick twice over: 0xf4240 is 1,000,000, so
 * `ltoa` fills seven places, `buf[7]` gets the `'0'` sentinel, and `buf[1..6]`
 * are drawn from the right.
 *
 * The only real difference is that x lives in a register here and in a stack
 * slot there, which is the compiler's choice and not the program's.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void draw_counter_long(uint16_t lo, uint16_t hi, int16_t x, int16_t y,
                       int16_t all)
{
    uint16_t buf = dg_enter(0x10);
    uint32_t v   = (((uint32_t)hi << 16) | lo) + 0xf4240;
    int16_t  si;

    long_int_to_string((uint16_t)v, (uint16_t)(v >> 16), buf, 10);
    DG8(buf + 7) = '0';

    for (si = 7; si > 1; si--, x = (int16_t)(x - 0x20)) {
        if (all != 0 || DG8(buf + si) == '0')
            draw_odometer_digit((char)DG8(buf + si - 1), x, y);
        else
            si = 0;
    }

    dg_leave(0x10);
}

/*
 * 0x025d8
 *
 * Repaint all three counters in full, with no scroll: the long one at 0xd0 and
 * the two short ones at 0x184 and 0x238. Every caller of this wants the whole
 * band back, so `all` is 1 on all three.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void redraw_counters(void)
{
    set_clip_counter_strip();
    draw_counter_long(DGU16(0x4ead), DGU16(0x4eaf), 0xd0, 0, 1);
    draw_counter_word(DG16(0x50af), 0x184, 0, 1);
    draw_counter_word(DG16(0x50b1), 0x238, 0, 1);
}

/*
 * 0x024fa
 *
 * Start the counters rolling. The scroll positions go to -4 and 0, and the
 * band is repainted.
 *
 * The -4 is a delay, not a position: `step_counters` adds to it before it
 * looks, so the first counter spends four steps at or below zero - drawing
 * nothing, because the draw is skipped while the scroll is not positive -
 * before its first digit moves. The second starts at 0 and moves at once.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void start_counters(void)
{
    DG16(0x4eb3) = -4;
    DG16(0x4eb1) = 0;
    redraw_counters();
}

/*
 * 0x02510
 *
 * One step of the two rolling counters.
 *
 * Each is a value and a scroll: the scroll climbs to 0x15, a whole digit cell,
 * and then wraps to 0 and takes one off the value. So a counter never jumps -
 * it slides from one number to the next, and `draw_counter_word` is handed the
 * scroll as its `y`.
 *
 * **The first counter rolls faster the more it has left**: over 0xfa0 the
 * scroll gains 4 a step, over 0xbb8 three, over 0x708 two, otherwise one. So a
 * large tally is not slow, and the last hundred or so still count one at a
 * time. The second counter always gains one.
 *
 * State 0x2000 is the one that changes the rule. Outside it both counters step
 * whenever they have anything left; inside it neither *starts*, and only a
 * scroll already off zero is allowed to finish - which is how a roll that is
 * half-way through a digit is never left standing between two of them.
 *
 * The value is written back whether or not it was decremented, and the draw is
 * skipped when the scroll is at or below zero: at zero there is nothing to
 * slide, and the negative is the four-step delay `start_counters` set.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void step_counters(void)
{
    set_clip_counter_strip();

    if (DGU16(0x4e6b) != 0x2000 || DG16(0x4eb3) != 0) {
        int16_t si = DG16(0x50af);

        if (si != 0) {
            if (si > 0xfa0)
                DG16(0x4eb3) = (int16_t)(DG16(0x4eb3) + 4);
            else if (si > 0xbb8)
                DG16(0x4eb3) = (int16_t)(DG16(0x4eb3) + 3);
            else if (si > 0x708)
                DG16(0x4eb3) = (int16_t)(DG16(0x4eb3) + 2);
            else
                DG16(0x4eb3) = (int16_t)(DG16(0x4eb3) + 1);

            if (DG16(0x4eb3) > 0x15) {
                DG16(0x4eb3) = 0;
                si--;
            }
            DG16(0x50af) = si;

            if (DG16(0x4eb3) > 0)
                draw_counter_word(DG16(0x50af), 0x184, DG16(0x4eb3), 0);
        }
    }

    if (DGU16(0x4e6b) != 0x2000 || DG16(0x4eb1) != 0) {
        if (DG16(0x50b1) != 0) {
            DG16(0x4eb1) = (int16_t)(DG16(0x4eb1) + 1);
            if (DG16(0x4eb1) > 0x15) {
                DG16(0x4eb1) = 0;
                DG16(0x50b1) = (int16_t)(DG16(0x50b1) - 1);
            }

            if (DG16(0x4eb1) > 0)
                draw_counter_word(DG16(0x50b1), 0x238, DG16(0x4eb1), 0);
        }
    }
}

/*
 * 0x012ab
 *
 * NOT TRANSCRIBED YET. The screen state 0x2000 dispatches to, from
 * `game_round`.
 */
void sub_012ab(void)
{
    not_transcribed("0x012ab");
}

/*
 * 0x02710
 *
 * NOT TRANSCRIBED YET. Called on the way out of a round, but only when the
 * state that ended it was 0x200.
 */
void sub_02710(void)
{
    not_transcribed("0x02710");
}

/*
 * 0x02a34
 *
 * **Read a number in an arbitrary base.** Digits are `0`-`9` and then `A`
 * upwards without a limit - subtracting 0x37 from anything at or above `A`
 * gives 10 for `A`, 35 for `Z`, and keeps going past it - which is what lets
 * the caller ask for base 0x22.
 *
 * It **reverses the string first**, in place and permanently, and then
 * accumulates with a `place` that starts at 1 and is multiplied by the base
 * each time round. So the reversal is what makes the first character the most
 * significant; without it the loop would read the number backwards.
 *
 * Nothing validates. A character below `0` yields a negative digit and is
 * accumulated like any other.
 */
int32_t parse_base(uint16_t text, int16_t base)
{
    int32_t  total = 0;
    int32_t  place = 1;
    uint16_t si;

    string_reverse(text);

    for (si = text; DG8(si) != 0; si++) {
        int16_t digit = (DG8(si) >= 'A')
                        ? (int16_t)(DG8(si) - 0x37)
                        : (int16_t)(DG8(si) - 0x30);

        total += (int32_t)long_multiply((uint32_t)place, (uint32_t)(int32_t)digit);
        place  = (int32_t)long_multiply((uint32_t)place, (uint32_t)(int32_t)base);
    }

    return total;
}

/*
 * 0x02900
 *
 * **A score code into a score.** The code is `PASSWORD-XXXXX...`: the password
 * up to the dash, then five hex digits holding the score, then the rest as a
 * base-0x22 checksum.
 *
 * **`Z` and `Y` stand in for `0` and `O`** in what the player types, and are
 * swapped back before anything is parsed - because a printed code with a zero
 * and a letter O next to each other is a code that gets typed in wrong. They
 * are swapped *back again* at the end, so the caller's buffer comes out as the
 * player typed it: `password_to_level` is about to be handed the same string.
 *
 * The checksum is the score multiplied by each of the **first three characters
 * of the password** and the three products added - so a code carries its own
 * password, and one lifted from another player's game does not verify.
 *
 * A code with no dash at all answers 0. One that fails the checksum answers
 * 0xffffffff, which the caller shows a message for; the two are different
 * answers on purpose.
 *
 * Both halves go through `parse_base`, which **reverses what it is given**, so
 * each is copied into a local first. That is why there are two buffers here and
 * not two pointers.
 */
int32_t score_code_to_score(uint16_t text)
{
    uint16_t fp    = dg_enter(0x2c);
    uint16_t tail  = fp;                    /* [bp-0x2c], the checksum text */
    uint16_t five  = (uint16_t)(fp + 0x24); /* [bp-8], the five score digits */
    uint16_t dash;
    uint16_t si;
    int16_t  i;
    int32_t  score, check, sum;

    dash = string_chr(text, '-');

    if (dash == 0) {
        dg_leave(0x2c);
        return 0;
    }

    dash++;

    for (si = dash; DG8(si) != 0; si++) {
        if (DG8(si) == 'Z')
            DG8(si) = '0';
        if (DG8(si) == 'Y')
            DG8(si) = 'O';
    }

    for (i = 0; i < 5; i++)
        DG8((uint16_t)(five + i)) = DG8((uint16_t)(dash + i));

    DG8((uint16_t)(five + 5)) = 0;

    string_copy(tail, (uint16_t)(dash + 5));

    score = parse_base(five, 0x10);
    check = parse_base(tail, 0x22);

    sum  = (int32_t)long_multiply((uint32_t)score, DG8(text));
    sum += (int32_t)long_multiply((uint32_t)score, DG8((uint16_t)(text + 1)));
    sum += (int32_t)long_multiply((uint32_t)score, DG8((uint16_t)(text + 2)));

    for (si = dash; DG8(si) != 0; si++) {
        if (DG8(si) == '0')
            DG8(si) = 'Z';
        if (DG8(si) == 'O')
            DG8(si) = 'Y';
    }

    dg_leave(0x2c);

    if (check == sum)
        return score;

    return -1;
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
 * 0x02c93
 *
 * Advance an object one step: add its velocity to its position, apply gravity,
 * clamp, and work out where that puts it on screen.
 *
 * Position is a pair of 32-bit values at +0x16 and +0x1a carrying nine
 * fractional bits; velocity is the two 16-bit words at +0x36 and +0x38 that
 * `apply_contact_friction` writes, sign-extended before they are added. The
 * whole-number position is then those two shifted right by nine into +0x1e and
 * +0x20 - arithmetically, so a negative position stays negative.
 *
 * Gravity applies only with bit 0 set at +6, and its **direction comes from the
 * material record**: field +8 of the 0x3a-byte record at 0xea6 + 0x3a * type,
 * the same field `apply_contact_friction` reads as the normal load. Positive
 * pulls one way, anything else the other, always by 0x400 - which is
 * two whole units, given the nine fractional bits.
 *
 * Both axes are clamped to -1000..6000. A clamp does not just fix the
 * whole-number word: it rewrites the fixed-point value from the limit and
 * shifts it back up by nine, so the fraction is discarded rather than left
 * describing a position the object no longer has. Clamping only the visible
 * word would leave the two disagreeing and the object would creep.
 *
 * `place_object_for_draw` runs last, so a caller gets both the new position
 * and the new drawing position from one call.
 */
void integrate_object(uint16_t obj)
{
    uint16_t rec;

    DG32(obj + 0x16) += DG16(obj + 0x36);
    DG32(obj + 0x1a) += DG16(obj + 0x38);

    if ((DG16(obj + 6) & 1) != 0) {
        rec = (uint16_t)(0xea6 + 0x3a * DG16(obj + 4));
        if (DG16(rec + 8) > 0)
            DG32(obj + 0x1a) += 0x400;
        else
            DG32(obj + 0x1a) -= 0x400;
    }

    DG16(obj + 0x1e) = (int16_t)(DG32(obj + 0x16) >> 9);
    DG16(obj + 0x20) = (int16_t)(DG32(obj + 0x1a) >> 9);

    if (DG16(obj + 0x1e) < -1000) {
        DG16(obj + 0x1e) = -1000;
        DG32(obj + 0x16) = -1000;
        DG32(obj + 0x16) <<= 9;
    } else if (DG16(obj + 0x1e) > 6000) {
        DG16(obj + 0x1e) = 6000;
        DG32(obj + 0x16) = 6000;
        DG32(obj + 0x16) <<= 9;
    }

    if (DG16(obj + 0x20) < -1000) {
        DG16(obj + 0x20) = -1000;
        DG32(obj + 0x1a) = -1000;
        DG32(obj + 0x1a) <<= 9;
    } else if (DG16(obj + 0x20) > 6000) {
        DG16(obj + 0x20) = 6000;
        DG32(obj + 0x1a) = 6000;
        DG32(obj + 0x1a) <<= 9;
    }

    place_object_for_draw(obj);
}

/*
 * 0x02da0
 *
 * Apply contact friction to an object: work out how hard the surface it is
 * touching resists, and take that out of its velocity.
 *
 * The contact is described by the link at the object's own +0x84, whose first
 * word names the other object. Both objects' *types*, at +4, index a table of
 * 0x3a-byte material records at DGROUP 0xea6 - so this reads two records, one
 * per side of the contact.
 *
 * The contact angle is the link's +4, in the whole-turn-is-0x10000 space the
 * sine tables use. An angle of exactly 0 or 0x8000 - dead flat, either way up -
 * is nudged by 0x1000, a sixteenth of a turn, toward whichever side of the link
 * has a zero byte at +2 or +3. A flat contact has no direction to resolve
 * along, and the nudge gives it one.
 *
 * Grip is the larger of the two materials' +6, except that a type 5 object with
 * a non-zero +0x12 forces 0x100 regardless of either material.
 *
 * From there it is ordinary resolution into the contact frame: cosine and sine
 * of the *negated* angle, the normal load from |+8 of the first material|, and
 * a tangential term that is only counted when the object's own velocity and the
 * angle share a sign - pushing into the surface rather than away from it. Grip
 * times the sum of those two magnitudes, shifted down by 8, is the friction;
 * projected back through the cosine it becomes the amount to remove.
 *
 * A flag bit 0x20 at the object's +6 raises the floor of that amount from 2 to
 * 0x20, so some objects are held far more firmly than others.
 *
 * Friction opposes motion rather than reversing it: the subtraction is clamped
 * at zero from whichever side the velocity started on, so an object is brought
 * to rest and never pushed backwards.
 *
 * Finally +0x38 takes the perpendicular component, `clamp_record_pair` is run,
 * and the long at +0x1a is rebuilt from +0x20 shifted up by 9 - biased by one
 * before the shift and one after when the normal load is positive, which is a
 * rounding step and not a sign fix.
 */
void apply_contact_friction(uint16_t obj)
{
    uint16_t link  = (uint16_t)(obj + 0x84);
    uint16_t other = DGU16(link);
    uint16_t rec_a = (uint16_t)(0xea6 + 0x3a * DG16(obj + 4));
    uint16_t rec_b = (uint16_t)(0xea6 + 0x3a * DG16(other + 4));
    int16_t load   = DG16(rec_a + 8);
    int16_t angle  = DG16(link + 4);
    int16_t grip, cos_a, sin_a, aload, normal, tangent, drag, push, perp;
    int16_t v, step;
    int32_t q;

    if (angle == 0 || angle == (int16_t)0x8000) {
        if (DG8(link + 2) == 0)
            angle = (int16_t)(angle + 0x1000);
        else if (DG8(link + 3) == 0)
            angle = (int16_t)(angle - 0x1000);
    }

    v = DG16(obj + 0x36);

    if (DG16(other + 4) == 5 && DG16(other + 0x12) != 0)
        grip = 0x100;
    else
        grip = DG16(rec_a + 6) > DG16(rec_b + 6) ? DG16(rec_a + 6)
                                                 : DG16(rec_b + 6);

    cos_a = angle_cos((uint16_t)(int16_t)-angle);
    sin_a = angle_sin((uint16_t)(int16_t)-angle);
    aload = abs16(load);

    normal = (int16_t)((int32_t)mul16x16(cos_a, aload) >> 14);

    if ((v > 0 && angle > 0) || (v < 0 && angle < 0))
        tangent = (int16_t)((int32_t)mul16x16(sin_a, v) >> 14);
    else
        tangent = 0;

    drag = (int16_t)((int32_t)mul16x16((int16_t)(abs16(normal)
                                                 + abs16(tangent)),
                                       grip) >> 8);
    push = (int16_t)((int32_t)mul16x16(cos_a, drag) >> 14);
    push = (int16_t)(abs16(push) + ((DG16(obj + 6) & 0x20) ? 0x20 : 2));

    perp = (int16_t)((int32_t)mul16x16(sin_a, aload) >> 14);
    v = (int16_t)(v + (int16_t)((int32_t)mul16x16(abs16(cos_a), perp) >> 14));

    if (v < 0) {
        step = (int16_t)(v + push);
        v = step < 0 ? step : 0;
    } else {
        step = (int16_t)(v - push);
        v = step > 0 ? step : 0;
    }

    DG16(obj + 0x36) = v;

    DG16(obj + 0x38) = (int16_t)
        ((int32_t)mul16x16(angle_sin((uint16_t)(int16_t)-angle),
                           ((angle + 0x4000) & 0x8000) ? (int16_t)-v : v)
         >> 14);

    clamp_record_pair(obj);

    q = (int16_t)DG16(obj + 0x20);
    if (load >= 0)
        q = ((q + 1) << 9) - 1;
    else
        q = q << 9;

    DG16(obj + 0x1c) = (int16_t)(q >> 16);
    DG16(obj + 0x1a) = (int16_t)(q & 0xFFFF);
}

/*
 * 0x03566
 *
 * Find every object whose bounding box comes within the given margins of one
 * object's, and chain them onto it.
 *
 * A box is the position at +0x1e/+0x20 and the extent at +0x44/+0x46 that
 * `set_object_extent` fills in. Four separations are measured - the candidate's
 * far edge against this one's near edge, and its near edge against this one's
 * far edge, in each axis - and each is tested against one of the four margin
 * arguments. Any test failing drops the candidate.
 *
 * What is then stored is not the separation but a **signed nearness**, one per
 * axis, and the rule is not obvious: whichever of the two separations is
 * smaller in magnitude decides, and the value stored is that side's - but
 * clamped away from zero first. A non-negative far-edge separation is stored as
 * -1 and a non-positive near-edge separation as 1, so the answer keeps the sign
 * that says which side the candidate is on and never reads as "exactly
 * touching" when it is not.
 *
 * Results are chained through +0x78 in reverse: each new object takes the head
 * and becomes the head, so the list comes out in the opposite order to the walk.
 * The nearness pair goes in the candidate's own +0x7a and +0x7c, which means an
 * object can only be on one such list at a time.
 *
 * The object itself is skipped, and so is anything with bit 0x2000 at +8. The
 * walk is the usual `pick_by_flag` then `pick_for_record` pair - note the
 * second is given only bit 0x1000 of the caller's flags, not all of them.
 */
void link_nearby_objects(uint16_t obj, uint16_t flags,
                         int16_t margin_x0, int16_t margin_x1,
                         int16_t margin_y0, int16_t margin_y1)
{
    int16_t ax0, ax1, ay0, ay1;
    int16_t bx0, bx1, by0, by1;
    int16_t near, far_, hi, lo;
    uint16_t si;

    DG16(obj + 0x78) = 0;

    ax0 = DG16(obj + 0x1e);
    ax1 = (int16_t)(ax0 + DG16(obj + 0x44));
    ay0 = DG16(obj + 0x20);
    ay1 = (int16_t)(ay0 + DG16(obj + 0x46));

    si = (uint16_t)pick_by_flag(flags);

    while (si != 0) {
        if (si != obj && (DG16(si + 8) & 0x2000) == 0) {
            bx0 = DG16(si + 0x1e);
            bx1 = (int16_t)(bx0 + DG16(si + 0x44));
            by0 = DG16(si + 0x20);
            by1 = (int16_t)(by0 + DG16(si + 0x46));

            far_ = (int16_t)(bx1 - ax0);
            if (far_ >= margin_x0) {
                hi = far_ >= 0 ? (int16_t)-1 : far_;
                near = (int16_t)(bx0 - ax1);
                if (near <= margin_x1) {
                    lo = near > 0 ? near : (int16_t)1;
                    far_ = abs16(near) < abs16(far_) ? lo : hi;

                    hi = (int16_t)(by1 - ay0);
                    if (hi >= margin_y0) {
                        int16_t dy = hi;

                        hi = dy >= 0 ? (int16_t)-1 : dy;
                        near = (int16_t)(by0 - ay1);
                        if (near <= margin_y1) {
                            lo = near > 0 ? near : (int16_t)1;
                            dy = abs16(near) < abs16(dy) ? lo : hi;

                            DG16(si + 0x78) = DG16(obj + 0x78);
                            DG16(obj + 0x78) = (int16_t)si;
                            DG16(si + 0x7a) = far_;
                            DG16(si + 0x7c) = dy;
                        }
                    }
                }
            }
        }

        si = (uint16_t)pick_for_record(si, (uint16_t)(flags & 0x1000));
    }
}

/*
 * 0x036de
 *
 * Build the chain of objects that overlap a box, the way `link_nearby_objects`
 * builds the one that overlaps a part's own box - but the box is given as four
 * offsets from the object's position rather than taken from its size.
 *
 * The chain is threaded through +0x78, newest first, starting from the asking
 * object's own +0x78; the object itself and anything hidden - bit 13 of +8 -
 * are left out. The walk is the same `pick_by_flag` and `pick_for_record` pair,
 * and the second is given only bit 12 of the flags.
 */
void link_objects_in_range(uint16_t obj, uint16_t flags,
                           int16_t x0, int16_t x1, int16_t y0, int16_t y1)
{
    uint16_t si;

    DGU16((uint16_t)(obj + 0x78)) = 0;

    x0 = (int16_t)(x0 + DG16((uint16_t)(obj + 0x1e)));
    x1 = (int16_t)(x1 + DG16((uint16_t)(obj + 0x1e)));
    y0 = (int16_t)(y0 + DG16((uint16_t)(obj + 0x20)));
    y1 = (int16_t)(y1 + DG16((uint16_t)(obj + 0x20)));

    for (si = (uint16_t)pick_by_flag(flags); si != 0;
         si = (uint16_t)pick_for_record(si, (uint16_t)(flags & 0x1000))) {

        int16_t l, r, t, b;

        if (si == obj)
            continue;
        if (DGU16((uint16_t)(si + 8)) & 0x2000)
            continue;

        l = DG16((uint16_t)(si + 0x1e));
        r = (int16_t)(l + DG16((uint16_t)(si + 0x44)));
        t = DG16((uint16_t)(si + 0x20));
        b = (int16_t)(t + DG16((uint16_t)(si + 0x46)));

        if (l >= x1 || r <= x0 || t >= y1 || b <= y0)
            continue;

        DGU16((uint16_t)(si + 0x78)) = DGU16((uint16_t)(obj + 0x78));
        DGU16((uint16_t)(obj + 0x78)) = si;
    }
}

/*
 * 0x03782
 *
 * Build the chain of objects whose *outline* crosses a given line, rather than
 * whose box overlaps another - `link_nearby_objects` and
 * `link_objects_in_range` both work on boxes, and this one does not.
 *
 * Each candidate's outline is the array of points at +0x82, `si[0x80]` of them,
 * two bytes each and taken as offsets from the object's own position. The loop
 * walks them as segments, wrapping the last back to the first, and asks
 * `intersect_segments` whether each crosses the line the caller gave. The first
 * one that does puts the object on the chain and ends its walk - the counter is
 * set to the point count, which the increment then pushes past the end.
 *
 * The two points are carried relative to the *asking* object, which is why the
 * four words handed to `intersect_segments` are differences rather than
 * positions.
 */
void link_objects_crossing(uint16_t obj, uint16_t flags, uint16_t line)
{
    uint16_t fp = dg_enter(0x1a);
    uint16_t v1a = (uint16_t)(fp + 0x00);   /* [bp-0x1a] where they crossed */
    uint16_t v16 = (uint16_t)(fp + 0x04);   /* [bp-0x16] the segment */
    uint16_t v0e = (uint16_t)(fp + 0x0c);   /* [bp-0x0e] the first y */
    uint16_t v0c = (uint16_t)(fp + 0x0e);   /* [bp-0x0c] this y */
    uint16_t v0a = (uint16_t)(fp + 0x10);   /* [bp-0x0a] the last y */
    uint16_t v08 = (uint16_t)(fp + 0x12);   /* [bp-8] the first x */
    uint16_t v06 = (uint16_t)(fp + 0x14);   /* [bp-6] this x */
    uint16_t v04 = (uint16_t)(fp + 0x16);   /* [bp-4] the last x */
    uint16_t v02 = (uint16_t)(fp + 0x18);   /* [bp-2] the point */
    uint16_t si, di;

    DGU16((uint16_t)(obj + 0x78)) = 0;

    for (si = (uint16_t)pick_by_flag(flags); si != 0;
         si = (uint16_t)pick_for_record(si, (uint16_t)(flags & 0x1000))) {

        DG16(v02) = 1;
        di = DGU16((uint16_t)(si + 0x82));

        DG16(v04) = (int16_t)(DG16((uint16_t)(si + 0x1e)) + DG8(di));
        DG16(v08) = DG16(v04);
        DG16(v0a) = (int16_t)(DG16((uint16_t)(si + 0x20))
                              + DG8((uint16_t)(di + 1)));
        DG16(v0e) = DG16(v0a);
        DG16(v06) = (int16_t)(DG16((uint16_t)(si + 0x1e))
                              + DG8((uint16_t)(di + 4)));
        DG16(v0c) = (int16_t)(DG16((uint16_t)(si + 0x20))
                              + DG8((uint16_t)(di + 5)));

        while (di != 0) {
            DG16(v16) = (int16_t)(DG16(v04)
                                  - DG16((uint16_t)(obj + 0x1e)));
            DG16((uint16_t)(v16 + 2)) =
                (int16_t)(DG16(v0a) - DG16((uint16_t)(obj + 0x20)));
            DG16((uint16_t)(v16 + 4)) =
                (int16_t)(DG16(v06) - DG16((uint16_t)(obj + 0x1e)));
            DG16((uint16_t)(v16 + 6)) =
                (int16_t)(DG16(v0c) - DG16((uint16_t)(obj + 0x20)));

            if (intersect_segments(line, v16, v1a) != 0) {
                DGU16((uint16_t)(si + 0x78)) = DGU16((uint16_t)(obj + 0x78));
                DGU16((uint16_t)(obj + 0x78)) = si;
                DG16(v02) = DG16((uint16_t)(si + 0x80));
            }

            DG16(v02)++;

            if (DG16((uint16_t)(si + 0x80)) < DG16(v02)) {
                di = 0;
                continue;
            }

            di = (uint16_t)(di + 4);
            DG16(v04) = DG16(v06);
            DG16(v0a) = DG16(v0c);

            if (DG16((uint16_t)(si + 0x80)) == DG16(v02)) {
                DG16(v06) = DG16(v08);
                DG16(v0c) = DG16(v0e);
            } else {
                DG16(v06) = (int16_t)(DG16((uint16_t)(si + 0x1e))
                                      + DG8((uint16_t)(di + 4)));
                DG16(v0c) = (int16_t)(DG16((uint16_t)(si + 0x20))
                                      + DG8((uint16_t)(di + 5)));
            }
        }
    }

    dg_leave(0x1a);
}

/*
 * 0x038b9
 *
 * The fourth "what is near me": a box given as four offsets, like
 * `link_objects_in_range`, but matching a *point* rather than a box. Only
 * objects with bit 2 of +0x0a are considered, and the point tested is the one
 * at +0x72 and +0x73 - where that kind is held - rather than the corner of its
 * rectangle.
 *
 * The two vertical tests are computed as flags and `test`-ed together rather
 * than short-circuited, which is the compiler's way with `&&` over two
 * comparisons whose operands it has already loaded.
 */
void link_objects_at_point(uint16_t obj, int16_t x0, int16_t x1,
                           int16_t y0, int16_t y1)
{
    uint16_t si;

    DGU16((uint16_t)(obj + 0x78)) = 0;

    x0 = (int16_t)(x0 + DG16((uint16_t)(obj + 0x1e)));
    x1 = (int16_t)(x1 + DG16((uint16_t)(obj + 0x1e)));
    y0 = (int16_t)(y0 + DG16((uint16_t)(obj + 0x20)));
    y1 = (int16_t)(y1 + DG16((uint16_t)(obj + 0x20)));

    for (si = (uint16_t)pick_by_flag(0x3000); si != 0;
         si = (uint16_t)pick_for_record(si, 0x1000)) {

        int16_t px, py;

        if (si == obj)
            continue;
        if (DGU16((uint16_t)(si + 8)) & 0x2000)
            continue;
        if (!(DGU16((uint16_t)(si + 0x0a)) & 4))
            continue;

        px = (int16_t)(DG16((uint16_t)(si + 0x1e))
                       + DG8((uint16_t)(si + 0x72)));
        py = (int16_t)(DG16((uint16_t)(si + 0x20))
                       + DG8((uint16_t)(si + 0x73)));

        if (px < x0 || px > x1)
            continue;
        if (!((py >= y0) && (py <= y1)))
            continue;

        DGU16((uint16_t)(si + 0x78)) = DGU16((uint16_t)(obj + 0x78));
        DGU16((uint16_t)(obj + 0x78)) = si;
    }
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
int16_t outlines_cross(uint16_t a, uint16_t b)
{
    uint16_t fp = dg_enter(0x38);
    uint16_t segB = (uint16_t)(fp + 0x04);  /* [bp-0x34], four words */
    uint16_t segA = (uint16_t)(fp + 0x0c);  /* [bp-0x2c], four words */
    uint16_t out  = (uint16_t)(fp + 0x00);  /* [bp-0x38], two words */
    uint16_t by2  = (uint16_t)(fp + 0x1a);  /* [bp-0x1e] */
    uint16_t bx2  = (uint16_t)(fp + 0x20);  /* [bp-0x18] */
    uint16_t fby  = (uint16_t)(fp + 0x18);  /* [bp-0x20] */
    uint16_t fbx  = (uint16_t)(fp + 0x1e);  /* [bp-0x1a] */
    uint16_t by1  = (uint16_t)(fp + 0x1c);  /* [bp-0x1c] */
    uint16_t bx1  = (uint16_t)(fp + 0x22);  /* [bp-0x16] */
    uint16_t by0  = (uint16_t)(fp + 0x14);  /* [bp-0x24] */
    uint16_t bx0  = (uint16_t)(fp + 0x16);  /* [bp-0x22] */
    uint16_t ay0  = (uint16_t)(fp + 0x24);  /* [bp-0x14] */
    uint16_t ax0  = (uint16_t)(fp + 0x26);  /* [bp-0x12] */
    uint16_t ay2  = (uint16_t)(fp + 0x2a);  /* [bp-0x0e] */
    uint16_t fay  = (uint16_t)(fp + 0x28);  /* [bp-0x10] */
    uint16_t fax  = (uint16_t)(fp + 0x2e);  /* [bp-0x0a] */
    uint16_t ay1  = (uint16_t)(fp + 0x2c);  /* [bp-0x0c] */
    uint16_t ax2  = (uint16_t)(fp + 0x30);  /* [bp-8] */
    uint16_t ax1  = (uint16_t)(fp + 0x32);  /* [bp-6] */
    uint16_t j    = (uint16_t)(fp + 0x34);  /* [bp-4] */
    uint16_t i    = (uint16_t)(fp + 0x36);  /* [bp-2] */
    uint16_t si, di;
    int16_t answer = 0;

    DG16(ax0) = DG16((uint16_t)(a + 0x1e));
    DG16(ay0) = DG16((uint16_t)(a + 0x20));
    DG16(bx0) = DG16((uint16_t)(b + 0x1e));
    DG16(by0) = DG16((uint16_t)(b + 0x20));

    DG16(i) = 1;
    si = DGU16((uint16_t)(a + 0x82));

    if (si != 0) {
        DG16(ax1) = (int16_t)(DG16(ax0) + DG8(si));
        DG16(fax) = DG16(ax1);
        DG16(ay1) = (int16_t)(DG16(ay0) + DG8((uint16_t)(si + 1)));
        DG16(fay) = DG16(ay1);
        DG16(ax2) = (int16_t)(DG16(ax0) + DG8((uint16_t)(si + 4)));
        DG16(ay2) = (int16_t)(DG16(ay0) + DG8((uint16_t)(si + 5)));
    }

    while (si != 0) {
        DG16(segA) = (int16_t)(DG16(ax1) - DG16(ax1));
        DG16((uint16_t)(segA + 2)) = (int16_t)(DG16(ay1) - DG16(ay1));
        DG16((uint16_t)(segA + 4)) = (int16_t)(DG16(ax2) - DG16(ax1));
        DG16((uint16_t)(segA + 6)) = (int16_t)(DG16(ay2) - DG16(ay1));
        step_pair_apart(segA);

        DG16(j) = 1;
        di = DGU16((uint16_t)(b + 0x82));

        if (di != 0) {
            DG16(bx1) = (int16_t)(DG16(bx0) + DG8(di));
            DG16(fbx) = DG16(bx1);
            DG16(by1) = (int16_t)(DG16(by0) + DG8((uint16_t)(di + 1)));
            DG16(fby) = DG16(by1);
            DG16(bx2) = (int16_t)(DG16(bx0) + DG8((uint16_t)(di + 4)));
            DG16(by2) = (int16_t)(DG16(by0) + DG8((uint16_t)(di + 5)));
        }

        while (di != 0) {
            DG16(segB) = (int16_t)(DG16(bx1) - DG16(ax1));
            DG16((uint16_t)(segB + 2)) = (int16_t)(DG16(by1) - DG16(ay1));
            DG16((uint16_t)(segB + 4)) = (int16_t)(DG16(bx2) - DG16(ax1));
            DG16((uint16_t)(segB + 6)) = (int16_t)(DG16(by2) - DG16(ay1));
            step_pair_apart(segB);

            if (intersect_segments(segA, segB, out) != 0
                && (DG16((uint16_t)(out + 2)) != DG16((uint16_t)(segA + 6))
                    || DG16(out) != DG16((uint16_t)(segA + 4)))) {
                answer = 1;
                goto done;
            }

            DG16(j)++;
            if (DG16((uint16_t)(b + 0x80)) < DG16(j)) {
                di = 0;
                continue;
            }

            di = (uint16_t)(di + 4);
            DG16(bx1) = DG16(bx2);
            DG16(by1) = DG16(by2);

            if (DG16((uint16_t)(b + 0x80)) == DG16(j)) {
                DG16(bx2) = DG16(fbx);
                DG16(by2) = DG16(fby);
            } else {
                DG16(bx2) = (int16_t)(DG16(bx0) + DG8((uint16_t)(di + 4)));
                DG16(by2) = (int16_t)(DG16(by0) + DG8((uint16_t)(di + 5)));
            }
        }

        DG16(i)++;
        if (DG16((uint16_t)(a + 0x80)) < DG16(i)) {
            si = 0;
            continue;
        }

        si = (uint16_t)(si + 4);
        DG16(ax1) = DG16(ax2);
        DG16(ay1) = DG16(ay2);

        if (DG16((uint16_t)(a + 0x80)) == DG16(i)) {
            DG16(ax2) = DG16(fax);
            DG16(ay2) = DG16(fay);
        } else {
            DG16(ax2) = (int16_t)(DG16(ax0) + DG8((uint16_t)(si + 4)));
            DG16(ay2) = (int16_t)(DG16(ay0) + DG8((uint16_t)(si + 5)));
        }
    }

done:
    dg_leave(0x38);
    return answer;
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
int16_t angle_between_centres(uint16_t a, uint16_t b)
{
    int16_t acx = (int16_t)(DG16((uint16_t)(a + 0x1e))
                            + (int16_t)(DG16((uint16_t)(a + 0x44)) >> 1));
    int16_t acy = (int16_t)(DG16((uint16_t)(a + 0x20))
                            + (int16_t)(DG16((uint16_t)(a + 0x46)) >> 1));
    int16_t bcx = (int16_t)(DG16((uint16_t)(b + 0x1e))
                            + (int16_t)(DG16((uint16_t)(b + 0x44)) >> 1));
    int16_t bcy = (int16_t)(DG16((uint16_t)(b + 0x20))
                            + (int16_t)(DG16((uint16_t)(b + 0x46)) >> 1));
    int32_t dx = (int32_t)(int16_t)(acx - bcx);
    int32_t dy = (int32_t)(int16_t)(bcy - acy);

    return atan2_long((uint16_t)dx, (uint16_t)((uint32_t)dx >> 16),
                      (uint16_t)dy, (uint16_t)((uint32_t)dy >> 16));
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
int16_t object_overlaps_any(uint16_t obj)
{
    uint16_t fp = dg_enter(0x18);
    uint16_t sy2 = (uint16_t)(fp + 0x00);   /* [bp-0x18] */
    uint16_t sx2 = (uint16_t)(fp + 0x02);   /* [bp-0x16] */
    uint16_t sy1 = (uint16_t)(fp + 0x04);   /* [bp-0x14] */
    uint16_t sx1 = (uint16_t)(fp + 0x06);   /* [bp-0x12] */
    uint16_t sy0 = (uint16_t)(fp + 0x08);   /* [bp-0x10] */
    uint16_t sx0 = (uint16_t)(fp + 0x0a);   /* [bp-0x0e] */
    uint16_t y2  = (uint16_t)(fp + 0x0c);   /* [bp-0x0c] */
    uint16_t x2  = (uint16_t)(fp + 0x0e);   /* [bp-0x0a] */
    uint16_t y1  = (uint16_t)(fp + 0x10);   /* [bp-8] */
    uint16_t x1  = (uint16_t)(fp + 0x12);   /* [bp-6] */
    uint16_t y0  = (uint16_t)(fp + 0x14);   /* [bp-4] */
    uint16_t x0  = (uint16_t)(fp + 0x16);   /* [bp-2] */
    uint16_t di = obj;
    uint16_t si;
    int16_t answer = 0;

    DG16(x0) = DG16((uint16_t)(di + 0x1e));
    DG16(y0) = DG16((uint16_t)(di + 0x20));
    DG16(x1) = (int16_t)(DG16(x0) + DG16((uint16_t)(di + 0x50)));
    DG16(y1) = (int16_t)(DG16(y0) + DG16((uint16_t)(di + 0x52)));
    DG16(x2) = (int16_t)(DG16(x0) + DG16((uint16_t)(di + 0x44)));
    DG16(y2) = (int16_t)(DG16(y0) + DG16((uint16_t)(di + 0x46)));

    for (si = (uint16_t)pick_by_flag(0x3000); si != 0;
         si = (uint16_t)pick_for_record(si, 0x1000)) {

        if (DGU16((uint16_t)(di + 4)) == 0x0c
            && DGU16((uint16_t)(si + 4)) == 0x2a)
            continue;
        if (DGU16((uint16_t)(si + 4)) == 0x0c
            && DGU16((uint16_t)(di + 4)) == 0x2a)
            continue;
        if (si == di)
            continue;
        if (DGU16((uint16_t)(si + 8)) & 0x2000)
            continue;

        DG16(sx0) = DG16((uint16_t)(si + 0x1e));
        DG16(sy0) = DG16((uint16_t)(si + 0x20));
        DG16(sx1) = (int16_t)(DG16(sx0) + DG16((uint16_t)(si + 0x50)));
        DG16(sy1) = (int16_t)(DG16(sy0) + DG16((uint16_t)(si + 0x52)));
        DG16(sx2) = (int16_t)(DG16(sx0) + DG16((uint16_t)(si + 0x44)));
        DG16(sy2) = (int16_t)(DG16(sy0) + DG16((uint16_t)(si + 0x46)));

        if ((DGU16((uint16_t)(di + 6)) & 0x4000)
            && (DGU16((uint16_t)(si + 6)) & 0x4000)) {
            if (DG16(sx0) >= DG16(x1) || DG16(sx1) <= DG16(x0)
                || DG16(sy0) >= DG16(y1) || DG16(sy1) <= DG16(y0))
                continue;

            answer = 1;
            goto done;
        }

        if (DG16(sx0) >= DG16(x2) || DG16(sx2) <= DG16(x0)
            || DG16(sy0) >= DG16(y2) || DG16(sy2) <= DG16(y0))
            continue;

        if (outlines_cross(di, si) != 0) {
            answer = 1;
            goto done;
        }
    }

done:
    dg_leave(0x18);
    return answer;
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
 * 0x04652
 *
 * Put up the waiting cursor, remembering the one it replaces at DGROUP 0x4ec3
 * so `restore_cursor` can put it back. A cursor that is *already* the waiting
 * one is not remembered, which is what stops two of these in a row losing the
 * cursor the first one replaced.
 */
void wait_cursor(void)
{
    if (DG16(0x4ec5) != 1)
        DG16(0x4ec3) = DG16(0x4ec5);

    select_cursor(1);
}

/*
 * 0x0466e
 *
 * And put back whatever `wait_cursor` remembered.
 */
void restore_cursor(void)
{
    select_cursor((int16_t)DG16(0x4ec3));
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
void select_cursor(int16_t which)
{
    int16_t si = which;
    int16_t hot_y, hot_x;

    if (si > 0x1a)
        si = 0;

    if (si == DG16(0x4ec5))
        return;

    DG16(0x4ec5) = si;

    if (si < 9) {
        hot_y = DG16((uint16_t)(0x284a + 2 * si));
        hot_x = DG16((uint16_t)(0x285c + 2 * si));
    } else {
        hot_y = 0;
        hot_x = 0;
    }

    set_cursor(DGU16((uint16_t)(DGU16(0x52f6) + 2 * si)), hot_y, hot_x);
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
    uint16_t tool = DGU16(0x4e69);

    switch (tool) {
    case 1:  return 4;
    case 2:  return 5;
    case 3:  return 6;
    case 4:  return 6;
    case 5:  return 7;
    case 6:  return 7;
    case 7:  return 2;
    case 8:  return 3;
    case 9:
        if (DGU16((uint16_t)(DGU16(0x50d5) + 4)) == 8)
            return 8;
        if (DGU16((uint16_t)(DGU16(0x50d5) + 4)) == 0x0a)
            return 9;
        return 0;
    default:
        return 0;
    }
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
void reverse_link_ends(uint16_t rec)
{
    uint16_t di, si, t;
    uint8_t b;

    di = DGU16((uint16_t)(DGU16((uint16_t)(rec + 2))
                          + DG8((uint16_t)(rec + 0xa)) * 2 + 0x5a));

    while (di != 0 && DGU16((uint16_t)(di + 4)) == 7) {
        t = DGU16((uint16_t)(di + 0x5a));
        DGU16((uint16_t)(di + 0x5a)) = DGU16((uint16_t)(di + 0x5c));
        DGU16((uint16_t)(di + 0x5c)) = t;
        DGU16((uint16_t)(di + 0x5e)) = DGU16((uint16_t)(di + 0x5a));
        DGU16((uint16_t)(di + 0x60)) = DGU16((uint16_t)(di + 0x5c));

        t = DGU16((uint16_t)(di + 0x6a));
        DGU16((uint16_t)(di + 0x6a)) = DGU16((uint16_t)(di + 0x6c));
        DGU16((uint16_t)(di + 0x6c)) = t;

        si = DGU16((uint16_t)(di + 0x66));
        t = DGU16((uint16_t)(si + 0x14));
        DGU16((uint16_t)(si + 0x14)) = DGU16((uint16_t)(si + 0x18));
        DGU16((uint16_t)(si + 0x18)) = t;
        t = DGU16((uint16_t)(si + 0x16));
        DGU16((uint16_t)(si + 0x16)) = DGU16((uint16_t)(si + 0x1a));
        DGU16((uint16_t)(si + 0x1a)) = t;
        t = DGU16((uint16_t)(si + 0x1c));
        DGU16((uint16_t)(si + 0x1c)) = DGU16((uint16_t)(si + 0x20));
        DGU16((uint16_t)(si + 0x20)) = t;
        t = DGU16((uint16_t)(si + 0x1e));
        DGU16((uint16_t)(si + 0x1e)) = DGU16((uint16_t)(si + 0x22));
        DGU16((uint16_t)(si + 0x22)) = t;
        t = DGU16((uint16_t)(si + 0x24));
        DGU16((uint16_t)(si + 0x24)) = DGU16((uint16_t)(si + 0x28));
        DGU16((uint16_t)(si + 0x28)) = t;
        t = DGU16((uint16_t)(si + 0x26));
        DGU16((uint16_t)(si + 0x26)) = DGU16((uint16_t)(si + 0x2a));
        DGU16((uint16_t)(si + 0x2a)) = t;

        di = DGU16((uint16_t)(di + 0x5c));
    }

    t = DGU16((uint16_t)(rec + 2));
    DGU16((uint16_t)(rec + 2)) = DGU16((uint16_t)(rec + 4));
    DGU16((uint16_t)(rec + 6)) = DGU16((uint16_t)(rec + 2));
    DGU16((uint16_t)(rec + 4)) = t;
    DGU16((uint16_t)(rec + 8)) = t;

    b = DG8((uint16_t)(rec + 0xa));
    DG8((uint16_t)(rec + 0xa)) = DG8((uint16_t)(rec + 0xb));
    DG8((uint16_t)(rec + 0xc)) = DG8((uint16_t)(rec + 0xa));
    DG8((uint16_t)(rec + 0xb)) = b;
    DG8((uint16_t)(rec + 0xd)) = b;

    mark_part_shapes(DGU16(rec), 3);
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
uint16_t part_under_pointer(uint16_t exclude, uint16_t part)
{
    uint16_t si = part;
    uint16_t px = DGU16(0x5784), py = DGU16(0x5782);
    uint16_t ox = (uint16_t)(DGU16((uint16_t)(si + 0x2a)) - DGU16(0x4ea3));
    uint16_t oy = (uint16_t)(DGU16((uint16_t)(si + 0x2c)) - DGU16(0x4ea1));
    uint16_t x0, y0, x1, y1;
    uint16_t link = DGU16((uint16_t)(si + 0x54));
    uint16_t link_end = link ? DGU16((uint16_t)(link + 2)) : 0;
    uint16_t e0 = DGU16((uint16_t)(si + 0x66));
    uint16_t e0_part = e0 ? DGU16(e0) : 0;
    uint16_t e1 = DGU16((uint16_t)(si + 0x68));
    uint16_t e1_part = e1 ? DGU16(e1) : 0;
    uint16_t cur;
    int16_t i;

    x0 = ox;
    y0 = oy;
    x1 = (uint16_t)(x0 + DGU16((uint16_t)(si + 0x44)));
    y1 = (uint16_t)(y0 + DGU16((uint16_t)(si + 0x46)));

    if (exclude != 0
        && (exclude == si || exclude == link_end
            || exclude == e0_part || exclude == e1_part)) {
        x0 = (uint16_t)(x0 - 0xb);
        y0 = (uint16_t)(y0 - 0xb);
        x1 = (uint16_t)(x1 + 0xb);
        y1 = (uint16_t)(y1 + 0xb);
    }

    if (!((int16_t)x0 < (int16_t)px && (int16_t)x1 > (int16_t)px
          && (int16_t)y0 < (int16_t)py && (int16_t)y1 > (int16_t)py))
        return 0;

    if (link != 0 && DGU16(0x4e69) != 9) {
        x0 = (uint16_t)(ox + DG8((uint16_t)(si + 0x56)));
        y0 = (uint16_t)(oy + DG8((uint16_t)(si + 0x57)));
        x1 = (uint16_t)(x0 + DGU16((uint16_t)(si + 0x58)));
        y1 = ((int16_t)DGU16((uint16_t)(si + 0x46)) >> 1)
             < (int16_t)DGU16((uint16_t)(si + 0x58))
             ? (uint16_t)(y0 + 0xa)
             : (uint16_t)(y0 + DGU16((uint16_t)(si + 0x58)));

        if (DGU16((uint16_t)(link + 2)) == exclude) {
            x0 = (uint16_t)(x0 - 0xb);
            y0 = (uint16_t)(y0 - 0xb);
        }

        if ((int16_t)x0 < (int16_t)px && (int16_t)x1 > (int16_t)px
            && (int16_t)y0 < (int16_t)py && (int16_t)y1 > (int16_t)py) {
            if (DGU16((uint16_t)(link + 4)) == si) {
                DGU16((uint16_t)(link + 4)) = DGU16((uint16_t)(link + 6));
                DGU16((uint16_t)(link + 6)) = si;
            }
            return DGU16((uint16_t)(link + 2));
        }
    }

    cur = e0;
    for (i = 0; i < 2; i++) {
        if (cur != 0 && DGU16(0x4e69) != 9
            && DGU16((uint16_t)(si + 4)) != 7) {
            x0 = (uint16_t)(ox + DG8((uint16_t)(si + i * 2 + 0x6a)) - 8);
            y0 = (uint16_t)(oy + DG8((uint16_t)(si + i * 2 + 0x6b)) - 4);
            x1 = (uint16_t)(x0 + 0x10);
            y1 = (uint16_t)(y0 + 8);

            if (DGU16(cur) == exclude) {
                x0 = (uint16_t)(x0 - 0xb);
                y0 = (uint16_t)(y0 - 0xb);
            }

            if ((int16_t)x0 < (int16_t)px && (int16_t)x1 > (int16_t)px
                && (int16_t)y0 < (int16_t)py && (int16_t)y1 > (int16_t)py) {
                if (DGU16((uint16_t)(cur + 2)) == si)
                    reverse_link_ends(cur);
                return DGU16(cur);
            }
        }
        cur = e1;
    }

    return si;
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
uint16_t find_part_from(uint16_t rec)
{
    uint16_t di = rec;
    uint16_t si, cur, best;

    if (di != 0) {
        si = part_under_pointer(di, di);
        if (si != 0)
            return si;
    }

    best = 0;
    cur = pick_by_flag(0x3000);

    while (cur != 0) {
        si = part_under_pointer(di, cur);

        if (si == cur && (DGU16((uint16_t)(cur + 6)) & 0x8000) != 0
            && di != 0) {
            si = 0;
        } else if ((DGU16((uint16_t)(si + 6)) & 0x8000) != 0 && di != 0) {
            si = 0;
        }

        if (si != 0) {
            if ((DGU16((uint16_t)(si + 6)) & 0x8000) != 0)
                best = si;
            else
                return si;
        }

        cur = pick_for_record(cur, 0x1000);
    }

    if (best != 0)
        return best;

    if (DGU16(0x50d5) != 0
        && DGU16((uint16_t)(DGU16(0x50d5) + 4)) == 0x0a)
        return 0;

    return di;
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
uint16_t find_belt_anchor(uint16_t out_end, uint16_t rec)
{
    uint16_t si = find_part_from(rec);
    int16_t e0, e1, d0, d1;

    if (si == 0)
        return 0;

    if ((DGU16((uint16_t)(si + 8)) & 4) == 0)
        return 0;

    if (DGU16((uint16_t)(si + 8)) & 8) {
        e0 = (int16_t)(DGU16((uint16_t)(si + 0x1e)) - DGU16(0x4ea3)
                       + DG8((uint16_t)(si + 0x6a)));
        e1 = (int16_t)(DGU16((uint16_t)(si + 0x1e)) - DGU16(0x4ea3)
                       + DG8((uint16_t)(si + 0x6c)));

        d0 = (int16_t)((int16_t)DGU16(0x5784) - e0);
        if (d0 < 0)
            d0 = (int16_t)-d0;
        d1 = (int16_t)((int16_t)DGU16(0x5784) - e1);
        if (d1 < 0)
            d1 = (int16_t)-d1;

        DGU16(out_end) = (d0 >= d1) ? 1 : 0;
    } else {
        DGU16(out_end) = 0;
    }

    if (DGU16((uint16_t)(si + 4)) == 7) {
        if (DGU16((uint16_t)(si + 0x5a)) != 0)
            si = 0;
    } else if (DGU16((uint16_t)(si + DGU16(out_end) * 2 + 0x66)) != 0) {
        si = 0;
    }

    return si;
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
uint16_t part_flip_options(uint16_t part)
{
    uint16_t si = part;
    uint16_t kind = (uint16_t)((int16_t)DGU16((uint16_t)(si + 4)) * 0x3a);
    uint16_t di = 0;

    if (DGU16((uint16_t)(si + 4)) == 8 || DGU16((uint16_t)(si + 4)) == 0x0a)
        return 0;

    if (DGU16((uint16_t)(si + 8)) & 0x80)
        di |= 1;
    if (DGU16((uint16_t)(si + 8)) & 0x100)
        di |= 2;

    if (DGU16((uint16_t)(si + 6)) & 0x400) {
        if (DGU16(0x4e69) == 9) {
            di |= 4;
        } else {
            call_part_flip(DGU16((uint16_t)(kind + 0x0ed4)),
                           DGU16((uint16_t)(kind + 0x0ed6)), si, 1);
            DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(si + 8));

            if (object_overlaps_any(si) == 0)
                di |= 4;

            call_part_flip(DGU16((uint16_t)(kind + 0x0ed4)),
                           DGU16((uint16_t)(kind + 0x0ed6)), si, 1);
            DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(si + 8));
        }
    }

    if (DGU16((uint16_t)(si + 6)) & 0x200) {
        if (DGU16(0x4e69) == 9) {
            di |= 8;
        } else {
            call_part_flip(DGU16((uint16_t)(kind + 0x0ed4)),
                           DGU16((uint16_t)(kind + 0x0ed6)), si, 2);
            DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(si + 8));

            if (object_overlaps_any(si) == 0)
                di |= 8;

            call_part_flip(DGU16((uint16_t)(kind + 0x0ed4)),
                           DGU16((uint16_t)(kind + 0x0ed6)), si, 2);
            DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(si + 8));
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
uint16_t part_handle_at_pointer(uint16_t part)
{
    uint16_t si = part;
    int16_t px = (int16_t)DGU16(0x5784), py = (int16_t)DGU16(0x5782);
    int16_t di, x_mid, x_end, y0, y_mid, y_end;
    uint16_t rec, end, idx;

    DGU16(0x50bd) = part_flip_options(si);

    if (DGU16((uint16_t)(si + 4)) == 8) {
        rec = DGU16((uint16_t)(DGU16((uint16_t)(si + 0x54)) + 6));

        di = (int16_t)(DGU16((uint16_t)(rec + 0x2a))
                       + DG8((uint16_t)(rec + 0x56)) - DGU16(0x4ea3));
        y0 = (int16_t)(DGU16((uint16_t)(rec + 0x2c))
                       + DG8((uint16_t)(rec + 0x57)) - DGU16(0x4ea3));

        if (di - 11 <= px && px < di && y0 - 11 <= py && py < y0)
            return 8;
        if (px >= di && di + 10 > px && py >= y0 && y0 + 10 > py)
            return 7;
    }

    if (DGU16((uint16_t)(si + 4)) == 0x0a) {
        end = DGU16((uint16_t)(si + 0x66));
        rec = DGU16((uint16_t)(end + 4));
        idx = DG8((uint16_t)(end + 0x0b));

        di = (int16_t)(DGU16((uint16_t)(rec + 0x2a))
                       + DG8((uint16_t)(rec + idx * 2 + 0x6a))
                       - DGU16(0x4ea3) - 8);
        y0 = (int16_t)(DGU16((uint16_t)(rec + 0x2c))
                       + DG8((uint16_t)(rec + idx * 2 + 0x6b))
                       - DGU16(0x4ea3) - 4);

        if (di - 11 <= px && px < di && y0 - 11 <= py && py < y0)
            return 8;
        if (px >= di && di + 15 > px && py >= y0 && y0 + 7 > py)
            return 7;
    }

    di = (int16_t)(DGU16((uint16_t)(si + 0x2a)) - DGU16(0x4ea3));
    x_mid = (int16_t)(di + ((int16_t)DGU16((uint16_t)(si + 0x44)) >> 1) - 6);
    x_end = (int16_t)(di + (int16_t)DGU16((uint16_t)(si + 0x44)));
    y0 = (int16_t)(DGU16((uint16_t)(si + 0x2c)) - DGU16(0x4ea1));
    y_mid = (int16_t)(y0 + ((int16_t)DGU16((uint16_t)(si + 0x46)) >> 1) - 6);
    y_end = (int16_t)(y0 + (int16_t)DGU16((uint16_t)(si + 0x46)));

    if (di - 11 <= px && px < di && y0 - 11 <= py && py < y0)
        return 8;

    if (DGU16(0x50bd) & 1) {
        if (di - 11 <= px && px < di && py >= y_mid && y_mid + 11 > py)
            return 3;
        if (px > x_end && x_end + 11 > px && py >= y_mid && y_mid + 11 > py)
            return 4;
    }

    if (DGU16(0x50bd) & 2) {
        if (y0 - 11 <= py && py < y0 && px >= x_mid && x_mid + 11 > px)
            return 5;
        if (py > y_end && y_end + 11 > py && px >= x_mid && x_mid + 11 > px)
            return 6;
    }

    if (DGU16(0x50bd) & 4) {
        if (di - 11 <= px && px < di && py > y_end && y_end + 11 > py)
            return 1;
    }

    if (DGU16(0x50bd) & 8) {
        if (px > x_end && x_end + 11 > px && py > y_end && y_end + 11 > py)
            return 2;
    }

    if (px >= di && px < x_end && py >= y0 && py < y_end)
        return 7;

    return 0x0a;
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
int16_t rope_ends_close(uint16_t rope)
{
    uint16_t si = DGU16((uint16_t)(rope + 4));
    uint16_t di;

    if (si == 0) {
        si = find_part_from(0);
        if (si == 0)
            return 0;
        if ((DGU16((uint16_t)(si + 8)) & 2) != 0
            || (DGU16((uint16_t)(si + 8)) & 1) == 0)
            return 0;
        return 1;
    }

    di = DGU16((uint16_t)(rope + 6));
    if (di == 0) {
        di = find_part_from(0);
        if (di == 0)
            return 0;
        if ((DGU16((uint16_t)(di + 8)) & 2) != 0
            || (DGU16((uint16_t)(di + 8)) & 1) == 0)
            return 0;
    }

    return points_within_140((uint16_t)(si + 0x1e), (uint16_t)(di + 0x1e));
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
 * 0x05628
 *
 * Take a node out of a doubly linked list: the previous one's `next` at +0
 * becomes this one's, and the next one - if there is one - has its `prev` at
 * +2 pointed back past it. Nothing is written into the node itself, so it
 * still points at both of its old neighbours when this returns.
 */
void unlink_node(uint16_t node)
{
    DGU16(DGU16((uint16_t)(node + 2))) = DGU16(node);

    if (DGU16(node) != 0)
        DGU16((uint16_t)(DGU16(node) + 2)) = DGU16((uint16_t)(node + 2));
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
 * a save produces. `read_list` in seg0dff.c said the opposite for a while,
 * because this header led with the word "sorted".
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
int16_t bin_part_at_index(int16_t index)
{
    int16_t dx = 0;
    uint16_t si, di;

    if (index < 0) {
        si = DGU16(0x50d3);
        while (dx != index) {
            di = DGU16((uint16_t)(si + 4));
            while (si != 0x50d7 && DGU16((uint16_t)(si + 4)) == di)
                si = DGU16((uint16_t)(si + 2));
            dx--;
        }
        return (int16_t)si;
    }

    si = DGU16(DGU16(0x50d3));
    while (dx != index) {
        di = DGU16((uint16_t)(si + 4));
        while (si != 0 && DGU16((uint16_t)(si + 4)) == di)
            si = DGU16(si);
        dx++;
    }
    if (si != 0)
        si = DGU16((uint16_t)(si + 2));
    return (int16_t)si;
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
void refile_part_list(uint16_t part)
{
    uint16_t si = part;
    uint16_t list;

    unlink_node(si);

    if (DGU16((uint16_t)(si + 6)) & 0x4000) {
        DGU16((uint16_t)(si + 6)) =
            (uint16_t)((DGU16((uint16_t)(si + 6)) & 0xf7ff) | 0x2000);
        list = 0x521b;
    } else {
        DGU16((uint16_t)(si + 6)) =
            (uint16_t)((DGU16((uint16_t)(si + 6)) & 0xf7ff) | 0x1000);
        list = 0x5179;
    }

    insert_sorted(si, list);

    if (DGU16(0x50d3) != 0x50d7 && DGU16(DGU16(0x50d3)) == 0)
        DGU16(0x50d3) = DGU16((uint16_t)(DGU16(0x50d3) + 2));
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
uint16_t bin_scroll_end(void)
{
    uint16_t saved = DGU16(0x50d3);
    uint16_t si, last;

    while ((si = (uint16_t)bin_part_at_index(5)) != 0)
        DGU16(0x50d3) = si;

    last = DGU16(0x50d3);
    DGU16(0x50d3) = saved;
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
void mark_needs_refile(uint16_t part, uint8_t n)
{
    uint16_t fp = dg_enter(4);
    uint16_t rope = fp;                     /* [bp-4] */
    uint16_t i = (uint16_t)(fp + 2);        /* [bp-2] */
    uint16_t di = part;
    uint16_t si;

    if (DGU16((uint16_t)(di + 4)) != 0x31)
        DG8((uint16_t)(di + 0x14)) = n;

    if (DGU16((uint16_t)(di + 4)) == 7) {
        si = DGU16((uint16_t)(di + 0x68));
        if (si != 0)
            DG8((uint16_t)(DGU16(si) + 0x14)) = n;
        goto out;
    }

    DGU16(rope) = DGU16((uint16_t)(di + 0x54));
    if (DGU16(rope) != 0) {
        if (DG16(0x4e6b) == 0x1000) {
            compute_link_endpoints(DGU16(rope));
            if (rope_ends_close(DGU16(rope)) != 0)
                DG8((uint16_t)(DGU16((uint16_t)(DGU16(rope) + 2)) + 0x14)) = n;
        } else {
            DG8((uint16_t)(DGU16((uint16_t)(DGU16(rope) + 2)) + 0x14)) = n;
        }
    }

    if (DG16(0x4e6b) == 0x2000) {
        si = DGU16((uint16_t)(di + 0x66));
        if (si != 0 && DG8((uint16_t)(DGU16(si) + 0x14)) == 0) {
            DG8((uint16_t)(DGU16(si) + 0x14)) = n;
            refresh_link_geometry(si);
        }

        si = DGU16((uint16_t)(di + 0x68));
        if (si != 0 && DG8((uint16_t)(DGU16(si) + 0x14)) == 0) {
            DG8((uint16_t)(DGU16(si) + 0x14)) = n;
            refresh_link_geometry(si);
        }
        goto out;
    }

    for (DG16(i) = 0; DG16(i) < 2; DG16(i)++) {
        si = DGU16((uint16_t)(di + 0x66 + 2 * DGU16(i)));
        if (si == 0)
            continue;

        DG8((uint16_t)(DGU16(si) + 0x14)) = n;
        refresh_link_geometry(si);
    }

out:
    dg_leave(4);
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
 * the zeros `heap_calloc_far` gives, so a copy starts nowhere and on no list
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
uint16_t clone_part(uint16_t part)
{
    uint16_t fp = dg_enter(8);
    uint16_t failed = (uint16_t)(fp + 0x04);    /* [bp-4] */
    uint16_t dst_pt = (uint16_t)(fp + 0x02);    /* [bp-6] */
    uint16_t src_pt = (uint16_t)(fp + 0x00);    /* [bp-8] */
    uint16_t di = part;
    uint16_t si;
    int16_t i;

    DGU16(failed) = 0;

    si = heap_calloc_far(1, 0xa2);
    if (si == 0)
        goto give_up;

    DGU16((uint16_t)(si + 0x04)) = DGU16((uint16_t)(di + 0x04));
    DGU16((uint16_t)(si + 0x06)) = DGU16((uint16_t)(di + 0x06));
    DGU16((uint16_t)(si + 0x08)) = DGU16((uint16_t)(di + 0x08));
    DGU16((uint16_t)(si + 0x0a)) = DGU16((uint16_t)(di + 0x0a));
    DGU16((uint16_t)(si + 0x0c)) = DGU16((uint16_t)(di + 0x0c));
    DGU16((uint16_t)(si + 0x0e)) = DGU16((uint16_t)(di + 0x0e));
    DGU16((uint16_t)(si + 0x10)) = DGU16((uint16_t)(di + 0x10));
    DGU16((uint16_t)(si + 0x12)) = DGU16((uint16_t)(di + 0x12));
    DGU16((uint16_t)(si + 0x42)) = DGU16((uint16_t)(di + 0x42));
    DGU16((uint16_t)(si + 0x40)) = DGU16((uint16_t)(di + 0x40));
    DGU16((uint16_t)(si + 0x46)) = DGU16((uint16_t)(di + 0x46));
    DGU16((uint16_t)(si + 0x44)) = DGU16((uint16_t)(di + 0x44));
    DGU16((uint16_t)(si + 0x52)) = DGU16((uint16_t)(di + 0x52));
    DGU16((uint16_t)(si + 0x50)) = DGU16((uint16_t)(di + 0x50));

    if (DGU16((uint16_t)(si + 4)) == 8) {
        DGU16((uint16_t)(si + 0x54)) = heap_calloc_far(1, 0x38);
        if (DGU16((uint16_t)(si + 0x54)) == 0)
            goto give_up;
        DGU16((uint16_t)(DGU16((uint16_t)(si + 0x54)) + 2)) = si;
    }

    DGU16((uint16_t)(si + 0x56)) = DGU16((uint16_t)(di + 0x56));
    DGU16((uint16_t)(si + 0x58)) = DGU16((uint16_t)(di + 0x58));

    if (DGU16((uint16_t)(si + 4)) == 0x0a || DGU16((uint16_t)(si + 4)) == 7) {
        DGU16((uint16_t)(si + 0x66)) = heap_calloc_far(1, 0x2c);
        if (DGU16((uint16_t)(si + 0x66)) == 0)
            goto give_up;
        DGU16(DGU16((uint16_t)(si + 0x66))) = si;
    }

    DGU16((uint16_t)(si + 0x6a)) = DGU16((uint16_t)(di + 0x6a));
    DGU16((uint16_t)(si + 0x6c)) = DGU16((uint16_t)(di + 0x6c));

    DGU16((uint16_t)(si + 0x80)) =
        DGU16((uint16_t)(0x0ec4
                         + 0x3a * (int16_t)DG16((uint16_t)(si + 4))));

    if (DGU16((uint16_t)(si + 0x80)) != 0) {
        DGU16(src_pt) = DGU16((uint16_t)(di + 0x82));

        DGU16((uint16_t)(si + 0x82)) =
            heap_calloc_far(DGU16((uint16_t)(si + 0x80)), 4);
        DGU16(dst_pt) = DGU16((uint16_t)(si + 0x82));
        if (DGU16(dst_pt) == 0)
            goto give_up;

        for (i = 0; DG16((uint16_t)(si + 0x80)) > i; i++) {
            DGU16((uint16_t)(DGU16(dst_pt) + 2)) =
                DGU16((uint16_t)(DGU16(src_pt) + 2));
            DGU16(DGU16(dst_pt)) = DGU16(DGU16(src_pt));
            DGU16(dst_pt) += 4;
            DGU16(src_pt) += 4;
        }
    }

    DGU16((uint16_t)(si + 0x90)) = DGU16((uint16_t)(di + 0x90));
    DGU16((uint16_t)(si + 0x92)) = DGU16((uint16_t)(di + 0x92));
    DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(di + 0x94));

    goto out;

give_up:
    DGU16(failed) = 1;

out:
    {
        uint16_t answer;

        if (DGU16(failed) != 0) {
            free_part(si);
            answer = 0;
        } else {
            answer = si;
        }

        dg_leave(8);
        return answer;
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
void untie_rope(uint16_t part)
{
    uint16_t rope = DGU16((uint16_t)(part + 0x54));
    uint16_t end;

    if (rope == 0)
        return;

    end = DGU16((uint16_t)(rope + 4));
    if (end != 0) {
        DGU16((uint16_t)(end + 8)) &= 0xfffd;
        DGU16((uint16_t)(end + 0x94)) = DGU16((uint16_t)(end + 8));
        DGU16((uint16_t)(end + 0x54)) = 0;
        DGU16((uint16_t)(rope + 4)) = 0;
    }

    end = DGU16((uint16_t)(rope + 6));
    if (end != 0) {
        DGU16((uint16_t)(end + 8)) &= 0xfffd;
        DGU16((uint16_t)(end + 0x94)) = DGU16((uint16_t)(end + 8));
        DGU16((uint16_t)(end + 0x54)) = 0;
        DGU16((uint16_t)(rope + 6)) = 0;
    }

    if ((DGU16((uint16_t)(part + 6)) & 0x800) == 0)
        sub_05704(part);
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
 * `sub_05704`, taking some other part off - only the second end does, and the
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
 * Both slots end by calling `sub_05704` on the part unless bit 11 of its +6 is
 * set - the same guard, and the same fall-through into the common path, that
 * `untie_rope` has.
 */
void detach_belt(uint16_t part, uint16_t how)
{
    int16_t i;

    for (i = 0; i < 2; i++) {
        uint16_t belt = DGU16((uint16_t)(part + 0x66 + 2 * i));
        uint16_t other, next;
        int16_t  slot;

        if (belt == 0)
            continue;

        if (how != 0) {
            other = DGU16((uint16_t)(belt + 2));
            if (other != 0) {
                DGU16((uint16_t)(belt + 2)) = 0;
                DGU16((uint16_t)(belt + 6)) = 0;

                slot = (int16_t)DG8((uint16_t)(belt + 0x0a));
                DGU16((uint16_t)(other + 0x66 + 2 * slot)) = 0;

                next = DGU16((uint16_t)(other + 0x5a + 2 * slot));
                DGU16((uint16_t)(other + 0x5a + 2 * (slot + 2))) = 0;
                DGU16((uint16_t)(other + 0x5a + 2 * slot)) = 0;

                while (next != 0 && DG16((uint16_t)(next + 4)) == 7) {
                    uint16_t after = DGU16((uint16_t)(next + 0x5a));
                    int16_t  j;

                    for (j = 0; j < 4; j++)
                        DGU16((uint16_t)(next + 0x5a + 2 * j)) = 0;
                    DGU16((uint16_t)(next + 0x68)) = 0;
                    next = after;
                }
            }
        }

        other = DGU16((uint16_t)(belt + 4));
        if (other != 0) {
            slot = (int16_t)DG8((uint16_t)(belt + 0x0b));
            DGU16((uint16_t)(other + 0x66 + 2 * slot)) = 0;

            DGU16((uint16_t)(belt + 4)) = 0;
            DGU16((uint16_t)(belt + 8)) = 0;

            next = DGU16((uint16_t)(other + 0x5a + 2 * slot));
            DGU16((uint16_t)(other + 0x5a + 2 * (slot + 2))) = 0;
            DGU16((uint16_t)(other + 0x5a + 2 * slot)) = 0;

            if (next != 0 && how == 0) {
                slot = match_field_5a_5c(other, next);
                DGU16((uint16_t)(next + 0x5a + 2 * (slot + 2))) = 0;
                DGU16((uint16_t)(next + 0x5a + 2 * slot)) = 0;
            }
        }

        if ((DGU16((uint16_t)(part + 6)) & 0x800) == 0)
            sub_05704(part);
    }
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
void sub_05704(uint16_t part)
{
    int16_t i;

    if (!((DGU16(0x4e69) == 8 || DGU16(0x4e69) == 7)
          && DGU16(0x4e6b) == 0x1000)) {

        if (DGU16((uint16_t)(part + 0x54)) != 0
            && DG16((uint16_t)(part + 4)) != 8)
            untie_rope(DGU16((uint16_t)(DGU16((uint16_t)(part + 0x54)) + 2)));

        if (DG16((uint16_t)(part + 4)) != 0x0a
            && DG16((uint16_t)(part + 4)) != 7) {
            for (i = 0; i < 2; i++) {
                uint16_t slot = DGU16((uint16_t)(part + 0x66 + 2 * i));

                if (slot != 0)
                    detach_belt(DGU16(slot), 0);
            }
        }
    }

    DGU16((uint16_t)(part + 6)) =
        (uint16_t)((DGU16((uint16_t)(part + 6)) & 0xcfff) | 0x800);

    unlink_node(part);
    insert_sorted(part, 0x50d7);
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
 * at DGROUP 0xed0, indexed by kind times 0x3a - the same dispatch
 * `call_part_setup` is used for elsewhere - and afterwards +0x0c is copied to
 * +0x90. Both halves do that copy, and both do it to the part at the *far* end
 * rather than to the one they were given.
 */
void sub_051cb(uint16_t part)
{
    uint16_t other;
    int16_t  i, bx;

    if (DGU16((uint16_t)(part + 0x0a)) & 2) {
        for (i = 4; i < 6; i++) {
            other = DGU16((uint16_t)(part + 0x5a + 2 * i));
            if (other == 0)
                continue;

            DGU16((uint16_t)(part + 0x5a + 2 * i)) = 0;
            DGU16((uint16_t)(other + 0x62)) = 0;

            bx = (int16_t)(DG16((uint16_t)(other + 4)) * 0x3a);
            call_part_setup(DGU16((uint16_t)(bx + 0x0ed0)),
                            DGU16((uint16_t)(bx + 0x0ed2)), other);
        }

        bx = (int16_t)(DG16((uint16_t)(part + 4)) * 0x3a);
        call_part_setup(DGU16((uint16_t)(bx + 0x0ed0)),
                        DGU16((uint16_t)(bx + 0x0ed2)), part);

        DGU16((uint16_t)(part + 0x90)) = DGU16((uint16_t)(part + 0x0c));
        return;
    }

    other = DGU16((uint16_t)(part + 0x62));
    if (other == 0)
        return;

    DGU16((uint16_t)(other + 0x5a
                     + 2 * (DG8((uint16_t)(part + 0x7e)) + 4))) = 0;
    DGU16((uint16_t)(part + 0x62)) = 0;

    bx = (int16_t)(DG16((uint16_t)(part + 4)) * 0x3a);
    call_part_setup(DGU16((uint16_t)(bx + 0x0ed0)),
                    DGU16((uint16_t)(bx + 0x0ed2)), part);

    bx = (int16_t)(DG16((uint16_t)(other + 4)) * 0x3a);
    call_part_setup(DGU16((uint16_t)(bx + 0x0ed0)),
                    DGU16((uint16_t)(bx + 0x0ed2)), other);

    DGU16((uint16_t)(other + 0x90)) = DGU16((uint16_t)(other + 0x0c));
}

/*
 * 0x04c0d
 *
 * **The angle from one part to another**, in the sixteen-bit turn this code
 * works in - `atan2_long` answers it and 0x4000 is a quarter. `sub_04d4c` uses
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
 * `sub_04d4c` writes, which is what makes the two routines a pair: one decides
 * where a link points, the other where the next one hangs from.
 *
 * The differences are sign-extended to longs before the divide, because a part
 * can be further away than a word holds once the view's origin is in it.
 */
uint16_t sub_04c0d(uint16_t part, uint16_t other)
{
    int32_t dx, dy;

    if (other == 0) {
        dx = (int32_t)(int16_t)(DG16((uint16_t)(part + 0x1e))
                                - (DG16(0x5784) + DG16(0x4ea3)));
        dy = (int32_t)(int16_t)(DG16((uint16_t)(part + 0x20))
                                - (DG16(0x5782) + DG16(0x4ea1)));
    } else if (DG16((uint16_t)(other + 4)) == 7) {
        dx = (int32_t)(int16_t)(DG16((uint16_t)(part + 0x1e))
                                - DG16((uint16_t)(other + 0x1e)));
        dy = (int32_t)(int16_t)(DG16((uint16_t)(part + 0x20))
                                - DG16((uint16_t)(other + 0x20)));
    } else {
        int16_t slot = match_field_5a_5c((int16_t)part, other);

        dx = (int32_t)(int16_t)(
                 DG16((uint16_t)(part + 0x1e))
                 - (int16_t)(DG16((uint16_t)(other + 0x1e))
                             + DG8((uint16_t)(other + 0x6a + 2 * slot))));
        dy = (int32_t)(int16_t)(
                 DG16((uint16_t)(part + 0x20))
                 - (int16_t)(DG16((uint16_t)(other + 0x20))
                             + DG8((uint16_t)(other + 0x6b + 2 * slot))));
    }

    return (uint16_t)atan2_long((uint16_t)dx, (uint16_t)(dx >> 16),
                                (uint16_t)dy, (uint16_t)(dy >> 16));
}

/*
 * 0x04d4c
 *
 * **Point a chain link along the bisector of its two neighbours.** Called on a
 * kind-7 part after a neighbour has been spliced out, and always followed by
 * `mark_part_shapes(part, 3)`, which is what makes the new shape draw.
 *
 * The angle to each neighbour comes from `sub_04c0d`, and 0x2000 is added to
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
 * `sub_051cb` copies.
 */
void sub_04d4c(uint16_t part)
{
    uint16_t after, before, a1, a2, d, mid;
    int16_t  quad;

    after = DGU16((uint16_t)(part + 0x5c));
    if (after == 0)
        return;

    before = DGU16((uint16_t)(part + 0x5a));

    a1 = (uint16_t)(sub_04c0d(part, after) + 0x2000);
    a2 = (uint16_t)(sub_04c0d(part, before) + 0x2000);
    d = (uint16_t)(a2 - a1);

    if (d < 0x8000)
        mid = (uint16_t)(a1 + (d >> 1));
    else
        mid = (uint16_t)(a2 + ((uint16_t)(0 - d) >> 1));

    quad = (int16_t)((mid >> 14) & 3);

    if ((quad & 1) == 0) {
        uint8_t v = (uint8_t)(quad != 0 ? 6 : 0x0a);

        DG8((uint16_t)(part + 0x6d)) = v;
        DG8((uint16_t)(part + 0x6b)) = v;

        if ((quad == 0 && d < 0x8000) || (quad == 2 && d >= 0x8000)) {
            DG8((uint16_t)(part + 0x6a)) = 0;
            DG8((uint16_t)(part + 0x6c)) = 0x0f;
        } else {
            DG8((uint16_t)(part + 0x6c)) = 0;
            DG8((uint16_t)(part + 0x6a)) = 0x0f;
        }
    } else {
        uint8_t v = (uint8_t)(quad == 1 ? 6 : 0x0a);

        DG8((uint16_t)(part + 0x6c)) = v;
        DG8((uint16_t)(part + 0x6a)) = v;

        if ((quad == 1 && d < 0x8000) || (quad == 3 && d >= 0x8000)) {
            DG8((uint16_t)(part + 0x6b)) = 0;
            DG8((uint16_t)(part + 0x6d)) = 0x0f;
        } else {
            DG8((uint16_t)(part + 0x6d)) = 0;
            DG8((uint16_t)(part + 0x6b)) = 0x0f;
        }
    }

    DGU16((uint16_t)(part + 0x0c)) = (uint16_t)quad;
    DGU16((uint16_t)(part + 0x90)) = (uint16_t)quad;
}

/*
 * 0x05457
 *
 * **Discard a part - but only really in freeform mode.** Every path through
 * `sub_05482` ends here, and the rope and belt paths call it on what they
 * detached as well.
 *
 * The free is behind DGROUP 0x4e67, the freeform flag. In a level the part is
 * *not* unlinked and *not* freed: `sub_05704` has already put it in the bin at
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
void discard_part(uint16_t part)
{
    if (DGU16(0x4e67) != 0) {
        unlink_node(part);
        free_part(part);
    }

    if (part == DGU16(0x50d5))
        DGU16(0x50d5) = 0;
}

/*
 * 0x05482
 *
 * **Finish taking a part out**, on whatever DGROUP 0x50d5 points at. It takes
 * no argument, which is why `remove_all_parts` sets that word and clears it
 * again around the call.
 *
 * **It requires bit 11 of +6 to be set and leaves at once otherwise** - and that
 * is the bit `sub_05704` sets. So the two are a sequence and not alternatives:
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
void sub_05482(void)
{
    uint16_t p = DGU16(0x50d5);
    uint16_t rope, other, next;
    int16_t  a, b, i;

    if (p == 0)
        return;
    if ((DGU16((uint16_t)(p + 6)) & 0x800) == 0)
        return;

    if (DGU16((uint16_t)(p + 0x0a)) & 3)
        sub_051cb(p);

    rope = DGU16((uint16_t)(p + 0x54));
    if (DG16((uint16_t)(p + 4)) != 8 && rope != 0) {
        uint16_t r = DGU16((uint16_t)(rope + 2));

        untie_rope(r);
        discard_part(r);
    }

    if (DG16((uint16_t)(p + 4)) == 7) {
        next = DGU16((uint16_t)(p + 0x5a));
        if (next != 0) {
            a = match_field_5a_5c((int16_t)p, next);
            other = DGU16((uint16_t)(p + 0x5c));
            b = match_field_5a_5c((int16_t)p, other);

            DGU16((uint16_t)(next + 0x5a + 2 * (a + 2))) = other;
            DGU16((uint16_t)(next + 0x5a + 2 * a)) = other;
            DGU16((uint16_t)(other + 0x5a + 2 * (b + 2))) = next;
            DGU16((uint16_t)(other + 0x5a + 2 * b)) = next;

            if (DG16((uint16_t)(next + 4)) == 7) {
                sub_04d4c(next);
                mark_part_shapes(next, 3);
            }
            if (DG16((uint16_t)(other + 4)) == 7) {
                sub_04d4c(other);
                mark_part_shapes(other, 3);
            }

            mark_needs_refile(DGU16(DGU16((uint16_t)(p + 0x68))), 2);

            for (i = 0; i < 4; i++)
                DGU16((uint16_t)(p + 0x5a + 2 * i)) = 0;
            DGU16((uint16_t)(p + 0x68)) = 0;
        }
    } else if (DG16((uint16_t)(p + 4)) != 0x0a) {
        for (i = 0; i < 2; i++) {
            uint16_t slot = DGU16((uint16_t)(p + 0x66 + 2 * i));

            if (slot != 0) {
                uint16_t belt = DGU16(slot);

                detach_belt(belt, 1);
                discard_part(belt);
            }
        }
    }

    discard_part(p);
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
    uint16_t si = (uint16_t)pick_by_flag(0x3000);

    while (si != 0) {
        if (DGU16((uint16_t)(si + 6)) & 0x8000) {
            si = (uint16_t)pick_for_record(si, 0x1000);
            continue;
        }

        if (DG16((uint16_t)(si + 4)) == 8)
            untie_rope(si);
        else if (DG16((uint16_t)(si + 4)) == 0x0a)
            detach_belt(si, 1);
        else
            sub_05704(si);

        DGU16(0x50d5) = si;
        sub_05482();
        DGU16(0x50d5) = 0;

        si = (uint16_t)pick_by_flag(0x3000);
    }
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
void place_object_for_draw(uint16_t obj)
{
    int16_t type = DG16(obj + 4);
    uint16_t rec = (uint16_t)(0xea6 + 0x3a * type);
    uint16_t idx = DGU16(obj + 0xc);
    int16_t flags = DG16(obj + 8);
    uint16_t hot;

    DG16(obj + 0x2a) = DG16(obj + 0x1e);
    DG16(obj + 0x2c) = DG16(obj + 0x20);

    set_object_extent(obj);

    if (DG16(rec + 0x18) == 0)
        return;

    hot = (uint16_t)(DGU16(rec + 0x18) + 2 * idx);

    if ((flags & 0x10) != 0)
        DG16(obj + 0x2a) = (int16_t)(DG16(obj + 0x2a)
                                     + (DG16(obj + 0x40)
                                        - (int16_t)(int8_t)DG8(hot)
                                        - DG16(obj + 0x44)));
    else
        DG16(obj + 0x2a) = (int16_t)(DG16(obj + 0x2a)
                                     + (int16_t)(int8_t)DG8(hot));

    if ((flags & 0x20) != 0)
        DG16(obj + 0x2c) = (int16_t)(DG16(obj + 0x2c)
                                     + (DG16(obj + 0x42)
                                        - (int16_t)(int8_t)DG8(hot + 1)
                                        - DG16(obj + 0x46)));
    else
        DG16(obj + 0x2c) = (int16_t)(DG16(obj + 0x2c)
                                     + (int16_t)(int8_t)DG8(hot + 1));
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
void part_finish_angles(uint16_t part)
{
    uint16_t fp = dg_enter(0x0e);
    uint16_t pair = fp;                   /* [bp-0xe]: x0, y0, x1, y1 */
    uint16_t si = DGU16((uint16_t)(part + 0x82));
    int16_t n = 1;

    while (DG16((uint16_t)(part + 0x80)) > n) {
        int16_t dx, dy;

        DG16(pair) = DG8(si);
        DG16((uint16_t)(pair + 2)) = DG8((uint16_t)(si + 1));
        DG16((uint16_t)(pair + 4)) = DG8((uint16_t)(si + 4));
        DG16((uint16_t)(pair + 6)) = DG8((uint16_t)(si + 5));

        step_pair_apart(pair);

        dx = (int16_t)(DG16((uint16_t)(pair + 4)) - DG16(pair));
        dy = (int16_t)(DG16((uint16_t)(pair + 6))
                       - DG16((uint16_t)(pair + 2)));

        DG16((uint16_t)(si + 2)) =
            (int16_t)(0xc000 - (uint16_t)atan2_long((uint16_t)dx,
                                                    (uint16_t)(dx < 0 ? -1 : 0),
                                                    (uint16_t)dy,
                                                    (uint16_t)(dy < 0 ? -1 : 0)));

        n++;
        si = (uint16_t)(si + 4);
    }

    {
        uint16_t first = DGU16((uint16_t)(part + 0x82));
        int16_t dx, dy;

        DG16(pair) = DG8(si);
        DG16((uint16_t)(pair + 2)) = DG8((uint16_t)(si + 1));
        DG16((uint16_t)(pair + 4)) = DG8(first);
        DG16((uint16_t)(pair + 6)) = DG8((uint16_t)(first + 1));

        step_pair_apart(pair);

        dx = (int16_t)(DG16((uint16_t)(pair + 4)) - DG16(pair));
        dy = (int16_t)(DG16((uint16_t)(pair + 6))
                       - DG16((uint16_t)(pair + 2)));

        DG16((uint16_t)(si + 2)) =
            (int16_t)(0xc000 - (uint16_t)atan2_long((uint16_t)dx,
                                                    (uint16_t)(dx < 0 ? -1 : 0),
                                                    (uint16_t)dy,
                                                    (uint16_t)(dy < 0 ? -1 : 0)));
    }

    dg_leave(0x0e);
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
void mark_joined_shapes(uint16_t part, uint16_t mode)
{
    uint16_t fp = dg_enter(2);
    uint16_t rope = fp;                     /* [bp-2] */
    uint16_t si = part;
    uint16_t di;

    if (DGU16((uint16_t)(si + 4)) == 7) {
        di = DGU16((uint16_t)(si + 0x68));
        if (di != 0)
            mark_belt_shapes(DGU16(di), mode);
        goto out;
    }

    if (DGU16((uint16_t)(si + 4)) == 8 || DGU16((uint16_t)(si + 4)) == 0x0a)
        goto out;

    if (DG16(0x4e6b) != 0x2000) {
        DGU16(rope) = DGU16((uint16_t)(si + 0x54));
        if (DGU16(rope) != 0)
            add_sub_object_shapes(DGU16((uint16_t)(DGU16(rope) + 2)), (int16_t)mode);
    }

    di = DGU16((uint16_t)(si + 0x66));
    if (di != 0)
        mark_belt_shapes(DGU16(di), mode);

    di = DGU16((uint16_t)(si + 0x68));
    if (di != 0)
        mark_belt_shapes(DGU16(di), mode);

out:
    dg_leave(2);
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
void add_sub_object_shapes(uint16_t obj, int16_t mask)
{
    uint16_t sub = DGU16(obj + 0x54);

    if ((mask & 1) != 0) {
        alloc_shape((uint16_t)(sub + 0x28), (uint16_t)(sub + 0x2c), 4, 1, 0);
        alloc_shape((uint16_t)(sub + 0x30), (uint16_t)(sub + 0x34), 4, 1, 0);
    }

    if ((mask & 2) != 0) {
        alloc_shape((uint16_t)(sub + 0x18), (uint16_t)(sub + 0x1c), 4, 2, 0);
        alloc_shape((uint16_t)(sub + 0x20), (uint16_t)(sub + 0x24), 4, 2, 0);
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
void set_object_extent(uint16_t obj)
{
    int16_t type = DG16(obj + 4);
    uint16_t rec, pair, target;

    if (type == 8 || type == 0xa) {
        DG16(obj + 0x46) = 0;
        DG16(obj + 0x44) = 0;
        return;
    }

    if ((DG16(obj + 6) & 0x40) != 0) {
        DG16(obj + 0x44) = DG16(obj + 0x50);
        DG16(obj + 0x46) = DG16(obj + 0x52);
        return;
    }

    rec = (uint16_t)(0xea6 + 0x3a * type);

    if (DG16(rec + 0x1a) != 0) {
        pair = (uint16_t)(DGU16(rec + 0x1a) + 4 * DGU16(obj + 0xc));
        DG16(obj + 0x44) = DG16(pair);
        DG16(obj + 0x46) = DG16(pair + 2);
        return;
    }

    if (DG16(rec + 0x14) != 0) {
        target = DGU16(DGU16(rec + 0x14) + 2 * DGU16(obj + 0xc));
        DG16(obj + 0x44) = DG16(target + 6);
        DG16(obj + 0x46) = DG16(target + 8);
        return;
    }

    DG16(obj + 0x46) = 0;
    DG16(obj + 0x44) = 0;
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
void mark_belt_shapes(uint16_t part, uint16_t mode)
{
    uint16_t fp = dg_enter(0x12);
    uint16_t v12 = (uint16_t)(fp + 0x00);   /* [bp-0x12] the far part */
    uint16_t v10 = (uint16_t)(fp + 0x02);   /* [bp-0x10] the near part */
    uint16_t v0e = (uint16_t)(fp + 0x04);   /* [bp-0x0e] the far point */
    uint16_t v0c = (uint16_t)(fp + 0x06);   /* [bp-0x0c] the near point */
    uint16_t v0a = (uint16_t)(fp + 0x08);   /* [bp-0x0a] the box, 0x10 wide */
    uint16_t v06 = (uint16_t)(fp + 0x0c);   /* [bp-6] the box's corner */
    uint16_t v02 = (uint16_t)(fp + 0x10);   /* [bp-2] the slack */
    uint16_t si = DGU16((uint16_t)(part + 0x66));
    int16_t di;

    DG16(v0a) = 0x10;
    DG16((uint16_t)(v0a + 2)) = 0x10;

    if (DG16(0x4e6b) != 0x2000)
        goto plain;

    DGU16(v10) = DGU16((uint16_t)(si + 2));
    DGU16(v12) = DGU16((uint16_t)(DGU16(v10) + 0x5a
                                  + 2 * DG8((uint16_t)(si + 0x0a))));

    if (mode & 1) {
        DGU16(v0e) = (DGU16((uint16_t)(DGU16(v12) + 4)) == 7)
                     ? (uint16_t)(DGU16((uint16_t)(DGU16(v12) + 0x66)) + 0x24)
                     : (uint16_t)(si + 0x28);

        DG16(v02) = link_slack(DGU16(v10), si, 1);
        alloc_shape((uint16_t)(si + 0x24), DGU16(v0e), 4, 1, DG16(v02));

        for (di = 0; di < 2; di++) {
            DG16(v06) = (int16_t)(DG16((uint16_t)(si + 0x24 + 4 * di)) - 8);
            DG16((uint16_t)(v06 + 2)) =
                (int16_t)(DG16((uint16_t)(si + 0x26 + 4 * di)) - 8);
            alloc_shape(v06, v0a, 1, 1, 0);
        }
    }

    if (mode & 2) {
        DGU16(v0e) = (DGU16((uint16_t)(DGU16(v12) + 4)) == 7)
                     ? (uint16_t)(DGU16((uint16_t)(DGU16(v12) + 0x66)) + 0x1c)
                     : (uint16_t)(si + 0x20);

        DG16(v02) = link_slack(DGU16(v10), si, 2);
        alloc_shape((uint16_t)(si + 0x1c), DGU16(v0e), 4, 2, DG16(v02));

        for (di = 0; di < 2; di++) {
            DG16(v06) = (int16_t)(DG16((uint16_t)(si + 0x24 + 4 * di)) - 8);
            DG16((uint16_t)(v06 + 2)) =
                (int16_t)(DG16((uint16_t)(si + 0x26 + 4 * di)) - 8);
            alloc_shape(v06, v0a, 1, 2, 0);
        }
    }

    if (DGU16((uint16_t)(si + 4)) == DGU16(v12))
        goto out;

    DGU16(v12) = DGU16((uint16_t)(si + 4));
    DGU16(v10) = DGU16((uint16_t)(DGU16(v12) + 0x5a
                                  + 2 * DG8((uint16_t)(si + 0x0b))));

    if (mode & 1) {
        DGU16(v0c) = (DGU16((uint16_t)(DGU16(v10) + 4)) == 7)
                     ? (uint16_t)(DGU16((uint16_t)(DGU16(v10) + 0x66)) + 0x28)
                     : (uint16_t)(si + 0x24);

        DG16(v02) = link_slack(DGU16(v10), si, 1);
        alloc_shape(DGU16(v0c), (uint16_t)(si + 0x28), 4, 1, DG16(v02));

        for (di = 0; di < 2; di++) {
            DG16(v06) = (int16_t)(DG16((uint16_t)(si + 0x24 + 4 * di)) - 8);
            DG16((uint16_t)(v06 + 2)) =
                (int16_t)(DG16((uint16_t)(si + 0x26 + 4 * di)) - 8);
            alloc_shape(v06, v0a, 1, 1, 0);
        }
    }

    if (mode & 2) {
        DGU16(v0c) = (DGU16((uint16_t)(DGU16(v12) + 4)) == 7)
                     ? (uint16_t)(DGU16((uint16_t)(DGU16(v10) + 0x66)) + 0x20)
                     : (uint16_t)(si + 0x1c);

        DG16(v02) = link_slack(DGU16(v10), si, 2);
        alloc_shape(DGU16(v0c), (uint16_t)(si + 0x20), 4, 2, DG16(v02));

        for (di = 0; di < 2; di++) {
            DG16(v06) = (int16_t)(DG16((uint16_t)(si + 0x24 + 4 * di)) - 8);
            DG16((uint16_t)(v06 + 2)) =
                (int16_t)(DG16((uint16_t)(si + 0x26 + 4 * di)) - 8);
            alloc_shape(v06, v0a, 1, 2, 0);
        }
    }

    goto out;

plain:
    if (mode & 1) {
        DGU16(v10) = DGU16((uint16_t)(si + 2));
        DGU16(v12) = DGU16((uint16_t)(DGU16(v10) + 0x5a
                                      + 2 * DG8((uint16_t)(si + 0x0a))));

        while (DGU16(v10) != 0 && DGU16(v12) != 0) {
            DGU16(v0c) = (DGU16((uint16_t)(DGU16(v10) + 4)) == 7)
                         ? (uint16_t)(DGU16((uint16_t)(DGU16(v10) + 0x66))
                                      + 0x28)
                         : (uint16_t)(si + 0x24);

            DGU16(v0e) = (DGU16((uint16_t)(DGU16(v12) + 4)) == 7)
                         ? (uint16_t)(DGU16((uint16_t)(DGU16(v12) + 0x66))
                                      + 0x24)
                         : (uint16_t)(si + 0x28);

            DG16(v02) = link_slack(DGU16(v10), si, 1);
            alloc_shape(DGU16(v0c), DGU16(v0e), 4, 1, DG16(v02));

            DGU16(v10) = DGU16(v12);
            if (DGU16((uint16_t)(DGU16(v10) + 4)) != 7)
                DGU16(v12) = 0;
            else
                DGU16(v12) = DGU16((uint16_t)(DGU16(v12) + 0x5a));
        }

        for (di = 0; di < 2; di++) {
            DG16(v06) = (int16_t)(DG16((uint16_t)(si + 0x24 + 4 * di)) - 8);
            DG16((uint16_t)(v06 + 2)) =
                (int16_t)(DG16((uint16_t)(si + 0x26 + 4 * di)) - 8);
            alloc_shape(v06, v0a, 1, 1, 0);
        }
    }

    if (mode & 2) {
        DGU16(v10) = DGU16((uint16_t)(si + 2));
        DGU16(v12) = DGU16((uint16_t)(DGU16(v10) + 0x5a
                                      + 2 * DG8((uint16_t)(si + 0x0a))));

        while (DGU16(v10) != 0 && DGU16(v12) != 0) {
            DGU16(v0c) = (DGU16((uint16_t)(DGU16(v10) + 4)) == 7)
                         ? (uint16_t)(DGU16((uint16_t)(DGU16(v10) + 0x66))
                                      + 0x20)
                         : (uint16_t)(si + 0x1c);

            DGU16(v0e) = (DGU16((uint16_t)(DGU16(v12) + 4)) == 7)
                         ? (uint16_t)(DGU16((uint16_t)(DGU16(v12) + 0x66))
                                      + 0x1c)
                         : (uint16_t)(si + 0x20);

            DG16(v02) = link_slack(DGU16(v10), si, 2);
            alloc_shape(DGU16(v0c), DGU16(v0e), 4, 2, DG16(v02));

            DGU16(v10) = DGU16(v12);
            if (DGU16((uint16_t)(DGU16(v10) + 4)) != 7)
                DGU16(v12) = 0;
            else
                DGU16(v12) = DGU16((uint16_t)(DGU16(v12) + 0x5a));
        }

        for (di = 0; di < 2; di++) {
            DG16(v06) = (int16_t)(DG16((uint16_t)(si + 0x24 + 4 * di)) - 8);
            DG16((uint16_t)(v06 + 2)) =
                (int16_t)(DG16((uint16_t)(si + 0x26 + 4 * di)) - 8);
            alloc_shape(v06, v0a, 1, 2, 0);
        }
    }

out:
    dg_leave(0x12);
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
 * 0x0647f
 *
 * Register a part's shapes, by kind: a rope, kind 8, through
 * `add_sub_object_shapes`; a belt, kind 0x0a, through `mark_belt_shapes`;
 * everything else through `add_record_shapes`. Three lines and a dispatch, and
 * fifteen callers.
 */
void mark_part_shapes(uint16_t part, uint16_t mode)
{
    if (DGU16((uint16_t)(part + 4)) == 8) {
        add_sub_object_shapes(part, (int16_t)mode);
        return;
    }

    if (DGU16((uint16_t)(part + 4)) == 0x0a) {
        mark_belt_shapes(part, mode);
        return;
    }

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
 * 0x06699
 *
 * Put back what was drawn over: walk the shape list at DGROUP 0x4e52, step each
 * record's life at +5, and act on the ones that have run out.
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
    uint16_t fp = dg_enter(0x12);
    uint16_t prev = (uint16_t)(fp + 0x00);   /* [bp-0x12], a far pointer */
    uint16_t next = (uint16_t)(fp + 0x04);   /* [bp-0x0e], a far pointer */
    uint16_t cur  = (uint16_t)(fp + 0x08);   /* [bp-0x0a], a far pointer */
    uint16_t c    = (uint16_t)(fp + 0x0c);   /* [bp-6] */
    uint16_t b    = (uint16_t)(fp + 0x0e);   /* [bp-4] */
    uint16_t a    = (uint16_t)(fp + 0x10);   /* [bp-2] */
    int16_t si, di;

    set_clip_for_mode();

    DG8(0x3893) = 1;
    DG8(0x389e) = DG8(0x52cb);
    DG8(0x389d) = DG8(0x52cb);
    DGU16(0x38a8) = DGU16(0x38a2);

    DGU16(prev) = 0;
    DGU16((uint16_t)(prev + 2)) = 0;

    DGU16((uint16_t)(next + 2)) = DGU16(0x4e54);
    DGU16(next) = DGU16(0x4e52);

    for (;;) {
        uint16_t cs, co;

        DGU16((uint16_t)(cur + 2)) = DGU16((uint16_t)(next + 2));
        DGU16(cur) = DGU16(next);

        if ((DGU16(cur) | DGU16((uint16_t)(cur + 2))) == 0)
            break;

        cs = DGU16((uint16_t)(cur + 2));
        co = DGU16(cur);

        DGU16((uint16_t)(next + 2)) = FARU16(cs, (uint16_t)(co + 2));
        DGU16(next) = FARU16(cs, co);

        FAR8(cs, (uint16_t)(co + 5)) =
            (uint8_t)(FAR8(cs, (uint16_t)(co + 5)) - 1);

        if (FAR8(cs, (uint16_t)(co + 5)) != 0) {
            DGU16((uint16_t)(prev + 2)) = cs;
            DGU16(prev) = co;
            continue;
        }

        si = FAR16(cs, (uint16_t)(co + 6));
        di = FAR16(cs, (uint16_t)(co + 8));
        DG16(a) = FAR16(cs, (uint16_t)(co + 0x0a));
        DG16(b) = FAR16(cs, (uint16_t)(co + 0x0c));
        DG16(c) = FAR16(cs, (uint16_t)(co + 0x0e));

        clear_flag_2d44_thunk();

        if (FAR8(cs, (uint16_t)(co + 4)) & 4) {
            draw_belt_segment(si, di, DG16(a), DG16(b), DG16(c));
        } else {
            DG8(0x389c) = (uint8_t)(FAR8(cs, (uint16_t)(co + 4)) & 1);

            if (di == DG16(0x389a))
                di--;
            if (si == DG16(0x3896))
                si--;

            if (si < DG16(0x3896)
                && (int16_t)(si + DG16(a)) > DG16(0x3894)
                && di < DG16(0x389a)
                && (int16_t)(di + DG16(b)) > DG16(0x3898))
                fill_rect(si, di, DG16(a), DG16(b));
        }

        restore_cursor_following();

        if ((DGU16(prev) | DGU16((uint16_t)(prev + 2))) != 0) {
            FARU16(DGU16((uint16_t)(prev + 2)),
                   (uint16_t)(DGU16(prev) + 2)) = DGU16((uint16_t)(next + 2));
            FARU16(DGU16((uint16_t)(prev + 2)), DGU16(prev)) = DGU16(next);
        } else {
            DGU16(0x4e54) = DGU16((uint16_t)(next + 2));
            DGU16(0x4e52) = DGU16(next);
        }

        FARU16(cs, (uint16_t)(co + 2)) = DGU16(0x4e50);
        FARU16(cs, co) = DGU16(0x4e4e);
        DGU16(0x4e50) = cs;
        DGU16(0x4e4e) = co;
    }

    dg_leave(0x12);
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
void belt_in_dirty_rect(uint16_t part)
{
    uint16_t fp = dg_enter(0x20);
    uint16_t node = (uint16_t)(fp + 0x00);  /* [bp-0x20], a far pointer */
    uint16_t belt = (uint16_t)(fp + 0x04);  /* [bp-0x1c] */
    uint16_t endB = (uint16_t)(fp + 0x06);  /* [bp-0x1a] */
    uint16_t endA = (uint16_t)(fp + 0x08);  /* [bp-0x18] */
    uint16_t slotB = (uint16_t)(fp + 0x0a); /* [bp-0x16] */
    uint16_t slotA = (uint16_t)(fp + 0x0c); /* [bp-0x14] */
    uint16_t slack = (uint16_t)(fp + 0x0e); /* [bp-0x12] */
    uint16_t bottom = (uint16_t)(fp + 0x10);/* [bp-0x10] */
    uint16_t right = (uint16_t)(fp + 0x12); /* [bp-0x0e] */
    uint16_t top = (uint16_t)(fp + 0x14);   /* [bp-0x0c] */
    uint16_t left = (uint16_t)(fp + 0x16);  /* [bp-0x0a] */
    uint16_t by = (uint16_t)(fp + 0x18);    /* [bp-8] */
    uint16_t bx = (uint16_t)(fp + 0x1a);    /* [bp-6] */
    uint16_t ay = (uint16_t)(fp + 0x1c);    /* [bp-4] */
    uint16_t ax = (uint16_t)(fp + 0x1e);    /* [bp-2] */
    uint16_t di, si;

    DGU16(belt) = DGU16((uint16_t)(part + 0x66));
    DGU16(endA) = DGU16((uint16_t)(DGU16(belt) + 2));
    di = DGU16(endA);
    DGU16(endB) = DGU16((uint16_t)(DGU16(belt) + 4));

    DG16(slotA) = (int16_t)DG8((uint16_t)(DGU16(belt) + 0x0a));
    DG16(slotB) = 0;

    si = DGU16((uint16_t)(di + 0x5a + 2 * DGU16(slotA)));
    DG16(slack) = link_slack(di, DGU16(belt), 3);

    while (di != 0 && si != 0) {
        if (di != DGU16(endA)) {
            DG16(slotA) = 1;
            DG16(slack) = 0;
        }

        DG16(ax) = (int16_t)(DG16((uint16_t)(di + 0x2a))
                             + DG8((uint16_t)(di + 0x6a + 2 * DGU16(slotA))));
        DG16(ay) = (int16_t)(DG16((uint16_t)(di + 0x2c))
                             + DG8((uint16_t)(di + 0x6b + 2 * DGU16(slotA))));

        if (si == DGU16(endB)) {
            DG16(slotB) = (int16_t)DG8((uint16_t)(DGU16(belt) + 0x0b));
            DG16(slack) = link_slack(di, DGU16(belt), 3);
        }

        DG16(bx) = (int16_t)(DG16((uint16_t)(si + 0x2a))
                             + DG8((uint16_t)(si + 0x6a + 2 * DGU16(slotB))));
        DG16(by) = (int16_t)(DG16((uint16_t)(si + 0x2c))
                             + DG8((uint16_t)(si + 0x6b + 2 * DGU16(slotB))));

        if (DG16(ax) < DG16(bx)) {
            DG16(left) = (int16_t)(DG16(ax) - DG16(0x4ea3));
            DG16(right) = (int16_t)(DG16(bx) - DG16(0x4ea3));
        } else {
            DG16(left) = (int16_t)(DG16(bx) - DG16(0x4ea3));
            DG16(right) = (int16_t)(DG16(ax) - DG16(0x4ea3));
        }

        if (DG16(ay) < DG16(by)) {
            DG16(top) = (int16_t)(DG16(ay) - DG16(0x4ea1));
            DG16(bottom) = (int16_t)(DG16(by) - DG16(0x4ea1));
        } else {
            DG16(top) = (int16_t)(DG16(by) - DG16(0x4ea1));
            DG16(bottom) = (int16_t)(DG16(ay) - DG16(0x4ea1));
        }

        if (DG16(slack) > 0)
            DG16(bottom) = (int16_t)(DG16(bottom) + (DG16(slack) >> 1));

        DGU16((uint16_t)(node + 2)) = DGU16(0x4e54);
        DGU16(node) = DGU16(0x4e52);

        while ((DGU16(node) | DGU16((uint16_t)(node + 2))) != 0) {
            uint8_t *p = FAR_PTR(DGU16((uint16_t)(node + 2)), DGU16(node));

            if ((int16_t)(p[0x10] | (p[0x11] << 8)) < DG16(right)
                && (int16_t)(p[0x12] | (p[0x13] << 8)) > DG16(left)
                && (int16_t)(p[0x14] | (p[0x15] << 8)) < DG16(bottom)
                && (int16_t)(p[0x16] | (p[0x17] << 8)) > DG16(top)) {

                mark_needs_refile(part, 1);
                DGU16((uint16_t)(node + 2)) = 0;
                DGU16(node) = 0;
                si = DGU16(endB);
                break;
            }

            DGU16((uint16_t)(node + 2)) = (uint16_t)(p[2] | (p[3] << 8));
            DGU16(node) = (uint16_t)(p[0] | (p[1] << 8));
        }

        if (si == DGU16(endB)) {
            si = 0;
            di = 0;
        } else {
            di = si;
            si = DGU16((uint16_t)(si + 0x5a));
        }
    }

    dg_leave(0x20);
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
    uint16_t fp = dg_enter(0x0c);
    uint16_t node = (uint16_t)(fp + 0x00);  /* [bp-0x0c], a far pointer */
    uint16_t bottom = (uint16_t)(fp + 0x04);/* [bp-8] */
    uint16_t right = (uint16_t)(fp + 0x06); /* [bp-6] */
    uint16_t top = (uint16_t)(fp + 0x08);   /* [bp-4] */
    uint16_t left = (uint16_t)(fp + 0x0a);  /* [bp-2] */
    uint16_t di, si;

    for (di = (uint16_t)pick_by_flag(0x3000); di != 0;
         di = (uint16_t)pick_for_record(di, 0x1000)) {

        if (DG8((uint16_t)(di + 0x14)) != 0)
            continue;
        if (DGU16((uint16_t)(di + 8)) & 0x2000)
            continue;

        if (DGU16((uint16_t)(di + 4)) == 0x0a) {
            belt_in_dirty_rect(di);
            continue;
        }

        if (DGU16((uint16_t)(di + 4)) == 8) {
            int16_t span;

            si = DGU16((uint16_t)(di + 0x54));

            if (rope_ends_close(si) == 0)
                continue;

            if (DG16(0x4e69) == 9
                && (DGU16((uint16_t)(si + 4)) == DGU16(0x50d5)
                    || DGU16((uint16_t)(si + 6)) == DGU16(0x50d5))
                && point_in_play_area() == 0)
                continue;

            if (DG16((uint16_t)(si + 8)) < DG16((uint16_t)(si + 0x0c))) {
                DG16(left) = (int16_t)(DG16((uint16_t)(si + 8))
                                       - DG16(0x4ea3));
                DG16(right) = DG16(left);
                span = (int16_t)(DG16((uint16_t)(si + 0x14))
                                 - DG16((uint16_t)(si + 8)));
            } else {
                DG16(left) = (int16_t)(DG16((uint16_t)(si + 0x0c))
                                       - DG16(0x4ea3));
                DG16(right) = DG16(left);
                span = (int16_t)(DG16((uint16_t)(si + 0x10))
                                 - DG16((uint16_t)(si + 0x0c)));
            }
            DG16(right) = (int16_t)(DG16(right) + span);

            if (DG16((uint16_t)(si + 0x0a)) < DG16((uint16_t)(si + 0x0e))) {
                DG16(top) = (int16_t)(DG16((uint16_t)(si + 0x0a))
                                      - DG16(0x4ea3));
                DG16(bottom) = DG16(top);
                span = (int16_t)(DG16((uint16_t)(si + 0x16))
                                 - DG16((uint16_t)(si + 0x0a)));
            } else {
                DG16(top) = (int16_t)(DG16((uint16_t)(si + 0x0e))
                                      - DG16(0x4ea3));
                DG16(bottom) = DG16(top);
                span = (int16_t)(DG16((uint16_t)(si + 0x12))
                                 - DG16((uint16_t)(si + 0x0e)));
            }
            DG16(bottom) = (int16_t)(DG16(bottom) + span);
        } else {
            DG16(left) = (int16_t)(DG16((uint16_t)(di + 0x2a))
                                   - DG16(0x4ea3));
            DG16(top) = (int16_t)(DG16((uint16_t)(di + 0x2c))
                                  - DG16(0x4ea1));
            DG16(right) = (int16_t)(DG16(left)
                                    + DG16((uint16_t)(di + 0x44)));
            DG16(bottom) = (int16_t)(DG16(top)
                                     + DG16((uint16_t)(di + 0x46)));
        }

        DGU16((uint16_t)(node + 2)) = DGU16(0x4e54);
        DGU16(node) = DGU16(0x4e52);

        while ((DGU16(node) | DGU16((uint16_t)(node + 2))) != 0) {
            uint8_t *p = FAR_PTR(DGU16((uint16_t)(node + 2)), DGU16(node));

            if ((int16_t)(p[0x10] | (p[0x11] << 8)) < DG16(right)
                && (int16_t)(p[0x12] | (p[0x13] << 8)) > DG16(left)
                && (int16_t)(p[0x14] | (p[0x15] << 8)) < DG16(bottom)
                && (int16_t)(p[0x16] | (p[0x17] << 8)) > DG16(top)) {

                mark_needs_refile(di, 1);
                DGU16((uint16_t)(node + 2)) = 0;
                DGU16(node) = 0;
                break;
            }

            DGU16((uint16_t)(node + 2)) = (uint16_t)(p[2] | (p[3] << 8));
            DGU16(node) = (uint16_t)(p[0] | (p[1] << 8));
        }
    }

    dg_leave(0x0c);
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
    uint16_t fp = dg_enter(0x16);
    uint16_t v16 = (uint16_t)(fp + 0x00);   /* [bp-0x16] the kind's record */
    uint16_t v14 = (uint16_t)(fp + 0x02);   /* [bp-0x14] the part walked to */
    uint16_t v12 = (uint16_t)(fp + 0x04);   /* [bp-0x12] */
    uint16_t v10 = (uint16_t)(fp + 0x06);   /* [bp-0x10] */
    uint16_t v0e = (uint16_t)(fp + 0x08);   /* [bp-0x0e] */
    uint16_t v0c = (uint16_t)(fp + 0x0a);   /* [bp-0x0c] */
    uint16_t v0a = (uint16_t)(fp + 0x0c);   /* [bp-0x0a] */
    uint16_t v08 = (uint16_t)(fp + 0x0e);   /* [bp-8] */
    uint16_t v06 = (uint16_t)(fp + 0x10);   /* [bp-6] */
    uint16_t v04 = (uint16_t)(fp + 0x12);   /* [bp-4] */
    uint16_t v02 = (uint16_t)(fp + 0x14);   /* [bp-2] the level */
    uint16_t v01 = (uint16_t)(fp + 0x15);   /* [bp-1] the counter */
    uint16_t di, si;

    for (DG8(v01) = 6; DG8(v01) != 0; DG8(v01)--) {
        DG8(v02) = (uint8_t)(DG8(v01) - 1);

        DGU16(v14) = DGU16((uint16_t)(0x50bf + 2 * DG8(v02)));

        while (DGU16(v14) != 0) {
            DGU16(v16) = (uint16_t)(0x0ea6
                                    + 0x3a * (int16_t)DG16(
                                        (uint16_t)(DGU16(v14) + 4)));

            if (!(DG8((uint16_t)(DGU16(v16) + 0x1c)) == 0xff
                  || DG8((uint16_t)(DGU16(v16) + 0x1c)) >= DG8(v02)
                  || DG8((uint16_t)(DGU16(v16) + 0x1c)) <= 2))
                goto next;

            if (!(DG8((uint16_t)(DGU16(v16) + 0x1c)) == 0xff
                  || DG8((uint16_t)(DGU16(v16) + 0x1d)) >= DG8(v02)
                  || DG8((uint16_t)(DGU16(v16) + 0x1d)) <= 2))
                goto next;

            DGU16(v04) = DGU16((uint16_t)(DGU16(v14) + 0x2a));
            DGU16(v06) = DGU16((uint16_t)(DGU16(v14) + 0x2c));
            DGU16(v08) = (uint16_t)(DGU16(v04)
                                    + DGU16((uint16_t)(DGU16(v14) + 0x44)));
            DGU16(v0a) = (uint16_t)(DGU16(v06)
                                    + DGU16((uint16_t)(DGU16(v14) + 0x46)));

            for (di = (uint16_t)pick_by_flag(0x3000); di != 0;
                 di = (uint16_t)pick_for_record(di, 0x1000)) {

                if ((DGU16((uint16_t)(di + 0x0a)) & 0x20) != 0
                    || (DGU16((uint16_t)(di + 8)) & 0x2000) != 0)
                    continue;

                if (DGU16((uint16_t)(di + 4)) == 0x0a
                    || DGU16((uint16_t)(di + 4)) == 0x31)
                    continue;

                DGU16(v16) = (uint16_t)(0x0ea6
                                        + 0x3a * (int16_t)DG16(
                                            (uint16_t)(di + 4)));

                if (DG8((uint16_t)(DGU16(v16) + 0x1c)) > DG8(v02)
                    && DG8((uint16_t)(DGU16(v16) + 0x1c)) != 0xff)
                    continue;
                if (DG8((uint16_t)(DGU16(v16) + 0x1d)) > DG8(v02)
                    && DG8((uint16_t)(DGU16(v16) + 0x1d)) != 0xff)
                    continue;

                if (DGU16((uint16_t)(di + 4)) == 8) {
                    int16_t span;

                    si = DGU16((uint16_t)(di + 0x54));

                    if (rope_ends_close(si) == 0)
                        continue;

                    if (DG16(0x4e69) == 9
                        && (DGU16((uint16_t)(si + 4)) == DGU16(0x50d5)
                            || DGU16((uint16_t)(si + 6)) == DGU16(0x50d5))
                        && point_in_play_area() == 0)
                        continue;

                    if (DG16((uint16_t)(si + 8)) < DG16((uint16_t)(si + 0x0c))) {
                        DGU16(v0c) = DGU16((uint16_t)(si + 8));
                        DGU16(v10) = DGU16((uint16_t)(si + 8));
                        span = (int16_t)(DG16((uint16_t)(si + 0x14))
                                         - DG16((uint16_t)(si + 8)));
                    } else {
                        DGU16(v0c) = DGU16((uint16_t)(si + 0x0c));
                        DGU16(v10) = DGU16((uint16_t)(si + 0x0c));
                        span = (int16_t)(DG16((uint16_t)(si + 0x10))
                                         - DG16((uint16_t)(si + 0x0c)));
                    }
                    DGU16(v10) = (uint16_t)(DGU16(v10) + span);

                    if (DG16((uint16_t)(si + 0x0a)) < DG16((uint16_t)(si + 0x0e))) {
                        DGU16(v0e) = DGU16((uint16_t)(si + 0x0a));
                        DGU16(v12) = DGU16((uint16_t)(si + 0x0a));
                        span = (int16_t)(DG16((uint16_t)(si + 0x16))
                                         - DG16((uint16_t)(si + 0x0a)));
                    } else {
                        DGU16(v0e) = DGU16((uint16_t)(si + 0x0e));
                        DGU16(v12) = DGU16((uint16_t)(si + 0x0e));
                        span = (int16_t)(DG16((uint16_t)(si + 0x12))
                                         - DG16((uint16_t)(si + 0x0e)));
                    }
                    DGU16(v12) = (uint16_t)(DGU16(v12) + span);
                } else {
                    DGU16(v0c) = DGU16((uint16_t)(di + 0x2a));
                    DGU16(v0e) = DGU16((uint16_t)(di + 0x2c));
                    DGU16(v10) = (uint16_t)(DGU16(v0c)
                                            + DGU16((uint16_t)(di + 0x44)));
                    DGU16(v12) = (uint16_t)(DGU16(v0e)
                                            + DGU16((uint16_t)(di + 0x46)));
                }

                if (DG16(v0c) >= DG16(v08))
                    continue;
                if (DG16(v10) <= DG16(v04))
                    continue;
                if (DG16(v0e) >= DG16(v0a))
                    continue;
                if (DG16(v12) <= DG16(v06))
                    continue;

                link_record_into_buckets(di);
            }

        next:
            if (DG8((uint16_t)(DGU16(v14) + 0x7f)) == DG8(v02))
                DGU16(v14) = DGU16((uint16_t)(DGU16(v14) + 0x74));
            else
                DGU16(v14) = DGU16((uint16_t)(DGU16(v14) + 0x76));
        }
    }

    dg_leave(0x16);
}

/*
 * 0x06dbf
 *
 * The part at the other end of a part's rope: the rope record at +0x54 names
 * both ends at +4 and +6, and this answers whichever is not the one asked
 * about. A part with no rope answers 0, and so does one whose rope names it at
 * neither end - the `xor ax, ax` is reached from both.
 */
uint16_t rope_other_end(uint16_t part)
{
    uint16_t si = DGU16((uint16_t)(part + 0x54));

    if (si == 0)
        return 0;

    if (DGU16((uint16_t)(si + 4)) == part)
        return DGU16((uint16_t)(si + 6));

    return DGU16((uint16_t)(si + 4));
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
int16_t belt_orientation(uint16_t belt, int16_t which, int16_t dir)
{
    uint16_t fp = dg_enter(0x16);
    uint16_t v16 = (uint16_t)(fp + 0x00);   /* [bp-0x16] beyond the far end */
    uint16_t v14 = (uint16_t)(fp + 0x02);   /* [bp-0x14] beyond the near end */
    uint16_t v12 = (uint16_t)(fp + 0x04);   /* [bp-0x12] the far part */
    uint16_t v10 = (uint16_t)(fp + 0x06);   /* [bp-0x10] the near part */
    uint16_t v0e = (uint16_t)(fp + 0x08);   /* [bp-0x0e] the far record */
    uint16_t v0c = (uint16_t)(fp + 0x0a);   /* [bp-0x0c] the near record */
    uint16_t v0a = (uint16_t)(fp + 0x0c);   /* [bp-0x0a] the first bit */
    uint16_t v08 = (uint16_t)(fp + 0x0e);   /* [bp-8]  the far index */
    uint16_t v06 = (uint16_t)(fp + 0x10);   /* [bp-6]  the near index */
    uint16_t v04 = (uint16_t)(fp + 0x12);   /* [bp-4]  the far slot */
    uint16_t v02 = (uint16_t)(fp + 0x14);   /* [bp-2]  the near slot */
    uint16_t si = belt;
    int16_t cx = which;
    int16_t di = (int16_t)(1 - cx);
    int16_t answer;

    if (cx != 0) {
        DGU16(v10) = DGU16((uint16_t)(si + 4));
        DG16(v02) = (int16_t)DG8((uint16_t)(si + 0x0b));
        DGU16(v12) = DGU16((uint16_t)(si + 2));
        DG16(v04) = (int16_t)DG8((uint16_t)(si + 0x0a));
    } else {
        DGU16(v10) = DGU16((uint16_t)(si + 2));
        DG16(v02) = (int16_t)DG8((uint16_t)(si + 0x0a));
        DGU16(v12) = DGU16((uint16_t)(si + 4));
        DG16(v04) = (int16_t)DG8((uint16_t)(si + 0x0b));
    }

    DGU16(v14) = DGU16((uint16_t)(DGU16(v10) + 0x5a + 2 * DGU16(v02)));
    DGU16(v16) = DGU16((uint16_t)(DGU16(v12) + 0x5a + 2 * DGU16(v04)));

    if (DGU16(v12) == DGU16(v14)) {
        DGU16(v0e) = si;
        DGU16(v0c) = si;
        DG16(v06) = di;
        DG16(v08) = cx;
    } else {
        DGU16(v0c) = DGU16((uint16_t)(DGU16(v14) + 0x66));
        DGU16(v0e) = DGU16((uint16_t)(DGU16(v16) + 0x66));
        DG16(v06) = (int16_t)(1 - cx);
        DG16(v08) = (int16_t)(1 - di);
    }

    if (DG16((uint16_t)(si + 0x14 + 4 * di))
        > DG16((uint16_t)(DGU16(v0e) + 0x14 + 4 * DGU16(v08))))
        DG16(v0a) = 8;
    else
        DG16(v0a) = 0x10;

    if (dir == 0) {
        if (DG16((uint16_t)(si + 0x16 + 4 * cx))
            > DG16((uint16_t)(DGU16(v0c) + 0x16 + 4 * DGU16(v06)))) {
            answer = 1;
            goto out;
        }
        answer = (DG16((uint16_t)(si + 0x16 + 4 * di))
                  > DG16((uint16_t)(DGU16(v0e) + 0x16 + 4 * DGU16(v08))))
                 ? 2 : 4;
    } else {
        if (DG16((uint16_t)(si + 0x16 + 4 * cx))
            < DG16((uint16_t)(DGU16(v0c) + 0x16 + 4 * DGU16(v06)))) {
            answer = 1;
            goto out;
        }
        /*
         * `jge`, not `jl`. The two halves are **not** mirror images: with the
         * direction set the first test is `<` and the second `>=`, where with
         * it clear both are `>`. Reading the second as the first one flipped
         * gives 2 where the original gives 4, so the answer loses bit 2 - and
         * bit 2 is what `tension_belt` reads to decide which way a lever is
         * driven. The lever then never turns, and on the credits screen the
         * gun it is tied to never fires.
         */
        answer = (DG16((uint16_t)(si + 0x16 + 4 * di))
                  >= DG16((uint16_t)(DGU16(v0e) + 0x16 + 4 * DGU16(v08))))
                 ? 2 : 4;
    }

    answer = (int16_t)(answer | DG16(v0a));

out:
    dg_leave(0x16);
    return answer;
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
int32_t dev_tension_belt_calls;          /* ours: see reconstruct/devdump.c */

int16_t tension_belt(uint16_t part)
{
    dev_tension_belt_calls++;

    uint16_t fp = dg_enter(0x3a);
    uint16_t belt   = (uint16_t)(fp + 0x00);   /* [bp-0x3a] */
    uint16_t pC     = (uint16_t)(fp + 0x02);   /* [bp-0x38] */
    uint16_t pB     = (uint16_t)(fp + 0x04);   /* [bp-0x36] */
    uint16_t other  = (uint16_t)(fp + 0x06);   /* [bp-0x34] */
    uint16_t plo    = (uint16_t)(fp + 0x08);   /* [bp-0x32] */
    uint16_t phi    = (uint16_t)(fp + 0x0a);   /* [bp-0x30] */
    uint16_t pulley = (uint16_t)(fp + 0x0c);   /* [bp-0x2e] */
    uint16_t give   = (uint16_t)(fp + 0x0e);   /* [bp-0x2c] */
    uint16_t saved  = (uint16_t)(fp + 0x10);   /* [bp-0x2a] */
    uint16_t dir    = (uint16_t)(fp + 0x12);   /* [bp-0x28] */
    uint16_t answer = (uint16_t)(fp + 0x14);   /* [bp-0x26] */
    uint16_t moving = (uint16_t)(fp + 0x16);   /* [bp-0x24] */
    uint16_t orient = (uint16_t)(fp + 0x18);   /* [bp-0x22] */
    uint16_t slot   = (uint16_t)(fp + 0x1a);   /* [bp-0x20] */
    uint16_t end    = (uint16_t)(fp + 0x1c);   /* [bp-0x1e] */
    uint16_t dy2    = (uint16_t)(fp + 0x1e);   /* [bp-0x1c] */
    uint16_t dx2    = (uint16_t)(fp + 0x20);   /* [bp-0x1a] */
    uint16_t ny     = (uint16_t)(fp + 0x22);   /* [bp-0x18] */
    uint16_t nx     = (uint16_t)(fp + 0x24);   /* [bp-0x16] */
    uint16_t dy1    = (uint16_t)(fp + 0x26);   /* [bp-0x14] */
    uint16_t dx1    = (uint16_t)(fp + 0x28);   /* [bp-0x12] */
    uint16_t i      = (uint16_t)(fp + 0x2a);   /* [bp-0x10] */
    uint16_t k      = (uint16_t)(fp + 0x2c);   /* [bp-0x0e] */
    uint16_t gapB   = (uint16_t)(fp + 0x2e);   /* [bp-0x0c] */
    uint16_t slackB = (uint16_t)(fp + 0x30);   /* [bp-0x0a] */
    uint16_t dB     = (uint16_t)(fp + 0x32);   /* [bp-8] */
    uint16_t dA     = (uint16_t)(fp + 0x34);   /* [bp-6] */
    uint16_t gapA   = (uint16_t)(fp + 0x36);   /* [bp-4] */
    uint16_t slackA = (uint16_t)(fp + 0x38);   /* [bp-2] */
    uint16_t si = part;
    uint16_t di;
    int16_t t;

    DG16(answer) = 0;

    DG16(pulley) =
        (DGU16((uint16_t)(DGU16((uint16_t)(si + 0x5a)) + 4)) == 7) ? 1 : 0;

    if (DG16((uint16_t)(si + 0x20)) < DG16((uint16_t)(si + 0x24)))
        DG16(moving) = 0;
    else if (DG16((uint16_t)(si + 0x20)) > DG16((uint16_t)(si + 0x24)))
        DG16(moving) = 1;
    else
        DG16(moving) = -1;

    DGU16(belt) = DGU16((uint16_t)(si + 0x66));
    di = DGU16(DGU16(belt));
    DGU16(other) = (uint16_t)select_field_2_or_4((int16_t)si, DGU16(belt));

    if (DGU16((uint16_t)(DGU16(belt) + 2)) == si) {
        DG16(end) = 0;
        DG16(slot) = (int16_t)DG8((uint16_t)(DGU16(belt) + 0x0b));
        DG16(slackA) = DG16((uint16_t)(di + 0x96));
        DG16(slackB) = DG16((uint16_t)(di + 0x9c));
    } else {
        DG16(end) = 1;
        DG16(slot) = (int16_t)DG8((uint16_t)(DGU16(belt) + 0x0a));
        DG16(slackA) = DG16((uint16_t)(di + 0x9c));
        DG16(slackB) = DG16((uint16_t)(di + 0x96));
    }

    DG16(gapB) = link_endpoint_gap(DGU16(belt), DGU16(other), dx2, dy2);
    DG16(gapA) = link_endpoint_gap(DGU16(belt), si, dx1, dy1);

    DG16(dA) = (int16_t)(DG16(gapA) - DG16(slackA));

    if (DGU16((uint16_t)(si + 4)) != 0x31) {
        DG16(dB) = (int16_t)(DG16(gapB) - DG16(slackB));

        if (DG16(dA) > 0 && DG16(dB) < 0) {
            DG16(dA) = (int16_t)(DG16(dA) + DG16(dB));

            if (DG16(dA) > 0) {
                DG16(dB) = 0;
            } else {
                DG16(dB) = DG16(dA);
                DG16(dA) = 0;
            }

            if (DGU16((uint16_t)(DGU16(belt) + 2)) == si) {
                DG16(slackA) = (int16_t)(DG16(gapA) - DG16(dA));
                DG16((uint16_t)(di + 0x96)) = DG16(slackA);
                DG16(slackB) = (int16_t)(DG16(gapB) - DG16(dB));
                DG16((uint16_t)(di + 0x9c)) = DG16(slackB);
            } else {
                DG16(slackA) = (int16_t)(DG16(gapA) - DG16(dA));
                DG16((uint16_t)(di + 0x9c)) = DG16(slackA);
                DG16(slackB) = (int16_t)(DG16(gapB) - DG16(dB));
                DG16((uint16_t)(di + 0x96)) = DG16(slackB);
            }
        }
    }

    if (DG16(dA) <= 0)
        goto stretched;
    if (!(DGU16((uint16_t)(DGU16(other) + 6)) & 0x1000))
        goto stretched;
    if (DGU16((uint16_t)(DGU16(other) + 4)) == 0x31)
        goto stretched;
    if (DGU16((uint16_t)(si + 4)) == 0x31)
        goto stretched;
    if (DG16((uint16_t)(si + 0x3a))
        <= DG16((uint16_t)(DGU16(other) + 0x3a)))
        goto stretched;

    {
        int32_t p;
        int16_t m = DG16((uint16_t)(si + 0x3a));

        t = DG16(dA);
        if (t < 0)
            t = (int16_t)-t;

        p = (int32_t)mul16x16(t, (int16_t)(m - DG16((uint16_t)(DGU16(other)
                                                               + 0x3a))));
        DG16(phi) = (int16_t)(p >> 16);
        DG16(plo) = (int16_t)p;

        DG16(give) = (int16_t)long_divide(
            (((int32_t)(uint16_t)DG16(phi) << 16) | (uint16_t)DG16(plo)) + m,
            (int32_t)m);
    }

    t = DG16(slackB);
    if (t < 0)
        t = (int16_t)-t;

    if (t > (DG16(give) > 1 ? DG16(give) : 1))
        DG16(give) = (DG16(give) > 1) ? DG16(give) : 1;
    else
        DG16(give) = t;

    if (DG16(give) == 0)
        goto stretched;

    if (DGU16((uint16_t)(DGU16(belt) + 2)) == si) {
        DG16((uint16_t)(di + 0x9c)) -= DG16(give);
        DG16(slackB) = DG16((uint16_t)(di + 0x9c));

        tension_belt(DGU16(other));
        DGU16((uint16_t)(DGU16(other) + 6)) &= 0xfff0;
        resolve_collisions(DGU16(other));

        DG16(gapB) = link_endpoint_gap(DGU16(belt), DGU16(other), dx2, dy2);
        DG16(dB) = (int16_t)(DG16(gapB) - DG16(slackB));

        if (DG16(dB) != 0) {
            DG16((uint16_t)(di + 0x9c)) += DG16(dB);
            DG16(give) -= DG16(dB);
        }

        if (DG16(give) != 0) {
            DG16((uint16_t)(di + 0x96)) += DG16(give);
            DG16(slackA) = DG16((uint16_t)(di + 0x96));
            DG16(dA) = (int16_t)(DG16(gapA) - DG16(slackA));
        }
    } else {
        DG16((uint16_t)(di + 0x96)) -= DG16(give);
        DG16(slackB) = DG16((uint16_t)(di + 0x96));

        tension_belt(DGU16(other));
        DGU16((uint16_t)(DGU16(other) + 6)) &= 0xfff0;
        resolve_collisions(DGU16(other));

        DG16(gapB) = link_endpoint_gap(DGU16(belt), DGU16(other), dx2, dy2);
        DG16(dB) = (int16_t)(DG16(gapB) - DG16(slackB));

        if (DG16(dB) != 0) {
            DG16((uint16_t)(di + 0x96)) += DG16(dB);
            DG16(give) -= DG16(dB);
        }

        if (DG16(give) != 0) {
            DG16((uint16_t)(di + 0x9c)) += DG16(give);
            DG16(slackA) = DG16((uint16_t)(di + 0x9c));
            DG16(dA) = (int16_t)(DG16(gapA) - DG16(slackA));
        }
    }

stretched:
    if (DG16(dA) <= 0)
        goto out;

    if (DGU16((uint16_t)(DGU16(other) + 4)) != 0x31
        || DGU16((uint16_t)(si + 4)) == 0x31)
        goto move;

    /* The far end is an anchor: take the belt off the pulley instead. */
    if (DG16(pulley) == 0)
        goto out;

    if (DGU16((uint16_t)(DGU16(belt) + 2)) == DGU16(other)) {
        DG16((uint16_t)(di + 0x96)) -= DG16(dA);

        if (DG16((uint16_t)(di + 0x96)) < 0) {
            DG16(dA) = (int16_t)(DG16(dA) + DG16((uint16_t)(di + 0x96)));

            DG16(saved) = DG16(0x4e6b);
            DG16(0x4e6b) = 0x1000;
            mark_belt_shapes(di, 3);
            DG16(0x4e6b) = DG16(saved);

            DGU16(pB) = DGU16((uint16_t)(
                DGU16(other) + 0x5a
                + 2 * DG8((uint16_t)(DGU16(belt) + 0x0a))));
            DGU16(pC) = DGU16((uint16_t)(DGU16(pB) + 0x5a));

            DG16(k) = match_field_5a_5c((int16_t)DGU16(pB), DGU16(pC));

            DGU16((uint16_t)(DGU16(other) + 0x5a
                             + 2 * DG8((uint16_t)(DGU16(belt) + 0x0a)))) =
                DGU16(pC);
            DGU16((uint16_t)(DGU16(pC) + 0x5a + 2 * DGU16(k))) = DGU16(other);

            for (DG16(i) = 0; DG16(i) < 2; DG16(i)++)
                DGU16((uint16_t)(DGU16(pB) + 0x5a + 2 * DGU16(i))) = 0;

            DG16((uint16_t)(DGU16(DGU16(belt)) + 0x96)) =
                link_end_distance(DGU16(belt), 3, 0);
        }

        DG16((uint16_t)(di + 0x9c)) += DG16(dA);
    } else {
        DG16((uint16_t)(di + 0x9c)) -= DG16(dA);

        if (DG16((uint16_t)(di + 0x9c)) < 0) {
            DG16(dA) = (int16_t)(DG16(dA) + DG16((uint16_t)(di + 0x9c)));

            DG16(saved) = DG16(0x4e6b);
            DG16(0x4e6b) = 0x1000;
            mark_belt_shapes(di, 3);
            DG16(0x4e6b) = DG16(saved);

            DGU16(pB) = DGU16((uint16_t)(
                DGU16(other) + 0x5a
                + 2 * DG8((uint16_t)(DGU16(belt) + 0x0b))));
            DGU16(pC) = DGU16((uint16_t)(DGU16(pB) + 0x5c));

            DG16(k) = match_field_5a_5c((int16_t)DGU16(pB), DGU16(pC));

            DGU16((uint16_t)(DGU16(other) + 0x5a
                             + 2 * DG8((uint16_t)(DGU16(belt) + 0x0b)))) =
                DGU16(pC);
            DGU16((uint16_t)(DGU16(pC) + 0x5a + 2 * DGU16(k))) = DGU16(other);

            for (DG16(i) = 0; DG16(i) < 2; DG16(i)++)
                DGU16((uint16_t)(DGU16(pB) + 0x5a + 2 * DGU16(i))) = 0;

            DG16((uint16_t)(DGU16(DGU16(belt)) + 0x9c)) =
                link_end_distance(DGU16(belt), 3, 1);
        }

        DG16((uint16_t)(di + 0x96)) += DG16(dA);
    }

    goto out;

move:
    DG16(answer) = 1;

    DG16(nx) = (int16_t)long_divide(
        (int32_t)mul16x16(DG16(dx1), DG16(slackA)), (int32_t)DG16(gapA));
    DG16((uint16_t)(si + 0x1e)) += (int16_t)(DG16(nx) - DG16(dx1));
    DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
    DG32((uint16_t)(si + 0x16)) =
        (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x16)), 9);

    DG16(ny) = (int16_t)long_divide(
        (int32_t)mul16x16(DG16(dy1), DG16(slackA)), (int32_t)DG16(gapA));
    DG16((uint16_t)(si + 0x20)) += (int16_t)(DG16(ny) - DG16(dy1));
    DG32((uint16_t)(si + 0x1a)) = DG16((uint16_t)(si + 0x20));
    DG32((uint16_t)(si + 0x1a)) =
        (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x1a)), 9);

    place_object_for_draw(si);
    update_velocity(si, 0, 0, 1);

    if (DG16((uint16_t)(si + 0x1e)) == DG16((uint16_t)(si + 0x22)))
        DGU16((uint16_t)(si + 0x38)) = 0;

    if (DGU16((uint16_t)(si + 4)) == 0x31)
        goto out;
    if (DG16(moving) == -1)
        goto out;

    DG16(orient) = belt_orientation(DGU16(belt), DG16(end),
                                    DG16(moving) == 0 ? 0 : 1);

    if (DGU16((uint16_t)(DGU16(other) + 4)) == 3) {
        DG16(dir) = 0;

        if (DG16(orient) & 4) {
            if (DG16(slot) == 0) {
                if (DG16((uint16_t)(DGU16(other) + 0x0c)) > 0)
                    DG16(dir) = -1;
            } else {
                if (DG16((uint16_t)(DGU16(other) + 0x0c)) < 2)
                    DG16(dir) = 1;
            }
        } else {
            if (DG16(slot) == 0) {
                if (DG16((uint16_t)(DGU16(other) + 0x0c)) < 2)
                    DG16(dir) = 1;
            } else {
                if (DG16((uint16_t)(DGU16(other) + 0x0c)) > 0)
                    DG16(dir) = -1;
            }
        }

        if (queue_part(si, DGU16(other)) != 0) {
            DG16((uint16_t)(DGU16(other) + 0x12)) = DG16(dir);
            DGU16((uint16_t)(DGU16(other) + 0x3e)) =
                DGU16((uint16_t)(si + 0x3e));
            DGU16((uint16_t)(DGU16(other) + 0x3c)) =
                DGU16((uint16_t)(si + 0x3c));
        }
    } else {
        part_drive(DGU16(other), si, DGU16(other), 0, DGU16(orient),
                   DGU16((uint16_t)(0x0ea8
                                    + 0x3a * (int16_t)DG16((uint16_t)(si + 4)))),
                   DGU16((uint16_t)(si + 0x3c)),
                   DGU16((uint16_t)(si + 0x3e)));
    }

out:
    {
        int16_t r = DG16(answer);

        dg_leave(0x3a);
        return r;
    }
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
int32_t dev_queue_part_calls;            /* ours: see reconstruct/devdump.c */

int16_t queue_part(uint16_t src, uint16_t part)
{
    dev_queue_part_calls++;

    uint16_t fp = dg_enter(4);
    uint16_t lo = fp;                       /* [bp-4] */
    uint16_t hi = (uint16_t)(fp + 2);       /* [bp-2] */
    uint16_t si, di;
    int16_t answer;

    DGU16(hi) = DGU16((uint16_t)(src + 0x3e));
    DGU16(lo) = DGU16((uint16_t)(src + 0x3c));

    for (si = DGU16(0x4e58); si != 0; si = DGU16(si)) {
        if (DGU16((uint16_t)(si + 2)) != part)
            continue;
        if (DG16((uint16_t)(si + 6)) < DG16(hi))
            continue;
        if (DG16((uint16_t)(si + 6)) == DG16(hi)
            && DGU16((uint16_t)(si + 4)) < DGU16(lo))
            continue;

        answer = 0;
        goto out;
    }

    if (DGU16(0x4e58) != 0
        && (DG16((uint16_t)(DGU16(0x4e58) + 6)) > DG16(hi)
            || (DG16((uint16_t)(DGU16(0x4e58) + 6)) == DG16(hi)
                && DGU16((uint16_t)(DGU16(0x4e58) + 4)) >= DGU16(lo)))) {

        di = DGU16(0x4e58);
        si = DGU16(DGU16(0x4e58));

        while (si != 0) {
            if (DG16((uint16_t)(si + 6)) > DG16(hi)) {
                di = si;
                si = DGU16(si);
                continue;
            }
            if (DG16((uint16_t)(si + 6)) != DG16(hi))
                break;
            if (DGU16((uint16_t)(si + 4)) > DGU16(lo)) {
                di = si;
                si = DGU16(si);
                continue;
            }
            break;
        }

        si = DGU16(0x4e56);
        DGU16(0x4e56) = DGU16(DGU16(0x4e56));
        DGU16(si) = DGU16(di);
        DGU16(di) = si;
    } else {
        si = DGU16(0x4e56);
        DGU16(0x4e56) = DGU16(DGU16(0x4e56));
        DGU16(si) = DGU16(0x4e58);
        DGU16(0x4e58) = si;
    }

    DGU16((uint16_t)(si + 2)) = part;
    DGU16((uint16_t)(si + 6)) = DGU16(hi);
    DGU16((uint16_t)(si + 4)) = DGU16(lo);
    answer = 1;

out:
    dg_leave(4);
    return answer;
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
    uint16_t fp = dg_enter(8);
    uint16_t v8 = (uint16_t)(fp + 0);            /* [bp-8] */
    uint16_t v6 = (uint16_t)(fp + 2);            /* [bp-6] */
    uint16_t v4 = (uint16_t)(fp + 4);            /* [bp-4] */
    uint16_t v2 = (uint16_t)(fp + 6);            /* [bp-2] */
    uint16_t si, di, bx;

    for (si = (uint16_t)pick_by_flag(0x3000); si != 0; si = DGU16(v4)) {
        DGU16(v4) = (uint16_t)pick_for_record(si, 0x1000);

        if (DGU16((uint16_t)(si + 6)) & 0x10) {
            unlink_node(si);
            free_part(si);
            continue;
        }

        DGU16((uint16_t)(si + 6)) &= 0xfff0;
        DGU16((uint16_t)(si + 8)) = DGU16((uint16_t)(si + 0x94));

        DGU16((uint16_t)(si + 0x26)) = DGU16((uint16_t)(si + 0x8c));
        DGU16((uint16_t)(si + 0x22)) = DGU16((uint16_t)(si + 0x8c));
        DGU16((uint16_t)(si + 0x1e)) = DGU16((uint16_t)(si + 0x8c));
        DGU16((uint16_t)(si + 0x28)) = DGU16((uint16_t)(si + 0x8e));
        DGU16((uint16_t)(si + 0x24)) = DGU16((uint16_t)(si + 0x8e));
        DGU16((uint16_t)(si + 0x20)) = DGU16((uint16_t)(si + 0x8e));

        DG32((uint16_t)(si + 0x16)) = DG16((uint16_t)(si + 0x1e));
        DG32((uint16_t)(si + 0x1a)) = DG16((uint16_t)(si + 0x20));
        DG32((uint16_t)(si + 0x16)) =
            (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x16)), 9);
        DG32((uint16_t)(si + 0x1a)) =
            (int32_t)long_shift_left((uint32_t)DG32((uint16_t)(si + 0x1a)), 9);

        DGU16((uint16_t)(si + 0x0c)) = DGU16((uint16_t)(si + 0x90));
        DGU16((uint16_t)(si + 0x0e)) = DGU16((uint16_t)(si + 0x0c));
        DGU16((uint16_t)(si + 0x10)) = DGU16((uint16_t)(si + 0x0c));

        set_object_extent(si);

        DGU16((uint16_t)(si + 0x42)) = DGU16((uint16_t)(si + 0x46));
        DGU16((uint16_t)(si + 0x40)) = DGU16((uint16_t)(si + 0x44));

        place_object_for_draw(si);

        DG32((uint16_t)(si + 0x2e)) = DG32((uint16_t)(si + 0x2a));
        DG32((uint16_t)(si + 0x32)) = DG32((uint16_t)(si + 0x2a));
        DG32((uint16_t)(si + 0x48)) = DG32((uint16_t)(si + 0x44));
        DG32((uint16_t)(si + 0x4c)) = DG32((uint16_t)(si + 0x44));

        bx = (uint16_t)((int16_t)DG16((uint16_t)(si + 4)) * 0x3a);
        DGU16((uint16_t)(si + 0x3a)) = DGU16((uint16_t)(bx + 0x0ea8));

        DGU16((uint16_t)(si + 0x84)) = 0;
        DGU16((uint16_t)(si + 0x12)) = DGU16((uint16_t)(si + 0x92));
        DGU16((uint16_t)(si + 0x38)) = 0;
        DGU16((uint16_t)(si + 0x36)) = 0;
        DGU16((uint16_t)(si + 0x9a)) = 0;
        DGU16((uint16_t)(si + 0x98)) = 0;
        DGU16((uint16_t)(si + 0x96)) = 0;
        DGU16((uint16_t)(si + 0xa0)) = 0;
        DGU16((uint16_t)(si + 0x9e)) = 0;
        DGU16((uint16_t)(si + 0x9c)) = 0;

        if (DGU16((uint16_t)(si + 4)) != 0x0e) {
            for (DGU16(v2) = 0; DG16(v2) < 2; DGU16(v2)++)
                DGU16((uint16_t)(si + 0x5a + 2 * DGU16(v2))) =
                    DGU16((uint16_t)(si + 0x5a + 2 * (DGU16(v2) + 2)));
        }

        bx = (uint16_t)((int16_t)DG16((uint16_t)(si + 4)) * 0x3a);
        call_part_setup(DGU16((uint16_t)(bx + 0x0ed0)),
                        DGU16((uint16_t)(bx + 0x0ed2)), si);
    }

    for (si = (uint16_t)pick_by_flag(0x3000); si != 0;
         si = (uint16_t)pick_for_record(si, 0x1000)) {

        if (DGU16((uint16_t)(si + 4)) == 8) {
            compute_link_endpoints(DGU16((uint16_t)(si + 0x54)));
            continue;
        }

        if (DGU16((uint16_t)(si + 4)) != 0x0a)
            continue;

        di = DGU16((uint16_t)(si + 0x66));
        DGU16((uint16_t)(di + 2)) = DGU16((uint16_t)(di + 6));
        DGU16((uint16_t)(di + 4)) = DGU16((uint16_t)(di + 8));
        DG8((uint16_t)(di + 0x0a)) = DG8((uint16_t)(di + 0x0c));
        DG8((uint16_t)(di + 0x0b)) = DG8((uint16_t)(di + 0x0d));

        DGU16((uint16_t)(DGU16((uint16_t)(di + 2))
                         + 0x66 + 2 * DG8((uint16_t)(di + 0x0a)))) = di;
        DGU16((uint16_t)(DGU16((uint16_t)(di + 4))
                         + 0x66 + 2 * DG8((uint16_t)(di + 0x0b)))) = di;

        DGU16(v6) = DGU16((uint16_t)(di + 2));
        DGU16(v8) = DGU16((uint16_t)(DGU16(v6)
                                     + 0x5a + 2 * DG8((uint16_t)(di + 0x0a))));

        while (DGU16(v6) != 0) {
            if (DGU16((uint16_t)(DGU16(v6) + 4)) == 7)
                DGU16((uint16_t)(DGU16(v6) + 0x68)) = di;

            if (DGU16((uint16_t)(di + 4)) == DGU16(v6)) {
                DGU16(v6) = 0;
                continue;
            }

            DGU16(v6) = DGU16(v8);
            DGU16(v8) = DGU16((uint16_t)(DGU16(v8) + 0x5a));
        }

        refresh_link_geometry(di);

        DGU16((uint16_t)(si + 0x9a)) = (uint16_t)link_end_distance(di, 3, 0);
        DGU16((uint16_t)(si + 0x98)) = DGU16((uint16_t)(si + 0x9a));
        DGU16((uint16_t)(si + 0x96)) = DGU16((uint16_t)(si + 0x9a));

        DGU16((uint16_t)(si + 0xa0)) = (uint16_t)link_end_distance(di, 3, 1);
        DGU16((uint16_t)(si + 0x9e)) = DGU16((uint16_t)(si + 0xa0));
        DGU16((uint16_t)(si + 0x9c)) = DGU16((uint16_t)(si + 0xa0));

        DGU16((uint16_t)(di + 0x12)) = 0;
        DGU16((uint16_t)(di + 0x10)) = 0;
        DGU16((uint16_t)(di + 0x0e)) = 0;
    }

    dg_leave(8);
}

/*
 * 0x080b9
 *
 * Is the point at DGROUP 0x5782/0x5784 inside the play area? The box is
 * 8..0x237 across and 8..0x167 down, and both edges are inclusive.
 */
int16_t point_in_play_area(void)
{
    if (DG16(0x5784) < 8 || DG16(0x5784) > 0x237)
        return 0;
    if (DG16(0x5782) < 8 || DG16(0x5782) > 0x167)
        return 0;
    return 1;
}

/*
 * 0x080e7
 *
 * Take the object off both pages.
 *
 * The two words it erases are the driver's own `VMDS + 0x12` and `+ 0x14` -
 * the page being drawn into and the page on screen. They are page segments,
 * and `erase_object` takes them as handles, so `claim_page_slot` is what maps
 * a page to the record describing what is drawn on it. There is one such
 * record per page, which is why a double-buffered display has to erase twice.
 *
 * Clearing 0x52f2 first is what makes the sibling at 0x0810b, which sets it,
 * the other half of the pair; 0x08125 tests it before redrawing.
 */
void erase_both_pages(void)
{
    DG16(0x52f2) = 0;
    clear_flag_2d44_thunk();
    erase_object(vga_page_back);
    erase_object(vga_page_front);
}

/*
 * 0x0810b
 *
 * Turn the cursor back on and put it on the screen: set the flag at DGROUP
 * 0x52f2 and then call `restore_cursor_following`, which is guarded by that
 * same flag and so is bound to act.
 *
 * The pair to `clear_flag_2d44_thunk`, which is how the rest of the program
 * takes the cursor *off* the screen around a blit. This is the one that says
 * "whatever happened before, the cursor is wanted now" - where 0x08125 on its
 * own only puts back what a matching call had removed.
 */
void show_cursor_again(void)
{
    DGU16(0x52f2) = 1;
    restore_cursor_following();
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
 * 0x08125
 *
 * Let the cursor follow the mouse again, but only if DGROUP 0x52f2 says it
 * should. The pair to `clear_flag_2d44_thunk`, and the reason it is a routine
 * rather than a line is that the flag is what a caller sets to say "I turned
 * the cursor off, so put it back" - a caller that never turned it off leaves
 * 0x52f2 clear and this does nothing.
 */
void restore_cursor_following(void)
{
    if (DGU16(0x52f2) != 0)
        set_flag_2d44();
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
        redraw_cursor_all();
    else
        vm_show_page(wait_retrace);
}

/*
 * 0x081f9
 *
 * Show what has just been painted, and then make the page that was on show the
 * one drawn into: 0x38a6 takes 0x38a4 and 0x38a8 takes 0x38a2, which is the
 * pair of page words swapping roles, and the whole 0x280 by 0x170 picture is
 * copied across so the new back page starts as a copy of what the player is
 * looking at.
 *
 * Its neighbour at 0x08229 does the same three things in the other order -
 * copy first, then present - and the port has both, because which order a
 * caller wants is the whole difference between them.
 *
 * (Filed here rather than with `paint_game_screen`, which is what used to call
 * it: 0x081f9 is in this segment.)
 */
void present_back_page(void)
{
    present_frame(1);

    DGU16(0x38a6) = DGU16(0x38a4);
    DGU16(0x38a8) = DGU16(0x38a2);

    copy_rect_around_cursor(0, 0, 0x280, 0x170);
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
 * 0x08332
 *
 * Set the clipping box to the **play area**: 0,0 to 639,367. The same four
 * words as `set_clip_full_screen` below and thirty-two rows shorter, which is
 * the strip along the bottom the game keeps for itself.
 */
void set_clip_play_area(void)
{
    clip_left = 0;
    clip_top = 0;
    clip_right = 0x27F;
    clip_bottom = 0x16F;
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
 * 0x08229
 *
 * Put the whole picture back on the screen after something has been drawn over
 * it - which here is a message box.
 *
 * The two page words the driver keeps at 0x38a6 and 0x38a8 are set from 0x38a4
 * and 0x38a2, so both of the driver's current pointers name the pages the game
 * set up, and then the entire picture - 0,0 to 0x280 by 0x170, the 640 by 368
 * the line compare gives - is copied and presented.
 *
 * 0x170 and not 0x18f: the eighty rows below the split are the split screen and
 * are not this page's to repaint.
 */
void repaint_whole_screen(void)
{
    DGU16(0x38a6) = DGU16(0x38a4);
    DGU16(0x38a8) = DGU16(0x38a2);

    copy_rect_around_cursor(0, 0, 0x280, 0x170);
    present_frame(1);
}

/*
 * 0x084b0
 *
 * The largest allocation the heap could still satisfy.
 *
 * Borland's `heapwalk` is run over every block, and a free one offers its size
 * less the four bytes of its own header. The walk's record lives in this
 * routine's own frame - `lea ax,[bp-6]` is what is passed - so the three words
 * the callee fills are locals here, which is why the frame is eight bytes for
 * what looks like two variables.
 *
 * Then the space that has never been in a block at all: the running word at
 * [bp-8] holds the *last* block's address plus its size, which is the top of
 * the heap, and 0x52fc is what the stack is reserved below - `game_start` sets
 * it to 0x800. `neg` and subtract gives the gap between them, and the answer is
 * whichever of the two is bigger.
 *
 * The record is a **guest** frame, not a C local: `heapwalk` is handed a
 * DGROUP offset and a C local has none, so this reserves through `dg_enter`
 * the eight bytes the original's `sub sp,8` reserves. See dgroup.h.
 *
 * So a heap with no free block still answers what a fresh one would give.
 */
int16_t heap_largest_free(void)
{
    uint16_t fp   = dg_enter(8);
    uint16_t total = fp;                    /* [bp-8] */
    uint16_t info  = (uint16_t)(fp + 2);    /* [bp-6], the walk record */
    uint16_t best = 0, gap;

    DGU16(total) = 0;
    DGU16(info) = 0;

    while (heapwalk(info) == 2) {
        DGU16(total) = (uint16_t)(DGU16(info) + DGU16((uint16_t)(info + 2)));
        if (DGU16((uint16_t)(info + 4)) != 0)
            continue;
        if ((uint16_t)(DGU16((uint16_t)(info + 2)) - 4) > best)
            best = (uint16_t)(DGU16((uint16_t)(info + 2)) - 4);
    }

    gap = (uint16_t)(-(int16_t)DGU16(0x52fc) - DGU16(total));
    if (gap > best)
        best = gap;

    dg_leave(8);
    return (int16_t)best;
}

/*
 * 0x08432
 *
 * Is there room for another part? Answers 1 for yes and 0 for no, and says so
 * on screen when the answer changes.
 *
 * Three bands of `heap_largest_free`, with 0x4e83 remembering whether the
 * player has already been told:
 *
 *   under 0x0fa0   "OUT OF MEMORY" / "You can't place any more parts.", and 0
 *   under 0x1388   "MEMORY LOW" / "Memory is getting low...", once, and 1
 *   over  0x1770   the warning is armed again by clearing 0x4e83
 *
 * The gap between 0x1388 and 0x1770 is hysteresis: the flag is set at the
 * lower and only cleared at the higher, so a machine hovering near the edge is
 * not told twice.
 *
 * A message box has to be painted over and taken away again, which is what
 * `redraw_machine_area` and `repaint_whole_screen` are doing after each, and
 * `update_button_state` swallows the click that dismissed it.
 */
int16_t check_room_for_part(void)
{
    int16_t si = heap_largest_free();

    if ((uint16_t)si < 0x0fa0) {
        show_message_box(0x1d77, 0x1d85);   /* "OUT OF MEMORY" */
        DGU16(0x4e83) = 1;
        redraw_machine_area();
        repaint_whole_screen();
        update_button_state();
        return 0;
    }

    if ((uint16_t)si < 0x1388 && DGU16(0x4e83) == 0) {
        show_message_box(0x1d2f, 0x1d3a);   /* "MEMORY LOW" */
        DGU16(0x4e83) = 1;
        redraw_machine_area();
        repaint_whole_screen();
        update_button_state();
        return 1;
    }

    if ((uint16_t)si > 0x1770)
        DGU16(0x4e83) = 0;

    return 1;
}

/*
 * 0x08259
 *
 * Set the four holiday flags from today's date. Nothing else reads the date;
 * these four words are the whole result.
 *
 *   DGROUP 0x4e81  the 14th of February
 *   DGROUP 0x4e7f  the 17th of March
 *   DGROUP 0x4e7d  the 31st of October
 *   DGROUP 0x4e7b  the 25th of December
 *
 * All four are cleared first, so a second call on an ordinary day undoes a
 * first one on a holiday.
 *
 * `dos_getdate` leaves the year at the local's +0 and DOS's packed DX at +2, so
 * the day is the byte at +2 and the month the byte at +3 - which is why the
 * comparisons read a byte at a time rather than a word.
 */
void set_holiday_flags(void)
{
    uint16_t fp = dg_enter(4);
    uint16_t bp = (uint16_t)(fp + 4);
    uint16_t d = (uint16_t)(bp - 4);

    DG16(0x4e7b) = 0;
    DG16(0x4e7d) = 0;
    DG16(0x4e7f) = 0;
    DG16(0x4e81) = 0;

    dos_getdate(d);

    if (DG8(d + 3) == 2 && DG8(d + 2) == 0x0e)
        DG16(0x4e81) = 1;
    if (DG8(d + 3) == 3 && DG8(d + 2) == 0x11)
        DG16(0x4e7f) = 1;
    if (DG8(d + 3) == 0xa && DG8(d + 2) == 0x1f)
        DG16(0x4e7d) = 1;
    if (DG8(d + 3) == 0xc && DG8(d + 2) == 0x19)
        DG16(0x4e7b) = 1;

    dg_leave(4);
}

/*
 * 0x08364
 *
 * Make one piece of music the current one: stop and free whatever was playing,
 * open the new one and start it, and remember it at DGROUP 0x52d5.
 *
 * Asking for what is already playing does nothing at all - the test is first -
 * and -1 means "nothing", both as what was playing and as what is wanted.
 */
void select_music(int16_t id)
{
    if (id == DG16(0x52d5))
        return;

    if (DG16(0x52d5) != -1) {
        stop_music_or_effect(DG16(0x52d5));
        remove_and_free_records(DG16(0x52d5));
    }

    if (id != -1) {
        open_sound_file(DGU16(0x52f8), id);
        play_sound(id);
    }

    DG16(0x52d5) = id;
}

/*
 * 0x083ab
 *
 * Play a sound, and hold six of them back when the music is off.
 *
 * Ids 4, 9, 0x10, 0x12, 0x13 and 0x14 go out only when DGROUP 0x4ec1 is
 * non-zero - that is the setting TIM.CFG carries, and it defaults to 6 when
 * there is no file. Every other id plays whatever the setting says.
 *
 * The six are compared one at a time rather than looked up, and both branches
 * end in the same call: the test decides *whether*, never *what*.
 */
void play_sound(int16_t id)
{
    if (id == 0x10 || id == 0x12 || id == 9 || id == 0x13 || id == 0x14
        || id == 4) {
        if (DGU16(0x4ec1) != 0)
            start_sequence_by_id(id);
        return;
    }

    start_sequence_by_id(id);
}

/*
 * 0x083ea
 *
 * Stop a sound, or all of them.
 *
 * A number of its own stops that one sequence. Zero stops all twenty of the
 * effects, 1 to 0x14; -2 stops those *and* the seven pieces of music, 0x3e9 to
 * 0x3ef. Nothing else is a special value.
 */
void stop_music_or_effect(int16_t id)
{
    int16_t si;

    if (id != 0 && id != -2) {
        stop_sequences(id);
        return;
    }

    for (si = 1; si <= 0x14; si++)
        stop_sequences(si);

    if (id != -2)
        return;

    for (si = 0x3e9; si <= 0x3ef; si++)
        stop_sequences(si);
}

/*
 * 0x08510
 *
 * Free a block, with the heap checked either side of it. The check is the one
 * that hangs on a broken heap, so a free that corrupts the ring stops the game
 * at the free rather than somewhere unrelated later.
 */
void checked_free(uint16_t p)
{
    heap_check_or_hang();
    heap_free_far(p);
    heap_check_or_hang();
}

/*
 * 0x08528
 *
 * Check the heap, and **stop dead** if it is broken.
 *
 * The stop is written as a loop rather than a halt: `si` starts at 2, adds 2,
 * and is compared against 3 - which it steps over on every pass and never
 * equals. That is a deliberate hang, not a bug, and the port keeps it as one:
 * a corrupt heap that carried on would draw a wrong picture some seconds later
 * and look like a blitter fault.
 */
void heap_check_or_hang(void)
{
    int16_t si;

    if (heap_check() != -1)
        return;

    si = 2;
    while (si != 3)
        si = (int16_t)(si + 2);
}

/*
 * 0x08546
 *
 * Walk a list of screen regions and act on the one the pointer is in. The list
 * is one of the five `build_screen_regions` built, and its records are the
 * 0x1a-byte ones from there: a link at +0, a mask of which screens the region
 * belongs to at +2, its rectangle at +6 through +0xc, a cursor number at +0xe,
 * a screen to switch to at +0x10, and **two far function pointers**, at +0x12
 * for entering the region and +0x16 for clicking in it.
 *
 * DGROUP 0x4e6b is the screen the game is on, and a region whose mask does not
 * have that bit is skipped without its rectangle even being looked at. The
 * pointer's position is DGROUP 0x5782 and 0x5784, and 0x5774 being 2 is the
 * button.
 *
 * The walk **stops at the region it acts on** - `si` is zeroed rather than
 * followed - so the first match wins and the ones after it are never
 * considered. Running off the end of the list without a match sets the cursor
 * back to 0.
 *
 * The two far calls are the relocated pointers `build_screen_regions` files in;
 * the port cannot call through a guest far pointer and dispatches on the value
 * instead, which is why an unexpected one aborts rather than being ignored.
 */
void regions_handle_pointer(uint16_t list)
{
    uint16_t si = DGU16(list);

    while (si != 0) {
        if ((DGU16((uint16_t)(si + 2)) & DGU16(0x4e6b)) != 0
            && DG16((uint16_t)(si + 6)) <= DG16(0x5784)
            && DG16((uint16_t)(si + 0x0a)) >= DG16(0x5784)
            && DG16((uint16_t)(si + 8)) <= DG16(0x5782)
            && DG16((uint16_t)(si + 0x0c)) >= DG16(0x5782)) {

            if ((DGU16((uint16_t)(si + 0x12))
                 | DGU16((uint16_t)(si + 0x14))) != 0)
                call_region_handler(DGU16((uint16_t)(si + 0x12)),
                                    DGU16((uint16_t)(si + 0x14)), si);

            select_cursor((int16_t)DGU16((uint16_t)(si + 0x0e)));

            if (DGU16(0x5774) == 2) {
                if ((DGU16((uint16_t)(si + 0x16))
                     | DGU16((uint16_t)(si + 0x18))) != 0)
                    call_region_handler(DGU16((uint16_t)(si + 0x16)),
                                        DGU16((uint16_t)(si + 0x18)), si);

                DGU16(0x4e6b) = DGU16((uint16_t)(si + 0x10));
            }

            /* Acted on: the rest of the list is not looked at. */
            return;
        }

        si = DGU16(si);
        if (si == 0)
            select_cursor(0);
    }
}

/*
 * 0x085c9
 *
 * Build the game's screen regions: thirty-six records of 0x1a bytes off the
 * near heap, each pushed onto the front of one of five lists whose heads are
 * the words at DGROUP 0x4e71 through 0x4e79. Two of them are also remembered
 * on their own, at 0x4e6d and 0x4e6f, as well as going on a list.
 *
 * The original is 2,283 bytes of straight-line `mov word [si+n], imm` - what a
 * compiler makes of thirty-six initialisers written out one after another. It
 * is transcribed as the **table it is**, and the table is as much a
 * transcription as a routine would be: every value below was read out of the
 * image at the address above.
 *
 * `heap_calloc_far` zeroes each record, so a field the original never assigns
 * is zero, and a zero here means exactly that. Field 0 is the link, filled in
 * when the record goes on its list.
 *
 * **Seventeen of these words are relocations, not values**: the segment half of
 * a far function pointer, where the 0x0dff in the image is a paragraph count
 * from the load address that the loader adds the program's base to. `reloc`
 * says which, per record - *measured* from the relocation table of the
 * recovered executable, not inferred from a word looking like a segment. It has
 * to be per record and not per field: the same offset holds a real pointer in
 * one row and nothing at all in another, and adding the base to a field the
 * original never wrote puts the load segment where a zero belongs.
 */
static const struct {
    uint16_t head;      /* the list this record goes on */
    uint16_t also;      /* a second word that keeps it, or 0 for none */
    uint16_t reloc;     /* bit k: field[k] is a segment the loader fixes up */
    uint16_t field[12];
} screen_regions[36] = {
    /*      head     also   reloc     +02     +04     +06     +08     +0a     +0c     +0e     +10     +12     +14     +16     +18 */
    { 0x4e79,    0x0,  0x200, { 0x1000,    0x0,    0x0,    0x0,  0x27f,  0x16f,    0x0, 0x1000, 0x2f01,  0xdff,    0x0,    0x0 } },
    { 0x4e79,    0x0,  0x200, { 0x1000,    0x0,  0x240,    0x0,  0x278,   0x3f,   0x1a,    0x0, 0x2da9,  0xdff,    0x0,    0x0 } },
    { 0x4e79,    0x0,    0x0, { 0x1000,    0x0,  0x240,   0x43,  0x25b,   0x5a,    0x0,  0x800,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e79,    0x0,    0x0, { 0x1000,    0x0,  0x260,   0x43,  0x27f,   0x5a,    0x0,  0x400,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e79,    0x0,  0xa00, { 0x1000,    0x0,  0x240,   0x64,  0x278,   0x90,    0x2, 0x1000, 0x2dd2,  0xdff, 0x2e24,  0xdff } },
    { 0x4e79,    0x0,  0xa00, { 0x1000,    0x1,  0x240,   0x91,  0x278,   0xc4,    0x2, 0x1000, 0x2dd2,  0xdff, 0x2e24,  0xdff } },
    { 0x4e79,    0x0,  0xa00, { 0x1000,    0x2,  0x240,   0xc5,  0x278,   0xf8,    0x2, 0x1000, 0x2dd2,  0xdff, 0x2e24,  0xdff } },
    { 0x4e79,    0x0,  0xa00, { 0x1000,    0x3,  0x240,   0xf9,  0x278,  0x12c,    0x2, 0x1000, 0x2dd2,  0xdff, 0x2e24,  0xdff } },
    { 0x4e79,    0x0,  0xa00, { 0x1000,    0x4,  0x240,  0x12d,  0x278,  0x160,    0x2, 0x1000, 0x2dd2,  0xdff, 0x2e24,  0xdff } },
    { 0x4e79,    0x0,    0x0, { 0xc000,    0x0,    0x0,    0x0,  0x27f,  0x18f,    0x0, 0x1000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e77,    0x0,    0x0, {    0x2,    0x0,  0x110,   0x48,  0x210,   0xe8,   0x10, 0x8000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e77,    0x0,    0x0, {    0x2,    0x0,   0x3a,   0x5b,   0x4f,   0x7e,   0x10, 0x8000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e77,    0x0,    0x0, {    0x2,    0x0,   0xd8,   0x60,   0xf0,   0x77,   0x15, 0x1000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e77,    0x0,  0x200, {    0x2,    0x0,   0x39,   0x86,   0x5f,   0xab,    0x0,  0x400, 0x34eb,  0xdff,    0x0,    0x0 } },
    { 0x4e77,    0x0,  0x200, {    0x2,    0x0,   0x96,   0x8c,   0xbf,   0xa4,    0x0,  0x100, 0x3508,  0xdff,    0x0,    0x0 } },
    { 0x4e77,    0x0,    0x0, {    0x2,    0x0,   0x58,   0x5d,   0x6d,   0x6d,   0x11, 0x4000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e77,    0x0,    0x0, {    0x2,    0x0,   0x58,   0x6f,   0x6d,   0x7e,   0x11, 0x2000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e77,    0x0,    0x0, {    0x2,    0x0,   0xbc,   0x5c,   0xce,   0x7b,   0x12,  0x800,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e77,    0x0,    0x0, {    0x2,    0x0,   0x6d,   0x85,   0x8c,   0xa3,   0x13,  0x200,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e77,    0x0,  0x200, {    0x2,    0x0,   0xc8,   0x8c,   0xf1,   0xa4,    0x0,   0x80, 0x3525,  0xdff,    0x0,    0x0 } },
    { 0x4e77,    0x0,  0x200, {    0x2,    0x0,   0x41,   0xc8,   0xe1,   0xf8,    0x0,   0x40, 0x3542,  0xdff,    0x0,    0x0 } },
    { 0x4e77,    0x0,  0x200, {    0x2,    0x0,   0x41,  0x114,   0xe1,  0x144,    0x0,   0x20, 0x355f,  0xdff,    0x0,    0x0 } },
    { 0x4e75,    0x0,    0x0, { 0xd000,    0x0,   0x40,   0x56,   0xf8,   0x66,    0x0, 0x4000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e75,    0x0,    0x0, { 0xd000,    0x0,   0x40,   0x7c,   0xb0,   0xf3,    0x0, 0x2000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e75,    0x0,    0x0, { 0xd000,    0x0,   0x90,  0x10c,  0x100,  0x11c,    0x0, 0x1000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e75,    0x0,    0x0, { 0xd000,    0x0,   0xbc,   0x74,   0xdc,   0x94,    0x0,  0x800,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e75,    0x0,    0x0, { 0xd000,    0x0,   0xbc,   0xe0,   0xdc,  0x100,    0x0,  0x400,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e75,    0x0,    0x0, { 0xd000,    0x0,   0x40,  0x130,   0x90,  0x144,    0x0,  0x200,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e75,    0x0,    0x0, { 0xd000,    0x0,   0xc0,  0x130,  0x110,  0x144,    0x0,  0x100,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e73, 0x4e6f,    0x0, { 0x8000,    0x0,   0xc8,   0xd4,   0xc8,   0xe4,    0x0, 0x4000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e73, 0x4e6d,    0x0, { 0x8000,    0x0,  0x178,   0xd4,  0x178,   0xe4,    0x0, 0x2000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e71,    0x0,    0x0, { 0x8800,    0x0,   0x30,   0x4c,  0x1c0,  0x11d,    0x0, 0x4000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e71,    0x0,    0x0, { 0x8800,    0x0,  0x1cc,   0x42,  0x1ec,   0x62,    0x0, 0x2000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e71,    0x0,    0x0, { 0x8800,    0x0,  0x1cc,  0x108,  0x1ec,  0x128,    0x0, 0x1000,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e71,    0x0,    0x0, { 0x8000,    0x0,   0x90,  0x13c,  0x158,  0x14c,    0x0,  0x800,    0x0,    0x0,    0x0,    0x0 } },
    { 0x4e71,    0x0,    0x0, { 0x8800,    0x0,  0x1f0,  0x12c,  0x218,  0x154,    0x0,  0x400,    0x0,    0x0,    0x0,    0x0 } },
};

/*
 * 0x085c9
 *
 * The routine the table above is the body of: allocate, fill, link, repeat.
 */
void build_screen_regions(void)
{
    uint16_t i;

    for (i = 0; i < 36; i++) {
        uint16_t si = heap_calloc_far(1, 0x1a);
        uint16_t k;

        if (screen_regions[i].also != 0)
            DGU16(screen_regions[i].also) = si;

        for (k = 0; k < 12; k++) {
            uint16_t v = screen_regions[i].field[k];

            if ((screen_regions[i].reloc & (1u << k)) != 0)
                v = (uint16_t)(v + (uint16_t)(IMAGE_BASE >> 4));

            DGU16((uint16_t)(si + 2 + 2 * k)) = v;
        }

        DGU16(si) = DGU16(screen_regions[i].head);
        DGU16(screen_regions[i].head) = si;
    }
}

/*
 * 0x08f27
 *
 * Program the CRTC's **Line Compare**, the split-screen line: from the scan
 * line it names down, the card stops following the start address and fetches
 * from offset 0 instead.
 *
 * Ten bits again, spread the way the hardware spreads them - the low eight in
 * Line Compare itself at index 0x18, bit 8 in Overflow bit 4, bit 9 in Maximum
 * Scan Line bit 6 - and the two high registers are read back and merged rather
 * than written whole, so the timing bits sharing them survive. Exactly the
 * shape of `vm_set_display_lines`, which does the same for the blanking line.
 *
 * The four `shl bl,1` in each half are a shift by four, and by five in the
 * second, written out because an 8086 has no shift by an immediate count.
 *
 * **This is why the game's screens are 368 rows.** It is called with 0x16f -
 * 367 - as the game screen is set up, alongside `vm_set_display_lines(0x1bf)`.
 * So the card is told to show 448 lines and to restart at address 0 after 368
 * of them: the picture is the first 368 rows and the last 80 are the *split
 * screen*, showing memory from the start of the plane. That band is not
 * leftover garbage in a page, and it is not the other page bleeding through -
 * it is a hardware feature this program uses, and anything composing a frame
 * has to honour it or the bottom eighty rows are wrong.
 */
void vm_set_line_compare(uint16_t line)
{
    uint8_t v;

    io_out8(PORT_CRTC_INDEX, 0x18);
    io_out8(PORT_CRTC_DATA, (uint8_t)(line & 0xFF));

    io_out8(PORT_CRTC_INDEX, 0x07);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xEF) | (((line >> 8) & 1) << 4));
    io_out8(PORT_CRTC_DATA, v);

    io_out8(PORT_CRTC_INDEX, 0x09);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xBF) | (((line >> 8) & 2) << 5));
    io_out8(PORT_CRTC_DATA, v);
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
 * NOT a transcription: a boundary the port chose. The enclosing routine
 * contains three inlined copies of this loop, and this is the loop, lifted out
 * so the port writes it once. Its address is given below rather than here,
 * because an address on the first line of a block *is* the provenance mark.
 *
 * Walk each list once, from its head to either a null entry or a match. The
 * body matches 0x09904..0x09941 instruction for instruction - the 0x1c-strided
 * table at 0x54a7/0x54a9, the `(off | seg) == 0` test, the compare against the
 * wanted pair, the `off += 8` - but **the enclosing routine is not
 * transcribed**: 0x098e0 takes one argument at bp+6, reads what it is looking
 * for out of the globals at 0x5482 and 0x5484 rather than from arguments, and
 * carries on past this loop at 0x09941. Its signature here is the helper's,
 * not the original's.
 *
 * It carried a bare `0x098e0` before, which files a routine as transcribed
 * under `tests/provenance.py`, and this is not one.
 */
void scan_entry_list(int16_t idx, uint16_t want_off, uint16_t want_seg,
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
 * 0x0917f
 *
 * The game's own `fclose`, and the last of the set over the archive.
 *
 * A null `FILE` is -1 before anything else. With the archive open and an entry
 * for this stream, the close is against the entry rather than the stream: the
 * one-entry cache is thrown away by looking up handle 0, whatever `FILE` the
 * entry carries at +0x10 is closed, its +0xe cleared, and the count of open
 * resources at DGROUP 0x5486 dropped by one.
 *
 * A stream with no entry is closed directly through the runtime, and skips both
 * the entry bookkeeping and the open count. That is the path a **saved
 * machine** takes: it was opened by name and never came out of the archive.
 *
 * A failure sets bit 0 of DGROUP 0x567b, which is where this layer collects
 * whether anything went wrong.
 */
int16_t game_fclose(uint16_t file)
{
    uint16_t si = 0;
    int16_t di = 0;

    if (file == 0)
        return -1;

    if (DG16(0x547e) != 0)
        si = archive_entry_for(file);

    if (si == 0) {
        di = stdio_fclose(file);
        DG16(0x567b) = (int16_t)(DGU16(0x567b) | (di == -1 ? 1 : 0));
        return di;
    }

    archive_entry_for(0);

    if (DGU16(si + 0x10) != 0)
        di = stdio_fclose(DGU16(si + 0x10));

    DG16(si + 0xe) = 0;
    DG8(0x5486) = (uint8_t)(DG8(0x5486) - 1);

    DG16(0x567b) = (int16_t)(DGU16(0x567b) | (di == -1 ? 1 : 0));
    return di;
}

/*
 * 0x093e0
 *
 * `rewind` over the archive: `game_fseek` to nought from the start, and
 * nothing else. Five pushes and a call.
 */
void game_rewind(uint16_t file)
{
    game_fseek(file, 0, 0, 0);
}

/*
 * 0x091ef
 *
 * The game's own `fread`. Everything that reads a resource comes through here,
 * and it decides between the loose file and the archive.
 *
 * With the archive closed - DGROUP 0x547e zero - or with no entry for this
 * `FILE`, it is a plain forward to the runtime's `fread` and nothing else
 * happens. An entry whose +0x10 holds a `FILE` of its own is forwarded the same
 * way, with that one substituted.
 *
 * Otherwise the read is against a stretch of the archive, and three things have
 * to happen that a plain `fread` would not do.
 *
 * The request is **clamped** to what is left of the entry, by taking whole
 * items off the count until `size * count` fits in `+6:+8` minus the position
 * at `+0xa:+0xc`. The comparison is 32-bit with the request's high half a
 * constant zero, which is why the first branch of it can never be taken.
 *
 * Then the archive file is made current and seeked to the entry's base plus the
 * position, and the `FILE` to read from is looked up in the table at DGROUP
 * 0x549f, 0x1c bytes per open archive.
 *
 * Afterwards the entry's position and the archive's own running total at
 * 0x54a1 both advance by what was actually read - `n * size`, not what was
 * asked for.
 */
uint16_t game_fread(uint16_t buf, uint16_t size, uint16_t count, uint16_t file)
{
    uint16_t di = 0;

    if (DG16(0x547e) != 0)
        di = archive_entry_for(file);

    if (di == 0)
        return stdio_fread(buf, size, count, file);

    if (DGU16(di + 0x10) != 0)
        return stdio_fread(buf, size, count, DGU16(di + 0x10));

    {
        uint16_t bytes = (uint16_t)((int16_t)size * (int16_t)count);
        uint16_t n, got, base_lo, base_hi;

        for (;;) {
            uint16_t lo, hi;

            if (bytes == 0)
                break;

            lo = (uint16_t)(DGU16(di + 6) - DGU16(di + 0xa));
            hi = (uint16_t)(DGU16(di + 8) - DGU16(di + 0xc)
                            - (DGU16(di + 6) < DGU16(di + 0xa) ? 1 : 0));

            if (hi != 0)
                break;
            if (bytes <= lo)
                break;

            count--;
            bytes = (uint16_t)(bytes - size);
        }

        make_file_current(DGU16(di));

        base_lo = (uint16_t)(DGU16(di + 2) + DGU16(di + 0xa));
        base_hi = (uint16_t)(DGU16(di + 4) + DGU16(di + 0xc)
                             + (base_lo < DGU16(di + 2) ? 1 : 0));
        seek_file_to(base_lo, base_hi);

        file = DGU16(0x549f + 0x1c * DGU16(di));

        n = stdio_fread(buf, size, count, file);

        got = (uint16_t)((int16_t)n * (int16_t)size);

        DG16(di + 0xa) = (int16_t)(DGU16(di + 0xa) + got);
        if (DGU16(di + 0xa) < got)
            DG16(di + 0xc) = (int16_t)(DGU16(di + 0xc) + 1);

        {
            uint16_t t = (uint16_t)(0x54a1 + 0x1c * DGU16(di));

            DG16(t) = (int16_t)(DGU16(t) + got);
            if (DGU16(t) < got)
                DG16(t + 2) = (int16_t)(DGU16(t + 2) + 1);
        }

        return n;
    }
}

/*
 * 0x092dc
 *
 * The game's own `fseek`, and `game_fread`'s counterpart: the same choice
 * between the loose file and the archive, the same substitution when an entry
 * carries its own `FILE`.
 *
 * Against an archive entry there is no seeking to do on the file at all - only
 * the entry's own position at +0xa:+0xc is moved, and the archive file is
 * seeked when something is actually read. So the three whences are worked out
 * here in 32-bit arithmetic:
 *
 *   0  the offset as given
 *   1  the offset plus the position now
 *   2  the entry's size minus the offset, or zero if the offset is larger
 *
 * and the result is clamped to the entry's size, so seeking past the end parks
 * at the end rather than reporting an error. The answer is 0 either way.
 */
int16_t game_fseek(uint16_t file, uint16_t lo, uint16_t hi, int16_t whence)
{
    uint16_t si = 0;

    if (DG16(0x547e) != 0)
        si = archive_entry_for(file);

    if (si == 0)
        return stdio_fseek(file, lo, hi, whence);

    if (DGU16(si + 0x10) != 0)
        return stdio_fseek(DGU16(si + 0x10), lo, hi, whence);

    if (whence == 1) {
        uint16_t nlo = (uint16_t)(lo + DGU16(si + 0xa));

        hi = (uint16_t)(hi + DGU16(si + 0xc) + (nlo < lo ? 1 : 0));
        lo = nlo;
    } else if (whence == 2) {
        if (DGU16(si + 8) > hi
            || (DGU16(si + 8) == hi && DGU16(si + 6) > lo)) {
            uint16_t nlo = (uint16_t)(DGU16(si + 6) - lo);

            hi = (uint16_t)(DGU16(si + 8) - hi
                            - (DGU16(si + 6) < lo ? 1 : 0));
            lo = nlo;
        } else {
            lo = 0;
            hi = 0;
        }
    }

    if (DGU16(si + 8) < hi
        || (DGU16(si + 8) == hi && DGU16(si + 6) < lo)) {
        hi = DGU16(si + 8);
        lo = DGU16(si + 6);
    }

    DG16(si + 0xc) = (int16_t)hi;
    DG16(si + 0xa) = (int16_t)lo;
    return 0;
}

/*
 * 0x095cf
 *
 * Give a file a buffer, whether it is a loose file or one inside the archive.
 *
 * The difference is **which file gets buffered**, not how. A file the archive
 * knows about has a record from `archive_entry_for`, and that record's +0x10
 * is the handle of the archive it lives in - so the buffer goes on the archive
 * rather than on the caller's own handle, which is the one that will actually
 * be read from. A loose file, or a record with no archive behind it, gets the
 * buffer on itself.
 *
 * DGROUP 0x547e is whether the archive is in use at all; with it clear the
 * lookup is skipped.
 */
void stdio_setbuf_for(uint16_t file, uint16_t buf)
{
    uint16_t rec = 0;

    if (DGU16(0x547e) != 0)
        rec = archive_entry_for(file);

    if (rec == 0) {
        stdio_setbuf(file, buf);
        return;
    }

    if (DGU16((uint16_t)(rec + 0x10)) != 0)
        stdio_setbuf(DGU16((uint16_t)(rec + 0x10)), buf);
}

/*
 * 0x09f68
 *
 * **`stricmp` over two far strings.** A null pointer on either side answers 1
 * rather than crashing, and answers it *before* looking at the other, so two
 * nulls compare as unequal too.
 *
 * Each byte goes through `tolower` before it is compared, which is the whole
 * reason this exists rather than `strcmp`: the listing it sorts holds names DOS
 * hands back in capitals and text the game wrote in whatever case it liked.
 *
 * The loop ends on the *first* string's NUL, so the answer for a prefix is the
 * second string's next character negated - and the two pointers are advanced in
 * the caller's own stack slots, not in registers.
 */
int16_t far_stricmp(uint16_t a_off, uint16_t a_seg, uint16_t b_off,
                    uint16_t b_seg)
{
    int16_t si, di;

    if ((b_off | b_seg) == 0 || (a_off | a_seg) == 0)
        return 1;

    for (;;) {
        si = (int16_t)to_lower(FAR8(a_seg, a_off));
        a_off++;
        di = (int16_t)to_lower(FAR8(b_seg, b_off));
        b_off++;

        if (si == 0 || si != di)
            return (int16_t)(si - di);
    }
}

/*
 * 0x0a62c
 *
 * **Put back everything saved for one page and size**, then give the records
 * away.
 *
 * The slot comes from `find_saved_rect_slot`, and an empty slot or an empty
 * list does nothing at all - not even the page switch below. The first record
 * sets the copy's source and destination pages, at DGROUP 0x38a6 and 0x38a8,
 * from its own +8 and +0xa; every record after it is restored between the same
 * two pages, so a list is only ever built for one pair.
 *
 * Each record is restored one of two ways, by the kind at +0xc:
 *
 *   1  a page-to-page copy of the rectangle, through `copy_rect_thunk`
 *   4  a restore from a saved buffer, through `restore_rect_thunk`, the far
 *      pointer at +0x14
 *
 * and any other kind is skipped in silence, which is how a record can be
 * parked in the list without being drawn.
 *
 * **x and width are in bytes and y and height in pixels.** The two `<< 3`s
 * turn +0 and +4 into pixels for the copy; +2 and +6 are passed through as
 * they stand. That asymmetry is the planar layout showing through - a byte is
 * eight pixels across and one pixel down.
 *
 * The whole chain then goes onto the free list at 0x56e0 in one splice, using
 * the last record the walk saw rather than walking it again.
 */
void restore_saved_rects(uint16_t w, uint16_t h, uint16_t page)
{
    uint16_t slot = find_saved_rect_slot(w, h, page);
    uint16_t rec, last = 0;

    if (slot == 0)
        return;

    rec = DGU16(slot);
    if (rec == 0)
        return;

    DGU16(0x38a6) = DGU16((uint16_t)(rec + 8));
    DGU16(0x38a8) = DGU16((uint16_t)(rec + 0xa));

    while (rec != 0) {
        int16_t x  = (int16_t)(DG16(rec) << 3);
        int16_t rw = (int16_t)(DG16((uint16_t)(rec + 4)) << 3);

        if (DGU16((uint16_t)(rec + 0xc)) == 1)
            copy_rect_thunk((uint16_t)x, DGU16((uint16_t)(rec + 2)),
                            (uint16_t)rw, DGU16((uint16_t)(rec + 6)));
        else if (DGU16((uint16_t)(rec + 0xc)) == 4)
            restore_rect_thunk(DGU16((uint16_t)(rec + 0x14)),
                               DGU16((uint16_t)(rec + 0x16)),
                               DG16(rec), DG16((uint16_t)(rec + 2)),
                               DG16((uint16_t)(rec + 4)),
                               DG16((uint16_t)(rec + 6)));

        last = rec;
        rec = DGU16((uint16_t)(rec + 0x18));
    }

    DGU16((uint16_t)(last + 0x18)) = DGU16(0x56e0);
    DGU16(0x56e0) = DGU16(slot);
    DGU16(slot) = 0;
}

/*
 * 0x0a42a
 *
 * Put back the saved rectangles for **a list of page-and-size pairs**, and
 * then, on one of the two paths, take one off every remaining record's +0xe.
 *
 * The argument picks which table to walk: non-zero takes the one at DGROUP
 * 0x2d0e and **stops after a single entry**, zero takes the one at 0x2d0a and
 * walks it until an entry whose second word is null. The two share the loop,
 * and the test at the bottom is what makes one of them a loop and the other a
 * single pass.
 *
 * Each entry is two near pointers, four bytes apart, and the values are read
 * *through* them and handed to `restore_saved_rects` as width and height, with
 * the page always zero.
 *
 * The copy's source and destination pages, 0x38a6 and 0x38a8, are saved on the
 * way in and put back at the end, because `restore_saved_rects` sets them from
 * the first record it finds and would otherwise leave them wherever the last
 * list went.
 *
 * The count pass only runs on the zero path, over all twenty slots at 0x56b8
 * and every record on each chain. It decrements the word at +0xe - which
 * `find_saved_rect_slot` reads as the *page*. Both readings cannot be right,
 * and the disagreement is recorded rather than resolved: the field is only
 * compared for equality there and only decremented here, so nothing seen so
 * far tells them apart.
 */
void restore_saved_rect_lists(int16_t which)
{
    uint16_t saved_src = DGU16(0x38a6);
    uint16_t saved_dst = DGU16(0x38a8);
    uint16_t entry = (uint16_t)(which != 0 ? 0x2d0e : 0x2d0a);

    for (;;) {
        restore_saved_rects(DGU16(DGU16(entry)),
                            DGU16(DGU16((uint16_t)(entry + 2))), 0);
        entry = (uint16_t)(entry + 4);

        if (which != 0)
            break;
        if (DGU16((uint16_t)(entry + 2)) == 0)
            break;
    }

    DGU16(0x38a6) = saved_src;
    DGU16(0x38a8) = saved_dst;

    if (which != 0)
        return;

    {
        uint16_t slot = 0x56b8;
        int16_t  left = 0x14;

        while (left != 0) {
            uint16_t rec = DGU16(slot);

            while (rec != 0) {
                DG16((uint16_t)(rec + 0xe)) =
                    (int16_t)(DG16((uint16_t)(rec + 0xe)) - 1);
                rec = DGU16((uint16_t)(rec + 0x18));
            }

            slot = (uint16_t)(slot + 2);
            left--;
        }
    }
}

/*
 * 0x0a5e2
 *
 * Find the slot in the table of **twenty saved-rectangle objects** at DGROUP
 * 0x56b8 that already holds a given page, width and height - or, failing that,
 * the first empty slot.
 *
 * Each slot is a word: a near pointer to a record, or zero. A record matches
 * when its page at +0xe, its width at +8 and its height at +0xa are all the
 * ones asked for. The answer is **the slot**, not the record, so a caller can
 * put a new record into it.
 *
 * The first empty slot is remembered as the walk goes past it - `or dx,dx`
 * keeps the *first* one rather than the last - and is what comes back when
 * nothing matched. A full table with no match answers 0, which is also what an
 * empty slot's own contents look like, so the two are told apart by the caller
 * looking at what the slot holds rather than by the answer.
 */
uint16_t find_saved_rect_slot(uint16_t w, uint16_t h, uint16_t page)
{
    uint16_t slot  = 0x56b8;
    uint16_t empty = 0;
    int16_t  left  = 0x14;

    while (left != 0) {
        uint16_t rec = DGU16(slot);

        if (rec == 0) {
            if (empty == 0)
                empty = slot;
        } else if (DGU16((uint16_t)(rec + 0xe)) == page
                   && DGU16((uint16_t)(rec + 8)) == w
                   && DGU16((uint16_t)(rec + 0xa)) == h) {
            return slot;
        }

        slot = (uint16_t)(slot + 2);
        left--;
    }

    return empty;
}

/*
 * 0x0a6d7
 *
 * Give back every saved rectangle held for one page and size: find the slot,
 * walk its chain of records to the end through the links at +0x18, and put the
 * whole chain onto the free list at DGROUP 0x56e0 in one move rather than one
 * record at a time. The slot is then cleared.
 *
 * A slot that does not exist, or holds nothing, is left alone. The list is
 * pushed on the front, so the freed records come back in the reverse of the
 * order they were taken - which nothing depends on, but it is what happens.
 */
void free_saved_rects(uint16_t w, uint16_t h, uint16_t page)
{
    uint16_t slot = find_saved_rect_slot(w, h, page);
    uint16_t rec, last;

    if (slot == 0)
        return;

    rec = DGU16(slot);
    if (rec == 0)
        return;

    last = rec;
    while (DGU16((uint16_t)(last + 0x18)) != 0)
        last = DGU16((uint16_t)(last + 0x18));

    DGU16((uint16_t)(last + 0x18)) = DGU16(0x56e0);
    DGU16(0x56e0) = DGU16(slot);
    DGU16(slot) = 0;
}

/*
 * 0x0a78e
 *
 * Let the cursor follow the mouse again, and redraw it where the mouse now is.
 * The pair to `clear_flag_2d44` three instructions below: DGROUP 0x2d44 is what
 * `redraw_cursor` tests before it asks the driver for the position, so clearing
 * it pins the cursor and setting it releases it.
 */
void set_flag_2d44(void)
{
    DGU16(0x2d44) = 1;
    redraw_cursor(DGU16(0x38a4));
}

/*
 * 0x0a7ae
 *
 * What the timer calls, four ticks in five: read the keyboard and the mouse,
 * move the pointer, and release the frame.
 *
 * It refuses to run at all when DGROUP 0x5752 is above 1 or 0x5740 is already
 * set - the first is the cursor's nesting guard and the second is this routine
 * being in progress - so a redraw cannot be interrupted by the tick that would
 * start another.
 *
 * The eight scan codes it reads are the keypad's: 0x47 0x48 0x49 across the
 * top, 0x4b 0x4d either side, 0x4f 0x50 0x51 across the bottom. Any of the top
 * three moves up, any of the bottom three down, and the corners count for both
 * of their directions - which is what makes the diagonals work. Two pixels a
 * tick, clamped to the screen, and the clamp is against the *hot spot* rather
 * than the pointer's own position.
 *
 * Then Enter, Space, keypad 5 and Insert are all the same button - `si` ends up
 * 1 if any of them is down - and are ORed with the real one. `button_state`
 * turns each into a state, and the accumulators at 0x5768 and 0x576a keep it
 * until the next frame reads them.
 *
 * The last two lines are the ones everything waits on: 0x5740 is cleared and
 * **0x5754 is set**, which is the flag `wait_and_latch_frame` spins on.
 */
void timer_callback(void)
{
    int16_t moved = 0;
    int16_t k_end, k_down, k_pgdn, k_left, k_right, k_home, k_up, k_pgup;
    int16_t si, di;

    if (DG16(0x5752) > 1 || DG16(0x5740) != 0)
        return;

    DG16(0x5740) = 1;

    k_end   = bit0_of_468c(0x4f);
    k_down  = bit0_of_468c(0x50);
    k_pgdn  = bit0_of_468c(0x51);
    k_left  = bit0_of_468c(0x4b);
    k_right = bit0_of_468c(0x4d);
    k_home  = bit0_of_468c(0x47);
    k_up    = bit0_of_468c(0x48);
    k_pgup  = bit0_of_468c(0x49);

    if (k_home != 0 || k_up != 0 || k_pgup != 0) {
        moved = 1;
        DG16(0x576c) = (int16_t)(DG16(0x576c) - 2);
        if (DG16(0x576c) - DG16(0x577e) < 0)
            DG16(0x576c) = 0;
    }

    if (k_end != 0 || k_down != 0 || k_pgdn != 0) {
        moved = 1;
        DG16(0x576c) = (int16_t)(DG16(0x576c) + 2);
        if (DG16(0x576c) - DG16(0x577e) > (int16_t)(DG16(0x3f7c) - 1))
            DG16(0x576c) = (int16_t)(DG16(0x3f7c) - 1);
    }

    if (k_end != 0 || k_left != 0 || k_home != 0) {
        moved = 1;
        DG16(0x576e) = (int16_t)(DG16(0x576e) - 2);
        if (DG16(0x576e) - DG16(0x5780) < 0)
            DG16(0x576e) = 0;
    }

    if (k_pgdn != 0 || k_right != 0 || k_pgup != 0) {
        moved = 1;
        DG16(0x576e) = (int16_t)(DG16(0x576e) + 2);
        if (DG16(0x576e) - DG16(0x5780) > (int16_t)(DG16(0x3f7a) - 1))
            DG16(0x576e) = (int16_t)(DG16(0x3f7a) - 1);
    }

    if (moved != 0)
        mouse_move_to(DGU16(0x576e), DGU16(0x576c));

    if (DGU16(0x2d44) != 0 && DGU16(0x5752) == 0) {
        isr_stack_switch(1);
        redraw_cursor(DGU16(0x38a4));
        isr_stack_switch(0);
    }

    di = flag_bit_48ea(0);

    si = (bit0_of_468c(0x39) != 0 || bit0_of_468c(0x1c) != 0
          || bit0_of_468c(0x4c) != 0 || bit0_of_468c(0x52) != 0) ? 1 : 0;

    di |= (si != 0) ? 1 : 0;

    si = button_state(0, di);
    if (si <= 1)
        si = DG16(0x576a);
    DG16(0x576a) = (int16_t)(di | (si & 0xfffe));

    di = (DGU16(0x2d42) != 0 && flag_bit_48ea(1) != 0) ? 1 : 0;
    di |= bit0_of_468c(1);

    si = button_state(1, di);
    if (si <= 1)
        si = DG16(0x5768);
    DG16(0x5768) = (int16_t)(di | (si & 0xfffe));

    DG16(0x5740) = 0;
    DG16(0x5754) = 1;
}

/*
 * 0x0aa14
 *
 * Choose the mouse cursor: which bitmap, and where its hot spot is. The three
 * are kept at DGROUP 0x5770, 0x5780 and 0x577e, and a call that names what is
 * already showing does nothing at all - not even the redraw.
 *
 * A cursor of 0 means none, and then both hot-spot words are zeroed rather than
 * taking the arguments, so turning the cursor off cannot leave a stale offset
 * behind for the next one.
 *
 * DGROUP 0x5752 is raised over the redraw and put back afterwards. It is not a
 * simple flag: the value it had is *saved*, so a redraw inside a redraw leaves
 * the outer one's state alone when it finishes.
 */
void set_cursor(uint16_t bitmap, int16_t hot_y, int16_t hot_x)
{
    uint16_t saved;

    if (DGU16(0x5770) == bitmap && DG16(0x5780) == hot_y
        && DG16(0x577e) == hot_x)
        return;

    saved = DGU16(0x5752);
    DGU16(0x5752) = 1;

    DGU16(0x5770) = bitmap;

    if (bitmap == 0) {
        DG16(0x577e) = 0;
        DG16(0x5780) = 0;
    } else {
        DG16(0x5780) = hot_y;
        DG16(0x577e) = hot_x;
    }

    redraw_cursor(DGU16(0x38a4));

    DGU16(0x5752) = saved;
}

/*
 * 0x0aa76
 *
 * Put the pointer somewhere, clamped to the screen, and tell the driver.
 *
 * Each coordinate is pinned to 0 below and to the screen size less one above -
 * the width at DGROUP 0x3f7a and the height at 0x3f7c - so a caller may ask
 * for anything and the pointer stays on the screen. Both bounds are the
 * *game's* idea of the screen, which is why this follows a mode change: the
 * copy-protection screen sets 0x3f7c to 0x18f before it does any of this.
 *
 * The result is written to **two pairs**: 0x5784/0x5782, which is where the
 * game reads the pointer, and 0x576e/0x576c, which is where it remembers it.
 * Then `mouse_move_to` moves the driver's own cursor to match, so the three
 * agree.
 */
void move_pointer_to(int16_t x, int16_t y)
{
    if (x < 0)
        x = 0;
    else if ((int16_t)(DG16(0x3f7a) - 1) < x)
        x = (int16_t)(DG16(0x3f7a) - 1);

    if (y < 0)
        y = 0;
    else if ((int16_t)(DG16(0x3f7c) - 1) < y)
        y = (int16_t)(DG16(0x3f7c) - 1);

    DG16(0x5784) = x;
    DG16(0x576e) = x;
    DG16(0x5782) = y;
    DG16(0x576c) = y;

    mouse_move_to((uint16_t)x, (uint16_t)y);
}

/*
 * 0x0ab1f
 *
 * Draw the cursor on a page: put back what was under the last one, save what is
 * under the new one, draw it, and remember where.
 *
 * The page's slot holds both states at once - the *previous* one at +0x14 and
 * the current at +8 - and bits 0 and 1 of the byte at +0x1f say which of them
 * is live. That is what lets the erase happen after the save rather than before
 * it, so the two rectangles can overlap without the erase undoing the save.
 *
 * Clipping is set wide open first: DGROUP 0x3894 and 0x3898 to zero, 0x3896 and
 * 0x389a to the screen's size less one, and both page pointers at 0x38a6 and
 * 0x38a8 to the slot's own page - so the cursor is drawn on that page whichever
 * one is being shown.
 *
 * A slot buffer of zero means "nothing was saved", and then the erase is a
 * single `plot_pixel_clipped` of the byte at +0x1e instead of a rectangle - the
 * one-pixel case, which a saved rectangle would be wasteful for.
 *
 * DGROUP 0x2d3e turns the whole cursor off: with it clear nothing is drawn and
 * bit 1 of +0x13 is cleared instead, which is what `redraw_cursor` tests.
 */
void draw_cursor(uint16_t page)
{
    uint16_t slot = claim_page_slot(page);
    uint16_t saved;

    if (slot == 0)
        return;

    saved = DGU16(0x5752);
    DGU16(0x5752) = 1;

    restage_object_rect(page);
    save_or_restore_draw_state(1);

    DG16(0x38a6) = DG16(slot);
    DG16(0x38a8) = DG16(slot);
    DG8(0x3893) = 1;
    DG16(0x3898) = 0;
    DG16(0x3894) = 0;
    DG16(0x389a) = (int16_t)(DG16(0x3f7c) - 1);
    DG16(0x3896) = (int16_t)(DG16(0x3f7a) - 1);

    /* Put back what the last cursor covered. */
    if ((DG8((uint16_t)(slot + 0x1f)) & 2) != 0) {
        if (DGU16((uint16_t)(slot + 0x1c)) != 0) {
            if (DG16((uint16_t)(slot + 0x18)) > 0
                && DG16((uint16_t)(slot + 0x1a)) > 0) {
                uint16_t b = (uint16_t)(4 * DGU16((uint16_t)(slot + 0x1c)));

                restore_rect_thunk(DGU16((uint16_t)(0x5754 + b)),
                                   DGU16((uint16_t)(0x5756 + b)),
                                   DG16((uint16_t)(slot + 0x14)),
                                   DG16((uint16_t)(slot + 0x16)),
                                   DG16((uint16_t)(slot + 0x18)),
                                   DG16((uint16_t)(slot + 0x1a)));
            }
        } else {
            plot_pixel_clipped(DG16((uint16_t)(slot + 0x14)),
                               DG16((uint16_t)(slot + 0x16)),
                               (int16_t)DG8((uint16_t)(slot + 0x1e)));
        }
        DG8((uint16_t)(slot + 0x1f)) =
            (uint8_t)(DG8((uint16_t)(slot + 0x1f)) & 0xfd);
    }

    /* Save what the new one will cover. */
    if (DGU16(0x2d3e) != 0) {
        if (DGU16((uint16_t)(slot + 0x10)) != 0
            && DGU16((uint16_t)(slot + 2)) != 0) {
            if (DG16((uint16_t)(slot + 0x0c)) > 0
                && DG16((uint16_t)(slot + 0x0e)) > 0) {
                uint16_t b = (uint16_t)(4 * DGU16((uint16_t)(slot + 0x10)));

                save_rect_thunk(DGU16((uint16_t)(0x5754 + b)),
                                DGU16((uint16_t)(0x5756 + b)),
                                DG16((uint16_t)(slot + 8)),
                                DG16((uint16_t)(slot + 0x0a)),
                                DG16((uint16_t)(slot + 0x0c)),
                                DG16((uint16_t)(slot + 0x0e)));
            }
        } else {
            DG8((uint16_t)(slot + 0x12)) =
                (uint8_t)read_pixel_clipped(DG16((uint16_t)(slot + 8)),
                                            DG16((uint16_t)(slot + 0x0a)));
        }

        /* And draw it. */
        if (DGU16((uint16_t)(slot + 2)) != 0
            && DGU16((uint16_t)(slot + 0x10)) != 0) {
            int16_t y = DG16((uint16_t)(slot + 6));

            /*
             * On adapter 8 a negative y is nudged one further up before the
             * blit, and the x argument is replaced by zero.
             */
            if (DG8(0x38ad) == 8 && y < 0)
                draw_bitmap(DGU16((uint16_t)(slot + 2)),
                            DG16((uint16_t)(slot + 4)),
                            (int16_t)(y - 1), 0);
            else
                draw_bitmap(DGU16((uint16_t)(slot + 2)),
                            DG16((uint16_t)(slot + 4)), y, 0);
        } else {
            DG16(0x573e) = (int16_t)((DG16(0x573e) + 1) & 0x0f);
            plot_pixel_clipped(DG16((uint16_t)(slot + 4)),
                               DG16((uint16_t)(slot + 6)),
                               DG16(0x573e));
        }

        DG8((uint16_t)(slot + 0x13)) =
            (uint8_t)(DG8((uint16_t)(slot + 0x13)) | 2);
    } else {
        DG8((uint16_t)(slot + 0x13)) =
            (uint8_t)(DG8((uint16_t)(slot + 0x13)) & 0xfd);
    }

    save_or_restore_draw_state(0);

    /* Give back the buffer the erase used, if nothing else wants it. */
    if ((DG8((uint16_t)(slot + 0x1f)) & 1) != 0
        && DGU16((uint16_t)(slot + 0x1c)) != 0
        && DGU16(0x5740) == 0) {
        clear_slot_5734((int16_t)DGU16((uint16_t)(slot + 0x1c)));
        DGU16((uint16_t)(slot + 0x1c)) = 0;
        DG8((uint16_t)(slot + 0x1f)) =
            (uint8_t)(DG8((uint16_t)(slot + 0x1f)) & 0xfe);
    }

    DGU16(0x5752) = saved;
}

/*
 * 0x0acc3
 *
 * Redraw the cursor on a page, if anything about it has changed.
 *
 * The page's slot comes from `claim_page_slot`; a page with no slot is not
 * drawn on at all. Then the mouse's position is read into DGROUP 0x576e and
 * 0x576c - but only when DGROUP 0x2d42 says to, so a caller that has already
 * decided where the cursor goes can suppress it - and the hot spot is
 * subtracted to give the top-left corner at 0x56e2 and 0x56e4.
 *
 * The redraw is then skipped when all four of the slot's remembered values
 * still agree with what was just worked out **and** bit 1 of the slot's byte at
 * +0x13 is set. A cursor of 0 skips the comparison and always redraws, which is
 * how it gets erased.
 *
 * 0x5752 is raised across the whole thing and restored, the same nesting guard
 * `set_cursor` uses.
 */
void redraw_cursor(uint16_t page)
{
    uint16_t slot = claim_page_slot(page);
    uint16_t saved;

    if (slot == 0)
        return;

    saved = DGU16(0x5752);
    DGU16(0x5752) = 1;

    if (DGU16(0x2d42) != 0)
        read_pair_4740(0x576e, 0x576c);

    DG16(0x56e2) = (int16_t)(DG16(0x576e) - DG16(0x5780));
    DG16(0x56e4) = (int16_t)(DG16(0x576c) - DG16(0x577e));

    if (DGU16(0x5770) == 0
        || DG16((uint16_t)(slot + 4)) != DG16(0x56e2)
        || DG16((uint16_t)(slot + 6)) != DG16(0x56e4)
        || DGU16((uint16_t)(slot + 2)) != DGU16(0x5770)
        || (DG8((uint16_t)(slot + 0x13)) & 2) == 0)
        draw_cursor(page);

    DGU16(0x5752) = saved;
}

/*
 * 0x0b078
 *
 * **Redraw the cursor**, and with it everything the cursor was standing on.
 *
 * This is what the whole saved-rectangle machinery exists for. The cursor is
 * drawn over the picture, so before it can move, what it covered has to go
 * back - and because the game is double buffered, on both pages, in an order
 * that never leaves either half restored.
 *
 * The re-entry guard at DGROUP 0x5752 is raised for the whole routine and the
 * *entering* value put back at the end, not a zero, so a call from inside a
 * call leaves the flag as it found it.
 *
 * The order, which is the substance of it:
 *
 *   1  a pending move at 0x577a/0x577c is taken first and cleared, so the
 *      pointer is where it is going before anything is drawn. Either word
 *      being non-zero is enough to trigger it.
 *   2  the cursor is erased from the drawing page.
 *   3  when the pages differ, `show_page_thunk` is told whether nothing else
 *      is pending - no palette waiting at 0x2d3a and the fade already where
 *      0x5786 asks - so it can wait for retrace only when it is worth it.
 *   4  a palette waiting at 0x2d3a/0x2d3c is loaded, remembered at
 *      0x5738/0x573a, and **cleared**, so one request loads once. Loading one
 *      also resets 0x573c to zero, which forces the fade below to run.
 *   5  the fade runs only when 0x5786 differs from 0x573c, and 0x573c is then
 *      caught up.
 *   6  if 0x2d34 says the screen was disturbed, the saved rectangles for both
 *      pages and for the copy pair are given back, the whole screen is copied
 *      between pages, and the objects are moved or erased. Otherwise only the
 *      drawing page's objects are erased.
 *   7  when the pages are the same, the cursor goes back on, the two pages'
 *      object lists are swapped, each page's own saved rectangle is copied
 *      back through its slot, and the backdrop is restored between them.
 *   8  and whatever page it took, the lists of lists at 0x2d0a are put back.
 *
 * The `add sp, 6` after each `free_saved_rects` is the caller cleaning three
 * arguments; the middle call passes 0x2d32 as the page, where the other two
 * pass zero.
 */
void redraw_cursor_all(void)
{
    uint16_t was = DGU16(0x5752);

    DGU16(0x5752) = 1;

    if (DGU16(0x577c) != 0 || DGU16(0x577a) != 0) {
        move_pointer_to((int16_t)DGU16(0x577c), (int16_t)DGU16(0x577a));
        DGU16(0x577a) = 0;
        DGU16(0x577c) = 0;
    }

    draw_cursor(DGU16(0x38a2));

    if (DGU16(0x2d32) != 0) {
        uint16_t quiet =
            ((DGU16(0x2d3a) | DGU16(0x2d3c)) == 0
             && DGU16(0x5786) == DGU16(0x573c)) ? 1 : 0;

        show_page_thunk(quiet);
    }

    if ((DGU16(0x2d3a) | DGU16(0x2d3c)) != 0) {
        set_palette_pointer(DGU16(0x2d3a), DGU16(0x2d3c));
        DGU16(0x573a) = DGU16(0x2d3c);
        DGU16(0x5738) = DGU16(0x2d3a);
        DGU16(0x2d3c) = 0;
        DGU16(0x2d3a) = 0;
        DGU16(0x573c) = 0;
    }

    if (DGU16(0x5786) != DGU16(0x573c)) {
        fade_palette_run(DGU16(0x2d36), DGU16(0x2d38), 0, DGU16(0x5786));
        DGU16(0x573c) = DGU16(0x5786);
    }

    if (DGU16(0x2d34) == 0) {
        erase_object(DGU16(0x38a2));
    } else {
        if (DGU16(0x2d32) != 0) {
            DGU16(0x38a6) = DGU16(0x38a4);
            DGU16(0x38a8) = DGU16(0x38a2);
        } else {
            DGU16(0x38a6) = DGU16(0x38a2);
            DGU16(0x38a8) = DGU16(0x38a4);
        }

        free_saved_rects(DGU16(0x38a0), DGU16(0x38a2), 0);
        free_saved_rects(DGU16(0x38a0), DGU16(0x38a4), DGU16(0x2d32));
        free_saved_rects(DGU16(0x38a6), DGU16(0x38a8), 0);

        copy_rect_thunk(0, 0, DGU16(0x3f7a), DGU16(0x3f7c));

        if (DGU16(0x2d32) != 0) {
            restore_object_backdrop(DGU16(0x38a4), DGU16(0x38a2));
            clear_object_covered(DGU16(0x38a2));
        } else {
            erase_object(DGU16(0x38a2));
        }

        DGU16(0x2d34) = 0;
    }

    if (DGU16(0x2d32) == 0) {
        uint16_t rec;

        clear_object_covered(DGU16(0x38a4));
        draw_cursor(DGU16(0x38a2));
        swap_page_objects(DGU16(0x38a4), DGU16(0x38a2));

        DGU16(0x38a8) = DGU16(0x38a4);
        DGU16(0x38a6) = DGU16(0x38a2);

        rec = claim_page_slot(DGU16(0x38a4));
        if (rec != 0)
            copy_rect_thunk(DGU16((uint16_t)(rec + 8)),
                            DGU16((uint16_t)(rec + 0xa)),
                            DGU16((uint16_t)(rec + 0xc)),
                            DGU16((uint16_t)(rec + 0xe)));

        rec = claim_page_slot(DGU16(0x38a2));
        if (rec != 0)
            copy_rect_thunk(DGU16((uint16_t)(rec + 8)),
                            DGU16((uint16_t)(rec + 0xa)),
                            DGU16((uint16_t)(rec + 0xc)),
                            DGU16((uint16_t)(rec + 0xe)));

        restore_object_backdrop(DGU16(0x38a4), DGU16(0x38a2));
    }

    restore_saved_rect_lists(0);

    DGU16(0x5752) = was;
}

/*
 * 0x0b28e
 *
 * Copy a rectangle from the page on screen to the page being drawn to, with
 * the pointer out of the way.
 *
 * Both pages are asked whether their object - the mouse pointer - overlaps the
 * rectangle, and the two answers decide what has to be taken down and put back.
 * The usual way is: erase the pointer from the page being drawn to, copy, put
 * the shown page's backdrop back if it was covered, and draw the pointer again.
 *
 * There is a second way, taken only when DGROUP 0x2d32 is clear *and* the drawn
 * page's pointer is in the way: draw the pointer on the shown page first, copy,
 * and erase it from the shown page afterwards - so the copy carries the pointer
 * across rather than working around it.
 *
 * A rectangle with no width or no height is not copied, but everything else
 * still happens. DGROUP 0x5752 is pinned throughout and put back at the end.
 */
void copy_rect_around_cursor(int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t fp = dg_enter(0x0e);
    uint16_t saved = fp;                    /* [bp-0x0e] */
    uint16_t hit_draw = 0, hit_shown = 0;   /* [bp-2], [bp-4] */
    uint16_t si;

    DGU16(saved) = DGU16(0x5752);
    DGU16(0x5752) = 1;

    si = claim_page_slot(DGU16(0x38a6));
    if (si != 0 && (DG8((uint16_t)(si + 0x13)) & 2)
        && (int16_t)(x + w) > DG16((uint16_t)(si + 8))
        && (int16_t)(DG16((uint16_t)(si + 8))
                     + DG16((uint16_t)(si + 0x0c))) > x
        && (int16_t)(y + h) > DG16((uint16_t)(si + 0x0a))
        && (int16_t)(DG16((uint16_t)(si + 0x0a))
                     + DG16((uint16_t)(si + 0x0e))) > y)
        hit_shown = 1;

    si = claim_page_slot(DGU16(0x38a8));
    if (si != 0 && (DG8((uint16_t)(si + 0x13)) & 2)
        && (int16_t)(x + w) > DG16((uint16_t)(si + 8))
        && (int16_t)(DG16((uint16_t)(si + 8))
                     + DG16((uint16_t)(si + 0x0c))) > x
        && (int16_t)(y + h) > DG16((uint16_t)(si + 0x0a))
        && (int16_t)(DG16((uint16_t)(si + 0x0a))
                     + DG16((uint16_t)(si + 0x0e))) > y)
        hit_draw = 1;

    if (DG16(0x2d32) == 0 && hit_draw != 0) {
        draw_cursor(DGU16(0x38a6));

        if (w > 0 && h > 0)
            copy_rect_thunk((uint16_t)x, (uint16_t)y,
                            (uint16_t)w, (uint16_t)h);

        erase_object(DGU16(0x38a6));
    } else {
        if (hit_draw != 0)
            erase_object(DGU16(0x38a8));

        if (w > 0 && h > 0)
            copy_rect_thunk((uint16_t)x, (uint16_t)y,
                            (uint16_t)w, (uint16_t)h);

        if (hit_shown != 0) {
            restore_object_backdrop(DGU16(0x38a6), DGU16(0x38a8));
            clear_object_covered(DGU16(0x38a8));
        }

        if (hit_draw != 0)
            draw_cursor(DGU16(0x38a8));
    }

    DGU16(0x5752) = DGU16(saved);

    dg_leave(0x0e);
}

/*
 * 0x0b542
 *
 * The button state machine, one eight-byte record per button at DGROUP 0x5742:
 * the state at +0, whether it was down last time at +2, a press count at +4,
 * and a repeat delay at +6.
 *
 * The states are 0 up, 2 pressed, 4 clicked and 8 held. A release with the
 * state at 8 goes straight back to 0; otherwise the press count goes up and the
 * state becomes 2 the first time and 4 after that - which is what tells a click
 * from a double one.
 *
 * A change also latches where the pointer was, at DGROUP 0x5776 and 0x5778 -
 * from the driver when 0x2d42 says so, and from the last known position when it
 * does not - and reloads the delay from 0x2d40. The delay then counts down on
 * every call, and while it is still running *and* something has been pressed,
 * the answer is the raw button rather than the state, which is what holds a
 * click on screen long enough to be seen.
 */
int16_t button_state(uint16_t index, int16_t down)
{
    uint16_t si = (uint16_t)(0x5742 + index * 8);

    if (DG16((uint16_t)(si + 2)) != down) {
        DG16((uint16_t)(si + 2)) = down;

        if (down == 0) {
            if (DG16(si) == 8) {
                DG16(si) = 0;
            } else {
                DG16((uint16_t)(si + 4))++;
                if (DG16((uint16_t)(si + 4)) == 1 && DG16(si) != 2)
                    DG16(si) = 2;
                else
                    DG16(si) = 4;
            }
        }

        if (DGU16(0x2d42) != 0) {
            read_pair_4740(0x5778, 0x5776);
        } else {
            DGU16(0x5778) = DGU16(0x576e);
            DGU16(0x5776) = DGU16(0x576c);
        }

        DG16((uint16_t)(si + 6)) = DG16(0x2d40);
    }

    if (DG16((uint16_t)(si + 6)) != 0)
        DG16((uint16_t)(si + 6))--;

    if (DG16((uint16_t)(si + 6)) != 0 && DG16((uint16_t)(si + 4)) <= 0)
        return down;

    if (down != 0)
        DG16(si) = 8;
    else if (DG16((uint16_t)(si + 4)) == 0)
        DG16(si) = 0;

    DG16((uint16_t)(si + 4)) = 0;

    return DG16(si);
}

/*
 * 0x0b82c
 *
 * Switch the interrupt handler onto a stack of its own, and back: a non-zero
 * argument saves SS:SP at DGROUP 0x317e and puts SP at 0x2e7c inside DGROUP, a
 * zero one puts the saved pair back. The entry at 0x0b84b is the second half
 * reached directly.
 *
 * It does this by popping its own return address and argument off the stack,
 * changing SS:SP, and pushing them back - the only way to return onto a stack
 * you have just swapped.
 *
 * **The switch itself means nothing here.** The port's handler runs on a real
 * thread with a real stack of its own, which is what the private stack was for.
 * The two DGROUP words are still written, because anything else can read them.
 */
void isr_stack_switch(int16_t to_private)
{
    if (to_private != 0) {
        DGU16(0x317e) = DGROUP_SEG;
        DGU16(0x3180) = guest_sp;
        return;
    }

    /* The restore half at 0x0b84b: the saved pair goes back into SS:SP. */
}

/*
 * 0x0b859
 *
 * Set the mouse's mickeys-per-pixel, INT 33h AX=0x0f, the same value for both
 * axes: the argument goes into CX and DX alike. The start-up asks for 3.
 *
 * Nothing of it is in guest memory, so the port sends it to the IO boundary and
 * there is nothing here for the two artefacts to disagree about.
 */
void mouse_set_speed(uint16_t mickeys)
{
    io_mouse_set_speed(mickeys, mickeys);
}

/*
 * 0x0b93d
 *
 * `fread` into a **huge** pointer, one byte at a time, answering how many whole
 * items came in.
 *
 * A byte at a time because the destination may cross a segment: each one is
 * stored through the far pointer and then `huge_add_to` steps and renormalises
 * it - reached here by its near door at 0x0be7f.
 *
 * The count is `size * count` as a 32-bit product, and the answer is the bytes
 * actually read divided by the size, which is why a partial last item does not
 * count. The loop stops on the count running out or on `game_fgetc` answering
 * -1, and the test is made **before** the decrement, so a count of zero reads
 * nothing.
 */
uint32_t fread_huge(uint16_t dst_off, uint16_t dst_seg, uint16_t size_lo,
                    uint16_t size_hi, uint16_t count_lo, uint16_t count_hi,
                    uint16_t file)
{
    uint16_t fp = dg_enter(0xe);
    uint16_t dst = (uint16_t)(fp + 0xe - 8);   /* [bp-8], the far pointer */
    uint32_t total = long_multiply(((uint32_t)count_hi << 16) | count_lo,
                                   ((uint32_t)size_hi << 16) | size_lo);
    uint32_t got = 0;

    DG16(dst + 2) = (int16_t)dst_seg;
    DG16(dst) = (int16_t)dst_off;

    while (total != 0) {
        int16_t c;

        total--;

        dg_call(6);                       /* one argument and a far return */
        c = game_fgetc(file);
        dg_uncall(6);

        if (c == -1)
            break;

        *FAR_PTR(DGU16(dst + 2), DGU16(dst)) = (uint8_t)c;
        huge_add_to(dst, DGROUP_SEG, 1);
        got++;
    }

    dg_leave(0xe);
    return ulong_divide(got, ((uint32_t)size_hi << 16) | size_lo);
}

/*
 * 0x0b9c9
 *
 * Draw one bitmap scaled - the same choice `draw_bitmap` makes, from the same
 * marker in field 4, and the same normalisation of the header's far pointer
 * written back into it.
 *
 * Two of the four forms have no scaled blitter: 0xfffd, the already-scaled
 * one, and 0xffff, the offset-table one, both simply do nothing here. That is
 * the original's own silence and not a gap - a bitmap in either form is never
 * asked to be drawn scaled.
 */
void draw_bitmap_scaled(uint16_t hdr, int16_t x, int16_t y,
                        int16_t w, int16_t h, uint16_t mode)
{
    DGU16(hdr) = (uint16_t)(DGU16(hdr) + (DGU16((uint16_t)(hdr + 2)) >> 4));
    DGU16((uint16_t)(hdr + 2)) = (uint16_t)(DGU16((uint16_t)(hdr + 2)) & 0x0f);

    switch (DGU16((uint16_t)(hdr + 4))) {
    case 0xfffd:
    case 0xffff:
        return;
    case 0xfffe:
        blit_scaled_a(hdr, x, y, mode, w, h);
        return;
    default:
        blit_scaled_b(hdr, x, y, mode, w, h);
        return;
    }
}

/*
 * 0x093a2
 *
 * The game's own `ftell`, and the third of the trio over the archive - the same
 * choice between the loose file and the archive as `game_fread` and
 * `game_fseek`, and the same substitution when an entry carries its own `FILE`.
 *
 * Inside an archive entry the answer is the entry's own position at +0xa:+0xc,
 * not the file's, so a caller sees the resource as if it were a file of its
 * own.
 */
int32_t game_ftell(uint16_t file)
{
    uint16_t si = 0;

    if (DG16(0x547e) != 0)
        si = archive_entry_for(file);

    if (si == 0)
        return stdio_ftell(file);

    if (DGU16(si + 0x10) != 0)
        return stdio_ftell(DGU16(si + 0x10));

    return (int32_t)(((uint32_t)DGU16(si + 0xc) << 16) | DGU16(si + 0xa));
}

/*
 * 0x093f6
 *
 * The game's own `fgetc`, and `game_fread`'s shape one byte at a time: the same
 * choice between the loose file and the archive, the same substitution when an
 * entry carries a `FILE` at +0x10, the same table at DGROUP 0x549f.
 *
 * The two DGROUP cells it sets on the way through - 0x548d with the `FILE` it
 * was asked about and 0x548b with the one it actually read from - are written
 * on every path, including the plain one, so something downstream reads them.
 *
 * At the end of an entry it answers -1 without touching the file at all. The
 * test is a 32-bit compare of the position at +0xa:+0xc against the size at
 * +6:+8, written high half first.
 *
 * Both the entry position and the archive's running total advance by one.
 */
int16_t game_fgetc(uint16_t file)
{
    uint16_t si = 0;

    DG16(0x548d) = (int16_t)file;

    if (DG16(0x547e) != 0)
        si = archive_entry_for(file);

    if (si == 0) {
        DG16(0x548b) = (int16_t)file;
        return stdio_fgetc(file);
    }

    if (DGU16(si + 0x10) != 0) {
        DG16(0x548b) = (int16_t)DGU16(si + 0x10);
        return stdio_fgetc(DGU16(si + 0x10));
    }

    if (DGU16(si + 0xc) > DGU16(si + 8)
        || (DGU16(si + 0xc) == DGU16(si + 8)
            && DGU16(si + 0xa) >= DGU16(si + 6)))
        return -1;

    make_file_current(DGU16(si));

    {
        uint16_t lo = (uint16_t)(DGU16(si + 2) + DGU16(si + 0xa));
        uint16_t hi = (uint16_t)(DGU16(si + 4) + DGU16(si + 0xc)
                                 + (lo < DGU16(si + 2) ? 1 : 0));
        int16_t got;
        uint16_t t;

        seek_file_to(lo, hi);

        file = DGU16(0x549f + 0x1c * DGU16(si));
        DG16(0x548b) = (int16_t)file;
        got = stdio_fgetc(file);

        DG16(si + 0xa) = (int16_t)(DGU16(si + 0xa) + 1);
        if (DGU16(si + 0xa) == 0)
            DG16(si + 0xc) = (int16_t)(DGU16(si + 0xc) + 1);

        t = (uint16_t)(0x54a1 + 0x1c * DGU16(si));
        DG16(t) = (int16_t)(DGU16(t) + 1);
        if (DGU16(t) == 0)
            DG16(t + 2) = (int16_t)(DGU16(t + 2) + 1);

        return got;
    }
}

/*
 * 0x08fc3
 *
 * Takes one argument, ignores it, and answers 1. Six instructions: a frame,
 * `mov ax, 1`, and a jump to the epilogue that goes nowhere.
 *
 * Both its callers are on a failure path in the file layer - one of them after
 * a failed open, having first checked three flags - and it sits immediately
 * before `game_fopen` in the same source file. That reads like the routine
 * that would have asked the user what to do about a disk error, left answering
 * "carry on" in the shipped build. **That is a reading of where it is called
 * from, not something the bytes say**: what the bytes say is that it answers 1.
 */
int16_t answer_carry_on(uint16_t what)
{
    (void)what;
    return 1;
}

/*
 * 0x08fcd
 *
 * The game's own `fopen`. Answers one of the ten 0x12-byte archive-entry
 * blocks at DGROUP 0x55c3, not a `FILE` - which is why every one of
 * `game_fread`, `game_fgetc`, `game_fseek` and `game_ftell` starts by asking
 * `archive_entry_for` whether the thing it was handed is one of these.
 *
 * With no archive loaded it is a plain forward to the runtime's `fopen` and the
 * caller gets a real `FILE`.
 *
 * Otherwise a free block is found - +0xe is the in-use flag - and the name
 * hashed, which leaves the hash at DGROUP 0x5482 for `find_entry_for_pointer`
 * to look up. Then the **loose file is tried first**: if a real file of that
 * name exists it is opened and the block simply carries its `FILE` at +0x10.
 * That is how a patched or unpacked file overrides the archive.
 *
 * Failing that the archive is searched. The entry's own header is read - a
 * 13-byte name and a 4-byte size - and the name compared case-insensitively
 * against the one asked for, which is the check that the hash found the right
 * file rather than a colliding one. The entry's data then starts where `ftell`
 * says the file now is.
 *
 * The retry loop around the loose-file open exists for removable media: 0x5488
 * is set by the critical-error handler and 0x38ad says whether to prompt. Both
 * are dead here.
 */
uint16_t game_fopen(uint16_t name, uint16_t mode)
{
    uint16_t fp = dg_enter(0x14);
    uint16_t bp = (uint16_t)(fp + 0x14);
    uint16_t hdr = (uint16_t)(bp - 0x10);
    uint16_t si, di;
    int16_t left;
    uint16_t r = 0;

    if (DG8(0x5487) != 0)
        make_file_current(0);

    load_archive_map();
    DG16(0x567b) = 0;

    if (DG16(0x547e) == 0) {
        r = stdio_fopen(name, mode);
        goto out;
    }

    DG16(0x548b) = 0;
    DG16(0x548d) = 0;

    si = 0x55c3;
    for (left = 0xa; left != 0; left--) {
        if (DGU16(si + 0xe) == 0)
            break;
        si = (uint16_t)(si + 0x12);
    }

    if (left == 0)
        goto out;

    dg_call(6);                           /* one argument and a far return */
    hash_filename(name);
    dg_uncall(6);

    DG8(0x5489) = 1;

    for (;;) {
        DG8(0x5488) = 0;
        di = stdio_fopen(name, mode);

        if (DG16(0x4e85) != 0) {
            r = di;
            goto out;
        }
        if (DG8(0x5488) != 0 && DG8(0x38ad) != 0)
            not_transcribed("0x08fc3, the prompt for a missing disk");
        if (DG8(0x5488) == 0)
            break;
    }

    DG8(0x5489) = 0;

    if (di != 0) {
        DG16(si) = 0;
        DG16(si + 0xc) = 0;
        DG16(si + 0xa) = 0;
        DG16(si + 8) = 0;
        DG16(si + 6) = 0;
        DG16(si + 4) = 0;
        DG16(si + 2) = 0;
        DG16(si + 0xe) = 1;
        DG16(si + 0x10) = (int16_t)di;
        goto found;
    }

    dg_call(6);                           /* one argument and a far return */
    {
        int16_t ok = find_entry_for_pointer(si);

        dg_uncall(6);
        if (ok == 0)
            goto out;
    }

    make_file_current(DGU16(si));

    {
        uint16_t lo = (uint16_t)(DGU16(si + 2) + DGU16(si + 0xa));
        uint16_t hi = (uint16_t)(DGU16(si + 4) + DGU16(si + 0xc)
                                 + (lo < DGU16(si + 2) ? 1 : 0));
        int32_t pos;
        uint16_t t;

        seek_file_to(lo, hi);

        di = DGU16(0x549f + 0x1c * DGU16(0x5480));

        stdio_fread(hdr, 0xd, 1, di);
        stdio_fread((uint16_t)(si + 6), 4, 1, di);

        pos = stdio_ftell(di);
        DG16(si + 4) = (int16_t)((uint32_t)pos >> 16);
        DG16(si + 2) = (int16_t)pos;

        t = (uint16_t)(0x54a1 + 0x1c * DGU16(0x5480));
        DG16(t + 2) = (int16_t)((uint32_t)pos >> 16);
        DG16(t) = (int16_t)pos;
    }

    if (string_compare_nocase(hdr, name) != 0)
        goto out;

    DG16(si + 0xc) = 0;
    DG16(si + 0xa) = 0;
    DG16(si + 0x10) = 0;
    DG16(si + 0xe) = 1;

found:
    DG8(0x5486) = (uint8_t)(DG8(0x5486) + 1);
    r = si;

out:
    dg_leave(0x14);
    return r;
}

/*
 * 0x0960f
 *
 * Load `RESOURCE.MAP`, which is what tells the game where everything in the
 * archives is. Runs once - DGROUP 0x548a is the flag that says so.
 *
 * Before opening anything it takes over **INT 24h**, DOS's critical-error
 * handler, keeping the old vector at DGROUP 0x5677. That is what stops a
 * missing disk from aborting the program, and the handler it installs is at
 * 0x9bdf. The segment pushed for it reads 0x0000 in the image and is a
 * relocation; the port works it out from where the program is.
 *
 * The file itself is four bytes into the table at DGROUP 0x28d2 - the byte
 * offsets `hash_filename` packs, so the hash function is **defined by the
 * file**, not by the program - then a count of archives, and then for each
 * archive a 13-byte name into its 0x1c-byte record at DGROUP 0x548f, a count of
 * entries, and a block of eight bytes per entry holding a hash and an offset.
 *
 * The block is one entry longer than the count, which leaves room for the
 * terminator the lookup relies on.
 *
 * The count of archives **accumulates** into DGROUP 0x547e, and the first index
 * of this map is worked out from it afterwards, so a second map would append
 * rather than replace.
 */
void load_archive_map(void)
{
    uint16_t fp = dg_enter(0x16);
    uint16_t bp = (uint16_t)(fp + 0x16);
    uint16_t count = (uint16_t)(bp - 8);      /* [bp-8] */
    uint16_t lo = (uint16_t)(bp - 0xc);       /* [bp-0xc] */
    uint16_t hi = (uint16_t)(bp - 0x10);      /* [bp-0x10] */
    uint16_t file, di;
    uint32_t v;

    if (DG8(0x548a) != 0) {
        dg_leave(0x16);
        return;
    }

    v = dos_getvect(0x24);
    DG16(0x5679) = (int16_t)(v >> 16);
    DG16(0x5677) = (int16_t)v;

    dos_setvect(0x24, 0x9bdf, (uint16_t)(IMAGE_BASE >> 4));
    DG8(0x548a) = 1;

    file = stdio_fopen(0x28d6, 0x28e3);
    if (file == 0) {
        dg_leave(0x16);
        return;
    }

    stdio_fread(0x28d2, 4, 1, file);
    stdio_fread(count, 2, 1, file);

    DG16(0x547e) = (int16_t)(DGU16(0x547e) + DGU16(count));
    di = (uint16_t)(DGU16(0x547e) - DGU16(count) + 1);

    for (; (int16_t)di <= DG16(0x547e); di++) {
        uint16_t rec = (uint16_t)(0x548f + 0x1c * di);
        uint16_t blk_off, blk_seg;
        uint32_t p;

        stdio_fread(rec, 0xd, 1, file);
        stdio_fread(count, 2, 1, file);

        p = dos_alloc_bytes((uint16_t)((DGU16(count) + 1) << 3), 0, 1, 0);
        blk_off = (uint16_t)p;
        blk_seg = (uint16_t)(p >> 16);

        DG16(rec + 0x1a) = (int16_t)blk_seg;
        DG16(rec + 0x18) = (int16_t)blk_off;
        DG16(rec + 0xe) = (int16_t)di;

        while (DGU16(count) != 0) {
            uint8_t *e;

            DG16(count) = (int16_t)(DGU16(count) - 1);

            stdio_fread(lo, 4, 1, file);
            stdio_fread(hi, 4, 1, file);

            e = FAR_PTR(blk_seg, blk_off);
            *(uint16_t *)(e + 2) = DGU16(lo + 2);
            *(uint16_t *)e = DGU16(lo);
            *(uint16_t *)(e + 6) = DGU16(hi + 2);
            *(uint16_t *)(e + 4) = DGU16(hi);

            blk_off = (uint16_t)(blk_off + 8);
        }
    }

    stdio_fclose(file);
    dg_leave(0x16);
}

/*
 * 0x0980d
 *
 * Hash a filename, answering the hash in DX:AX and leaving it at DGROUP
 * 0x5482 as well. A null name answers zero and stores zero.
 *
 * The name is **uppercased in place**, in the caller's own buffer, and any
 * `\\` or `:` restarts the two running values and moves the start of the name
 * past it - so only the last path component counts and the caller's pointer is
 * left pointing at it.
 *
 * Two things are accumulated over the name: a sum and an exclusive-or. Then the
 * last component is copied into a 13-byte buffer, padded with zeros, and four
 * of its bytes - at the offsets in the table at DGROUP 0x28d2 - are packed into
 * a 32-bit value eight bits at a time. The sum times the exclusive-or is added
 * to that, **as a 16-bit product sign-extended**: the `imul` computes 32 bits
 * and the `cwd` after it throws the top half away, which is the compiler
 * treating the result as an `int`.
 */
int32_t hash_filename(uint16_t name)
{
    uint16_t fp = dg_enter(0x16);
    uint16_t bp = (uint16_t)(fp + 0x16);
    uint16_t buf = (uint16_t)(bp - 0x16);
    uint16_t si;
    uint16_t sum = 0, eor = 0;
    uint32_t acc = 0;
    int16_t i;

    if (name == 0) {
        DG16(0x5484) = 0;
        DG16(0x5482) = 0;
        dg_leave(0x16);
        return 0;
    }

    si = name;
    while (DG8(si) != 0) {
        uint8_t c;

        if (DG8(si) >= 'a' && DG8(si) <= 'z')
            DG8(si) = (uint8_t)(DG8(si) ^ 0x20);

        c = DG8(si);
        sum = (uint16_t)(sum + c);
        eor ^= c;

        if (DG8(si) == '\\' || DG8(si) == ':') {
            eor = 0;
            sum = 0;
            name = (uint16_t)(si + 1);
        }
        si++;
    }

    string_copy_padded(buf, name, 0xd);

    for (i = 0; i < 4; i++) {
        uint8_t c = DG8((uint16_t)(buf + DG8((uint16_t)(0x28d2 + i))));

        acc = long_shift_left(acc, 8) + c;
    }

    acc = (uint32_t)((int32_t)acc
                     + (int32_t)(int16_t)(sum * eor));

    DG16(0x5484) = (int16_t)(acc >> 16);
    DG16(0x5482) = (int16_t)acc;

    dg_leave(0x16);
    return (int32_t)acc;
}

/*
 * 0x098e0
 *
 * Find which archive holds a file, and answer whether one does.
 *
 * What is looked for is not an argument - it is the **filename hash** at
 * 0x5482, which `hash_filename` leaves there. Records are 0x1c bytes from
 * 0x54a7, and each one's first field is a far pointer to the list of
 * eight-byte entries `load_archive_map` read: the hash at +0 and the file's
 * offset within the archive at +4, ending at an all-zero hash. 0x5480 holds the
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
 * On success the caller's block takes the record index, the file's offset, and
 * two zeroed 32-bit fields at +6 and +0xa.
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
 * 0x09a62
 *
 * Make a given resource file the open one, opening it and closing whatever was
 * open before.
 *
 * The first thing it does is a **file-exists test written as an open and an
 * immediate close**: with the flag at 0x5486 clear it tries the file by name
 * and shuts it again, keeping only whether that worked. That is the loose-file
 * probe - the game asks whether a real file is there before settling for the
 * packed copy - and finding one forces a reopen even when the same index is
 * already current.
 *
 * Without that, an index that is already current returns immediately, which is
 * why the routine is cheap enough to call before every read: 18,930 calls, 26
 * of which reach DOS.
 *
 * The open itself retries forever. A failure calls the prompt at 0x08fc3 - but
 * only while 0x38ad is set - and tries again, which is how a program on
 * removable media asks for the right disk. With 0x38ad clear it spins on
 * `fopen` with nothing to change the answer.
 *
 * Afterwards the believed file position at +0x12 is zeroed, because a freshly
 * opened file is at nought, and `archive_entry_for(0)` is called to throw away
 * the one-entry cache - the `FILE` pointers it remembers are about to be stale.
 *
 * The fast path above is the common one - 18,930 calls against 26 that open
 * anything - but every occurrence the harness samples is off it, which is why
 * this needed the runtime's own `fopen` before it could be checked at all.
 */
void make_file_current(uint16_t index)
{
    uint16_t si;
    int16_t exists = 0;

    if (DG8(0x5486) == 0 && index != 0) {
        uint16_t f = stdio_fopen((uint16_t)(0x548f + 0x1c * index), 0x28e6);

        stdio_fclose(f);
        if (f != 0)
            exists = 1;
    }

    if (index == DGU16(0x5480) && exists == 0 && DG8(0x5487) == 0)
        return;

    si = (uint16_t)(0x548f + 0x1c * DGU16(0x5480));
    if (DGU16(si + 0x10) != 0) {
        stdio_fclose(DGU16(si + 0x10));
        DG16(si + 0x10) = 0;
    }

    DG16(0x5480) = (int16_t)index;
    si = (uint16_t)(0x548f + 0x1c * DGU16(0x5480));

    if (index != 0) {
        DG8(0x5489) = 1;
        for (;;) {
            uint16_t f = stdio_fopen(si, 0x28e9);

            DG16(si + 0x10) = (int16_t)f;
            if (f != 0)
                break;
            if (DG8(0x38ad) != 0)
                not_transcribed("0x08fc3, the prompt for a missing disk");
        }
        DG8(0x5489) = 0;
    }

    DG16(si + 0x14) = 0;
    DG16(si + 0x12) = 0;

    archive_entry_for(0);
    DG8(0x5487) = 0;
}

/*
 * 0x09b38
 *
 * Put a file at a given position, without asking DOS if it is already there.
 *
 * The record is the 0x1c-byte entry at DGROUP 0x548f selected by 0x5480 - the
 * same table `find_entry_for_pointer` walks, four bytes lower. Its +0x12 holds
 * the position DOS is believed to be at, as a 32-bit value, and a seek to that
 * same place does nothing at all.
 *
 * That cache is why the loader can afford to ask for a seek before every read:
 * measured over a run, 18,930 calls reach DOS 319 times. The archive is read
 * forward, so the believed position is nearly always right.
 *
 * The 319 that do reach DOS go through the runtime's own `fseek`, always from
 * the start of the file. That used to be a stand-in in io.c that did nothing,
 * with the caller verified only on occurrences where the buffer does not move;
 * it is the real routine now.
 */
void seek_file_to(uint16_t lo, uint16_t hi)
{
    uint16_t rec = (uint16_t)(0x548f + 0x1c * DGU16(0x5480));

    if (DGU16(rec + 0x14) == hi && DGU16(rec + 0x12) == lo)
        return;

    stdio_fseek(DGU16(rec + 0x10), lo, hi, 0);

    DG16(rec + 0x14) = (int16_t)hi;
    DG16(rec + 0x12) = (int16_t)lo;
}

/*
 * 0x09b7c
 *
 * Find the archive entry standing in for an open file, or answer null if the
 * file is a real one.
 *
 * This is the pivot of the loader's two-way lookup: the game asks for each
 * resource as a loose file first and falls back to the packed archive, and this
 * is what tells the two apart afterwards. Everything above it - `fread`,
 * `fseek`, `ftell` - checks here before deciding whether to touch DOS.
 *
 * The table is ten entries of 0x12 bytes at DGROUP 0x55c3, keyed by the `FILE`
 * pointer itself, and there is a **one-entry cache** in front of it: 0x547a
 * holds the last pointer asked about and 0x547c the answer. A repeat question
 * is answered without walking anything, which matters because the read path
 * asks on every call.
 *
 * A null pointer clears the cache and answers null - that is how the cache is
 * invalidated when a file is closed.
 *
 * Two things end the walk with no match: running out of entries, and finding
 * one whose +0xe is zero. The second is checked **after** the loop rather than
 * inside it, so an entry matching the pointer but not yet open is found and
 * then rejected. Both paths also clear 0x547a, so the negative answer is not
 * cached - only positive ones are.
 */
uint16_t archive_entry_for(uint16_t file)
{
    uint16_t si;
    int16_t n;

    if (file == 0) {
        DG16(0x547a) = 0;
        DG16(0x547c) = 0;
        return 0;
    }

    if (DG16(0x547e) == 0)
        return 0;

    if (file == DGU16(0x547a))
        return DGU16(0x547c);

    DG16(0x547a) = (int16_t)file;

    si = 0x55c3;
    n = 0xa;
    while (n != 0 && si != file) {
        si = (uint16_t)(si + 0x12);
        n--;
    }

    if (n == 0 || DG16(si + 0xe) == 0) {
        si = 0;
        DG16(0x547a) = 0;
    }

    DG16(0x547c) = (int16_t)si;
    return si;
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
 * 0x0ad51
 *
 * Put back whatever an object was covering, and mark it no longer drawn.
 *
 * Bit 1 of the byte at +0x13 says the object is currently on screen; with it
 * clear this does nothing but the bookkeeping around it. With it set there are
 * two ways back. If the object has a buffer slot at +0x10 and a rectangle with
 * both extents positive, the saved pixels are put back with `vm_restore_rect`,
 * the buffer being the far pointer at DGROUP 0x5754 + 4 * slot - one-based, as
 * `claim_buffer_slot` hands them out. Otherwise a single pixel is replaced from
 * the colour byte at +0x12, which is what `restage_object_rect` left there.
 *
 * The rectangle used is the **clipped** one at +8, not the unclipped position
 * at +4, so an object partly off-screen restores only the part that was drawn.
 *
 * The two words at 0x38a6 and 0x38a8 are both set from the record's first word
 * before any of that. They are inside the driver's data block, and the same
 * value goes to both.
 *
 * `save_or_restore_draw_state` brackets the whole thing, and the global at
 * 0x5752 is forced to 1 for the duration and put back at the end - the same
 * pattern `restage_object_rect` uses.
 */
void erase_object(uint16_t handle)
{
    uint16_t rec, slot;
    int16_t saved;

    rec = claim_page_slot(handle);
    if (rec == 0)
        return;

    saved = DG16(0x5752);
    DG16(0x5752) = 1;

    save_or_restore_draw_state(1);

    DG16(0x38a6) = DG16(rec);
    DG16(0x38a8) = DG16(rec);

    if ((DG8(rec + 0x13) & 2) != 0) {
        if (DG16(rec + 0x10) != 0 && DG16(rec + 0xc) > 0
            && DG16(rec + 0xe) > 0) {
            slot = (uint16_t)(4 * DGU16(rec + 0x10));
            vm_restore_rect(DGU16(0x5754 + slot), DGU16(0x5756 + slot),
                            DG16(rec + 8), DG16(rec + 0xa),
                            DG16(rec + 0xc), DG16(rec + 0xe));
        } else {
            plot_pixel_clipped(DG16(rec + 8), DG16(rec + 0xa),
                               DG8(rec + 0x12));
        }
        DG8(rec + 0x13) &= 0xfd;
    }

    save_or_restore_draw_state(0);
    DG16(0x5752) = saved;
}

/*
 * 0x0adf1
 *
 * Put back what an object covered on one page, and make the other page the
 * one being drawn to.
 *
 * The slot's +0x10 says where the backdrop was kept: non-zero and it is a far
 * pointer in the pair of tables at DGROUP 0x5754 and 0x5756, four bytes apart
 * per slot, and the rectangle goes back whole. Zero and there was only ever one
 * pixel, whose colour is the byte at +0x12.
 *
 * The draw state is saved across it and DGROUP 0x5752 pinned, which is what
 * stops the cursor being redrawn in the middle.
 */
void restore_object_backdrop(uint16_t from_page, uint16_t to_page)
{
    uint16_t fp = dg_enter(2);
    uint16_t saved = fp;                        /* [bp-2] */
    uint16_t si = claim_page_slot(from_page);

    if (si == 0)
        goto out;

    DGU16(saved) = DGU16(0x5752);
    DGU16(0x5752) = 1;

    save_or_restore_draw_state(1);

    DGU16(0x38a6) = to_page;
    DGU16(0x38a8) = to_page;

    if (DG8((uint16_t)(si + 0x13)) & 2) {
        if (DGU16((uint16_t)(si + 0x10)) != 0
            && DG16((uint16_t)(si + 0x0c)) > 0
            && DG16((uint16_t)(si + 0x0e)) > 0) {
            uint16_t bx = (uint16_t)(DGU16((uint16_t)(si + 0x10)) << 2);

            restore_rect_thunk(DGU16((uint16_t)(bx + 0x5754)),
                               DGU16((uint16_t)(bx + 0x5756)),
                               DG16((uint16_t)(si + 8)),
                               DG16((uint16_t)(si + 0x0a)),
                               DG16((uint16_t)(si + 0x0c)),
                               DG16((uint16_t)(si + 0x0e)));
        } else {
            plot_pixel_clipped(DG16((uint16_t)(si + 8)),
                               DG16((uint16_t)(si + 0x0a)),
                               (int16_t)DG8((uint16_t)(si + 0x12)));
        }
    }

    save_or_restore_draw_state(0);
    DGU16(0x5752) = DGU16(saved);

out:
    dg_leave(2);
}

/*
 * 0x0ae8e
 *
 * **Swap the object lists of two pages.** Each page's slot holds the head of a
 * list of saved rectangles; this exchanges the two heads, so everything drawn
 * over one page is now attributed to the other.
 *
 * Nothing happens unless both pages have slots: `claim_page_slot` is asked for
 * each, and either answering zero leaves the two lists alone.
 *
 * The swap is done with the re-entry guard at DGROUP 0x5752 raised and put
 * back afterwards, because for the two instructions between the two stores
 * neither list is whole - one head is in a local and the other is in both
 * slots - and a cursor redraw arriving there would walk it.
 */
void swap_page_objects(uint16_t page_a, uint16_t page_b)
{
    uint16_t slot_a = claim_page_slot(page_b);
    uint16_t slot_b;
    uint16_t was;
    uint16_t head;

    if (slot_a == 0)
        return;

    slot_b = claim_page_slot(page_a);
    if (slot_b == 0)
        return;

    was = DGU16(0x5752);
    DGU16(0x5752) = 1;

    head = DGU16(slot_a);
    DGU16(slot_a) = DGU16(slot_b);
    DGU16(slot_b) = head;

    DGU16(0x5752) = was;
}

/*
 * 0x0aedc
 *
 * Say a page's object no longer covers anything: clear bit 1 of the slot's
 * +0x13. A page with no slot is left alone.
 */
void clear_object_covered(uint16_t page)
{
    uint16_t si = claim_page_slot(page);

    if (si != 0)
        DG8((uint16_t)(si + 0x13)) &= 0xfd;
}

/*
 * 0x0aef6
 *
 * Age an object's on-screen rectangle by one frame: copy where it is now into
 * where it was, then work out where it is now from the current globals and clip
 * that to the screen.
 *
 * The record has two parallel blocks. The current one runs from +8 - x, y, w, h
 * at +8/+0xa/+0xc/+0xe, a buffer slot at +0x10, and two bytes at +0x12/+0x13 -
 * and the previous one from +0x14 with the same shape, its bytes at
 * +0x1e/+0x1f. Copying one onto the other is the whole of the first half.
 *
 * Before that copy it may hand back a buffer slot, and **only the
 * `clear_slot_5734` call survives**: the two stores beside it, zeroing +0x1c
 * and clearing bit 0 of +0x1f, are both overwritten a few instructions later by
 * the copy. They are dead as written, and transcribed anyway.
 *
 * If the object's parent at +2 no longer matches the global at 0x5770 it is
 * re-parented, which means asking the driver how big the new parent's image is
 * and claiming a scratch buffer for it. `claim_buffer_slot` ignores the size it
 * is handed, so that measurement goes nowhere - see 0x0b5ed. A null parent
 * gives slot zero and a 1 by 1 rectangle.
 *
 * The unclipped position goes to +4/+6 and is kept; the clipped copy goes to
 * +8. Clipping is one-sided in the usual way: a negative coordinate is pulled
 * to zero and taken out of the extent, and an extent running past 0x3f7a or
 * 0x3f7c - the screen width and height - is cut back to the edge. Nothing stops
 * an extent going negative if the rectangle is entirely off-screen.
 *
 * The global at 0x5752 is set to 1 for the duration and put back at the end,
 * and 0x5740 being non-zero suppresses both the slot release and the
 * re-parenting.
 */
void restage_object_rect(uint16_t handle)
{
    uint16_t rec, parent;
    int16_t saved, x, y, w, h;

    rec = claim_page_slot(handle);
    if (rec == 0)
        return;

    saved = DG16(0x5752);
    DG16(0x5752) = 1;

    if ((DG8(rec + 0x1f) & 1) != 0 && DG16(rec + 0x1c) != 0
        && DG16(0x5740) == 0) {
        clear_slot_5734(DG16(rec + 0x1c));
        DG16(rec + 0x1c) = 0;
        DG8(rec + 0x1f) &= 0xfe;
    }

    DG16(rec + 0x14) = DG16(rec + 8);
    DG16(rec + 0x16) = DG16(rec + 0xa);
    DG16(rec + 0x18) = DG16(rec + 0xc);
    DG16(rec + 0x1a) = DG16(rec + 0xe);
    DG16(rec + 0x1c) = DG16(rec + 0x10);
    DG8(rec + 0x1f) = DG8(rec + 0x13);
    DG8(rec + 0x1e) = DG8(rec + 0x12);

    if (DGU16(rec + 2) != DGU16(0x5770) && DG16(0x5740) == 0) {
        DG8(rec + 0x1f) |= 1;
        DG16(rec + 2) = DG16(0x5770);

        if (DGU16(0x5770) != 0) {
            int32_t asked;

            parent = DGU16(0x5770);
            asked = (int16_t)(uint16_t)vm_buffer_size(DGU16(parent + 6),
                                                      DGU16(parent + 8));
            DG16(rec + 0x10) = claim_buffer_slot((uint16_t)asked,
                                                 (uint16_t)(asked >> 16), 0, 0);
        } else {
            DG16(rec + 0x10) = 0;
        }
    }

    if (DG16(0x2d42) != 0)
        read_pair_4740(0x576e, 0x576c);

    x = (int16_t)(DG16(0x576e) - DG16(0x5780));
    y = (int16_t)(DG16(0x576c) - DG16(0x577e));

    if (DGU16(0x5770) != 0) {
        parent = DGU16(0x5770);
        w = DG16(parent + 6);
        h = DG16(parent + 8);
    } else {
        h = 1;
        w = 1;
    }

    DG16(rec + 4) = x;
    DG16(rec + 6) = y;

    if (x < 0) {
        w = (int16_t)(w + x);
        x = 0;
    }
    if (x + w >= DG16(0x3f7a))
        w = (int16_t)(DG16(0x3f7a) - x);
    if (y < 0) {
        h = (int16_t)(h + y);
        y = 0;
    }
    if (y + h >= DG16(0x3f7c))
        h = (int16_t)(DG16(0x3f7c) - y);

    DG16(rec + 8) = x;
    DG16(rec + 0xa) = y;
    DG16(rec + 0xc) = w;
    DG16(rec + 0xe) = h;

    DG16(0x5752) = saved;
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

/*
 * 0x0b5ed
 *
 * Make sure the four scratch buffers exist, then claim a free one and answer
 * its **one-based** index, or -1 if all four are taken. `clear_slot_5734` is
 * the release.
 *
 * The four buffers are far pointers at DGROUP 0x5758, four bytes apart; the
 * four in-use bytes are at 0x5734, which is why that array is one-based - zero
 * is the "no slot" answer. Any buffer still null is allocated on the way past,
 * so the first call does all four allocations and later ones do none.
 *
 * The size is the word at 0x5756, or, if that is zero, whatever the driver
 * says a 64 by 64 planar image needs - reached through the thunk at 0x21ab9,
 * which is `vm_buffer_size` here. Only the low word of the driver's DX:AX
 * answer is kept, and it is then sign-extended by `cwd` into the 32-bit size
 * DOS is asked for, so a size at or above 0x8000 would be asked for as a
 * negative length. Nothing seen produces one.
 *
 * **The four argument words are ignored.** The routine opens by loading each
 * of the two pairs and storing them straight back where they came from, which
 * is a no-op, and then never reads them again. The caller at 0x0aef6 goes to
 * the trouble of asking the driver for a size and passing it in, and this
 * discards it in favour of 0x5756 or the 64 by 64 default. Transcribed as it
 * stands, with the parameters named and voided.
 */
int16_t claim_buffer_slot(uint16_t a_lo, uint16_t a_hi,
                          uint16_t b_lo, uint16_t b_hi)
{
    int16_t i;
    uint16_t size;
    int32_t asked;

    (void)a_lo;
    (void)a_hi;
    (void)b_lo;
    (void)b_hi;

    if (DG16(0x5756) != 0)
        size = DGU16(0x5756);
    else
        size = (uint16_t)vm_buffer_size(0x40, 0x40);

    asked = (int16_t)size;

    for (i = 0; i < 4; i++) {
        if ((DGU16(0x5758 + 4 * i) | DGU16(0x575a + 4 * i)) == 0) {
            uint32_t p = dos_alloc_bytes((uint16_t)asked,
                                         (uint16_t)(asked >> 16), 0, 0);

            DG16(0x575a + 4 * i) = (int16_t)(p >> 16);
            DG16(0x5758 + 4 * i) = (int16_t)(p & 0xFFFF);
        }
    }

    for (i = 0; i < 4; i++) {
        if (DG8(0x5734 + i) == 0
            && (DGU16(0x5758 + 4 * i) | DGU16(0x575a + 4 * i)) != 0) {
            DG8(0x5734 + i) = 1;
            return (int16_t)(i + 1);
        }
    }

    return -1;
}

/*
 * 0x0b69c
 *
 * Clear one byte of the four-entry array at DGROUP 0x5734, addressed
 * **one-based**: the argument is decremented before it is used as the index.
 *
 * Zero is rejected, and so is anything that lands at index 4 or above. Nothing
 * rejects a *negative* argument: the bound is `jge 4`, a signed test that a
 * negative index passes, so a caller passing a number below zero writes a zero
 * byte in front of the array. No caller seen does, but the guard is genuinely
 * one-sided and the port reproduces it rather than adding the missing half.
 */
void clear_slot_5734(int16_t n)
{
    int16_t i = (int16_t)(n - 1);

    if (n != 0 && i < 4)
        DG8((uint16_t)(0x5734 + i)) = 0;
}
