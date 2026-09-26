/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Drawing the machine**: its five layers, the ropes, belts and
 * curves between parts, the part being carried, and the panels, buttons and
 * odometer around it. Part allocation lives here too, next to the drawing
 * that depends on it.
 *
 * This file corresponds to the original's **code segment 14de**, image
 * 0x14de0..0x172c0 - it ends with a routine that does nothing, 0x172bc, called
 * as 14de:24dc. Functions are in address order and each carries the image
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

/*
 * **Where Tab sends the pointer on a message box's two buttons**, DGROUP 0x259c..0x25a2, 0x06 bytes: which
 * stop it is on - 0xffff until the first Tab, and back to 0 past the last
 * - and the x of each, the y being fixed.
 */
struct game_message_tabs {
    uint16_t  stop;          /* +0x00 [2]  which of the message box's buttons the tab key is on */
    int16_t   stop_x[2];          /* +0x02 [4]  their x; the y is always 0xde. 232 and 360 in the image */
} PACKED;

struct game_message_tabs GAME_MESSAGE_TABS DGROUP_AT(0x259c) = { .stop = 0xffff, .stop_x = { 0x00e8, 0x0168 } };

/*
 * **The menu strip's animation tables**, DGROUP 0x25a2..0x25d6, 0x34 bytes, as
 * `draw_machine_layer_f` reads them: by frame, which of the menu bitmaps to
 * draw and where; and for frames past the fourth, where the four-frame
 * sprite goes. The names are ours; the extents are the routine's bounds
 * and the run ends exactly at 0x25d6.
 */
struct machine_draw_menu_anim {
    uint16_t  picture[6];         /* +0x00 [0xc]  a bitmap index in menu_bmp_ptr's set */
    int16_t   picture_x[6];       /* +0x0c [0xc] */
    int16_t   picture_y[6];       /* +0x18 [0xc] */
    int16_t   sprite_x[4];        /* +0x24 [8]  by the frame modulo four */
    int16_t   sprite_y[4];        /* +0x2c [8] */
} PACKED;

struct machine_draw_menu_anim MACHINE_DRAW_MENU_ANIM DGROUP_AT(0x25a2) = {
    .picture = { 0x0003, 0x0004, 0x0005, 0x0006, 0x0003, 0x0003 },
    .picture_x = { 0x0258, 0x0254, 0x0254, 0x0254, 0x0260, 0x0265 },
    .picture_y = { 0x0013, 0x0010, 0x000f, 0x0013, 0x0013, 0x0013 },
    .sprite_x = { 0x0250, 0x0252, 0x0250, 0x0251 },
    .sprite_y = { 0x001a, 0x0018, 0x001b, 0x0019 },
};

/*
 * **The selection box's animation phase**, DGROUP 0x25d6..0x25d8, 0x02 bytes:
 * 0 to 3 and back, stepped once per `draw_part_selection` and turned into the
 * marching-ants offset.
 */
struct machine_draw_selection_phase {
    uint16_t  phase;          /* +0x00 [2] */
} PACKED;

struct machine_draw_selection_phase MACHINE_DRAW_SELECTION_PHASE DGROUP_AT(0x25d6);


/*
 * 0x14236 .. 0x14d42 - the **part initialisers**, fifty-one routines.
 *
 * The table of part kinds at DGROUP 0x2966 carries one far pointer each, at
 * +0x0c, and `make_part` calls it through `call_part_init`. Fifty-eight kind
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

/*
 * OURS: reach one part initialiser by its image address.
 *
 * The original has no such routine. Each kind's initialiser is called through
 * the relocated far pointer at +0x0c of its entry in the table at DGROUP
 * 0x2966, and the port has no way to call one - so `call_part_init` in io.c
 * turns the pointer back into an image address and this turns that address
 * into a call. It is the same stand-in as `part_setup` and `part_finish`.
 *
 * An address with no case **aborts**: a part built by nothing at all would
 * surface much later as a level that cannot be solved.
 */
uint16_t part_init(uint32_t at, struct part *part)
{
    switch (at) {
    case 0x14236: return part_init_bowling_ball(part);
    case 0x14267: return part_init_14267(part);
    case 0x142a1: return part_init_ramp(part);
    case 0x142e6: return part_init_seesaw(part);
    case 0x14320: return part_init_balloon(part);
    case 0x14361: return part_init_conveyor(part);
    case 0x143b3: return part_init_mouse_cage(part);
    case 0x143fb: return part_init_pulley(part);
    case 0x1443d: return part_init_belt(part);
    case 0x1446c: return part_init_basketball(part);
    case 0x1449d: return part_init_rope(part);
    case 0x144cb: return part_init_bird_cage(part);
    case 0x1450c: return part_init_pokey(part);
    case 0x14547: return part_init_jack_in_the_box(part);
    case 0x1458f: return part_init_gear(part);
    case 0x145d1: return part_init_bob_the_fish(part);
    case 0x14607: return part_init_bellow(part);
    case 0x1463d: return part_init_bucket(part);
    case 0x1467e: return part_init_cannon(part);
    case 0x146bd: return part_init_dynamite(part);
    case 0x146fc: return part_init_146fc(part);
    case 0x1472d: return part_init_electric_plug(part);
    case 0x1476c: return part_init_dynamite_plunger(part);
    case 0x147a7: return part_init_hook(part);
    case 0x147c5: return part_init_fan(part);
    case 0x14804: return part_init_flashlight(part);
    case 0x1483a: return part_init_generator(part);
    case 0x14874: return part_init_gun(part);
    case 0x148af: return part_init_baseball(part);
    case 0x148e0: return part_init_light(part);
    case 0x148ff: return part_init_magnifying_glass(part);
    case 0x14919: return part_init_monkey(part);
    case 0x14954: return part_init_pumpkin(part);
    case 0x14985: return part_init_heart_balloon(part);
    case 0x149c6: return part_init_christmas_tree(part);
    case 0x149f7: return part_init_boxing_glove(part);
    case 0x14a2d: return part_init_rocket(part);
    case 0x14a67: return part_init_scissors(part);
    case 0x14aa2: return part_init_solar_panel(part);
    case 0x14ab9: return part_init_trampoline(part);
    case 0x14aef: return part_init_windmill(part);
    case 0x14b37: return part_init_mort_the_mouse(part);
    case 0x14b72: return part_init_cannon_ball(part);
    case 0x14ba3: return part_init_tennis_ball(part);
    case 0x14bd4: return part_init_candle(part);
    case 0x14c12: return part_init_corner_pipe(part);
    case 0x14c48: return part_init_14c48(part);
    case 0x14c62: return part_init_motor(part);
    case 0x14ca0: return part_init_14ca0(part);
    case 0x14cd9: return part_init_14cd9(part);
    case 0x14d0a: return part_init_14d0a(part);

    default:
        break;
    }

    {
        static char what[64];

        io_format(what, sizeof what, "the part initialiser at %#07lx",
                 (unsigned long)at);
        not_transcribed(what);
    }
    return 1;
}

