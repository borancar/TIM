/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The intro and the copy protection**: the Sierra screen, the title and
 * credits animations, the question from the manual, and the loop that runs
 * a game.
 *
 * The second module of the original's **code segment 0dff**, image
 * 0x0e4be..0x0ef19. Its data is DGROUP 0x2370..0x258c: the credit roll and
 * the copy protection's answers, and then its literal pool, the intro's file
 * names. Functions are in address order and each carries the image offset it
 * was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -d
 * JUDGE: data 0x2370..0x258c
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
 * **The intro's credit roll**, at DGROUP 0x2370 - where each of the animated
 * pieces is put and which bitmap it is. `game_intro` walks it two entries at a
 * time, drawing a pair on each frame it is given, and stops on an entry whose
 * x is zero.
 *
 * Sixty-two entries and that terminator, measured out of the image; the y is
 * an offset from 415, which is where the strip sits on the screen.
 */
struct intro_step {
    int16_t   x;                  /* +0x00  zero ends the roll */
    int16_t   y;                  /* +0x02  415 is added before drawing */
    int16_t   bitmap;             /* +0x04  an index into the intro's list */
} PACKED;

/* DGROUP 0x2370..0x24ea, 0x17a bytes. */
struct game_intro_steps {
    struct intro_step step[63];   /* +0x00 [0x17a] */
} PACKED;

struct game_intro_steps g_game_intro_steps = {
    {
        { 632, 14, 3 },
        { 640, 47, 7 },
        { 629, 13, 0 },
        { 640, 47, 7 },
        { 629, 11, 1 },
        { 640, 47, 7 },
        { 620, 11, 2 },
        { 640, 47, 7 },
        { 602, 11, 3 },
        { 640, 47, 7 },
        { 594, 13, 4 },
        { 640, 47, 7 },
        { 593, 13, 0 },
        { 640, 47, 7 },
        { 593, 11, 1 },
        { 632, 47, 7 },
        { 584, 11, 2 },
        { 626, 47, 7 },
        { 566, 11, 3 },
        { 619, 47, 7 },
        { 558, 13, 4 },
        { 612, 47, 7 },
        { 557, 14, 0 },
        { 605, 47, 7 },
        { 557, 12, 1 },
        { 598, 47, 7 },
        { 548, 12, 2 },
        { 591, 47, 7 },
        { 530, 12, 3 },
        { 584, 47, 7 },
        { 522, 14, 4 },
        { 577, 47, 7 },
        { 521, 14, 0 },
        { 570, 47, 7 },
        { 521, 13, 1 },
        { 563, 47, 7 },
        { 512, 13, 2 },
        { 557, 47, 7 },
        { 495, 13, 3 },
        { 550, 47, 7 },
        { 487, 15, 4 },
        { 543, 47, 7 },
        { 486, 14, 0 },
        { 536, 47, 7 },
        { 486, 12, 1 },
        { 529, 47, 7 },
        { 477, 12, 2 },
        { 522, 47, 7 },
        { 460, 12, 3 },
        { 515, 47, 7 },
        { 451, 14, 4 },
        { 508, 47, 7 },
        { 451, 14, 4 },
        { 508, 47, 7 },
        { 451, 14, 5 },
        { 501, 47, 7 },
        { 451, 14, 5 },
        { 501, 47, 7 },
        { 451, 14, 5 },
        { 501, 47, 7 },
        { 451, 10, 6 },
        { 494, 47, 7 },
    },
    /* step */
};

/*
 * **The copy-protection answers**, DGROUP 0x24ea..0x254a, 0x60 bytes: three rows of sixteen
 * part numbers, one row per icon the page asks for, which
 * `copy_protect_screen` compares against the three the player picked. The
 * rows are 0x20 apart in the code that read them, which is the sixteen.
 */
struct game_copy_protection {
    int16_t   answer[3][16];      /* +0x00 [0x60]  [icon][page] */
} PACKED;

