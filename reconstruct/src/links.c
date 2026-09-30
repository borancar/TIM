/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **What is near what**: chaining the parts close to, inside, crossing or
 * holding a point near a part through +0x78, and a platform collecting and
 * carrying what rests on it.
 *
 * The original's **code segment 0000** (`_TEXT`), image 0x03566..0x03b17,
 * split out of machine.c on 2026-09-27. **Both ends of this file are ours.**
 * Nothing from 0x03566 to 0x03f4d names a byte of `_DATA` or calls back
 * across itself, so no module boundary in that stretch leaves a byte to
 * read; the file follows the subject, and `rotate_point` onwards is
 * geometry.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT -O -Z
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x0417e
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
void link_nearby_objects(struct part *obj, uint16_t flags,
                         int16_t margin_x0, int16_t margin_x1,
                         int16_t margin_y0, int16_t margin_y1)
{
    register struct part *si;
    int16_t ax0;                        /* [bp-2] */
    int16_t ax1;                        /* [bp-4] */
    int16_t ay0;                        /* [bp-6] */
    int16_t ay1;                        /* [bp-8] */
    int16_t bx0;                        /* [bp-0xa] */
    int16_t bx1;                        /* [bp-0xc] */
    int16_t by0;                        /* [bp-0xe] */
    int16_t by1;                        /* [bp-0x10] */
    int16_t dx;                         /* [bp-0x12] */
    int16_t dy;                         /* [bp-0x14] */
    int16_t near_;                      /* [bp-0x16] */
    int16_t hi;                         /* [bp-0x18] */
    int16_t lo;                         /* [bp-0x1a] */

    obj->next_linked = 0;
    ax0 = obj->pos[0].x;
    ax1 = ax0 + obj->size[0].width;
    ay0 = obj->pos[0].y;
    ay1 = ay0 + obj->size[0].height;

    for (si = pick_by_flag(flags); si != NULL;
         si = pick_for_record(si, flags & TRAIT_IN_MOVING_LIST)) {
        if (obj == si)
            continue;
        if (si->state & STATE_GONE)
            continue;

        bx0 = si->pos[0].x;
        bx1 = bx0 + si->size[0].width;
        by0 = si->pos[0].y;
        by1 = by0 + si->size[0].height;

        dx = bx1 - ax0;
        if (dx < margin_x0)
            continue;
        if (dx >= 0)
            hi = -1;
        else
            hi = dx;
        near_ = bx0 - ax1;
        if (near_ > margin_x1)
            continue;
        if (near_ <= 0)
            lo = 1;
        else
            lo = near_;
        dx = abs(near_) < abs(dx) ? lo : hi;

        dy = by1 - ay0;
        if (dy < margin_y0)
            continue;
        if (dy >= 0)
            hi = -1;
        else
            hi = dy;
        near_ = by0 - ay1;
        if (near_ > margin_y1)
            continue;
        if (near_ <= 0)
            lo = 1;
        else
            lo = near_;
        dy = abs(near_) < abs(dy) ? lo : hi;

        si->next_linked = obj->next_linked;
        obj->next_linked = si;
        si->link_dx = dx;
        si->link_dy = dy;
    }
}

/*
 * 0x042da
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
void link_objects_in_range(struct part *obj, uint16_t flags,
                           int16_t x0, int16_t x1, int16_t y0, int16_t y1)
{
    register struct part *si;
    int16_t l;                          /* [bp-2] */
    int16_t r;                          /* [bp-4] */
    int16_t t;                          /* [bp-6] */
    int16_t b;                          /* [bp-8] */

    obj->next_linked = 0;
    x0 += obj->pos[0].x;
    x1 += obj->pos[0].x;
    y0 += obj->pos[0].y;
    y1 += obj->pos[0].y;

    for (si = pick_by_flag(flags); si != NULL;
         si = pick_for_record(si, flags & TRAIT_IN_MOVING_LIST)) {
        if (obj == si)
            continue;
        if (si->state & STATE_GONE)
            continue;

        l = si->pos[0].x;
        r = l + si->size[0].width;
        t = si->pos[0].y;
        b = t + si->size[0].height;
        if (si->kind == 62)                     /* 1.11: only its first 0x1e */
            r = l + 0x1e;

        if (l < x1 && r > x0 && t < y1 && b > y0) {
            si->next_linked = obj->next_linked;
            obj->next_linked = si;
        }
    }
}

/*
 * 0x0437d
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
void link_objects_crossing(struct part *obj, uint16_t flags, const int16_t *line)
{
    register struct part *si;
    register struct part_point *pt;
    int16_t n;                          /* [bp-2] the point */
    int16_t x_last;                     /* [bp-4] */
    int16_t x_this;                     /* [bp-6] */
    int16_t x_first;                    /* [bp-8] */
    int16_t y_last;                     /* [bp-0xa] */
    int16_t y_this;                     /* [bp-0xc] */
    int16_t y_first;                    /* [bp-0xe] */
    int16_t seg[4];                     /* [bp-0x16] the segment */
    uint8_t cross[4];                   /* [bp-0x1a] where they crossed */

    obj->next_linked = 0;

    for (si = pick_by_flag(flags); si != NULL;
         si = pick_for_record(si, flags & TRAIT_IN_MOVING_LIST)) {
        n = 1;
        pt = NEAR_ZERO(si->points);
        x_first = x_last = si->pos[0].x + pt[0].x;
        y_first = y_last = si->pos[0].y + pt[0].y;
        x_this = si->pos[0].x + pt[1].x;
        y_this = si->pos[0].y + pt[1].y;

        while (pt != NEAR_ZERO((struct part_point *)0)) {
            seg[0] = x_last - obj->pos[0].x;
            seg[1] = y_last - obj->pos[0].y;
            seg[2] = x_this - obj->pos[0].x;
            seg[3] = y_this - obj->pos[0].y;

            if (intersect_segments(line, seg, cross)) {
                si->next_linked = obj->next_linked;
                obj->next_linked = si;
                n = si->point_count;
            }

            n++;
            if ((int16_t)si->point_count < n) {
                pt = NEAR_ZERO((struct part_point *)0);
            } else {
                pt++;
                x_last = x_this;
                y_last = y_this;
                if ((int16_t)si->point_count == n) {
                    x_this = x_first;
                    y_this = y_first;
                } else {
                    x_this = si->pos[0].x + pt[1].x;
                    y_this = si->pos[0].y + pt[1].y;
                }
            }
        }
    }
}

