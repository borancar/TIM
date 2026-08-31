/*
 * Which routines the emulator must not execute, and how to stand in for them.
 *
 * NOT a transcription: the table is the seam, not the game. Each entry says
 * where the original's routine is, what the port calls it, and the three
 * things needed to put the guest back afterwards - whether it returns near or
 * far, how many stack words it takes, and what a `ret N` removes.
 *
 * **Every one of those was read off the routine, not inferred.** The thirteen
 * DOS wrappers here are far and leave the arguments to the caller; `dos_creat`
 * sitting among them is *near* and pascal - `ret 4` at 0x0d59a - and taking it
 * for one of its siblings desynchronises the guest's stack, which surfaces as a
 * crash somewhere else entirely. That is the trap CLAUDE.md records about the
 * allocator's four conventions, in a place where it costs a whole run.
 *
 * The table starts with the routines that would otherwise execute `int 21h`,
 * because that interrupt is deliberately not serviced: the port's io layer
 * already is DOS, and a routine reaching the interrupt means it has not been
 * dispatched here yet. It grows from what the traps report, rather than from
 * guessing what the game will want.
 */
#include <stdio.h>
#include <string.h>

#include "native.h"
#include "../../reconstruct/dgroup.h"
#include "../../reconstruct/tim.h"

#define FAR_C(a, n, r, f)  { (a), #f, (void *)(f), 0, 0, 0, 1, (n), 0, (r), 0, 0 }
/* ...and one whose arguments are not all plain words. */
#define FAR_A(a, d, r, f)  { (a), #f, (void *)(f), (d), 0, 0, 1, 0, 0, (r), 0, 0 }
#define NEAR_P(a, n, p, r, f) { (a), #f, (void *)(f), 0, 0, 0, 0, (n), (p), (r), 0, 0 }
/*
 * A video driver routine. The offset is into VM.OVL, not the image, and the
 * loader chooses where that goes - `native_bind_overlay` fills the address in
 * once the game has loaded it. They are entered through far pointers in
 * DGROUP, so they are far.
 */
#define OVL_C(a, n, r, f)  { (a), #f, (void *)(f), 0, 0, 1, 1, (n), 0, (r), 0, 0 }
#define OVL_A(a, d, r, f)  { (a), #f, (void *)(f), (d), 0, 1, 1, 0, 0, (r), 0, 0 }
/* A routine whose arguments arrive in registers. `g` is the order they are in. */
#define REG_N(a, g, n, r, f)  { (a), #f, (void *)(f), 0, (g), 0, 0, (n), 0, (r), 0, 0 }

/*
 * The polygon filler's register conventions, read off each entry earlier and
 * recorded in tools/verify.py's specs. They are **not** uniform:
 * `poly_edge_vertical` takes one x in bp and has si and cx the other way round
 * from the four that take two. Each list is that routine's own.
 */
static const int R_poly_walk[] = { UC_X86_REG_ES, UC_X86_REG_AX, UC_X86_REG_BX,
                                   UC_X86_REG_SI, UC_X86_REG_BP, UC_X86_REG_CX,
                                   UC_X86_REG_DI };
static const int R_edge_vert[] = { UC_X86_REG_ES, UC_X86_REG_BP,
                                   UC_X86_REG_SI, UC_X86_REG_CX };
static const int R_edge_two[]  = { UC_X86_REG_ES, UC_X86_REG_BX, UC_X86_REG_BP,
                                   UC_X86_REG_CX, UC_X86_REG_SI };
static const int R_outline[]   = { UC_X86_REG_DI, UC_X86_REG_SI,
                                   UC_X86_REG_BP };

