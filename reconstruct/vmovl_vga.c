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
 * VM.OVL VGA:0x0000
 *
 * The driver's start-up, and the only entry `vm_init` reaches directly rather
 * than through the vector table. It answers 2 in AX and its own vector table in
 * DX:SI - which is how `vm_init` knows where to copy the table from.
 *
 * Its three arguments are (0x3890, 0x4412, DGROUP) and the order is the
 * opposite of how the pushes read; see docs/video-driver.md. The first is the
 * distance from DGROUP to the driver's own data, kept at `cs:0x13c` and turned
 * into a segment at `cs:0x13a`. The second is a DGROUP address it copies 76
 * bytes from into its own `cs:0x206`.
 *
 * The screen height at `driverDS:0x6ec` - DGROUP 0x3f7c - picks the BIOS mode.
 * Only 0x1e0 is reached here, which is mode 0x12 with both pages at 0xa000;
 * 0x190 wants the same mode with the pages at 0xa800 and five CRTC registers
 * adjusted, 0x15e wants mode 0x10, and anything else falls back to 0x0e. The
 * three that are not reached are stubs.
 *
 * The row table at `driverDS:0x6f2` is then filled with 480 entries, each 0x50
 * further on than the last - one row start per scan line, at 80 bytes a row.
 *
 * Last it opens the map mask to all four planes and sets the graphics
 * controller to write mode 2, which is the mode every blit in this driver
 * assumes.
 */
uint16_t vm_driver_init(uint16_t data_delta, uint16_t params, uint16_t ds)
{
    uint16_t cs = DGU16(0x48f6);
    int16_t i;

    (void)ds;

    far_move(params, DGROUP_SEG, 0x206, cs, 0x4c);

    *(uint16_t *)FAR_PTR(cs, 0x13c) = data_delta;
    *(uint16_t *)FAR_PTR(cs, 0x13a) =
        (uint16_t)((data_delta >> 4) + DGROUP_SEG);

    DG8(VMDS + 0x6e8) = 1;
    DG8(VMDS + 0x21) = 0x10;
    DG16(VMDS + 0x14) = (int16_t)0xa000;
    DG16(VMDS + 0x12) = (int16_t)0xa800;
    DG16(VMDS + 0x10) = (int16_t)0xa800;

    switch (DGU16(VMDS + 0x6ec)) {
    case 0x1e0:
        io_bios_set_mode(0x12);
        vm_reset_attributes();
        DG16(VMDS + 0x12) = (int16_t)0xa000;
        DG16(VMDS + 0x10) = (int16_t)0xa000;
        break;
    case 0x15e:
        not_transcribed("VGA:0x00b4, the 0x15e screen height");
        return 0;
    case 0x190:
        not_transcribed("VGA:0x006c, the 0x190 screen height");
        return 0;
    default:
        not_transcribed("VGA:0x005f, the fallback screen height");
        return 0;
    }

    {
        uint16_t row = 0;

        for (i = 0; i < 0x1e0; i++) {
            DG16(VMDS + 0x6f2 + 2 * i) = (int16_t)row;
            row = (uint16_t)(row + 0x50);
        }
    }

    io_out16(PORT_SEQ_INDEX, 0x0f02);
    io_out16(PORT_GC_INDEX, 0x0205);

    DG16(VMDS + 0x6ea) = 0x280;
    DG16(VMDS + 6) = 0x27f;
    DG16(VMDS + 0xa) = (int16_t)(DGU16(VMDS + 0x6ec) - 1);

    return 2;
}

/*
 * VM.OVL VGA:0x011d
 *
 * Put the attribute controller's sixteen palette registers back to the
 * identity - register `n` holding `n` - and restore whatever the index
 * register held before.
 *
 * It writes each pair with interrupts off and reads Input Status 1 first,
 * because that read is what puts the one port back to expecting an index
 * rather than a value. The two `jmp $+2`s between the writes are an I/O delay
 * for hardware that needs one.
 *
 * The loop runs from 0xf **down to 1**, so register 0 is never written; it
 * keeps whatever the mode set left there.
 */
void vm_reset_attributes(void)
{
    uint8_t saved = io_in8(PORT_ATTR);
    int16_t cl;

    for (cl = 0xf; cl >= 1; cl--) {
        io_in8(PORT_INPUT_ST1);
        io_out8(PORT_ATTR, (uint8_t)cl);
        io_out8(PORT_ATTR, (uint8_t)cl);
    }

    io_out8(PORT_ATTR, saved);
}

/*
 * VM.OVL VGA:0x0252
 *
 * A single `retf`: the driver's do-nothing entry. Three slots of the vector
 * table point at it - 0x436a, 0x4376 and 0x4382 - so the game can call them
 * unconditionally and this adapter simply declines.
 *
 * Its arguments are whatever the caller pushed and it reads none of them; the
 * port takes none, for the same reason.
 */
