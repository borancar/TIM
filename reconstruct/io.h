/*
 * The port's own hardware boundary. NOT a transcription of anything.
 *
 * The original reached the VGA through `out dx, al` and writes to A000:0000.
 * There is no VGA here, so every one of those becomes a call into this file.
 * The split is a boundary *we* chose for porting; the binary does not prove
 * it, and it exists so that a modern backend can replace the hardware without
 * touching a single transcribed routine.
 *
 * The plane model is deliberately real. The game's blitter programs the
 * sequencer's map mask, the graphics controller's bit mask, write mode and
 * set/reset, and reads a byte to load the latches before writing it back.
 * Flattening that into "draw a pixel" would mean rewriting the blitter rather
 * than transcribing it, which is the one thing this project does not do.
 */
#ifndef IO_H
#define IO_H

#include <stdint.h>

#define VGA_PLANE_BYTES 0x10000
#define VGA_PLANES      4

/* Ports the transcribed code writes to, named as the hardware names them. */
#define PORT_SEQ_INDEX  0x3C4
#define PORT_SEQ_DATA   0x3C5
#define PORT_GC_INDEX   0x3CE
#define PORT_GC_DATA    0x3CF
#define PORT_CRTC_INDEX 0x3D4
#define PORT_CRTC_DATA  0x3D5
#define PORT_INPUT_ST1  0x3DA

void     io_out8(uint16_t port, uint8_t value);
uint8_t  io_in8(uint16_t port);

/* A000 segment access, going through the latches exactly as the hardware does. */
void     vga_write(uint16_t offset, uint8_t value);
uint8_t  vga_read(uint16_t offset);

/* What the CRTC would be scanning out: 8-bit palette indices, width*height. */
void     vga_compose(uint8_t *out, int32_t width, int32_t height);
int32_t  vga_visible_lines(void);
uint16_t vga_start_address(void);

/* The 18-bit DAC, as 8-bit RGB triples, for the backend and for --raw dumps. */
void     vga_palette_rgb(uint8_t out[768]);

void     io_reset(void);

/*
 * A trace of everything a routine did to the hardware, which is how a
 * transcription is proved against the original: the emulator records the same
 * sequence from the real code, and the two are compared event for event. This
 * needs no mapping of the original's whole machine state into the port's,
 * which a register-level comparison would.
 */
#define IO_TRACE_MAX 65536

typedef struct {
    uint16_t port;      /* or 0xA000 for a video memory access */
    uint16_t offset;    /* video memory offset, else 0 */
    uint8_t  value;
    uint8_t  is_read;
} io_event;

void     io_trace_begin(void);
int32_t  io_trace_count(void);
const io_event *io_trace_events(void);

#endif /* IO_H */
