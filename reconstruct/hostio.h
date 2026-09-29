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

#include <stddef.h>
#include <stdint.h>

/* A host attribute, which Turbo C++ 3.0 reads as nothing (see dgroup.h,
   "The two compilers"). Ours. */
#ifdef __TURBOC__
#  define WEAK
#else
#  define WEAK __attribute__((weak))
#endif

struct part;

/* **Nothing below is Borland C++'s.** It is the port's IO layer, which only a
   host branch calls, and a declaration the original compiler has to read is
   memory it no longer has: BC++ 2.0, run with what a real machine left,
   failed vidload.c's `load_video_driver` with "Register allocation failure"
   once dos.c's twelve prototypes had joined the headers. */
#ifndef __TURBOC__

/*
 * OURS: **the BIOS data area**, 0040:0000, as much of it as the game reads -
 * the equipment word, the keyboard flags and ring, the diskette motor bits
 * and the video mode. The ring's head and tail are offsets from 0040:0000,
 * which is what the BIOS stores, so `kbd_buffer` is indexed `(off - 0x1e) / 2`.
 */
struct bios_data_area {
    uint8_t   pad_00[0x10];
    uint8_t   equipment;          /* 0x10  the equipment word's low byte; bits
                                     4-5 are the initial video mode */
    uint8_t   equipment_high;     /* 0x11 */
    uint8_t   pad_12[5];
    uint8_t   kbd_flags;          /* 0x17  shift, ctrl, alt, the locks */
    uint8_t   kbd_flags2;         /* 0x18 */
    uint8_t   alt_keypad;         /* 0x19 */
    uint16_t  kbd_head;           /* 0x1a  the next key to read */
    uint16_t  kbd_tail;           /* 0x1c  where the next key goes */
    uint16_t  kbd_buffer[16];     /* 0x1e..0x3e */
    uint8_t   pad_3e;
    uint16_t  motor_status;       /* 0x3f  diskette motors, one bit a drive */
    uint8_t   pad_41[8];
    uint8_t   video_mode;         /* 0x49 */
    uint8_t   pad_4a[0x36];
    uint16_t  kbd_start;          /* 0x80  the ring's first word */
    uint16_t  kbd_end;            /* 0x82  one past its last */
} __attribute__((packed));

extern struct bios_data_area g_bios;

/*
 * OURS: the two ROM bytes `detect_pcjr` reads - the model byte at F000:FFFE,
 * an AT's 0xfc here, and F000:C000, where a PCjr has its 0x21 - and the
 * monochrome adapter's text screen at B000:0000, which `mono_clear` and
 * `mono_puts` write.
 */
extern const uint8_t g_rom_model;
extern const uint8_t g_rom_c000;
extern uint16_t g_mono_screen[0x800];

/* One scancode into the game's own keyboard interrupt; bit 7 is a break. */
void     io_keyboard_scancode(uint8_t code);
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
/* OURS: the host's formatting, for the game's units, which include no
   <stdio.h> - `not_transcribed` messages are built with `io_format`. */
void     io_format(char *buf, uint32_t size, const char *fmt, ...);
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

/* The Sound Blaster Pro mixer's FM volume, 0..7 a side. See io.c. */
void io_fm_volume(uint8_t *left, uint8_t *right);
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

/*
 * OURS: reconstruct/devlua.c, the scripting listener - see that file. It is
 * **weak** because only `devtim` links it: `libtim.so` and `covtim` link the
 * other dev*.c files and must not carry a socket, and a weak symbol lets them
 * link with the hook simply absent. Test it before calling it.
 */
void     dev_lua_flip(int32_t flip) WEAK;

/*
 * OURS: a sound the game asked to play, by identifier. Called unconditionally
 * from `play_sound`, the way `dev_flip_dump` is called from the page flip, so
 * the *decision* to report is dev-only and the shipping binary gets a no-op.
 */
void     dev_sound_played(int16_t id);

/* The developer build's note that a puzzle was solved - see devdump.c. */
void     dev_level_solved(int16_t level, int16_t score);
int32_t  dev_simulate_machine(int32_t max_frames);

/*
 * OURS: write every part's bin icon as raw pixels. See devdump.c.
 */
void     dev_part_pics(void);

/*
 * OURS: report which part kinds each level holds. See devdump.c.
 */
void     dev_level_scan(void);

