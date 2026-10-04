/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The trampoline**: its hit, setup and step.
 *
 * The thirty-ninth module of the original's **code segment 172c**, image
 * 0x1b17f..0x1b2a8 - one module for each kind of part; parts/ball.c says how
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
 * 172c:3ebf, image 0x1b17f - kind 39's hit test.
 *
 * The hook belongs to the kind 39 part - `di`, at the arriving object's +0x84 -
 * and runs on whatever arrived, `si`.
 *
 * It only catches a thing that lands squarely on it: an object already spoken
 * for at +0x8a is refused, and so is one whose middle is more than 14 across
 * from the part's own middle, both by answering 1.
 *
 * Caught, the part is set going at +0x12 and the object is let go: a sideways
 * speed of 0x400 or more is halved, the link at +0x84 is dropped and the "in
 * contact" bit of +6 with it, and the sixteenths at +0x1a are rebuilt from the
 * whole pixels so the release leaves no fractional position behind.
 *
 * Form 3 - and only form 3 - throws it as well: the downward speed at +0x38
 * becomes minus its own size, less another 0x400, and `clamp_record_pair` holds
 * that to what the kind allows.
 */
uint16_t part_hit_trampoline(struct part *part)
{
    struct part *di = part->contact;
    int16_t edge = part->contact_edge;  /* [bp-2] */
    int16_t apart;                      /* [bp-4] */

    if (edge == 0) {
        apart = part->pos[0].x + (part->size[0].width >> 1);
        apart -= di->pos[0].x + (di->size[0].width >> 1);

        if (apart < -0x0e || apart > 0x0e)
            return 1;

        di->direction = 1;

        if (abs(part->vel_x) >= 0x400)
            part->vel_x >>= 1;

        part->contact = 0;
        part->traits &= ~TRAIT_ON_SURFACE;

        part->fy = part->pos[0].y;
        part->fy <<= 9;

        if (di->form == 3) {
            part->vel_y = 0 - abs(part->vel_y);
            part->vel_y -= 0x400;
            clamp_record_pair(part);
        }

        return 0;
    }

    return 1;
}

/*
 * 172c:3f72, image 0x1b232 - a setup.
 *
 * A wide box, 47 by 16, sitting 11 down from the part's origin.
 */
void part_setup_trampoline(struct part *part)
{
    struct part_point *si = part->points;

    si->x = 0;
    si->y = 11;
    si++;
    si->x = 47;
    si->y = 11;
    si++;
    si->x = 47;
    si->y = 27;
    si++;
    si->x = 0;
    si->y = 27;

    part_finish_angles(part);
}

/*
 * 172c:3fae, image 0x1b26e - kind 39's step.
 *
 * Five frames once it is set going, the third playing sound 3, and reaching
 * the fifth wraps the form back to 0 and switches it off - so it plays through
 * and stops rather than looping.
 */
void part_step_trampoline(struct part *part)
{
    if (part->direction != 0) {
        part->form++;

        if (part->form == 3)
            play_sound(3);

        if (part->form == 5) {
            part->form = 0;
            part->direction = 0;
        }

        place_object_for_draw(part);
    }
}
