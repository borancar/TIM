/*
 * Developer entry point. NOT a transcription, and NOT part of what ships.
 *
 * Everything a comparison needs lives here so that main.c stays what the
 * original's start-up was. tools/ calls this binary, never ./tim.
 *
 * **It behaves as `tim` does**: with no options it opens the window, captures
 * the mouse and plays, and Shift+F2 writes a snapshot. That was the other way
 * round until now - headless unless `TIM_WINDOW` was set - and the asymmetry
 * was a nuisance every time a state had to be reached by playing. The tools
 * that drive this binary in batch set `TIM_HEADLESS=1`, which is the honest
 * shape of it: a window is what a person needs and its absence is what a
 * comparison needs, so the comparison is the one that asks.
 *
 * Headless is not a different run. `dev_flip_dump` composes its frames from
 * the planes on the guest's own page flip, never from the window, so what a
 * tool reads is the same either way.
 *
 * `--help` lists every option and every environment variable, and `usage()`
 * below is the only place that list lives - a flag added without a line there
 * is a flag nobody can find. Most of what this binary does is steered by the
 * environment rather than by arguments, so leaving those out would be no help
 * at all: TIM_FLIPS filled this machine's disk twice while its meaning was
 * only ever written down in a C comment.
 */
/*
 * `nanosleep` is POSIX, not C. See devwav.c for the same note.
 */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "dgroup.h"
#include "io.h"
#include "sdl.h"
#include "tim.h"

#define W 640
#define H 480

/*
 * OURS: Shift+F2, the same as main.c's. `TIM_SNAP=<path>` moves the file.
 */
static void on_hotkey(int32_t id)
{
    char path[512];

    if (id != SDL_HOTKEY_SNAPSHOT)
        return;
    io_next_snapshot_path(path, sizeof path, "devtim");
    io_write_snapshot(path);
}

/*
 * OURS: pick up a restored machine and keep playing it.
 *
 * A snapshot holds the machine and not the port's call stack, so something has
 * to choose where to start executing. This is `game_round` at 0x0eff5 **minus
 * its `round_setup`** - the dispatch, the two states that end a round, and the
 * teardown - because `round_setup` is what would load the level again and
 * throw away the very state being restored.
 *
 * Below it is `game_play`'s tail at 0x0eed5, which is what advances the puzzle
 * count and writes the record out, so a resumed session can finish its round
 * and go on to the next one exactly as a fresh run would. Rounds after the
 * first are the transcribed `game_round`, not this copy.
 *
 * Both are transcribed routines written out a second time, which is normally
 * the thing this project refuses to do; they are here because the alternative
 * is a resume that either restarts the round or stops after it. Keep them
 * matching their originals if either changes - the addresses above are where
 * to look.
 *
 * What does **not** come back is anything a C local was holding: the button
 * repeat counters in the screen loops, whether a repaint was pending, the part
 * being dragged. A resumed round starts those afresh, so a snapshot taken
 * mid-drag comes back with the part put down.
 */
static void resume_from_snapshot(void)
{
    while (DGU16(0x4e6b) != 0x200 && DGU16(0x4e6b) != 1) {
        heap_check_or_hang();

        if (DGU16(0x4e6b) == 2)
            game_screen();
        else if (DGU16(0x4e6b) == 0x2000)
            run_machine_loop();
        else
            game_screen_loop();
    }

    if (DGU16(0x4e6b) == 0x200)
        finish_level();

    round_teardown();

    while (DG16(0x4ebf) != 0) {
        if (DG16(0x4e6b) == 1) {
            DG16(0x4ebf) = 0;
        } else {
            DG16(0x4ebd) = (int16_t)(DG16(0x4ebd) + 1);
            if (DG16(0x4ebd) > DG16(0x4eb7)) {
                DG16(0x4eb7) = DG16(0x4ebd);
                sub_12bed();
            }
            game_round();
        }
    }

    /*
     * And out the same way `game_main` goes, which this had been leaving off:
     * a resumed session that quit ran the rounds and then simply returned, so
     * `game_teardown` - the password on the way out, the frees, the vectors
     * handed back - was never reached from here. It was reachable by playing
     * from the start and not by resuming, which is the sort of difference a
     * harness introduces and then hides.
     */
    game_teardown(1);
}

/*
 * OURS: the whole of what this binary accepts, in one place.
 */
