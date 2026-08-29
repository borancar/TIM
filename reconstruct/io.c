/*
 * The port's own hardware boundary. NOT a transcription of anything.
 * See io.h for why the plane model is modelled rather than flattened.
 */
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "tim.h"

static uint8_t io_in8_raw(uint16_t port);

static uint8_t  planes[VGA_PLANES][VGA_PLANE_BYTES];
static uint8_t  latch[VGA_PLANES];

static uint8_t  seq_index, gc_index, crtc_index;
static uint8_t  seq[8];
static uint8_t  gc[16];
static uint8_t  crtc[32];
static uint8_t  dac[256][3];
static uint8_t  attr_pal[16];

static uint8_t  dac_index;
static int32_t  dac_phase;
static uint8_t  dac_latch[3];
static int32_t  dac_write_mode = 1;

static io_event trace[IO_TRACE_MAX];
static int32_t  trace_n = -1;      /* -1 = not tracing */

void io_trace_begin(void)      { trace_n = 0; }
int32_t io_trace_count(void)   { return trace_n < 0 ? 0 : trace_n; }
const io_event *io_trace_events(void) { return trace; }

static void trace_add(uint16_t port, uint16_t offset, uint8_t value, uint8_t rd)
{
    if (trace_n < 0 || trace_n >= IO_TRACE_MAX)
        return;
    trace[trace_n].port = port;
    trace[trace_n].offset = offset;
    trace[trace_n].value = value;
    trace[trace_n].is_read = rd;
    trace_n++;
}

/*
 * The VGA BIOS's own CRTC table for mode 12h. The game read-modify-writes
 * three of these registers, so they have to start at the values the BIOS left
 * rather than at zero - see docs/executable.md.
 */
static const uint8_t CRTC_MODE12[25] = {
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
    0xFF
};

void vga_load_plane(int32_t plane, const uint8_t *src, int32_t len)
{
    if (plane >= 0 && plane < VGA_PLANES && len <= VGA_PLANE_BYTES)
        memcpy(planes[plane], src, (size_t)len);
}

void vga_load_regs(const uint8_t *gc9, uint8_t map_mask)
{
    for (int32_t i = 0; i < 9; i++)
        gc[i] = gc9[i];
    seq[2] = (uint8_t)(map_mask & 0x0F);
}

void vga_store_plane(int32_t plane, uint8_t *dst, int32_t len)
{
    if (plane >= 0 && plane < VGA_PLANES && len <= VGA_PLANE_BYTES)
        memcpy(dst, planes[plane], (size_t)len);
}

static uint16_t alloc_seg[DOS_ALLOC_PRIMED];
static uint16_t alloc_largest[DOS_ALLOC_PRIMED];
static uint8_t  alloc_failed[DOS_ALLOC_PRIMED];
static int32_t  alloc_n, alloc_at;

void io_prime_dos_alloc(const uint16_t *segs, const uint16_t *largest,
                        const uint8_t *failed, int32_t n)
{
    if (n > DOS_ALLOC_PRIMED)
        n = DOS_ALLOC_PRIMED;
    for (int32_t i = 0; i < n; i++) {
        alloc_seg[i] = segs[i];
        alloc_largest[i] = largest[i];
        alloc_failed[i] = failed[i];
    }
    alloc_n = n;
    alloc_at = 0;
}

uint16_t io_dos_alloc(uint16_t paragraphs, uint16_t *largest, int32_t *failed)
{
    (void)paragraphs;
    if (alloc_at >= alloc_n) {
        /* Nothing primed: refuse loudly rather than invent an address. */
        not_transcribed("a DOS allocation with nothing primed for it");
        *failed = 1;
        *largest = 0;
        return 0;
    }
    *largest = alloc_largest[alloc_at];
    *failed = alloc_failed[alloc_at];
    return alloc_seg[alloc_at++];
}

/*
 * Release a DOS block. The port has no DOS arena - allocations are primed by
 * the verifier rather than served - so there is nothing to give back and this
 * does nothing. It exists so the transcribed routine that calls it reads the
 * way the original does instead of having the call quietly dropped.
 */
void io_dos_free(uint16_t seg)
{
    (void)seg;
}

/*
 * Borland's own `malloc`, which the port does not have.
 *
 * The runtime's heap is deliberately not transcribed - see STATUS.md - and a
 * port that faked a pointer would also have to fake the block header the real
 * one writes, which the whole-memory comparison would then catch. So this
 * refuses rather than inventing an address, and the routines that call it are
 * only verifiable on the paths that do not.
 */
uint16_t io_malloc(uint16_t bytes)
{
    (void)bytes;
    not_transcribed("Borland malloc, which the port has no heap for");
    return 0;
}

/*
 * Borland's own `free`, the counterpart of `io_malloc` above and refused for
 * the same reason: the port has no heap to give a block back to.
 */
void io_free(uint16_t off)
{
    heap_free(off);
}

void not_transcribed(const char *what)
{
    fprintf(stderr, "reached %s, which is not transcribed yet\n", what);
    abort();
}

