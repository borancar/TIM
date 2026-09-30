/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **ADJUST PARTS BIN** - new in 1.11. In freeform the player chooses what
 * the parts bin holds: a page of 28 icons, seven by four, out of a list of
 * sixty kinds; a click puts one of that kind in the bin, MORE pages on, and
 * CLEAR PARTS BIN empties it. The third page is only reached after typing
 * T, I and M on this screen.
 *
 * In 1.11, image 0x1304b..0x1351f, between `screen.c`'s last routine and
 * `levels.c`'s first. **A module of its own**: `game_screen` and this
 * module call each other through TLINK's `nop / push cs / call` - and so does
 * this module's call of `bin_scroll_back`, which is defined above it and
 * would be a bare `push cs / call` from the same file. Its data, the kind
 * list at DGROUP 0x22e8, comes after `screen.c`'s literal pool, and its
 * uninitialised data, 0x507c..0x5086, after `goals.c`'s. That BSS holds the
 * page shown and `game_screen`'s description editing state, whose every
 * reference is in `screen.c`. The file's name is ours.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 * JUDGE: data 0x22e8..0x2360
 */
#include <ctype.h>
#ifndef __TURBOC__
#include "hostlib.h"
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * The module's uninitialised data, DGROUP 0x507c..0x5086, highest first as
 * Borland lays it out from the header's order of mention: the first icon
 * shown at 0x5084, then the description's blink count, redraw count, editing
 * flag and caret.
 */
int16_t g_bin_adjust_top;       /* DGROUP 0x5084  0, 0x1c or 0x38 */
uint16_t g_desc_blink;          /* DGROUP 0x5082  frames counted; bit 3 shows the caret */
int16_t g_desc_redraw;          /* DGROUP 0x5080  passes left to redraw the description */
int16_t g_desc_caret_on;        /* DGROUP 0x507e  the description is being edited */
char *g_desc_caret;             /* DGROUP 0x507c  the caret, in g_level_hint */

/*
 * DGROUP 0x22e8..0x2360: **the kinds the adjuster offers**, in the order of
 * its pages - 28 a page, the last four of the sixty on the third.
 */
int16_t g_bin_adjust_kinds[60] = {
    0, 9, 43, 28, 44, 4, 3, 16, 35, 39,
    10, 23, 7, 27, 37, 21, 26, 38, 24, 50,
    1, 46, 47, 48, 2, 8, 14, 5, 13, 40,
    30, 25, 29, 18, 19, 36, 45, 22, 17, 11,
    12, 42, 6, 15, 31, 51, 52, 53, 54, 58,
    59, 60, 61, 62, 63, 64, 32, 33, 34, 65,
};

/*
 * 0x1304b
 *
 * **The adjuster's own loop**, entered from `game_screen` in state 4 and
 * left when the state is anything but 4, 8 or 0x10: 0x1000 from its DONE
 * button, which `game_screen` takes as leaving too, or 2 from the right
 * button. Its regions are the play screen's - the bin's scroll arrows keep
 * working here. The name is ours.
 */
void adjust_bin_screen(void)
{
    register int16_t typed;             /* si: how much of TIM has been typed */
    register int16_t slot;              /* di */
    int16_t redraw_buttons;             /* [bp-2] */
    int16_t repaint_all;                /* [bp-4] */
    struct part *part;                  /* [bp-6] */

    g_bin_adjust_top = redraw_buttons = repaint_all = typed = 0;

    wait_cursor();
    draw_adjust_screen();
    draw_adjust_icons(g_bin_adjust_top);
    present_back_page();
    restore_cursor();

    while (g_round_state == 4 || g_round_state == 0x10 || g_round_state == 8) {
        update_button_state();
        g_last_key = translate_key(bios_read_key());

        if (toupper(g_last_key & 0x7f) == 'T' && typed == 0)
            typed = 1;
        if (toupper(g_last_key & 0x7f) == 'I' && typed == 1)
            typed = 2;
        if (toupper(g_last_key & 0x7f) == 'M' && typed == 2)
            typed = 3;

        regions_handle_pointer(g_regions_play);

        switch (g_round_state) {
        case 0x100:                     /* DONE */
            draw_adjust_done(1);
            present_back_page();
            g_round_state = 0x1000;
            break;

        case 0x80:                      /* CLEAR PARTS BIN */
            draw_adjust_clear(1);
            present_back_page();
            if (ask_yes_no(g_messages.clear_parts_bin, g_messages.clear_parts_bin_body)) {
                clear_parts_bin();
                g_held_parts.bin_list = &g_held_parts.parts_bin;
                g_machine_has_bin = 1;
                g_redraw_e = 2;
            }
            draw_adjust_icons(g_bin_adjust_top);
            present_back_page();
            redraw_buttons = 2;
            g_round_state = 4;
            break;

        case 0x40:                      /* MORE */
            draw_adjust_more(1);
            present_back_page();
            g_bin_adjust_top += 0x1c;
            if (g_bin_adjust_top > 0x38)
                g_bin_adjust_top = 0;
            else if (g_bin_adjust_top == 0x38 && typed != 3)
                g_bin_adjust_top = 0;
            draw_adjust_icons(g_bin_adjust_top);
            present_back_page();
            redraw_buttons = 2;
            g_round_state = 4;
            break;

        case 0x20:                      /* an icon */
            slot = (g_pointer.pointer_x - 0x40) / 0x40
                 + (g_pointer.pointer_y - 0x40) / 0x32 * 7;
            slot += g_bin_adjust_top;
            if (slot < 0x3c) {
                slot = g_bin_adjust_kinds[slot];
                part = make_part(slot);
                if (check_room_in_bin()) {
                    insert_sorted(part, &g_held_parts.parts_bin);
                    g_machine_has_bin = 1;
                    g_redraw_e = 2;
                } else
                    free_part(part);
            }
            g_round_state = 4;
            break;

        case 0x10:
            bin_scroll_back(4);
            break;

        case 8:
            bin_scroll_forward(4);
            break;
        }

        if (repaint_all != 0) {
            draw_adjust_screen();
            draw_adjust_icons(g_bin_adjust_top);
            present_back_page();
            repaint_all = redraw_buttons = 0;
        }
        if (g_redraw_e != 0) {
            draw_machine_layer_a();
            g_redraw_e--;
        }
        if (g_redraw_a != 0) {
            draw_machine_layer_e();
            g_redraw_a--;
        }
        if (redraw_buttons != 0) {
            draw_adjust_buttons();
            redraw_buttons--;
        }

        present_frame(1);

        if (g_pointer.button_right == 2)
            g_round_state = 2;
    }
}

