/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 14de**, image
 * 0x14de0..0x1c250. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include <stdio.h>

#include "tim.h"
#include "io.h"
#include "dgroup.h"

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

    DGU16(0x50d9) = 0;
    DGU16(0x50d7) = 0;
    DGU16(0x517b) = 0;
    DGU16(0x5179) = 0;
    DGU16(0x521d) = 0;
    DGU16(0x521b) = 0;

    for (si = 0; si < 0x33; si++) {
        int16_t wanted = 0;

        if (si == 0x20 || si == 0x21 || si == 0x22) {
            if (si == 0x20 && DGU16(0x4e7d) != 0)
                wanted = 1;
            if (si == 0x21 && DGU16(0x4e81) != 0)
                wanted = 1;
            if (si == 0x22 && DGU16(0x4e7b) != 0)
                wanted = 1;
        } else if (si != 0x14 && si != 0x29 && si != 0x31) {
            wanted = 1;
        }

        if (wanted != 0) {
            uint16_t rec = make_part((uint16_t)si);

            if (rec != 0)
                insert_sorted(rec, 0x50d7);
        }
    }

    DGU16(0x50d3) = 0x50d7;
    DGU16(0x50b1) = 0;
    DGU16(0x50af) = 0;
    DGU16(0x50b3) = 0x43;
    DGU16(0x50b5) = 0x110;
    DG16(0x50b9) = -8;
    DG16(0x50b7) = -8;
    DGU16(0x50bb) = 0x3e9;
    DGU16(0x4eaf) = 0;
    DGU16(0x4ead) = 0;

    recompute_kind_physics();
}

/*
 * 0x14236 .. 0x14c8x - the **part initialisers**, forty-two of forty-eight.
 *
 * Every one of these is the same four steps, and the table at DGROUP 0x2966
 * points each part at its own:
 *
 *   1. OR some bits into the part's flags at +6, +8 and +0x0a, if it has any;
 *   2. take four bytes per bitmap - `heap_calloc_far(count, 4)` - into +0x82;
 *   3. refuse, by answering 1, if that allocation failed;
 *   4. call the part's own setup in segment 0x172c, and answer 0.
 *
 * They are transcribed as the table they are, because that is what they are:
 * the only things that differ between them are three flag words and which setup
 * is called. Every value here was read out of the image at the address in the
 * first column, and `part_init` dispatches on that address - `call_part_init`
 * in io.c reaches it because the original arrives through a relocated far
 * pointer the port has no way to call.
 *
 * **Five of the forty-eight are not in here** and are written out separately:
 * 0x143fb, 0x1443d, 0x1449d, 0x14aa2 and 0x14c48 allocate something else, or
 * nothing at all, and are not this routine with different constants.
 *
 * A sixth, 0x14c62, looked like one of them for a while and is not: the window
 * I first read it through ended four bytes before its call, so it appeared to
 * have none at all. It is in the table.
 */
static const struct {
    uint32_t at;                /* an image address: it does not fit in 16 */
    uint16_t flags6;
    uint16_t flags8;
    uint16_t flags10;
    uint16_t setup;
} part_inits[43] = {
    /*    at       +6      +8      +0a     setup */
    { 0x14236, 0x0000, 0x0000, 0x0000, 0x0001 },
    { 0x14267, 0x0040, 0x0180, 0x0000, 0x48ab },
    { 0x142a1, 0x0600, 0x0080, 0x0000, 0x2728 },
    { 0x142e6, 0x0400, 0x000c, 0x0000, 0x40f0 },
    { 0x14320, 0x0020, 0x0004, 0x0000, 0x012d },
    { 0x14361, 0x0000, 0x0081, 0x0000, 0x24d0 },
    { 0x143b3, 0x0400, 0x0801, 0x0000, 0x2ee1 },
    { 0x1446c, 0x0000, 0x0000, 0x0000, 0x0001 },
    { 0x144cb, 0x0020, 0x0004, 0x0000, 0x0f70 },
    { 0x1450c, 0x0400, 0x8000, 0x0000, 0x0c1c },
    { 0x14547, 0x0400, 0x1001, 0x0000, 0x295d },
    { 0x1458f, 0x0000, 0x0001, 0x0000, 0x0001 },
    { 0x145d1, 0x0000, 0x1000, 0x0000, 0x1be9 },
    { 0x14607, 0x0400, 0x0000, 0x0000, 0x0371 },
    { 0x1463d, 0x0020, 0x0004, 0x0000, 0x07b2 },
    { 0x1467e, 0x0400, 0x1000, 0x0004, 0x0b88 },
    { 0x146bd, 0x0420, 0x1000, 0x0004, 0x1261 },
    { 0x146fc, 0x0000, 0x0000, 0x0000, 0x08a1 },
    { 0x1472d, 0x0200, 0x1000, 0x0002, 0x1556 },
    { 0x1476c, 0x0400, 0x1004, 0x0000, 0x3294 },
    { 0x147a7, 0x0200, 0x0004, 0x0000, 0x19db },
    { 0x147c5, 0x0400, 0x1000, 0x0001, 0x1a32 },
    { 0x14804, 0x0400, 0x0000, 0x0000, 0x1d28 },
    { 0x1483a, 0x0000, 0x1001, 0x0002, 0x1dfb },
    { 0x14874, 0x0400, 0x1004, 0x0000, 0x23b1 },
    { 0x148af, 0x0000, 0x0000, 0x0000, 0x00c9 },
    { 0x148e0, 0x0200, 0x1004, 0x0000, 0x2b58 },
    { 0x148ff, 0x0400, 0x0000, 0x0000, 0x3030 },
    { 0x14919, 0x0400, 0x1805, 0x0000, 0x2cce },
    { 0x14954, 0x0000, 0x0000, 0x0000, 0x35f4 },
    { 0x14985, 0x0020, 0x0004, 0x0000, 0x2682 },
    { 0x149c6, 0x0000, 0x0000, 0x0000, 0x1075 },
    { 0x149f7, 0x0400, 0x0000, 0x0000, 0x065b },
    { 0x14a2d, 0x0000, 0x1000, 0x0004, 0x3737 },
    { 0x14a67, 0x0400, 0x1000, 0x0000, 0x389b },
    { 0x14ab9, 0x0000, 0x1000, 0x0000, 0x3f72 },
    { 0x14aef, 0x0400, 0x0801, 0x0000, 0x496f },
    { 0x14b37, 0x0400, 0x8000, 0x0000, 0x346f },
    { 0x14b72, 0x0000, 0x0000, 0x0000, 0x0065 },
    { 0x14ba3, 0x0000, 0x0000, 0x0000, 0x00c9 },
    { 0x14bd4, 0x0020, 0x1000, 0x0004, 0x0950 },
    { 0x14c12, 0x0600, 0x0000, 0x0000, 0x377b },
    { 0x14c62, 0x0400, 0x0001, 0x0001, 0x1435 },
};

