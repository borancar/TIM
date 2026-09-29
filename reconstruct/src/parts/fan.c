/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The fan**: its setup, step and flip.
 *
 * The eighteenth module of the original's **code segment 172c**, image
 * 0x18cf2..0x18ea9 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x32c8..0x32dc
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x32c8..0x32d2. Connection points, 5 pairs.
 */
struct point8 FAN_POINTS_32C8[5] = {
    { 0x00, 0x0b }, { 0x16, 0x00 }, { 0x1f, 0x0e }, { 0x17, 0x1f },
    { 0x03, 0x1f },
};

/*
 * DGROUP 0x32d2..0x32dc. Connection points, 5 pairs.
 */
struct point8 FAN_POINTS_32D2[5] = {
    { 0x00, 0x0e }, { 0x09, 0x00 }, { 0x1f, 0x0b }, { 0x1c, 0x1f },
    { 0x08, 0x1f },
};

/*
 * 172c:1a32, image 0x18cf2 - a setup.
 *
 * Five points, bit 4 of +8 choosing 0x32d2 or 0x32c8.
 */
void part_setup_fan(struct part *part)
{
    const struct point8 *si;
    struct part_point *di;
    int16_t i;

    if (part->flags_08 & 0x10)
        si = FAN_POINTS_32D2;
    else
        si = FAN_POINTS_32C8;

    for (i = 0, di = part->points; i < 5; i++, di++, si++) {
        di->x = si->x;
        di->y = si->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:1a82, image 0x18d42 - kind 24's step. The fan.
 *
 * Four frames on a loop while it is on, the first playing sound 9, and DGROUP
 * 0x52cf set to 2. The blast is a box reaching 0x100 out in the direction the
 * mirror bit says and ten up, and everything in it is pushed.
 *
 * The push is not a constant: it is the fan's force times how much slower than
 * 0x100 the thing is already going, shifted down eight and then *divided by the
 * thing's own mass*. So a heavy object barely moves and one already at full
 * speed is not pushed at all.
 *
 * Two kinds answer differently. Kind 0x28 with bit 13 of +6 is switched on
 * rather than pushed, with its +0x9c set to 0x14 - but only below 0xc8, so a
 * fast one is left alone. Kind 0x2d is reset to its first frame after being
 * pushed.
 */
void part_step_fan(struct part *part)
{
    struct part *si;
    int16_t push;                       /* [bp-2] */
    int16_t v;                          /* [bp-4] */
    int16_t scale;                      /* [bp-6] */
    int32_t force;                      /* [bp-0xa] */

    if (part->direction != 0) {
        DG52BD.sound_request_09 = 2;

        if (part->form == part->form_prev)
            play_sound(9);

        part->form++;
        if (part->form == 4)
            part->form = 0;

        if (part->flags_08 & 0x10) {
            link_nearby_objects(part, 0x3000, (int16_t)0xff00, 0, -10, 0);
            push = (int16_t)0xf000;
        } else {
            link_nearby_objects(part, 0x3000, 0, 0x100, -10, 0);
            push = 0x1000;
        }

        for (si = part->next_linked; si != NULL;
             si = si->next_linked) {
            if (si->flags_06 & 0x2000) {
                if (si->kind == KIND_WINDMILL && abs(si->link_dx) < 0xc8) {
                    si->direction = 1;
                    si->spin = 0x14;
                }
            } else {
                scale = 0x100 - abs(si->link_dx);
                force = mul16x16(push, scale);
                force >>= 8;
                v = force / PART_KINDS[si->kind].weight;
                si->vel_x += v;

                clamp_record_pair(si);

                if (si->kind == KIND_CANDLE)
                    si->form = si->direction = si->spin = 0;
            }
        }

        place_object_for_draw(part);
    }
}

/*
 * 172c:1bbd, image 0x18e7d - kind 24's flip. Bit 4, its setup, and the two
 * marks without `mark_joined_shapes`, the same shape as `part_flip_dynamite`.
 */
void part_flip_fan(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_fan(part);

    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
