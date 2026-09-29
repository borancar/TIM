/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game screen**: its loop, the part being carried and dragged, the bin,
 * the screen states - quit, restart, freeform, load, save, gravity, air -
 * and the panels around the play area.
 *
 * The fifth module of the original's **code segment 0dff**, image
 * 0x0f8c2..0x11d00. Its data is DGROUP 0x2630..0x283a: the belt anchor, the
 * bin's two repeat counters and the table of goal tests, the play screen's tab
 * stops, the master levels' marker positions, and its literal pool. Where it
 * begins is a choice - see puzzles.c. Functions are in address order and each
 * carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x2630..0x283a
 *
 * Built with Borland C++ and without `-d`: the pool keeps both copies of
 * "*.TIM". Thirteen of the port's routines here were once not the
 * original's functions - the ten cases of `game_screen`'s switch, the two
 * resize arms of `part_key_shortcut`, and `region_cursor_bin_above` begun
 * sixteen bytes late - and are back in the routines they belong to.
 */
#include <string.h>
#ifdef __TURBOC__
#include <stdlib.h>
#else
#include "hostlib.h"
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * The part in the player's hand. The original reads the word at 0x50d5 again
 * at every use rather than keeping it in a register, and a call in between
 * can change it, so this is a macro and not a local.
 */
#define CARRIED HELD_PARTS.dragged_part

/*
 * **The game screen's own words and the goal tests**, DGROUP 0x2630..0x27ee -
 * the first of this module's data.
 */
struct goal_tests GOAL_TESTS = {
    0, 0, 0,
    {
        goal_test_puzzle_1,
        goal_test_puzzle_2,
        goal_test_pop_balloons,
        goal_test_puzzle_4,
        goal_test_puzzle_5,
        goal_test_puzzles_6_58,
        goal_test_puzzles_7_51_65,
        goal_test_pop_balloons,
        goal_test_puzzle_9,
        goal_test_puzzles_10_32,
        goal_test_puzzle_11,
        goal_test_puzzle_12,
        goal_test_puzzle_13,
        goal_test_puzzles_14_15_64_73,
        goal_test_puzzles_14_15_64_73,
        goal_test_puzzles_16_56_83,
        goal_test_puzzle_17,
        goal_test_puzzle_18,
        goal_test_puzzles_19_48,
        goal_test_puzzle_20,
        goal_test_puzzle_21,
        goal_test_puzzle_22,
        goal_test_puzzle_23,
        goal_test_puzzle_24,
        goal_test_puzzle_25,
        goal_test_puzzle_26,
        goal_test_pop_balloons,
        goal_test_puzzle_28,
        goal_test_puzzle_29,
        goal_test_pop_balloons,
        goal_test_puzzle_31,
        goal_test_puzzles_10_32,
        goal_test_pop_balloons,
        goal_test_puzzle_34,
        goal_test_puzzle_35,
        goal_test_puzzle_36,
        goal_test_puzzle_37,
        goal_test_puzzle_38,
        goal_test_puzzle_39,
        goal_test_puzzle_40,
        goal_test_puzzle_41,
        goal_test_puzzles_42_75,
        goal_test_puzzle_43,
        goal_test_puzzle_44,
        goal_test_pop_balloons,
        goal_test_puzzle_46,
        goal_test_puzzle_47,
        goal_test_puzzles_19_48,
        goal_test_puzzle_49,
        goal_test_pop_balloons,
        goal_test_puzzles_7_51_65,
        goal_test_puzzle_52,
        goal_test_puzzles_53_54_63_67_87,
        goal_test_puzzles_53_54_63_67_87,
        goal_test_puzzle_55,
        goal_test_puzzles_16_56_83,
        goal_test_puzzles_57_74,
        goal_test_puzzles_6_58,
        goal_test_puzzle_59,
        goal_test_puzzle_60,
        goal_test_puzzle_61,
        goal_test_pop_balloons,
        goal_test_puzzles_53_54_63_67_87,
        goal_test_puzzles_14_15_64_73,
        goal_test_puzzles_7_51_65,
        goal_test_puzzle_66,
        goal_test_puzzles_53_54_63_67_87,
        goal_test_puzzle_68,
        goal_test_puzzle_69,
        goal_test_puzzle_70,
        goal_test_puzzle_71,
        goal_test_puzzle_72,
        goal_test_puzzles_14_15_64_73,
        goal_test_puzzles_57_74,
        goal_test_puzzles_42_75,
        goal_test_puzzle_76,
        goal_test_puzzle_77,
        goal_test_puzzle_78,
        goal_test_puzzle_79,
        goal_test_puzzle_80,
        goal_test_puzzle_81,
        goal_test_puzzle_82,
        goal_test_puzzles_16_56_83,
        goal_test_puzzle_84,
        goal_test_puzzle_85,
        goal_test_puzzle_86,
        goal_test_puzzles_53_54_63_67_87,
        goal_test_puzzle_88,
        goal_test_puzzle_89,
        goal_test_puzzle_90,
        goal_test_puzzle_91,
        goal_test_puzzle_92,
        goal_test_puzzle_93,
        goal_test_puzzle_94,
        goal_test_puzzle_95,
        goal_test_puzzle_96,
        goal_test_puzzle_97,
        goal_test_puzzle_98,
        goal_test_puzzle_99,
        goal_test_puzzle_100,
        goal_test_puzzle_101,
        goal_test_puzzle_102,
        goal_test_puzzle_103,
        goal_test_puzzle_104,
        goal_test_puzzle_105,
        goal_test_puzzle_106,
        goal_test_puzzle_107,
        goal_test_puzzle_108,
        goal_test_puzzle_109,
        goal_test_puzzle_110,
    },
};

/*
 * **Where Tab sends the pointer on the play screen's controls**, DGROUP 0x27ee..0x2818, 0x2a bytes: which
 * stop it is on - 0xffff until the first Tab, and back to 0 past the last
 * - and the x of each, the y being fixed.
 */
struct game_play_tabs {
    uint16_t  stop;          /* +0x00 [2]  which of the eleven tab stops on the play screen */
    int16_t   stop_x[9];          /* +0x02 [0x12]  stops 9 and 10 take x from the two knobs instead */
    int16_t   stop_y[11];         /* +0x14 [0x16]  and its eleventh word, at 0x2816, is also the
                                            first of the level table below, which nothing
                                            reads as that */
} PACKED;

struct game_play_tabs GAME_PLAY_TABS = {
    0xffff, /* stop */
    {
        0x0042, 0x0064, 0x0064, 0x00c4, 0x00e6, 0x0051, 0x0079, 0x009e,
        0x00ce,
    },
    /* stop_x */
    {
        0x006b, 0x0065, 0x0073, 0x0074, 0x006e, 0x009e, 0x0097, 0x0098,
        0x0098, 0x00ec, 0x0138,
    },
    /* stop_y */
};

/*
 * **Where each master level's marker is drawn**, DGROUP 0x2818..0x2824, 0x0c bytes: an x per
 * level from 1 to 6, which `paint_panel_e` reads as `0x2816 + 2 * level`.
 * The word before it, at 0x2816, is the last of the tab stops above.
 */
struct game_master_level_x {
    int16_t   level_x[6];         /* +0x00 [0xc]  level 1 first */
} PACKED;

struct game_master_level_x GAME_MASTER_LEVEL_X = {
    { 0x0085, 0x0088, 0x008e, 0x0094, 0x009b, 0x00a3 }, /* level_x */
};


/*
 * The helper below is the host's alone. Under the original compiler a
 * function here would be code in the module and move every routine after
 * it; the routines that call it say what the original does instead.
 */
#ifndef __TURBOC__
/* The parts bin's initial repeat delay, in loop iterations. Ours - see below. */
#define BIN_REPEAT_DELAY 9

/*
 * OURS: not a transcription, but a **deliberate deviation** chosen by the
 * project owner on 2026-09-06 - the only one in this file.
 *
 * The original has no initial repeat delay. `bin_scroll_back` and
 * `bin_scroll_forward` fire on call 0, 3, 6 ... of `game_screen_loop` with the
 * counter reset only on release, so a press begins repeating at once - read
 * instruction by instruction against 0x10cc8 and 0x10d37, which match.
 *
 * That is fine on the machine it was written for and not on this one. The
 * frame wait at `game_screen_loop` is a **minimum** - eight ticks of a 236.7 Hz
 * timer - and a 386 spent longer than that on the frame itself, so its loop ran
 * slower than the 29.6 iterations a second the port achieves. At the port's
 * rate an ordinary click of about 150 ms spans four iterations, which is enough
 * for the counter to come round to 3 and scroll a second page. Measured: one
 * click gave one page about 20% of the time.
 *
 * So the press still fires immediately, then nothing until the delay, then the
 * original's one-in-three.
 *
 * **The delay is ours, and it is twelve loop iterations - about 400 ms.**
 *
 * An earlier version of this took the 12 from DGROUP 0x2d40, the original's
 * own constant, and said so as if that gave it provenance. It does not.
 * `button_state` reloads its per-button countdown from that word and decrements
 * it once per call, and it is called from `timer_callback` - so its unit is a
 * **timer tick at 236.7 Hz**, where 12 is about 51 ms. Using the same number as
 * a count of *loop iterations* at 29.6 Hz stretches it eightfold. The two
 * quantities are not the same quantity, and borrowing the digits was dressing a
 * chosen number as a measured one.
 *
 * In its own units it would not work either: 51 ms expires part-way through an
 * ordinary 150 ms click, which is 4.4 iterations here, so the counter would
 * still come round and scroll again.
 *
 * Twelve iterations is therefore chosen, on the only grounds that hold - it
 * sits past a click and short of a deliberate hold - and is written here as a
 * constant of ours rather than read from a word that means something else.
 *
 * The test is `n % 3` and not `(n - BIN_REPEAT_DELAY) % 3`. The subtraction
 * was there to make the first repeat land exactly at the end of the delay
 * whatever the delay was, and with a delay that is a multiple of three - which
 * this one is, and which the constant above asks it to stay - the two are the
 * same expression. Arithmetic that can never change an answer is worse than
 * none: it reads as though it matters.
 */
static int32_t bin_repeat_due(int16_t n)
{
    if (n == 0)
        return 1;
    if (n < BIN_REPEAT_DELAY)
        return 0;
    return (n % 3) == 0;
}
#endif

/*
 * 0x0f8c2
 *
 * **The game screen's own loop** - where the game sits while a level is being
 * built and run, and the last piece between the briefing and playing.
 *
 * It is a loop on the same DGROUP 0x4e6b that `game_round` dispatches on, so a
 * screen leaves by *writing into that word* rather than by returning: it runs
 * while 0x4e6b is neither 0x2000 nor 2.
 *
 * One pass, in order: clear the two cursor hints to -1, take a key, update the
 * button, scroll the play area, step the counters, offer the key to the music
 * shortcut if this is freeform, let the regions see the pointer, run the bin's
 * arrows if a region asked for them, and then either the pointer frame - if
 * the pointer is in the play area - or the edge-scroll flags if it is not.
 *
 * `si` is the "was outside last frame" latch. It exists so that leaving the
 * play area with a part in hand re-marks that part exactly **once**, on the
 * frame the pointer crosses out, rather than every frame it stays out.
 *
 * **Five deferred redraws** follow, at 0x4e93 down to 0x4e8b, each a countdown
 * and a layer: a change asks for N frames of redraw and gets one a frame. Then
 * the machine is stepped and drawn, the selection decoration goes on if
 * something is selected, and the rubber-band line goes on if a mover asked for
 * one - 0x52c5 is its colour and -1 means no line.
 *
 * **The frame pacing is a spin on 0x44ef**, which the timer counts down: the
 * loop waits until eight have gone by, then reloads 0x2710 and presents. That
 * is the same counter the copy-protection screen reads its page number from,
 * and it is why the two clocks differ there.
 *
 * On the way out, a part still in hand with bit 0x800 in +6 is thrown away if
 * it is a rope or a belt that reached something, and otherwise handed to
 * finish_part_removal. A rope that never reached anything takes the second path, because
 * the kind test falls through to the belt test and then out.
 */