void vm_nothing(void)
{
}

/*
 * VM.OVL VGA:0x0f57
 *
 * NOT TRANSCRIBED YET. **Blend a run of palette entries towards one colour**,
 * in place, and hand the result to the DAC through VGA:0x0ec1.
 *
 * The routine is read and understood; what is missing is somewhere to put it.
 * It works on the driver's *own* palette buffer - the segment at `cs:[0x1a0]`,
 * which is not DGROUP and which this port does not model: `vm_set_palette`
 * takes a host pointer and the buffer has never had to exist. Writing
 * accessors for a buffer whose address and layout have not been established
 * would be inventing the one thing this routine is about, so it aborts instead.
 *
 * What it does, for whoever models that buffer:
 *
 * The source is 0x30 bytes - sixteen entries - past the destination, and the
 * colour blended towards is a single three-byte entry the loop cycles over:
 * `bp` walks r, g, b and is pulled back by three every third byte. So one run
 * of entries fades towards one colour, which is how the game fades a screen to
 * black or up from it without a table per step.
 *
 * The arithmetic is the DAC's six-bit range, not a byte's:
 *
 *     out = src * w / 0x3f + colour * (0x3f - w) / 0x3f
 *
 * done as `mul dl` then `div dh` on eight-bit halves, so both terms are exact
 * over the range the hardware uses and neither can overflow. The weight is
 * flipped to `0x3f - w` for the second term and flipped back afterwards, in
 * `dl`, rather than kept in two registers.
 *
 * The `add sp, cx` after the call is a no-op that reads as a bug: `loop` has
 * just taken `cx` to zero. Noted because the next person to read it will
 * wonder too.
 */
void vm_blend_palette(uint16_t first, uint16_t count, uint16_t colour,
                      uint8_t weight)
{
    (void)first;
    (void)count;
    (void)colour;
    (void)weight;
    not_transcribed("VGA:0x0f57, and the driver's palette buffer at cs:[0x1a0]");
}

/*
 * VM.OVL VGA:0x0fd4
 *
 * How much memory a list of bitmaps needs, as a 32-bit total in DX:AX.
 *
 * The list is an array of near pointers ending in a null, and each bitmap's
 * cost is `(width / 2) * height` - half a byte per pixel, which is what four
 * planes of one bit each come to.
 *
 * The total is then multiplied by **1.25**: shifted right two and added back
 * to itself. That quarter is the driver's own per-bitmap overhead, and it is
 * charged against the whole list at once rather than per bitmap.
 *
 * The second argument is a word that is zeroed and nothing else - an out
 * parameter the routine never fills in.
 */
uint32_t vm_bitmap_list_size(uint16_t list, uint16_t out)
{
    uint32_t total = 0;

    for (;;) {
        uint16_t p = DGU16(list);

        if (p == 0)
            break;

        total += (uint32_t)(DGU16(p + 6) >> 1) * DGU16(p + 8);
        list = (uint16_t)(list + 2);
    }

    DG16(out) = 0;

    return total + (total >> 2);
}

/*
 * VM.OVL VGA:0x1015
 *
 * Turn a buffer of chunky 4-bit pixels into the planar form the driver blits,
 * and fill in a list of bitmap headers pointing into the result. Reached
 * through the vector table at DGROUP 0x437e.
 *
 * It works **in place**, through video memory as scratch: the whole buffer is
 * converted once into the plane at A000:6d60, and then each bitmap is read back
 * out of it, four planes at a time, into the space the chunky data occupied.
 * That is why the destination it walks forward is the same pointer it was
 * handed as the source, and why the video segment 0xa6d6 appears three times as
 * a constant.
 *
 * The list at `list` is a null-terminated run of near pointers to headers. For
 * each header the size of one plane is `(width / 2) * height / 4` - the width
 * halved because two pixels share a chunky byte, and the product quartered
 * because the 32-bit `mul` result is used **low word only**, `shr ax` twice,
 * with the high word discarded. Then:
 *
 *   +0/+2   the far pointer to the four planes
 *   +4      the offset of the mask that follows them
 *
 * and the running pointer advances by five plane-sizes: four of image and one
 * of mask, renormalised into segment and offset each time round.
 *
 * The two calls that do the reading back share their arguments: the first
 * leaves the destination segment on the stack and the second is pushed to sit
 * on top of it, so five words are cleaned where only three were pushed. That is
 * why `push cs` plus a **near** `ret` is used throughout this family - the
 * pushed CS is part of the frame and the caller disposes of it.
 */
