/*
 * Finding the routines the port stands in for, and standing in for them.
 *
 * NOT a transcription. The list itself is `routines.def`, which is not
 * compiled: `genshims.py` reads it and writes `shims.c`, one typed shim per
 * routine. What is left here is the part that cannot be generated - working
 * out where each one lands once the loader has placed the program and, for the
 * video driver, once the game has loaded VM.OVL.
 */
#define TIM_HOST 1        /* a host unit: the host's <stdio.h> and its FILE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "shim.h"
#include "../../reconstruct/dgroup.h"
#include "../../reconstruct/tim.h"
#include "../../reconstruct/io.h"

/* Where each entry ended up, alongside shim_table. Kept apart from the
 * generated file so regenerating it cannot lose the run's own state. */
static uint32_t bound_at[512];
/* Cleared whenever a binding changes; see `lookup` for why there is a hash. */
static int32_t hash_ready;
static uint32_t bound_hits[512];

/*
 * `TIM_NATIVE_LAYERS` - which of the port's layers stand in for the guest's.
 *
 * NOT a transcription. The hybrid's usual question is "does the port's version
 * of this routine agree with the original's", and the answer has always been
 * measured with *everything* the port has dispatched at once. That conflates
 * two things: whether the port's hardware and memory are faithful, and whether
 * its game logic is. Selecting layers separates them.
 *
 *   TIM_NATIVE_LAYERS=io      the port supplies VM.OVL, SX.OVL and the
 *                             Borland runtime; the game's own code is the
 *                             original's, executed
 *   TIM_NATIVE_LAYERS=vm,mem  any comma-separated subset of vm sx dos mem game
 *   unset, or `all`           what the hybrid has always done
 *
 * `io` is exactly "everything that is not the game", which is the split this
 * was asked for: the port as the machine, the original as the program.
 *
 * A layer that is not selected is not bound, so the guest executes the
 * original's bytes there - the emulator's traps still fire for anything the
 * port has no body for at all, which is the other reason to want this: a run
 * with `game` deselected names the IO the game reaches without the port's own
 * game code standing in the way.
 */
static int32_t layer_wanted(const char *layer)
{
    static const char *want;
    const char *p;
    size_t n = strlen(layer);

    if (want == NULL) {
        want = getenv("TIM_NATIVE_LAYERS");
        if (want == NULL || *want == 0)
            want = "all";
    }

    if (strcmp(want, "all") == 0)
        return 1;

    /* `io` is the four that are not the game. */
    if (strcmp(want, "io") == 0)
        return strcmp(layer, "game") != 0;

    for (p = want; *p; ) {
        size_t len = strcspn(p, ",");

        if (len == n && strncmp(p, layer, n) == 0)
            return 1;
        p += len;
        if (*p == ',')
            p++;
    }
    return 0;
}

/*
 * Which entries this run will stand in for. Worked out once and printed, so a
 * run says what it was rather than leaving it to be inferred from what traps.
 */
static int32_t selected[512];

static void select_layers(void)
{
    static int32_t done;
    int32_t i, n = 0;

    if (done)
        return;
    done = 1;

    for (i = 0; i < shim_count && i < 512; i++) {
        selected[i] = layer_wanted(shim_table[i].layer);
        n += selected[i];
    }

    if (getenv("TIM_NATIVE_LAYERS") != NULL)
        fprintf(stderr, "native: layers %s - %d of %d routines dispatched\n",
                getenv("TIM_NATIVE_LAYERS"), n, shim_count);
}

void native_bind_image(void)
{
    int32_t i;

    select_layers();
    for (i = 0; i < shim_count && i < 512; i++)
        if (!shim_table[i].overlay && selected[i])
            bound_at[i] = IMAGE_BASE + shim_table[i].at;
    hash_ready = 0;
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
    select_layers();
    for (i = 0; i < shim_count && i < 512; i++)
        if (shim_table[i].overlay && !bound_at[i] && selected[i]) {
            bound_at[i] = (uint32_t)seg * 16 + shim_table[i].at;
            n++;
        }
    if (n)
        hash_ready = 0;
    return n;
}