void game_screen_loop(void)
{
    int16_t si;

    reset_level_state();
    si = 0;
    TIMER.frame_budget = 0x2710;

    while (round_state != 0x2000 && round_state != 2) {
        drop_cursor = band_colour = 0xffff;

        last_key = bios_read_key() >> 8;

        update_button_state();
        scroll_play_area();
        step_counters();

        if (freeform != 0)
            select_music_by_key();

        regions_handle_pointer(regions_play);

        if (round_state == 0x800)
            bin_scroll_back();
        else if (round_state == 0x400)
            bin_scroll_forward();

        if (point_in_play_area() != 0) {
            pointer_frame();
            si = 0;
        } else {
            if (HELD_PARTS.dragged_part != 0 && si == 0) {
                mark_joined_shapes(HELD_PARTS.dragged_part, 3);
                mark_part_shapes(HELD_PARTS.dragged_part, 3);
            }
            edge_scroll_flags();
            si = 1;
        }

        if (redraw_e != 0) { draw_machine_layer_a(); redraw_e--; }
        if (redraw_d != 0) { draw_machine_layer_b(); redraw_d--; }
        if (redraw_c != 0) { draw_machine_layer_c(); redraw_c--; }
        if (redraw_b != 0) { draw_machine_layer_d(); redraw_b--; }
        if (redraw_a != 0) { draw_machine_layer_e(); redraw_a--; }

        mark_parts_in_dirty_rects();
        replay_shapes();
        step_and_draw_machine(0);

        if (HELD_PARTS.dragged_part != 0 && drop_cursor != -1)
            draw_part_selection(HELD_PARTS.dragged_part, drop_cursor, 1);

        if (band_colour != -1) {
            cursor_redraw_off_thunk();
            VMDS.second_colour = (uint8_t)band_colour;
            clip_and_draw_line(anchor_x - origin_x,
                               anchor_y - origin_y,
                               band_x - origin_x,
                               band_y - origin_y);
            restore_cursor_following();
            alloc_shape((const uint8_t *)&anchor_x,
                        (const uint8_t *)&band_x,
                        4, 2, 0);
        }

        if (redraw_carried != 0) { draw_carried_icon(); redraw_carried--; }

        seg172c_nothing();

        while ((int16_t)(0x2710 - TIMER.frame_budget) < 8)
            ;
        TIMER.frame_budget = 0x2710;

        present_frame(1);
        shift_all_histories();

        if (POINTER.button_right == 2)
            round_state = 2;
    }

    if (HELD_PARTS.dragged_part != 0
        && (HELD_PARTS.dragged_part->flags_06 & 0x800) != 0) {
        if (HELD_PARTS.dragged_part->kind == KIND_BELT
            && HELD_PARTS.dragged_part->rope->end_a != 0)
            discard_carried_part();
        else if (HELD_PARTS.dragged_part->kind == KIND_ROPE
                 && HELD_PARTS.dragged_part->belt[0]->end_a != 0)
            discard_carried_part();
        else
            finish_part_removal();
    }
}

/*
 * 0x0faf9
 *
 * **Pick a tune from the keyboard.** DGROUP 0x52f1 is the last key the loop
 * read, *not* a level number, and this is a jump table on it at CS:0x1b8c -
 * the scancode less two, refusing anything past 0x2e.
 *
 * Read as scancodes the sixteen entries are exactly the number row and the
 * first seven letters:
 *
 *     1 2 3 4 5 6 7 8 9   ->  tunes 0x3e9 .. 0x3f1
 *     A B C D E F G       ->  tunes 0x3f2 .. 0x3f8
 *
 * which is why the table looked like an arbitrary jumble of levels - 2..10,
 * then 30, 48, 46, 32, 18, 33, 34 - when read as anything else. The remaining
 * thirty-one entries all point at the arm that loads -1.
 *
 * `game_screen_loop` only calls this when 0x4e67 is set, so the shortcut is a
 * freeform-mode feature and not a cheat that works everywhere.
 *
 * -1 means silence and returns without touching anything. Otherwise the tune
 * is remembered at 0x50bb - which is where `game_round` reads it from when it
 * restarts the music - and started.
 */
void select_music_by_key(void)
{
    int16_t si;

    switch (last_key) {
    case 2:  si = 0x3e9; break;
    case 3:  si = 0x3ea; break;
    case 4:  si = 0x3eb; break;
    case 5:  si = 0x3ec; break;
    case 6:  si = 0x3ed; break;
    case 7:  si = 0x3ee; break;
    case 8:  si = 0x3ef; break;
    case 9:  si = 0x3f0; break;
    case 10:  si = 0x3f1; break;
    case 30:  si = 0x3f2; break;
    case 48:  si = 0x3f3; break;
    case 46:  si = 0x3f4; break;
    case 32:  si = 0x3f5; break;
    case 18:  si = 0x3f6; break;
    case 33:  si = 0x3f7; break;
    case 34:  si = 0x3f8; break;
    default: si = -1;
    }

    if (si != -1) {
        LEVEL_SETTINGS.tune = si;
        select_music(LEVEL_SETTINGS.tune);
    }
}

/*
 * 0x0fbda
 *
 * Put the level back to the state it starts in: no tool selected, nothing in
 * hand, and the eight words from 0x4e69 and 0x4e87 through 0x4e93 cleared.
 *
 * Those seven at 0x4e87 upward are the loop's **deferred redraw counters** -
 * the ones `game_screen_loop` decrements a frame at a time - so clearing them is
 * cancelling every redraw that was still owed, which is right because the
 * three calls after it redraw everything anyway.
 */
void reset_level_state(void)
{
    redraw_e = redraw_d = redraw_c = redraw_b
        = redraw_a = redraw_carried = loop_frames
        = tool = 0;
    HELD_PARTS.dragged_part = 0;

    clear_layer_heads();
    reset_machine();
    redraw_machine_area();
}

/*
 * 0x0fc0e
 *
 * **One frame of whatever the pointer is doing to a part** - the level loop's
 * pointer half, and the routine that turns a position into a tool.
 *
 * `si` is set when the hand is already busy: tool 9, or any tool with the top
 * bit, which is a drag in progress. Only when it is *not* busy does this look
 * for something new - `find_part_from` on the currently carried part, with a
 * part whose +6 has bit 0x8000 refused - and only then does
 * `part_handle_at_pointer` choose a tool from where the pointer is.
 *
 * So a drag keeps its tool for as long as it lasts, and a fresh pointer picks
 * one every frame. Nothing under the pointer clears the tool and returns.
 *
 * 0x52c7 is set to 0xa on every frame the hand is not already carrying, which
 * is the plain cursor; the movers overwrite it with 0xe or 0xc when they have
 * an opinion about dropping.
 *
 * The tool then picks an arm through a jump table at CS:0x1cfe, on the tool
 * less one with the top bit stripped, and **anything outside 1 to 10 falls
 * through doing nothing**. Four of the ten act only on the press edge, three
 * act every frame, and tool 10's whole body is to let go of the part.
 */
void pointer_frame(void)
{
    int16_t si;

    if (tool == 9 || (tool & 0x8000) != 0)
        si = 1;
    else
        si = 0;

    if (si == 0) {
        HELD_PARTS.dragged_part = (find_part_from(HELD_PARTS.dragged_part));
        if (HELD_PARTS.dragged_part != 0
            && (HELD_PARTS.dragged_part->flags_06 & 0x8000) != 0)
            HELD_PARTS.dragged_part = 0;
    }

    if (HELD_PARTS.dragged_part != 0) {
        if (tool != 9)
            drop_cursor = 0x0a;

        if (si == 0)
            tool = part_handle_at_pointer(HELD_PARTS.dragged_part);

        switch (tool & 0x7fff) {
        case 9:
            move_carried();
            break;
        case 7:
            if (POINTER.button_left == 2)
                pick_up_part();
            break;
        case 8:
            if (POINTER.button_left == 2)
                discard_carried_part();
            break;
        case 1:
            if (POINTER.button_left == 2)
                flip_carried_end_1();
            break;
        case 2:
            if (POINTER.button_left == 2)
                flip_carried_end_2();
            break;
        case 3:
        case 4:
        case 5:
        case 6:
            run_drag_frame();
            break;
        case 10:
            if (POINTER.button_left == 2)
                HELD_PARTS.dragged_part = 0;
            break;
        }
    } else {
        tool = 0;
    }
}

/*
 * 0x0fd02
 *
 * **Carrying a part off the edge of the play area asks for a redraw.**
 *
 * Only while a part is in hand - tool 9 with something at 0x50d5 - and only
 * for a part that is neither a rope nor a belt, because those two are drawn
 * from their endpoints and do not hang off the pointer.
 *
 * 0x4e89 is set to 1 unconditionally, and then each edge the pointer has gone
 * past sets its own counter to 3: above 8 or below 0x12f in 0x5782, left of 8
 * or right of 0x1ff in 0x5784. The right edge sets two of them, 0x4e93 as well
 * as 0x4e8b.
 *
 * Three, not one, because these are the countdowns `game_screen_loop` works through a
 * frame at a time - the strip has to be repainted for three frames, not
 * redrawn once.
 */
void edge_scroll_flags(void)
{
    uint16_t kind;                      /* dx */

    if (tool == 9 && HELD_PARTS.dragged_part != 0) {
        kind = HELD_PARTS.dragged_part->kind;
        if (kind != 8 && kind != 0x0a) {
            redraw_carried = 1;

            if (POINTER.pointer_y < 8)
                redraw_d = 3;
            if (POINTER.pointer_y > 0x12f)
                redraw_c = 3;
            if (POINTER.pointer_x < 8)
                redraw_b = 3;
            if (POINTER.pointer_x > 0x1ff)
                redraw_a = redraw_e = 3;
        }
    }
}

/*
 * 0x0fd65
 *
 * **Scroll the play area** when the pointer is against an edge, and re-file
 * everything in it if it moved.
 *
 * The current origins at 0x4ea3 and 0x4ea1 are first copied down to 0x4e9b and
 * 0x4e99, which is where `draw_carried_icon` reads the *previous* position
 * from - so this is also what makes an icon's backdrop restorable after a
 * scroll.
 *
 * Then four edges, each a pair of tests: the pointer at or past the edge, and
 * the origin not already at its stop. Left stops at -8 and top at -8; right
 * and bottom stop at 0x50b7 and 0x50b9, which is where the level's own extent
 * is kept. A step is 0x10 either way. **Both axes can move in one call** - the
 * flag is shared and the two offsets are independent - so a pointer held in a
 * corner scrolls diagonally.
 *
 * Nothing is written back unless something moved. When it did, every part is
 * walked - `pick_by_flag(0x3000)` then `pick_for_record(si, 0x1000)` - and
 * each one that does not have bit 0x2000 in +8 is marked for re-filing and its
 * shapes re-marked. The parts do not move; the window over them does, so what
 * was drawn where is no longer true.
 *
 * The origins are stored **after** that walk, not before, so the marking sees
 * the old position.
 */
void scroll_play_area(void)
{
    struct part *si;
    int16_t  x;                         /* di */
    int16_t  y;                         /* [bp-2] */
    int16_t  moved;                     /* [bp-4] */

    origin_c_x = origin_b_x;
    origin_c_y = origin_b_y;
    origin_b_x = origin_x;
    origin_b_y = origin_y;

    x = origin_x;
    y = origin_y;
    moved = 0;

    if (POINTER.pointer_x <= 0 && origin_x != -8) {
        moved = 1;
        x -= 0x10;
    }
    if (POINTER.pointer_x >= 0x27f && origin_x != LEVEL_SETTINGS.extent_y) {
        moved = 1;
        x += 0x10;
    }
    if (POINTER.pointer_y <= 0 && origin_y != -8) {
        moved = 1;
        y -= 0x10;
    }
    if (POINTER.pointer_y >= 0x16f && origin_y != LEVEL_SETTINGS.extent_x) {
        moved = 1;
        y += 0x10;
    }

    if (moved != 0) {
        si = pick_by_flag(0x3000);
        while (si != NULL) {
            if ((si->flags_08 & 0x2000) == 0) {
                mark_needs_refile(si, 2);
                mark_part_shapes(si, 3);
            }
            si = pick_for_record(si, 0x1000);
        }

        origin_x = x;
        origin_y = y;
    }
}

