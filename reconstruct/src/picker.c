/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The file picker and wrapped text**: the load and save dialog - its
 * listing, its name field, its checks on a name - the path walking, and the
 * text wrapped into a box.
 *
 * The seventh module of the original's **code segment 0dff**, image
 * 0x12c26..0x1405b. Its data is DGROUP 0x28ec..0x2966 - the characters a name
 * may not hold, the picker's tab stops and the dialog's strings - and its
 * uninitialised data DGROUP 0x567e..0x56b6, declared from the top down
 * because Borland lays it out from the last first mention up. Built with
 * Borland C++ and without `-d`: the pool keeps each call site's own copy of
 * a repeated string. Functions are in address order and each carries the
 * image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x28ec..0x2966
 */
#include <string.h>
#ifndef __TURBOC__
#include "hostlib.h"
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * The module's uninitialised data, highest first: the wrapped text's lines
 * at 0x56a6, the picker text at 0x568f, the name buffer at 0x5682 and the
 * caret at 0x567e.
 */
char *g_text_line[8];

/*
 * **The characters a filename may not contain**, DGROUP 0x28ec..0x28fa, 0x0e bytes: fourteen
 * of them, `*` `/` `,` `-` `[` `]` `&` `@` `^` `%` `?` `(` `)` `:`, which
 * `validate_filename` tests one by one. The run ends at 0x28fa.
 */
struct game_forbidden_chars {
    uint8_t   forbidden[14] NONSTRING;  /* +0x00 [0xe]  a set, not a string */
} PACKED;

struct game_forbidden_chars GAME_FORBIDDEN_CHARS = {
    "*/,-[]&@^%?():", /* forbidden */
};

/*
 * **Where Tab sends the pointer on the file picker's controls**, DGROUP 0x28fa..0x2918, 0x1e bytes: which
 * stop it is on - 0xffff until the first Tab, and back to 0 past the last
 * - and the x of each, the y being fixed.
 */
struct game_picker_tabs {
    uint16_t  stop;          /* +0x00 [2]  which of the picker's seven tab stops */
    int16_t   stop_x[7];          /* +0x02 [0xe]  where `picker_tab` parks the pointer */
    int16_t   stop_y[7];          /* +0x10 [0xe] */
} PACKED;

struct game_picker_tabs GAME_PICKER_TABS = {
    0xffff, /* stop */
    { 0x0090, 0x0080, 0x00c0, 0x00d0, 0x00d0, 0x0060, 0x00e0 }, /* stop_x */
    { 0x005c, 0x0082, 0x0112, 0x0080, 0x00ec, 0x013a, 0x013a }, /* stop_y */
};




/*
 * **The file picker and the wrapped-text block**, DGROUP 0x568f..0x56a6, 0x17 bytes.
 */
struct game_picker_text {
    int16_t   picker_mode;        /* +0x00 [2]  0x80 from the mode it was opened from, else 0 */
    int16_t   scroll;             /* +0x02 [2]  clamped on the way in, not on the way out */
    int16_t   entry_count;        /* +0x04 [2] */
    uint8_t far *text_start;    /* +0x06 [4]  where the listing's text begins: `entry_max`
                                     four-byte pointer slots into `block`, same segment */
    uint8_t far *block;         /* +0x0a [4]  allocated once and kept; a null
                                     pointer is the end */
    int16_t   entry_max;          /* +0x0e [2] */
    uint8_t   _pad_569f;          /* +0x10 [1]  a byte: 0x56a0 follows at +0x11 */
    int16_t   text_height;        /* +0x11 [2]  the block's measured extents, which the centring uses */
    int16_t   text_width;         /* +0x13 [2]  the widest line, clamped to the box */
    int16_t   line_count;         /* +0x15 [2]  how many lines, for the table at 0x56a6 */
} PACKED;

struct game_picker_text GAME_PICKER_TEXT;

/*
 * **The shared name buffer**, DGROUP 0x5682..0x568f, 0x0d bytes. `listing_to_name` strips a
 * listing record's `<`, `>` and spaces into it and answers its address, so the
 * caller has a near string it can hand to `strcpy`.
 *
 * Thirteen bytes, which is what a DOS 8.3 name and its NUL take - and what is
 * left between `dg_5677`, which ends at 0x5682, and `game_picker_text`.
 */
struct game_name_buffer {
    char      name[0xd];          /* +0x00 [0xd] */
} PACKED;

struct game_name_buffer GAME_NAME_BUFFER;


/*
 * The file picker's two caret counters, DGROUP 0x567e..0x5682 - the lowest
 * of the module's uninitialised data, so the last it declares: Borland lays
 * `_BSS` out in reverse order of first mention, which is also why this has
 * no `extern` in dgroup.h.
 */
struct picker_caret {
    uint16_t  caret_blink;        /* +0x00  bumped on every pass; the caret is `*` */
    uint16_t  caret_blink_b;      /* +0x02  a different counter, and a different asterisk at 0x2954 */
} PACKED;

struct picker_caret PICKER_CARET;

/*
 * 0x12c26
 *
 * **The file picker**, and a whole screen with its own loop. It answers 1 when
 * the player chose a file, leaving the name at DGROUP 0x52fe where
 * `load_animation` is then given it.
 *
 * **The mode word 0x4e6b is the whole of the state machine**, exactly as it is
 * in `game_screen`: the regions the pointer is over write it, this loop reads
 * it, and the jump table at 0x1318d dispatches on it. The picker borrows the
 * word, keeping what it displaced at 0x568f - which is also how every drawing
 * routine below knows whether it is a LOAD or a SAVE without being told.
 *
 * **`was`, the previous pass's mode, is what makes the text fields work.** A
 * field commits when it *stops* having focus, and by then 0x4e6b no longer says
 * so - the only place the old value survives is this local, taken at the bottom
 * of the loop. So both field blocks are entered when either the current or the
 * previous mode is theirs.
 *
 * **A full repaint suppresses the partial ones.** The five counters below are
 * not cleared by it; they are simply not acted on in a pass that repainted
 * everything, and stay pending for the next one. Drawing them again over a
 * fresh screen would be redundant, and the counters exist so that a redraw
 * asked for while a message box was up is not lost.
 *
 * Two of its calls are into `dos_chdir` and `dos_setdisk`, which are stubs in
 * this port for the reason given at each: there is nowhere to change to. So the
 * picker draws, scrolls and types, and reaches a stub the moment a directory is
 * actually chosen.
 */
