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
static void on_block(uc_engine *uc, uint64_t address, uint32_t size, void *ud)
{
    native_fn *f = native_lookup((uint32_t)address);

    (void)size;
    (void)ud;
    if (f) {
        native_call(uc, f);
        uc_emu_stop(uc);
    }
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
 * An interrupt. Not serviced - see the note at the top - so this is where the
 * run ends and the backtrace is worth having.
 */
static void on_intr(uc_engine *uc, uint32_t intno, void *ud)
{
    char why[128];
    uint16_t ax = 0;

    (void)ud;
    uc_reg_read(uc, UC_X86_REG_AX, &ax);
    snprintf(why, sizeof why,
             "int %02xh (ah=%02x) reached: the routine that issued it is not "
             "dispatched natively yet", intno, ax >> 8);
    guest_backtrace(uc, why);
    g_stop = 1;
    uc_emu_stop(uc);
}

/* The port's stubs abort; this is what they print first, and it is the guest's
 * chain rather than the port's, which is the half that says who asked. */
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
    cs = (uint16_t)(IMAGE_BASE >> 4);
    ip = 0x0dfff;

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

    fprintf(stderr, "native: entry %04x:%04x  stack %04x:%04x  %d routines "
            "dispatched\n", cs, ip, ss, sp, native_count);

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
        io_service_timer();
        io_service_display();
    }

    {
        int32_t i;

        fprintf(stderr, "native: dispatched calls\n");
        for (i = 0; i < native_count; i++)
            if (native_table[i].hits)
                fprintf(stderr, "    %-20s %u\n", native_table[i].name,
                        native_table[i].hits);
    }
    return g_stop ? 1 : 0;
}
