#define _POSIX_C_SOURCE 200809L
/*
 * The port's own hardware boundary. NOT a transcription of anything.
 * See io.h for why the plane model is modelled rather than flattened.
 */
#include <string.h>

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "dgroup.h"
#include "io.h"
#include "tim.h"

static uint8_t io_in8_raw(uint16_t port);

static uint8_t  planes[VGA_PLANES][VGA_PLANE_BYTES];
static uint8_t  latch[VGA_PLANES];

static uint8_t  seq_index, gc_index, crtc_index;
static uint8_t  seq[8];
static uint8_t  gc[16];
static uint8_t  crtc[32];

/*
 * OURS: what to do when the guest finishes a frame. The backend registers
 * itself here rather than io.c calling it, so that devtim - which has no window
 * and must not link one - is the same io.c with nothing registered.
 */
/* OURS: a monotonic clock, for the tick rate and the window's refresh. */
static double io_now(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/*
 * OURS: the display's frame rate, and where in a frame the clock says we are.
 *
 * Mode 0x12 is 640x480 at **59.94 Hz** - 525 lines of which 480 are active,
 * the standard IBM timing, and the rate DOSBox reports for this game. The game
 * does not change it: it moves Start Vertical Blank and leaves every other
 * timing register alone, so the Sierra logo and the intro screens run at the
 * same rate and only the number of *displayed* lines differs.
 *
 * The port does not model the BIOS's timing registers - `io_bios_set_mode`
 * zeroes the CRTC and only the handful of registers the game itself writes
 * mean anything - so this comes from the clock rather than from a register
 * file that is not there.
 */
#define VGA_FRAME_HZ     59.94
#define VGA_TOTAL_LINES  525.0
#define VGA_ACTIVE_LINES 480.0

/* Where in the frame we are, 0 at the top of the picture and 1 at the next. */
static double vga_frame_phase(void)
{
    double f = io_now() * VGA_FRAME_HZ;

    return f - (double)(long long)f;
}

static int32_t vga_in_vblank(void)
{
    return vga_frame_phase() >= VGA_ACTIVE_LINES / VGA_TOTAL_LINES;
}

static void (*present_hook)(void);
static void (*abort_hook)(void);

/*
 * The thread that may talk to the display, which is the one that set the hook
 * up - `main`'s. **SDL's renderer belongs to one thread and must be used from
 * that thread only.**
 *
 * The timer runs on a thread of its own, and the guest's timer handler writes
 * I/O ports like any other guest code, so it reached `io_service_display` and
 * called SDL from the wrong thread. Two threads inside `SDL_RenderPresent` at
 * once wedges the renderer: gdb caught both of them in `SDL_BlitCopy`, the
 * main one under `vm_show_page`, and with the main thread stuck inside SDL the
 * game stops - which is a screen that freezes with the machine half finished,
 * and, when the two corrupt each other rather than jam, a segfault after a few
 * minutes. Three different-looking faults, one cause.
 *
 * `present_busy` did not catch it: it is a re-entry guard for *one* thread and
 * says nothing about two.
 */
static pthread_t display_thread;
static int32_t   display_thread_known;

void io_on_present(void (*fn)(void))
{
    present_hook = fn;
    display_thread = pthread_self();
    display_thread_known = 1;
}

/* Whether this thread is the one that owns the window. */
static int32_t on_display_thread(void)
{
    return !display_thread_known
           || pthread_equal(pthread_self(), display_thread);
}

/*
 * OURS: refresh the window because time has passed, not because the guest
 * finished a frame.
 *
 * The page-flip hook above is the right cue for a *capture* - it is the one
 * instant a frame is complete and not half-drawn. It is the wrong cue for a
 * window, and the Sierra logo is what showed that: its animation loop draws
 * straight onto the page that is already being displayed and never flips at
 * all, so nothing was ever redrawn and the screen stayed on whatever the last
 * flip left. On the real machine the CRTC scans the page out sixty times a
 * second whether the game asks or not.
 *
 * So the port refreshes on the clock as well, at the mode's own 59.94 Hz,
 * from wherever the
 * guest happens to touch the display hardware. The rate limit is what stops a
 * blit turning into one present per register write, and the re-entry guard is
 * what stops a present that itself reads the VGA from calling itself.
 */
static double present_last;
static int32_t present_busy;

void io_service_display(void)
{
    double now;

    if (!present_hook || present_busy || !on_display_thread())
        return;

    now = io_now();
    if (now - present_last < 1.0 / VGA_FRAME_HZ)
        return;

    present_last = now;
    present_busy = 1;
    present_hook();
    present_busy = 0;
}

void io_on_abort(void (*fn)(void))
{
    abort_hook = fn;
}
/*
 * `TIM_TRACE=crtc,dac` prints the register writes that decide what the screen
 * even is - the blanking line, the line compare, and the DAC. Ours: the
 * original has no such thing, and a fault that is invisible in a frame of
 * indices (a correct picture under an all-black palette) is a line of output
 * here. Off unless the variable asks.
 */
static int32_t trace_crtc_on = -1;
static int32_t trace_dac_on = -1;

static int32_t trace_asks(const char *what)
{
    const char *spec = getenv("TIM_TRACE");
    return spec && strstr(spec, what) != NULL;
}

static void io_trace_crtc(uint8_t index, uint8_t value)
{
    if (trace_crtc_on < 0)
        trace_crtc_on = trace_asks("crtc");
    if (trace_crtc_on && (index == 0x15 || index == 0x18 || index == 0x07
                          || index == 0x09))
        fprintf(stderr, "[crtc] %02x <- %02x\n", index, value);
}

static void io_trace_dac(uint8_t index, int32_t phase, uint8_t value)
{
    static int32_t writes;

    if (trace_dac_on < 0)
        trace_dac_on = trace_asks("dac");
    if (trace_dac_on && (writes++ % 256) == 0)
        fprintf(stderr, "[dac] write %d: index %02x phase %d value %02x\n",
                writes - 1, index, phase, value);
}

static uint8_t  dac[256][3];
static uint8_t  attr_pal[16];
/*
 * The attribute controller's index/data flip-flop. One port, 0x3C0, takes an
 * index and then a value, and a read of Input Status 1 puts it back to
 * expecting an index. The driver's own start-up relies on that reset: it reads
 * 0x3DA before every pair it writes.
 */
static uint8_t  attr_index;
static uint8_t  attr_expect_data;

static uint8_t  dac_index;
static int32_t  dac_phase;
static uint8_t  dac_latch[3];
static int32_t  dac_write_mode = 1;

static io_event trace[IO_TRACE_MAX];
static int32_t  trace_n = -1;      /* -1 = not tracing */

void io_trace_begin(void)      { trace_n = 0; }
int32_t io_trace_count(void)   { return trace_n < 0 ? 0 : trace_n; }
int32_t io_trace_full(void)    { return trace_n >= IO_TRACE_MAX; }
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

/*
 * OURS: load the recovered image the way DOS's loader would.
 *
 * The port has no EXE loader of its own and needs one to run at all: the game's
 * code, its initialised data and the whole of DGROUP live in `out/TIM.img`, and
 * the segment immediates in it are **paragraph counts from the load address**,
 * not values. The relocation table that says which words those are is measured
 * rather than decoded - `tools/unlzexe.py` runs the LZEXE stub twice at
 * different load segments and diffs - and it is carried in the recovered
 * executable's own header, which is where this reads it from.
 *
 * The load segment is 0x0110, a PSP at 0x0100 and the usual 0x10 paragraphs of
 * it, which is where the reference emulator puts the program. It matters that
 * the two agree: a captured DGROUP address means nothing if the port put DGROUP
 * somewhere else.
 *
 * Everything after the copy is what Borland's startup does before it reaches
 * main, and the port does it here because the startup itself is not the game:
 * the program block is cut down to DGROUP + 64 KB, the tail becomes the arena,
 * and SS:SP points at the top of DGROUP.
 */
#define LOAD_SEG   0x0110u
#define PSP_SEG    0x0100u
#define MEM_TOP    0x9FFFu
#define IMG_DGROUP 0x2D3C0u

static int32_t read_file(const char *path, uint8_t **out, int32_t *len)
{
    FILE *f = fopen(path, "rb");
    long n;

    *out = NULL;
    *len = 0;
    if (!f)
        return 0;

    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);

    *out = (uint8_t *)malloc((size_t)n);
    if (!*out || fread(*out, 1, (size_t)n, f) != (size_t)n) {
        free(*out);
        *out = NULL;
        fclose(f);
        return 0;
    }
    fclose(f);
    *len = (int32_t)n;
    return 1;
}

