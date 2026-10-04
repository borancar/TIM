/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Kind 65's handlers**, new in 1.11 - a part that only has a shape: its
 * setup is all it has of its own. Kind 65 is the one `build_part_list`
 * offers only on St Patrick's day, and its icon is a **shamrock**.
 * `check_briefing --screen bin-stpatrick` places one and runs the machine
 * (`TIM_DATE=03-17`): it leaves the screen on both sides alike.
 *
 * In 1.11, image 0x1e33d.. in the part kinds' code segment. **Both ends are
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
 * DGROUP 0x314e..0x3158: the five points `part_setup_kind_65` copies.
 */
struct point8 g_kind_65_outline[5] = {
    { 0, 11 }, { 13, 0 }, { 28, 15 }, { 12, 28 },
    { 8, 28 },
};

/* 0x1e33d - a setup: five points from DGROUP 0x314e. */
void part_setup_kind_65(struct part *part)
{
    struct part_point *si;
    const struct point8 *di;
    int16_t i;

    di = g_kind_65_outline;
    for (i = 0, si = part->points; i < 5; i++, si++, di++) {
        si->x = di->x;
        si->y = di->y;
    }

    part_finish_angles(part);
}