struct game_copy_protection g_game_copy_protection = {
    {
        {
            0x000f, 0x0024, 0x001d, 0x000f, 0x0007, 0x0013, 0x0019, 0x0010,
            0x0010, 0x0018, 0x0011, 0, 0x0009, 0x000d, 0x0011, 0x0013,
        },
        {
            0x000c, 0x0013, 0x0018, 0x001d, 0x0011, 0x001b, 0x000c, 0,
            0x0007, 0x0012, 0x0010, 0x000f, 0x0012, 0x0003, 0x000d, 0x001b,
        },
        {
            0x001d, 0x001b, 0, 0x000c, 0x001d, 0x0024, 0x001d, 0x001d,
            0x0019, 0x000d, 0, 0x0010, 0x0018, 0x000c, 0x0018, 0x000d,
        },
    },
    /* answer */
};

/*
 * 0x0e4be
 *
 * The intros: the Sierra logo, then the title screen and the credits, looping
 * between the last two until a key or a mouse button ends it. `main` calls this
 * second, after the bring-up.
 *
 * **The Sierra logo** is a table of six-byte entries at DGROUP 0x2370 - an x, a
 * y offset and a bitmap index - walked one entry a frame with a rectangle
 * cleared behind each. DGROUP 0x44ef is a frame budget that starts at 0x2710
 * and is compared against as the animation runs, so a slow machine drops
 * entries rather than falling behind. The `add ax, 0xff88` that sets the limit
 * is a subtraction of 0x78 written as an addition, which is what the compiler
 * does with a negative constant.
 *
 * **The title loop** is the shape worth reading. DGROUP 0x4e6b is a state, and
 * 0x8000 means the title and anything else the credits, chosen by which .gkc
 * file is loaded and remembered in the local at [bp-0xa]. Each pass draws,
 * presents, polls, and counts DGROUP 0x4ea7 up; at 0x110 frames for the title
 * or 0x152 for the credits the pass ends and the other one starts. A key or a
 * button - which is what makes DGROUP 0x5772 or 0x5774 become 2 - drops
 * straight out.
 *
 * **The pages swap roles twice.** For the logo both 0x38a2 and 0x38a4 are
 * A000, so the logo is drawn and shown on the same page; the game's screens
 * then go back to the usual A000/A820 pair, and the last lines set 0x38a4 to
 * 0xa190 and 0x38a2 to 0xa8c0 - the two pages offset by 0x190 paragraphs, which
 * is the 400-line screen sitting inside the 480-line mode.
 */
