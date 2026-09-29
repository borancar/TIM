/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Choosing a puzzle, and the parts' pictures**: the puzzle picker - its
 * list, its arrows, the password box - and the loading and freeing of the
 * part bitmaps.
 *
 * The fourth module of the original's **code segment 0dff**, image
 * 0x0f0b0..0x0f8c2. Its data is DGROUP 0x260a..0x262f: the picker's tab stops
 * and then its literal pool. Where it ends is not in the bytes: the eight
 * routines from 0x0f8c2 to 0x0ff80 name no data of their own and make no call
 * back into either neighbour, so they could close this module or open the
 * next with the same code; they open the next, where the game screen's loop
 * belongs. Functions are in address order and each carries the image offset
 * it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -d
 * JUDGE: data 0x260a..0x262f
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
 * **Where Tab sends the pointer on the puzzle picker's controls**, DGROUP 0x260a..0x2620, 0x16 bytes: which
 * stop it is on - 0xffff until the first Tab, and back to 0 past the last
 * - and the x of each, the y being fixed.
 */
struct game_puzzle_tabs {
    uint16_t  stop;          /* +0x00 [2]  which of the puzzle screen's five tab stops */
    int16_t   stop_x[5];          /* +0x02 [0xa]  where `puzzle_tab` parks the pointer */
    int16_t   stop_y[5];          /* +0x0c [0xa] */
} PACKED;

struct game_puzzle_tabs GAME_PUZZLE_TABS DGROUP_AT(0x260a) = {
    0xffff, /* stop */
    { 0x0080, 0x00d0, 0x01e0, 0x01e0, 0x0208 }, /* stop_x */
    { 0x0052, 0x0142, 0x004e, 0x0114, 0x0140 }, /* stop_y */
};

/*
 * **The module's literal pool**, DGROUP 0x2620..0x262f: "*", ": ", "part"
 * and ".bmp" - `puzzle_draw_password` the star, `puzzle_draw_list` the
 * separator, and `load_part_bitmap`, which builds "partNN.bmp", the last two.
 * The zero byte after it is the next module's `_DATA` starting on a word.
 */
struct game_part_names {
    char      star[2];            /* +0x00  "*" */
    char      title_sep[3];       /* +0x02  ": " */
    char      part[5];            /* +0x05  "part" */
    char      bmp[5];             /* +0x0a  ".bmp" */
} PACKED;

struct game_part_names GAME_PART_NAMES DGROUP_AT(0x2620) = {
    "*", /* star */
    ": ", /* title_sep */
    "part", /* part */
    ".bmp", /* bmp */
};

/*
 * **The text the player types on the puzzle screen**, DGROUP 0x542e..0x5456, 0x28 bytes - a
 * password or a score code - which `picker_type` fills to 0x19 characters
 * and `password_to_level`, `score_code_to_score` and `puzzle_draw_password`
 * read. Forty bytes, up to DG5456.
 */
struct game_typed_text {
    char typed[0x28];             /* +0x00 [0x28] */
} PACKED;

struct game_typed_text GAME_TYPED_TEXT DGROUP_BSS(0x542e);

/*
 * **The picker's own state**, DGROUP 0x5428..0x542e. Declared after the typed
 * text because Borland lays a module's uninitialised data out from the last
 * declaration up.
 */
struct puzzle_state {
    /* Ours in name only: the password field's cursor blink, stepped every time
       the puzzle picker redraws the line and showing a star while bit 3 is
       set. */
    uint16_t  password_blink;     /* +0x00  0x5428 */
    int16_t   selected_level;     /* +0x02  0x542a  the puzzle picker's row;
                                     game_round copies it to round_number */
    /* **The first puzzle the list shows**, from `puzzle_page_of_score`, and
       the arrows page it by 0x15 - the twenty-one rows a page holds - with 1
       as the floor and the level count as the ceiling. */
    int16_t   puzzle_page;        /* +0x04  0x542c */
} PACKED;

struct puzzle_state PUZZLE_STATE DGROUP_BSS(0x5428);

