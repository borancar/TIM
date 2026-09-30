/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Drawing the machine**: its five layers, the belts, ropes and
 * curves between parts, the part being carried, and the panels, buttons and
 * odometer around it. Part allocation lives here too, next to the drawing
 * that depends on it.
 *
 * This file corresponds to the original's **code segment 14de**, image
 * 0x14de0..0x172c0 - it ends with a routine that does nothing, 0x172bc, called
 * as 14de:24dc. Functions are in address order and each carries the image
 * offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -d -O -Z
 * JUDGE: data 0x259c..0x25e8
 *
 * **Turbo C++ 3.0, `-mm -d`, without `-O`**: a `return` leaves its `jmp` to
 * the epilogue even when the epilogue is next (`ask_yes_no`), the two
 * `"(click button to continue)"` share one copy, and the calls to routines
 * earlier in the file are `push cs / call`. Its data is DGROUP
 * 0x259c..0x25e8: the message box's tab stops, the menu animation, the
 * selection's phase, and its literal pool, the three button labels. The
 * messages it draws - "PUZZLE ", " COMPLETED!" and the rest - are the strings
 * module's arrays in `g_messages`: "PUZZLE " is named from segment 0dff as well,
 * and a literal belongs to one module.
 */

#define TIM_MACHINE_DRAW_C
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
 * The DGROUP records only this file reads or writes. Each is laid over the
 * DGROUP byte array at the address its macro names, like the shared ones in
 * dgroup.h; they are declared here because nothing else uses them.
 */

/* The message box's tab stops, DGROUP 0x259c. */
struct game_message_tabs g_game_message_tabs = { 0xffff, { 0x00e8, 0x0168 } };
/* The menu button's animation, DGROUP 0x25a2. */
struct machine_draw_menu_anim g_machine_draw_menu_anim = {
    { 0x0003, 0x0004, 0x0005, 0x0006, 0x0003, 0x0003 },     /* picture */
    { 0x0258, 0x0254, 0x0254, 0x0254, 0x0260, 0x0265 },     /* picture_x */
    { 0x0013, 0x0010, 0x000f, 0x0013, 0x0013, 0x0013 },     /* picture_y */
    { 0x0250, 0x0252, 0x0250, 0x0251 },                     /* sprite_x */
    { 0x001a, 0x0018, 0x001b, 0x0019 },                     /* sprite_y */
};
/* The selection outline's phase, DGROUP 0x25d6. */
uint16_t g_selection_phase = 0;

/*
 * 0x16ca7
 *
 * **The frame the title bar sits in**: a shadow, a tiled interior, and a
 * border of edge and corner pieces from the set at DGROUP 0x4ecb.
 *
 * The rectangle arrives as **two corners and not a size**, which is worth
 * saying because the call passes 0x220 and 0x158 and those read as a width and
 * a height: every use of them here is a subtraction, `x2 - x1` and `y2 - y1`.
 *
 * `filled` gates the first part - a filled rectangle offset down and left of
 * the frame, and two pieces at +0x4a and +0x4c - which is the drop shadow, so
 * a caller can have the frame without it.
 *
 * Then the interior. The clip box is set to the four corners and the tile at
 * +0x54 is laid in steps of 0x80 across and 0x40 down, so one tile covers any
 * size. The clip then goes back to the whole screen or to the play area
 * depending on whether the state at 0x4e6b is 0x8000 - the same fork
 * `draw_panel` makes, and it has to happen before the border is drawn or the
 * border would be clipped away by its own frame.
 *
 * The border is four runs of 8 pixels - top and bottom together in one loop
 * across x, left and right together in one loop down y - and then four
 * corners, each placed by an offset from its own corner rather than from the
 * origin. Nine pieces in all: +0x20 to +0x26 for the runs, +0x18 to +0x1e for
 * the corners.
 */
void draw_title_bar(register int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                    uint16_t filled)
{
    int16_t y;
    register int16_t i;

    g_vmds.page_dst = g_vmds.page_back;
    g_vmds.clip_enabled = 0;
    g_vmds.second_colour = g_vmds.fill_colour = 0;
    g_vmds.fill_enabled = 1;
    cursor_redraw_off_thunk();
    if (filled != 0) {
        fill_rect(x1 - 0x0c, y1 + 0x0c, x2 - x1, y2 - y1);
        draw_bitmap(g_border_art[0x25],
                    x1 - 0x0f, y1 + 7, 0);
        draw_bitmap(g_border_art[0x26],
                    x1 - 0x0f, y2 - 9, 0);
        draw_bitmap(g_border_art[0x27],
                    x2 - 0x20, y2 - 9, 0);
    }
    g_vmds.clip_left = x1;
    g_vmds.clip_right = x2;
    g_vmds.clip_top = y1;
    g_vmds.clip_bottom = y2;
    g_vmds.clip_enabled = 1;
    for (y = y1; y < y2; y += 0x40)
        for (i = x1; i < x2; i += 0x80)
            draw_bitmap(g_border_art[0x2a],
                        i, y, 0);
    if (g_round_state == 0x8000)
        set_clip_full_screen();
    else
        set_clip_play_area();
    g_vmds.clip_enabled = 0;
    for (i = x1; i < x2; i += 8) {
        draw_bitmap(g_border_art[0x12],
                    i, y1 - 4, 0);
        draw_bitmap(g_border_art[0x13],
                    i, y2, 0);
    }
    for (i = y1; i < y2; i += 8) {
        draw_bitmap(g_border_art[0x10],
                    x1 - 4, i, 0);
        draw_bitmap(g_border_art[0x11],
                    x2, i, 0);
    }
    draw_bitmap(g_border_art[0xc],
                x1 - 7, y1 - 7, 0);
    draw_bitmap(g_border_art[0xd],
                x2 - 0x11, y1 - 7, 0);
    draw_bitmap(g_border_art[0xe],
                x1 - 7, y2 - 0x11, 0);
    draw_bitmap(g_border_art[0xf],
                x2 - 0x11, y2 - 0x11, 0);
}

/*
 * 0x16ebf
 *
 * Draw a **scroll** of a given width with a string centred on it: the two end
 * caps and a repeating middle out of the set at DGROUP 0x52f4, and the text
 * twice, once dark and once light one pixel up and left.
 *
 * The centring is measured, not assumed - `text_width_thunk` is asked how wide
 * the string is and the difference from the scroll's width is halved - so a
 * string wider than the scroll centres to a negative offset and runs off both
 * ends rather than being clipped or wrapped.
 *
 * The middle piece is laid every 8 pixels from `x + 0x18` to `x + w - 0x18`,
 * which is what lets one scroll bitmap stretch to any width. The right cap
 * goes at that same `x + w - 0x18` - `add ax, 0xffe8` at 0x15080, the same two
 * instructions as the loop bound at 0x1506f - so it sits where the middle
 * stopped. Putting it at `x + w` left a bar of bare background between the
 * last middle piece and the cap.
 *
 * The text is drawn twice for a shadow: colour 0xf at `centre - 1, y + 6`,
 * then colour 5 at `centre, y + 5`, **the same string both times**.
 *
 * 0x150c1 is `mov ax, [bp+6]` / `inc word [bp+6]` / `push ax` - a post
 * increment, so the pointer that is pushed is the one *before* the increment
 * and the increment itself is a dead store, the routine returning three
 * instructions later. This was read as though the second pass started a
 * character later, and the briefing's title bar duly came out reading "UZZLE 1:
 * TUTORIAL" with the light pass painted over the dark one. The order of the
 * three instructions is the whole of the evidence.
 */
void draw_scroll_text(const char *str, register int16_t x, register int16_t y,
                      int16_t w)
{
    int16_t i;
    int16_t centre;

    centre = x + ((w - (int16_t)text_width_thunk(str)) >> 1);
    cursor_redraw_off_thunk();
    draw_bitmap(g_panel_art[0], x, y, 0);
    for (i = x + 0x18; i < x + w - 0x18; i += 8)
        draw_bitmap(g_panel_art[0x1],
                    i, y + 2, 0);
    draw_bitmap(g_panel_art[0x2],
                x + w - 0x18, y, 0);
    g_vmds.text_style = 1;                    /* transparent: no background line */
    g_vmds.text_colour = 0x0f;
    draw_string(str, centre - 1, y + 6);
    g_vmds.text_colour = 5;
    /* `str++`: the original steps its own copy of the pointer, to no end. */
    draw_string(str++, centre, y + 5);
    restore_cursor_following();
}

/*
 * 0x16f96
 *
 * **Draw a button**: a left cap, as many middle pieces as the word needs, a
 * right cap, and the word over them. Three bitmaps out of the set at DGROUP
 * 0x52f4 at 0x58, 0x5c and 0x60 - and `pressed` is a *word* index added to each,
 * so the pressed button is the next bitmap along from the raised one in all
 * three places rather than the same art drawn differently.
 *
 * **The width is measured and then rounded up to a multiple of 8**, which is the
 * middle piece's width; the caps sit at `x` and at `x + rounded + 8`. That
 * rounding is why `message_box` can right-align its second button by arithmetic
 * alone - the same `(w + 7) & ~7` there and here, so the two agree without
 * either asking the other.
 *
 * **The label is centred in the rounding, not in the button**: the offset is
 * half of what the rounding added, plus 8 for the left cap. So a word that
 * rounds up by seven pixels sits three to the right of where a word that rounds
 * up by one does, and both look centred because the caps absorb it.
 *
 * `pressed` moves the label as well as choosing the art - *down* two and *left*
 * one, `y + 2 * pressed` against `x - pressed`. Two different multipliers on the
 * same flag, which is what makes the word look pushed into the button rather
 * than merely moved.
 */
