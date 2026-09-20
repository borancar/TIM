/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Bitmap and screen files**: the formats a bitmap can be held in
 * and the code that loads and draws each of them.
 *
 * This file corresponds to the original's **code segment 248f**, image
 * 0x248f0..0x26190. The segment was found by looking at where the binary's own
 * far calls land: 140 of them carry the segment 0x248f, and nothing between
 * 0x248f0 and the sound module at 0x26190 is reached any other way. Functions
 * are in address order and each carries the image offset it was read from.
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
 * **The flipped quadtree's state**, DGROUP 0x63f6..0x6400, 0x0a bytes - `bitmaps.c`'s statics
 * below `BITMAPS`, used by nothing outside 0x2493b..0x24f72.
 *
 * `draw_vqt_flipped` sets the two flags from `BITMAPS.draw_flags`; the leaf
 * sets `index_bits` and `palette`. Which flag mirrors which axis is read off
 * the fill loops - `fill_rows_mirror_x`, chosen when `flip_x` alone is set,
 * walks x from the right - and the names are ours.
 */
struct bitmaps_flip_state {
    int16_t   flip_y;             /* +0x00 [2]  bit 0 of the draw flags */
    int16_t   flip_x;             /* +0x02 [2]  bit 1 of the draw flags */
    uint16_t  _pad_63fa;          /* +0x04 [2]  not touched by these routines */
    uint16_t  index_bits;         /* +0x06 [2]  bits per pixel index, or 8 */
    dg_near_t palette;            /* +0x08 [2]  the leaf's palette, in its frame */
} __attribute__((packed));

struct bitmaps_flip_state BITMAPS_FLIP_STATE DGROUP_BSS(0x63f6);
_Static_assert(sizeof(struct bitmaps_flip_state) == 0x0a, "DGROUP 0x63f6..0x6400, 0x0a bytes");
DG_ASSERT_AT(struct bitmaps_flip_state, flip_y,     0x00);
DG_ASSERT_AT(struct bitmaps_flip_state, flip_x,     0x02);
DG_ASSERT_AT(struct bitmaps_flip_state, _pad_63fa,  0x04);
DG_ASSERT_AT(struct bitmaps_flip_state, index_bits, 0x06);
DG_ASSERT_AT(struct bitmaps_flip_state, palette,    0x08);


/*
 * 0x248fe
 *
 * Open the bit reader on a block of data, and answer the record - which is at
 * DGROUP 0x6402 and is the same four words `vqt_node` reads: a 32-bit bit
 * position, then the data as a far pointer.
 *
 * There is only one of them. DGROUP 0x6400 says whether it is in use and a
 * second open answers 0 rather than taking it away from the first.
 */
struct vqt_reader *open_bit_reader(struct far_ptr data)
{
    if (BITMAPS.in_use != 0)
        return VQTRD(0);

    BITMAPS.in_use = 1;
    BITMAPS.data = data;
    BITMAPS.pos = 0;

    /* The address of the eight bytes above, not a handle - `mov ax, 0x6402`
       at 0x2492b. They are a `vqt_reader`'s head - the position and the block
       - and nothing reads past them through this one. */
    return (struct vqt_reader *)(void *)&BITMAPS.pos;
}

/*
 * 0x24930
 *
 * Give the bit reader back.
 */
void close_bit_reader(void)
{
    BITMAPS.in_use = 0;
}

/*
 * 0x2493b
 *
 * **One pixel through the leaf's palette**: read an index through
 * `DG49BA.read_fn` and answer the palette byte it names. The palette is the
 * table `vqt_flip_leaf` read into its own frame and filed at `BITMAPS_FLIP_STATE.palette`;
 * the index is added to that offset as a 16-bit word, `add bx,ax`.
 *
 * Reached only as `BITMAPS.pixel_fn`, which the leaf sets to 0x004b before
 * handing a mirrored fill its rectangle. The name is ours.
 */
uint16_t read_palette_pixel(uint16_t bits)
{
    uint8_t index = (uint8_t)call_bitmap_read(DG49BA.read_fn, bits);

    return VQTPAL(BITMAPS_FLIP_STATE.palette)[index];
}

/*
 * 0x24954
 *
 * **Draw a quadtree bitmap, mirrored as `BITMAPS.draw_flags` says**, and the
 * body `draw_offset_bitmap` calls with the reader already open.
 *
 * Bit 1 of the flags mirrors x and bit 0 mirrors y (`BITMAPS_FLIP_STATE.flip_x`,
 * `flip_y`). The pair also chooses the fill a leaf hands a whole rectangle
 * to: x alone 0x0275, y alone 0x02c4, both 0x0313, neither 0 - and 0 means
 * the leaf plots pixel by pixel itself. The original has a dead `jmp` at
 * 0x249a4 after the x-alone arm.
 *
 * The driver's fill byte at 0x389c is forced to 1 for the walk and put back
 * after, and the walk runs between `cursor_redraw_off` and `cursor_redraw_on`. The
 * byte is saved sign-extended, `cbw`, and restored as its low half.
 *
 * **Unreachable with this game's data**: its one caller is
 * `draw_offset_bitmap`, which no shipped bitmap reaches - see the count beside
 * it. Nothing has run this transcription. The name is ours.
 */
void draw_vqt_flipped(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t saved;                /* [bp-2] */

    BITMAPS_FLIP_STATE.flip_x = (BITMAPS.draw_flags & 2) ? 1 : 0;
    BITMAPS_FLIP_STATE.flip_y = (BITMAPS.draw_flags & 1) ? 1 : 0;

    if (BITMAPS_FLIP_STATE.flip_x != 0)
        BITMAPS.fill_fn = (BITMAPS_FLIP_STATE.flip_y != 0) ? 0x0313 : 0x0275;
    else
        BITMAPS.fill_fn = (BITMAPS_FLIP_STATE.flip_y != 0) ? 0x02c4 : 0;

    saved = (int16_t)(int8_t)VMDS.fill_enabled;
    VMDS.fill_enabled = 1;
    cursor_redraw_off();
    vqt_flip_node(x, y, w, h);
    cursor_redraw_on();
    VMDS.fill_enabled = (uint8_t)saved;
}

/*
 * 0x249ed
 *
 * **One node of the mirrored quadtree**: `vqt_node`'s shape - four bits
 * through `DG49BA.read_fn`, and for each quadrant recurse on a set bit or
 * hand it to `vqt_flip_leaf` on a clear one - with each quadrant's origin
 * moved when an axis is mirrored.
 *
 * The halves are `w >> 1` and `(w + 1) >> 1`, both `sar`, so they are signed.
 * Unmirrored, the narrow quadrants sit at x and the wide ones at `x + (w >>
 * 1)`; with `flip_x` the wide ones move to x and the narrow ones to `x +
 * ((w + 1) >> 1)`. y the same with `flip_y`. The bits are 8, 4, 2, 1 for the
 * quadrants (narrow, short), (wide, short), (narrow, tall), (wide, tall).
 *
 * Two things differ from `vqt_node` and are the original's: **either
 * dimension being 0 ends the node**, not both; and like `vqt_screen_node`,
 * only the first quadrant's leaf redraws the cursor after it.
 *
 * Unreachable with this game's data; see `draw_vqt_flipped`. The name is ours.
 */
