/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game as a program**: `main`, the bring-up, the intro and the
 * copy protection, the round and screen state machines, the level loop and
 * the drag that places a part, and the teardown that prints your password.
 *
 * This file corresponds to the original's **code segment 0dff**, image
 * 0x0dff0..0x14de0. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * The DGROUP records only this file reads or writes. Each is laid over the
 * DGROUP byte array at the address its macro names, like the shared ones in
 * dgroup.h; they are declared here because nothing else uses them.
 */

struct game_master_levels GAME_MASTER_LEVELS DGROUP_AT(0x0116) = {
    { 0x0000, 0x0003, 0x0005, 0x0008, 0x000a, 0x000d, 0x000f }, /* master_level_ok */
};

/*
 * **The path separator**, DGROUP 0x1bca..0x1bcc, 0x02 bytes: a near pointer to
 * the backslash string, which the path builders concatenate.
 */
struct game_path_sep {
    uint16_t  path_sep_ptr;      /* +0x00 [2]  a near pointer to the "\\" at 0x236e, `DG1BCC.path_sep` */          /* +0x00 */
} PACKED;

struct game_path_sep GAME_PATH_SEP DGROUP_AT(0x1bca) = {
    0x236e, /* path_sep_ptr */
};


/*
 * **The characters a filename may not contain**, DGROUP 0x28ec..0x28fa, 0x0e bytes: fourteen
 * of them, `*` `/` `,` `-` `[` `]` `&` `@` `^` `%` `?` `(` `)` `:`, which
 * `validate_filename` tests one by one. The run ends at 0x28fa.
 */
struct game_forbidden_chars {
    uint8_t   forbidden[14] NONSTRING;  /* +0x00 [0xe]  a set, not a string */
} PACKED;

struct game_forbidden_chars GAME_FORBIDDEN_CHARS DGROUP_AT(0x28ec) = {
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

struct game_picker_tabs GAME_PICKER_TABS DGROUP_AT(0x28fa) = {
    0xffff, /* stop */
    { 0x0090, 0x0080, 0x00c0, 0x00d0, 0x00d0, 0x0060, 0x00e0 }, /* stop_x */
    { 0x005c, 0x0082, 0x0112, 0x0080, 0x00ec, 0x013a, 0x013a }, /* stop_y */
};

/*
 * **The file dialog's strings**, DGROUP 0x2918..0x2966: the ".TIM" extension
 * `pick_file` forces, the eleven DOS device names `validate_filename` refuses,
 * its mode, and the stars, patterns and dots the picker and the directory scan
 * use - each the copy one call site pushes.
 */
struct game_file_strings {
    char      tim[4];             /* +0x00  "TIM" */
    char      con[4];             /* +0x04  "con" */
    char      aux[4];             /* +0x08  "aux" */
    char      com1[5];            /* +0x0c  "com1" */
    char      com2[5];            /* +0x11  "com2" */
    char      com3[5];            /* +0x16  "com3" */
    char      com4[5];            /* +0x1b  "com4" */
    char      prn[4];             /* +0x20  "prn" */
    char      lpt1[5];            /* +0x24  "lpt1" */
    char      lpt2[5];            /* +0x29  "lpt2" */
    char      nul[4];             /* +0x2e  "nul" */
    char      null[5];            /* +0x32  "null" */
    char      mode_rb[3];         /* +0x37  "rb" */
    char      star_a[2];          /* +0x3a  "*" */
    char      star_b[2];          /* +0x3c  "*" */
    char      star_dot_star_a[4]; /* +0x3e  "*.*" */
    char      dot[2];             /* +0x42  "." */
    char      dot_dot_a[3];       /* +0x44  ".." */
    char      star_dot_star_b[4]; /* +0x47  "*.*" */
    char      dot_dot_b[3];       /* +0x4b  ".." */
} PACKED;

struct game_file_strings GAME_FILE_STRINGS DGROUP_AT(0x2918) = {
    "TIM", /* tim */
    "con", /* con */
    "aux", /* aux */
    "com1", /* com1 */
    "com2", /* com2 */
    "com3", /* com3 */
    "com4", /* com4 */
    "prn", /* prn */
    "lpt1", /* lpt1 */
    "lpt2", /* lpt2 */
    "nul", /* nul */
    "null", /* null */
    "rb", /* mode_rb */
    "*", /* star_a */
    "*", /* star_b */
    "*.*", /* star_dot_star_a */
    ".", /* dot */
    "..", /* dot_dot_a */
    "*.*", /* star_dot_star_b */
    "..", /* dot_dot_b */
};


struct game_directories GAME_DIRECTORIES DGROUP_BSS(0x530b);

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

struct game_name_buffer GAME_NAME_BUFFER DGROUP_BSS(0x5682);

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

struct game_picker_text GAME_PICKER_TEXT DGROUP_WAS(0x568f);

/*
 * NOT a transcription: the port's factoring of the eleven **identical inline
 * blocks** at 0x13205 to 0x133c3. Each is `strnicmp` against one reserved DOS
 * device name followed by a check that the byte after it ends the stem, and the
 * original repeats the whole thing eleven times rather than looping. Every
 * constant is kept, in the order the original tests them - including the last
 * pair, which do not agree with each other. The names are the DGROUP copies
 * each block pushes, 0x291c to 0x294a.
 */
static const struct {
    const char *name;
    uint16_t len;
    uint16_t after;
} reserved_names[] = {
    { GAME_FILE_STRINGS.con,  3, 3 },
    { GAME_FILE_STRINGS.aux,  3, 3 },
    { GAME_FILE_STRINGS.com1, 4, 4 },
    { GAME_FILE_STRINGS.com2, 4, 4 },
    { GAME_FILE_STRINGS.com3, 4, 4 },
    { GAME_FILE_STRINGS.com4, 4, 4 },
    { GAME_FILE_STRINGS.prn,  3, 3 },
    { GAME_FILE_STRINGS.lpt1, 4, 4 },
    { GAME_FILE_STRINGS.lpt2, 4, 4 },
    { GAME_FILE_STRINGS.nul,  3, 3 },
    { GAME_FILE_STRINGS.null, 3, 4 },   /* compared for THREE bytes - see below */
};

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
uint16_t pick_file(uint16_t arg1, uint16_t arg2, const char *pattern)
{
    char pat[38];                  /* [bp-0x26], 0x26 bytes */

    int16_t  reload    = 2;             /* [bp-6]    */
    int16_t  idx       = 0;             /* [bp-0xa]  */
    int16_t  rp_up     = 0;             /* [bp-0xc]  */
    int16_t  rp_down   = 0;             /* [bp-0xe]  */
    int16_t  rp_list   = 0;             /* [bp-0x10] */
    int16_t  rp_file   = 0;             /* [bp-0x12] */
    int16_t  rp_name   = 0;             /* [bp-0x14] */
    int16_t  valid;                     /* [bp-0x16] */
    int16_t  repaint   = 0;             /* si */
    uint16_t was       = 0x8000;        /* di */
    uint16_t answer;

    /*
     * **The pattern is the third argument, and it is copied.** `fill_file_listing`
     * takes it apart to build the extension filter and `string_chr` walks it in
     * place, so what the listing filters on is this copy and never the caller's
     * constant.
     */
    string_copy(pat, pattern);

    DG4E4E.name_buf[0] = 0;
    GAME_PICKER_TEXT.picker_mode = DG4E67.state;
    DG4E67.state = 0x8000;

    for (;;) {
        if (reload != 0) {
            picker_begin(arg1, arg2, pat);

            if (((uint16_t)GAME_PICKER_TEXT.entry_max) == 0) {
                answer = 0;
                goto out;
            }

            reload  = 0;
            repaint = 1;
        }

        update_button_state();
        DG52ED.last_key = (uint8_t)bios_read_key();

        if ((DG52ED.last_key) == '\t' && DG4E67.state != 0x4000
            && DG4E67.state != 0x1000)
            picker_tab();

        if (((DG52ED.last_key) == '\r' || (DG52ED.last_key) == ' '
             || (DG52ED.last_key) == 0x1b /* Esc */)
            && DG4E67.state == 0x4000)
            DG5768.button_left = 0;

        regions_handle_pointer(DG4E67.regions_c_ptr);

        if (DG4E67.state == 0x100)
            goto dispatch;

        /*
         * **The path field.** It is entered while the field has focus *or* had
         * it on the pass before - `was` is last pass's 0x4e6b, taken at the
         * bottom of the loop - because losing focus is what commits the typed
         * path, and by then 0x4e6b no longer says the field.
         */
        if (DG4E67.state == 0x4000 || was == 0x4000) {
            DG4E67.file_op_active = 1;

            if (((DG52ED.last_key) == '\r' || DG4E67.state != 0x4000)
                && was == 0x4000) {
                /*
                 * A path of exactly `X:` skips the first `chdir` and goes
                 * straight to the second. Every other path is handed to
                 * `chdir` **twice** - once to find out whether it is reachable
                 * and once to go there - and the drive is selected only after
                 * the second succeeds.
                 */
                if ((GAME_DIRECTORIES.path_field[1] == ':' && GAME_DIRECTORIES.path_field[2] == 0)
                    || dos_chdir((const char *)GAME_DIRECTORIES.path_field) == 0) {
                    if (dos_chdir((const char *)GAME_DIRECTORIES.path_field) == 0) {
                        dos_setdisk((uint8_t)GAME_DIRECTORIES.path_field[0]);
                        reload = 2;
                    } else {
                        dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);
                        show_message_box(DG1BCC.path_error, (char *)DG1BCC.path_error_body);
                        wait_cursor();
                        paint_panel_frame();
                        restore_cursor();
                        repaint = 1;
                        rp_name = 2;
                    }

                    if (DG4E67.state == 0x4000)
                        DG4E67.state = 0x8000;
                } else {
                    dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);
                    show_message_box(DG1BCC.path_error, (char *)DG1BCC.path_error_body);
                    wait_cursor();
                    paint_panel_frame();
                    restore_cursor();
                    repaint = 1;
                    rp_name = 2;

                    if (DG4E67.state == 0x4000)
                        DG4E67.state = 0x8000;
                }
            } else {
                if (was == 0x4000)
                    picker_type((DG52ED.last_key),
                        (char *)GAME_DIRECTORIES.path_field, 0x50);

                rp_name = 2;
            }

            DG4E67.file_op_active = 0;
        }