void draw_button(const char *str, register int16_t x, int16_t y,
                 register int16_t pressed)
{
    int16_t i;
    int16_t w;
    int16_t rounded;
    int16_t text_off;
    int16_t right;

    w = (int16_t)text_width_thunk(str);
    rounded = (w + 7) & 0xfff8;
    right = x + rounded + 8;
    text_off = ((rounded - w) >> 1) + 8;
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((g_panel_art + 0x2c)[pressed]),
                x, y, 0);
    for (i = x + 8; i < right; i += 8)
        draw_bitmap(((g_panel_art + 0x2e)[pressed]),
                    i, y, 0);
    draw_bitmap(((g_panel_art + 0x30)[pressed]),
                right, y, 0);
    g_vmds.text_style = 1;            /* transparent: no background line */
    g_vmds.text_colour = 5;
    draw_string(str, x + text_off - pressed, y + 2 * pressed + 4);
    restore_cursor_following();
}

/*
 * 0x17080
 *
 * Draw a **panel**: a tiled background inside `x,y,w,h`, a bevel around it,
 * and the ornamented border the game's menus and the copy-protection screen
 * are built out of.
 *
 * The clip box is set to the rectangle first - and `g_vmds.clip_bottom` to
 * `y + h - 1`, one less, where `g_vmds.clip_right` is `x + w` - so the tiling cannot
 * escape it. The background is the bitmap at +0x74 of the set the game keeps a
 * pointer to at DGROUP 0x52f4, laid down every 0x40 in both directions, which
 * is why a panel of any size costs the same tile.
 *
 * Then the clip goes back to the whole screen or to the play area, chosen by
 * whether DGROUP 0x4e6b is 0x8000 - the intro's state - and the bevel is four
 * lines: white (0xf) across the top and down the left, and 0xe then 6 for the
 * two other sides, so the panel reads as raised.
 *
 * The ornaments are the rest: a column of +0x1c every 8 pixels down the left
 * from `y + 0x13`, a row of +0x1e every 8 across the bottom from `x + 0x10`,
 * and four corner pieces at +0x14, +0x16, +0x18 and +0x1a. Every one is placed
 * by an offset from a corner rather than from the middle, which is what lets
 * the same routine draw a 0x20-wide button and a 0x220-wide panel.
 */
void draw_panel(register int16_t x, register int16_t y, int16_t w, int16_t h)
{
    int16_t i;
    int16_t j;

    g_vmds.clip_left = x;
    g_vmds.clip_right = x + w;
    g_vmds.clip_top = y;
    g_vmds.clip_bottom = y + h - 1;
    g_vmds.clip_enabled = 1;
    cursor_redraw_off_thunk();
    for (j = 0; j < h; j += 0x40)
        for (i = 0; i < w; i += 0x40)
            draw_bitmap(g_panel_art[0x3a], i + x, j + y, 0);
    if (g_round_state == 0x8000)
        set_clip_full_screen();
    else
        set_clip_play_area();
    g_vmds.second_colour = 0x0f;
    clip_and_draw_line(x, y + 1, x + w, y + 1);
    clip_and_draw_line(x + w - 1, y, x + w - 1, y + h);
    g_vmds.second_colour = 0x0e;
    clip_and_draw_line(x, y, x + w, y);
    g_vmds.second_colour = 0x06;
    clip_and_draw_line(x + w, y, x + w, y + h);
    for (i = y + 0x13; i < y + h; i += 8)
        draw_bitmap(g_panel_art[0xe], x - 2, i, 0);
    for (i = x + 0x10; i < x + w; i += 8)
        draw_bitmap(g_panel_art[0xf], i, y + h - 4, 0);
    draw_bitmap(g_panel_art[0xa], x - 7, y - 4, 0);
    draw_bitmap(g_panel_art[0xb], x + w - 0x10, y - 4, 0);
    draw_bitmap(g_panel_art[0xc], x - 7, y + h - 0x10, 0);
    draw_bitmap(g_panel_art[0xd], x + w - 0x13, y + h - 0xe, 0);
    restore_cursor_following();
}

/*
 * 0x17270
 *
 * Draw a **sunken box**: nine pieces of art, tiled. `draw_panel` above is the
 * raised one, built out of lines and ornaments; this is the other kind, and it
 * is built out of nothing but blits.
 *
 * The pieces are all in the set at DGROUP 0x52f4: the interior at +0x56, the
 * left and right edges at +0x6c and +0x6e, the top and bottom at +0x70 and
 * +0x72, and the four corners at +0x64, +0x66, +0x68 and +0x6a.
 *
 * The tiles are 8 pixels and the corners are **16**, which is why the edges
 * inset by 8 and the corners by 16. Both loops start at 8 and stop while
 * `w - 8` is still greater, so the last tile before the far edge is skipped and
 * the edge piece covers it.
 *
 * The clip is set to the play area first, not to the box, because every piece
 * is placed rather than tiled past a boundary - so nothing here can escape and
 * nothing has to be clipped to stop it.
 */
void draw_sunken_box(register int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t j;
    register int16_t i;

    set_clip_play_area();
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    for (j = 8; h - 8 > j; j += 8) {
        for (i = 8; w - 8 > i; i += 8)
            draw_bitmap(g_panel_art[0x2b], i + x, j + y, 0);
        draw_bitmap(g_panel_art[0x36], x, j + y, 0);
        draw_bitmap(g_panel_art[0x37], x + w - 8, j + y, 0);
    }
    for (i = 8; w - 8 > i; i += 8) {
        draw_bitmap(g_panel_art[0x38], i + x, y, 0);
        draw_bitmap(g_panel_art[0x39], i + x, y + h - 8, 0);
    }
    draw_bitmap(g_panel_art[0x32], x, y, 0);
    draw_bitmap(g_panel_art[0x33], x + w - 0x10, y, 0);
    draw_bitmap(g_panel_art[0x34], x, y + h - 0x10, 0);
    draw_bitmap(g_panel_art[0x35], x + w - 0x10, y + h - 0x10, 0);
    restore_cursor_following();
}

/*
 * 0x173db
 *
 * **A filled, framed area** of the panel: a rectangle in a given colour with
 * the same nine-piece border around it that `draw_title_bar` uses - four runs
 * of 8 pixels and four corners, from the set at DGROUP 0x4ecb.
 *
 * Unlike `draw_title_bar` this one takes a **width and a height** and works
 * out the far corner itself, into two locals, on the way in. The two routines
 * draw the same kind of frame and disagree about how to be told where it goes,
 * which is worth knowing before reading either from memory of the other.
 *
 * The border pieces are a different set from the title bar's: +0x34 and +0x36
 * for the top and bottom runs, +0x30 and +0x32 for the sides, +0x28 to +0x2e
 * for the corners. All four corners sit 8 pixels out except the bottom-left,
 * which is **5** - `0xfffb` and not `0xfff8`, once, and it is not a
 * misreading: the byte is `fb`.
 *
 * The colour is passed in and written to both 0x389d and 0x389e before the
 * fill, so the interior and whatever else reads the second colour agree.
 */
void fill_panel_area(register int16_t x, int16_t y, int16_t w, int16_t h,
                     uint8_t colour)
{
    int16_t x2;
    int16_t y2;
    register int16_t n;

    x2 = x + w;
    y2 = y + h;
    cursor_redraw_off_thunk();
    g_vmds.page_dst = g_vmds.page_back;
    g_vmds.second_colour = g_vmds.fill_colour = colour;
    fill_rect(x, y, w, h);
    for (n = x; n < x2; n += 8) {
        draw_bitmap(g_border_art[0x1a], n, y - 8, 0);
        draw_bitmap(g_border_art[0x1b], n, y2, 0);
    }
    for (n = y; n < y2; n += 8) {
        draw_bitmap(g_border_art[0x18], x - 8, n, 0);
        draw_bitmap(g_border_art[0x19], x2, n, 0);
    }
    draw_bitmap(g_border_art[0x14], x - 8, y - 8, 0);
    draw_bitmap(g_border_art[0x15], x2 - 8, y - 8, 0);
    draw_bitmap(g_border_art[0x16], x - 8, y2 - 5, 0);
    draw_bitmap(g_border_art[0x17], x2 - 8, y2 - 8, 0);
}

/*
 * 0x17519
 *
 * **A message box with one button.** It is a doorway: the box itself is
 * 0x15698, and this passes it the title, the body, "CONTINUE" for the first
 * button and **zero for the second**, which is how the box is told there is
 * only one.
 *
 * The zero is pushed first and the strings after, so what the box reads as its
 * fourth argument is the absent button rather than a flag saying how many there
 * are. That is the whole difference between this and `ask_yes_no` below.
 */
void show_message_box(const char *title, char *body)
{
    message_box(title, body, "CONTINUE", NULL);
}

/*
 * 0x17533
 *
 * **A message box with two buttons**, answering which was pressed. The other
 * doorway into 0x15698, twenty-six bytes past the first, and the only
 * difference is that both button strings are given: 0x25e1 and 0x25e5.
 *
 * Quit, restart and both freeform handlers ask through this one. It is its own
 * routine and not an argument to `show_message_box` because that is what the
 * original has - two entry points to one body, the way Borland's runtime is
 * built and the way the part tables reach shared code.
 *
 * Its `jmp` to the instruction after it, at 0x15694, is the compiler leaving a
 * return path in that nothing needed; transcribed as the fall-through it is.
 */
