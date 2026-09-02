/*
 * The Incredible Machine, run as the original binary with the port's hardware.
 *
 * NOT a transcription. See native.h for the arrangement; the short version is
 * that the emulator executes TIM.EXE, `reconstruct/io.c` **is** the VGA, and
 * routines listed in dispatch.c are executed by the port instead of by the
 * guest. What the game draws therefore goes through the same plane model, the
 * same blitter registers and the same frame composition that the port's own
 * screens are checked against - which is the point: it exercises them inside
 * the real game rather than against a captured call.
 *
 * Nothing here services `int 21h`. That is deliberate and is the whole method:
 * the io layer already is DOS, the Borland routines that would issue the
 * interrupt are dispatched natively, and an interrupt that fires anyway is a
 * routine nobody has wired up yet. It stops the run and prints the guest's own
 * call chain, which says which routine and who wanted it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "sha256.h"
#include "shim.h"
#include "../../reconstruct/dgroup.h"
#include "../../reconstruct/io.h"
#include "../../reconstruct/sdl.h"
#include "../../reconstruct/tim.h"

#define DEFAULT_OUT "out"
#define LOAD_SEG    0x0110u
#define VGA_A000    0xA0000u

/* How many instructions to run before coming up for air - the display and the
 * guest's timer are serviced between slices, not from inside a hook. */
#define SLICE 200000

static uc_engine *g_uc;
static int32_t    g_stop;
/*
 * A clean stop, as against `g_stop` which is a fault. A DOS game does not
 * exit, so every run here had to be killed from outside - which costs the
 * whole budget in wall clock however early the frame wanted arrived, and puts
 * the run's own report out of reach, because a killed process prints nothing.
 */
static int32_t    g_done;
static int32_t    g_stop_at = -2;

/*
 * A routine the port has, reached by the guest. Run ours, then put the machine
 * where the `ret` would have left it and stop the slice so the outer loop
 * restarts at the caller.
 */
/*
 * Places worth being told about the first time the guest reaches them, so a
 * run can be read as progress rather than as a frame count. They are the
 * game's own structure: the intro, then the menu and the briefing behind it.
 */
static const struct { uint32_t at; const char *what; } milestones[] = {
    { 0x0e4be, "game_intro"  },
    { 0x0eed5, "game_play"   },
    { 0x10f03, "game_screen" },
    { 0x0eff5, "game_round"  },
    { 0x0f04b, "round_setup" },
};
static uint8_t milestone_seen[sizeof milestones / sizeof milestones[0]];

/* Counts for TIM_ENTRIES, one per transcribed routine, by image offset. */
static uint32_t *entry_hits;

static void note_entry(uint32_t img)
{
    uint32_t start = 0;
    int32_t i;

    if (!entry_hits)
        return;
    /* Only when the address *is* the routine's start. */
    if (!sym_for(img, &start) || start != img)
        return;
    i = sym_index(img);
    if (i >= 0)
        entry_hits[i]++;
}

static void write_entries(const char *path)
{
    FILE *f = fopen(path, "w");
    int32_t i;

    if (!f)
        return;
    for (i = 0; i < sym_count_of(); i++)
        if (entry_hits[i])
            fprintf(f, "%u %s %u\n", sym_at(i), sym_name(i), entry_hits[i]);
    fclose(f);
}

/*
 * `TIM_COVER=<lo>:<hi>:<path>` - which bytes of one routine the guest actually
 * executed.
 *
 * **A routine verified on one path is not verified.** `tools/verify.py` proves
 * a transcription by stopping at its entry, letting the original body run and
 * comparing what each side did; what it cannot say is how much of the routine
 * the compared calls went through. A branch nobody took is a transcription
 * nobody checked, and it reads as a pass.
 *
 * Unicorn's block hook gives the address and length of each basic block, so
 * the whole block is marked - which is exactly right here, because a block
 * runs to its end by definition.
 */
static uint8_t *cover_map;
static uint32_t cover_lo, cover_hi;
static char cover_path[224];

static void cover_note(uint64_t address, uint32_t size)
{
    uint32_t a, at;

    if (!cover_map || address < IMAGE_BASE)
        return;
    at = (uint32_t)address - IMAGE_BASE;
    for (a = at; a < at + size; a++)
        if (a >= cover_lo && a < cover_hi)
            cover_map[a - cover_lo] = 1;
}

static void cover_write(void)
{
    uint32_t a;
    FILE *f;

    if (!cover_map || !cover_path[0] || (f = fopen(cover_path, "w")) == NULL)
        return;
    for (a = cover_lo; a < cover_hi; a++)
        if (cover_map[a - cover_lo])
            fprintf(f, "%x\n", a);
    fclose(f);
    fprintf(stderr, "native: wrote %s\n", cover_path);
}

static void on_block(uc_engine *uc, uint64_t address, uint32_t size, void *ud)
{
    uint32_t i;

    (void)ud;

    cover_note(address, size);

    if (address >= IMAGE_BASE) {
        uint32_t img = (uint32_t)address - IMAGE_BASE;

        for (i = 0; i < sizeof milestones / sizeof milestones[0]; i++)
            if (milestones[i].at == img && !milestone_seen[i]) {
                milestone_seen[i] = 1;
                fprintf(stderr, "native: reached %s\n", milestones[i].what);
            }
    }
    /*
     * `TIM_ENTRIES=<path>` - every transcribed routine the *emulator* still
     * enters, and how often.
     *
     * `tools/native/coverage.py` counts what the original reaches, which is a
     * fact about the original and goes stale the moment anything is
     * dispatched: a routine whose callers have all been taken over is never
     * reached again, and most of the queue is now in that position. Six
     * routines were dispatched to exercise the pascal convention and not one
     * of them was called. This counts what is actually left.
     *
     * Only entries, not every block: an address that is a routine's first
     * instruction is a call, and the rest is the routine's own control flow.
     */
    if (native_dispatch(uc, (uint32_t)address)) {
        uc_emu_stop(uc);
        return;
    }

    /* Counted only when the dispatcher did *not* take it, so the file is what
     * is left rather than everything that was called. Counted before, it
     * showed atan2_long 452 times with atan2_long dispatched. */
    if (address >= IMAGE_BASE)
        note_entry((uint32_t)address - IMAGE_BASE);
}

