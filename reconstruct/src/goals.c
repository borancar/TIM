/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The goal tests**, one per puzzle, the step and time counters, and
 * `finish_level`, which scores a solved level and offers the next.
 *
 * A module of the original's **code segment 0000** (`_TEXT`), from image
 * 0x01476; split out of machine.c on 2026-09-27. Its `_DATA` is
 * `finish_level`'s literal pool, DGROUP 0x283a..0x2849, between screen.c's
 * and the next module's in the link order, and its `_BSS` is `g_belt_far_end` and
 * `g_goal_condition`, 0x5456..0x546c, between puzzles.c's and levels.c's - which is what puts
 * the goal tests and `finish_level` in one module.
 *
 * **Where it ends is not settled.** This file stops after `finish_level`,
 * at 0x02809, but nothing proves a module ends there: the next proven
 * boundary is somewhere in 0x036de..0x03ba9 (`rehome_carried_part` and
 * `outlines_cross` between them), and the score code and physics between
 * have no data of their own to say which module they are in. Its start,
 * where the goal tests begin, is not proven either - see runloop.c.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 * JUDGE: data 0x283a..0x2849
 */
#ifdef __TURBOC__
#include <stdlib.h>
#else
#include "hostlib.h"
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* **This module's `_BSS`**, DGROUP 0x5456..0x546c: the goal conditions. */
struct part *g_belt_far_end;   /* DGROUP 0x5456  the far end's +0x5a, stashed while it is detached */
/* **Ten words the goal tests keep between frames**, and what each means
   depends on the test. `goal_test_puzzles_19_48` at 0x01bb4 does
   `inc word ptr [0x5458]` - a count of frames the goal has held, and
   passing 0xc wins. `goal_test_puzzle_78` at 0x015bf does
   `cmp word ptr [bx + 0x5458], 0` with `bx` twice the mouse-cage count: a
   flag per cage. The ten is `clear_machine`'s, which zeroes them with
   `cmp si, 0xa / jl` over a word stride; nothing yet says the table is no
   longer than that. */
uint16_t g_goal_condition[10];   /* DGROUP 0x5458..0x546c */

/*
 * 0x01476 - every kind-0 part must sit at 0x108 with +0x20 equal to +0x24.
 */
void goal_test_puzzle_2(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BOWLING_BALL) {
            if (((uint16_t)si->pos[0].y) != 0x108
                || ((uint16_t)si->pos[0].y)
                   != ((uint16_t)si->pos[1].y))
                ok = 0;
        }
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x014ad - **the first object only**, no walk at all: `[0x5179]` is taken and
 * its link is never followed. Its x must be past 0x1e0 and its y exactly 0xc8.
 */
void goal_test_puzzle_20(void)
{
    struct part *si = g_moving_parts.next;

    if ((int16_t)((uint16_t)si->pos[0].x) > 0x1e0
        && ((uint16_t)si->pos[0].y) == 0xc8)
        g_round_state = 0x200;
}

/*
 * 0x014cc - walk to the first kind-0xc part and ask whether its y is past
 * 0x12c. The walk is unguarded, like `goal_test_puzzle_1`'s: a level that selects
 * this goal is a level that has one.
 */
void goal_test_puzzle_21(void)
{
    struct part *si = g_moving_parts.next;

    while (si->kind != KIND_POKEY)
        si = si->next;

    if ((int16_t)((uint16_t)si->pos[0].y) > 0x12c)
        g_round_state = 0x200;
}

/*
 * 0x014ee - **count, do not test.** Every kind-4 part whose +0x0c is zero adds
 * one, and the goal is met while fewer than two of them are left.
 */
void goal_test_puzzle_22(void)
{
    register struct part *si;
    int16_t n;

    n = 0;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BALLOON
            && si->form == 0)
            n++;
        si = si->next;
    }

    if (n < 2)
        g_round_state = 0x200;
}

/*
 * 0x0151b - the kind-9 part must be inside a box: +0x1e strictly between
 * 0x1a8 and 0x1da, +0x20 strictly between 0x88 and 0x98.
 *
 * **The search has no end test.** `while (si->+4 != 9) si = si->+0;` walks off
 * the end of the list if there is no kind-9 part, which the original does too:
 * a level that selects this goal is a level that has one. Transcribed as the
 * unguarded walk it is.
 */
void goal_test_puzzle_1(void)
{
    struct part *si = g_moving_parts.next;

    while (si->kind != KIND_BASKETBALL)
        si = si->next;

    if ((int16_t)((uint16_t)si->pos[0].x) > 0x1a8
        && (int16_t)((uint16_t)si->pos[0].x) < 0x1da
        && (int16_t)((uint16_t)si->pos[0].y) > 0x88
        && (int16_t)((uint16_t)si->pos[0].y) < 0x98)
        g_round_state = 0x200;
}

/*
 * 0x01552 - **four kinds, and a memory.** The picker's walk, and the longest
 * of the goal tests bar one.
 *
 * A kind-0xc part must be past its own +0x8c in x and at 0 in +0x0c; a kind-0xf
 * must be under 0xb in +0x0c; a kind-0xb must be short of its +0x8e once 8 is
 * added to its y. Any of those failing fails the goal.
 *
 * The kind-6 parts are counted as they are met, and each gets its own word of
 * the run at DGROUP 0x5458: a part that is on sets its word, and a part that is
 * off **and whose word was never set** fails. So a kind-6 part has to have been
 * on at some point in the run, not now - which is what the words are for, since
 * they survive between frames.
 */
