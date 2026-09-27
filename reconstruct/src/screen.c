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
 * JUDGE: compiler 3.00
 * JUDGE: built-with -mm -d
 * JUDGE: data 0x2630..0x283a
 *
 * **Not byte-exact yet, and not only for want of reading.** Thirteen of the
 * port's routines here are not functions in the original: the ten
 * `screen_state_*` are the cases of `game_screen`'s switch, sharing its
 * registers and jumping back into its loop; `carried_part_grow` and
 * `carried_part_shrink` are two arms of `part_key_shortcut`; and
 * `region_cursor_bin_above` begins inside its neighbour. They go back into
 * the routines they are part of before the rest can be matched.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **The game screen's own words and the goal tests**, DGROUP 0x2630..0x27ee -
 * the first of this module's data.
 */
struct dg_2630 DG2630 DGROUP_WAS(0x2630) = {
    0, 0, 0,
    {
        goal_test_puzzle_1,
        goal_test_puzzle_2,
        goal_test_puzzles_3_8_27_30_33_45_50_62,
        goal_test_puzzle_4,
        goal_test_puzzle_5,
        goal_test_puzzles_6_58,
        goal_test_puzzles_7_51_65,
        goal_test_puzzles_3_8_27_30_33_45_50_62,
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
        goal_test_puzzles_3_8_27_30_33_45_50_62,
        goal_test_puzzle_28,
        goal_test_puzzle_29,
        goal_test_puzzles_3_8_27_30_33_45_50_62,
        goal_test_puzzle_31,
        goal_test_puzzles_10_32,
        goal_test_puzzles_3_8_27_30_33_45_50_62,
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
        goal_test_puzzles_3_8_27_30_33_45_50_62,
        goal_test_puzzle_46,
        goal_test_puzzle_47,
        goal_test_puzzles_19_48,
        goal_test_puzzle_49,
        goal_test_puzzles_3_8_27_30_33_45_50_62,
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
        goal_test_puzzles_3_8_27_30_33_45_50_62,
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

struct game_play_tabs GAME_PLAY_TABS DGROUP_AT(0x27ee) = {
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

struct game_master_level_x GAME_MASTER_LEVEL_X DGROUP_AT(0x2818) = {
    { 0x0085, 0x0088, 0x008e, 0x0094, 0x009b, 0x00a3 }, /* level_x */
};

/*
 * **The module's literal pool**, DGROUP 0x2824..0x283a. What follows is the
 * next module's: `finish_level`'s two buttons.
 */
struct game_level_strings GAME_LEVEL_STRINGS DGROUP_AT(0x2824) = {
    "ff.lev", /* ff_lev */
    "*.TIM", /* tim_filter_load */
    "*.TIM", /* tim_filter_save */
    ": ", /* title_sep */
};

/*
 * The two helpers below are the host's alone. Under the original compiler a
 * function here would be code in the module and move every routine after
 * it; the routines that call them say what the original does instead.
 */
#ifndef __TURBOC__
/*
 * OURS: what both resize arms do once they have decided which way to go.
 *
 * The original writes these four calls out twice in each arm - once for the
 * width and once for the height - so four copies in all, identical but for the
 * field they follow. Factored here because the *decision* above it is the part
 * that differs, and that is left written out.
 */
static void carried_part_resized(struct part *part, struct part_kind *kind)
{
    kind->settle(part);
    place_object_for_draw(part);
    mark_needs_refile(part, 2);
    mark_joined_shapes(part, 3);
}

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
    int16_t si = 0;
    struct part *part;

    reset_level_state();
    TIMER.frame_budget = 0x2710;

    while (DG4E67.state != 0x2000 && DG4E67.state != 2) {
        DG52BD.band_colour = 0xffff;
        DG52BD.drop_cursor = 0xffff;

        DG52ED.last_key = (uint8_t)(bios_read_key() >> 8);

        update_button_state();
        scroll_play_area();
        step_counters();

        if (DG4E67.freeform != 0)
            select_music_by_key();

        regions_handle_pointer(DG4E67.regions_play_ptr);

        if (DG4E67.state == 0x800)
            bin_scroll_back();
        else if (DG4E67.state == 0x400)
            bin_scroll_forward();

        if (point_in_play_area() != 0) {
            pointer_frame();
            si = 0;
        } else {
            if (DG50D3.dragged_part_ptr != 0 && si == 0) {
                mark_joined_shapes(PART_PTR(DG50D3.dragged_part_ptr), 3);
                mark_part_shapes(PART_PTR(DG50D3.dragged_part_ptr), 3);
            }
            edge_scroll_flags();
            si = 1;
        }

        if (DG4E67.redraw_e != 0) { draw_machine_layer_a(); DG4E67.redraw_e--; }
        if (DG4E67.redraw_d != 0) { draw_machine_layer_b(); DG4E67.redraw_d--; }
        if (DG4E67.redraw_c != 0) { draw_machine_layer_c(); DG4E67.redraw_c--; }
        if (DG4E67.redraw_b != 0) { draw_machine_layer_d(); DG4E67.redraw_b--; }
        if (DG4E67.redraw_a != 0) { draw_machine_layer_e(); DG4E67.redraw_a--; }

        mark_parts_in_dirty_rects();
        replay_shapes();
        step_and_draw_machine(0);

        if (DG50D3.dragged_part_ptr != 0 && DG52BD.drop_cursor != -1)
            draw_part_selection(PART_PTR(DG50D3.dragged_part_ptr), ((uint16_t)DG52BD.drop_cursor), 1);

        if (DG52BD.band_colour != -1) {
            cursor_redraw_off_thunk();
            VMDS.second_colour = ((uint8_t)DG52BD.band_colour);
            clip_and_draw_line(
                (int16_t)(((uint16_t)DG52BD.anchor_x) - ((uint16_t)DG4E67.origin_x)),
                (int16_t)(((uint16_t)DG52BD.anchor_y) - ((uint16_t)DG4E67.origin_y)),
                (int16_t)(((uint16_t)DG52BD.band_x) - ((uint16_t)DG4E67.origin_x)),
                (int16_t)(((uint16_t)DG52BD.band_y) - ((uint16_t)DG4E67.origin_y)));
            restore_cursor_following();
            alloc_shape((const uint8_t *)&DG52BD.anchor_x,
                        (const uint8_t *)&DG52BD.band_x,
                        4, 2, 0);
        }

        if (DG4E67.redraw_carried != 0) { draw_carried_icon(); DG4E67.redraw_carried--; }

        seg172c_nothing();

        while ((int16_t)(0x2710 - ((uint16_t)TIMER.frame_budget)) < 8)
            ;
        TIMER.frame_budget = 0x2710;

        present_frame(1);
        shift_all_histories();

        if (DG5768.button_right == 2)
            DG4E67.state = 2;
    }

    part = PART_PTR(DG50D3.dragged_part_ptr);
    if (part == PART_NONE || (part->flags_06 & 0x800) == 0)
        return;

    if (part->kind == KIND_BELT
        && ROPE_PTR(part->rope_ptr)->end_a_ptr != 0) {
        discard_carried_part();
        return;
    }

    if (part->kind == KIND_ROPE
        && ((uint16_t)BELT_PTR(part->belt_ptr[0])->end_a_ptr) != 0) {
        discard_carried_part();
        return;
    }

    finish_part_removal();
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
    static const struct { uint8_t key; int16_t tune; } TUNES[] = {
        {  2, 0x3e9 }, {  3, 0x3ea }, {  4, 0x3eb }, {  5, 0x3ec },
        {  6, 0x3ed }, {  7, 0x3ee }, {  8, 0x3ef }, {  9, 0x3f0 },
        { 10, 0x3f1 }, { 30, 0x3f2 }, { 48, 0x3f3 }, { 46, 0x3f4 },
        { 32, 0x3f5 }, { 18, 0x3f6 }, { 33, 0x3f7 }, { 34, 0x3f8 },
    };
    uint16_t key = (DG52ED.last_key);
    int16_t si = -1;
    uint16_t i;

    if ((uint16_t)(key - 2) <= 0x2e) {
        for (i = 0; i < sizeof TUNES / sizeof TUNES[0]; i++)
            if (TUNES[i].key == key) {
                si = TUNES[i].tune;
                break;
            }
    }

    if (si == -1)
        return;

    DG50AF.tune = si;
    select_music(DG50AF.tune);
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
    DG4E67.tool = 0;
    DG4E67.loop_frames = 0;
    DG4E67.redraw_carried = 0;
    DG4E67.redraw_a = 0;
    DG4E67.redraw_b = 0;
    DG4E67.redraw_c = 0;
    DG4E67.redraw_d = 0;
    DG4E67.redraw_e = 0;
    DG50D3.dragged_part_ptr = 0;

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
    uint16_t si;

    si = (DG4E67.tool == 9 || (DG4E67.tool & 0x8000)) ? 1 : 0;

    if (si == 0) {
        DG50D3.dragged_part_ptr = dg_near(dgroup, find_part_from(PART_PTR(DG50D3.dragged_part_ptr)));
        if (DG50D3.dragged_part_ptr != 0
            && (PART_PTR(DG50D3.dragged_part_ptr)->flags_06 & 0x8000))
            DG50D3.dragged_part_ptr = 0;
    }

    if (DG50D3.dragged_part_ptr == 0) {
        DG4E67.tool = 0;
        return;
    }

    if (DG4E67.tool != 9)
        DG52BD.drop_cursor = 0x0a;

    if (si == 0)
        DG4E67.tool = part_handle_at_pointer(PART_PTR(DG50D3.dragged_part_ptr));

    switch ((uint16_t)((DG4E67.tool & 0x7fff) - 1)) {
    case 0:                                     /* tool 1 */
        if (DG5768.button_left == 2)
            flip_carried_end_1();
        return;
    case 1:                                     /* tool 2 */
        if (DG5768.button_left == 2)
            flip_carried_end_2();
        return;
    case 2: case 3: case 4: case 5:             /* tools 3 to 6 */
        run_drag_frame();
        return;
    case 6:                                     /* tool 7 */
        if (DG5768.button_left == 2)
            pick_up_part();
        return;
    case 7:                                     /* tool 8 */
        if (DG5768.button_left == 2)
            discard_carried_part();
        return;
    case 8:                                     /* tool 9 */
        move_carried();
        return;
    case 9:                                     /* tool 10 */
        if (DG5768.button_left == 2)
            DG50D3.dragged_part_ptr = 0;
        return;
    default:
        return;
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
    uint16_t kind;

    if (DG4E67.tool != 9 || DG50D3.dragged_part_ptr == 0)
        return;

    kind = PART_PTR(DG50D3.dragged_part_ptr)->kind;
    if (kind == 8 || kind == 0x0a)
        return;

    DG4E67.redraw_carried = 1;

    if (DG5768.pointer_y < 8)
        DG4E67.redraw_d = 3;
    if (DG5768.pointer_y > 0x12f)
        DG4E67.redraw_c = 3;
    if (DG5768.pointer_x < 8)
        DG4E67.redraw_b = 3;
    if (DG5768.pointer_x > 0x1ff) {
        DG4E67.redraw_e = 3;
        DG4E67.redraw_a = 3;
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
    uint16_t di, y, moved = 0;
    struct part *si;

    DG4E67.origin_c_x = ((uint16_t)DG4E67.origin_b_x);
    DG4E67.origin_c_y = ((uint16_t)DG4E67.origin_b_y);
    DG4E67.origin_b_x = ((uint16_t)DG4E67.origin_x);
    DG4E67.origin_b_y = ((uint16_t)DG4E67.origin_y);

    di = ((uint16_t)DG4E67.origin_x);
    y = ((uint16_t)DG4E67.origin_y);

    if ((int16_t)((uint16_t)DG5768.pointer_x) <= 0 && DG4E67.origin_x != -8) {
        moved = 1;
        di = (uint16_t)(di - 0x10);
    }
    if ((int16_t)((uint16_t)DG5768.pointer_x) >= 0x27f && ((uint16_t)DG4E67.origin_x) != ((uint16_t)DG50AF.extent_y)) {
        moved = 1;
        di = (uint16_t)(di + 0x10);
    }
    if ((int16_t)((uint16_t)DG5768.pointer_y) <= 0 && DG4E67.origin_y != -8) {
        moved = 1;
        y = (uint16_t)(y - 0x10);
    }
    if ((int16_t)((uint16_t)DG5768.pointer_y) >= 0x16f && ((uint16_t)DG4E67.origin_y) != ((uint16_t)DG50AF.extent_x)) {
        moved = 1;
        y = (uint16_t)(y + 0x10);
    }

    if (moved == 0)
        return;

    si = pick_by_flag(0x3000);
    while (si != PART_NONE) {
        if ((si->flags_08 & 0x2000) == 0) {
            mark_needs_refile(si, 2);
            mark_part_shapes(si, 3);
        }
        si = pick_for_record(si, 0x1000);
    }

    DG4E67.origin_x = di;
    DG4E67.origin_y = y;
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
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);

    part->pos[0].y = -1;
    part->pos[0].x = -1;

    if (part->kind == KIND_BELT)
        move_carried_rope();
    else if (part->kind == KIND_ROPE)
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
    struct rope *link = ROPE_PTR(PART_PTR(DG50D3.dragged_part_ptr)->rope_ptr);
    struct part *di = PART_PTR(link->end_a_ptr);
    int16_t close = rope_ends_close(link);
    struct part *si;

    if (DG5768.button_left == 2) {
        if (close == 0) {
            if (di != PART_NONE)
                discard_carried_part();
            return;
        }

        si = find_part_from(PART_NONE);

        if (di != PART_NONE) {
            si->flags_08 |= 2;
            si->start_flags = si->flags_08;
            link->end_b_ptr = dg_near(dgroup, si);
            si->rope_ptr = dg_near(dgroup, link);

            compute_link_endpoints(link);
            mark_needs_refile(PART_PTR(DG50D3.dragged_part_ptr), 2);
            refile_part_list(PART_PTR(DG50D3.dragged_part_ptr));
            DG4E67.tool = 0;
            DG50D3.dragged_part_ptr = 0;
            return;
        }

        si->flags_08 |= 2;
        si->start_flags = si->flags_08;
        link->end_a_ptr = dg_near(dgroup, si);
        si->rope_ptr = dg_near(dgroup, link);
        return;
    }

    if (di == PART_NONE)
        return;

    DG52BD.anchor_x = (uint16_t)(((uint16_t)di->pos[0].x)
                               + di->grab.x);
    DG52BD.anchor_y = (uint16_t)(((uint16_t)di->pos[0].y)
                               + di->grab.y);
    DG52BD.band_x = (uint16_t)(((uint16_t)DG5768.pointer_x) + ((uint16_t)DG4E67.origin_x));
    DG52BD.band_y = (uint16_t)(((uint16_t)DG5768.pointer_y) + ((uint16_t)DG4E67.origin_y));

    DG52BD.band_colour = (close != 0) ? 0x0a : 0x0c;
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
    struct part *far_;                /* [bp-4] */
    int16_t end;     /* [bp-2] */
    struct belt *si = BELT_PTR(PART_PTR(DG50D3.dragged_part_ptr)->belt_ptr[0]);
    struct part *di;
    uint16_t idx;

    far_ = PART_PTR(si->end_a_ptr);

    di = find_belt_anchor(&end, PART_PTR(DG2630.belt_anchor_ptr));

    if (di == PART_PTR(DG5456.belt_far_end_ptr) && far_ != PART_NONE)
        di = PART_NONE;
    else if (di == far_ && far_ != PART_NONE)
        di = PART_NONE;

    DG2630.belt_anchor_ptr = dg_near(dgroup, di);

    if (DG5768.button_left == 2) {
        if (di == PART_NONE) {
            if (far_ != PART_NONE)
                discard_carried_part();
            return;
        }

        if (far_ == PART_NONE) {
            if (di->kind != KIND_PULLEY) {
                di->belt_ptr[(uint16_t)end] = dg_near(dgroup, si);
                si->end_a_ptr = dg_near(dgroup, di);
                si->home_a_ptr = dg_near(dgroup, di);
                si->slot_a = (uint8_t)(uint16_t)end;
                si->home_slot_a = (uint8_t)(uint16_t)end;
                DG5456.belt_far_end_ptr = dg_near(dgroup, di);
            }
            return;
        }

        if (PART_PTR(DG5456.belt_far_end_ptr)->kind == KIND_PULLEY) {
            PART_PTR(DG5456.belt_far_end_ptr)->link_ptr[0] = dg_near(dgroup, di);
            PART_PTR(DG5456.belt_far_end_ptr)->link_ptr[2] = dg_near(dgroup, di);
            mark_joined_shapes(PART_PTR(DG5456.belt_far_end_ptr), 3);
            mark_part_shapes(PART_PTR(DG5456.belt_far_end_ptr), 3);
            mark_needs_refile(PART_PTR(DG5456.belt_far_end_ptr), 2);
        } else {
            idx = si->slot_a;
            PART_PTR(DG5456.belt_far_end_ptr)->link_ptr[idx] = dg_near(dgroup, di);
            PART_PTR(DG5456.belt_far_end_ptr)->link_ptr[idx + 2] = dg_near(dgroup, di);
        }

        refresh_link_geometry(si);
        mark_needs_refile(PART_PTR(DG50D3.dragged_part_ptr), 2);

        if (di->kind == KIND_PULLEY) {
            di->link_ptr[1] = DG5456.belt_far_end_ptr;
            di->link_ptr[3] = DG5456.belt_far_end_ptr;
            di->belt_ptr[1] = dg_near(dgroup, si);
            if (PART_PTR(DG5456.belt_far_end_ptr)->kind == KIND_PULLEY)
                aim_link_at_bisector(PART_PTR(DG5456.belt_far_end_ptr));
            DG5456.belt_far_end_ptr = dg_near(dgroup, di);
        } else {
            di->link_ptr[(uint16_t)end] = DG5456.belt_far_end_ptr;
            di->link_ptr[(uint16_t)end + 2] = DG5456.belt_far_end_ptr;
            di->belt_ptr[(uint16_t)end] = dg_near(dgroup, si);
            si->end_b_ptr = dg_near(dgroup, di);
            si->home_b_ptr = dg_near(dgroup, di);
            si->slot_b = (uint8_t)(uint16_t)end;
            si->home_slot_b = (uint8_t)(uint16_t)end;
            if (PART_PTR(DG5456.belt_far_end_ptr)->kind == KIND_PULLEY)
                aim_link_at_bisector(PART_PTR(DG5456.belt_far_end_ptr));
            refile_part_list(PART_PTR(DG50D3.dragged_part_ptr));
            DG4E67.tool = 0;
            DG50D3.dragged_part_ptr = 0;
        }
        return;
    }

    if (far_ == PART_NONE) {
        return;
    }

    if (PART_PTR(DG5456.belt_far_end_ptr)->kind == KIND_PULLEY) {
        end = 1;
        aim_link_at_bisector(PART_PTR(DG5456.belt_far_end_ptr));
        mark_joined_shapes(PART_PTR(DG5456.belt_far_end_ptr), 3);
        mark_part_shapes(PART_PTR(DG5456.belt_far_end_ptr), 3);
        mark_needs_refile(PART_PTR(DG5456.belt_far_end_ptr), 2);
    } else {
        end = (int16_t)si->slot_a;
    }

    DG52BD.anchor_x = (uint16_t)(((uint16_t)PART_PTR(DG5456.belt_far_end_ptr)->pos[0].x)
                    + PART_PTR(DG5456.belt_far_end_ptr)->attach[(uint16_t)end].x);
    DG52BD.anchor_y = (uint16_t)(((uint16_t)PART_PTR(DG5456.belt_far_end_ptr)->pos[0].y)
                    + PART_PTR(DG5456.belt_far_end_ptr)->attach[(uint16_t)end].y);
    DG52BD.band_x = (uint16_t)(((uint16_t)DG5768.pointer_x) + ((uint16_t)DG4E67.origin_x));
    DG52BD.band_y = (uint16_t)(((uint16_t)DG5768.pointer_y) + ((uint16_t)DG4E67.origin_y));

    DG52BD.band_colour = (di != PART_NONE) ? 0x0a : 0x0c;
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
    struct part *part;
    struct rope *si;
    int16_t di;

    part_key_shortcut();

    part = PART_PTR(DG50D3.dragged_part_ptr);

    if (part->flags_0a & 8) {
        part->pos[0].x =
            (uint16_t)(((uint16_t)DG5768.pointer_x) - DG4E67.drag_offset_x + ((uint16_t)DG4E67.origin_x));

        if ((int16_t)(((uint16_t)part->pos[0].x)
                      + ((uint16_t)part->size[0].width))
            <= (int16_t)(((uint16_t)DG4E67.origin_x) + 0x0c))
            part->pos[0].x =
                (uint16_t)(((uint16_t)DG4E67.origin_x) - ((uint16_t)part->size[0].width)
                           + 12);

        if ((int16_t)((uint16_t)part->pos[0].x)
            >= (int16_t)(((uint16_t)DG4E67.origin_x) + 0x235))
            part->pos[0].x =
                (uint16_t)(((uint16_t)DG4E67.origin_x) + 565);

        part->pos[0].y =
            (uint16_t)(((uint16_t)DG5768.pointer_y) - DG4E67.drag_offset_y + ((uint16_t)DG4E67.origin_y));

        if ((int16_t)(((uint16_t)part->pos[0].y)
                      + ((uint16_t)part->size[0].height))
            <= (int16_t)(((uint16_t)DG4E67.origin_y) + 0x0c))
            part->pos[0].y =
                (uint16_t)(((uint16_t)DG4E67.origin_y) - ((uint16_t)part->size[0].height)
                           + 12);

        if ((int16_t)((uint16_t)part->pos[0].y)
            >= (int16_t)(((uint16_t)DG4E67.origin_y) + 0x165))
            part->pos[0].y =
                (uint16_t)(((uint16_t)DG4E67.origin_y) + 357);
    } else {
        part->pos[0].x =
            (uint16_t)(((((uint16_t)DG5768.pointer_x) - DG4E67.drag_offset_x) & 0xfff0)
                       + ((uint16_t)DG4E67.origin_x));
        if ((int16_t)(((uint16_t)part->pos[0].x)
                      + ((uint16_t)part->size[0].width))
            <= (int16_t)((uint16_t)DG4E67.origin_x))
            part->pos[0].x =
                (uint16_t)(((uint16_t)part->pos[0].x) + 16);

        part->pos[0].y =
            (uint16_t)(((((uint16_t)DG5768.pointer_y) - DG4E67.drag_offset_y) & 0xfff0)
                       + ((uint16_t)DG4E67.origin_y));
        if ((int16_t)(((uint16_t)part->pos[0].y)
                      + ((uint16_t)part->size[0].height))
            <= (int16_t)((uint16_t)DG4E67.origin_y))
            part->pos[0].y =
                (uint16_t)(((uint16_t)part->pos[0].y) + 16);
    }

    place_object_for_draw(part);
    retension_pulleys(part);

    si = ROPE_PTR(part->rope_ptr);
    di = (si != ROPE_NONE) ? (int16_t)(rope_ends_close(si) == 0) : 0;

    if (part->flags_0a & 1)
        rehome_carried_part();
    else if (part->flags_0a & 2)
        break_second_attachment(part);

    if (object_overlaps_any(part) != 0) {
        DG52BD.drop_cursor = 0x0e;
    } else if (DG5768.button_left == 2) {
        mark_joined_shapes(part, 3);

        if (di != 0) {
            untie_rope(PART_PTR(si->owner_ptr));
            discard_part(PART_PTR(si->owner_ptr));
            DG4E67.redraw_e = 2;
        }

        mark_needs_refile(part, 2);
        part->start_x = ((uint16_t)part->pos[0].x);
        part->start_y = ((uint16_t)part->pos[0].y);
        refile_part_list(part);
        DG4E67.tool = 0;
        DG50D3.dragged_part_ptr = 0;
    } else {
        DG52BD.drop_cursor = 0x0c;
    }

    if (DG5768.button_left != 2)
        part_moved(part);
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
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    static const uint16_t KEYS[6] = { 12, 13, 21, 45, 74, 78 };
    uint16_t key = (DG52ED.last_key);
    int32_t i;

    for (i = 0; i < 6; i++)
        if (KEYS[i] == key)
            break;

    if (i == 6)
        return;

    switch (KEYS[i]) {
    case 45:
        if (part->flags_06 & 0x400)
            flip_carried_end_1();
        return;
    case 21:
        if (part->flags_06 & 0x200)
            flip_carried_end_2();
        return;
    case 13:
    case 78:
        carried_part_grow();
        return;
    case 12:
    case 74:
        carried_part_shrink();
        return;
    default:
        return;
    }
}

/*
 * 0x10466
 *
 * **Grow the part in your hand** - the `=` and `+` arm of
 * `part_key_shortcut`, scancodes 13 and 78.
 *
 * Which axis grows is decided first, and it is not a choice the player makes:
 * the **shorter side grows**, so +0x50 unless +0x52 is already bigger. Kind 2
 * is the exception and always takes the width, whatever its height is.
 *
 * Then the chosen side moves by 0x10 if the kind's maximum leaves room -
 * `cs:0x0eb2` for the width and `cs:0x0eb4` for the height, both compared
 * signed - and +0x40 or +0x42 is brought along to match. Nothing happens at
 * all when the part is already at its limit; there is no clamp, just no step.
 *
 * The original holds the kind in SI, loaded by `part_key_shortcut` and, as the
 * note there says, never used by that routine itself - only by these two arms.
 * It is the part's own +4, so it is recomputed here rather than passed.
 */
void carried_part_grow(void)
{
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    struct part_kind *kind = &PART_KINDS[part->kind];

    if ((int16_t)part->set_size.height
            <= (int16_t)part->set_size.width
        || part->kind == KIND_RAMP) {
        if (kind->max_w
                > (int16_t)part->set_size.width) {
            part->set_size.width =
                (uint16_t)(part->set_size.width + 0x10);
            part->mirror_size.width = part->set_size.width;
            carried_part_resized(part, kind);
        }
    } else {
        if (kind->max_h
                > (int16_t)part->set_size.height) {
            part->set_size.height =
                (uint16_t)(part->set_size.height + 0x10);
            part->mirror_size.height = part->set_size.height;
            carried_part_resized(part, kind);
        }
    }
}

/*
 * 0x10551
 *
 * **Shrink the part in your hand** - the `-` arm, scancodes 12 and 74, and the
 * mirror of `carried_part_grow` instruction for instruction: the same choice of
 * axis, the minimum at `cs:0x0eb6` and `cs:0x0eb8` instead of the maximum, the
 * comparison the other way round, and 0x10 subtracted rather than added.
 */
void carried_part_shrink(void)
{
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    struct part_kind *kind = &PART_KINDS[part->kind];

    if ((int16_t)part->set_size.height
            <= (int16_t)part->set_size.width
        || part->kind == KIND_RAMP) {
        if (kind->min_w
                < (int16_t)part->set_size.width) {
            part->set_size.width =
                (uint16_t)(part->set_size.width - 0x10);
            part->mirror_size.width = part->set_size.width;
            carried_part_resized(part, kind);
        }
    } else {
        if (kind->min_h
                < (int16_t)part->set_size.height) {
            part->set_size.height =
                (uint16_t)(part->set_size.height - 0x10);
            part->mirror_size.height = part->set_size.height;
            carried_part_resized(part, kind);
        }
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
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    uint16_t si = 0, idx;
    struct belt *rec;
    struct part *di;

    DG4E67.drag_offset_x = (uint16_t)(((uint16_t)DG5768.pointer_x)
                               - ((uint16_t)part->pos[0].x)
                               + ((uint16_t)DG4E67.origin_x));
    DG4E67.drag_offset_y = (uint16_t)(((uint16_t)DG5768.pointer_y)
                               - ((uint16_t)part->pos[0].y)
                               + ((uint16_t)DG4E67.origin_y));

    di = PART_PTR(part->rope_ptr);
    if (di != PART_NONE)
        si = di->kind;

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);

    if (part->kind == KIND_BELT) {
        untie_rope(part);
    } else if (part->kind == KIND_ROPE) {
        rec = BELT_PTR(part->belt_ptr[0]);
        idx = ((int8_t)rec->slot_b);
        DG5456.belt_far_end_ptr = PART_PTR(rec->end_b_ptr)->link_ptr[idx];
        detach_belt(part, 0);
    } else {
        detach_part_to_bin(part);
    }

    if (part->kind == KIND_BELT) {
        di->kind = si;
        PART_PTR(si)->flags_08 |= 2;
        PART_PTR(si)->start_flags = PART_PTR(si)->flags_08;
        PART_PTR(si)->rope_ptr = dg_near(dgroup, di);
    }

    DG4E67.tool = 9;
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
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);

    mark_joined_shapes(part, 3);
    mark_part_shapes(part, 3);

    if (part->kind == KIND_BELT) {
        untie_rope(part);
        discard_part(part);
    } else if (part->kind == KIND_ROPE) {
        detach_belt(part, 1);
        discard_part(part);
    } else {
        detach_part_to_bin(part);
        finish_part_removal();
    }

    DG4E67.redraw_e = 2;
    DG4E67.tool = 0;
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
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    struct part_kind *kind = &PART_KINDS[part->kind];

    kind->flip(part, 1);
    part->start_flags = part->flags_08;
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
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    struct part_kind *kind = &PART_KINDS[part->kind];

    kind->flip(part, 2);
    part->start_flags = part->flags_08;
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
    int16_t si = 0;
    struct part *part;
    struct part_kind *kind;

    if ((DG4E67.tool & 0x8000) == 0) {
        if (DG5768.button_left == 2)
            DG4E67.tool |= 0x8000;
        return;
    }

    switch ((uint16_t)(DG4E67.tool - 0x8003)) {
    case 0: si = drag_carried_part_first();   break;
    case 1: si = settle_carried_part_first(); break;
    case 2: si = drag_carried_part_pair();    break;
    case 3: si = settle_carried_part();       break;
    default: break;
    }

    if (si != 0) {
        part = PART_PTR(DG50D3.dragged_part_ptr);
        kind = &PART_KINDS[part->kind];

        part->mirror_size.height = part->set_size.height;
        part->mirror_size.width = part->set_size.width;

        kind->settle(part);
        place_object_for_draw(part);
        mark_joined_shapes(part, 3);
        mark_part_shapes(part, 3);
        mark_needs_refile(part, 2);
    }

    if (DG5768.button_left == 2) {
        DG4E67.tool = 0;
        DG50D3.dragged_part_ptr = 0;
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
    uint16_t moved;                    /* [bp-8] */
    uint16_t hi;    /* [bp-6] */
    uint16_t lo;    /* [bp-4] */
    uint16_t was;    /* [bp-2] */
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    struct part_kind *kind  = &PART_KINDS[part->kind];
    int16_t  si, di;

    moved = 0;
    was = ((uint16_t)part->pos[0].x);

    si = (int16_t)((((uint16_t)DG5768.pointer_x) & 0xfff0) + ((uint16_t)DG4E67.origin_x));

    lo = ((uint16_t)kind->min_w);
    hi = ((uint16_t)kind->max_w);

    di = (int16_t)(was - si + part->set_size.width);

    if (di > (int16_t)hi) {
        si = (int16_t)(si + (di - (int16_t)hi));
        di = (int16_t)hi;
    } else if (di < (int16_t)lo) {
        si = (int16_t)(si - ((int16_t)lo - di));
        di = (int16_t)lo;
    }

    if (was != (uint16_t)si) {
        part->pos[0].x = (uint16_t)si;
        part->set_size.width = (uint16_t)di;

        for (;;) {
            kind->settle(part);
            place_object_for_draw(part);
            kind->setup(part);
            if (object_overlaps_any(part) == 0)
                break;
            part->pos[0].x =
                (uint16_t)(((uint16_t)part->pos[0].x) + 16);
            part->set_size.width =
                (uint16_t)(part->set_size.width - 0x10);
        }

        if (((uint16_t)part->pos[0].x) != was) {
            part->start_x = ((uint16_t)part->pos[0].x);
            moved = 1;
        }
    }

    {
        int16_t answer = (int16_t)moved;

        return answer;
    }
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
    int16_t moved;                    /* [bp-6] */
    int16_t hi;    /* [bp-4] */
    int16_t lo;    /* [bp-2] */
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    struct part_kind *kind  = &PART_KINDS[part->kind];
    uint16_t was   = part->set_size.width;
    int16_t  si;

    moved = 0;

    si = (int16_t)((((uint16_t)DG5768.pointer_x) & 0xfff0) + ((uint16_t)DG4E67.origin_x) + 0x10
                   - ((uint16_t)part->pos[0].x));

    lo = (int16_t)((uint16_t)kind->min_w);
    hi = (int16_t)((uint16_t)kind->max_w);

    if (si > (int16_t)hi)
        si = (int16_t)hi;
    else if (si < (int16_t)lo)
        si = (int16_t)lo;

    if (was != (uint16_t)si) {
        part->set_size.width = (uint16_t)si;

        for (;;) {
            kind->settle(part);
            place_object_for_draw(part);
            kind->setup(part);
            if (object_overlaps_any(part) == 0)
                break;
            part->set_size.width =
                (uint16_t)(part->set_size.width - 0x10);
        }

        if (part->set_size.width != was)
            moved = 1;
    }

    {
        int16_t answer = (int16_t)moved;
        return answer;
    }
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
    int16_t moved;                    /* [bp-8] */
    int16_t hi;    /* [bp-6] */
    int16_t lo;    /* [bp-4] */
    int16_t was;    /* [bp-2] */
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    struct part_kind *kind  = &PART_KINDS[part->kind];
    int16_t  si, di;

    moved = 0;
    was = (int16_t)((uint16_t)part->pos[0].y);

    si = (int16_t)((((uint16_t)DG5768.pointer_y) & 0xfff0) + ((uint16_t)DG4E67.origin_y));

    lo = (int16_t)((uint16_t)kind->min_h);
    hi = (int16_t)((uint16_t)kind->max_h);

    di = (int16_t)((uint16_t)was - si + part->set_size.height);

    if (di > (int16_t)hi) {
        si = (int16_t)(si + (di - (int16_t)hi));
        di = (int16_t)hi;
    } else if (di < (int16_t)lo) {
        si = (int16_t)(si - ((int16_t)lo - di));
        di = (int16_t)lo;
    }

    if ((uint16_t)was != (uint16_t)si) {
        part->pos[0].y = (uint16_t)si;
        part->set_size.height = (uint16_t)di;

        for (;;) {
            kind->settle(part);
            place_object_for_draw(part);
            kind->setup(part);
            if (object_overlaps_any(part) == 0)
                break;
            part->pos[0].y =
                (uint16_t)(((uint16_t)part->pos[0].y) + 16);
            part->set_size.height =
                (uint16_t)(part->set_size.height - 0x10);
        }

        if (((uint16_t)part->pos[0].y) != (uint16_t)was) {
            part->start_y = ((uint16_t)part->pos[0].y);
            moved = 1;
        }
    }

    {
        int16_t answer = (int16_t)moved;
        return answer;
    }
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
    uint16_t moved;                    /* [bp-6] */
    uint16_t hi;    /* [bp-4] */
    uint16_t lo;    /* [bp-2] */
    struct part *part = PART_PTR(DG50D3.dragged_part_ptr);
    uint16_t was   = part->set_size.height;
    struct part_kind *kind  = &PART_KINDS[part->kind];
    int16_t  y;

    moved = 0;

    y = (int16_t)((((uint16_t)DG5768.pointer_y) & 0xfff0) + ((uint16_t)DG4E67.origin_x) + 0x10
                  - ((uint16_t)part->pos[0].y));

    lo = ((uint16_t)kind->min_h);
    hi = ((uint16_t)kind->max_h);

    if (y > (int16_t)hi)
        y = (int16_t)hi;
    else if (y < (int16_t)lo)
        y = (int16_t)lo;

    if ((uint16_t)y != was) {
        part->set_size.height = (uint16_t)y;

        for (;;) {
            kind->settle(part);
            place_object_for_draw(part);
            kind->setup(part);
            if (object_overlaps_any(part) == 0)
                break;
            part->set_size.height =
                (uint16_t)(part->set_size.height - 0x10);
        }

        if (part->set_size.height != was)
            moved = 1;
    }

    {
        int16_t answer = (int16_t)moved;

        return answer;
    }
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
    uint16_t si;

    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        DG2630.back_held = 0;
        DG4E67.state = 0x1000;
        DG4E67.redraw_a = 2;
        return;
    }

    if (bin_repeat_due(((int16_t)DG2630.back_held))) {   /* deviation: see above */
        si = (uint16_t)bin_part_at_index(-5);
        if (si != DG50D3.bin_list_ptr) {
            DG50D3.bin_list_ptr = si;
            DG4E67.redraw_e = 2;
        } else {
            si = bin_scroll_end();
            if (si != DG50D3.bin_list_ptr) {
                DG50D3.bin_list_ptr = si;
                DG4E67.redraw_e = 2;
            }
        }
    }

    DG2630.back_held++;
    DG4E67.redraw_a = 2;
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
    uint16_t si;

    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        DG2630.forward_held = 0;
        DG4E67.state = 0x1000;
        DG4E67.redraw_a = 2;
        return;
    }

    if (bin_repeat_due(((int16_t)DG2630.forward_held))) {   /* deviation: see above */
        si = (uint16_t)bin_part_at_index(5);
        if (si != 0)
            DG50D3.bin_list_ptr = si;
        else
            DG50D3.bin_list_ptr = dg_near(dgroup, &DG50D3.parts_bin);
        DG4E67.redraw_e = 2;
    }

    DG2630.forward_held++;
    DG4E67.redraw_a = 2;
}

/*
 * 0x10da9
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
 * The call is the near-to-far thunk - `push si / push cs / call` - so the
 * callee's `retf` finds a full far return; the `nop` between is the assembler
 * padding it, and `pop cx` is the caller clearing its argument.
 */
void region_cursor_bin_above(struct region *region)
{
    if (DG4E67.tool == 9) {
        region_cursor_bin(region);
        region->code = 0x1000;
        return;
    }

    region->cursor = 0x1a;
    region->code = 0x2000;
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
    if (DG4E67.tool == 9) {
        uint16_t kind = PART_PTR(DG50D3.dragged_part_ptr)->kind;

        region->cursor =
            (kind == 8) ? 8 : (kind == 0x0a) ? 9 : 0;
        return;
    }

    region->cursor =
        bin_part_at_index((int16_t)region->slot) != 0 ? 2 : 0;
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
    struct part *saved;                /* [bp-2] */
    struct part *part, *clone;

    if (DG4E67.tool == 9) {
        uint16_t kind = PART_PTR(DG50D3.dragged_part_ptr)->kind;

        if (kind == 8 || kind == 0x0a)
            DG4E67.redraw_e = 2;

        discard_carried_part();
        DG4E67.tool = 0;
        return;
    }

    DG4E67.drag_offset_y = 0;
    DG4E67.drag_offset_x = 0;

    part = PART_PTR(PART_PTR(bin_part_at_index(
                     (int16_t)region->slot))->next_ptr);
    DG50D3.dragged_part_ptr = dg_near(dgroup, part);

    if (part == PART_NONE) {
        return;
    }

    if (DG4E67.freeform != 0) {
        clone = clone_part(PART_PTR(DG50D3.dragged_part_ptr));
        saved = PART_PTR(DG50D3.dragged_part_ptr);
        DG50D3.dragged_part_ptr = 0;

        if (check_room_for_part() != 0) {
            DG50D3.dragged_part_ptr = dg_near(dgroup, saved);
            clone->next_ptr = PART_PTR(DG50D3.dragged_part_ptr)->next_ptr;
            if (clone->next_ptr != 0)
                PART_PTR(clone->next_ptr)->prev_ptr = dg_near(dgroup, clone);
            clone->prev_ptr = DG50D3.dragged_part_ptr;
            PART_PTR(DG50D3.dragged_part_ptr)->next_ptr = dg_near(dgroup, clone);
            DG50D3.dragged_part_ptr = dg_near(dgroup, clone);
        } else {
            free_part(clone);
        }
    }

    if (DG50D3.dragged_part_ptr != 0) {
        uint16_t kind;

        DG4E67.tool = 9;
        kind = PART_PTR(DG50D3.dragged_part_ptr)->kind;
        if (kind == 8 || kind == 0x0a)
            DG4E67.redraw_e = 2;
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
    /*
     * **The original's `sub sp,0x16` is this routine's own locals**, and the
     * port keeps them in `s` below rather than in DGROUP - so there was
     * nothing left for the reservation to protect. It was kept on the reading
     * that a callee's frame has to land *below* this one; that is true of a
     * routine whose locals are the guest's, and this one's are not. Without
     * it a callee's frame lands 0x18 higher, on bytes nothing reads.
     */
    struct screen_loop s = {0, 0, 0, 0, 0, 0, 0, 0};

    reset_machine();
    paint_game_screen(1);
    set_palette_pointer(DG52ED.pal_tim_ptr);
    show_cursor_again();

    while (s.done == 0) {
        update_button_state();

        DG52ED.last_key = (uint8_t)(bios_read_key() >> 8);
        if ((DG52ED.last_key) == SC_TAB)
            tab_move_pointer();

        regions_handle_pointer(DG4E67.regions_panel_ptr);

        if (key_is_down(SC_ALT) && key_is_down(SC_V)) {
            show_message_box(DG1BCC.version_number, (char *)DG1BCC.this_is_version);
            s.repaint_all = 1;
            DG4E67.state = 2;
        }

        switch (DG4E67.state) {
        case 0x8000:
            paint_panel_a(1);
            present_back_page();
            DG4E67.state = 0x1000;
            s.done = 1;
            break;
        case 0x4000: screen_state_4000(&s); break;
        case 0x2000: screen_state_2000(&s); break;
        case 0x1000: screen_state_1000(&s); break;
        case 0x0800: screen_state_0800(&s); break;
        case 0x0400: screen_state_0400(&s); break;
        case 0x0200: screen_state_0200(&s); break;
        case 0x0100: screen_state_0100(&s); break;
        case 0x0080: screen_state_0080(&s); break;
        case 0x0040: screen_state_0040(&s); break;
        case 0x0020: screen_state_0020(&s); break;
        default:
            /* State 2 among them: not in the table, so nothing runs. */
            break;
        }

        if (s.repaint_all != 0) {
            paint_game_screen(1);
            s.repaint_all--;
        } else {
            if (s.repaint_e != 0) {
                paint_panel_e();
                s.repaint_e--;
            }
            if (s.repaint_f != 0) {
                paint_panel_f();
                s.repaint_f--;
            }
            if (s.repaint_g != 0) {
                paint_panel_g();
                s.repaint_g--;
            }
        }

        present_frame(1);
    }
}

/*
 * 0x10fde
 *
 * **The volume knob, upwards.** State 0x4000, and the counterpart at 0x11025
 * is the same thing downwards.
 *
 * It is an *auto-repeat*, which is why it needs the loop's own counter. While
 * the button state at DGROUP 0x5774 is 1 or 2 - held - `held` counts passes,
 * and the step is taken only on every eighth one, so holding the knob down
 * walks the level up at a readable rate rather than as fast as the screen
 * loops. Let go and 0x5774 is neither, which resets the counter and puts the
 * state back to 2, the screen's resting state.
 *
 * The level is DGROUP 0x4ec1, and it stops at 6 rather than wrapping. Each
 * step redraws through 0x12bed and then sets the master level from the table
 * at DGROUP 0x116, indexed by the level - so the table is what the knob's
 * seven positions *mean*, and the knob itself only counts.
 *
 * The tail sets `repaint_e` to **2 and not 1**, which is the double buffer: one
 * per page, so the knob does not draw itself into whichever page happens to be
 * current and leave the other a frame stale.
 */
void screen_state_4000(struct screen_loop *s)
{
    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        s->held = 0;
        DG4E67.state = 2;
    } else {
        if (s->held % 8 == 0 && ((int16_t)DG4E67.master_level) != 6) {
            DG4E67.master_level++;
            write_config();
            set_master_level_ok(GAME_MASTER_LEVELS.master_level_ok[DG4E67.master_level]);
        }
        s->held++;
    }

    s->repaint_e = 2;
}

/*
 * 0x11025
 *
 * **The volume knob, downwards.** State 0x2000: 0x10fde with `dec` for `inc`
 * and a floor of 0 for its ceiling of 6. Transcribed as its own routine
 * because the original has two, and which one a state reaches is the whole
 * difference between them.
 */
void screen_state_2000(struct screen_loop *s)
{
    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        s->held = 0;
        DG4E67.state = 2;
    } else {
        if (s->held % 8 == 0 && ((int16_t)DG4E67.master_level) != 0) {
            DG4E67.master_level--;
            write_config();
            set_master_level_ok(GAME_MASTER_LEVELS.master_level_ok[DG4E67.master_level]);
        }
        s->held++;
    }

