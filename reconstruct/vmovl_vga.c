/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the `VGA:` chunk of `VM.OVL`, the video driver of The
 * Incredible Machine (Dynamix / Sierra On-Line, 1993). No licence is asserted:
 * this is derived from someone else's binary.
 *
 * `VM.OVL` is a container of eight per-adapter drivers - VGA, EGA, MCG, CGA,
 * TAN, HEG, EVG, EVA - each compressed. Only the **VGA** one is reconstructed;
 * the other seven are deliberate non-goals. The chunk expands to about 10 KB
 * and the game loads it into a block of 0x2b1 paragraphs, so it is a
 * translation unit of its own and gets a file of its own.
 *
 * **Addresses in this file are offsets within the loaded VGA driver**, written
 * `VM.OVL VGA:0xNNNN`, not image offsets - the loader chooses the segment, so
 * there is no fixed image address to quote. Dump it with
 * tools/dump_overlay.py and disassemble with
 * `tools/disasm.py --file out/res/VM_VGA.mem`.
 *
 * Every pixel the game draws is written by this driver: attributing the A000
 * writes of nine frames to the instructions that made them found 19
 * instructions, all of them here.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * The driver's own data segment, which it loads from `cs:[0x13a]`. These are
 * NOT DGROUP - the driver is a separate module with its own data - so they are
 * named by their offset within it.
 */
/*
 * The driver's data is **not** kept here. It lives inside DGROUP at offset
 * 0x3890 - see dgroup.h - because that is where the original keeps it: the
 * game writes the driver's page segments directly through DGROUP.
 */

/*
 * VM.OVL VGA:0x138e
 *
 * How many bytes a `w` by `h` planar image needs.
 *
 * A row is `w >> 3` bytes plus one, plus another if the width is not a whole
 * number of bytes, and then rounded up to an even count. The unconditional
 * extra byte is not slack: a planar blit at an arbitrary x has to shift the
 * source across a byte boundary, so every row needs one byte more than its
 * pixels occupy.
 *
 * That row count times the height gives a 32-bit product - one `mul`, so
 * unsigned - and the result is shifted left twice for the four planes.
 *
 * The game reaches this through the far pointer at DGROUP 0x435e, which the
 * loader fills in; the thunk at image 0x21ab9 is an `ljmp` through it.
 * Measured: 0x435e held 424b:138e, and 0x424b is the segment the loader chose
 * for the driver in these runs.
 */
uint32_t vm_buffer_size(uint16_t w, uint16_t h)
{
    uint16_t row = (uint16_t)((w >> 3) + 1);

    if ((w & 7) != 0)
        row++;
    if ((row & 1) != 0)
        row++;

    return ((uint32_t)h * row) << 2;
}

/*
 * VM.OVL VGA:0x14c9
 *
 * Plot one pixel.
 *
 * The byte is `row_table[y] + (x >> 3)` in the page being drawn into, and the
 * bit is `0x80 >> (x & 7)` - the leftmost pixel of a byte is the high bit. That
 * mask goes into the Graphics Controller's bit mask register, write mode 2 is
 * selected, and then the byte is **read before it is written**: in write mode 2
 * the low nibble of the written byte is the colour and the latches supply every
 * bit the mask protects, so without the read the other seven pixels of the byte
 * would be destroyed.
 *
 * The bit mask is put back to 0xff on the way out. Write mode 2 is left
 * selected, which is what the rest of the driver expects.
 *
 * There is no clipping here - the caller does it. Reaching this with a y
 * outside the row table reads a word from beyond it and writes somewhere
 * arbitrary in the page.
 *
 * The driver returns no value. AX on return holds 0xff08, the last word sent
 * to the Graphics Controller, and the thunk at 0x2244d passes that back to its
 * own caller - so a caller comparing against -1 can still tell a clipped call
 * from a drawn one, by accident rather than design. Returned here for that
 * reason and for no other.
 */