        /*
         * The name field, the same shape and a different buffer. The original
         * tests the mode **twice** on the way in - once for each half of the
         * `||` - and the second test can never fail once the first has let it
         * through, so the branch it guards is unreachable and is not written
         * out here.
         */
        if (DG4E67.state == 0x1000 || was == 0x1000) {
            if (((DG52ED.last_key) == '\r' || DG4E67.state != 0x1000)
                && was == 0x1000) {
                force_extension((char *)DG4E4E.name_buf, GAME_FILE_STRINGS.tim);

                if (DG4E67.state == 0x1000)
                    DG4E67.state = 0x8000;
            } else if (was == 0x1000) {
                picker_type((DG52ED.last_key), (char *)DG4E4E.name_buf,
                            sizeof DG4E4E.name_buf);
            }

            rp_file = 2;
        }

    dispatch:
        switch (DG4E67.state) {
        case 0x0800:                    /* the up arrow */
            if (DG5768.button_left == 1 || DG5768.button_left == 2) {
                int16_t v = (int16_t)(GAME_PICKER_TEXT.scroll - 1);

                if (v >= 0) {
                    GAME_PICKER_TEXT.scroll = v;
                    rp_list = 2;
                }
            } else {
                DG4E67.state = 0x8000;
            }
            rp_up = 2;
            break;

        case 0x0400:                    /* the down arrow */
            if (DG5768.button_left == 1 || DG5768.button_left == 2) {
                int16_t v = (int16_t)(GAME_PICKER_TEXT.scroll + 1);

                if ((int16_t)(GAME_PICKER_TEXT.entry_count - 12) >= v) {
                    GAME_PICKER_TEXT.scroll = v;
                    rp_list = 2;
                }
            } else {
                DG4E67.state = 0x8000;
            }
            rp_down = 2;
            break;

        case 0x2000: {                  /* a click in the listing */
            const char far *rec;

            /*
             * **Which row was clicked is arithmetic, not a hit test.** The
             * pointer's y at 0x5782 less the box's top, divided by the ten
             * pixels a row takes, plus the scroll position.
             */
            idx = (int16_t)((int16_t)(DG5768.pointer_y - 0x7c) / 10
                            + GAME_PICKER_TEXT.scroll);

            if (idx >= GAME_PICKER_TEXT.entry_count) {
                DG4E67.state = 0x8000;
                break;
            }

            /* Entry `idx` of the array of far pointers at the block's
               front, followed to the text it names. */
            {
                char far * far *entries =
                    (char far * far *)GAME_PICKER_TEXT.block;

                rec = entries[idx];
            }

            if (*rec != ':' && *rec != '<') {
                string_copy((char *)DG4E4E.name_buf, listing_to_name(rec));
                rp_file = 2;
                DG4E67.state = 0x8000;
                break;
            }

            /*
             * Row zero is the `:` when there is one, and the only way to tell
             * it from a directory called nothing is that we are not at a root.
             */
            if (idx != 0 || path_is_root((const char *)GAME_DIRECTORIES.path_field) != 0)
                path_join((char *)GAME_DIRECTORIES.path_field, rec);
            else
                path_up((char *)GAME_DIRECTORIES.path_field);

            DG4E67.file_op_active = 1;

            if (dos_chdir((const char *)GAME_DIRECTORIES.path_field) == 0)
                dos_setdisk((uint8_t)GAME_DIRECTORIES.path_field[0]);

            DG4E67.file_op_active = 0;
            reload = 2;
            DG4E4E.name_buf[0] = 0;
            DG4E67.state = 0x8000;
            break;
        }

        case 0x0200:                    /* the LOAD or SAVE button */
            valid = (int16_t)validate_filename();

            if (valid == 0) {
                picker_draw_action();
                show_message_box(DG1BCC.file_error,
                                 ((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x100
                                     ? (char *)DG1BCC.cant_open_for_loading
                                     : (char *)DG1BCC.cant_open_for_saving);
                wait_cursor();
                paint_panel_frame();
                restore_cursor();
                repaint = 1;
                DG4E67.state = 0x8000;
            } else if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x80) {
                if (valid == 2) {
                    picker_draw_action();

                    if (ask_yes_no(DG1BCC.overwrite_file, (char *)DG1BCC.overwrite_body)
                        == 0) {
                        wait_cursor();
                        paint_panel_frame();
                        restore_cursor();
                        repaint = 1;
                        DG4E67.state = 0x8000;
                    }
                }
            } else if (is_machine_file((char *)DG4E4E.name_buf) == 0) {
                picker_draw_action();
                show_message_box(DG1BCC.wrong_format, (char *)DG1BCC.wrong_format_body);
                wait_cursor();
                paint_panel_frame();
                restore_cursor();
                repaint = 1;
                DG4E67.state = 0x8000;
            }
            break;

        default:
            break;
        }

        was = DG4E67.state;

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

        if (DG4E67.state == 0x200 || DG4E67.state == 0x100)
            break;
    }