    s->repaint_e = 2;
}

/*
 * 0x11072
 *
 * **Quit game.** State 0x1000: draw the panel piece that shows the button
 * pressed, put that on the screen, and ask.
 *
 * `present_back_page` before the question and not after: the player has to see
 * the button go down before a box appears over it, and the box is drawn into
 * the page that is now the back one.
 *
 * Yes leaves by writing 1 into the state and setting `done`, which is the only
 * way out of `game_screen`'s loop - it does not return a value. No puts the
 * state back to 2 and asks for one whole-screen repaint, which is what paints
 * over where the box was.
 */
void screen_state_1000(struct screen_loop *s)
{
    paint_panel_b(1);
    present_back_page();

    if (ask_yes_no(DG1BCC.quit_game, (char *)DG1BCC.quit_body)) {   /* "QUIT GAME" / "Are you sure ..." */
        DG4E67.state = 1;
        s->done = 1;
    } else {
        DG4E67.state = 2;
        s->repaint_all = 1;
    }
}

/*
 * 0x110ad
 *
 * **Restart level.** State 0x0800: the same shape as quit - draw the button
 * pressed, present it, ask - and the same two ways out.
 *
 * Yes clears the machine through `remove_all_parts` and then leaves the loop
 * with the state at **0x1000**, not 1. 0x1000 is quit's state, so a restart
 * goes out the way a quit does and `game_round` is what tells them apart; this
 * routine does not restart anything itself.
 */
