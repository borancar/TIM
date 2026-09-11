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
#define _POSIX_C_SOURCE 200809L   /* setenv, for --device and --module */

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
    while (DG4E67.state != 0x200 && DG4E67.state != 1) {
        heap_check_or_hang();

        if (DG4E67.state == 2)
            game_screen();
        else if (DG4E67.state == 0x2000)
            run_machine_loop();
        else
            game_screen_loop();
    }

    if (DG4E67.state == 0x200)
        finish_level();

    round_teardown();

    while (DG4E67.playing != 0) {
        if (((int16_t)DG4E67.state) == 1) {
            DG4E67.playing = 0;
        } else {
            DG4E67.round_number = (int16_t)(DG4E67.round_number + 1);
            if (DG4E67.round_number > DG4E67.furthest_level) {
                DG4E67.furthest_level = DG4E67.round_number;
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
 * OURS: `TIM_SAVEMACHINE=<name>` - write the restored machine out as a `.TIM`
 * file through the game's own writer, and stop.
 *
 * Why it exists: the files in `solution_snaps` are *port* snapshots - memory and
 * hardware, no CPU - so only the port can open one, and the only question that
 * can be asked of a solved puzzle is "did it solve". The hybrid cannot load
 * one at all, which was tried and abandoned: the port installs its timer by
 * dispatch and so its memory carries an empty interrupt table, and even with
 * that filled in the guest ran off into unmapped code.
 *
 * A machine *file* has none of those problems. It is what the game itself
 * writes and reads - `save_machine` at 0x1292d and `load_animation` at 0x12915
 * - so both sides can reach the same machine through the game's own loader,
 * with no CPU state to invent. The goal test comes from the level, which is
 * what `--level` already selects, so a solution is a level number and a file.
 *
 * The name goes at DGROUP 0x52fe because that is where the file picker leaves
 * it and where `save_machine` reads it; this is standing in for the picker,
 * not for the writer.
 */
static void save_machine_file(const char *name)
{
    int32_t i;

    for (i = 0; name[i] && i < (int32_t)sizeof DG52FE.name - 1; i++)
        DG52FE.name[i] = name[i];
    DG52FE.name[i] = 0;

    if (save_machine((char *)DG52FE.name) != 0)
        fprintf(stderr, "io: save_machine reported an error for %s\n", name);
    else
        fprintf(stderr, "io: wrote the machine as %s\n", name);
}

/*
 * OURS: `--level <n>` - start on a puzzle instead of on round 1.
 *
 * Four transcribed calls in the original's order with one word set between two
 * of them, and the word is the one the puzzle picker itself writes: `sub_1201d`
 * ends with `0x4ebd = 0x542a`, the row that was chosen. `game_setup` leaves
 * 0x4ebd at 1 and `round_setup` reads it a moment later to build `L<n>.LEV`,
 * so between those two is the only place the number can be put.
 *
 * `game_round` rather than `game_play`: one puzzle is what was asked for, and
 * `game_play`'s loop only exists to raise 0x4ebd and go round again. Calling
 * the transcribed round means the level load, the briefing, the play screen,
 * `finish_level` and `round_teardown` are all the original's.
 *
 * The intro still runs - it is what loads every part bitmap - and it is
 * `dev_autoplay` that ends it, not a click. See devdump.c for why that one
 * step has to write the button word.
 */
static void play_level(int32_t level)
{
    game_startup();
    game_intro();
    game_setup();
    DG4E67.round_number = (uint16_t)level;
    game_round();
    game_teardown(1);
}

/*
 * OURS: the whole of what this binary accepts, in one place.
 */
static void usage(void)
{
    printf(
"usage: devtim [--restore FILE] [--level N] [--run]\n"
"              [--raw FILE [--lines N]]\n"
"              [--device NAME] [--module NAME]\n"
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
"  --level N       play puzzle N instead of round 1, with no pointer and no\n"
"                  keyboard: the intro, the briefing and the puzzle screen\n"
"                  are all stepped through by writing the state word the\n"
"                  game's own regions write. The same as TIM_LEVEL.\n"
"  --run           start the machine as well, the way clicking the box above\n"
"                  the parts bin does - and DO NOTHING if it is already\n"
"                  running, which a restored snapshot may well be. Clicking\n"
"                  that control a second time is a stop. The same as TIM_RUN.\n"
"  --raw FILE      write the composed frame as 8-bit palette indices and exit.\n"
"                  Indices, not a picture: two of them can share a colour.\n"
"  --lines N       CRTC blanking line for --raw (default 399).\n"
"  --device NAME   which SX.OVL music overlay to load, instead of what\n"
"                  RESOURCE.CFG says. The same as TIM_DEVICE.\n"
"  --module NAME   which SX.OVL digitised-sound overlay to load. The same\n"
"                  as TIM_MODULE.\n"
"\n"
"the sound overlays, and what the two bytes of RESOURCE.CFG choose:\n"
"\n"
"  the device is the MUSIC, and is byte 1. --device or TIM_DEVICE:\n"
"    0 STD:  the PC speaker                                 (transcribed)\n"
"    1 TAN:  Tandy / PCjr three-voice\n"
"    2 ADL:  AdLib, and the FM half of every Sound Blaster   (transcribed)\n"
"    3 M32:  Roland MT-32\n"
"    4 SBP:  Sound Blaster Pro, two OPL2s in stereo          (transcribed)\n"
"    5 PS1:  IBM PS/1 audio\n"
"    6 PRO:  Pro Audio Spectrum\n"
"    7 GMD:  General MIDI\n"
"    8 NLD:  no driver - recorded as 3, and shares MT-32's bank\n"
"\n"
"  the module is the DIGITISED sound, and is byte 2. --module or TIM_MODULE:\n"
"    0 ASB:  Sound Blaster                                   (transcribed)\n"
"    1 APS:  Pro Audio Spectrum\n"
"    2 ATD:  Tandy / Disney Sound Source\n"
"    3 APA:  (not identified)\n"
"\n"
"  Either takes a name or a number, and `none` is the game's own -2. A\n"
"  Sound Blaster is ADL: and ASB: together - that is what INSTALL.COM\n"
"  writes and what the shipped RESOURCE.CFG says. The overlays marked\n"
"  transcribed are the ones the port has a body for; the rest load and\n"
"  then abort on purpose, and TIM_ABORTSNAP catches one for reading.\n"
"\n"
"  Note that the game plays no music on device 4: load_sound_bank has no\n"
"  case for it and answers null, so every music record fails to load. That\n"
"  is the original's behaviour, not the port's.\n"
"\n"
"environment, general:\n"
"  TIM_DIR=DIR     where TIM.img and TIM.unpacked.exe are (default out)\n"
"  TIM_HEADLESS=1  open no window. What the tools in tools/ set, so a batch\n"
"                  comparison needs no display; frames come from the planes\n"
"                  either way, so headless is not a different run.\n"
"  TIM_RESTORE=F   the same as --restore\n"
"  TIM_SAVEMACHINE=NAME  with --restore, write the restored machine out as\n"
"                  that .TIM file through the game's own `save_machine` and\n"
"                  stop. A machine file can be loaded by either side through\n"
"                  the game's own loader, which a port snapshot cannot: it\n"
"                  carries no CPU state and no interrupt table. Pair it with\n"
"                  --level N, which supplies the goal the machine is judged\n"
"                  against. TIM_SAVEDIR says where the bytes land.\n"
"  TIM_LOADMACHINE=NAME  load that .TIM over the level already up, once the\n"
"                  play screen is reached - the game's own round_teardown,\n"
"                  load_animation and reset_machine, in that order, which is\n"
"                  what the file picker does. Pair it with --level N for the\n"
"                  goal and --run to start the machine.\n"
"  TIM_LEVEL=N     the same as --level\n"
"  TIM_RUN=1       the same as --run. Either of these two arms the autoplay\n"
"                  driver, which reports what it did on stderr:\n"
"                    autoplay leaves the intro / the briefing\n"
"                    autoplay starts the machine\n"
"                    autoplay - the machine is running   (it was already)\n"
"                    autoplay - the puzzle is up         (--level, no --run)\n"
"  TIM_DEVICE=N    the music overlay - see the list above. Overrides byte 1\n"
"                  of RESOURCE.CFG without editing it; the file itself is\n"
"                  untouched, the guest simply reads different bytes.\n"
"  TIM_MODULE=N    the digitised-sound overlay, byte 2. The same.\n"
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
"  TIM_DATE=MM-DD  the date the game is told, instead of the fixed\n"
"                  2000-11-02 every comparison sees. Four parts are on\n"
"                  the calendar and are in no level: 02-14 the heart\n"
"                  balloon, 10-31 the pumpkin, 12-25 the christmas tree.\n"
"                  YYYY-MM-DD works too; the weekday is computed.\n"
"  TIM_LEVELSCAN=LO:HI  load each level and print the part kinds it holds\n"
"  TIM_PARTPICS=DIR  draw every part's bin icon and write it to DIR as raw\n"
"                  indexed pixels, with palette.bin beside them\n"
"  TIM_SFXALL=N    ask the game for sound identifiers 1..N and exit, so\n"
"                  every waveform reaches the card. Use with TIM_SFXDIR.\n"
"  TIM_FMDIR=DIR   with TIM_SFXALL, render the OPL while each sound plays\n"
"                  into DIR - the effects that are FM and never reach the DAC\n"
"  TIM_SFXDIR=DIR  write every distinct waveform the card plays into DIR\n"
"                  as its own WAV, named by length, rate and checksum.\n"
"  TIM_TRACE=WHAT  trace to stderr. One of:\n"
"                    speaker  the tone and the gate, as the game's own\n"
"                             driver programs the 8253 and port 0x61\n"
"                    sb       the Sound Blaster: DSP commands, resets,\n"
"                             the IRQ, and each DMA block with its rate\n"
"                    sfx      each sound effect the game asks for, by\n"
"                             identifier. Effects only - `play_sound` also\n"
"                             starts the music, and tunes are filtered out\n"
"                    mouse    button events and INT 33h queries\n"
"                    btn      the button as the guest sees it, sampled once\n"
"                             a page flip - which is the only way to see a\n"
"                             still hold, since it makes no SDL events\n"
"                    crtc     writes to the CRTC registers\n"
"                    dac      palette writes\n"
"                    level    when a puzzle is solved, which is the only\n"
"                             signal from outside that a machine worked -\n"
"                             the screen keeps painting either way\n"
"                    autoplay the state word at DGROUP 0x4e6b on every page\n"
"                             flip, with the button at 0x5774. Which screen\n"
"                             the game is on is all --level and --run reason\n"
"                             about, and the sequence is not guessable from\n"
"                             the source - print it rather than deduce it\n"
"\n"
"environment, driving a run:\n"
"  TIM_CLICK=F:X:Y[,...]   click at X,Y once flip F has been presented\n"
"  TIM_KEY=F:SCAN[:ASCII][,...]  put a key in the BIOS ring at flip F.\n"
"                          Scancodes, because that is what the game\n"
"                          reads: 45 is X and 21 is Y, the two flip axes.\n"
"  TIM_POINTER=X:Y         put the pointer there without clicking\n"
"  TIM_PARTS=FLIP:PATH     dump the parts list at FLIP, one line per\n"
"                          part, for comparison with tools/parts.py\n"
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
        else if (!strcmp(argv[i], "--level") && i + 1 < argc)
            setenv("TIM_LEVEL", argv[++i], 1);
        else if (!strcmp(argv[i], "--run"))
            setenv("TIM_RUN", "1", 1);
        else if (!strcmp(argv[i], "--device") && i + 1 < argc)
            setenv("TIM_DEVICE", argv[++i], 1);
        else if (!strcmp(argv[i], "--module") && i + 1 < argc)
            setenv("TIM_MODULE", argv[++i], 1);
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
            /*
             * A snapshot is always past the intro, and it is also the case
             * `--run` has to be careful about: the machine may already be
             * going. `dev_autoplay` reads that off 0x4e6b and says so rather
             * than clicking the run control again, which would stop it.
             */
            dev_autoplay_past_intro();
            {
                const char *out = getenv("TIM_SAVEMACHINE");

                if (out != NULL && *out) {
                    save_machine_file(out);
                    return 0;
                }
            }
            resume_from_snapshot();
        } else if (getenv("TIM_LEVEL") != NULL) {
            play_level((int32_t)strtol(getenv("TIM_LEVEL"), NULL, 0));
        } else if (getenv("TIM_LEVELSCAN") != NULL) {
            game_startup();
            dev_level_scan();
            return 0;
        } else if (getenv("TIM_PARTPICS") != NULL) {
            game_startup();
            dev_part_pics();
            return 0;
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
                /*
                 * Silence whatever is still ringing first. Without this each
                 * capture holds the tail of every effect before it - the rms
                 * climbs run to run and then plateaus, which is voices piling
                 * up rather than any property of the sound being asked for.
                 */
                stop_all_voices();
                play_sound((int16_t)id);
                if (getenv("TIM_FMDIR") != NULL) {
                    dev_fm_capture(id, 1.2);
                } else {
                    ts.tv_sec = 1;
                    ts.tv_nsec = 200000000;
                    nanosleep(&ts, NULL);
                }
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