void goal_test_puzzle_78(void)
{
    register struct part *si;
    int16_t ok;
    int16_t n;

    n = 0;
    ok = 1;
    si = pick_by_flag((PART_IN_PLACED_LIST | PART_IN_MOVING_LIST));

    while (si != NULL) {
        if (si->kind == KIND_POKEY) {
            if ((int16_t)((uint16_t)si->pos[0].x)
                <= (int16_t)si->start_x
                || si->form != 0)
                ok = 0;
        }

        if (si->kind == KIND_BOB_THE_FISH
            && (int16_t)si->form >= 0x0b)
            ok = 0;

        if (si->kind == KIND_BIRD_CAGE
            && (int16_t)(((uint16_t)si->pos[0].y) + 8)
               >= (int16_t)si->start_y)
            ok = 0;

        if (si->kind == KIND_MOUSE_CAGE
            && (si->flags_06 & PART_FROM_LEVEL) != 0) {
            if (si->direction == 0) {
                if (g_goal_condition[n] == 0)
                    ok = 0;
            } else
                g_goal_condition[n] = 1;

            n++;
        }

        si = pick_for_record(si, PART_IN_MOVING_LIST);
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x015fa - **pop all the balloons.** A kind-4 part that is still there, bit
 * 0x8000 in its +6, and not yet popped, bit 0x2000 clear in its +8, is one the
 * player has not dealt with, and one of those is enough to fail. So the level
 * is won when every balloon is either gone or popped.
 *
 * **This was transcribed inverted and the level won on the first frame.** The
 * original's three tests are all *skips* - `jne`, `je`, `jne` to the next
 * object - so the object that reaches `xor dx, dx` is the one that satisfies
 * all three, and the port had read them as the conditions for passing rather
 * than for failing. With every balloon present and unpopped the port answered
 * "solved" before the machine had run a frame.
 *
 * Reading a chain of skips as a chain of requirements inverts the whole test
 * and nothing about the C looks wrong afterwards; only running it says so. The
 * other six goal tests were re-read against the disassembly after this and are
 * right.
 */
void goal_test_pop_balloons(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BALLOON
            && (si->flags_06 & PART_FROM_LEVEL) != 0
            && (si->flags_08 & PART_GONE) == 0)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01630 - **two parts, each in its own box.** The picker's walk keeps the
 * last kind-0 part and the last kind-9 part it meets, and then asks whether
 * each is where it should be: the kind-9 between 0x148 and 0x198 in x and past
 * 0x11c in y, the kind-0 between 0x1c8 and 0x21a and past 0x11c.
 *
 * **Neither is initialised before the walk.** A level with no kind-0 part
 * reaches the test with whatever was in DI, and one with no kind-9 with
 * whatever was on the stack. The port starts both at zero, which is one
 * definite wrong answer where the original has an indefinite one; the levels
 * that select this goal have both parts.
 */
void goal_test_puzzle_79(void)
{
    struct part *nine;                  /* [bp-2] */
    register struct part *si;
    register struct part *zero;

#ifndef __TURBOC__
    zero = nine = NULL;    /* ours: the original leaves both unset */
#endif
    for (si = pick_by_flag((PART_IN_PLACED_LIST | PART_IN_MOVING_LIST)); si != NULL;
         si = pick_for_record(si, PART_IN_MOVING_LIST)) {
        if (si->kind == KIND_BOWLING_BALL)
            zero = si;
        if (si->kind == KIND_BASKETBALL)
            nine = si;
    }

    if (nine->pos[0].x > 0x148
        && nine->pos[0].x < 0x198
        && nine->pos[0].y > 0x11c
        && zero->pos[0].x > 0x1c8
        && zero->pos[0].x < 0x21a
        && zero->pos[0].y > 0x11c)
        g_round_state = 0x200;
}

/*
 * 0x016a6 - **two kinds, opposite sides.** Every kind-0x1c part must sit
 * between 0x1e8 and 0x210 in x and at 0x110 or below in y; every kind-0x2c
 * part must be *left* of 0x1e8 and at 0x110 or below. One that is not fails
 * the whole test.
 */
void goal_test_puzzle_23(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASEBALL) {
            if ((int16_t)((uint16_t)si->pos[0].x) < 0x1e8
                || (int16_t)((uint16_t)si->pos[0].x) > 0x210
                || (int16_t)((uint16_t)si->pos[0].y) < 0x110)
                ok = 0;
        } else if (si->kind == KIND_TENNIS_BALL) {
            if ((int16_t)((uint16_t)si->pos[0].x) >= 0x1e8
                || (int16_t)((uint16_t)si->pos[0].y) < 0x110)
                ok = 0;
        }
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x016fb - a kind-0 part whose +0x8e is past 0x64 and whose y stands exactly
 * 0x40 above it. +0x8e is read as where the part started, so this is "it has
 * fallen 64 and no more", but that is a reading of the arithmetic.
 */
void goal_test_puzzle_26(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BOWLING_BALL
            && (int16_t)si->start_y > 0x64
            && (uint16_t)(((uint16_t)si->pos[0].y)
                          - si->start_y) == 0x40)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x0172d - any kind-0x11 part at exactly y 0x118.
 */
void goal_test_puzzle_43(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BUCKET
            && ((uint16_t)si->pos[0].y) == 0x118)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01753 - **three kinds, and one of them must be present at all.** Kind 0xb
 * must be at y 0xf8, kind 0xc at x 0x1a9, and kind 0x2b must have bit 4 of its
 * +0x0a set. The kind-0x2b arm also sets a second flag, and the goal needs
 * both: a level with no kind-0x2b part can never meet it.
 */
void goal_test_puzzle_39(void)
{
    register struct part *si;
    int16_t ok;
    int16_t seen;

    seen = 0;
    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BIRD_CAGE) {
            if (((uint16_t)si->pos[0].y) != 0xf8)
                ok = 0;
        } else if (si->kind == KIND_POKEY) {
            if (((uint16_t)si->pos[0].x) != 0x1a9)
                ok = 0;
        } else if (si->kind == KIND_CANNON_BALL) {
            seen = 1;
            if ((si->flags_0a & PART_IN_BUCKET) == 0)
                ok = 0;
        }
        si = si->next;
    }

    if (ok && seen)
        g_round_state = 0x200;
}

/*
 * 0x017ad - every kind-0x24 part must be at -0x30 or above in y, which is off
 * the top of the play area. Five puzzles use it, more than any other.
 */
void goal_test_puzzles_53_54_63_67_87(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_ROCKET
            && (int16_t)((uint16_t)si->pos[0].y) > -0x30)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x017db - `goal_test_pop_balloons` with a line drawn across it: a balloon fails only
 * if it is **below 0x12c** as well as still present and unpopped. One above
 * that line is out of play and does not count.
 */
void goal_test_puzzle_25(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BALLOON
            && (int16_t)si->start_x > 0x12c
            && (si->flags_06 & PART_FROM_LEVEL) != 0
            && (si->flags_08 & PART_GONE) == 0)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01819 - a kind-0xc part at 0x1ba or beyond in x and exactly 0x11f in y.
 */
void goal_test_puzzle_41(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_POKEY
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x1ba
            && ((uint16_t)si->pos[0].y) == 0x11f)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01846 - **two lists.** Every kind-0x1b part on the list at 0x521b must
 * have reached 6 in its +0x0c, and there must be no kind-0x1b part at all on
 * the list at 0x50d7. Whatever those two lists are, one holds the ones that
 * count and the other the ones that disqualify.
 */
