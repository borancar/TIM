/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Setting off what the seesaw lands on**: `trigger_things_at`.
 *
 * The forty-first module of the original's **code segment 172c**, image
 * 0x1ba3d..0x1bb6b; parts/ball.c says how the boundaries are known. Functions
 * are in address order and each carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: via-assembler
 * JUDGE: assembler bc3.00
 *
 * **This module went through the assembler, and that is the whole of the
 * evidence for its boundary.** The switch's cases 6 and 0x0f both end in a
 * call and a `pop cx`, and the image shares the one after case 0x0f, which
 * ends `pop cx / jmp $+2` - a jump to the next instruction, which Borland
 * C++'s own object writer removes and TASM does not. Compiled straight to an
 * object the compiler shares the other copy instead, whatever the source
 * says; through `-S` and TASM it is the image, byte for byte. The seesaw's
 * own routines cannot have gone the same way: through TASM a call to a
 * routine later in the same file is a bare `push cs / call`, where the image
 * has TLINK's `nop` in front of every one, and `part_setup_seesaw`'s jumps
 * are sized differently. So a module ends before this routine. Whether
 * `drive_ropes` and `push_speed_for_mass` are on this side of that boundary
 * or the seesaw's is not settled: both are byte-exact either way, and no
 * call or data reference tells them apart. The original presumably held an
 * `asm` statement to send it through TASM; nothing of one is left in the
 * bytes, so the marker above stands in for it.
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 172c:477d, image 0x1ba3d
 *
 * Set going whatever is in the chain at +0x78, at a point `dx` from the part's
 * own position.
 *
 * The original dispatches on the kind through a jump table in its own code
 * segment - six kinds at 172c:4893 and six targets twelve bytes after them -
 * which is the compiler's `switch`, so the port writes it as one.
 *
 * Four of the six turn on only if the thing is within a window of the point,
 * and the window depends on the mirror bit: kind 0x10 at 0x36..0x3c or 0..8,
 * kind 0x25 at 0x19..0x25 or 0..0x0c, and kinds 0x19 and 0x16 the same but
 * only when `mode` is 1. The other two are handed to the routines that already
 * know what to do with them.
 */
void trigger_things_at(struct part *part, int16_t mode, int16_t dx)
{
    struct part *si;
    int16_t d;                          /* di */
    int16_t x;                          /* [bp-2] */

    x = part->pos[0].x + dx;

    for (si = part->next_linked; si != NULL;
         si = si->next_linked) {
        d = x - si->pos[0].x;

        switch (si->kind) {
        case 0x10:
            if (si->state & STATE_FLIP_HORIZONTAL) {
                if (d >= 54 && d <= 60)
                    si->direction = 1;
            } else if (d >= 0 && d <= 8) {
                si->direction = 1;
            }
            break;
        case 0x06:
            trigger_mouse_cage(si);
            break;
        case 0x25:
            if (si->state & STATE_FLIP_HORIZONTAL) {
                if (d >= 25 && d <= 37)
                    si->direction = 1;
            } else if (d >= 0 && d <= 12) {
                si->direction = 1;
            }
            break;
        case 0x19:
            if (mode == 1) {
                if (si->state & STATE_FLIP_HORIZONTAL) {
                    if (d >= 13 && d <= 24)
                        si->direction = 1;
                } else if (d >= 5 && d <= 16) {
                    si->direction = 1;
                }
            }
            break;
        case 0x16:
            if (mode == 1) {
                if (si->state & STATE_FLIP_HORIZONTAL) {
                    if (d >= 0 && d <= 31)
                        si->direction = 1;
                } else if (d >= 103 && d <= 135) {
                    si->direction = 1;
                }
            }
            break;
        case 0x0f:
            break_bob_the_fish(si);
            break;
        }
    }
}
