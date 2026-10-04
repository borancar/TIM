/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The balls**: the setups the bowling ball and the basketball, the cannon ball, and the baseball and the tennis ball share.
 *
 * The first module of the original's **code segment 172c**, image
 * 0x172c0..0x173ed - one module for each kind of part; parts/ball.c says how
 * the boundaries are known. Functions are in address order and each carries
 * the image offset it was read from.
 *
 * **How segment 172c's boundaries are known.** Its 45 modules - 43 of part
 * kinds, then screenshot.c and vgadac.c - were one file, parts.c, until
 * 2026-09-27. Three kinds of evidence place the boundaries:
 *
 * - **The calls.** Borland C++ calls a routine defined earlier in the same
 *   file with a bare `push cs / call`. TLINK rewrites every other far call
 *   into the same segment as `nop / push cs / call`. So a *backward* call
 *   with the `nop` proves a boundary between caller and callee, and one
 *   without it rules a boundary out. The 42 backward calls were checked and
 *   none crosses a line drawn here.
 * - **The kind table.** Between the lines the calls force, a module holds one
 *   kind's hit, setup, step, flip and drive hooks, as `g_part_kinds` names
 *   them. So a module is a kind, and the file is named for it.
 * - **The data.** Each module's `_DATA` is its own run of DGROUP
 *   0x3182..0x355a, in link order: the point tables its setups copy, the
 *   tables of pointers to them, and the few word tables its steps read. A
 *   module with no tables has no run. The conveyor's five bytes are followed
 *   by a pad byte at 0x3335, because the next `_DATA` starts on a word.
 *
 * The judge checks all three: every routine matches, the far calls are
 * checked against their callees, and each module's `_DATA` is compared at
 * the base its references give.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 172c:0001, image 0x172c1 - a setup.
 *
 * Eight points round a 32 by 32 part, written straight out.
 */
void part_setup_big_ball(struct part *part)
{
    struct part_point *si = part->points;

    si->x = 8;
    si->y = 0;
    si++;
    si->x = 23;
    si->y = 0;
    si++;
    si->x = 31;
    si->y = 8;
    si++;
    si->x = 31;
    si->y = 23;
    si++;
    si->x = 23;
    si->y = 31;
    si++;
    si->x = 8;
    si->y = 31;
    si++;
    si->x = 0;
    si->y = 23;
    si++;
    si->x = 0;
    si->y = 8;

    part_finish_angles(part);
}

/*
 * 172c:0065, image 0x17325 - a setup.
 *
 * The same eight-cornered shape on a 23 by 23 part.
 */
void part_setup_cannon_ball(struct part *part)
{
    struct part_point *si = part->points;

    si->x = 7;
    si->y = 0;
    si++;
    si->x = 15;
    si->y = 0;
    si++;
    si->x = 22;
    si->y = 8;
    si++;
    si->x = 22;
    si->y = 15;
    si++;
    si->x = 14;
    si->y = 22;
    si++;
    si->x = 8;
    si->y = 22;
    si++;
    si->x = 0;
    si->y = 15;
    si++;
    si->x = 0;
    si->y = 8;

    part_finish_angles(part);
}

/*
 * 172c:00c9, image 0x17389 - a setup.
 *
 * The same again, smaller still - 15 by 15.
 */
void part_setup_small_ball(struct part *part)
{
    struct part_point *si = part->points;

    si->x = 3;
    si->y = 0;
    si++;
    si->x = 11;
    si->y = 0;
    si++;
    si->x = 14;
    si->y = 4;
    si++;
    si->x = 14;
    si->y = 10;
    si++;
    si->x = 11;
    si->y = 14;
    si++;
    si->x = 3;
    si->y = 14;
    si++;
    si->x = 0;
    si->y = 10;
    si++;
    si->x = 0;
    si->y = 4;

    part_finish_angles(part);
}