uint16_t vm_plot_pixel(int16_t x, int16_t y, uint8_t colour)
{
    uint16_t base = vga_seg_offset(vga_page_dst);
    uint16_t di   = (uint16_t)(vga_row_offset(y) + ((uint16_t)x >> 3));
    uint8_t  mask = (uint8_t)(0x80 >> (x & 7));

    io_out16(PORT_GC_INDEX, (uint16_t)(0x08 | (mask << 8)));
    io_out16(PORT_GC_INDEX, 0x0205);      /* write mode 2 */

    vga_read((uint16_t)(base + di));
    vga_write((uint16_t)(base + di), colour);

    io_out16(PORT_GC_INDEX, 0xFF08);
    return 0xFF08;
}

/*
 * VM.OVL VGA:0x150f
 *
 * Make the page just drawn visible and swap the two pages over, then
 * optionally wait out a whole vertical retrace.
 *
 * The two page variables hold **segments**, 0xA000 and 0xA820; the start
 * address the CRTC wants is the segment shifted right by four, and only its
 * low byte is written, to index 0x0C. That is why the page offset is always a
 * multiple of 256 and why the game never writes index 0x0D.
 *
 * `vga_screen_height == 400` takes fifteen paragraphs off the start address. It is
 * never taken in the mode this game runs - the height here is 480, with
 * blanking moved up to 399 - and is transcribed rather than dropped because it
 * is in the original.
 */
void vm_show_page(uint16_t wait_retrace)
{
    uint16_t shown = vga_page_back;
    uint16_t other = vga_page_front;
    vga_page_front = vga_page_back;
    vga_page_back = other;

    uint16_t start = (uint16_t)(shown >> 4);
    if (vga_screen_height == 400)
        start = (uint16_t)(start - 0x0F);

    io_out16(bios_crtc_base(), (uint16_t)(0x0C | ((start & 0xFF) << 8)));

    if (wait_retrace) {
        while (io_in8(PORT_INPUT_ST1) & 0x08)
            ;
        while (!(io_in8(PORT_INPUT_ST1) & 0x08))
            ;
    }
}

/*
 * VM.OVL VGA:0x1561
 *
 * Copy a rectangle from one page to the other, at the same position in both.
 * This is how the double buffer is kept coherent: the region a sprite is about
 * to be drawn over is restored from the page that still holds the clean
 * background.
 *
 * It is done in **write mode 1**, the latch copy: `movsb` reads a byte, which
 * loads all four latches, and writes it, which stores all four - so one byte
 * moved is eight pixels across four planes, and the byte value itself is
 * never looked at. The mode is set to 1 on the way in and back to 2 on the
 * way out, which is the driver's resting mode.
 *
 * The rectangle is widened to byte boundaries first: the left edge rounds down
 * to a multiple of 8 and the right edge up, because a plane byte is eight
 * pixels and there is no partial-byte copy in this mode.
 *
 * The row loop is `dec dx / jne`, so a height of 0 runs 65536 times. That is
 * transcribed as written rather than guarded.
 */
void vm_copy_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    io_out16(PORT_GC_INDEX, 0x0105);

    uint16_t right = (uint16_t)((x + width + 7) & 0xFFF8);
    uint16_t left  = (uint16_t)(x & 0xFFF8);
    uint16_t span  = (uint16_t)((right - left) >> 3);
    uint16_t col   = (uint16_t)(left >> 3);

    uint16_t rows = height;
    uint16_t di   = (uint16_t)(vga_row_offset(y) + col);
    uint16_t src  = vga_seg_offset(vga_page_src);
    uint16_t dst  = vga_seg_offset(vga_page_dst);

    do {
        for (uint16_t i = 0; i < span; i++)
            vga_write((uint16_t)(dst + di + i),
                      vga_read((uint16_t)(src + di + i)));
        di = (uint16_t)(di + 0x50);
    } while (--rows);

    io_out16(PORT_GC_INDEX, 0x0205);
}

/*
 * VM.OVL VGA:0x254, 0x25c
 *
 * Edge masks for a span that does not start or end on a byte boundary. A byte
 * is eight pixels, so a partial byte is written with the graphics controller's
 * bit mask holding these: `left[b]` has the bits from b rightwards, `right[b]`
 * the bits left of b. Transcribed data, and it carries its address for the
 * same reason a routine does.
 */