void vqt_flip_node(int16_t x, int16_t y, int16_t w, int16_t h)
{
    /* The quadrant's two widths and two heights, and where each begins.
       Not halves of anything: `_narrow` and `_wide` are `w >> 1` and
       `(w + 1) >> 1`, which `flip_x` swaps the origins of. */
    int16_t w_narrow, w_wide;     /* [bp-2], [bp-4] */
    int16_t h_short, h_tall;      /* [bp-6], [bp-8] */
    int16_t x_narrow, x_wide;     /* [bp-0xa], [bp-0xe] */
    int16_t y_short, y_tall;      /* [bp-0xc], [bp-0x10] */
    uint8_t code;                 /* [bp-0x11] */

    if (w == 0)
        return;
    if (h == 0)
        return;

    y_short = 0;
    x_narrow = 0;
    w_narrow = x_wide = (int16_t)(w >> 1);
    w_wide = (int16_t)((int16_t)(w + 1) >> 1);
    h_short = y_tall = (int16_t)(h >> 1);
    h_tall = (int16_t)((int16_t)(h + 1) >> 1);

    if (BITMAPS_FLIP_STATE.flip_x != 0) {
        x_narrow = w_wide;
        x_wide = 0;
    }
    if (BITMAPS_FLIP_STATE.flip_y != 0) {
        y_short = h_tall;
        y_tall = 0;
    }

    code = (uint8_t)call_bitmap_read(DG49BA.read_fn, 4);

    if (code & 8) {
        vqt_flip_node((int16_t)(x + x_narrow), (int16_t)(y + y_short), w_narrow, h_short);
    } else {
        vqt_flip_leaf((int16_t)(x + x_narrow), (int16_t)(y + y_short), w_narrow, h_short);
        redraw_cursor(VMDS.page_front_ptr);
    }

    if (code & 4)
        vqt_flip_node((int16_t)(x + x_wide), (int16_t)(y + y_short), w_wide, h_short);
    else
        vqt_flip_leaf((int16_t)(x + x_wide), (int16_t)(y + y_short), w_wide, h_short);

    if (code & 2)
        vqt_flip_node((int16_t)(x + x_narrow), (int16_t)(y + y_tall), w_narrow, h_tall);
    else
        vqt_flip_leaf((int16_t)(x + x_narrow), (int16_t)(y + y_tall), w_narrow, h_tall);

    if (code & 1)
        vqt_flip_node((int16_t)(x + x_wide), (int16_t)(y + y_tall), w_wide, h_tall);
    else
        vqt_flip_leaf((int16_t)(x + x_wide), (int16_t)(y + y_tall), w_wide, h_tall);
}

/*
 * 0x24b65
 *
 * **Fill a rectangle pixel by pixel, x from the right**: x from `x1 - 1` down
 * to `x0`, and for each, y from `y0` up to `y1 - 1`. A colour comes through
 * `BITMAPS.pixel_fn` with `BITMAPS_FLIP_STATE.index_bits`, and goes to `DG49BA.plot_fn`
 * unless it is 0 and `BITMAPS.plot_zero` is clear.
 *
 * One of three written out rather than shared, which differ only in which
 * axis counts down; this is `fill_fn` 0x0275, for `flip_x` alone. Every
 * compare is signed. Unreachable with this game's data. The name is ours.
 */
void fill_rows_mirror_x(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t colour;               /* [bp-1] */
    int16_t xi, yi;               /* di, si */

    for (xi = (int16_t)(x1 - 1); xi >= x0; xi--) {
        for (yi = y0; yi < y1; yi++) {
            colour = (uint8_t)call_bitmap_read(BITMAPS.pixel_fn, BITMAPS_FLIP_STATE.index_bits);
            if (colour != 0 || BITMAPS.plot_zero != 0)
                call_bitmap_plot(DG49BA.plot_fn, xi, yi, colour);
        }
    }
}

/*
 * 0x24bb4
 *
 * `fill_rows_mirror_x`'s twin for `flip_y` alone, `fill_fn` 0x02c4: x from
 * `x0` up, and y from `y1 - 1` down to `y0`. Unreachable with this game's
 * data. The name is ours.
 */
void fill_rows_mirror_y(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t colour;               /* [bp-1] */
    int16_t xi, yi;               /* di, si */

    for (xi = x0; xi < x1; xi++) {
        for (yi = (int16_t)(y1 - 1); yi >= y0; yi--) {
            colour = (uint8_t)call_bitmap_read(BITMAPS.pixel_fn, BITMAPS_FLIP_STATE.index_bits);
            if (colour != 0 || BITMAPS.plot_zero != 0)
                call_bitmap_plot(DG49BA.plot_fn, xi, yi, colour);
        }
    }
}

/*
 * 0x24c03
 *
 * The third, for both mirrored, `fill_fn` 0x0313: x from `x1 - 1` down and y
 * from `y1 - 1` down. Unreachable with this game's data. The name is ours.
 */
void fill_rows_mirror_xy(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    uint8_t colour;               /* [bp-1] */
    int16_t xi, yi;               /* di, si */

    for (xi = (int16_t)(x1 - 1); xi >= x0; xi--) {
        for (yi = (int16_t)(y1 - 1); yi >= y0; yi--) {
            colour = (uint8_t)call_bitmap_read(BITMAPS.pixel_fn, BITMAPS_FLIP_STATE.index_bits);
            if (colour != 0 || BITMAPS.plot_zero != 0)
                call_bitmap_plot(DG49BA.plot_fn, xi, yi, colour);
        }
    }
}

/*
 * 0x24c55
 *
 * **The mirrored quadtree's leaf**: paint one rectangle from what the bit
 * stream says next.
 *
 * Either dimension 0 paints nothing. A 1 by 1 leaf is one colour read with 8
 * bits and plotted through `DG49BA.plot_fn`, skipped if 0 and `plot_zero` is
 * clear. Anything larger starts with a palette size:
 *
 *   - `bits` is enough to count the pixels - the bit length of `area - 1` when
 *     the area is under 256, and 8 otherwise - and `n` is read with it;
 *   - `BITMAPS_FLIP_STATE.index_bits` is the bit length of `n`, and then `n` is a count;
 *   - if `index_bits * area + 8 * n` is **not** less than `8 * area`, a
 *     palette would not pay, and every pixel is 8 bits raw. The compare is
 *     unsigned 32-bit; the product is `long_multiply_2`, and `8 * area` is the
 *     shift at 0x0be41, which is `long_shift_left` entered with CL already 3.
 *
 * The raw case, and the palette case once its table is read, hand the
 * rectangle to `BITMAPS.fill_fn` if a mirror is set, and otherwise walk x
 * outer and y inner, reading with `vqt_read_bits` **directly** and plotting
 * with `plot_pixel_clipped` **directly** - not through the pointers, and with
 * no `plot_zero` test. A palette of one colour fills the rectangle through
 * `DG49BA.fill_fn` instead, having set both of the driver's colour bytes.
 *
 * `sub sp,0x110`: the palette is read into the bottom of the frame and its
 * address filed at `BITMAPS_FLIP_STATE.palette` for `read_palette_pixel` to index, so the
 * frame is the guest's. Unreachable with this game's data; see
 * `draw_vqt_flipped`. The name is ours.
 */
