/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The frame**: whether the pointer is over the play area, the cursor
 * held off while something draws, the mouse buttons' edges, presenting the
 * back page, the clipping rectangle for each mode, the holiday flags, the
 * music and sound cues, and the checks that the heap has room.
 *
 * One module of the original's **code segment 0000** (`_TEXT`), image
 * 0x080b9..0x08546. From 0x08136 to 0x08497 its routines reach each other
 * with the bare `push cs / call` Borland writes only within one file, and
 * its `_DATA` is 0x286e, last frame's buttons, which TLINK put after
 * machine.c's. **Both ends are ours**: nothing reaches back across
 * 0x080b9..0x08136, and the module ends somewhere before 0x08eb5, whose
 * call back to `checked_free` goes through TLINK's `nop / push cs / call`
 * and so crosses into another file. The end is put where the subject
 * changes, before the pointer's regions.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 * JUDGE: data 0x286e..0x2870
 */
#include <stdlib.h>
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **Last frame's button state**, DGROUP 0x286e..0x2870, 0x02 bytes.
 * `update_button_state` keeps it so that "changed" cannot last two frames in
 * a row.
 */
struct machine_button_prev {
    int16_t   prev;          /* +0x00 [2] */
} PACKED;

/* Initialised, because the image has it in `_DATA`: without one Borland
   puts it in `_BSS`. */
struct machine_button_prev MACHINE_BUTTON_PREV DGROUP_AT(0x286e) = { 0 };

/*
 * 0x080b9
 *
 * Is the point at DGROUP 0x5782/0x5784 inside the play area? The box is
 * 8..0x237 across and 8..0x167 down, and both edges are inclusive.
 */
int16_t point_in_play_area(void)
{
    if (DG5768.pointer_x >= 8 && DG5768.pointer_x <= 0x237
        && DG5768.pointer_y >= 8 && DG5768.pointer_y <= 0x167)
        return 1;
    else
        return 0;
}

/*
 * 0x080e7
 *
 * Take the object off both pages.
 *
 * The two words it erases are the driver's own `VMDS.page_back_ptr` and `VMDS.page_front_ptr` -
 * the page being drawn into and the page on screen. They are page segments,
 * and `erase_object` takes them as handles, so `claim_page_slot` is what maps
 * a page to the record describing what is drawn on it. There is one such
 * record per page, which is why a double-buffered display has to erase twice.
 *
 * Clearing 0x52f2 first is what makes the sibling at 0x0810b, which sets it,
 * the other half of the pair; 0x08125 tests it before redrawing.
 */
void erase_both_pages(void)
{
    DG52ED.cursor_follows = 0;
    cursor_redraw_off_thunk();
    erase_object(VMDS.page_back_ptr);
    erase_object(VMDS.page_front_ptr);
}

/*
 * 0x0810b
 *
 * Turn the cursor back on and put it on the screen: set the flag at DGROUP
 * 0x52f2 and then call `restore_cursor_following`, which is guarded by that
 * same flag and so is bound to act.
 *
 * The pair to `cursor_redraw_off_thunk`, which is how the rest of the program
 * takes the cursor *off* the screen around a blit. This is the one that says
 * "whatever happened before, the cursor is wanted now" - where 0x08125 on its
 * own only puts back what a matching call had removed.
 */
void show_cursor_again(void)
{
    DG52ED.cursor_follows = 1;
    restore_cursor_following();
}

/*
 * 0x0811b
 *
 * A one-call forwarder to `cursor_redraw_off`, in the same segment, reached from
 * 48 sites. Whatever the flag means, this is how most of the program clears
 * it; the sibling at 0x08125 is how it is set again, guarded by 0x52f2.
 */
void cursor_redraw_off_thunk(void)
{
    cursor_redraw_off();
}

/*
 * 0x08125
 *
 * Let the cursor follow the mouse again, but only if DGROUP 0x52f2 says it
 * should. The pair to `cursor_redraw_off_thunk`, and the reason it is a routine
 * rather than a line is that the flag is what a caller sets to say "I turned
 * the cursor off, so put it back" - a caller that never turned it off leaves
 * 0x52f2 clear and this does nothing.
 */
void restore_cursor_following(void)
{
    if (DG52ED.cursor_follows != 0)
        cursor_redraw_on();
}

/*
 * 0x08136
 *
 * Advance the button state for one frame. Three states live in DGROUP 0x5774
 * and the previous frame's is kept at 0x286e:
 *
 *   0  not pressed
 *   1  held
 *   2  the frame of a change
 *
 * It waits for the frame first, takes the two flag bits, and then walks the
 * state machine. The last test - a 2 while the previous frame was also 2
 * becomes a 1 - is what stops "changed" lasting two frames in a row.
 *
 * The third branch is `cmp [0x5774],0 / je`, so anything that is not zero
 * becomes 1: the state is normalised, not merely tested.
 */
