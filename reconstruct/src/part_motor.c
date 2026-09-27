/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The motor**: its step, setup and flip.
 *
 * The fourteenth module of the original's **code segment 172c**, image
 * 0x18689..0x18793 - one module for each kind of part; part_ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 172c:13c9, image 0x18689 - kind 50's step.
 *
 * It passes its own state down its rope - as 1 or -1 by its mirror bit while
 * it is on, and as 0 when it is off - and, while it is on, runs its three
 * frames backwards, wrapping -1 round to 2. The first frame of a turn plays
 * sound 0x0c and sets DGROUP 0x52cd to 2.
 */
void part_step_motor(struct part *part)
{
    struct part *di;

    if ((di = rope_other_end(part)) != PART_NONE && !(di->flags_08 & 0x800)) {
        if (part->direction == 0)
            di->direction = 0;
        else if (part->flags_08 & 0x10)
            di->direction = 1;
        else
            di->direction = -1;
    }

    if (part->direction != 0) {
        DG52BD.sound_request_0c = 2;

        if (part->form == part->form_prev)
            play_sound(0x0c);

        part->form--;
        if (part->form == -1)
            part->form = 2;
    }
}

/*
 * 172c:1435, image 0x186f5 - five slots from one of two tables, chosen by the
 * flag at +8 bit 4, which also decides the grab box's first byte: 0x25 when
 * set and 0 when clear. The other three box bytes are constant.
 */
void part_setup_motor(struct part *part)
{
    const struct point8 *src;
    int16_t i;
    struct part_point *dst;

    if (part->flags_08 & 0x10) {
        part->grab.x = 0x25;
        src = PARTSHAPES.s_32ae;
    } else {
        part->grab.x = 0;
        src = PARTSHAPES.s_32a4;
    }

    part->grab.y = 0x0d;
    part->grab_size = 0x12;

    for (i = 0, dst = POINTS(part->points_ptr); i < 5; i++, dst++, src++) {
        dst->x = src->x;
        dst->y = src->y;
    }

    part_finish_angles(part);
}

/*
 * 172c:149b, image 0x1875b - kind 50's flip.
 *
 * The bit-4 flip, its setup, and **three** marks rather than two:
 * `mark_joined_shapes` as well, which is what a part with something tied to it
 * needs so the other end is redrawn too.
 */
void part_flip_motor(struct part *part)
{
    part->flags_08 ^= 0x10;

    part_setup_motor(part);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