/*
 * 0x0fe47
 *
 * Move whatever is in your hand, by kind. Tool 9's arm of the level loop.
 *
 * **+0x20 and +0x1e are set to -1 first**, both of them, before anything looks
 * at the kind. A carried part has no position until the mover gives it one,
 * and -1 is what the drawing code reads as "nowhere yet" - so a frame that
 * ends up not placing it leaves it off the board rather than at its old spot.
 */
void move_carried(void)
{
    HELD_PARTS.dragged_part->pos[0].x
        = HELD_PARTS.dragged_part->pos[0].y = -1;

    if (HELD_PARTS.dragged_part->kind == KIND_BELT)
        move_carried_rope();
    else if (HELD_PARTS.dragged_part->kind == KIND_ROPE)
        move_carried_belt();
    else
        move_carried_part();
}

/*
 * 0x0fe84
 *
 * Move a carried **rope** with the pointer, and drop it when the button goes
 * down.
 *
 * Two halves. With the button down, `rope_ends_close` decides what happens:
 * ends too far apart and the rope is thrown away, but only if it had a far
 * part - `di` non-zero - because a rope attached to nothing has nothing to
 * come apart. Close enough, and `find_part_from(0)` is asked what is under the
 * pointer and the rope is joined to it: bit 2 into that part's +8, +0x94
 * refreshed, and the link's **+6 or +4** set depending on whether the far end
 * was already taken. Then the endpoints are recomputed, the rope re-filed, and
 * both the tool and the carried part cleared.
 *
 * With the button up it is only preview: 0x52c1 and 0x52c3 take the far part's
 * anchor - its +0x1e and +0x20 plus the bytes at +0x56 and +0x57 - and 0x52bd
 * and 0x52bf take the pointer in play-area coordinates, so something else can
 * draw the rubber-band line. 0x52c5 is the colour, 0xa when the ends are close
 * enough to join and 0xc when they are not.
 */
void move_carried_rope(void)
{
    struct part *si;
    struct part *di;
    int16_t close;                      /* [bp-2] */
    struct rope *link;                  /* [bp-4] */

    link = HELD_PARTS.dragged_part->rope;
    di = link->end_a;
    close = rope_ends_close(link);

    if (POINTER.button_left == 2) {
        if (close == 0) {
            if (di != NULL)
                discard_carried_part();
        } else {
            si = find_part_from(NULL);

            if (di != NULL) {
                si->flags_08 |= 2;
                si->start_flags = si->flags_08;
                link->end_b = si;
                si->rope = link;

                compute_link_endpoints(link);
                mark_needs_refile(HELD_PARTS.dragged_part, 2);
                refile_part_list(HELD_PARTS.dragged_part);
                tool = 0;
                HELD_PARTS.dragged_part = 0;
            } else {
                si->flags_08 |= 2;
                si->start_flags = si->flags_08;
                link->end_a = si;
                si->rope = link;
            }
        }
    } else if (di != NULL) {
        anchor_x = di->pos[0].x + di->grab.x;
        anchor_y = di->pos[0].y + di->grab.y;
        band_x = POINTER.pointer_x + origin_x;
        band_y = POINTER.pointer_y + origin_y;

        if (close != 0)
            band_colour = 0x0a;
        else
            band_colour = 0x0c;
    }
}

/*
 * 0x0ff80
 *
 * Move a carried **belt** with the pointer, attach it when the button goes
 * down, and preview it when the button is up.
 *
 * `find_belt_anchor` says what is under the pointer and which of its ends;
 * 0x2630 remembers that between frames. Two anchors are refused outright: the
 * one already at the belt's other end (0x5456) and the far part it is already
 * joined to, and both only when there *is* a far part - so the first end can
 * legally land on anything.
 *
 * With the button down and no anchor, a belt that already had a far part is
 * thrown away and one that did not is simply left alone.
 *
 * **The two ends are not symmetric.** The first end - no far part yet - just
 * records itself in the anchor's +0x66 pair and in the link's +2, +6, +0xa and
 * +0xc, and refuses a pulley outright. The second end does the geometry: a
 * pulley anchor takes the belt in its single socket at +0x5a and +0x5e, any
 * other part takes it at the end named by the link's +0xa **and again two
 * slots further on**, then the link is re-measured and the whole thing
 * re-filed and let go of. A pulley on the far side is passed to aim_link_at_bisector
 * either way.
 *
 * Button up is preview only, and it changes the machine anyway when the far
 * end is a pulley: aim_link_at_bisector and three marks, before working out the line to
 * draw. 0x52c1 and 0x52c3 are the anchor point, 0x52bd and 0x52bf the pointer
 * in play-area coordinates, 0x52c5 the colour - 0xa where it would attach and
 * 0xc where it would not.
 */
void move_carried_belt(void)
{
    struct belt *si;
    struct part *di;
    int16_t end;                        /* [bp-2] */
    struct part *far_;                  /* [bp-4] */

    si = HELD_PARTS.dragged_part->belt[0];
    far_ = si->end_a;

    di = find_belt_anchor(&end, GOAL_TESTS.belt_anchor);

    if (di == belt_far_end && far_ != NULL)
        di = NULL;
    else if (di == far_ && far_ != NULL)
        di = NULL;

    GOAL_TESTS.belt_anchor = di;

    if (POINTER.button_left == 2) {
        if (di == NULL) {
            if (far_ != NULL)
                discard_carried_part();
        } else if (far_ != NULL) {
            if (belt_far_end->kind == KIND_PULLEY) {
                belt_far_end->link[2]
                    = belt_far_end->link[0] = di;
                mark_joined_shapes(belt_far_end, 3);
                mark_part_shapes(belt_far_end, 3);
                mark_needs_refile(belt_far_end, 2);
            } else {
                belt_far_end->link[si->slot_a + 2]
                    = belt_far_end->link[si->slot_a] = di;
            }

            refresh_link_geometry(si);
            mark_needs_refile(HELD_PARTS.dragged_part, 2);

            if (di->kind == KIND_PULLEY) {
                di->link[3] = di->link[1] = belt_far_end;
                di->belt[1] = si;
                if (belt_far_end->kind == KIND_PULLEY)
                    aim_link_at_bisector(belt_far_end);
                belt_far_end = di;
            } else {
                di->link[end + 2] = di->link[end] = belt_far_end;
                di->belt[end] = si;
                si->home_b = si->end_b = di;
                si->home_slot_b = si->slot_b = end;
                if (belt_far_end->kind == KIND_PULLEY)
                    aim_link_at_bisector(belt_far_end);
                refile_part_list(HELD_PARTS.dragged_part);
                tool = 0;
                HELD_PARTS.dragged_part = 0;
            }
        } else if (di->kind != KIND_PULLEY) {
            di->belt[end] = si;
            si->home_a = si->end_a = di;
            si->home_slot_a = si->slot_a = end;
            belt_far_end = di;
        }
    } else if (far_ != NULL) {
        if (belt_far_end->kind == KIND_PULLEY) {
            end = 1;
            aim_link_at_bisector(belt_far_end);
            mark_joined_shapes(belt_far_end, 3);
            mark_part_shapes(belt_far_end, 3);
            mark_needs_refile(belt_far_end, 2);
        } else {
            end = si->slot_a;
        }

        anchor_x = belt_far_end->pos[0].x
                        + belt_far_end->attach[end].x;
        anchor_y = belt_far_end->pos[0].y
                        + belt_far_end->attach[end].y;
        band_x = POINTER.pointer_x + origin_x;
        band_y = POINTER.pointer_y + origin_y;

        if (di != NULL)
            band_colour = 0x0a;
        else
            band_colour = 0x0c;
    }
}

/*
 * 0x101dc
 *
 * **Move an ordinary carried part** with the pointer - everything that is not
 * a rope or a belt - and put it down when the button goes down.
 *
 * The level's own opinion is asked first, through `part_key_shortcut`,
 * which does nothing at all on all but six levels.
 *
 * Then the position, and bit 8 of +0xa decides which of two quite different
 * ways: **free** placement follows the pointer exactly and is clamped into the
 * play area by 0xc at the near edges and 0x235 and 0x165 at the far ones;
 * **snapped** placement masks the pointer to a multiple of 16 and, if the part
 * would end up entirely off the near edge, nudges it back by one whole cell
 * rather than clamping. Both take the grab offset at 0x4e97 and 0x4e95 off
 * first, so the part stays held where it was picked up.
 *
 * `di` is whether the part's rope is *not* close enough to stay joined -
 * `neg/sbb/inc` around `rope_ends_close`, which is Borland's `== 0` - and it
 * is only consulted when the part is actually put down.
 *
 * Then +0xa again: bit 1 re-homes the part onto whatever it is near, bit 2
 * goes to break_second_attachment instead, and neither is tried if the other matched.
 *
 * The ending is three-way. Overlapping something sets the cursor colour at
 * 0x52c7 to 0xe and nothing else happens - you cannot drop a part inside
 * another. The button down commits: marks, then **a rope that has come too far
 * apart is untied and thrown away**, then +0x8c and +0x8e remember where the
 * part landed, it is re-filed, and the hand is emptied. Neither of those and
 * the colour is 0xc, meaning it would drop cleanly.
 *
 * `part_moved` runs on every frame the button is *not* down, which is to say
 * while it is still being dragged rather than when it lands.
 */
void move_carried_part(void)
{
    struct rope *si;
    int16_t di;

    part_key_shortcut();

    if (CARRIED->flags_0a & 8) {
        CARRIED->pos[0].x = POINTER.pointer_x - drag_offset_x + origin_x;
        if (CARRIED->pos[0].x + CARRIED->size[0].width <= origin_x + 0x0c)
            CARRIED->pos[0].x = origin_x - CARRIED->size[0].width + 0x0c;
        if (CARRIED->pos[0].x >= origin_x + 0x235)
            CARRIED->pos[0].x = origin_x + 0x235;

        CARRIED->pos[0].y = POINTER.pointer_y - drag_offset_y + origin_y;
        if (CARRIED->pos[0].y + CARRIED->size[0].height <= origin_y + 0x0c)
            CARRIED->pos[0].y = origin_y - CARRIED->size[0].height + 0x0c;
        if (CARRIED->pos[0].y >= origin_y + 0x165)
            CARRIED->pos[0].y = origin_y + 0x165;
    } else {
        CARRIED->pos[0].x = ((POINTER.pointer_x - drag_offset_x) & 0xfff0) + origin_x;
        if (CARRIED->pos[0].x + CARRIED->size[0].width <= origin_x)
            CARRIED->pos[0].x += 0x10;

        CARRIED->pos[0].y = ((POINTER.pointer_y - drag_offset_y) & 0xfff0) + origin_y;
        if (CARRIED->pos[0].y + CARRIED->size[0].height <= origin_y)
            CARRIED->pos[0].y += 0x10;
    }

    place_object_for_draw(CARRIED);
    retension_pulleys(CARRIED);

    if ((si = CARRIED->rope) != NULL)
        di = !rope_ends_close(si);
    else
        di = 0;

    if (CARRIED->flags_0a & 1)
        rehome_carried_part();
    else if (CARRIED->flags_0a & 2)
        break_second_attachment(CARRIED);

    if (object_overlaps_any(CARRIED) != 0) {
        drop_cursor = 0x0e;
    } else if (POINTER.button_left == 2) {
        mark_joined_shapes(CARRIED, 3);

        if (di != 0) {
            untie_rope(si->owner);
            discard_part(si->owner);
            redraw_e = 2;
        }

        mark_needs_refile(CARRIED, 2);
        CARRIED->start_x = CARRIED->pos[0].x;
        CARRIED->start_y = CARRIED->pos[0].y;
        refile_part_list(CARRIED);
        tool = 0;
        HELD_PARTS.dragged_part = 0;
    } else {
        drop_cursor = 0x0c;
    }

    if (POINTER.button_left != 2)
        part_moved(CARRIED);
}

