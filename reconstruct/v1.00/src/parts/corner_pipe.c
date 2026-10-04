/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The corner pipe**: its setup and flip.
 *
 * The thirty-sixth module of the original's **code segment 172c**, image
 * 0x1aa3b..0x1aae4 - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x3432..0x3472
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * DGROUP 0x3432..0x3472. Connection points, 8 pairs a row, indexed [form].
 */
struct point8 g_corner_pipe_points[4][8] = {
    {
        { 0, 0 }, { 19, 1 }, { 30, 12 }, { 31, 31 },
        { 16, 31 }, { 16, 20 }, { 11, 15 }, { 0, 15 },
    },
    {
        { 0, 31 }, { 1, 12 }, { 12, 1 }, { 31, 0 },
        { 31, 15 }, { 20, 15 }, { 15, 20 }, { 15, 31 },
    },
    {
        { 0, 16 }, { 11, 16 }, { 16, 11 }, { 16, 0 },
        { 31, 0 }, { 30, 19 }, { 19, 30 }, { 0, 31 },
    },
    {
        { 0, 0 }, { 15, 0 }, { 15, 11 }, { 20, 16 },
        { 31, 16 }, { 31, 31 }, { 12, 30 }, { 1, 19 },
    },
};

/*
 * 172c:377b, image 0x1aa3b - kind 47's setup. The corner pipe.
 *
 * Eight points, from one of four tables chosen by comparing the form against
 * 0, 1 and 2 one at a time - `cmp [si+0xc], 0 / jne` and so on - with the
 * fourth for anything else.
 */
void part_setup_corner_pipe(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->form == 0)
        src = g_corner_pipe_points[0];
    else if (part->form == 1)
        src = g_corner_pipe_points[1];
    else if (part->form == 2)
        src = g_corner_pipe_points[2];
    else
        src = g_corner_pipe_points[3];

    for (i = 0, dst = part->points; i < 8; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:37e5, image 0x1aaa5 - kind 47's flip, and **the only one that reads the
 * second argument**.
 *
 * Every other flip in this segment takes the part alone. This one tests
 * [bp+8], which `part_flip_options` passes as 1 for the X key and 2 for the Y
 * key, and turns over bit 0 of the form at +0x0c for X and bit 1 for Y. So the
 * part has two independent axes held in one word, which is what X and Y
 * flipping separately means for it.
 *
 * The port's old flip dispatcher had `(void)which` and threw that away, so
 * this kind would have flipped the same axis whichever key was pressed. The three
 * flips written before this one were re-read to check they really do take the
 * part alone; they do.
 */
void part_flip_corner_pipe(struct part *part, uint16_t which)
{
    if (which == 1)
        part->form ^= 1;
    else
        part->form ^= 2;

    part->start_form = part->form;

    part_setup_corner_pipe(part);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