void goal_test_puzzles_10_32(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_GUN
            && (int16_t)si->form < 6)
            ok = 0;
        si = si->next;
    }

    si = g_held_parts.parts_bin.next;
    while (si != NULL) {
        if (si->kind == KIND_GUN)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01888 - **the first of these to walk with the picker rather than the
 * link.** `pick_by_flag((PART_IN_PLACED_LIST | PART_IN_MOVING_LIST))` gives the first record and `pick_for_record`
 * the next, which is a different set from the plain `+0` chain the others
 * follow. Kind 0xf must be under 0xb in its +0x0c and kind 0x2b at 0x170 or
 * beyond in y.
 */
void goal_test_puzzle_46(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = pick_by_flag((PART_IN_PLACED_LIST | PART_IN_MOVING_LIST));

    while (si != NULL) {
        if (si->kind == KIND_BOB_THE_FISH
            && (int16_t)si->form >= 0x0b)
            ok = 0;
        if (si->kind == KIND_CANNON_BALL
            && (int16_t)((uint16_t)si->pos[0].y) < 0x170)
            ok = 0;
        si = pick_for_record(si, PART_IN_MOVING_LIST);
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x018d9 - every kind-0xd part on the 0x521b list must be at 0x12 in its
 * +0x0c.
 */
void goal_test_puzzle_34(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_JACK_IN_THE_BOX
            && si->form != 0x12)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01907 - every kind-0x12 part on the 0x521b list must be at 0xb in +0x0c.
 */
void goal_test_puzzles_14_15_64_73(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_CANNON
            && si->form != 0x0b)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01935 - a kind-4 part must be at 0 in +0x0c and **back where it started**,
 * +0x1e and +0x20 equal to +0x26 and +0x28.
 *
 * The frame count at 0x4ea7 is tested **inside the walk and outside the kind
 * test**, so it is asked once per object and the answer is the same every
 * time: fewer than 0x14 frames since the machine started and the goal fails.
 * Transcribed where the original puts it rather than hoisted, because the
 * position is what says it is not part of the kind-4 case.
 */
void goal_test_puzzle_24(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BALLOON) {
            if (si->form != 0)
                ok = 0;
            if (si->pos[0].x != si->pos[2].x
                || si->pos[0].y != si->pos[2].y)
                ok = 0;
        }

        if ((int16_t)g_machine_frames < 0x14)
            ok = 0;

        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x0197e - every kind-0x23 part on the 0x521b list must be at 9 in +0x0c.
 */
void goal_test_puzzle_55(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BOXING_GLOVE
            && si->form != 9)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x019ac - a kind-0 part between 0x148 and 0x168 in x and at exactly 0xe8.
 */
void goal_test_puzzle_38(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BOWLING_BALL
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x148
            && (int16_t)((uint16_t)si->pos[0].x) <= 0x168
            && ((uint16_t)si->pos[0].y) == 0xe8)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x019e0 - a kind-0x2c part at 0x118 or beyond in x and 0x5b or beyond in y.
 */
void goal_test_puzzle_44(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_TENNIS_BALL
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x118
            && (int16_t)((uint16_t)si->pos[0].y) >= 0x5b)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01a0c - every kind-0x1c part must be between 0x168 and 0x17a in x and at
 * 0xc1 or beyond in y.
 */
void goal_test_puzzle_71(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASEBALL
            && ((int16_t)((uint16_t)si->pos[0].x) < 0x168
                || (int16_t)((uint16_t)si->pos[0].x) > 0x17a
                || (int16_t)((uint16_t)si->pos[0].y) < 0xc1))
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01a49 - no kind-0x2d part may still be at 0 in its +0x0c.
 */
void goal_test_puzzles_16_56_83(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_CANDLE
            && si->form == 0)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01a77 - a kind-9 part inside a box: 0x1a8 to 0x1b9 in x, 0x68 to 0x79 in y.
 */
void goal_test_puzzle_80(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASKETBALL
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x1a8
            && (int16_t)((uint16_t)si->pos[0].x) <= 0x1b9
            && (int16_t)((uint16_t)si->pos[0].y) >= 0x68
            && (int16_t)((uint16_t)si->pos[0].y) <= 0x79)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01ab0 - **one at each end.** Among the kind-0x11 parts sitting at y 0x118,
 * one must be at 0x74 or left of it and another at 0x198 or right of it. Two
 * separate flags, and the goal needs both, so a single part cannot satisfy it.
 */
void goal_test_puzzle_47(void)
{
    register struct part *si;
    int16_t left;
    int16_t right;

    left = right = 0;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BUCKET
            && ((uint16_t)si->pos[0].y) == 0x118) {
            if ((int16_t)((uint16_t)si->pos[0].x) <= 0x74)
                left = 1;
            if ((int16_t)((uint16_t)si->pos[0].x) >= 0x198)
                right = 1;
        }
        si = si->next;
    }

    if (left && right)
        g_round_state = 0x200;
}

/*
 * 0x01af7 - no kind-0xf part on the 0x521b list may have reached 0xb in +0x0c,
 * **and** 0x4e87 must have reached 0x134.
 *
 * The second test sits inside the walk and outside the kind test, so it is
 * asked once per object with the same answer every time, the way
 * `goal_test_puzzle_24` asks about the frame count. Transcribed where the original
 * puts it.
 */
void goal_test_puzzle_70(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BOB_THE_FISH
            && (int16_t)si->form >= 0x0b)
            ok = 0;

        if ((int16_t)((uint16_t)g_loop_frames) < 0x134)
            ok = 0;

        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01b2f - every kind-0x1d part on the 0x521b list must be at something other
 * than 0 or 2 in +0x0c. Two failing values rather than one passing one, which
 * is the shape `goal_test_puzzles_19_48` below has as well.
 */
void goal_test_puzzle_69(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_LIGHT
            && (si->form == 0
                || si->form == 2))
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01b63 - a kind-0xb part at exactly 0x108 in y.
 */
void goal_test_puzzle_52(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BIRD_CAGE
            && ((uint16_t)si->pos[0].y) == 0x108)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01b89 - **held for twelve frames, or held and then let go.**
 *
 * Every kind-0x1f part on the 0x521b list must be between 1 and 4 in its
 * +0x0c - zero fails and so does 5 or more. While that holds, the counter at
 * 0x5458 goes up, and passing 0xc wins.
 *
 * The second test is the odd one: if the condition has *stopped* holding but
 * the counter is not zero, that wins too. And **nothing here ever clears
 * 0x5458**, unlike `goal_test_puzzles_6_58` which zeroes it on a failed frame. So once
 * the condition has held for a single frame this goal is met the moment it
 * stops holding. Written as it is; the asymmetry with 0x1d1d is the original's.
 */
void goal_test_puzzles_19_48(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_MONKEY
            && (si->form == 0
                || (int16_t)si->form >= 5))
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_goal_condition[0]++;

    if ((int16_t)g_goal_condition[0] > 0x0c)
        g_round_state = 0x200;

    if (!ok && g_goal_condition[0] != 0)
        g_round_state = 0x200;
}

/*
 * 0x01bd9 - a kind-9 part between 8 and 0x28 in x and at exactly 0x28 in y,
 * which is the top left corner.
 */