/*
 * 0x10410
 *
 * **The keyboard shortcuts for the part in your hand.** DGROUP 0x52f1 is the
 * last key, and six scancodes have a meaning here; every other key falls
 * straight out, which is what this routine does almost every frame.
 *
 * The table of six at CS:0x2650 is searched with a `loop` and a hit jumps
 * through the parallel table twelve bytes further on. Read as scancodes it is
 * obvious what they are:
 *
 *     45  X          flip the first end, if the part has one
 *     21  Y          flip the second end, if the part has one
 *     13  =   78  +  grow the part in your hand
 *     12  -   74  -  shrink it
 *
 * X and Y flipping the two axes is what identifies the table; as level
 * numbers - which is how this was first written up, because 0x52f1 was
 * mistaken for the level - 12, 13, 21, 45, 74 and 78 look like nothing at all.
 *
 * The two flip arms are here in full because they are five instructions each;
 * the resize pair are ~235 bytes apiece and have routines of their own.
 *
 * `si` is loaded with the part's kind at entry and never used. That is the
 * original's, not an omission.
 */
void part_key_shortcut(void)
{
    int16_t si;                         /* the carried part's kind */

    si = CARRIED->kind;

    switch (last_key) {
    case 0x2d:                          /* X */
        if (CARRIED->flags_06 & 0x400)
            flip_carried_end_1();
        break;
    case 0x15:                          /* Y */
        if (CARRIED->flags_06 & 0x200)
            flip_carried_end_2();
        break;
    case 0x0d:                          /* = */
    case 0x4e:                          /* keypad + */
        if (CARRIED->set_size.height <= CARRIED->set_size.width
            || CARRIED->kind == KIND_RAMP) {
            if (PART_KINDS[si].max_w > CARRIED->set_size.width) {
                CARRIED->set_size.width += 0x10;
                CARRIED->mirror_size.width = CARRIED->set_size.width;
                PART_KINDS[si].settle(CARRIED);
                place_object_for_draw(CARRIED);
                mark_needs_refile(CARRIED, 2);
                mark_joined_shapes(CARRIED, 3);
            }
        } else {
            if (PART_KINDS[si].max_h > CARRIED->set_size.height) {
                CARRIED->set_size.height += 0x10;
                CARRIED->mirror_size.height = CARRIED->set_size.height;
                PART_KINDS[si].settle(CARRIED);
                place_object_for_draw(CARRIED);
                mark_needs_refile(CARRIED, 2);
                mark_joined_shapes(CARRIED, 3);
            }
        }
        break;
    case 0x0c:                          /* - */
    case 0x4a:                          /* keypad - */
        if (CARRIED->set_size.height <= CARRIED->set_size.width
            || CARRIED->kind == KIND_RAMP) {
            if (PART_KINDS[si].min_w < CARRIED->set_size.width) {
                CARRIED->set_size.width -= 0x10;
                CARRIED->mirror_size.width = CARRIED->set_size.width;
                PART_KINDS[si].settle(CARRIED);
                place_object_for_draw(CARRIED);
                mark_needs_refile(CARRIED, 2);
                mark_joined_shapes(CARRIED, 3);
            }
        } else {
            if (PART_KINDS[si].min_h < CARRIED->set_size.height) {
                CARRIED->set_size.height -= 0x10;
                CARRIED->mirror_size.height = CARRIED->set_size.height;
                PART_KINDS[si].settle(CARRIED);
                place_object_for_draw(CARRIED);
                mark_needs_refile(CARRIED, 2);
                mark_joined_shapes(CARRIED, 3);
            }
        }
        break;
    }
}

/*
 * 0x10658
 *
 * **Pick a placed part up** and start carrying it - tool 7's arm, taken when
 * the button goes down on a part's body.
 *
 * The grab offset is saved first: 0x4e97 and 0x4e95 are the pointer less the
 * part's own origin, so a part picked up by its corner stays held by its
 * corner however far the pointer then moves.
 *
 * `di` is the part's +0x54 and `si` **its +4, read only if +0x54 is not zero**.
 * `si` is used again at the end, and only on the branch where the part is a
 * rope - which is exactly when +0x54 is set - so the original's conditional
 * load is safe. It is initialised to 0 here because C says so; the original
 * would be carrying whatever SI held.
 *
 * The part is unmarked, then detached according to kind: a rope untied, a belt
 * detached with `how` 0 after stashing its far end's +0x5a at 0x5456, anything
 * else through detach_part_to_bin. A rope then has its link put back the other way
 * round - `di->+4 = si`, `si->+0x54 = di` - with bit 2 set in the far part's
 * +8 and +0x94 refreshed to match, so the rope is now held by the end you did
 * not grab.
 *
 * Tool 9 last, which is what makes everything else treat this as carried.
 */
void pick_up_part(void)
{
    struct part *si;                    /* what the rope's end A holds */
    struct rope *di;
    struct belt *rec;                   /* [bp-2] */

    drag_offset_x = POINTER.pointer_x - CARRIED->pos[0].x + origin_x;
    drag_offset_y = POINTER.pointer_y - CARRIED->pos[0].y + origin_y;

    if ((di = CARRIED->rope) != NULL)
        si = di->end_a;
#ifndef __TURBOC__
    else
        si = NULL;     /* never read: only a part with a rope reaches the use */
#endif

    mark_joined_shapes(CARRIED, 3);
    mark_part_shapes(CARRIED, 3);

    if (CARRIED->kind == KIND_BELT) {
        untie_rope(CARRIED);
    } else if (CARRIED->kind == KIND_ROPE) {
        rec = CARRIED->belt[0];
        belt_far_end = rec->end_b->link[rec->slot_b];
        detach_belt(CARRIED, 0);
    } else {
        detach_part_to_bin(CARRIED);
    }

    if (CARRIED->kind == KIND_BELT) {
        di->end_a = si;
        si->flags_08 |= 2;
        si->start_flags = si->flags_08;
        si->rope = di;
    }

    tool = 9;
}

/*
 * 0x10733
 *
 * **Throw away the part in hand**, whatever kind it is, and leave the player
 * holding nothing.
 *
 * Its shapes are unmarked first - `mark_joined_shapes` and `mark_part_shapes`
 * both with mode 3 - so nothing that was drawn for it is left claiming space.
 * Then three ways to go, on the kind at +4:
 *
 *   a rope, kind 8      untie it from both ends, then discard
 *   a belt, kind 0x0a   detach it with `how` 1, then discard
 *   anything else       detach_part_to_bin and finish_part_removal
 *
 * The two ends of the first two are why they need untying before discarding: a
 * rope or a belt is joined to parts that outlive it, and freeing the record
 * without breaking the joins leaves those parts pointing at it.
 *
 * It closes by setting 0x4e93 to 2 and the tool at 0x4e69 to 0 - the hand is
 * empty, so no tool is selected. Both are unconditional and outside the
 * branch, and are transcribed there.
 */
void discard_carried_part(void)
{
    mark_joined_shapes(CARRIED, 3);
    mark_part_shapes(CARRIED, 3);

    if (CARRIED->kind == KIND_BELT) {
        untie_rope(CARRIED);
        discard_part(CARRIED);
    } else if (CARRIED->kind == KIND_ROPE) {
        detach_belt(CARRIED, 1);
        discard_part(CARRIED);
    } else {
        detach_part_to_bin(CARRIED);
        finish_part_removal();
    }

    redraw_e = 2;
    tool = 0;
}

/*
 * 0x107b6
 *
 * Flip the carried part's **first** end, for real - the arm the level loop
 * takes for tool 1 when the button has just gone down.
 *
 * The same flip hook `part_flip_options` uses to *test* an end, called once
 * with 1 and not undone, so this is the move rather than the trial. +0x94 is
 * refreshed from +8 afterwards for the same reason it is there: the hook
 * changes +8 and the two must not drift apart.
 */
void flip_carried_end_1(void)
{
    PART_KINDS[CARRIED->kind].flip(CARRIED, 1);
    CARRIED->start_flags = CARRIED->flags_08;
}

/*
 * 0x107e6
 *
 * The **second** end, and `flip_carried_end_1` with a 2 in it - tool 2's arm.
 * Kept as two routines because the original has two; they differ in one
 * immediate and nothing else.
 */
void flip_carried_end_2(void)
{
    PART_KINDS[CARRIED->kind].flip(CARRIED, 2);
    CARRIED->start_flags = CARRIED->flags_08;
}

/*
 * 0x10816
 *
 * **Run one frame of a drag.** The level loop's arm for tools 3 to 6, and the
 * only place the four drag routines are called from.
 *
 * The top bit of 0x4e69 is what says a drag is in progress. Without it, this
 * does one thing: if the button has just gone down, set the bit and return -
 * so the frame that starts a drag does no dragging.
 *
 * With it, the low bits pick one of four through a jump table at CS:0x28f4,
 * indexed by `0x4e69 - 0x8003`, and anything outside 0..3 falls through with
 * nothing moved rather than being rejected:
 *
 *     0x8003  drag_carried_part_first     0x8005  drag_carried_part_pair
 *     0x8004  settle_carried_part_first   0x8006  settle_carried_part
 *
 * If the part moved, +0x42 and +0x40 take copies of +0x52 and +0x50 - the
 * position the *next* frame will treat as where it came from - and the part is
 * re-hooked, re-placed and marked three ways.
 *
 * The button being down **ends** the drag, clearing both the tool and the
 * carried part. That is not a mistake: 0x5774 is 2 only on the press edge, so
 * the drag runs while the button is up and the next press drops it.
 */
void run_drag_frame(void)
{
    int16_t si;

    if (tool & 0x8000) {
        si = 0;

        switch (tool) {
        case 0x8003: si = drag_carried_part_first();   break;
        case 0x8004: si = settle_carried_part_first(); break;
        case 0x8005: si = drag_carried_part_pair();    break;
        case 0x8006: si = settle_carried_part();       break;
        }

        if (si != 0) {
            CARRIED->mirror_size = CARRIED->set_size;

            PART_KINDS[CARRIED->kind].settle(CARRIED);
            place_object_for_draw(CARRIED);
            mark_joined_shapes(CARRIED, 3);
            mark_part_shapes(CARRIED, 3);
            mark_needs_refile(CARRIED, 2);
        }

        if (POINTER.button_left == 2) {
            tool = 0;
            HELD_PARTS.dragged_part = 0;
        }
    } else if (POINTER.button_left == 2) {
        tool |= 0x8000;
    }
}

/*
 * 0x108ec
 *
 * Drag the carried part by its **first** pair - `drag_carried_part_pair`'s
 * sibling, on +0x1e and +0x50 rather than +0x20 and +0x52, driven by the other
 * pointer axis and clamped against the kind's other pair of bounds, +0x0c and
 * +0x10. It remembers into +0x8c where the other remembers into +0x8e.
 *
 * The four differ only in which words they touch; each is written out rather
 * than folded into one routine taking offsets, because that is how the
 * original has them and a table of offsets would be a different program that
 * happens to agree.
 */
int16_t drag_carried_part_first(void)
{
    int16_t si;                         /* the new edge */
    int16_t di;                         /* the new size */
    int16_t was;                        /* [bp-2] */
    int16_t lo;                         /* [bp-4] */
    int16_t hi;                         /* [bp-6] */
    int16_t moved;                      /* [bp-8] */

    moved = 0;
    was = CARRIED->pos[0].x;
    si = (POINTER.pointer_x & 0xfff0) + origin_x;
    lo = PART_KINDS[CARRIED->kind].min_w;
    hi = PART_KINDS[CARRIED->kind].max_w;
    di = was - si + CARRIED->set_size.width;

    if (di > hi) {
        si += di - hi;
        di = hi;
    } else if (di < lo) {
        si -= lo - di;
        di = lo;
    }

    if (was != si) {
        CARRIED->pos[0].x = si;
        CARRIED->set_size.width = di;
        while (PART_KINDS[CARRIED->kind].settle(CARRIED),
               place_object_for_draw(CARRIED),
               PART_KINDS[CARRIED->kind].setup(CARRIED),
               object_overlaps_any(CARRIED)) {
            CARRIED->pos[0].x += 0x10;
            CARRIED->set_size.width -= 0x10;
        }

        if (CARRIED->pos[0].x != was) {
            CARRIED->start_x = CARRIED->pos[0].x;
            moved = 1;
        }
    }

    return moved;
}