/*
 * 0x14236
 *
 * Run one part's initialiser, found by its own address. Answers 1 when the
 * per-bitmap slots could not be allocated, which is what makes `make_part`
 * throw the part away.
 */
uint16_t part_init(uint32_t at, uint16_t part)
{
    int32_t i;

    for (i = 0; i < 43; i++) {
        if (part_inits[i].at != at)
            continue;

        if (part_inits[i].flags6 != 0)
            DGU16((uint16_t)(part + 6)) =
                (uint16_t)(DGU16((uint16_t)(part + 6)) | part_inits[i].flags6);
        if (part_inits[i].flags8 != 0)
            DGU16((uint16_t)(part + 8)) =
                (uint16_t)(DGU16((uint16_t)(part + 8)) | part_inits[i].flags8);
        if (part_inits[i].flags10 != 0)
            DGU16((uint16_t)(part + 0x0a)) =
                (uint16_t)(DGU16((uint16_t)(part + 0x0a))
                           | part_inits[i].flags10);

        DGU16((uint16_t)(part + 0x82)) =
            heap_calloc_far(DGU16((uint16_t)(part + 0x80)), 4);

        if (DGU16((uint16_t)(part + 0x82)) == 0)
            return 1;

        part_setup(part_inits[i].setup, part);
        return 0;
    }

    return part_init_special(at, part);
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
 * the answer is 0. The port dispatches that far pointer on its value, as it
 * does everywhere else it cannot call one.
 *
 * The heap is checked three times: before the allocation, after it, and at the
 * end.
 */
uint16_t make_part(uint16_t n)
{
    uint16_t si;
    uint16_t bx = (uint16_t)(n << 4);
    int16_t failed = 0;

    heap_check_or_hang();

    si = heap_calloc_far(1, 0xa2);
    if (si == 0) {
        failed = 1;
        goto done;
    }

    heap_check_or_hang();

    DGU16((uint16_t)(si + 4)) = n;
    DGU16((uint16_t)(si + 6)) = DGU16((uint16_t)(bx + 0x2966));
    DGU16((uint16_t)(si + 0x0a)) = DGU16((uint16_t)(bx + 0x2968));
    DGU16((uint16_t)(si + 0x50)) = DGU16((uint16_t)(bx + 0x296a));
    DGU16((uint16_t)(si + 0x52)) = DGU16((uint16_t)(bx + 0x296c));
    DGU16((uint16_t)(si + 0x44)) = DGU16((uint16_t)(bx + 0x296e));
    DGU16((uint16_t)(si + 0x46)) = DGU16((uint16_t)(bx + 0x2970));
    DGU16((uint16_t)(si + 0x80)) =
        DGU16((uint16_t)(n * 0x3a + 0x0ec4));
    DGU16((uint16_t)(si + 0x8c)) = 0xffff;
    DGU16((uint16_t)(si + 0x8e)) = 0xffff;
    DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(bx + 0x2972));

    if ((DGU16((uint16_t)(bx + 0x2972)) | DGU16((uint16_t)(bx + 0x2974))) != 0
        && call_part_init(DGU16((uint16_t)(bx + 0x2972)),
                          DGU16((uint16_t)(bx + 0x2974)), si) == 1) {
        failed = 1;
        goto done;
    }

    DGU16((uint16_t)(si + 0x94)) = DGU16((uint16_t)(si + 8));

    set_object_extent(si);

    DGU16((uint16_t)(si + 0x42)) = DGU16((uint16_t)(si + 0x46));
    DGU16((uint16_t)(si + 0x40)) = DGU16((uint16_t)(si + 0x44));

    heap_check_or_hang();

done:
    if (failed != 0) {
        if (si != 0)
            free_part(si);
        return 0;
    }

    return si;
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
void free_part(uint16_t part)
{
    if (part == 0)
        return;

    if (DGU16((uint16_t)(part + 0x82)) != 0)
        checked_free(DGU16((uint16_t)(part + 0x82)));

    if (DGU16((uint16_t)(part + 0x54)) != 0
        && (DGU16((uint16_t)(part + 8)) & 1) == 0)
        checked_free(DGU16((uint16_t)(part + 0x54)));

    if (DGU16((uint16_t)(part + 0x66)) != 0
        && (DGU16((uint16_t)(part + 4)) == 7
            || DGU16((uint16_t)(part + 4)) == 0x0a))
        checked_free(DGU16((uint16_t)(part + 0x66)));

    checked_free(part);
}

/*
 * 0x15004
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
 * goes at `x + w`, past the last middle piece.
 *
 * The text is drawn twice for a shadow: colour 0xf at `centre - 1, y + 6`,
 * then colour 5 at `centre, y + 5`. **The second call reads the string pointer
 * and increments it in the same expression** - `mov ax,[bp+6]` then
 * `inc word [bp+6]` before the push - so the light pass starts one character
 * *later* than the dark one. The shadow is a whole character wider than the
 * text it shadows, and that is what the original does.
 */
void draw_scroll_text(uint16_t str, int16_t x, int16_t y, int16_t w)
{
    uint16_t set = DGU16(0x52f4);
    int16_t  centre;
    int16_t  i;

    centre = (int16_t)(x + (w - (int16_t)text_width_thunk(str)) / 2);

    clear_flag_2d44_thunk();

    draw_bitmap(DGU16(set), x, y, 0);

    for (i = (int16_t)(x + 0x18); i < (int16_t)(x + w - 0x18);
         i = (int16_t)(i + 8))
        draw_bitmap(DGU16((uint16_t)(set + 2)), i, (int16_t)(y + 2), 0);

    draw_bitmap(DGU16((uint16_t)(set + 4)), (int16_t)(x + w), y, 0);

    DG8(0x3892) = 1;                    /* transparent: no background line */
    DG8(0x3890) = 0x0f;
    draw_string(str, (int16_t)(centre - 1), (int16_t)(y + 6));

    DG8(0x3890) = 5;
    draw_string(str + 1, centre, (int16_t)(y + 5));

    restore_cursor_following();
}

/*
 * 0x151c8
 *
 * Draw a **panel**: a tiled background inside `x,y,w,h`, a bevel around it,
 * and the ornamented border the game's menus and the copy-protection screen
 * are built out of.
 *
 * The clip box is set to the rectangle first - and `clip_bottom` to
 * `y + h - 1`, one less, where `clip_right` is `x + w` - so the tiling cannot
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
void draw_panel(int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t set = DGU16(0x52f4);
    int16_t  i, j;

    clip_left    = x;
    clip_right   = (int16_t)(x + w);
    clip_top     = y;
    clip_bottom  = (int16_t)(y + h - 1);
    clip_enabled = 1;

    clear_flag_2d44_thunk();

    for (j = 0; j < h; j = (int16_t)(j + 0x40))
        for (i = 0; i < w; i = (int16_t)(i + 0x40))
            draw_bitmap(DGU16((uint16_t)(set + 0x74)),
                        (int16_t)(x + i), (int16_t)(y + j), 0);

    if (DGU16(0x4e6b) == 0x8000)
        set_clip_full_screen();
    else
        set_clip_play_area();

    vga_second_colour = 0x0f;
    clip_and_draw_line(x, (int16_t)(y + 1), (int16_t)(x + w), (int16_t)(y + 1));
    clip_and_draw_line((int16_t)(x + w - 1), y,
              (int16_t)(x + w - 1), (int16_t)(y + h));

    vga_second_colour = 0x0e;
    clip_and_draw_line(x, y, (int16_t)(x + w), y);

    vga_second_colour = 0x06;
    clip_and_draw_line((int16_t)(x + w), y,
              (int16_t)(x + w), (int16_t)(y + h));

    for (i = (int16_t)(y + 0x13); i < (int16_t)(y + h); i = (int16_t)(i + 8))
        draw_bitmap(DGU16((uint16_t)(set + 0x1c)), (int16_t)(x - 2), i, 0);

    for (i = (int16_t)(x + 0x10); i < (int16_t)(x + w); i = (int16_t)(i + 8))
        draw_bitmap(DGU16((uint16_t)(set + 0x1e)), i,
                    (int16_t)(y + h - 4), 0);

    draw_bitmap(DGU16((uint16_t)(set + 0x14)), (int16_t)(x - 7),
                (int16_t)(y - 4), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x16)), (int16_t)(x + w - 0x10),
                (int16_t)(y - 4), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x18)), (int16_t)(x - 7),
                (int16_t)(y + h - 0x10), 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x1a)), (int16_t)(x + w - 0x13),
                (int16_t)(y + h - 0xe), 0);

    restore_cursor_following();
}

/*
 * 0x15a7e
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
void draw_odometer_digit(char c, int16_t x, int16_t y)
{
    uint8_t  digit = (uint8_t)(c + 0xd0);   /* `add al, 0xd0` is `- '0'` */
    uint16_t list  = DGU16(0x4ecd);
    int16_t  row;

    if (digit < 5) {
        row = (int16_t)(6 - (int16_t)digit * 0x15) + y;
        clear_flag_2d44_thunk();
        draw_bitmap(DGU16(list), x, row, 0);
    } else {
        digit = (uint8_t)(digit + 0xfb);    /* `add al, 0xfb` is `- 5` */
        row = (int16_t)(6 - (int16_t)digit * 0x15) + y;
        clear_flag_2d44_thunk();
        draw_bitmap(DGU16((uint16_t)(list + 2)), x, row, 0);
    }

    restore_cursor_following();
}