uint16_t pick_file(uint16_t arg1, uint16_t arg2, char *pattern)
{
    char far *rec;                      /* [bp-4], [bp-2] */
    int16_t  reload;                    /* [bp-6]    */
    int16_t  v;                         /* [bp-8]    */
    int16_t  idx;                       /* [bp-0xa]  */
    int16_t  rp_up;                     /* [bp-0xc]  */
    int16_t  rp_down;                   /* [bp-0xe]  */
    int16_t  rp_list;                   /* [bp-0x10] */
    int16_t  rp_file;                   /* [bp-0x12] */
    int16_t  rp_name;                   /* [bp-0x14] */
    int16_t  valid;                     /* [bp-0x16] */
    char     pat[14];                   /* [bp-0x26], below the switch's own word */
    int16_t  repaint;                   /* si */
    uint16_t was;                       /* di */

    /*
     * **The pattern is the third argument, and it is copied.** `fill_file_listing`
     * takes it apart to build the extension filter and `strchr` walks it in
     * place, so what the listing filters on is this copy and never the caller's
     * constant.
     */
    strcpy(pat, pattern);

    g_picked_name[0] = 0;
    reload = 2;
    idx = 0;
    GAME_PICKER_TEXT.picker_mode = g_round_state;
    was = g_round_state = 0x8000;
    repaint = rp_list = rp_file = rp_name = 0;
#ifndef __TURBOC__
    /* The original never sets these two before the first pass reads them,
       and a stale count only redraws an arrow as it already is. */
    rp_up = rp_down = 0;
#endif

    while (g_round_state != 0x200 && g_round_state != 0x100) {
        if (reload != 0) {
            picker_begin(arg1, arg2, pat);

            if (GAME_PICKER_TEXT.entry_max == 0)
                return 0;

            reload  = 0;
            repaint = 1;
        }

        update_button_state();
        g_last_key = (uint8_t)bios_read_key();

        if ((g_last_key) == '\t' && g_round_state != 0x4000
            && g_round_state != 0x1000)
            picker_tab();

        if (((g_last_key) == '\r' || (g_last_key) == ' '
             || (g_last_key) == 0x1b /* Esc */)
            && g_round_state == 0x4000)
            POINTER.button_left = 0;

        regions_handle_pointer(g_regions_c);

        if (g_round_state == 0x100)
            goto dispatch;

        /*
         * **The path field.** It is entered while the field has focus *or* had
         * it on the pass before - `was` is last pass's 0x4e6b, taken at the
         * bottom of the loop - because losing focus is what commits the typed
         * path, and by then 0x4e6b no longer says the field. The original
         * tests the mode twice on the way in, once in each half of the `||`.
         */
        if ((g_round_state != 0x4000 && was == 0x4000) || g_round_state == 0x4000) {
            g_file_op_active = 1;

            if (((g_last_key) != '\r' && g_round_state == 0x4000)
                || was != 0x4000) {
                if (was == 0x4000)
                    picker_type((g_last_key), (char *)GAME_DIRECTORIES.path_field, 0x50);

                rp_name = 2;
            } else {
                /*
                 * A path of exactly `X:` skips the first `chdir` and goes
                 * straight to the second. Every other path is handed to
                 * `chdir` **twice** - once to find out whether it is reachable
                 * and once to go there - and the drive is selected only after
                 * the second succeeds.
                 */
                if ((GAME_DIRECTORIES.path_field[1] == ':' && !GAME_DIRECTORIES.path_field[2])
                    || dos_chdir((char *)GAME_DIRECTORIES.path_field) == 0) {
                    if (dos_chdir((char *)GAME_DIRECTORIES.path_field) == 0) {
                        dos_setdisk(GAME_DIRECTORIES.path_field[0]);
                        reload = 2;
                    } else {
                        dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);
                        show_message_box(MESSAGES.path_error, MESSAGES.path_error_body);
                        wait_cursor();
                        paint_panel_frame();
                        restore_cursor();
                        repaint = 1;
                        rp_name = 2;
                    }

                    if (g_round_state == 0x4000)
                        g_round_state = 0x8000;
                } else {
                    dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);
                    show_message_box(MESSAGES.path_error, MESSAGES.path_error_body);
                    wait_cursor();
                    paint_panel_frame();
                    restore_cursor();
                    repaint = 1;
                    rp_name = 2;

                    if (g_round_state == 0x4000)
                        g_round_state = 0x8000;
                }
            }

            g_file_op_active = 0;
        }

        /* The name field, the same shape and a different buffer. */
        if ((g_round_state != 0x1000 && was == 0x1000) || g_round_state == 0x1000) {
            if (((g_last_key) != '\r' && g_round_state == 0x1000)
                || was != 0x1000) {
                if (was == 0x1000)
                    picker_type((g_last_key), (char *)g_picked_name, 0x0d);
            } else {
                force_extension((char *)g_picked_name, "TIM");

                if (g_round_state == 0x1000)
                    g_round_state = 0x8000;
            }

            rp_file = 2;
        }

    dispatch:
        switch (g_round_state) {
        case 0x0800:                    /* the up arrow */
            if (POINTER.button_left == 1 || POINTER.button_left == 2) {
                v = GAME_PICKER_TEXT.scroll - 1;

                if (v >= 0) {
                    GAME_PICKER_TEXT.scroll = v;
                    rp_list = 2;
                }
            } else {
                g_round_state = 0x8000;
            }
            rp_up = 2;
            break;

        case 0x0400:                    /* the down arrow */
            if (POINTER.button_left == 1 || POINTER.button_left == 2) {
                v = GAME_PICKER_TEXT.scroll + 1;

                if (GAME_PICKER_TEXT.entry_count - 12 >= v) {
                    GAME_PICKER_TEXT.scroll = v;
                    rp_list = 2;
                }
            } else {
                g_round_state = 0x8000;
            }
            rp_down = 2;
            break;

        case 0x2000:                    /* a click in the listing */
            /*
             * **Which row was clicked is arithmetic, not a hit test.** The
             * pointer's y at 0x5782 less the box's top, divided by the ten
             * pixels a row takes, plus the scroll position.
             */
            idx = GAME_PICKER_TEXT.scroll + (POINTER.pointer_y - 0x7c) / 10;

            if (idx < GAME_PICKER_TEXT.entry_count) {
                /* Entry `idx` of the array of far pointers at the block's
                   front, followed to the text it names. */
                rec = ((char far * far *)GAME_PICKER_TEXT.block)[idx];

                if (*rec != ':' && *rec != '<') {
                    strcpy((char *)g_picked_name, listing_to_name(rec));
                    rp_file = 2;
                } else {
                    /*
                     * Row zero is the `:` when there is one, and the only way
                     * to tell it from a directory called nothing is that we
                     * are not at a root.
                     */
                    if (idx != 0 || path_is_root((char *)GAME_DIRECTORIES.path_field) != 0)
                        path_join((char *)GAME_DIRECTORIES.path_field, rec);
                    else
                        path_up((char *)GAME_DIRECTORIES.path_field);

                    g_file_op_active = 1;

                    if (dos_chdir((char *)GAME_DIRECTORIES.path_field) == 0)
                        dos_setdisk(GAME_DIRECTORIES.path_field[0]);

                    g_file_op_active = 0;
                    reload = 2;
                    g_picked_name[0] = 0;
                }
            }

            g_round_state = 0x8000;
            break;

        case 0x0200:                    /* the LOAD or SAVE button */
            if ((valid = validate_filename()) != 0) {
                if (GAME_PICKER_TEXT.picker_mode == 0x80) {
                    if (valid == 2) {
                        picker_draw_action();

                        if (ask_yes_no(MESSAGES.overwrite_file, MESSAGES.overwrite_body) == 0) {
                            wait_cursor();
                            paint_panel_frame();
                            restore_cursor();
                            repaint = 1;
                            g_round_state = 0x8000;
                        }
                    }
                } else if (is_machine_file((char *)g_picked_name) == 0) {
                    picker_draw_action();
                    show_message_box(MESSAGES.wrong_format, MESSAGES.wrong_format_body);
                    wait_cursor();
                    paint_panel_frame();
                    restore_cursor();
                    repaint = 1;
                    g_round_state = 0x8000;
                }
            } else {
                picker_draw_action();
                show_message_box(MESSAGES.file_error,
                                 GAME_PICKER_TEXT.picker_mode == 0x100
                                     ? MESSAGES.cant_open_for_loading
                                     : MESSAGES.cant_open_for_saving);
                wait_cursor();
                paint_panel_frame();
                restore_cursor();
                repaint = 1;
                g_round_state = 0x8000;
            }
            break;
        }

        was = g_round_state;

        /*
         * **A whole repaint is not one of the partial ones.** When it happens
         * every pending partial redraw is left pending and the pass ends - the
         * full paint has already drawn all of them, and running them again
         * would draw over what it just put down.
         */
        if (repaint != 0) {
            picker_repaint();
            repaint--;
        } else {
            if (rp_up != 0) {
                picker_draw_up();
                rp_up--;
            }
            if (rp_down != 0) {
                picker_draw_down();
                rp_down--;
            }
            if (rp_list != 0) {
                picker_draw_list();
                rp_list--;
            }
            if (rp_name != 0) {
                picker_draw_name();
                rp_name--;
            }
            if (rp_file != 0) {
                picker_draw_filename();
                rp_file--;
            }

            present_frame(1);
        }
    }

    /*
     * The listing block goes back **only when it is not the borrowed one**:
     * `picker_begin` will take the pointer at 0x3576 if there is one, and
     * freeing that would hand back memory the picker never owned.
     */
    if (GAME_PICKER_TEXT.block != g_scratch_block) {
        dos_free_far(GAME_PICKER_TEXT.block);
        GAME_PICKER_TEXT.block = NULL;
        GAME_PICKER_TEXT.text_start = NULL;
    }

    picker_draw_action();

    if (g_round_state != 0x200 || strlen((char *)g_picked_name) == 0) {
        g_picked_name[0] = 0;
        return 0;
    } else {
        strcpy(g_picked_machine, (char *)g_picked_name);
        return 1;
    }
}

