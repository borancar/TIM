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

/*
 * The driver's own data segment, which it loads from `cs:[0x13a]`. These are
 * NOT DGROUP - the driver is a separate module with its own data - so they are
 * named by their offset within it.
 */
/*
 * Not `static`: tools/verify.py seeds these from the original's own driver
 * data segment before calling a routine, so that both sides are asked the
 * same question. A concession to testability, noted rather than left to be
 * discovered.
 */
uint16_t vga_page_back  = 0xA000;      /* VGA:DS 0x12, being drawn into */
uint16_t vga_page_front = 0xA820;      /* VGA:DS 0x14, on screen */
uint16_t vga_screen_height = 480;      /* VGA:DS 0x6ec, the mode's height */

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
 * The driver's row table, at VGA:DS 0x6f2: the byte offset of the start of
 * each scan line. Indexed by y, so the driver never multiplies. It is built by
 * the driver's own set-up, which is not transcribed yet - until it is,
 * tools/verify.py seeds it from the original's memory.
 */
uint16_t vga_row_offset[512];          /* VGA:DS 0x6f2 */

uint16_t vga_copy_src_seg = 0xA000;    /* VGA:DS 0x16 */
uint16_t vga_copy_dst_seg = 0xA820;    /* VGA:DS 0x18 */

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
    uint16_t di   = (uint16_t)(vga_row_offset[y] + col);
    uint16_t src  = vga_seg_offset(vga_copy_src_seg);
    uint16_t dst  = vga_seg_offset(vga_copy_dst_seg);

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