/*
 * 0x15af8
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
 * 0x15dfd
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
    uint16_t fp     = dg_enter(0x12);
    uint16_t digits = (uint16_t)(fp + 2);      /* [bp-0x10] */
    uint16_t part;
    int16_t  kind, count, y, text_x, text_y;

    DGU16(0x38a8) = DGU16(0x38a2);
    clip_enabled = 1;
    set_clip_play_area();
    fill_enabled = 1;
    vga_fill_colour   = DG8(0x52c9);
    vga_second_colour = DG8(0x52c9);

    clear_flag_2d44_thunk();
    fill_rect(0x241, 0x63, 0x37, 2);
    fill_rect(0x240, 0x65, 0x38, 0x103);
    restore_cursor_following();

    DG8(0x3892) = 1;                            /* transparent text */

    part = DGU16(DGU16(0x50d3));
    y    = 0x64;

    while (part != 0 && y <= 0x134) {
        uint16_t icon;

        kind = DG16((uint16_t)(part + 4));
        count = (part == DGU16(0x50d5)) ? 0 : 1;

        for (;;) {
            part = DGU16(part);
            if (part == 0)
                break;
            if (DG16((uint16_t)(part + 4)) != kind)
                break;
            if (part != DGU16(0x50d5))
                count++;
        }

        if (count == 0)
            continue;

        clear_flag_2d44_thunk();

        icon = DGU16((uint16_t)(DGU16(0x4ec7) + 2 * kind));
        draw_bitmap_centred(icon, 0x240, y, 0x38, 0x2a);

        int_to_string(count, digits, 10);
        text_x = (int16_t)(0x240 + (0x38 - (int16_t)text_width_thunk(digits)) / 2);

        text_y = (int16_t)(y + DG16((uint16_t)(icon + 8))
                           + (0x2a - DG16((uint16_t)(icon + 8))) / 2 + 1);
        if (text_y > 0x161)
            text_y = 0x161;

        DG8(0x3890) = 0;
        draw_string(digits, (int16_t)(text_x - 2), (int16_t)(text_y + 1));

        DG8(0x3890) = 0x0e;
        draw_string(digits, (int16_t)(text_x - 1), text_y);

        restore_cursor_following();

        y = (int16_t)(y + 0x34);
    }

    dg_leave(0x12);
}

/*
 * 0x15b16
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
    uint16_t set;
    int16_t  x;

    set_clip_play_area();
    DGU16(0x38a8) = DGU16(0x38a2);
    clear_flag_2d44_thunk();

    set = DGU16(0x4ecb);
    for (x = 0x10; x < 0x22f; x = (int16_t)(x + 8))
        draw_bitmap(DGU16((uint16_t)(set + 0xc)), x, 0, 0);

    draw_bitmap(DGU16(set), 0, 0, 0);
    draw_bitmap(DGU16((uint16_t)(set + 2)), 0x230, 0, 0);
    draw_bitmap(DGU16((uint16_t)(set + 0x14)), 0x238, 0, 0);

    restore_cursor_following();
}

/*
 * 0x15b9f
 *
 * The play area's **bottom edge**: the tile at +0xe laid every 8 pixels along
 * y = 0x168, then the two corners at +4 and +6 on y = 0x160 - the corners sit
 * eight pixels higher than the run they close, because they are taller.
 */
void draw_machine_layer_c(void)
{
    uint16_t set;
    int16_t  x;

    set_clip_play_area();
    DGU16(0x38a8) = DGU16(0x38a2);
    clear_flag_2d44_thunk();

    set = DGU16(0x4ecb);
    for (x = 0x10; x < 0x22f; x = (int16_t)(x + 8))
        draw_bitmap(DGU16((uint16_t)(set + 0xe)), x, 0x168, 0);

    draw_bitmap(DGU16((uint16_t)(set + 4)), 0, 0x160, 0);
    draw_bitmap(DGU16((uint16_t)(set + 6)), 0x230, 0x160, 0);

    restore_cursor_following();
}

