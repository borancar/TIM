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
 * the flip number, stopping after `<last>` if one is given.
 *
 * **`<last>` is a stopping point, not a filter.** `TIM_FLIPS=out:800` writes
 * eight hundred frames and then stops, which at 308 KB each is a quarter of a
 * gigabyte - and reading it as "write flip 800" is how several gigabytes of
 * pixels have now been written twice to answer questions about three frames.
 *
 * **`TIM_FLIPWANT=<f1>,<f2>,...` is the filter**, and is what a side-by-side
 * should use: only those flips are written, and the rest are composed for
 * nothing or not at all. It pairs with `<last>` - the stop still ends the run -
 * so `TIM_FLIPS=out:790 TIM_FLIPWANT=740,760,790` writes three files and
 * stops, which is what every comparison in `tools/` actually needs.
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
/*
 * `TIM_SNAPAT=<flip>` - write the port's whole state at that page flip.
 *
 * The same file Shift+F2 writes in a window, taken from the clock instead of
 * the key, and it exists for the same reason the runner's does: a capture that
 * can only be made by pressing a key cannot be made by a check, and a feature
 * no check exercises does not stay correct. `TIM_SNAP` moves the path for
 * both.
 */
static void snapshot_at(int32_t flip)
{
    static int32_t at = -2;
    char path[512];

    if (at == -2) {
        const char *spec = getenv("TIM_SNAPAT");

        at = spec ? (int32_t)strtol(spec, NULL, 0) : -1;
    }
    if (at < 0 || flip != at)
        return;

    io_next_snapshot_path(path, sizeof path, "devtim");
    io_write_snapshot(path);
}

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

/*
 * How many flips `TIM_FLIPWANT` can name. It was 16, and the list past that
 * was dropped without a word: asking for `4,50..65` - seventeen - wrote every
 * one but flip 65, and the run then looked like a port too slow to reach it.
 * Five minutes were spent raising a timeout that could never have helped, and
 * "the port paces on a wall clock" was written down as the reason. A filter
 * that quietly discards what it was asked for is worse than one that refuses.
 */
#define DEV_WANT 256