/*
 * 0x14dec
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
void draw_title_bar(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                    uint16_t filled)
{
    dg_near_t set = DG4E67.bmp_4ecb_ptr;
    int16_t  x, y;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    VMDS.clip_enabled = 0;
    VMDS.fill_enabled = 1;
    VMDS.fill_colour   = 0;
    VMDS.second_colour = 0;

    cursor_redraw_off_thunk();

    if (filled != 0) {
        fill_rect((int16_t)(x1 - 0x0c), (int16_t)(y1 + 0x0c),
                  (int16_t)(x2 - x1), (int16_t)(y2 - y1));
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x25]),
                    (int16_t)(x1 - 0x0f), (int16_t)(y1 + 7), 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x26]),
                    (int16_t)(x1 - 0x0f), (int16_t)(y2 - 9), 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x27]),
                    (int16_t)(x2 - 0x20), (int16_t)(y2 - 9), 0);
    }

    VMDS.clip_left    = x1;
    VMDS.clip_right   = x2;
    VMDS.clip_top     = y1;
    VMDS.clip_bottom  = y2;
    VMDS.clip_enabled = 1;

    for (y = y1; y < y2; y = (int16_t)(y + 0x40))
        for (x = x1; x < x2; x = (int16_t)(x + 0x80))
            draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x2a]), x, y, 0);

    if (DG4E67.state == 0x8000)
        set_clip_full_screen();
    else
        set_clip_play_area();

    VMDS.clip_enabled = 0;

    for (x = x1; x < x2; x = (int16_t)(x + 8)) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x12]), x, (int16_t)(y1 - 4), 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x13]), x, y2, 0);
    }

    for (y = y1; y < y2; y = (int16_t)(y + 8)) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x10]), (int16_t)(x1 - 4), y, 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x11]), x2, y, 0);
    }

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xc]),
                (int16_t)(x1 - 7), (int16_t)(y1 - 7), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xd]),
                (int16_t)(x2 - 0x11), (int16_t)(y1 - 7), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xe]),
                (int16_t)(x1 - 7), (int16_t)(y2 - 0x11), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xf]),
                (int16_t)(x2 - 0x11), (int16_t)(y2 - 0x11), 0);
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
void draw_scroll_text(const char *str, int16_t x, int16_t y, int16_t w)
{
    dg_near_t set = DG52ED.panel_art_ptr;
    int16_t  centre;
    int16_t  i;

    centre = (int16_t)(x + (w - (int16_t)text_width_thunk(str)) / 2);

    cursor_redraw_off_thunk();

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0]), x, y, 0);

    for (i = (int16_t)(x + 0x18); i < (int16_t)(x + w - 0x18);
         i = (int16_t)(i + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x1]), i, (int16_t)(y + 2), 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x2]),
                (int16_t)(x + w - 0x18), y, 0);

    VMDS.text_style = 1;                    /* transparent: no background line */
    VMDS.text_colour = 0x0f;
    draw_string(str, (int16_t)(centre - 1), (int16_t)(y + 6));

    VMDS.text_colour = 5;
    draw_string(str, centre, (int16_t)(y + 5));

    restore_cursor_following();
}

/*
 * 0x150db
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
void draw_button(const char *str, uint16_t x, uint16_t y, uint16_t pressed)
{
    dg_near_t set = DG52ED.panel_art_ptr;
    int16_t  w, rounded, right, text_off, i;

    w = (int16_t)text_width_thunk(str);
    rounded = (int16_t)((w + 7) & 0xfff8);
    right = (int16_t)(x + rounded + 8);
    text_off = (int16_t)(((rounded - w) >> 1) + 8);

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[pressed + 0x2c]),
                (int16_t)x, (int16_t)y, 0);

    for (i = (int16_t)(x + 8); i < right; i = (int16_t)(i + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[pressed + 0x2e]),
                    i, (int16_t)y, 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[pressed + 0x30]),
                right, (int16_t)y, 0);

    VMDS.text_style = 1;            /* transparent: no background line */
    VMDS.text_colour = 5;
    draw_string(str,
                (int16_t)(x + text_off - (int16_t)pressed),
                (int16_t)(y + 2 * (int16_t)pressed + 4));

    restore_cursor_following();
}