void vqt_flip_leaf(int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t frame = dg_alloca(0x110);  /* [bp-0x110] the palette */
    int16_t x1, y1;               /* [bp-4], [bp-2] */
    uint32_t area;                /* [bp-8], [bp-6] */
    uint32_t sum;
    int16_t n;                    /* [bp-0xe] */
    uint16_t bits;                /* [bp-0x10] */
    uint8_t *at;                  /* [bp-0xc], a cursor into the palette */
    uint8_t colour;               /* [bp-9] */
    uint8_t al;
    int16_t xi, yi;               /* di, si */

    if (w == 0)
        goto out;
    if (h == 0)
        goto out;

    if (w == 1 && h == 1) {
        colour = (uint8_t)call_bitmap_read(DG49BA.read_fn, 8);
        if (colour == 0 && BITMAPS.plot_zero == 0)
            goto out;
        call_bitmap_plot(DG49BA.plot_fn, x, y, colour);
        goto out;
    }

    area = (uint32_t)(uint16_t)w * (uint16_t)h;      /* `mul`, unsigned */

    bits = 8;
    if ((area >> 8) == 0) {                          /* DX and AH both zero */
        bits = 0;
        al = (uint8_t)((uint8_t)area - 1);
        while (al != 0) {
            bits++;
            al >>= 1;
        }
    }

    n = (int16_t)(uint8_t)call_bitmap_read(DG49BA.read_fn, bits);

    BITMAPS_FLIP_STATE.index_bits = 0;
    al = (uint8_t)n;
    while (al != 0) {
        BITMAPS_FLIP_STATE.index_bits++;
        al >>= 1;
    }
    n++;

    sum = long_multiply_2((uint32_t)(int32_t)(int16_t)BITMAPS_FLIP_STATE.index_bits, area)
          + (uint32_t)(int32_t)(int16_t)(n << 3);

    if (sum >= long_shift_left(area, 3)) {
        x1 = (int16_t)(x + w);
        y1 = (int16_t)(y + h);

        if (BITMAPS.fill_fn != 0) {
            BITMAPS_FLIP_STATE.index_bits = 8;
            BITMAPS.pixel_fn = DG49BA.read_fn;
            call_bitmap_fill(BITMAPS.fill_fn, x, y, x1, y1);
            goto out;
        }

        for (xi = x; xi < x1; xi++) {
            for (yi = y; yi < y1; yi++) {
                colour = (uint8_t)vqt_read_bits(8);
                if (colour != 0)
                    (void)plot_pixel_clipped(xi, yi, colour);
            }
        }
        goto out;
    }

    if (n == 1) {
        colour = (uint8_t)call_bitmap_read(DG49BA.read_fn, 8);
        VMDS.fill_colour = colour;
        VMDS.second_colour = colour;
        if (colour == 0 && BITMAPS.plot_zero == 0)
            goto out;
        call_bitmap_fill_rect(DG49BA.fill_fn, x, y, w, h);
        goto out;
    }

    at = VQTPAL(frame);
    BITMAPS_FLIP_STATE.palette = frame;
    while (--n >= 0) {
        *at = (uint8_t)call_bitmap_read(DG49BA.read_fn, 8);
        at++;
    }

    x1 = (int16_t)(x + w);
    y1 = (int16_t)(y + h);

    if (BITMAPS.fill_fn != 0) {
        BITMAPS.pixel_fn = 0x004b;
        call_bitmap_fill(BITMAPS.fill_fn, x, y, x1, y1);
        goto out;
    }

    for (xi = x; xi < x1; xi++) {
        for (yi = y; yi < y1; yi++) {
            uint8_t index = (uint8_t)vqt_read_bits(BITMAPS_FLIP_STATE.index_bits);

            colour = VQTPAL(BITMAPS_FLIP_STATE.palette)[index];
            if (colour != 0)
                (void)plot_pixel_clipped(xi, yi, colour);
        }
    }

out:
    dg_free(0x110);
}

/*
 * 0x24e9a
 *
 * Draw a bitmap held through the "BMP:OFF:" offset table: open the bit reader
 * on the header's pixel block, choose how pixels are plotted, and hand the
 * rectangle to `draw_vqt_flipped` with `mode` as its mirror flags.
 *
 * **Unreachable with this game's data, and counted rather than assumed.**
 * `load_bitmaps` looks for "BMP:SCN:" first and takes the compressed form when
 * it is there; only when it is absent does it look for "BMP:OFF:", set the
 * 0xffff marker this draws, and then *require* a "BMP:VQT:" chunk. Across the
 * 162 extracted resources: 58 files carry OFF: and **all 58 carry SCN: as
 * well**, so the compressed branch always wins, and VQT: appears in **none**,
 * so the offset branch would fail before it drew anything even if it were
 * taken. RLE: is in one file, PARTBIN.BMP, and SCL: in none. Nothing has run
 * this transcription.
 *
 * **What the reader is opened on is read off the instructions, not
 * interpreted.** The header's pointer is segment first. The segment is
 * normalised - `seg + (off >> 4)` - and the remainder `off & 0xf` is computed
 * and stored at `[bp-4]`, which nothing reads. Then `cwd` sign-extends the
 * normalised segment into DX, and the pushes are the segment and then DX. The
 * last push is the first argument, and `open_bit_reader` at 0x24912..0x2491b
 * files its first word at 0x6406 - `data.off`, which `vqt_read_bits` adds the
 * byte position to - and its second at 0x6408, `data.seg`. So the block is
 * read from `seg:0000`, or `seg:ffff` for a segment of 0x8000 and above, and
 * the 0..15 bytes of remainder are dropped. Whether the original meant that
 * is not something the instructions say; this does what they do.
 *
 * The plot pointer is the driver's own plot - the far pointer at DGROUP
 * 0x439e, `DG4342.font[22]` - when the whole bitmap is inside the clip box,
 * tested signed and inclusive, and `plot_pixel_clipped` otherwise, with the
 * clip switched on. The `0x1c25` written there is a segment immediate and so
 * a relocation: the program's own base plus 0x1c25. Colour 0 is never
 * plotted, `plot_zero` being cleared here and nowhere set.
 *
 * The clip switch and the driver's two colour bytes - which a one-colour leaf
 * overwrites - are saved first and put back on every path, including an
 * `open_bit_reader` that refuses, which also leaves `BITMAPS.reader_ptr` 0.
 */
void draw_offset_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    uint8_t saved_second = VMDS.second_colour;    /* [bp-6] */
    uint8_t saved_fill = VMDS.fill_colour;        /* [bp-7] */
    uint8_t saved_clip = VMDS.clip_enabled;       /* [bp-5] */
    uint16_t seg;                                   /* [bp-0xa], [bp-2] */
    uint16_t rem;                                   /* [bp-4], never read */
    int16_t w, h;                                   /* si, di */

    seg = (uint16_t)(bmp->data.seg + (bmp->data.off >> 4));
    rem = (uint16_t)(bmp->data.off & 0xf);
    (void)rem;

    BITMAPS.reader_ptr = dg_near(dgroup, open_bit_reader((struct far_ptr){
        (uint16_t)(((int16_t)seg < 0) ? 0xffff : 0),     /* DX after `cwd` */
        seg }));

    if (BITMAPS.reader_ptr != 0) {
        w = bmp->width;
        h = bmp->height;

        if (x >= VMDS.clip_left
            && y >= VMDS.clip_top
            && (int16_t)(x + w) <= VMDS.clip_right
            && (int16_t)(y + h) <= VMDS.clip_bottom) {
            DG49BA.plot_fn = DG4342.font[22];
        } else {
            DG49BA.plot_fn = (struct far_ptr){
                0x61fd, (uint16_t)((IMAGE_BASE >> 4) + 0x1c25) };
            VMDS.clip_enabled = 1;
        }

        BITMAPS.plot_zero = 0;
        BITMAPS.draw_flags = mode;
        draw_vqt_flipped(x, y, w, h);
        close_bit_reader();
    }

    VMDS.clip_enabled = saved_clip;
    VMDS.second_colour = saved_second;
    VMDS.fill_colour = saved_fill;
}

/*
 * 0x24f72
 *
 * Load a bitmap file, whichever of four shapes it is in, and answer the list.
 * The start-up calls it for "cp.bmp" and "gp_bord.bmp"; it is the door that
 * `load_bitmap_list` is only one road out of.
 *
 * The chunk names decide, in this order:
 *
 *   "BMP:SCN:"  a screen: the headers are read and every bitmap's field 4 set
 *               to 0xfffe, and the pixels are already where they belong.
 *   "BMP:OFF:"  a table of 32-bit offsets, one a bitmap, into one block read
 *               whole; each header is pointed at its own offset within it. The
 *               field-4 marker here is 0xffff.
 *   "BMP:VQT:"  the quadtree form, decoded by `decode_vqt_list` into a block
 *               the driver sized; marker 0xfffc.
 *   none of them - `load_bitmap_list`, the planar form.
 *
 * And then two more that modify whatever was loaded: "BMP:RLE:" compresses the
 * lot in place at sixteen colours, and "BMP:SCL:" sets field 4 to 0xfffd.
 *
 * The file record is copied aside and put back around the header read, because
 * `read_bmp_info` moves the position and the chunk search afterwards has to
 * start where it did.
 *
 * Any failure frees the list and answers 0; the record is closed only if this
 * routine opened it.
 */