    /*
     * The listing block goes back **only when it is not the borrowed one**:
     * `picker_begin` will take the pointer at 0x3576 if there is one, and
     * freeing that would hand back memory the picker never owned.
     */
    if (GAME_PICKER_TEXT.block != DG3576.scratch) {
        dos_free_far(GAME_PICKER_TEXT.block);
        GAME_PICKER_TEXT.block = NULL;
        GAME_PICKER_TEXT.text_start = NULL;
    }

    picker_draw_action();

    if (DG4E67.state == 0x200 && string_length((const char *)DG4E4E.name_buf) != 0) {
        string_copy((char *)DG52FE.name, (const char *)DG4E4E.name_buf);
        answer = 1;
    } else {
        DG4E4E.name_buf[0] = 0;
        answer = 0;
    }

out:
    return answer;
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
    uint16_t i;
    FILE *file;
    int16_t  bad = 0;

    si = (char *)DG4E4E.name_buf;

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
        if (string_chr((char *)DG4E4E.name_buf,
                       GAME_FORBIDDEN_CHARS.forbidden[i]) != NULL)
            return 0;
    }

    for (i = 0; i < sizeof reserved_names / sizeof reserved_names[0]; i++) {
        uint16_t after;

        if (string_ncompare_i((const char *)DG4E4E.name_buf, reserved_names[i].name,
                              reserved_names[i].len) != 0)
            continue;

        after = (uint8_t)DG4E4E.name_buf[reserved_names[i].after];
        if (after == 0 || after == '.')
            return 0;
    }

    file = game_fopen((char *)DG4E4E.name_buf, GAME_FILE_STRINGS.mode_rb);

    if (file != 0) {
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
    if (DG4E67.state != 0x200) {
        draw_button(DG1BCC.cancel, 0xc0, 0x130, 1);
    } else if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x100) {
        draw_button(DG1BCC.load, 0x40, 0x130, 1);
    } else {
        draw_button(DG1BCC.save, 0x40, 0x130, 1);
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

    len = (int16_t)string_length(buf);

    if (c == '\b') {
        if (len != 0)
            buf[len - 1] = 0;
    } else if (len < max && c != '\t') {
        string_concat(buf, str);
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
    const char *last = 0;
    int16_t  n = 0;
    char sep = *(const char *)dg_near_ptr(GAME_PATH_SEP.path_sep_ptr);

    while (*si != 0) {
        if (*si == sep) {
            last = si;
            n++;
        }
        si++;
    }

    if (n == 1 && last[1] == 0)
        return 1;

    return 0;
}

/*
 * 0x13516
 *
 * **Drop the last component of a path**, in place. It walks to the terminator
 * counting separators - the character is not a literal here but `*GAME_PATH_SEP.path_sep_ptr`,
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
    char *last = 0;
    int16_t  n = 0;
    char sep = *(const char *)dg_near_ptr(GAME_PATH_SEP.path_sep_ptr);

    while (*si != 0) {
        if (*si == sep) {
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
    uint16_t len;

    while (*entry != 0) {
        entry++;
        name[di] = *entry;
        di++;
    }

    if (path_is_root(path) == 0)
        string_concat(path, (const char *)dg_near_ptr(GAME_PATH_SEP.path_sep_ptr));

    string_concat(path, name);

    len = string_length(path);
    path[len - 1] = 0;
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

    if (name[0] == 0)
        return;

    si = name;
    while (*si != 0 && *si != '.')
        si++;

    si[0] = '.';
    si[1] = 0;

    string_concat(name, ext);
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
    string_copy((char *)DG4E4E.name_buf, name);
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
    if (DG4E4E.name_buf[0] != 0)
        return (char *)DG4E4E.name_buf;

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
void picker_begin(uint16_t arg1, uint16_t arg2, const char *pattern)
{
    uint32_t v;

    (void)arg1;
    (void)arg2;

    if (GAME_PICKER_TEXT.block == FAR_NULL_PTR) {
        if (DG3576.scratch != FAR_NULL_PTR) {
            GAME_PICKER_TEXT.entry_max = 0x3e8;
            GAME_PICKER_TEXT.block = DG3576.scratch;
        } else {
            v = dos_alloc_bytes(0xffffffffu, 0, 0).bytes;

            if ((int32_t)v > 0x7530)
                v = 0x7530;

            GAME_PICKER_TEXT.entry_max = (uint16_t)long_divide((int32_t)v, 0x16);

            GAME_PICKER_TEXT.block = (dos_alloc_bytes(v, 0, 0).ptr);
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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    draw_title_bar(0x30, 0x31, 0x110, 0x149, 1);

    draw_sunken_box(0x36, 0x129, 0x40, 0x20);
    draw_sunken_box(0xb6, 0x129, 0x50, 0x20);

    if (((uint16_t)GAME_PICKER_TEXT.picker_mode) == 0x100) {
        draw_scroll_text(DG1BCC.load_machine, 0x50, 0x34, 0xa0);
        draw_button(DG1BCC.load, 0x40, 0x130, 0);
    } else {
        draw_scroll_text(DG1BCC.save_machine, 0x50, 0x34, 0xa0);
        draw_button(DG1BCC.save, 0x40, 0x130, 0);
    }

    draw_sunken_box(0xbc, 0x74, 0x20, 0x20);
    draw_sunken_box(0xbc, 0xe0, 0x20, 0x20);

    picker_draw_up();
    picker_draw_down();

    draw_button(DG1BCC.cancel, 0xc0, 0x130, 0);

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
    int16_t pressed = (DG4E67.state == 0x800) ? 1 : 0;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[pressed + 0x25]),
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
    int16_t pressed = (DG4E67.state == 0x400) ? 1 : 0;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();
    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.panel_art_ptr)->bmp_ptr[pressed + 0x27]),
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

    string_copy(buf, (const char *)GAME_DIRECTORIES.path_field);

    while ((int16_t)text_width_thunk(si) > 0xac)
        si++;

    if (DG4E67.state == 0x4000) {
        DG5677.caret_blink++;
        if ((DG5677.caret_blink & 8) != 0)
            string_concat(si, GAME_FILE_STRINGS.star_a);
    }

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
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

    string_copy(buf, (const char *)DG4E4E.name_buf);

    while ((int16_t)text_width_thunk(si) > 0x64)
        si++;

    if (DG4E67.state == 0x1000) {
        DG5677.caret_blink_b++;
        if ((DG5677.caret_blink_b & 8) != 0)
            string_concat(si, GAME_FILE_STRINGS.star_b);
    }

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    draw_scroll_text(DG1BCC.file_name, 0x30, 0x10c, 0x54);
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
    int16_t  x = 0x40;                  /* [bp-0xa] */
    int16_t  y = 0x78;                  /* di */
    int16_t  w = 0x70;                  /* [bp-0xc] */
    int16_t  room = 0x80;               /* [bp-0xe] */
    /* The block is an array of far pointers, one per entry. */
    char far * far *p;                  /* [bp-4], [bp-2] */
    int16_t  top, i;

    fill_panel_area(x, y, w, room, 0);

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    VMDS.text_style = 1;                    /* transparent text */
    VMDS.text_colour = 0x0f;

    if (GAME_PICKER_TEXT.entry_count > 0x0c) {
        top = GAME_PICKER_TEXT.scroll;
        if (top > GAME_PICKER_TEXT.entry_count)
            top = (int16_t)(GAME_PICKER_TEXT.entry_count - 12);
    } else {
        top = 0;
    }

    p = (char far * far *)GAME_PICKER_TEXT.block;
    p += top;                           /* skip the rows scrolled past */

    i = 0;
    while (i < GAME_PICKER_TEXT.entry_count && room >= 0x0a) {
        const char far *t = *p;

        p++;
        if (*t == ':')
            t = DG1BCC.parent_dir;

        cursor_redraw_off_thunk();
        draw_string_body(t,
                         (int16_t)(x + 4), (int16_t)(y + 4));
        restore_cursor_following();

        y    += 0x0a;
        room -= 0x0a;
        i++;
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
void fill_file_listing(const char *pattern)
{
    /* Two cursors into the one block: the array of far pointers at its
       front, and the text they point at, both stepped inside one segment. */
    char far * far *ptr;                /* [bp-4], [bp-2]: into the array */
    char far *txt;                      /* [bp-8], [bp-6]: into the text */
    const char *want_ext;                  /* [bp+6], rewritten in place */
    char *name;                      /* di */
    const char *name_ext;                  /* [bp-0xa]                        */
    int16_t  n;                         /* [bp-0xe]                        */
    uint16_t more;                      /* [bp-0xc]                        */

    GAME_PICKER_TEXT.entry_count = 0;
    dos_get_cur_dir((char *)GAME_DIRECTORIES.path_field);

    ptr = (char far * far *)GAME_PICKER_TEXT.block;
    txt = (char far *)GAME_PICKER_TEXT.text_start;

    want_ext = string_chr((char *)pattern, '.');
    if (want_ext != NULL && want_ext[1] == '*')
        want_ext = NULL;

    if (GAME_DIRECTORIES.path_field[3] != 0) {
        *ptr++ = txt;

        *txt++ = ':';
        *txt++ = 0;

        GAME_PICKER_TEXT.entry_count++;
    }

    more = dos_findfirst(GAME_FILE_STRINGS.star_dot_star_a, 0x10);

    while (more == 0 && ((uint16_t)GAME_PICKER_TEXT.entry_count) < ((uint16_t)GAME_PICKER_TEXT.entry_max)) {
        name     = dos_find_name();
        name_ext = string_chr(name, '.');

        if ((dos_find_attr() & 0x10) != 0) {
            if (string_compare(name, GAME_FILE_STRINGS.dot) != 0
                && string_compare(name,
                                  GAME_FILE_STRINGS.dot_dot_a) != 0) {
                *ptr++ = txt;
                GAME_PICKER_TEXT.entry_count++;

                *txt++ = '<';

                do {
                    *txt++ = *name;
                } while (*name++ != 0);

                txt[-1] = '>';
                *txt++  = 0;
            }
        } else if (want_ext == NULL
                   || (name_ext[1] == want_ext[1]
                       && name_ext[2] == want_ext[2]
                       && name_ext[3] == want_ext[3])) {
            *ptr++ = txt;
            GAME_PICKER_TEXT.entry_count++;

            n = 0;
            while (*name != 0 && *name != '.') {
                *txt++ = *name;
                name++;
                n++;
            }

            while (n < 8) {
                *txt++ = ' ';
                n++;
            }

            do {
                *txt++ = *name;
            } while (*name++ != 0);
        }

        more = dos_findnext(GAME_FILE_STRINGS.star_dot_star_b, 0x10);
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
       and `p[1]` is the next, which is what the original's `+ 4` is. */
    char far * far *p;                  /* [bp-4], [bp-2] */
    int16_t  swapped = 1;

    while (swapped) {
        swapped = 0;

        p = (char far * far *)GAME_PICKER_TEXT.block;

        /* Skip the ":" entry - the current directory - if it is first, so
           the sort below never moves it. */
        if (p[0] != NULL) {
            if (*p[0] == ':')
                p++;
        }

        while (p[0] != NULL && p[1] != NULL) {
            /* The pointers are swapped; the names are read through. */
            char far *a = p[0];
            char far *b = p[1];
            const char far *name_a = a;
            const char far *name_b = b;
            int16_t  swap = 0;

            /* "<PARENT DIR>" and the directories sort first. */
            if (*name_a != '<' && *name_b == '<')
                swap = 1;
            else if (*name_a == '<' && *name_b != '<')
                swap = 0;
            else if (far_stricmp(name_a, name_b) > 0)
                swap = 1;

            if (swap) {
                p[0] = b;
                p[1] = a;
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
        return GAME_FILE_STRINGS.dot_dot_b;

    si = (char *)GAME_NAME_BUFFER.name;

    while (*entry != 0) {
        char c = *entry;

        if (c != '<' && c != '>' && c != ' ') {
            *si = c;
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
    uint16_t line_height;
    uint16_t i;
    int16_t  left, top, left_at;

    VMDS.text_style = 1;                        /* transparent */
    line_height = font_line_height(0);

    wrap_text_to_box(str, w, h, line_height);

    left = (int16_t)(x + (w - GAME_PICKER_TEXT.text_width - 1) / 2);
    top  = (int16_t)(y + (h - GAME_PICKER_TEXT.text_height - 1) / 2 + 1);

    VMDS.clip_left   = left;
    VMDS.clip_right  = (int16_t)(left + w);
    VMDS.clip_top    = top;
    VMDS.clip_bottom = (int16_t)(top + h);

    i       = 0;
    left_at = GAME_PICKER_TEXT.line_count;

    while (GAME_TEXT_LINES.line_ptr[i] != 0 && *dg_near_ptr(GAME_TEXT_LINES.line_ptr[i]) != 0
           && left_at-- != 0) {
        char *start = (char *)dg_near_ptr(GAME_TEXT_LINES.line_ptr[i]);
        char *end   = (char *)dg_near_ptr(GAME_TEXT_LINES.line_ptr[i + 1]) - 1;
        char  saved;

        while (end > start && (uint8_t)*end <= ' ')
            end--;
        end++;

        saved = *end;
        *end = 0;

        cursor_redraw_off_thunk();

        VMDS.text_colour = 0x0f;
        draw_string(start, (int16_t)(left - 1), (int16_t)(top + 1));

        VMDS.text_colour = 5;
        draw_string(start, left, top);

        restore_cursor_following();

        *end = saved;
        i++;
        top = (int16_t)(top + line_height);
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
void wrap_text_to_box(char *str, int16_t w, int16_t h, uint16_t line_height)
{
    char space[2];            /* [bp-0xc], a two-byte " " */
    int16_t o_len[3];   /* [bp-0xa] */
    int16_t o_wide[2];   /* [bp-4]   */
    char    *at     = str;
    int16_t  used   = 0;         /* height used so far */
    int16_t  run    = 0;         /* width on the current line */
    int16_t  space_w;
    int16_t  cap    = (int16_t)(line_height * 7);

    if (h > cap)
        h = cap;

    GAME_PICKER_TEXT.line_count = 0;
    GAME_PICKER_TEXT.text_height  = 0;
    GAME_PICKER_TEXT.text_width  = 0;

    if (*at != 0) {
        GAME_TEXT_LINES.line_ptr[(uint16_t)GAME_PICKER_TEXT.line_count] = dg_near(dgroup, at);
        GAME_PICKER_TEXT.line_count++;
    }

    (*space)     = ' ';
    space[1] = 0;
    space_w = (int16_t)text_width_thunk(space);

    while (*at != 0 && (int16_t)(used + line_height) < h) {
        int16_t word_w, word_len;

        measure_word(at, (uint8_t *)o_wide,
                     (uint8_t *)o_len);
        word_w   = o_wide[0];
        word_len = o_len[0];

        if ((run != 0 || used == 0) && (int16_t)(run + word_w) >= w) {
            run  = 0;
            used = (int16_t)(used + line_height);
            GAME_TEXT_LINES.line_ptr[(uint16_t)GAME_PICKER_TEXT.line_count] = dg_near(dgroup, at);
            GAME_PICKER_TEXT.line_count++;
            if ((int16_t)(used + line_height) >= h)
                break;
        }

        at += word_len;
        run = (int16_t)(run + word_w);
        if (run > GAME_PICKER_TEXT.text_width)
            GAME_PICKER_TEXT.text_width = run;
        if (GAME_PICKER_TEXT.text_width > w)
            GAME_PICKER_TEXT.text_width = w;

        while (*at != 0 && (uint8_t)*at <= ' '
               && (int16_t)(used + line_height) < h) {
            if (*at == 0x0d) {
                run  = 0;
                used = (int16_t)(used + line_height);
                GAME_TEXT_LINES.line_ptr[(uint16_t)GAME_PICKER_TEXT.line_count] = dg_near(dgroup, at + 1);
                GAME_PICKER_TEXT.line_count++;
            } else if (*at == ' ') {
                run = (int16_t)(run + space_w);
            }
            at++;
        }
    }

    GAME_PICKER_TEXT.text_height = used;

    if (run == 0 && ((uint16_t)GAME_PICKER_TEXT.line_count) != 0)
        GAME_PICKER_TEXT.line_count--;
    else
        GAME_PICKER_TEXT.text_height = (int16_t)(GAME_PICKER_TEXT.text_height + line_height);

    GAME_TEXT_LINES.line_ptr[(uint16_t)GAME_PICKER_TEXT.line_count] = dg_near(dgroup, at);
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
void measure_word(char *str, uint8_t * out_width, uint8_t * out_length)
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

    *(int16_t *)(out_width) = (int16_t)text_width(str);
    *(int16_t *)(out_length) = len;

    *at = saved;
}

/*
 * 0x1405b
 *
 * Build the list of parts a level may use, and reset the machine's state around
 * it: the list head at DGROUP 0x50d7, the two pairs at 0x5179 and 0x521b, the
 * play area at 0x50af..0x50b5, and the two at 0x4ead.
 *
 * Parts 0 to 0x32 are all included except in three cases. **0x14, 0x29 and 0x31
 * are never included**, and are excluded by falling into a branch that leaves
 * the flag clear rather than by being tested against a list. And **0x20, 0x21
 * and 0x22 are conditional**, each on its own word - 0x4e7d, 0x4e81 and 0x4e7b -
 * which is what makes three of the parts appear only when the game says so.
 *
 * The three conditionals are written as three independent `if`s inside the same
 * branch rather than as a switch, so a part number that is not one of the three
 * reaches the end of them with its flag still clear and is left out too - which
 * cannot happen, because only those three get in there.
 *
 * The play area is 0x43,0x110 to -8,-8 - the negative pair being the origin
 * rather than a size, which is worth saying because it reads like a mistake.
 */
void build_part_list(void)
{
    int16_t si;

    DG50D3.parts_bin.prev_ptr = 0;
    DG50D3.parts_bin.next_ptr = 0;
    DG5179.moving_parts.prev_ptr = 0;
    DG5179.moving_parts.next_ptr = 0;
    DG521B.placed_parts.prev_ptr = 0;
    DG521B.placed_parts.next_ptr = 0;

    for (si = 0; si < 0x33; si++) {
        int16_t wanted = 0;

        if (si == 0x20 || si == 0x21 || si == 0x22) {
            if (si == 0x20 && ((uint16_t)DG4E67.holiday_halloween) != 0)
                wanted = 1;
            if (si == 0x21 && ((uint16_t)DG4E67.holiday_valentine) != 0)
                wanted = 1;
            if (si == 0x22 && ((uint16_t)DG4E67.holiday_christmas) != 0)
                wanted = 1;
        } else if (si != 0x14 && si != 0x29 && si != 0x31) {
            wanted = 1;
        }

        if (wanted != 0) {
            struct part *rec = make_part((uint16_t)si);

            if (rec != PART_NONE)
                insert_sorted(rec, &DG50D3.parts_bin);
        }
    }

    DG50D3.bin_list_ptr = dg_near(dgroup, &DG50D3.parts_bin);
    DG50AF.bonus_2 = 0;
    DG50AF.bonus_1 = 0;
    DG50AF.gravity = 0x43;
    DG50AF.air = 0x110;
    DG50AF.extent_x = -8;
    DG50AF.extent_y = -8;
    DG50AF.tune = 0x3e9;
    DG4E67.counter = 0;

    recompute_kind_physics();
}

/*
 * 0x14133
 *
 * Make one part: a 0xa2-byte record off the near heap, filled from the
 * sixteen-byte-per-part table at DGROUP 0x2966 and the bitmap list
 * `load_part_bitmap` left at 0xeba.
 *
 * The fields that come across are the part's kind at +6, its size at +0xa and
 * +0x50/+0x52, its extent at +0x44/+0x46, its bitmaps at +0x80 and a word at
 * +0x94. The two at +0x8c and +0x8e start at -1 rather than 0, which is what
 * "no link" looks like everywhere else in this game.
 *
 * Each part may also have an **init function** in the table, at +12 of its
 * entry, and a part that answers 1 from it is refused - the record is freed and
 * the answer is `PART_NONE`, offset 0. The port dispatches that far pointer on
 * its value, as it does everywhere else it cannot call one.
 *
 * The heap is checked three times: before the allocation, after it, and at the
 * end.
 */
struct part *make_part(uint16_t kind)
{
    struct part *part = PART_NONE;
    int16_t failed = 0;

    heap_check_or_hang();

    /* A refusal is the offset 0 `or ax,ax` at 0x14159 tests, so `part` is
       no part on the `done` path below. */
    part = (struct part *)(void *)heap_calloc_far(1, sizeof(struct part));
    if (part == NULL) {
        part = PART_NONE;
        failed = 1;
        goto done;
    }

    heap_check_or_hang();

    part->kind = kind;
    part->flags_06 = PART_TEMPLATES[kind].flags_06;
    part->flags_0a = PART_TEMPLATES[kind].flags_0a;
    part->set_size.width = PART_TEMPLATES[kind].set_size.width;
    part->set_size.height = PART_TEMPLATES[kind].set_size.height;
    part->size[0].width = PART_TEMPLATES[kind].size.width;
    part->size[0].height = PART_TEMPLATES[kind].size.height;
    part->point_count =
        PART_KINDS[kind].point_count;
    part->start_x = 0xffff;
    part->start_y = 0xffff;
    if (PART_TEMPLATES[kind].init != NULL
        && PART_TEMPLATES[kind].init(part) == 1) {
        failed = 1;
        goto done;
    }

    part->start_flags = part->flags_08;

    set_object_extent(part);

    part->mirror_size.height = part->size[0].height;
    part->mirror_size.width = part->size[0].width;

    heap_check_or_hang();

done:
    if (failed != 0) {
        if (part != PART_NONE)
            free_part(part);
        return PART_NONE;
    }

    return part;
}

/*
 * 0x14236 .. 0x14d42 - the **part initialisers**, fifty-one routines.
 *
 * The table of part kinds at DGROUP 0x2966 carries one far pointer each, at
 * +0x0c, and `make_part` calls it through that. Fifty-eight kind
 * slots reach fifty-one distinct routines: five kinds have no initialiser at
 * all and three - 1, 46 and 48 - share 0x14267.
 *
 * Nearly all of them are the same four steps:
 *
 *   1. OR some bits into the part's flags at +6, +8 and +0x0a, if it has any;
 *   2. take four bytes per bitmap - `heap_calloc_far(count, 4)` - into +0x82;
 *   3. refuse, by answering 1, if that allocation failed;
 *   4. call the part's own setup in segment 0x172c, and answer 0.
 *
 * Three skip step 2 - 0x147a7, 0x148e0 and 0x148ff call their setup with no
 * allocation. Two more skip both: 0x14aa2 and 0x14c48 only set flags and
 * bytes. And three allocate something else instead - 0x143fb and 0x1449d a
 * 0x2c-byte belt at +0x66, 0x1443d a 0x38-byte rope at +0x54 - each writing
 * the part's own address into the new record as its back-pointer.
 *
 * **They were a table until 2026-09-11**, six columns standing in for the
 * bodies: three flag words, the setup, a list of stores and a flag for
 * whether it allocated. That is not what the binary holds. The constants live
 * as immediates inside fifty-one separate functions - searching the whole
 * image for any two of them adjacent as data finds nothing - and the form
 * cost three defects, every one recorded in a comment beside it. The worst is
 * the one it could not report: the table had **forty-eight** of the fifty-one,
 * and 0x14ca0, 0x14cd9 and 0x14d0a were missing outright.
 */

/* 0x14236 */
uint16_t part_init_bowling_ball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_0001(part);
    return 0;
}

/* 0x14267 */
uint16_t part_init_14267(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0040);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0180);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_48ab(part);
    return 0;
}

/* 0x142a1 */
uint16_t part_init_ramp(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0600);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0080);
    part->form = 0x0001;
    part->start_form = 0x0001;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_ramp(part);
    return 0;
}

/* 0x142e6 */
uint16_t part_init_seesaw(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x000c);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_seesaw(part);
    return 0;
}

/* 0x14320 */
uint16_t part_init_balloon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 16;
    part->attach[0].y = 47;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_balloon(part);
    return 0;
}

/* 0x14361 */
uint16_t part_init_conveyor(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0081);
    part->form = 0x001c;
    part->start_form = 0x001c;
    part->direction = 0x0000;
    part->start_direction = 0x0000;
    part->grab.x = 59;
    part->grab_size = 0x000e;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_conveyor(part);
    return 0;
}

/* 0x143b3 */
uint16_t part_init_mouse_cage(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0801);
    part->grab.x = 30;
    part->grab.y = 4;
    part->grab_size = 0x000c;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_mouse_cage(part);
    return 0;
}

/* 0x143fb */
uint16_t part_init_pulley(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 0;
    part->attach[0].y = 8;
    part->attach[1].x = 15;
    part->attach[1].y = 8;

    part->belt_ptr[0] = dg_near(dgroup, heap_calloc_far(1, 0x2c));
    if (part->belt_ptr[0] == 0)
        return 1;
    BELT_PTR(part->belt_ptr[0])->owner_ptr = dg_near(dgroup, part);
    return 0;
}

/* 0x1443d */
uint16_t part_init_belt(struct part *part)
{
    part->rope_ptr = dg_near(dgroup, heap_calloc_far(1, 0x38));
    if (part->rope_ptr == 0)
        return 1;
    ROPE_PTR(part->rope_ptr)->owner_ptr = dg_near(dgroup, part);
    return 0;
}

/* 0x1446c */
uint16_t part_init_basketball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_0001(part);
    return 0;
}