/*
 * 0x15c13
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
    uint16_t set;
    int16_t  y;

    set_clip_play_area();
    DGU16(0x38a8) = DGU16(0x38a2);
    clear_flag_2d44_thunk();

    set = DGU16(0x4ecb);
    for (y = 8; y < 0x162; y = (int16_t)(y + 8))
        draw_bitmap(DGU16((uint16_t)(set + 8)), 0, y, 0);

    draw_bitmap(DGU16(set), 0, 0, 0);
    draw_bitmap(DGU16((uint16_t)(set + 4)), 0, 0x160, 0);

    restore_cursor_following();
}

/*
 * 0x15c83
 *
 * NOT TRANSCRIBED YET. The fifth of the five.
 */
void draw_machine_layer_e(void)
{
    not_transcribed("0x15c83");
}

/*
 * 0x15f76
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
void draw_bitmap_centred(uint16_t bmp, int16_t x, int16_t y,
                         int16_t w, int16_t h)
{
    x = (int16_t)(x + (w - DG16((uint16_t)(bmp + 6))) / 2);
    y = (int16_t)(y + (h - DG16((uint16_t)(bmp + 8))) / 2);

    draw_bitmap(bmp, x, y, 0);
}

/*
 * 0x16181
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
    uint16_t si;

    if (DGU16(0x50d5) != 0 && DG8((uint16_t)(DGU16(0x50d5) + 0x14)) != 0) {
        link_record_into_buckets(DGU16(0x50d5));
        DG8((uint16_t)(DGU16(0x50d5) + 0x14))--;
    }

    for (si = (uint16_t)pick_by_flag(0x3000); si != 0;
         si = (uint16_t)pick_for_record(si, 0x1000)) {

        if ((redraw_all != 0 || DG8((uint16_t)(si + 0x14)) != 0)
            && si != DGU16(0x50d5))
            link_record_into_buckets(si);

        if (redraw_all != 0)
            DG8((uint16_t)(si + 0x14)) = 0;
        else if (DG8((uint16_t)(si + 0x14)) != 0)
            DG8((uint16_t)(si + 0x14))--;
    }

    refile_overlapping_parts();
    draw_machine(0, 0);
}

/*
 * 0x166d6
 *
 * Clear six words at DGROUP 0x50bf. The loop counts *down* from 5 and tests
 * `jge`, so index 0 is cleared too - six entries, not five. What they hold is
 * not established.
 */
void clear_word_array_50bf(void)
{
    int16_t i = 5;

    do {
        word_array_50bf(i) = 0;
        i--;
    } while (i >= 0);
}

/*
 * 0x166ef
 *
 * Link a record into up to two buckets, and mark it linked.
 *
 * Which buckets is decided by two bytes in the record's kind entry - the same
 * 0x3a-byte table `clamp_record_pair` indexes, read here at +0x1c rather than
 * +0x0a - and a byte of 0xff means "not in this bucket". The bucket heads are
 * the six-word array at DGROUP 0x50bf, which is the array
 * `clear_word_array_50bf` zeroes; that the two routines agree about it is what
 * identifies it as a set of list heads.
 *
 * The insertion is at the head: the record's link at +0x74 (or +0x76 for the
 * second bucket) takes the old head and the head becomes the record. For the
 * first bucket only, the bucket number is also stored at +0x7f.
 *
 * One record is special - the one whose address is at DGROUP 0x50d5 always
 * goes into bucket 0 whatever its kind says.
 */
void link_record_into_buckets(uint16_t rec)
{
    int16_t kind = DG16(rec + 4);
    int16_t i;

    DG16(rec + 0x0A) |= 0x20;

    for (i = 0; i < 2; i++) {
        uint8_t slot = DG8((uint16_t)(0xEC2 + kind * 0x3A + i));

        if (slot == 0xFF)
            continue;
        if (rec == DGU16(0x50D5))
            slot = 0;

        DGU16((uint16_t)(rec + 0x74 + i * 2)) = DGU16(0x50BF + slot * 2);
        DGU16(0x50BF + slot * 2) = rec;
        if (i == 0)
            DG8(rec + 0x7F) = slot;
    }
}

/*
 * 0x143fb, 0x1443d, 0x1449d, 0x14aa2 and 0x14c48
 *
 * The five part initialisers that are not the common one. Each is small and
 * none calls into segment 172c, so there is nothing to dispatch afterwards.
 *
 * Three of them take a record of their own instead of the per-bitmap array -
 * 0x2c bytes at +0x66, or 0x38 at +0x54 - and write the part's own address into
 * it, which is the back-pointer that lets whatever walks those records get from
 * one back to its part. The other two only set flags and a couple of bytes.
 *
 * All five answer 1 when an allocation fails, which is what makes `make_part`
 * throw the part away, and 0 otherwise.
 */
uint16_t part_init_special(uint32_t at, uint16_t part)
{
    switch (at) {
    case 0x143fb:
        DGU16((uint16_t)(part + 8)) =
            (uint16_t)(DGU16((uint16_t)(part + 8)) | 4);
        DG8((uint16_t)(part + 0x6a)) = 0;
        DG8((uint16_t)(part + 0x6b)) = 8;
        DG8((uint16_t)(part + 0x6c)) = 0x0f;
        DG8((uint16_t)(part + 0x6d)) = 8;

        DGU16((uint16_t)(part + 0x66)) = heap_calloc_far(1, 0x2c);
        if (DGU16((uint16_t)(part + 0x66)) == 0)
            return 1;
        DGU16(DGU16((uint16_t)(part + 0x66))) = part;
        return 0;

    case 0x1443d:
        DGU16((uint16_t)(part + 0x54)) = heap_calloc_far(1, 0x38);
        if (DGU16((uint16_t)(part + 0x54)) == 0)
            return 1;
        DGU16((uint16_t)(DGU16((uint16_t)(part + 0x54)) + 2)) = part;
        return 0;

    case 0x1449d:
        DGU16((uint16_t)(part + 0x66)) = heap_calloc_far(1, 0x2c);
        if (DGU16((uint16_t)(part + 0x66)) == 0)
            return 1;
        DGU16(DGU16((uint16_t)(part + 0x66))) = part;
        return 0;

    case 0x14aa2:
        DGU16((uint16_t)(part + 8)) =
            (uint16_t)(DGU16((uint16_t)(part + 8)) | 0x1000);
        DGU16((uint16_t)(part + 0x0a)) =
            (uint16_t)(DGU16((uint16_t)(part + 0x0a)) | 2);
        return 0;

    case 0x14c48:
        DGU16((uint16_t)(part + 8)) =
            (uint16_t)(DGU16((uint16_t)(part + 8)) | 4);
        DG8((uint16_t)(part + 0x6a)) = 0;
        DG8((uint16_t)(part + 0x6b)) = 0;
        return 0;

    default:
        break;
    }

    {
        static char what[64];

        snprintf(what, sizeof what, "the part initialiser at %#07lx",
                 (unsigned long)at);
        not_transcribed(what);
    }
    return 1;
}

