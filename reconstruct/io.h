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
#include <stdio.h>

/*
 * OURS: this layer's whole state, so a machine reached by playing can be
 * replayed by a tool. See the end of io.c for what counts as state and what is
 * host scaffolding. 1 on success, 0 on a short read, a bad magic or a version
 * this build does not know.
 */
/*
 * OURS: the whole port, written out on a keypress, so a state reached by
 * playing can be compared against the runner. See io.c.
 */
void     io_next_snapshot_path(char *buf, size_t n, const char *prefix);
int32_t  io_write_snapshot(const char *path);
int32_t  io_read_snapshot(const char *path);

/*
 * OURS: a key into the BIOS ring, scancode in the high byte and ASCII in
 * the low one. `bios_read_key` is the guest end of it.
 */
void     io_key_press(uint16_t key);
void     io_bios_init(void);

/*
 * OURS: the PC speaker. `fn` is told the tone in hertz and whether the gate
 * at port 0x61 has it connected; it is called whenever either changes.
 */
void     io_on_speaker(void (*fn)(double hz, int32_t on));

/*
 * OURS: a block of eight-bit PCM the Sound Blaster was asked to play, with
 * its sample rate. Handed over whole, as the DMA hands it to the card.
 */
void     io_on_pcm(void (*fn)(const uint8_t *pcm, int32_t n, int32_t rate));

/*
 * OURS: a second, passive listener on the same blocks, for capture. It is
 * separate from `io_on_pcm` so that recording never displaces playback - the
 * developer binary registers one and the window registers the other, and a
 * run does both at once.
 */
void     io_on_pcm_tap(void (*fn)(const uint8_t *pcm, int32_t n, int32_t rate));
void     io_on_pcm_tap2(void (*fn)(const uint8_t *pcm, int32_t n, int32_t rate));

/*
 * OURS: how many OPL key-on events have gone to the chip. A sound that makes
 * no PCM block and no key-on made no sound at all.
 */
long     io_keyon_count(void);

/*
 * The card's completion interrupt. A driver registers the handler for the IRQ
 * it thinks the card is on; only the one the card is actually on is kept.
 * `io_sb_poll` fires it once the block it is playing has had time to play out,
 * and the display service calls it once a frame.
 */
void     io_on_sb_irq(uint8_t irq, void (*fn)(void));
void     io_sb_poll(void);
void     io_sb_wait(void);

/*
 * For a runner that can deliver a real interrupt to guest code: answers 1 and
 * the IRQ number when the card has a completion pending and no C handler is
 * registered for it. `io_sb_poll` handles the C case and leaves this one.
 */
int32_t  io_sb_irq_take(uint8_t *irq);
int32_t  io_sb_irq_owed(void);
void     io_sb_irq_delivered(void);

int32_t  io_state_save(FILE *f);
int32_t  io_state_load(FILE *f);

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
#define PORT_ATTR       0x3C0
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

/*
 * OURS: register what to do when the guest flips the page - the write to CRTC
 * 0x0C. The backend registers itself, so io.c never has to know a window
 * exists and devtim links the same file with nothing registered.
 */
void     io_on_present(void (*fn)(void));

/* reconstruct/devdump.c - the part list at a chosen flip. Ours, not a
 * transcription, and a no-op unless TIM_PARTS asks for it. */
void     dev_flip_dump(int32_t flip);
/* The frame the port stopped on, when TIM_FRAME asks. devmain.c registers it
 * as the abort hook; the shipping binary has no equivalent, deliberately. */
void     dev_final_frame(void);

/*
 * OURS: start the `TIM_WAV` capture if it was asked for. Developer binary
 * only, like everything else declared here from devdump.c.
 */
void     dev_wav_open(void);

/*
 * OURS: start the `TIM_SFXDIR` capture - one WAV per distinct waveform.
 */
void     dev_sfx_open(void);

/*
 * Called when the game finishes writing a file - `io_dos_close` on an overlay
 * handle. The shipping binary's version does nothing; `devdump.c` writes the
 * bytes out, so a save can be compared against the original's **byte for
 * byte** rather than only by the screen it leaves behind. A machine file never
 * reaches a pixel, so nothing else can check it.
 */
void     dev_file_written(const char *name, const uint8_t *data, uint32_t len);