/*
 * The A000 aperture, **trapped rather than serviced**.
 *
 * **In practice this hook does not fire, and that is not a fault.** Every path
 * into video memory configures the graphics controller first, and the VGA port
 * trap below catches the `out` before a pixel is written: withdrawing
 * draw_bitmap and vm_blit_bitmap together - the only way to get the emulator
 * back into the drawing path at all, since everything above them is dispatched
 * - stops at `wrote VGA port 3ce` inside vm_blit_bitmap, not at A000.
 *
 * It stays because it costs nothing and it covers the case the port trap
 * cannot: emulated code writing pixels with the controller already set up by a
 * dispatched routine. Nothing does that today. A second line of defence that
 * never fires is worth having only if it is known to be second - hence this
 * note, so nobody later reads its silence as coverage.
 *
 * If the graphics are the port's, the guest has no business in video memory:
 * every routine that touches it is a drawing routine, and a drawing routine
 * reaching the aperture is one that should have been dispatched natively and
 * was not. Servicing the access instead - handing the byte to the port's plane
 * model and letting the guest carry on - makes that routine *work*, quietly,
 * and the fact that it is still being emulated never surfaces.
 *
 * That is the same mistake as a stub returning zero instead of aborting, and
 * this project has paid for it often enough to know what it looks like: the
 * screen comes out right and nothing says which half drew it.
 *
 * So the aperture behaves exactly like `int 21h` here. It stops the run and
 * says who reached it.
 */
static void on_vga_access(uc_engine *uc, uc_mem_type type, uint64_t addr,
                          int size, int64_t value, void *ud)
{
    char why[160];
    int writing = (type == UC_MEM_WRITE);

    (void)ud;
    snprintf(why, sizeof why,
             "the guest %s A000:%04x (%d byte%s%s) - video memory is the "
             "port's, so this is a drawing routine that is not dispatched "
             "natively yet",
             writing ? "wrote" : "read",
             (unsigned)(addr - VGA_A000), size, size == 1 ? "" : "s",
             writing ? "" : "");
    guest_backtrace(uc, why);
    g_stop = 1;
    uc_emu_stop(uc);
}

/*
 * The VGA's own registers - the sequencer, graphics controller, CRTC, attribute
 * controller and DAC.
 *
 * **Trapped, for the same reason as A000.** A routine programming the map mask
 * or the bit mask is setting up a blit, and a blit is the port's work; if the
 * guest is still doing it, the routine around it has not been dispatched. This
 * catches the setup where the aperture trap catches the transfer, and between
 * them there is nowhere for an emulated drawing routine to hide.
 *
 * The rest of the machine's ports are not graphics and are still serviced: the
 * speaker at 0x61, the 8253 at 0x40-0x43, the keyboard at 0x60. Trapping those
 * would stop the run on the first note played, which says nothing about the
 * graphics layer.
 */
static int32_t is_vga_port(uint32_t port)
{
    return (port >= 0x3B0 && port <= 0x3DF) || port == 0x3C0;
}

static void trap_port(uc_engine *uc, const char *how, uint32_t port,
                      uint32_t value, int size)
{
    char why[176];

    snprintf(why, sizeof why,
             "the guest %s VGA port %03x (%d byte%s, value %04x) - the "
             "registers are the port's, so the routine setting up this blit "
             "is not dispatched natively yet",
             how, port, size, size == 1 ? "" : "s", value);
    guest_backtrace(uc, why);
    g_stop = 1;
    uc_emu_stop(uc);
}

static void on_out(uc_engine *uc, uint32_t port, int size, uint32_t value,
                   void *ud)
{
    (void)ud;
    if (is_vga_port(port)) {
        trap_port(uc, "wrote", port, value, size);
        return;
    }
    if (size == 2)
        io_out16((uint16_t)port, (uint16_t)value);
    else
        io_out8((uint16_t)port, (uint8_t)value);
}

static uint32_t on_in(uc_engine *uc, uint32_t port, int size, void *ud)
{
    (void)ud;
    if (is_vga_port(port)) {
        trap_port(uc, "read", port, 0, size);
        return 0;
    }
    return io_in8((uint16_t)port);
}

/*
 * Read and write the interrupt vector table, which lives at the bottom of the
 * guest's memory like the hardware's.
 */
static uint32_t ivt_get(uint32_t n)
{
    uint32_t at = n * 4;

    return (uint32_t)(guest_mem[at] | (guest_mem[at + 1] << 8)) |
           ((uint32_t)(guest_mem[at + 2] | (guest_mem[at + 3] << 8)) << 16);
}

static void ivt_set(uint32_t n, uint16_t seg, uint16_t off)
{
    uint32_t at = n * 4;

    guest_mem[at] = (uint8_t)off;
    guest_mem[at + 1] = (uint8_t)(off >> 8);
    guest_mem[at + 2] = (uint8_t)seg;
    guest_mem[at + 3] = (uint8_t)(seg >> 8);
}