static void dump_frame(int32_t flip)
{
    static char dir[480];
    static int32_t last = -2;           /* -2 unread, -1 no limit */
    static int32_t want[DEV_WANT];
    static int32_t nwant;
    char path[512];
    uint8_t *fb;
    FILE *f;
    int32_t i;

    if (last == -2) {
        const char *spec = getenv("TIM_FLIPS");
        const char *colon = spec ? strrchr(spec, ':') : NULL;
        const char *sel = getenv("TIM_FLIPWANT");

        last = -1;
        dir[0] = 0;
        nwant = 0;
        if (spec) {
            snprintf(dir, sizeof dir, "%s", spec);
            if (colon) {
                dir[colon - spec] = 0;
                last = (int32_t)strtol(colon + 1, NULL, 0);
            }
        }

        while (sel && *sel) {
            if (nwant == DEV_WANT) {
                fprintf(stderr, "TIM_FLIPWANT: more than %d flips; the rest "
                        "would be dropped silently\n", DEV_WANT);
                abort();
            }
            want[nwant++] = (int32_t)strtol(sel, NULL, 0);
            sel = strchr(sel, ',');
            if (sel)
                sel++;
        }
    }

    if (!dir[0] || (last >= 0 && flip > last))
        return;

    /*
     * With a wanted set, nothing else is composed at all - the cost of a frame
     * is the compose as much as the write.
     */
    if (nwant != 0) {
        for (i = 0; i < nwant; i++)
            if (want[i] == flip)
                break;
        if (i == nwant)
            return;
    }

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
 * `TIM_CLICK=<flip>:<x>:<y>[,<flip>:<x>:<y>...]` presses the left button at
 * each of those flips and lets it go two flips later.
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
/*
 * `TIM_SAVEDIR=<dir>` writes out every file the game finishes writing, under
 * the DOS name it was written as. Ours.
 *
 * A machine file never reaches a pixel, so the screen comparisons that prove
 * the picker and the panel say nothing at all about the writer - the port
 * could get every field wrong and still draw the same screen afterwards. This
 * is what lets the bytes be compared against the original's, which the
 * emulator holds in its own overlay.
 *
 * It writes on **close**, not at exit: a run that is stopped from outside -
 * which is how the port is always stopped, since a DOS game does not exit -
 * would otherwise lose the file it had just written.
 */
void dev_file_written(const char *name, const uint8_t *data, uint32_t len)
{
    const char *dir = getenv("TIM_SAVEDIR");
    char path[1024];
    const char *leaf = name;
    const char *p;
    FILE *f;

    if (dir == NULL || dir[0] == 0)
        return;

    for (p = name; *p; p++)
        if (*p == '\\' || *p == '/')
            leaf = p + 1;

    snprintf(path, sizeof path, "%s/%s", dir, leaf);

    f = fopen(path, "wb");
    if (f == NULL)
        return;

    if (len != 0)
        fwrite(data, 1, len, f);
    fclose(f);
}

#define DEV_CLICKS 16

static void dev_click(int32_t flip)
{
    static int32_t at[DEV_CLICKS], cx[DEV_CLICKS], cy[DEV_CLICKS];
    static int32_t n = -1;
    int32_t i;

    /*
     * More than one, comma-separated: `TIM_CLICK=200:320:200,600:290:300`.
     * One click reaches the copy-protection screen and the briefing, and
     * anything past the briefing needs another - so a single click could take
     * the port exactly as far as it had already been taken and no further.
     */
    if (n < 0) {
        const char *spec = getenv("TIM_CLICK");

        n = 0;
        while (spec && *spec && n < DEV_CLICKS) {
            if (sscanf(spec, "%d:%d:%d", &at[n], &cx[n], &cy[n]) != 3)
                break;
            n++;
            spec = strchr(spec, ',');
            if (spec)
                spec++;
        }
    }

    for (i = 0; i < n; i++) {
        if (flip == at[i])
            io_mouse_input(cx[i], cy[i], 1);
        else if (flip == at[i] + 2)
            io_mouse_input(cx[i], cy[i], 0);
    }
}

#define DEV_KEYS 24

/*
 * `TIM_KEY=<flip>:<scancode>[:<ascii>][,...]` puts a key in the BIOS ring at
 * that flip, the way the keyboard interrupt would have.
 *
 * Scancodes because that is what the game reads: `bios_read_key` answers
 * scancode-in-the-high-byte and every table in the game is indexed by it - 45
 * is X and 21 is Y for the two flip axes, 0x2f is V. The ASCII is optional and
 * only the text fields want it.
 *
 * A key is a single event, unlike a click, which has to be held and let go -
 * so there is no second flip here.
 */
static void dev_key(int32_t flip)
{
    static int32_t at[DEV_KEYS], scan[DEV_KEYS], ascii[DEV_KEYS];
    static int32_t n = -1;
    int32_t i;

    if (n < 0) {
        const char *spec = getenv("TIM_KEY");

        n = 0;
        while (spec && *spec && n < DEV_KEYS) {
            ascii[n] = 0;
            if (sscanf(spec, "%d:%i:%i", &at[n], &scan[n], &ascii[n]) < 2)
                break;
            n++;
            spec = strchr(spec, ',');
            if (spec)
                spec++;
        }
    }

    for (i = 0; i < n; i++)
        if (flip == at[i])
            io_key_press((uint16_t)((scan[i] << 8) | (ascii[i] & 0xff)));
}

/*
 * `TIM_POINTER=<flip>:<x>:<y>` moves the pointer there, with no button, at that
 * flip.
 *
 * Ours, and it exists for the comparison rather than for the game. A reference
 * capture carries the pointer wherever the person who took it left it, and the
 * port's pointer is wherever `TIM_CLICK` put it - so two runs of the same
 * screen differ by a cursor, in both places it is drawn, and a frame that is
 * otherwise identical reports several hundred differing pixels. Parking the
 * port's pointer where the reference's is makes the two comparable; it is not
 * a way of hiding a difference, because the cursor is still drawn and still
 * compared.
 */
static void dev_pointer(int32_t flip)
{
    static int32_t at = -2, x, y;

    if (at == -2) {
        const char *spec = getenv("TIM_POINTER");

        at = -1;
        if (spec)
            sscanf(spec, "%d:%d:%d", &at, &x, &y);
    }

    if (at >= 0 && flip == at)
        io_mouse_input(x, y, 0);
}

/*
 * `TIM_FRAME=<file>` writes the composed frame, as palette indices, when the
 * port stops - which is what `tools/compare_port.py` asks the port for: is the
 * frame it stopped on the right one? Beside it goes `<file>.pal`, the DAC as
 * 768 bytes of 8-bit RGB.
 *
 * The indices say what was drawn and the palette says whether any of it is
 * visible, and the two are worth asking separately: a frame that is right under
 * a black palette is a palette fault, not a drawing one. This project has been
 * caught by that once already, reading correct frames through a guessed palette
 * and going looking for a drawing bug that was not there.
 *
 * Ours. It lived in `sdl.c` and therefore in the shipping binary, which is what
 * the Makefile's rule about developer flags exists to prevent; `devmain.c`
 * registers it as the abort hook so it happens here instead, and no window is
 * needed for it.
 */
void dev_final_frame(void)
{
    const char *dump = getenv("TIM_FRAME");
    uint8_t *fb;
    FILE *f;

    if (!dump)
        return;

    fb = malloc((size_t)FRAME_W * FRAME_H);
    if (!fb)
        return;

    vga_compose(fb, FRAME_W, FRAME_H);
    f = fopen(dump, "wb");
    if (f) {
        fwrite(fb, 1, (size_t)FRAME_W * FRAME_H, f);
        fclose(f);
        fprintf(stderr, "wrote %dx%d indices to %s\n",
                FRAME_W, FRAME_H, dump);
    }
    free(fb);

    {
        char    pal_path[512];
        uint8_t pal[768];
        FILE   *pf;

        snprintf(pal_path, sizeof pal_path, "%s.pal", dump);
        vga_palette_rgb(pal);
        pf = fopen(pal_path, "wb");
        if (pf) {
            fwrite(pal, 1, sizeof pal, pf);
            fclose(pf);
        }
    }
}

void dev_flip_dump(int32_t flip)
{
    dev_click(flip);
    dev_pointer(flip);
    dev_key(flip);

    static const char *want = (const char *)-1;
    static int32_t at;
    FILE *f;

    snapshot_at(flip);

    note_flip(flip);
    hash_frame(flip);
    dump_frame(flip);

    /*
     * `TIM_STOPFLIP=<n>` - leave once that flip is on disk. A DOS game does
     * not exit, so a tool wanting a few flips has to kill this from outside
     * and pays its whole timeout however early the flip arrived. Opt-in, so a
     * run that does not ask for it behaves exactly as it always did.
     */
    {
        static int32_t stop = -2;

        if (stop == -2) {
            const char *spec = getenv("TIM_STOPFLIP");

            stop = spec ? (int32_t)strtol(spec, NULL, 0) : -1;
        }
        if (stop >= 0 && flip >= stop)
            exit(0);
    }

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

/*
 * OURS: report a part hook that has no transcription, instead of aborting.
 *
 * `TIM_SURVEY_HOOKS` turns one run into a list of every hook a screen needs,
 * which is a great deal faster than meeting them one abort at a time. It is
 * **only** in this file: the shipping binary links `devstub.c`, whose version
 * answers 0 so the stub aborts as it must.
 */
int32_t dev_survey_hook(uint16_t off, uint16_t kind)
{
    static int32_t on = -1;

    if (on < 0)
        on = getenv("TIM_SURVEY_HOOKS") != NULL;

    if (!on)
        return 0;

    fprintf(stderr, "HOOK 172c:%04x kind %u\n", off, kind);
    return 1;
}