struct bmp_set *load_bitmaps(char *name)
{
    /* **68 bytes each, and `saved_a` was 52.** `copy_file_record` writes 0x43
       into both, so every call ran fifteen bytes past this one - silently,
       because what is above it in the frame is the port's own locals and the
       original's `[bp-0x5e]` to `[bp-0x1a]` is 0x44 whichever end it is
       measured from. AddressSanitizer caught it on the first run of a build
       that had never linked; see the note in CLAUDE.md. */
    uint8_t saved_a[68];                      /* [bp-0x5e] */
    uint8_t saved_b[68];                      /* [bp-0xa2] */
    uint16_t count_at; /* [bp-4]    */
    /* [bp-2]. The guest keeps a word here and reads it back through
       `BMPLIST`; the port keeps the array. `read_bmp_info` fills it in, and
       the two routines that still want the near pointer get it from
       `dg_near`. */
    bmp_ptr_t *list_at;
    /* **One `long`, not two words.** The four bytes the offset table holds
       per bitmap are a 32-bit offset into the block, which the original reads
       into `[bp-0x14]` and then adds with `huge_add`; reading them as two
       words meant putting them back together at the one place they are used.
       The slot is fourteen bytes and only these four are read. */
    int32_t offset_at;    /* [bp-0x14] */
    /* Two more slots the original addresses as `count_at` less a constant
       rather than by name: [bp-0x1a] is the kind and [bp-6] the size
       `vm_bitmap_list_size` writes. 0x8e - 0x16 and 0x8e - 2. */
    int16_t kind_at[3];   /* [bp-0x1a] */
    int16_t size_at;   /* [bp-6]    */

    /*
     * `name` is either an open file record's handle or the address of a
     * filename - the original tells them apart by asking `file_record_valid`
     * whether the number matches one of four `file_ptr`s. A pointer outside
     * guest memory cannot be a handle, so `dg_is_guest` answers that half
     * exactly; measured on 2026-09-09 the test never fires at all, on any of
     * the ten call sites.
     */
    FILE *as_file = dg_is_guest(name) ? (FILE *)name : NULL;
    FILE *di = as_file;
    uint16_t opened = 0;                        /* [bp-8]  */
    uint8_t *block = FAR_NULL_PTR;               /* [bp-0xc], [bp-0xa] */
    uint16_t kind = 0;                          /* [bp-0x1a] */
    uint16_t i;
    uint32_t r;

    list_at = NULL;

    if (as_file == NULL || file_record_valid(as_file) == 0) {
        opened = 1;
        di = open_file_record(name);
        if (di == 0)
            goto fail;
    }

    copy_file_record(saved_a, di);

    if (seek_named_chunk(di, CHUNK2.bmp_scn, 0) != -1) {
        copy_file_record(saved_b, di);
        restore_file_record_from(saved_a);

        if (read_bmp_info(di, &count_at, &list_at) == 0)
            goto fail;

        set_field_4_of_each(0xfffe, list_at);
        restore_file_record_from(saved_b);
        kind = 0;
    } else {
        if (seek_named_chunk(di, CHUNK2.bmp_off, 0) == -1)
            goto planar;

        game_fread((uint8_t *)kind_at, 2, 1, di);
        kind = (uint16_t)kind_at[0];

        restore_file_record_from(saved_a);

        if (read_bmp_info(di, &count_at, &list_at) == 0)
            goto fail;

        set_field_4_of_each(0xffff, list_at);

        if (seek_named_chunk(di, CHUNK2.bmp_vqt, 0) == -1)
            goto fail;
    }

    if (kind == 0) {
        uint32_t size = file_record_size(di);
        block = dos_alloc_bytes(size, 0, 0).ptr;
        if (block == FAR_NULL_PTR)
            goto fail;

        read_far(block, (int32_t)size, di);

        if (seek_named_chunk(di, CHUNK2.bmp_off_b, 0) == -1) {
            dos_free_far(block);
            goto fail;
        }

        for (i = 0; i < count_at; i++) {
            struct bitmap *si;

            if (game_fread((uint8_t *)&offset_at, 4, 1, di) != 1) {
                dos_free_far(block);
                goto fail;
            }

            /* `huge_add` answers the normalised pair, which is `far_of`'s. */
            si = BMP_PTR(list_at[i]);
            si->data = far_to_rev(far_of(block + offset_at));
        }
    } else {
        /* The four bytes `huge_add_to` steps; each header files the
           normalised pair, which is what that call leaves in them. */
        uint8_t *fp2;
        r = vm_bitmap_list_size(list_at,
                                (uint8_t *)&size_at);
        block = dos_alloc_bytes(r, 0, 0).ptr;
        if (block == FAR_NULL_PTR)
            goto fail;

        set_field_4_of_each(0xfffc, list_at);

        fp2 = block;

        for (i = 0; i < count_at; i++) {
            struct bitmap *si = BMP_PTR(list_at[i]);

            si->data = far_to_rev(far_of(fp2));

            fp2 += (uint16_t)(si->width * si->height);
        }

        decode_vqt_list(di, list_at);
    }
    goto loaded;

planar:
    list_at = load_bitmap_list((char *)di)->bmp_ptr;

loaded:
    count_at = count_list(list_at);

    if (seek_named_chunk(di, CHUNK2.bmp_rle, 0) != -1)
        compress_bitmap_list(list_at, 0x10);

    if (seek_named_chunk(di, CHUNK2.bmp_scl, 0) != -1)
        set_field_4_of_each(0xfffd, list_at);

    goto out;

fail:
    free_bitmaps_thunk(list_at);
    list_at = NULL;

out:
    if (opened != 0)
        close_file_record(di);

    return list_at != NULL ? (struct bmp_set *)list_at : BMPSET_NONE;
}

/*
 * 0x252b4
 *
 * Walk a bitmap list and write the same word into every header's `mask_off`,
 * which is the +4 the original writes and the field a loader leaves its
 * marker in.
 *
 * The array is the second argument and the word the first, which is the order
 * the compiler pushed them and not the order it reads them.
 */
void set_field_4_of_each(uint16_t value, bmp_ptr_t * list)
{
    bmp_ptr_t *p = list;
    struct bitmap *hdr;

    while ((hdr = BMP_PTR(*p)) != BMP_NONE) {
        hdr->mask_off = value;
        p++;
    }
}

/*
 * 0x252d0
 *
 * A thunk from this module into `free_bitmaps` in segment 1c25, which is a
 * `push`, an `lcall` and nothing else. It exists because the two are different
 * translation units and the call has to be far.
 */
void free_bitmaps_thunk(bmp_ptr_t * list)
{
    free_bitmaps(list);
}

/*
 * 0x252e0
 *
 * Count the entries in a null-terminated array of words. A null array answers
 * 0 without looking at it, which is what the test before the loop is for.
 *
 * The array is what the routine is handed, so it is spelled as one. The
 * original's test is `list == 0` on the offset, and offset 0 is `dgroup`
 * itself rather than a C null pointer - `dg_near` answers 0 for both, which is
 * why the guard is written through it and not as `list == NULL`.
 */
uint16_t count_list(bmp_ptr_t * list)
{
    uint16_t n = 0;

    if (list == NULL || list == BMPLIST(0))
        return 0;

    while (BMP_PTR(list[n]) != BMP_NONE)
        n++;

    return n;
}

/*
 * 0x25300
 *
 * Draw one bitmap, choosing how by the marker its loader left in `mask_off`.
 *
 * **It takes the header, not its offset.** The guest pushes one word and the
 * original does `BMP_PTR(hdr)->` throughout; what that word names is a
 * `struct bitmap`, so the port takes one and the four routines below it do
 * too. `BMPP` is the offset a caller still holds turned into it.
 *
 * The header's far pointer is normalised first - paragraphs out of the offset
 * and into the segment - and *written back*, so a bitmap drawn twice is
 * normalised once. Then:
 *
 *   0xfffd  scaled, through the driver at VGA:0x271b, and with three arguments
 *           rather than four
 *   0xfffe  compressed, by 0x20185 - the form `compress_bitmap_list` writes
 *   0xffff  the offset-table form, by 0x24e9a
 *   other   plain planar, through the driver's structured blit at VGA:0x1707 -
 *           and "other" is not a fall-through for the unexpected, it is the
 *           ordinary case: an uncompressed bitmap's `mask_off` holds the
 *           offset of its mask, which is a small number and not a marker at
 *           all.
 */
void draw_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    bmp->data = far_normalise_rev(bmp->data);

    switch (bmp->mask_off) {
    case 0xfffd:
        blit_scaled_thunk(bmp, x, y);
        return;
    case 0xfffe:
        draw_compressed_bitmap(bmp, x, y, mode);
        return;
    case 0xffff:
        draw_offset_bitmap(bmp, x, y, mode);
        return;
    default:
        blit_bitmap_thunk(bmp, x, y, mode);
        return;
    }
}