native_fn native_table[] = {
    /*
     * The DOS wrappers. All far and caller-cleaned, except where marked.
     * Addresses and conventions read from the disassembly; see the note above
     * about the one that differs.
     */
    FAR_C (0x0b6b7, 2, RET_AX,   dos_findfirst),
    FAR_C (0x0b6d3, 2, RET_AX,   dos_findnext),
    FAR_C (0x0b755, 1, RET_AX,   dos_chdir),
    FAR_C (0x0b794, 1, RET_AX,   dos_unlink),
    FAR_C (0x0bd4a, 1, RET_NONE, dos_getdate),
    FAR_C (0x0c018, 1, RET_AX,   dos_isatty),
    FAR_C (0x0c0c3, 4, RET_DXAX, dos_lseek),
    FAR_C (0x0c185, 3, RET_AX,   dos_read),
    FAR_C (0x0c8a3, 4, RET_AX,   dos_ioctl),
    FAR_C (0x0cd3d, 3, RET_AX,   dos_getattr),
    FAR_C (0x0cd80, 1, RET_AX,   dos_close),
    FAR_C (0x0d707, 2, RET_AX,   dos_open_named),
    FAR_C (0x0b7b3, 1, RET_NONE, dos_get_cur_dir),
    FAR_C (0x0bd70, 1, RET_DXAX, dos_getvect),
    FAR_C (0x0bd7f, 3, RET_NONE, dos_setvect),
    FAR_C (0x0b819, 1, RET_NONE, dos_setdisk),
    FAR_C (0x0df7a, 3, RET_AX,   dos_write),

    /*
     * The stdio layer above them. Dispatching these as well as the DOS
     * wrappers is not belt and braces: `read_tim_cfg` divided by zero with
     * only the wrappers native, because the buffering above them was still
     * being emulated. Reusing the port's is the point of the exercise.
     */
    FAR_C (0x0ce15, 1, RET_AX,   stdio_fclose),
    FAR_C (0x0ce92, 1, RET_AX,   flush_stream),
    FAR_C (0x0d0ce, 2, RET_AX,   stdio_fopen),
    FAR_C (0x0d1c4, 4, RET_AX,   stdio_fread),
    FAR_C (0x0d26c, 4, RET_AX,   stdio_fseek),
    FAR_C (0x0d404, 1, RET_AX,   stdio_fgetc),
    FAR_C (0x0c27b, 1, RET_DXAX, dos_tell),
    FAR_C (0x0da6d, 3, RET_AX,   read_translated),

    /*
     * What adapter the machine has. The original asks the BIOS - `int 10h
     * ah=1a`, then `ah=12h bl=10` - and CLAUDE.md records what happens when
     * nothing answers: the game decides there is no VGA and no EGA, fails to
     * load VM.OVL and prints "Unable to initialize vm." There is no BIOS here
     * either, and the port's version already knows the answer.
     */
    { 0x225d2, "detect_adapter", (void *)detect_adapter, 0, 0, 0, 0, 0, 0,
      RET_AX, 0, 0 },   /* near: it ends `ret`, unlike most of this file */

    /* The mouse driver, INT 33h. There is none here; the io layer is it. */
    FAR_C (0x21f1d, 0, RET_AX,   mouse_init),
    FAR_C (0x0b859, 1, RET_NONE, mouse_set_speed),
    FAR_C (0x21f8d, 4, RET_NONE, mouse_set_ranges),
    FAR_C (0x21fbe, 2, RET_NONE, mouse_set_user_handler),
    FAR_C (0x2200f, 0, RET_NONE, mouse_save_vga),
    FAR_C (0x22074, 0, RET_NONE, mouse_restore_vga),
    FAR_C (0x22113, 2, RET_AX,   mouse_move_to),

    /* The sound driver's timer, and the keyboard handler. Both take their
     * own vectors, which is why they reach `int 21h` with dos_getvect
     * already dispatched - they do not go through it. */
    /* NOT dispatched: `timer_install` is left to the guest so that its own
     * handler goes into the IVT and `deliver_int` can run it. Its `int 21h`
     * AH=35/25 are the vector table, which native.c answers. */

    /* The keyboard handler's installation - it takes its own vector. */
    FAR_C (0x21094, 1, RET_AX,   install_keyboard),

    /*
     * Drawing. These are the routines verified against the original earlier,
     * so the intro and the briefing are drawn by the port rather than by the
     * guest - which is what "a native graphics layer" means once the driver
     * underneath it is already native.
     *
     * `restore_write_mode` is here for its own sake: it is the routine
     * `load_screen_plain` was calling wrongly until today, and having the port
     * do both halves keeps the pair honest.
     */
    FAR_C (0x1e94c, 0, RET_NONE, restore_write_mode),

    /*
     * The polygon filler. Register arguments, which is why the table needed
     * them: none of these takes anything on the stack. `poly_edge_vertical`
     * disagrees with the four that take two x's about which register holds
     * which end, so each has its own list rather than a shared one.
     *
     * All near but `clip_polygon`, which ends `retf` at 0x21087.
     */
    REG_N (0x1f562, R_poly_walk, 7, RET_NONE, poly_walk),
    REG_N (0x1f265, R_edge_vert, 4, RET_NONE, poly_edge_vertical),
    REG_N (0x1f3bf, R_edge_two,  5, RET_NONE, poly_edge_diagonal),
    REG_N (0x1f281, R_edge_two,  5, RET_NONE, poly_edge_steep),
    REG_N (0x1f3e6, R_edge_two,  5, RET_NONE, poly_edge_shallow_right),
    REG_N (0x1f4a1, R_edge_two,  5, RET_NONE, poly_edge_shallow_left),
    REG_N (0x1f219, R_outline,   3, RET_NONE, poly_outline),
    FAR_C (0x20c07, 0, RET_NONE, clip_polygon),

    /*
     * Display timing. Far, with the argument at [bp+6]; it programs the CRTC
     * directly, which is what the port trap caught it doing.
     */
    FAR_C (0x08f77, 1, RET_NONE, vm_set_display_lines),

    /*
     * The driver's own bring-up. Dispatched because the overlay's routines
     * cannot be bound until the game has recorded where the loader put it -
     * `vm_init` is what writes that pointer to DGROUP 0x48f4, and it calls into
     * the driver *before* it does. So the emulator would reach the driver's
     * mode-set with nothing yet bound, which is what it did.
     */
    FAR_C (0x22483, 3, RET_AX, vm_init),

    /*
     * **The video driver.** Every routine VM.OVL has that the port has too,
     * which is all of them but `vm_blit_scaled` - that one is still a stub,
     * and is deliberately left out so the game reaching it aborts with a
     * backtrace rather than being quietly emulated.
     *
     * Addresses come from the port's own provenance comments rather than a
     * hand-kept list. Reading them with a regex over the whole comment picked
     * a later `VGA:` mentioned in prose for two of them - 0x254 for
     * `vm_span_dithered`, which is really 0x027a - so they are taken from
     * `provenance.check`, which reads only the first line. That is the same
     * trap the annotator hit and the same one that put two wrong values in
     * part_inits.
     */
    OVL_C(0x0000, 3, RET_AX,   vm_driver_init),
    OVL_C(0x011d, 0, RET_NONE, vm_reset_attributes),
    OVL_C(0x0252, 0, RET_NONE, vm_nothing),
    OVL_C(0x027a, 5, RET_NONE, vm_span_dithered),
    OVL_C(0x034f, 5, RET_NONE, vm_span),
    OVL_C(0x03db, 8, RET_NONE, vm_blit_scaled_row),
    OVL_A(0x0938, "wwpwwl", RET_NONE, vm_blit_run),
    OVL_C(0x0998, 4, RET_NONE, vm_draw_line),
    OVL_C(0x0be6, 2, RET_NONE, vm_fill_spans),
    OVL_A(0x0ec1, "pww",    RET_NONE, vm_set_palette),
    OVL_C(0x0f15, 2, RET_NONE, vm_load_palette),
    OVL_C(0x0f57, 4, RET_NONE, vm_blend_palette),
    OVL_C(0x0fd4, 2, RET_DXAX, vm_bitmap_list_size),
    OVL_C(0x1015, 5, RET_NONE, vm_load_bitmap_list),
    OVL_C(0x10b8, 5, RET_NONE, vm_chunky_to_planar),
    OVL_C(0x11bb, 5, RET_NONE, vm_read_four_planes),
    OVL_C(0x11ee, 5, RET_NONE, vm_build_mask_plane),
    OVL_C(0x124b, 6, RET_NONE, vm_blit_glyph),
    OVL_C(0x12fb, 6, RET_NONE, vm_save_rect),
    OVL_C(0x138e, 2, RET_DXAX, vm_buffer_size),
    OVL_C(0x13b9, 6, RET_NONE, vm_restore_rect),
    OVL_C(0x1453, 2, RET_AX,   vm_read_pixel),
    OVL_C(0x14c9, 3, RET_AX,   vm_plot_pixel),
    OVL_C(0x150f, 1, RET_NONE, vm_show_page),
    OVL_C(0x1561, 4, RET_NONE, vm_copy_rect),
    OVL_C(0x15d0, 6, RET_NONE, vm_blit_rows),
    OVL_C(0x1707, 4, RET_NONE, vm_blit_bitmap),

    /*
     * Memory. `dos_alloc_bytes` is what the first useful trap this runner
     * produced pointed at - `game_main` -> `game_startup` -> here, issuing
     * `int 21h ah=48` - and it is the reason the arena never has to be
     * modelled: the io layer answers the allocation instead.
     */
    FAR_C (0x21abd, 4, RET_DXAX, dos_alloc_bytes),
    FAR_C (0x21b34, 2, RET_NONE, dos_free_far),

    /* Near, and the callee removes its own arguments: `ret 4` and `ret 2`. */
    NEAR_P(0x0d584, 2, 4, RET_AX,   dos_creat),
    NEAR_P(0x0d59d, 1, 2, RET_NONE, dos_truncate),
};