void goal_test_puzzle_82(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASKETBALL
            && (int16_t)((uint16_t)si->pos[0].x) >= 8
            && (int16_t)((uint16_t)si->pos[0].x) <= 0x28
            && ((uint16_t)si->pos[0].y) == 0x28)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01c0a - **two ways to win, and the second is an overlap.**
 *
 * The walk keeps the last kind-0x2b part and the last kind-0x11 part. If no
 * kind-0x2b part was found at all, nothing happens. If every kind-0x2b part
 * had bit 4 of its +0x0a set, that alone wins.
 *
 * Otherwise the two parts have to overlap, and the test is written out as four
 * comparisons rather than as a rectangle: the kind-0x2b part's centre - its
 * +0x22 plus half its width - must be within 5 of the kind-0x11 part's left
 * and right edges, and the bottom of the kind-0x2b part must be more than 0x16
 * below the other's top and above its bottom.
 *
 * The kind-0x11 part is held in CX and **is not initialised**; a level with
 * none reaches the overlap test with whatever CX held. Zero here, as in
 * `goal_test_puzzle_79`.
 */
void goal_test_puzzle_81(void)
{
    int16_t l, r, t, b, mid, bot;       /* [bp-2] .. [bp-0xc] */
    int16_t flagged;                    /* [bp-0xe] */
    register struct part *si;
    register struct part *hit;
    struct part *other;

#ifndef __TURBOC__
    other = NULL;  /* ours: the original leaves it unset */
#endif
    hit = NULL;
    flagged = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_CANNON_BALL) {
            hit = si;
            if ((si->flags_0a & PART_IN_BUCKET) == 0)
                flagged = 0;
        }
        if (si->kind == KIND_BUCKET)
            other = si;
        si = si->next;
    }

    if (hit != NULL) {
        if (flagged != 0)
            g_round_state = 0x200;
        else {
            l = other->pos[1].x;
            r = l + other->size[0].width;
            t = other->pos[1].y;
            b = t + other->size[0].height;
            mid = hit->pos[1].x + (hit->size[0].width >> 1);
            bot = hit->pos[1].y + hit->size[0].height;
            if (mid + 5 > l && mid - 5 < r && t + 0x16 < bot && bot < b)
                g_round_state = 0x200;
        }
    }
}

/*
 * 0x01cc4 - **any** kind-0 part below 0x170. Written the moment one is seen,
 * and the walk continues rather than stopping.
 */
void goal_test_puzzle_4(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BOWLING_BALL
            && (int16_t)((uint16_t)si->pos[0].y) > 0x170)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01cea - **any** kind-0x1c part with +0x1e in 0x18..0xf3 inclusive and
 * +0x20 exactly 0xf9.
 */
void goal_test_puzzle_5(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASEBALL
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x18
            && (int16_t)((uint16_t)si->pos[0].x) <= 0xf3
            && ((uint16_t)si->pos[0].y) == 0xf9)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01d1d - every kind-6 part in the **0x521b** list must have a non-zero
 * +0x12, and it must **stay** that way: 0x5458 counts consecutive frames that
 * pass and is reset by any frame that does not. The goal is five in a row -
 * the test is `> 4` - so a condition that flickers never wins.
 */
void goal_test_puzzles_6_58(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_MOUSE_CAGE
            && ((uint16_t)si->direction) == 0)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_goal_condition[0]++;
    else
        g_goal_condition[0] = 0;

    if ((int16_t)g_goal_condition[0] > 4)
        g_round_state = 0x200;
}

/*
 * 0x01d5e - every kind-0xf part in the 0x521b list must have +0xc at least
 * 0xb.
 */
void goal_test_puzzles_7_51_65(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BOB_THE_FISH
            && (int16_t)si->form < 0x0b)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01d8c - every kind-0x11 part must be at y 0xf8.
 */
void goal_test_puzzle_9(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BUCKET
            && ((uint16_t)si->pos[0].y) != 0xf8)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01dbb - **exactly three, and all of them on.** Kind-0x18 parts on the
 * 0x521b list are counted, and any one with a zero +0x12 clears the flag. The
 * goal needs the flag and a count of exactly three, so two lit and one missing
 * is a fail and so is a fourth.
 */
void goal_test_puzzle_11(void)
{
    register struct part *si;
    int16_t n;
    int16_t ok;

    n = 0;
    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_FAN) {
            n++;
            if (((uint16_t)si->direction) == 0)
                ok = 0;
        }
        si = si->next;
    }

    if (ok && n == 3)
        g_round_state = 0x200;
}

/*
 * 0x01df1 - a kind-0x2c part at 0x154 or beyond in x and exactly 0x139 in y.
 */
void goal_test_puzzle_12(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_TENNIS_BALL
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x154
            && ((uint16_t)si->pos[0].y) == 0x139)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01e1e - **twelve frames of it.** No kind-0xe part on the 0x521b list may
 * have +0x0c equal to +0x10; while none does, the counter at 0x5458 goes up,
 * and passing 0xc wins. Like `goal_test_puzzles_19_48` it never clears the counter,
 * unlike `goal_test_puzzles_6_58` which does.
 */
void goal_test_puzzle_13(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_placed_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_GEAR
            && si->form == ((uint16_t)si->form_prev2))
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_goal_condition[0]++;

    if ((int16_t)g_goal_condition[0] > 0x0c)
        g_round_state = 0x200;
}

/*
 * 0x01e59 - three kinds over the picker's walk: kind 0x16 must be at 2 in its
 * +0x0c, and kinds 0x29 and 0x13 must both have bit 13 of their +8 set. Bit
 * 0x2000 is the same "already dealt with" bit `goal_test_pop_balloons` reads on a
 * balloon.
 */