/*
 * 0x253e7
 *
 * Load a screen - a whole 320x200 image rather than a sprite - and paint it.
 *
 * A "SCR:VQT:" chunk means the quadtree form: the file is read whole into a
 * block from DOS, the bit reader is opened on it, the cursor is pinned so it
 * does not smear as the picture arrives, and `vqt_screen_node` paints the lot
 * as one node covering 0x140 by 0xc8. Then the cursor is released and the
 * reader closed.
 *
 * Without that chunk it falls back to `load_screen_plain` and the file record's
 * position is put back first, which is why the record was copied aside before
 * the chunk search.
 *
 * Answers -1 on any failure, and the block is freed on every path - the
 * picture is in video memory by then, not in it.
 */
uint16_t load_screen(char *name)
{
    uint8_t saved[78];                    /* [bp-0x4e] */

    FILE *si = (FILE *)name;          /* a handle, or a name to open */
    uint16_t opened = 0;                    /* [bp-2]  */
    uint8_t *block = FAR_NULL_PTR;           /* [bp-6], [bp-4] */
    uint16_t di = 0;

    if (file_record_valid(si) == 0) {
        opened = 1;
        si = open_file_record(name);
        if (si == 0) {
            di = 0xffff;
            goto out;
        }
    }

    copy_file_record(saved, si);

    if (seek_named_chunk(si, CHUNK2.scr_vqt, 0) == -1) {
        restore_file_record_from(saved);
        di = load_screen_plain((char *)si);
        goto close;
    }

    {
        uint32_t size = file_record_size(si);
        block = dos_alloc_bytes(size, 0, 0).ptr;
        if (block == FAR_NULL_PTR) {
            di = 0xffff;
            goto out;
        }

        read_far(block, (int32_t)size, si);
    }

    /* The reader files the pair; a block DOS handed out starts a segment. */
    BITMAPS.reader_ptr = dg_near(dgroup, open_bit_reader(far_of(block)));
    if (BITMAPS.reader_ptr == 0) {
        di = 0xffff;
        goto out;
    }

    cursor_redraw_off();
    vqt_screen_node(0, 0, 0x140, 0xc8);
    cursor_redraw_on();
    close_bit_reader();

close:
    if (opened != 0)
        close_file_record(si);

out:
    if (block != FAR_NULL_PTR)
        dos_free_far(block);
    return di;
}

/*
 * 0x2551a
 *
 * Read a 32-bit count of bytes from a file into a far destination, through a
 * bounce buffer, because the read below it takes a **** buffer and a
 * 16-bit count.
 *
 * The buffer is as big as the near heap will give it: it asks for 0x4000 and
 * halves down to 0x800, then steps down by 0x100 at a time, and if even that
 * fails it falls back to 0x100 bytes of its own stack. So a machine with a full
 * heap reads in small pieces rather than failing.
 *
 * The destination is renormalised every 0x10000/buffer reads - which is what
 * the divide at the top is for - by adding a whole segment to the pointer and
 * starting the offset again, so a destination longer than 64 KB is written
 * without the offset ever wrapping.
 *
 * A short read ends it, whatever the count still says. A **** routine.
 */
void read_far(uint8_t far *dst, int32_t count, FILE *file)
{
    /* The only slot of this frame that is not already a C local below - the
       other ten bytes are `buf`, `per_segment`, `left_in_segment` and the two
       walk words, each carrying its own `[bp-N]`. */
    _Alignas(2) uint8_t fallback[0x100];        /* [bp-0x10a] */

    /* [bp-6], a heap block or `fallback`. The heap hands its blocks out
       `volatile`, which this one is not: it is a private bounce buffer, read
       and written by this routine alone between the `game_fread` that fills
       it and the `far_copy` that empties it. The cast drops the qualifier
       once, here, rather than carrying it through both calls. */
    uint8_t *  buf;
    int16_t si = 0x4000;
    int16_t per_segment;                /* [bp-8]   */
    int16_t left_in_segment;            /* [bp-0xa] */
    /* [bp-4], [bp-2]: the cursor the copy steps by the offset alone, and the
       segment-sized stride `huge_add_to` takes it back to. The two agree
       except where a segment's worth of chunks falls short of 0x10000
       bytes, and then the original leaves the gap and so does this. */
    uint8_t *walk;
    uint8_t *ptr = dst;
    /* One Borland `long`, pushed as [bp+8] and [bp+0xa]: the loop compares
       `si` against it with `cwd / cmp dx,[bp+0xa] / jg / cmp ax,[bp+8] / jbe`,
       which is a signed 32-bit compare and not two word tests. */
    int32_t remaining = count;

    for (;;) {
        if (si == 0)
            break;
        buf = heap_malloc_far((uint16_t)si);
        if (buf != NULL)
            break;
        if (si > 0x800)
            si = (int16_t)(si >> 1);
        else
            si = (int16_t)(si - 0x100);
    }

    if (si == 0) {
        buf = fallback;
        si = 0x100;
    }

    /* Only the *high word* is tested - `xor ax,ax / or ax,[bp+0xa]` - so
       this is not `count >= 0x10000` however much it reads like it. */
    per_segment = ((uint16_t)((uint32_t)count >> 16) != 0)
                  ? (int16_t)long_divide(0x00010000L, si)
                  : 0;
    left_in_segment = per_segment;

    walk = ptr;

    while (remaining != 0) {
        uint16_t want = (uint16_t)(((int32_t)si <= remaining)
                                   ? (uint16_t)si : (uint16_t)remaining);
        uint16_t got = game_fread(buf, 1, want, file);

        if (got == 0)
            break;

        far_copy(walk, buf, got);

        walk += got;
        remaining -= got;

        if (per_segment != 0 && --left_in_segment == 0) {
            /* `huge_add_to` by a segment, on the four bytes the original
               reserves for it. */
            ptr += 0x00010000L;

            left_in_segment = per_segment;
            walk = ptr;
        }
    }

    if (buf != NULL && buf != fallback)
        heap_free_far(buf);
}

/*
 * 0x25639
 *
 * Read a "BMP:VQT:" chunk and decode every bitmap in the list out of it.
 *
 * It buys the biggest buffer it can and then reads the file through it as a
 * sliding window, which is the whole shape of the routine:
 *
 *   - one pass over the list to find the largest single bitmap, because the
 *     buffer has to hold at least that much;
 *   - if the whole file fits in free memory, take the file's size instead and
 *     forget the largest, since nothing will ever have to slide;
 *   - failing that, fall back to the scratch block at DGROUP 0x3576, which is
 *     0x3ab4 bytes and is refused if the largest bitmap will not fit in it.
 *
 * Then, for each bitmap: the record at DGROUP 0x640c is filled in with where
 * the four planes go - each one a quarter of the pixels apart - and where each
 * row starts, and `vqt_node` walks the quadtree that paints it. Afterwards the
 * bits consumed are rounded up to whole bytes, the unread tail of the buffer is
 * slid down to the front, and as much as will fit is read in behind it.
 *
 * The header's far pointer is normalised on the way in - paragraphs out of the
 * offset and into the segment - so a bitmap whose planes cross a segment
 * boundary is addressed the same way as one that does not.
 *
 * A **** routine.
 */