void screen_state_0800(struct screen_loop *s)
{
    paint_panel_c(1);
    present_back_page();

    if (ask_yes_no(DG1BCC.restart_level, (char *)DG1BCC.restart_body)) {   /* "RESTART LEVEL" */
        remove_all_parts();
        DG4E67.state = 0x1000;
        s->done = 1;
    } else {
        DG4E67.state = 2;
        s->repaint_all = 1;
    }
}

/*
 * 0x110ed
 *
 * **Freeform mode.** State 0x0400, and it does nothing at all if DGROUP 0x4e67
 * says the game is already in it - the test at 0x110f2 jumps past everything,
 * including the repaint, to the common tail.
 *
 * Otherwise it asks, and yes tears the round down, loads `ff.lev`, resets the
 * machine and clears five words: the mode flag 0x4e67 goes to 1 and 0x4eaf,
 * 0x4ead, 0x50b1 and 0x50af to zero. `start_counters` last.
 *
 * **The state goes to 2 on every path**, including the one that did nothing,
 * because all three converge on 0x11321 - so this cannot be left half-entered.
 * The whole-screen repaint is asked for on the two that got as far as the
 * question, and not on the early exit.
 */
void screen_state_0400(struct screen_loop *s)
{
    if (DG4E67.freeform != 0) {
        DG4E67.state = 2;
        return;
    }

    paint_panel_level(1);
    present_back_page();

    if (ask_yes_no(DG1BCC.freeform_mode, (char *)DG1BCC.freeform_body)) {   /* "FREEFORM MODE" */
        round_teardown();
        load_animation((char *)GAME_LEVEL_STRINGS.ff_lev);
        reset_machine();

        DG4E67.freeform = 1;
        DG4E67.counter = 0;
        DG50AF.bonus_2 = 0;
        DG50AF.bonus_1 = 0;

        start_counters();
    }

    s->repaint_all = 1;
    DG4E67.state = 2;
}