/* 0x1449d */
uint16_t part_init_rope(struct part *part)
{
    part->belt_ptr[0] = dg_near(dgroup, heap_calloc_far(1, 0x2c));
    if (part->belt_ptr[0] == 0)
        return 1;
    BELT_PTR(part->belt_ptr[0])->owner_ptr = dg_near(dgroup, part);
    return 0;
}

/* 0x144cb */
uint16_t part_init_bird_cage(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 21;
    part->attach[0].y = 2;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_bird_cage(part);
    return 0;
}

/* 0x1450c */
uint16_t part_init_pokey(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x8000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_pokey(part);
    return 0;
}

/* 0x14547 */
uint16_t part_init_jack_in_the_box(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1001);
    part->grab.x = 8;
    part->grab.y = 9;
    part->grab_size = 0x000e;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_jack_in_the_box(part);
    return 0;
}

/* 0x1458f */
uint16_t part_init_gear(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0001);
    part->grab.y = 13;
    part->grab.x = 13;
    part->grab_size = 0x0008;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_0001(part);
    return 0;
}

/* 0x145d1 */
uint16_t part_init_bob_the_fish(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_bob_the_fish(part);
    return 0;
}

/* 0x14607 */
uint16_t part_init_bellow(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_bellow(part);
    return 0;
}

