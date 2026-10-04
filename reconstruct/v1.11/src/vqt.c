/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The quadtree walkers, which were written in assembly.** This file
 * corresponds to the second module of the original's code segment 248f,
 * image 0x25953..0x26198: `vqt_read_bits`, the two node walkers, their two
 * leaves and `far_copy`. It was part of `bitmaps.c` until 2026-09-26; the C
 * module ends at 0x25953 with a compiler's epilogue, and what follows is not
 * a compiler's - `vqt_read_bits` expanded in place as a macro, BP borrowed as
 * a pointer, `add sp,0x10a / pop bp` epilogues, and early exits that jump to
 * an epilogue placed *before* the routine's own entry.
 *
 * **So it is TASM source**, `vqt.asm`, with the host's transcription here.
 * The functions are in address order and each carries the image offset it was
 * read from, as everywhere else.
 *
 * The records it walks are `bitmaps.c`'s: `g_bitmaps.walk`, the reader the
 * bitmap and screen loaders point it at.
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x275dd
 *
 * **Read `bits` bits** from the reader `g_bitmaps.walk` names, and step its
 * position past them. `g_vqt_read_fn`'s only target.
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
uint16_t near vqt_read_bits(uint16_t bits)
{
    struct vqt_reader *rd = g_bitmaps.walk;
    uint16_t turn = (uint16_t)((bits & 0x1f) % 16);
    uint16_t mask = (uint16_t)(((uint16_t)(0xff00u << turn)
                                | (uint16_t)(0xff00u >> ((16 - turn) & 15)))
                               & 0x00ff);
    uint32_t pos = (uint32_t)rd->pos;
    uint16_t word;

    rd->pos = (int32_t)(pos + bits);
    word = (uint16_t)(rd->data[pos >> 3] | (rd->data[(pos >> 3) + 1] << 8));
    return (uint16_t)((word >> (pos & 7)) & mask);
}

/*
 * 0x2762b
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
void near vqt_screen_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    struct vqt_reader *rd;
    uint16_t code;
    uint32_t pos;

    if ((w | h) == 0)
        return;

    rd = g_bitmaps.walk;
    pos = (uint32_t)rd->pos;
    rd->pos = (int32_t)(pos + 4);

    /* Four bits at `pos`, read as a word so a nibble can straddle a byte. */
    code = (uint16_t)(((uint16_t)(rd->data[pos >> 3]
                                  | (rd->data[(pos >> 3) + 1] << 8))
                       >> (pos & 7)) & 0x0f);

    if (code & 8) {
        vqt_screen_node(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
    } else {
        fill_screen_quadrant(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
        redraw_cursor(g_vmds.page_front);
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
 * 0x27734
 *
 * **The screen quadtree's leaf: paint one rectangle straight onto the page**
 * from what the bit stream says next. `fill_quadrant`'s arithmetic, byte for
 * byte, with a different destination: not a plane buffer but video memory,
 * one plane to a pixel.
 *
 * A pixel at x goes into the byte `g_vmds.row_offset[y] + (x >> 2)` of the page
 * `g_vmds.page_dst`, and the plane is chosen for each write with the
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
 * through DGROUP 0x436e, `g_vm_driver.entry[VM_SLOT_SPAN]`, the driver's span fill at
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
void near fill_screen_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
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
        at = (uint16_t)(g_vmds.row_offset[y] + (x >> 2));
        io_out16(PORT_SEQ_INDEX,
                 (uint16_t)(((uint16_t)(uint8_t)(1 << (x & 3)) << 8) | 0x02));
        vga_write((uint16_t)(vga_seg_offset(g_vmds.page_dst) + at), colour);
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
                at = (uint16_t)(g_vmds.row_offset[(uint16_t)yi]
                                + ((uint16_t)xi >> 2));
                io_out16(PORT_SEQ_INDEX,
                         (uint16_t)(((uint16_t)(uint8_t)(1 << (xi & 3)) << 8)
                                    | 0x02));
                vga_write((uint16_t)(vga_seg_offset(g_vmds.page_dst) + at),
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
        row = g_vmds.row_offset[y];                   /* di */
        rows = h;                                     /* si */
        do {
            vm_span((uint16_t)((colour << 8) | colour), x, (int16_t)w,
                    vga_window_at((uint16_t)g_vmds.page_dst, row));
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
            at = (uint16_t)(g_vmds.row_offset[(uint16_t)yi]
                            + ((uint16_t)xi >> 2));
            io_out16(PORT_SEQ_INDEX,
                     (uint16_t)(((uint16_t)(uint8_t)(1 << (xi & 3)) << 8)
                                | 0x02));
            vga_write((uint16_t)(vga_seg_offset(g_vmds.page_dst) + at),
                      colour);
            yi++;
        } while (yi < y1);
        yi = (int16_t)y;
        xi++;
    } while ((uint16_t)xi < (uint16_t)x1);            /* `jae`, unsigned */
}

/*
 * 0x27a20
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
void near far_copy(uint8_t far *dst, const uint8_t far *src,
              uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        dst[i] = src[i];
}

/*
 * 0x27a42
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
void near vqt_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    struct vqt_reader *rd;
    uint16_t code;
    uint32_t pos;

    if ((w | h) == 0)
        return;

    rd = g_bitmaps.walk;
    pos = (uint32_t)rd->pos;
    rd->pos = (int32_t)(pos + 4);

    /* Four bits at `pos`, read as a word so a nibble can straddle a byte. The
       offset is stepped inside the segment, which is why `data` stays a pair. */
    code = (uint16_t)(((uint16_t)(rd->data[pos >> 3]
                                  | (rd->data[(pos >> 3) + 1] << 8))
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
 * 0x27b3f
 *
 * **The quadtree's leaf: paint one rectangle of the bitmap** from what the bit
 * stream says next. Every pixel goes into the first plane of the reader record
 * `g_bitmaps.walk` names, at `plane[0] + row[y] + x` - a 16-bit offset
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
void near fill_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
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
        rd = g_bitmaps.walk;
        rd->plane[0][(uint16_t)rd->row[y] + x] = colour;
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
                rd = g_bitmaps.walk;
                rd->plane[0][(uint16_t)rd->row[yi]
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
                rd = g_bitmaps.walk;
                rd->plane[0][(uint16_t)rd->row[yi]
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
            rd = g_bitmaps.walk;
            rd->plane[0][(uint16_t)rd->row[yi]
                                     + (uint16_t)xi] = colour;
            yi++;
        } while (yi < y1);
        yi = (int16_t)y;
        xi++;
    } while (xi < x1);
}