/*
 * OURS: tell the autoplay driver that the intro is behind us, which is what a
 * restored snapshot means. The intro is the one phase it cannot read off the
 * state word, because the animations and the running machine both sit at
 * 0x2000. See devdump.c.
 */

/*
 * OURS: a date for `io_dos_getdate` to answer instead of its fixed one.
 * Answers 1 when it filled the three in, 0 to leave the default alone - the
 * shipping binary always answers 0, so a comparison's date cannot move.
 */
int32_t  dev_date_override(uint16_t *year, uint16_t *monthday,
                           uint16_t *weekday);

/* OURS: whether TIM_TRACE names the `sfx` channel. */
int32_t  trace_asks_sfx(void);
int32_t  trace_asks_level(void);
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
 * OURS: render the OPL while one sound plays, into its own WAV. See devwav.c.
 */
void     dev_fm_capture(int32_t id, double seconds);

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

/* What the CRTC would be scanning out: 8-bit palette indices, width*height. */
void     vga_compose(uint8_t *out, int32_t width, int32_t height);
int32_t  vga_visible_lines(void);
int32_t  vga_line_compare(void);
uint16_t vga_start_address(void);

/* The 18-bit DAC, as 8-bit RGB triples, for the backend and for --raw dumps. */
void     vga_palette_rgb(uint8_t out[768]);

/*
 * OURS. Called where a transcribed routine branches into one that has not been
 * transcribed yet. It aborts rather than returning, because a silently wrong
 * pixel is exactly what this project exists to avoid.
 */
void     not_transcribed(const char *what);
void     port_abort(const char *msg);

/*
 * OURS: DOS memory allocation, INT 21h AH=48h, the resize and the free, over
 * the arena `io_dos_arena_reset` hands out.
 */
uint16_t io_dos_alloc(uint16_t paragraphs, uint16_t *largest, int32_t *failed);
void     io_dos_free(uint16_t seg);
uint16_t io_dos_resize(uint16_t seg, uint16_t paragraphs);

/*
 * OURS: hand the arena the memory the program's own block does not use, which
 * is what Borland's startup does with INT 21h AH=4Ah before it calls main.
 */
void     io_dos_arena_reset(uint16_t first_free, uint16_t mem_top);

/*
 * OURS: DGROUP's address, the stack, the arena and the BIOS bytes - what
 * DOS's loader and Borland's startup leave behind. See hostio.c.
 */
void     io_start_program(void);
void     io_dos_free(uint16_t seg);

/*
 * DOS directory services, on the game directory. See hostio.c. The files
 * themselves the game opens with the C library.
 */
void     io_set_game_dir(const char *path);
void     io_dos_getdate(uint16_t *year, uint16_t *monthday,
                        uint16_t *weekday);
uint16_t io_bios_display_combination(void);

/*
 * OURS: the BIOS font-pointer service, INT 10h AX=1130h. It answers in
 * **ES:BP** rather than in AX, so it needs a pair; the struct is what a C
 * caller can be handed and what `vm_init` files the way the original does.
 */
struct bios_font {
    uint16_t es;
    uint16_t bp;
};

struct bios_font io_bios_font(uint8_t which);
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
void     io_mouse_set_handler(uint16_t mask, void (*handler)(void));
/* OURS: the host's pointer, delivered as the driver's event. */
void     io_mouse_input(int32_t x, int32_t y, uint16_t buttons);

uint16_t io_dos_curdrive(void);
void     io_dos_getcwd(uint8_t *buf);
int16_t  io_dos_getattr(const char *name);
int16_t  io_dos_setattr(const char *name, uint16_t attr);
int16_t  io_dos_mkdir(const char *path);
int16_t  io_dos_rmdir(const char *path);
uint16_t io_dos_drive_fixed(uint8_t drive);
void     io_dos_disk_reset(void);
int16_t  io_dos_chdir(const char *path);
int16_t  io_dos_setdisk(uint8_t drive);
int16_t  io_dos_findfirst(const char *pattern, uint16_t attr,
                          uint8_t *name, uint8_t *attr_out,
                          uint32_t *size_out);
int16_t  io_dos_findnext(uint8_t *name, uint8_t *attr_out,
                         uint32_t *size_out);
int16_t  io_dos_unlink(const char *name);

void     io_bios_set_mode(uint16_t mode);
void     io_reset(void);

#endif /* !__TURBOC__ */

#endif /* IO_H */