static const uint8_t MASK_LEFT[8] = {
    0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01
};
static const uint8_t MASK_RIGHT[8] = {
    0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE
};

/*
 * VM.OVL VGA:0x034f
 *
 * Fill `count` pixels of one scan line with a colour, starting at pixel `x`
 * within the row that `dst_off` begins.
 *
 * **Register arguments**, not stack: AL the colour, BX the x, CX the count,
 * ES:DI the row. It is reached through the driver's vector table but it is not
 * a C function, so the C here takes the registers as parameters.
 *
 * Write mode 2 with the bit mask: the byte written carries the colour in its
 * low nibble for all four planes at once, and the bit mask picks which pixels
 * of the byte change. Every write is preceded by a read, which is not for the
 * value - it loads the latches, so the pixels the mask excludes come back
 * unchanged. That read is why a "discarded" read of video memory must never be
 * optimised away.
 *
 * A colour with any of the high four bits set is not a colour at all and goes
 * to a different routine at VGA:0x27a, which is not transcribed yet.
 *
 * The bit mask is left as the last partial byte set it. The original does not
 * restore it and neither does this.
 */
void vm_span(uint16_t ax, uint16_t bx, int16_t cx,
             uint16_t dst_seg, uint16_t di)
{
    uint16_t base = vga_seg_offset(dst_seg);
    uint8_t colour;

    if (cx <= 0)
        return;
    if (ax & 0x00F0) {
        not_transcribed("VM.OVL VGA:0x27a, the high-colour span");
        return;
    }

    di = (uint16_t)(di + (bx >> 3));
    bx &= 7;
    colour = (uint8_t)(ax & 0xFF);

    if ((uint16_t)(bx + cx) < 8) {
        uint8_t mask = (uint8_t)(MASK_LEFT[bx] & MASK_RIGHT[(bx + cx) & 7]);
        io_out16(PORT_GC_INDEX, (uint16_t)(0x08 | (mask << 8)));
        vga_read((uint16_t)(base + di));
        vga_write((uint16_t)(base + di), colour);
        return;
    }

    /* The first, partial byte. */
    cx = (int16_t)(cx - (int16_t)(8 - bx));
    io_out16(PORT_GC_INDEX, (uint16_t)(0x08 | (MASK_LEFT[bx] << 8)));
    vga_read((uint16_t)(base + di));
    vga_write((uint16_t)(base + di), colour);
    di++;

    /* The whole bytes between the two edges. */
    uint16_t remaining = (uint16_t)cx;
    uint16_t whole = (uint16_t)(remaining & 0xFFF8);
    if (whole) {
        whole >>= 3;
        io_out16(PORT_GC_INDEX, 0xFF08);
        while (whole--) {
            vga_read((uint16_t)(base + di));
            vga_write((uint16_t)(base + di), colour);
            di++;
        }
    }

    /* The last, partial byte. */
    if (remaining & 7) {
        io_out16(PORT_GC_INDEX,
                 (uint16_t)(0x08 | (MASK_RIGHT[remaining & 7] << 8)));
        vga_read((uint16_t)(base + di));
        vga_write((uint16_t)(base + di), colour);
    }
}

/*
 * VM.OVL VGA:0x264 (and a second copy at VGA:0x990)
 *
 * One bit per pixel position within a byte. The blitter rotates this along the
 * row rather than recomputing it.
 */
static const uint8_t BIT_MASK[8] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

/*
 * VM.OVL VGA:0x0938
 *
 * Draw a run of pixels along one scan line from a byte-per-pixel source. This
 * is the main blitter: 117,575 calls while the title screen runs, and about
 * 44% of every pixel the game writes.
 *
 * **Register arguments**, and one of them is a *flag*: the routine's first
 * instruction is `jb 0x965`, so the carry flag on entry chooses the direction.
 * Carry clear walks the destination left to right; carry set walks it right to
 * left while the source still advances forwards, which is a horizontal flip.
 * Both occur - 67,312 forward and 5,886 backward while the title screen runs.
 * The direction flag is always clear, so `lodsb` always advances.
 *
 * One pixel per iteration, in write mode 2 with a single-bit mask rotated
 * along the row: `ror ah,1 / adc di,0` moves to the next byte exactly when the
 * bit wraps, with no compare. The graphics controller's *index* is written
 * once outside the loop and only the data port is written per pixel.
 *
 * The source is a scratch buffer in DGROUP, not artwork in a file - the game
 * composes the run first and blits it. Following a blit's source address will
 * land on anonymous memory every time.
 *
 * `loop` decrements CX and tests, so a count of 0 draws 65536 pixels. That is
 * transcribed as written.
 */
