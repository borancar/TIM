/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The ramp**: its setup, settle and flip.
 *
 * The twenty-sixth module of the original's **code segment 172c**, image
 * 0x199e8..0x19aa2 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3344..0x3394
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3344..0x3364. Connection points, 4 pairs a row, indexed [form].
 */
struct point8 g_ramp_points[4][4] = {
    {
        { 0, 0 }, { 15, 15 }, { 15, 31 }, { 0, 16 },
    },
    {
        { 0, 0 }, { 31, 15 }, { 31, 31 }, { 0, 16 },
    },
    {
        { 0, 0 }, { 47, 15 }, { 47, 31 }, { 0, 16 },
    },
    {
        { 0, 0 }, { 63, 15 }, { 63, 31 }, { 0, 16 },
    },
};

/*
 * DGROUP 0x3364..0x336c. **Which row of points, by form**: a near pointer each.
 */
struct point8 *g_ramp_point_table[4] = {
    g_ramp_points[0], g_ramp_points[1], g_ramp_points[2], g_ramp_points[3],
};

/*
 * DGROUP 0x336c..0x338c. Connection points, 4 pairs a row, indexed [form].
 */
struct point8 g_ramp_points_flipped[4][4] = {
    {
        { 0, 15 }, { 15, 0 }, { 15, 16 }, { 0, 31 },
    },
    {
        { 0, 15 }, { 31, 0 }, { 31, 16 }, { 0, 31 },
    },
    {
        { 0, 15 }, { 47, 0 }, { 47, 16 }, { 0, 31 },
    },
    {
        { 0, 15 }, { 63, 0 }, { 63, 16 }, { 0, 31 },
    },
};

/*
 * DGROUP 0x338c..0x3394. **Which row of points, by form**: a near pointer each.
 */
struct point8 *g_ramp_point_table_flipped[4] = {
    g_ramp_points_flipped[0], g_ramp_points_flipped[1], g_ramp_points_flipped[2], g_ramp_points_flipped[3],
};

/*
 * 172c:2728, image 0x199e8 - kind 2's setup. The ramp.
 *
 * Four points, from one of two tables of offsets picked by the mirror bit at
 * +8 and indexed by the form. The same shape as the bellow's and the
 * scissors' - `shl bx,1` on the form, `[bx + 0x338c]` or `[bx + 0x3364]` -
 * and the copy steps the source two bytes at a time and the destination four.
 */
void part_setup_ramp(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->state & STATE_FLIP_HORIZONTAL)
        src = g_ramp_point_table_flipped[part->form];
    else
        src = g_ramp_point_table[part->form];

    for (i = 0, dst = part->points; i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:2789, image 0x19a49 - kind 2's settle. The same copy of the dragged
 * size into the real one, a form of `width / 0x10 - 1`, and then its own setup
 * at 172c:2728 to rebuild the connection points from it.
 */
void part_settle_ramp(struct part *part)
{
    part->size[0] = part->set_size;
    part->start_form = part->form = part->size[0].width / 0x10 - 1;

    part_setup_ramp(part);
}

/*
 * 172c:27b6, image 0x19a76 - **kind 2's flip**, the hook at +0x30 of its kind
 * record that `part_flip_options` calls to try an end and then put it back.
 *
 * Flipping is one `xor` of bit 0x10 in +8, which is why calling it twice with
 * the same argument restores the part exactly - the caller relies on that, and
 * it is the whole reason a routine that changes the machine can be used as a
 * test.
 *
 * The argument the caller pushes past the part is **not read**: the frame
 * takes only [bp+6]. Kind 2 has one flip, so which end was asked for makes no
 * difference to it.
 *
 * The outline is reloaded for the new bit, and the part is then marked twice -
 * `mark_part_shapes` with 3 and `mark_needs_refile` with 2 - so what was drawn
 * for the old orientation is re-filed for the new one.
 */
void part_flip_ramp(struct part *part)
{
    part->state ^= STATE_FLIP_HORIZONTAL;

    part_setup_ramp(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
