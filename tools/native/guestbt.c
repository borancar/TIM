/*
 * The guest's call chain, printed from its own stack frames.
 *
 * NOT a transcription. This is the thing the hybrid runner exists to give you:
 * when the game reaches something the port has not implemented, the useful
 * question is never "what address" but "who asked for it", and one frame of
 * that is worth more than a disassembly window.
 *
 * **Why the BP chain rather than a shadow stack.** Tracking every `call` and
 * `ret` needs a hook on every instruction, which costs more than the emulation
 * and is exactly the kind of instrumentation this project has been bitten by
 * before. Borland's large model builds a frame in every routine that takes
 * arguments - `push bp / mov bp,sp` - so the chain is already in memory and
 * costs nothing until something goes wrong. Routines with no frame are simply
 * not in the walk, and the walk says so rather than inventing them.
 *
 * A frame is `[bp] = caller's bp`, `[bp+2] = return offset`, and for a far
 * call `[bp+4] = return segment`. Which of the two it is cannot be known from
 * the frame, so both readings are resolved against the symbol table and the
 * one that lands inside a known routine is printed. Where neither does, both
 * are shown - a wrong guess dressed as a fact is worse than an honest pair.
 */
#include <stdio.h>
#include <string.h>

#include "native.h"
#include "../../reconstruct/dgroup.h"

extern const struct { uint32_t at; const char *name; uint8_t stub; } sym_table[];
extern const int32_t sym_count;

/*
 * How far past a routine's start an address may sit and still be called part
 * of it. The table has 700 entries over a 180 KB image, so the gaps are small;
 * a hit further out than this is in something untranscribed, and saying
 * "unknown" is the honest answer rather than naming whatever came before it.
 */
#define SYM_SPAN 0x2000u

const char *sym_for(uint32_t off, uint32_t *start)
{
    int32_t lo = 0, hi = sym_count - 1, best = -1;

    while (lo <= hi) {
        int32_t mid = (lo + hi) / 2;

        if (sym_table[mid].at <= off) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    if (best < 0 || off - sym_table[best].at > SYM_SPAN)
        return NULL;
    if (start)
        *start = sym_table[best].at;
    return sym_table[best].name;
}

static uint16_t peek(uint32_t linear)
{
    if (linear + 1 >= GUEST_MEM_BYTES)
        return 0;
    return (uint16_t)(guest_mem[linear] | (guest_mem[linear + 1] << 8));
}

/* Print one address both as seg:off and as the image offset a listing uses. */
static void say_where(const char *lead, uint16_t seg, uint16_t off)
{
    uint32_t linear = (uint32_t)seg * 16 + off;
    uint32_t image = linear - IMAGE_BASE;
    uint32_t start = 0;
    const char *name = (linear >= IMAGE_BASE) ? sym_for(image, &start) : NULL;

    if (name)
        fprintf(stderr, "%s%04x:%04x  image %#07x  %s+%#x\n",
                lead, seg, off, image, name, image - start);
    else
        fprintf(stderr, "%s%04x:%04x  image %#07x  (not a transcribed "
                "routine)\n", lead, seg, off, image);
}

void guest_backtrace(uc_engine *uc, const char *why)
{
    uint16_t cs = 0, ip = 0, bp = 0, ss = 0, sp = 0;
    int32_t depth;

    uc_reg_read(uc, UC_X86_REG_CS, &cs);
    uc_reg_read(uc, UC_X86_REG_IP, &ip);
    uc_reg_read(uc, UC_X86_REG_BP, &bp);
    uc_reg_read(uc, UC_X86_REG_SS, &ss);
    uc_reg_read(uc, UC_X86_REG_SP, &sp);

    fprintf(stderr, "\n=== %s\n", why);
    fprintf(stderr, "    sp %04x:%04x  bp %04x\n", ss, sp, bp);
    say_where("  at  ", cs, ip);

    for (depth = 0; depth < 24 && bp; depth++) {
        uint32_t frame = (uint32_t)ss * 16 + bp;
        uint16_t saved = peek(frame);
        uint16_t r_off = peek(frame + 2);
        uint16_t r_seg = peek(frame + 4);
        uint32_t far_lin  = (uint32_t)r_seg * 16 + r_off;
        uint32_t near_lin = (uint32_t)cs * 16 + r_off;
        const char *as_far  = (far_lin  >= IMAGE_BASE)
                            ? sym_for(far_lin  - IMAGE_BASE, NULL) : NULL;
        const char *as_near = (near_lin >= IMAGE_BASE)
                            ? sym_for(near_lin - IMAGE_BASE, NULL) : NULL;

        /* The sentinel native.c puts under `game_main`. Checked before the
         * frame is printed, not after, or the bottom of every walk is a line
         * naming whatever routine 0xffff happens to fall inside. */
        if (r_off == 0xFFFF) {
            fprintf(stderr, "  <-  (entry)\n");
            break;
        }
        if (as_far) {
            say_where("  <-  ", r_seg, r_off);
        } else if (as_near) {
            say_where("  <-  ", cs, r_off);
        } else {
            /* Neither reading lands anywhere known. Show both and let the
             * reader decide, rather than picking one and sounding certain. */
            fprintf(stderr, "  <-  far %04x:%04x or near %04x:%04x  "
                    "(neither is a transcribed routine)\n",
                    r_seg, r_off, cs, r_off);
        }

        if (saved <= bp)          /* frames grow downwards; anything else is
                                   * not a frame and the walk ends here. */
            break;
        bp = saved;
    }
    fprintf(stderr, "\n");
}