void game_intro(void)
{
    int16_t budget;                         /* [bp-2] */
    int16_t stage;                          /* [bp-4] */
    int16_t running;                        /* [bp-6] */
    int16_t frame;                          /* [bp-8] */
    uint16_t which;                         /* [bp-0xa] */
    struct bitmap **bitmaps;                /* [bp-0xc] */
    struct bitmap **gkc;                    /* [bp-0xe] */
    register int16_t si;
    register const struct intro_step *step;

    g_timer.frame_budget = 0x2710;

    set_palette_pointer(g_pal_black);      /* black.pal */

    bitmaps = load_bitmaps(WRITABLE_LITERAL("sierra.bmp"));

    g_vmds.page_front = g_vmds.page_back = 0xa000;

    for (si = 0; si < 3; si++)
        present_frame(1);

    g_vmds.page_back += 0x12c;
    g_round_state = 0x8000;
    g_music_now = -1;

    stage = 0;
    while (stage != 4 && g_round_state == 0x8000) {
        if (stage == 0) {
            g_vmds.page_dst = g_vmds.page_front;
            cursor_redraw_off_thunk();
            load_screen(WRITABLE_LITERAL("sierra.scr"));
            set_palette_pointer(g_pal_sierra);  /* sierra.pal */
            stage = 1;
            budget = g_timer.frame_budget + 0xff88;
            step = &g_game_intro_steps.step[0];

        /*
         * `jl` - the step runs while the counter is *under* the budget, and
         * the budget is set 0x78 below whatever the counter was. So nothing
         * moves until DGROUP 0x44ef counts down, which is the timer's doing:
         * this is the frame pacing, not a frame counter.
         */
        } else if (step->x != 0 && g_timer.frame_budget + 6 < budget) {
            g_vmds.clip_enabled = 1;
            g_vmds.clip_left = g_vmds.clip_top = 0;
            g_vmds.clip_right = 639;
            g_vmds.clip_bottom = 479;
            g_vmds.fill_enabled = 1;
            g_vmds.second_colour = g_vmds.fill_colour = 0;

            g_vmds.page_dst = g_vmds.page_back;
            fill_rect(448, 415, 192, 65);

            draw_bitmap(bitmaps[step->bitmap],
                        step->x, step->y + 415, 0);

            if (step->bitmap == 0)
                play_sound(0x14);

            step++;

            draw_bitmap(bitmaps[step->bitmap],
                        step->x, step->y + 415, 0);

            step++;

            g_vmds.page_dst = g_vmds.page_front;
            g_vmds.page_src = g_vmds.page_back;
            copy_rect_thunk(448, 425, 192, 75);

            budget = g_timer.frame_budget;

            if (step->x == 0) {
                play_sound(0x13);
                stage = 4;
            }
        }

        regions_handle_pointer(g_regions_play);
        update_button_state();

        if (g_stop_requested != 0)
            game_teardown(1);
    }

    free_bitmaps_thunk(bitmaps);

    g_saved_clip_top = g_saved_clip_left = 0;
    g_saved_clip_right = 0x27f;
    g_saved_clip_bottom = 0x18f;

    load_all_parts();

    gkc = load_bitmaps(WRITABLE_LITERAL("corners.bmp"));

    for (si = 0x37; si <= 0x39; si++)
        load_part_bitmap(si);

    set_palette_pointer(g_pal_black);      /* black.pal */

    g_vmds.page_front = 0xa000;
    g_vmds.page_back = 0xa820;

    for (si = 0; si < 3; si++)
        present_frame(1);

    g_vmds.page_dst = 0xa000;
    vm_set_display_lines(0x18f);
    update_button_state();

    if (g_pointer.button_left == 2 || g_pointer.button_right == 2)
        which = g_round_state = 2;

    frame = 0x3f6;

    if (g_round_state == 0x8000) {
        which = 0x8000;
        g_round_state = 0x2000;
    } else {
        which = g_round_state = 2;
    }

    while (which == 0x8000 || which == 0x4000) {
        cursor_redraw_off_thunk();

        /*
         * The two animations are placed differently: the title screen sits
         * eight pixels left of the origin, the credits sixteen. The compiler
         * merged the two arms' identical stores into one after them.
         */
        if (which == 0x8000) {
            load_animation(WRITABLE_LITERAL("title.gkc"));
            g_origin_c_x = g_origin_b_x = g_origin_x = -8;
        } else {
            load_animation(WRITABLE_LITERAL("credits.gkc"));
            g_origin_c_x = g_origin_b_x = g_origin_x = -16;
        }
        g_origin_c_y = g_origin_b_y = g_origin_y = 0;

        clear_machine();
        set_clip_full_screen();

        g_vmds.page_dst = g_vmds.page_back;
        g_vmds.second_colour = g_vmds.fill_colour = g_fill_colour;
        g_vmds.fill_enabled = 1;

        fill_rect(0, 0, 640, 400);

        step_and_draw_machine(1);
        draw_frame_corners(gkc);
        present_frame(1);

        g_vmds.page_src = g_vmds.page_front;
        g_vmds.page_dst = g_vmds.page_back;
        copy_rect_around_cursor(0, 0, 640, 400);

        if (which == 0x8000)
            select_music(0x3e9);
        else
            select_music(frame);

        running = 1;

        while (running != 0) {
            if (g_sound_request_01 != 0) g_sound_request_01 = 1;
            if (g_sound_request_02 != 0) g_sound_request_02 = 1;
            if (g_sound_request_09 != 0) g_sound_request_09 = 1;
            if (g_sound_request_0c != 0) g_sound_request_0c = 1;

            update_button_state();
            step_machine();
            mark_parts_in_dirty_rects();
            step_loop_frames();
            replay_shapes();

            step_and_draw_machine(0);
            draw_frame_corners(gkc);
            present_frame(1);

            if (g_machine_frames == 0)
                set_palette_pointer(g_pal_tim);  /* tim.pal */

            if (g_sound_request_01 == 1) stop_music_or_effect(1);
            if (g_sound_request_02 == 1) stop_music_or_effect(2);
            if (g_sound_request_09 == 1) stop_music_or_effect(9);
            if (g_sound_request_0c == 1) stop_music_or_effect(0xc);

            shift_all_histories();

            if (g_pointer.button_left == 2 || g_pointer.button_right == 2) {
                which = g_round_state = 2;
                running = 0;
            }

            g_machine_frames++;

            if (which == 0x8000) {
                if ((int16_t)g_machine_frames > 0x110)
                    running = 0;
            } else if ((int16_t)g_machine_frames > 0x152) {
                running = 0;
            }
        }

        release_part_queue();
        reset_machine();

        for (si = 1; si <= 0x14; si++)
            stop_music_or_effect(si);

        free_all_lists();

        if (which == 0x8000) {
            which = 0x4000;
        } else if (which == 0x4000) {
            which = 0x8000;
            frame++;
            if (frame > 0x3f8)
                frame = 0x3ea;
        }
    }

    for (si = 0x37; si <= 0x39; si++)
        free_part_bitmap(si);

    g_icons_bmp = load_bitmaps(WRITABLE_LITERAL("icons.bmp"));
    g_round_state = 0x8000;

    copy_protect_screen(gkc);

    g_round_state = 2;

    set_palette_pointer(g_pal_black);      /* black.pal */
    present_frame(1);

    free_bitmaps_thunk(gkc);

    stop_music_or_effect(0);
    show_cursor_again();

    g_vmds.page_front = 0xa190;
    g_vmds.page_back = 0xa8c0;
    g_vmds.screen.screen_height = 367;

    vm_set_display_lines(0x1bf);
    vm_set_line_compare(0x16f);

    for (si = 0; si < 3; si++)
        present_frame(1);

    g_saved_clip_left = 8;
    g_saved_clip_right = 0x237;
    g_saved_clip_top = 8;
    g_saved_clip_bottom = 0x167;
}

