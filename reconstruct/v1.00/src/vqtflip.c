/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The bit reader and the mirrored quadtree**: segment 248f's first module,
 * image 0x248fe..0x24e9a. Functions are in address order and each carries
 * the image offset it was read from.

 * **Segment 248f holds four modules, and this is one.** In the medium model
 * each file's code goes into a segment named for it unless it is told
 * otherwise, and these four were told the same name: three C files and the
 * assembly module in vqt.c, in that order. The boundaries are the image's
 * own evidence. Every far call from one routine of the segment to another is
 * `nop / push cs / call` - TLINK's rewrite of a `9A` - where Borland C++
 * calls a routine *defined earlier in the same file* with a bare
 * `push cs / call`, as it does 411 times elsewhere in the image. So
 * `draw_offset_bitmap` is not in `open_bit_reader`'s file, and `draw_bitmap`
 * is not in `draw_offset_bitmap`'s. The data agrees: each file's `_DATA` is
 * its own run of DGROUP.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -G -Z
 * JUDGE: data 0x49ba..0x49c6
 * JUDGE: via-assembler
 * JUDGE: assembler bc3.00
 *
 * **Borland C++ 3.0, `-mm -O -G -Z`, through the assembler.** BC++ 3.0 is
 * the only one of the Borland compilers tried that turns an early `return`
 * into a copy of the epilogue, as `open_bit_reader` has it; `-G` is the
 * `add sp,2` after a call; `-Z` is the register it keeps across statements.
 * `vqt_flip_leaf` counts bits with inline `asm` and reads the count back
 * through `_CX`, so the module went `bcc -B`: the compiler wrote assembly and
 * TASM made the object. The sign of it in the bytes is a `jmp` followed by a
 * `nop` where the compiler could not size a jump across an `asm` block and
 * one-pass TASM shortened it afterwards. Which TASM is not settled; 2.51
 * reproduces it.
 *
 * **Its data**: `_DATA` is `g_min_run` and the three `vqt_` code pointers,
 * DGROUP 0x49ba..0x49c6, and `_BSS` is 0x63f6..0x6414. Borland C++ lays a module's uninitialised variables out in
 * reverse order of declaration, so `g_bitmaps` is declared before
 * `g_bitmaps_flip_state` and sits above it.
 *
 * **Its own records are real pointers** on both compilers: the bit reader,
 * the three code pointers the offset-table draw calls through, and the leaf's
 * palette.
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifndef __TURBOC__
/* The leaf files its own frame's palette in a global for the fill to read:
   the original's, and a warning only on the host. */
#  pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif

/*
 * **The module's statics**, DGROUP 0x6400..0x6414 and 0x63f6..0x6400, in
 * reverse: declared this way round, TCC places them as the image has them.
 *
 * The eight bytes at 0x6402 are the singleton bit reader, and
 * `open_bit_reader` answers their address rather than a handle. They are a
 * `vqt_reader` **truncated after `data`**: no plane table and no row table.
 * That is the defect the quadtree format has - `load_screen` puts this reader
 * in `reader` for `vqt_screen_node` to walk, and the leaf then reads a plane
 * table that is not there. `VQT` occurs zero times in the four shipped
 * archives, which is the measured half of why nobody noticed.
 *
 * The record is declared in dgroup.h, because the assembly module beside
 * this one (vqt.c) walks `walk`. Field names are ours; the offsets are the
 * original's.
 */
struct bitmaps_state g_bitmaps;

/*
 * **The flipped quadtree's state**, 0x63f6..0x6400. `draw_vqt_flipped` sets
 * the two flags from `g_bitmaps.draw_flags`; the leaf sets `index_bits` and
 * `palette`. Which flag mirrors which axis is read off the fill loops -
 * `fill_rows_flip_horizontal`, chosen when `flip_horizontal` alone is set, walks x from the
 * right - and the names are ours. `unused` is a word the module declared and
 * no instruction in the image names (`tools/xrefs.py`).
 */