/*
 * 0x1675e
 *
 * Draw the machine: the six bucket lists, deepest first.
 *
 * The buckets are filled by `link_record_into_buckets` and each is a tree
 * walked by the byte at +0x7f exactly as `refile_overlapping_parts` walks it -
 * equal to the level takes +0x74, anything else +0x76. Every part visited has
 * bit 5 of +0x0a cleared, which is the "already in a bucket" mark, so the
 * lists are emptied by being drawn; `clear_word_array_50bf` at the end takes
 * the heads with them.
 *
 * A rope, kind 8, and a belt, kind 0x0a, each draw themselves; kind 0x31 draws
 * nothing at all. Everything else goes through the one blitter, which is told
 * the level as well, so a part in two buckets is drawn twice at two depths.
 *
 * The page being drawn into, VMDS 0x38a8, is set from 0x38a2 first, and the
 * clip is put back to whatever the mode wants.
 */
void draw_machine(int16_t a, int16_t b)
{
    uint16_t fp = dg_enter(2);
    uint16_t v02 = (uint16_t)(fp + 0);          /* [bp-2] the level */
    uint16_t v01 = (uint16_t)(fp + 1);          /* [bp-1] the counter */
    uint16_t si;

    DGU16(0x38a8) = DGU16(0x38a2);
    DG8(0x3893) = 1;
    set_clip_for_mode();

    for (DG8(v01) = 6; DG8(v01) != 0; DG8(v01)--) {
        DG8(v02) = (uint8_t)(DG8(v01) - 1);

        for (si = DGU16((uint16_t)(0x50bf + 2 * DG8(v02))); si != 0;
             si = (DG8((uint16_t)(si + 0x7f)) == DG8(v02)
                   ? DGU16((uint16_t)(si + 0x74))
                   : DGU16((uint16_t)(si + 0x76)))) {

            DGU16((uint16_t)(si + 0x0a)) &= 0xffdf;

            if (DGU16((uint16_t)(si + 4)) == 8)
                draw_rope(si, a);
            else if (DGU16((uint16_t)(si + 4)) == 0x0a)
                draw_belt(si, a);
            else if (DGU16((uint16_t)(si + 4)) != 0x31)
                draw_part(si, (int16_t)DG8(v02), a, b);
        }
    }

    clear_word_array_50bf();

    dg_leave(2);
}

/*
 * 0x167fa
 *
 * Draw a rope: two straight lines in colour 0, from the four points its record
 * keeps at +8 through +0x16. Two lines and not one because a rope over a pulley
 * has a corner in it; a rope with nothing at either end - +4 or +6 zero - draws
 * nothing at all.
 *
 * With `a` set all eight coordinates are scaled into the preview window first,
 * exactly as `draw_belt` and `draw_part` scale theirs.
 */
void draw_rope(uint16_t part, int16_t a)
{
    uint16_t fp = dg_enter(0x10);
    uint16_t p[8];
    uint16_t si = DGU16((uint16_t)(part + 0x54));
    int32_t k;

    for (k = 0; k < 8; k++)
        p[k] = (uint16_t)(fp + 0x10 - 2 * (k + 1));   /* [bp-2] .. [bp-0x10] */

    if (DGU16((uint16_t)(si + 4)) == 0 || DGU16((uint16_t)(si + 6)) == 0)
        goto out;

    clear_flag_2d44_thunk();

    for (k = 0; k < 8; k++)
        DG16(p[k]) = (int16_t)(DG16((uint16_t)(si + 8 + 2 * k))
                               - DG16((k & 1) ? 0x4ea1 : 0x4ea3));

    if (a != 0) {
        for (k = 0; k < 8; k++)
            DG16(p[k]) = (int16_t)((int16_t)long_shift_right(
                (int32_t)mul16x16(DG16(p[k]), a), 10)
                + ((k & 1) ? 0x48 : 0x110));
    }

    DG8(0x389e) = 0;

    clip_and_draw_line(DG16(p[0]), DG16(p[1]), DG16(p[2]), DG16(p[3]));
    clip_and_draw_line(DG16(p[4]), DG16(p[5]), DG16(p[6]), DG16(p[7]));

    restore_cursor_following();

out:
    dg_leave(0x10);
}

/*
 * 0x1697d
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
void draw_curve(uint8_t colour, int16_t shift,
                int32_t x0, int32_t x1, int32_t x2,
                int32_t y0, int32_t y1, int32_t y2)
{
    int32_t ddx = x0 + x2 - 2 * x1;
    int32_t ddy = y0 + y2 - 2 * y1;
    int32_t dx = (int32_t)long_shift_left((uint32_t)(x1 - x0),
                                          (uint8_t)(shift + 1));
    int32_t dy = (int32_t)long_shift_left((uint32_t)(y1 - y0),
                                          (uint8_t)(shift + 1));
    int32_t s2 = (int32_t)(int16_t)(shift << 1);
    int32_t X = (int32_t)long_shift_left((uint32_t)x0, (uint8_t)s2);
    int32_t Y = (int32_t)long_shift_left((uint32_t)y0, (uint8_t)s2);
    int32_t steps = (int32_t)(int16_t)(1 << shift);
    int16_t px = (int16_t)long_shift_right(X, (uint8_t)s2);
    int16_t py = (int16_t)long_shift_right(Y, (uint8_t)s2);
    int32_t i;

    DG8(0x389e) = colour;

    for (i = 0; i <= steps; i++) {
        int16_t sx = (int16_t)long_shift_right(X, (uint8_t)s2);
        int16_t sy = (int16_t)long_shift_right(Y, (uint8_t)s2);

        if (px != sx || py != sy) {
            clip_and_draw_line(px, py, sx, sy);
            px = sx;
            py = sy;
        }

        X += dx + (int32_t)long_multiply_2((uint32_t)ddx,
                                           (uint32_t)(2 * i + 1));
        Y += dy + (int32_t)long_multiply_2((uint32_t)ddy,
                                           (uint32_t)(2 * i + 1));
    }
}

/*
 * 0x16b39
 *
 * One length of belt between two points. Slack of four or less is a straight
 * line; anything more is a curve whose middle control point is the midpoint
 * pushed **down** by the slack, so a loose belt sags.
 */