uint16_t ask_yes_no(const char *title, char *body)
{
    return message_box(title, body, "YES", "NO");
}

/*
 * 0x1754e
 *
 * **The message box.** Both doorways above reach it - `show_message_box` with
 * one button and `ask_yes_no` with two - and it answers 1 for the first button
 * and 0 for the second or for none.
 *
 * **It takes the screen over by borrowing the state word.** DGROUP 0x4e6b is
 * what `game_screen` and `game_round` dispatch on; it is saved, set to 0x8000
 * while the box is up, and put back on the way out. So the box's own loop tests
 * the same word those screens do, 0x4000 and 0x2000 mean its two buttons here,
 * and nothing underneath can act on a click meant for it.
 *
 * **The buttons' keys come from their first letter.** `[si]` is the first byte
 * of the first button's string, and the shortcuts are chosen from it: 'Y' takes
 * Y for the first button and N for the second, 'R' takes R and A, 'C' takes C -
 * and Enter, which is the only key that means the same as a button rather than
 * naming one. So "YES"/"NO" and "CONTINUE" get their keys without a table, and
 * a button whose word began with something else would get none.
 *
 * **The second button is right-aligned by measurement**: its width is rounded
 * up to a multiple of 8 and taken from 0x168, and that x is filed into the
 * region record at [0x4e6d]+6 so the clickable area moves with it. The first
 * button's own width plus 0xd8 goes into [0x4e6f]+0xa the same way. A box with
 * one button files only the first.
 *
 * **One button means the second cannot be chosen**: with `di` zero, a state of
 * 0x2000 is turned straight back into 0x8000 at 0x15814, so the loop carries on
 * rather than leaving with an answer nothing asked for.
 *
 * On the way out the chosen button is drawn again pressed and presented, which
 * is what makes it flash before the box goes.
 */
uint16_t message_box(const char *title, char *body,
                     register const char *button1,
                     register const char *button2)
{
    uint16_t saved;
    int16_t second_x;

    wait_cursor();
    saved = g_round_state;
    g_round_state = 0x8000;
    draw_title_bar(0xb0, 0x70, 0x190, 0xf8, 1);
    draw_scroll_text(title, 0xb8, 0x74, 0xd0);
    draw_panel(0xb8, 0x90, 0xd0, 0x5a);
    draw_wrapped_text(body, 0xbc, 0x94, 0xc8, 0x30);
    draw_button(button1, 0xc8, 0xd4, 0);
    g_region_kept_b->x1 = text_width_thunk(button1) + 0xd8;
    if (button2 != NULL) {
        second_x = 0x168 - ((text_width_thunk(button2) + 7) & 0xfff8);
        draw_button(button2, second_x, 0xd4, 0);
        g_region_kept_a->x0 = second_x;
    }
    present_back_page();
    restore_cursor();
    while (g_round_state == 0x8000) {
        update_button_state();
        g_last_key = (uint8_t)(bios_read_key() >> 8);
        if (g_last_key == SC_TAB)
            message_box_tab(button2);
        else {
            if (*button1 == 'Y') {
                if (g_last_key == SC_Y)
                    g_round_state = 0x4000;
                if (g_last_key == SC_N)
                    g_round_state = 0x2000;
            }
            if (*button1 == 'R') {
                if (g_last_key == SC_R)
                    g_round_state = 0x4000;
                if (g_last_key == SC_A)
                    g_round_state = 0x2000;
            }
            if (*button1 == 'C') {
                if (g_last_key == SC_C)
                    g_round_state = 0x4000;
                if (g_last_key == SC_ENTER)
                    g_round_state = 0x4000;
            }
        }
        regions_handle_pointer(g_regions_b);
        if (button2 == NULL && g_round_state == 0x2000)
            g_round_state = 0x8000;
        present_frame(1);
    }
    update_button_state();
    if (g_round_state == 0x4000) {
        draw_button(button1, 0xc8, 0xd4, 1);
        present_back_page();
        g_round_state = saved;
        return 1;
    }
    if (button2 != NULL) {
        draw_button(button2, second_x, 0xd4, 1);
        present_back_page();
    }
    g_round_state = saved;
    return 0;
}

/*
 * 0x17769
 *
 * **Tab walks the pointer between the buttons.** A counter at DGROUP 0x259c
 * steps on each press and the pointer is moved to the x that counter names in
 * the table at 0x259e - 232 for the first button, 360 for the second - at a
 * fixed y of 0xde.
 *
 * With no second button the counter is put straight back to zero, so Tab keeps
 * the pointer on the only button there is rather than sending it to where the
 * other one would have been. With one, it wraps at 2.
 *
 * It moves the *pointer*, not a highlight: there is no selected button in this
 * box, only where the mouse is, and Tab is a way of driving the mouse from the
 * keyboard.
 */
void message_box_tab(const char *button2)
{
    g_game_message_tabs.stop++;

    if (button2 != NULL) {
        if (g_game_message_tabs.stop == 2)
            g_game_message_tabs.stop = 0;
    } else {
        g_game_message_tabs.stop = 0;
    }

    move_pointer_to(g_game_message_tabs.stop_x[g_game_message_tabs.stop],
                    0xde);
}

/*
 * 0x1779e
 *
 * **The panel that says a puzzle is finished.** A title bar, two lines of
 * text, and - unless this was the last puzzle - the password for the next one
 * with the score code appended to it.
 *
 * The two lines are built in locals rather than drawn piecewise, because
 * `draw_scroll_text` takes one string and a width: "PUZZLE " and the level
 * number and " COMPLETED!" are concatenated first, and so are "Total bonus
 * points: " and the sum of DGROUP 0x50af and 0x50b1. Both use the same
 * scratch buffer at [bp-8] for the number, one after the other.
 *
 * The password line is the level's own line of `password.txt`, and
 * `score_to_code` then appends `-XXXXX...` to it in place - so the buffer
 * holds the whole thing and is drawn once. On the last puzzle none of that
 * happens: there is no next password to give.
 *
 * "(click button to continue)" is drawn twice, black at (0xd3, 0xee) and then
 * white one pixel up and to the right, which is the drop shadow the rest of
 * this module draws the same way. The 0xd4 in the second call is a coordinate;
 * the disassembly annotates it as `black.pal` because a string happens to
 * start at that DGROUP offset.
 *
 * This routine does not wait for the click it asks for. It presents the page
 * and returns, and the caller at 0x02710 does the waiting.
 */
void show_level_complete(void)
{
    char num[8];
    char line[30];
    char bonus[30];
    char code[40];

    repaint_whole_screen();
    strcpy(line, g_messages.puzzle_prefix);
    itoa(g_round_number, num, 0xa);
    strcat(line, num);
    strcat(line, g_messages.completed);
    strcpy(bonus, g_messages.total_bonus_points);
    itoa(g_level_settings.bonus_1 + g_level_settings.bonus_2, num, 0xa);
    strcat(bonus, num);
    draw_title_bar(0xb0, 0x70, 0x190, 0xf8, 1);
    draw_scroll_text(line, 0xb8, 0x80, 0xd0);
    draw_scroll_text(bonus, 0xb8, 0x9c, 0xd0);
    if (g_round_number < g_level_count) {
        /* The literal ends in a NUL of its own: the image has two after it. */
        draw_scroll_text(g_messages.new_password, 0xb8, 0xc4, 0xd0);
        read_password_line(g_round_number, code);
        score_to_code(g_odometer_total, code);
        draw_scroll_text(code, 0xb8, 0xd8, 0xd0);
    }
    cursor_redraw_off_thunk();
    g_vmds.text_colour = 0;
    draw_string(g_messages.click_button_to_continue, 0xd3, 0xee);
    g_vmds.text_colour = 0x0f;
    draw_string(g_messages.click_button_to_continue, 0xd4, 0xed);
    restore_cursor_following();
    present_back_page();
}

/*
 * 0x17908
 *
 * Wipe the play area and draw the machine into it again - what a message box
 * needs doing behind it once it has gone.
 *
 * The driver is set up first: 0x38a8 takes the page from 0x38a2, the two bytes
 * at 0x389d and 0x389e take the colour at 0x52cb, 0x389c is set and the on/off
 * byte at 0x3893 is cleared so nothing clips. Then the area 8,8 to 0x230 by
 * 0x160 is filled - inside the frame, not the whole screen - and the machine
 * is drawn over it.
 *
 * `step_and_draw_machine(1)` rather than 0: the argument is redraw-everything,
 * so nothing is left to the dirty rectangles that have just been painted over.
 */
void redraw_machine_area(void)
{
    g_vmds.page_dst = g_vmds.page_back;
    g_vmds.second_colour = g_vmds.fill_colour = (uint8_t)g_fill_colour;
    g_vmds.fill_enabled = 1;
    g_vmds.clip_enabled = 0;
    cursor_redraw_off_thunk();
    fill_rect(8, 8, 0x230, 0x160);
    draw_machine_thunk();
    step_and_draw_machine(1);
    present_back_page();
}