static void usage(void)
{
    printf(
"usage: devtim [--restore FILE] [--raw FILE [--lines N]]\n"
"\n"
"The developer build of the port. It plays exactly as ./tim does - a window,\n"
"the mouse captured, Shift+F2 for a snapshot - and adds what a comparison\n"
"needs. ./tim itself takes no arguments on purpose: a DOS game has no command\n"
"line.\n"
"\n"
"options:\n"
"  -h, --help      this text\n"
"  --restore FILE  start from a snapshot written by Shift+F2 instead of from\n"
"                  the beginning. Memory and hardware come back; the port's\n"
"                  own call stack cannot, so the round is re-entered at the\n"
"                  screen the snapshot was on and C locals start afresh.\n"
"  --raw FILE      write the composed frame as 8-bit palette indices and exit.\n"
"                  Indices, not a picture: two of them can share a colour.\n"
"  --lines N       CRTC blanking line for --raw (default 399).\n"
"\n"
"environment, general:\n"
"  TIM_DIR=DIR     where TIM.img and TIM.unpacked.exe are (default out)\n"
"  TIM_HEADLESS=1  open no window. What the tools in tools/ set, so a batch\n"
"                  comparison needs no display; frames come from the planes\n"
"                  either way, so headless is not a different run.\n"
"  TIM_RESTORE=F   the same as --restore\n"
"  TIM_SNAP=PATH   write Shift+F2's snapshot here instead of numbering one\n"
"  TIM_SNAPDIR=DIR where the numbered snapshots go (default out)\n"
"  TIM_SNAPAT=N    write a snapshot at flip N without anyone pressing a key\n"
"  TIM_ABORTDUMP=F where a stub's abort dumps memory and registers\n"
"  TIM_ABORTSNAP=F where a stub's abort writes the *whole* machine.\n"
"                  DGROUP is not enough when the stub is inside code\n"
"                  the game loaded - a sound module, an overlay.\n"
"  TIM_GAMEDIR=DIR the directory the guest sees as its own, instead of\n"
"                  incredible-machine. The comparison tools set it so a\n"
"                  sound device in RESOURCE.CFG cannot change what they\n"
"                  measure.\n"
"  TIM_SURVEY_HOOKS=1  a part hook with no transcription reports itself\n"
"                  and the run carries on, so one pass names every hook a\n"
"                  screen needs. Only devtim has it; tim aborts, which is\n"
"                  what a missing hook must do.\n"
"  TIM_WAV=FILE    write every block the Sound Blaster plays to FILE as a\n"
"                  WAV, resampled to one rate so a run that mixes 11 and\n"
"                  22 kHz is one playable file. The header is rewritten\n"
"                  after each block, so a killed run still leaves audio.\n"
"  TIM_SFXALL=N    ask the game for sound identifiers 1..N and exit, so\n"
"                  every waveform reaches the card. Use with TIM_SFXDIR.\n"
"  TIM_SFXDIR=DIR  write every distinct waveform the card plays into DIR\n"
"                  as its own WAV, named by length, rate and checksum.\n"
"  TIM_TRACE=WHAT  trace to stderr. One of:\n"
"                    speaker  the tone and the gate, as the game's own\n"
"                             driver programs the 8253 and port 0x61\n"
"                    sb       the Sound Blaster: DSP commands, resets,\n"
"                             the IRQ, and each DMA block with its rate\n"
"                    mouse    button events and INT 33h queries\n"
"                    crtc     writes to the CRTC registers\n"
"                    dac      palette writes\n"
"\n"
"environment, driving a run:\n"
"  TIM_CLICK=F:X:Y[,...]   click at X,Y once flip F has been presented\n"
"  TIM_KEY=F:SCAN[:ASCII][,...]  put a key in the BIOS ring at flip F.\n"
"                          Scancodes, because that is what the game\n"
"                          reads: 45 is X and 21 is Y, the two flip axes.\n"
"  TIM_POINTER=X:Y         put the pointer there without clicking\n"
"  TIM_PARTS=...           place parts before the run starts\n"
"\n"
"environment, capturing what it drew:\n"
"  TIM_FLIPS=DIR[:LAST]    write a frame per page flip, stopping after LAST.\n"
"                          A STOPPING POINT, NOT A FILTER: every flip up to\n"
"                          LAST is written, which is 308 KB each.\n"
"  TIM_FLIPWANT=F1,F2,...  write only these flips. This is the filter, and\n"
"                          runs of the form 50..65 are allowed.\n"
"  TIM_FLIPHASH=PATH       a digest per flip instead of the frame\n"
"  TIM_FLIPCOUNT=PATH      how many flips the run made\n"
"  TIM_STOPFLIP=N          stop once flip N has been presented\n"
"  TIM_FRAME=PATH          write the final frame when the run ends\n"
"  TIM_SAVEDIR=DIR         where the game's own saved files go\n");
}

