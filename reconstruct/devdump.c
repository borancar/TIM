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
 * `TIM_FLIPHASH=<file>` writes **one line per flip**: the flip number and a
 * CRC-32 of the composed frame. That is what a comparison against the original
 * actually needs. A frame is 307200 bytes, the port makes about sixty flips a
 * second, and writing them all out to prove that none of them differ cost five
 * gigabytes of a sixteen-gigabyte /tmp for a result that fits in a few hundred
 * kilobytes. The digest says *which* flips differ; the frames are then worth
 * having for those flips and no others.
 *
 * A CRC-32 misses a difference with probability about 2e-10 per flip, and a
 * real fault differs on many flips at once, so missing all of them is that
 * number raised to a power. It is far below every other uncertainty here.
 *
 * `TIM_FLIPS=<dir>` or `<dir>:<last>` writes whole composed frames, named by
 * the flip number, stopping after `<last>` if one is given. **For the handful
 * of flips a side-by-side needs**, not for a run: use `<last>` or the digests
 * will have cost less than the frames.
 *
 * `TIM_FLIPCOUNT=<file>` rewrites one small file with the current flip number
 * and writes no frames at all. That is what a liveness watch wants: whether
 * the count is still rising says everything, and six gigabytes of pixels were
 * once written to answer it. That is worth having over the window's own `TIM_FRAMES`,
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
#define FRAME_H 480

extern int32_t dev_tension_belt_calls;
extern int32_t dev_queue_part_calls;

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

/*
 * The composed frame at this flip, if TIM_FLIPS asks - in the same `TIMSCRN1`
 * container `tools/capture.py` writes for the original, so a port frame and a
 * reference frame are the same kind of file and every tool reads both.
 *
 * It used to be bare indices, and the palette had to be supplied by whoever
 * rendered them. Rendering a port frame through a *guessed* EGA palette made
 * correctly drawn text look like garbage and sent a diagnosis off after a fault
 * that was not there; the frame carries its own colours now, so that cannot
 * happen again. Indices are still what gets compared - a right frame under a
 * black palette is a palette fault, not a drawing one - but the colours travel
 * with them.
 */
static void note_flip(int32_t flip)
{
    static const char *path = (const char *)-1;
    FILE *f;

    if (path == (const char *)-1)
        path = getenv("TIM_FLIPCOUNT");
    if (!path)
        return;

    f = fopen(path, "w");
    if (f) {
        fprintf(f, "%d\n", flip);
        fclose(f);
    }
}

/*
 * CRC-32, the ordinary IEEE one, so `zlib.crc32` on the other side agrees
 * without either of us needing a library. The table is built once on first use.
 */
static uint32_t crc32_of(const uint8_t *p, size_t n)
{
    static uint32_t table[256];
    static int32_t  built;
    uint32_t        crc = 0xFFFFFFFFu;
    size_t          i;

    if (!built) {
        for (i = 0; i < 256; i++) {
            uint32_t c = (uint32_t)i;
            int32_t  k;

            for (k = 0; k < 8; k++)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        built = 1;
    }

    for (i = 0; i < n; i++)
        crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFFu;
}

/* One line per flip: the number and a digest of the frame. No pixels. */
static void hash_frame(int32_t flip)
{
    static const char *path = (const char *)-1;
    static FILE *out;
    uint8_t *fb;

    if (path == (const char *)-1) {
        path = getenv("TIM_FLIPHASH");
        if (path)
            out = fopen(path, "w");
    }
    if (!out)
        return;

    fb = malloc((size_t)FRAME_W * FRAME_H);
    if (!fb)
        return;

    vga_compose(fb, FRAME_W, FRAME_H);
    fprintf(out, "%d %08x\n", flip,
            crc32_of(fb, (size_t)FRAME_W * FRAME_H));
    fflush(out);
    free(fb);
}

static void dump_frame(int32_t flip)
{
    static char dir[480];
    static int32_t last = -2;           /* -2 unread, -1 no limit */
    char path[512];
    uint8_t *fb;
    FILE *f;

    if (last == -2) {
        const char *spec = getenv("TIM_FLIPS");
        const char *colon = spec ? strrchr(spec, ':') : NULL;

        last = -1;
        dir[0] = 0;
        if (spec) {
            snprintf(dir, sizeof dir, "%s", spec);
            if (colon) {
                dir[colon - spec] = 0;
                last = (int32_t)strtol(colon + 1, NULL, 0);
            }
        }
    }

    if (!dir[0] || (last >= 0 && flip > last))
        return;

    fb = malloc((size_t)FRAME_W * FRAME_H);
    if (!fb)
        return;

    vga_compose(fb, FRAME_W, FRAME_H);
    snprintf(path, sizeof path, "%s/flip%04d.scrn", dir, flip);

    f = fopen(path, "wb");
    if (f) {
        uint8_t  pal[768];
        uint16_t head[3];

        vga_palette_rgb(pal);
        head[0] = (uint16_t)FRAME_W;
        head[1] = (uint16_t)FRAME_H;
        head[2] = (uint16_t)(vga_visible_lines() - 1);

        fwrite("TIMSCRN1", 1, 8, f);
        fwrite(head, 2, 3, f);
        fwrite(pal, 1, sizeof pal, f);
        fwrite(fb, 1, (size_t)FRAME_W * FRAME_H, f);
        fclose(f);
    }
    free(fb);
}

/*
 * `TIM_CLICK=<flip>:<x>:<y>` presses the left button at that flip and lets it
 * go two flips later.
 *
 * Ours, and the reason it exists is that the game past the intro is behind a
 * pointer: the intro runs to a click, the menu is clicks, and none of it can
 * be reached by a tool that has no hands. A flip number is the one clock both
 * sides of this project already agree on, so a click placed at a flip happens
 * at the same point of the program every run - which a click placed at a
 * wall-clock moment would not.
 *
 * Two flips of hold because the game samples the button once a frame and
 * `update_button_state` needs to see it down and then up to call it a click.
 */
static void dev_click(int32_t flip)
{
    static int32_t at = -2, x, y;

    if (at == -2) {
        const char *spec = getenv("TIM_CLICK");

        at = -1;
        if (spec)
            sscanf(spec, "%d:%d:%d", &at, &x, &y);
    }

    if (at < 0)
        return;

    if (flip == at)
        io_mouse_input(x, y, 1);
    else if (flip == at + 2)
        io_mouse_input(x, y, 0);
}

void dev_flip_dump(int32_t flip)
{
    dev_click(flip);

    static const char *want = (const char *)-1;
    static int32_t at;
    FILE *f;

    note_flip(flip);
    hash_frame(flip);
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
    fprintf(f, "flip %d origin %d,%d mode %04x tension_belt_calls %d "
            "queue_part_calls %d\n", flip,
            DG16(0x4ea3), DG16(0x4ea1), DGU16(0x4e6b),
            dev_tension_belt_calls, dev_queue_part_calls);
    dump_chain(f, "part", PART_LIST);
    dump_chain(f, "move", MOVING_LIST);
    fclose(f);
    fprintf(stderr, "wrote the part list at flip %d to %s\n", flip, want);
}