/*
 * 0x0ea39
 *
 * **The copy-protection screen.** Thirty-two part icons in a grid of eight,
 * three empty slots, an OK button, and the line "Please select, in order, the
 * three parts listed on page N of the user's manual."
 *
 * The page is `(0x44ef & 0xf) + 1` - taken from the frame counter, so it is a
 * different page each time and the answer cannot be memorised. And the answer
 * is **a table**: the three parts wanted for page N are the Nth words of the
 * three arrays at DGROUP 0x24ea, 0x250a and 0x252a, sixteen pages each.
 *
 * The grid skips the parts that are not real: index `si` shows part `si`, or
 * `si + 1` past 0x13, with 0x1e becoming 0x23 and 0x20 becoming 0x24. Those
 * are the same holes `build_part_list` leaves - 0x14, 0x29 and 0x31 are never
 * offered - and the click at the bottom runs the identical remap on the cell it
 * lands in, so the two agree by construction rather than by a shared table.
 *
 * A click inside the grid writes the part into the next of the three slots and
 * wraps after the third, so a fourth click starts over. A click on the OK
 * button at 0x248,0x158 calls `game_teardown`. Tab - scancode 0x0f out of
 * `bios_read_key` - walks a highlight around the grid and onto the button.
 *
 * **The copy this project was built from is cracked, in one byte, and this
 * routine is where.** The wait loop is entered on the wrong side and exits at
 * once: after `[bp-0x12]` is cleared the routine jumps to 0x0eddd, which
 * *sets* it to 1, and 0x0ede2 leaves when it is not zero. So the screen is
 * drawn and the routine returns without ever polling, and any answer passes.
 *
 * The arithmetic is exact rather than a reading of the listing. The bytes at
 * 0x0ec79 are `e9 61 01`, the next instruction is at 0x0ec7c, and
 * 0x0ec7c + 0x161 is 0x0eddd. The loop's **test** is at 0x0ede2 - `cmp
 * [bp-0x12], 0`, `jne` out at 0x0ede6, `jmp` back to the body at 0x0ede8 -
 * which is where a Borland `while` enters, and 0x0ede2 - 0x0ec7c is 0x166. So
 * the shipped byte is 0x61 where the compiler emitted 0x66:
 *
 *     e9 66 01   jmp 0x0ede2    the test, and the loop runs
 *     e9 61 01   jmp 0x0eddd    `done = 1`, and it does not
 *
 * One byte, landing on a real instruction boundary either way, turning the
 * check into a formality. No compiler emits a jump into the middle of a loop
 * body to set its own exit flag.
 *
 * **The default here is the binary's, crack and all**, so this file stays a
 * transcription: `goto check`, the loop never runs, and any answer passes -
 * which is what the shipped bytes do and what `out/TIM.img` still holds.
 *
 * **Both jumps are kept, under `TIM_COPY_PROTECTION`**, so the crack is
 * readable here rather than only in a commit message. Defining it takes the
 * jump to the loop's test instead and the screen waits for a real answer -
 * the same edit in C that 0x66 is in the binary, and a **deliberate
 * deviation** rather than a transcription. Pair it with `tools/uncrack.py`,
 * which takes the byte out of the image and the executable: an un-cracked port
 * against a cracked reference disagrees on this screen, and every screen
 * comparison that crosses it fails, so the two want moving together. The
 * judge and tools/link.py compare this build, against their own copy of the
 * image with the byte put back: the game as it was built, which the link
 * reproduces whole (`--cracked` for the default and the shipped byte).
 *
 * The labels are guarded too, not just the `goto`. An unused label is a
 * `-Wall` warning, and the build is warning-clean.
 *
 * The distance between the two targets is worth keeping in view:
 * 0x0ede2 - 0x0eddd is 5, and `c7 46 ee 01 00` - `mov word [bp-0x12], 1` -
 * is five bytes. So the crack is exactly "take the length of the `done = 1`
 * instruction off the entry jump", landing it *on* that instruction instead of
 * after it. One byte, no relocation, no change of size.
 *
 * The loop body was transcribed all along, because it is there and has to be
 * right if it is ever reached; now it is reached.
 *
 * One earlier note is withdrawn. It said the original "does not run this
 * routine at all" while the screen is up, measured as zero addresses executed
 * in 0x0ea39..0x0edf0 from a snapshot taken there. That is what a routine that
 * has already drawn its screen and returned looks like - which is exactly what
 * the crack makes it do - so the measurement was of the patch, not of the
 * game.
 */