/* 0x1463d */
uint16_t part_init_bucket(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 18;
    part->attach[0].y = 0;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_bucket(part);
    return 0;
}

/* 0x1467e */
uint16_t part_init_cannon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_cannon(part);
    return 0;
}

/* 0x146bd */
uint16_t part_init_dynamite(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0420);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_dynamite(part);
    return 0;
}

/* 0x146fc */
uint16_t part_init_146fc(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_08a1(part);
    return 0;
}

/* 0x1472d */
uint16_t part_init_electric_plug(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_electric_plug(part);
    return 0;
}

/* 0x1476c */
uint16_t part_init_dynamite_plunger(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_dynamite_plunger(part);
    return 0;
}

/* 0x147a7 */
uint16_t part_init_hook(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part_setup_hook(part);
    return 0;
}

/* 0x147c5 */
uint16_t part_init_fan(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0001);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_fan(part);
    return 0;
}

/* 0x14804 */
uint16_t part_init_flashlight(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_flashlight(part);
    return 0;
}

/* 0x1483a */
uint16_t part_init_generator(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1001);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_generator(part);
    return 0;
}

/* 0x14874 */
uint16_t part_init_gun(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_gun(part);
    return 0;
}

/* 0x148af */
uint16_t part_init_baseball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_00c9(part);
    return 0;
}

