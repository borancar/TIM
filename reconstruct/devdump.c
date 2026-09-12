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
#include <strings.h>
#include <time.h>

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
#define DEV_KEY_HOLD 4

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

    /*
     * **A key is pressed at its flip and released four flips later**, and the
     * hold is the point rather than a detail. The ring only needs the make -
     * that is all a key was until 2026-09-06, and the note above used to say
     * so - but `keyboard_isr` also sets a bit in the array at DGROUP 0x468c,
     * and `timer_callback` samples that bit once a tick. A make and a break
     * in the same instant leaves nothing to sample, so Space and the arrows
     * did nothing at all: measured, and it looked exactly like the handler
     * still being missing.
     */
    for (i = 0; i < n; i++) {
        if (flip == at[i])
            io_keyboard_scancode((uint8_t)scan[i]);
        if (flip == at[i] + DEV_KEY_HOLD)
            io_keyboard_scancode((uint8_t)(scan[i] | 0x80));
    }
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
 * `TIM_LEVEL=<n>` and `TIM_RUN=1` - reach a puzzle, and start it, with no
 * pointer and no keyboard anywhere in it.
 *
 * OURS, and not a transcription. What it exists for: every route into a puzzle
 * so far has been `TIM_CLICK`, which is a flip number and a pair of pixel
 * coordinates - so a tool that wants to run a machine has to know where the
 * control is drawn and when the screen it is drawn on arrives. Both are facts
 * about the *picture*, and neither is what the tool is actually asking for.
 * This asks for the thing itself.
 *
 * **It writes the words the game's own regions write, and nothing else.** The
 * state at DGROUP 0x4e6b is what a click produces: `regions_handle_pointer`
 * ends a click with `0x4e6b = [region+0x10]`, so putting a value there by hand
 * is the same event arriving by a different road. The three that matter:
 *
 *   0x2000  run the machine. It is the box above the parts bin, the region at
 *           (576,0)-(632,63), whose +0x10 `region_cursor_bin_above` sets to
 *           0x2000 while your hand is empty. `run_machine_loop` then loops
 *           *while* 0x4e6b is 0x2000, which is what makes this word the honest
 *           answer to "is the machine running" rather than a flag of our own.
 *   2       the briefing, waiting. State 2 is not in `game_screen`'s jump
 *           table, so the screen simply sits and presents itself.
 *   0x8000  the briefing's own button - two regions on the 0x4e77 list carry
 *           it - which paints the panel, leaves 0x1000 behind and ends the
 *           screen. 0x1000 is then where `game_screen_loop` sits while a
 *           puzzle is being built.
 *
 * **The intro is the exception, and it is worth saying why.** Its title and
 * credits animations have no exit but a click: the loop ends on
 * `0x5774 == 2 || 0x5772 == 2` - the buttons as the guest sees them, after
 * `update_button_state` - and when the title runs out it starts the credits,
 * and when the credits run out it starts the title again. There is no frame
 * count, no key and no state that ends it. So this writes 0x5774 itself, which
 * is one layer further in than the rest of this routine and is called out here
 * rather than hidden: it is the button word, not a mouse event, and the game
 * has left us nothing else to write.
 *
 * The flip hook is the right place for all of it because 0x5774 is rebuilt by
 * `update_button_state` at the top of every pass and read further down, and a
 * page flip happens in between.
 *
 * **Which screen we are on is read from the state, not counted.** The phases
 * are ordered, but a restored snapshot can begin on any of them - the point
 * the user made when this was asked for - so the only thing carried across
 * flips is whether the intro is behind us, which a restore sets on the spot.
 * A snapshot taken with the machine already running is then reported as such
 * and left alone, rather than being "started" a second time - which on the run
 * control is a *stop*.
 */
static int32_t autoplay_past_intro;

void dev_autoplay_past_intro(void)
{
    autoplay_past_intro = 1;
}