void goal_test_puzzle_17(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = pick_by_flag((PART_IN_PLACED_LIST | PART_IN_MOVING_LIST));

    while (si != NULL) {
        if (si->kind == KIND_DYNAMITE_PLUNGER
            && si->form != 2)
            ok = 0;
        if (si->kind == KIND_BLAST
            && (si->flags_08 & PART_GONE) == 0)
            ok = 0;
        if (si->kind == KIND_DYNAMITE
            && (si->flags_08 & PART_GONE) == 0)
            ok = 0;
        si = pick_for_record(si, PART_IN_MOVING_LIST);
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01eb9 - a kind-0x2a part at 0x199 or beyond in x and exactly 0x10d in y.
 */
void goal_test_puzzle_18(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_MORT_THE_MOUSE
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x199
            && ((uint16_t)si->pos[0].y) == 0x10d)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x01ee6 - every kind-4 part that is still there, bit 0x8000 in +6, must have
 * moved off 0 in +0x0c - **and** the frame count at 0x4ea7 must have
 * reached 0x82, which is asked once per object the way `goal_test_puzzle_24`
 * asks it.
 */
void goal_test_puzzle_76(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BALLOON
            && (si->flags_06 & PART_FROM_LEVEL) != 0
            && si->form != 0)
            ok = 0;

        if ((int16_t)g_machine_frames < 0x82)
            ok = 0;

        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01f25 - `goal_test_puzzle_17` without its kind-0x16 arm: both a kind-0x13 and a
 * kind-0x29 part must have bit 13 of +8 set.
 */
void goal_test_puzzles_42_75(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = pick_by_flag((PART_IN_PLACED_LIST | PART_IN_MOVING_LIST));

    while (si != NULL) {
        if (si->kind == KIND_DYNAMITE
            && (si->flags_08 & PART_GONE) == 0)
            ok = 0;
        if (si->kind == KIND_BLAST
            && (si->flags_08 & PART_GONE) == 0)
            ok = 0;
        si = pick_for_record(si, PART_IN_MOVING_LIST);
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01f77 - every kind-0x2a part must have reached 0x170 in y.
 */
void goal_test_puzzles_57_74(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_MORT_THE_MOUSE && (int16_t)((uint16_t)si->pos[0].y) < 0x170)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01fa6 - every kind-9 part must be between 0x156 and 0x1ba in x and at 0xda
 * or beyond in y.
 */
void goal_test_puzzle_31(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASKETBALL
            && ((int16_t)((uint16_t)si->pos[0].x) < 0x156
                || (int16_t)((uint16_t)si->pos[0].x) > 0x1ba
                || (int16_t)((uint16_t)si->pos[0].y) < 0xda))
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x01fe3 - a kind-0x1c part whose +0x8c reads exactly 0x219 and which has
 * reached 0x40 in y.
 */
void goal_test_puzzle_66(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASEBALL
            && si->start_x == 0x219
            && (int16_t)((uint16_t)si->pos[0].y) >= 0x40)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x02010 - every kind-0x1c part must have reached 0x170 in y.
 */
void goal_test_puzzle_29(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASEBALL
            && (int16_t)((uint16_t)si->pos[0].y) < 0x170)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x0203f - a kind-0xb part at exactly 0xf8 in y.
 */
void goal_test_puzzle_61(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BIRD_CAGE && ((uint16_t)si->pos[0].y) == 0xf8)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x02065 - **six shelves, one flag each.** A kind-0x1c part standing at each
 * of the six y values, and all six wanted at once. The original keeps three in
 * registers and three on its stack, which is why there are six separate
 * variables here and not an array.
 */
void goal_test_puzzle_28(void)
{
    /* One flag per row a baseball has to reach, in the order they are
       tested: 0x39, 0x99, 0xf9, 0x69, 0xc9 and 0x129. */
    int16_t row1, row2, row3;           /* dx, cx, di */
    int16_t row4, row5, row6;           /* [bp-2], [bp-4], [bp-6] */
    register struct part *si;

    row1 = row2 = row3 = row4 = row5 = row6 = 0;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASEBALL) {
            if (si->pos[0].y == 0x39)
                row1 = 1;
            if (si->pos[0].y == 0x99)
                row2 = 1;
            if (si->pos[0].y == 0xf9)
                row3 = 1;
            if (si->pos[0].y == 0x69)
                row4 = 1;
            if (si->pos[0].y == 0xc9)
                row5 = 1;
            if (si->pos[0].y == 0x129)
                row6 = 1;
        }
        si = si->next;
    }

    if (row1 && row2 && row3 && row4 && row5 && row6)
        g_round_state = 0x200;
}

/*
 * 0x020fa - the picker's walk with three ways to fail and one to be
 * disqualified: kind 0x1b under 6 in +0x0c, kind 0xf at 0xb or over, and kind
 * 0x14 without bit 13 of +8 all clear the flag, while a kind-0xc part that is
 * *not* at 0 in +0x0c **sets** 0x5458, and the goal needs that word zero.
 * Nothing here clears it either.
 */
void goal_test_puzzle_36(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = pick_by_flag((PART_IN_PLACED_LIST | PART_IN_MOVING_LIST));

    while (si != NULL) {
        if (si->kind == KIND_GUN
            && (int16_t)si->form < 6)
            ok = 0;
        if (si->kind == KIND_BOB_THE_FISH
            && (int16_t)si->form >= 0x0b)
            ok = 0;
        if (si->kind == KIND_POKEY
            && si->form != 0)
            g_goal_condition[0] = 1;
        if (si->kind == KIND_BULLET
            && (si->flags_08 & PART_GONE) == 0)
            ok = 0;
        si = pick_for_record(si, PART_IN_MOVING_LIST);
    }

    if (ok && g_goal_condition[0] == 0)
        g_round_state = 0x200;
}

/*
 * 0x02172 - a kind-0x2a part between 0x19b and 0x1cc in x and at exactly 0x12d.
 */
void goal_test_puzzle_86(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_MORT_THE_MOUSE
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x19b && (int16_t)((uint16_t)si->pos[0].x) <= 0x1cc
            && ((uint16_t)si->pos[0].y) == 0x12d)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x021a6 - **three kinds in one corner, and one of them must exist.**
 *
 * Kinds 9, 0 and 0x2b all have to be between 0x18 and 0x94 in x and at 0x100
 * or beyond in y. A kind-0x2b part also raises a second flag, and the goal
 * needs both - so a level with no kind-0x2b part can never meet it, the same
 * shape `goal_test_puzzle_39` has.
 */
void goal_test_puzzle_77(void)
{
    register struct part *si;
    int16_t ok;
    int16_t seen;

    seen = 0;
    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_CANNON_BALL)
            seen = 1;

        if (si->kind == KIND_BASKETBALL
            || si->kind == KIND_BOWLING_BALL
            || si->kind == KIND_CANNON_BALL) {
            if ((int16_t)((uint16_t)si->pos[0].x) < 0x18
                || (int16_t)((uint16_t)si->pos[0].x) > 0x94
                || (int16_t)((uint16_t)si->pos[0].y) < 0x100)
                ok = 0;
        }

        si = si->next;
    }

    if (ok && seen)
        g_round_state = 0x200;
}

/*
 * 0x021fd - a kind-0xb part between 0x1b6 and 0x1c0 in x and at exactly 0x108.
 */
void goal_test_puzzle_85(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BIRD_CAGE
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x1b6 && (int16_t)((uint16_t)si->pos[0].x) <= 0x1c0
            && ((uint16_t)si->pos[0].y) == 0x108)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x02231 - every kind-0xc part must have reached 0xc8 in y.
 */
void goal_test_puzzle_60(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_POKEY && (int16_t)((uint16_t)si->pos[0].y) < 0xc8)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x02260 - a kind-0x2a part between 0x20 and 0x78 in x and past 0x120 in y.
 */
void goal_test_puzzle_37(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_MORT_THE_MOUSE
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x20
            && (int16_t)((uint16_t)si->pos[0].x) <= 0x78
            && (int16_t)((uint16_t)si->pos[0].y) > 0x120)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x02292 - **exactly two, and both set.** Kind-0x2b parts are counted and any
 * one without bit 4 of its +0x0a fails; a kind-0x11 part short of 0x1388 in y
 * fails as well - 5000, which is far below the play area, so that is "has it
 * fallen off the bottom".
 *
 * The goal needs the flag and a count of exactly two, so one of them missing
 * is a fail and so is a third.
 */