/* 0x148e0 */
uint16_t part_init_light(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0200);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1004);

    part_setup_light(part);
    return 0;
}

/* 0x148ff */
uint16_t part_init_magnifying_glass(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part_setup_magnifying_glass(part);
    return 0;
}

/* 0x14919 */
uint16_t part_init_monkey(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1805);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_monkey(part);
    return 0;
}

/* 0x14954 */
uint16_t part_init_pumpkin(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_pumpkin(part);
    return 0;
}

/* 0x14985 */
uint16_t part_init_heart_balloon(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 18;
    part->attach[0].y = 35;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_heart_balloon(part);
    return 0;
}

/* 0x149c6 */
uint16_t part_init_christmas_tree(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_christmas_tree(part);
    return 0;
}

/* 0x149f7 */
uint16_t part_init_boxing_glove(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_boxing_glove(part);
    return 0;
}

/* 0x14a2d */
uint16_t part_init_rocket(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_rocket(part);
    return 0;
}

/* 0x14a67 */
uint16_t part_init_scissors(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_scissors(part);
    return 0;
}

/* 0x14aa2 */
uint16_t part_init_solar_panel(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0002);

    return 0;
}

/* 0x14ab9 */
uint16_t part_init_trampoline(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_trampoline(part);
    return 0;
}

