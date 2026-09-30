/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The magnifying glass**: its setup, step and flip, and the distance to what it holds.
 *
 * The thirty-first module of the original's **code segment 172c**, image
 * 0x1a2f0..0x1a4ff - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* 172c:3030, image 0x1a2f0 - kind 30's setup, and it does nothing at all:
 * `push bp / mov bp,sp / pop bp / retf`. It has a function here rather than a
 * line in the dispatcher so that its address can be named - to the verifier,
 * and to the coverage tool, which cannot see a routine that exists only as a
 * case. */
void part_setup_magnifying_glass(struct part *part)
{
    (void)part;
}

/*
 * 172c:3035, image 0x1a2f5 - kind 30's step.
 *
 * It reaches for whatever is passing: `link_nearby_objects` builds a chain
 * through +0x78 of everything within 0x20 either side, and this picks one of
 * them to hold at +0x62.
 *
 * Two questions are asked of each candidate. Kinds 0x1d, 0x19 and 0x2d in a
 * form other than zero *block* it - unless the mirror bits agree for 0x19, or
 * the form is 2 for 0x1d - and something is only taken hold of at all if
 * something else blocked. Anything else with bit 2 of +0x0a in form zero is a
 * candidate: it has to be moving towards this part, and to be within 0x30
 * across and no further down than across.
 *
 * What it held last step wins outright if it is still there; otherwise the
 * slowest candidate wins, which is what makes it settle on the thing it can
 * actually catch. Taking hold steps the held part's +0x9c and says this part
 * moved.
 */
void part_step_magnifying_glass(struct part *part)
{
    struct part *si;
    int16_t v02;                        /* [bp-2]   something blocked */
    int16_t v04;                        /* [bp-4]   held it last step */
    int16_t v06;                        /* [bp-6]   the slowest so far */
    int16_t v08;                        /* [bp-8]   this one will do */
    int16_t v0a;                        /* [bp-0xa] the reach */
    int16_t v0c;                        /* [bp-0xc] the drop */
    struct part *v0e;                   /* [bp-0xe] the one held */

    link_nearby_objects(part, (PART_IN_PLACED_LIST | PART_IN_MOVING_LIST), -0x20, 0x20, 0, 0);

    v0e = NULL;
    v04 = v02 = 0;
    v06 = 0x190;

    si = part->next_linked;
    while (si != NULL) {
        if ((si->kind == KIND_LIGHT || si->kind == KIND_FLASHLIGHT
             || si->kind == KIND_CANDLE) && si->form != 0) {
            if (part->flags_08 & PART_FLIP_HORIZONTAL) {
                if (si->link_dx > 0)
                    v02 = 1;
            } else {
                if (si->link_dx < 0)
                    v02 = 1;
            }

            /* A flashlight facing the *other* way takes the block back. */
            if (si->kind == KIND_FLASHLIGHT) {
                if ((si->flags_08 ^ part->flags_08) & PART_FLIP_HORIZONTAL)
                    v02 = 0;
            } else if (si->kind == KIND_LIGHT && si->form == 2) {
                v02 = 0;
            }
        } else if ((si->flags_0a & PART_IGNITES) && si->form == 0 && v04 == 0) {
            v08 = 0;

            if (part->flags_08 & PART_FLIP_HORIZONTAL) {
                if (si->link_dx < 0)
                    v08 = 1;
            } else {
                if (si->link_dx > 0)
                    v08 = 1;
            }

            grab_distance(part, si, &v0a, &v0c);

            if (v0a >= 0x30 || v0c > v0a)
                v08 = 0;

            if (v08 != 0) {
                if (part->link[4] == si) {
                    v0e = si;
                    v04 = 1;
                } else if (abs(si->link_dx) < abs(v06)) {
                    v06 = si->link_dx;
                    v0e = si;
                }
            }
        }

        if (v02 != 0 && v04 != 0)
            si = NULL;
        else
            si = si->next_linked;
    }

    if (v02 == 0)
        v0e = NULL;

    if ((part->link[4] = v0e) != 0) {
        v0e->spin++;
        part_moved(part);
    }
}

/*
 * 172c:31af, image 0x1a46f - kind 30's flip: bit 4 and a redraw, no setup.
 */
void part_flip_magnifying_glass(struct part *part)
{
    part->flags_08 ^= PART_FLIP_HORIZONTAL;
    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:31dc, image 0x1a49c
 *
 * How far one part is from another's grip, as two absolute distances written
 * through pointers.
 *
 * The grip is the part's own left edge, or its right edge when bit 4 of +8 is
 * clear, and eight down from its top; the other part's point is its position
 * plus the two bytes at +0x72 and +0x73, which is where that kind is held.
 */
void grab_distance(struct part *a, struct part *b, int16_t *out_x, int16_t *out_y)
{
    int16_t hx;                         /* [bp-2] */
    int16_t y;                          /* [bp-4] */
    int16_t hy;                         /* [bp-6] */
    int16_t x;                          /* cx */

    x = a->pos[0].x;
    if (!(a->flags_08 & PART_FLIP_HORIZONTAL))
        x += a->size[0].width;
    y = a->pos[0].y + 8;
    hx = b->pos[0].x + b->hold.x;
    hy = b->pos[0].y + b->hold.y;

    *out_x = abs(x - hx);
    *out_y = abs(y - hy);
}