/*
 * 0x151c8
 *
 * Draw a **panel**: a tiled background inside `x,y,w,h`, a bevel around it,
 * and the ornamented border the game's menus and the copy-protection screen
 * are built out of.
 *
 * The clip box is set to the rectangle first - and `VMDS.clip_bottom` to
 * `y + h - 1`, one less, where `VMDS.clip_right` is `x + w` - so the tiling cannot
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
    dg_near_t set = DG52ED.panel_art_ptr;
    int16_t  i, j;

    VMDS.clip_left    = x;
    VMDS.clip_right   = (int16_t)(x + w);
    VMDS.clip_top     = y;
    VMDS.clip_bottom  = (int16_t)(y + h - 1);
    VMDS.clip_enabled = 1;

    cursor_redraw_off_thunk();

    for (j = 0; j < h; j = (int16_t)(j + 0x40))
        for (i = 0; i < w; i = (int16_t)(i + 0x40))
            draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x3a]),
                        (int16_t)(x + i), (int16_t)(y + j), 0);

    if (DG4E67.state == 0x8000)
        set_clip_full_screen();
    else
        set_clip_play_area();

    VMDS.second_colour = 0x0f;
    clip_and_draw_line(x, (int16_t)(y + 1), (int16_t)(x + w), (int16_t)(y + 1));
    clip_and_draw_line((int16_t)(x + w - 1), y,
              (int16_t)(x + w - 1), (int16_t)(y + h));

    VMDS.second_colour = 0x0e;
    clip_and_draw_line(x, y, (int16_t)(x + w), y);

    VMDS.second_colour = 0x06;
    clip_and_draw_line((int16_t)(x + w), y,
              (int16_t)(x + w), (int16_t)(y + h));

    for (i = (int16_t)(y + 0x13); i < (int16_t)(y + h); i = (int16_t)(i + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xe]), (int16_t)(x - 2), i, 0);

    for (i = (int16_t)(x + 0x10); i < (int16_t)(x + w); i = (int16_t)(i + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xf]), i,
                    (int16_t)(y + h - 4), 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xa]), (int16_t)(x - 7),
                (int16_t)(y - 4), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xb]), (int16_t)(x + w - 0x10),
                (int16_t)(y - 4), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xc]), (int16_t)(x - 7),
                (int16_t)(y + h - 0x10), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xd]), (int16_t)(x + w - 0x13),
                (int16_t)(y + h - 0xe), 0);

    restore_cursor_following();
}

/*
 * 0x153b8
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
void draw_sunken_box(int16_t x, int16_t y, int16_t w, int16_t h)
{
    dg_near_t set = DG52ED.panel_art_ptr;
    int16_t  i, j;

    set_clip_play_area();

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    for (j = 8; (int16_t)(h - 8) > j; j = (int16_t)(j + 8)) {
        for (i = 8; (int16_t)(w - 8) > i; i = (int16_t)(i + 8))
            draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x2b]),
                        (int16_t)(i + x), (int16_t)(j + y), 0);

        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x36]), x, (int16_t)(j + y), 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x37]),
                    (int16_t)(x + w - 8), (int16_t)(j + y), 0);
    }

    for (i = 8; (int16_t)(w - 8) > i; i = (int16_t)(i + 8)) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x38]), (int16_t)(i + x), y, 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x39]),
                    (int16_t)(i + x), (int16_t)(y + h - 8), 0);
    }

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x32]), x, y, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x33]), (int16_t)(x + w - 0x10), y, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x34]), x, (int16_t)(y + h - 0x10), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x35]), (int16_t)(x + w - 0x10),
                (int16_t)(y + h - 0x10), 0);

    restore_cursor_following();
}

/*
 * 0x15523
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
void fill_panel_area(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t colour)
{
    dg_near_t set = DG4E67.bmp_4ecb_ptr;
    int16_t  x2  = (int16_t)(x + w);
    int16_t  y2  = (int16_t)(y + h);
    int16_t  n;

    cursor_redraw_off_thunk();
    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    VMDS.fill_colour = (uint8_t)colour;
    VMDS.second_colour = (uint8_t)colour;

    fill_rect(x, y, w, h);

    for (n = x; n < x2; n = (int16_t)(n + 8)) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x1a]), n, (int16_t)(y - 8), 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x1b]), n, y2, 0);
    }

    for (n = y; n < y2; n = (int16_t)(n + 8)) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x18]), (int16_t)(x - 8), n, 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x19]), x2, n, 0);
    }

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x14]),
                (int16_t)(x - 8), (int16_t)(y - 8), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x15]),
                (int16_t)(x2 - 8), (int16_t)(y - 8), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x16]),
                (int16_t)(x - 8), (int16_t)(y2 - 5), 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x17]),
                (int16_t)(x2 - 8), (int16_t)(y2 - 8), 0);
}

/*
 * 0x15661
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
    message_box(title, body, GAME_BUTTON_LABELS.continue_btn, NULL);
}

/*
 * 0x1567b
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
    return message_box(title, body, GAME_BUTTON_LABELS.yes, GAME_BUTTON_LABELS.no);
}

/*
 * 0x15698
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
                     const char *button1, const char *button2)
{
    uint16_t saved;
    int16_t  second_x = 0;

    wait_cursor();

    saved = DG4E67.state;
    DG4E67.state = 0x8000;

    draw_title_bar(0xb0, 0x70, 0x190, 0xf8, 1);
    draw_scroll_text(title, 0xb8, 0x74, 0xd0);
    draw_panel(0xb8, 0x90, 0xd0, 0x5a);
    draw_wrapped_text(body, 0xbc, 0x94, 0xc8, 0x30);

    draw_button(button1, 0xc8, 0xd4, 0);
    REGION_PTR(DG4E67.region_kept_b_ptr)->x1 =
        (uint16_t)(text_width_thunk(button1) + 0xd8);

    if (button2 != NULL) {
        second_x = (int16_t)(0x168
                             - ((text_width_thunk(button2) + 7) & 0xfff8));
        draw_button(button2, (uint16_t)second_x, 0xd4, 0);
        REGION_PTR(DG4E67.region_kept_a_ptr)->x0 = second_x;
    }

    present_back_page();
    restore_cursor();

    while (DG4E67.state == 0x8000) {
        update_button_state();

        DG52ED.last_key = (uint8_t)(bios_read_key() >> 8);

        if ((DG52ED.last_key) == SC_TAB) {
            message_box_tab(button2);
        } else {
            if (*button1 == 'Y') {
                if ((DG52ED.last_key) == SC_Y)
                    DG4E67.state = 0x4000;
                if ((DG52ED.last_key) == SC_N)
                    DG4E67.state = 0x2000;
            }
            if (*button1 == 'R') {
                if ((DG52ED.last_key) == SC_R)
                    DG4E67.state = 0x4000;
                if ((DG52ED.last_key) == SC_A)
                    DG4E67.state = 0x2000;
            }
            if (*button1 == 'C') {
                if ((DG52ED.last_key) == SC_C)
                    DG4E67.state = 0x4000;
                if ((DG52ED.last_key) == SC_ENTER)
                    DG4E67.state = 0x4000;
            }
        }

        regions_handle_pointer(DG4E67.regions_b_ptr);

        if (button2 == NULL && DG4E67.state == 0x2000)
            DG4E67.state = 0x8000;

        present_frame(1);
    }

    update_button_state();

    if (DG4E67.state == 0x4000) {
        draw_button(button1, 0xc8, 0xd4, 1);
        present_back_page();
        DG4E67.state = saved;
        return 1;
    }

    if (button2 != NULL) {
        draw_button(button2, (uint16_t)second_x, 0xd4, 1);
        present_back_page();
    }
    DG4E67.state = saved;
    return 0;
}

/*
 * 0x1588c
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
    GAME_MESSAGE_TABS.stop++;

    if (button2 != NULL) {
        if (GAME_MESSAGE_TABS.stop == 2)
            GAME_MESSAGE_TABS.stop = 0;
    } else {
        GAME_MESSAGE_TABS.stop = 0;
    }

    move_pointer_to(GAME_MESSAGE_TABS.stop_x[GAME_MESSAGE_TABS.stop],
                    0xde);
}

/*
 * 0x158c5
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
    char code[40];                    /* [bp-0x6c], password and code */
    char bonus[30]; /* [bp-0x44], the second line */
    char line[30]; /* [bp-0x26], the first line */
    char num[8]; /* [bp-8],    a number as text */

    repaint_whole_screen();

    string_copy(line, DG1BCC.puzzle_prefix);
    int_to_string(DG4E67.round_number, num, 0xa);
    string_concat(line, num);
    string_concat(line, DG1BCC.completed);

    string_copy(bonus, DG1BCC.total_bonus_points);
    int_to_string((int16_t)(DG50AF.bonus_1 + DG50AF.bonus_2), num, 0xa);
    string_concat(bonus, num);

    draw_title_bar(0xb0, 0x70, 0x190, 0xf8, 1);
    draw_scroll_text(line,  0xb8, 0x80, 0xd0);
    draw_scroll_text(bonus, 0xb8, 0x9c, 0xd0);

    if (DG4E67.round_number < DG4E67.level_count) {
        draw_scroll_text(DG1BCC.new_password, 0xb8, 0xc4, 0xd0);

        read_password_line(DG4E67.round_number, code);
        score_to_code(DG4E67.counter,
                      code);

        draw_scroll_text(code, 0xb8, 0xd8, 0xd0);
    }

    cursor_redraw_off_thunk();

    VMDS.text_colour = 0;
    draw_string(DG1BCC.click_button_to_continue, 0xd3, 0xee);

    VMDS.text_colour = 0x0f;
    draw_string(DG1BCC.click_button_to_continue, 0xd4, 0xed);

    restore_cursor_following();
    present_back_page();
}

/*
 * 0x15a2f
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
    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    VMDS.fill_colour = ((uint8_t)DG52BD.fill_colour);
    VMDS.second_colour = ((uint8_t)DG52BD.fill_colour);
    VMDS.fill_enabled = 1;
    VMDS.clip_enabled = 0;

    cursor_redraw_off_thunk();
    fill_rect(8, 8, 0x230, 0x160);
    draw_machine_thunk();
    step_and_draw_machine(1);
    present_back_page();
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
    uint16_t list  = DG4E67.score2_bmp_ptr;
    int16_t  row;

    if (digit < 5) {
        row = (int16_t)(6 - (int16_t)digit * 0x15) + y;
        cursor_redraw_off_thunk();
        draw_bitmap(BMP_PTR(BMPSET_PTR(list)->bmp_ptr[0]), x, row, 0);
    } else {
        digit = (uint8_t)(digit + 0xfb);    /* `add al, 0xfb` is `- 5` */
        row = (int16_t)(6 - (int16_t)digit * 0x15) + y;
        cursor_redraw_off_thunk();
        draw_bitmap(BMP_PTR(BMPSET_PTR(list)->bmp_ptr[1]), x, row, 0);
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
    dg_near_t set;
    int16_t  x;

    set_clip_play_area();
    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    set = DG4E67.bmp_4ecb_ptr;
    for (x = 0x10; x < 0x22f; x = (int16_t)(x + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x6]), x, 0, 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0]), 0, 0, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x1]), 0x230, 0, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xa]), 0x238, 0, 0);

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
    dg_near_t set;
    int16_t  x;

    set_clip_play_area();
    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    set = DG4E67.bmp_4ecb_ptr;
    for (x = 0x10; x < 0x22f; x = (int16_t)(x + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x7]), x, 0x168, 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x2]), 0, 0x160, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x3]), 0x230, 0x160, 0);

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
    dg_near_t set;
    int16_t  y;

    set_clip_play_area();
    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    set = DG4E67.bmp_4ecb_ptr;
    for (y = 8; y < 0x162; y = (int16_t)(y + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x4]), 0, y, 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0]), 0, 0, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x2]), 0, 0x160, 0);

    restore_cursor_following();
}

