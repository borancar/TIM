/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The mouse cage**: its setup, hit, step and flip, and what sets it off.
 *
 * The thirtieth module of the original's **code segment 172c**, image
 * 0x1a1a1..0x1a2f0 - one module for each kind of part; parts/ball.c says how
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
 * 172c:2ee1, image 0x1a1a1 - a setup.
 *
 * The bounding rectangle again, instruction for instruction the same as
 * 172c:24d0 - two kinds that want the same shape and got their own copy.
 */
void part_setup_mouse_cage(struct part *part)
{
    struct part_point *si;

    si = part->points;
    si->x = si->y = 0;
    si++;
    si->x = part->size[0].width;
    si->y = 0;
    si++;
    si->x = part->size[0].width;
    si->y = part->size[0].height;
    si++;
    si->x = 0;
    si->y = part->size[0].height;

    part_finish_angles(part);
}

/*
 * 172c:2f25, image 0x1a1e5 - kind 6's hit test. The mousetrap.
 *
 * Anything that touches a trap springs it, whatever it was: the hook is the
 * trap's, run on the object that arrived, and the trap itself is the one at
 * that object's +0x84. `trigger_mouse_cage` does the rest.
 */
uint16_t part_hit_mouse_cage(struct part *part)
{
    trigger_mouse_cage(part->contact);
    return 1;
}

/*
 * 172c:2f3e, image 0x1a1fe - kind 6's step. The mousetrap.
 *
 * A trap that is not already going looks for a mouse - kind 0x0c - within
 * 0x10 either side, and the first one it finds sets it off. Going or not, it
 * passes its own state along its rope to whatever is not already busy.
 *
 * While it is going the form flips between two and the countdown at +0x96 runs
 * out; reaching zero switches it off again.
 */
void part_step_mouse_cage(struct part *part)
{
    struct part *di;

    if (part->direction == 0) {
        link_nearby_objects(part, 0x1000, -0x10, 0x10, 0, 0);

        di = part->next_linked;
        while (di != NULL) {
            if (di->kind == KIND_POKEY) {
                part->direction = 1;
                di = NULL;
            } else {
                di = di->next_linked;
            }
        }
    }

    if ((di = rope_other_end(part)) != NULL && !(di->flags_08 & 0x800))
        di->direction = part->direction;

    if (part->direction != 0) {
        part->form ^= 1;
        part->kind_state--;
        if (part->kind_state == 0)
            part->direction = 0;
    }
}

/*
 * 172c:2fba, image 0x1a27a - **kind 6's flip**, the +0x30 hook.
 *
 * The same `xor` of bit 0x10 in +8 that kind 2 uses, so calling it twice
 * restores the part and `part_flip_options` can use it as a test. Where kind 2
 * reloads an outline from a table, this one just moves the **anchor byte** at
 * +0x56: 3 when the bit is set and 0x1e when it is clear. That is the whole
 * difference between the two flips - one changes the shape, the other changes
 * where the shape is held.
 *
 * Then the same three marks, in the same order, with the same arguments.
 */
void part_flip_mouse_cage(struct part *part)
{
    part->flags_08 ^= 0x10;

    if (part->flags_08 & 0x10)
        part->grab.x = 3;
    else
        part->grab.x = 30;

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}

/*
 * 172c:2ffd, image 0x1a2bd
 *
 * Set the mouse cage going, in the direction its mirror bit says. A part that
 * was not going already plays sound 0x0d, and either way its +0x96 is put back
 * to 0x64.
 */
void trigger_mouse_cage(struct part *part)
{
    if (part->direction == 0)
        play_sound(0x0d);

    if (part->flags_08 & 0x10)
        part->direction = -1;
    else
        part->direction = 1;

    part->kind_state = 0x64;
}