/*
 * 0x1319d
 *
 * **Is the typed name usable?** Three answers, not two: 0 for no, 1 for a name
 * that is free to create, and **2 for one that already exists** - which the
 * caller needs to tell apart so it can ask before overwriting.
 *
 * The rejections, in the order they are made:
 *
 *   an empty name, or one starting with `.`; a space anywhere in the stem - the
 *   scan stops at the first `.`, so spaces in an extension are not looked at;
 *   any of the fourteen characters in the table at DGROUP 0x28ec, which are
 *   `*` `/` `,` `-` `[` `]` `&` `@` `^` `%` `?` `(` `)` `:`; and any of eleven
 *   reserved DOS device names.
 *
 * A device name only counts when it is the *whole stem* - the byte after it has
 * to be the terminator or a `.` - which is why `CONFIG.TIM` survives and
 * `CON.TIM` does not.
 *
 * **The eleventh entry is wrong in the original.** "null" is compared for
 * **three** bytes - so it tests the same `nul` the tenth entry does - but checks
 * the byte at +4 rather than +3. The effect is that *any* four-letter stem
 * beginning `NUL` is rejected: `NULA` and `NULX` as much as `NULL`. Written as
 * `strnicmp(name, "null", 4)`, which is plainly what was meant, it would have
 * caught `NULL` alone. Transcribed as it is.
 *
 * Last, it *opens the file* to find out whether it is there, and closes it
 * again. A name that opens answers 2. One that does not answers 1 only when
 * 0x568f says 0x80 - the mode the picker was opened from - and 0 otherwise, so
 * asking to load something that is not there is a rejection rather than an
 * answer the caller has to interpret.
 */
uint16_t validate_filename(void)
{
    char    *si;
    int16_t  i;
    int16_t  bad = 0;
    FILE *file;

    si = (char *)g_picked_name;

    if (*si == 0)
        bad = 1;
    if (*si == '.')
        bad = 1;

    while (*si != 0 && *si != '.') {
        if (*si == ' ')
            bad = 1;
        si++;
    }

    if (bad)
        return 0;

    for (i = 0; i < 0x0e; i++) {
        if (strchr((char *)g_picked_name,
                       GAME_FORBIDDEN_CHARS.forbidden[i]) != NULL)
            return 0;
    }

    if (strnicmp((char *)g_picked_name, "con", 3) == 0
        && (g_picked_name[3] == 0 || g_picked_name[3] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "aux", 3) == 0
        && (g_picked_name[3] == 0 || g_picked_name[3] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "com1", 4) == 0
        && (g_picked_name[4] == 0 || g_picked_name[4] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "com2", 4) == 0
        && (g_picked_name[4] == 0 || g_picked_name[4] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "com3", 4) == 0
        && (g_picked_name[4] == 0 || g_picked_name[4] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "com4", 4) == 0
        && (g_picked_name[4] == 0 || g_picked_name[4] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "prn", 3) == 0
        && (g_picked_name[3] == 0 || g_picked_name[3] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "lpt1", 4) == 0
        && (g_picked_name[4] == 0 || g_picked_name[4] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "lpt2", 4) == 0
        && (g_picked_name[4] == 0 || g_picked_name[4] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "nul", 3) == 0
        && (g_picked_name[3] == 0 || g_picked_name[3] == '.'))
        return 0;
    if (strnicmp((char *)g_picked_name, "null", 3) == 0
        && (g_picked_name[4] == 0 || g_picked_name[4] == '.'))
        return 0;

    if ((file = game_fopen((char *)g_picked_name, "rb")) != 0) {
        game_fclose(file);
        return 2;
    }

    if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x80)
        return 1;

    return 0;
}

