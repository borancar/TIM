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

static void dump_chain(FILE *f, const char *name, uint16_t head)
{
    uint16_t si;
    int32_t n = 0;

    for (si = DGU16(head); si != 0 && n < 4096; si = DGU16(si), n++)
        fprintf(f,
                "%s %04x kind %2u form %2u pos %5d,%5d size %4d,%4d "
                "f6 %04x f8 %04x a %04x x62 %04x x66 %04x x78 %04x x84 %04x\n",
                name, si,
                DGU16((uint16_t)(si + 0x04)), DGU16((uint16_t)(si + 0x0c)),
                DG16((uint16_t)(si + 0x1e)), DG16((uint16_t)(si + 0x20)),
                DG16((uint16_t)(si + 0x44)), DG16((uint16_t)(si + 0x46)),
                DGU16((uint16_t)(si + 0x06)), DGU16((uint16_t)(si + 0x08)),
                DGU16((uint16_t)(si + 0x0a)), DGU16((uint16_t)(si + 0x62)),
                DGU16((uint16_t)(si + 0x66)), DGU16((uint16_t)(si + 0x78)),
                DGU16((uint16_t)(si + 0x84)));
}

void dev_flip_dump(int32_t flip)
{
    static const char *want = (const char *)-1;
    static int32_t at;
    FILE *f;

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

    fprintf(f, "flip %d origin %d,%d mode %04x\n", flip,
            DG16(0x4ea3), DG16(0x4ea1), DGU16(0x4e6b));
    dump_chain(f, "part", PART_LIST);
    dump_chain(f, "move", MOVING_LIST);
    fclose(f);
    fprintf(stderr, "wrote the part list at flip %d to %s\n", flip, want);
}