/*
 * 0x17954
 *
 * Draw one **odometer digit**: the character `c`, at `x`, scrolled by `y`.
 *
 * The ten digits are two bitmaps, not ten, and not one. `c - '0'` picks which:
 * under 5 the first, from 5 the second with 5 taken off. Each is a vertical
 * strip of five digits 0x15 pixels apart, so the digit wanted is reached by
 * drawing the whole strip at `6 - digit * 0x15 + y` and letting the clip box
 * the caller set keep the rest of it off the screen.
 *
 * That is also what makes `y` a *scroll*. A counter rolling from one value to
 * the next passes `y` from 0 to 0x15 and the strip slides a whole cell, so the
 * old digit leaves upwards as the new one arrives - which is why the two
 * bitmaps are strips in the first place.
 *
 * The pair around the drawing is the cursor: `0x0811b` takes it off the screen
 * so the blit does not capture it, `0x08125` puts it back if it was the one
 * that removed it.
 *
 * The digit is written back into the argument slot before it is used, which
 * costs a byte and reads oddly, but the original does it and a register would
 * have done.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
void draw_odometer_digit(uint8_t c, register int16_t x, int16_t y)
{
    register int16_t row;

    c += 0xd0;                              /* `- '0'` */
    if (c < 5) {
        row = 6 - c * 0x15;
        row += y;
        cursor_redraw_off_thunk();
        draw_bitmap(g_score2_bmp[0], x, row, 0);
    } else {
        c += 0xfb;                          /* `- 5` */
        row = 6 - c * 0x15;
        row += y;
        cursor_redraw_off_thunk();
        draw_bitmap(g_score2_bmp[1], x, row, 0);
    }
    restore_cursor_following();
}

/*
 * 0x179c8
 *
 * Draw the machine and everything around it, as five calls and nothing else.
 * `paint_game_screen` calls this once the play area has been cleared, so the
 * five run in the order they overlap in and none of them clears anything.
 */
void draw_machine_thunk(void)
{
    draw_machine_layer_a();
    draw_machine_layer_b();
    draw_machine_layer_c();
    draw_machine_layer_d();
    draw_machine_layer_e();
}

/*
 * 0x179e6
 *
 * The play area's **top edge**: the tile at +0xc of the border set at DGROUP
 * 0x4ecb laid every 8 pixels from x = 0x10 to x = 0x22f at y = 0, then three
 * single pieces - +0 at the left, +2 at 0x230, +0x14 at 0x238.
 *
 * The run stops at 0x22f and the next piece starts at 0x230, so the tiles and
 * the corner meet exactly; the loop's `jl` is what makes the last tile land at
 * 0x228 and not overlap it.
 */
void draw_machine_layer_b(void)
{
    register int16_t x;

    set_clip_play_area();
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    for (x = 0x10; x < 0x22f; x += 8)
        draw_bitmap(g_border_art[0x6], x, 0, 0);
    draw_bitmap(g_border_art[0], 0, 0, 0);
    draw_bitmap(g_border_art[0x1], 0x230, 0, 0);
    draw_bitmap(g_border_art[0xa], 0x238, 0, 0);
    restore_cursor_following();
}

/*
 * 0x17a65
 *
 * The play area's **bottom edge**: the tile at +0xe laid every 8 pixels along
 * y = 0x168, then the two corners at +4 and +6 on y = 0x160 - the corners sit
 * eight pixels higher than the run they close, because they are taller.
 */
void draw_machine_layer_c(void)
{
    register int16_t x;

    set_clip_play_area();
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    for (x = 0x10; x < 0x22f; x += 8)
        draw_bitmap(g_border_art[0x7], x, 0x168, 0);
    draw_bitmap(g_border_art[0x2], 0, 0x160, 0);
    draw_bitmap(g_border_art[0x3], 0x230, 0x160, 0);
    restore_cursor_following();
}

/*
 * 0x17ad9
 *
 * The play area's **left edge**: the tile at +8 laid every 8 pixels *down*
 * x = 0, from y = 8 to y = 0x161, then the same two corner pieces the top and
 * bottom edges use - +0 at the top and +4 at 0x160.
 *
 * The corners are drawn three times over between the edges, once by each of
 * the three routines that meets there. Transcribed as the repetition it is.
 */
void draw_machine_layer_d(void)
{
    register int16_t y;

    set_clip_play_area();
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    for (y = 8; y < 0x162; y += 8)
        draw_bitmap(g_border_art[0x4], 0, y, 0);
    draw_bitmap(g_border_art[0], 0, 0, 0);
    draw_bitmap(g_border_art[0x2], 0, 0x160, 0);
    restore_cursor_following();
}

/*
 * 0x17b43
 *
 * The play area's **right edge and the bin's own frame** - the last of the
 * five, and the only one that looks at the state.
 *
 * Two vertical runs: the tile at +0xa down x = 0x238 from y = 8 to 0x161, and
 * the one at +0x10 down x = 0x278 to y = 0x16e - the second runs thirteen
 * pixels further, because it closes the bin rather than the play area.
 *
 * Then the fixed pieces: +2 and +6 closing the play area's right side at
 * (0x230, 0) and (0x230, 0x160), a line in colour 0 across the top of the bin
 * from x = 0x238 to 0x27f, and +0x14 twice on x = 0x238 - at y = 0 and y =
 * 0x3b - so the same picture caps the bin at two heights.
 *
 * **The state at 0x4e6b picks one of two markers, or neither.** 0x800 draws
 * +0x50 at x = 0x248 and 0x400 draws +0x52 at x = 0x25d, both at y = 0x45, and
 * any other state draws no marker at all. That is the only thing in the five
 * layers that changes with the state.
 *
 * The last two are +0x14 again at (0x238, 0x59) - a third time - and +0x12 at
 * (0x240, 0x168).
 */
void draw_machine_layer_e(void)
{
    register int16_t n;

    draw_machine_layer_f();
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    for (n = 8; n < 0x162; n += 8)
        draw_bitmap(g_border_art[0x5], 0x238, n, 0);
    for (n = 0; n < 0x16f; n += 8)
        draw_bitmap(g_border_art[0x8], 0x278, n, 0);
    draw_bitmap(g_border_art[0x1], 0x230, 0, 0);
    draw_bitmap(g_border_art[0x3], 0x230, 0x160, 0);
    g_vmds.second_colour = 0;
    clip_and_draw_line(0x238, 0, 0x27f, 0);
    draw_bitmap(g_border_art[0xa], 0x238, 0, 0);
    draw_bitmap(g_border_art[0xa], 0x238, 0x3b, 0);
    draw_bitmap(g_border_art[0xb], 0x23f, 0x42, 0);
    if (g_round_state == 0x800)
        draw_bitmap(g_border_art[0x28], 0x248, 0x45, 0);
    else if (g_round_state == 0x400)
        draw_bitmap(g_border_art[0x29], 0x25d, 0x45, 0);
    draw_bitmap(g_border_art[0xa], 0x238, 0x59, 0);
    draw_bitmap(g_border_art[0x9], 0x240, 0x168, 0);
    restore_cursor_following();
}

/*
 * 0x17caf
 *
 * **The parts bin**: the column down the right of the screen listing the parts
 * the player has, each as its icon with a count under it.
 *
 * The list at DGROUP 0x50d3 is walked, and this is the part worth reading
 * slowly: the parts are **grouped by kind as it goes**, not counted in
 * advance. For each run, the kind is taken from +4 of the first entry, and the
 * walk continues while the next entry has the same kind, counting as it goes.
 * The entry the game has singled out - the one at 0x50d5 - is *not* counted:
 * it starts the count at 0 rather than 1 and is skipped inside the run. So the
 * number under an icon is how many are left to place, and the one being
 * carried is already gone from it.
 *
 * A run whose count comes to zero draws nothing at all, icon included.
 *
 * The count is turned into a string and centred in the 0x38-wide cell -
 * `text_width_thunk` measured, not assumed - and drawn twice for a shadow:
 * colour 0 at one pixel left and one down, then 0xe at the true place. The
 * baseline is the icon's own height plus one, and is clamped to 0x161 so a
 * tall part cannot push its number off the bottom.
 *
 * The cells are 0x34 apart and the walk stops at y = 0x134, so the bin holds
 * however many fit and the rest of the list is simply not shown.
 *
 * The two `fill_rect`s at the top clear the column in two pieces - 0x241 wide
 * by 0x37 and 0x240 by 0x103 - which overlap by a pixel in x.
 */
void draw_machine_layer_a(void)
{
    int16_t text_x;
    int16_t text_y;
    int16_t kind;
    int16_t count;
    char digits[8];
    struct bitmap *icon;
    register struct part *part;
    register int16_t y;

    g_vmds.page_dst = g_vmds.page_back;
    g_vmds.clip_enabled = 1;
    set_clip_play_area();
    g_vmds.fill_enabled = 1;
    g_vmds.second_colour = g_vmds.fill_colour = (uint8_t)g_bin_colour;
    cursor_redraw_off_thunk();
    fill_rect(0x241, 0x63, 0x37, 2);
    fill_rect(0x240, 0x65, 0x38, 0x103);
    restore_cursor_following();
    g_vmds.text_style = 1;                            /* transparent text */
    part = (g_held_parts.bin_list->next);
    y = 0x64;
    while (part != NULL && y <= 0x134) {
        kind = part->kind;
        if (part == g_held_parts.dragged_part)
            count = 0;
        else
            count = 1;
        /* The step to the next part is at the loop's test, as a statement:
           `mov si,[si] / or si,si`. An assignment inside a `while` condition
           goes through AX under both Borland compilers, so the loop is
           entered at the step, which is what its bytes do. */
        goto next;
        do {
            if (part != g_held_parts.dragged_part)
                count++;
next:
            part = part->next;
        } while (part != NULL && part->kind == kind);
        if (count == 0)
            continue;
        cursor_redraw_off_thunk();
        icon = g_icons_bmp[kind];
        draw_bitmap_centred(icon, 0x240, y, 0x38, 0x2a);
        itoa(count, digits, 10);
        text_x = ((0x38 - (int16_t)text_width_thunk(digits)) >> 1) + 0x240;
        if ((text_y = y + icon->height + ((0x2a - icon->height) >> 1) + 1) > 0x161)
            text_y = 0x161;
        g_vmds.text_colour = 0;
        draw_string(digits, text_x - 2, text_y + 1);
        g_vmds.text_colour = 0x0e;
        draw_string(digits, text_x - 1, text_y);
        restore_cursor_following();
        y += 0x34;
    }
}

