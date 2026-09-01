/*
 * Finding the routines the port stands in for, and standing in for them.
 *
 * NOT a transcription. The list itself is `routines.def`, which is not
 * compiled: `genshims.py` reads it and writes `shims.c`, one typed shim per
 * routine. What is left here is the part that cannot be generated - working
 * out where each one lands once the loader has placed the program and, for the
 * video driver, once the game has loaded VM.OVL.
 */
#include <stdio.h>

#include "native.h"
#include "shim.h"
#include "../../reconstruct/dgroup.h"

/* Where each entry ended up, alongside shim_table. Kept apart from the
 * generated file so regenerating it cannot lose the run's own state. */
static uint32_t bound_at[512];
static uint32_t bound_hits[512];

void native_bind_image(void)
{
    int32_t i;

    for (i = 0; i < shim_count && i < 512; i++)
        if (!shim_table[i].overlay)
            bound_at[i] = IMAGE_BASE + shim_table[i].at;
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
    for (i = 0; i < shim_count && i < 512; i++)
        if (shim_table[i].overlay && !bound_at[i]) {
            bound_at[i] = (uint32_t)seg * 16 + shim_table[i].at;
            n++;
        }
    return n;
}

int32_t native_count_routines(void)
{
    return shim_count;
}

/* The routine registered at this linear address, or -1. */
static int32_t lookup(uint32_t linear)
{
    int32_t i;

    for (i = 0; i < shim_count && i < 512; i++)
        if (bound_at[i] == linear)
            return i;
    return -1;
}

/*
 * Run the port's routine in place of the guest's, and let the shim put the
 * machine back: it knows whether the return is near or far, what a `ret N`
 * removes, and whether the answer goes in AX or DX:AX, because those are
 * expressed in its own C rather than in a table beside it.
 */
int32_t native_dispatch(uc_engine *uc, uint32_t linear)
{
    int32_t i = lookup(linear);
    call_t c;

    if (i < 0)
        return 0;

    c.uc = uc;
    uc_reg_read(uc, UC_X86_REG_SS, &c.ss);
    uc_reg_read(uc, UC_X86_REG_SP, &c.sp);
    uc_reg_read(uc, UC_X86_REG_CS, &c.cs);
    c.stack = (uint32_t)c.ss * 16 + c.sp;
    c.at = c.stack;

    /*
     * The port's own stack pointer, set to the guest's.
     *
     * Several transcribed routines build a structure on the stack and hand its
     * *DGROUP offset* to another - in the large model SS and DS are one
     * segment, so `lea ax,[bp-0x34]` yields an ordinary offset and the callee
     * cannot tell it from a pointer to a global. A C local has no such offset,
     * so the port carries `guest_sp` and `dg_enter` reserves below it.
     *
     * tools/verify.py sets it at every entry. This did not, and `dg_enter`
     * then reserved below whatever was left there - writing a routine's locals
     * over live guest memory. `load_bitmaps` reserves 0xa2 bytes that way and
     * the screens went from identical to 76,817 pixels out the moment it was
     * dispatched. Three routines already dispatched use dg_enter and were
     * matching on luck, not on correctness.
     *
     * The guest's SP is the right value because SS is DGROUP: the offset the
     * port needs is the one the guest is already using.
     */
    guest_sp = c.sp;

    /* A call out of the guest is the clean boundary Shift+F2 waits for; this
     * writes nothing unless the key armed it. */
    native_snapshot_if_armed(uc, shim_table[i].name);

    shim_table[i].shim(&c);
    bound_hits[i]++;
    return 1;
}

void native_report(void)
{
    int32_t i;

    fprintf(stderr, "native: dispatched calls\n");
    for (i = 0; i < shim_count && i < 512; i++)
        if (bound_hits[i])
            fprintf(stderr, "    %-24s %u\n", shim_table[i].name,
                    bound_hits[i]);
}