/* 0x14aef */
uint16_t part_init_windmill(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0801);
    part->grab.x = 15;
    part->grab.y = 15;
    part->grab_size = 0x0008;

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_windmill(part);
    return 0;
}

/* 0x14b37 */
uint16_t part_init_mort_the_mouse(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x8000);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_mort_the_mouse(part);
    return 0;
}

/* 0x14b72 */
uint16_t part_init_cannon_ball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_cannon_ball(part);
    return 0;
}

/* 0x14ba3 */
uint16_t part_init_tennis_ball(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_00c9(part);
    return 0;
}

/* 0x14bd4 */
uint16_t part_init_candle(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x1000);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_candle(part);
    return 0;
}

/* 0x14c12 */
uint16_t part_init_corner_pipe(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0600);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_corner_pipe(part);
    return 0;
}

/* 0x14c48 */
uint16_t part_init_14c48(struct part *part)
{
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);
    part->attach[0].x = 0;
    part->attach[0].y = 0;

    return 0;
}

/* 0x14c62 */
uint16_t part_init_motor(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0400);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0001);
    part->flags_0a =
        (uint16_t)(part->flags_0a | 0x0001);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_motor(part);
    return 0;
}

/* 0x14ca0 */
uint16_t part_init_14ca0(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_1105(part);
    return 0;
}