void update_button_state(void)
{
    int16_t prev;

    wait_and_latch_frame();
    prev = ((int16_t)DG5768.button_left);

    if (read_mouse_button(0))
        DG5768.button_left = 1;
    if (read_mouse_button(1))
        DG5768.button_right = 2;

    if (prev == 2 && MACHINE_BUTTON_PREV.prev != 1) {
        DG5768.button_left = 2;
    } else if (((int16_t)DG5768.button_left) == 1 && MACHINE_BUTTON_PREV.prev == 0) {
        DG5768.button_left = 2;
    } else if (((int16_t)DG5768.button_left) != 0) {
        DG5768.button_left = 1;
    } else {
        DG5768.button_left = 0;
    }

    if (((int16_t)DG5768.button_left) == 2 && MACHINE_BUTTON_PREV.prev == 2)
        DG5768.button_left = 1;

    MACHINE_BUTTON_PREV.prev = ((int16_t)DG5768.button_left);
}

/*
 * 0x081cc
 *
 * Present the frame. Three paths, chosen by two DGROUP flags: an optional
 * call to the routine at 0x0e34a first, then either a hook at 0x0b078 or the
 * driver's page flip. Called from sixteen places.
 */
void present_frame(uint16_t wait_retrace)
{
    if (DG52ED.stop_requested != 0)
        game_teardown(1);

    if (DG52ED.cursor_follows != 0)
        redraw_cursor_all();
    else
        vm_show_page(wait_retrace);
}

/*
 * 0x081f9
 *
 * Show what has just been painted, and then make the page that was on show the
 * one drawn into: 0x38a6 takes 0x38a4 and 0x38a8 takes 0x38a2, which is the
 * pair of page words swapping roles, and the whole 0x280 by 0x170 picture is
 * copied across so the new back page starts as a copy of what the player is
 * looking at.
 *
 * Its neighbour at 0x08229 does the same three things in the other order -
 * copy first, then present - and the port has both, because which order a
 * caller wants is the whole difference between them.
 *
 * (Filed here rather than with `paint_game_screen`, which is what used to call
 * it: 0x081f9 is in this segment.)
 */
void present_back_page(void)
{
    present_frame(1);

    VMDS.page_src_ptr = VMDS.page_front_ptr;
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    copy_rect_around_cursor(0, 0, 0x280, 0x170);
}

/*
 * 0x08229
 *
 * Put the whole picture back on the screen after something has been drawn over
 * it - which here is a message box.
 *
 * The two page words the driver keeps at 0x38a6 and 0x38a8 are set from 0x38a4
 * and 0x38a2, so both of the driver's current pointers name the pages the game
 * set up, and then the entire picture - 0,0 to 0x280 by 0x170, the 640 by 368
 * the line compare gives - is copied and presented.
 *
 * 0x170 and not 0x18f: the eighty rows below the split are the split screen and
 * are not this page's to repaint.
 */
void repaint_whole_screen(void)
{
    VMDS.page_src_ptr = VMDS.page_front_ptr;
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    copy_rect_around_cursor(0, 0, 0x280, 0x170);
    present_frame(1);
}

/*
 * 0x08259
 *
 * Set the four holiday flags from today's date. Nothing else reads the date;
 * these four words are the whole result.
 *
 *   DGROUP 0x4e81  the 14th of February
 *   DGROUP 0x4e7f  the 17th of March
 *   DGROUP 0x4e7d  the 31st of October
 *   DGROUP 0x4e7b  the 25th of December
 *
 * All four are cleared first, so a second call on an ordinary day undoes a
 * first one on a holiday.
 *
 * `dos_getdate` leaves the year at the local's +0 and DOS's packed DX at +2, so
 * the day is the byte at +2 and the month the byte at +3 - which is why the
 * comparisons read a byte at a time rather than a word.
 */
void set_holiday_flags(void)
{
    uint8_t d[4];

    DG4E67.holiday_valentine = DG4E67.holiday_stpatrick
        = DG4E67.holiday_halloween = DG4E67.holiday_christmas = 0;
    dos_getdate((uint8_t *)d);
    if (d[3] == 2 && d[2] == 0x0e)
        DG4E67.holiday_valentine = 1;
    if (d[3] == 3 && d[2] == 0x11)
        DG4E67.holiday_stpatrick = 1;
    if (d[3] == 0xa && d[2] == 0x1f)
        DG4E67.holiday_halloween = 1;
    if (d[3] == 0xc && d[2] == 0x19)
        DG4E67.holiday_christmas = 1;
}

