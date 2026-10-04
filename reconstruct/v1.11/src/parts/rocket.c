/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The rocket**: its step and setup.
 *
 * The thirty-fifth module of the original's **code segment 172c**, image
 * 0x1a8f5..0x1aa3b - one module for each kind of part; parts/ball.c says how
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
 * 190f:3600, image 0x1c6f0 - kind 36's step. The kicker.
 *
 * It forgets whatever it was touching - +0x84 to zero - and starts itself once
 * its counter passes 0x14. Then it runs its frames, wrapping 0x0a back to 7 so
 * the last four loop; form 6 plays sound 0x0f, and from form 7 on it is
 * *lifting*, taking 0x400 off its own downward velocity every step.
 *
 * From form 7 it also reaches out, the same way the paddle wheel does: a point
 * match first, switching on everything it finds, and then in the odd frames a
 * box match for a kind 4 to switch on and a cat to wake.
 */
void part_step_rocket(struct part *part)
{
    struct part *di;

    if (part->direction == 0 && part->spin > 0x14)
        part->direction = 1;

    if (part->direction != 0) {
        part->form++;
        if (part->form == 0x0a)
            part->form = 7;

        if (part->form == 6)
            play_sound(0x0f);

        if (part->form >= 7) {
            part->contact = 0;                  /* 1.11: 1.00 cleared it first thing */
            part->vel_y -= 0x400;
            clamp_record_pair(part);
        }

        place_object_for_draw(part);

        if (part->form >= 7) {
            link_objects_at_point(part, -4, 18, 48, 81);

            for (di = part->next_linked; di != NULL;
                 di = di->next_linked)
                if (di->direction == 0)
                    di->direction = 1;

            if (part->form & 1) {
                link_objects_in_range(part, TRAIT_IN_MOVING_LIST, -4, 18, 48, 81);

                for (di = part->next_linked; di != NULL;
                     di = di->next_linked) {
                    if (di->kind == KIND_BALLOON) {
                        di->direction = 1;
                    } else if (di->kind == 62) {     /* 1.11 */
                        di->direction = 1;
                    } else if (di->kind == KIND_POKEY && di->form == 0) {
                        di->form = 1;
                        di->kind_state = 0;
                        place_object_for_draw(di);
                        play_sound(7);
                    }
                }
            }
        }
    }
}

/*
 * 190f:370a, image 0x1c7fa - a setup.
 *
 * Four points and a grab box, and it is tall and narrow - 14 by 51 - so the
 * two top corners are inset where the bottom two are not.
 */
void part_setup_rocket(struct part *part)
{
    struct part_point *si;

    part->hold.x = 11;
    part->hold.y = 60;

    si = part->points;
    si->x = 4;
    si->y = 0;
    si++;
    si->x = 10;
    si->y = 0;
    si++;
    si->x = 14;
    si->y = 51;
    si++;
    si->x = 0;
    si->y = 51;

    part_finish_angles(part);
}
