/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Running the machine**: the loop that steps it frame by frame, clearing
 * and restarting it, and asking the level's goal test.
 *
 * A module of the original's **code segment 0000** (`_TEXT`), image
 * 0x012ab..0x01476, split out of machine.c on 2026-09-27. Its start is
 * proven: `run_machine_loop` reaches stepmach.c's `step_machine` through
 * TLINK's `nop / push cs / call`. Its end is not: `finish_level` (0x02710)
 * reaches `run_machine_loop` the same way, so a module ends somewhere in
 * 0x013e9..0x025d8, and nothing in the calls or the data says where. It is
 * put where the goal tests begin, `check_goal` being the routine that calls
 * them through the level's pointer. No data of its own.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
0x012ab
 *
 * **The machine running** - the loop the game sits in after the start button,
 * and the counterpart to `game_screen_loop`. It runs while DGROUP 0x4e6b is
 * 0x2000 and, like that one, is left by writing into the same word.
 *
 * `clear_machine` first, then each frame: **latch the four sound channels**
 * down to 1 if they are non-zero, take the button and a key, let the regions
 * see the pointer, step the machine, and draw it.
 *
 * Those four at 0x52cd..0x52d3 are a request-and-acknowledge. Anything that
 * wants a sound stopped writes a non-zero value; this pins it to 1 on the way
 * in and, *after the frame has been presented*, stops that channel if it is
 * still 1. So a sound started during the same frame - which would have set the
 * word to something other than 1 - survives, and one left over from the
 * previous frame does not. Sound ids 1, 2, 9 and 0x0c.
 *
 * The pacing is the same spin on 0x44ef that the editor loop uses, waiting for
 * eight ticks; the difference is that here the elapsed count is **accumulated
 * into 0x4ea5**, so the machine's running time is measured rather than just
 * paced. 0x4ea7 counts frames.
 *
 * Three ways out, all by writing 0x4e6b: the goal test writing 0x200, the
 * left button writing 0x1000 - back to the editor - and 0x5772 writing 2. The
 * goal test is skipped entirely when 0x4e67 is set, which is freeform: a
 * machine with no puzzle has nothing to win. Scancode 0x2f, `V`, forces the
 * win in the same branch.
 *
 * `restart_machine` on the way out, whichever exit was taken.
 */
void run_machine_loop(void)
{
    clear_machine();
    DG4E67.elapsed_ticks = 0;
    TIMER.frame_budget = 0x2710;

    while (DG4E67.state == 0x2000) {
        if (((uint16_t)DG52BD.sound_request_01) != 0) DG52BD.sound_request_01 = 1;
        if (((uint16_t)DG52BD.sound_request_02) != 0) DG52BD.sound_request_02 = 1;
        if (((uint16_t)DG52BD.sound_request_09) != 0) DG52BD.sound_request_09 = 1;
        if (((uint16_t)DG52BD.sound_request_0c) != 0) DG52BD.sound_request_0c = 1;

        update_button_state();
        DG52ED.last_key = (uint8_t)(bios_read_key() >> 8);
        regions_handle_pointer(DG4E67.regions_play_ptr);

        step_machine();
        mark_parts_in_dirty_rects();
        step_loop_frames();
        replay_shapes();
        step_and_draw_machine(0);

        while ((int16_t)(0x2710 - ((uint16_t)TIMER.frame_budget)) < 8)
            ;
        DG4E67.elapsed_ticks += 0x2710 - TIMER.frame_budget;
        TIMER.frame_budget = 0x2710;

        present_frame(1);

        if (((uint16_t)DG52BD.sound_request_01) == 1) stop_music_or_effect(1);
        if (((uint16_t)DG52BD.sound_request_02) == 1) stop_music_or_effect(2);
        if (((uint16_t)DG52BD.sound_request_09) == 1) stop_music_or_effect(9);
        if (((uint16_t)DG52BD.sound_request_0c) == 1) stop_music_or_effect(0x0c);

        shift_all_histories();

        if (DG4E67.freeform == 0) {
            check_goal();
            if ((DG52ED.last_key) == SC_V)
                DG4E67.state = 0x200;
        }

        if (DG5768.button_left == 2)
            DG4E67.state = 0x1000;
        if (DG5768.button_right == 2)
            DG4E67.state = 2;

        DG4E67.machine_frames++;
    }

    restart_machine();
}

/*
 * 0x013e9
 *
 * Clear the machine down to nothing: reset every part, put the cursor back to
 * 0, take both pages' drawings off, and zero the handful of words the running
 * machine keeps - the one at 0x50d5, the animation phase at 0x4ea7, the four
 * at 0x52cd and the ten at 0x5458.
 */
void clear_machine(void)
{
    register int16_t si;

    reset_machine();
    select_cursor(0);
    erase_both_pages();

    DG50D3.dragged_part_ptr = 0;
    DG4E67.machine_frames = 0;
    DG52BD.sound_request_01 = DG52BD.sound_request_02 =
        DG52BD.sound_request_09 = DG52BD.sound_request_0c = 0;

    for (si = 0; si < 10; si++)
        DG5456.goal_condition[si] = 0;
}

/*
 * 0x01431
 *
 * The other half of the pair: fold the list at 0x4e58 back onto 0x4e56, reset
 * the machine, and hand it to the two routines that follow.
 */
void restart_machine(void)
{
    splice_list_4e58_onto_4e56();
    reset_machine();
    show_cursor_again();
    stop_music_or_effect(0);
}

/*
 * 0x0144e
 *
 * Step the counter at DGROUP 0x4e87, wrapping 0x2a00 back to 0x1c00. What it
 * counts is not established; the range is 0x1c00..0x29ff.
 *
 * **The wrap is unverified.** It needs 10,752 calls to reach, and over the two
 * intro screens the routine is called 428 times with the counter never above
 * 0x1ab. The branch is transcribed from the disassembly and has never been
 * run against the original.
 */
void step_loop_frames(void)
{
    DG4E67.loop_frames++;
    if (DG4E67.loop_frames == 0x2a00)
        DG4E67.loop_frames = 0x1c00;
}

/*
 * 0x01465
 *
 * Run this level's goal test. The round at DGROUP 0x4ebd is scaled by four -
 * the entries are far pointers - and called through `[bx + 0x2632]`.
 *
 * **The round is never 0**, so what the original addresses from 0x2632 is a
 * table that starts at 0x2636: `goal_test[0]` is puzzle 1, and the four bytes
 * at 0x2632 are the two bin-scroll repeat counters and nothing else.
 */
void check_goal(void)
{
    DG2630.goal_test[DG4E67.round_number - 1]();
}