void copy_protect_screen(struct bitmap **bitmaps)
{
    int16_t x;                  /* [bp-2] */
    int16_t y;                  /* [bp-4] */
    int16_t part;               /* [bp-6] */
    int16_t highlight;          /* [bp-8] */
    int16_t answers[3];         /* [bp-0xe] */
    int16_t slot;               /* [bp-0x10] */
    int16_t done;               /* [bp-0x12] */
    int16_t page;               /* [bp-0x14] */
    char numbuf[16];            /* [bp-0x24] */
    /* The message is 79 bytes and a NUL with a one-digit page, and one more
       from page 10 - which runs into `numbuf`, spent by then. */
    char msg[OVERRUN(80, 1)];   /* [bp-0x74] */
    register int16_t si;
    register int16_t pick;      /* the part a click lands on */

    g_vmds.screen.screen_height = 399;

    for (si = 0; si < 3; si++)
        answers[si] = -1;

    highlight = -1;
    slot      = 0;
    page      = g_timer.frame_budget & 0xf;

    set_clip_full_screen();
    g_vmds.page_dst = g_vmds.page_back;
    g_vmds.second_colour = g_vmds.fill_colour = g_fill_colour;
    g_vmds.fill_enabled   = 1;

    cursor_redraw_off_thunk();
    fill_rect(0, 0, 640, 400);
    restore_cursor_following();

    draw_frame_corners(bitmaps);

    draw_panel(48, 16, 544, 224);         /* the panel */
    draw_panel(192, 300, 64, 48);         /* the three slots */
    draw_panel(288, 300, 64, 48);
    draw_panel(384, 300, 64, 48);
    draw_panel(584, 344, 32, 32);        /* the OK button */

    cursor_redraw_off_thunk();
    draw_bitmap(g_panel_art[0x12], 588, 350, 0);
    restore_cursor_following();

    itoa(page + 1, numbuf, 10);
    strcpy(msg, g_messages.please_select_in_order);
    strcat(msg, numbuf);
    strcat(msg, g_messages.of_the_users_manual);
    draw_scroll_text(msg, 64, 262, 512);

    for (si = 0; si < 0x20; si++) {
        x    = (int16_t)(((si % 8) << 6) + 64);
        y    = (int16_t)((si / 8) * 48 + 32);
        part = si;
        if (part > 0x13)
            part++;
        if (part == 0x1e)
            part = 0x23;
        if (part == 0x20)
            part = 0x24;

        cursor_redraw_off_thunk();
        draw_bitmap_centred(g_icons_bmp[part],
                            x, y, 64, 48);
        restore_cursor_following();
    }

    select_music(page + 0x3e9);
    present_frame(1);

    g_vmds.page_src = g_vmds.page_front;
    g_vmds.page_dst = g_vmds.page_back;
    copy_rect_around_cursor(0, 0, 640, 400);
    set_palette_pointer(g_pal_tim);
    show_cursor_again();

    done = 0;
#ifdef TIM_COPY_PROTECTION
    goto test;                  /* e9 66 01: jmp 0x0ede2, the loop's test */
#else
    goto check;                 /* e9 61 01: jmp 0x0eddd, `done = 1` */
#endif

    do {
        update_button_state();

        g_last_key = (uint8_t)(bios_read_key() >> 8);
        if ((g_last_key) == SC_TAB) {          /* Tab walks the highlight */
            highlight++;
            if (highlight == 0x21)
                highlight = 0;
            if (highlight == 0x20)
                move_pointer_to(616, 392);    /* the button */
            else
                move_pointer_to((uint16_t)(((highlight % 8) << 6) + 0x50),
                          (uint16_t)((highlight / 8) * 48 + 48));
        }

        select_cursor((g_pointer.pointer_x >= 584 && g_pointer.pointer_y >= 344)
                      ? 0x15 : 0);

        if (((int16_t)g_pointer.button_left) == 2) {            /* the frame of a click */
            if (g_pointer.pointer_x >= 64 && g_pointer.pointer_x < 576
                && g_pointer.pointer_y >= 32 && g_pointer.pointer_y < 224) {
                pick = (g_pointer.pointer_x - 64) / 64
                       + (g_pointer.pointer_y - 32) / 48 * 8;
                if (pick > 0x13)
                    pick++;
                if (pick == 0x1e)
                    pick = 0x23;
                if (pick == 0x20)
                    pick = 0x24;

                answers[slot] = pick;
                draw_answer_slot(g_icons_bmp[pick],
                                 slot);
                slot++;
                if (slot == 3)
                    slot = 0;
            }

            if (g_pointer.pointer_x >= 584 && g_pointer.pointer_y >= 344)
                game_teardown(1);
        }

        present_frame(1);

        if (g_game_copy_protection.answer[0][page] == answers[0]
            && g_game_copy_protection.answer[1][page] == answers[1]
            && g_game_copy_protection.answer[2][page] == answers[2])
#ifndef TIM_COPY_PROTECTION
check:
#endif
            done = 1;

        /* The loop's test is where a `while` enters, and the empty statement
           is the label's way of standing on it. */
#ifdef TIM_COPY_PROTECTION
test:
#endif
        ;
    } while (done == 0);
}