static void dev_autoplay(int32_t flip)
{
    static int32_t armed = -1;
    static int32_t want_run;
    static int32_t trace = -1;
    uint16_t state;

    if (armed < 0) {
        armed = (getenv("TIM_LEVEL") != NULL || getenv("TIM_RUN") != NULL);
        want_run = getenv("TIM_RUN") != NULL;
    }
    if (!armed)
        return;

    state = DG4E67.state;

    /*
     * `TIM_TRACE=autoplay` prints the state word at every flip. Which screen
     * the game is on is the whole of what this routine reasons about, and the
     * sequence is not guessable from the source - the intro presents three
     * frames before its own loop starts, so the first `0x2000` a run sees is
     * not the one the reasoning assumed. Two wrong fixes went in before this
     * was printed rather than deduced.
     */
    if (trace < 0)
        trace = (getenv("TIM_TRACE") != NULL
                 && strstr(getenv("TIM_TRACE"), "autoplay") != NULL);
    if (trace)
        fprintf(stderr, "io: autoplay flip %d state %04x btn %04x\n",
                flip, (unsigned)state, (unsigned)DG5768.button_left);

    if (!autoplay_past_intro) {
        static int32_t nudged;

        /*
         * The intro's animations run with 0x4e6b at 0x2000, which is the same
         * word the running machine uses - so this is the one phase that has to
         * be remembered rather than recognised.
         *
         * **The button is written until the intro lets go, not once.** Three
         * of the title loop's page flips happen before its `while`, and
         * `update_button_state` at the top of every pass rebuilds 0x5774 - so
         * a single write can land on a flip that is thrown away, and did:
         * measured, the first attempt wrote at flip 6, the intro carried on,
         * and flip 7 was read as the machine already running because the phase
         * had been advanced on the *write* rather than on its effect. The
         * intro is behind us when the state leaves 0x2000 after a nudge, which
         * is the effect itself and not a proxy for it.
         */
        if (state == 0x2000) {
            DG5768.button_left = 2;
            nudged = 1;
        } else if (nudged) {
            autoplay_past_intro = 1;
            fprintf(stderr, "io: autoplay leaves the intro at flip %d\n", flip);
        }
        return;
    }

    if (state == 0x2000) {
        fprintf(stderr, "io: autoplay - the machine is running (flip %d)\n",
                flip);
        armed = 0;
    } else if (state == 2) {
        /*
         * **This fires twice, and the message says only what was written.**
         * State 2 is the briefing sitting and presenting itself - but the
         * intro's own tail sets 0x4e6b to 2 as well, several flips before
         * `game_setup`, and a flip lands there. Measured: flip 9 is the tail
         * and flip 13 is the briefing. The first write is harmless because
         * `round_setup` ends by putting 2 back, so nothing is skipped; naming
         * the screen in the message would have been a guess that is wrong half
         * the time.
         */
        DG4E67.state = 0x8000;
        fprintf(stderr, "io: autoplay takes state 2 forward at flip %d\n",
                flip);
    } else if (state == 0x1000) {
        /*
         * `TIM_LOADMACHINE=<name>` - load a machine file over the level that
         * is already up, which is the whole point of extracting one.
         *
         * The three calls are the game's own, in the game's own order: it is
         * what `screen_state_0040` does at 0x11... after the file picker
         * returns - `round_teardown`, `load_animation`, `reset_machine`. The
         * level is already loaded, so the goal the machine is judged against
         * is the level's; this only replaces the parts.
         *
         * Done here rather than in `play_level` because the load has to happen
         * once the play screen is up, which is the state this arm is.
         */
        {
            static int32_t loaded;
            const char *file = getenv("TIM_LOADMACHINE");

            if (file != NULL && *file && !loaded) {
                int32_t i;

                loaded = 1;
                for (i = 0;
                     file[i] && i < (int32_t)sizeof DG52FE.name - 1; i++)
                    DG52FE.name[i] = file[i];
                DG52FE.name[i] = 0;

                round_teardown();
                load_animation((char *)DG52FE.name);
                reset_machine();
                fprintf(stderr, "io: autoplay loaded the machine %s at flip "
                        "%d\n", file, flip);
                return;         /* let it settle before starting */
            }
        }

        if (want_run) {
            DG4E67.state = 0x2000;
            fprintf(stderr, "io: autoplay starts the machine at flip %d\n",
                    flip);
        } else {
            fprintf(stderr, "io: autoplay - the puzzle is up (flip %d)\n",
                    flip);
            armed = 0;
        }
    }
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
        FILE *pf;

        snprintf(pal_path, sizeof pal_path, "%s.pal", dump);
        vga_palette_rgb(pal);
        pf = fopen(pal_path, "wb");
        if (pf) {
            fwrite(pal, 1, sizeof pal, pf);
            fclose(pf);
        }
    }
}