/*
 * A part hook with no transcription. The developer binary can be asked to
 * report it and carry on - answering non-zero - so one run names every hook a
 * screen needs instead of aborting on the first. What ships answers 0 and the
 * stub aborts, which is the only correct behaviour for a missing hook.
 */
int32_t  dev_survey_hook(uint16_t off, uint16_t kind);

/*
 * OURS: refresh the window because time has passed. The flip is the right cue
 * for a capture and the wrong one for a window - see io.c, and the Sierra logo,
 * which never flips at all.
 */
void     io_service_display(void);

/*
 * OURS: what to do just before a stub aborts. The window backend registers a
 * hold here so the last frame stays up; devtim registers nothing.
 */
void     io_on_abort(void (*fn)(void));

/*
 * OURS: the guest's clock. `io_set_timer` registers the transcribed interrupt
 * handler and the divisor the guest programmed; `io_service_timer` runs it for
 * however much real time has passed. See io.c for where it is called from and
 * why there.
 */
void     io_set_timer(void (*fn)(void));
void     io_service_timer(void);
void     io_stop_timer(void);

/*
 * OURS: hold the timer off. The guest's own `cli` regions are what these stand
 * for - see io.c, which lists the three that need them and says that none has
 * them yet.
 */
void     io_lock(void);
void     io_unlock(void);
void     call_timer_handler(uint16_t off, uint16_t seg);
uint16_t call_part_init(uint16_t off, uint16_t seg, uint16_t part);
void call_part_setup(uint16_t off, uint16_t seg, uint16_t part);
uint16_t call_part_hook(uint16_t off, uint16_t seg, uint16_t part,
                        const char *what);
uint16_t call_part_drive(uint16_t off, uint16_t seg,
                         uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, uint16_t p6, uint16_t p7);

/* What the CRTC would be scanning out: 8-bit palette indices, width*height. */
void     vga_compose(uint8_t *out, int32_t width, int32_t height);
int32_t  vga_visible_lines(void);
int32_t  vga_line_compare(void);
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
 * OURS: the verifier's allocation-underrun flag. Armed, an allocation with
 * nothing primed is recorded and answered as a failure instead of aborting the
 * process the library is loaded into - see the note in io.c. `tim` and
 * `devtim` never arm it and abort exactly as before.
 */
void     io_arm_stub_trap(void);
void     io_disarm_stub_trap(void);
int16_t  io_stub_reached(void);   /* how many ran past the primed list */
int32_t  io_primed_allocs(void);  /* how many were primed */

/*
 * OURS: DOS memory allocation, INT 21h AH=48h. There is no DOS here and no
 * arena, so the port cannot decide where a block goes. tools/verify.py primes
 * these with what DOS actually answered during the original's own call, which
 * leaves everything around the allocation - the size arithmetic, the rounding,
 * the zero fill - genuinely compared, rather than declaring the whole routine
 * unverifiable.
 */
/*
 * How many of the original's DOS allocations `tools/verify.py` can replay for
 * one compared call. It was 16, and `load_all_parts` makes far more than that
 * - it loads every part bitmap - so it exhausted the list on every run and
 * could never be verified. Truncation is silent in `io_prime_dos_alloc`, but
 * it is no longer invisible: running off the end sets the harness's underrun
 * flag and the comparison reports RAN OUT rather than a difference.
 *
 * At 512 it verifies. An earlier version of this note said it still ran out
 * and built a "finding" on top of that - the port asking for more allocations
 * than the original made, narrowed to something about running the calls in
 * sequence. **All of it was wrong**, and the cause was one line in the
 * Makefile: `libtim.so` depended on the `.c` files and not the headers, so
 * raising this constant rebuilt nothing and the run used the sixteen-entry
 * library. It only took effect when an unrelated edit to io.c forced a
 * rebuild. The dependency is fixed; the lesson is that a stale build answers
 * confidently and a header-only change is exactly when to distrust it.
 */
#define DOS_ALLOC_PRIMED 512

void     io_prime_dos_alloc(const uint16_t *segs, const uint16_t *largest,
                            const uint8_t *failed, int32_t n);
uint16_t io_dos_alloc(uint16_t paragraphs, uint16_t *largest, int32_t *failed);
void     io_dos_free(uint16_t seg);
uint16_t io_dos_resize(uint16_t seg, uint16_t paragraphs);