int main(int argc, char **argv)
{
    const char *raw = NULL;
    const char *restore = getenv("TIM_RESTORE");
    int32_t lines = 399;

    for (int32_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage();
            return 0;
        }
        if (!strcmp(argv[i], "--raw") && i + 1 < argc)
            raw = argv[++i];
        else if (!strcmp(argv[i], "--restore") && i + 1 < argc)
            restore = argv[++i];
        else if (!strcmp(argv[i], "--lines") && i + 1 < argc)
            lines = (int32_t)strtol(argv[++i], NULL, 0);
        else {
            fprintf(stderr, "unknown option: %s\n\n", argv[i]);
            usage();
            return 2;
        }
    }

    io_reset();

    if (!raw) {
        /* The same start-up main.c does. */
        /*
         * OURS: `TIM_GAMEDIR` points the guest's file world somewhere other
         * than `incredible-machine`.
         *
         * The comparison tools need this. They compare *graphics*, and the
         * sound device ought not to matter to them - but it does, because a
         * driver the port has no body for stops the run and a different one
         * changes its timing. Before this they inherited whatever
         * RESOURCE.CFG happened to say, so setting the sound device broke
         * `tools/check_briefing.py` outright.
         *
         * A developer flag, so it is in DEVFLAGS and cannot reach `tim`:
         * the shipping game reads its own directory and nothing else.
         */
        {
            const char *game = getenv("TIM_GAMEDIR");

            if (game && *game)
                io_set_game_dir(game);
        }

        const char *dir = getenv("TIM_DIR");
        char img[512], exe[512];

        if (!dir)
            dir = "out";
        snprintf(img, sizeof img, "%s/TIM.img", dir);
        snprintf(exe, sizeof exe, "%s/TIM.unpacked.exe", dir);

        if (!io_load_program(img, exe)) {
            fprintf(stderr,
                    "cannot read %s and %s - run tools/unlzexe.py first, or "
                    "set TIM_DIR\n", img, exe);
            return 1;
        }

        /*
         * A restore replaces every byte of this, so the start-up runs only to
         * settle what the load derives - where DGROUP is, above all - and the
         * snapshot is laid over the top. `setup_streams` is deliberately not
         * called on that path: the stream table lives in DGROUP and the file
         * being restored already has it, opened against the handles
         * `io_state_load` brings back.
         */
        if (!restore)
            setup_streams();

        dev_wav_open();
        dev_sfx_open();

        if (getenv("TIM_HEADLESS") == NULL) {
            if (!sdl_open())
                return 1;
            io_on_present(sdl_present);
            io_on_abort(sdl_hold);
            sdl_on_hotkey(on_hotkey);
        } else {
            io_on_abort(dev_final_frame);
        }

        if (restore && !io_read_snapshot(restore))
            return 1;

        io_set_timer(timer_tick);

        /*
         * `TIM_SFXALL=N` asks the game for each sound identifier in turn
         * instead of playing, so that every waveform passes through the card
         * and `TIM_SFXDIR` can write it. The identifiers are the ones the
         * `INF:` index in TIM.SX carries and the ones `play_sound` is called
         * with in the game's own code - 0x13 and 0x14 in the intro.
         *
         * This asks the *game* to decompress them, which is the only honest
         * way to get at them: the records in TIM.SX are compressed, and a
         * capture at the DMA is the bytes the hardware was actually handed.
         * The alternative - decoding the container ourselves - would be
         * writing our own version of something the original already does.
         */
        if (restore) {
            resume_from_snapshot();
        } else if (getenv("TIM_SFXALL") != NULL) {
            const char *spec = getenv("TIM_SFXALL");
            int32_t first = 1, last = atoi(spec);
            int32_t id;
            const char *dash = strchr(spec, ':');

            if (dash != NULL) {
                first = last;
                last = atoi(dash + 1);
            }
            if (last <= 0)
                last = 20;
            game_startup();
            for (id = first; id <= last; id++) {
                struct timespec ts;

                fprintf(stderr, "sfx: asking for sound %d (opl key-ons so far %ld)\n",
                        id, io_keyon_count());
                play_sound((int16_t)id);
                ts.tv_sec = 1;
                ts.tv_nsec = 200000000;
                nanosleep(&ts, NULL);
            }
            return 0;
        } else {
            game_main();
        }

        if (getenv("TIM_HEADLESS") == NULL)
            sdl_hold();
        return 0;
    }

    vm_set_display_lines((uint16_t)lines);

    {
        uint8_t *fb = malloc((size_t)W * H);
        FILE *f;

        if (!fb)
            return 1;
        vga_compose(fb, W, H);
        if ((f = fopen(raw, "wb")) == NULL) {
            perror(raw);
            free(fb);
            return 1;
        }
        fwrite(fb, 1, (size_t)W * H, f);
        fclose(f);
        free(fb);
    }
    return 0;
}
