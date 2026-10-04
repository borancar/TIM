/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Kind 58's handlers**, new in 1.11 - by what they do, a trap for kind
 * 54: one that walks into it is caught, and it plays its catch. The name
 * waits for its icon.
 *
 * In 1.11, image 0x1e247.. in the part kinds' code segment. **Both ends are
 * ours**: each kind in 1.00 is a module of its own, and so is this one until
 * the far calls between its routines and its neighbours' say otherwise.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x1e247
 *
 * Any kind 54 on its feet in the trap's mouth, 0x24..0x26 across and
 * 0x28..0x32 down, is caught - gone, and marked (`traits2` 0x80) - and the
 * trap springs: sound 0x17 at form 1, then forms 6..8 over and over.
 */
void part_step_kind_58(register struct part *part)
{
    struct part *what;

    link_objects_in_range(part, 0x1000, 36, 38, 40, 50);
    for (what = part->next_linked; what != 0; what = what->next_linked) {
        if (what->kind == 0x36 && what->form < 6) {
            part->direction = 1;
            mark_part_shapes(what, 3);
            what->state |= STATE_GONE;
            what->traits2 |= 0x80;
        }
    }
    if (part->direction != 0) {
        if (part->form == 1)
            play_sound(0x17);
        part->form++;
        if (part->form == 9)
            part->form = 6;
        place_object_for_draw(part);
    }
}
