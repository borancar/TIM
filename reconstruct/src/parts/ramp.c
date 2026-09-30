/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The ramp**: its setup, settle and flip.
 *
 * The twenty-sixth module of the original's **code segment 172c**, image
 * 0x199e8..0x19aa2 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x2ee6..0x2f36
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3344..0x334c. Connection points, 4 pairs.
 */
struct point8 g_ramp_points_3344[4] = {
    { 0x00, 0x00 }, { 0x0f, 0x0f }, { 0x0f, 0x1f }, { 0x00, 0x10 },
};

/*
 * DGROUP 0x334c..0x3354. Connection points, 4 pairs.
 */
struct point8 g_ramp_points_334c[4] = {
    { 0x00, 0x00 }, { 0x1f, 0x0f }, { 0x1f, 0x1f }, { 0x00, 0x10 },
};

/*
 * DGROUP 0x3354..0x335c. Connection points, 4 pairs.
 */
struct point8 g_ramp_points_3354[4] = {
    { 0x00, 0x00 }, { 0x2f, 0x0f }, { 0x2f, 0x1f }, { 0x00, 0x10 },
};

/*
 * DGROUP 0x335c..0x3364. Connection points, 4 pairs.
 */
struct point8 g_ramp_points_335c[4] = {
    { 0x00, 0x00 }, { 0x3f, 0x0f }, { 0x3f, 0x1f }, { 0x00, 0x10 },
};

/*
 * DGROUP 0x3364..0x336c. **Which table of points, by form**: a near pointer each.
 */
struct point8 *g_ramp_point_table_3364[4] = {
    g_ramp_points_3344, g_ramp_points_334c, g_ramp_points_3354, g_ramp_points_335c,
};

/*
 * DGROUP 0x336c..0x3374. Connection points, 4 pairs.
 */
struct point8 g_ramp_points_336c[4] = {
    { 0x00, 0x0f }, { 0x0f, 0x00 }, { 0x0f, 0x10 }, { 0x00, 0x1f },
};

/*
 * DGROUP 0x3374..0x337c. Connection points, 4 pairs.
 */
struct point8 g_ramp_points_3374[4] = {
    { 0x00, 0x0f }, { 0x1f, 0x00 }, { 0x1f, 0x10 }, { 0x00, 0x1f },
};

/*
 * DGROUP 0x337c..0x3384. Connection points, 4 pairs.
 */
struct point8 g_ramp_points_337c[4] = {
    { 0x00, 0x0f }, { 0x2f, 0x00 }, { 0x2f, 0x10 }, { 0x00, 0x1f },
};

/*
 * DGROUP 0x3384..0x338c. Connection points, 4 pairs.
 */
struct point8 g_ramp_points_3384[4] = {
    { 0x00, 0x0f }, { 0x3f, 0x00 }, { 0x3f, 0x10 }, { 0x00, 0x1f },
};

/*
 * DGROUP 0x338c..0x3394. **Which table of points, by form**: a near pointer each.
 */
struct point8 *g_ramp_point_table_338c[4] = {
    g_ramp_points_336c, g_ramp_points_3374, g_ramp_points_337c, g_ramp_points_3384,
};

/*
 * 190f:26e1, image 0x1b7d1 - kind 2's setup. The ramp.
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
        src = g_ramp_point_table_338c[part->form];
    else
        src = g_ramp_point_table_3364[part->form];

    for (i = 0, dst = part->points; i < 4; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 190f:273f, image 0x1b82f - kind 2's settle. The same copy of the dragged
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
 * 190f:276c, image 0x1b85c - **kind 2's flip**, the hook at +0x30 of its kind
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