void goal_test_puzzle_84(void)
{
    register struct part *si;
    int16_t ok;
    int16_t n;

    n = 0;
    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_CANNON_BALL) {
            n++;
            if ((si->flags_0a & PART_IN_BUCKET) == 0)
                ok = 0;
        }

        if (si->kind == KIND_BUCKET
            && (int16_t)((uint16_t)si->pos[0].y) < 0x1388)
            ok = 0;

        si = si->next;
    }

    if (ok && n == 2)
        g_round_state = 0x200;
}

/*
 * 0x022d8 - **one part to the right of another, but not far.**
 *
 * The walk keeps the last kind-0x2a part's x and the last kind-0xb part's x,
 * and any kind-0xb part not at 0x108 in y fails. Then the kind-0x2a must be to
 * the right of the kind-0xb and within 0x32 of it.
 *
 * Both are held in registers and **neither is initialised**; a level missing
 * either kind reaches the comparison with whatever was there. Zero here, as in
 * `goal_test_puzzle_79`.
 */
void goal_test_puzzle_68(void)
{
    register struct part *si;
    int16_t ok;
    int16_t a, b;

    ok = 1;
    si = g_moving_parts.next;
#ifndef __TURBOC__
    a = 0;      /* ours: the original leaves both unset */
    b = 0;
#endif

    while (si != NULL) {
        if (si->kind == KIND_MORT_THE_MOUSE)
            a = si->pos[0].x;

        if (si->kind == KIND_BIRD_CAGE) {
            b = si->pos[0].x;
            if (((uint16_t)si->pos[0].y) != 0x108)
                ok = 0;
        }

        si = si->next;
    }

    if (ok && a > b && (int16_t)(b + 0x32) > a)
        g_round_state = 0x200;
}

/*
 * 0x02322 - every kind-0x1c part must have bit 4 of its +0x0a set.
 */
void goal_test_puzzle_72(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASEBALL
            && (si->flags_0a & PART_IN_BUCKET) == 0)
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x02351 - a kind-0x11 part between 0xd0 and 0xee in x and at exactly 0x128 in
 * y, **and** a kind-0x2b part with bit 4 of its +0x0a set somewhere on the
 * list. The second is a flag rather than a test, so any one of them will do.
 */
void goal_test_puzzle_59(void)
{
    register struct part *si;
    int16_t ok;
    int16_t seen;

    seen = 0;
    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_CANNON_BALL
            && (si->flags_0a & PART_IN_BUCKET) != 0)
            seen = 1;

        if (si->kind == KIND_BUCKET) {
            if ((int16_t)((uint16_t)si->pos[0].x) < 0xd0
                || (int16_t)((uint16_t)si->pos[0].x) > 0xee
                || ((uint16_t)si->pos[0].y) != 0x128)
                ok = 0;
        }

        si = si->next;
    }

    if (ok && seen)
        g_round_state = 0x200;
}

/*
 * 0x023a4 - **two kinds, opposite corners.** A kind-0x1c part must be at 0x4a or
 * left of it and 0x124 or below; a kind-0x2c part at 0x1c6 or right of it
 * and 0x124 or below. Either failing fails the goal.
 */
void goal_test_puzzle_49(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASEBALL
            && ((int16_t)((uint16_t)si->pos[0].x) > 0x4a || (int16_t)((uint16_t)si->pos[0].y) < 0x124))
            ok = 0;

        if (si->kind == KIND_TENNIS_BALL
            && ((int16_t)((uint16_t)si->pos[0].x) < 0x1c6 || (int16_t)((uint16_t)si->pos[0].y) < 0x124))
            ok = 0;

        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x023ef - every kind-9 part must be between 0xf6 and 0x14c in x and at
 * exactly 0xe8 in y.
 */
void goal_test_puzzle_40(void)
{
    register struct part *si;
    int16_t ok;

    ok = 1;
    si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BASKETBALL
            && ((int16_t)((uint16_t)si->pos[0].x) < 0xf6
                || (int16_t)((uint16_t)si->pos[0].x) > 0x14c
                || ((uint16_t)si->pos[0].y) != 0xe8))
            ok = 0;
        si = si->next;
    }

    if (ok)
        g_round_state = 0x200;
}

/*
 * 0x0242c - a kind-0 part inside a box: 0x1d6 to 0x1fc in x, 0xc6 to 0xd0 in
 * y. The last of the goal tests with a body of its own; the entries for
 * puzzles 88 to 110 below it only reuse one or do nothing.
 */
void goal_test_puzzle_35(void)
{
    struct part *si = g_moving_parts.next;

    while (si != NULL) {
        if (si->kind == KIND_BOWLING_BALL
            && (int16_t)((uint16_t)si->pos[0].x) >= 0x1d6
            && (int16_t)((uint16_t)si->pos[0].x) <= 0x1fc
            && (int16_t)((uint16_t)si->pos[0].y) >= 0xc6
            && (int16_t)((uint16_t)si->pos[0].y) <= 0xd0)
            g_round_state = 0x200;
        si = si->next;
    }
}

/*
 * 0x02467 - puzzle 88's goal is `goal_test_puzzle_86`'s: `push cs` and a near call to
 * 0x02172, which is the far call that routine returns from.
 */
void goal_test_puzzle_88(void)
{
    goal_test_puzzle_86();
}

/*
 * 0x02470 - puzzle 89's goal is `goal_test_puzzle_55`'s: `push cs` and a near call to
 * 0x0197e, which is the far call that routine returns from.
 */
void goal_test_puzzle_89(void)
{
    goal_test_puzzle_55();
}

/*
 * 0x02479 - puzzle 90's goal is `goal_test_puzzles_53_54_63_67_87`'s: `push cs` and a near call to
 * 0x017ad, which is the far call that routine returns from.
 */
void goal_test_puzzle_90(void)
{
    goal_test_puzzles_53_54_63_67_87();
}

/*
 * 0x02482 - puzzle 91's goal is `goal_test_pop_balloons`'s: `push cs` and a near call to
 * 0x015fa, which is the far call that routine returns from.
 */
void goal_test_puzzle_91(void)
{
    goal_test_pop_balloons();
}

/*
 * 0x0248b - puzzle 92's goal is `goal_test_puzzle_29`'s: `push cs` and a near call to
 * 0x02010, which is the far call that routine returns from.
 */
void goal_test_puzzle_92(void)
{
    goal_test_puzzle_29();
}

/*
 * 0x02494 - puzzle 93's goal is `goal_test_puzzle_61`'s: `push cs` and a near call to
 * 0x0203f, which is the far call that routine returns from.
 */