/*
 * OURS: port 0x61, the speaker control latch.
 *
 * The sound driver only ever read-modify-writes it - `in al,0x61; or al,3` to
 * connect the timer to the speaker, `and al,0xfc` to disconnect - so what
 * matters is that a read gives back what was last written. Measured across a
 * whole run: the guest writes exactly two values, 0x20 and 0x23, and reads
 * return the last one, so the emulator models it as a plain latch and so does
 * this.
 *
 * It starts at 0x20 because that is what the machine already holds when the
 * driver first reads it - bit 5 is set before the game touches the port, and
 * every read-modify-write preserves it. Starting at zero makes the first note
 * write 0x03 where the original writes 0x23, which is exactly how this was
 * found.
 */
static uint8_t port61 = 0x20;

void io_reset(void)
{
    port61 = 0x20;
    memset(planes, 0, sizeof planes);
    memset(latch, 0, sizeof latch);
    memset(seq, 0, sizeof seq);
    memset(gc, 0, sizeof gc);
    memset(crtc, 0, sizeof crtc);
    memcpy(crtc, CRTC_MODE12, sizeof CRTC_MODE12);
    memset(dac, 0, sizeof dac);
    dac_index = 0;
    dac_phase = 0;
    dac_write_mode = 1;
    for (int32_t i = 0; i < 16; i++)
        attr_pal[i] = (uint8_t)i;
    seq[2] = 0x0F;                    /* map mask: all planes enabled */
    gc[8]  = 0xFF;                    /* bit mask: every bit writable */
}

void io_out8(uint16_t port, uint8_t value)
{
    trace_add(port, 0, value, 0);
    switch (port) {
    case PORT_SEQ_INDEX:  seq_index  = value & 0x07; break;
    case PORT_SEQ_DATA:   seq[seq_index] = value;    break;
    case PORT_GC_INDEX:   gc_index   = value & 0x0F; break;
    case PORT_GC_DATA:    gc[gc_index] = value;      break;
    case PORT_CRTC_INDEX: crtc_index = value & 0x1F; break;
    case PORT_CRTC_DATA:  crtc[crtc_index] = value;  break;
    case 0x61:            port61 = value;            break;
    case PORT_DAC_WRITE:
        dac_index = value;
        dac_phase = 0;
        dac_write_mode = 1;
        break;
    case PORT_DAC_READ:
        dac_write_mode = 0;
        break;
    case PORT_DAC_DATA:
        dac_latch[dac_phase++] = (uint8_t)(value & 0x3F);
        if (dac_phase == 3) {
            dac[dac_index][0] = dac_latch[0];
            dac[dac_index][1] = dac_latch[1];
            dac[dac_index][2] = dac_latch[2];
            dac_index++;
            dac_phase = 0;
        }
        break;
    default: break;
    }
}

void io_out16(uint16_t port, uint16_t value)
{
    /* A word write to an index port carries the index in the low byte and the
     * data in the high byte, which is how the driver sets the start address. */
    io_out8(port, (uint8_t)(value & 0xFF));
    io_out8((uint16_t)(port + 1), (uint8_t)(value >> 8));
}

uint16_t bios_crtc_base(void) { return PORT_CRTC_INDEX; }

uint16_t vga_seg_offset(uint16_t seg)
{
    return (uint16_t)((seg - 0xA000u) << 4);
}

uint8_t io_in8(uint16_t port)
{
    uint8_t v = io_in8_raw(port);
    trace_add(port, 0, v, 1);
    return v;
}

static uint8_t io_in8_raw(uint16_t port)
{
    switch (port) {
    case PORT_SEQ_DATA:  return seq[seq_index];
    case PORT_GC_DATA:   return gc[gc_index];
    case PORT_CRTC_DATA: return crtc[crtc_index];
    /* The DAC state register: 3 while the write index is the live one. */
    case PORT_DAC_READ:  return (uint8_t)(dac_write_mode ? 0x03 : 0x00);
    case 0x61:           return port61;
    /*
     * Input status 1. Bit 3 is vertical retrace, bit 0 display enable.
     *
     * OURS, and it has to toggle. The driver waits for a whole retrace *edge*
     * - first while the bit is set, then until it is set again - so a constant
     * answer hangs one of the two loops forever whichever value is chosen.
     * Toggling per read satisfies both and returns promptly, which is what a
     * port that composes whole frames wants.
     */
    case PORT_INPUT_ST1: {
        static uint8_t n;
        n++;
        return (uint8_t)(0x01 | ((n & 1) ? 0x00 : 0x08));
    }
    default: return 0x00;
    }
}

/* A read loads all four latches and returns the plane the GC selects. */
uint8_t vga_read(uint16_t offset)
{
    trace_add(0xA000, offset, 0, 1);
    for (int32_t p = 0; p < VGA_PLANES; p++)
        latch[p] = planes[p][offset];
    return latch[gc[4] & 0x03];
}

static uint8_t apply_rotate(uint8_t v)
{
    uint8_t r = gc[3] & 0x07;
    return (uint8_t)((v >> r) | (v << (8 - r)));
}