/*
 * 0x1114f
 *
 * **Leave freeform mode**, and reload the level if it needs it. State 0x0200.
 *
 * Two tests on DGROUP 0x4e67, not one, and they are not the same test. The
 * first asks whether the game is *in* freeform mode and only then puts the
 * question; answering yes clears the flag and sets `reload`. The second asks
 * again, having possibly just cleared it - so the reload below runs both for
 * someone who has this moment left freeform mode and for someone who was never
 * in it. Reading the second as an `else` of the first loses that.
 *
 * The reload itself is conditional on either `select_puzzle_screen` answering non-zero or
 * `reload` being set, and clears `reload` on its way out.
 *
 * The state goes to 2 and the screen repaints whole, on every path, at 0x11321
 * - the same convergence "freeform mode" uses.
 */
void screen_state_0200(struct screen_loop *s)
{
    paint_panel_d(1);
    present_back_page();

    if (DG4E67.freeform != 0) {
        if (ask_yes_no(DG1BCC.leave_freeform_mode, (char *)DG1BCC.leave_freeform_body)) {   /* "LEAVE FREEFORM MODE" */
            DG4E67.freeform = 0;
            s->reload = 1;
        }
    }

    if (DG4E67.freeform == 0) {
        if (select_puzzle_screen() != 0 || s->reload != 0) {
            round_teardown();
            load_level(((uint16_t)DG4E67.round_number));
            reset_machine();
            s->reload = 0;
            start_counters();
        }
    }

    s->repaint_all = 1;
    DG4E67.state = 2;
}