/*
 * The **sound driver**, SX.OVL, at the one address every call to it goes to.
 *
 * NOT a transcription, and hand-written rather than generated because its
 * shape is outside what `routines.def` can say. Every other dispatched routine
 * takes its arguments off a stack frame or out of named registers and answers
 * in AX or DX:AX. This one is entered with a *function number* in BP and
 * answers in AX **and** CX, which the generator has no vocabulary for - one
 * routine is not worth a fifth calling convention in it.
 *
 * The address is not a constant and is not in the image. `install_driver`
 * stores the far pointer the loader gave it - `SND16(0x1e7)` the offset and
 * `SND16(0x1e9)` the segment - and the fifty call sites in the sound module
 * are all `push bp / mov bp,<n> / lcall cs:[0x1e7]`. So binding that pointer
 * puts the port's `sx_driver_call` in front of the whole driver at one place,
 * whichever of the nine devices the loader chose.
 *
 * `bp` is read straight from the guest rather than through `areg`, because it
 * is not an argument in the frame sense: the call site sets it and pops it
 * back afterwards, so it is the selector rather than a parameter.
 */
static uint32_t sound_bound;
static uint32_t sound_hits[18];

int32_t native_bind_sound(uc_engine *uc)
{
    uint16_t seg = (uint16_t)SND16(0x1e9);
    uint16_t off = (uint16_t)SND16(0x1e7);

    (void)uc;
    if (sound_bound || !layer_wanted("sx") || (seg == 0 && off == 0))
        return 0;

    sound_bound = (uint32_t)seg * 16 + off;
    return 1;
}

static void sh_sx_driver_call(call_t *c)
{
    uint16_t bp = 0, ax = 0, cx = 0, es = 0;

    uc_reg_read(c->uc, UC_X86_REG_BP, &bp);
    uc_reg_read(c->uc, UC_X86_REG_AX, &ax);
    uc_reg_read(c->uc, UC_X86_REG_CX, &cx);
    uc_reg_read(c->uc, UC_X86_REG_ES, &es);

    if (bp < 18)
        sound_hits[bp]++;

    sx_driver_call(bp, &ax, &cx, es);

    uc_reg_write(c->uc, UC_X86_REG_CX, &cx);
    /* `rf_ax` puts AX back and returns far, which is what the driver's own
     * `retf` does. CX is written first because that call moves CS:IP. */
    rf_ax(c, ax, 0);
}

int32_t native_count_routines(void)
{
    return shim_count;
}

/*
 * The routine registered at this linear address, or -1.
 *
 * **This is the hybrid's hottest path and it is not obvious why.** The lookup
 * is not per dispatched call - it is per *basic block*: `on_block` asks the
 * question about every block the guest executes, because that is how a call
 * into a dispatched routine is noticed at all. A linear scan of the table is
 * therefore 229 comparisons times every block of every frame, and a sampling
 * profile put 46% of the runner's leaf time inside this function, ahead of
 * everything in Unicorn.
 *
 * So it is a hash, sized to keep the load under a quarter. The common answer
 * is "no", and an open-addressed miss stops at the first empty slot - about
 * 1.3 probes at this load, against 229. Re-profiled afterwards: 46% down to
 * 3%.
 *
 * **And it makes the runner no faster, which is the more useful finding.**
 * A fixed 2000-frame run took 26.54s before and 26.54s after, with the same
 * 22s of CPU - because 63.2 million of the run's 63.2 million dispatched calls
 * are `frame_pending`, the guest's spin waiting for the timer. The freed time
 * goes into more turns of that spin: 37M loop iterations became 63M, and the
 * game reached the same frame at the same moment. The hybrid is bound by the
 * frame and tick pacing, not by how fast anything here runs, which is the
 * entanglement CLAUDE.md already describes.
 *
 * Kept anyway: it is a real removal of quadratic work, it makes the profile
 * mean something, and it is what would matter if the pacing were ever untied
 * from the wall clock. But nobody should expect a run to finish sooner.
 *
 * The obvious alternative was measured and is worse. Withdrawing
 * `frame_pending` from `routines.def`, so the guest runs its own four
 * instructions inside the slice instead of paying a stop and restart, took the
 * same run from 26.5s to 31.0s and from 22s of CPU to 28.6s. Emulating the
 * spin costs more than dispatching it.
 *
 * A zero in `bound_at` means unbound, and cannot collide with a real address:
 * every binding is `IMAGE_BASE + offset` or `segment * 16 + offset`, and
 * neither base is zero.
 */