void goal_test_puzzle_93(void)
{
    goal_test_puzzle_61();
}

/*
 * 0x0249d - puzzle 94's goal is `goal_test_puzzles_57_74`'s: `push cs` and a near call to
 * 0x01f77, which is the far call that routine returns from.
 */
void goal_test_puzzle_94(void)
{
    goal_test_puzzles_57_74();
}

/*
 * 0x024a6 - puzzle 95's goal is `goal_test_puzzle_13`'s: `push cs` and a near call to
 * 0x01e1e, which is the far call that routine returns from.
 */
void goal_test_puzzle_95(void)
{
    goal_test_puzzle_13();
}

/*
 * 0x024af - puzzle 96 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_96(void)
{
}

/*
 * 0x024b4 - puzzle 97 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_97(void)
{
}

/*
 * 0x024b9 - puzzle 98 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_98(void)
{
}

/*
 * 0x024be - puzzle 99 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_99(void)
{
}

/*
 * 0x024c3 - puzzle 100 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_100(void)
{
}

/*
 * 0x024c8 - puzzle 101 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_101(void)
{
}

/*
 * 0x024cd - puzzle 102 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_102(void)
{
}

/*
 * 0x024d2 - puzzle 103 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_103(void)
{
}

/*
 * 0x024d7 - puzzle 104 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_104(void)
{
}

/*
 * 0x024dc - puzzle 105 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_105(void)
{
}

/*
 * 0x024e1 - puzzle 106 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_106(void)
{
}

/*
 * 0x024e6 - puzzle 107 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_107(void)
{
}

/*
 * 0x024eb - puzzle 108 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_108(void)
{
}

/*
 * 0x024f0 - puzzle 109 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_109(void)
{
}

/*
 * 0x024f5 - puzzle 110 has no goal test: the routine sets up a frame and
 * returns, so the state is never set to 0x200 from here.
 */
void goal_test_puzzle_110(void)
{
}

/*
 * 0x024fa
 *
 * Start the counters rolling. The scroll positions go to -4 and 0, and the
 * band is repainted.
 *
 * The -4 is a delay, not a position: `step_counters` adds to it before it
 * looks, so the first counter spends four steps at or below zero - drawing
 * nothing, because the draw is skipped while the scroll is not positive -
 * before its first digit moves. The second starts at 0 and moves at once.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void start_counters(void)
{
    g_bonus_1_scroll = -4;
    g_bonus_2_scroll = 0;
    redraw_counters();
}

/*
 * 0x02510
 *
 * One step of the two rolling counters.
 *
 * Each is a value and a scroll: the scroll climbs to 0x15, a whole digit cell,
 * and then wraps to 0 and takes one off the value. So a counter never jumps -
 * it slides from one number to the next, and `draw_counter_word` is handed the
 * scroll as its `y`.
 *
 * **The first counter rolls faster the more it has left**: over 0xfa0 the
 * scroll gains 4 a step, over 0xbb8 three, over 0x708 two, otherwise one. So a
 * large tally is not slow, and the last hundred or so still count one at a
 * time. The second counter always gains one.
 *
 * State 0x2000 is the one that changes the rule, and it changes it in
 * *opposite* directions for the two counters: 0x251f is `jne` and 0x2592 is
 * `je`. The long counter steps while the machine is not running - which is the
 * only state this routine is ever called in - and in each case a scroll
 * already off zero is allowed to finish, which is how a roll half-way through
 * a digit is never left standing between two of them. The second counter's
 * arm is dead, and the note in the block below says why.
 *
 * The value is written back whether or not it was decremented, and the draw is
 * skipped when the scroll is at or below zero: at zero there is nothing to
 * slide, and the negative is the four-step delay `start_counters` set.
 *
 * **This was wrong until 2026-09-18**, and the note that stood here said why
 * it survived: the counters belong to the game proper, the intro never reaches
 * them, and nothing had run them against the original. Both blocks tested
 * `state != 0x2000`, so the port rolled the second counter in the editor where
 * the original leaves it standing - 0293 against 0300 on the level screen, the
 * only pixels of it that ever differed.
 */
void step_counters(void)
{
    register int16_t si;

    set_clip_counter_strip();

    if (g_round_state != 0x2000 || g_bonus_1_scroll != 0) {
        if ((si = g_level_settings.bonus_1) != 0) {
            if (si > 0xfa0)
                g_bonus_1_scroll += 4;
            else if (si > 0xbb8)
                g_bonus_1_scroll += 3;
            else if (si > 0x708)
                g_bonus_1_scroll += 2;
            else
                g_bonus_1_scroll++;

            if (g_bonus_1_scroll > 0x15) {
                g_bonus_1_scroll = 0;
                si--;
            }
            g_level_settings.bonus_1 = si;

            if (g_bonus_1_scroll > 0)
                draw_counter_word(g_level_settings.bonus_1, 0x184, g_bonus_1_scroll, 0);
        }
    }

    /* **And the second counter's state test is the other way round** - `je`
       at 0x2592 where the first block has `jne` at 0x251f - which makes this
       block unreachable in the shipped game. Its only caller is the editor
       loop at 0x0f8f5, whose condition at 0x0fa91 is `state != 0x2000 &&
       state != 2`, so the state is never 0x2000 here; and the other way in,
       a scroll already off zero, needs someone to arm it. The only non-zero
       write to 0x4eb1 is the -9 at 0x2750, in the teardown that zeroes both
       counters first, so the `bonus_2 != 0` test below stops it there.
       The second reel therefore shows the level's bonus and never rolls.
       Transcribed as it behaves - see STATUS.md. */
    if (g_round_state == 0x2000 || g_bonus_2_scroll != 0) {
        if (g_level_settings.bonus_2 != 0) {
            g_bonus_2_scroll++;
            if (g_bonus_2_scroll > 0x15) {
                g_bonus_2_scroll = 0;
                g_level_settings.bonus_2--;
            }
            if (g_bonus_2_scroll > 0)
                draw_counter_word(g_level_settings.bonus_2, 0x238, g_bonus_2_scroll, 0);
        }
    }
}

