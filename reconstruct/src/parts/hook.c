/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The hook**: its setup and flip.
 *
 * The seventeenth module of the original's **code segment 172c**, image
 * 0x18c9b..0x18cf2 - one module for each kind of part; parts/ball.c says how
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
 * 172c:19db, image 0x18c9b - a setup, and one of the few that writes no
 * connection points at all: just the two bytes of the grab box.
 *
 * +0x6a is always 7; +0x6b is 0x0e or 1 as **bit 5** of +8 says - which is the
 * bit `part_flip_hook` turns over, where every other flip in this segment uses
 * bit 4. It does not call `part_finish_angles`, because it changed nothing
 * that would need the angles redone.
 */
void part_setup_hook(struct part *part)
{
    part->attach[0].x = 7;

    if (part->flags_08 & PART_FLIP_VERTICAL)
        part->attach[0].y = 14;
    else
        part->attach[0].y = 1;
}

/*
 * 172c:19fa, image 0x18cba - kind 23's flip, and **it turns over bit 5, not
 * bit 4**. Every other flip in this segment xors 0x10; this one xors 0x20, so
 * whatever "the other way round" means for this kind is held somewhere else in
 * the flags. Its setup at 172c:19db is one of the six that write +0x6a.
 */
void part_flip_hook(struct part *part)
{
    part->flags_08 ^= PART_FLIP_VERTICAL;

    part_setup_hook(part);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);
    mark_needs_refile(part, 2);
}