/*
 * 0x0edf1
 *
 * **Draw the part you just picked into one of the three answer slots** on the
 * copy-protection screen. `bmp` is the part's icon out of the table
 * `copy_protect_screen` loaded into DGROUP 0x4ec7, and `slot` is 0, 1 or 2.
 *
 * The slot's box is 0x40 by 0x30 at y 0x12c, and its x is `slot * 0x60 + 0xc0`
 * - so the three sit 0x60 apart starting at 0xc0, which is 0x20 wider than the
 * boxes and leaves the gap between them.
 *
 * The panel is drawn first and the icon centred into it afterwards, so a part
 * whose picture is smaller than the box is not left with the previous pick's
 * pixels around it. `cursor_redraw_off_thunk` and `restore_cursor_following`
 * bracket the drawing the way they do everywhere the cursor might be over what
 * is being painted.
 *
 * Then the page pointers are put back - 0x38a6 from 0x38a4 and 0x38a8 from
 * 0x38a2 - and the whole screen is copied around the cursor, because the frame
 * just presented was drawn on the other page.
 *
 * **Unreachable until 2026-09-06.** It is called only from the click inside the
 * grid, and the crack removed from `copy_protect_screen` returned before the
 * screen ever polled, so no click ever arrived. Restoring that one byte is what
 * made this reachable, and it aborted on the first click.
 */