int32_t io_load_program(const char *img_path, const char *exe_path)
{
    uint8_t *img = NULL, *exe = NULL;
    int32_t img_len = 0, exe_len = 0;
    uint32_t base = (uint32_t)LOAD_SEG << 4;
    uint16_t nrel, tbl;
    int32_t i;

    if (!read_file(img_path, &img, &img_len))
        return 0;
    if (!read_file(exe_path, &exe, &exe_len)) {
        free(img);
        return 0;
    }
    if (base + (uint32_t)img_len > GUEST_MEM_BYTES || exe_len < 0x20) {
        free(img);
        free(exe);
        return 0;
    }

    memcpy(guest_mem + base, img, (size_t)img_len);

    nrel = (uint16_t)(exe[6] | (exe[7] << 8));
    tbl  = (uint16_t)(exe[0x18] | (exe[0x19] << 8));

    for (i = 0; i < (int32_t)nrel; i++) {
        int32_t at = tbl + 4 * i;
        uint16_t off, seg;
        uint32_t where;

        if (at + 4 > exe_len)
            break;
        off = (uint16_t)(exe[at] | (exe[at + 1] << 8));
        seg = (uint16_t)(exe[at + 2] | (exe[at + 3] << 8));
        where = base + ((uint32_t)seg << 4) + off;
        if (where + 1 >= GUEST_MEM_BYTES)
            continue;
        {
            uint16_t v = (uint16_t)(guest_mem[where]
                                    | (guest_mem[where + 1] << 8));

            v = (uint16_t)(v + LOAD_SEG);
            guest_mem[where] = (uint8_t)v;
            guest_mem[where + 1] = (uint8_t)(v >> 8);
        }
    }

    free(img);
    free(exe);

    dgroup_base = base + IMG_DGROUP;

    /* The startup's own stack, at the top of a 64 KB DGROUP. */
    guest_sp = 0xFFFE;

    /*
     * The program keeps everything up to the end of DGROUP; the rest becomes
     * the arena. `dgroup_base` is a linear address, so the paragraph above it
     * plus 0x1000 is where the program's block ends.
     */
    io_dos_arena_reset((uint16_t)((dgroup_base >> 4) + 0x1000), MEM_TOP);

    /* The BIOS data area the game reads: keyboard flags and the video mode. */
    guest_mem[0x400 + 0x17] = 0;
    guest_mem[0x400 + 0x49] = 0x03;

    return 1;
}

/*
 * OURS: a DOS memory arena, for when the port runs on its own.
 *
 * The verifier primes allocations with what DOS actually answered during the
 * original's own call, and that stays the better answer when it is available -
 * it keeps a routine's arithmetic comparable without the port having to agree
 * with DOS about *where* a block goes. But a port that only ever runs under the
 * verifier is not a port, and running the game needs somewhere for its two
 * hundred allocations to come from.
 *
 * So: first fit over a list of blocks, split on allocation, coalesced on free -
 * which is what the shared emulator does, and what DOS did. A stub that handed
 * out the same segment every time would give two live allocations the same
 * memory, and the game frees and reallocates often enough to notice.
 *
 * The arena starts **above the program's own block**, not just above the image.
 * The recovered header asks for every paragraph it can get, so DOS gives the
 * program all of conventional memory and nothing is free until Borland's
 * startup hands the tail back. Modelling it the other way puts DOS's blocks
 * inside DGROUP, where the startup has already put the stack - see CLAUDE.md,
 * which records how that failure looked from the outside.
 */
struct arena_block { uint16_t seg, paras; uint8_t used; };

/*
 * The block table **grows**. It was a fixed 256 entries, and the game holds
 * more live blocks than that - one per part's bitmaps alone is fifty-odd -
 * so it filled, and a full table meant an allocation could not be split off
 * the front of a free block. The code then handed out the block *whole* and
 * shrank its record to the size asked for, which loses the tail: after that
 * every large free block was swallowed by the next small request. The symptom
 * was the last dozen part bitmaps failing to load, and then a part drawn from
 * a null bitmap list painting the screen white.
 *
 * A table that cannot grow must at least refuse; this one grows, and refuses
 * only if it cannot.
 */
static struct arena_block *arena;
static int32_t arena_n, arena_cap;
static uint16_t arena_top;

static int32_t arena_room(void)
{
    struct arena_block *bigger;
    int32_t want;

    if (arena_n < arena_cap)
        return 1;

    want = arena_cap ? arena_cap * 2 : 256;
    bigger = realloc(arena, (size_t)want * sizeof *arena);
    if (bigger == NULL)
        return 0;

    arena = bigger;
    arena_cap = want;
    return 1;
}

void io_dos_arena_reset(uint16_t first_free, uint16_t mem_top)
{
    arena_n = 0;
    arena_top = mem_top;
    if (mem_top > first_free && arena_room()) {
        arena[0].seg = first_free;
        arena[0].paras = (uint16_t)(mem_top - first_free);
        arena[0].used = 0;
        arena_n = 1;
    }
}

/* Merge neighbouring free blocks, so a freed block can be used again. */
static void arena_coalesce(void)
{
    int32_t i, j;

    for (i = 0; i < arena_n; i++)
        for (j = i + 1; j < arena_n; j++)
            if (arena[j].seg < arena[i].seg) {
                struct arena_block t = arena[i];

                arena[i] = arena[j];
                arena[j] = t;
            }

    for (i = 0; i + 1 < arena_n; ) {
        if (!arena[i].used && !arena[i + 1].used
            && (uint16_t)(arena[i].seg + arena[i].paras) == arena[i + 1].seg) {
            arena[i].paras = (uint16_t)(arena[i].paras + arena[i + 1].paras);
            for (j = i + 1; j + 1 < arena_n; j++)
                arena[j] = arena[j + 1];
            arena_n--;
        } else {
            i++;
        }
    }
}

