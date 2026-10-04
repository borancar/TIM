/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Text**: choosing a font, measuring a string and drawing one a glyph at a
 * time.
 *
 * A module of the original's **code segment 1c25**, image 0x2149e..0x21ab5,
 * split out of engine.c on 2026-09-27. Its `_DATA` is the colour map at DGROUP
 * 0x471e, between the keyboard's byte and the joystick's block, and its `_BSS`
 * is the five font tables at 0x6176..0x628e, which fontload.c fills. The
 * `vm_show_page` thunk at 0x2149a before it and the two thunks at 0x21ab5
 * after it are assembly. Functions are in address order and each carries the
 * image offset it was read from.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -G -O
 * JUDGE: data 0x471e..0x4723
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The text colour map**, DGROUP 0x471e..0x4723, 0x05 bytes: `draw_char` maps a glyph
 * pixel value below five through it. The image holds 0, 1, 2, 3, 4.
 */
struct engine_text_colours {
    uint8_t   colour[5];          /* +0x00 [5] */
} PACKED;

struct engine_text_colours g_engine_text_colours = { { 0x00, 0x01, 0x02, 0x03, 0x04 } };

#ifdef __TURBOC__
/* The driver's glyph entry, slot 1 (DGROUP 0x434a): everything in
   registers. Called from C, because Borland C++ 2.0's own assembler reads a
   name after `call` as a C label - `call dword ptr g_vm_driver+8` assembled as
   `g_vm_driver-8`. */
typedef void (far *vm_glyph_fn)(void);
#endif

/* **This module's `_BSS`**, DGROUP 0x6176..0x628e: the five font tables. BC++
   2.0 orders them by name, and these names are ones that order puts where
   the image has them - `g_engine_font_*` did not; see files.c. */
struct engine_font_kinds g_font_kinds;
struct engine_font_bodies g_font_bodies;   /* DGROUP 0x618a */
struct engine_font_widths g_font_widths;   /* DGROUP 0x61da */
struct engine_font_slots g_font_slots;   /* DGROUP 0x622a */
struct engine_underline_rows g_underline_rows;   /* DGROUP 0x627a */

/*
 * 0x2149e
 *
 * **Slot 0 of every font array is the current font**, and this is what moves a
 * font in and out of it. The call does two different jobs depending on its
 * argument, which is why it answers a slot number rather than nothing:
 *
 *   `set_font(n)`  copies slot `n` over slot 0 - the five header bytes and the
 *                  three far pointers - and answers `n`. A slot that was never
 *                  loaded is refused and the answer is 0.
 *   `set_font(0)`  changes nothing and instead *asks* which slot the current
 *                  font came from, by looking for one whose far pointer equals
 *                  slot 0's. It answers 0 when slot 0 is empty and 0x14 when
 *                  no slot matches, and the caller has to tell those apart from
 *                  a real slot itself.
 *
 * The search compares the segment first and then the offset, which is why the
 * pointers are read as two words rather than one long.
 */
uint16_t set_font(register int16_t slot)
{
    register int16_t found = 0;
    uint8_t far **p;                    /* [bp-2] */
    uint8_t far *cur;                   /* [bp-6] */

    if (slot == 0) {
        if ((cur = g_font_bodies.body[0]) != NULL) {
            /* Which slot holds the same pointer as slot 0. */
            for (p = &g_font_bodies.body[found = 1]; found < 0x14; found++, p++)
                if (*p == cur)
                    break;
        } else
            found = 0;
    } else if (font_slot_in_use(slot)) {
        found = slot;

        g_font_kinds.kind[0] = g_font_kinds.kind[slot];
        g_vmds.font_cell_width[0] = g_vmds.font_cell_width[slot];
        g_vmds.font_cell_height[0] = g_vmds.font_cell_height[slot];
        g_underline_rows.underline_row[0] = g_underline_rows.underline_row[slot];
        g_vmds.font_first_char[0] = g_vmds.font_first_char[slot];
        g_vmds.font_char_count[0] = g_vmds.font_char_count[slot];

        g_font_bodies.body[0] = g_font_bodies.body[slot];
        g_font_widths.width[0] = g_font_widths.width[slot];
        g_font_slots.slot[0] = g_font_slots.slot[slot];
    }

    return (uint16_t)found;
}