struct bitmaps_flip_state {
    int16_t   flip_vertical;             /* 0x63f6  bit 0 of the draw flags */
    int16_t   flip_horizontal;             /* 0x63f8  bit 1 of the draw flags */
    uint16_t  unused;             /* 0x63fa */
    int16_t   index_bits;         /* 0x63fc  bits per pixel index, or 8 -
                                     signed: widened with `cwd` */
    uint8_t * palette;            /* 0x63fe  the leaf's palette, in its frame */
};

struct bitmaps_flip_state g_bitmaps_flip_state;

/*
 * **The three code pointers the offset-table bitmap draws through**, and the
 * shortest run worth encoding, DGROUP 0x49ba - this module's initialised
 * data, and the first of it. `g_vqt_read_fn` is a *near* pointer to a routine of
 * this segment, which only this module could have initialised; `g_min_run` is
 * read by `compress_row` in segment 1c25. `draw_offset_bitmap` repoints
 * `g_vqt_plot_fn` before a draw - at the driver's plot when the bitmap is wholly
 * inside the clip box, back at `plot_pixel_clipped` when it is not. Names are
 * ours.
 */
int16_t g_min_run = 6;
bmp_fill_fn g_vqt_fill_fn = fill_rect;
bmp_plot_fn g_vqt_plot_fn = plot_pixel_clipped;
bmp_read_fn g_vqt_read_fn = vqt_read_bits;


/*
 * 0x248fe
 *
 * Open the bit reader on a block of data, and answer the record - the
 * singleton at DGROUP 0x6402, a 32-bit bit position and then the data as a
 * far pointer, the same two fields `vqt_node` reads.
 *
 * There is only one of them. DGROUP 0x6400 says whether it is in use and a
 * second open answers 0 rather than taking it away from the first.
 */
struct vqt_reader *open_bit_reader(uint8_t far *data)
{
    if (g_bitmaps.in_use != 0)
        return 0;
    g_bitmaps.in_use = 1;
    g_bitmaps.reader.data = data;
    g_bitmaps.reader.pos = 0;
    return (struct vqt_reader *)&g_bitmaps.reader;
}

/*
 * 0x24930
 *
 * Give the bit reader back.
 */
void close_bit_reader(void)
{
    g_bitmaps.in_use = 0;
}

/*
 * 0x2493b
 *
 * **One pixel through the leaf's palette**: read an index through
 * `g_vqt_read_fn` and answer the palette byte it names. The palette is the
 * table `vqt_flip_leaf` read into its own frame and filed at
 * `g_bitmaps_flip_state.palette`.
 *
 * Reached only as `g_bitmaps.pixel_fn`, which the leaf points here before
 * handing a mirrored fill its rectangle. The name is ours.
 */
pixel_byte_t near read_palette_pixel(uint16_t bits)
{
    return g_bitmaps_flip_state.palette[(uint8_t)g_vqt_read_fn(bits)];
}

/*
 * 0x24954
 *
 * **Draw a quadtree bitmap, mirrored as `g_bitmaps.draw_flags` says**, and the
 * body `draw_offset_bitmap` calls with the reader already open.
 *
 * Bit 1 of the flags mirrors x and bit 0 mirrors y. The pair also chooses the
 * fill a leaf hands a whole rectangle to: x alone `fill_rows_flip_horizontal`, y
 * alone `fill_rows_flip_vertical`, both `fill_rows_flip_both`, neither none - and
 * none means the leaf plots pixel by pixel itself.
 *
 * The driver's fill byte is forced to 1 for the walk and put back after, and
 * the walk runs between `cursor_redraw_off` and `cursor_redraw_on`. The byte
 * is saved sign-extended, `cbw`, and restored as its low half.
 *
 * **Unreachable with this game's data**: its one caller is
 * `draw_offset_bitmap`, which no shipped bitmap reaches - see the count beside
 * it. Nothing has run this transcription. The name is ours.
 *
 * **Statements, not `?:`**: the flags are set by `if`/`else`, and the fill is
 * chosen by an `if` nested in each arm - the dead `jmp` at 0x249a4 is the
 * outer arm's own exit, left behind by `-O`.
 */