/*
 * 0x025d8
 *
 * Repaint all three counters in full, with no scroll: the long one at 0xd0 and
 * the two short ones at 0x184 and 0x238. Every caller of this wants the whole
 * band back, so `all` is 1 on all three.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void redraw_counters(void)
{
    set_clip_counter_strip();
    draw_counter_long(g_odometer_total, 0xd0, 0, 1);
    draw_counter_word(g_level_settings.bonus_1, 0x184, 0, 1);
    draw_counter_word(g_level_settings.bonus_2, 0x238, 0, 1);
}

/*
 * 0x0262b
 *
 * Draw a **four-digit counter** at `x`, scrolled by `y`, right digit first.
 *
 * The four digits come out of a five-digit string, and the fifth is why
 * `0x2710` is added first: 10000 forces `itoa` to fill all five places, so
 * `buf[1..4]` are the digits wanted with their leading zeros already there and
 * no zero-padding loop needed. `buf[0]` is the 1 that was added, and is never
 * drawn.
 *
 * Then a sentinel: `buf[5]`, the terminator, is overwritten with `'0'`. That
 * one byte is the whole of the leading-zero test below - it makes the digit to
 * the right of the units read as a zero, so the units are always drawn.
 *
 * `all` decides how much of the number gets redrawn:
 *
 *   set    every digit, which is what a full repaint wants
 *   clear  only while `buf[si]` is `'0'` - the units, then the tens if the
 *          units are zero, then the hundreds if the tens are too, and stop
 *
 * That reads like a mistake and is exactly right. The counter is stepped by
 * one, so the digits that move are the trailing zeros and the one above them:
 * 1300 going to 1299 changes three digits, 1301 going to 1300 changes one. The
 * test asks "did the digit to my right just wrap", and stops at the first that
 * did not.
 *
 * Stopping is done by setting the index to 0 and letting the shared `dec` take
 * it to -1, so the loop's own test ends it - and the x step happens on that
 * pass too, harmlessly, because nothing reads x again.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void draw_counter_word(register int16_t value, int16_t x, int16_t y,
                       int16_t all)
{
    char buf[8];
    register int16_t si;

    value += 0x2710;
    itoa(value, buf, 10);
    buf[5] = '0';

    for (si = 5; si > 1; si--, x -= 0x20) {
        if (all != 0 || buf[si] == '0')
            draw_odometer_digit(buf[si - 1], x, y);
        else
            si = 0;
    }
}

/*
 * 0x02686
 *
 * Draw a **six-digit counter** at `x`, scrolled by `y`. The 32-bit sibling of
 * `draw_counter_word`, and the same trick twice over: 0xf4240 is 1,000,000, so
 * `ltoa` fills seven places, `buf[7]` gets the `'0'` sentinel, and `buf[1..6]`
 * are drawn from the right.
 *
 * The only real difference is that x lives in a register here and in a stack
 * slot there, which is the compiler's choice and not the program's.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void draw_counter_long(int32_t value, register int16_t x, int16_t y,
                       int16_t all)
{
    char buf[16];
    register int16_t si;

    value += 0xf4240L;
    ltoa(value, buf, 10);
    buf[7] = '0';

    for (si = 7; si > 1; si--, x -= 0x20) {
        if (all != 0 || buf[si] == '0')
            draw_odometer_digit(buf[si - 1], x, y);
        else
            si = 0;
    }
}

/*
 * 0x026e8
 *
 * Clip to the **counter strip** and draw into the visible page.
 *
 * Full width, rows 0x1b to 0x45 - the band the three counters sit in - and
 * `g_vmds.page_dst` set to 0xa000 rather than to whichever page is being built.
 * The counters are drawn straight onto the screen, outside the double
 * buffering, which is what lets them roll while the machine below them is
 * still being composed.
 *
 * The strip is 0x2b rows and a digit cell is 0x15, so two cells and a line:
 * enough for a digit and the one arriving behind it.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void set_clip_counter_strip(void)
{
    g_vmds.page_dst = 0xa000;
    g_vmds.clip_enabled = 1;
    g_vmds.clip_left    = 0;
    g_vmds.clip_right   = 0x27f;
    g_vmds.clip_top     = 0x1b;
    g_vmds.clip_bottom  = 0x45;
}

/*
 * 0x02710
 *
 * **A puzzle has been solved.** Bank the bonus, show the panel, then offer
 * REPLAY or ADVANCE and do not return until one of them is taken.
 *
 * The two bonus counters at 0x50af and 0x50b1 are added into the 32-bit score
 * and zeroed, and both counter scroll positions are set negative - -4 and -9,
 * where `start_counters` uses -4 and 0. `step_counters` adds before it looks
 * and skips the draw while the position is not positive, so the band stays
 * blank for four steps and nine rather than four and none: the counters that
 * were just emptied do not flicker back up.
 *
 * The score and the level are also copied to 0x4eab/0x4ea9 and 0x4eb5, but
 * **only when this was not the last puzzle**. That pair is what a later
 * password resumes from, and there is nothing to resume past the end.
 *
 * Then a wait: `update_button_state` until the button reads 2, presenting a
 * frame each time round. This is the "(click button to continue)" the panel
 * asks for, and it is spelt out here rather than in the panel.
 *
 * The dialog's wording is chosen **once**, before the loop - the replay branch
 * jumps back in below it, so a second time round asks the same question. On
 * the last puzzle it is "SOLVED ALL PUZZLES" and advancing goes to freeform;
 * otherwise "REPLAY SOLUTION". REPLAY answers non-zero and runs the machine
 * again from 0x4e6b = 0x2000, which is the state `step_counters` reads to
 * leave the counters alone.
 *
 * At the end, finishing the last puzzle sets 0x4e67 and steps the level back
 * one, so the caller's increment lands on the same puzzle rather than past it.
 *
 * The near call at 0x02763 is `redraw_counters`; the listing annotates it as
 * "CONTINUE" because a string starts at that DGROUP offset.
 */
void finish_level(void)
{
    char *body;                         /* [bp-2] */
    register int16_t clicked;
    register const char *title;

#ifndef __TURBOC__
    dev_level_solved(g_round_number,
                     (int16_t)((uint32_t)g_banked_score >> 16));    /* ours */
#endif

    g_odometer_total += g_level_settings.bonus_1 + g_level_settings.bonus_2;
    if (g_round_number < g_level_count) {
        g_banked_score = g_odometer_total;
        g_password_puzzle = g_round_number;
    }

    show_level_complete();

    g_bonus_1_scroll = -4;
    g_bonus_2_scroll = -9;
    g_level_settings.bonus_1 = 0;
    g_level_settings.bonus_2 = 0;

    redraw_counters();
    play_sound(0x13);
    show_cursor_again();

    clicked = 0;
    while (clicked == 0) {
        update_button_state();
        if (g_pointer.button_left == 2)
            clicked = 1;
        present_frame(1);
    }

    if (g_round_number >= g_level_count) {
        title = g_messages.solved_all_puzzles;
        body = (char *)g_messages.solved_all_body;
    } else {
        title = g_messages.replay_solution;
        body = (char *)g_messages.replay_body;
    }

    /* The two buttons are this module's literal pool, DGROUP 0x283a. */
    while (message_box(title, body, "REPLAY", "ADVANCE")) {
        g_round_state = 0x2000;
        clear_layer_heads();
        reset_machine();
        redraw_machine_area();
        run_machine_loop();
        g_round_state = 0x200;
        repaint_whole_screen();
    }

    if (g_round_number >= g_level_count) {
        g_freeform = 1;
        g_round_number--;
    }
}