static uint8_t combine(uint8_t src, uint8_t lat)
{
    switch ((gc[3] >> 3) & 0x03) {
    case 1:  return (uint8_t)(src & lat);
    case 2:  return (uint8_t)(src | lat);
    case 3:  return (uint8_t)(src ^ lat);
    default: return src;
    }
}

/*
 * The plane update without the trace event, so a 16-bit write can do two
 * bytes while recording the single access the hardware actually saw.
 */
static void vga_write_raw(uint16_t offset, uint8_t value)
{
    uint8_t mapmask = seq[2] & 0x0F;
    uint8_t bitmask = gc[8];
    uint8_t mode    = gc[5] & 0x03;

    for (int32_t p = 0; p < VGA_PLANES; p++) {
        uint8_t src, out;

        if (!(mapmask & (1u << p)))
            continue;

        switch (mode) {
        case 0:
            src = apply_rotate(value);
            if (gc[1] & (1u << p))                  /* enable set/reset */
                src = (gc[0] & (1u << p)) ? 0xFF : 0x00;
            out = combine(src, latch[p]);
            out = (uint8_t)((out & bitmask) | (latch[p] & (uint8_t)~bitmask));
            break;
        case 1:
            /* Latch straight through: the byte-copy mode a blit uses. */
            out = latch[p];
            break;
        case 2:
            src = (value & (1u << p)) ? 0xFF : 0x00;
            out = combine(src, latch[p]);
            out = (uint8_t)((out & bitmask) | (latch[p] & (uint8_t)~bitmask));
            break;
        default: /* 3 */
            src = (gc[0] & (1u << p)) ? 0xFF : 0x00;
            out = (uint8_t)(apply_rotate(value) & bitmask);
            out = (uint8_t)((src & out) | (latch[p] & (uint8_t)~out));
            break;
        }
        planes[p][offset] = out;
    }
}

void vga_write(uint16_t offset, uint8_t value)
{
    trace_add(0xA000, offset, value, 0);
    vga_write_raw(offset, value);
}

/*
 * A 16-bit write to the aperture. The guest's `rep movsw` moves words, and the
 * emulator's hook fires **once per access**, recording the low byte - so a port
 * that wrote the two bytes separately would produce twice as many events as the
 * original and disagree even with identical planes. That is not a theoretical
 * worry: it is exactly how VGA:0x13b9 first failed.
 */
void vga_write16(uint16_t offset, uint16_t value)
{
    trace_add(0xA000, offset, (uint8_t)(value & 0xFF), 0);
    vga_write_raw(offset, (uint8_t)(value & 0xFF));
    vga_write_raw((uint16_t)(offset + 1), (uint8_t)(value >> 8));
}

int32_t vga_visible_lines(void)
{
    /* Start Vertical Blank: ten bits across 0x15, 0x07 bit 3, 0x09 bit 5. */
    int32_t svb = crtc[0x15]
                | (((crtc[0x07] >> 3) & 1) << 8)
                | (((crtc[0x09] >> 5) & 1) << 9);
    return svb;
}

uint16_t vga_start_address(void)
{
    return (uint16_t)((crtc[0x0C] << 8) | crtc[0x0D]);
}

void vga_compose(uint8_t *out, int32_t width, int32_t height)
{
    int32_t row_bytes = crtc[0x13] ? crtc[0x13] * 2 : width / 8;
    int32_t span = width / 8;
    int32_t blank = vga_visible_lines();
    uint16_t base = vga_start_address();

    memset(out, 0, (size_t)(width * height));
    for (int32_t y = 0; y < height && y < blank; y++) {
        int32_t src = base + y * row_bytes;
        uint8_t *dst = out + (size_t)y * width;
        for (int32_t bx = 0; bx < span; bx++) {
            uint16_t o = (uint16_t)(src + bx);
            uint8_t b0 = planes[0][o], b1 = planes[1][o];
            uint8_t b2 = planes[2][o], b3 = planes[3][o];
            if (!(b0 | b1 | b2 | b3))
                continue;
            for (int32_t bit = 0; bit < 8; bit++) {
                int32_t sh = 7 - bit;
                uint8_t v = (uint8_t)(((b0 >> sh) & 1)
                                    | (((b1 >> sh) & 1) << 1)
                                    | (((b2 >> sh) & 1) << 2)
                                    | (((b3 >> sh) & 1) << 3));
                if (v)
                    dst[bx * 8 + bit] = attr_pal[v];
            }
        }
    }
}

void vga_palette_rgb(uint8_t out[768])
{
    for (int32_t i = 0; i < 256; i++) {
        /* Six-bit DAC to eight bits the way the hardware does it: bit
         * replication, not a multiply. v*255/63 agrees at 0 and 63 and is
         * one out in the middle, which compares as a difference on every
         * mid-tone pixel. */
        for (int32_t c = 0; c < 3; c++) {
            uint8_t v = dac[i][c] & 0x3F;
            out[i * 3 + c] = (uint8_t)((v << 2) | (v >> 4));
        }
    }
}