void vm_blit_run(uint16_t bx, uint16_t cx, const uint8_t *src,
                 uint16_t dst_seg, uint16_t di, int32_t backwards)
{
    uint16_t base = vga_seg_offset(dst_seg);
    uint16_t byte_col = (uint16_t)(bx >> 3);
    uint8_t mask = BIT_MASK[bx & 7];

    di = (uint16_t)(di + byte_col);

    io_out8(PORT_GC_INDEX, 0x08);
    do {
        io_out8(PORT_GC_DATA, mask);
        vga_read((uint16_t)(base + di));
        vga_write((uint16_t)(base + di), *src++);
        if (!backwards) {
            uint8_t carry = (uint8_t)(mask & 1);
            mask = (uint8_t)((mask >> 1) | (mask << 7));
            di = (uint16_t)(di + carry);
        } else {
            uint8_t carry = (uint8_t)((mask >> 7) & 1);
            mask = (uint8_t)((mask << 1) | (mask >> 7));
            di = (uint16_t)(di - carry);
        }
    } while (--cx);
}

/*
 * VM.OVL VGA:0x0be6
 *
 * Fill a list of horizontal spans with one colour - about 24% of every pixel
 * the game writes, from only 1,078 calls, so this is what paints large areas.
 *
 * The span list is a stream: a first row, a row count, and then one `x1, x2`
 * pair per row. A pair whose `x2` is below its `x1` leaves that row alone,
 * which is how a shape with a concave edge is described.
 *
 * The row table is reached in a way worth spelling out. The instruction is
 * `mov di, [bp+di]`, and **BP-based addressing defaults to SS**, not DS - so
 * the table is read through the *game's* stack segment, which is its DGROUP.
 * The driver keeps the byte distance from DGROUP to its own data segment in
 * `cs:[0x13c]` (0x3890 = (driver DS - DGROUP) * 16) and adds it to 0x6f2, so
 * `SS:(0x6f2 + 0x3890)` is exactly `driverDS:0x6f2` - the same row table
 * VGA:0x1561 uses. Measured: the entry for row 415 is 33,200, which is 415*80.
 *
 * The colour comes from `VGA:DS 0x0d`. A colour with any high nibble bit set
 * takes a different, patterned path at VGA:0x0cd9, which is **not transcribed**
 * - and is never taken: all 1,078 calls on the intro screens pass a colour
 * whose high nibble is zero.
 *
 * Whole bytes go through `rep stosb` with the bit mask at 0xff and **no read**,
 * because with every bit writable the latches cannot contribute. The partial
 * bytes at each end do read first, to load them. That asymmetry is the
 * original's and is transcribed rather than tidied.
 */