void near draw_vqt_flipped(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t saved;

    if (g_bitmaps.draw_flags & 2)
        g_bitmaps_flip_state.flip_horizontal = 1;
    else
        g_bitmaps_flip_state.flip_horizontal = 0;
    if (g_bitmaps.draw_flags & 1)
        g_bitmaps_flip_state.flip_vertical = 1;
    else
        g_bitmaps_flip_state.flip_vertical = 0;
    if (g_bitmaps_flip_state.flip_horizontal != 0) {
        if (g_bitmaps_flip_state.flip_vertical != 0)
            g_bitmaps.fill_fn = fill_rows_flip_both;
        else
            g_bitmaps.fill_fn = fill_rows_flip_horizontal;
    } else {
        if (g_bitmaps_flip_state.flip_vertical != 0)
            g_bitmaps.fill_fn = fill_rows_flip_vertical;
        else
            g_bitmaps.fill_fn = 0;
    }
    saved = (int8_t)g_vmds.fill_enabled;
    g_vmds.fill_enabled = 1;
    cursor_redraw_off();
    vqt_flip_node(x, y, w, h);
    cursor_redraw_on();
    g_vmds.fill_enabled = (uint8_t)saved;
}

/*
 * 0x249ed
 *
 * **One node of the mirrored quadtree**: `vqt_node`'s shape - four bits
 * through `g_vqt_read_fn`, and for each quadrant recurse on a set bit or
 * hand it to `vqt_flip_leaf` on a clear one - with each quadrant's origin
 * moved when an axis is mirrored.
 *
 * The halves are `w >> 1` and `(w + 1) >> 1`, both `sar`, so they are signed.
 * Unmirrored, the narrow quadrants sit at x and the wide ones at `x + (w >>
 * 1)`; with `flip_horizontal` the wide ones move to x and the narrow ones to `x +
 * ((w + 1) >> 1)`. y the same with `flip_vertical`. The bits are 8, 4, 2, 1 for the
 * quadrants (narrow, short), (wide, short), (narrow, tall), (wide, tall).
 *
 * Two things differ from `vqt_node` and are the original's: **either
 * dimension being 0 ends the node**, not both; and like `vqt_screen_node`,
 * only the first quadrant's leaf redraws the cursor after it.
 *
 * Unreachable with this game's data; see `draw_vqt_flipped`. The name is ours.
 */
void near vqt_flip_node(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t w_narrow;
    int16_t w_wide;
    int16_t h_short;
    int16_t h_tall;
    int16_t x_narrow;
    int16_t y_short;
    int16_t x_wide;
    int16_t y_tall;
    uint8_t code;

    if (w == 0)
        return;
    if (h == 0)
        return;
    x_narrow = y_short = 0;
    x_wide = w_narrow = w >> 1;
    w_wide = (w + 1) >> 1;
    y_tall = h_short = h >> 1;
    h_tall = (h + 1) >> 1;
    if (g_bitmaps_flip_state.flip_horizontal != 0) {
        x_narrow = w_wide;
        x_wide = 0;
    }
    if (g_bitmaps_flip_state.flip_vertical != 0) {
        y_short = h_tall;
        y_tall = 0;
    }
    code = (uint8_t)g_vqt_read_fn(4);
    if (code & 8)
        vqt_flip_node(x + x_narrow, y + y_short, w_narrow, h_short);
    else {
        vqt_flip_leaf(x + x_narrow, y + y_short, w_narrow, h_short);
        redraw_cursor(g_vmds.page_front);
    }
    if (code & 4)
        vqt_flip_node(x + x_wide, y + y_short, w_wide, h_short);
    else
        vqt_flip_leaf(x + x_wide, y + y_short, w_wide, h_short);
    if (code & 2)
        vqt_flip_node(x + x_narrow, y + y_tall, w_narrow, h_tall);
    else
        vqt_flip_leaf(x + x_narrow, y + y_tall, w_narrow, h_tall);
    if (code & 1)
        vqt_flip_node(x + x_wide, y + y_tall, w_wide, h_tall);
    else
        vqt_flip_leaf(x + x_wide, y + y_tall, w_wide, h_tall);
}

