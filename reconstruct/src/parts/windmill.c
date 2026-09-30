/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The windmill**: its setup, step and flip.
 *
 * The forty-third module of the original's **code segment 172c**, image
 * 0x1bc2f..0x1bd14 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 190f:488a, image 0x1d97a - a setup.
 *
 * Three points, a tall triangle - the peak at y 17 and the base at 47.
 */
void part_setup_windmill(struct part *part)
{
    struct part_point *si = part->points;

    si->x = 8;
    si->y = 47;
    si++;
    si->x = 18;
    si->y = 17;
    si++;
    si->x = 28;
    si->y = 47;

    part_finish_angles(part);
}

/*
 * 190f:48bc, image 0x1d9ac - kind 40's step.
 *
 * A countdown at +0x9c: while it is running the part is "on", which it says in
 * the word at +0x12 and passes to whatever its belt is tied to - as 1, or -1
 * when bit 4 of its flags at +8 is set, which is the mirrored form. The other
 * end is only told if it is not already busy, bit 11 of its own +8.
 *
 * Being on also steps the form at +0x0c round the four frames, and a form that
 * has changed since the last one drawn is handed to `place_object_for_draw`.
 *
 * The original leaves AX as whatever fell out; nothing reads it.
 */
void part_step_windmill(struct part *part)
{
    struct part *di;

    part->direction = 0;

    if (part->spin != 0) {
        part->spin--;
        if (part->spin != 0)
            part->direction = 1;
    }

    if ((di = belt_other_end(part)) != NULL && !(di->state & STATE_SELF_DRIVEN)) {
        if (part->direction != 0) {
            if (part->state & STATE_FLIP_HORIZONTAL)
                di->direction = -1;
            else
                di->direction = 1;
        } else {
            di->direction = 0;
        }
    }

    if (part->direction != 0) {
        if (part->form == 3)
            part->form = 0;
        else
            part->form++;
    }

    if (part->form != part->form_prev)
        place_object_for_draw(part);
}

/*
 * 190f:493f, image 0x1da2f - kind 40's flip: bit 4 and three marks, with
 * **neither a setup nor a draw**. The leanest of the twenty.
 */
void part_flip_windmill(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;
    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