void vm_fill_spans(uint16_t spans_seg, uint16_t spans_off)
{
    const uint8_t *spans = FAR_PTR(spans_seg, spans_off);
    uint16_t base = vga_seg_offset(vga_page_dst);
    uint8_t colour = vga_fill_colour;
    uint16_t y, rows;

    io_out16(PORT_GC_INDEX, 0x0205);      /* write mode 2 */
    io_out16(PORT_GC_INDEX, 0xFF08);      /* bit mask: every bit */

    y = (uint16_t)(spans[0] | (spans[1] << 8));
    rows = (uint16_t)(spans[2] | (spans[3] << 8));
    spans += 4;

    if (colour & 0xF0) {
        not_transcribed("VM.OVL VGA:0x0cd9, the patterned span fill");
        return;
    }

    for (;;) {
        uint16_t x1 = (uint16_t)(spans[0] | (spans[1] << 8));
        uint16_t x2 = (uint16_t)(spans[2] | (spans[3] << 8));
        int16_t  w  = (int16_t)(x2 - x1);
        spans += 4;

        if (w >= 0) {
            uint16_t cx = (uint16_t)(w + 1);
            uint16_t di = (uint16_t)(vga_row_offset(y) + (x1 >> 3));
            uint16_t bit = (uint16_t)(x1 & 7);

            if (bit + cx < 8) {
                uint8_t mask = (uint8_t)(MASK_LEFT[bit]
                                         & MASK_RIGHT[(bit + cx) & 7]);
                io_out16(PORT_GC_INDEX, (uint16_t)(0x08 | (mask << 8)));
                vga_read((uint16_t)(base + di));
                vga_write((uint16_t)(base + di), colour);
            } else {
                uint16_t tail;
                cx = (uint16_t)(cx - (8 - bit));
                io_out16(PORT_GC_INDEX,
                         (uint16_t)(0x08 | (MASK_LEFT[bit] << 8)));
                vga_read((uint16_t)(base + di));
                vga_write((uint16_t)(base + di), colour);
                di++;

                tail = (uint16_t)(cx & 7);
                cx >>= 3;
                if (cx) {
                    io_out8(PORT_GC_DATA, 0xFF);
                    while (cx--) {
                        vga_write((uint16_t)(base + di), colour);
                        di++;
                    }
                }
                if (tail) {
                    io_out8(PORT_GC_DATA, MASK_RIGHT[tail]);
                    vga_read((uint16_t)(base + di));
                    vga_write((uint16_t)(base + di), colour);
                }
            }
        }

        if ((int16_t)--rows <= 0)
            break;
        y++;
    }
}

/*
 * VM.OVL VGA:0x0ec1
 *
 * Load `count` colours into the DAC starting at index `first`, from three
 * bytes each of six-bit red, green and blue.
 *
 * It waits for vertical retrace before touching the DAC, which is what stops
 * the palette changing mid-frame and tearing the colours. Then it reads the
 * DAC state register: if the low two bits are not 3 the DAC is part way
 * through a triple, and it writes one byte to nudge it - belt and braces,
 * since the write to the index port that follows resets the component counter
 * anyway.
 *
 * Interrupts are disabled around the transfer, so a handler cannot write the
 * DAC in the middle of a colour.
 *
 * `loop` counts *bytes*, not colours - `count` is tripled on the way in.
 */
void vm_set_palette(const uint8_t *rgb, uint16_t first, uint16_t count)
{
    uint16_t bytes = (uint16_t)(count * 3);

    while (!(io_in8(PORT_INPUT_ST1) & 0x08))
        ;

    /* One read, not two: the original reads the state register once, masks
     * it, and writes that same value back if it is not 3. */
    uint8_t state = (uint8_t)(io_in8(PORT_DAC_READ) & 3);
    if (state != 3)
        io_out8(PORT_DAC_DATA, state);

    io_out8(PORT_DAC_WRITE, (uint8_t)first);
    do {
        io_out8(PORT_DAC_DATA, *rgb++);
    } while (--bytes);
}

/*
 * NOT a transcription: two lines that appear over and over in the routine
 * below, factored out for readability. The original has no such helpers - it
 * repeats the instructions - so they carry no address of their own.
 */
static void line_pixel(uint16_t base, uint16_t di, uint8_t colour)
{
    vga_read((uint16_t)(base + di));
    vga_write((uint16_t)(base + di), colour);
}

/*
 * NOT a transcription either, for the same reason: set the bit mask.
 */
static void line_mask(uint8_t mask)
{
    io_out16(PORT_GC_INDEX, (uint16_t)(0x08 | (mask << 8)));
}