/*
 * 0x111bd
 *
 * **Load a machine**, and freeform only: the test on DGROUP 0x4e67 at the top
 * jumps straight to the common tail if the game is not in freeform mode, so
 * the button exists on every screen and does nothing on most of them.
 *
 * The picker is bracketed by two **directory dances**, and they are not the
 * same one. Before: change to the path at DGROUP 0x530b and, if that worked,
 * select the drive its first character names - which is the game's own
 * directory being restored. After: `dos_get_cur_dir` writes wherever the
 * picker left the process into 0x530b, and the second pair does the same dance
 * with 0x535b. So the picker is free to wander and the game puts itself back.
 *
 * `0x4e85` is 1 around each dance and 0 between them, which is the only thing
 * that distinguishes "the game is doing file IO" from the rest of the loop.
 *
 * Choosing a file tears the round down, loads it - `load_animation` with the
 * name at 0x52fe, which is where the picker left it - and resets the machine.
 * Choosing nothing does none of that, and either way the screen repaints whole
 * and the state returns to 2.
 */
void screen_state_0100(struct screen_loop *s)
{
    if (DG4E67.freeform == 0) {
        DG4E67.state = 2;
        return;
    }

    paint_panel_free_a(1);
    present_back_page();

    DG4E67.file_op_active = 1;
    if (dos_chdir((const char *)GAME_DIRECTORIES.picker_dir) == 0)
        dos_setdisk((uint8_t)GAME_DIRECTORIES.picker_dir[0]);
    DG4E67.file_op_active = 0;

    if (pick_file(0, 0, GAME_LEVEL_STRINGS.tim_filter_load)) {
        round_teardown();
        load_animation((char *)DG52FE.name);
        reset_machine();
    }

    DG4E67.file_op_active = 1;
    dos_get_cur_dir((char *)GAME_DIRECTORIES.picker_dir);
    if (dos_chdir((const char *)GAME_DIRECTORIES.game_dir) == 0)
        dos_setdisk((uint8_t)GAME_DIRECTORIES.game_dir[0]);
    DG4E67.file_op_active = 0;

    s->repaint_all = 1;
    DG4E67.state = 2;
}

