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

/* The VGA driver, by offset into VM.OVL. See gensyms.py. */
extern const struct { uint32_t at; const char *name; uint8_t stub; }
    ovl_sym_table[];
extern const int32_t ovl_sym_count;

/*
 * How far past a routine's start an address may sit and still be called part
 * of it. The table has 700 entries over a 180 KB image, so the gaps are small;
 * a hit further out than this is in something untranscribed, and saying
 * "unknown" is the honest answer rather than naming whatever came before it.
 */
#define SYM_SPAN 0x2000u

/*
 * The VGA driver's base, as the loader placed it. `native_bind_overlay` reads
 * the same word; this reads it again rather than caching it, because a
 * backtrace can be printed before the overlay is loaded and a cached zero
 * would name driver routines at addresses that are not the driver's.
 */
static uint32_t overlay_base(void)
{
    uint16_t seg = DGU16(0x48f6);

    return seg ? (uint32_t)seg * 16 : 0;
}

/*
 * A driver address, if this is one. The trap that matters most fires inside
 * the driver - it is what touches A000 and the VGA ports - and the image table
 * cannot hold it, so every such frame used to print "not a transcribed
 * routine" and the walk looked broken where it was working.
 */
static const char *sym_for_overlay(uint32_t linear, uint32_t *start)
{
    uint32_t base = overlay_base(), off;
    int32_t lo = 0, hi = ovl_sym_count - 1, best = -1;

    if (!base || linear < base)
        return NULL;
    off = linear - base;
    if (off > 0xffffu)                  /* past any one overlay */
        return NULL;

    while (lo <= hi) {
        int32_t mid = (lo + hi) / 2;

        if (ovl_sym_table[mid].at <= off) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    if (best < 0 || off - ovl_sym_table[best].at > SYM_SPAN)
        return NULL;
    if (start)
        *start = off - ovl_sym_table[best].at;
    return ovl_sym_table[best].name;
}

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
    uint32_t start = 0, into = 0;
    const char *name = (linear >= IMAGE_BASE) ? sym_for(image, &start) : NULL;
    const char *ovl;

    if (name) {
        fprintf(stderr, "%s%04x:%04x  image %#07x  %s+%#x\n",
                lead, seg, off, image, name, image - start);
        return;
    }

    /* The driver, which is not in the image and is where a trap usually is. */
    ovl = sym_for_overlay(linear, &into);
    if (ovl) {
        fprintf(stderr, "%s%04x:%04x  VM.OVL VGA:%#06x  %s+%#x\n",
                lead, seg, off, (linear - overlay_base()), ovl, into);
        return;
    }

    fprintf(stderr, "%s%04x:%04x  image %#07x  (not a transcribed "
            "routine)\n", lead, seg, off, image);
}

void guest_backtrace(uc_engine *uc, const char *why)
{
    uint16_t cs = 0, ip = 0, bp = 0, ss = 0, sp = 0;
    int32_t depth, walked = 0;

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
            walked = 1;
            say_where("  <-  ", r_seg, r_off);
        } else if (as_near) {
            walked = 1;
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
    /*
     * A leaf with no frame has no chain, and the walk above then says nothing
     * useful - which is exactly the case a divide error lands in. So when it
     * produced nothing, read the raw stack and report the words that *could*
     * be return addresses.
     *
     * This is a **scan, not a chain**: it cannot tell a return address from a
     * far pointer that happens to be stored on the stack, and it is labelled
     * that way. Candidates are still worth far more than silence, because the
     * one that matters is nearly always among the first few.
     */
    if (!walked) {
        int32_t shown = 0;
        uint16_t at;

        fprintf(stderr, "  no frame to walk - scanning the stack for words "
                "that could be return addresses:\n");
        for (at = sp; at < 0xFFF0 && shown < 8; at += 2) {
            uint32_t here = (uint32_t)ss * 16 + at;
            uint16_t off = peek(here);
            uint16_t seg = peek(here + 2);
            uint32_t lin = (uint32_t)seg * 16 + off;

            if (lin < IMAGE_BASE || !sym_for(lin - IMAGE_BASE, NULL))
                continue;
            say_where("  ?   ", seg, off);
            shown++;
        }
        if (!shown)
            fprintf(stderr, "  ?   nothing on the stack resolves to a "
                    "transcribed routine\n");
    }

    fprintf(stderr, "\n");
}