/* 0x14cd9 */
uint16_t part_init_14cd9(struct part *part)
{
    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_10b6(part);
    return 0;
}

/* 0x14d0a */
uint16_t part_init_14d0a(struct part *part)
{
    part->flags_06 =
        (uint16_t)(part->flags_06 | 0x0020);
    part->flags_08 =
        (uint16_t)(part->flags_08 | 0x0004);

    part->points_ptr =
        dg_near(dgroup, heap_calloc_far(part->point_count, 4));
    if (part->points_ptr == 0)
        return 1;

    part_setup_1105(part);
    return 0;
}

/*
 * 0x14d43
 *
 * Throw the whole machine away: every part on the three lists at DGROUP
 * 0x50d7, 0x521b and 0x5179 is freed and the three heads cleared. The intro
 * calls it between one animation and the next, which is why the credits get a
 * clean machine rather than the title screen's leftovers.
 */
void free_all_lists(void)
{
    free_part_list(PART_PTR(DG50D3.parts_bin.next_ptr));
    free_part_list(PART_PTR(DG521B.placed_parts.next_ptr));
    free_part_list(PART_PTR(DG5179.moving_parts.next_ptr));

    DG50D3.parts_bin.next_ptr = 0;
    DG5179.moving_parts.next_ptr = 0;
    DG521B.placed_parts.next_ptr = 0;
}

/*
 * 0x14d71
 *
 * Free every part on one list. The next pointer is taken out of the record
 * *before* the record is freed, which is the only way to walk a list you are
 * destroying.
 */
void free_part_list(struct part *si)
{
    /* `or si,si` at 0x14d8c: the list ends on an offset of 0, which as a
       pointer is `PART_NONE` and never NULL. */
    while (si != PART_NONE) {
        struct part *next = PART_PTR(si->next_ptr);

        free_part(si);
        si = next;
    }
}

/*
 * 0x14d95
 *
 * Give a part back: its per-bitmap array, then two records it may or may not
 * own, then the part itself. Every free goes through the checked one, so a
 * corrupt heap stops here rather than later.
 *
 * The two conditions are the interesting part. The record at +0x54 is freed
 * only when bit 0 of the flags at +8 is **clear** - with it set the record
 * belongs to something else and freeing it would be a double free. And the
 * record at +0x66 is freed only for parts 7 and 0x0a, compared by number
 * rather than by a flag: two particular parts allocate it and the rest leave
 * the field as whatever it was.
 *
 * A null part is not an error; it returns.
 */
void free_part(struct part *part)
{
    if (part == PART_NONE)   /* the offset: `or si,si` at 0x14d9c */
        return;

    if (part->points_ptr != 0)
        checked_free(dg_near_ptr(part->points_ptr));

    if (part->rope_ptr != 0
        && (part->flags_08 & 1) == 0)
        checked_free(dg_near_ptr(part->rope_ptr));

    if (part->belt_ptr[0] != 0
        && (part->kind == KIND_PULLEY
            || part->kind == KIND_ROPE))
        checked_free(dg_near_ptr(part->belt_ptr[0]));

    checked_free((uint8_t *)part);
}