void vm_load_bitmap_list(uint16_t list, uint16_t dst_off, uint16_t dst_seg,
                         uint16_t count_lo, uint16_t count_hi)
{
    uint32_t count = ((((uint32_t)count_hi << 16) | count_lo) >> 2);
    uint16_t off = dst_off;
    uint16_t seg = dst_seg;
    uint16_t di = 0;
    uint16_t bx = list;

    vm_chunky_to_planar(off, seg, 0, 0xa6d6, (uint16_t)count);

    for (;;) {
        uint16_t si = DGU16(bx);
        uint16_t size, prod, old_off, total;

        if (si == 0)
            break;

        prod = (uint16_t)((uint16_t)(DGU16((uint16_t)(si + 6)) >> 1)
                          * DGU16((uint16_t)(si + 8)));
        size = (uint16_t)(prod >> 2);

        DGU16(si) = seg;
        DGU16((uint16_t)(si + 2)) = off;

        old_off = off;
        off = (uint16_t)(off + size * 4);
        DGU16((uint16_t)(si + 4)) = off;

        vm_read_four_planes(di, 0xa6d6, old_off, seg, size);
        vm_build_mask_plane(di, 0xa6d6, off, seg, size);

        di = (uint16_t)(di + size);

        total = (uint16_t)(size + off);
        seg = (uint16_t)(seg + (total >> 4));
        off = (uint16_t)(total & 0x0f);

        bx = (uint16_t)(bx + 2);
    }
}

/*
 * VM.OVL VGA:0x10b8
 *
 * Chunky to planar. Reads four bytes - eight pixels, two to a byte, the high
 * nibble first - and writes one byte to each of the four planes at the same
 * video address.
 *
 * The original does it with sixteen `shl al,1 / rcl <reg>,1` pairs per word,
 * rotating each bit out of the source and into one of `ch`, `cl`, `bh`, `bl` in
 * turn. Those four are planes 3, 2, 1 and 0, so a pixel's most significant bit
 * lands in plane 3, and after thirty-two bits each register holds eight pixels'
 * worth of one plane. Written here as the shift it is rather than as a table.
 *
 * The **source pointer is huge**, and it is carried by the flags: `add si,2`
 * sets carry when the offset wraps, `rcl dh,1` catches it and four `shl dh,1`
 * move it to make 0x1000, which is added to DS. Only the second of the two word
 * reads is checked, because `lodsw` sets no flags to check.
 *
 * The plane is chosen by writing 1, 2, 4 and 8 straight to the sequencer's data
 * port, the map-mask index having been left selected on the way in.
 */
void vm_chunky_to_planar(uint16_t src_off, uint16_t src_seg,
                         uint16_t dst_off, uint16_t dst_seg, uint16_t count)
{
    uint16_t si = src_off;
    uint16_t seg = src_seg;
    uint16_t di = (uint16_t)(vga_seg_offset(dst_seg) + dst_off);
    uint16_t n = count;

    io_out16(PORT_GC_INDEX, 0x0205);      /* write mode 2 */
    io_out16(PORT_GC_INDEX, 0xFF08);      /* bit mask: every bit */
    io_out16(PORT_GC_INDEX, 0x0005);      /* write mode 0 */
    io_out16(PORT_SEQ_INDEX, 0x0102);     /* map mask: plane 0 */

    while (n != 0) {
        uint8_t pl[4];                    /* pl[0]=bl .. pl[3]=ch */
        uint16_t w;
        uint16_t carry;
        int32_t k, bit;
        uint8_t b;

        pl[0] = pl[1] = pl[2] = pl[3] = 0;

        w = FARU16(seg, si);              /* lodsw: no flags, no carry check */
        si = (uint16_t)(si + 2);

        for (k = 0; k < 2; k++) {
            b = (uint8_t)(k == 0 ? (w & 0xFF) : (w >> 8));
            for (bit = 7; bit >= 0; bit--) {
                int32_t p = 3 - ((7 - bit) & 3);

                pl[p] = (uint8_t)((pl[p] << 1) | ((b >> bit) & 1));
            }
        }

        w = FARU16(seg, si);
        carry = ((uint16_t)(si + 2) < si) ? 0x1000 : 0;
        si = (uint16_t)(si + 2);

        for (k = 0; k < 2; k++) {
            b = (uint8_t)(k == 0 ? (w & 0xFF) : (w >> 8));
            for (bit = 7; bit >= 0; bit--) {
                int32_t p = 3 - ((7 - bit) & 3);

                pl[p] = (uint8_t)((pl[p] << 1) | ((b >> bit) & 1));
            }
        }

        seg = (uint16_t)(seg + carry);

        for (k = 0; k < 4; k++) {
            io_out8(PORT_SEQ_DATA, (uint8_t)(1 << k));
            vga_write(di, pl[k]);
        }

        di++;
        n--;
    }

    io_out16(PORT_GC_INDEX, 0x0205);      /* write mode 2 */
    io_out16(PORT_SEQ_INDEX, 0x0F02);     /* map mask: every plane */
}