/*
 * 0x0f0b0
 *
 * **The SELECT PUZZLE screen**, and what "leave freeform mode" puts up before
 * going back to the puzzles: a list of them, and a field for the password that
 * unlocks one you have not reached.
 *
 * It answers **whether the puzzle changed** - 1 when the score it leaves at
 * 0x4ebd differs from the one it arrived with - so the caller knows whether to
 * load a level.
 *
 * **It saves the score at 0x4eaf/0x4ead on the way in** and restores it if the
 * player presses Escape, because entering a score code overwrites it and Escape
 * has to mean "as you were".
 *
 * The furthest level reached at 0x4eb7 is the gate: a row beyond it puts up
 * NEED PASSWORD and does nothing else. A password that is found unlocks its
 * level; a score code that verifies sets the score with it, and one that does
 * not sets the score to **zero** rather than refusing - the message says so.
 * Passing 0x4eb7 writes `tim.cfg` on the spot, so a level unlocked by password
 * is still unlocked next time the game is run.
 *
 * Both scroll arrows move a **page** of 0x15 rather than a row, and both set
 * `di` to 4, which is the auto-repeat delay: four passes of the loop have to go
 * by before the arrow can move again. That is the same counter the mode word
 * uses to say the arrow is held.
 *
 * The five repaint counters and the `was` local are `pick_file`'s, and so is
 * the rule that a full repaint suppresses the partial ones.
 */
uint16_t select_puzzle_screen(void)
{
    int16_t  row;                       /* [bp-2] */
    int16_t  level;                     /* [bp-4] */
    uint16_t was;                       /* [bp-6] */
    int16_t  full;                      /* [bp-8] */
    int16_t  rp_up;                     /* [bp-0xa] */
    int16_t  rp_down;                   /* [bp-0xc] */
    int16_t  rp_pass;                   /* [bp-0xe] */
    int16_t  page;                      /* [bp-0x10] */
    int32_t  saved;                     /* [bp-0x14] */
    register int16_t repaint;
    register int16_t hold;

    saved = DG4E67.counter;
    hold = 0;

    PUZZLE_STATE.selected_level = DG4E67.round_number;
    page = PUZZLE_STATE.puzzle_page = puzzle_page_of_score();

    /*
     * The screen **opens the current level** before it draws anything: if the
     * player is on a puzzle further than 0x4eb7 says has been reached, 0x4eb7
     * catches up. So arriving here is itself enough to unlock the row you are
     * standing on, and the list's colours are right on the first paint.
     */
    if (DG4E67.round_number > DG4E67.furthest_level)
        DG4E67.furthest_level = DG4E67.round_number;

    full = 1;
    rp_up = rp_down = repaint = rp_pass = 0;
    was = DG4E67.state = 0x8000;

    while (DG4E67.state != 0x400) {
        update_button_state();
        DG52ED.last_key = bios_read_key();

        if (DG52ED.last_key == '\t' && DG4E67.state != 0x800)
            puzzle_tab();

        if ((DG52ED.last_key == '\r' || DG52ED.last_key == ' '
             || DG52ED.last_key == 0x1b /* Esc */)
            && DG4E67.state == 0x800)
            DG5768.button_left = 0;

        regions_handle_pointer(DG4E67.regions_a);

        if (DG52ED.last_key == 0x1b /* Esc */) {
            /*
             * Escape: put the score back, restart the counters, reset the clip,
             * and leave with the mode the loop's tail ends on.
             */
            DG4E67.counter = saved;
            start_counters();
            set_clip_play_area();
            PUZZLE_STATE.selected_level = DG4E67.round_number;
            DG4E67.state = 0x400;

        /*
         * The password field, entered while it has focus or had it last pass -
         * `pick_file`'s trick, and for the same reason.
         */
        } else if ((DG4E67.state != 0x800 && was == 0x800)
                   || DG4E67.state == 0x800) {
            if ((DG52ED.last_key != '\r' && DG4E67.state == 0x800)
                || was != 0x800) {
                if (was == 0x800)
                    picker_type(DG52ED.last_key, GAME_TYPED_TEXT.typed, 0x19);
            } else {
                update_button_state();

                level = password_to_level(GAME_TYPED_TEXT.typed);

                if (level == -1) {
                    show_message_box(DG1BCC.bad_password, DG1BCC.bad_password_body);
                    DG4E67.state = 0x8000;
                    full = 1;
                } else {
                    DG4E67.counter = score_code_to_score(GAME_TYPED_TEXT.typed);

                    if (DG4E67.counter == -1) {
                        DG4E67.counter = 0;
                        show_message_box(DG1BCC.score_code_invalid, DG1BCC.score_code_body);
                        full = 1;
                    }

                    PUZZLE_STATE.selected_level = level;

                    if (PUZZLE_STATE.selected_level > DG4E67.level_count)
                        PUZZLE_STATE.selected_level = DG4E67.level_count;

                    repaint = 1;
                    start_counters();
                    set_clip_play_area();

                    page = puzzle_page_of_score();
                    if (page != PUZZLE_STATE.puzzle_page) {
                        PUZZLE_STATE.puzzle_page = page;
                        repaint = 1;
                    }
                }

                if (level > DG4E67.furthest_level) {
                    DG4E67.furthest_level = level;
                    write_config();            /* write tim.cfg */
                    repaint = 1;
                }

                if (DG4E67.state == 0x800)
                    DG4E67.state = 0x8000;
            }

            rp_pass = 2;
        }

        if (hold != 0)
            hold--;

        switch (DG4E67.state) {
        case 0x2000:                    /* the up arrow: a page back */
            if (hold == 0) {
                if (DG5768.button_left != 1 && DG5768.button_left != 2) {
                    DG4E67.state = 0x8000;
                } else if (PUZZLE_STATE.puzzle_page > 1) {
                    PUZZLE_STATE.puzzle_page -= 0x15;
                    if (PUZZLE_STATE.puzzle_page < 1)
                        PUZZLE_STATE.puzzle_page = 1;
                    repaint = 1;
                    hold = 4;
                }
            }
            rp_up = 2;
            break;

        case 0x1000:                    /* the down arrow: a page on */
            if (hold == 0) {
                if (DG5768.button_left != 1 && DG5768.button_left != 2) {
                    DG4E67.state = 0x8000;
                } else if (PUZZLE_STATE.puzzle_page + 0x15 <= DG4E67.level_count) {
                    PUZZLE_STATE.puzzle_page += 0x15;
                    repaint = 1;
                    hold = 4;
                }
            }
            rp_down = 2;
            break;

        case 0x4000:                    /* a click in the list */
            row = PUZZLE_STATE.puzzle_page + (DG5768.pointer_y - 0x4c) / 10;

            if (row <= DG4E67.level_count) {
                if (row > DG4E67.furthest_level) {
                    show_message_box(DG1BCC.need_password, DG1BCC.need_password_body);
                    repaint = 1;
                } else if (row != PUZZLE_STATE.selected_level) {
                    PUZZLE_STATE.selected_level = row;

                    /*
                     * Choosing a puzzle *earlier* than the one being played
                     * zeroes the score, because the score belongs to the run
                     * that got this far.
                     */
                    if (PUZZLE_STATE.selected_level < DG4E67.round_number) {
                        DG4E67.counter = 0;
                        start_counters();
                        set_clip_play_area();
                    }

                    repaint = 1;
                }
            }

            DG4E67.state = 0x8000;
            break;
        }

        was = DG4E67.state;

        if (full != 0) {
            wait_cursor();
            puzzle_repaint();
            restore_cursor();
            full--;
        } else {
            if (rp_up != 0) {
                puzzle_draw_up();
                present_back_page();
                rp_up--;
            }
            if (rp_down != 0) {
                puzzle_draw_down();
                present_back_page();
                rp_down--;
            }
            if (repaint != 0)
                puzzle_draw_list(PUZZLE_STATE.puzzle_page, PUZZLE_STATE.selected_level);
            if (rp_pass != 0) {
                puzzle_draw_password(GAME_TYPED_TEXT.typed);
                rp_pass--;
            }
        }

        if (repaint != 0) {
            present_back_page();
            repaint--;
        } else {
            present_frame(1);
        }
    }

    puzzle_draw_ok(1);
    present_back_page();

    if (PUZZLE_STATE.selected_level != DG4E67.round_number) {
        DG4E67.round_number = PUZZLE_STATE.selected_level;
        return 1;
    }
    return 0;
}