static uint16_t arena_largest(void)
{
    uint16_t best = 0;
    int32_t i;

    for (i = 0; i < arena_n; i++)
        if (!arena[i].used && arena[i].paras > best)
            best = arena[i].paras;

    return best;
}

uint16_t io_dos_alloc(uint16_t paragraphs, uint16_t *largest, int32_t *failed)
{
    int32_t i;

    /* What the original's own run answered, when the verifier has it. */
    if (alloc_at < alloc_n) {
        *largest = alloc_largest[alloc_at];
        *failed = alloc_failed[alloc_at];
        return alloc_seg[alloc_at++];
    }

    if (arena_n == 0) {
        not_transcribed("a DOS allocation with no arena and nothing primed");
        *failed = 1;
        *largest = 0;
        return 0;
    }

    /* DOS refuses 0 and 0xffff paragraphs; 0xffff is the "how much" probe. */
    if (paragraphs != 0 && paragraphs != 0xFFFF) {
        for (i = 0; i < arena_n; i++) {
            if (!arena[i].used && arena[i].paras >= paragraphs) {
                uint16_t seg = arena[i].seg;

                if (arena[i].paras > paragraphs) {
                    if (!arena_room())
                        break;          /* refuse rather than lose the tail */
                    arena[arena_n].seg = (uint16_t)(seg + paragraphs);
                    arena[arena_n].paras =
                        (uint16_t)(arena[i].paras - paragraphs);
                    arena[arena_n].used = 0;
                    arena_n++;
                }
                arena[i].paras = paragraphs;
                arena[i].used = 1;
                arena_coalesce();
                *failed = 0;
                *largest = arena_largest();
                return seg;
            }
        }
    }

    *failed = 1;
    *largest = arena_largest();
    return 0;
}

/*
 * Release a DOS block. The port has no DOS arena - allocations are primed by
 * the verifier rather than served - so there is nothing to give back and this
 * does nothing. It exists so the transcribed routine that calls it reads the
 * way the original does instead of having the call quietly dropped.
 */
/*
 * Shrink or grow a DOS block in place, INT 21h AH=4Ah. Answers 0 on success and
 * the largest size available on failure, which is what DOS puts in BX.
 *
 * Only shrinking happens here, and only from a routine that has just measured
 * how much of a block it actually filled - so the tail goes back to the arena
 * and the next allocation can have it.
 */
uint16_t io_dos_resize(uint16_t seg, uint16_t paragraphs)
{
    int32_t i;

    for (i = 0; i < arena_n; i++) {
        if (!arena[i].used || arena[i].seg != seg)
            continue;

        if (paragraphs <= arena[i].paras) {
            /*
             * A shrink whose tail cannot be recorded leaves the block as it is
             * rather than losing it. DOS answers 0 either way - the caller only
             * asked for the block to be no larger than this, and it is not.
             */
            if (paragraphs < arena[i].paras) {
                if (!arena_room())
                    return 0;
                arena[arena_n].seg = (uint16_t)(seg + paragraphs);
                arena[arena_n].paras =
                    (uint16_t)(arena[i].paras - paragraphs);
                arena[arena_n].used = 0;
                arena_n++;
            }
            arena[i].paras = paragraphs;
            arena_coalesce();
            return 0;
        }
        return arena[i].paras;         /* cannot grow in place */
    }

    return 0;                          /* not ours: nothing to do */
}

void io_dos_free(uint16_t seg)
{
    int32_t i;

    for (i = 0; i < arena_n; i++)
        if (arena[i].used && arena[i].seg == seg) {
            arena[i].used = 0;
            arena_coalesce();
            return;
        }
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
    return heap_malloc(bytes);
}

/*
 * Borland's own `free`, the counterpart of `io_malloc` above and refused for
 * the same reason: the port has no heap to give a block back to.
 */
void io_free(uint16_t off)
{
    heap_free(off);
}



/*
 * DOS file services, **read-only**, served from the game directory.
 *
 * The port opens the game's own files rather than being handed their contents,
 * for the same reason the emulator does: a routine that reads a file can then
 * be checked byte for byte instead of against a recording. Nothing here writes,
 * creates or deletes - the guarantee that makes it safe to let the game run
 * against the real directory.
 *
 * Handles are numbered from 5, which is what DOS hands out once stdin, stdout,
 * stderr, stdaux and stdprn have taken 0 to 4. That is not cosmetic: the guest
 * stores the number it is given and the comparison sees it, so a port that
 * counted from zero would differ on the first open.
 *
 * DOS filenames are upper case and the host's may not be, so a name that does
 * not open as given is retried lower case. Anything else - a path, a wildcard -
 * is left alone and simply fails.
 */
#define DOS_HANDLES 24
#define DOS_FIRST_HANDLE 5

static FILE    *dos_file[DOS_HANDLES];
static char     game_dir[512] = "incredible-machine";

void io_set_game_dir(const char *path)
{
    size_t n = strlen(path);

    if (n >= sizeof game_dir)
        n = sizeof game_dir - 1;
    memcpy(game_dir, path, n);
    game_dir[n] = 0;
}

static FILE *dos_try(const char *name, int32_t lower)
{
    char path[1024];
    size_t i, n = strlen(name);

    if (n > 255)
        return NULL;
    snprintf(path, sizeof path, "%s/%s", game_dir, name);
    if (lower) {
        for (i = strlen(game_dir) + 1; path[i]; i++)
            if (path[i] >= 'A' && path[i] <= 'Z')
                path[i] = (char)(path[i] - 'A' + 'a');
    }
    return fopen(path, "rb");
}

/*
 * Put a named file on a given handle at a given offset.
 *
 * tools/verify.py calls this before comparing a routine that reads files. The
 * harness seeds guest memory, but a handle and a file position are not in guest
 * memory, so without this the port arrives with nothing open and every such
 * routine is unverifiable however faithfully it is transcribed. The emulator
 * records what DOS actually opened - see `TimMachine._dos` - and this reopens
 * the same file at the same offset.
 */
void io_prime_file(int16_t handle, const char *name, int32_t pos)
{
    int16_t i = (int16_t)(handle - DOS_FIRST_HANDLE);
    FILE *f;

    if (i < 0 || i >= DOS_HANDLES)
        return;

    f = dos_try(name, 0);
    if (f == NULL)
        f = dos_try(name, 1);
    if (f == NULL)
        return;

    if (dos_file[i] != NULL)
        fclose(dos_file[i]);
    dos_file[i] = f;
    fseek(f, (long)pos, SEEK_SET);
}

/*
 * The BIOS display-combination code, as INT 10h AH=1Ah answers it: the active
 * display in BL, an inactive second one in BH, and 0x1a back in AL to say the
 * call is supported at all.
 *
 * The port's own, and measured against the emulator, which answers BL=8 - VGA
 * with a colour monitor - and no second display. This is the call that decides
 * which driver the game loads, so answering it differently would load a
 * different `VM.OVL` and every frame after would be a different game.
 */
uint16_t io_bios_display_combination(void)
{
    return 0x0008;
}

/*
 * OURS: call one of a screen region's handlers.
 *
 * The original reaches them with `lcall [si+0x12]`, a far pointer sitting in
 * the region record - relocated into place by `build_screen_regions`. There is
 * no way to call through a guest far pointer here, so the port matches the
 * value against the handlers it knows and aborts on one it does not. That is
 * the same standing as a stub: the alternative is doing nothing, and a handler
 * silently not running is a click that looks like it landed nowhere.
 */
