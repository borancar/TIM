/*
 * Dump the machine's part list at a chosen page flip.
 *
 * OURS, and not a transcription. It exists because a screen that differs in a
 * hundred pixels does not say *which part* is wrong, and reasoning backwards
 * from the pixels went astray twice: the records are recycled between the two
 * intro machines - the title's are freed and the credits' built over the same
 * addresses - so an address names one part on one screen and a different part
 * on the other, and any comparison that keys on an address is worthless.
 *
 * So this dumps the list, and `tools/parts.py` dumps the original's at the same
 * flip, and the two are compared as lists in walk order. Set
 *
 *     TIM_PARTS=<flip>:<path>
 *
 * and the port writes one line per part at that flip. The cue is the same one
 * `tools/capture.py` takes its reference frames on - the write to CRTC 0x0C
 * that makes a composed frame visible - so "flip 295" means the same instant on
 * both sides.
 *
 * `TIM_FLIPS=<dir>` writes the composed frame at every flip instead, named by
 * the flip number. That is worth having over the window's own `TIM_FRAMES`,
 * which writes one frame per *refresh*: the port refreshes on a wall clock as
 * well as on the guest's flips, so how many frames a run produces depends on
 * how busy the machine is, the two sides have to be matched by content rather
 * than by number, and a run that ends early looks exactly like a screen that
 * stopped matching. Numbered by flip there is nothing to match - flip N is
 * flip N - and a run that ends early simply has no file for the flips it never
 * reached.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "tim.h"
#include "dgroup.h"

/* The head of the list every part is on, and the one the moving ones are on. */
#define PART_LIST   0x521b
#define MOVING_LIST 0x5179

/* What the game programs the CRTC for, and what a capture holds. */
#define FRAME_W 640
#define FRAME_H 400

extern int32_t dev_tension_belt_calls;

static void dump_chain(FILE *f, const char *name, uint16_t head)
{
    uint16_t si;
    int32_t n = 0;

    for (si = DGU16(head); si != 0 && n < 4096; si = DGU16(si), n++)
        fprintf(f,
                "%s %04x kind %2u form %2u pos %5d,%5d size %4d,%4d "
                "f6 %04x f8 %04x a %04x near %5d,%5d "
                "dir %5d vel %5d,%5d wt %5d mom %04x%04x spin %5d "
                "x62 %04x x66 %04x x78 %04x x84 %04x\n",
                name, si,
                DGU16((uint16_t)(si + 0x04)), DGU16((uint16_t)(si + 0x0c)),
                DG16((uint16_t)(si + 0x1e)), DG16((uint16_t)(si + 0x20)),
                DG16((uint16_t)(si + 0x44)), DG16((uint16_t)(si + 0x46)),
                DGU16((uint16_t)(si + 0x06)), DGU16((uint16_t)(si + 0x08)),
                DGU16((uint16_t)(si + 0x0a)),
                DG16((uint16_t)(si + 0x7a)), DG16((uint16_t)(si + 0x7c)),
                DG16((uint16_t)(si + 0x12)),
                DG16((uint16_t)(si + 0x36)), DG16((uint16_t)(si + 0x38)),
                DG16((uint16_t)(si + 0x3a)),
                DGU16((uint16_t)(si + 0x3e)), DGU16((uint16_t)(si + 0x3c)),
                DG16((uint16_t)(si + 0x9c)),
                DGU16((uint16_t)(si + 0x62)),
                DGU16((uint16_t)(si + 0x66)), DGU16((uint16_t)(si + 0x78)),
                DGU16((uint16_t)(si + 0x84)));
}

/* The composed frame at this flip, as palette indices, if TIM_FLIPS asks. */
static void dump_frame(int32_t flip)
{
    static const char *dir = (const char *)-1;
    char path[512];
    uint8_t *fb;
    FILE *f;

    if (dir == (const char *)-1)
        dir = getenv("TIM_FLIPS");
    if (!dir)
        return;

    fb = malloc((size_t)FRAME_W * FRAME_H);
    if (!fb)
        return;

    vga_compose(fb, FRAME_W, FRAME_H);
    snprintf(path, sizeof path, "%s/flip%04d.raw", dir, flip);

    f = fopen(path, "wb");
    if (f) {
        fwrite(fb, 1, (size_t)FRAME_W * FRAME_H, f);
        fclose(f);
    }
    free(fb);
}

void dev_flip_dump(int32_t flip)
{
    static const char *want = (const char *)-1;
    static int32_t at;
    FILE *f;

    dump_frame(flip);

    if (want == (const char *)-1) {
        const char *s = getenv("TIM_PARTS");
        const char *colon = s ? strchr(s, ':') : NULL;

        want = colon ? colon + 1 : NULL;
        at = colon ? (int32_t)strtol(s, NULL, 0) : -1;
    }

    if (!want || flip != at)
        return;

    f = fopen(want, "w");
    if (!f)
        return;

    /*
     * The call counts of the routines a divergence is usually chased into, so
     * the verifier can be pointed at the *occurrence* that matters instead of
     * a guess. A one-step reproduction is worth little if the check runs on
     * call 20 and the fault is on call 3000.
     */
    fprintf(f, "flip %d origin %d,%d mode %04x tension_belt_calls %d\n", flip,
            DG16(0x4ea3), DG16(0x4ea1), DGU16(0x4e6b),
            dev_tension_belt_calls);
    dump_chain(f, "part", PART_LIST);
    dump_chain(f, "move", MOVING_LIST);
    fclose(f);
    fprintf(stderr, "wrote the part list at flip %d to %s\n", flip, want);
}