/*
 * OURS: `TIM_TRACE=sfx` names each sound effect the game asks for.
 *
 * **Effects only.** `play_sound` is also how music is started - `select_music`
 * calls it with the tune - so the identifier is the filter: effects are 1..20
 * and tunes are 0x3e9..0x3f8, and the two never overlap. Without that the
 * trace is mostly tunes, which is not what anyone is watching for.
 *
 * This is how a sound gets named: run a machine with a known part in it and
 * read which identifier it queues. That is what settled the cannon as 6 and
 * dynamite as 8, where listening to waveforms had put both in the wrong
 * category entirely.
 */
void dev_sound_played(int16_t id)
{
    static int32_t on = -1;

    if (on < 0)
        on = trace_asks_sfx();
    if (on && id > 0 && id <= 20)
        fprintf(stderr, "io: sfx play_sound(%d)\n", (int)id);
}

/*
 * OURS: `TIM_TRACE=level` says when a puzzle is solved.
 *
 * `finish_level` at 0x02710 is the one place the game decides a machine has
 * done what the briefing asked, and it is reached by no other route - so this
 * is the whole of "did it work", and it is what `tools/check_solutions.py`
 * watches for. Without it a solved level and a level that merely ran for a
 * while look identical from outside: the screen keeps painting either way.
 *
 * The score is read before `finish_level` banks the bonus, so it is the score
 * the level was entered with rather than the one it ends on.
 */
void dev_level_solved(int16_t level, int16_t score)
{
    static int32_t on = -1;

    if (on < 0)
        on = trace_asks_level();
    if (on)
        fprintf(stderr, "io: level solved=%d score=%d frames=%d\n",
                (int)level, (int)score, (int)DG4E67.machine_frames);
}

/*
 * OURS: `TIM_SIMULATE=<frames>` - run a restored machine with no clock, no
 * input and no display, and say whether the goal test fired.
 *
 * `run_machine_loop` at 0x012ab is the game's own loop and this is **not a
 * second copy of it**: it is the same per-frame sequence with the hardware
 * taken out. Each frame the game latches its sound requests, reads the button
 * and a key, lets the play regions see the pointer, and then does the six
 * things that are the machine - `step_machine`, `mark_parts_in_dirty_rects`,
 * `step_word_4e87`, `replay_shapes`, `step_and_draw_machine`,
 * `shift_all_histories` - before `check_goal`. The physics reads
 * `machine_frames` and nothing else about time: the eight-tick spin the loop
 * paces itself with, and the tick total it banks into `elapsed_ticks`, feed
 * the score and the display, not the parts. So here the spin is replaced by
 * the eight ticks it waits for, the frame is not presented, no sound is
 * started or stopped, and there is no pointer, button or key - which means no
 * timer thread either, and a run that gives the same answer every time, where
 * the real loop's tick accumulation does not (see CLAUDE.md).
 *
 * What it proves is narrower than `check_solutions.py` and faster by two
 * orders: the parts, their physics and the goal, over the whole solution set,
 * in seconds rather than an hour. It does not exercise the loop's own input
 * and presentation path, and `finish_level` is not called, because that is
 * the dialog - the goal test writing 0x200 into 0x4e6b is where the game
 * decides the machine worked, and that word is what is read.
 *
 * Answers the frame count it stopped at; the verdict goes to stderr.
 */