/*
 * 0x21575
 *
 * The width of a font's characters, for a font named by slot: the byte at
 * 0x38c4 + slot, with the same rule as `font_line_height` beside it - an
 * empty slot answers 0, except slot 0. **Nothing in the image calls it**: no
 * near or far call reaches 0x21575 and no word holds its offset. It is here
 * because the module has it.
 */
uint16_t font_char_width(register int16_t slot)
{
    uint8_t w;

    if (font_slot_in_use(slot) || slot == 0)
        w = g_vmds.font_cell_width[slot];
    else
        w = 0;
    return w;
}

/*
 * 0x215a5
 *
 * The height of a font's characters, for a font named by slot: the byte at
 * 0x38d8 + slot, which is the same table `load_font` fills.
 *
 * A slot that `font_slot_in_use` says is empty answers 0 - **except slot 0**,
 * which answers its height anyway. The test is `if (!in_use(slot) && slot != 0)
 * return 0`, so the current font is always measurable whether or not it is
 * filed in the table.
 */
uint16_t font_line_height(register int16_t slot)
{
    uint8_t h;

    if (font_slot_in_use(slot) || slot == 0)
        h = g_vmds.font_cell_height[slot];
    else
        h = 0;
    return h;
}

/*
 * 0x215d5
 *
 * Whether a font slot - an entry of `g_font_bodies`, DGROUP 0x618a - is
 * in use.
 * Answers 1 for a non-null far pointer there, 0 otherwise.
 *
 * The index is refused at both ends - not positive, or 0x14 and over - so the
 * table is twenty entries and index 0 is never accepted, which is what makes 0
 * usable as "no entry".
 */
uint16_t font_slot_in_use(register int16_t index)
{
    return index > 0 && index < 0x14
           && g_font_bodies.body[index] != NULL;
}

/*
 * 0x215ff
 *
 * `text_width`, reached the way every caller reaches it: the string arrives as
 * a near offset and the body wants a far pointer, so this pushes `ds` in front
 * of it and calls through. Nothing else.
 */
uint16_t text_width_thunk(const char *str)
{
    return text_width(str);
}

/*
 * 0x21610
 *
 * How wide a string is in the current font. The body; 0x215ff below is the
 * door, and exists only to make a far pointer out of the caller's near one.
 *
 * A character's width comes from one of two places, chosen once before the
 * loop: if the font has a width table - the far pointer at DGROUP 0x61da is
 * not null - each character is looked up in the table at 0x622a, and if it has
 * not, every character is the fixed width at 0x38c4. So a proportional font
 * and a fixed one go through the same loop with the test hoisted out of it.
 *
 * A character is turned into an index by subtracting the font's first code at
 * 0x38ec, and two tests then drop it: a negative index - a character below the
 * font's range - and one at or past the count at 0x3900. Either **stops the
 * measurement**, rather than skipping the character: the `jl` and the `jle`
 * both go to the loop's own test, which then sees the same non-NUL byte and
 * ... does not loop, because the pointer was already advanced. A string with
 * an out-of-range character measures only as far as that character.
 */
uint16_t text_width(const char far *str)
{
    register int16_t index;
    register uint16_t width = 0;
    int16_t proportional = g_font_widths.width[0] != NULL;

    while (*str != 0) {
        index = (uint8_t)*str++ - g_vmds.font_first_char[0];
        /* A character outside the font is skipped, not the end: both tests
           go back to the loop's own. */
        if (index >= 0 && g_vmds.font_char_count[0] > index)
            /* `les bx, [0x622a]`: the width table is far. See `draw_char`. */
            width += proportional ? g_font_slots.slot[0][index]
                                  : g_vmds.font_cell_width[0];
    }

    return width;
}