/*
 * VM.OVL VGA:0x11bb
 *
 * Read the same run of video memory once through each plane, into four
 * consecutive blocks of the destination. The source offset is pushed and popped
 * around every `rep movsb` so all four passes read the same bytes; only the
 * destination advances.
 */
void vm_read_four_planes(uint16_t src_off, uint16_t src_seg,
                         uint16_t dst_off, uint16_t dst_seg, uint16_t count)
{
    uint16_t src = (uint16_t)(vga_seg_offset(src_seg) + src_off);
    uint16_t di = dst_off;
    int32_t plane;

    for (plane = 0; plane < 4; plane++) {
        uint16_t k;

        io_out16(PORT_GC_INDEX, (uint16_t)(0x04 | (plane << 8)));

        for (k = 0; k < count; k++)
            FAR8(dst_seg, (uint16_t)(di + k)) = vga_read((uint16_t)(src + k));

        di = (uint16_t)(di + count);
    }
}

/*
 * VM.OVL VGA:0x11ee
 *
 * Build the mask that goes with a bitmap: a bit is set where the pixel is
 * **not** in any plane, which is to say where its colour is 0. The four planes
 * of a byte are ORed together and the result inverted, so the mask marks what
 * the blit must leave alone.
 *
 * The read-map-select index is written once and only the data port is touched
 * after that, which is why the plane numbers go out as bytes rather than as the
 * usual index-and-data word.
 */
void vm_build_mask_plane(uint16_t src_off, uint16_t src_seg,
                         uint16_t dst_off, uint16_t dst_seg, uint16_t count)
{
    uint16_t src = (uint16_t)(vga_seg_offset(src_seg) + src_off);
    uint16_t si = 0;
    uint16_t di = dst_off;
    uint16_t n = count;

    io_out16(PORT_GC_INDEX, 0x0004);      /* read map select, plane 0 */

    while (n != 0) {
        uint8_t any;
        int32_t plane;

        io_out8(PORT_GC_DATA, 0);
        any = vga_read((uint16_t)(src + si));

        for (plane = 1; plane < 4; plane++) {
            io_out8(PORT_GC_DATA, (uint8_t)plane);
            any |= vga_read((uint16_t)(src + si));
        }

        FAR8(dst_seg, di) = (uint8_t)~any;

        di++;
        si++;
        n--;
    }
}

/*
 * VM.OVL VGA:0x12fb
 *
 * Save a rectangle of the source page into a buffer, all four planes.
 *
 * The buffer arrives as a far pointer and is **renormalised** first - the
 * offset's high bits are folded into the segment, leaving an offset of 0..15 -
 * so a rectangle bigger than a segment still addresses correctly as the
 * destination index runs on.
 *
 * A row's width is counted in whole bytes: `((x + w) >> 3) - (x >> 3) + 1`,
 * then rounded up to a whole number of words because the copy is `rep movsw`.
 * So the saved rectangle is byte-aligned and generally wider than asked for,
 * which is why the caller's buffer size allows a spare byte per row.
 *
 * The planes are read 3, 2, 1, 0 - the loop counts down in AH and ends on the
 * `jge` failing at -1 - and each is stored one after another, the destination
 * index running continuously across planes and rows while the source resets to
 * the row start plus 0x50 each time.
 *
 * Read mode 0 is selected with a full bit mask and set/reset cleared before the
 * copy, and write mode 2 is put back afterwards, which is what the rest of the
 * driver expects to find.
 */