int32_t native_count = (int32_t)(sizeof native_table / sizeof native_table[0]);

void native_bind_image(void)
{
    int32_t i;

    for (i = 0; i < native_count; i++)
        if (!native_table[i].overlay)
            native_table[i].linear = IMAGE_BASE + native_table[i].at;
}

/*
 * The video driver's segment, as the game itself records it.
 *
 * `vm_init` stores the far pointer `load_video_driver` gave it at DGROUP
 * 0x48f4, so reading the segment half finds the driver as soon as it is
 * loaded - before it has drawn anything, which a heuristic watching for the
 * first write to A000 cannot manage. tools/verify.py resolves it the same way
 * and for the same reason.
 */
int32_t native_bind_overlay(uc_engine *uc)
{
    uint16_t seg = DGU16(0x48f6);
    int32_t i, n = 0;

    (void)uc;
    if (!seg)
        return 0;
    for (i = 0; i < native_count; i++)
        if (native_table[i].overlay && !native_table[i].linear) {
            native_table[i].linear = (uint32_t)seg * 16 + native_table[i].at;
            n++;
        }
    return n;
}

native_fn *native_lookup(uint32_t linear)
{
    int32_t i;

    for (i = 0; i < native_count; i++)
        if (native_table[i].linear == linear)
            return &native_table[i];
    return NULL;
}