/*
 * 0x11258
 *
 * **Save the machine**, freeform only, and it is a *retry loop* - the one
 * handler here that is. The picker is asked, the file written, and if the write
 * answers anything but zero the player gets "FILE ERROR" and is asked again.
 * Cancelling the picker sets the result to zero, which is what ends the loop:
 * the same word means "no error" and "nothing to do", and the loop cannot tell
 * them apart because it does not need to.
 *
 * The state is written back to 0x80 at the top of every pass - the state it is
 * already in - so the screen underneath keeps showing the save button pressed
 * while the picker is up.
 *
 * The error box repaints the game screen with `paint_game_screen(0)`, a **0 and
 * not a 1**, which is the argument that says do not present it; the loop is
 * about to put the picker up again over the top.
 *
 * Two directory dances around the whole thing, as in 0x111bd - see there for
 * what they are.
 */
void screen_state_0080(struct screen_loop *s)
{
    if (DG4E67.freeform == 0) {
        DG4E67.state = 2;
        return;
    }

    paint_panel_free_b(1);
    present_back_page();

    DG4E67.file_op_active = 1;
    if (dos_chdir((const char *)GAME_DIRECTORIES.picker_dir) == 0)
        dos_setdisk((uint8_t)GAME_DIRECTORIES.picker_dir[0]);
    DG4E67.file_op_active = 0;

    s->file_err = 1;
    while (s->file_err != 0) {
        DG4E67.state = 0x80;

        if (pick_file(0, 0, GAME_LEVEL_STRINGS.tim_filter_save)) {
            s->file_err = save_machine((char *)DG52FE.name);
            if (s->file_err != 0) {
                show_message_box(DG1BCC.file_error, (char *)DG1BCC.disk_write_protected);
                paint_game_screen(0);
            }
        } else {
            s->file_err = 0;
        }
    }

    dos_get_cur_dir((char *)GAME_DIRECTORIES.picker_dir);
    DG4E67.file_op_active = 1;
    if (dos_chdir((const char *)GAME_DIRECTORIES.game_dir) == 0)
        dos_setdisk((uint8_t)GAME_DIRECTORIES.game_dir[0]);
    DG4E67.file_op_active = 0;

    s->repaint_all = 1;
    DG4E67.state = 2;
}