/*
 * OURS: hand the arena the memory the program's own block does not use, which
 * is what Borland's startup does with INT 21h AH=4Ah before it calls main.
 * Under the verifier this is never called and primed allocations answer
 * instead.
 */
void     io_dos_arena_reset(uint16_t first_free, uint16_t mem_top);

/*
 * OURS: put the recovered image in memory the way DOS's loader would, apply its
 * relocations, and set DGROUP, the stack and the arena. Answers 0 if either
 * file could not be read. See io.c.
 */
int32_t  io_load_program(const char *img_path, const char *exe_path);
void     io_dos_free(uint16_t seg);
uint16_t io_malloc(uint16_t bytes);
void     io_free(uint16_t off);

/*
 * DOS file services, served read-only from the game directory. See io.c.
 */
void     io_set_game_dir(const char *path);
void     io_prime_file(int16_t handle, const char *name, int32_t pos);
void     io_dos_getdate(uint16_t *year, uint16_t *monthday,
                        uint16_t *weekday);
uint16_t io_bios_display_combination(void);
/*
 * OURS: the mouse, INT 33h. `io_mouse_reset` answers whether a driver is there
 * - the port says yes, as the reference emulator does. The rest are settings
 * the driver holds and guest memory never sees; see io.c.
 */
uint16_t io_mouse_reset(void);
void     io_mouse_show(void);
void     io_mouse_hide(void);
void     io_mouse_move_to(uint16_t x, uint16_t y);
void     io_mouse_set_speed(uint16_t x_mickeys, uint16_t y_mickeys);
void     io_mouse_set_x_range(uint16_t lo, uint16_t hi);
void     io_mouse_set_y_range(uint16_t lo, uint16_t hi);
void     io_mouse_set_handler(uint16_t mask, uint16_t off, uint16_t seg);
/* OURS: the host's pointer, delivered as the driver's event. */
void     io_mouse_input(int32_t x, int32_t y, uint16_t buttons);

uint16_t io_dos_curdrive(void);
void     io_dos_getcwd(uint8_t *buf);
int16_t  io_dos_getattr(const char *name);
int16_t  io_dos_chdir(const char *path);
int16_t  io_dos_setdisk(uint8_t drive);
int16_t  io_dos_findfirst(const char *pattern, uint16_t attr,
                          uint8_t *name, uint8_t *attr_out,
                          uint32_t *size_out);
int16_t  io_dos_findnext(uint8_t *name, uint8_t *attr_out,
                         uint32_t *size_out);
int16_t  io_dos_devinfo(int16_t handle);
int16_t  io_dos_open(const char *name);
int16_t  io_dos_creat(const char *name);
int16_t  io_dos_write(int16_t handle, const uint8_t *buf, uint16_t count);
int32_t  io_dos_forget(const char *name);
int16_t  io_dos_read(int16_t handle, uint8_t *buf, uint16_t count);
int32_t  io_dos_lseek(int16_t handle, int32_t pos, int16_t whence);
void     io_dos_close(int16_t handle);

void     io_bios_set_mode(uint16_t mode);
void     io_reset(void);

/*
 * A trace of everything a routine did to the hardware, which is how a
 * transcription is proved against the original: the emulator records the same
 * sequence from the real code, and the two are compared event for event. This
 * needs no mapping of the original's whole machine state into the port's,
 * which a register-level comparison would.
 */
/*
 * Big enough for a whole frame of the machine: `draw_machine` alone writes
 * 125,896 times, and the trace holds reads as well. At 65,536 it filled part
 * way through and the comparison then read as "the port stopped early", which
 * is the most misleading shape a limit can take - the verifier now says when
 * the trace was cut off rather than letting it look like a difference.
 */
#define IO_TRACE_MAX (1 << 20)

typedef struct {
    uint16_t port;      /* or 0xA000 for a video memory access */
    uint16_t offset;    /* video memory offset, else 0 */
    uint8_t  value;
    uint8_t  is_read;
} io_event;

void     io_trace_begin(void);
int32_t  io_trace_count(void);
int32_t  io_trace_full(void);
const io_event *io_trace_events(void);

#endif /* IO_H */
