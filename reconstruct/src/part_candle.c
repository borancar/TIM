/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The candle**: its setup and step.
 *
 * The seventh module of the original's **code segment 172c**, image
 * 0x17c10..0x17d1d - one module for each kind of part; part_ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 172c:0950, image 0x17c10 - a setup.
 *
 * Three points - a triangle - and the grab box at +0x72 and +0x73 written
 * **before** them, which is the order the original uses.
 */
void part_setup_candle(struct part *part)
{
    struct part_point *si;

    part->hold.x = 0x0f;
    part->hold.y = 0x02;

    si = POINTS(part->points_ptr);
    si->x = 8;
    si->y = 31;
    si++;
    si->x = 14;
    si->y = 22;
    si++;
    si->x = 21;
    si->y = 31;

    part_finish_angles(part);
}

/*
 * 172c:098a, image 0x17c4a - kind 45's step. The paddle wheel.
 *
 * Once its +0x9c has counted past 0x14 it starts itself, and then runs four
 * frames on a loop - 5 wraps back to 1, so frame 0 is only ever the first one.
 *
 * Every step it reaches for the point at +0x72 of whatever is nearby, in a box
 * 9 to 0x12 across and ten up to five down, and switches each on. In the odd
 * frames it reaches again over the same box for two kinds by name: a kind 4 is
 * switched on, and a cat - kind 0x0c - in form 0 is woken with sound 7.
 */
void part_step_candle(struct part *part)
{
    struct part *si;

    if (part->direction == 0 && part->spin > 0x14)
        part->direction = 1;

    if (part->direction != 0) {
        if (part->form == 5)
            part->form = 1;
        else
            part->form++;

        place_object_for_draw(part);

        link_objects_at_point(part, 9, 0x12, -10, 5);

        for (si = PART_PTR(part->next_linked_ptr); si != PART_NONE;
             si = PART_PTR(si->next_linked_ptr))
            if (si->direction == 0)
                si->direction = 1;

        if (part->form & 1) {
            link_objects_in_range(part, 0x1000, 9, 0x12, -10, 5);

            for (si = PART_PTR(part->next_linked_ptr); si != PART_NONE;
                 si = PART_PTR(si->next_linked_ptr)) {
                if (si->kind == KIND_BALLOON) {
                    si->direction = 1;
                } else if (si->kind == KIND_POKEY && si->form == 0) {
                    si->form = 1;
                    si->word_96 = 0;
                    place_object_for_draw(si);
                    play_sound(7);
                }
            }
        }
    }
}