/*
 * 0x132d0
 *
 * **One page of icons**, from entry `top` of the kind list: seven across at
 * 0x40 pixels from x 0x40, four down at 0x32 from y 0x40, each centred in a
 * 0x40 by 0x30 box, over a panel painted first. The name is ours.
 */
void draw_adjust_icons(register int16_t top)
{
    register int16_t i;
    int16_t x;                          /* [bp-2] */
    int16_t y;                          /* [bp-4] */
    int16_t kind;                       /* [bp-6] */

    g_vmds.page_dst = g_vmds.page_back;
    draw_panel(0x38, 0x40, 0x1cc, 0xc8);

    for (i = 0; i < 0x1c; i++) {
        if (top + i < 0x3c) {
            x = i % 7 * 0x40 + 0x40;
            y = i / 7 * 0x32 + 0x40;
            kind = g_bin_adjust_kinds[top + i];
            cursor_redraw_off_thunk();
            draw_bitmap_centred(g_icons_bmp[kind], x, y, 0x40, 0x30);
            restore_cursor_following();
        }
    }
}

/*
 * 0x1336e
 *
 * The adjuster's frame: the title bar, its title and its three buttons. The
 * name is ours.
 */
void draw_adjust_screen(void)
{
    set_clip_play_area();
    g_vmds.page_dst = g_vmds.page_back;
    draw_title_bar(0x20, 0x20, 0x220, 0x158, 1);
    draw_scroll_text(g_messages.adjust_parts_bin, 0x64, 0x27, 0x16c);
    draw_adjust_buttons();
}

/*
 * 0x133b4
 *
 * The three buttons, up, on their panel. The name is ours.
 */
void draw_adjust_buttons(void)
{
    set_clip_play_area();
    g_vmds.page_dst = g_vmds.page_back;
    draw_panel(0x180, 0x11c, 0x80, 0x30);
    draw_adjust_done(0);
    draw_adjust_more(0);
    draw_adjust_clear(0);
}

/*
 * 0x133f7
 *
 * DONE, up (`frame` 0) or down (1): panel bitmap 0x10 + `frame`. The name
 * is ours, like its two siblings'.
 */
void draw_adjust_done(uint16_t frame)
{
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((g_panel_art + 0x10)[frame]), 0x190, 0x124, 0);
    restore_cursor_following();
}

/*
 * 0x1342d
 *
 * MORE: panel bitmap 0x27 + `frame`.
 */
void draw_adjust_more(uint16_t frame)
{
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((g_panel_art + 0x27)[frame]), 0x1b4, 0x130, 0);
    restore_cursor_following();
}

/*
 * 0x13463
 *
 * CLEAR: panel bitmap 0x1f + `frame`.
 */
void draw_adjust_clear(uint16_t frame)
{
    g_vmds.page_dst = g_vmds.page_back;
    cursor_redraw_off_thunk();
    draw_bitmap(((g_panel_art + 0x1f)[frame]), 0x1dc, 0x124, 0);
    restore_cursor_following();
}

/*
 * 0x13499
 *
 * **Is there room for one more part?** `frame.c`'s `check_room_for_part`
 * over again - the same three bands of `heap_largest_free` and the same
 * warnings - except that what it repaints after a message box is this
 * screen's icons. The name is ours.
 */
int16_t check_room_in_bin(void)
{
    register uint16_t si;

    si = heap_largest_free();
    if (si < 0x0fa0) {
        show_message_box(g_messages.out_of_memory, (char *)g_messages.you_cant_place_any);
        g_memory_warned = 1;
        draw_adjust_icons(g_bin_adjust_top);
        present_back_page();
        update_button_state();
        return 0;
    } else if (si < 0x1388 && g_memory_warned == 0) {
        show_message_box(g_messages.memory_low, (char *)g_messages.memory_is_getting_low);
        g_memory_warned = 1;
        draw_adjust_icons(g_bin_adjust_top);
        present_back_page();
        update_button_state();
    } else if (si > 0x1770)
        g_memory_warned = 0;
    return 1;
}
