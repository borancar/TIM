/*
 * **Which of a routine's locals another routine reaches.**
 *
 * NOT a transcription. This exists because of a question the port cannot
 * answer from its own sources: a routine hands another the address of one of
 * its `[bp-N]` locals, and nothing static says how far the callee reads from
 * there. Get that wrong and a local that looks private is really element 2 of
 * an array whose base is a different local - which is exactly how
 * `draw_part_extra` lost two polygon corners.
 *
 * The measurement is only possible here. In the large model SS *is* DGROUP -
 * measured, 0x2e4c in every one of 40M sampled instructions, in every CS
 * including the overlays - so a near pointer to a local looks like a near
 * pointer to a global and the C cannot tell them apart. The emulator can: it
 * sees the address, and Borland's `push bp / mov bp,sp` leaves the frame chain
 * in memory to attribute it to.
 *
 * The rule for one access:
 *
 *   walk the BP chain outwards and find the innermost frame whose BP is above
 *   the address. Frame 0 is the routine doing the reading, so a hit there is
 *   its own local and is not interesting. A hit in frame k > 0 is a routine
 *   reading into a *caller's* frame, and `BP(k) - address` is which local.
 *
 * The routine owning frame k is named by the return address in frame k-1,
 * resolved the way `guest_backtrace` resolves one: the far reading and the
 * near reading are both tried against the symbol table and whichever lands in
 * a known routine wins. Where neither does the record still goes out, with the
 * raw addresses, because a run that names nothing is still evidence about how
 * deep the reach was.
 *
 * **Writes only, and it is the emulator that decides that.** `guest_mem` is
 * given to Unicorn with `uc_mem_map_ptr`, and over such a region a
 * `UC_HOOK_MEM_READ` that lets execution continue breaks the guest - with a
 * callback whose entire body is `return`, the game leaves the rails inside
 * twelve frames and the backtrace shows `game_main+0x8` calling
 * `part_flip_options`, which is not a call that exists.
 * `UC_HOOK_MEM_READ_AFTER` does the same; the identical write hook runs clean.
 * The one read hook already in `native.c`, `on_vga_access`, escapes it by
 * calling `uc_emu_stop` and never returning to running code.
 *
 * **And it sees only the code the emulator still runs.** A routine dispatched
 * to the port writes `guest_mem` from C and never goes near Unicorn, so its
 * accesses are invisible here. That is the opposite of a detail: the reaches
 * this was built to find are mostly in dispatched code - `draw_polygon`
 * walking three points from an address `draw_part_extra` handed it - and none
 * of those appear. What does appear is the still-emulated side, which is the
 * part no static reading of the port's sources can cover at all. Taking the
 * other half needs the same bookkeeping inside the port's own accessors, where
 * `dg_enter` already knows the frame.
 *
 * A routine whose prologue omits `push bp / mov bp,sp` has no frame and is not
 * in the chain, so a hit from inside it is attributed to whoever called it.
 * Borland builds a frame in everything that takes arguments, which is
 * everything that could have been handed an address, so this bites rarely -
 * but it is why the output says how deep a hit was rather than only who made
 * it.
 *
 * **Arguments look like a caller's locals and are not.** The four or six bytes
 * of linkage above a frame's BP are followed by the arguments the caller
 * pushed, and only above those do the caller's own locals begin. Nothing here
 * knows where the arguments end, so a hit close above BP is probably an
 * argument fetch. The distance is recorded and `slots.py` draws the line,
 * where it can be drawn against the port's own `dg_enter` sizes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "../../reconstruct/dgroup.h"


static FILE    *g_out;
static uint32_t g_seen;

/*
 * Every (owner, local, reader) triple already written, in an open-addressed
 * table. A run makes millions of stack accesses and almost all of them repeat,
 * so without a dedupe the log is gigabytes of the same handful of facts - and
 * a *linear* dedupe is worse than none, because it turns a hook that fires on
 * every stack access into an O(n^2) scan. This hook is on the hot path of the
 * emulator and its cost is visible in how far the run gets.
 */
#define TRIP_SLOTS (1u << 16)
struct trip { uint32_t owner, reader; uint16_t off, depth; uint8_t used; };
static struct trip g_tab[TRIP_SLOTS];