int32_t dev_simulate_machine(int32_t max_frames)
{
    int32_t frames = 0;

    if (DG4E67.state != 0x2000)
        DG4E67.state = 0x2000;       /* what --run does once the puzzle is up */

    clear_machine();
    DG4E67.elapsed_ticks = 0;
    DG44EE.frame_budget = 0x2710;

    while (DG4E67.state == 0x2000 && frames < max_frames) {
        step_machine();
        mark_parts_in_dirty_rects();
        step_word_4e87();
        replay_shapes();
        step_and_draw_machine(0);

        DG4E67.elapsed_ticks = (uint16_t)(DG4E67.elapsed_ticks + 8);
        DG44EE.frame_budget = 0x2710;

        shift_all_histories();

        if (DG4E67.freeform == 0)
            check_goal();

        DG4E67.machine_frames++;
        frames++;
    }

    if (DG4E67.state == 0x200)
        fprintf(stderr, "io: simulate solved=1 level=%d frames=%d score=%d\n",
                (int)DG4E67.round_number, (int)frames, (int)DG4E67.score);
    else
        fprintf(stderr, "io: simulate solved=0 level=%d frames=%d state=%04x\n",
                (int)DG4E67.round_number, (int)frames, (unsigned)DG4E67.state);
    return frames;
}

/*
 * OURS: `TIM_PARTPICS=<dir>` writes every part's bin icon as raw pixels.
 *
 * **The game draws them.** An icon is one of four bitmap formats - scaled,
 * compressed, offset-table, or plain planar, chosen by the marker in field 4 -
 * and all four decoders are already transcribed. Decoding them again in Python
 * would be writing our own version of something the original does, which is
 * the rule this project is built on. So each icon is drawn to a cleared box
 * with `draw_bitmap_centred`, exactly as the parts bin draws it, and the
 * composed frame is read back.
 *
 * The list at DGROUP 0x4ec7 is the game's own: `icons.bmp`, indexed by kind,
 * the same table `draw_machine_layer_a` walks to fill the bin. So the mapping
 * from a part to its picture is the game's and not a guess.
 *
 * Output is raw indexed pixels plus the palette, because turning them into
 * PNGs is presentation and belongs in tools/, not here.
 */
#define PIC_W 64
#define PIC_H 48
#define PIC_X 128
#define PIC_Y 128

/*
 * OURS: `TIM_LEVELSCAN=<lo>:<hi>` says which part kinds each level holds.
 *
 * **The game reads the levels.** A part record on disk is not a fixed stride -
 * a rope adds 0x38 bytes, kind 7 carries an extra part number, and a version
 * word decides whether +0x0a is even present - so a byte scan cannot tell a
 * kind from any other small number. Measured: scanning the archive for the
 * word 32 found "59 pumpkins" in one 2.6 KB level, which is every coordinate
 * that happened to be 32. `load_level` is the game's own reader, and the part
 * list it leaves at DGROUP 0x521b is the answer.
 */
/*
 * OURS: `TIM_DATE=MM-DD` or `TIM_DATE=YYYY-MM-DD`, the date the game is told.
 *
 * Four parts are on the calendar and cannot be reached any other way - they
 * are in no level, and `machine.c` sets their flags from `dos_getdate`:
 *
 *     TIM_DATE=02-14   the heart balloon, kind 33
 *     TIM_DATE=03-17   sets 0x4e7f, which nothing reads
 *     TIM_DATE=10-31   the pumpkin, kind 32
 *     TIM_DATE=12-25   the christmas tree, kind 34
 *
 * The weekday is computed rather than asked for, by Sakamoto's method, so the
 * three answers are consistent with each other - a game that is told the 25th
 * of December and a Tuesday when it was a Thursday is being told two things.
 */
