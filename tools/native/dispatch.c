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

#define FAR_C(a, n, r, f)  { (a), #f, (void *)(f), 0, 1, (n), 0, (r), 0, 0 }
#define NEAR_P(a, n, p, r, f) { (a), #f, (void *)(f), 0, 0, (n), (p), (r), 0, 0 }

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
    { 0x225d2, "detect_adapter", (void *)detect_adapter, 0, 0, 0, 0,
      RET_AX, 0, 0 },   /* near: it ends `ret`, unlike most of this file */

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
void native_call(uc_engine *uc, native_fn *f)
{
    uint16_t ss = 0, sp = 0, cs = 0;
    uint32_t stack;
    uint16_t a[8];
    uint32_t result = 0;
    int32_t i;

    uc_reg_read(uc, UC_X86_REG_SS, &ss);
    uc_reg_read(uc, UC_X86_REG_SP, &sp);
    uc_reg_read(uc, UC_X86_REG_CS, &cs);
    stack = (uint32_t)ss * 16 + sp;

    for (i = 0; i < 8; i++) {
        uint32_t at = stack + (f->far_call ? 4 : 2) + 2 * i;

        a[i] = (i < f->nargs && at + 1 < GUEST_MEM_BYTES)
             ? (uint16_t)(guest_mem[at] | (guest_mem[at + 1] << 8)) : 0;
    }

    switch (f->nargs) {
    case 0: result = ((uint32_t (*)(void))f->fn)(); break;
    case 1: result = ((uint32_t (*)(uint16_t))f->fn)(a[0]); break;
    case 2: result = ((uint32_t (*)(uint16_t, uint16_t))f->fn)(a[0], a[1]);
            break;
    case 3: result = ((uint32_t (*)(uint16_t, uint16_t, uint16_t))f->fn)
                     (a[0], a[1], a[2]); break;
    case 4: result = ((uint32_t (*)(uint16_t, uint16_t, uint16_t,
                                    uint16_t))f->fn)(a[0], a[1], a[2], a[3]);
            break;
    default:
        fprintf(stderr, "native: %s takes %d words, which the adapter does "
                "not cover\n", f->name, f->nargs);
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