/*
 * Deliver an interrupt to the guest, the way the hardware would.
 *
 * The game installs its own handler for the 8253's IRQ0 and then waits on a
 * word that only the handler sets - `wait_and_latch_frame` sits on it - so
 * without ticks the intro plays and the menu never arrives. The port drives
 * that from a thread, which cannot be used here: it would run the handler
 * while the emulator is executing guest code on the same memory.
 *
 * So the guest gets a real interrupt instead, at a slice boundary where the
 * machine is stopped and CS:IP is a whole instruction. Push flags, CS and IP,
 * clear IF, and enter the vector; the handler's own `iret` puts it all back.
 * That is the CPU's own sequence, and it means the game runs **its** timer
 * handler rather than the port's stand-in.
 *
 * A guest that has cleared IF is in a region it has asked not to be
 * interrupted in, and is left alone until it sets it again.
 */
/*
 * The stack pointer when the last interrupt was delivered, or 0.
 *
 * **One at a time.** Delivering another while the guest is still in the
 * handler re-enters it, and every entry pushes six more bytes: the run reached
 * 8,000 slices and died with the stack walked off the bottom of DGROUP. Real
 * hardware does not do that either - the 8259 holds the next one off until the
 * handler acknowledges it. The `iret` puts SP back, and that is the signal.
 */
static uint16_t g_in_handler_sp;

static int32_t deliver_int(uc_engine *uc, uint32_t n)
{
    uint32_t vec = ivt_get(n);
    uint16_t cs = 0, ip = 0, ss = 0, sp = 0, fl = 0;
    uint32_t at;

    if (!vec)
        return 0;

    uc_reg_read(uc, UC_X86_REG_SP, &sp);
    if (g_in_handler_sp) {
        if (sp < g_in_handler_sp)
            return 0;                  /* still inside it */
        g_in_handler_sp = 0;           /* it has returned */
    }
    uc_reg_read(uc, UC_X86_REG_FLAGS, &fl);
    if (!(fl & 0x200))                 /* IF clear: the guest said not now */
        return 0;

    uc_reg_read(uc, UC_X86_REG_CS, &cs);
    uc_reg_read(uc, UC_X86_REG_IP, &ip);
    uc_reg_read(uc, UC_X86_REG_SS, &ss);

    sp -= 6;
    at = (uint32_t)ss * 16 + sp;
    guest_mem[at + 0] = (uint8_t)ip;
    guest_mem[at + 1] = (uint8_t)(ip >> 8);
    guest_mem[at + 2] = (uint8_t)cs;
    guest_mem[at + 3] = (uint8_t)(cs >> 8);
    guest_mem[at + 4] = (uint8_t)fl;
    guest_mem[at + 5] = (uint8_t)(fl >> 8);

    fl &= (uint16_t)~0x200;            /* the CPU clears IF on entry */
    uc_reg_write(uc, UC_X86_REG_FLAGS, &fl);
    uc_reg_write(uc, UC_X86_REG_SP, &sp);
    {
        uint16_t h_seg = (uint16_t)(vec >> 16), h_off = (uint16_t)vec;

        uc_reg_write(uc, UC_X86_REG_CS, &h_seg);
        uc_reg_write(uc, UC_X86_REG_IP, &h_off);
    }
    g_in_handler_sp = sp;
    return 1;
}

/*
 * An interrupt the guest issued. Not serviced - see the note at the top - so
 * this is where the run ends and the backtrace is worth having.
 *
 * The two exceptions are `int 21h` AH=25 and AH=35, which set and read an
 * interrupt vector. Those are not DOS services in the sense this program
 * refuses to implement - no file, no memory, no device - they are the IVT,
 * which is guest memory, and the game needs them to install the timer handler
 * that `deliver_int` then runs.
 */
static void on_intr(uc_engine *uc, uint32_t intno, void *ud)
{
    char why[128];
    uint16_t ax = 0;

    (void)ud;
    uc_reg_read(uc, UC_X86_REG_AX, &ax);

    if (intno == 0x21 && (ax >> 8) == 0x25) {
        uint16_t ds = 0, dx = 0;

        uc_reg_read(uc, UC_X86_REG_DS, &ds);
        uc_reg_read(uc, UC_X86_REG_DX, &dx);
        ivt_set(ax & 0xFF, ds, dx);
        return;
    }
    if (intno == 0x21 && (ax >> 8) == 0x35) {
        uint32_t v = ivt_get(ax & 0xFF);
        uint16_t es = (uint16_t)(v >> 16), bx = (uint16_t)v;

        uc_reg_write(uc, UC_X86_REG_ES, &es);
        uc_reg_write(uc, UC_X86_REG_BX, &bx);
        return;
    }
    snprintf(why, sizeof why,
             "int %02xh (ah=%02x) reached: the routine that issued it is not "
             "dispatched natively yet", intno, ax >> 8);
    guest_backtrace(uc, why);
    g_stop = 1;
    uc_emu_stop(uc);
}

/* The port's stubs abort; this is what they print first, and it is the guest's
 * chain rather than the port's, which is the half that says who asked. */
/* How many frames the guest has asked to be shown. Cheap, and the difference
 * between "running the intro" and "spinning on a word". */
static uint32_t g_frames;

/*
 * The composed frame, as the port's own plane model has it: 768 bytes of
 * palette then 640x480 indices. `TIM_FRAME=<n>:<path>` writes the nth.
 *
 * Deliberately the port's `vga_compose`, which is what the screen comparisons
 * read - so a frame out of the emulated game and a frame out of the port are
 * the same kind of thing and can be compared directly.
 */
/*
 * `TIM_FRAMEHASH=<path>[:<from>:<to>]` - a digest per frame instead of the
 * frame itself.
 *
 * The comparison asks one question of every frame, "is this byte for byte that
 * one", and 32 bytes answer it as well as 308 KB. A window of 720 frames is
 * 222 MB written and read back to say what 23 KB says, and this machine's disk
 * has been filled twice by frames nobody looked at.
 *
 * The frames themselves are still worth having when something *fails* - the
 * closest-frame diagnostic needs the pixels - so this is in addition to
 * TIM_FRAMES rather than instead of it.
 */