/*
 * 0x17e27
 *
 * Draw a bitmap **centred in a box**: the caller gives a corner and a size,
 * and the picture's own width and height - the words at +6 and +8 of its
 * header - decide where inside it lands.
 *
 * Both halves are `sar`, an arithmetic shift, so a picture *wider* than the
 * box centres to a negative offset and hangs off both sides equally rather
 * than being pinned to the left. That is what puts a part's icon in the middle
 * of its cell in the copy-protection grid whatever size the part is.
 */
void draw_bitmap_centred(register struct bitmap *bmp, register int16_t x,
                         int16_t y, int16_t w, int16_t h)
{
    x += (w - bmp->width) >> 1;
    y += (h - bmp->height) >> 1;
    draw_bitmap(bmp, x, y, 0);
}

/*
 * 0x17e5b
 *
 * The **animated header** at the top of the parts bin, clipped to
 * (0x240, 0xa)..(0x277, 0x3b) and drawn from the set at DGROUP 0x4ec9 - the
 * `gp_menu.bmp` that `game_setup` loaded and kept.
 *
 * The frame comes from the counter at 0x4e87, halved. **And the counter is set
 * to zero on the way in**, so every call draws frame 0 and the other branches
 * are dead here - they exist for a caller that does not reset it, and there
 * is none in what has been read so far. Transcribed whole rather than reduced
 * to the branch that runs, because reducing it would be writing a routine the
 * original does not have.
 *
 * Two pieces slide: the one at +2 by `((f - 4) * 2) mod 0x38` and the one at
 * +4 by `((f - 4) * 4) mod 0x38`, both from x = 0x208, and both pinned at 0
 * until the frame reaches 4. So the second moves at twice the speed of the
 * first, and neither moves at all for the first four frames.
 *
 * Then one of two figures, chosen at frame 6: below it, the picture named by
 * the tables at 0x25a2, 0x25ae and 0x25ba - index, x and y, all indexed by the
 * frame; at or above it, the frame is taken modulo 4 and the picture is +0x10
 * of the set with its position from 0x25c6 and 0x25ce. Four tables, and each
 * carries the address it came from.
 *
 * The clip is put back to the play area on the way out, which is why
 * `draw_machine_layer_e` can carry on drawing the border afterwards.
 */
void draw_machine_layer_f(void)
{
    int16_t slide_b;
    register int16_t frame;
    register int16_t slide_a;

    g_vmds.clip_enabled = 1;
    g_vmds.clip_top = 0x0a;
    g_vmds.clip_bottom = 0x3b;
    g_vmds.clip_left = 0x240;
    g_vmds.clip_right = 0x277;
    g_loop_frames = 0;
    if ((frame = g_loop_frames >> 1) >= 4)
        slide_a = ((frame - 4) * 2) % 0x38;
    else
        slide_a = 0;
    if ((frame = g_loop_frames >> 1) >= 4)
        slide_b = ((frame - 4) * 4) % 0x38;
    else
        slide_b = 0;
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(g_menu_bmp[0], 0x240, 0x0a, 0);
    draw_bitmap(g_menu_bmp[0x1], slide_a + 0x208, 0x1a, 0);
    draw_bitmap(g_menu_bmp[0x2], slide_b + 0x208, 0x20, 0);
    if (frame < 6)
        /* the picture, its x and its y, by frame */
        draw_bitmap(g_menu_bmp[g_machine_draw_menu_anim.picture[frame]], g_machine_draw_menu_anim.picture_x[frame],
                    g_machine_draw_menu_anim.picture_y[frame], 0);
    if (frame < 4)
        draw_bitmap(g_menu_bmp[0x7], 0x24a, 0x2a, 0);
    else {
        frame &= 3;
        /* the sprite's x and y, by the frame modulo four */
        draw_bitmap(((g_menu_bmp + 8)[frame]),
                    g_machine_draw_menu_anim.sprite_x[frame],
                    g_machine_draw_menu_anim.sprite_y[frame], 0);
    }
    restore_cursor_following();
    set_clip_play_area();
}

/*
 * 0x17eab
 *
 * **Draw the part in your hand at the pointer**, and tell the shape allocator
 * where it went so the backdrop under it can be restored.
 *
 * The icon is the kind's entry in the list at DGROUP 0x4ec7 - the one
 * `game_intro` loaded from "icons.bmp" - indexed by the carried part's kind at
 * +4, doubled. It is drawn straight at 0x5784,0x5782, the pointer, with the
 * clip set to the play area first so it cannot spill into the panel.
 *
 * `cursor_redraw_off_thunk` **both sides of the draw**, not once: the flag is
 * cleared, the bitmap goes down, and it is cleared again. Transcribed as the
 * two calls it is.
 *
 * Then the shape: the point handed to `alloc_shape` is the pointer offset by
 * 0x4e9f and 0x4e9d - the icon's hot spot - and the extent is the bitmap's own
 * +6 and +8. Both go in as **addresses of locals**: `lea ax,[bp-6]`.
 */
void draw_carried_icon(void)
{
    uint16_t kind;
    int16_t at[2];
    struct extent16 ext;
    register struct bitmap *bmp;

    set_clip_play_area();
    kind = g_held_parts.dragged_part->kind;
    bmp = g_icons_bmp[kind];
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(bmp, g_pointer.pointer_x, g_pointer.pointer_y, 0);
    cursor_redraw_off_thunk();
    at[0] = g_pointer.pointer_x + g_origin_b_x;
    at[1] = g_pointer.pointer_y + g_origin_b_y;
    ext.width = bmp->width;
    ext.height = bmp->height;
    alloc_shape((uint8_t *)at, (uint8_t *)&ext, 1, 2, 0);
}

/*
 * 0x17f2d
 *
 * One frame of the machine: settle the display buckets, run the physics, draw.
 *
 * A part carries a countdown at +0x14 saying it has moved and its bucket is
 * stale. Each frame every part with a non-zero one is put back in its bucket
 * by `link_record_into_buckets` and the countdown steps down, so a part that
 * moved is re-filed for as many frames as the count says. With `redraw_all`
 * set the count is ignored and cleared instead, which is how the first frame
 * of a machine files everything at once.
 *
 * The part at DGROUP 0x50d5 - the one being dragged - is done first and then
 * skipped in the walk, so it is filed before anything can be filed on top of
 * it, and only once.
 */
void step_and_draw_machine(int16_t redraw_all)
{
    struct part *si;

    if (g_held_parts.dragged_part != 0 && g_held_parts.dragged_part->redraw_count != 0) {
        link_record_into_buckets(g_held_parts.dragged_part);
        g_held_parts.dragged_part->redraw_count--;
    }

    for (si = pick_by_flag((TRAIT_IN_PLACED_LIST | TRAIT_IN_MOVING_LIST)); si != NULL;
         si = pick_for_record(si, TRAIT_IN_MOVING_LIST)) {
        if ((redraw_all != 0 || si->redraw_count != 0)
            && si != g_held_parts.dragged_part)
            link_record_into_buckets(si);

        if (redraw_all != 0)
            si->redraw_count = 0;
        else if (si->redraw_count != 0)
            si->redraw_count--;
    }

    refile_overlapping_parts();
    draw_machine(0, 0);
}

/*
 * 0x17fb0
 *
 * **Draw the selection around a part**: the marching-ants box, the four edge
 * strips, and whichever of the six handles that part can actually use.
 *
 * `0x25d6` is a phase counter cycling 0 to 3 and back, and it is what makes
 * the border crawl: every strip is drawn offset by it, or by `4 - it`, so the
 * pattern walks by a pixel a frame. It is stepped **once per call**, at the
 * top, before anything is drawn.
 *
 * The box comes from three different places depending on kind. A belt takes
 * its link's far part and that part's +0x56/+0x57 anchor; a rope takes its
 * +0x66 record's part and the end its +0xb names, offset by -8 and -4; and
 * everything else takes the part's own +0x2a/+0x2c and +0x44/+0x46.
 *
 * **The belt arm reads `[bp-0x20]` before anything has written it.** It uses
 * it in `([bp-0x20] >> 1) < si->+0x58`, the same shape of test
 * `part_handle_at_pointer` makes against the part's +0x46 - which is what that
 * local holds on *every other* path through this routine. On the belt path it
 * is whatever the previous call left on the stack. It is transcribed as the
 * uninitialised read it is, because the alternative is inventing the height
 * the original never fetched; the C reads a local that is only assigned later,
 * which is undefined behaviour and is the point.
 *
 * Each edge is clipped to 8..0x237 and 8..0x167, and an edge that had to be
 * clipped has its strip suppressed rather than drawn short - that is what the
 * four flags are for. A box taller than 0x80 gets its vertical strips drawn
 * twice, half a screen apart, because the strip bitmap is only that tall.
 *
 * Handle 0xe is the odd one: it draws two crossed lines rather than a border,
 * in colour 0 then 0xc, which is the "no" cursor.
 *
 * The handles themselves come from `part_flip_options` through 0x50bd, the
 * same four bits `part_handle_at_pointer` tests, so what is drawn and what can
 * be grabbed cannot disagree. The corner at +0x36 is always drawn.
 *
 * Last, the box is grown by 0xc on every side - 0x18 and 0x19 on the two
 * extents, which is not symmetric and is what the original writes - and handed
 * to `alloc_shape` so the whole decoration can be lifted off again.
 */