#define HASH_BITS 10
#define HASH_SIZE (1u << HASH_BITS)

static int16_t hash_slot[HASH_SIZE];    /* index + 1; 0 is empty */

static uint32_t hash_of(uint32_t linear)
{
    return (linear * 2654435761u) >> (32 - HASH_BITS);
}

static void hash_rebuild(void)
{
    int32_t i;

    memset(hash_slot, 0, sizeof hash_slot);
    for (i = 0; i < shim_count && i < 512; i++) {
        uint32_t h;

        if (!bound_at[i])
            continue;
        for (h = hash_of(bound_at[i]); hash_slot[h];
             h = (h + 1) & (HASH_SIZE - 1))
            ;
        hash_slot[h] = (int16_t)(i + 1);
    }
    hash_ready = 1;
}

static int32_t lookup(uint32_t linear)
{
    uint32_t h;

    if (!hash_ready)
        hash_rebuild();

    for (h = hash_of(linear); hash_slot[h]; h = (h + 1) & (HASH_SIZE - 1)) {
        int32_t i = hash_slot[h] - 1;

        if (bound_at[i] == linear)
            return i;
    }
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

    if (i < 0 && !(sound_bound && linear == sound_bound))
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
     * so the port carries `guest_sp` and `dg_alloca` reserves below it.
     *
     * tools/verify.py sets it at every entry. This did not, and `dg_alloca`
     * then reserved below whatever was left there - writing a routine's locals
     * over live guest memory. `load_bitmaps` reserves 0xa2 bytes that way and
     * the screens went from identical to 76,817 pixels out the moment it was
     * dispatched. Three routines already dispatched use dg_alloca and were
     * matching on luck, not on correctness.
     *
     * The guest's SP is the right value because SS is DGROUP: the offset the
     * port needs is the one the guest is already using.
     */
    guest_sp = c.sp;

    if (i < 0) {
        native_snapshot_if_armed(uc, "sx_driver_call");
        sh_sx_driver_call(&c);
        return 1;
    }

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

    /*
     * **Printed whether or not the driver was dispatched, and that is the
     * point.** It is the one quantity that compares the port's SX.OVL against
     * the original's: run the same frames with `sx` selected and deselected,
     * and the key-ons are what the two drivers did to the same card. The port
     * supplies the OPL either way, so this counts the same thing on both
     * sides - which is what the CLAUDE.md note about wall-clock sound
     * comparisons says to look for.
     */
    fprintf(stderr, "native: %ld OPL key-ons\n", io_keyon_count());
    fprintf(stderr, "native: dispatched calls\n");
    for (i = 0; i < shim_count && i < 512; i++)
        if (bound_hits[i])
            fprintf(stderr, "    %-24s %u\n", shim_table[i].name,
                    bound_hits[i]);

    /*
     * The driver is counted per *function*, not as one number. It is bound at
     * a single address, so a single total would say only "the hook fired" -
     * and which of the eighteen the game actually asks for is the thing worth
     * knowing, and the thing the port's own dispatcher can get wrong one case
     * at a time.
     */
    if (sound_bound) {
        int32_t fn, any = 0;

        for (fn = 0; fn < 18; fn++)
            any += (sound_hits[fn] != 0);
        fprintf(stderr, "native: SX.OVL bound at %05x, %d of 18 functions "
                "reached\n", sound_bound, any);
        for (fn = 0; fn < 18; fn++)
            if (sound_hits[fn])
                fprintf(stderr, "    sx function %-13d %u\n", fn,
                        sound_hits[fn]);
    }
}