/*
 * 0x24b65
 *
 * **Fill a rectangle pixel by pixel, x from the right**: x from `x1 - 1` down
 * to `x0`, and for each, y from `y0` up to `y1 - 1`. A colour comes through
 * `g_bitmaps.pixel_fn` with `g_bitmaps_flip_state.index_bits`, and goes to
 * `g_vqt_plot_fn` unless it is 0 and `g_bitmaps.plot_zero` is clear.
 *
 * One of three written out rather than shared, which differ only in which
 * axis counts down; this is the fill for `flip_horizontal` alone. Every compare is
 * signed. Unreachable with this game's data. The name is ours.
 */
void near fill_rows_flip_horizontal(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t colour;
    int16_t yi;
    int16_t xi;

    for (xi = x1 - 1; xi >= x0; xi--)
        for (yi = y0; yi < y1; yi++)
            if ((colour = (uint8_t)g_bitmaps.pixel_fn(g_bitmaps_flip_state.index_bits)) != 0
                || g_bitmaps.plot_zero != 0)
                g_vqt_plot_fn(xi, yi, colour);
}

/*
 * 0x24bb4
 *
 * `fill_rows_flip_horizontal`'s twin for `flip_vertical` alone: x from `x0` up, and y from
 * `y1 - 1` down to `y0`. Unreachable with this game's data. The name is ours.
 */
void near fill_rows_flip_vertical(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t colour;
    int16_t yi;
    int16_t xi;

    for (xi = x0; xi < x1; xi++)
        for (yi = y1 - 1; yi >= y0; yi--)
            if ((colour = (uint8_t)g_bitmaps.pixel_fn(g_bitmaps_flip_state.index_bits)) != 0
                || g_bitmaps.plot_zero != 0)
                g_vqt_plot_fn(xi, yi, colour);
}

/*
 * 0x24c03
 *
 * The third, for both mirrored: x from `x1 - 1` down and y from `y1 - 1`
 * down. Unreachable with this game's data. The name is ours.
 */
void near fill_rows_flip_both(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t colour;
    int16_t yi;
    int16_t xi;

    for (xi = x1 - 1; xi >= x0; xi--)
        for (yi = y1 - 1; yi >= y0; yi--)
            if ((colour = (uint8_t)g_bitmaps.pixel_fn(g_bitmaps_flip_state.index_bits)) != 0
                || g_bitmaps.plot_zero != 0)
                g_vqt_plot_fn(xi, yi, colour);
}

/*
 * 0x24c55
 *
 * **The mirrored quadtree's leaf**: paint one rectangle from what the bit
 * stream says next.
 *
 * Either dimension 0 paints nothing. A 1 by 1 leaf is one colour read with 8
 * bits and plotted through `g_vqt_plot_fn`, skipped if 0 and `plot_zero` is
 * clear. Anything larger starts with a palette size:
 *
 *   - `bits` is enough to count the pixels - the bit length of `area - 1` when
 *     the area is under 256, and 8 otherwise - and `n` is read with it;
 *   - `g_bitmaps_flip_state.index_bits` is the bit length of `n`, and then `n`
 *     is a count;
 *   - if `index_bits * area + 8 * n` is **not** less than `8 * area`, a
 *     palette would not pay, and every pixel is 8 bits raw. The compare is
 *     unsigned 32-bit.
 *
 * The raw case, and the palette case once its table is read, hand the
 * rectangle to `g_bitmaps.fill_fn` if a mirror is set, and otherwise walk x
 * outer and y inner, reading with `vqt_read_bits` **directly** and plotting
 * with `plot_pixel_clipped` **directly** - not through the pointers, and with
 * no `plot_zero` test. A palette of one colour fills the rectangle through
 * `g_vqt_fill_fn` instead, having set both of the driver's colour bytes.
 *
 * **The two bit lengths are counted in `asm`**, and read back through `_CX`:
 * the area comes from a `mul` the compiler would have made a call to
 * `F_LXMUL@` for, and the second count starts from the palette size still
 * in AX after the read that fetched it. The host has the same arithmetic in
 * C.
 *
 * The palette is read into the bottom of the frame and its address filed at
 * `g_bitmaps_flip_state.palette` for `read_palette_pixel` to index.
 * Unreachable with this game's data; see `draw_vqt_flipped`. The name is ours.
 */