void draw_part_selection(register struct part *part, int16_t which, int16_t flags)
{
    int16_t step;
    int16_t tall;
    int16_t keep_t;
    int16_t keep_b;
    int16_t keep_l;
    int16_t keep_r;
    int16_t hx;
    int16_t hxm;
    int16_t hxr;
    int16_t hy;
    int16_t hym;
    int16_t hyb;
    uint16_t idx;
    struct point16 at;
    struct extent16 ext;
    struct rope *rec;
    struct part *end;

    if (g_selection_phase == 3)
        g_selection_phase = 0;
    else
        g_selection_phase++;
    step = 4 - g_selection_phase;
    keep_t = keep_b = keep_l = keep_r = 1;
    g_vmds.page_dst = g_vmds.page_back;
    if (part->kind == KIND_BELT) {
        end = (part->belt->end_b);
        at.x = end->box[0].x + end->grab.x;
        at.y = end->box[0].y + end->grab.y;
        ext.width = end->grab_size;
        /* Reads the height before it is written: the original's. */
        if ((ext.height >> 1) < (int16_t)end->grab_size)
            ext.height = 0x0a;
        else
            ext.height = end->grab_size;
    } else if (part->kind == KIND_ROPE) {
        rec = part->rope[0];
        end = rec->end_b;
        idx = rec->slot_b;
        at.x = end->box[0].x + end->attach[idx].x - 8;
        at.y = end->box[0].y + end->attach[idx].y - 4;
        ext.width = 0x10;
        ext.height = 8;
    } else {
        at = part->box[0];
        ext = part->size[0];
    }
    g_vmds.clip_left = at.x - g_origin_x;
    g_vmds.clip_right = at.x + ext.width - g_origin_x - 1;
    g_vmds.clip_top = at.y - g_origin_y;
    g_vmds.clip_bottom = at.y + ext.height - g_origin_y - 1;
    g_vmds.clip_enabled = 1;
    if (g_vmds.clip_left < 8) {
        g_vmds.clip_left = 8;
        keep_l = 0;
    }
    if (g_vmds.clip_right > 0x237) {
        g_vmds.clip_right = 0x237;
        keep_r = 0;
    }
    if (g_vmds.clip_top < 8) {
        g_vmds.clip_top = 8;
        keep_t = 0;
    }
    if (g_vmds.clip_bottom > 0x167) {
        g_vmds.clip_bottom = 0x167;
        keep_b = 0;
    }
    if (which == 0x0e) {
        g_vmds.second_colour = 0;
        clip_and_draw_line(g_vmds.clip_left, g_vmds.clip_top + 1,
                           g_vmds.clip_right, g_vmds.clip_bottom + 1);
        clip_and_draw_line(g_vmds.clip_left, g_vmds.clip_bottom + 1,
                           g_vmds.clip_right, g_vmds.clip_top + 1);
        g_vmds.second_colour = 0x0c;
        clip_and_draw_line(g_vmds.clip_left, g_vmds.clip_top,
                           g_vmds.clip_right, g_vmds.clip_bottom);
        clip_and_draw_line(g_vmds.clip_left, g_vmds.clip_bottom,
                           g_vmds.clip_right, g_vmds.clip_top);
    }
    at.x = g_vmds.clip_left + g_origin_x;
    at.y = g_vmds.clip_top + g_origin_y;
    ext.width = g_vmds.clip_right - g_vmds.clip_left + 1;
    if ((ext.height = g_vmds.clip_bottom - g_vmds.clip_top + 1) > 0x80)
        tall = 1;
    else
        tall = 0;
    cursor_redraw_off_thunk();
    if (keep_l) {
        draw_bitmap_scaled(((g_cursor_art + 1)[which]), g_vmds.clip_left, g_vmds.clip_top - step, 8, 0x88, 0);
        if (tall)
            draw_bitmap_scaled(((g_cursor_art + 1)[which]), g_vmds.clip_left, g_vmds.clip_top - step + 0x80,
                               8, 0x88, 0);
    }
    if (keep_t)
        draw_bitmap_scaled(g_cursor_art[which],
                           g_vmds.clip_left - g_selection_phase,
                           g_vmds.clip_top, 0x110, 1, 0);
    if (keep_r) {
        g_vmds.clip_right++;
        draw_bitmap_scaled(((g_cursor_art + 1)[which]), g_vmds.clip_right - 1,
                           g_vmds.clip_top - g_selection_phase,
                           8, 0x88, 0);
        if (tall)
            draw_bitmap_scaled(((g_cursor_art + 1)[which]), g_vmds.clip_right - 1,
                               g_vmds.clip_top - g_selection_phase + 0x80,
                               8, 0x88, 0);
        g_vmds.clip_right--;
    }
    if (keep_b) {
        g_vmds.clip_bottom++;
        draw_bitmap_scaled(g_cursor_art[which], g_vmds.clip_left - step, g_vmds.clip_bottom - 1,
                           0x110, 1, 0);
    }
    set_clip_for_mode();
    hx = at.x - g_origin_x - 12;
    hxm = hx + (ext.width >> 1) + 6;
    hxr = hx + ext.width + 0x0c;
    hy = at.y - g_origin_y - 11;
    hym = hy + (ext.height >> 1) + 6;
    hyb = hy + ext.height + 0x0c;
    g_vmds.fill_enabled = 1;
    g_vmds.second_colour = g_vmds.fill_colour = 0x0f;
    g_level_settings.flip_options = part_flip_options(part);
    draw_bitmap(g_cursor_art[0x1b], hx, hy, 0);
    if (g_level_settings.flip_options & 1) {
        draw_bitmap(g_cursor_art[0x1c], hx, hym, 0);
        draw_bitmap(g_cursor_art[0x1c], hxr, hym, 0);
    }
    if (g_level_settings.flip_options & 2) {
        draw_bitmap(g_cursor_art[0x1d], hxm, hy, 0);
        draw_bitmap(g_cursor_art[0x1d], hxm, hyb, 0);
    }
    if (g_level_settings.flip_options & 4)
        draw_bitmap(g_cursor_art[0x1e], hx, hyb, 0);
    if (g_level_settings.flip_options & 8)
        draw_bitmap(g_cursor_art[0x1f], hxr, hyb, 0);
    at.x -= 0x0c;
    at.y -= 0x0c;
    ext.width += 0x18;
    ext.height += 0x19;
    alloc_shape((uint8_t *)&at, (uint8_t *)&ext, flags, 2, 0);
    restore_cursor_following();
}

/*
 * 0x184bc
 *
 * Clear six words at DGROUP 0x50bf. The loop counts *down* from 5 and tests
 * `jge`, so index 0 is cleared too - six entries, not five. What they hold is
 * not established.
 */
void clear_layer_heads(void)
{
#ifdef __TURBOC__
    /* The count is in AX, Borland's pseudo-register: no register is saved. */
    for (_AX = 5; (int16_t)_AX >= 0; _AX--)
        g_layer_head[_AX] = 0;
#else
    int16_t i;

    for (i = 5; i >= 0; i--)
        g_layer_head[i] = 0;
#endif
}

/*
 * 0x184d5
 *
 * Link a record into up to two buckets, and mark it linked.
 *
 * Which buckets is decided by two bytes in the record's kind entry - the same
 * 0x3a-byte table `clamp_record_pair` indexes, read here at +0x1c rather than
 * +0x0a - and a byte of 0xff means "not in this bucket". The bucket heads are
 * the six-word array at DGROUP 0x50bf, which is the array
 * `clear_layer_heads` zeroes; that the two routines agree about it is what
 * identifies it as a set of list heads.
 *
 * The insertion is at the head: the record's link at +0x74 (or +0x76 for the
 * second bucket) takes the old head and the head becomes the record. For the
 * first bucket only, the bucket number is also stored at +0x7f.
 *
 * One record is special - the one whose address is at DGROUP 0x50d5 always
 * goes into bucket 0 whatever its kind says.
 */
void link_record_into_buckets(register struct part *rec)
{
    int16_t kind;
    uint8_t slot;
#ifndef __TURBOC__
    uint16_t _CX;               /* the count: CX, Borland's pseudo-register */
#endif

    rec->traits2 |= TRAIT2_FILED;
    kind = rec->kind;
    for (_CX = 0; (int16_t)_CX < 2; _CX++) {
        if ((slot = g_part_kinds[kind].refile_level[_CX]) != 0xff) {
            if (rec == g_held_parts.dragged_part)
                slot = 0;
            rec->layer_next[_CX] = g_layer_head[slot];
            g_layer_head[slot] = rec;
            if (_CX == 0)
                rec->layer_slot = slot;
        }
    }
}