/*
 * 0x0448e
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
void link_objects_at_point(struct part *obj, int16_t x0, int16_t x1,
                           int16_t y0, int16_t y1)
{
    register struct part *si;
    int16_t px;                         /* [bp-2] */
    int16_t py;                         /* [bp-4] */

    obj->next_linked = 0;
    x0 += obj->pos[0].x;
    x1 += obj->pos[0].x;
    y0 += obj->pos[0].y;
    y1 += obj->pos[0].y;

    for (si = pick_by_flag((TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST)); si != NULL;
         si = pick_for_record(si, TRAIT_IN_MOVING_LIST)) {
        if (obj == si)
            continue;
        if (si->state & STATE_GONE)
            continue;

        if (si->traits2 & TRAIT2_IGNITES) {
            px = si->pos[0].x + si->hold.x;
            py = si->pos[0].y + si->hold.y;
            if (px >= x0 && px <= x1 && ((py >= y0) & (py <= y1))) {
                si->next_linked = obj->next_linked;
                obj->next_linked = si;
            }
        }
    }
}

/*
 * 0x0453a
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
void collect_carried(register struct part *obj)
{
    register struct part *si;
    int16_t carried;
    int16_t left;                       /* [bp-2] */
    int16_t right;                      /* [bp-4] */
    int16_t top;                        /* [bp-6] */
    int16_t bottom;                     /* [bp-8] */
    int16_t their_mid;                  /* [bp-0xa] */
    int16_t their_top;                  /* [bp-0xc] */
    int16_t their_bottom;               /* [bp-0xe] */
    int16_t rim;                        /* [bp-0x10] */

    if (obj->kind == KIND_BUCKET) {
        obj->next_linked = 0;
        left = obj->pos[1].x + 4;
        right = left + 0x1c;
        top = obj->pos[1].y;
        bottom = top + obj->size[0].height;
        rim = obj->pos[0].y + 0x14;             /* 1.11 */

        for (si = g_moving_parts.next; si != NULL;
             si = si->next) {
            if (obj == si)
                continue;
            if (si->state & STATE_GONE)
                continue;
            if (si->kind == KIND_BIRD_CAGE)
                continue;

            their_mid = si->pos[1].x + (si->size[0].width >> 1);
            their_top = si->pos[1].y;
            their_bottom = their_top + si->size[0].height;
            if (si->kind == KIND_ROCKET)        /* 1.11 */
                their_bottom -= 0x0c;

            carried = 0;
            if (si->contact != 0 && si->contact == obj
                && si->vel_y > 0 && their_mid > left && their_mid < right)
                carried = 1;
            if (!carried && their_mid > left && their_mid < right
                && top + 0x14 < their_bottom && their_bottom - 4 < bottom)
                carried = 1;

            if (carried) {
                si->next_linked = obj->next_linked;
                obj->next_linked = si;
                si->traits2 |= TRAIT2_IN_BUCKET;
                PART_VEL(si) = PART_VEL(obj);
                si->kind_state = rim;           /* 1.11 */
            }
        }
    }
}

/*
 * 0x0463d
 *
 * Is `node` on the chain hanging off `rec`? Only records whose type word at
 * +4 is 0x11 have such a chain; anything else answers no without looking.
 * The chain is linked through the near pointer at +0x78.
 */
int16_t chain_contains(register struct part *rec, struct part *node)
{
    register struct part *p;

    if (rec->kind == 0x11)
        for (p = rec->next_linked; p != NULL;
             p = p->next_linked)
            if (p == node)
                return 1;
    return 0;
}

/*
 * 0x04667
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
void carry_riders_along(register struct part *obj)
{
    register struct part *si;
    int16_t dx;                         /* [bp-2] */
    int16_t dy;                         /* [bp-4] */

    if (obj->kind == KIND_BUCKET) {
        dx = obj->pos[0].x - obj->pos[1].x;
        dy = obj->pos[0].y - obj->pos[1].y;
        if (dx != 0 || dy != 0)
            for (si = obj->next_linked; si != NULL;
                 si = si->next_linked) {
                si->pos[0].x += dx;
                si->pos[0].y += dy;
                place_object_for_draw(si);
                si->fx = si->pos[0].x;
                si->fx = (int32_t)((uint32_t)si->fx << 9);
                si->fy = si->pos[0].y;
                si->fy = (int32_t)((uint32_t)si->fy << 9);
            }
    }
}