/*
 * OURS: the guest's clock.
 *
 * The original's pacing comes from an 8253 interrupt on vector 8, and the code
 * that waits for it *spins*: `wait_and_latch_frame` sits on a DGROUP flag that
 * only the handler clears, and touches no hardware while it does. There is
 * nothing there for a single-threaded port to hook - no port read, no call out -
 * and that is not an accident of this routine, it is what waiting for an
 * interrupt looks like.
 *
 * So the port runs the handler on **a thread**, which is what it is: something
 * that happens to the guest rather than something the guest does. The rate
 * comes from what the guest programmed into the 8253, so a game that asks for a
 * different one gets it.
 *
 * **The locking is not finished, and this is where it will go.** On the real
 * machine the handler could not interleave with the guest's own instructions,
 * and where that mattered the game said so - `cli` around the timer's own
 * bookkeeping at 0x20654 and 0x206c1, and around the sound module's at
 * 0x26a57. Those three take this mutex, which is what `cli` means here: the
 * tick cannot land inside one of them and see half an update.
 *
 * The regions are exactly the original's, no wider: at 0x20654 it is the single
 * `or [0x44f7], cx` that sets a slot's bit in the mask the handler reads - one
 * instruction there, a read-modify-write here, and racy either way without it.
 */
static void (*timer_handler)(void);
static uint16_t timer_divisor;
static int32_t  timer_lo_next = 1;
static pthread_t timer_thread;
/*
 * **Recursive**, because the guest's own masking is. `retire_and_tick` at
 * 0x26a57 is `pushf; cli; ...; popf` and not `cli; ...; sti` - it puts the flag
 * back rather than turning interrupts on - so a caller that already had them
 * off keeps them off, and the region nests. A plain mutex would deadlock on
 * the second one instead of nesting.
 *
 * There is a static initialiser for a recursive mutex but it is a GNU
 * extension and `-std=c11` hides it, so the attribute is set at first use
 * through `pthread_once` - which costs one atomic read per lock and nothing
 * else.
 */
static pthread_mutex_t timer_lock;
static pthread_once_t  timer_lock_once = PTHREAD_ONCE_INIT;