void draw_belt_segment(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t slack)
{
    int16_t mx, my;

    if (slack <= 4) {
        clip_and_draw_line(x0, y0, x1, y1);
        return;
    }

    mx = (int16_t)((int16_t)(x0 + x1) >> 1);
    my = (int16_t)(((int16_t)(y0 + y1) >> 1) + slack);

    draw_curve(DG8(0x389e), 4, x0, mx, x1, y0, my, y1);
}

/*
 * 0x16baf
 *
 * Draw a belt: every length of it, from the part it starts at to the part it
 * ends at, following the chain of pulleys through each one's +0x5a links.
 *
 * Each end of a length is either a pulley - kind 7, whose own belt record
 * carries the tangent points at +0x14 and +0x18 - or the part the belt is
 * fastened to, whose points come from the belt record itself. The two cases
 * differ in more than the source: an end that is *not* a pulley sets the flag
 * that makes the length sag, because that is the length whose slack was
 * measured. A length between two pulleys is drawn straight.
 *
 * With `a` set every point is scaled into the preview window before drawing,
 * and the little cap bitmap that marks a fastening is left off.
 */
void draw_belt(uint16_t part, int16_t a)
{
    uint16_t fp = dg_enter(0x0e);
    uint16_t v0e = (uint16_t)(fp + 0x00);       /* [bp-0x0e] the belt */
    uint16_t v0c = (uint16_t)(fp + 0x02);       /* [bp-0x0c] the slack */
    uint16_t v0a = (uint16_t)(fp + 0x04);       /* [bp-0x0a] sags */
    uint16_t v08 = (uint16_t)(fp + 0x06);       /* [bp-8]  y1 */
    uint16_t v06 = (uint16_t)(fp + 0x08);       /* [bp-6]  x1 */
    uint16_t v04 = (uint16_t)(fp + 0x0a);       /* [bp-4]  y0 */
    uint16_t v02 = (uint16_t)(fp + 0x0c);       /* [bp-2]  x0 */
    uint16_t di, si;

    DGU16(v0e) = DGU16((uint16_t)(part + 0x66));

    di = DGU16((uint16_t)(DGU16(v0e) + 2));
    si = DGU16((uint16_t)(di + 0x5a
                          + 2 * DG8((uint16_t)(DGU16(v0e) + 0x0a))));
    if (si == 0)
        si = DGU16((uint16_t)(DGU16(v0e) + 4));

    while (di != 0 && si != 0) {
        DG16(v0a) = 0;

        if (DGU16((uint16_t)(di + 4)) == 7) {
            DG16(v02) = (int16_t)(
                DG16((uint16_t)(DGU16((uint16_t)(di + 0x66)) + 0x18))
                - DG16(0x4ea3));
            DG16(v04) = (int16_t)(
                DG16((uint16_t)(DGU16((uint16_t)(di + 0x66)) + 0x1a))
                - DG16(0x4ea1));
        } else {
            DG16(v02) = (int16_t)(DG16((uint16_t)(DGU16(v0e) + 0x14))
                                  - DG16(0x4ea3));
            DG16(v04) = (int16_t)(DG16((uint16_t)(DGU16(v0e) + 0x16))
                                  - DG16(0x4ea1));
            DG16(v0a) = 1;
        }

        if (DGU16((uint16_t)(si + 4)) == 7) {
            DG16(v06) = (int16_t)(
                DG16((uint16_t)(DGU16((uint16_t)(si + 0x66)) + 0x14))
                - DG16(0x4ea3));
            DG16(v08) = (int16_t)(
                DG16((uint16_t)(DGU16((uint16_t)(si + 0x66)) + 0x16))
                - DG16(0x4ea1));
        } else {
            DG16(v06) = (int16_t)(DG16((uint16_t)(DGU16(v0e) + 0x18))
                                  - DG16(0x4ea3));
            DG16(v08) = (int16_t)(DG16((uint16_t)(DGU16(v0e) + 0x1a))
                                  - DG16(0x4ea1));
            DG16(v0a) = 1;
        }

        if (a != 0) {
            DG16(v02) = (int16_t)((int16_t)long_shift_right(
                (int32_t)mul16x16(DG16(v02), a), 10) + 0x110);
            DG16(v04) = (int16_t)((int16_t)long_shift_right(
                (int32_t)mul16x16(DG16(v04), a), 10) + 0x48);
            DG16(v06) = (int16_t)((int16_t)long_shift_right(
                (int32_t)mul16x16(DG16(v06), a), 10) + 0x110);
            DG16(v08) = (int16_t)((int16_t)long_shift_right(
                (int32_t)mul16x16(DG16(v08), a), 10) + 0x48);
        }

        DG8(0x389e) = 6;
        clear_flag_2d44_thunk();

        if (DG16(v0a) != 0) {
            DG16(v0c) = link_slack(di, DGU16(v0e), 3);
            draw_belt_segment(DG16(v02), DG16(v04), DG16(v06), DG16(v08),
                              DG16(v0c));
        } else {
            clip_and_draw_line(DG16(v02), DG16(v04), DG16(v06), DG16(v08));
        }

        if (a == 0) {
            if (DGU16((uint16_t)(di + 4)) != 0x31
                && DGU16((uint16_t)(di + 4)) != 7)
                draw_bitmap(DGU16((uint16_t)(DGU16(0x4ecb) + 0x48)),
                            (int16_t)(DG16(v02) - 5),
                            (int16_t)(DG16(v04) - 2), 0);

            if (DGU16((uint16_t)(si + 4)) != 0x31
                && DGU16((uint16_t)(si + 4)) != 7)
                draw_bitmap(DGU16((uint16_t)(DGU16(0x4ecb) + 0x48)),
                            (int16_t)(DG16(v06) - 5),
                            (int16_t)(DG16(v08) - 2), 0);
        }

        restore_cursor_following();

        di = si;
        if (DGU16((uint16_t)(di + 4)) == 7)
            si = DGU16((uint16_t)(si + 0x5a));
        else
            si = 0;
    }

    dg_leave(0x0e);
}