/*
 * 0x082c3
 *
 * Set the clip box: to the saved rectangle at DGROUP 0x52d7..0x52dd when the
 * mode word at 0x4e6b is any of seven values, and to a fixed one otherwise.
 *
 * The seven are single bits - 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000,
 * 0x8000 - but they are compared **for equality**, one at a time, not tested
 * as a mask, so a word with two of them set matches none. Transcribed as seven
 * compares rather than folded into a mask test.
 *
 * The saved rectangle is stored in descending order - 0x52dd is the left edge
 * and 0x52d7 the bottom - which is worth saying because it looks like a
 * transcription error otherwise.
 *
 * The fixed box is 0x110,0x48 to 0x20f,0xe7: 256 wide by 160 tall.
 */
void set_clip_for_mode(void)
{
    /* the screen state, not a video mode */
    if (DG4E67.state == 0x2000 || DG4E67.state == 0x1000
        || DG4E67.state == 0x200 || DG4E67.state == 0x8000
        || DG4E67.state == 0x4000 || DG4E67.state == 0x800
        || DG4E67.state == 0x400) {
        VMDS.clip_left = DG52BD.saved_clip_left;
        VMDS.clip_right = DG52BD.saved_clip_right;
        VMDS.clip_top = DG52BD.saved_clip_top;
        VMDS.clip_bottom = DG52BD.saved_clip_bottom;
    } else {
        VMDS.clip_left = 0x110;
        VMDS.clip_right = 0x20F;
        VMDS.clip_top = 0x48;
        VMDS.clip_bottom = 0xE7;
    }
}

/*
 * 0x08332
 *
 * Set the clipping box to the **play area**: 0,0 to 639,367. The same four
 * words as `set_clip_full_screen` below and thirty-two rows shorter, which is
 * the strip along the bottom the game keeps for itself.
 */
void set_clip_play_area(void)
{
    VMDS.clip_top = VMDS.clip_left = 0;
    VMDS.clip_right = 0x27F;
    VMDS.clip_bottom = 0x16F;
}

/*
 * 0x0834b
 *
 * Set the clipping box to the whole visible screen: 0,0 to 639,399. The
 * bottom is 0x18f, which is the blanking line the CRTC is programmed with -
 * so the clip box is the *visible* 400 rows, not the 480 the mode scans.
 */
void set_clip_full_screen(void)
{
    VMDS.clip_top = VMDS.clip_left = 0;
    VMDS.clip_right = 0x27F;
    VMDS.clip_bottom = 0x18F;
}

/*
 * 0x08364
 *
 * Make one piece of music the current one: stop and free whatever was playing,
 * open the new one and start it, and remember it at DGROUP 0x52d5.
 *
 * Asking for what is already playing does nothing at all - the test is first -
 * and -1 means "nothing", both as what was playing and as what is wanted.
 */
void select_music(register int16_t id)
{
    if (id != DG52BD.music_now) {
        if (DG52BD.music_now != -1) {
            stop_music_or_effect(DG52BD.music_now);
            remove_and_free_records(DG52BD.music_now);
        }
        if (id != -1) {
            open_sound_file((char *)FILEREC_PTR(DG52ED.tim_sx_ptr), id);
            play_sound(id);
        }
        DG52BD.music_now = id;
    }
}

/*
 * 0x083ab
 *
 * Play a sound, and hold six of them back when the music is off.
 *
 * Ids 4, 9, 0x10, 0x12, 0x13 and 0x14 go out only when DGROUP 0x4ec1 is
 * non-zero - that is the setting TIM.CFG carries, and it defaults to 6 when
 * there is no file. Every other id plays whatever the setting says.
 *
 * The six are compared one at a time rather than looked up, and both branches
 * end in the same call: the test decides *whether*, never *what*.
 */
void play_sound(register int16_t id)
{
#ifndef __TURBOC__
    /*
     * NOT PART OF THE TRANSCRIPTION - an observation hook, and the only line
     * in this routine that is not the original's. It is a no-op in `tim`
     * (devstub.c) and reports under `TIM_TRACE=sfx` in `devtim`, the same
     * arrangement `dev_flip_dump` has at the page flip. It changes nothing the
     * game does; it says what the game asked for.
     */
    dev_sound_played(id);
#endif
    if (id == 0x10 || id == 0x12 || id == 9 || id == 0x13 || id == 0x14
        || id == 4) {
        if (DG4E67.master_level != 0)
            start_sequence_by_id(id);
    } else
        start_sequence_by_id(id);
}