/*
 * 0x13402
 *
 * **Redraw the picker's one action button.** Which word it carries is not a
 * parameter: it is read back out of the mode word DGROUP 0x4e6b, and when that
 * says 0x200 - the picker's own mode - out of *0x568f*, the value 0x4e6b held
 * before the picker took it. So the button says LOAD or SAVE according to which
 * handler opened the picker, and the picker itself does not have to be told.
 *
 * Anything else says CANCEL, and it moves: 0xc0 against 0x40. The two are
 * different buttons in the same place in the code, not one button relabelled.
 */
void picker_draw_action(void)
{
    if (g_round_state != 0x200) {
        draw_button(MESSAGES.cancel, 0xc0, 0x130, 1);
    } else if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x100) {
        draw_button(MESSAGES.load, 0x40, 0x130, 1);
    } else {
        draw_button(MESSAGES.save, 0x40, 0x130, 1);
    }

    present_back_page();
}

/*
 * 0x1345f
 *
 * **Tab inside the picker**, and the same trick as the panel's at 0x1156c: it
 * warps the pointer rather than moving any focus. Seven stops, cursor at DGROUP
 * 0x28fa, x at 0x28fc and y at 0x290a - and here the two tables are the same
 * length, because none of the picker's controls is a slider whose position has
 * to be worked out from a value.
 */
void picker_tab(void)
{
    GAME_PICKER_TABS.stop++;

    if (GAME_PICKER_TABS.stop == 7)
        GAME_PICKER_TABS.stop = 0;

    move_pointer_to(GAME_PICKER_TABS.stop_x[GAME_PICKER_TABS.stop],
                    GAME_PICKER_TABS.stop_y[GAME_PICKER_TABS.stop]);
}

/*
 * 0x13490
 *
 * **One keystroke into the picker's name field.** Backspace - 8 - takes the
 * last byte off, and does nothing on an empty field. Anything else is appended
 * *as a string*: the character is stored into a two-byte local with a NUL after
 * it and handed to `strcat`, which is why this routine has locals at all.
 *
 * Two characters never reach the field: backspace, which is handled above, and
 * **tab**, which is excluded explicitly. Tab is a key the picker wants for
 * moving the pointer, and a field that swallowed it would take it away.
 *
 * The length check is `< max`, and `max` counts the NUL's room the way the
 * caller passed it - this routine does not add one.
 */
void picker_type(uint8_t c, char *buf, int16_t max)
{
    char str[2];                  /* [bp-2], the two-byte string */
    int16_t  len;

    str[0] = (char)c;
    str[1] = 0;

    len = (int16_t)strlen(buf);

    if (c == '\b') {
        if (len != 0)
            *(buf + len - 1) = 0;
    } else if (len < max && c != '\t') {
        strcat(buf, str);
    }
}

/*
 * 0x134dd
 *
 * **Is this path a drive's root?** One separator in the whole string, and it is
 * the last byte - "C:\\" and nothing else. It counts the same way `path_up`
 * does, against the same shared "\\" at DGROUP 0x1bca, which is what keeps the
 * two agreeing about where the walk up has to stop.
 */
uint16_t path_is_root(const char *path)
{
    const char *si = path;
    const char *last;
    int16_t  n = 0;

    while (*si != 0) {
        if (*si == *g_path_separator) {
            last = si;
            n++;
        }
        si++;
    }

    if (n == 1 && last[1] == 0)
        return 1;
    else
        return 0;
}

/*
 * 0x13516
 *
 * **Drop the last component of a path**, in place. It walks to the terminator
 * counting separators - the character is not a literal here but `*g_path_separator`,
 * the one-character string "\\" the rest of the module shares - and remembers
 * the last one it saw.
 *
 * The two cases differ by one byte, and that byte is the whole point: with a
 * single separator the cut is *after* it, leaving "C:\\", because a drive with
 * its backslash taken off means the current directory rather than the root.
 * With more than one it cuts *at* the separator, leaving the parent. With none
 * it does nothing at all.
 */
void path_up(char *path)
{
    char *si = path;
    char *last;
    int16_t  n = 0;

    while (*si != 0) {
        if (*si == *g_path_separator) {
            last = si;
            n++;
        }
        si++;
    }

    if (n == 1)
        last[1] = 0;
    else if (n > 1)
        *last = 0;
}

/*
 * 0x1354c
 *
 * **Join a listed name onto the path.** The name comes in as a *far* pointer -
 * it is in the picker's own list block, not DGROUP - and the path is near, so
 * the name is copied through a fourteen-byte local first.
 *
 * That copy is off by one at both ends, deliberately, and `fill_file_listing` is what
 * makes it right: a directory is written into the listing as `<NAME>`. This
 * stores from the **second** byte, past the `<`, and after the join chops the
 * **last** byte, the `>`. So `<DOS>` arrives and `\\DOS` leaves.
 *
 * The separator goes in only when the path is not already a root, because a
 * root already ends in one and `path_is_root` is the routine that knows.
 *
 * The loop tests the byte *before* stepping and stores the byte *after*, so the
 * NUL is copied along with the rest and the local needs no terminating of its
 * own.
 */
void path_join(char *path, const char far * entry)
{
    char name[14];                            /* [bp-0xe] */
    uint16_t di   = 0;

    while (*entry != 0) {
        entry++;
        name[di] = *entry;
        di++;
    }

    if (path_is_root(path) == 0)
        strcat(path, g_path_separator);

    strcat(path, name);

    *(path + strlen(path) - 1) = 0;
}

/*
 * 0x135a6
 *
 * **Force a name into 8.3.** The eighth byte is cut off first, unconditionally
 * and before anything is looked at, so a long name loses its tail rather than
 * its extension. Then the first `.` - or the terminator, if there is none - is
 * where the new one goes, and the extension the caller passed is appended.
 *
 * An empty name is left empty: the cut at byte 8 has already happened, but
 * nothing is appended, so the picker cannot end up holding a name that is only
 * an extension.
 */
void force_extension(char *name, const char *ext)
{
    char *si;

    name[8] = 0;

    if (name[0] != 0) {
        si = name;
        while (*si != 0 && *si != '.')
            si++;

        si[0] = '.';
        si[1] = 0;

        strcat(name, ext);
    }
}