/*
 * 0x1132a
 *
 * **The gravity slider**, and it is the exact inverse of the knob
 * `paint_panel_f` draws. That routine puts the knob at
 * `0x50b5 * 0xa0 / 0x200 + 0x3d`; this takes the pointer's x at DGROUP 0x5784,
 * subtracts 67 - the `add ax, 0xffbd` - multiplies by 0x200 and divides by
 * 0xa0. The two agree by construction, so the knob lands under the pointer.
 *
 * The multiply and the divide are a long, because `x * 0x200` leaves a word
 * before the divide brings it back, which is the same reason `paint_panel_f`
 * uses one.
 *
 * Outside freeform mode it refuses, with a box saying so, and asks for a whole
 * repaint to paint over it. Inside, it only acts while the button is held -
 * this is a *drag*, not a click - and letting go returns the state to 2.
 *
 * The clamp is to 0 and 0x200 and it is done on the value, not the pointer, so
 * dragging past either end of the track pins the knob rather than stopping the
 * drag. And the store is guarded by `!=`: an unchanged value neither writes
 * 0x50b5 nor asks for the repaint, so holding the knob still costs nothing.
 *
 * `recompute_kind_physics` afterwards, because gravity is not a display value -
 * every kind's behaviour is derived from it.
 */
void screen_state_0040(struct screen_loop *s)
{
    int32_t v;

    if (DG4E67.freeform == 0) {
        /* "CAN'T CHANGE GRAVITY" */
        show_message_box(DG1BCC.cant_change_gravity, (char *)DG1BCC.gravity_body);
        s->repaint_all = 1;
        DG4E67.state = 2;
        return;
    }

    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        DG4E67.state = 2;
        return;
    }

    v = long_divide(mul16x16((int16_t)(DG5768.pointer_x - 67), 0x200),
                    0xa0);
    if (v < 0)
        v = 0;
    else if (v > 0x200)
        v = 0x200;

    if ((int16_t)v != DG50AF.air) {
        DG50AF.air = (int16_t)v;
        s->repaint_f = 2;
        recompute_kind_physics();
    }
}

/*
 * 0x113c3
 *
 * **The air-pressure slider.** 0x1132a with three numbers changed: the scale is
 * 0x80 rather than 0x200, the value goes to DGROUP 0x50b3 rather than 0x50b5,
 * and it is `repaint_g` that is asked for. The x is taken the same way, less
 * the same 67, over the same 0xa0 of track - and `paint_panel_g` divides by
 * 0x80 where `paint_panel_f` divides by 0x200, which is the same pair of
 * numbers seen from the drawing side.
 *
 * Transcribed as its own routine rather than folded into the gravity one with
 * the differences as arguments. The original has two, reached by two states,
 * and what a screen does is decided by which one it reaches.
 */