/*
 * VM.OVL VGA:0x0998
 *
 * Draw a line. Reached through the vector at DGROUP 0x434e, with the endpoints
 * in BX,CX to DX,SI and the destination page in ES.
 *
 * Four cases, chosen before any drawing: a single pixel, a horizontal run, a
 * vertical run, and the general one - which splits again into an exact
 * diagonal and the two major axes.
 *
 * The two major-axis cases are **not** Bresenham. They divide once to get a
 * whole and a fractional step - `div` twice, the second with a zero dividend
 * so it divides the remainder scaled by 0x10000 - and then draw a *run* of
 * that many pixels per row, adding the fraction into an accumulator at
 * VGA:DS 0x6c2 and lengthening the run by one whenever it carries. So a
 * shallow line is drawn as horizontal runs, not pixel by pixel.
 *
 * The exact diagonal writes **two** pixels per step - the one to the side and
 * the one below - so the line has no diagonal gaps. That is deliberate and is
 * transcribed as written.
 *
 * The bit mask rotates along the row exactly as in `vm_blit_run`, with the
 * byte pointer advancing when it wraps, and the mask table is a second copy of
 * the one at VGA:0x264, here at VGA:0x990.
 *
 * Every write is preceded by a read that loads the latches; the mask is set
 * with a 16-bit `out` to 0x3ce carrying index 8 and the mask together.
 */
