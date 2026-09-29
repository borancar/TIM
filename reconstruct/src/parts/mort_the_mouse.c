/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Mort the mouse**: its setup, hit, step and flip.
 *
 * The thirty-third module of the original's **code segment 172c**, image
 * 0x1a72f..0x1a8b4 - one module for each kind of part; parts/ball.c says how
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

/*
 * 172c:346f, image 0x1a72f - a setup.
 *
 * Five points: a flat-bottomed shape with a peak in the middle of its top.
 */
void part_setup_mort_the_mouse(struct part *part)
{
    struct part_point *si = part->points;

    si->x = 0;
    si->y = 6;
    si++;
    si->x = 12;
    si->y = 0;
    si++;
    si->x = 23;
    si->y = 6;
    si++;
    si->x = 23;
    si->y = 10;
    si++;
    si->x = 0;
    si->y = 10;

    part_finish_angles(part);
}

/*
 * 172c:34b5, image 0x1a775 - kind 42's hit test.
 *
 * It reads the thing that hit it and does nothing with it: the mouse is solid
 * and that is all. Answers 1.
 */
uint16_t part_hit_mort_the_mouse(struct part *part)
{
    struct part *other = part->contact;   /* read, and never used */

    (void)other;
    return 1;
}

/*
 * 172c:34d0, image 0x1a790 - kind 42's step. The mouse.
 *
 * It runs when it is startled and then stops. The countdown at +0x96 is how
 * many steps of running are left; each one flips the form between 0 and 1 and
 * moves it three or four pixels the way its mirror bit points - four on the
 * odd frame and three on the even, which is what makes the gait uneven.
 *
 * With the countdown spent it waits for a touch - bit 0 of +6 - and then looks
 * for a kind-0x0c anywhere in a box 0x80 either side and 8 below,
 * `link_nearby_objects` building the candidates. The slowest one it finds
 * decides which way it runs: something moving right sends it left and clears
 * the mirror bit, anything else sends it right. Five steps of running, and a
 * form of 1 to start.
 *
 * A form that has changed is carried into the sixteenths at +0x16 and drawn.
 */
void part_step_mort_the_mouse(struct part *part)
{
    struct part *di;
    int16_t slowest;                    /* [bp-2] */
    int16_t step;                       /* [bp-4] */

    if (part->kind_state != 0) {
        part->kind_state--;
        part->form ^= 1;

        if (part->form != 0)
            step = 4;
        else
            step = 3;

        if (part->flags_08 & 0x10)
            part->pos[0].x += step;
        else
            part->pos[0].x -= step;
    } else if (part->flags_06 & 1) {
        link_nearby_objects(part, 0x1000, (int16_t)0xff80, 0x80, -8, 8);

        slowest = 0x190;

        for (di = part->next_linked; di != NULL;
             di = di->next_linked)
            if (di->kind == KIND_POKEY && abs(di->link_dx) < abs(slowest))
                slowest = di->link_dx;

        if (slowest != 0x190) {
            part->form = 1;
            part->kind_state = 5;

            if (slowest > 0) {
                part->flags_08 &= 0xffef;
                part->pos[0].x -= 3;
            } else {
                part->flags_08 |= 0x10;
                part->pos[0].x += 3;
            }
        }
    }

    if (part->form != part->form_prev) {
        part->fx = part->pos[0].x;
        part->fx <<= 9;
        place_object_for_draw(part);
    }
}

/*
 * 172c:35c7, image 0x1a887 - kind 42's flip, the same as kind 30's.
 */
void part_flip_mort_the_mouse(struct part *part)
{
    part->flags_08 ^= 0x10;
    place_object_for_draw(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