/*
 * 0x083ea
 *
 * Stop a sound, or all of them.
 *
 * A number of its own stops that one sequence. Zero stops all twenty of the
 * effects, 1 to 0x14; -2 stops those *and* the seven pieces of music, 0x3e9 to
 * 0x3ef. Nothing else is a special value.
 */
void stop_music_or_effect(register int16_t id)
{
    register int16_t si;

    if (id == 0 || id == -2) {
        for (si = 1; si <= 0x14; si++)
            stop_sequences(si);
        if (id == -2)
            for (si = 0x3e9; si <= 0x3ef; si++)
                stop_sequences(si);
    } else
        stop_sequences(id);
}

/*
 * 0x08432
 *
 * Is there room for another part? Answers 1 for yes and 0 for no, and says so
 * on screen when the answer changes.
 *
 * Three bands of `heap_largest_free`, with 0x4e83 remembering whether the
 * player has already been told:
 *
 *   under 0x0fa0   "OUT OF MEMORY" / "You can't place any more parts.", and 0
 *   under 0x1388   "MEMORY LOW" / "Memory is getting low...", once, and 1
 *   over  0x1770   the warning is armed again by clearing 0x4e83
 *
 * The gap between 0x1388 and 0x1770 is hysteresis: the flag is set at the
 * lower and only cleared at the higher, so a machine hovering near the edge is
 * not told twice.
 *
 * A message box has to be painted over and taken away again, which is what
 * `redraw_machine_area` and `repaint_whole_screen` are doing after each, and
 * `update_button_state` swallows the click that dismissed it.
 */
int16_t check_room_for_part(void)
{
    register uint16_t si;

    si = heap_largest_free();
    if (si < 0x0fa0) {
        show_message_box(DG1BCC.out_of_memory, (char *)DG1BCC.you_cant_place_any);
        DG4E67.memory_warned = 1;
        redraw_machine_area();
        repaint_whole_screen();
        update_button_state();
        return 0;
    } else if (si < 0x1388 && DG4E67.memory_warned == 0) {
        show_message_box(DG1BCC.memory_low, (char *)DG1BCC.memory_is_getting_low);
        DG4E67.memory_warned = 1;
        redraw_machine_area();
        repaint_whole_screen();
        update_button_state();
    } else if (si > 0x1770)
        DG4E67.memory_warned = 0;
    return 1;
}

/*
 * 0x084b0
 *
 * The largest allocation the heap could still satisfy.
 *
 * Borland's `heapwalk` is run over every block, and a free one offers its size
 * less the four bytes of its own header. The walk's record lives in this
 * routine's own frame - `lea ax,[bp-6]` is what is passed - so the three words
 * the callee fills are locals here, which is why the frame is eight bytes for
 * what looks like two variables.
 *
 * Then the space that has never been in a block at all: the running word at
 * [bp-8] holds the *last* block's address plus its size, which is the top of
 * the heap, and 0x52fc is what the stack is reserved below - `game_start` sets
 * it to 0x800. `neg` and subtract gives the gap between them, and the answer is
 * whichever of the two is bigger.
 *
 * The record is the original's `[bp-6]`, the three words `heapwalk` fills.
 *
 * So a heap with no free block still answers what a fresh one would give.
 */
int16_t heap_largest_free(void)
{
    register uint16_t best;
    register uint16_t gap;
    struct heapinfo info;               /* [bp-6], the walk record */
    int16_t total;                      /* [bp-8] */

    info.block_ptr = 0;
    best = 0;
    total = 0;
    while (heapwalk(&info) == 2) {
        total = info.block_ptr + info.size;
        if (!info.in_use && (uint16_t)(info.size - 4) > best)
            best = info.size - 4;
    }
    gap = -(int16_t)DG52ED.stack_floor - total;
    if (gap > best)
        best = gap;
    return best;
}

/*
 * 0x08510
 *
 * Free a block, with the heap checked either side of it. The check is the one
 * that hangs on a broken heap, so a free that corrupts the ring stops the game
 * at the free rather than somewhere unrelated later.
 */
void checked_free(uint8_t *p)
{
    heap_check_or_hang();
    heap_free_far(p);
    heap_check_or_hang();
}

/*
 * 0x08528
 *
 * Check the heap, and **stop dead** if it is broken.
 *
 * The stop is written as a loop rather than a halt: `si` starts at 2, adds 2,
 * and is compared against 3 - which it steps over on every pass and never
 * equals. That is a deliberate hang, not a bug, and the port keeps it as one:
 * a corrupt heap that carried on would draw a wrong picture some seconds later
 * and look like a blitter fault.
 */
void heap_check_or_hang(void)
{
    register int16_t si;

    if (heap_check() == -1)
        for (si = 2; si != 3; si += 2)
            ;
}