/*
 * 0x1854a
 *
 * Draw the machine: the six bucket lists, deepest first.
 *
 * The buckets are filled by `link_record_into_buckets` and each is a tree
 * walked by the byte at +0x7f exactly as `refile_overlapping_parts` walks it -
 * equal to the level takes +0x74, anything else +0x76. Every part visited has
 * bit 5 of +0x0a cleared, which is the "already in a bucket" mark, so the
 * lists are emptied by being drawn; `clear_layer_heads` at the end takes
 * the heads with them.
 *
 * A belt, kind 8, and a rope, kind 0x0a, each draw themselves; kind 0x31 draws
 * nothing at all. Everything else goes through the one blitter, which is told
 * the level as well, so a part in two buckets is drawn twice at two depths.
 *
 * The page being drawn into, `g_vmds.page_dst`, is set from `g_vmds.page_back` first, and the
 * clip is put back to whatever the mode wants.
 */
void draw_machine(register int16_t a, int16_t b)
{
    uint8_t counter;
    uint8_t level;
    register struct part *part;

    g_vmds.page_dst = g_vmds.page_back;
    g_vmds.clip_enabled = 1;
    set_clip_for_mode();
    for (counter = 6; counter > 0; counter--) {
        level = counter - 1;
        part = g_layer_head[level];
        while (part != NULL) {
            part->traits2 &= ~TRAIT2_FILED;
            if (part->kind == KIND_BELT)
                draw_belt(part, a);
            else if (part->kind == KIND_ROPE)
                draw_rope(part, a);
            else if (part->kind != KIND_ANCHOR)
                draw_part(part, level, a, b);
            if (part->layer_slot == level)
                part = part->layer_next[0];
            else
                part = part->layer_next[1];
        }
    }
    clear_layer_heads();
}

/*
 * 0x185e1
 *
 * Draw a belt: two straight lines in colour 0, from the four points its record
 * keeps at +8 through +0x16. Two lines and not one because a belt over a pulley
 * has a corner in it; a belt with nothing at either end - +4 or +6 zero - draws
 * nothing at all.
 *
 * With `a` set all eight coordinates are scaled into the preview window first,
 * exactly as `draw_rope` and `draw_part` scale theirs.
 */
void draw_belt(struct part *part, register int16_t a)
{
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
    int16_t x3;
    int16_t y3;
    register struct belt *belt;

    belt = part->belt;
    if (belt->end_a == 0)
        return;
    if (belt->end_b == 0)
        return;
    cursor_redraw_off_thunk();
    x0 = belt->pt[0][0].x - g_origin_x;
    y0 = belt->pt[0][0].y - g_origin_y;
    x1 = belt->pt[0][1].x - g_origin_x;
    y1 = belt->pt[0][1].y - g_origin_y;
    x2 = belt->pt[0][2].x - g_origin_x;
    y2 = belt->pt[0][2].y - g_origin_y;
    x3 = belt->pt[0][3].x - g_origin_x;
    y3 = belt->pt[0][3].y - g_origin_y;
    if (a != 0) {
        x0 = (int16_t)(mul16x16(x0, a) >> 10);
        x0 += 0x110;
        y0 = (int16_t)(mul16x16(y0, a) >> 10);
        y0 += 0x48;
        x1 = (int16_t)(mul16x16(x1, a) >> 10);
        x1 += 0x110;
        y1 = (int16_t)(mul16x16(y1, a) >> 10);
        y1 += 0x48;
        x2 = (int16_t)(mul16x16(x2, a) >> 10);
        x2 += 0x110;
        y2 = (int16_t)(mul16x16(y2, a) >> 10);
        y2 += 0x48;
        x3 = (int16_t)(mul16x16(x3, a) >> 10);
        x3 += 0x110;
        y3 = (int16_t)(mul16x16(y3, a) >> 10);
        y3 += 0x48;
    }
    g_vmds.second_colour = 0;
    clip_and_draw_line(x0, y0, x1, y1);
    clip_and_draw_line(x2, y2, x3, y3);
    restore_cursor_following();
}

/*
 * 0x18764
 *
 * Draw a quadratic curve through three points, by forward differences in
 * 32-bit fixed point.
 *
 * `shift` is the resolution: 1 << shift steps, and every coordinate is carried
 * shifted left by 2 * shift so the divisions come out as shifts. The second
 * difference is `p0 + p2 - 2*p1`, the first is `(p1 - p0) << (shift + 1)`, and
 * each step adds the first difference plus the second times the odd number
 * 2i + 1 - which is the standard way of stepping a parabola without a divide.
 *
 * A step that lands on the same pixel as the last draws nothing, so a slow
 * curve does not draw the same line over and over.
 *
 * The loop counter and its limit are compared as a 32-bit pair, the high words
 * signed and the low words unsigned; both are small and non-negative here, so
 * the port writes the comparison the values actually mean.
 */
void draw_curve(uint16_t colour, int16_t shift,
                int32_t x0, int32_t x1, int32_t x2,
                int32_t y0, int32_t y1, int32_t y2)
{
    int16_t px;
    int16_t py;
    int32_t ddx;
    int32_t ddy;
    int32_t dx;
    int32_t dy;
    int32_t X;
    int32_t Y;
    int32_t i;
    int32_t steps;
    int32_t s2;
    register int16_t sx;
    register int16_t sy;

    /* A word parameter, of which the colour is the low byte: the caller
       zero-extends it (`mov ah,0`), and only AL is read here. */
    g_vmds.second_colour = (uint8_t)colour;
    ddx = x2 + x0 - x1 * 2;
    ddy = y2 + y0 - y1 * 2;
    dx = (int32_t)((uint32_t)(x1 - x0) << (shift + 1));
    dy = (int32_t)((uint32_t)(y1 - y0) << (shift + 1));
    s2 = shift << 1;
    X = (int32_t)((uint32_t)x0 << s2);
    Y = (int32_t)((uint32_t)y0 << s2);
    steps = 1 << shift;
    px = (int16_t)(X >> s2);
    py = (int16_t)(Y >> s2);
    for (i = 0; i <= steps; i++) {
        sx = (int16_t)(X >> s2);
        sy = (int16_t)(Y >> s2);
        if (px != sx || py != sy) {
            clip_and_draw_line(px, py, sx, sy);
            px = sx;
            py = sy;
        }
        X += dx + ddx * (i * 2 + 1);
        Y += dy + ddy * (i * 2 + 1);
    }
}

/*
 * 0x18920
 *
 * One length of rope between two points. Slack of four or less is a straight
 * line; anything more is a curve whose middle control point is the midpoint
 * pushed **down** by the slack, so a loose rope sags.
 */
void draw_rope_segment(register int16_t x0, register int16_t y0, int16_t x1,
                       int16_t y1, int16_t slack)
{
    int16_t mx;
    int16_t my;

    if (slack > 4) {
        mx = (x0 + x1) >> 1;
        my = ((y0 + y1) >> 1) + slack;
        draw_curve(g_vmds.second_colour, 4, x0, mx, x1, y0, my, y1);
    } else
        clip_and_draw_line(x0, y0, x1, y1);
}

/*
 * 0x18996
 *
 * Draw a rope: every length of it, from the part it starts at to the part it
 * ends at, following the chain of pulleys through each one's +0x5a links.
 *
 * Each end of a length is either a pulley - kind 7, whose own rope record
 * carries the tangent points at +0x14 and +0x18 - or the part the rope is
 * fastened to, whose points come from the rope record itself. The two cases
 * differ in more than the source: an end that is *not* a pulley sets the flag
 * that makes the length sag, because that is the length whose slack was
 * measured. A length between two pulleys is drawn straight.
 *
 * With `a` set every point is scaled into the preview window before drawing,
 * and the little cap bitmap that marks a fastening is left off.
 */
void draw_rope(struct part *part, int16_t a)
{
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
    int16_t sags;
    int16_t slack;
    struct bitmap *knot;
    struct rope *rope;
    register struct part *next;
    register struct part *cur;

    rope = part->rope[0];
    cur = rope->end_a;
    if ((next = cur->link[rope->slot_a]) == NULL)
        next = rope->end_b;
    while (cur != NULL && next != NULL) {
        sags = 0;
        if (cur->kind == KIND_PULLEY) {
            x0 = cur->rope[0]->pt[0][1].x - g_origin_x;
            y0 = cur->rope[0]->pt[0][1].y - g_origin_y;
        } else {
            x0 = rope->pt[0][0].x - g_origin_x;
            y0 = rope->pt[0][0].y - g_origin_y;
            sags = 1;
        }
        if (next->kind == KIND_PULLEY) {
            x1 = next->rope[0]->pt[0][0].x - g_origin_x;
            y1 = next->rope[0]->pt[0][0].y - g_origin_y;
        } else {
            x1 = rope->pt[0][1].x - g_origin_x;
            y1 = rope->pt[0][1].y - g_origin_y;
            sags = 1;
        }
        if (a != 0) {
            x0 = (int16_t)(mul16x16(x0, a) >> 10);
            x0 += 0x110;
            y0 = (int16_t)(mul16x16(y0, a) >> 10);
            y0 += 0x48;
            x1 = (int16_t)(mul16x16(x1, a) >> 10);
            x1 += 0x110;
            y1 = (int16_t)(mul16x16(y1, a) >> 10);
            y1 += 0x48;
        }
        g_vmds.second_colour = 6;
        cursor_redraw_off_thunk();
        if (sags != 0) {
            slack = link_slack(cur, rope, 3);
            draw_rope_segment(x0, y0, x1, y1, slack);
        } else
            clip_and_draw_line(x0, y0, x1, y1);
        if (a == 0) {
            /* 1.11 draws the knot from the rope's own bitmaps, where 1.00
               took the panel border's 0x24th. */
            knot = g_part_kinds[KIND_ROPE].bitmaps[0];
            if (cur->kind != KIND_ANCHOR && cur->kind != KIND_PULLEY)
                draw_bitmap(knot, x0 - 5, y0 - 2, 0);
            if (next->kind != KIND_ANCHOR && next->kind != KIND_PULLEY)
                draw_bitmap(knot, x1 - 5, y1 - 2, 0);
        }
        restore_cursor_following();
        cur = next;
        if (cur->kind != KIND_PULLEY)
            next = NULL;
        else
            next = next->link[0];
    }
}