static void hash_frame(uint32_t frame, const char *path)
{
    static uint8_t fb[640 * 480];
    static FILE *out;
    uint8_t pal[768], digest[32];
    sha256_t sh;
    int32_t i;

    if (!out && !(out = fopen(path, "w")))
        return;

    vga_compose(fb, 640, 480);
    vga_palette_rgb(pal);
    sha256_init(&sh);
    sha256_update(&sh, pal, sizeof pal);
    sha256_update(&sh, fb, sizeof fb);
    sha256_final(&sh, digest);

    fprintf(out, "%u ", frame);
    for (i = 0; i < 32; i++)
        fprintf(out, "%02x", digest[i]);
    fputc('\n', out);
    fflush(out);
}

static void dump_frame(const char *path)
{
    static uint8_t fb[640 * 480];
    uint8_t pal[768];
    FILE *f = fopen(path, "wb");

    if (!f)
        return;
    vga_compose(fb, 640, 480);
    vga_palette_rgb(pal);
    fwrite(pal, 1, sizeof pal, f);
    fwrite(fb, 1, sizeof fb, f);
    fclose(f);
    fprintf(stderr, "native: wrote %s\n", path);
}

/*
 * Whether this run has a window. Set once in `main`, because `sdl_open` must
 * happen on the thread that will present and `on_present` must not be deciding
 * policy per frame.
 */
static int32_t g_windowed;

/*
 * **Shift+F2 snapshots the machine**, and this is the whole of why it waits.
 *
 * The key arrives during `sdl_pump`, which runs inside the present hook, which
 * runs between emulator slices - so the guest is stopped, but stopped wherever
 * the slice happened to end: mid-routine, mid-blit, with a half-built frame on
 * the stack. A snapshot taken there restores into a machine that is *in the
 * middle of something*, and the first thing wrong with it will look like a
 * fault in whatever routine is resumed rather than in the capture.
 *
 * The next call out of the guest is a clean boundary and costs nothing to wait
 * for: the arguments are pushed, the return address is on the stack, and the
 * guest is between instructions by construction. So the key only *arms* this,
 * and `native_dispatch` takes the picture.
 */
static int32_t g_snap_armed;
/*
 * Where the next capture goes. Chosen at the moment it is taken, not at
 * start-up, because `io_next_snapshot_path` picks the first free
 * `snapNNN.snap` and a run may take several - one numbering shared with the
 * port, so a session's captures come out in the order they were made whichever
 * binary made them. `TIM_SNAP` still pins a fixed file for the checks.
 */
static char    g_snap_path[512];

static void on_hotkey(int32_t id)
{
    if (id != SDL_HOTKEY_SNAPSHOT)
        return;
    g_snap_armed = 1;
    fprintf(stderr, "native: snapshot armed - taking it at the next call out "
            "of the guest\n");
}

/* The registers a restore has to put back. Written as one block so a version
 * bump is one struct and not a list of reads to keep in step. */
struct snap_regs {
    uint32_t ax, bx, cx, dx, si, di, bp, sp;
    uint32_t cs, ds, es, ss, ip, flags;
};

#define SNAP_MAGIC   0x50414e54u          /* "TNAP" */
#define SNAP_VERSION 1u

static void snap_read_regs(uc_engine *uc, struct snap_regs *r)
{
    uc_reg_read(uc, UC_X86_REG_AX, &r->ax); uc_reg_read(uc, UC_X86_REG_BX, &r->bx);
    uc_reg_read(uc, UC_X86_REG_CX, &r->cx); uc_reg_read(uc, UC_X86_REG_DX, &r->dx);
    uc_reg_read(uc, UC_X86_REG_SI, &r->si); uc_reg_read(uc, UC_X86_REG_DI, &r->di);
    uc_reg_read(uc, UC_X86_REG_BP, &r->bp); uc_reg_read(uc, UC_X86_REG_SP, &r->sp);
    uc_reg_read(uc, UC_X86_REG_CS, &r->cs); uc_reg_read(uc, UC_X86_REG_DS, &r->ds);
    uc_reg_read(uc, UC_X86_REG_ES, &r->es); uc_reg_read(uc, UC_X86_REG_SS, &r->ss);
    uc_reg_read(uc, UC_X86_REG_IP, &r->ip);
    uc_reg_read(uc, UC_X86_REG_FLAGS, &r->flags);
}

static void snap_write_regs(uc_engine *uc, const struct snap_regs *r)
{
    uc_reg_write(uc, UC_X86_REG_AX, &r->ax); uc_reg_write(uc, UC_X86_REG_BX, &r->bx);
    uc_reg_write(uc, UC_X86_REG_CX, &r->cx); uc_reg_write(uc, UC_X86_REG_DX, &r->dx);
    uc_reg_write(uc, UC_X86_REG_SI, &r->si); uc_reg_write(uc, UC_X86_REG_DI, &r->di);
    uc_reg_write(uc, UC_X86_REG_BP, &r->bp); uc_reg_write(uc, UC_X86_REG_SP, &r->sp);
    uc_reg_write(uc, UC_X86_REG_CS, &r->cs); uc_reg_write(uc, UC_X86_REG_DS, &r->ds);
    uc_reg_write(uc, UC_X86_REG_ES, &r->es); uc_reg_write(uc, UC_X86_REG_SS, &r->ss);
    uc_reg_write(uc, UC_X86_REG_IP, &r->ip);
    uc_reg_write(uc, UC_X86_REG_FLAGS, &r->flags);
}