/*
 * 0x10a00
 *
 * Settle the carried part on its **first** axis - `settle_carried_part`'s
 * sibling, on +0x50 and +0x1e rather than +0x52 and +0x20, taking its target
 * from 0x5784 and 0x4ea3 and clamping against the kind's +0x0c and +0x10.
 *
 * The fourth and last of the drag family, and like the other three it lifts by
 * a whole row at a time until `object_overlaps_any` is satisfied, with the
 * first placement before the first test - a do-while, as written.
 */
int16_t settle_carried_part_first(void)
{
    int16_t si;                         /* the new size */
    int16_t di;                         /* the old */
    int16_t lo;                         /* [bp-2] */
    int16_t hi;                         /* [bp-4] */
    int16_t moved;                      /* [bp-6] */

    moved = 0;
    di = CARRIED->set_size.width;
    si = (POINTER.pointer_x & 0xfff0) + origin_x + 0x10 - CARRIED->pos[0].x;
    lo = PART_KINDS[CARRIED->kind].min_w;
    hi = PART_KINDS[CARRIED->kind].max_w;

    if (si > hi)
        si = hi;
    else if (si < lo)
        si = lo;

    if (di != si) {
        CARRIED->set_size.width = si;
        while (PART_KINDS[CARRIED->kind].settle(CARRIED),
               place_object_for_draw(CARRIED),
               PART_KINDS[CARRIED->kind].setup(CARRIED),
               object_overlaps_any(CARRIED)) {
            CARRIED->set_size.width -= 0x10;
        }

        if (CARRIED->set_size.width != di)
            moved = 1;
    }

    return moved;
}

/*
 * 0x10ada
 *
 * Drag the carried part **along its other axis**, and say whether it moved -
 * `settle_carried_part`'s twin, and not a mirror of it.
 *
 * Where that one moves +0x52 alone, this moves +0x20 and +0x52 **together and
 * in opposite directions**: the new +0x20 comes from the pointer, +0x52 is
 * derived from it as `was + pointer_delta`, and the settling loop adds 0x10 to
 * one while taking 0x10 off the other. The pair is a diagonal, which is what a
 * part with two ends slides along.
 *
 * The clamp is on +0x52 against the same kind bounds at +0x12 and +0x0e, and
 * the *overshoot is pushed back into +0x20* rather than discarded - `si +=
 * di - hi` - so the two stay consistent when the end is pinned.
 *
 * On success +0x8e takes a copy of the new +0x20, which the plain vertical
 * drag does not do.
 */
int16_t drag_carried_part_pair(void)
{
    int16_t si;                         /* the new edge */
    int16_t di;                         /* the new size */
    int16_t was;                        /* [bp-2] */
    int16_t lo;                         /* [bp-4] */
    int16_t hi;                         /* [bp-6] */
    int16_t moved;                      /* [bp-8] */

    moved = 0;
    was = CARRIED->pos[0].y;
    si = (POINTER.pointer_y & 0xfff0) + origin_y;
    lo = PART_KINDS[CARRIED->kind].min_h;
    hi = PART_KINDS[CARRIED->kind].max_h;
    di = was - si + CARRIED->set_size.height;

    if (di > hi) {
        si += di - hi;
        di = hi;
    } else if (di < lo) {
        si -= lo - di;
        di = lo;
    }

    if (was != si) {
        CARRIED->pos[0].y = si;
        CARRIED->set_size.height = di;
        while (PART_KINDS[CARRIED->kind].settle(CARRIED),
               place_object_for_draw(CARRIED),
               PART_KINDS[CARRIED->kind].setup(CARRIED),
               object_overlaps_any(CARRIED)) {
            CARRIED->pos[0].y += 0x10;
            CARRIED->set_size.height -= 0x10;
        }

        if (CARRIED->pos[0].y != was) {
            CARRIED->start_y = CARRIED->pos[0].y;
            moved = 1;
        }
    }

    return moved;
}

/*
 * 0x10bee
 *
 * Drop the carried part onto something solid, and say whether it moved.
 *
 * Its y at +0x52 is first put where the pointer is - the pointer's row at
 * 0x5782 taken down to a multiple of 16, plus the play area's top at 0x4ea3
 * and one more row, less the part's own height at +0x20 - and then clamped
 * into the band its kind allows, +0x12 low and +0x0e high in the kind record.
 * The two are read through the usual `kind * 0x3a` stride.
 *
 * If that lands where it already was, nothing happens and the answer is 0.
 * Otherwise it settles: place it, ask `object_overlaps_any`, and while the
 * answer is yes lift it a whole row and ask again. The loop has no bound of
 * its own - it is the clamp above and the ceiling of the play area that end
 * it - and it is transcribed as the do-while the original writes, because the
 * first placement happens before the first test.
 *
 * Two of the three calls in the loop are per-kind hooks reached through far
 * pointers in the kind record, +0x32 and +0x2a. For every kind this game's
 * early levels use, +0x32 is the do-nothing hook.
 *
 * The answer is whether the part ended up somewhere other than where it
 * started, which is not the same as whether the loop ran: a part lifted and
 * put back reports 0.
 */
int16_t settle_carried_part(void)
{
    int16_t si;                         /* the new size */
    int16_t di;                         /* the old */
    int16_t lo;                         /* [bp-2] */
    int16_t hi;                         /* [bp-4] */
    int16_t moved;                      /* [bp-6] */

    moved = 0;
    di = CARRIED->set_size.height;
    /* **origin_x, not origin_y**, at 0x10c08: the original adds the
       horizontal scroll to the pointer's y here. */
    si = (POINTER.pointer_y & 0xfff0) + origin_x + 0x10 - CARRIED->pos[0].y;
    lo = PART_KINDS[CARRIED->kind].min_h;
    hi = PART_KINDS[CARRIED->kind].max_h;

    if (si > hi)
        si = hi;
    else if (si < lo)
        si = lo;

    if (di != si) {
        CARRIED->set_size.height = si;
        while (PART_KINDS[CARRIED->kind].settle(CARRIED),
               place_object_for_draw(CARRIED),
               PART_KINDS[CARRIED->kind].setup(CARRIED),
               object_overlaps_any(CARRIED)) {
            CARRIED->set_size.height -= 0x10;
        }

        if (CARRIED->set_size.height != di)
            moved = 1;
    }

    return moved;
}

/*
 * 0x10cc8
 *
 * **Scroll the parts bin back**, held down.
 *
 * Let go - the button word at 0x5774 is neither 1 nor 2 - and it resets its
 * own repeat phase and hands the screen back to state 0x1000. Held, it moves
 * one page every *third* call: `[0x2632] % 3`, a signed divide by 3 whose
 * remainder is the test, with the counter incremented on every call whether it
 * scrolled or not. That is the auto-repeat, and three frames is its rate.
 *
 * A page back is `bin_part_at_index(-5)`. When that answers where the cursor
 * already is there is nothing before it, and the bin **wraps to the far end**
 * through `bin_scroll_end` rather than stopping. Both moves ask for a redraw
 * by putting 2 in 0x4e93; a move that changes nothing asks for none.
 *
 * 0x4e8b is set to 2 on every path, held or not.
 */