/*
 * OURS: `TIM_DEVICE` and `TIM_MODULE`, which choose the two sound overlays
 * without editing RESOURCE.CFG.
 *
 * The two bytes are independent indices into two tables in DGROUP - the
 * devices at 0x4a1c and the modules at 0x4a2e - and `setup_sound_device` loads
 * one chunk of `SX.OVL` from each. The device is the **music**, the module the
 * **digitised sound**; a Sound Blaster is `ADL:` and `ASB:` together, which is
 * what the shipped RESOURCE.CFG says and what INSTALL.COM writes.
 *
 * Either may be a name or a number, and `none` is the game's own "no such
 * device" of -2. A name is matched on its first three letters so `adl`, `ADL`
 * and `ADL:` all work.
 *
 * Only three of the nine device chunks have a body in the port. The rest load
 * and then abort in `driver_kind`, on purpose - a driver the port cannot drive
 * must say so rather than play nothing - and `TIM_ABORTSNAP` is how to catch
 * one for reading.
 */
static const char *const sound_devices[9] = {
    "STD", "TAN", "ADL", "M32", "SBP", "PS1", "PRO", "GMD", "NLD"
};
static const char *const sound_modules[4] = { "ASB", "APS", "ATD", "APA" };

static int32_t sound_index(const char *what, const char *spec,
                           const char *const *names, int32_t n, int32_t *out)
{
    int32_t i;
    char *end;
    long v;

    if (strcasecmp(spec, "none") == 0) {
        *out = -2;
        return 1;
    }

    for (i = 0; i < n; i++) {
        if (strncasecmp(spec, names[i], 3) == 0) {
            *out = i;
            return 1;
        }
    }

    v = strtol(spec, &end, 0);
    if (end != spec && *end == 0 && v >= 0 && v < n) {
        *out = (int32_t)v;
        return 1;
    }

    fprintf(stderr, "%s: '%s' is not one of", what, spec);
    for (i = 0; i < n; i++)
        fprintf(stderr, " %s", names[i]);
    fprintf(stderr, " or none\n");
    return 0;
}

int32_t dev_sound_cfg(uint8_t cfg[3])
{
    static int32_t said;
    const char *dev = getenv("TIM_DEVICE");
    const char *mod = getenv("TIM_MODULE");
    int32_t v, changed = 0;

    if (dev != NULL && sound_index("TIM_DEVICE", dev, sound_devices, 9, &v)) {
        cfg[1] = (uint8_t)v;
        changed = 1;
    }
    if (mod != NULL && sound_index("TIM_MODULE", mod, sound_modules, 4, &v)) {
        cfg[2] = (uint8_t)v;
        changed = 1;
    }

    /*
     * Once, however many times the file is opened: `dos_try` is called for
     * each spelling of the name it is willing to try.
     */
    if (changed && !said) {
        said = 1;
        fprintf(stderr, "sound: device %d (%s), module %d (%s)\n",
                (int8_t)cfg[1],
                (int8_t)cfg[1] >= 0 && cfg[1] < 9 ? sound_devices[cfg[1]] : "none",
                (int8_t)cfg[2],
                (int8_t)cfg[2] >= 0 && cfg[2] < 4 ? sound_modules[cfg[2]] : "none");
    }

    return changed;
}

int32_t dev_date_override(uint16_t *year, uint16_t *monthday,
                          uint16_t *weekday)
{
    static const int32_t t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    const char *spec = getenv("TIM_DATE");
    int32_t y = 2000, m = 0, d = 0, w;

    if (spec == NULL)
        return 0;

    if (sscanf(spec, "%d-%d-%d", &y, &m, &d) != 3) {
        y = 2000;
        if (sscanf(spec, "%d-%d", &m, &d) != 2) {
            fprintf(stderr, "TIM_DATE wants MM-DD or YYYY-MM-DD, not '%s'\n",
                    spec);
            return 0;
        }
    }
    if (m < 1 || m > 12 || d < 1 || d > 31) {
        fprintf(stderr, "TIM_DATE: %02d-%02d is not a date\n", m, d);
        return 0;
    }

    {
        int32_t yy = y - (m < 3);
        w = (yy + yy / 4 - yy / 100 + yy / 400 + t[m - 1] + d) % 7;
    }

    *year = (uint16_t)y;
    *monthday = (uint16_t)((m << 8) | d);
    *weekday = (uint16_t)w;

    {
        static int said;

        if (!said) {
            said = 1;
            fprintf(stderr, "dev: the game is told it is %04d-%02d-%02d\n",
                    y, m, d);
        }
    }
    return 1;
}

