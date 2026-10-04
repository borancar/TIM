/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Kind 51's handlers**, new in 1.11 - by what they do, the vacuum
 * cleaner: it plugs in, and while it runs it hums, draws every moving part in
 * a column above it up towards its nozzle and swallows what comes close -
 * popping a balloon, eating Mort. The name waits for its icon. Its record in
 * `g_part_kinds` names these, and two helpers follow them.
 *
 * In 1.11, image 0x1e5af.. in the part kinds' code segment. **Both ends are
 * ours**: each kind in 1.00 is a module of its own, and so is this one until
 * the far calls between its routines and its neighbours' say otherwise.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3164..0x3184: its eight points facing one way and the other, indexed [flipped].
 */
struct point8 g_kind_51_points[2][8] = {
    {
        { 0, 2 }, { 3, 0 }, { 12, 7 }, { 23, 31 },
        { 44, 36 }, { 44, 46 }, { 21, 46 }, { 8, 41 },
    },
    {
        { 24, 31 }, { 35, 7 }, { 44, 0 }, { 47, 2 },
        { 39, 41 }, { 26, 46 }, { 3, 46 }, { 3, 36 },
    },
};

/*
 * 0x1e5af
 *
 * **Something ran into it**: if it is running and the thing came in at the
 * nozzle - edge 5, or straight down (angle 0x8000) - it is swallowed.
 * Answers 0 when it was, 1 otherwise.
 */
uint16_t part_hit_kind_51(register struct part *part)
{
    int16_t edge;                       /* [bp-2] */
    int16_t angle;                      /* [bp-4] */
    struct part *vacuum;

    vacuum = part->contact;
    edge = part->contact_edge;
    angle = part->contact_angle;
    if (vacuum->direction != 0 && (edge == 5 || angle == (int16_t)0x8000)) {
        kind_51_swallow(part);
        return 0;
    }
    return 1;
}

/* 0x1e5f3 - a setup: eight points, from one table or the other by facing. */
void part_setup_kind_51(struct part *part)
{
    register const struct point8 *src;
    register struct part_point *dst;
    int16_t i;

    if (part->state & STATE_FLIP_HORIZONTAL)
        src = g_kind_51_points[1];
    else
        src = g_kind_51_points[0];
    for (i = 0, dst = part->points; i < 8; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 0x1e643
 *
 * **Running.** `direction` is the power. On, it hums (sound 0x15, held by
 * `g_sound_request_15`), turns through forms 5..8, and gathers the moving
 * parts in the 0x40 above it. Each that is below its top edge + 0x2c is off
 * any surface; one below + 0x2f is swallowed; one falling slower than 0x400
 * is pulled up by `kind_51_pull` for its weight, scaled by how far off to
 * the side it is (0x50 less `link_dy`, over 64), to 0x400 at most; and one
 * still moving is steered towards the nozzle's middle by half the pull, to
 * 0x200 either way. Kind 54 caught in it shows form 6, and a candle is put
 * out. Off, it winds down through forms 4..0.
 */
void part_step_kind_51(struct part *part)
{
    int16_t pull;                       /* [bp-2] */
    int16_t force;                      /* [bp-4] */
    int16_t reach;                      /* [bp-6] */
    int16_t mid;                        /* [bp-8] */
    int16_t their;                      /* [bp-0xa] */
    int32_t product;                    /* [bp-0xe] */
    register struct part *what;

#ifndef __TURBOC__
    /*
     * OURS, on the host only: `force` is read uninitialised in the original
     * when the first part gathered is already rising faster than 0x400 -
     * the steering takes half of whatever the stack held at [bp-4], and after
     * that the last part's pull. The host starts it at 0; the stack word the
     * original read cannot be had.
     */
    force = 0;
#endif
    if (part->direction != 0) {
        g_sound_request_15 = 2;
        if (part->form == part->form_prev)
            play_sound(0x15);
        part->form++;
        if (part->form == 9)
            part->form = 5;
        link_nearby_objects(part, 0x1000, 0, 0, 0, 0x40);
        mid = part->pos[0].x + (part->size[0].width >> 1);
        for (what = part->next_linked; what != 0; what = what->next_linked) {
            their = what->pos[0].x + (what->size[0].width >> 1);
            if (what->pos[0].y > part->pos[0].y + 44) {
                what->traits &= ~TRAIT_ON_SURFACE;
                if (what->pos[0].y < part->pos[0].y + 47)
                    kind_51_swallow(what);
                if (what->vel_y > -0x400) {
                    pull = kind_51_pull(what);
                    reach = 0x50 - abs(what->link_dy);
                    product = mul16x16(pull, reach);
                    force = (int16_t)(product >> 6);
                    what->vel_y -= force;
                    clamp_record_pair(what);
                    if (what->vel_y < -0x400)
                        what->vel_y = -0x400;
                }
                if (what->pos[0].y != what->pos[2].y) {
                    if (what->kind == 0x36 && what->form < 6) {
                        what->form = 6;
                        place_object_for_draw(what);
                    }
                    if (mid - 4 > their) {
                        if (what->vel_x < 0x200) {
                            what->vel_x += force >> 1;
                            if (what->vel_x > 0x200)
                                what->vel_x = 0x200;
                        }
                    } else if (mid + 4 < their) {
                        if (what->vel_x > -0x200) {
                            what->vel_x -= force >> 1;
                            if (what->vel_x < -0x200)
                                what->vel_x = -0x200;
                        }
                    } else if (what->vel_x > 0) {
                        what->vel_x -= force >> 1;
                        if (what->vel_x < 0)
                            what->vel_x = 0;
                    } else {
                        what->vel_x += force >> 1;
                        if (what->vel_x > 0)
                            what->vel_x = 0;
                    }
                }
            }
            if (what->kind == KIND_CANDLE)
                what->form = what->direction = what->spin = 0;
        }
        place_object_for_draw(part);
    } else if (part->form != 0) {
        if (part->form < 5)
            part->form--;
        else
            part->form = 4;
        place_object_for_draw(part);
    }
}

/* 0x1e822 - turn it round, and redraw. */
void part_flip_kind_51(register struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    part_setup_kind_51(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 0x1e84e
 *
 * How hard it pulls a part up, by the weight of its kind: 0xc00 for the
 * lightest, halving roughly at each step, 0x80 for the heaviest.
 */
int16_t kind_51_pull(struct part *what)
{
    int16_t weight;                     /* si */
    int16_t pull;                       /* cx */

    weight = g_part_kinds[what->kind].weight;
    if (weight < 2)
        pull = 0xc00;
    else if (weight < 6)
        pull = 0xa00;
    else if (weight < 0xa)
        pull = 0x800;
    else if (weight < 0x15)
        pull = 0x600;
    else if (weight < 0x79)
        pull = 0x400;
    else if (weight < 0x97)
        pull = 0x200;
    else if (weight < 0xc9)
        pull = 0x100;
    else
        pull = 0x80;
    return pull;
}

/*
 * 0x1e8bb
 *
 * **Swallowed**: a balloon is popped; anything else is gone, marked
 * (`traits2` 0x100, which only this sets) and redrawn away - and Mort
 * squeaks (0xd) as he goes.
 */
void kind_51_swallow(register struct part *what)
{
    switch (what->kind) {
    case 0x2a:
        mark_part_shapes(what, 3);
        what->state |= STATE_GONE;
        what->traits2 |= 0x100;
        play_sound(0xd);
        break;
    case KIND_BALLOON:
        what->direction = 1;
        break;
    default:
        mark_part_shapes(what, 3);
        what->state |= STATE_GONE;
        what->traits2 |= 0x100;
        break;
    }
}