/*
 * 0x18b94
 *
 * Draw one part, at one level, either scaled or not.
 *
 * `a` and `b` are the scale: zero means "no scaling" and everything goes
 * through `draw_bitmap` at the position it was worked out at; anything else
 * multiplies each coordinate by `a` and each size by `b`, takes ten fractional
 * bits off, and offsets into the scaled window at 0x110, 0x48 - so the same
 * routine draws the play area and the small preview.
 *
 * There are two ways a part is made of bitmaps, and bit 6 of the flags at +6
 * chooses between them.
 *
 * **Tiled.** The part's size at +0x44 and +0x46 in sixteenths gives a grid,
 * and the bitmap for each cell is picked from the eight around the form: a
 * single row runs form, form+px+1, form+3 left to right; a single column runs
 * form+4, form+py+5, form+7 top to bottom; anything else stays on the form
 * itself. `px` and `py` start from bit 4 of the position and flip every cell,
 * so a run of middle tiles alternates between two bitmaps rather than
 * repeating one.
 *
 * **Listed.** A chain of records, each holding the level it draws at, up to
 * four frame numbers, and an x,y offset for each - `krec[0x16]` indexed by the
 * form when bit 12 of +8 is set, and otherwise a single record built in DGROUP
 * at 0x124 out of the form, the level and the kind's own adjustment. A record
 * whose level does not match is skipped, unless this is the part being dragged
 * at DGROUP 0x50d5, which always draws. Bits 4 and 5 of +8 mirror the part
 * horizontally and vertically: the offset is measured from the far edge
 * instead, and the mirror is passed on to the blitter in the mode word.
 */
void draw_part(register struct part *part, uint8_t level, int16_t a, int16_t b)
{
    uint16_t kind;
    uint16_t form;
    int16_t col;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    int16_t sx;
    int16_t sy;
    int16_t cols;
    int16_t rows;
    int16_t x0;
    uint16_t flip;
    uint16_t idx;
    uint16_t px;
    uint16_t py;
    uint8_t frame;
    const struct point8 *hot;          /* the kind's hot spot for this form, a table offset */
    int16_t i;
    const struct part_kind *kindrec;
    const struct draw_step *step;
    struct bitmap *bmp;

    kind = part->kind;
    form = part->form;
    kindrec = &g_part_kinds[kind];
    if ((hot = kindrec->hotspots) != 0)
        hot += form;
    cursor_redraw_off_thunk();
    if (part->traits & TRAIT_TILED) {
        cols = part->size[0].width >> 4;
        rows = part->size[0].height >> 4;
        x0 = part->pos[0].x - g_origin_x;
        y = part->pos[0].y - g_origin_y;
        if (hot != 0) {
            x0 += (int8_t)hot->x;
            y = (int8_t)hot->y;
        }
        px = (x0 & 0x10) >> 4;
        py = (y & 0x10) >> 4;
        idx = form;
        for (i = 0; i < rows; i++, y += 0x10, py ^= 1)
            for (col = 0, x = x0; col < cols; col++, x += 0x10, px ^= 1) {
                if (rows == 1) {
                    if (col == 0)
                        idx = form;
                    else if (cols - 1 == col)
                        idx = form + 3;
                    else
                        idx = form + px + 1;
                } else if (cols == 1) {
                    if (i == 0)
                        idx = form + 4;
                    else if (rows - 1 == i)
                        idx = form + 7;
                    else
                        idx = form + py + 5;
                }
                if (a != 0) {
                    w = (int16_t)(mul16x16(0x10, b) >> 10);
                    h = (int16_t)(mul16x16(0x10, b) >> 10);
                    sx = (int16_t)(mul16x16(x, a) >> 10);
                    sx += 0x110;
                    sy = (int16_t)(mul16x16(y, a) >> 10);
                    sy += 0x48;
                    draw_bitmap_scaled(kindrec->bitmaps[idx],
                                       sx, sy, w, h, 0);
                } else
                    draw_bitmap(kindrec->bitmaps[idx],
                                x, y, 0);
            }
    } else {
        if (part->state & STATE_DRAW_STEPS)
            step = (kindrec->bitmaps2[form]);
        else {
            step = &g_default_draw_step;
            g_default_draw_step.frame[0] = (uint8_t)form;
            g_default_draw_step.level = level;
            if (hot != 0) {
                g_default_draw_step.offset[0].x = hot->x;
                g_default_draw_step.offset[0].y = hot->y;
            } else
                g_default_draw_step.offset[0].x = g_default_draw_step.offset[0].y = 0;
        }
        for (; step != NULL; step = step->next) {
            if (step->level != level && part != g_held_parts.dragged_part)
                continue;
            else
                frame = 0;
            frame = step->frame[0];
            for (i = 0; i < 4 && frame != 0xff; frame = step->frame[i + 1], i++) {
                bmp = kindrec->bitmaps[frame];
                x = part->pos[0].x - g_origin_x;
                y = part->pos[0].y - g_origin_y;
                if (part->state & STATE_FLIP_HORIZONTAL) {
                    x += part->flip_size.width - (int8_t)step->offset[i].x - bmp->width;
                    flip = DRAW_FLIP_HORIZONTAL;
                } else {
                    x += (int8_t)step->offset[i].x;
                    flip = 0;
                }
                if (part->state & STATE_FLIP_VERTICAL) {
                    y += part->flip_size.height - (int8_t)step->offset[i].y - bmp->height;
                    flip |= DRAW_FLIP_VERTICAL;
                } else
                    y += (int8_t)step->offset[i].y;
                if (a != 0) {
                    w = (int16_t)(mul16x16(bmp->width, b) >> 10);
                    h = (int16_t)(mul16x16(bmp->height, b) >> 10);
                    sx = (int16_t)(mul16x16(x, a) >> 10);
                    sx += 0x110;
                    sy = (int16_t)(mul16x16(y, a) >> 10);
                    sy += 0x48;
                    draw_bitmap_scaled(bmp, sx, sy, w, h, flip);
                } else
                    draw_bitmap(bmp, x, y, flip);
            }
        }
    }
    if (g_round_state == 0x2000 && part->kind == KIND_MAGNIFYING_GLASS)
        draw_part_extra(part);
    restore_cursor_following();
}

/*
 * 0x18fe9
 *
 * The extra a kind-0x1e part draws while the machine is in state 0x2000: a
 * three-point outline in colour 0x0e from the part it is linked to at +0x62,
 * and then the rectangle that outline covers registered as a shape so it gets
 * erased again.
 *
 * A part with nothing at +0x62 draws nothing.
 *
 * The three points share their x - the part's own left or right edge, chosen
 * by the mirror bit 4 of +8 - except for the middle one, which reaches across
 * to the linked part at its +0x72, +0x73 offset. So it is a bracket rather
 * than a triangle, which is why the bounding box is worked out from the
 * extremes rather than from all three.
 */
void draw_part_extra(register struct part *part)
{
    int16_t x[3];
    int16_t y[3];
    int16_t corner[2];
    int16_t size[2];
    struct part *held;

    if ((held = part->link[4]) == NULL)
        return;
    g_vmds.second_colour = g_vmds.fill_colour = 0x0e;
    x[1] = held->pos[0].x + held->hold.x - g_origin_x;
    y[0] = part->pos[0].y + 6 - g_origin_y;
    y[1] = held->pos[0].y + held->hold.y - g_origin_y;
    y[2] = part->pos[0].y + 0x10 - g_origin_y;
    x[0] = x[2] = ((part->state & STATE_FLIP_HORIZONTAL) ? part->pos[0].x - 1
                                           : part->pos[0].x + 0x0f)
                  - g_origin_x;
    draw_polygon(3, x, y);
    size[0] = (x[0] < x[1] ? (corner[0] = x[0], x[1] - x[0])
                           : (corner[0] = x[1], x[0] - x[1])) + 1;
    corner[1] = y[0] < y[1] ? y[0] : y[1];
    size[1] = (y[2] < y[1] ? y[1] : y[2]) - corner[1] + 1;
    corner[0] += g_origin_b_x;
    corner[1] += g_origin_b_y;
    alloc_shape((const uint8_t *)corner, (const uint8_t *)size, 1, 2, 0);
}

/*
 * 190f:0007, image 0x190f7
 *
 *
 * The module's first routine, and it does nothing at all: a frame and a `retf`.
 * It is here because the segment's own offset 0 has to be something.
 */
void seg172c_nothing(void)
{
}