void dev_level_scan(void)
{
    const char *spec = getenv("TIM_LEVELSCAN");
    const char *colon;
    int32_t lo, hi, n;

    if (spec == NULL)
        return;

    colon = strchr(spec, ':');
    lo = (int32_t)strtol(spec, NULL, 0);
    hi = colon ? (int32_t)strtol(colon + 1, NULL, 0) : lo;

    for (n = lo; n <= hi; n++) {
        uint8_t seen[256];
        uint16_t si;
        int32_t k, count = 0;

        memset(seen, 0, sizeof seen);
        load_level((uint16_t)n);

        for (si = DG521B.placed_parts.next_ptr; si != 0 && count < 4096;
             si = PART_PTR(si)->next_ptr, count++) {
            uint16_t kind = PART_PTR(si)->kind;

            if (kind < 256)
                seen[kind] = 1;
        }

        /* The three lists separately, walked from their head words: the
           parts on the machine at 0x521b, the moving ones at 0x5179 and the
           bin - what the player is given - at 0x50d7. */
        {
            volatile struct list_node *heads[3] = {
                &DG521B.placed_parts, &DG5179.moving_parts, &DG50D3.parts_bin,
            };
            static const char *names[3] = { "placed", "moving", "bin" };
            int32_t h;

            printf("level %d ", n);
            for (h = 0; h < 3; h++) {
                int32_t c = 0;

                printf(" %s", names[h]);
                for (si = heads[h]->next_ptr; si != 0 && c < 4096; si = PART_PTR(si)->next_ptr) {
                    printf("%c%d/%x", c ? ',' : ' ', PART_PTR(si)->kind,
                           PART_PTR(si)->flags_06 & 0x3800);   /* kind / list bits */
                    c++;
                }
                printf(" (%d) ", c);
            }
            printf("\n");
        }

        /*
         * **Stop rather than print a zero.** `load_level` allocates a record
         * per part and this loop frees nothing, so the heap runs out - from
         * level 1 the loader starts failing at about the twelfth, and every
         * level after it came out as `parts 0  kinds` with nothing to say it
         * was the scan that failed and not the level that was empty. No level
         * has no parts. That zero sent a search for a part kind past the two
         * levels that hold it, and the polygon path was written up as
         * unreachable on the strength of it.
         *
         * `round_teardown` is not the fix and makes it worse - it frees the
         * lists but not the per-kind bitmaps, and the loader then fails four
         * levels sooner. Until the leak is found, scan in chunks: a run
         * beginning at 13 reads 13 onwards correctly.
         */
        if (count == 0) {
            printf("level %d  SCAN FAILED - the heap is exhausted, not the "
                   "level empty; re-run with TIM_LEVELSCAN=%d:%d\n", n, n, hi);
            fflush(stdout);
            return;
        }

        printf("level %d  parts %d  kinds", n, count);
        for (k = 0; k < 256; k++)
            if (seen[k])
                printf(" %d", k);
        printf("\n");
        fflush(stdout);
    }
}