void draw_answer_slot(struct bitmap *bmp, uint16_t slot)
{
    int16_t x;

    g_vmds.page_dst = g_vmds.page_back;

    x = (int16_t)(slot * 96 + 192);

    draw_panel(x, 300, 64, 48);
    cursor_redraw_off_thunk();
    draw_bitmap_centred(bmp, x, 300, 64, 48);
    restore_cursor_following();
    present_frame(1);

    g_vmds.page_src = g_vmds.page_front;
    g_vmds.page_dst = g_vmds.page_back;
    copy_rect_around_cursor(0, 0, 640, 400);
}

/*
 * 0x0ee6e
 *
 * Draw the four corner pieces of the intro's frame, from the four bitmaps the
 * record holds: top left at the origin, top right at 0x262, bottom left at
 * 0x175, bottom right at both. The positions are constants in the code, so the
 * frame is the same size whatever is inside it.
 */
void draw_frame_corners(struct bitmap **rec)
{
    cursor_redraw_off_thunk();

    draw_bitmap(rec[0], 0, 0, 0);
    draw_bitmap(rec[1], 610, 0, 0);
    draw_bitmap(rec[2], 0, 373, 0);
    draw_bitmap(rec[3], 610, 373, 0);

    restore_cursor_following();
}

/*
 * 0x0eed5
 *
 * **The game.** `game_main`'s third call, and everything after the intro and the
 * copy protection is inside it: set up, run rounds until something says stop,
 * take it all down.
 *
 * The loop is a `while` and not a `do`, and that matters - the jump at 0x0eedd
 * goes to the test, so `game_setup` has to leave DGROUP 0x4ebf non-zero or the
 * game ends before a single round runs.
 *
 * A round is `game_round`, and what happens afterwards depends on the state at
 * 0x4e6b. When it is 1 the loop is stopped by clearing 0x4ebf; otherwise the
 * count at 0x4ebd goes up, and if it has passed the best at 0x4eb7 the best is
 * caught up and 0x12bed is called - which is the shape of a high score being
 * written out.
 *
 * The counter is compared with `jle`, so the record is only rewritten when it
 * is genuinely beaten and not on a tie.
 */
void game_play(void)
{
    game_setup();

    while (g_playing != 0) {
        game_round();

        if (((int16_t)g_round_state) == 1) {
            g_playing = 0;
        } else {
            g_round_number++;
            if (g_round_number > g_furthest_level) {
                g_furthest_level = g_round_number;
                write_config();
            }
        }
    }

    free_two_bitmap_lists();
}