/*
 * 0x15c83
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
    dg_near_t set;
    int16_t  n;

    draw_machine_layer_f();

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    set = DG4E67.bmp_4ecb_ptr;

    for (n = 8; n < 0x162; n = (int16_t)(n + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x5]), 0x238, n, 0);

    for (n = 0; n < 0x16f; n = (int16_t)(n + 8))
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x8]), 0x278, n, 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x1]), 0x230, 0, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x3]), 0x230, 0x160, 0);

    VMDS.second_colour = 0;
    clip_and_draw_line(0x238, 0, 0x27f, 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xa]), 0x238, 0, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xa]), 0x238, 0x3b, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xb]), 0x23f, 0x42, 0);

    if (DG4E67.state == 0x800)
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x28]), 0x248, 0x45, 0);
    else if (DG4E67.state == 0x400)
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x29]), 0x25d, 0x45, 0);

    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0xa]), 0x238, 0x59, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x9]), 0x240, 0x168, 0);

    restore_cursor_following();
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
    char digits[16];      /* [bp-0x10] */
    uint16_t part;
    int16_t  kind, count, y, text_x, text_y;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    VMDS.clip_enabled = 1;
    set_clip_play_area();
    VMDS.fill_enabled = 1;
    VMDS.fill_colour   = ((uint8_t)DG52BD.bin_colour);
    VMDS.second_colour = ((uint8_t)DG52BD.bin_colour);

    cursor_redraw_off_thunk();
    fill_rect(0x241, 0x63, 0x37, 2);
    fill_rect(0x240, 0x65, 0x38, 0x103);
    restore_cursor_following();

    VMDS.text_style = 1;                            /* transparent text */

    part = PART_PTR(DG50D3.bin_list_ptr)->next_ptr;
    y    = 0x64;

    while (part != 0 && y <= 0x134) {
        struct bitmap *icon;

        kind = ((int16_t)PART_PTR(part)->kind);
        count = (part == DG50D3.dragged_part_ptr) ? 0 : 1;

        for (;;) {
            part = PART_PTR(part)->next_ptr;
            if (part == 0)
                break;
            if (((int16_t)PART_PTR(part)->kind) != kind)
                break;
            if (part != DG50D3.dragged_part_ptr)
                count++;
        }

        if (count == 0)
            continue;

        cursor_redraw_off_thunk();

        icon = BMP_PTR(BMPSET_PTR(DG4E67.icons_bmp_ptr)->bmp_ptr[kind]);
        draw_bitmap_centred(icon, 0x240, y, 0x38, 0x2a);

        int_to_string(count, digits, 10);
        text_x = (int16_t)(0x240 + (0x38 - (int16_t)text_width_thunk(digits)) / 2);

        text_y = (int16_t)(y + icon->height
                           + (0x2a - icon->height) / 2 + 1);
        if (text_y > 0x161)
            text_y = 0x161;

        VMDS.text_colour = 0;
        draw_string(digits, (int16_t)(text_x - 2), (int16_t)(text_y + 1));

        VMDS.text_colour = 0x0e;
        draw_string(digits, (int16_t)(text_x - 1), text_y);

        restore_cursor_following();

        y = (int16_t)(y + 0x34);
    }
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
void draw_bitmap_centred(struct bitmap *bmp, int16_t x, int16_t y,
                         int16_t w, int16_t h)
{
    x = (int16_t)(x + (w - bmp->width) / 2);
    y = (int16_t)(y + (h - bmp->height) / 2);

    draw_bitmap(bmp, x, y, 0);
}

/*
 * 0x15faa
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
    dg_near_t set;
    int16_t  frame, slide_a, slide_b;

    VMDS.clip_enabled = 1;
    VMDS.clip_top     = 0x0a;
    VMDS.clip_bottom  = 0x3b;
    VMDS.clip_left    = 0x240;
    VMDS.clip_right   = 0x277;

    DG4E67.loop_frames = 0;

    frame = (int16_t)(DG4E67.loop_frames >> 1);
    slide_a = (frame >= 4) ? (int16_t)(((frame - 4) * 2) % 0x38) : 0;

    frame = (int16_t)(DG4E67.loop_frames >> 1);
    slide_b = (frame >= 4) ? (int16_t)(((frame - 4) * 4) % 0x38) : 0;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    cursor_redraw_off_thunk();

    set = DG4E67.menu_bmp_ptr;
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0]), 0x240, 0x0a, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x1]), (int16_t)(0x208 + slide_a), 0x1a, 0);
    draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x2]), (int16_t)(0x208 + slide_b), 0x20, 0);

    if (frame < 6) {
        /* 0x25a2 the picture, 0x25ae its x, 0x25ba its y - by frame. */
        uint16_t which = MACHINE_DRAW_MENU_ANIM.picture[frame];

        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[which]),
                    MACHINE_DRAW_MENU_ANIM.picture_x[frame],
                    MACHINE_DRAW_MENU_ANIM.picture_y[frame], 0);
    }

    if (frame < 4) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[0x7]), 0x24a, 0x2a, 0);
    } else {
        int16_t f = (int16_t)(frame & 3);

        /* 0x25c6 its x and 0x25ce its y, by the frame modulo four. */
        draw_bitmap(BMP_PTR(BMPSET_PTR(set)->bmp_ptr[f + 0x8]),
                    MACHINE_DRAW_MENU_ANIM.sprite_x[f],
                    MACHINE_DRAW_MENU_ANIM.sprite_y[f], 0);
    }

    restore_cursor_following();
    set_clip_play_area();
}

/*
 * 0x160fc
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
 * +6 and +8. Both go in as **addresses of locals**, which is why this needs a
 * guest frame: `lea ax,[bp-6]` yields a DGROUP offset the callee reads, and a
 * C local has none. See dg_alloca in dgroup.h.
 */
