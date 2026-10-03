/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Kind 62's handlers**, new in 1.11 - by what they do, a blower: once
 * started it spins up, and while it runs it pushes every moving part in a
 * strip in front of it away and up, blows out a candle, turns a windmill and
 * trips a mouse cage. The name waits for its icon. Its record in
 * `g_part_kinds` names these, and one helper follows them.
 *
 * In 1.11, image 0x1e37e.. in the part kinds' code segment. **Both ends are
 * ours**: each kind in 1.00 is a module of its own, and so is this one until
 * the far calls between its routines and its neighbours' say otherwise.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3158..0x3164: the six points `part_setup_kind_62` copies.
 */
struct point8 g_kind_62_points[6] = {
    { 1, 14 }, { 8, 0 }, { 24, 0 }, { 29, 14 },
    { 29, 21 }, { 2, 21 },
};

/* 0x1e37e - a setup: six points from DGROUP 0x3158. */
void part_setup_kind_62(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_kind_62_points;
    for (i = 0, si = part->points; i < 6; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}

/* 0x1e3bf - turn it round, and redraw. */
void part_flip_kind_62(register struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    part_setup_kind_62(part);
    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 0x1e3f2
 *
 * **Running.** `direction` is the power: while it is on, `spin` counts up to
 * 0x14 from rest and is then held at 0x28, and at 0x28 the blades turn
 * (forms 6..11). With the power off it turns while `spin` runs down, and
 * stops in form 0. In forms 6..11 it hums (sound 0x19, held by
 * `g_sound_request_19`), creeps against its own draught (0x20 a frame, to
 * 0x100), and blows: every moving part in the strip in front of it, 0x4b
 * wide and 0x23 high, is pushed away and up by `kind_62_push` - to 0x600 at
 * most - a candle is put out, a windmill set turning and a mouse cage
 * tripped.
 */
void part_step_kind_62(register struct part *part)
{
    int16_t push;                       /* [bp-2] */
    struct part *what;

    if (part->direction != 0) {
        if (part->spin < 0x14 && part->form == 0)
            part->spin++;
        else
            part->spin = 0x28;
        if (part->spin == 0x28) {
            part->form++;
            if (part->form == 0xc)
                part->form = 6;
            place_object_for_draw(part);
        }
        part->direction = 0;
    } else if (part->form != 0) {
        if (part->spin != 0)
            part->spin--;
        if (part->spin == 0)
            part->form = 0;
        else {
            part->form++;
            if (part->form == 0xc)
                part->form = 6;
        }
        place_object_for_draw(part);
    }

    if (part->form < 6 || part->form > 0xb)
        return;

    g_sound_request_19 = 2;
    if (part->form == part->form_prev)
        play_sound(0x19);

    if (part->state & STATE_FLIP_HORIZONTAL) {
        if (part->vel_x > -0x100)
            part->vel_x -= 0x20;
        link_objects_in_range(part, 0x3000, 0x1e, 0x4b, -0x23, 0);
    } else {
        if (part->vel_x < 0x100)
            part->vel_x += 0x20;
        link_objects_in_range(part, 0x3000, -0x2d, 0, -0x23, 0);
    }

    for (what = part->next_linked; what != 0; what = what->next_linked) {
        if (what->traits & TRAIT_IN_MOVING_LIST) {
            push = kind_62_push(what);
            if (part->state & STATE_FLIP_HORIZONTAL) {
                if (what->vel_x < 0x600)
                    what->vel_x += push;
            } else if (what->vel_x > -0x600)
                what->vel_x -= push;
            if (what->vel_y > -0x600)
                what->vel_y -= push;
            clamp_record_pair(what);
            if (what->kind == KIND_CANDLE)
                what->form = what->direction = what->spin = 0;
        } else if (what->kind == KIND_WINDMILL) {
            what->direction = 1;
            what->spin = 0x14;
        } else if (what->kind == KIND_MOUSE_CAGE)
            trigger_mouse_cage(what);
    }
}

/*
 * 0x1e578
 *
 * How hard the draught pushes a part: 0x600 a frame for one whose kind
 * weighs under 0x97, 0x400 for a heavier one.
 */
int16_t kind_62_push(register struct part *what)
{
    int16_t weight;                     /* [bp-2] */
    int16_t push;                       /* cx */

    weight = g_part_kinds[what->kind].weight;
    if (weight < 0x97)
        push = 0x600;
    else
        push = 0x400;
    return push;
}