/*
 * 0x16db1
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
void draw_part(uint16_t part, int16_t level, int16_t a, int16_t b)
{
    uint16_t fp = dg_enter(0x2a);
    uint16_t v2a = (uint16_t)(fp + 0x00);   /* [bp-0x2a] the bitmap */
    uint16_t v28 = (uint16_t)(fp + 0x02);   /* [bp-0x28] the record */
    uint16_t v26 = (uint16_t)(fp + 0x04);   /* [bp-0x26] the kind's record */
    uint16_t v24 = (uint16_t)(fp + 0x06);   /* [bp-0x24] the adjustment */
    uint16_t v21 = (uint16_t)(fp + 0x09);   /* [bp-0x21] the frame */
    uint16_t v20 = (uint16_t)(fp + 0x0a);   /* [bp-0x20] py */
    uint16_t v1e = (uint16_t)(fp + 0x0c);   /* [bp-0x1e] px */
    uint16_t v1c = (uint16_t)(fp + 0x0e);   /* [bp-0x1c] the bitmap index */
    uint16_t v1a = (uint16_t)(fp + 0x10);   /* [bp-0x1a] the mirror flags */
    uint16_t v18 = (uint16_t)(fp + 0x12);   /* [bp-0x18] */
    uint16_t v16 = (uint16_t)(fp + 0x14);   /* [bp-0x16] rows */
    uint16_t v14 = (uint16_t)(fp + 0x16);   /* [bp-0x14] columns */
    uint16_t v12 = (uint16_t)(fp + 0x18);   /* [bp-0x12] */
    uint16_t v10 = (uint16_t)(fp + 0x1a);   /* [bp-0x10] */
    uint16_t v0e = (uint16_t)(fp + 0x1c);   /* [bp-0x0e] */
    uint16_t v0c = (uint16_t)(fp + 0x1e);   /* [bp-0x0c] */
    uint16_t v0a = (uint16_t)(fp + 0x20);   /* [bp-0x0a] y */
    uint16_t v08 = (uint16_t)(fp + 0x22);   /* [bp-8] x */
    uint16_t v06 = (uint16_t)(fp + 0x24);   /* [bp-6] the column */
    uint16_t v04 = (uint16_t)(fp + 0x26);   /* [bp-4] the form */
    uint16_t v02 = (uint16_t)(fp + 0x28);   /* [bp-2] the kind */
    uint16_t si = part;
    int16_t di;

    DGU16(v02) = DGU16((uint16_t)(si + 4));
    DGU16(v04) = DGU16((uint16_t)(si + 0x0c));
    DGU16(v26) = (uint16_t)(0x0ea6 + 0x3a * (int16_t)DG16(v02));

    DGU16(v24) = DGU16((uint16_t)(DGU16(v26) + 0x18));
    if (DGU16(v24) != 0)
        DGU16(v24) = (uint16_t)(DGU16(v24) + 2 * DGU16(v04));

    clear_flag_2d44_thunk();

    if (DGU16((uint16_t)(si + 6)) & 0x40) {
        DG16(v14) = (int16_t)(DG16((uint16_t)(si + 0x44)) >> 4);
        DG16(v16) = (int16_t)(DG16((uint16_t)(si + 0x46)) >> 4);

        DG16(v18) = (int16_t)(DG16((uint16_t)(si + 0x1e)) - DG16(0x4ea3));
        DG16(v0a) = (int16_t)(DG16((uint16_t)(si + 0x20)) - DG16(0x4ea1));

        if (DGU16(v24) != 0) {
            DG16(v18) = (int16_t)(DG16(v18) + (int8_t)DG8(DGU16(v24)));
            DG16(v0a) = (int8_t)DG8((uint16_t)(DGU16(v24) + 1));
        }

        DG16(v1e) = (int16_t)((DG16(v18) & 0x10) >> 4);
        DG16(v20) = (int16_t)((DG16(v0a) & 0x10) >> 4);
        DGU16(v1c) = DGU16(v04);

        for (di = 0; di < DG16(v16); di++,
             DG16(v0a) = (int16_t)(DG16(v0a) + 0x10),
             DG16(v20) ^= 1) {

            for (DG16(v06) = 0, DG16(v08) = DG16(v18);
                 DG16(v06) < DG16(v14);
                 DG16(v06)++,
                 DG16(v08) = (int16_t)(DG16(v08) + 0x10),
                 DG16(v1e) ^= 1) {

                if (DG16(v16) == 1) {
                    if (DG16(v06) == 0)
                        DGU16(v1c) = DGU16(v04);
                    else if ((int16_t)(DG16(v14) - 1) == DG16(v06))
                        DGU16(v1c) = (uint16_t)(DGU16(v04) + 3);
                    else
                        DGU16(v1c) = (uint16_t)(DGU16(v04) + DGU16(v1e) + 1);
                } else if (DG16(v14) == 1) {
                    if (di == 0)
                        DGU16(v1c) = (uint16_t)(DGU16(v04) + 4);
                    else if ((int16_t)(DG16(v16) - 1) == di)
                        DGU16(v1c) = (uint16_t)(DGU16(v04) + 7);
                    else
                        DGU16(v1c) = (uint16_t)(DGU16(v04) + DGU16(v20) + 5);
                }

                {
                    uint16_t bmp = DGU16((uint16_t)(
                        DGU16((uint16_t)(DGU16(v26) + 0x14)) + 2 * DGU16(v1c)));

                    if (a != 0) {
                        DG16(v0c) = (int16_t)long_shift_right(
                            (int32_t)mul16x16(0x10, b), 10);
                        DG16(v0e) = (int16_t)long_shift_right(
                            (int32_t)mul16x16(0x10, b), 10);
                        DG16(v10) = (int16_t)((int16_t)long_shift_right(
                            (int32_t)mul16x16(DG16(v08), a), 10) + 0x110);
                        DG16(v12) = (int16_t)((int16_t)long_shift_right(
                            (int32_t)mul16x16(DG16(v0a), a), 10) + 0x48);

                        draw_bitmap_scaled(bmp, DG16(v10), DG16(v12),
                                           DG16(v0c), DG16(v0e), 0);
                    } else {
                        draw_bitmap(bmp, DG16(v08), DG16(v0a), 0);
                    }
                }
            }
        }

        goto done;
    }

    if (DGU16((uint16_t)(si + 8)) & 0x1000) {
        DGU16(v28) = DGU16((uint16_t)(
            DGU16((uint16_t)(DGU16(v26) + 0x16)) + 2 * DGU16(v04)));
    } else {
        DGU16(v28) = 0x124;
        DG8(0x127) = (uint8_t)DGU16(v04);
        DG8(0x126) = (uint8_t)level;

        if (DGU16(v24) != 0) {
            DG8(0x12b) = DG8(DGU16(v24));
            DG8(0x12c) = DG8((uint16_t)(DGU16(v24) + 1));
        } else {
            DG8(0x12c) = 0;
            DG8(0x12b) = 0;
        }
    }

    while (DGU16(v28) != 0) {
        if (DG8((uint16_t)(DGU16(v28) + 2)) != (uint8_t)level
            && si != DGU16(0x50d5))
            goto next;

        DG8(v21) = DG8((uint16_t)(DGU16(v28) + 3));

        for (di = 0; ; di++) {
            DGU16(v2a) = DGU16((uint16_t)(
                DGU16((uint16_t)(DGU16(v26) + 0x14)) + 2 * DG8(v21)));

            DG16(v08) = (int16_t)(DG16((uint16_t)(si + 0x1e)) - DG16(0x4ea3));
            DG16(v0a) = (int16_t)(DG16((uint16_t)(si + 0x20)) - DG16(0x4ea1));

            if (DGU16((uint16_t)(si + 8)) & 0x10) {
                DG16(v08) = (int16_t)(
                    DG16(v08)
                    + (DG16((uint16_t)(si + 0x40))
                       - (int8_t)DG8((uint16_t)(DGU16(v28) + 2 * di + 7))
                       - DG16((uint16_t)(DGU16(v2a) + 6))));
                DGU16(v1a) = 2;
            } else {
                DG16(v08) = (int16_t)(
                    DG16(v08)
                    + (int8_t)DG8((uint16_t)(DGU16(v28) + 2 * di + 7)));
                DGU16(v1a) = 0;
            }

            if (DGU16((uint16_t)(si + 8)) & 0x20) {
                DG16(v0a) = (int16_t)(
                    DG16(v0a)
                    + (DG16((uint16_t)(si + 0x42))
                       - (int8_t)DG8((uint16_t)(DGU16(v28) + 2 * di + 8))
                       - DG16((uint16_t)(DGU16(v2a) + 8))));
                DGU16(v1a) |= 1;
            } else {
                DG16(v0a) = (int16_t)(
                    DG16(v0a)
                    + (int8_t)DG8((uint16_t)(DGU16(v28) + 2 * di + 8)));
            }

            if (a != 0) {
                DG16(v0c) = (int16_t)long_shift_right(
                    (int32_t)mul16x16(DG16((uint16_t)(DGU16(v2a) + 6)), b), 10);
                DG16(v0e) = (int16_t)long_shift_right(
                    (int32_t)mul16x16(DG16((uint16_t)(DGU16(v2a) + 8)), b), 10);
                DG16(v10) = (int16_t)((int16_t)long_shift_right(
                    (int32_t)mul16x16(DG16(v08), a), 10) + 0x110);
                DG16(v12) = (int16_t)((int16_t)long_shift_right(
                    (int32_t)mul16x16(DG16(v0a), a), 10) + 0x48);

                draw_bitmap_scaled(DGU16(v2a), DG16(v10), DG16(v12),
                                   DG16(v0c), DG16(v0e), DGU16(v1a));
            } else {
                draw_bitmap(DGU16(v2a), DG16(v08), DG16(v0a), DGU16(v1a));
            }

            DG8(v21) = DG8((uint16_t)(DGU16(v28) + di + 4));

            if (di + 1 >= 4 || DG8(v21) == 0xff)
                break;
        }

    next:
        DGU16(v28) = DGU16(DGU16(v28));
    }