void draw_carried_icon(void)
{
    struct extent16 ext;                     /* [bp-0xa], [bp-8] */
    int16_t at[3];     /* [bp-6],  [bp-4]  */
    uint16_t kind;
    struct bitmap *si;

    set_clip_play_area();

    kind = PART_PTR(DG50D3.dragged_part_ptr)->kind;
    si = BMP_PTR(BMPSET_PTR(DG4E67.icons_bmp_ptr)->bmp_ptr[kind]);

    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    cursor_redraw_off_thunk();
    draw_bitmap(si, (int16_t)((uint16_t)DG5768.pointer_x), (int16_t)((uint16_t)DG5768.pointer_y), 0);
    cursor_redraw_off_thunk();

    at[0] = (int16_t)(((uint16_t)DG5768.pointer_x) + ((uint16_t)DG4E67.origin_b_x));
    at[1] = (int16_t)(((uint16_t)DG5768.pointer_y) + ((uint16_t)DG4E67.origin_b_y));
    ext.width  = si->width;
    ext.height = si->height;

    alloc_shape((uint8_t *)at,
                (uint8_t *)&ext, 1, 2, 0);
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
    struct part *si;

    if (DG50D3.dragged_part_ptr != 0 && PART_PTR(DG50D3.dragged_part_ptr)->redraw_count != 0) {
        link_record_into_buckets(PART_PTR(DG50D3.dragged_part_ptr));
        PART_PTR(DG50D3.dragged_part_ptr)->redraw_count--;
    }

    for (si = pick_by_flag(0x3000); si != PART_NONE;
         si = pick_for_record(si, 0x1000)) {
        if ((redraw_all != 0 || si->redraw_count != 0)
            && si != PART_PTR(DG50D3.dragged_part_ptr))
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
 * 0x16209
 *
 * **Draw the selection around a part**: the marching-ants box, the four edge
 * strips, and whichever of the six handles that part can actually use.
 *
 * `0x25d6` is a phase counter cycling 0 to 3 and back, and it is what makes
 * the border crawl: every strip is drawn offset by it, or by `4 - it`, so the
 * pattern walks by a pixel a frame. It is stepped **once per call**, at the
 * top, before anything is drawn.
 *
 * The box comes from three different places depending on kind. A rope takes
 * its link's far part and that part's +0x56/+0x57 anchor; a belt takes its
 * +0x66 record's part and the end its +0xb names, offset by -8 and -4; and
 * everything else takes the part's own +0x2a/+0x2c and +0x44/+0x46.
 *
 * **The rope arm reads `[bp-0x20]` before anything has written it.** It uses
 * it in `([bp-0x20] >> 1) < si->+0x58`, the same shape of test
 * `part_handle_at_pointer` makes against the part's +0x46 - which is what that
 * local holds on *every other* path through this routine. On the rope path it
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
void draw_part_selection(struct part *part, uint16_t which, uint8_t flags)
{
    int16_t at[15];    /* [bp-0x1e], [bp-0x1c] */
    struct extent16 ext;   /* [bp-0x22] width, [bp-0x20] height */
    uint16_t idx, bmp;
    struct part *si;
    struct belt *rec;
    int16_t  step, tall;
    int16_t  keep_l = 1, keep_r = 1, keep_t = 1, keep_b = 1;
    int16_t  hx, hxm, hxr, hy, hym, hyb;

    if (MACHINE_DRAW_SELECTION_PHASE.phase == 3)
        MACHINE_DRAW_SELECTION_PHASE.phase = 0;
    else
        MACHINE_DRAW_SELECTION_PHASE.phase++;

    step = (int16_t)(4 - MACHINE_DRAW_SELECTION_PHASE.phase);

    VMDS.page_dst_ptr = VMDS.page_back_ptr;

    if (part->kind == KIND_BELT) {
        si = PART_PTR(ROPE_PTR(part->rope_ptr)->end_b_ptr);
        at[0] = (int16_t)(((uint16_t)si->box[0].x)
                               + si->grab.x);
        at[1] = (int16_t)(((uint16_t)si->box[0].y)
                                               + si->grab.y);
        ext.width = (int16_t)si->grab_size;
        /* Reads the height before it is written; see the comment above. */
        ext.height = (int16_t)(((int16_t)ext.height >> 1)
             < (int16_t)si->grab_size)
            ? 0x0a : si->grab_size;
    } else if (part->kind == KIND_ROPE) {
        rec = BELT_PTR(part->belt_ptr[0]);
        si = PART_PTR(rec->end_b_ptr);
        idx = ((int8_t)rec->slot_b);
        at[0] = (int16_t)(((uint16_t)si->box[0].x)
                               + si->attach[idx].x - 8);
        at[1] = (int16_t)(((uint16_t)si->box[0].y)
                       + si->attach[idx].y - 4);
        ext.width = 0x10;
        ext.height = 8;
    } else {
        at[1] = (int16_t)((uint16_t)part->box[0].y);
        at[0] = (int16_t)((uint16_t)part->box[0].x);
        ext.height = (int16_t)((uint16_t)part->size[0].height);
        ext.width = (int16_t)((uint16_t)part->size[0].width);
    }

    VMDS.clip_left = (uint16_t)((uint16_t)at[0] - ((uint16_t)DG4E67.origin_x));
    VMDS.clip_right = (uint16_t)((uint16_t)at[0] + (uint16_t)ext.width - ((uint16_t)DG4E67.origin_x) - 1);
    VMDS.clip_top = (uint16_t)((uint16_t)at[1] - ((uint16_t)DG4E67.origin_y));
    VMDS.clip_bottom = (uint16_t)((uint16_t)at[1]
                               + (uint16_t)ext.height
                               - ((uint16_t)DG4E67.origin_y) - 1);
    VMDS.clip_enabled = 1;

    if (VMDS.clip_left < 8)      { VMDS.clip_left = 8;     keep_l = 0; }
    if (VMDS.clip_right > 0x237)  { VMDS.clip_right = 0x237; keep_r = 0; }
    if (VMDS.clip_top < 8)      { VMDS.clip_top = 8;     keep_t = 0; }
    if (VMDS.clip_bottom > 0x167)  { VMDS.clip_bottom = 0x167; keep_b = 0; }

    if (which == 0x0e) {
        VMDS.second_colour = 0;
        clip_and_draw_line((int16_t)((uint16_t)VMDS.clip_left),
                           (int16_t)(((uint16_t)VMDS.clip_top) + 1),
                           (int16_t)((uint16_t)VMDS.clip_right),
                           (int16_t)(((uint16_t)VMDS.clip_bottom) + 1));
        clip_and_draw_line((int16_t)((uint16_t)VMDS.clip_left),
                           (int16_t)(((uint16_t)VMDS.clip_bottom) + 1),
                           (int16_t)((uint16_t)VMDS.clip_right),
                           (int16_t)(((uint16_t)VMDS.clip_top) + 1));
        VMDS.second_colour = 0x0c;
        clip_and_draw_line((int16_t)((uint16_t)VMDS.clip_left), (int16_t)((uint16_t)VMDS.clip_top),
                           (int16_t)((uint16_t)VMDS.clip_right), (int16_t)((uint16_t)VMDS.clip_bottom));
        clip_and_draw_line((int16_t)((uint16_t)VMDS.clip_left), (int16_t)((uint16_t)VMDS.clip_bottom),
                           (int16_t)((uint16_t)VMDS.clip_right), (int16_t)((uint16_t)VMDS.clip_top));
    }

    at[0] = (int16_t)(((uint16_t)VMDS.clip_left) + ((uint16_t)DG4E67.origin_x));
    at[1] = (int16_t)(((uint16_t)VMDS.clip_top) + ((uint16_t)DG4E67.origin_y));
    ext.width = (int16_t)(((uint16_t)VMDS.clip_right) - ((uint16_t)VMDS.clip_left) + 1);
    ext.height = (int16_t)(((uint16_t)VMDS.clip_bottom) - ((uint16_t)VMDS.clip_top) + 1);

    tall = ((int16_t)ext.height > 0x80) ? 1 : 0;

    cursor_redraw_off_thunk();

    bmp = (uint16_t)(DG52ED.cursor_art_ptr + which * 2);

    if (keep_t) {
        draw_bitmap_scaled(BMP_PTR(BMP_PTR(bmp)->data.off),
                           (int16_t)((uint16_t)VMDS.clip_left),
                           (int16_t)(((uint16_t)VMDS.clip_top) - step), 8, 0x88, 0);
        if (tall)
            draw_bitmap_scaled(BMP_PTR(BMP_PTR(bmp)->data.off),
                               (int16_t)((uint16_t)VMDS.clip_left),
                               (int16_t)(((uint16_t)VMDS.clip_top) - step + 0x80),
                               8, 0x88, 0);
    }

    if (keep_l)
        draw_bitmap_scaled(BMP_PTR(BMP_PTR(bmp)->data.seg),
                           (int16_t)(((uint16_t)VMDS.clip_left) - MACHINE_DRAW_SELECTION_PHASE.phase),
                           (int16_t)((uint16_t)VMDS.clip_top), 0x110, 1, 0);

    if (keep_r) {
        VMDS.clip_right++;
        draw_bitmap_scaled(BMP_PTR(BMP_PTR(bmp)->data.off),
                           (int16_t)(((uint16_t)VMDS.clip_right) - 1),
                           (int16_t)(((uint16_t)VMDS.clip_top) - MACHINE_DRAW_SELECTION_PHASE.phase),
                           8, 0x88, 0);
        if (tall)
            draw_bitmap_scaled(BMP_PTR(BMP_PTR(bmp)->data.off),
                               (int16_t)(((uint16_t)VMDS.clip_right) - 1),
                               (int16_t)(((uint16_t)VMDS.clip_top) - MACHINE_DRAW_SELECTION_PHASE.phase
                                         + 0x80), 8, 0x88, 0);
        VMDS.clip_right--;
    }

    if (keep_b) {
        VMDS.clip_bottom++;
        draw_bitmap_scaled(BMP_PTR(BMP_PTR(bmp)->data.seg),
                           (int16_t)(((uint16_t)VMDS.clip_left) - step),
                           (int16_t)(((uint16_t)VMDS.clip_bottom) - 1), 0x110, 1, 0);
    }

    set_clip_for_mode();

    hx  = (int16_t)((uint16_t)at[0] - ((uint16_t)DG4E67.origin_x) - 12);
    hxm = (int16_t)(hx + ((int16_t)ext.width >> 1) + 6);
    hxr = (int16_t)(hx + (int16_t)ext.width + 0x0c);
    hy  = (int16_t)((uint16_t)at[1] - ((uint16_t)DG4E67.origin_y) - 11);
    hym = (int16_t)(hy + ((int16_t)ext.height >> 1) + 6);
    hyb = (int16_t)(hy + (int16_t)ext.height + 0x0c);

    VMDS.fill_enabled = 1;
    VMDS.fill_colour = 0x0f;
    VMDS.second_colour = 0x0f;

    DG50AF.flip_options = part_flip_options(part);

    draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.cursor_art_ptr)->bmp_ptr[0x1b]), hx, hy, 0);

    if (DG50AF.flip_options & 1) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.cursor_art_ptr)->bmp_ptr[0x1c]), hx, hym, 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.cursor_art_ptr)->bmp_ptr[0x1c]), hxr, hym, 0);
    }
    if (DG50AF.flip_options & 2) {
        draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.cursor_art_ptr)->bmp_ptr[0x1d]), hxm, hy, 0);
        draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.cursor_art_ptr)->bmp_ptr[0x1d]), hxm, hyb, 0);
    }
    if (DG50AF.flip_options & 4)
        draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.cursor_art_ptr)->bmp_ptr[0x1e]), hx, hyb, 0);
    if (DG50AF.flip_options & 8)
        draw_bitmap(BMP_PTR(BMPSET_PTR(DG52ED.cursor_art_ptr)->bmp_ptr[0x1f]), hxr, hyb, 0);

    at[0] = (int16_t)((uint16_t)at[0] - 0x0c);
    at[1] = (int16_t)((uint16_t)at[1] - 0x0c);
    ext.width = (int16_t)((uint16_t)ext.width + 0x18);
    ext.height = (int16_t)((uint16_t)ext.height + 0x19);

    alloc_shape((uint8_t *)at,
                (uint8_t *)&ext, flags, 2, 0);

    restore_cursor_following();
}