void vm_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    uint16_t base = vga_seg_offset(vga_page_dst);
    uint8_t colour;
    uint8_t mask;
    uint16_t di;
    int16_t bp = 0x50;
    int16_t run, rest;

    /* Both are *stored*, not kept in registers, exactly as the original does. */
    vga_line_colour = vga_second_colour;
    vga_line_mask = BIT_MASK[x1 & 7];
    colour = (uint8_t)vga_line_colour;
    mask = vga_line_mask;
    di = (uint16_t)(vga_row_offset(y1) + (uint16_t)(x1 >> 3));

    if (x1 == x2 && y1 == y2) {                     /* VGA:0x09d5 */
        line_mask(mask);
        line_pixel(base, di, colour);
        return;
    }

    if (x1 == x2) {                                 /* VGA:0x0a23, vertical */
        int16_t n = (int16_t)(y2 - y1);
        if (n <= 0) {
            n = (int16_t)-n;
            bp = (int16_t)-bp;
        }
        line_mask(mask);
        for (;;) {
            line_pixel(base, di, colour);
            di = (uint16_t)(di + bp);
            if (--n < 0)
                return;
        }
    }

    if (y1 == y2) {                                 /* VGA:0x09eb, horizontal */
        int16_t n = (int16_t)(x2 - x1);
        line_mask(mask);
        line_pixel(base, di, colour);
        for (;;) {
            uint8_t carry = (uint8_t)(mask & 1);
            mask = (uint8_t)((mask >> 1) | (mask << 7));
            if (carry)
                di++;
            line_mask(mask);
            line_pixel(base, di, colour);
            if (--n == 0)
                return;
        }
    }

    /* VGA:0x0a50, the general case. */
    {
        int16_t ex = (int16_t)(x2 - x1);
        int16_t ey = (int16_t)(y2 - y1);

        if (ey <= 0) {
            ey = (int16_t)-ey;
            bp = (int16_t)-bp;
        }

        if (ey == ex) {                             /* VGA:0x0a6c, diagonal */
            int16_t n = ex;
            line_mask(mask);
            line_pixel(base, di, colour);
            for (;;) {
                uint8_t carry = (uint8_t)(mask & 1);
                mask = (uint8_t)((mask >> 1) | (mask << 7));
                if (carry) {
                    di++;
                    line_mask(mask);
                    line_pixel(base, di, colour);
                    if (--n == 0)
                        return;
                    di = (uint16_t)(di + bp);
                    vga_read((uint16_t)(base + di));
                    vga_write((uint16_t)(base + di), colour);
                } else {
                    line_mask(mask);
                    line_pixel(base, di, colour);
                    di = (uint16_t)(di + bp);
                    vga_read((uint16_t)(base + di));
                    vga_write((uint16_t)(base + di), colour);
                    if (--n == 0)
                        return;
                }
            }
        }

        if ((uint16_t)ey < (uint16_t)ex) {          /* VGA:0x0ab2, x major */
            rest = ex;
            vga_dda_whole = (uint16_t)((uint16_t)ex / (uint16_t)(ey + 1));
            vga_dda_frac = (uint16_t)(
                ((uint32_t)((uint16_t)ex % (uint16_t)(ey + 1)) << 16)
                / (uint16_t)(ey + 1));
            vga_dda_acc = 0;
            line_mask(mask);
            run = (int16_t)(vga_dda_whole + 1);
            line_pixel(base, di, colour);
            for (;;) {
                vga_dda_saved = rest;
                rest = (int16_t)(rest - run);
                if (rest < 0) {
                    run = vga_dda_saved;
                    rest = 0;
                }
                for (;;) {
                    uint8_t carry = (uint8_t)(mask & 1);
                    mask = (uint8_t)((mask >> 1) | (mask << 7));
                    if (carry)
                        di++;
                    line_mask(mask);
                    line_pixel(base, di, colour);
                    if (--run == 0)
                        break;
                }
                if (rest == 0)
                    return;
                di = (uint16_t)(di + bp);
                vga_read((uint16_t)(base + di));
                vga_write((uint16_t)(base + di), colour);
                {
                    uint32_t sum = (uint32_t)vga_dda_acc + vga_dda_frac;
                    vga_dda_acc = (uint16_t)sum;
                    run = (int16_t)(vga_dda_whole + (sum > 0xFFFF ? 1 : 0));
                }
            }
        }

        /* VGA:0x0b35, y major. */
        rest = ey;
        vga_dda_whole = (uint16_t)((uint16_t)ey / (uint16_t)(ex + 1));
        vga_dda_frac = (uint16_t)(
            ((uint32_t)((uint16_t)ey % (uint16_t)(ex + 1)) << 16)
            / (uint16_t)(ex + 1));
        vga_dda_acc = 0;
        line_mask(mask);
        run = (int16_t)(vga_dda_whole + 1);
        line_pixel(base, di, colour);
        for (;;) {
            vga_dda_saved = rest;
            rest = (int16_t)(rest - run);
            if (rest < 0) {
                run = vga_dda_saved;
                rest = 0;
            }
            for (;;) {
                /* The mask is set again for every pixel of the run, even
                 * though a column cannot change it. The original does that
                 * and the port has to, or the trace is half as long while
                 * the pixels come out identical - which is exactly how this
                 * was found. */
                di = (uint16_t)(di + bp);
                vga_read((uint16_t)(base + di));
                line_mask(mask);
                vga_write((uint16_t)(base + di), colour);
                if (--run == 0)
                    break;
            }
            if (rest == 0)
                return;
            {
                uint8_t carry = (uint8_t)(mask & 1);
                mask = (uint8_t)((mask >> 1) | (mask << 7));
                if (carry)
                    di++;
                line_mask(mask);
                line_pixel(base, di, colour);
            }
            {
                uint32_t sum = (uint32_t)vga_dda_acc + vga_dda_frac;
                vga_dda_acc = (uint16_t)sum;
                run = (int16_t)(vga_dda_whole + (sum > 0xFFFF ? 1 : 0));
            }
        }
    }
}

/*
 * VM.OVL VGA:0x0f15
 *
 * Load a sixteen-colour palette into the DAC and keep a copy of it.
 *
 * A null segment does nothing at all - the routine returns before touching
 * either the DAC or the copy.
 *
 * The copy is the odd part: the same 48 bytes are moved **twice**, into two
 * consecutive 48-byte slots, by rewinding the source pointer by 0x30 between
 * the two `rep movsw`. Sixteen colours of three bytes is exactly 48, so the
 * destination holds two identical palettes side by side.
 */
void vm_load_palette(uint16_t off, uint16_t seg)
{
    uint16_t di = vga_pal_copy_off;
    uint16_t es = vga_pal_copy_seg;
    int32_t i;

    if (seg == 0)
        return;

    vm_set_palette(FAR_PTR(seg, off), 0, 0x10);

    for (i = 0; i < 0x18; i++) {
        FARU16(es, di) = FARU16(seg, off);
        off = (uint16_t)(off + 2);
        di = (uint16_t)(di + 2);
    }
    off = (uint16_t)(off - 0x30);
    for (i = 0; i < 0x18; i++) {
        FARU16(es, di) = FARU16(seg, off);
        off = (uint16_t)(off + 2);
        di = (uint16_t)(di + 2);
    }
}