/*
 * Called by `native_dispatch` at every call out of the guest; writes nothing
 * unless the key armed it. `at` names the routine about to run, and goes in
 * the file so a snapshot says where it was taken rather than only when.
 */
void native_snapshot_if_armed(uc_engine *uc, const char *at)
{
    uint32_t magic = SNAP_MAGIC, version = SNAP_VERSION;
    struct snap_regs r;
    char where[64];
    FILE *f;

    if (!g_snap_armed)
        return;
    g_snap_armed = 0;

    io_next_snapshot_path(g_snap_path, sizeof g_snap_path);

    if ((f = fopen(g_snap_path, "wb")) == NULL) {
        fprintf(stderr, "native: cannot write %s\n", g_snap_path);
        return;
    }
    memset(where, 0, sizeof where);
    snprintf(where, sizeof where, "%s", at ? at : "?");
    snap_read_regs(uc, &r);

    if (fwrite(&magic, sizeof magic, 1, f) != 1
        || fwrite(&version, sizeof version, 1, f) != 1
        || fwrite(where, 1, sizeof where, f) != sizeof where
        || fwrite(&r, sizeof r, 1, f) != 1
        || fwrite(guest_mem, 1, GUEST_MEM_BYTES, f) != GUEST_MEM_BYTES
        || !io_state_save(f)) {
        fprintf(stderr, "native: snapshot to %s failed\n", g_snap_path);
        fclose(f);
        return;
    }
    fclose(f);
    fprintf(stderr, "native: wrote %s at %s (frame %u)\n",
            g_snap_path, where, g_frames);
}

/* 1 if the machine now *is* the snapshot. */
static int32_t snap_restore(uc_engine *uc, const char *path)
{
    uint32_t magic = 0, version = 0;
    struct snap_regs r;
    char where[64];
    FILE *f = fopen(path, "rb");

    if (!f) {
        fprintf(stderr, "native: cannot read %s\n", path);
        return 0;
    }
    if (fread(&magic, sizeof magic, 1, f) != 1
        || fread(&version, sizeof version, 1, f) != 1
        || magic != SNAP_MAGIC || version != SNAP_VERSION) {
        fprintf(stderr, "native: %s is not a snapshot this build knows\n",
                path);
        fclose(f);
        return 0;
    }
    if (fread(where, 1, sizeof where, f) != sizeof where
        || fread(&r, sizeof r, 1, f) != 1
        || fread(guest_mem, 1, GUEST_MEM_BYTES, f) != GUEST_MEM_BYTES
        || !io_state_load(f)) {
        fprintf(stderr, "native: %s is truncated or damaged\n", path);
        fclose(f);
        return 0;
    }
    fclose(f);
    where[sizeof where - 1] = 0;
    snap_write_regs(uc, &r);
    fprintf(stderr, "native: restored %s, taken at %s\n", path, where);
    return 1;
}