/*
 * 0x166d6
 *
 * Clear six words at DGROUP 0x50bf. The loop counts *down* from 5 and tests
 * `jge`, so index 0 is cleared too - six entries, not five. What they hold is
 * not established.
 */
void clear_layer_heads(void)
{
    int16_t i = 5;

    do {
        DG50BF.layer_head_ptr[i] = 0;
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
void link_record_into_buckets(struct part *rec)
{
    int16_t kind = ((int16_t)rec->kind);
    int16_t i;

    rec->flags_0a |= 0x20;

    for (i = 0; i < 2; i++) {
        uint8_t slot = PART_KINDS[kind].refile_level[i];

        if (slot == 0xFF)
            continue;
        if (rec == PART_PTR(DG50D3.dragged_part_ptr))
            slot = 0;

        rec->layer_next_ptr[i] = DG50BF.layer_head_ptr[slot];
        DG50BF.layer_head_ptr[slot] = dg_near(dgroup, rec);
        if (i == 0)
            rec->layer_slot = slot;
    }
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
 * lists are emptied by being drawn; `clear_layer_heads` at the end takes
 * the heads with them.
 *
 * A rope, kind 8, and a belt, kind 0x0a, each draw themselves; kind 0x31 draws
 * nothing at all. Everything else goes through the one blitter, which is told
 * the level as well, so a part in two buckets is drawn twice at two depths.
 *
 * The page being drawn into, `VMDS.page_dst_ptr`, is set from `VMDS.page_back_ptr` first, and the
 * clip is put back to whatever the mode wants.
 */
void draw_machine(int16_t a, int16_t b)
{
    uint8_t  v02;          /* [bp-2] the level */
    uint8_t  v01;          /* [bp-1] the counter */
    uint16_t si;

    VMDS.page_dst_ptr = VMDS.page_back_ptr;
    VMDS.clip_enabled = 1;
    set_clip_for_mode();

    for (v01 = 6; v01 != 0; v01--) {
        v02 = (uint8_t)(v01 - 1);

        for (si = DG50BF.layer_head_ptr[v02]; si != 0;
             si = (PART_PTR(si)->layer_slot == v02
                   ? PART_PTR(si)->layer_next_ptr[0]
                   : PART_PTR(si)->layer_next_ptr[1])) {
            PART_PTR(si)->flags_0a &= 0xffdf;

            if (PART_PTR(si)->kind == KIND_BELT)
                draw_rope(PART_PTR(si), a);
            else if (PART_PTR(si)->kind == KIND_ROPE)
                draw_belt(PART_PTR(si), a);
            else if (PART_PTR(si)->kind != KIND_ANCHOR)
                draw_part(PART_PTR(si), (int16_t)v02, a, b);
        }
    }

    clear_layer_heads();

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
void draw_rope(struct part *part, int16_t a)
{
    /*
     * The frame really is eight words - the four points the two lines are
     * drawn between - and the original addresses them from BP downwards, so
     * `p[0]` is `[bp-2]` and `p[7]` is `[bp-0x10]`. Held as the array it is,
     * the table is the same eight words in the other order.
     */
    int16_t words[8];
    int16_t *p[8];
    struct rope *si = ROPE_PTR(part->rope_ptr);
    int32_t k;

    for (k = 0; k < 8; k++)
        p[k] = &words[7 - k];                  /* [bp-2] down to [bp-0x10] */

    if (si->end_a_ptr == 0 || si->end_b_ptr == 0)
        goto out;

    cursor_redraw_off_thunk();

    for (k = 0; k < 8; k++)
        *p[k] = (int16_t)((k & 1)
                          ? si->pt[0][k >> 1].y - DG4E67.origin_y
                          : si->pt[0][k >> 1].x - DG4E67.origin_x);

    if (a != 0) {
        for (k = 0; k < 8; k++)
            *p[k] = (int16_t)((int16_t)long_shift_right(
                mul16x16((*p[k]), a), 10)
                + ((k & 1) ? 0x48 : 0x110));
    }

    VMDS.second_colour = 0;

    clip_and_draw_line((*p[0]), (*p[1]), (*p[2]), (*p[3]));
    clip_and_draw_line((*p[4]), (*p[5]), (*p[6]), (*p[7]));

    restore_cursor_following();

out:
    return;
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

    VMDS.second_colour = colour;

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

    draw_curve(VMDS.second_colour, 4, x0, mx, x1, y0, my, y1);
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
void draw_belt(struct part *part, int16_t a)
{
    struct belt *v0e;       /* [bp-0x0e] the belt */
    int16_t  v0c;       /* [bp-0x0c] the slack */
    int16_t  v0a;       /* [bp-0x0a] sags */
    int16_t  v08;       /* [bp-8]  y1 */
    int16_t  v06;       /* [bp-6]  x1 */
    int16_t  v04;       /* [bp-4]  y0 */
    int16_t  v02;       /* [bp-2]  x0 */
    struct part *si;
    struct part *di;

    v0e = BELT_PTR(part->belt_ptr[0]);

    di = PART_PTR(v0e->end_a_ptr);
    si = PART_PTR(di->link_ptr[v0e->slot_a]);
    if (si == PART_NONE)
        si = PART_PTR(v0e->end_b_ptr);

    while (di != PART_NONE && si != PART_NONE) {
        v0a = 0;

        if (di->kind == KIND_PULLEY) {
            v02 = (int16_t)(
                BELT_PTR(di->belt_ptr[0])->pt[0][1].x
                - DG4E67.origin_x);
            v04 = (int16_t)(
                BELT_PTR(di->belt_ptr[0])->pt[0][1].y
                - DG4E67.origin_y);
        } else {
            v02 = (int16_t)(v0e->pt[0][0].x
                                  - DG4E67.origin_x);
            v04 = (int16_t)(v0e->pt[0][0].y
                                  - DG4E67.origin_y);
            v0a = 1;
        }

        if (si->kind == KIND_PULLEY) {
            v06 = (int16_t)(
                BELT_PTR(si->belt_ptr[0])->pt[0][0].x
                - DG4E67.origin_x);
            v08 = (int16_t)(
                BELT_PTR(si->belt_ptr[0])->pt[0][0].y
                - DG4E67.origin_y);
        } else {
            v06 = (int16_t)(v0e->pt[0][1].x
                                  - DG4E67.origin_x);
            v08 = (int16_t)(v0e->pt[0][1].y
                                  - DG4E67.origin_y);
            v0a = 1;
        }

        if (a != 0) {
            v02 = (int16_t)((int16_t)long_shift_right(
                mul16x16(v02, a), 10) + 0x110);
            v04 = (int16_t)((int16_t)long_shift_right(
                mul16x16(v04, a), 10) + 0x48);
            v06 = (int16_t)((int16_t)long_shift_right(
                mul16x16(v06, a), 10) + 0x110);
            v08 = (int16_t)((int16_t)long_shift_right(
                mul16x16(v08, a), 10) + 0x48);
        }

        VMDS.second_colour = 6;
        cursor_redraw_off_thunk();

        if (v0a != 0) {
            v0c = link_slack(di, v0e, 3);
            draw_belt_segment(v02, v04, v06, v08,
                              v0c);
        } else {
            clip_and_draw_line(v02, v04, v06, v08);
        }

        if (a == 0) {
            if (di->kind != KIND_ANCHOR
                && di->kind != KIND_PULLEY)
                draw_bitmap(BMP_PTR(BMPSET_PTR(DG4E67.bmp_4ecb_ptr)->bmp_ptr[0x24]),
                            (int16_t)(v02 - 5),
                            (int16_t)(v04 - 2), 0);

            if (si->kind != KIND_ANCHOR
                && si->kind != KIND_PULLEY)
                draw_bitmap(BMP_PTR(BMPSET_PTR(DG4E67.bmp_4ecb_ptr)->bmp_ptr[0x24]),
                            (int16_t)(v06 - 5),
                            (int16_t)(v08 - 2), 0);
        }

        restore_cursor_following();

        di = si;
        if (di->kind == KIND_PULLEY)
            si = PART_PTR(si->link_ptr[0]);
        else
            si = PART_NONE;
    }

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
void draw_part(struct part *part, int16_t level, int16_t a, int16_t b)
{
    struct bitmap *v2a;   /* [bp-0x2a] the bitmap */
    const struct draw_step *v28;   /* [bp-0x28] the record */
    const struct part_kind *v26;   /* [bp-0x26] the kind's record */
    uint16_t v24;   /* [bp-0x24] the adjustment */
    const struct point8 *hot;
    uint8_t  v21;   /* [bp-0x21] the frame */
    uint16_t v20;   /* [bp-0x20] py */
    uint16_t v1e;   /* [bp-0x1e] px */
    uint16_t v1c;   /* [bp-0x1c] the bitmap index */
    uint16_t v1a;   /* [bp-0x1a] the mirror flags */
    int16_t  v18;   /* [bp-0x18] */
    int16_t  v16;   /* [bp-0x16] rows */
    int16_t  v14;   /* [bp-0x14] columns */
    int16_t  v12;   /* [bp-0x12] */
    int16_t  v10;   /* [bp-0x10] */
    int16_t  v0e;   /* [bp-0x0e] */
    int16_t  v0c;   /* [bp-0x0c] */
    int16_t  v0a;   /* [bp-0x0a] y */
    int16_t  v08;   /* [bp-8] x */
    int16_t  v06;   /* [bp-6] the column */
    uint16_t v04;   /* [bp-4] the form */
    uint16_t v02;   /* [bp-2] the kind */
    int16_t di;

    v02 = part->kind;
    v04 = part->form;
    v26 = &PART_KINDS[v02];

    v24 = v26->hotspots_ptr;
    hot = POINT_TABLE(v24);                    /* the hot spot by form, if the kind has them */

    cursor_redraw_off_thunk();

    if (part->flags_06 & 0x40) {
        v14 = (int16_t)(part->size[0].width >> 4);
        v16 = (int16_t)(part->size[0].height >> 4);

        v18 = (int16_t)(part->pos[0].x - DG4E67.origin_x);
        v0a = (int16_t)(part->pos[0].y - DG4E67.origin_y);

        if (v24 != 0) {
            v18 = (int16_t)(v18 + (int8_t)hot[v04].x);
            v0a = (int8_t)hot[v04].y;
        }

        v1e = (int16_t)((v18 & 0x10) >> 4);
        v20 = (int16_t)((v0a & 0x10) >> 4);
        v1c = v04;

        for (di = 0; di < v16; di++,
             v0a = (int16_t)(v0a + 0x10),
             v20 ^= 1) {
            for (v06 = 0, v08 = v18;
                 v06 < v14;
                 v06++,
                 v08 = (int16_t)(v08 + 0x10),
                 v1e ^= 1) {
                if (v16 == 1) {
                    if (v06 == 0)
                        v1c = v04;
                    else if ((int16_t)(v14 - 1) == v06)
                        v1c = (uint16_t)(v04 + 3);
                    else
                        v1c = (uint16_t)(v04 + v1e + 1);
                } else if (v14 == 1) {
                    if (di == 0)
                        v1c = (uint16_t)(v04 + 4);
                    else if ((int16_t)(v16 - 1) == di)
                        v1c = (uint16_t)(v04 + 7);
                    else
                        v1c = (uint16_t)(v04 + v20 + 5);
                }

                {
                    struct bitmap *bmp = BMP_PTR(BMPSET_PTR(v26->bitmaps_ptr)->bmp_ptr[v1c]);

                    if (a != 0) {
                        v0c = (int16_t)long_shift_right(
                            mul16x16(0x10, b), 10);
                        v0e = (int16_t)long_shift_right(
                            mul16x16(0x10, b), 10);
                        v10 = (int16_t)((int16_t)long_shift_right(
                            mul16x16(v08, a), 10) + 0x110);
                        v12 = (int16_t)((int16_t)long_shift_right(
                            mul16x16(v0a, a), 10) + 0x48);

                        draw_bitmap_scaled(bmp, v10, v12,
                                           v0c, v0e, 0);
                    } else {
                        draw_bitmap(bmp, v08, v0a, 0);
                    }
                }
            }
        }

        goto done;
    }

    if (part->flags_08 & 0x1000) {
        v28 = DRAWSTEP_PTR(OFF_TABLE(v26->bitmaps2_ptr)[v04]);
    } else {
        v28 = &DG0124;
        DG0124.frame[0] = (uint8_t)v04;
        DG0124.level = (uint8_t)level;

        if (v24 != 0) {
            DG0124.offset[0].x = hot[v04].x;
            DG0124.offset[0].y = hot[v04].y;
        } else {
            DG0124.offset[0].y = 0;
            DG0124.offset[0].x = 0;
        }
    }

    while (v28 != DRAWSTEP_NONE) {
        if (v28->level != (uint8_t)level
            && part != PART_PTR(DG50D3.dragged_part_ptr))
            goto next;

        v21 = v28->frame[0];

        for (di = 0; ; di++) {
            v2a = BMP_PTR(BMPSET_PTR(v26->bitmaps_ptr)->bmp_ptr[v21]);

            v08 = (int16_t)(part->pos[0].x - DG4E67.origin_x);
            v0a = (int16_t)(part->pos[0].y - DG4E67.origin_y);

            if (part->flags_08 & 0x10) {
                v08 = (int16_t)(
                    v08
                    + (part->mirror_size.width
                       - (int8_t)v28->offset[di].x
                       - v2a->width));
                v1a = 2;
            } else {
                v08 = (int16_t)(
                    v08
                    + (int8_t)v28->offset[di].x);
                v1a = 0;
            }

            if (part->flags_08 & 0x20) {
                v0a = (int16_t)(
                    v0a
                    + (part->mirror_size.height
                       - (int8_t)v28->offset[di].y
                       - v2a->height));
                v1a |= 1;
            } else {
                v0a = (int16_t)(
                    v0a
                    + (int8_t)v28->offset[di].y);
            }

            if (a != 0) {
                v0c = (int16_t)long_shift_right(
                    mul16x16(v2a->width, b), 10);
                v0e = (int16_t)long_shift_right(
                    mul16x16(v2a->height, b), 10);
                v10 = (int16_t)((int16_t)long_shift_right(
                    mul16x16(v08, a), 10) + 0x110);
                v12 = (int16_t)((int16_t)long_shift_right(
                    mul16x16(v0a, a), 10) + 0x48);

                draw_bitmap_scaled(v2a, v10, v12,
                                   v0c, v0e, v1a);
            } else {
                draw_bitmap(v2a, v08, v0a, v1a);
            }

            v21 = v28->frame[di + 1];

            if (di + 1 >= 4 || v21 == 0xff)
                break;
        }

    next:
        v28 = DRAWSTEP_PTR(v28->next);
    }

done:
    if (((int16_t)DG4E67.state) == 0x2000 && part->kind == KIND_MAGNIFYING_GLASS)
        draw_part_extra(part);

    restore_cursor_following();

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
void draw_part_extra(struct part *part)
{
    /*
     * **The frame, as the original reserves it.** `sub sp,0x14` at 0x171b5,
     * which `tools/frames.py` checks. Laying the locals out inside one array
     * rather than as separate C variables is what keeps them adjacent, and
     * adjacency is not incidental here: the three x's and the three y's are
     * arrays this routine hands to `draw_polygon` by address.
     */
    int16_t size[2];  /* [bp-0x14], [bp-0x12] */
    int16_t corner[2];  /* [bp-0x10], [bp-0x0e] */
    int16_t y[3];  /* [bp-0x0c] .. [bp-8]  */
    int16_t x[3];  /* [bp-6] .. [bp-2]     */
    struct part *di = PART_PTR(part->link_ptr[4]);
    int16_t edge;

    if (di == PART_NONE)
        goto out;

    VMDS.fill_colour = 0x0e;
    VMDS.second_colour = 0x0e;

    x[1] = (int16_t)(di->pos[0].x
                          + di->hold.x - DG4E67.origin_x);
    y[0] = (int16_t)(part->pos[0].y + 6 - DG4E67.origin_y);
    y[1] = (int16_t)(di->pos[0].y
                          + di->hold.y - DG4E67.origin_y);
    y[2] = (int16_t)(part->pos[0].y + 0x10 - DG4E67.origin_y);

    if (part->flags_08 & 0x10)
        edge = (int16_t)(part->pos[0].x - 1);
    else
        edge = (int16_t)(part->pos[0].x + 0x0f);

    edge = (int16_t)(edge - DG4E67.origin_x);
    x[2] = edge;
    x[0] = edge;

    draw_polygon(3, x, y);

    if (x[0] < x[1]) {
        corner[0] = x[0];
        size[0] = (int16_t)(x[1] - x[0]);
    } else {
        corner[0] = x[1];
        size[0] = (int16_t)(x[0] - x[1]);
    }
    size[0]++;

    corner[1] = y[0] < y[1] ? y[0] : y[1];

    size[1] = (int16_t)((y[2] >= y[1] ? y[2] : y[1])
                          - corner[1] + 1);

    corner[0] = (int16_t)(corner[0] + DG4E67.origin_b_x);
    corner[1] = (int16_t)(corner[1] + DG4E67.origin_b_y);

    alloc_shape((const uint8_t *)corner, (const uint8_t *)size,
                1, 2, 0);

out:
}

/*
 * 172c:0000, image 0x172bc
 *
 *
 * The module's first routine, and it does nothing at all: a frame and a `retf`.
 * It is here because the segment's own offset 0 has to be something.
 */
void seg172c_nothing(void)
{
}