/*
 * 0x21670
 *
 * **Draw one character**, and answer how wide it was. Everything the game puts
 * on the screen in words goes through here.
 *
 * *Where the glyph is.* Three font formats, chosen by the marker at DGROUP
 * 0x6176 that `load_font` negated out of the file's first byte:
 *
 *   bit 0 set  proportional. The width is the character's own byte in the
 *              table at 0x622a, and the glyph starts at the offset the word
 *              table at 0x61da holds for it, from the block at 0x618a.
 *   2          fixed, **one byte to a pixel**. The glyph is `index * w * h`
 *              into the block.
 *   otherwise  fixed, **one bit to a pixel**, so a row is `(w + 7) >> 3`
 *              bytes and the glyph is `((w + 7) >> 3) * index * h` in.
 *
 * A character below the font's first code, or at or past its count, draws
 * nothing and answers 0.
 *
 * *Where it goes.* The clip box is tested once for the whole glyph, and the
 * answer picks which routine every pixel then goes through: `plot_pixel_clipped`
 * when any edge is crossed, and the driver's own plot - the far pointer at
 * DGROUP 0x439e - when none is. So a glyph wholly inside the box pays no clip
 * test per pixel, and one that crosses an edge pays it on all of them. The
 * test is `x < left || y < top || x + w > right || y + h > bottom`, and the
 * two width comparisons are **unsigned** where the two origin ones are signed.
 *
 * *The style byte at 0x3892* is five independent things, and they are why this
 * routine is as long as it is:
 *
 *   bit 0  clear means opaque: each row is first drawn as a line in the
 *          background colour at 0x3891 before any pixel of the glyph.
 *   bit 1  bold - every lit pixel is drawn again one to the right.
 *   bit 2  italic - the whole glyph starts `h / 2` to the right and loses one
 *          column every second row, which is a shear done by moving the origin
 *          rather than by transforming anything.
 *   bit 3  underline - on the row the font names at 0x627a, a *blank* pixel is
 *          drawn in the entering colour instead of being skipped.
 *   bit 4  half-tone - a lit pixel is only drawn where `x + y` is odd.
 *
 * In the one-byte-per-pixel format the byte is a colour, not a mask, and a
 * value under 5 is looked up in the table at 0x471e first - which is how the
 * game recolours a font's own shading without touching the glyph.
 *
 * The colour at 0x3890 is saved on the way in and put back on the way out,
 * because the byte-per-pixel path writes it as it goes.
 */