void vm_save_rect(uint16_t buf_off, uint16_t buf_seg,
                  int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t seg  = (uint16_t)(buf_seg + (buf_off >> 4));
    uint16_t di   = (uint16_t)(buf_off & 0xf);
    uint8_t *buf  = FAR_PTR(seg, 0);
    uint16_t col  = (uint16_t)((uint16_t)x >> 3);
    uint16_t base = vga_seg_offset(vga_page_src);
    uint16_t bytes, words;
    int16_t plane;

    io_out16(PORT_GC_INDEX, 0x0005);      /* read mode 0, write mode 0 */
    io_out16(PORT_GC_INDEX, 0xFF08);      /* bit mask: every bit */
    io_out16(PORT_GC_INDEX, 0x0000);      /* set/reset: none */

    bytes = (uint16_t)((((uint16_t)(x + w)) >> 3) - col + 1);
    words = (uint16_t)(bytes >> 1);
    if ((bytes & 1) != 0)
        words++;

    for (plane = 3; plane >= 0; plane--) {
        uint16_t si = (uint16_t)(vga_row_offset(y) + col);
        int16_t row;

        io_out16(PORT_GC_INDEX, (uint16_t)(0x04 | (plane << 8)));

        for (row = 0; row < h; row++) {
            uint16_t k;

            for (k = 0; k < (uint16_t)(words * 2); k++)
                buf[di++] = vga_read((uint16_t)(base + si + k));
            si = (uint16_t)(si + 0x50);
        }
    }

    io_out16(PORT_GC_INDEX, 0x0205);      /* write mode 2 */
}

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
 * VM.OVL VGA:0x13b9
 *
 * Restore a rectangle from a buffer into the page being drawn into - the exact
 * counterpart of `vm_save_rect`, and it has to agree with it byte for byte or
 * the saved image comes back shifted.
 *
 * It agrees by construction: the same renormalisation of the buffer pointer,
 * the same whole-byte row width rounded up to words, the same four planes in
 * the same order. What differs is the direction and how a plane is selected.
 * Reading picks one plane with the Graphics Controller's read map select;
 * writing enables one plane with the **Sequencer's map mask** at 0x3c4, whose
 * value is a bit per plane rather than a number - 8, 4, 2, 1, shifted right
 * each time round.
 *
 * The loop ends on the bit falling out of the mask: `shr ah,1` sets carry only
 * when the 1 is shifted away, so `jae` runs it exactly four times.
 *
 * On the way out write mode 2 is restored and the map mask is put back to 0x0f,
 * all four planes enabled, which is the state the rest of the driver assumes.
 * Leaving a single plane enabled here would make every later write monochrome.
 */
void vm_restore_rect(uint16_t buf_off, uint16_t buf_seg,
                     int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t seg  = (uint16_t)(buf_seg + (buf_off >> 4));
    uint16_t si   = (uint16_t)(buf_off & 0xf);
    const uint8_t *buf = FAR_PTR(seg, 0);
    uint16_t col  = (uint16_t)((uint16_t)x >> 3);
    uint16_t base = vga_seg_offset(vga_page_dst);
    uint16_t bytes, words, mask;

    io_out16(PORT_GC_INDEX, 0x0005);      /* read mode 0, write mode 0 */
    io_out16(PORT_GC_INDEX, 0xFF08);      /* bit mask: every bit */
    io_out16(PORT_GC_INDEX, 0x0000);      /* set/reset: none */

    bytes = (uint16_t)((((uint16_t)(x + w)) >> 3) - col + 1);
    words = (uint16_t)(bytes >> 1);
    if ((bytes & 1) != 0)
        words++;

    for (mask = 8; mask != 0; mask >>= 1) {
        uint16_t di = (uint16_t)(vga_row_offset(y) + col);
        int16_t row;

        io_out16(PORT_SEQ_INDEX, (uint16_t)(0x02 | (mask << 8)));

        for (row = 0; row < h; row++) {
            uint16_t k;

            for (k = 0; k < words; k++) {
                uint16_t v = (uint16_t)(buf[si] | (buf[si + 1] << 8));

                vga_write16((uint16_t)(base + di + k * 2), v);
                si = (uint16_t)(si + 2);
            }
            di = (uint16_t)(di + 0x50);
        }
    }

    io_out16(PORT_GC_INDEX, 0x0205);      /* write mode 2 */
    io_out16(PORT_SEQ_INDEX, 0x0F02);     /* map mask: all four planes */
}

/*
 * VM.OVL VGA:0x1453
 *
 * Read the colour of one pixel from the source page.
 *
 * A pixel's four bits live in four different planes at the same byte address,
 * so this reads the same byte four times, selecting a different plane between
 * each with the Graphics Controller's read map select. The bit tested is
 * `0x80 >> (x & 7)` and each plane contributes one bit of the answer, plane 0
 * the least significant.
 *
 * The index register is set once, to 4, and the three later changes are written
 * to the **data port** at 0x3cf alone - `inc dx` and then a byte `out` - rather
 * than re-sending the index each time. A port model that only understands the
 * paired 16-bit write would read plane 0 four times and answer a colour of 0 or
 * 15.
 *
 * Write mode 1 is selected first and mode 2 restored at the end. Neither
 * affects reading; the driver is just leaving the registers as the rest of it
 * expects.
 */