void decode_vqt_list(FILE *file, bmp_ptr_t *list)
{
    /*
     * `sub sp,0x1ca`. The reader record is at the bottom of it and the named
     * locals sit above; both are Borland locals, so the frame is a C array.
     *
     * **The reader record keeps the guest's stack, and one slot is why.**
     * `BITMAPS.reader_ptr = dg_near(dgroup, rd)` files the record's address into
     * a guest word that `vqt_node`, `vqt_screen_node` and `fill_quadrant`
     * fetch back out and write through, so the address has to be one the guest
     * can hold. The frame was a C array for a while and that word then took
     * the distance to somewhere outside guest memory; `dg_near` refuses such a
     * pointer now, which turned a wrong number into an abort and is what made
     * this worth putting back rather than leaving.
     *
     * The original has a defect of its own here and it is a different one: the
     * word outlives the call, holding a stack offset into a frame that has
     * been given back, and `load_screen_vqt` puts the *other* reader in it -
     * the eight-byte singleton at 0x6402, which has no plane table at +0x08
     * and no row table at +0x18 for the leaf to read. Reserving the frame does
     * not fix that and is not meant to.
     *
     * Nothing exercises either. `VQT` does not occur once in the four shipped
     * `RESOURCE.00*` archives - `BMP:` occurs 61 times and `SCR:` once - so
     * the quadtree form is a code path the release has no data for, and the
     * routine is entered zero times by the intro and by all 28 level
     * snapshots. Boran's reading is that it was broken in the original too and
     * that is why it never shipped. The archives are the measured half of
     * that; the rest is a reading and this comment is not evidence for it.
     */
    /* **The reader record is the guest's, and has to be.** Its address is
       filed into `BITMAPS.reader_ptr` for `vqt_node`, `vqt_screen_node` and
       `fill_quadrant` to fetch back out and write through, and a C array has
       no DGROUP address to file. This is `framify.py --in-dgroup`'s shape: the
       original's `sub sp,0x1ca` is reserved and `rd` is a typed pointer into
       it, so the body reads `rd->plane[i].seg` while the bytes stay the
       guest's. The other locals are ordinary C ones - only this slot is
       walled. */
    struct vqt_reader *rd =
        VQTRD(dg_alloca(0x1ca));                      /* [bp-0x1ca] */

    /* [bp-0xa]/[bp-8], the far pointer `huge_add_to` steps - a huge pointer,
       filed back into the reader as the normalised pair that call leaves. */
    uint8_t *cur;
    bmp_ptr_t *at = list;      /* [bp-2]  */
    uint32_t largest = 0;                   /* [bp-0x20] */
    uint32_t free_bytes, file_left;
    uint32_t buffer;                        /* [bp-0x18]/[bp-0x1a] */
    uint8_t *block = FAR_NULL_PTR;           /* [bp-0xe], [bp-0xc] */
    uint16_t index = 0;                     /* [bp-0x12] */
    struct bitmap *si;
    struct bitmap *hdr;

    while ((hdr = BMP_PTR(*at)) != BMP_NONE) {
        uint32_t need = buffer_size_thunk((uint16_t)hdr->width,
                                          (uint16_t)hdr->height)
                        & 0xffffu;

        if (largest < need)
            largest = need;

        at++;
    }

    free_bytes = dos_alloc_bytes(0xffffffffu, 0, 0).bytes;
    file_left = file_record_size(file);
    buffer = free_bytes;

    if (file_left <= free_bytes) {
        buffer = file_left;
        largest = 0;
    }

    if (largest <= buffer) {
        block = dos_alloc_bytes(buffer, 0, 0).ptr;
        if (block == FAR_NULL_PTR)
            goto no_block;
        goto have_block;
    }

no_block:
    if (dg_far_ptr(DG3576.scratch) == FAR_NULL_PTR)
        goto done;
    if (largest > 0x3ab4)
        goto done;

    block = dg_far_ptr(DG3576.scratch);
    buffer = 0x3ab4;

have_block:
    BITMAPS.reader_ptr = dg_near(dgroup, rd);
    rd->pos = 0;
    /* A DOS block starts a segment, and the scratch pointer is kept
       normalised, so the pair filed is the one the original files. */
    rd->data = far_of(block);

    read_far(block, (int32_t)buffer, file);
    file_left -= buffer;

    at = list;

    while ((si = BMP_PTR(*at)) != BMP_NONE) {
        uint32_t used;
        /* **Stepped as a pair, on purpose.** `quarter` goes onto the offset
           and the segment stays put, without renormalising - so each of the
           four values is a different `seg:off` for the same linear address
           and all four are *stored*. That is the one shape a host pointer
           cannot carry, which is why this stays two words rather than
           becoming a `MK_FP`. */
        struct far_ptr plane;
        uint16_t row;
        int16_t i;
        uint32_t quarter;

        plane = far_normalise(far_of_rev(si->data));

        quarter = (uint32_t)(uint16_t)((int16_t)(si->width
                                                 * si->height)
                                       >> 2);

        for (i = 0; i < 4; i++) {
            rd->plane[i] = plane;
            plane.off = (uint16_t)(plane.off + quarter);
        }

        row = 0;
        for (i = 0; si->height > i; i++) {
            rd->row[i] = (int16_t)row;
            row = (uint16_t)(row + si->width);
        }

        vqt_node(0, 0, (uint16_t)si->width, (uint16_t)si->height);

        used = rd->pos;
        used = (uint32_t)long_shift_right((int32_t)(used + 7), 3);

        rd->pos = 0;

        cur = dg_far_ptr(rd->data);

        if (file_left != 0) {
            uint32_t chunk;

            far_copy(cur, cur + (int32_t)used,
                     (uint16_t)((uint16_t)buffer - (uint16_t)used));

            cur += (int32_t)(buffer - used);

            chunk = (used >= file_left) ? file_left : used;
            if (chunk > buffer)
                chunk = buffer;

            read_far(cur, (int32_t)chunk, file);
            file_left -= chunk;
        } else {
            rd->data = far_of(cur + (int32_t)used);
        }

        at++;
        index++;
    }

    if (block != dg_far_ptr(DG3576.scratch))
        dos_free_far(block);

done:
    (void)index;
    dg_free(0x1ca);
}

/*
 * 0x25953
 *
 * **Read `bits` bits** from the reader `BITMAPS.reader_ptr` names, and step its
 * position past them. `DG49BA.read_fn`'s only target.
 *
 * The same read `vqt_node` makes for its four: a word at `data.off + (pos >>
 * 3)`, the offset stepped inside the segment, shifted down by the position's
 * low three bits. The mask is `0xff00 rol bits` with the high byte cleared,
 * which is `(1 << bits) - 1` for the eight counts that make sense and 0 for a
 * count of 0; the rotate is written out so the other counts give what `rol`
 * gives. The position is a 32-bit add of the whole 16-bit count.
 *
 * A **** routine: it reuses BP as the record pointer and restores it. Reached
 * from the mirrored quadtree only, which this game's data never draws. The
 * name is ours.
 */
uint16_t vqt_read_bits(uint16_t bits)
{
    struct vqt_reader *rd = VQTRD(BITMAPS.reader_ptr);
    uint16_t turn = (uint16_t)((bits & 0x1f) % 16);
    uint16_t mask = (uint16_t)(((uint16_t)(0xff00u << turn)
                                | (uint16_t)(0xff00u >> ((16 - turn) & 15)))
                               & 0x00ff);
    uint32_t pos = rd->pos;
    uint16_t word;

    rd->pos = pos + bits;
    word = *(const uint16_t *)(void *)(dg_far_ptr(rd->data) + (pos >> 3));
    return (uint16_t)((word >> (pos & 7)) & mask);
}

/*
 * 0x259a1
 *
 * The quadtree walk again, but for a whole *screen* rather than a bitmap: the
 * same four-bit code, the same halves, the same reader record at DGROUP 0x640c,
 * and `fill_screen_quadrant` where `vqt_node` has its own leaf. The original
 * has the two written out separately rather than sharing one, and the port
 * keeps them apart for the same reason - they are two routines at two
 * addresses.
 *
 * One thing is not symmetric and is not a slip in the reading: **only the first
 * quadrant redraws the cursor** after its fill. The other three do not. That
 * keeps the pointer on top while a screen paints itself in without paying for a
 * redraw at every leaf.
 */