/*
 * Run one, then leave the machine exactly as the routine's own `ret` would
 * have. The arguments are words on the stack above the return address - four
 * bytes up for a far call, two for a near one - which is where the routine
 * itself would have found them.
 */
/*
 * Every argument goes through as a `uintptr_t`. A word, a long and a host
 * pointer all pass unchanged that way - the callee reads whatever width its
 * own prototype declares - so one set of casts covers the lot and there is no
 * per-routine glue to get wrong.
 */
#define A uintptr_t

static uint16_t word_at(uint32_t at)
{
    return (at + 1 < GUEST_MEM_BYTES)
         ? (uint16_t)(guest_mem[at] | (guest_mem[at + 1] << 8)) : 0;
}

void native_call(uc_engine *uc, native_fn *f)
{
    uint16_t ss = 0, sp = 0, cs = 0;
    uint32_t stack;
    uintptr_t a[8];
    uintptr_t result = 0;
    int32_t i;

    uc_reg_read(uc, UC_X86_REG_SS, &ss);
    uc_reg_read(uc, UC_X86_REG_SP, &sp);
    uc_reg_read(uc, UC_X86_REG_CS, &cs);
    stack = (uint32_t)ss * 16 + sp;

    for (i = 0; i < 8; i++)
        a[i] = 0;

    if (f->regs) {
        /* Straight out of the machine, at the routine's own entry - which is
         * where the original would have read them. */
        for (i = 0; i < f->nargs && i < 8; i++) {
            uint16_t v = 0;

            uc_reg_read(uc, f->regs[i], &v);
            a[i] = v;
        }
    } else {
        const char *d = f->args;
        uint32_t at = stack + (f->far_call ? 4 : 2);
        int32_t n = 0;

        while (n < 8 && ((d && *d) || (!d && n < f->nargs))) {
            char kind = d ? *d++ : 'w';
            uint16_t lo = word_at(at), hi = word_at(at + 2);

            if (kind == 'p') {
                /* The guest's memory is the port's, mapped rather than copied,
                 * so this is the whole conversion. */
                a[n++] = (uintptr_t)FAR_PTR(hi, lo);
                at += 4;
            } else if (kind == 'l') {
                a[n++] = (uintptr_t)(((uint32_t)hi << 16) | lo);
                at += 4;
            } else {
                a[n++] = lo;
                at += 2;
            }
        }
        f->nargs = (uint8_t)n;      /* what the call below needs */
    }

    switch (f->nargs) {
    case 0: result = ((uintptr_t (*)(void))f->fn)(); break;
    case 1: result = ((uintptr_t (*)(A))f->fn)(a[0]); break;
    case 2: result = ((uintptr_t (*)(A, A))f->fn)(a[0], a[1]); break;
    case 3: result = ((uintptr_t (*)(A, A, A))f->fn)(a[0], a[1], a[2]); break;
    case 4: result = ((uintptr_t (*)(A, A, A, A))f->fn)
                     (a[0], a[1], a[2], a[3]); break;
    case 5: result = ((uintptr_t (*)(A, A, A, A, A))f->fn)
                     (a[0], a[1], a[2], a[3], a[4]); break;
    case 6: result = ((uintptr_t (*)(A, A, A, A, A, A))f->fn)
                     (a[0], a[1], a[2], a[3], a[4], a[5]); break;
    case 7: result = ((uintptr_t (*)(A, A, A, A, A, A, A))f->fn)
                     (a[0], a[1], a[2], a[3], a[4], a[5], a[6]); break;
    case 8: result = ((uintptr_t (*)(A, A, A, A, A, A, A, A))f->fn)
                     (a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]); break;
    default:
        fprintf(stderr, "native: %s takes %d arguments, which the adapter "
                "does not cover\n", f->name, f->nargs);
        uc_emu_stop(uc);
        return;
    }

    /* The return address the `call` pushed, and then the stack as the routine
     * would have left it: its own return address gone, and its arguments too
     * if it is one of the pascal ones. */
    {
        uint16_t r_off = (uint16_t)(guest_mem[stack] |
                                    (guest_mem[stack + 1] << 8));
        uint16_t r_seg = f->far_call
                       ? (uint16_t)(guest_mem[stack + 2] |
                                    (guest_mem[stack + 3] << 8)) : cs;
        uint16_t new_sp = (uint16_t)(sp + (f->far_call ? 4 : 2) + f->pops);
        uint16_t ax = (uint16_t)result;
        uint16_t dx = (uint16_t)(result >> 16);

        if (f->ret != RET_NONE)
            uc_reg_write(uc, UC_X86_REG_AX, &ax);
        if (f->ret == RET_DXAX)
            uc_reg_write(uc, UC_X86_REG_DX, &dx);

        uc_reg_write(uc, UC_X86_REG_SP, &new_sp);
        uc_reg_write(uc, UC_X86_REG_CS, &r_seg);
        uc_reg_write(uc, UC_X86_REG_IP, &r_off);
    }
    f->hits++;
}