uint16_t vm_read_pixel(int16_t x, int16_t y)
{
    uint16_t base   = vga_seg_offset(vga_page_src);
    uint16_t off    = (uint16_t)(vga_row_offset(y) + ((uint16_t)x >> 3));
    uint8_t  bit    = (uint8_t)(0x80 >> (x & 7));
    uint16_t colour = 0;

    io_out16(PORT_GC_INDEX, 0x0105);      /* write mode 1 */
    io_out16(PORT_GC_INDEX, 0x0004);      /* read map select: plane 0 */

    if (vga_read((uint16_t)(base + off)) & bit)
        colour |= 1;
    io_out8(PORT_GC_DATA, 1);
    if (vga_read((uint16_t)(base + off)) & bit)
        colour |= 2;
    io_out8(PORT_GC_DATA, 2);
    if (vga_read((uint16_t)(base + off)) & bit)
        colour |= 4;
    io_out8(PORT_GC_DATA, 3);
    if (vga_read((uint16_t)(base + off)) & bit)
        colour |= 8;

    io_out16(PORT_GC_INDEX, 0x0205);      /* write mode 2 */
    return colour;
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
/*
 * VM.OVL VGA:0x15d0
 *
 * Blit a band of chunky 4-bit pixels straight onto the page, converting to
 * planes as it goes. This is how a screen file's pixels arrive: `0x23b29` reads
 * a band into a buffer and hands it here, row after row, so a 320x200 picture
 * never needs a 64 KB buffer.
 *
 * The conversion is the same as `vm_chunky_to_planar`: four source bytes -
 * eight pixels, two to a byte, the high nibble first - rotated bit by bit into
 * four registers that are then written to planes 0 to 3 at one address. What is
 * different is the destination, which walks a page rather than a flat block:
 * `0x50 - (w >> 3)` is added at the end of every row to step to the next.
 *
 * `x` is used only for its whole bytes - `x >> 3` - so this cannot place a band
 * at a bit offset the way the structured blit can.
 *
 * It is one of the two places the driver patches its own code: the row count
 * goes into cs:[0x15ce] and the two row figures into cs:[0x15ca] and
 * cs:[0x15cc]. The port keeps them in locals, for the reason `vm_blit_bitmap`
 * gives.
 */
void vm_blit_rows(uint16_t src_off, uint16_t src_seg, int16_t x, int16_t y,
                  int16_t w, int16_t h)
{
    uint16_t base = vga_seg_offset(vga_page_dst);
    uint16_t di = (uint16_t)(vga_row_offset(y) + (uint16_t)(x >> 3));
    uint16_t si = (uint16_t)(src_off & 0x0f);
    uint16_t seg = (uint16_t)((src_off >> 4) + src_seg);
    uint16_t across = (uint16_t)(w >> 3);       /* cs:[0x15ca] */
    uint16_t step = (uint16_t)(0x50 - across);  /* cs:[0x15cc] */
    int16_t rows = h;                           /* cs:[0x15ce] */

    io_out16(PORT_GC_INDEX, 0x0005);            /* write mode 0 */
    io_out16(PORT_GC_INDEX, 0xFF08);            /* bit mask: every bit */
    io_out8(PORT_SEQ_INDEX, 0x02);              /* select the map mask */

    for (;;) {
        uint16_t n = across;

        while (n != 0) {
            uint8_t pl[4];                      /* pl[0]=bl .. pl[3]=ch */
            int32_t k, bit;

            pl[0] = pl[1] = pl[2] = pl[3] = 0;

            for (k = 0; k < 4; k++) {
                uint8_t b = FAR8(seg, si);

                si++;
                for (bit = 7; bit >= 0; bit--) {
                    int32_t p = 3 - ((7 - bit) & 3);

                    pl[p] = (uint8_t)((pl[p] << 1) | ((b >> bit) & 1));
                }
            }

            for (k = 0; k < 4; k++) {
                io_out8(PORT_SEQ_DATA, (uint8_t)(1 << k));
                vga_write((uint16_t)(base + di), pl[k]);
            }

            di++;
            n--;
        }

        rows--;
        if (rows <= 0)
            break;

        di = (uint16_t)(di + step);
    }

    io_out8(PORT_SEQ_DATA, 0x0f);               /* map mask: every plane */
    io_out16(PORT_GC_INDEX, 0x0205);            /* write mode 2 */
    io_out16(PORT_GC_INDEX, 0x0003);            /* function select: replace */
}

/*
 * VM.OVL VGA:0x1707
 *
 * The **structured blit**: draw a planar bitmap with its mask. Nearly
 * everything on these screens reaches the page through here. Reached by vector
 * 0x43ba, and 3,782 bytes of hand-written assembly - 0x1707 to 0x25cc, the
 * largest single routine in the original.
 *
 * It works in five passes over the bitmap. The first writes the *mask* into the
 * Graphics Controller's bit-mask register a byte at a time and stores a zero
 * through it, which clears the sprite's pixels in all four planes at once. Then
 * four passes, one a plane, with read-map-select and map-mask set to that plane
 * and the function set to OR, so each plane's bits drop into the hole the mask
 * just made. Every write is preceded by a read, because that is what loads the
 * latches the OR combines with.
 *
 * Shifting the bitmap to an x that is not a multiple of eight is done with
 * `ror ax, cl` on a pair of bytes - the byte being drawn in AL and the previous
 * byte's leftover in AH - which is the standard way to carry bits across a byte
 * boundary on this machine. `cl` is `x & 7` throughout and AH is shifted down
 * by `8 - cl` after each byte to line the leftover up for the next one.
 *
 * **Its parameters live in its own code segment**, at cs:0x25d5, and it pushes
 * and pops seven of them around its work so it can be re-entered. The port uses
 * ordinary locals: those words are inside the range tools/verify.py already
 * excludes from the memory comparison, because the driver patches its own code
 * and a C transcription has nowhere to patch.
 *
 * `mode` selects one of four bodies through a jump table at cs:0x25cd, and its
 * low two bits are a vertical and a horizontal flip. **Only mode 0 - neither
 * flip - is transcribed.** The other three are the same loops walking the
 * source the other way; they are an abort rather than a guess.
 */
void vm_blit_bitmap(uint16_t hdr, int16_t x, int16_t y, uint16_t mode)
{
    uint16_t seg      = DGU16(hdr);
    uint16_t src      = DGU16((uint16_t)(hdr + 2));
    uint16_t mask_at  = DGU16((uint16_t)(hdr + 4));
    int16_t  w        = DG16((uint16_t)(hdr + 6));
    int16_t  h        = DG16((uint16_t)(hdr + 8));

    uint16_t base     = vga_seg_offset(vga_page_dst);
    uint16_t rowbytes = (uint16_t)(w >> 3);          /* cs:[0x25d5] */
    uint16_t planestep = (uint16_t)((mask_at - src) >> 2);  /* cs:[0x25d7] */
    int16_t  rows     = h;                           /* cs:[0x25dd] */
    uint8_t  cols     = (uint8_t)((w + 7) >> 3);     /* DH */
    uint8_t  edge_right = 0, edge_left = 0;          /* cs:[0x25e2], [0x25e3] */
    uint16_t si       = src;
    uint16_t mask_p   = mask_at;                     /* cs:[0x25db] */
    uint16_t di;
    uint8_t  cl;
    int16_t  plane;

    di = (uint16_t)((y >= 0) ? vga_row_offset(y) : (uint16_t)(y * 80));
    di = (uint16_t)(di + (uint16_t)(x >> 3));
    cl = (uint8_t)(x & 7);

    if ((mode & 1) != 0) {
        uint16_t n = (uint16_t)((rows - 1) * rowbytes);

        si = (uint16_t)(si + n);
        mask_p = (uint16_t)(mask_p + n);
    }
    if ((mode & 2) != 0) {
        uint16_t n = (uint16_t)(rowbytes - 1);

        si = (uint16_t)(si + n);
        mask_p = (uint16_t)(mask_p + n);
    }

    if (clip_enabled != 0) {
        int16_t over;

        /* off the right-hand edge */
        over = (int16_t)(clip_right + 1 - (x + w));
        if (over <= 0) {
            over = (int16_t)(-over);
            if (over >= w)
                goto done;
            cols = (uint8_t)(cols - (uint8_t)(over >> 3));
            edge_right = 1;
        }

        /* off the left-hand edge */
        over = (int16_t)(x - clip_left);
        if (over < 0) {
            int16_t bx = over;

            if ((int16_t)(over + w) <= 0)
                goto done;
            edge_left = 1;
            bx = (int16_t)((-bx + 7) >> 3);
            cols = (uint8_t)(cols - (uint8_t)bx);
            di = (uint16_t)(di + bx);
            if ((mode & 2) != 0)
                bx = (int16_t)(-bx);
            si = (uint16_t)(si + bx);
            mask_p = (uint16_t)(mask_p + bx);
        }

        /* off the bottom */
        over = (int16_t)(y + h - clip_bottom);
        if (over > 0) {
            if (over >= h)
                goto done;
            over--;
            rows = (int16_t)(rows - over);
        }

        /* off the top */
        over = (int16_t)(clip_top - y);
        if (over >= 0) {
            uint16_t n;

            if (over >= h)
                goto done;
            rows = (int16_t)(rows - over);
            di = (uint16_t)(di + over * 80);
            n = (uint16_t)((uint8_t)over * (uint8_t)rowbytes);
            if ((mode & 1) != 0)
                n = (uint16_t)(-(int16_t)n);
            si = (uint16_t)(si + n);
            mask_p = (uint16_t)(mask_p + n);
        }
    }

    if (mode != 0)
        not_transcribed("VGA:0x1707 with a flip - modes 1, 2 and 3");


    /* ------------------------------------------------ the mask, all planes */
    {
        uint16_t p = mask_p;
        uint16_t d = di;
        int16_t row;

        io_out8(PORT_GC_INDEX, 0x08);        /* select the bit mask */

        for (row = rows; row != 0; row--) {
            uint16_t sp = p, dp = d;
            uint8_t ch = cols;
            uint8_t ah = 0;
            uint8_t al;

            if ((edge_left & 1) != 0) {
                ah = (uint8_t)~FAR8(seg, (uint16_t)(sp - 1));
                if (ch == 0)
                    goto mask_spill;
            }

            while (ch != 0) {
                uint16_t both;

                al = (uint8_t)~FAR8(seg, sp);
                sp++;
                both = (uint16_t)((ah << 8) | al);
                both = (uint16_t)((both >> cl) | (both << (16 - cl)));
                al = (uint8_t)both;
                ah = (uint8_t)(both >> 8);

                io_out8(PORT_GC_DATA, al);
                (void)vga_read((uint16_t)(base + dp));
                vga_write((uint16_t)(base + dp), 0);
                dp++;

                ah = (uint8_t)(ah >> (8 - cl));
                ch--;
            }

            if ((edge_right & 1) != 0)
                goto mask_next;

        mask_spill:
            {
                uint16_t both = (uint16_t)(ah << 8);

                both = (uint16_t)((both >> cl) | (both << (16 - cl)));
                io_out8(PORT_GC_DATA, (uint8_t)both);
                (void)vga_read((uint16_t)(base + dp));
                vga_write((uint16_t)(base + dp), 0);
            }

        mask_next:
            p = (uint16_t)(p + rowbytes);
            d = (uint16_t)(d + 0x50);
        }

        io_out8(PORT_GC_DATA, 0xff);         /* every bit writable again */
    }

    io_out16(PORT_GC_INDEX, 0x0005);         /* write mode 0 */
    io_out16(PORT_GC_INDEX, 0x1003);         /* function select: OR */

    /* --------------------------------------------- and then the four planes */
    for (plane = 0; plane < 4; plane++) {
        uint16_t p = si;
        uint16_t d = di;
        int16_t row;

        io_out16(PORT_GC_INDEX, (uint16_t)(0x04 | (plane << 8)));
        io_out16(PORT_SEQ_INDEX, (uint16_t)(0x02 | ((1 << plane) << 8)));

        for (row = rows; row != 0; row--) {
            uint16_t sp = p, dp = d;
            uint8_t ch = cols;
            uint8_t ah = 0;
            uint8_t al;

            if ((edge_left & 1) != 0) {
                ah = FAR8(seg, (uint16_t)(sp - 1));
                if (ch == 0)
                    goto plane_spill;
            }

            while (ch != 0) {
                uint16_t both;

                al = FAR8(seg, sp);
                sp++;
                both = (uint16_t)((ah << 8) | al);
                both = (uint16_t)((both >> cl) | (both << (16 - cl)));
                al = (uint8_t)both;
                ah = (uint8_t)(both >> 8);

                (void)vga_read((uint16_t)(base + dp));
                vga_write((uint16_t)(base + dp), al);
                dp++;

                ah = (uint8_t)(ah >> (8 - cl));
                ch--;
            }

            if ((edge_right & 1) != 0)
                goto plane_next;

        plane_spill:
            {
                uint16_t both = (uint16_t)(ah << 8);

                both = (uint16_t)((both >> cl) | (both << (16 - cl)));
                (void)vga_read((uint16_t)(base + dp));
                vga_write((uint16_t)(base + dp), (uint8_t)both);
            }

        plane_next:
            p = (uint16_t)(p + rowbytes);
            d = (uint16_t)(d + 0x50);
        }

        si = (uint16_t)(si + planestep);
    }

    /*
     * The epilogue, and the label the clipped-out paths jump to. Those `jmp`s
     * land *inside* it rather than at a return, so a blit that is entirely off
     * the edge still puts these three registers back - which is observable, and
     * was the one thing the port got wrong here.
     */
done:
    io_out16(PORT_GC_INDEX, 0x0205);         /* write mode 2 */
    io_out16(PORT_GC_INDEX, 0x0003);         /* function select: replace */
    io_out16(PORT_SEQ_INDEX, 0x0f02);        /* map mask: every plane */
}

/*
 * VM.OVL VGA:0x271b
 *
 * NOT TRANSCRIBED YET. Draw a bitmap scaled. Reached through vector 0x43ca, and
 * taking three arguments where the plain blit takes four.
 */
void vm_blit_scaled(uint16_t hdr, int16_t x, int16_t y)
{
    (void)hdr;
    (void)x;
    (void)y;
    not_transcribed("VGA:0x271b");
}

