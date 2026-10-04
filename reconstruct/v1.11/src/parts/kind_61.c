/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Kind 61's handlers**, new in 1.11 - by what they do, the alligator: it
 * swallows Mort and kind 54 when they reach its mouth, knocks or throws
 * back anything else, and when it snaps (form 8) it works whatever lies in
 * front of it - a seesaw, a mouse cage, the fishbowl, a bellows, a plug, a
 * pair of scissors. The name waits for its icon. Its record in
 * `g_part_kinds` names these, and four helpers of its own follow them.
 *
 * In 1.11, image 0x1da61.. in the part kinds' code segment. **Both ends are
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
 * DGROUP 0x30fc..0x310c: the four corners of its 0x50-by-0x10 box, facing
 * one way and the other - the same four points either way - indexed [flipped].
 */
struct point8 g_kind_61_points[2][4] = {
    {
        { 0, 0 }, { 79, 0 }, { 79, 15 }, { 0, 15 },
    },
    {
        { 0, 0 }, { 79, 0 }, { 79, 15 }, { 0, 15 },
    },
};

/*
 * 0x1da61
 *
 * **Something ran into it.** `part` is what moved; its `contact` is the
 * alligator. Where along the top the mover's middle is, against the
 * alligator's left edge and which way it faces, decides:
 *
 *   - Mort (0x2a) or kind 54 at the mouth, or striking its end edge, is
 *     swallowed: the alligator shows form 7 and remembers a meal (`spin`),
 *     the mover's shapes are marked, it is gone, and Mort squeaks (0xd);
 *   - anything else at the mouth is knocked away: form 7, 0x600 sideways
 *     away from the mouth and 0xea00 up;
 *   - anything at the tail is thrown up at no less than 0x600;
 *   - a fall onto its back edge (2) faster than 0x400 makes it snap.
 *
 * It answers 0 when it dealt with the mover and 1 otherwise.
 */
uint16_t part_hit_kind_61(register struct part *part)
{
    int16_t edge;                       /* [bp-2] */
    int16_t mid;                        /* [bp-4] */
    int16_t eats;                       /* [bp-6]  1, and 2 once it has */
    int16_t fling;                      /* [bp-8] */
    int16_t knock;                      /* [bp-0xa] */
    struct part *gator;

    if (part->kind == 0x2a || part->kind == 0x36)
        eats = 1;
    else
        eats = 0;
    gator = part->contact;
    edge = part->contact_edge;
    fling = knock = 0;
    mid = part->pos[0].x + (part->size[0].width >> 1);

    if (gator->state & STATE_FLIP_HORIZONTAL) {
        if (eats != 0) {
            if (edge == 1)
                eats = 2;
            else if (edge == 0) {
                if (gator->pos[0].x + 64 < mid)
                    eats = 2;
                else if (gator->pos[0].x + 16 > mid)
                    fling = 1;
            }
        } else if (edge == 0) {
            if (gator->pos[0].x + 54 < mid)
                knock = 1;
            else if (gator->pos[0].x + 16 > mid)
                fling = 1;
        }
    } else {
        if (eats != 0) {
            if (edge == 3)
                eats = 2;
            else if (edge == 0) {
                if (gator->pos[0].x + 16 > mid)
                    eats = 2;
                else if (gator->pos[0].x + 64 < mid)
                    fling = 1;
            }
        } else if (edge == 0) {
            if (gator->pos[0].x + 26 > mid)
                knock = 1;
            else if (gator->pos[0].x + 64 < mid)
                fling = 1;
        }
    }

    if (eats == 2) {
        gator->form = 7;
        gator->spin = 1;
        gator->traits2 |= 0x80;
        mark_part_shapes(part, 3);
        part->state |= STATE_GONE;
        part->traits2 |= 0x200;
        if (part->kind == 0x2a)
            play_sound(0xd);
        return 0;
    }
    if (knock != 0) {
        gator->form = 7;
        if (gator->state & STATE_FLIP_HORIZONTAL)
            part->vel_x -= 0x600;
        else
            part->vel_x += 0x600;
        part->vel_y = (int16_t)0xea00;
        clamp_record_pair(part);
        return 0;
    }
    if (fling != 0) {
        part->vel_y = 0 - (abs(part->vel_y) < 0x600 ? 0x600 : abs(part->vel_y));
        return 0;
    }
    if (edge == 2 && gator->form < 7 && part->vel_y < -0x400)
        gator->form = 7;
    return 1;
}