uint16_t near draw_char(uint8_t c, int16_t x, register int16_t y)
{
    /* The glyph's bytes, walked and never stored - so a pointer, and the
       segment that does not move stops being carried alongside. */
    const uint8_t far *glyph;           /* [bp-4] */
    uint8_t  entering;                  /* [bp-5] */
    uint16_t col;                       /* [bp-8] */
    uint16_t row;                       /* [bp-0xa] */
    uint8_t  mask;                      /* [bp-0xb] */
    uint8_t  one_bit;                   /* [bp-0xc] */
    uint8_t  pixel;                     /* [bp-0xd] */
    uint16_t w;                         /* [bp-0x10] */
    uint16_t h;                         /* [bp-0x12] */
    int16_t  index;                     /* [bp-0x14] */
    /* **Chosen once per glyph**: `plot_pixel_clipped` when any part of the
       glyph is outside the clip box, the driver's own plot (slot 22,
       DGROUP 0x439e) when none is - which is where `plot_pixel_clipped`
       itself jumps when nothing needs clipping. */
    bmp_plot_fn plot;                   /* [bp-0x18] */
    register int16_t px;

    entering = g_vmds.text_colour;
    index = c - g_vmds.font_first_char[0];
    if (index < 0 || g_vmds.font_char_count[0] <= index)
        return 0;

    if (g_font_kinds.kind[0] & 1) {
        /*
         * **Both tables are far pointers.** `les bx, [0x622a]` and
         * `les bx, [0x61da]` load a segment as well as an offset, so the width
         * table and the glyph-offset table live in the font's own block and
         * not in DGROUP. Reading the two words as near offsets took the widths
         * and the glyph offsets out of low DGROUP - which drew every character
         * of every proportional string as a block of noise, and is why the
         * briefing's title bar and its description came out smeared while the
         * panel's labels, which are bitmaps, were right.
         */
        w = g_font_slots.slot[0][index];
        h = g_vmds.font_cell_height[0];
        glyph = g_font_bodies.body[0]
                + ((const uint16_t far *)g_font_widths.width[0])[index];
    } else if (g_font_kinds.kind[0] == 2) {
        w = g_vmds.font_cell_width[0];
        h = g_vmds.font_cell_height[0];
        glyph = g_font_bodies.body[0] + index * w * h;
    } else {
        w = g_vmds.font_cell_width[0];
        h = g_vmds.font_cell_height[0];
        glyph = g_font_bodies.body[0] + ((w + 7) >> 3) * index * h;
    }

    if (x < g_vmds.clip_left || y < g_vmds.clip_top
        || x + w > (uint16_t)g_vmds.clip_right
        || y + h > (uint16_t)g_vmds.clip_bottom)
        plot = plot_pixel_clipped;
    else
        plot = ((bmp_plot_fn)g_vm_driver.entry[VM_SLOT_PLOT_PIXEL]);

    if (g_font_kinds.kind[0] <= 1)
        one_bit = 1;
    else
        one_bit = 0;

    if (g_vmds.text_style & 4)
        x += h / 2;

    for (row = 0; row < h; row++, y++, glyph++) {
        if (!((int8_t)g_vmds.text_style & 1)) {
            g_vmds.second_colour = g_vmds.text_back;
            clip_and_draw_line(x, y, x + w, y);
        }

        mask = 0x80;
        for (col = 0; col < w; col++) {
            if (one_bit) {
                if (!mask) {
                    mask = 0x80;
                    glyph++;
                }
                pixel = *glyph & mask;
                mask >>= 1;
            } else {
                if ((pixel = *glyph) != 0)
                    g_vmds.text_colour = pixel < 5
                                       ? g_engine_text_colours.colour[pixel]
                                       : pixel;
                if (w - 1 > col)
                    glyph++;
            }

            px = x + col;

            if (pixel) {
                if (g_vmds.text_style & 0x10) {
                    /* half-tone: every other pixel, but bold still draws */
                    if ((px + y) & 1)
                        plot(px, y, (int8_t)g_vmds.text_colour);
                    else if (g_vmds.text_style & 2)
                        plot(px + 1, y, (int8_t)g_vmds.text_colour);
                } else {
                    plot(px, y, (int8_t)g_vmds.text_colour);
                    if (g_vmds.text_style & 2)
                        plot(px + 1, y, (int8_t)g_vmds.text_colour);
                }
            } else if ((g_vmds.text_style & 8)
                       && g_underline_rows.underline_row[0] == row)
                plot(px, y, entering);
        }

        if ((g_vmds.text_style & 4) && (row & 1))
            x--;
    }

    g_vmds.text_colour = entering;
    return w;
}

/*
 * 0x218d4
 *
 * `draw_string_body`, reached the way the game reaches it: the string arrives
 * as a near offset and the body wants a far pointer. Nothing else.
 */
void draw_string(const char *str, int16_t x, int16_t y)
{
    draw_string_body(str, x, y);
}

/*
 * 0x218eb
 *
 * **Draw a string.** The body, and it takes a **far** pointer; 0x218d4 below is
 * the door that puts `ds` in front of a caller's near one. The picker's listing
 * is what needs the far form - its text is in a block DOS handed over, not in
 * DGROUP - and a null is both halves being zero, not just the offset.
 *
 * Two paths, and the whole of the difference is speed. The **slow** one calls
 * `draw_char` for each character and moves x on by what it answers, plus one
 * more when the style says bold. The **fast** one hands the glyph to the
 * driver in registers - `es:si` the pixels, `bx` and `cx` the size, `dx` and
 * `bp` the position - through the far pointer at DGROUP 0x434a, and is only
 * taken when nothing about the drawing is unusual:
 *
 *   the style byte at 0x3892 is 0 or 1 - no bold, italic, underline or
 *   half-tone; the clip flag at 0x3893 is clear; and the font is one of the
 *   two 1-bit formats.
 *
 * Even inside the fast path a character wider than 8 pixels goes back through
 * `draw_char`, because the driver's entry takes a byte a row.
 *
 * A null string - both halves of the pointer zero - draws nothing.
 *
 * The fast path is the driver entry at 0x434a - VGA:0x124b, `vm_blit_glyph` -
 * and it is reached in earnest: `draw_title_bar` turns the clip box off and
 * leaves it off, which is one of the three conditions on its own.
 */
