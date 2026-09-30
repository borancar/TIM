/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The solar panel**: its setup and step.
 *
 * The thirty-eighth module of the original's **code segment 172c**, image
 * 0x1b0a5..0x1b17f - one module for each kind of part; parts/ball.c says how
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
 * 190f:3d55, image 0x1ce45 - no slots at all, and no finish. It only turns
 * the two part numbers at +0x62 and +0x64 into two bits of the form at
 * +0x0c, so a part that was read off disk with those links set comes out in
 * the form that matches them.
 */
void part_setup_solar_panel(struct part *part)
{
    part->form = 0;
    if (part->link[4] != 0)
        part->form |= 1;
    if (part->link[5] != 0)
        part->form |= 2;
}

/*
 * 190f:3d58, image 0x1ce48 - kind 38's step. **It looks around, but only every
 * eighth frame.**
 *
 * The frame counter at 0x4ea7 masked to 3 bits must read 4, so seven frames in
 * eight this does nothing but pass its state on. On the eighth it clears its
 * own +0x12 and asks `link_nearby_objects` for everything within 0x1a in each
 * direction; anything with a +0x12 of its own sets this part's, either
 * outright for kinds 0x1d, 0x2d and 0x29, or for kind 0x19 only when the sign
 * of its +0x7a and bit 4 of its flags **disagree** - so a kind-0x19 part
 * facing the wrong way is ignored.
 *
 * Then, every frame, +0x12 is passed to whatever the two words at +0x62 and
 * +0x64 point at. The loop runs its index from 4 to 5 over a table based at
 * +0x5a, which is those two and no others.
 */
void part_step_solar_panel(struct part *part)
{
    struct part *si;
    int16_t i;                          /* [bp-2] */

    part->state |= STATE_STEPPED;

    if ((g_machine_frames & 7) == 4) {
        part->direction = 0;

        link_nearby_objects(part, (TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST), -0x1a, 0x1a, -0x1a, 0x1a);

        for (si = part->next_linked; si != NULL;
             si = si->next_linked) {
            if (si->direction != 0) {
                if (si->kind == KIND_LIGHT || si->kind == KIND_CANDLE
                    || si->kind == KIND_BLAST) {
                    part->direction = 1;
                } else if (si->kind == KIND_FLASHLIGHT) {
                    if (si->link_dx < 0) {
                        if (!(si->state & STATE_FLIP_HORIZONTAL))
                            part->direction = 1;
                    } else {
                        if (si->state & STATE_FLIP_HORIZONTAL)
                            part->direction = 1;
                    }
                }
            }
        }
    }

    for (i = 4; i < 6; i++)
        if ((si = part->link[i]) != NULL)
            si->direction = part->direction;
}