static void on_present(void)
{
    g_frames++;

    /* `TIM_STOP=<frame>` - run to that frame and stop, having presented it. */
    if (g_stop_at == -2) {
        const char *spec = getenv("TIM_STOP");

        g_stop_at = spec ? (int32_t)strtol(spec, NULL, 0) : -1;
    }

    /*
     * `TIM_SNAPAT=<frame>` arms the snapshot from the clock instead of from
     * the key, so a capture can be taken without a keyboard. That is what
     * makes the feature testable at all - Shift+F2 cannot be pressed by a
     * check - and it is the same arming, so what it proves is the same code.
     */
    {
        static int32_t snap_at = -2;

        if (snap_at == -2) {
            const char *spec = getenv("TIM_SNAPAT");

            snap_at = spec ? (int32_t)strtol(spec, NULL, 0) : -1;
        }
        if (snap_at > 0 && (int32_t)g_frames == snap_at)
            g_snap_armed = 1;
    }
    if (g_stop_at > 0 && (int32_t)g_frames >= g_stop_at)
        g_done = 1;

    {
        const char *spec = getenv("TIM_FRAME");
        const char *every = getenv("TIM_FRAMES");
        int32_t at = -1, step = 0;
        char path[256], dir[224];

        if (spec && sscanf(spec, "%d:%255s", &at, path) == 2 &&
            (int32_t)g_frames == at)
            dump_frame(path);

        /*
         * `TIM_FRAMES=<dir>:<step>[:<from>:<to>]` - every step-th frame, so a
         * whole sequence comes out of one run rather than one run per frame,
         * and optionally only across a window of frame numbers.
         *
         * The window is not a convenience. The title screen animates, so a
         * sampled frame can only ever land on a phase that is a multiple of
         * the step: at step 20 this reproduced ten of the port's sixteen
         * title-screen flips byte for byte and missed six, and the six were
         * exactly the phases no multiple of 20 falls on. It read as a
         * rendering fault and was a sampling one. Every frame across a narrow
         * window settles that, without the 308 KB a frame costs adding up to a
         * gigabyte over a whole run - which has filled this machine's disk
         * twice already.
         */
        {
            const char *hspec = getenv("TIM_FRAMEHASH");
            char hpath[224];
            int32_t hfrom = 0, hto = 0;
            int32_t hgot = hspec ? sscanf(hspec, "%223[^:]:%d:%d",
                                          hpath, &hfrom, &hto) : 0;

            if (hgot >= 1 && (hgot < 3 || ((int32_t)g_frames >= hfrom &&
                                           (int32_t)g_frames <= hto)))
                hash_frame(g_frames, hpath);
        }

        if (every) {
            int32_t from = 0, to = 0;
            int32_t got = sscanf(every, "%223[^:]:%d:%d:%d", dir, &step,
                                 &from, &to);

            if (got >= 2 && step > 0 && g_frames % (uint32_t)step == 0 &&
                (got < 4 || ((int32_t)g_frames >= from &&
                             (int32_t)g_frames <= to))) {
                char p[300];

                snprintf(p, sizeof p, "%s/f%05u.raw", dir, g_frames);
                dump_frame(p);
            }
        }
    }

    /*
     * A click, at a frame chosen the way the port's own comparisons choose
     * one. The intro waits for input - it presented five thousand frames and
     * went nowhere without this - and the copy-protection screen behind it
     * wants the same. `TIM_CLICK=<frame>:<x>:<y>,...` takes the same form
     * devdump.c reads, so a sequence that drives the port drives this too.
     *
     * **The button is held, and for far longer than the port holds it.** The
     * port's clicks are placed at page *flips* and the game polls its input
     * once a flip, so two flips is a press it cannot miss. Here the frames are
     * the window's, at 59.94 Hz, and the guest's `update_button_state` runs
     * about once in four of them - 75 polls across 300 frames, measured from
     * the level-one screen. A two-frame press therefore lands, and is gone
     * again, entirely between two polls.
     *
     * That is exactly what it looked like: `io_mouse_input` reached with the
     * right position and mask, `DG8(0x48eb)` going 01 and back to 00, and the
     * guest's button state never leaving 0 - which reads as input that never
     * arrives rather than input that arrives and is not looked at. Eight
     * clicks in a run changed nothing but a timing counter.
     *
     * So the hold is sixteen frames by default, about four polls, and
     * `TIM_CLICK=<frame>:<x>:<y>[:<hold>]` sets it per click.
     */
#define CLICK_HOLD 16
    {
        static int32_t at[8], cx[8], cy[8], hold[8], n = -1;
        int32_t i;

        if (n < 0) {
            const char *spec = getenv("TIM_CLICK");

            n = 0;
            while (spec && *spec && n < 8) {
                hold[n] = CLICK_HOLD;
                if (sscanf(spec, "%d:%d:%d:%d", &at[n], &cx[n], &cy[n],
                           &hold[n]) < 3)
                    break;
                n++;
                spec = strchr(spec, ',');
                if (spec)
                    spec++;
            }
        }
        for (i = 0; i < n; i++) {
            /* Held down across the whole window rather than pressed once:
             * the guest reads the byte, it does not receive an event, so what
             * matters is that the byte is 1 when it happens to look. */
            /* A hold of 0 is a **move with no press**, which is the one
             * gesture this could not express: the pointer cannot be put
             * somewhere without clicking there, so a game that latches the
             * position on the frame before the button can never be driven to
             * the place the click was meant for. */
            if (hold[i] == 0) {
                if ((int32_t)g_frames == at[i])
                    io_mouse_input(cx[i], cy[i], 0);
            } else if ((int32_t)g_frames >= at[i]
                       && (int32_t)g_frames < at[i] + hold[i])
                io_mouse_input(cx[i], cy[i], 1);
            else if ((int32_t)g_frames == at[i] + hold[i])
                io_mouse_input(cx[i], cy[i], 0);
        }
    }

    /*
     * **The window, last**, so what it shows is the frame the writers above
     * just wrote rather than one composed a second time - CLAUDE.md's rule
     * that the file writer is a mode of the same composed frame and not a
     * parallel path. `sdl_present` ends by pumping SDL's queue, so this is
     * also where the real mouse and the keyboard reach the guest: registering
     * one hook gets the display and the input together, exactly as main.c
     * gets them.
     */
    if (g_windowed)
        sdl_present();
}

static void on_port_abort(void)
{
    if (g_uc)
        guest_backtrace(g_uc, "the port reached a routine it has not "
                        "transcribed yet");
}

