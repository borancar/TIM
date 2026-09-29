/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The generator**: its hit, setup and step.
 *
 * The twenty-first module of the original's **code segment 172c**, image
 * 0x190a0..0x191c8 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 172c:1de0, image 0x190a0 - kind 26's hit test. The pulley wheel.
 *
 * Nothing happens. The original still loads the part at the object's +0x84 into
 * a local and then never reads it, which is a hook written from the same
 * template as the ones that do use it - so the wheel is touchable and is
 * unmoved by being touched.
 */
uint16_t part_hit_generator(struct part *part)
{
    struct part *other = PART_PTR(part->contact_ptr);   /* read, and never used */

    (void)other;
    return 1;
}

/*
 * 172c:1dfb, image 0x190bb - a setup.
 *
 * A part 0x47 by 0x1f with its grab box at +0x56..+0x58, four corner points
 * written straight out, and **the form recomputed after the finish**: +0x0c is
 * masked to its bottom two bits and then bit 2 or bit 3 set for each of the
 * two links that is attached.
 *
 * That is `part_setup_solar_panel`'s question asked one field along - there it is
 * bits 0 and 1 of a +0x0c that starts at zero, here bits 2 and 3 of one that
 * keeps whatever two bits it already had.
 */
void part_setup_generator(struct part *part)
{
    struct part_point *si;

    part->grab.x = 56;
    part->grab.y = 18;
    part->grab_size = 0x0c;

    si = POINTS(part->points_ptr);

    si->x = 21;
    si->y = 0;
    si++;
    si->x = 71;
    si->y = 0;
    si++;
    si->x = 71;
    si->y = 31;
    si++;
    si->x = 21;
    si->y = 31;

    part_finish_angles(part);

    part->form &= 3;

    if (part->link_ptr[4] != 0)
        part->form |= 4;

    if (part->link_ptr[5] != 0)
        part->form |= 8;
}

/*
 * 172c:1e5c, image 0x1911c - kind 26's step. The pulley wheel.
 *
 * It stops if the gear its rope reaches is not turning - kind 0x0e with its
 * last two forms equal - and otherwise runs its four frames in the direction
 * its +0x12 says, wrapping within the low two bits so the form's other bits
 * survive the turn. The first frame plays sound 0x0c and sets DGROUP 0x52cd.
 *
 * Whatever happens it passes its own state on to links 4 and 5.
 */
void part_step_generator(struct part *part)
{
    struct part *di;
    int16_t i;                          /* [bp-2] */

    if (part->direction != 0 && (di = rope_other_end(part)) != PART_NONE
        && di->kind == KIND_GEAR && di->form_prev == di->form_prev2)
        part->direction = 0;

    if (part->direction != 0) {
        DG52BD.sound_request_0c = 2;

        if (part->form == part->form_prev)
            play_sound(0x0c);

        if (part->direction > 0) {
            if ((part->form & 3) == 3)
                part->form -= 3;
            else
                part->form++;
        } else {
            if ((part->form & 3) == 0)
                part->form += 3;
            else
                part->form--;
        }

        place_object_for_draw(part);
    }

    for (i = 4; i < 6; i++)
        if ((di = PART_PTR(part->link_ptr[i])) != PART_NONE)
            di->direction = part->direction;
}