/* 0x1dc13 - a setup: four points, from one table or the other by facing. */
void part_setup_kind_61(struct part *part)
{
    register const struct point8 *src;
    register struct part_point *dst;
    int16_t i;

    if (part->state & STATE_FLIP_HORIZONTAL)
        src = g_kind_61_points[1];
    else
        src = g_kind_61_points[0];
    for (i = 0, dst = part->points; i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 0x1dc63
 *
 * **The animation, and the snap.** Forms 0..5 loop while it waits; 7 on is
 * the snap `hit` starts, which at 10 ends unless it has eaten (and then
 * chews, sound 0x18) and at 15 is over. At form 8 - the jaws shut - it finds
 * what is in front of the mouth, a strip of 0x10 at its facing end, and acts
 * on each part there by kind, measured from `mouth`.
 */
void part_step_kind_61(register struct part *part)
{
    int16_t mouth;                      /* [bp-2] */
    struct part *what;

    part->form++;
    if (part->form == 6)
        part->form = 0;
    if (part->form == 10) {
        if (part->spin == 0)
            part->form = 0;
        else
            play_sound(0x18);
    }
    if (part->form == 15) {
        part->form = 0;
        part->spin = 0;
    }
    place_object_for_draw(part);

    if (part->form != 8)
        return;

    if (part->state & STATE_FLIP_HORIZONTAL) {
        mouth = part->pos[0].x + 72;
        link_objects_in_range(part, 0x2000, 64, 80, -17, 0);
    } else {
        mouth = part->pos[0].x + 8;
        link_objects_in_range(part, 0x2000, 0, 16, -17, 0);
    }

    for (what = part->next_linked; what != 0; what = what->next_linked) {
        switch (what->kind) {
        case KIND_BOB_THE_FISH:
            break_bob_the_fish(what);
            break;
        case KIND_MOUSE_CAGE:
            trigger_mouse_cage(what);
            break;
        case KIND_SEESAW:
            kind_61_tip_seesaw(what, mouth);
            break;
        case KIND_BELLOW:
            kind_61_press_bellow(what, mouth);
            break;
        case KIND_ELECTRIC_PLUG:
            kind_61_pull_plug(what, mouth);
            break;
        case KIND_SCISSORS:
            kind_61_close_scissors(what, mouth);
            break;
        }
    }
}

/* 0x1dd85 - turn it round: the other table's points, and redraw. */
void part_flip_kind_61(register struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    part_setup_kind_61(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 0x1ddb1
 *
 * A seesaw in the jaws: its raised end is pushed down if the mouth is on
 * that side - the left in form 0, the right in form 2.
 */
void kind_61_tip_seesaw(register struct part *seesaw, int16_t mouth)
{
    if (seesaw->form == 0) {
        if (seesaw->pos[0].x + 26 > mouth)
            seesaw->direction = 1;
    } else if (seesaw->form == 2) {
        if (seesaw->pos[0].x + 54 < mouth)
            seesaw->direction = -1;
    }
}

/* 0x1ddec - a bellows at rest, bitten on its handle end, blows. */
void kind_61_press_bellow(register struct part *bellow, int16_t mouth)
{
    if (bellow->form == 0) {
        if (bellow->state & STATE_FLIP_HORIZONTAL) {
            if (bellow->pos[0].x + 20 < mouth)
                bellow->direction = 1;
        } else if (bellow->pos[0].x + 36 > mouth)
            bellow->direction = 1;
    }
}

/*
 * 0x1de23
 *
 * A plug that is in, bitten by its head, comes out: its form back by four,
 * its points set again, sound 0x11, and it will move unless it was placed
 * out.
 */
void kind_61_pull_plug(register struct part *plug, register int16_t mouth)
{
    if (plug->form >= 4 && plug->pos[0].x - 2 < mouth
        && plug->pos[0].x + 20 > mouth) {
        plug->form -= 4;
        part_setup_electric_plug(plug);
        play_sound(0x11);
        if (plug->form != plug->start_form)
            plug->direction = 1;
        else
            plug->direction = 0;
    }
}

/* 0x1de76 - open scissors, bitten by the handles, close. */
void kind_61_close_scissors(register struct part *scissors, int16_t mouth)
{
    if (scissors->form == 0) {
        if (scissors->state & STATE_FLIP_HORIZONTAL) {
            if (scissors->pos[0].x + 18 < mouth)
                scissors->direction = 1;
        } else if (scissors->pos[0].x + 24 > mouth)
            scissors->direction = 1;
    }
}