/*
 * 0x135dc
 *
 * Hand the picker a name to start from: a straight copy into DGROUP 0x4e5a,
 * the one buffer the picker answers out of.
 *
 * **Nothing calls it**, by three searches rather than one: no near `call` to
 * it in this module, no far `call` anywhere in the image - `9a ec 55 ff 0d` -
 * and its far pointer `0dff:55ec` is not *stored* anywhere either, which is how
 * the region handlers are reached and would have been missed by the first two.
 * The same holds for `picker_name` beside it.
 *
 * The pointer search was run against `path_join` as a control: that one is
 * called, by a near `call`, and its pointer is likewise stored nowhere - so the
 * search distinguishes dispatch through a table from a direct call, rather than
 * answering "nowhere" to everything. They are the picker's public face, written and never
 * used, because `pick_file` fills 0x4e5a itself and copies the answer to
 * 0x52fe on the way out. Transcribed because they are there, and recorded as
 * dead because saying "unreached on the paths tried" would suggest a path
 * exists.
 */
void picker_set_name(const char *name)
{
    strcpy((char *)g_picked_name, name);
}

/*
 * 0x135ef
 *
 * The picker's answer: **the buffer's address, or zero when it is empty.** A
 * caller would get a pointer it could hand straight to `load_animation`,
 * without having to know where the name lives.
 *
 * There is no caller. See `picker_set_name` above: neither is reachable from
 * anywhere in the image.
 */
char *picker_name(void)
{
    if (g_picked_name[0] != 0)
        return (char *)g_picked_name;
    else
        return NULL;
}

/*
 * 0x13606
 *
 * **Get the listing a place to live, then fill it and draw it.**
 *
 * The block is allocated once and kept: the far pointer at DGROUP 0x5699 being
 * non-null is the whole test, and on the second and later openings everything
 * below is skipped. There is a second source before DOS is asked at all - the
 * pointer at 0x3576, some other part of the game's block, which is taken with a
 * flat capacity of 0x3e8 entries rather than a measured one.
 *
 * Otherwise it asks `dos_alloc_bytes` for **0xffffffff** bytes, which is the
 * "how much is there" question, and clamps the answer to 0x7530. So the picker
 * takes what is free up to 30000 bytes and no more - the listing is allowed to
 * grow into spare memory, but not to eat it.
 *
 * The entry size is 0x16, and that is where 0x5695 comes from: the block is
 * *two* arrays, `count` far pointers of four bytes each and then the records
 * themselves, so the second pointer is the first plus `4 * count`. Only the
 * offset is added - the segment is shared - which is what keeps a listing this
 * size inside one segment.
 */
void picker_begin(uint16_t arg1, uint16_t arg2, char *pattern)
{
    uint32_t v;

    (void)arg1;
    (void)arg2;

    if (GAME_PICKER_TEXT.block == NULL) {
        if (g_scratch_block != NULL) {
            GAME_PICKER_TEXT.entry_max = 0x3e8;
            GAME_PICKER_TEXT.block = g_scratch_block;
        } else {
            v = DOS_ALLOC_BYTES(dos_alloc_bytes(0xffffffffUL, 0));

            if ((int32_t)v > 0x7530)
                v = 0x7530;

            GAME_PICKER_TEXT.entry_max = (int32_t)v / 0x16;

            GAME_PICKER_TEXT.block = dos_alloc_bytes(v, 0);
        }

        /* The table of pointers sits at the head of the block and the text
           after it, so the start is one pointer a line in, in the block's own
           segment. */
        GAME_PICKER_TEXT.text_start = (uint8_t far *)
            ((char far * far *)GAME_PICKER_TEXT.block + GAME_PICKER_TEXT.entry_max);
    }

    fill_file_listing(pattern);
    sort_file_listing();
    GAME_PICKER_TEXT.scroll = 0;
}

/*
 * 0x136c9
 *
 * **The picker's whole screen.** Everything the loop redraws piecemeal, laid
 * down once: the title bar, the four sunken wells - two for the buttons, two
 * for the scroll arrows - the heading, the buttons, and then the same four
 * routines the loop calls for its partial repaints.
 *
 * The heading and the left button say LOAD or SAVE according to 0x568f, and
 * they are drawn from **two separate branches** rather than one branch choosing
 * two strings. Both buttons are drawn unpressed here; `picker_draw_action` is
 * what draws them pressed, and it exists precisely because this routine cannot
 * be called for a button going down.
 *
 * The wells are placed round what goes in them, not derived from it: the button
 * well at (0x36, 0x129) is 0x40 by 0x20 for a button drawn at (0x40, 0x130).
 */
void picker_repaint(void)
{
    VMDS.page_dst = VMDS.page_back;

    draw_title_bar(0x30, 0x31, 0x110, 0x149, 1);

    draw_sunken_box(0x36, 0x129, 0x40, 0x20);
    draw_sunken_box(0xb6, 0x129, 0x50, 0x20);

    if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x100) {
        draw_scroll_text(MESSAGES.load_machine, 0x50, 0x34, 0xa0);
        draw_button(MESSAGES.load, 0x40, 0x130, 0);
    } else {
        draw_scroll_text(MESSAGES.save_machine, 0x50, 0x34, 0xa0);
        draw_button(MESSAGES.save, 0x40, 0x130, 0);
    }

    draw_sunken_box(0xbc, 0x74, 0x20, 0x20);
    draw_sunken_box(0xbc, 0xe0, 0x20, 0x20);

    picker_draw_up();
    picker_draw_down();

    draw_button(MESSAGES.cancel, 0xc0, 0x130, 0);

    picker_draw_name();
    picker_draw_list();
    picker_draw_filename();

    present_back_page();
}

/*
 * 0x137e4
 *
 * **The list's up arrow**, redrawn. Which of the two pieces of art it uses is
 * read out of the mode word DGROUP 0x4e6b - 0x800 is "this arrow is held down"
 * - so the picker never has to tell it, the same way `picker_draw_action` reads
 * its own word back.
 *
 * The pair sits at +0x4a in the art set at DGROUP 0x52f4, and the pressed one
 * is the *second*, which is why the index is doubled before it is added.
 */
void picker_draw_up(void)
{
    int16_t pressed;

    if (g_round_state == 0x800)
        pressed = 1;
    else
        pressed = 0;

    VMDS.page_dst = VMDS.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((g_panel_art + 0x25)[pressed]),
                0xc4, 0x78, 0);
    restore_cursor_following();
}

/*
 * 0x1382a
 *
 * **The list's down arrow.** `picker_draw_up`'s twin, and the only differences
 * are the three numbers: the mode it answers to is 0x400, its art is at +0x4e,
 * and it sits 0x70 further down at y 0xe8.
 */
void picker_draw_down(void)
{
    int16_t pressed;

    if (g_round_state == 0x400)
        pressed = 1;
    else
        pressed = 0;

    VMDS.page_dst = VMDS.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((g_panel_art + 0x27)[pressed]),
                0xc4, 0xe8, 0);
    restore_cursor_following();
}

