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
#define PORT_DAC_MASK   0x3C6
#define PORT_DAC_READ   0x3C7
#define PORT_DAC_WRITE  0x3C8
#define PORT_DAC_DATA   0x3C9
#define PORT_INPUT_ST1  0x3DA

void     io_out8(uint16_t port, uint8_t value);
void     io_out16(uint16_t port, uint16_t value);
uint8_t  io_in8(uint16_t port);

/*
 * The original reads the CRTC's base port out of the BIOS data area at
 * 0040:0063 rather than assuming one. There is no BIOS here, so the answer
 * comes from the port instead - the substitution belongs on this side of the
 * boundary, not inside a transcribed routine.
 */
uint16_t bios_crtc_base(void);

/*
 * OURS. The driver holds its pages as real-mode **segments** - 0xA000 and
 * 0xA820 - and reaches them with DS/ES. There are no segments here, so a page
 * segment becomes an offset into the planes. The arithmetic is the hardware's
 * own: a segment is sixteen bytes.
 */
uint16_t vga_seg_offset(uint16_t seg);

/* A000 segment access, going through the latches exactly as the hardware does. */
void     vga_write(uint16_t offset, uint8_t value);
void     vga_write16(uint16_t offset, uint16_t value);
uint8_t  vga_read(uint16_t offset);

/* What the CRTC would be scanning out: 8-bit palette indices, width*height. */
void     vga_compose(uint8_t *out, int32_t width, int32_t height);
int32_t  vga_visible_lines(void);
uint16_t vga_start_address(void);

/* The 18-bit DAC, as 8-bit RGB triples, for the backend and for --raw dumps. */
void     vga_palette_rgb(uint8_t out[768]);

/*
 * OURS, for verification. A routine that *reads* video memory - the latch copy
 * does nothing else - can only be compared against the original if it is
 * looking at the same pixels, so tools/verify.py loads the original's planes
 * in before the call and reads them back out after.
 */
void     vga_load_plane(int32_t plane, const uint8_t *src, int32_t len);
void     vga_load_regs(const uint8_t *gc9, uint8_t map_mask);
void     vga_store_plane(int32_t plane, uint8_t *dst, int32_t len);

/*
 * OURS. Called where a transcribed routine branches into one that has not been
 * transcribed yet. It aborts rather than returning, because a silently wrong
 * pixel is exactly what this project exists to avoid.
 */
void     not_transcribed(const char *what);

/*
 * OURS: DOS memory allocation, INT 21h AH=48h. There is no DOS here and no
 * arena, so the port cannot decide where a block goes. tools/verify.py primes
 * these with what DOS actually answered during the original's own call, which
 * leaves everything around the allocation - the size arithmetic, the rounding,
 * the zero fill - genuinely compared, rather than declaring the whole routine
 * unverifiable.
 */
#define DOS_ALLOC_PRIMED 16

void     io_prime_dos_alloc(const uint16_t *segs, const uint16_t *largest,
                            const uint8_t *failed, int32_t n);
uint16_t io_dos_alloc(uint16_t paragraphs, uint16_t *largest, int32_t *failed);
void     io_dos_free(uint16_t seg);
uint16_t io_malloc(uint16_t bytes);
void     io_free(uint16_t off);
void     io_file_seek(uint16_t handle, uint16_t lo, uint16_t hi);
uint16_t io_fopen(uint16_t name, uint16_t mode);

/*
 * DOS file services, served read-only from the game directory. See io.c.
 */
void     io_set_game_dir(const char *path);
void     io_prime_file(int16_t handle, const char *name, int32_t pos);
int16_t  io_dos_open(const char *name);
int16_t  io_dos_read(int16_t handle, uint8_t *buf, uint16_t count);
int32_t  io_dos_lseek(int16_t handle, int32_t pos, int16_t whence);
void     io_dos_close(int16_t handle);
void     io_fclose(uint16_t file);

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