void draw_string_body(const char far *str, int16_t x, int16_t y)
{
    uint16_t w;                         /* [bp-2] */
    uint16_t h;                         /* [bp-4] */
    int16_t  index;                     /* [bp-6] */
    /* A fixed-width glyph's size in bytes, worked out for the first one
       that needs it and kept for the rest of the string. */
    uint16_t stride = 0;                /* [bp-8] */
    const uint8_t far *glyph;           /* [bp-0xc] */

    /*
     * **The width tested is the previous character's.** `[bp-2]` is seeded
     * with the font's fixed width at 0x38c4 before the loop and the test at
     * the top of each pass reads whatever the last pass left there; only
     * then does the fast branch work out this character's width and store
     * it. For a fixed-width font that makes no difference, and for a
     * proportional one it means a narrow character following a wide one
     * goes to `draw_char` and a wide one following a narrow one goes to the
     * driver - which is how a run of text can be drawn two ways. That is
     * not a reading of the structure; the seed at 0x218f8 is there in the
     * prologue because the first pass has no previous width to use.
     */
    w = g_vmds.font_cell_width[0];

    if (!str)
        return;

    /*
     * The three tests are not all the same kind. The style at 0x3892 is
     * compared with `jle` - **signed**, so a style byte with bit 7 set passes
     * it - the clip flag at 0x3893 is sign-extended with `cbw` before being
     * tested against zero, and the font marker at 0x6176 is compared with
     * `jbe`, unsigned. Written as three unsigned tests they would agree on
     * every value this game uses and disagree on a style of 0x80 or more.
     */
    if ((int8_t)g_vmds.text_style <= 1 && !(int8_t)g_vmds.clip_enabled
        && g_font_kinds.kind[0] <= 1) {
        /*
         * The fast path: a character goes straight to the driver, and one
         * **wider than 8 pixels** falls back to `draw_char`, because the
         * driver's entry takes a byte a row and cannot express more.
         */
        while (*str != 0) {
            if (w > 8)
                x += draw_char(*str, x, y);
            else {
                index = (uint8_t)*str - g_vmds.font_first_char[0];
                if (g_font_widths.width[0] != NULL) {
                    /* Far pointers, as in `draw_char`; see the note there. */
                    w = g_font_slots.slot[0][index];
                    h = g_vmds.font_cell_height[0];
                    glyph = g_font_bodies.body[0]
                            + ((const uint16_t far *)g_font_widths.width[0])[index];
                } else {
                    if (stride == 0) {
                        w = g_vmds.font_cell_width[0];
                        h = g_vmds.font_cell_height[0];
                        stride = ((w + 7) >> 3) * h;
                    }
                    glyph = g_font_bodies.body[0] + index * stride;
                }
                /* The driver's glyph entry (slot 1, DGROUP 0x434a) takes
                   everything in registers, the row in BP. */
#ifdef __TURBOC__
                asm push bp
                asm push si
                asm push di
                asm mov es, word ptr glyph+2
                asm mov si, word ptr glyph
                asm mov bx, w
                asm mov cx, h
                asm mov dx, x
                asm mov bp, y
                ((vm_glyph_fn)g_vm_driver.entry[VM_SLOT_GLYPH])();
                asm pop di
                asm pop si
                asm pop bp
#else
                vm_blit_glyph(glyph, w, h, x, y);
#endif
                x += w;
            }
            str++;
        }
    } else {
        while (*str != 0) {
            w = draw_char(*str, x, y);
            x += w;
            if (g_vmds.text_style & 2)
                x++;
            str++;
        }
    }
}

/*
 * 0x21a50
 *
 * The width and height of one character of the current font, through two
 * pointers either of which may be null; answers 0 for a character outside
 * the font and 1 otherwise. The width is the proportional table's when the
 * font has one. **Nothing in the image calls it**, like `font_char_width`;
 * it is here because the module has it.
 */
uint16_t glyph_size(register int16_t c, register uint16_t *w,
                    register uint16_t *h)
{
    register uint16_t gw;
    uint16_t gh;

    c -= g_vmds.font_first_char[0];
    if (c < 0 || g_vmds.font_char_count[0] <= c)
        return 0;
    gw = g_font_widths.width[0] != NULL
         ? g_font_slots.slot[0][c] : g_vmds.font_cell_width[0];
    gh = g_vmds.font_cell_height[0];
    if (w)
        *w = gw;
    if (h)
        *h = gh;
    return 1;
}