/*
 * 0x0f468
 *
 * **Tab on the puzzle screen**, the third of these and the same trick: warp the
 * pointer. Five stops, cursor at DGROUP 0x260a, x at 0x260c and y at 0x2616 -
 * so 0x260c holds exactly five words and 0x2616 begins where it ends.
 */
void puzzle_tab(void)
{
    GAME_PUZZLE_TABS.stop++;

    if (GAME_PUZZLE_TABS.stop == 5)
        GAME_PUZZLE_TABS.stop = 0;

    move_pointer_to(GAME_PUZZLE_TABS.stop_x[GAME_PUZZLE_TABS.stop],
                    GAME_PUZZLE_TABS.stop_y[GAME_PUZZLE_TABS.stop]);
}

/*
 * 0x0f499
 *
 * **Which page a score is on.** Pages hold 0x15 puzzles and are numbered from
 * **1**, so this walks 1, 0x16, 0x2b... until the page's last puzzle - its
 * first plus 0x14 - reaches the score at DGROUP 0x542a, and answers the first.
 *
 * It counts rather than divides, which is why the page numbers are one-based
 * without any correction: the arithmetic never has to be shifted.
 */
uint16_t puzzle_page_of_score(void)
{
    int16_t page = 1;

    while (page + 0x14 < PUZZLE_STATE.selected_level)
        page += 0x15;

    return (uint16_t)page;
}