/*
 * 0x13870
 *
 * **The name field.** The buffer at DGROUP 0x53ab is copied into a local first,
 * and then the *pointer* is walked forward while the text is wider than 0xac
 * pixels - so a long name scrolls off the **left**, showing its end. That is
 * the right way round for typing: what you just typed stays in view.
 *
 * The caret is `*`, and it blinks by counting: 0x567e is bumped on every one of
 * these redraws and bit 3 decides whether the asterisk is appended, so it is on
 * for eight redraws and off for eight. It is appended *after* the width walk,
 * which means the caret can push the text past 0xac - the field is measured on
 * the name, not on the name plus caret.
 *
 * It only blinks when 0x4e6b says 0x4000, the field's own mode. Out of that
 * mode nothing is counted, so the caret is not merely hidden, it stops.
 */
void picker_draw_name(void)
{
    char buf[90];                  /* [bp-0x5a] */
    char *si  = buf;

    strcpy(si, (const char *)GAME_DIRECTORIES.path_field);

    while ((int16_t)text_width_thunk(si) > 0xac)
        si++;

    if (g_round_state == 0x4000) {
        PICKER_CARET.caret_blink++;
        if ((PICKER_CARET.caret_blink & 8) != 0)
            strcat(si, "*");
    }

    VMDS.page_dst = VMDS.page_back;
    fill_panel_area(0x40, 0x56, 0xb8, 0x10, 0);

    VMDS.text_back = 0;
    VMDS.text_colour = 0x0f;

    cursor_redraw_off_thunk();
    draw_string(si, 0x44, 0x5a);
    restore_cursor_following();
}

/*
 * 0x13902
 *
 * **The "File Name:" field**, and `picker_draw_name`'s twin down to the shape
 * of the code: copy, walk the pointer forward while the text is too wide, blink
 * a caret by counting, fill, draw.
 *
 * Everything that differs is a number - a different buffer (0x4e5a against
 * 0x53ab), a narrower field (0x64 against 0xac), a different mode (0x1000), a
 * different counter (0x5680) - and a *different asterisk*: 0x2954, where the
 * other field uses 0x2952. The two one-character strings sit next to each other
 * in the image, unpooled, which is how you can tell these are two routines and
 * not one called twice.
 *
 * This one also draws its own label, because the label belongs to the field.
 */
void picker_draw_filename(void)
{
    char buf[16];                  /* [bp-0x10] */
    char *si  = buf;

    strcpy(si, (const char *)g_picked_name);

    while ((int16_t)text_width_thunk(si) > 0x64)
        si++;

    if (g_round_state == 0x1000) {
        PICKER_CARET.caret_blink_b++;
        if ((PICKER_CARET.caret_blink_b & 8) != 0)
            strcat(si, "*");
    }

    VMDS.page_dst = VMDS.page_back;
    draw_scroll_text(MESSAGES.file_name, 0x30, 0x10c, 0x54);
    fill_panel_area(0x90, 0x10c, 0x70, 0x10, 0);

    VMDS.text_back = 0;
    VMDS.text_colour = 0x0f;

    cursor_redraw_off_thunk();
    draw_string(si, 0x94, 0x110);
    restore_cursor_following();
}

/*
 * 0x139ac
 *
 * **Draw the listing.** Twelve rows of ten pixels in a 0x70 by 0x80 box at
 * (0x40, 0x78), text transparent and white, and the far pointers walked in
 * order - so what the sort did is what shows.
 *
 * **This is where the `:` record gets its words.** A row whose text begins with
 * a colon is drawn as *"&lt;PARENT DIR&gt;"* instead: the pointer is swapped for one
 * into DGROUP and the row draws normally. So the listing holds a one-byte
 * marker and the screen holds a phrase, and nothing in between has to know both.
 *
 * The scroll position at 0x5691 is **clamped on the way in, not on the way
 * out**: a listing of twelve or fewer starts at zero whatever 0x5691 says, and
 * one that has scrolled past the end is pulled back to `count - 12`. The
 * comparison is `>`, against the count rather than against `count - 12`, so a
 * position exactly at the count is *kept* and only one past it is caught.
 *
 * Two conditions end the loop, the count and the room left, and the room is
 * counted down in the same 0x0a steps the rows are drawn in.
 */
void picker_draw_list(void)
{
    /* The block is an array of far pointers, one per entry. */
    char far * far *p;                  /* [bp-4], [bp-2] */
    const char far *t;                  /* [bp-8], [bp-6] */
    int16_t  x = 0x40;                  /* [bp-0xa] */
    int16_t  y = 0x78;                  /* di */
    int16_t  w = 0x70;                  /* [bp-0xc] */
    int16_t  room = 0x80;               /* [bp-0xe] */
    int16_t  i;                         /* si: the rows to skip, then the row */

    fill_panel_area(x, y, w, room, 0);

    VMDS.page_dst = VMDS.page_back;
    VMDS.text_style = 1;                    /* transparent text */
    VMDS.text_colour = 0x0f;

    if (GAME_PICKER_TEXT.entry_count <= 0x0c) {
        i = 0;
    } else {
        i = GAME_PICKER_TEXT.scroll;
        if (i > GAME_PICKER_TEXT.entry_count)
            i = GAME_PICKER_TEXT.entry_count - 12;
    }

    p = (char far * far *)GAME_PICKER_TEXT.block;
    while (i--)                         /* skip the rows scrolled past */
        p++;

    for (i = 0; i < GAME_PICKER_TEXT.entry_count && room >= 0x0a; i++) {
        t = *p;
        p++;
        if (*t == ':')
            t = MESSAGES.parent_dir;

        cursor_redraw_off_thunk();
        draw_string_body(t, x + 4, y + 4);
        restore_cursor_following();

        y    += 0x0a;
        room -= 0x0a;
    }
}

/*
 * 0x13a8a
 *
 * **Fill the listing.** Two pointers walk the block `picker_begin` set up: one
 * along the far-pointer array at its front, one along the text after it. Each
 * entry files a pointer and appends its text, and the array is terminated with
 * a null far pointer rather than a count - though a count is kept at 0x5693 as
 * well, because the fill also has to stop when the block is full.
 *
 * **The three record shapes are what `path_join` reads back.** A directory is
 * written `<NAME>`: `<`, then the name copied *including its NUL*, then the
 * byte before the write pointer - which is that NUL - overwritten with `>` and
 * a fresh NUL put down. That is where `path_join`'s off-by-one at both ends
 * comes from, and it is no longer a guess.
 *
 * A file is written as a fixed 8-character stem padded with spaces and then
 * everything from the `.` onwards, so the extensions line up in a column
 * without the drawing code measuring anything.
 *
 * And a lone `:` goes in first when DGROUP 0x53ae is not zero - the fourth byte
 * of the current directory, so "there is something past X:\\". It is the way
 * back up, and it is needed because `.` and `..` are both thrown away below.
 *
 * **The extension filter reads DGROUP when a name has no dot.** `strchr` for
 * `.` answers zero for a name like README, and the three comparisons that
 * follow are made through that zero, against DGROUP's own first bytes. Near
 * pointers make it harmless rather than fatal, and it is why a pattern of
 * `*.*` - whose second byte is `*` - is turned into *no filter at all* before
 * the loop starts, rather than into a filter that always matches.
 */