static int32_t already(uint32_t owner, uint32_t reader, uint16_t off,
                       uint16_t depth)
{
    uint32_t h = owner * 2654435761u ^ reader * 40503u ^ (off << 3) ^ depth;
    uint32_t i;

    h &= TRIP_SLOTS - 1;
    for (i = 0; i < TRIP_SLOTS; i++) {
        struct trip *t = &g_tab[(h + i) & (TRIP_SLOTS - 1)];

        if (!t->used) {
            t->owner = owner; t->reader = reader;
            t->off = off; t->depth = depth; t->used = 1;
            return 0;
        }
        if (t->owner == owner && t->reader == reader
            && t->off == off && t->depth == depth)
            return 1;
    }
    return 1;                          /* full: stop recording, do not grow */
}

static uint16_t peek16(uint32_t linear)
{
    return (uint16_t)(guest_mem[linear] | (guest_mem[linear + 1] << 8));
}

/* The image offset a return address means, taking whichever reading lands. */
static uint32_t owner_of(uint16_t cs, uint16_t r_off, uint16_t r_seg)
{
    uint32_t far_lin  = (uint32_t)r_seg * 16 + r_off;
    uint32_t near_lin = (uint32_t)cs * 16 + r_off;

    if (far_lin >= IMAGE_BASE && sym_for(far_lin - IMAGE_BASE, NULL))
        return far_lin;
    if (near_lin >= IMAGE_BASE && sym_for(near_lin - IMAGE_BASE, NULL))
        return near_lin;
    return near_lin;                   /* unnamed; the address is still a fact */
}

void native_slots_open(const char *path)
{
    g_out = fopen(path, "w");
    if (g_out == NULL) {
        fprintf(stderr, "TIM_SLOTS: cannot write %s\n", path);
        return;
    }
    fprintf(g_out, "# owner reader local depth kind\n");
}

void native_slots_access(uc_engine *uc, uint32_t type, uint64_t address,
                         int32_t size, int64_t value, void *ud)
{
    uint16_t cs = 0, ip = 0, bp = 0, ss = 0;
    uint32_t base;
    uint16_t frames[24];
    int32_t n = 0, k;
    uint16_t off, owner_bp;
    uint32_t owner, reader;

    (void)size; (void)value; (void)ud;
    if (g_out == NULL)
        return;

    uc_reg_read(uc, UC_X86_REG_SS, &ss);
    base = (uint32_t)ss * 16;
    if (address < base || address >= base + 0x10000)
        return;
    off = (uint16_t)(address - base);

    uc_reg_read(uc, UC_X86_REG_BP, &bp);
    if (bp == 0)
        return;

    /*
     * **The fast path, and it is most of them.** An address below the current
     * BP is this routine's own local, which is the ordinary case and the one
     * this tool has nothing to say about. Deciding it costs two register reads
     * and a compare; only the minority at or above BP - arguments, and reaches
     * into a caller - pays for the chain walk below.
     */
    if (off < bp)
        return;

    /* The chain, outwards. It must climb; anything else is not a frame. */
    {
        uint16_t b = bp;

        while (n < 24) {
            uint16_t next;

            frames[n++] = b;
            next = peek16(base + b);
            if (next <= b)
                break;
            b = next;
        }
    }

    for (k = 0; k < n; k++)
        if (frames[k] > off)
            break;
    if (k <= 0 || k >= n)
        return;                        /* its own local, or off the chain */

    owner_bp = frames[k];
    uc_reg_read(uc, UC_X86_REG_CS, &cs);
    uc_reg_read(uc, UC_X86_REG_IP, &ip);
    owner = owner_of(cs, peek16(base + frames[k - 1] + 2),
                     peek16(base + frames[k - 1] + 4));
    reader = (uint32_t)cs * 16 + ip;

    if (already(owner, reader, (uint16_t)(owner_bp - off), (uint16_t)k))
        return;

    fprintf(g_out, "%07x %07x %u %d %c\n", owner, reader,
            (unsigned)(owner_bp - off), k,
            type == UC_MEM_WRITE ? 'w' : 'r');
    /* flushed as it goes: the run this most wants to watch is one that ends in
       a trap, and a buffered tail is exactly what such a run loses */
    fflush(g_out);
    g_seen++;
}

void native_slots_close(void)
{
    if (g_out == NULL)
        return;
    fclose(g_out);
    fprintf(stderr, "TIM_SLOTS: %u distinct (owner, reader, local) triples\n",
            g_seen);
    g_out = NULL;
}