static void make_timer_lock(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&timer_lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

static pthread_mutex_t *the_timer_lock(void)
{
    pthread_once(&timer_lock_once, make_timer_lock);
    return &timer_lock;
}
static volatile int32_t timer_running;

void io_lock(void)
{
    pthread_mutex_lock(the_timer_lock());
}

void io_unlock(void)
{
    pthread_mutex_unlock(the_timer_lock());
}

static void *timer_loop(void *arg)
{
    (void)arg;

    while (timer_running) {
        struct timespec ts;
        double hz = 1193182.0 / (double)(timer_divisor ? timer_divisor : 0x10000);
        double period = 1.0 / hz;

        ts.tv_sec = (time_t)period;
        ts.tv_nsec = (long)((period - (double)ts.tv_sec) * 1e9);
        nanosleep(&ts, NULL);

        if (!timer_handler)
            continue;

        pthread_mutex_lock(the_timer_lock());
        timer_handler();
        pthread_mutex_unlock(the_timer_lock());
    }
    return NULL;
}

void io_set_timer(void (*fn)(void))
{
    timer_handler = fn;
    if (!timer_running) {
        timer_running = 1;
        pthread_create(&timer_thread, NULL, timer_loop, NULL);
    }
}

void io_stop_timer(void)
{
    if (timer_running) {
        timer_running = 0;
        pthread_join(timer_thread, NULL);
    }
}

void io_service_timer(void)
{
    /*
     * Nothing to do: the thread above is the clock. This is kept because the
     * retrace poll calls it, and a guest waiting on the retrace is still a
     * guest that should be allowed to run - on a single core the thread needs
     * the chance.
     */
    sched_yield();
}

/*
 * OURS: call a slot's timer handler. The same problem as a region's, and the
 * same answer - the original reaches it through a far pointer in DGROUP and
 * the port dispatches on the value.
 */
/*
 * OURS: not a transcription. The drive hook, which is the one that takes seven
 * arguments. Forty-eight of the fifty-eight kinds point it at the do-nothing
 * `retf` in segment 0000 that answers 0; the other ten are in segment 172c.
 */
uint16_t call_part_drive(uint16_t off, uint16_t seg,
                         uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4,
                         uint16_t p5, uint16_t p6, uint16_t p7)
{
    if (seg == (uint16_t)((dgroup_base - 0x2D3C0 + 0x172c0) >> 4))
        return part_drive_172c(off, p1, p2, p3, p4, p5, p6, p7);

    if (seg == (uint16_t)((dgroup_base - 0x2D3C0) >> 4) && off == 0x02b5)
        return part_hook_no(p1);

    {
        static char msg[64];

        snprintf(msg, sizeof msg, "a part's drive at %04x:%04x", seg, off);
        not_transcribed(msg);
    }
    return 0;
}

/*
 * OURS: not a transcription. One door for the two per-kind hooks the physics
 * dispatches through - the step at +0x26 of a kind's record and the hit test at
 * +0x22. Both live in segment 172c or are the do-nothing `retf` in segment
 * 0000, and both take the part and answer a word, so one helper serves.
 */
uint16_t call_part_hook(uint16_t off, uint16_t seg, uint16_t part,
                        const char *what)
{
    if (seg == (uint16_t)((dgroup_base - 0x2D3C0 + 0x172c0) >> 4))
        return part_hook_172c(off, part);

    if (seg == (uint16_t)((dgroup_base - 0x2D3C0) >> 4)) {
        switch (off) {
        case 0x0297: return part_hook_yes(part);
        case 0x02a1: part_hook_none_2a1(part); return 0;
        case 0x02a6: part_hook_none_2a6(part); return 0;
        case 0x02ab: part_hook_none_2ab(part); return 0;
        case 0x02b0: part_hook_none_2b0(part); return 0;
        case 0x02b5: return part_hook_no(part);
        default: break;
        }
    }

    {
        static char msg[80];

        snprintf(msg, sizeof msg, "a part's %s at %04x:%04x", what, seg, off);
        not_transcribed(msg);
    }
    return 0;
}

/*
 * OURS: not a transcription. The original runs a part's setup through the far
 * pointer at +0x2a of its kind's record; C cannot call one, so the dispatch is
 * by value, as everywhere else the port meets a guest function pointer.
 */
void call_part_setup(uint16_t off, uint16_t seg, uint16_t part)
{
    if (seg == (uint16_t)((dgroup_base - 0x2D3C0 + 0x172c0) >> 4)) {
        part_setup(off, part);
        return;
    }

    if (seg == (uint16_t)((dgroup_base - 0x2D3C0) >> 4)) {
        switch (off) {
        case 0x02a1: part_hook_none_2a1(part); return;
        case 0x02a6: part_hook_none_2a6(part); return;
        case 0x02ab: part_hook_none_2ab(part); return;
        case 0x02b0: part_hook_none_2b0(part); return;
        default: break;
        }
    }

    {
        static char what[64];

        snprintf(what, sizeof what, "a part's setup at %04x:%04x", seg, off);
        not_transcribed(what);
    }
}

/*
 * OURS: call a part's init function. The third of these, and the same reason -
 * the original reaches it through a far pointer in a table, relocated into
 * place by the loader, and the port has no way to call one.
 */
uint16_t call_part_init(uint16_t off, uint16_t seg, uint16_t part)
{
    /*
     * Every one of these is in segment 0dff, so the offset alone identifies it
     * and `part_init` finds it by its image address.
     */
    if (seg == (uint16_t)((dgroup_base - 0x2D3C0 + 0xdff0) >> 4))
        return part_init((uint32_t)0xdff0 + off, part);

    {
        static char what[64];

        snprintf(what, sizeof what, "a part's init at %04x:%04x", seg, off);
        not_transcribed(what);
    }
    return 0;
}

void call_timer_handler(uint16_t off, uint16_t seg)
{

    switch (off) {
    case 0xa7ae:
        timer_callback();
        return;
    case 0x193e:
        /* The sound module's, in its own code segment. */
        sound_service();
        return;
    default:
        break;
    }

    {
        static char what[64];

        snprintf(what, sizeof what,
                 "a timer slot's handler at %04x:%04x", seg, off);
        not_transcribed(what);
    }
}

/*
 * Call a region's `enter` or `click` handler.
 *
 * NOT a transcription of anything: the original does `call far [si+0x12]`, and
 * the port cannot call through a guest far pointer. The offsets are the ones
 * `build_screen_regions` files into the table, and the segment is the module's
 * - all of these are in seg0dff, which is why only the offset is switched on.
 *
 * An offset with no case **aborts** rather than being ignored, for the reason
 * every stub here aborts: a region handler that silently does nothing is a
 * cursor that does not change and a click that goes nowhere, which looks like a
 * drawing fault.
 */
void call_region_handler(uint16_t off, uint16_t seg, uint16_t region)
{
    (void)seg;

    switch (off) {
    case 0x34eb:
        region_cursor_restart(region);
        return;
    case 0x3508:
        region_cursor_load(region);
        return;
    case 0x3525:
        region_cursor_save(region);
        return;
    case 0x3542:
        region_cursor_gravity(region);
        return;
    case 0x355f:
        region_cursor_air(region);
        return;
    default:
        break;
    }

    {
        static char what[64];

        snprintf(what, sizeof what,
                 "a screen region's handler at %04x:%04x", seg, off);
        not_transcribed(what);
    }
}

/*
 * OURS: the mouse, INT 33h.
 *
 * A mouse is one of the few things a DOS game asked the hardware about that a
 * modern host simply *has*, so the port answers the reset call the way a driver
 * would rather than pretending there is none. The reference emulator answers
 * the same, which is what makes the routines above it comparable at all - with
 * no mouse the game's start-up takes a different branch and writes different
 * bytes.
 *
 * Everything else here is a setting the driver keeps on the game's behalf -
 * where the cursor is, how far it may travel, how fast it moves, whether it is
 * drawn, which handler to call. None of it is in guest memory, so none of it is
 * anything the two artefacts could disagree about; the calls exist so that the
 * transcriptions have somewhere real to send them, and so that wiring SDL3
 * input in later is one file's work.
 */
uint16_t io_mouse_reset(void)
{
    return 0xFFFF;                  /* a driver is installed */
}

void io_mouse_show(void)
{
}

void io_mouse_hide(void)
{
}

/*
 * OURS: where the driver thinks the pointer is, and how far it may go.
 *
 * The game asks in quarter-pixels - `mouse_move_to` shifts by two on the way
 * in - and the driver holds the position; guest memory never sees it except
 * through the event callback. So these are the driver's own variables and
 * there is nothing to compare them against.
 */
static int32_t mouse_x, mouse_y;
static int32_t mouse_x_lo, mouse_x_hi = 0x7fffffff;
static int32_t mouse_y_lo, mouse_y_hi = 0x7fffffff;
static uint16_t mouse_mask;
static int32_t  mouse_installed;

void io_mouse_move_to(uint16_t x, uint16_t y)
{
    mouse_x = (int32_t)x;
    mouse_y = (int32_t)y;
}

void io_mouse_set_speed(uint16_t x_mickeys, uint16_t y_mickeys)
{
    (void)x_mickeys;
    (void)y_mickeys;
}

void io_mouse_set_x_range(uint16_t lo, uint16_t hi)
{
    mouse_x_lo = (int16_t)lo;
    mouse_x_hi = (int16_t)hi;
}

void io_mouse_set_y_range(uint16_t lo, uint16_t hi)
{
    mouse_y_lo = (int16_t)lo;
    mouse_y_hi = (int16_t)hi;
}

/*
 * OURS: INT 33h AX=0x0c, "call this on these events".
 *
 * The offset and segment name `mouse_event` at image 0x21fcf and nothing else -
 * `mouse_init` is the one caller and that is what it passes - so the port
 * remembers only *that* it was installed, and calls the routine directly. A
 * dispatch by offset, the way `call_timer_handler` does it, would be inventing
 * a choice where the original has one destination.
 *
 * The mask is kept because the driver is supposed to honour it, and because a
 * port that delivered events the game never asked for would be inventing
 * input.
 */
void io_mouse_set_handler(uint16_t mask, uint16_t off, uint16_t seg)
{
    (void)off;
    (void)seg;
    mouse_mask = mask;
    mouse_installed = 1;
}

/*
 * OURS: the host's pointer, turned into the event the driver would raise.
 *
 * The window calls this with a position in screen pixels and the buttons it
 * sees; the driver's units are quarter-pixels, which is why everything is
 * shifted by two. The position is clamped to the range the game set, because
 * a real driver clamps and the game relies on it - `mouse_set_ranges` is how
 * it fences the pointer into the play area.
 *
 * The mask decides whether an event is raised at all: bit 0 is movement, bits
 * 1 and 2 the left button down and up, bits 3 and 4 the right. Delivering
 * events the game did not ask for would be putting input into a program that
 * never requested it.
 *
 * Nothing here is a transcription. The original had a mouse driver in memory
 * doing it, and this is the only part of the mouse that is genuinely ours.
 */
void io_mouse_input(int32_t x, int32_t y, uint16_t buttons)
{
    int32_t qx = x << 2, qy = y << 2;
    uint16_t events = 0;
    static uint16_t last_buttons;

    if (!mouse_installed)
        return;

    if (qx < mouse_x_lo) qx = mouse_x_lo;
    if (qx > mouse_x_hi) qx = mouse_x_hi;
    if (qy < mouse_y_lo) qy = mouse_y_lo;
    if (qy > mouse_y_hi) qy = mouse_y_hi;

    if (qx != mouse_x || qy != mouse_y)
        events |= 0x01;
    if ((buttons & 1) && !(last_buttons & 1)) events |= 0x02;
    if (!(buttons & 1) && (last_buttons & 1)) events |= 0x04;
    if ((buttons & 2) && !(last_buttons & 2)) events |= 0x08;
    if (!(buttons & 2) && (last_buttons & 2)) events |= 0x10;

    mouse_x = qx;
    mouse_y = qy;
    last_buttons = buttons;

    if ((events & mouse_mask) == 0)
        return;

    io_lock();
    mouse_event(buttons, (uint16_t)qx, (uint16_t)qy);
    io_unlock();
}

/*
 * The current drive, as INT 21h AH=19h answers it: 0 for A, 1 for B, and so on.
 *
 * The port's own, measured against the emulator, which answers 2 - drive C.
 */
uint16_t io_dos_curdrive(void)
{
    return 2;
}

/*
 * The current directory on a drive, as INT 21h AH=47h fills it in: the path
 * without a leading backslash, NUL-terminated.
 *
 * The port's own, and again measured: the emulator answers an empty path, so
 * the game's idea of where it is running is the root. Writing this machine's
 * real working directory would make the game's own strings differ from the
 * reference for no useful reason.
 */
void io_dos_getcwd(uint8_t *buf)
{
    buf[0] = 0;
}

/*
 * Ask whether a file exists, and answer the attribute byte DOS would.
 *
 * The port's own, because the original asks DOS - INT 21h AH=43h with AL=0 -
 * and this machine has no DOS. **Measured against the emulator**, which is the
 * reference here: a file that is there answers 0x20, the archive bit, and one
 * that is not sets carry, which the caller reads as -1.
 */
/*
 * The date DOS would answer to INT 21h AH=2Ah: the year in CX, the month and
 * day in DX, the weekday in AL.
 *
 * The port's own, and **measured against the emulator** rather than taken from
 * this machine's clock - a real date would make every run differ from every
 * other, which is exactly what a reproducible comparison cannot have. The
 * emulator answers 2000-11-02, a Wednesday.
 */
void io_dos_getdate(uint16_t *year, uint16_t *monthday, uint16_t *weekday)
{
    *year = 0x07d0;
    *monthday = 0x0b02;
    *weekday = 0x0004;
}

/*
 * INT 21h AH=4Eh and AH=4Fh - find first and find next.
 *
 * The port's own, because the original asks DOS and this machine has no DOS,
 * and **written to answer what the emulator answers**: the game directory's
 * entries, upper-cased, sorted by name, with `.` and `..` prepended when the
 * caller asked for directories and the game is not at the root - which it never
 * is here, because `io_dos_getcwd` says the root.
 *
 * The match is **DOS's, not fnmatch's**. DOS splits both the name and the
 * pattern into an 8-character stem and a 3-character extension and matches the
 * two fields separately, so `*` is a wild stem with an *empty* extension and
 * matches README but not README.TXT. Getting that wrong is not a near miss: it
 * puts every file into a pane that asked only for directories.
 *
 * The attribute is a *permission*, not a filter. Ordinary files come back
 * always; directories only when bit 4 is asked for.
 */
#define FIND_MAX 512

static char  find_names[FIND_MAX][13];
static int32_t find_is_dir[FIND_MAX];
static uint32_t find_size[FIND_MAX];
static int32_t find_count;
static int32_t find_next_i;

/*
 * One field of a DOS wildcard, `width` characters wide. A `*` fills the rest of
 * the field with `?`, and a `?` matches one character *or the end* of it.
 */
static int32_t dos_match_field(const char *val, const char *pat, int32_t width)
{
    char expanded[9];
    int32_t n = 0, i;

    for (i = 0; pat[i] != 0 && n < width; i++) {
        if (pat[i] == '*') {
            while (n < width)
                expanded[n++] = '?';
            break;
        }
        expanded[n++] = pat[i];
    }
    expanded[n] = 0;

    if (n < (int32_t)strlen(val))
        return 0;

    for (i = 0; i < n; i++) {
        char vc = (i < (int32_t)strlen(val)) ? val[i] : 0;

        if (expanded[i] == '?')
            continue;
        if (vc != expanded[i])
            return 0;
    }

    return 1;
}

static void dos_split(const char *v, char *stem, char *ext)
{
    const char *dot;
    size_t n;

    /*
     * `.` and `..` are entries whose *name* is the dots and whose extension is
     * blank. Partitioning them on the dot would make the extension a dot and
     * stop `*` from matching them - which loses the entry a browser climbs out
     * by.
     */
    if (strcmp(v, ".") == 0 || strcmp(v, "..") == 0) {
        strcpy(stem, v);
        ext[0] = 0;
        return;
    }

    dot = strchr(v, '.');
    n = (dot != NULL) ? (size_t)(dot - v) : strlen(v);
    if (n > 8)
        n = 8;
    memcpy(stem, v, n);
    stem[n] = 0;

    if (dot == NULL) {
        ext[0] = 0;
        return;
    }

    n = strlen(dot + 1);
    if (n > 3)
        n = 3;
    memcpy(ext, dot + 1, n);
    ext[n] = 0;
}

static int32_t dos_match(const char *name, const char *pattern)
{
    char ns[9], ne[4], ps[9], pe[4];

    dos_split(name, ns, ne);
    dos_split(pattern, ps, pe);

    return dos_match_field(ns, ps, 8) && dos_match_field(ne, pe, 3);
}

static int32_t find_cmp(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

int16_t io_dos_findfirst(const char *pattern, uint16_t attr,
                         uint8_t *name, uint8_t *attr_out, uint32_t *size_out)
{
    const char *leaf = pattern;
    const char *p;
    DIR *d;
    struct dirent *e;

    for (p = pattern; *p; p++)
        if (*p == '\\' || *p == '/')
            leaf = p + 1;

    find_count  = 0;
    find_next_i = 0;

    d = opendir(game_dir);
    if (d != NULL) {
        while ((e = readdir(d)) != NULL && find_count < FIND_MAX) {
            char up[13];
            struct stat st;
            char path[1024];
            size_t i, n = strlen(e->d_name);

            if (n > 12)
                continue;
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                continue;

            for (i = 0; i < n; i++)
                up[i] = (e->d_name[i] >= 'a' && e->d_name[i] <= 'z')
                        ? (char)(e->d_name[i] - 'a' + 'A') : e->d_name[i];
            up[n] = 0;

            snprintf(path, sizeof path, "%s/%s", game_dir, e->d_name);
            if (stat(path, &st) != 0)
                continue;

            if (S_ISDIR(st.st_mode) && !(attr & 0x10))
                continue;
            if (!dos_match(up, leaf))
                continue;

            strcpy(find_names[find_count], up);
            find_is_dir[find_count] = S_ISDIR(st.st_mode) ? 1 : 0;
            find_size[find_count]   = S_ISDIR(st.st_mode)
                                      ? 0 : (uint32_t)st.st_size;
            find_count++;
        }
        closedir(d);
    }

    /*
     * Sorted by name, because `readdir` has no order and the reference does
     * sort - an unsorted listing would differ from the emulator's on nothing
     * but the filesystem's mood.
     */
    if (find_count > 1) {
        int32_t i, j;

        for (i = 0; i < find_count; i++)
            for (j = i + 1; j < find_count; j++)
                if (find_cmp(find_names[i], find_names[j]) > 0) {
                    char     tn[13];
                    int32_t  td;
                    uint32_t ts;

                    strcpy(tn, find_names[i]);
                    strcpy(find_names[i], find_names[j]);
                    strcpy(find_names[j], tn);
                    td = find_is_dir[i];
                    find_is_dir[i] = find_is_dir[j];
                    find_is_dir[j] = td;
                    ts = find_size[i];
                    find_size[i] = find_size[j];
                    find_size[j] = ts;
                }
    }

    return io_dos_findnext(name, attr_out, size_out);
}

int16_t io_dos_findnext(uint8_t *name, uint8_t *attr_out, uint32_t *size_out)
{
    if (find_next_i >= find_count)
        return 18;                      /* no more files */

    strcpy((char *)name, find_names[find_next_i]);
    *attr_out = (uint8_t)(find_is_dir[find_next_i] ? 0x10 : 0x20);
    *size_out = find_size[find_next_i];
    find_next_i++;

    return 0;
}

int16_t io_dos_getattr(const char *name)
{
    FILE *f = dos_try(name, 0);

    if (f == NULL)
        f = dos_try(name, 1);
    if (f == NULL)
        return -1;

    fclose(f);
    return 0x20;
}

/*
 * The device-information word for a handle, as INT 21h AH=44h AL=0 answers it.
 *
 * The port's own, and measured the same way: a disk file answers 0, the
 * console handles 0 to 2 answer 0x80. Bit 7 is the one every caller looks at -
 * it is what `isatty` is.
 */
int16_t io_dos_devinfo(int16_t handle)
{
    if (handle >= 0 && handle < DOS_FIRST_HANDLE)
        return 0x80;

    return 0;
}

int16_t io_dos_open(const char *name)
{
    int16_t h;
    FILE *f = dos_try(name, 0);

    if (f == NULL)
        f = dos_try(name, 1);
    if (f == NULL)
        return -1;

    for (h = 0; h < DOS_HANDLES; h++) {
        if (dos_file[h] == NULL) {
            dos_file[h] = f;
            return (int16_t)(h + DOS_FIRST_HANDLE);
        }
    }
    fclose(f);
    return -1;
}

static FILE *dos_of(int16_t handle)
{
    int16_t i = (int16_t)(handle - DOS_FIRST_HANDLE);

    if (i < 0 || i >= DOS_HANDLES)
        return NULL;
    return dos_file[i];
}

int16_t io_dos_read(int16_t handle, uint8_t *buf, uint16_t count)
{
    FILE *f = dos_of(handle);

    if (f == NULL)
        return -1;
    return (int16_t)fread(buf, 1, count, f);
}

int32_t io_dos_lseek(int16_t handle, int32_t pos, int16_t whence)
{
    FILE *f = dos_of(handle);
    int w = whence == 1 ? SEEK_CUR : whence == 2 ? SEEK_END : SEEK_SET;

    if (f == NULL)
        return -1;
    if (fseek(f, (long)pos, w) != 0)
        return -1;
    return (int32_t)ftell(f);
}

void io_dos_close(int16_t handle)
{
    int16_t i = (int16_t)(handle - DOS_FIRST_HANDLE);

    if (i < 0 || i >= DOS_HANDLES || dos_file[i] == NULL)
        return;
    fclose(dos_file[i]);
    dos_file[i] = NULL;
}

void not_transcribed(const char *what)
{
    fprintf(stderr, "reached %s, which is not transcribed yet\n", what);

    /*
     * Show whatever had been drawn before giving up. A stub must abort - a
     * silent no-op in a drawing path is a missing frame that looks like a
     * blitter fault - but aborting with the window still open and the last
     * frame in it says far more about where the port got to than the line
     * above does. devtim registers nothing here and aborts straight away.
     */
    if (abort_hook)
        abort_hook();

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

/*
 * Put the VGA back to how a BIOS mode 0x12 set leaves it: planes cleared, the
 * CRTC loaded with the BIOS's own timing table, the DAC and attribute palette
 * back to the identity, and the map and bit masks wide open.
 *
 * The port's own. It is what `io_reset` has always done to the video state -
 * this only gives it a name, so that the driver's start-up can ask for a mode
 * set instead of the port pretending mode changes do not happen.
 */
void io_bios_set_mode(uint16_t mode)
{
    if (mode != 0x12) {
        not_transcribed("a BIOS video mode other than 0x12");
        return;
    }

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
    attr_index = 0;
    attr_expect_data = 0;
    seq[2] = 0x0F;
    gc[8]  = 0xFF;

    /*
     * The BIOS records the mode it just set at 0040:0049, and that byte is
     * ordinary memory the verifier compares - so a mode set that does not
     * write it differs from the original by exactly one byte.
     */
    guest_mem[0x449] = (uint8_t)mode;
}

void io_reset(void)
{
    int32_t h;

    for (h = 0; h < DOS_HANDLES; h++) {
        if (dos_file[h] != NULL) {
            fclose(dos_file[h]);
            dos_file[h] = NULL;
        }
    }
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
    attr_index = 0;
    attr_expect_data = 0;
    seq[2] = 0x0F;                    /* map mask: all planes enabled */
    gc[8]  = 0xFF;                    /* bit mask: every bit writable */
}

void io_out8(uint16_t port, uint8_t value)
{
    io_service_display();
    trace_add(port, 0, value, 0);
    switch (port) {
    case PORT_SEQ_INDEX:  seq_index  = value & 0x07; break;
    case PORT_SEQ_DATA:   seq[seq_index] = value;    break;
    case PORT_GC_INDEX:   gc_index   = value & 0x0F; break;
    case PORT_GC_DATA:    gc[gc_index] = value;      break;
    case PORT_CRTC_INDEX: crtc_index = value & 0x1F; break;
    case PORT_CRTC_DATA:
        crtc[crtc_index] = value;
        io_trace_crtc(crtc_index, value);
        /*
         * CRTC 0x0C is the high byte of the start address, and writing it is
         * how this game flips pages - it alternates 0x00 and 0x82 and never
         * touches the low byte. So a write here *is* the guest saying "this
         * frame is finished", which is the same cue tools/capture.py takes its
         * reference frames on. Anything else the guest sets on the CRTC is
         * just a register.
         */
        if (crtc_index == 0x0C) {
            /*
             * The same instant tools/capture.py counts its flips on, so a flip
             * number means the same thing on both sides. `dev_flip_dump` is
             * the port's own tooling and does nothing unless asked.
             */
            static int32_t flips;

            dev_flip_dump(flips++);

            /*
             * The flip is the guest's, and it counts wherever it happens; the
             * *presenting* is the window's and only the window's thread may
             * do it.
             */
            if (present_hook && on_display_thread())
                present_hook();
        }
        break;
    case 0x61:            port61 = value;            break;
    /*
     * The 8253's counter 0. The guest writes the divisor low byte then high,
     * and the port reads the rate out of that rather than being told it - so
     * the transcribed `timer_install` needs no line it would not otherwise
     * have, and a guest that asks for a different rate gets one.
     */
    case 0x40:
        if (timer_lo_next) {
            timer_divisor = (uint16_t)((timer_divisor & 0xFF00) | value);
            timer_lo_next = 0;
        } else {
            timer_divisor = (uint16_t)((timer_divisor & 0x00FF) | (value << 8));
            timer_lo_next = 1;
        }
        break;
    case 0x43:
        timer_lo_next = 1;               /* the mode byte restarts the pair */
        break;
    case PORT_ATTR:
        if (attr_expect_data) {
            if (attr_index < 16)
                attr_pal[attr_index] = (uint8_t)(value & 0x3F);
            attr_expect_data = 0;
        } else {
            attr_index = (uint8_t)(value & 0x1F);
            attr_expect_data = 1;
        }
        break;
    case PORT_DAC_WRITE:
        dac_index = value;
        dac_phase = 0;
        dac_write_mode = 1;
        break;
    case PORT_DAC_READ:
        dac_write_mode = 0;
        break;
    case PORT_DAC_DATA:
        io_trace_dac(dac_index, dac_phase, value);
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
     * Input status 1. Bit 3 is vertical retrace, bit 0 display enable (low
     * while the picture is being scanned out).
     *
     * OURS - and it is now **answered from the clock**, which is the whole
     * point of it. This used to toggle bit 3 on every read: the driver waits
     * for a retrace edge, first while the bit is set and then until it is set
     * again, and a constant answer hangs one of those two loops whichever
     * value is chosen, so alternating satisfied both and returned at once.
     *
     * Returning at once is exactly what was wrong with it. That wait is the
     * game's frame pacing - `vga_page_flip` does it after every flip it is
     * asked to, and `vm_set_palette` does it before every palette write - and
     * on the original each one costs up to a frame. Costing nothing made the
     * whole game run as fast as the host could push it.
     *
     * The window asserted is the vertical blanking interval, lines 480 to 524
     * of 525, and not the retrace pulse itself. The pulse is lines 490 and
     * 491 - 63 microseconds - and a poll that took longer than that between
     * reads would fall straight through it and wait another whole frame. The
     * blanking interval is 1.4 ms and cannot be missed. It costs the wait up
     * to 1.4 ms of its 16.7, and leaves the *rate* - one frame per wait, which
     * is what pacing means - exact. Ours, approximate, and not measured
     * against a real card.
     */
    case PORT_INPUT_ST1:
        /*
         * The guest is waiting for retrace, which is a guest waiting for time
         * to pass - so this is where the port lets it pass. See io_service_timer.
         */
        io_service_timer();
        /* Reading this port also puts the attribute controller's flip-flop
         * back to expecting an index - see attr_expect_data above. */
        attr_expect_data = 0;
        return (uint8_t)(vga_in_vblank() ? 0x09 : 0x00);
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

/*
 * `TIM_TRACE=vram:<offset>` prints the graphics-controller state every time a
 * byte is written to that plane offset. Ours. What a byte of video memory ends
 * up holding depends on the write mode, the map mask and the bit mask as much
 * as on the value, so "the port wrote the same value and the plane holds
 * something else" is answered here and nowhere else.
 */
static int32_t trace_vram = -2;

void vga_write(uint16_t offset, uint8_t value)
{
    if (trace_vram == -2) {
        const char *spec = getenv("TIM_TRACE");
        const char *at = spec ? strstr(spec, "vram:") : NULL;

        trace_vram = at ? (int32_t)strtol(at + 5, NULL, 0) : -1;
    }
    if (trace_vram >= 0 && offset == (uint16_t)trace_vram)
        fprintf(stderr, "[vram] %04x <- %02x  mode=%d setreset=%02x/%02x "
                        "rotate=%02x mask=%02x mapmask=%02x\n",
                offset, value, gc[5] & 3, gc[0], gc[1], gc[3], gc[8], seq[2]);

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

/*
 * How many scan lines are picture: the blanking line **and everything above
 * it**, so `svb + 1`.
 *
 * The game never touches Vertical Display End - the card goes on scanning 480 -
 * and moves Start Vertical Blank instead, to 0x18f for its own screens and
 * 0x1d6 for the Sierra logo. That makes the picture 400 rows and 471, which is
 * what DOSBox reports for the same two screens, and the game draws a full
 * 640-pixel row at y=399, which it would have no reason to do for a line it
 * could not show.
 *
 * `tools/tim.py` blanks its reference frames the same way. That agreement is
 * worth nothing on its own - the two shared the off-by-one for as long as they
 * shared the convention - so the count is pinned to DOSBox's reading and to the
 * row the game draws, not to either of ours.
 */
int32_t vga_visible_lines(void)
{
    /* Start Vertical Blank: ten bits across 0x15, 0x07 bit 3, 0x09 bit 5. */
    int32_t svb = crtc[0x15]
                | (((crtc[0x07] >> 3) & 1) << 8)
                | (((crtc[0x09] >> 5) & 1) << 9);
    return svb + 1;
}

/*
 * The CRTC's line compare: the scan line at which the card restarts fetching
 * from offset 0. Ten bits, spread as the hardware spreads them - the low eight
 * at index 0x18, bit 8 in Overflow bit 4, bit 9 in Maximum Scan Line bit 6.
 *
 * A card that has never been told otherwise leaves all ten set, which is 0x3ff
 * and past any line, so the split never happens - and that is what the BIOS
 * mode set leaves. The port's CRTC file starts zeroed, though, so an
 * unprogrammed compare would read as 0 and split at the very first line. It is
 * treated as "no split" when the register has not been written.
 */
int32_t vga_line_compare(void)
{
    int32_t lc = crtc[0x18]
               | (((crtc[0x07] >> 4) & 1) << 8)
               | (((crtc[0x09] >> 6) & 1) << 9);

    return lc == 0 ? 0x7fffffff : lc;
}

uint16_t vga_start_address(void)
{
    return (uint16_t)((crtc[0x0C] << 8) | crtc[0x0D]);
}

void vga_compose(uint8_t *out, int32_t width, int32_t height)
{
    int32_t row_bytes = crtc[0x13] ? crtc[0x13] * 2 : width / 8;
    int32_t span = width / 8;
    int32_t shown = vga_visible_lines();
    int32_t split = vga_line_compare();
    uint16_t base = vga_start_address();

    memset(out, 0, (size_t)(width * height));
    for (int32_t y = 0; y < height && y < shown; y++) {
        /*
         * **The split screen.** From the line compare down the card stops
         * following the start address and fetches from offset 0, which is what
         * makes the game's screens 368 rows of picture with a fixed band
         * under them: 0x08f27 sets the compare to 367 while the blanking line
         * says 448. Without this the bottom eighty rows show whatever the
         * start address happens to run into, which looks exactly like another
         * page bleeding through - and was read that way once.
         */
        int32_t src = (y >= split ? 0 : base) + (y - (y >= split ? split : 0))
                      * row_bytes;
        uint8_t *dst = out + (size_t)y * width;
        for (int32_t bx = 0; bx < span; bx++) {
            uint16_t o;

            /*
             * **Past the end of a plane is black, not the start of it.**
             *
             * The address is 16 bits and a `uint16_t` here wrapped it, which
             * is defensible as hardware and is not what the reference does.
             * It matters in exactly two frames of the intro: the transition
             * out of the logo flips to start 0x8200 while the blanking line
             * still says 470 rows, and 470 rows of 80 bytes from 0x8200 runs
             * off the end of the plane at row 403. Wrapped, the bottom of the
             * screen showed the top of the logo again.
             *
             * The game never draws there - the palette is still black at those
             * two flips and nothing was ever visible - so this is a choice
             * about addresses the picture does not use, and it is settled the
             * way the emulator settles it. Neither side is checked against a
             * real card.
             */
            if (src + bx > 0xFFFF)
                continue;
            o = (uint16_t)(src + bx);
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