void fill_file_listing(char *pattern)
{
    /* Two cursors into the one block: the array of far pointers at its
       front, and the text they point at, both stepped inside one segment. */
    char far * far *ptr;                /* [bp-4], [bp-2]: into the array */
    char far *txt;                      /* [bp-8], [bp-6]: into the text */
    char    *ext;                       /* [bp-0xa] the name's extension */
    int16_t  more;                      /* [bp-0xc] */
    int16_t  n;                         /* [bp-0xe] */
    char    *name;                      /* di */

    GAME_PICKER_TEXT.entry_count = 0;
    dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);

    ptr = (char far * far *)GAME_PICKER_TEXT.block;
    txt = (char far *)GAME_PICKER_TEXT.text_start;

    /* The pattern's own extension, or none if it is a wildcard. */
    pattern = strchr(pattern, '.');
    if (pattern != NULL && pattern[1] == '*')
        pattern = NULL;

    if (GAME_DIRECTORIES.path_field[3] != 0) {
        *ptr++ = txt;
        *txt++ = ':';
        *txt++ = 0;
        GAME_PICKER_TEXT.entry_count++;
    }

    more = dos_findfirst("*.*", 0x10);

    while (more == 0 && GAME_PICKER_TEXT.entry_count < GAME_PICKER_TEXT.entry_max) {
        name = dos_find_name();
        ext  = strchr(name, '.');

        if (dos_find_attr() & 0x10) {
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                *ptr++ = txt;
                GAME_PICKER_TEXT.entry_count++;

                *txt++ = '<';
                while ((*txt++ = *name++) != 0)
                    ;
                txt[-1] = '>';
                *txt++  = 0;
            }
        } else if (pattern == NULL
                   || (ext[1] == pattern[1]
                       && ext[2] == pattern[2]
                       && ext[3] == pattern[3])) {
            *ptr++ = txt;
            GAME_PICKER_TEXT.entry_count++;

            n = 0;
            while (*name != 0 && *name != '.') {
                *txt = *name;
                name++;
                txt++;
                n++;
            }

            while (n < 8) {
                *txt++ = ' ';
                n++;
            }

            while ((*txt++ = *name++) != 0)
                ;
        }

        more = dos_findnext("*.*", 0x10);
    }

    *ptr = NULL;                        /* the list's terminator */
}

/*
 * 0x13c78
 *
 * **Sort the listing**, by exchanging the far pointers at the front of the
 * block and never the text they point at. A bubble sort: passes until one makes
 * no exchange.
 *
 * **Directories come first, and the test is the `<` the fill wrote.** There is
 * no type field to consult - the first byte of the record *is* the type - so
 * one against a directory sorts before, one after, and two of a kind fall
 * through to comparing the names. Which is why the four cases are written as
 * two nested tests rather than a comparison of two flags.
 *
 * The names are compared with `far_stricmp`, so `<Dos>` and `<DOS>` land where
 * a reader expects rather than where ASCII puts them.
 *
 * The `:` entry is **not sorted**: every pass starts one slot further in when
 * the first record begins with it, so the way back up stays at the top however
 * the rest moves.
 *
 * The walk ends on a null far pointer, and it checks *two* - the current slot
 * and the next - because a bubble pass compares a pair and there is no pair at
 * the last entry.
 */
void sort_file_listing(void)
{
    /* The block is an array of far pointers, one per entry. `p` walks it
       and `q` is the one after it. The pointers are swapped; the names are
       read through. */
    char far * far *p;                  /* [bp-4], [bp-2] */
    char far * far *q;                  /* [bp-8], [bp-6] */
    char far *t;                        /* [bp-0xc], [bp-0xa] */
    int16_t  swapped = 1;               /* si */

    while (swapped) {
        swapped = 0;

        p = (char far * far *)GAME_PICKER_TEXT.block;

        /* Skip the ":" entry - the current directory - if it is first, so
           the sort below never moves it. */
        if (*p != NULL && **p == ':')
            p++;

        while (*p != NULL && p[1] != NULL) {
            q = p + 1;

            /* "<PARENT DIR>" and the directories sort first. */
            if ((**p != '<' && **q == '<')
                || ((**p != '<' || **q == '<') && far_stricmp(*p, *q) > 0)) {
                t = *p;
                *p = *q;
                *q = t;
                swapped = 1;
            }

            p++;
        }
    }
}

/*
 * 0x13d75
 *
 * **A listing record back into a plain name.** The record is far and the answer
 * is near - DGROUP 0x5682, one shared buffer - so the caller gets something it
 * can hand to `strcpy` without carrying a segment around.
 *
 * It strips exactly three things: `<`, `>` and spaces. That undoes both of the
 * shapes `fill_file_listing` writes, the angle brackets round a directory and the
 * padding that lines the extensions up, with one filter rather than two.
 *
 * A `:` record does not go through the loop at all; it answers the constant
 * `".."`, so the way back up leaves here as a path DOS understands rather than
 * as the marker the listing keeps it as.
 */
char *listing_to_name(const char far * entry)
{
    char *si;

    if (*entry == ':')
        return "..";

    si = (char *)GAME_NAME_BUFFER.name;

    while (*entry != 0) {
        if (*entry != '<' && *entry != '>' && *entry != ' ') {
            *si = *entry;
            si++;
        }
        entry++;
    }

    *si = 0;
    return (char *)GAME_NAME_BUFFER.name;
}

/*
 * 0x13dc7
 *
 * **Draw a string wrapped into a box**, centred both ways, with a shadow.
 *
 * `wrap_text_to_box` does the wrapping and leaves its results in DGROUP: a
 * list of line pointers from 0x56a6, how many at 0x56a4, and the block's
 * measured height and width at 0x56a0 and 0x56a2. This routine only places and
 * draws them.
 *
 * The centring uses the *measured* extents, not the box: `(w - 0x56a2 - 1) / 2`
 * and `(h - 0x56a0 - 1) / 2`, the minus one making an odd remainder fall left
 * and up rather than right and down. The clip box is then set to the box as
 * placed, so a line the wrapper could not fit is cut rather than drawn over
 * the panel.
 *
 * **A line's end is the next line's start, less one.** The table holds only
 * starts, so each line is bounded by looking ahead - and the trailing spaces
 * are walked back over before drawing, then a NUL is written *into the
 * caller's string* to terminate it and the displaced byte is put back
 * afterwards. The string is modified and restored, which is why this cannot be
 * handed a string in read-only memory.
 *
 * Each line is drawn twice, colour 0xf one pixel left and one down and then
 * colour 5 at the true place - the same shadow the parts bin's numbers use.
 *
 * The loop ends on a null pointer, on a line that starts with a NUL, or when
 * the count runs out, and the count is tested **before** it is decremented, so
 * a count of one draws one line.
 */