done:
    if (DG16(0x4e6b) == 0x2000 && DGU16((uint16_t)(si + 4)) == 0x1e)
        draw_part_extra(si);

    restore_cursor_following();

    dg_leave(0x2a);
}

/*
 * 0x171b5
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
void draw_part_extra(uint16_t part)
{
    uint16_t fp = dg_enter(0x14);
    uint16_t v14 = (uint16_t)(fp + 0x00);       /* [bp-0x14] the size, x */
    uint16_t v12 = (uint16_t)(fp + 0x02);       /* [bp-0x12] the size, y */
    uint16_t v10 = (uint16_t)(fp + 0x04);       /* [bp-0x10] the corner, x */
    uint16_t v0e = (uint16_t)(fp + 0x06);       /* [bp-0x0e] the corner, y */
    uint16_t v0c = (uint16_t)(fp + 0x08);       /* [bp-0x0c] y[0] */
    uint16_t v0a = (uint16_t)(fp + 0x0a);       /* [bp-0x0a] y[1] */
    uint16_t v08 = (uint16_t)(fp + 0x0c);       /* [bp-8]    y[2] */
    uint16_t v06 = (uint16_t)(fp + 0x0e);       /* [bp-6]    x[0] */
    uint16_t v04 = (uint16_t)(fp + 0x10);       /* [bp-4]    x[1] */
    uint16_t v02 = (uint16_t)(fp + 0x12);       /* [bp-2]    x[2] */
    uint16_t si = part;
    uint16_t di = DGU16((uint16_t)(si + 0x62));
    int16_t edge;

    if (di == 0)
        goto out;

    DG8(0x389d) = 0x0e;
    DG8(0x389e) = 0x0e;

    DG16(v04) = (int16_t)(DG16((uint16_t)(di + 0x1e))
                          + DG8((uint16_t)(di + 0x72)) - DG16(0x4ea3));
    DG16(v0c) = (int16_t)(DG16((uint16_t)(si + 0x20)) + 6 - DG16(0x4ea1));
    DG16(v0a) = (int16_t)(DG16((uint16_t)(di + 0x20))
                          + DG8((uint16_t)(di + 0x73)) - DG16(0x4ea1));
    DG16(v08) = (int16_t)(DG16((uint16_t)(si + 0x20)) + 0x10 - DG16(0x4ea1));

    if (DGU16((uint16_t)(si + 8)) & 0x10)
        edge = (int16_t)(DG16((uint16_t)(si + 0x1e)) - 1);
    else
        edge = (int16_t)(DG16((uint16_t)(si + 0x1e)) + 0x0f);

    edge = (int16_t)(edge - DG16(0x4ea3));
    DG16(v02) = edge;
    DG16(v06) = edge;

    draw_polygon(3, v06, v0c);

    if (DG16(v06) < DG16(v04)) {
        DG16(v10) = DG16(v06);
        DG16(v14) = (int16_t)(DG16(v04) - DG16(v06));
    } else {
        DG16(v10) = DG16(v04);
        DG16(v14) = (int16_t)(DG16(v06) - DG16(v04));
    }
    DG16(v14)++;

    DG16(v0e) = DG16(v0c) < DG16(v0a) ? DG16(v0c) : DG16(v0a);

    DG16(v12) = (int16_t)((DG16(v08) >= DG16(v0a) ? DG16(v08) : DG16(v0a))
                          - DG16(v0e) + 1);

    DG16(v10) = (int16_t)(DG16(v10) + DG16(0x4e9f));
    DG16(v0e) = (int16_t)(DG16(v0e) + DG16(0x4e9d));

    alloc_shape(v10, v14, 1, 2, 0);

out:
    dg_leave(0x14);
}
