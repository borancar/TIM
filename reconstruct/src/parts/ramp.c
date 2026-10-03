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
 * DGROUP 0x2ee6..0x2f06. Connection points, 4 pairs a row, indexed [form].
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
 * DGROUP 0x2f06..0x2f0e. **Which row of points, by form**: a near pointer each.
 */
struct point8 *g_ramp_point_table[4] = {
    g_ramp_points[0], g_ramp_points[1], g_ramp_points[2], g_ramp_points[3],
};

/*
 * DGROUP 0x2f0e..0x2f2e. Connection points, 4 pairs a row, indexed [form].
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
 * DGROUP 0x2f2e..0x2f36. **Which row of points, by form**: a near pointer each.
 */
struct point8 *g_ramp_point_table_flipped[4] = {
    g_ramp_points_flipped[0], g_ramp_points_flipped[1],
    g_ramp_points_flipped[2], g_ramp_points_flipped[3],
};

#ifndef __TURBOC__
extern struct draw_step g_kind_51_draw_steps[9];    /* gamedata.c */
struct game_copy_protection {                       /* intro.c's */
    int16_t   answer[3][16];
} PACKED;
extern struct game_copy_protection g_game_copy_protection;

/*
 * OURS: **what the original reads for form -1.** Puzzles 117 and 143 have
 * a ramp whose form is 0xffff in the level file, and the original indexes
 * its table of point tables at -1: the word before the table, which is the
 * last point of the table in front of it read as a near pointer - 0x1000
 * for the plain table, 0x1f00 for the mirrored one - and four points from
 * there. Both land in constant data: 0x1000 is the last byte of
 * `g_kind_51_draw_steps[4]` and the first seven of `[5]` (its null `next`,
 * its level and its four frames), 0x1f00 is `g_game_copy_protection`'s
 * `answer[0][5..8]`. A draw step holds a pointer, so its host layout is not
 * the image's and the eight bytes are put together from the same fields;
 * the answers are words on both, so they are read where they are.
 */
static const struct point8 *ramp_points_before_table(int16_t flipped)
{
    static struct point8 at_1000[4];
    const struct draw_step *a = &g_kind_51_draw_steps[4];
    const struct draw_step *b = &g_kind_51_draw_steps[5];

    if (flipped)
        return (const struct point8 *)&g_game_copy_protection.answer[0][5];

    at_1000[0].x = (uint8_t)a->offset[3].y;
    at_1000[0].y = 0;                   /* `b->next`, null: two zero bytes */
    at_1000[1].x = 0;
    at_1000[1].y = b->level;
    at_1000[2].x = b->frame[0];
    at_1000[2].y = b->frame[1];
    at_1000[3].x = b->frame[2];
    at_1000[3].y = b->frame[3];
    return at_1000;
}
#endif

/*
 * 190f:26e1, image 0x1b7d1 - kind 2's setup. The ramp.
 *
 * Four points, from one of two tables of offsets picked by the mirror bit at
 * +8 and indexed by the form. The same shape as the bellow's and the
 * scissors' - `shl bx,1` on the form, then `g_ramp_point_table_flipped[bx]`
 * or `g_ramp_point_table[bx]` -
 * and the copy steps the source two bytes at a time and the destination four.
 */
void part_setup_ramp(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

#ifdef __TURBOC__
    if (part->state & STATE_FLIP_HORIZONTAL)
        src = g_ramp_point_table_flipped[part->form];
    else
        src = g_ramp_point_table[part->form];
#else
    if (part->state & STATE_FLIP_HORIZONTAL)
        src = part->form >= 0 ? g_ramp_point_table_flipped[part->form]
                              : ramp_points_before_table(1);
    else
        src = part->form >= 0 ? g_ramp_point_table[part->form]
                              : ramp_points_before_table(0);
#endif

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
