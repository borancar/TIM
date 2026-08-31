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
#include "shim.h"
#include "../../reconstruct/dgroup.h"
#include "../../reconstruct/io.h"
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

static void on_block(uc_engine *uc, uint64_t address, uint32_t size, void *ud)
{
    uint32_t i;

    (void)size;
    (void)ud;

    if (address >= IMAGE_BASE) {
        uint32_t img = (uint32_t)address - IMAGE_BASE;

        for (i = 0; i < sizeof milestones / sizeof milestones[0]; i++)
            if (milestones[i].at == img && !milestone_seen[i]) {
                milestone_seen[i] = 1;
                fprintf(stderr, "native: reached %s\n", milestones[i].what);
            }
    }
    if (native_dispatch(uc, (uint32_t)address))
        uc_emu_stop(uc);
}

/*
 * The A000 aperture, **trapped rather than serviced**.
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

static void on_present(void)
{
    g_frames++;

    {
        const char *spec = getenv("TIM_FRAME");
        const char *every = getenv("TIM_FRAMES");
        int32_t at = -1, step = 0;
        char path[256], dir[224];

        if (spec && sscanf(spec, "%d:%255s", &at, path) == 2 &&
            (int32_t)g_frames == at)
            dump_frame(path);

        /* `TIM_FRAMES=<dir>:<step>` - every step-th frame, so a whole
         * sequence comes out of one run rather than one run per frame. */
        if (every && sscanf(every, "%223[^:]:%d", dir, &step) == 2 &&
            step > 0 && g_frames % (uint32_t)step == 0) {
            char p[300];

            snprintf(p, sizeof p, "%s/f%05u.raw", dir, g_frames);
            dump_frame(p);
        }
    }

    /*
     * A click, at a frame chosen the way the port's own comparisons choose
     * one. The intro waits for input - it presented five thousand frames and
     * went nowhere without this - and the copy-protection screen behind it
     * wants the same. `TIM_CLICK=<frame>:<x>:<y>,...` takes the same form
     * devdump.c reads, so a sequence that drives the port drives this too.
     *
     * The button goes down at the frame and up two later, which is what the
     * port does and what the game's own edge detection expects.
     */
    {
        static int32_t at[8], cx[8], cy[8], n = -1;
        int32_t i;

        if (n < 0) {
            const char *spec = getenv("TIM_CLICK");

            n = 0;
            while (spec && *spec && n < 8) {
                if (sscanf(spec, "%d:%d:%d", &at[n], &cx[n], &cy[n]) != 3)
                    break;
                n++;
                spec = strchr(spec, ',');
                if (spec)
                    spec++;
            }
        }
        for (i = 0; i < n; i++) {
            if ((int32_t)g_frames == at[i])
                io_mouse_input(cx[i], cy[i], 1);
            else if ((int32_t)g_frames == at[i] + 2)
                io_mouse_input(cx[i], cy[i], 0);
        }
    }
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
    io_on_present(on_present);

    fprintf(stderr, "native: entry %04x:%04x  stack %04x:%04x  %d routines "
            "dispatched\n", cs, ip, ss, sp, native_count_routines());

    while (!g_stop) {
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
         * The 8253's tick, at something like its real rate *relative to the
         * frames*. The hardware gives 18.2 ticks against 70 frames - about one
         * tick to four - and the game does not only wait on the handler, it
         * times the intro by counting ticks. One a slice is six a frame, some
         * twenty-five times too fast, and the intro then behaves like a game
         * whose clock is running away with it.
         *
         * The ratio is what matters here, not the absolute rate: the emulator
         * has no wall clock to keep and the frames come as fast as it draws.
         */
        if (slices % 2 == 0)
            deliver_int(uc, 8);

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
            fprintf(stderr, "native: %8u slices  %u frames  pace %5d btn %d  at "
                    "%04x:%04x %s\n", slices, g_frames,
                    (int)DGU16(0x44ef), (int)DGU16(0x5774), cs, ip,
                    n ? n : "(overlay or untranscribed)");
        }

        io_service_display();
    }

    fprintf(stderr, "native: %u frames presented\n", g_frames);
    native_report();
    return g_stop ? 1 : 0;
}