void vqt_screen_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    struct vqt_reader *rd;
    uint16_t code;
    uint32_t pos;

    if ((w | h) == 0)
        return;

    rd = VQTRD(BITMAPS.reader_ptr);
    pos = rd->pos;
    rd->pos = pos + 4;

    /* Four bits at `pos`, read as a word so a nibble can straddle a byte. */
    code = (uint16_t)((*(const uint16_t *)(void *)
                       (dg_far_ptr(rd->data) + (pos >> 3))
                       >> (pos & 7)) & 0x0f);

    if (code & 8) {
        vqt_screen_node(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
    } else {
        fill_screen_quadrant(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
        redraw_cursor(VMDS.page_front_ptr);
    }

    if (code & 4)
        vqt_screen_node((uint16_t)(x + (w >> 1)), y,
                        (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));
    else
        fill_screen_quadrant((uint16_t)(x + (w >> 1)), y,
                             (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));

    if (code & 2)
        vqt_screen_node(x, (uint16_t)(y + (h >> 1)),
                        (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_screen_quadrant(x, (uint16_t)(y + (h >> 1)),
                             (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));

    if (code & 1)
        vqt_screen_node((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                        (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_screen_quadrant((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                             (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
}

/*
 * 0x25aaa
 *
 * **The screen quadtree's leaf: paint one rectangle straight onto the page**
 * from what the bit stream says next. `fill_quadrant`'s arithmetic, byte for
 * byte, with a different destination: not a plane buffer but video memory,
 * one plane to a pixel.
 *
 * A pixel at x goes into the byte `VMDS.row_offset[y] + (x >> 2)` of the page
 * `VMDS.page_dst_ptr`, and the plane is chosen for each write with the
 * Sequencer's map mask - `mov ax,0x102 / shl ah,cl / out dx,ax` with CL the
 * low two bits of x, so plane `1 << (x & 3)`. The three places that write a
 * pixel each spell that out, and so does this.
 *
 * The shape is `fill_quadrant`'s: either dimension 0 paints nothing; a 1 by 1
 * leaf is one 8-bit read written; otherwise an 8-bit `area` of the low bytes,
 * a bit count of at least 1, a palette size stepped as a byte, and the same
 * unsigned 16-bit test choosing raw 8-bit pixels (x outer, y inner) or a
 * palette. The reads are `vqt_read_bits` written out in place, as there.
 *
 * Two things differ. A **one-colour** palette fills each row with one far call
 * through DGROUP 0x436e, `DG4342.font[10]`, the driver's span fill at
 * VGA:0x034f - registers AX the colour in both halves, BX x, CX w, ES:DI the
 * row - stepping DI by 0x50 a row. `vm_init` is the only writer of that table,
 * so this calls `vm_span` directly. And the **palette loop's x test is
 * unsigned**, `jae` at 0x25d89, where every other loop here is signed.
 *
 * The early exits jump to the epilogue before the entry, at 0x25aa2. Reached
 * only through a "BMP:VQT:" chunk, and the game ships none - see the count
 * beside `draw_offset_bitmap` at 0x24e9a, and `vqt_screen_node`, its only
 * caller. Nothing has run this transcription.
 */
void fill_screen_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint8_t palette[0x100];       /* [bp-0x10a] */
    uint16_t area;                /* [bp-6] */
    uint16_t bits;                /* cx */
    uint16_t n;                   /* [bp-4], stepped and counted as a byte */
    uint16_t index_bits;          /* [bp-2] */
    int16_t x1, y1;               /* [bp-8], [bp-0xa] */
    int16_t xi, yi;               /* di, si */
    uint16_t count, i, row, rows;
    uint16_t at;
    uint8_t colour, al;

    if (h == 0)
        return;
    if (w == 0)
        return;

    if (w == 1 && h == 1) {
        colour = (uint8_t)vqt_read_bits(8);
        at = (uint16_t)(VMDS.row_offset[y] + (x >> 2));
        io_out16(PORT_SEQ_INDEX,
                 (uint16_t)(((uint16_t)(uint8_t)(1 << (x & 3)) << 8) | 0x02));
        vga_write((uint16_t)(vga_seg_offset(VMDS.page_dst_ptr) + at), colour);
        return;
    }

    area = (uint16_t)((uint8_t)w * (uint8_t)h);     /* `mul bl` */

    bits = 8;
    if ((area >> 8) == 0) {
        bits = 0;
        al = (uint8_t)((uint8_t)area - 1);
        do {
            bits++;
            al >>= 1;
        } while (al != 0);
    }

    n = vqt_read_bits(bits);

    index_bits = 0;
    al = (uint8_t)n;
    while (al != 0) {
        index_bits++;
        al >>= 1;
    }

    xi = (int16_t)x;
    x1 = (int16_t)(x + w);
    yi = (int16_t)y;
    y1 = (int16_t)(y + h);

    n = (uint16_t)((n & 0xff00) | (uint8_t)(n + 1));   /* `inc byte ptr [bp-4]` */

    if ((uint16_t)(area << 3)
        <= (uint16_t)(area * index_bits + (uint16_t)(n << 3))) {
        do {
            do {
                colour = (uint8_t)vqt_read_bits(8);
                at = (uint16_t)(VMDS.row_offset[(uint16_t)yi]
                                + ((uint16_t)xi >> 2));
                io_out16(PORT_SEQ_INDEX,
                         (uint16_t)(((uint16_t)(uint8_t)(1 << (xi & 3)) << 8)
                                    | 0x02));
                vga_write((uint16_t)(vga_seg_offset(VMDS.page_dst_ptr) + at),
                          colour);
                yi++;
            } while (yi < y1);
            yi = (int16_t)y;
            xi++;
        } while (xi < x1);
        return;
    }

    if ((uint8_t)n == 1) {
        colour = (uint8_t)vqt_read_bits(8);
        row = VMDS.row_offset[y];                   /* di */
        rows = h;                                     /* si */
        do {
            vm_span((uint16_t)((colour << 8) | colour), x, (int16_t)w,
                    MK_FP((uint16_t)VMDS.page_dst_ptr, row));
            row = (uint16_t)(row + 0x50);
        } while (--rows != 0);
        return;
    }

    count = (uint8_t)n;
    i = 0;
    do {
        palette[i++] = (uint8_t)vqt_read_bits(8);
        count = (uint8_t)(count - 1);
    } while (count != 0);                             /* `dec byte ptr [bp-4]` */

    xi = (int16_t)x;
    do {
        do {
            colour = palette[vqt_read_bits(index_bits)];
            at = (uint16_t)(VMDS.row_offset[(uint16_t)yi]
                            + ((uint16_t)xi >> 2));
            io_out16(PORT_SEQ_INDEX,
                     (uint16_t)(((uint16_t)(uint8_t)(1 << (xi & 3)) << 8)
                                | 0x02));
            vga_write((uint16_t)(vga_seg_offset(VMDS.page_dst_ptr) + at),
                      colour);
            yi++;
        } while (yi < y1);
        yi = (int16_t)y;
        xi++;
    } while ((uint16_t)xi < (uint16_t)x1);            /* `jae`, unsigned */
}

/*
 * 0x25d96
 *
 * A far block move, destination first: words then a trailing byte, the odd
 * count carried out of `shr cx,1` in the carry flag.
 *
 * This is the third routine in the port that does this - `far_memcpy` at
 * 0x222c6 and `far_move` at 0x0bd2e are the others - and all three differ in
 * argument order or in which register holds the count. They are separate
 * routines in the original and stay separate here.
 *
 * **Both ends are `far` pointers and the 64K wrap is given up.** `rep movsw`
 * steps SI and DI as 16-bit registers, so both ends wrap inside their segment
 * on the original; a host pointer cannot, and the `far` tag says which kind
 * this is without giving the host that behaviour. The destination held out
 * longest, as a `seg:off` pair with `(uint16_t)(dst_off + i)` doing the wrap
 * explicitly.
 *
 * What makes giving it up sound is the same measurement that settled it for
 * `far_move` and `far_memcpy`: instrumented on 2026-09-09 across the intro to
 * flip 200 and a full level14 run, neither end ever reached
 * `off + count > 0x10000`. For either to wrap it would have to start above
 * 0xC000 in DGROUP with a large count, and the copy would then run into
 * DGROUP from offset 0, which is a bug rather than a behaviour.
 *
 * Under Borland the tag brings the wrap back, because there `far` pointer
 * arithmetic *is* 16-bit offset arithmetic - so the source says the right
 * thing for both compilers and only the host gives the behaviour up.
 */
void far_copy(uint8_t far *dst, const uint8_t far *src,
              uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        dst[i] = src[i];
}

/*
 * 0x25db8
 *
 * One node of the quadtree the "BMP:VQT:" chunk is: read four bits, and for
 * each quadrant either recurse into this again or hand it to `fill_quadrant` to
 * be painted.
 *
 * The bit reader is the record at DGROUP 0x640c: a 32-bit bit position at +0
 * and the data as a far pointer at +4. Four bits are taken and the position
 * advanced, the byte is found by shifting the position right three, a *word* is
 * read from there and shifted down by the position's low three bits - reading a
 * word rather than a byte is what lets a code straddle a byte boundary without
 * any special case.
 *
 * The four halves are `w >> 1` and `(w + 1) >> 1`, so an odd width puts the
 * extra column in the right-hand pair and an odd height the extra row in the
 * bottom pair. Bit 8 of the code is the top-left quadrant, then 4, 2 and 1
 * clockwise - and a set bit means subdivide, a clear one means fill.
 *
 * A zero width *and* height ends the recursion; either alone does not.
 *
 * A **** routine, and it shares the epilogue three bytes above its own
 * entry for the early return.
 */
void vqt_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    struct vqt_reader *rd;
    uint16_t code;
    uint32_t pos;

    if ((w | h) == 0)
        return;

    rd = VQTRD(BITMAPS.reader_ptr);
    pos = rd->pos;
    rd->pos = pos + 4;

    /* Four bits at `pos`, read as a word so a nibble can straddle a byte. The
       offset is stepped inside the segment, which is why `data` stays a pair. */
    code = (uint16_t)((*(const uint16_t *)(void *)
                       (dg_far_ptr(rd->data) + (pos >> 3))
                       >> (pos & 7)) & 0x0f);

    if (code & 8)
        vqt_node(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
    else
        fill_quadrant(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));

    if (code & 4)
        vqt_node((uint16_t)(x + (w >> 1)), y,
                 (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));
    else
        fill_quadrant((uint16_t)(x + (w >> 1)), y,
                      (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));

    if (code & 2)
        vqt_node(x, (uint16_t)(y + (h >> 1)),
                 (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_quadrant(x, (uint16_t)(y + (h >> 1)),
                      (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));

    if (code & 1)
        vqt_node((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                 (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_quadrant((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                      (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
}

/*
 * 0x25eb5
 *
 * **The quadtree's leaf: paint one rectangle of the bitmap** from what the bit
 * stream says next. Every pixel goes into the first plane of the reader record
 * `BITMAPS.reader_ptr` names, at `plane[0].off + row[y] + x` - a 16-bit offset
 * inside the plane's segment - and the other three planes are not touched.
 *
 * Either dimension 0 paints nothing; a 1 by 1 leaf is one byte read and
 * written. Anything larger starts with a palette size:
 *
 *   - `area` is `mul bl`, the **low bytes** of w and h multiplied, an 8-bit
 *     product;
 *   - `bits` is 8 for an area of 256 or more, and otherwise the bit length of
 *     `area - 1` **but at least 1** - this `dec`/`shr` loop has no `je` before
 *     it, where 0x24c55's has;
 *   - `n` is read with `bits`, `index_bits` is the bit length of `n`, and then
 *     `n` is stepped as a **byte**, `inc byte ptr [bp-4]`, so 255 becomes 0;
 *   - if `area * 8` is above `area * index_bits + n * 8`, unsigned 16-bit, a
 *     palette pays. Otherwise every pixel is 8 bits raw, x outer and y inner.
 *
 * A palette of one colour paints the rectangle with it, rows counted down on
 * `h` and each row a `loop` over `w`. A larger one is read into the frame -
 * `[bp-0x10a]`, 0x100 bytes, counted down on the byte `n`, so an `n` that
 * wrapped to 0 reads all 256 - and each pixel is an `index_bits` index into it.
 * The table's address never leaves the routine, so it is a C array.
 *
 * **The reads are 0x25953 written out in place.** The instructions at
 * 0x25f58..0x25f9a and 0x26112..0x26155 are `vqt_read_bits`'s, byte for byte,
 * but for loading the count from CX or `[bp-2]` rather than from its argument,
 * so they are called as it here. The 8-bit reads - 0x25eda, 0x25fe2, 0x26058,
 * 0x260cd - step the position the same way and keep AL without the mask,
 * which is the same byte.
 *
 * The early exits jump to an epilogue **before** the entry, at 0x25ead, and
 * the last one to an epilogue at 0x26190..0x26197. So the routine is 739 bytes,
 * not the 1,853 this comment once gave - and its last eight bytes lie past the
 * 0x26190 this file's header gives as the end of the segment.
 *
 * **Reached only through a "BMP:VQT:" chunk, and the game ships none.** See the
 * count beside `draw_offset_bitmap` at 0x24e9a: zero of the 162 extracted
 * resources carry VQT:, so `vqt_node`, its only caller, is never entered by
 * this data. Nothing has run this transcription.
 */
void fill_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint8_t palette[0x100];       /* [bp-0x10a] */
    struct vqt_reader *rd;
    uint16_t area;                /* [bp-6] */
    uint16_t bits;                /* cx */
    uint16_t n;                   /* [bp-4], stepped and counted as a byte */
    uint16_t index_bits;          /* [bp-2] */
    int16_t x1, y1;               /* [bp-8], [bp-0xa] */
    int16_t xi, yi;               /* di, si */
    uint16_t count, i;            /* cx in the one-colour rows */
    uint8_t colour, al;

    if (h == 0)
        return;
    if (w == 0)
        return;

    if (w == 1 && h == 1) {
        colour = (uint8_t)vqt_read_bits(8);
        rd = VQTRD(BITMAPS.reader_ptr);
        dg_far_ptr(rd->plane[0])[(uint16_t)rd->row[y] + x] = colour;
        return;
    }

    area = (uint16_t)((uint8_t)w * (uint8_t)h);     /* `mul bl` */

    bits = 8;
    if ((area >> 8) == 0) {
        bits = 0;
        al = (uint8_t)((uint8_t)area - 1);
        do {
            bits++;
            al >>= 1;
        } while (al != 0);
    }

    n = vqt_read_bits(bits);

    index_bits = 0;
    al = (uint8_t)n;
    while (al != 0) {
        index_bits++;
        al >>= 1;
    }

    xi = (int16_t)x;
    x1 = (int16_t)(x + w);
    yi = (int16_t)y;
    y1 = (int16_t)(y + h);

    n = (uint16_t)((n & 0xff00) | (uint8_t)(n + 1));   /* `inc byte ptr [bp-4]` */

    if ((uint16_t)(area << 3)
        <= (uint16_t)(area * index_bits + (uint16_t)(n << 3))) {
        do {
            do {
                colour = (uint8_t)vqt_read_bits(8);
                rd = VQTRD(BITMAPS.reader_ptr);
                dg_far_ptr(rd->plane[0])[(uint16_t)rd->row[yi]
                                         + (uint16_t)xi] = colour;
                yi++;
            } while (yi < y1);
            yi = (int16_t)y;
            xi++;
        } while (xi < x1);
        return;
    }

    if ((uint8_t)n == 1) {
        colour = (uint8_t)vqt_read_bits(8);
        yi = (int16_t)y;                              /* dx */
        do {
            xi = (int16_t)x;
            count = w;
            do {
                rd = VQTRD(BITMAPS.reader_ptr);
                dg_far_ptr(rd->plane[0])[(uint16_t)rd->row[yi]
                                         + (uint16_t)xi] = colour;
                xi++;
            } while (--count != 0);                   /* `loop` */
            yi++;
        } while (--h != 0);
        return;
    }

    count = (uint8_t)n;
    i = 0;
    do {
        palette[i++] = (uint8_t)vqt_read_bits(8);
        count = (uint8_t)(count - 1);
    } while (count != 0);                             /* `dec byte ptr [bp-4]` */

    xi = (int16_t)x;
    do {
        do {
            colour = palette[vqt_read_bits(index_bits)];
            rd = VQTRD(BITMAPS.reader_ptr);
            dg_far_ptr(rd->plane[0])[(uint16_t)rd->row[yi]
                                     + (uint16_t)xi] = colour;
            yi++;
        } while (yi < y1);
        yi = (int16_t)y;
        xi++;
    } while (xi < x1);
}