void dev_part_pics(void)
{
    const char *dir = getenv("TIM_PARTPICS");
    uint8_t *fb;
    uint8_t pal[768];
    uint16_t list, n, i;
    char path[512];
    FILE *f;

    if (dir == NULL)
        return;

    /*
     * `icons.bmp` is loaded by `game_intro`, not by `game_startup`, so on this
     * path the list is empty and the game's own loader is asked for it - with
     * the game's own name pointer, 0x2582, the one at game.c's load site.
     */
    if (DG4E67.icons_bmp_ptr == 0)
        DG4E67.icons_bmp_ptr = load_bitmaps((char *)DG254A.icons_bmp);

    /*
     * `game_startup` loads tim.pal into DGROUP 0x52ed but leaves **black.pal**
     * the active one - the game switches over later, in `game_intro`. Without
     * this the icons come out as black rectangles and look like broken art
     * rather than a missing palette, which is exactly how it first appeared.
     */
    set_palette_pointer(DG52ED.pal_tim_ptr.ptr);

    list = DG4E67.icons_bmp_ptr;
    n = count_list(BMPLIST(list));
    fb = malloc((size_t)FRAME_W * FRAME_H);
    if (fb == NULL || n == 0) {
        fprintf(stderr, "part pics: no icon list at 0x4ec7\n");
        free(fb);
        return;
    }

    vga_palette_rgb(pal);
    snprintf(path, sizeof path, "%s/palette.bin", dir);
    if ((f = fopen(path, "wb")) != NULL) {
        fwrite(pal, 1, sizeof pal, f);
        fclose(f);
    }

    for (i = 0; i < n; i++) {
        uint16_t icon = DGU16((uint16_t)(list + 2 * i));
        int32_t row;

        DG3890.clip_enabled = 1;
        DG3890.clip_left = 0;
        DG3890.clip_top = 0;
        DG3890.clip_right = 0x27f;
        DG3890.clip_bottom = 0x1df;
        DG3890.fill_enabled = 1;
        DG3890.fill_colour = 0;
        DG3890.second_colour = 0;
        fill_rect(PIC_X, PIC_Y, PIC_W, PIC_H);

        if (icon != 0)
            draw_bitmap_centred(icon, PIC_X, PIC_Y, PIC_W, PIC_H);

        vga_compose(fb, FRAME_W, FRAME_H);

        snprintf(path, sizeof path, "%s/part_%02u.raw", dir, (unsigned)i);
        if ((f = fopen(path, "wb")) == NULL)
            continue;
        for (row = 0; row < PIC_H; row++)
            fwrite(fb + (size_t)(PIC_Y + row) * FRAME_W + PIC_X, 1, PIC_W, f);
        fclose(f);
    }

    fprintf(stderr, "part pics: wrote %u icons of %dx%d into %s\n",
            (unsigned)n, PIC_W, PIC_H, dir);
    free(fb);
}

/*
 * OURS: the button as the *guest* sees it, once a page flip.
 *
 * `TIM_TRACE=mouse` shows what SDL delivered, and that cannot answer whether a
 * hold is reaching the game: a still hold produces no SDL events at all, so it
 * logs nothing, and a press followed by an adjacent release looks the same
 * whether the button stayed down for a second or was let go at once. This
 * samples DGROUP 0x48eb and 0x5774 on a clock instead - the page flip, about
 * thirty times a second - so a one-second hold is thirty lines saying 01.
 */
static void dev_button_sample(void)
{
    static int32_t on = -1;

    if (on < 0)
        on = trace_asks_sfx() ? 0 : (getenv("TIM_TRACE") != NULL
                                     && strstr(getenv("TIM_TRACE"), "btn")
                                     != NULL);
    if (on)
        fprintf(stderr, "io: btn 48eb %02x  5774 %04x  5768 %04x\n",
                DG48DA.buttons, (unsigned)DG5768.button_left, (unsigned)((uint16_t)DG5768.button_accum_a));
}

void dev_flip_dump(int32_t flip)
{
    dev_button_sample();
    dev_click(flip);
    dev_pointer(flip);
    dev_key(flip);
    dev_autoplay(flip);

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
            DG4E67.origin_x, DG4E67.origin_y, DG4E67.state,
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