int main(void)
{
    const char *dir = getenv("TIM_DIR");
    char img[512], exe[512];
    uint8_t *hdr = NULL;
    FILE *fp;
    uint16_t h[16];
    uint16_t cs, ip, ss, sp;
    uc_engine *uc;
    uc_err err;
    uc_hook hh;
    int32_t bound = 0;
    uint32_t slices = 0;
    uint32_t ticks = 0;

    if (!dir)
        dir = DEFAULT_OUT;
    snprintf(img, sizeof img, "%s/TIM.img", dir);
    snprintf(exe, sizeof exe, "%s/TIM.unpacked.exe", dir);

    io_reset();
    if (!io_load_program(img, exe)) {
        fprintf(stderr, "cannot read %s and %s - run tools/unlzexe.py first, "
                "or set TIM_DIR\n", img, exe);
        return 1;
    }

    /* The entry point and stack the loader would have used, from the same
     * header io_load_program relocated against. */
    fp = fopen(exe, "rb");
    if (!fp || fread(h, 2, 16, fp) != 16) {
        fprintf(stderr, "cannot read the EXE header from %s\n", exe);
        return 1;
    }
    fclose(fp);
    (void)hdr;
    (void)h;

    /*
     * **Start at the game's own `main`, not at the EXE's entry point.**
     *
     * Between the two is Borland's C start-up, which is not transcribed and is
     * nothing but DOS: it asks for the version (`int 21h ah=30`, the first
     * thing this runner ever trapped), hands the tail of the arena back, sets
     * up the PSP and runs the init table. Emulating it would mean writing the
     * DOS this project deliberately does not write, to reach a routine the
     * port already knows how to prepare for.
     *
     * `reconstruct/main.c` says what of it matters: the init table at DGROUP
     * 0x4e48 has a single entry and it is `setup_streams`, without which every
     * stream looks already-open and no file can be read. So do that, and enter
     * at `game_main`.
     *
     * SS and DS are DGROUP. That is Borland's large model - the stack lives at
     * the top of the same 64 KB segment as the data, which is the arrangement
     * CLAUDE.md records the emulator's arena having to respect - and it is
     * what `guest_sp` in dgroup.c already assumes.
     */
    setup_streams();

    ss = (uint16_t)(dgroup_base >> 4);
    sp = 0xFF9E;

    /*
     * **CS is the module's segment, not the image's.**
     *
     * `game_main` is at image 0x0dfff, and entering at CS = IMAGE_BASE>>4 with
     * IP = 0xdfff addresses exactly the same byte - so it starts, and runs, and
     * is wrong. Near calls are relative to CS, and the linker computed their
     * displacements for the segment each module is actually loaded at. With CS
     * one module too low, `game_startup`'s call to `read_tim_cfg` arrived at
     * 0110:2ba7 instead of f0f:4bb7 - a routine 0x10000 bytes away, which
     * promptly divided by zero.
     *
     * That cost an hour and two wrong diagnoses: first that a config file was
     * unreadable, then that the disassembler's `call 0x12ba7` was a listing
     * artefact of a displacement wrapping the segment. The listing was right
     * both times.
     *
     * seg0dff.c is based at image 0x0dff0, so the segment is that paragraph
     * plus the load address, and IP is what is left.
     */
    cs = (uint16_t)((IMAGE_BASE >> 4) + 0x0dff);
    ip = 0x000f;

    err = uc_open(UC_ARCH_X86, UC_MODE_16, &uc);
    if (err) {
        fprintf(stderr, "uc_open: %s\n", uc_strerror(err));
        return 1;
    }
    g_uc = uc;

    /*
     * **The one line the whole arrangement rests on.** The guest's megabyte is
     * the port's `guest_mem`, not a copy of it, so a routine executed natively
     * and a routine executed by the emulator write the same bytes. `dgroup_base`
     * already defaults to 0x2e4c0, which is this load segment's DGROUP.
     */
    err = uc_mem_map_ptr(uc, 0, GUEST_MEM_BYTES, UC_PROT_ALL, guest_mem);
    if (err) {
        fprintf(stderr, "uc_mem_map_ptr: %s\n", uc_strerror(err));
        return 1;
    }

    uc_reg_write(uc, UC_X86_REG_CS, &cs);
    uc_reg_write(uc, UC_X86_REG_IP, &ip);
    uc_reg_write(uc, UC_X86_REG_SS, &ss);
    uc_reg_write(uc, UC_X86_REG_SP, &sp);
    {
        uint16_t dg = (uint16_t)(dgroup_base >> 4);

        uc_reg_write(uc, UC_X86_REG_DS, &dg);
        uc_reg_write(uc, UC_X86_REG_ES, &dg);
    }

    /*
     * A return address `game_main` can come back to. It does not - the game
     * ends by exiting - but a far `retf` into whatever happened to be on the
     * stack is the kind of failure that looks like anything else, so give it
     * somewhere deliberate to land and notice when it does.
     */
    {
        uint32_t at = (uint32_t)ss * 16 + sp;

        guest_mem[at] = 0xFF; guest_mem[at + 1] = 0xFF;
        guest_mem[at + 2] = 0xFF; guest_mem[at + 3] = 0xFF;
    }

    /*
     * An `iret` for every vector, before the game looks at any of them.
     *
     * The game saves the handler it is replacing and chains to it - that is
     * what a well-behaved DOS program does - and on an empty IVT the saved
     * pointer is 0000:0000, so its own timer handler far-calls null. The run
     * reached 3,596 frames and then sat executing zeros, which is a hang that
     * looks nothing like its cause.
     *
     * A real machine has the BIOS there. This is the smallest thing that is
     * true of a real machine: one `iret` at 0050:0000, and every vector
     * pointing at it. The game's own handlers still go in the IVT over the top,
     * through `int 21h` AH=25; this only decides what chaining reaches.
     */
    {
        uint32_t i;

        guest_mem[0x500] = 0xCF;               /* iret */
        for (i = 0; i < 256; i++)
            ivt_set(i, 0x0050, 0x0000);
    }

    native_bind_image();

    uc_hook_add(uc, &hh, UC_HOOK_BLOCK, (void *)on_block, NULL, 1, 0);
    uc_hook_add(uc, &hh, UC_HOOK_INTR, (void *)on_intr, NULL, 1, 0);
    uc_hook_add(uc, &hh, UC_HOOK_INSN, (void *)on_out, NULL, 1, 0,
                UC_X86_INS_OUT);
    uc_hook_add(uc, &hh, UC_HOOK_INSN, (void *)on_in, NULL, 1, 0,
                UC_X86_INS_IN);
    uc_hook_add(uc, &hh, UC_HOOK_MEM_READ, (void *)on_vga_access, NULL,
                VGA_A000, VGA_A000 + 0xFFFF);
    uc_hook_add(uc, &hh, UC_HOOK_MEM_WRITE, (void *)on_vga_access, NULL,
                VGA_A000, VGA_A000 + 0xFFFF);

    io_on_abort(on_port_abort);

    /*
     * Page flips, so a run can be told from a hang.
     *
     * **The clock is deliberately not `io_set_timer`.** That starts a thread
     * which calls the handler while the emulator is executing guest code on
     * the same memory, and the two race: it segfaults within seconds. The port
     * can get away with it because nothing else is running; here the emulator
     * is. So the tick is driven from the loop below instead, between slices,
     * where the machine is stopped and the port has it to itself.
     */
    {
        const char *spec = getenv("TIM_ENTRIES");

        if (spec)
            entry_hits = calloc((size_t)sym_count_of(), sizeof *entry_hits);
    }

    /*
     * **The window, wired the way main.c wires it.** The hybrid is the game
     * being played by the port's code, so it shows the game: same `sdl.c`,
     * same grab, same Ctrl+Alt to hand the pointer back, same Escape to quit.
     *
     * Opened *before* `io_on_present`, because that call records the calling
     * thread as the one allowed to touch the renderer and the guest's timer
     * runs on another - see io.c, where two threads inside `SDL_RenderPresent`
     * wedged the renderer three different-looking ways.
     *
     * `TIM_HEADLESS=1` runs without one. tools/check_native.py sets it: a
     * comparison reads the composed frame and needs no window, and a machine
     * with no display should still be able to prove the port.
     */
    {
        const char *spec = getenv("TIM_COVER");
        char path[224];
        uint32_t lo = 0, hi = 0;

        if (spec && sscanf(spec, "%x:%x:%223s", &lo, &hi, path) == 3
            && hi > lo) {
            cover_lo = lo;
            cover_hi = hi;
            cover_map = calloc(hi - lo, 1);
            snprintf(cover_path, sizeof cover_path, "%s", path);
        }
    }

    g_windowed = getenv("TIM_HEADLESS") == NULL;
    if (g_windowed && !sdl_open()) {
        fprintf(stderr, "native: no window - set TIM_HEADLESS=1 to run "
                "without one\n");
        return 1;
    }
    io_on_present(on_present);
    if (g_windowed) {
        io_on_abort(sdl_hold);
        sdl_on_hotkey(on_hotkey);
    }

    /*
     * `TIM_SNAP=<path>` where Shift+F2 writes, and `TIM_RESTORE=<path>` a
     * machine to start as instead of the entry point. Restoring *after*
     * `native_bind_image` because binding reads the image, which the snapshot
     * then overwrites with the same bytes plus everything the run had changed.
     */
    {
        const char *from = getenv("TIM_RESTORE");

        if (from && !snap_restore(uc, from))
            return 1;
    }

    fprintf(stderr, "native: entry %04x:%04x  stack %04x:%04x  %d routines "
            "dispatched%s\n", cs, ip, ss, sp, native_count_routines(),
            g_windowed ? "" : "  (headless)");

    while (!g_stop && !g_done) {
        uint32_t at;

        uc_reg_read(uc, UC_X86_REG_CS, &cs);
        uc_reg_read(uc, UC_X86_REG_IP, &ip);
        at = (uint32_t)cs * 16 + ip;

        err = uc_emu_start(uc, at, 0, 0, SLICE);
        if (err && err != UC_ERR_OK) {
            char why[128];

            snprintf(why, sizeof why, "emulation stopped: %s",
                     uc_strerror(err));
            guest_backtrace(uc, why);
            break;
        }
        if (!bound)
            bound = native_bind_overlay(uc);

        /*
         * The 8253's tick, delivered against the frames.
         *
         * The rate is empirical. Six a frame reproduces the intro exactly, and
         * so do three and four, so the game is not sensitive to it within that
         * band. The hardware's own ratio is not in the band: 18.2 ticks
         * against 70 frames is about one to four, and at that rate the intro
         * stalls after fifteen frames. Why it does is **not understood** -
         * either these frames are not the guest's 70 Hz, or the intro waits on
         * something other than the tick count - and the honest thing is to say
         * so here rather than dress six up as a derivation.
         *
         * **Driven from the frames, not from the slices.** A slice is a batch
         * of emulated instructions, so tying the tick to it ties the guest's
         * clock to how much of the game is dispatched to the port: every
         * routine taken off the queue removes emulated instructions, the
         * slices thin out, the ticks thin out with them and the tick-counted
         * intro runs longer. Dispatching two block moves stretched it from 540
         * frames to 828 - the pixels were identical and the check failed,
         * because the frames it wanted were past the end of its window. That
         * is the whole queue's worth of false alarms, once each.
         *
         * The frame count is the guest's own cue and does not move, so the
         * ratio holds however much runs natively.
         */
        {
            uint32_t owed = (uint32_t)((uint64_t)g_frames * 6);

            while (ticks < owed) {
                deliver_int(uc, 8);
                ticks++;
            }
        }

        /* Where it is, every so often. A run that is drawing and a run that is
         * spinning on a word look identical from outside, and this is the
         * cheapest thing that tells them apart. */
        if (++slices % 400 == 0) {
            uint32_t start = 0;
            const char *n;

            uc_reg_read(uc, UC_X86_REG_CS, &cs);
            uc_reg_read(uc, UC_X86_REG_IP, &ip);
            at = (uint32_t)cs * 16 + ip;
            n = (at >= IMAGE_BASE) ? sym_for(at - IMAGE_BASE, &start) : NULL;
            /*
             * The offset into the routine, not just its name. `sym_for` gives
             * the nearest preceding symbol, so an address in an untranscribed
             * routine is reported under whichever transcribed one came before
             * it - 0x1e540 read as `huffman_update` for sixteen samples and it
             * is thirty bytes past that routine's last `ret`. Printed as
             * `huffman_update+0x208` the same line says "not really".
             */
            fprintf(stderr, "native: %8u slices  %u frames  pace %5d btn %d  at "
                    "%04x:%04x %s+%#x\n", slices, g_frames,
                    (int)DGU16(0x44ef), (int)DGU16(0x5774), cs, ip,
                    n ? n : "(overlay or untranscribed)",
                    n ? (unsigned)(at - IMAGE_BASE - start) : 0u);
        }

        io_service_display();
    }

    cover_write();
    fprintf(stderr, "native: %u frames presented\n", g_frames);
    {
        const char *spec = getenv("TIM_ENTRIES");

        if (spec && entry_hits)
            write_entries(spec);
    }
    native_report();
    return g_stop ? 1 : 0;
}