void near vqt_flip_leaf(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t y1;
    int16_t x1;
    uint32_t area;
    uint8_t colour;
    uint8_t *at;
    int16_t n;
    int16_t bits;
    uint8_t palette[256];
    int16_t yi;
    int16_t xi;

    if (w == 0)
        return;
    if (h == 0)
        return;
    if (w == 1 && h == 1) {
        if ((colour = (uint8_t)g_vqt_read_fn(8)) != 0 || g_bitmaps.plot_zero != 0) {
            g_vqt_plot_fn(x, y, colour);
            return;
        }
        return;
    } else {
#ifdef __TURBOC__
    asm mov ax, w
    asm mul word ptr h
    asm mov word ptr area, ax
    asm mov word ptr area+2, dx
    asm mov cx, 8
    asm or dx, dx
    asm jne counted
    asm or ah, ah
    asm jne counted
    asm xor cx, cx
    asm dec al
    asm je counted
count:
    asm inc cx
    asm shr al, 1
    asm jne count
counted:
    bits = _CX;
    g_vqt_read_fn(bits);
    asm xor cx, cx
    asm xor ah, ah
    asm mov n, ax
    asm or al, al
    asm je indexed
index:
    asm inc cx
    asm shr al, 1
    asm jne index
indexed:
    g_bitmaps_flip_state.index_bits = _CX;
#else
    {
        uint8_t al;

        area = (uint32_t)(uint16_t)w * (uint16_t)h;
        bits = 8;
        if ((area >> 8) == 0) {
            bits = 0;
            for (al = (uint8_t)((uint8_t)area - 1); al != 0; al >>= 1)
                bits++;
        }
        n = (uint8_t)g_vqt_read_fn(bits);
        g_bitmaps_flip_state.index_bits = 0;
        for (al = (uint8_t)n; al != 0; al >>= 1)
            g_bitmaps_flip_state.index_bits++;
    }
#endif
    n++;
    if (g_bitmaps_flip_state.index_bits * area + (n << 3) >= area << 3) {
        x1 = x + w;
        y1 = y + h;
        if (g_bitmaps.fill_fn != 0) {
            g_bitmaps_flip_state.index_bits = 8;
            g_bitmaps.pixel_fn = g_vqt_read_fn;
            g_bitmaps.fill_fn(x, y, x1, y1);
            return;
        }
        for (xi = x; xi < x1; xi++)
            for (yi = y; yi < y1; yi++)
                if ((colour = (uint8_t)vqt_read_bits(8)) != 0)
                    plot_pixel_clipped(xi, yi, colour);
        return;
    }
    if (n == 1) {
        if ((g_vmds.second_colour = g_vmds.fill_colour = (uint8_t)g_vqt_read_fn(8)) != 0
            || g_bitmaps.plot_zero != 0) {
            g_vqt_fill_fn(x, y, w, h);
            return;
        }
        return;
    }
    g_bitmaps_flip_state.palette = at = palette;
    while (--n >= 0) {
        *at = (uint8_t)g_vqt_read_fn(8);
        at++;
    }
    x1 = x + w;
    y1 = y + h;
    if (g_bitmaps.fill_fn != 0) {
        g_bitmaps.pixel_fn = read_palette_pixel;
        g_bitmaps.fill_fn(x, y, x1, y1);
        return;
    }
    for (xi = x; xi < x1; xi++)
        for (yi = y; yi < y1; yi++)
            if ((colour = g_bitmaps_flip_state.palette[(uint8_t)vqt_read_bits(
                     g_bitmaps_flip_state.index_bits)]) != 0)
                plot_pixel_clipped(xi, yi, colour);
    }
}