void bin_scroll_back(void)
{
    struct part *si;

    if (POINTER.button_left != 1 && POINTER.button_left != 2) {
        GOAL_TESTS.back_held = 0;
        round_state = 0x1000;
    } else {
#ifdef __TURBOC__
        if (GOAL_TESTS.back_held % 3 == 0) {
#else
        if (bin_repeat_due(GOAL_TESTS.back_held)) {         /* deviation: see above */
#endif
            si = bin_part_at_index(-5);
            if (si != HELD_PARTS.bin_list) {
                HELD_PARTS.bin_list = si;
                redraw_e = 2;
            } else {
                si = bin_scroll_end();
                if (si != HELD_PARTS.bin_list) {
                    HELD_PARTS.bin_list = si;
                    redraw_e = 2;
                }
            }
        }

        GOAL_TESTS.back_held++;
    }

    redraw_a = 2;
}

/*
 * 0x10d37
 *
 * **Scroll the parts bin forward**, held down - `bin_scroll_back`'s twin, with
 * its own repeat counter at 0x2634 and the same one-page-in-three rate.
 *
 * The two are not mirror images at the end stop. Back wraps by asking
 * `bin_scroll_end` where the last page is; forward wraps by writing the list
 * head 0x50d7 straight into the cursor, because the head is a constant and the
 * end is not. Forward also does not compare against the old cursor first: a
 * step that answers something sets it, and a step that answers nothing wraps,
 * so 0x4e93 is asked for a redraw either way.
 */
void bin_scroll_forward(void)
{
    struct part *p;                         /* [bp-2] */

    if (POINTER.button_left != 1 && POINTER.button_left != 2) {
        GOAL_TESTS.forward_held = 0;
        round_state = 0x1000;
    } else {
#ifdef __TURBOC__
        if (GOAL_TESTS.forward_held % 3 == 0) {
#else
        if (bin_repeat_due(GOAL_TESTS.forward_held)) {      /* deviation: see above */
#endif
            if ((p = bin_part_at_index(5)) != 0)
                HELD_PARTS.bin_list = p;
            else
                HELD_PARTS.bin_list = (&HELD_PARTS.parts_bin);
            redraw_e = 2;
        }

        GOAL_TESTS.forward_held++;
    }

    redraw_a = 2;
}

/*
 * 0x10d99
 *
 * The cursor over the **box above the parts bin** - the region at 576,0 to
 * 632,63, which is row 1 of the table and carries no action bit of its own.
 *
 * Two answers, and it changes the region's *bit* as well as its cursor. While
 * a part is being carried - tool 9 - it hands the cursor question straight to
 * `region_cursor_bin` below, the box beneath it, and sets +0x10 to 0x1000 so a
 * click here does what a click in the bin does. Otherwise the cursor is 0x1a
 * and the bit is 0x2000.
 *
 * So the box is not a fixed control: what it means depends on whether your
 * hand is full. A region's +0x10 is the bit `build_screen_regions` filed from
 * the table, and this is one of the places that rewrites it.
 *
 * It begins at 0x10d99, the byte after `bin_scroll_forward`'s `retf`; the
 * port had it at 0x10da9, which is the `push cs` of its own far call to
 * `region_cursor_bin` - the call TLINK turned into `nop / push cs / call`.
 */
void region_cursor_bin_above(struct region *region)
{
    if (tool == 9) {
        region_cursor_bin(region);
        region->code = 0x1000;
    } else {
        region->cursor = 0x1a;
        region->code = 0x2000;
    }
}

/*
 * 0x10dc2
 *
 * **The parts bin's cursor** - the region at 576,100 to 632,144, row 4, whose
 * click handler is 0x10e14 alongside it.
 *
 * Carrying a part, tool 9, the cursor says what is in your hand, by the kind
 * at +4 of the part at DGROUP 0x50d5: a rope, kind 8, gets cursor 8; a belt,
 * kind 0x0a, gets 9; anything else 0. That is the same question
 * `cursor_for_tool` asks for its own ninth tool, asked again here rather than
 * shared, and the pointer is loaded once into DI instead of twice - the two
 * routines are not the same code and are not transcribed as if they were.
 *
 * Otherwise the cursor says whether there is anything here to pick up:
 * `bin_part_at_index` is asked for the region's own +4, and a record answers
 * cursor 2 while nothing answers 0.
 */
void region_cursor_bin(struct region *region)
{
    uint16_t di;                        /* the carried part's kind */

    if (tool == 9) {
        di = CARRIED->kind;
        if (di == 8)
            region->cursor = 8;
        else if (di == 0x0a)
            region->cursor = 9;
        else
            region->cursor = 0;
    } else if (bin_part_at_index(region->slot) != 0) {
        region->cursor = 2;
    } else {
        region->cursor = 0;
    }
}

/*
 * 0x10e14
 *
 * **Clicking the parts bin** - the click handler of the region whose cursor is
 * `region_cursor_bin`, filed at +0x16 of the same row.
 *
 * With a part already in hand it is a bin: the part is thrown away by
 * `discard_carried_part` and the tool is cleared. A rope or a belt sets 0x4e93
 * to 2 on the way, which the discard would set anyway - the original tests the
 * kind twice, here and inside, and both are transcribed.
 *
 * With an empty hand it is a source. `bin_part_at_index` is asked for the
 * region's own +4 and the record it answers is **dereferenced** - the part
 * taken is the one its +0 names, not the entry itself - and nothing there ends
 * the click.
 *
 * Then the interesting part, which is what freeform mode means for the bin.
 * When 0x4e67 is set the part is **cloned** rather than taken, so the bin
 * never empties, and the clone is spliced into the list right after the
 * original: `clone->next = part->next`, that node's `prev` set back to the
 * clone when there is one, `clone->prev = part`, `part->next = clone`.
 *
 * The memory check around it is worth reading slowly. 0x50d5 is set to **0**
 * before `check_room_for_part` and put back afterwards, so the part being
 * picked up is not counted against the room it needs, and the answer decides
 * whether the clone is kept or handed straight back to `free_part`. A refused
 * clone leaves the hand empty and the bin exactly as it was.
 *
 * Whatever ends up in hand, a non-zero 0x50d5 selects tool 9 - which is what
 * makes `cursor_for_tool` and `region_cursor_bin` start answering by kind.
 */
void region_click_bin(struct region *region)
{
    struct part *si;                    /* the clone */
    struct part *saved;                    /* [bp-2] */

    if (tool == 9) {
        if (CARRIED->kind == 8 || CARRIED->kind == 0x0a)
            redraw_e = 2;

        discard_carried_part();
        tool = 0;
    } else {
        drag_offset_x = drag_offset_y = 0;

        if ((HELD_PARTS.dragged_part
                 = (bin_part_at_index(region->slot))->next) != 0) {
            if (freeform != 0) {
                si = clone_part(CARRIED);
                saved = HELD_PARTS.dragged_part;
                HELD_PARTS.dragged_part = 0;

                if (check_room_for_part() != 0) {
                    HELD_PARTS.dragged_part = saved;
                    if ((si->next = CARRIED->next) != 0)
                        si->next->prev = si;
                    si->prev = HELD_PARTS.dragged_part;
                    CARRIED->next = si;
                    HELD_PARTS.dragged_part = si;
                } else {
                    free_part(si);
                }
            }

            if (HELD_PARTS.dragged_part != 0) {
                tool = 9;
                if (CARRIED->kind == 8 || CARRIED->kind == 0x0a)
                    redraw_e = 2;
            }
        }
    }
}

/*
 * 0x10ef1
 *
 * **The playfield's cursor.** A region handler, of the same family as the five
 * at 0x34eb and after: it writes a cursor number into its region's own +0x0e,
 * which `regions_handle_pointer` reads a moment later.
 *
 * Named from the row that installs it rather than from what it does, as the
 * others here had to be. That row is
 *
 *     { 0x4e79, 0, 0x200, { 0x1000, 0, 0, 0, 0x27f, 0x16f, 0, 0x1000,
 *                           0x2f01, 0xdff, 0, 0 } }
 *
 * whose rectangle is 0..0x27f by 0..0x16f - the whole 640-wide picture down to
 * scan line 367, which is exactly where `vm_set_line_compare(0x16f)` splits
 * the screen. So the region is the play area entire, not a button in it, and
 * the cursor it asks for is the one the selected tool wants: the answer comes
 * straight from `cursor_for_tool` at 0x046d8 and is not looked at here.
 *
 * Unlike its five siblings this one has no condition of its own - they choose
 * between two cursors on a flag, and it delegates the whole question.
 */
void region_cursor_playfield(struct region *region)
{
    region->cursor = (uint16_t)cursor_for_tool();
}

/*
 * 0x10f03
 *
 * **The game screen.** State 2 dispatches here, so this is the first screen a
 * round shows - the level briefing for round 1 - and it stays here, running
 * its own loop, until something sets the state to one it does not handle.
 *
 * It paints once on the way in, through `paint_game_screen(1)`, and then loops:
 * take the button state and a key, let the regions at DGROUP 0x4e77 see the
 * pointer, and dispatch on the state at 0x4e6b through a **jump table** at
 * CS:0x34bf - eleven single-bit states, 0x0020 through 0x8000, with the
 * handlers in a second table 0x16 bytes further on.
 *
 * State 2 is *not* in that table. The search runs off the end and falls to the
 * bottom of the loop, so the screen simply sits and presents itself, which is
 * what a briefing waiting for a click is.
 *
 * **The repaint counters are counts and not flags, and that is the double
 * buffer showing through.** `si` repaints the whole screen and the three at
 * [bp-0xa], [bp-0xc] and [bp-0xe] repaint one panel piece each; every one is
 * decremented by one per pass rather than cleared, so setting a counter to two
 * paints the same thing into both pages. A flag would paint it into whichever
 * page happened to be current and leave the other stale for a frame.
 *
 * The whole-screen repaint and the piecewise ones are exclusive - `or si,si`
 * takes the first branch - so a full repaint does not also run the three.
 *
 * Alt and V together put up a version box: `key_is_down` is asked for
 * scancodes 0x38 and 0x2f, and both being down shows it and asks for a full
 * repaint afterwards.
 *
 * The eleven handlers are stubs. Each is named for the state that reaches it,
 * because that is what is known about it; what each screen *is* is not, and
 * naming them for their addresses would lose even that.
 */
void game_screen(void)
{
    int16_t  repaint_all;               /* si */
    int16_t  held;                      /* di: passes since the button went down */
    int16_t  air;                       /* [bp-2] */
    int16_t  gravity;                   /* [bp-4] */
    int16_t  reload;                    /* [bp-6]: the level wants loading again */
    int16_t  done;                      /* [bp-8]: leave the loop */
    int16_t  repaint_e;                 /* [bp-0xa] */
    int16_t  repaint_f;                 /* [bp-0xc] */
    int16_t  repaint_g;                 /* [bp-0xe] */
    int16_t  file_err;                  /* [bp-0x10]: what the last save answered */
    int32_t  t;                         /* [bp-0x14] */

    reset_machine();
    paint_game_screen(1);
    set_palette_pointer(pal_tim);
    repaint_all = repaint_e = repaint_f = repaint_g = reload = done = held = 0;
    show_cursor_again();

    while (done == 0) {
        update_button_state();

        last_key = bios_read_key() >> 8;
        if (last_key == SC_TAB)
            tab_move_pointer();

        regions_handle_pointer(regions_panel);

        if (key_is_down(SC_ALT) && key_is_down(SC_V)) {
            show_message_box(MESSAGES.version_number, MESSAGES.this_is_version);
            repaint_all = 1;
            round_state = 2;
        }

        /* Signed: the table at 0x114af sorts 0x8000 first. */
        switch ((int16_t)round_state) {
        case (int16_t)0x8000:
            paint_panel_a(1);
            present_back_page();
            round_state = 0x1000;
            done = 1;
            break;

        case 0x4000:                    /* the master level's up arrow */
            if (POINTER.button_left != 1 && POINTER.button_left != 2) {
                held = 0;
                round_state = 2;
            } else {
                if (held % 8 == 0 && master_level != 6) {
                    master_level++;
                    write_config();
                    set_master_level_ok(GAME_MASTER_LEVELS.master_level_ok[master_level]);
                }
                held++;
            }
            repaint_e = 2;
            break;

        case 0x2000:                    /* and its down arrow */
            if (POINTER.button_left != 1 && POINTER.button_left != 2) {
                held = 0;
                round_state = 2;
            } else {
                if (held % 8 == 0 && master_level != 0) {
                    master_level--;
                    write_config();
                    set_master_level_ok(GAME_MASTER_LEVELS.master_level_ok[master_level]);
                }
                held++;
            }
            repaint_e = 2;
            break;

        case 0x1000:                    /* QUIT */
            paint_panel_b(1);
            present_back_page();
            if (ask_yes_no(MESSAGES.quit_game, MESSAGES.quit_body)) {
                round_state = 1;
                done = 1;
            } else {
                round_state = 2;
                repaint_all = 1;
            }
            break;

        case 0x0800:                    /* RESTART */
            paint_panel_c(1);
            present_back_page();
            if (ask_yes_no(MESSAGES.restart_level, MESSAGES.restart_body)) {
                remove_all_parts();
                round_state = 0x1000;
                done = 1;
            } else {
                round_state = 2;
                repaint_all = 1;
            }
            break;

        case 0x0400:                    /* FREEFORM */
            if (freeform == 0) {
                paint_panel_level(1);
                present_back_page();
                if (ask_yes_no(MESSAGES.freeform_mode, MESSAGES.freeform_body)) {
                    round_teardown();
                    load_animation(WRITABLE_LITERAL("ff.lev"));
                    reset_machine();
                    freeform = 1;
                    odometer_total = 0;
                    LEVEL_SETTINGS.bonus_1 = LEVEL_SETTINGS.bonus_2 = 0;
                    start_counters();
                }
                repaint_all = 1;
            }
            round_state = 2;
            break;

        case 0x0200:                    /* the puzzle picker, or leaving freeform */
            paint_panel_d(1);
            present_back_page();
            if (freeform != 0
                && ask_yes_no(MESSAGES.leave_freeform_mode, MESSAGES.leave_freeform_body)) {
                freeform = 0;
                reload = 1;
            }
            if (freeform == 0
                && (select_puzzle_screen() != 0 || reload != 0)) {
                round_teardown();
                load_level(round_number);
                reset_machine();
                reload = 0;
                start_counters();
            }
            repaint_all = 1;
            round_state = 2;
            break;

        case 0x0100:                    /* LOAD */
            if (freeform != 0) {
                paint_panel_free_a(1);
                present_back_page();

                file_op_active = 1;
                if (dos_chdir((char *)GAME_DIRECTORIES.picker_dir) == 0)
                    dos_setdisk(GAME_DIRECTORIES.picker_dir[0]);
                file_op_active = 0;

                if (pick_file(0, 0, "*.TIM")) {
                    round_teardown();
                    load_animation((char *)PICKED_MACHINE.name);
                    reset_machine();
                }

                file_op_active = 1;
                dos_get_cur_dir((char *)GAME_DIRECTORIES.picker_dir);
                if (dos_chdir((char *)GAME_DIRECTORIES.game_dir) == 0)
                    dos_setdisk(GAME_DIRECTORIES.game_dir[0]);
                file_op_active = 0;

                repaint_all = 1;
            }
            round_state = 2;
            break;

        case 0x0080:                    /* SAVE */
            if (freeform != 0) {
                paint_panel_free_b(1);
                present_back_page();

                file_op_active = 1;
                if (dos_chdir((char *)GAME_DIRECTORIES.picker_dir) == 0)
                    dos_setdisk(GAME_DIRECTORIES.picker_dir[0]);
                file_op_active = 0;

                file_err = 1;
                while (file_err != 0) {
                    round_state = 0x80;
                    if (pick_file(0, 0, "*.TIM")) {
                        file_err = save_machine((char *)PICKED_MACHINE.name);
                        if (file_err != 0) {
                            show_message_box(MESSAGES.file_error, MESSAGES.disk_write_protected);
                            paint_game_screen(0);
                        }
                    } else {
                        file_err = 0;
                    }
                }

                dos_get_cur_dir((char *)GAME_DIRECTORIES.picker_dir);
                file_op_active = 1;
                if (dos_chdir((char *)GAME_DIRECTORIES.game_dir) == 0)
                    dos_setdisk(GAME_DIRECTORIES.game_dir[0]);
                file_op_active = 0;

                repaint_all = 1;
            }
            round_state = 2;
            break;

        case 0x0040:                    /* the gravity slider, which sets 0x50b5 */
            if (freeform == 0) {
                show_message_box(MESSAGES.cant_change_gravity, MESSAGES.gravity_body);
                repaint_all = 1;
                round_state = 2;
            } else if (POINTER.button_left != 1 && POINTER.button_left != 2) {
                round_state = 2;
            } else {
                t = mul16x16(POINTER.pointer_x - 0x43, 0x200);
                air = t / 0xa0;
                if (air < 0)
                    air = 0;
                else if (air > 0x200)
                    air = 0x200;
                if (air != LEVEL_SETTINGS.air) {
                    LEVEL_SETTINGS.air = air;
                    repaint_f = 2;
                    recompute_kind_physics();
                }
            }
            break;

        case 0x0020:                    /* the air slider, which sets 0x50b3 */
            if (freeform == 0) {
                show_message_box(MESSAGES.cant_change_air_pressure, MESSAGES.air_pressure_body);
                repaint_all = 1;
                round_state = 2;
            } else if (POINTER.button_left != 1 && POINTER.button_left != 2) {
                round_state = 2;
            } else {
                t = mul16x16(POINTER.pointer_x - 0x43, 0x80);
                gravity = t / 0xa0;
                if (gravity < 0)
                    gravity = 0;
                else if (gravity > 0x80)
                    gravity = 0x80;
                if (gravity != LEVEL_SETTINGS.gravity) {
                    LEVEL_SETTINGS.gravity = gravity;
                    repaint_g = 2;
                    recompute_kind_physics();
                }
            }
            break;
        }

        if (repaint_all != 0) {
            paint_game_screen(1);
            repaint_all--;
        } else {
            if (repaint_e != 0) {
                paint_panel_e();
                repaint_e--;
            }
            if (repaint_f != 0) {
                paint_panel_f();
                repaint_f--;
            }
            if (repaint_g != 0) {
                paint_panel_g();
                repaint_g--;
            }
        }

        present_frame(1);
    }
}

/*
 * 0x114db
 *
 * **The enter-freeform region's handler**, and the first of five that are the
 * same eleven instructions with one constant changed: write a cursor number
 * into the region's own +0x0e, which `regions_handle_pointer` selects a moment
 * later.
 *
 * They are far pointers in the region table rather than a field because the
 * answer depends on the mode - freeform or not, DGROUP 0x4e67 - and a table
 * cannot hold a condition.
 *
 * **This one is the other way round from the four below**, and that is the
 * point: it has a cursor *outside* freeform and none inside it, because outside
 * is when the button does something. The other four have a cursor inside
 * freeform and none outside, for the same reason - Load, Save and the two
 * sliders only work there.
 *
 * The region it belongs to is the one whose +0x10 is 0x400, which is
 * `game_screen`'s case 0x400. It was called `restart` here at first, from the state
 * number rather than the table; the restart region is the one with 0x800, and
 * it has **no** handler at all.
 *
 * All five regions stay clickable either way: the mode a region switches to is
 * at +0x10 and none of these touches it.
 */
void region_cursor_freeform(struct region *region)
{
    if (freeform != 0)
        region->cursor = 0;
    else
        region->cursor = 0x14;
}

/*
 * 0x114f8
 *
 * Load Machine's, and `region_cursor_freeform`'s twin the other way round: a
 * cursor in freeform, nothing outside it.
 */
void region_cursor_load(struct region *region)
{
    if (freeform != 0)
        region->cursor = 0x17;
    else
        region->cursor = 0;
}

/*
 * 0x11515
 *
 * Save Machine's.
 */
void region_cursor_save(struct region *region)
{
    if (freeform != 0)
        region->cursor = 0x16;
    else
        region->cursor = 0;
}

/*
 * 0x11532
 *
 * The gravity slider's.
 */
void region_cursor_gravity(struct region *region)
{
    if (freeform != 0)
        region->cursor = 0x18;
    else
        region->cursor = 0;
}

/*
 * 0x1154f
 *
 * The air-pressure slider's.
 */
void region_cursor_air(struct region *region)
{
    if (freeform != 0)
        region->cursor = 0x19;
    else
        region->cursor = 0;
}

/*
 * 0x1156c
 *
 * **Tab moves the mouse pointer**, not a focus ring. There is no keyboard
 * selection in this panel at all: Tab advances a cursor at DGROUP 0x27ee and
 * then *warps the pointer* to where that control is, so everything downstream -
 * the region test, the handlers this file is full of - carries on believing the
 * player moved the mouse there.
 *
 * The stop it lands on comes from a pair of tables in DGROUP, x at **0x27f0**
 * and y at **0x2802**. They are not the same length: x runs out after nine
 * entries, exactly where y's tenth begins, because stops 9 and 0x0a are the two
 * sliders and their x is *computed* from the value the slider is showing. Each
 * is the slider handler's own arithmetic run backwards - `* 0xa0 / 0x200 + 67`
 * against the handler's `(x - 67) * 0x200 / 0xa0` - so the pointer lands on the
 * knob where it already is and the next Tab does not move it.
 *
 * Which stops exist depends on the mode: in freeform (DGROUP 0x4e67) the cycle
 * is 0..0x0a with 5 skipped, and outside it the cycle stops at 7 and wraps.
 * Both wrap checks run, because the increment can arrive at 0x0b from either.
 */
void tab_move_pointer(void)
{
    int16_t  si;                        /* the stop's x */
    int32_t  t;                         /* [bp-4] */

    GAME_PLAY_TABS.stop++;

    if (freeform != 0) {
        if (GAME_PLAY_TABS.stop == 5)
            GAME_PLAY_TABS.stop = 6;
    } else if (GAME_PLAY_TABS.stop == 7) {
        GAME_PLAY_TABS.stop = 0;
    }

    if (GAME_PLAY_TABS.stop == 0x0b)
        GAME_PLAY_TABS.stop = 0;

    if (GAME_PLAY_TABS.stop == 9) {
        t = mul16x16(LEVEL_SETTINGS.air, 0xa0);
        si = t / 0x200;
        si += 0x43;
    } else if (GAME_PLAY_TABS.stop == 0x0a) {
        t = mul16x16(LEVEL_SETTINGS.gravity, 0xa0);
        si = t / 0x80;
        si += 0x43;
    } else {
        si = GAME_PLAY_TABS.stop_x[GAME_PLAY_TABS.stop];
    }

    move_pointer_to(si, GAME_PLAY_TABS.stop_y[GAME_PLAY_TABS.stop]);
}

/*
 * 0x11632
 *
 * **Paint the game screen**: the play area, the control panel down the left,
 * and the three ornaments that sit on it.
 *
 * The order is the order the pieces overlap in. The play area is cleared to
 * the colour at DGROUP 0x52cb - `fill_rect(8, 8, 0x230, 0x160)`, inside the
 * clip box `set_clip_play_area` just set - then the machine is drawn over it,
 * then the panel at `draw_panel(0x2c, 0x42, 0xd0, 0x109)` and its contents.
 *
 * The panel's contents are eleven separate painters, each of which takes a
 * flag this passes as zero, and the flag is presumably "redraw only". Four
 * always run; then the fork on 0x4e67 - the same word `round_setup` uses to
 * tell free play from a level - chooses **two** painters for free play and
 * **one** for a level. That is the control panel having a different set of
 * controls in the two modes.
 *
 * The three bitmaps at the end come from the set at 0x52f4, at +6, +0xa and
 * +8, placed at (0x53,0x42), (0x64,0xb2) and (0x5b,0xfe) - note the middle one
 * is +0xa and the last +8, which is not the order they are drawn in.
 *
 * `select_music` is given the level's own tune from 0x50bb, which
 * `read_level` filled in.
 *
 * The argument decides whether the finished screen is presented: non-zero
 * calls 0x081f9. So a caller can paint into the back page and show it, or
 * paint and leave it for something else to show.
 */
void paint_game_screen(uint16_t present)
{
    wait_cursor();
    set_clip_play_area();

    VMDS.page_dst = VMDS.page_back;
    VMDS.second_colour = VMDS.fill_colour = (uint8_t)fill_colour;
    VMDS.fill_enabled = 1;

    cursor_redraw_off_thunk();
    fill_rect(8, 8, 0x230, 0x160);

    draw_machine_thunk();
    paint_panel_frame();

    draw_panel(0x2c, 0x42, 0xd0, 0x109);

    paint_panel_a(0);
    paint_panel_b(0);
    paint_panel_c(0);
    paint_panel_d(0);

    if (freeform != 0) {
        paint_panel_free_a(0);
        paint_panel_free_b(0);
    } else {
        paint_panel_level(0);
    }

    paint_panel_e();
    paint_panel_f();
    paint_panel_g();

    cursor_redraw_off_thunk();
    draw_bitmap(panel_art[3], 0x53, 0x42, 0);
    draw_bitmap(panel_art[5], 0x64, 0xb2, 0);
    draw_bitmap(panel_art[4], 0x5b, 0xfe, 0);
    restore_cursor_following();

    select_music(LEVEL_SETTINGS.tune);

    if (present != 0)
        present_back_page();

    restore_cursor();
}

/*
 * 0x1175c
 *
 * **Draw the machine's parts into the play area**, which is the last thing the
 * title bar's painter does and the thing that puts the level's contents on the
 * screen.
 *
 * The scale comes from the level's own origin: the *larger* of 0x50b7 and
 * 0x50b9 plus 0x230, divided into 4 as a 32-bit division. Both are -8 for a
 * fresh level, so the divisor is 0x228 and the result is 0 - but the code
 * takes the larger and divides, and a level with a different origin would get
 * a different answer.
 *
 * Then every part in the buckets is linked in and drawn: `pick_by_flag` with
 * 0x3000 answers the first, `pick_for_record` with 0x1000 walks on from it,
 * and each is passed to `link_record_into_buckets` on the way. The loop ends
 * when the walk answers zero - and it is a `while` whose test is the *result*
 * of the walk, so a machine with no parts draws nothing and does not fault.
 *
 * `draw_machine` is then given the scale and 0x200, and the clip is put back
 * to the play area.
 *
 * **The scale is 0x40000 divided by the extent**, which is 1024 units per
 * pixel over a 256-pixel panel - not the extent divided by 4. The two long
 * arguments at 0x1179a are pushed the way Borland pushes a long, high word
 * first, so `mov ax, 4 / xor dx, dx / push ax / push dx` puts 0x0004_0000 on
 * the stack and it is the *dividend*. Reading it as a divisor of 4 gave 138
 * where the original gives 474, and a machine drawn at a third of its size
 * scaled every part's position off the panel - which is how it was caught: the
 * blitter's row buffer overran into DGROUP 0x124 and the part walk there never
 * terminated.
 *
 * The two locals stepped by two - 0x100 and 0xa0 becoming 0x102 and 0xa2 - are
 * computed and never read. Transcribed as the dead stores they are.
 */
void paint_panel_frame_rest(void)
{
    struct part *si;
    int16_t  di;                        /* set and stepped, and never read */
    int16_t  extent;                    /* [bp-2] */
    int16_t  scale;                     /* [bp-4] */
    int16_t  y;                         /* [bp-6] the same */

    VMDS.clip_enabled = 1;
    set_clip_for_mode();

    di = 0x100;
    y = 0xa0;
    extent = ((LEVEL_SETTINGS.extent_y > LEVEL_SETTINGS.extent_x) ? LEVEL_SETTINGS.extent_y : LEVEL_SETTINGS.extent_x) + 0x230;
    scale = 0x40000L / extent;
    di += 2;
    y += 2;
    (void)di;
    (void)y;

    VMDS.page_dst = VMDS.page_back;

    si = pick_by_flag(0x3000);
    while (si != NULL) {
        link_record_into_buckets(si);
        si = pick_for_record(si, 0x1000);
    }

    draw_machine(scale, 0x200);

    set_clip_play_area();
}

/*
 * 0x117ed
 *
 * **The title bar and the hint box** - the two pieces of text across the top
 * of the game screen, and the first thing `paint_game_screen` draws over the
 * cleared play area.
 *
 * The title is built in a 0x80-byte buffer and depends on the mode at DGROUP
 * 0x4e67. Free play gets "FREEFORM MODE" and nothing else. A level gets
 * "PUZZLE ", the round number from 0x4ebd, the separator at 0x2837, and then
 * the level's own title from **0x4ecf** - which `read_level` filled in from the
 * file. So "PUZZLE 1: TUTORIAL: PUT THE BALL IN THE HOOP" is three pieces from
 * three places, and only the middle one is a number.
 *
 * Then the drawing: a bar at (0x20, 0x20) 0x220 by 0x158 through 0x14de:0x000c,
 * a filled area at (0x110, 0x48) in the colour at 0x52cb, the title centred on
 * a scroll at (0x3c, 0x27) 0x1bc wide, and a panel at (0x110, 0xff).
 *
 * The hint below it comes from the same fork: free play gets the fixed string
 * at 0x22c0 about creating any machine you wish, and a level gets **0x4f1f**,
 * the hint `read_level` read out of the file - which for level one is "Make the
 * basketball go through the hoop." Both are drawn into the same box at
 * (0x114, 0x104) 0xf8 by 0x44, so the two paths differ only in the string.
 */
void paint_panel_frame(void)
{
    char title[120];
    char digits[8];

    if (freeform != 0) {
        strcpy(title, MESSAGES.freeform_mode_title);
    } else {
        strcpy(title, MESSAGES.puzzle_prefix);
        itoa(round_number, digits, 10);
        strcat(title, digits);
        strcat(title, ": ");
        strcat(title, (const char *)level_title);
    }

    set_clip_play_area();
    VMDS.page_dst = VMDS.page_back;

    draw_title_bar(0x20, 0x20, 0x220, 0x158, 1);
    fill_panel_area(0x110, 0x48, 0x100, 0xa0, ((uint16_t)fill_colour));

    draw_scroll_text(title, 0x3c, 0x27, 0x1bc);
    draw_panel(0x110, 0xff, 0x100, 0x4c);

    if (freeform != 0)
        draw_wrapped_text((char *)MESSAGES.freeform_hint, 0x114, 0x104, 0xf8, 0x44);
    else
        draw_wrapped_text((char *)level_hint, 0x114, 0x104, 0xf8, 0x44);

    paint_panel_frame_rest();
}

/*
 * 0x1190d
 *
 * Paint one of the control panel's four fixed decorations: bitmap
 * `list[0x20 / 2 + frame]` out of the list at DGROUP 0x52f4, at 0x3a,0x5b.
 *
 * Four routines with one body between them - the same six instructions with a
 * different position and a different entry in the list - so they are
 * transcribed as four rather than folded into one taking three arguments. The
 * original has four, and which one a caller uses is part of what the caller
 * says.
 *
 * `frame` is doubled and used as a word index, so it selects among consecutive
 * entries rather than naming a panel: `paint_game_screen` passes 0.
 */
void paint_panel_a(uint16_t frame)
{
    VMDS.page_dst = VMDS.page_back;

    cursor_redraw_off_thunk();
    draw_bitmap(((panel_art + 0x10)[frame]),
                0x3a, 0x5b, 0);
    restore_cursor_following();
}

/*
 * 0x11943
 *
 * Paint one of the control panel's four fixed decorations: bitmap
 * `list[0x24 / 2 + frame]` out of the list at DGROUP 0x52f4, at 0xd8,0x60.
 *
 * Four routines with one body between them - the same six instructions with a
 * different position and a different entry in the list - so they are
 * transcribed as four rather than folded into one taking three arguments. The
 * original has four, and which one a caller uses is part of what the caller
 * says.
 *
 * `frame` is doubled and used as a word index, so it selects among consecutive
 * entries rather than naming a panel: `paint_game_screen` passes 0.
 */
void paint_panel_b(uint16_t frame)
{
    VMDS.page_dst = VMDS.page_back;

    cursor_redraw_off_thunk();
    draw_bitmap(((panel_art + 0x12)[frame]),
                0xd8, 0x60, 0);
    restore_cursor_following();
}

/*
 * 0x11979
 *
 * Paint one of the control panel's four fixed decorations: bitmap
 * `list[0x3e / 2 + frame]` out of the list at DGROUP 0x52f4, at 0xbc,0x5c.
 *
 * Four routines with one body between them - the same six instructions with a
 * different position and a different entry in the list - so they are
 * transcribed as four rather than folded into one taking three arguments. The
 * original has four, and which one a caller uses is part of what the caller
 * says.
 *
 * `frame` is doubled and used as a word index, so it selects among consecutive
 * entries rather than naming a panel: `paint_game_screen` passes 0.
 */
void paint_panel_c(uint16_t frame)
{
    VMDS.page_dst = VMDS.page_back;

    cursor_redraw_off_thunk();
    draw_bitmap(((panel_art + 0x1f)[frame]),
                0xbc, 0x5c, 0);
    restore_cursor_following();
}

/*
 * 0x119af
 *
 * Paint one of the control panel's four fixed decorations: bitmap
 * `list[0x52 / 2 + frame]` out of the list at DGROUP 0x52f4, at 0x6d,0x85.
 *
 * Four routines with one body between them - the same six instructions with a
 * different position and a different entry in the list - so they are
 * transcribed as four rather than folded into one taking three arguments. The
 * original has four, and which one a caller uses is part of what the caller
 * says.
 *
 * `frame` is doubled and used as a word index, so it selects among consecutive
 * entries rather than naming a panel: `paint_game_screen` passes 0.
 */
void paint_panel_d(uint16_t frame)
{
    VMDS.page_dst = VMDS.page_back;

    cursor_redraw_off_thunk();
    draw_bitmap(((panel_art + 0x29)[frame]),
                0x6d, 0x85, 0);
    restore_cursor_following();
}

/*
 * 0x119e5
 *
 * Paint one of the free-play panel's pairs: bitmap `list[0x42 / 2 + frame]`
 * at 0x96,0x8c and then `list[0x3a / 2 + frame]` at 0xa6,0x8b, both
 * out of the list at DGROUP 0x52f4.
 *
 * Two bitmaps between one `cursor_redraw_off_thunk` and one
 * `restore_cursor_following`, not two of each - the cursor is lifted once and
 * put back once, so the second bitmap cannot land on a restored cursor.
 */
void paint_panel_free_a(uint16_t frame)
{
    VMDS.page_dst = VMDS.page_back;

    cursor_redraw_off_thunk();
    draw_bitmap(((panel_art + 0x21)[frame]),
                0x96, 0x8c, 0);
    draw_bitmap(((panel_art + 0x1d)[frame]),
                0xa6, 0x8b, 0);
    restore_cursor_following();
}

/*
 * 0x11a3f
 *
 * Paint one of the free-play panel's pairs: bitmap `list[0x46 / 2 + frame]`
 * at 0xc8,0x8c and then `list[0x3a / 2 + frame]` at 0xd8,0x8b, both
 * out of the list at DGROUP 0x52f4.
 *
 * Two bitmaps between one `cursor_redraw_off_thunk` and one
 * `restore_cursor_following`, not two of each - the cursor is lifted once and
 * put back once, so the second bitmap cannot land on a restored cursor.
 */
void paint_panel_free_b(uint16_t frame)
{
    VMDS.page_dst = VMDS.page_back;

    cursor_redraw_off_thunk();
    draw_bitmap(((panel_art + 0x23)[frame]),
                0xc8, 0x8c, 0);
    draw_bitmap(((panel_art + 0x1d)[frame]),
                0xd8, 0x8b, 0);
    restore_cursor_following();
}

/*
 * 0x11a99
 *
 * Paint the level indicator: bitmap `list[0x36 / 2 + frame]` out of the
 * list at DGROUP 0x52f4, at 0x39,0x86. The same six instructions as
 * `paint_panel_a`; see its comment for the shape.
 */
void paint_panel_level(uint16_t frame)
{
    VMDS.page_dst = VMDS.page_back;

    cursor_redraw_off_thunk();
    draw_bitmap(((panel_art + 0x1b)[frame]),
                0x39, 0x86, 0);
    restore_cursor_following();
}

/*
 * 0x11acf
 *
 * The panel's tiled background and the row of indicators over it, all out of
 * the bitmap list at DGROUP 0x52f4.
 *
 * The background is one bitmap - `list[0x56 / 2]` - stamped on an eight-pixel
 * grid from x 0x84 to 0xb4 and y 0x5f to 0x77. The two bounds are tested
 * differently: `cmp si, 0xb4 / jl` stops before 0xb4 and `cmp di, 0x77 / jle`
 * includes 0x77, so the grid is six columns by four rows and not five by four.
 *
 * Two of the indicators depend on what the round is: DGROUP 0x4e6b holding
 * 0x4000 picks entry 0x26 over 0x25, and 0x2000 picks 0x28 over 0x27.
 *
 * The last loop draws one bitmap per part in the level, `list[0x28 / 2 + si]`,
 * at the x in the word table at DGROUP 0x2816 - which is indexed from 1, so
 * its first word is not an x - and at a y that starts at 0x69 and steps *down*
 * by two each time, so the row leans.
 */
void paint_panel_e(void)
{
    int16_t si, di;
    int16_t y;                          /* [bp-2] */
    int16_t up;                         /* [bp-4] */
    int16_t down;                       /* [bp-6] */

    if (round_state == 0x4000)
        up = 0x26;
    else
        up = 0x25;
    if (round_state == 0x2000)
        down = 0x28;
    else
        down = 0x27;

    VMDS.page_dst = VMDS.page_back;
    cursor_redraw_off_thunk();

    for (si = 0x84; si < 0xb4; si += 8)
        for (di = 0x5f; di <= 0x77; di += 8)
            draw_bitmap(panel_art[0x2b], si, di, 0);

    draw_bitmap(panel_art[up],   0x58, 0x5d, 0);
    draw_bitmap(panel_art[down], 0x58, 0x6f, 0);
    draw_bitmap(panel_art[0x14], 0x6e, 0x60, 0);

    for (si = 1, y = 0x69; si <= master_level; si++, y -= 2)
        draw_bitmap(((panel_art + 0x14)[si]),
                    GAME_MASTER_LEVEL_X.level_x[si - 1], y, 0);

    restore_cursor_following();
}

/*
 * 0x11bd6
 *
 * A slider on the control panel: its track, its scale, and a knob whose
 * position comes from DGROUP 0x50b5.
 *
 * The knob's x is `0x50b5 * 0xa0 / 0x200 + 0x3d`, worked out as a long -
 * `mul16x16` then the runtime's long division - because the product overflows a word before
 * the divide brings it back. The two sliders differ in that divisor, 0x200
 * against 0x80, so they are not the same slider at two positions.
 */
void paint_panel_f(void)
{
    int16_t at;                         /* [bp-2] */
    int32_t t;                          /* [bp-6] */

    VMDS.page_dst = VMDS.page_back;
    cursor_redraw_off_thunk();

    draw_bitmap(panel_art[0x7], 0x41, 0xc8, 0);
    draw_bitmap(panel_art[0x9], 0x3d, 0xe5, 0);

    t = mul16x16(LEVEL_SETTINGS.air, 0xa0);
    at = t / 0x200;

    draw_bitmap(panel_art[0x6],
                at + 0x3d, 0xe0, 0);

    restore_cursor_following();
}

/*
 * 0x11c6b
 *
 * A slider on the control panel: its track, its scale, and a knob whose
 * position comes from DGROUP 0x50b3.
 *
 * The knob's x is `0x50b3 * 0xa0 / 0x80 + 0x3d`, worked out as a long -
 * `mul16x16` then the runtime's long division - because the product overflows a word before
 * the divide brings it back. The two sliders differ in that divisor, 0x80
 * against 0x200, so they are not the same slider at two positions.
 */
void paint_panel_g(void)
{
    int16_t at;                         /* [bp-2] */
    int32_t t;                          /* [bp-6] */

    VMDS.page_dst = VMDS.page_back;
    cursor_redraw_off_thunk();

    draw_bitmap(panel_art[0x8], 0x41, 0x114, 0);
    draw_bitmap(panel_art[0x9], 0x3d, 0x131, 0);

    t = mul16x16(LEVEL_SETTINGS.gravity, 0xa0);
    at = t / 0x80;

    draw_bitmap(panel_art[0x6],
                at + 0x3d, 0x12c, 0);

    restore_cursor_following();
}