void screen_state_0020(struct screen_loop *s)
{
    int32_t v;

    if (DG4E67.freeform == 0) {
        /* "CAN'T CHANGE AIR PRESSURE" */
        show_message_box(DG1BCC.cant_change_air_pressure, (char *)DG1BCC.air_pressure_body);
        s->repaint_all = 1;
        DG4E67.state = 2;
        return;
    }

    if (DG5768.button_left != 1 && DG5768.button_left != 2) {
        DG4E67.state = 2;
        return;
    }

    v = long_divide(mul16x16((int16_t)(DG5768.pointer_x - 67), 0x80),
                    0xa0);
    if (v < 0)
        v = 0;
    else if (v > 0x80)
        v = 0x80;

    if ((int16_t)v != DG50AF.gravity) {
        DG50AF.gravity = (int16_t)v;
        s->repaint_g = 2;
        recompute_kind_physics();
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
 * `screen_state_0400`. It was called `restart` here at first, from the state
 * number rather than the table; the restart region is the one with 0x800, and
 * it has **no** handler at all.
 *
 * All five regions stay clickable either way: the mode a region switches to is
 * at +0x10 and none of these touches it.
 */
void region_cursor_freeform(struct region *region)
{
    region->cursor = (DG4E67.freeform != 0) ? 0 : 0x14;
}

/*
 * 0x114f8
 *
 * Load Machine's, and `region_cursor_freeform`'s twin the other way round: a
 * cursor in freeform, nothing outside it.
 */
void region_cursor_load(struct region *region)
{
    region->cursor = (DG4E67.freeform != 0) ? 0x17 : 0;
}

/*
 * 0x11515
 *
 * Save Machine's.
 */
void region_cursor_save(struct region *region)
{
    region->cursor = (DG4E67.freeform != 0) ? 0x16 : 0;
}

/*
 * 0x11532
 *
 * The gravity slider's.
 */
void region_cursor_gravity(struct region *region)
{
    region->cursor = (DG4E67.freeform != 0) ? 0x18 : 0;
}

/*
 * 0x1154f
 *
 * The air-pressure slider's.
 */
void region_cursor_air(struct region *region)
{
    region->cursor = (DG4E67.freeform != 0) ? 0x19 : 0;
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
    int16_t  x;
    uint16_t stop;

    GAME_PLAY_TABS.stop++;

    if (DG4E67.freeform != 0) {
        if (GAME_PLAY_TABS.stop == 5)
            GAME_PLAY_TABS.stop = 6;
    } else if (GAME_PLAY_TABS.stop == 7) {
        GAME_PLAY_TABS.stop = 0;
    }

    if (GAME_PLAY_TABS.stop == 0x0b)
        GAME_PLAY_TABS.stop = 0;

    stop = GAME_PLAY_TABS.stop;

    if (stop == 9)
        x = (int16_t)long_divide(mul16x16(DG50AF.air, 0xa0), 0x200)
            + 0x43;
    else if (stop == 0x0a)
        x = (int16_t)long_divide(mul16x16(DG50AF.gravity, 0xa0), 0x80)
            + 0x43;
    else
        x = GAME_PLAY_TABS.stop_x[stop];

    move_pointer_to(x, GAME_PLAY_TABS.stop_y[stop]);
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
    dg_near_t set;

    wait_cursor();
    set_clip_play_area();

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    VMDS.fill_colour = ((uint8_t)DG52BD.fill_colour);
    VMDS.second_colour = ((uint8_t)DG52BD.fill_colour);
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

    if (DG4E67.freeform != 0) {
        paint_panel_free_a(0);
        paint_panel_free_b(0);
    } else {
        paint_panel_level(0);
    }

    paint_panel_e();
    paint_panel_f();
    paint_panel_g();

    cursor_redraw_off_thunk();
    set = DG52ED.panel_art_ptr;
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x3]), 0x53, 0x42, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x5]), 0x64, 0xb2, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x4]), 0x5b, 0xfe, 0);
    restore_cursor_following();

    select_music(DG50AF.tune);

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
    int16_t  extent;
    int16_t  scale;
    struct part *rec;

    VMDS.clip_enabled = 1;
    set_clip_for_mode();

    extent = (DG50AF.extent_y > DG50AF.extent_x) ? DG50AF.extent_y : DG50AF.extent_x;
    extent = (int16_t)(extent + 0x230);

    scale = (int16_t)long_divide(0x40000, (int32_t)extent);

    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    rec = pick_by_flag(0x3000);
    while (rec != PART_NONE) {
        link_record_into_buckets(rec);
        rec = pick_for_record(rec, 0x1000);
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

    if (DG4E67.freeform != 0) {
        string_copy(title, DG1BCC.freeform_mode_title);
    } else {
        string_copy(title, DG1BCC.puzzle_prefix);
        int_to_string(DG4E67.round_number, digits, 10);
        string_concat(title, digits);
        string_concat(title, GAME_LEVEL_STRINGS.title_sep);
        string_concat(title, (const char *)DG4E67.title);
    }

    set_clip_play_area();
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    draw_title_bar(0x20, 0x20, 0x220, 0x158, 1);
    fill_panel_area(0x110, 0x48, 0x100, 0xa0, ((uint16_t)DG52BD.fill_colour));

    draw_scroll_text(title, 0x3c, 0x27, 0x1bc);
    draw_panel(0x110, 0xff, 0x100, 0x4c);

    if (DG4E67.freeform != 0)
        draw_wrapped_text((char *)DG1BCC.freeform_hint, 0x114, 0x104, 0xf8, 0x44);
    else
        draw_wrapped_text((char *)DG4E67.hint, 0x114, 0x104, 0xf8, 0x44);

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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x10]),
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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x12]),
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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x1f]),
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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x29]),
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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x21]),
                0x96, 0x8c, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x1d]),
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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x23]),
                0xc8, 0x8c, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x1d]),
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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[frame + 0x1b]),
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
    int16_t left, right, y;
    int16_t si, di;

    left  = (DG4E67.state == 0x4000) ? 0x26 : 0x25;
    right = (DG4E67.state == 0x2000) ? 0x28 : 0x27;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    for (si = 0x84; si < 0xb4; si = (int16_t)(si + 8))
        for (di = 0x5f; di <= 0x77; di = (int16_t)(di + 8))
            draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[0x2b]), si, di, 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[left]),  0x58, 0x5d, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[right]), 0x58, 0x6f, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[0x14]),      0x6e, 0x60, 0);

    y = 0x69;
    for (si = 1; si <= ((int16_t)DG4E67.master_level); si++) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[si + 0x14]),
                    GAME_MASTER_LEVEL_X.level_x[si - 1], y, 0);
        y = (int16_t)(y - 2);
    }

    restore_cursor_following();
}

/*
 * 0x11bd6
 *
 * A slider on the control panel: its track, its scale, and a knob whose
 * position comes from DGROUP 0x50b5.
 *
 * The knob's x is `0x50b5 * 0xa0 / 0x200 + 0x3d`, worked out as a long -
 * `mul16x16` then `long_divide` - because the product overflows a word before
 * the divide brings it back. The two sliders differ in that divisor, 0x200
 * against 0x80, so they are not the same slider at two positions.
 */
void paint_panel_f(void)
{
    int16_t at;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[0x7]), 0x41, 0xc8, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[0x9]), 0x3d, 0xe5, 0);

    at = (int16_t)long_divide(
             mul16x16(DG50AF.air, 0xa0), 0x200);

    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[0x6]),
                (int16_t)(at + 0x3d), 0xe0, 0);

    restore_cursor_following();
}

/*
 * 0x11c6b
 *
 * A slider on the control panel: its track, its scale, and a knob whose
 * position comes from DGROUP 0x50b3.
 *
 * The knob's x is `0x50b3 * 0xa0 / 0x80 + 0x3d`, worked out as a long -
 * `mul16x16` then `long_divide` - because the product overflows a word before
 * the divide brings it back. The two sliders differ in that divisor, 0x80
 * against 0x200, so they are not the same slider at two positions.
 */
void paint_panel_g(void)
{
    int16_t at;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[0x8]), 0x41, 0x114, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[0x9]), 0x3d, 0x131, 0);

    at = (int16_t)long_divide(
             mul16x16(DG50AF.gravity, 0xa0), 0x80);

    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[0x6]),
                (int16_t)(at + 0x3d), 0x12c, 0);

    restore_cursor_following();
}