/*
 * 0x0f4b5
 *
 * **The puzzle screen's whole surface**, the counterpart of `picker_repaint`:
 * the title bar, two headings, three sunken wells, and then the same five
 * routines the loop calls for its partial redraws.
 *
 * The three wells are all placed round what goes in them: two 0x20 squares at
 * x 0x1cc for the arrows drawn at 0x1d4, and a 0x28 square at (0x1f0, 0x12c)
 * for the OK button at (0x200, 0x12e). The list has no well - it is drawn onto
 * the title bar's own surface, and `puzzle_draw_list` fills its rectangle
 * itself before writing the rows.
 *
 * The OK button is drawn with `0`, unpressed, because this is the paint that
 * puts the screen up rather than the one that answers a click.
 */
void puzzle_repaint(void)
{
    draw_title_bar(0x20, 0x20, 0x220, 0x158, 0);

    draw_scroll_text(DG1BCC.select_puzzle, 0xa8, 0x27, 0xc0);
    draw_scroll_text(DG1BCC.password, 0x20, 0x13c, 0x60);

    draw_sunken_box(0x1cc, 0x42, 0x20, 0x20);
    draw_sunken_box(0x1cc, 0x108, 0x20, 0x20);
    draw_sunken_box(0x1f0, 0x12c, 0x28, 0x28);

    puzzle_draw_up();
    puzzle_draw_down();
    puzzle_draw_ok(0);
    puzzle_draw_password((const char *)GAME_TYPED_TEXT.typed);
    puzzle_draw_list(PUZZLE_STATE.puzzle_page, PUZZLE_STATE.selected_level);

    present_back_page();
}

/*
 * 0x0f57e
 *
 * The puzzle list's **up arrow**, and `picker_draw_up`'s twin in a different
 * screen: the same art at +0x4a of the set, the pressed one chosen by reading
 * 0x4e6b back - 0x2000 here - and only the position differs.
 */
void puzzle_draw_up(void)
{
    register int16_t pressed;

    if (DG4E67.state == 0x2000)
        pressed = 1;
    else
        pressed = 0;

    VMDS.page_dst = VMDS.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((DG52ED.panel_art + 0x25)[pressed]),
                0x1d4, 0x46, 0);
    restore_cursor_following();
}

/*
 * 0x0f5c4
 *
 * The puzzle list's **down arrow**: art at +0x4e, mode 0x1000, and 0xca pixels
 * below its twin.
 */
void puzzle_draw_down(void)
{
    register int16_t pressed;

    if (DG4E67.state == 0x1000)
        pressed = 1;
    else
        pressed = 0;

    VMDS.page_dst = VMDS.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((DG52ED.panel_art + 0x27)[pressed]),
                0x1d4, 0x110, 0);
    restore_cursor_following();
}

/*
 * 0x0f60a
 *
 * The puzzle screen's **OK button**, and unlike the two arrows it is *told*
 * whether it is pressed rather than reading the mode - because the one caller
 * that draws it pressed does so on the way out of the loop, after the mode has
 * already been changed to the one that ends it.
 */
void puzzle_draw_ok(uint16_t pressed)
{
    VMDS.page_dst = VMDS.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((DG52ED.panel_art + 0x10)[pressed]),
                0x200, 0x12e, 0);
    restore_cursor_following();
}

/*
 * 0x0f640
 *
 * **The password field**, and the third of these after the picker's two. Same
 * shape - copy, walk the pointer while the text is too wide, blink a caret by
 * counting - with its own numbers: a 0x122-wide field, the counter at 0x5428,
 * the asterisk at 0x2620, and the mode 0x800.
 *
 * Unlike the picker's two it takes the buffer as an **argument** rather than
 * naming it, because the one caller has it in a local at 0x542e and the field
 * is the only thing that reads it.
 *
 * It sets the text colour and **not** the background byte at 0x3891, where the
 * picker's fields set both. Whatever 0x3891 was left holding by the last thing
 * drawn is what this uses.
 */
void puzzle_draw_password(const char *text)
{
    char buf[40];                  /* [bp-0x28] */
    register char *si = buf;

    strcpy(si, text);

    while ((int16_t)text_width_thunk(si) > 0x122)
        si++;

    if (DG4E67.state == 0x800) {
        PUZZLE_STATE.password_blink++;
        if ((PUZZLE_STATE.password_blink & 8) != 0)
            strcat(si, GAME_PART_NAMES.star);
    }

    VMDS.page_dst = VMDS.page_back;
    fill_panel_area(0x90, 0x13c, 0x130, 0x10, 0);

    VMDS.text_colour = 0x0f;

    cursor_redraw_off_thunk();
    draw_string(si, 0x94, 0x140);
    restore_cursor_following();
}