void draw_wrapped_text(char *str, int16_t x, int16_t y, int16_t w, int16_t h)
{
    char     saved;                     /* [bp-1] */
    int16_t  left;                      /* [bp-4] */
    int16_t  top;                       /* [bp-6] */
    int16_t  line_height;               /* [bp-8] */
    int16_t  left_at;                   /* [bp-0xa] */
    char   **l;                         /* di */
    char    *end;                       /* si */

    VMDS.text_style = 1;                        /* transparent */
    line_height = font_line_height(0);

    wrap_text_to_box(str, w, h, line_height);

    left = x;
    top  = y;
    left += (w - GAME_PICKER_TEXT.text_width - 1) / 2;
    top  += (h - GAME_PICKER_TEXT.text_height - 1) / 2;
    top++;

    VMDS.clip_left   = left;
    VMDS.clip_right  = left + w;
    VMDS.clip_top    = top;
    VMDS.clip_bottom = top + h;

    l       = g_text_line;
    left_at = GAME_PICKER_TEXT.line_count;

    while (*l != 0 && **l != 0 && left_at-- != 0) {
        end = l[1] - 1;
        while (*l < end && (uint8_t)*end <= ' ')
            end--;
        end++;

        saved = *end;
        *end = 0;

        cursor_redraw_off_thunk();

        VMDS.text_colour = 0x0f;
        draw_string(*l, left - 1, top + 1);

        VMDS.text_colour = 5;
        draw_string(*l++, left, top);

        restore_cursor_following();

        *end = saved;
        top += line_height;
    }

    set_clip_full_screen();
}

/*
 * 0x13ed2
 *
 * **Break a string into lines that fit a box.** The line starts go into the
 * table from DGROUP 0x56a6, how many at 0x56a4, and the block's measured
 * height and width at 0x56a0 and 0x56a2 - which `draw_wrapped_text` then uses
 * to centre it.
 *
 * The height is capped at **seven lines** before anything else: `h` is reduced
 * to `7 * line_height` if it is larger, so a tall box does not make a tall
 * block. Seven is a constant in the code, not a table size.
 *
 * The measuring is by *word*, through `measure_word`, which answers the word's
 * width and its length. A word that does not fit starts a new line - and the
 * test is `width + word > box` **or** nothing has been placed on this line yet
 * and the block is not empty, so a single word wider than the box still gets a
 * line to itself rather than looping.
 *
 * A carriage return, 0x0d, forces a line break and the next line starts *after*
 * it. A space adds the width of a space - measured once at the top from a
 * two-byte string - and is otherwise skipped. Any other character at or below
 * a space ends the scan.
 *
 * The width recorded at 0x56a2 is the widest line, clamped to the box.
 *
 * **The last line is counted only if it has something on it**: after the loop,
 * a run width of zero with at least one line already recorded takes one back
 * off the count; otherwise the height gains one more line. Then the entry past
 * the last is set to the point the scan stopped at, which is what makes
 * `draw_wrapped_text`'s "end is the next start, less one" work for the final
 * line as well.
 */
void wrap_text_to_box(char *str, int16_t w, int16_t h, int16_t line_height)
{
    int16_t  used;                      /* [bp-2]   height used so far */
    int16_t  wide;                      /* [bp-4]   the word's width */
    int16_t  space_w;                   /* [bp-6] */
    int16_t  cap;                       /* [bp-8] */
    int16_t  len;                       /* [bp-0xa] the word's length */
    char     space[2];                  /* [bp-0xc], a two-byte " " */
    char    *at = str;                  /* si */
    int16_t  run;                       /* di       width on the current line */

    cap = line_height * 7;
    if (h > cap)
        h = cap;

    run = used = 0;
    GAME_PICKER_TEXT.line_count = 0;
    GAME_PICKER_TEXT.text_width = GAME_PICKER_TEXT.text_height = 0;

    if (*at != 0)
        g_text_line[GAME_PICKER_TEXT.line_count++] = at;

    space[0] = ' ';
    space[1] = 0;
    space_w = text_width_thunk(space);

    while (*at != 0 && used + line_height < h) {
        measure_word(at, &wide, &len);

        if ((run != 0 || used != 0) && run + wide >= w) {
            run = 0;
            used += line_height;
            g_text_line[GAME_PICKER_TEXT.line_count++] = at;
            if (used + line_height >= h)
                break;
        }

        at += len;
        run += wide;
        if (run > GAME_PICKER_TEXT.text_width)
            GAME_PICKER_TEXT.text_width = run;
        if (GAME_PICKER_TEXT.text_width > w)
            GAME_PICKER_TEXT.text_width = w;

        while (*at != 0 && (uint8_t)*at <= ' ' && used + line_height < h) {
            if (*at == 0x0d) {
                run = 0;
                used += line_height;
                g_text_line[GAME_PICKER_TEXT.line_count++] = at + 1;
            } else if (*at == ' ') {
                run += space_w;
            }
            at++;
        }
    }

    GAME_PICKER_TEXT.text_height = used;

    if (run == 0 && GAME_PICKER_TEXT.line_count != 0)
        GAME_PICKER_TEXT.line_count--;
    else
        GAME_PICKER_TEXT.text_height += line_height;

    g_text_line[GAME_PICKER_TEXT.line_count] = at;
}

/*
 * 0x1401d
 *
 * Measure one word: how wide it is and how long, answered through the two near
 * pointers it is given.
 *
 * A word runs to the first character **at or below a space** - so a space, a
 * carriage return and a NUL all end it, and `wrap_text_to_box` then decides
 * which of those it was.
 *
 * The width comes from `text_width`, and to get it the routine writes a NUL
 * over the terminator, measures, and puts the displaced byte back - the same
 * trick `draw_wrapped_text` uses on the same string, for the same reason:
 * `text_width` stops at a NUL and there is nowhere else to put one.
 *
 * The length is counted separately as the walk goes rather than taken from the
 * pointer difference.
 */
void measure_word(char *str, int16_t *out_width, int16_t *out_length)
{
    char *at  = str;
    int16_t  len = 0;
    char     saved;

    while ((uint8_t)*at > ' ') {
        at++;
        len++;
    }

    saved   = *at;
    *at = 0;

    *out_width = text_width(str);
    *out_length = len;

    *at = saved;
}