/*
 * 0x0f6cc
 *
 * **The puzzle list.** 0x15 rows, each one built up in a local: `"PUZZLE "`,
 * the number, `": "`, and then the title out of the puzzle's own file.
 *
 * **A missing file ends the list**, and it ends it by setting the loop counter
 * to 0x34 - past its own limit of 0x15 - rather than by breaking. So the number
 * of puzzles is however many `l<n>.lev` files are actually present, and the
 * list finds out by asking rather than by being told.
 *
 * Three colours, and the middle one is the interesting one: white for the row
 * that is the current selection, **0x0a for a puzzle at or below the furthest
 * reached** at DGROUP 0x4eb7, and 0x0c for one beyond it. So the list shows
 * where the player has got to as well as where they are.
 */
void puzzle_draw_list(register int16_t first, int16_t selected)
{
    int16_t y;                          /* [bp-2] */
    char num[8];                        /* [bp-0xa] */
    char name[100];                     /* [bp-0x6e] */
    char title[80];                     /* [bp-0xbe] */
    int16_t i;

    VMDS.page_dst = VMDS.page_back;
    fill_panel_area(0x30, 0x48, 0x190, 0xd8, 0);

    for (i = 0, y = 0x4c; i < 0x15; i++, y += 0x0a, first++) {
        strcpy(name, DG1BCC.puzzle_prefix);
        itoa(first, num, 10);
        strcat(name, num);
        strcat(name, GAME_PART_NAMES.title_sep);

        if (get_puzzle_title(first, title) != 0) {
            strcat(name, title);

            if (first == selected)
                VMDS.text_colour = 0x0f;
            else if (first <= DG4E67.furthest_level)
                VMDS.text_colour = 0x0a;
            else
                VMDS.text_colour = 0x0c;

            cursor_redraw_off_thunk();
            draw_string(name, 0x34, y);
            restore_cursor_following();
        } else {
            i = 0x34;
        }
    }
}

/*
 * 0x0f7b6
 *
 * Load the part bitmaps: 0 to 8, then 9 on its own, then 0x0b to 0x30, then
 * 0x32 on its own. **10 and 0x31 are skipped**, and skipped by being left out
 * of the ranges rather than tested for - there is no part with those numbers.
 */
void load_all_parts(void)
{
    int16_t si;

    for (si = 0; si < 8; si++)
        load_part_bitmap((uint16_t)si);

    load_part_bitmap(9);

    for (si = 0x0b; si < 0x31; si++)
        load_part_bitmap((uint16_t)si);

    load_part_bitmap(0x32);
}

/*
 * 0x0f7f4
 *
 * Load one part's bitmaps: build "part" + the number + ".bmp", read it, and
 * keep the list at DGROUP 0xeba + 0x3a * n - so the parts' records are 0x3a
 * bytes apart and this is the first field of each.
 *
 * The heap is checked either side of the load, and the cursor is pinned across
 * it and released after: a load takes long enough for the pointer to want
 * redrawing, and redrawing it in the middle of one would draw it onto a page
 * that is being rebuilt.
 */
void load_part_bitmap(uint16_t n)
{
    char name[14];            /* [bp-0x16] */
    char number[8];          /* [bp-8]    */

    strcpy(name, GAME_PART_NAMES.part);
    itoa((int16_t)n, number, 10);
    strcat(name, number);
    strcat(name, GAME_PART_NAMES.bmp);

    heap_check_or_hang();
    cursor_redraw_off_thunk();

    PART_KINDS[n].bitmaps = load_bitmaps(name);

    restore_cursor_following();
    heap_check_or_hang();
}

/*
 * 0x0f86e
 *
 * Give back every part's bitmaps: 0 to 0x39, one at a time, and no skipping -
 * unlike `load_all_parts`, which leaves out 10 and 0x31 because there is no
 * part with those numbers. Freeing one that was never loaded is harmless, so
 * the loop is written plainly.
 */
void free_all_part_bitmaps(void)
{
    int16_t si;

    for (si = 0; si < 0x3a; si++)
        free_part_bitmap((uint16_t)si);
}

/*
 * 0x0f886
 *
 * Give one part's bitmaps back, and clear its slot. A slot that is already
 * empty is left alone.
 */
void free_part_bitmap(uint16_t n)
{
    if (PART_KINDS[n].bitmaps != 0) {
        free_bitmaps_thunk(PART_KINDS[n].bitmaps);
        PART_KINDS[n].bitmaps = 0;
    }
}
