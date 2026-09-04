/*
 * The Incredible Machine - reconstruction. The window.
 *
 * NOT a transcription, and not a reconstruction of anything: the original drew
 * by writing to an EGA-compatible card, and there is no card here. This is the
 * port's own backend and it is deliberately the only one - the file writer in
 * devmain.c is a *mode* of the same composed frame, never a second path.
 *
 * It is driven by the guest's own page-flip cue, the write to CRTC 0x0C that
 * changes which half of video memory is scanned out. That is the same cue
 * tools/capture.py uses to take reference frames, which is what makes a frame
 * here and a frame there the same frame.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <SDL3/SDL.h>

#include "io.h"
#include "sdl.h"

/*
 * 640x480 - the whole frame the card scans in mode 0x12 - and **not** 640x400.
 *
 * The game moves the CRTC's start-of-vertical-blank to say how much of that
 * frame is picture, and it does not always say the same thing: 0x18f for the
 * intro screens, 0x1bf for one of them, and **0x1d6 - 470 rows - for the
 * Sierra logo**. A 400-row buffer composes the first 400 of those and the
 * logo loses its bottom seventy.
 *
 * So the buffer is the full frame and `vga_visible_lines` decides how much of
 * it is put on the screen, which is what the hardware does: the card scans 480
 * lines either way and the monitor shows the ones before the blank.
 *
 * The **window follows the line count**, and the picture is never scaled to fit
 * a window of the wrong height: a row of the frame is a row of the window. When
 * the game moves the blanking line the window is resized to match, which is a
 * monitor changing mode and is what the player saw. The window opens at the 400
 * the intros use and grows to 471 for the logo - the two sizes DOSBox reports
 * for the same two screens.
 */
#define W 640
#define H 480

/*
 * Everything is shown at double size except the Sierra logo, which is shown as
 * it is. Ours, both of them: the original had a monitor and no say in this.
 *
 * **The rule names the logo, not the game**, and that is the second attempt.
 * The first asked "is the picture 400 lines" and doubled if so, on the reading
 * that 400 was what the game used and 471 was the logo. That is wrong, and a
 * snapshot of the level-1 briefing is what showed it: the game programs a
 * *third* height, 0x1bf, and its own screens are 448 lines, not 400. Under the
 * old rule the whole game would have appeared at half the size of its intro.
 *
 * So the exception is written as the exception. A height this file has never
 * seen is doubled like everything else, which is the right way round for a
 * rule with one known member.
 */
#define LOGO_LINES 471          /* 0x1d6 + 1, the Sierra logo and only that */
#define GAME_SCALE 2
#define WIN_W (W * GAME_SCALE)
#define WIN_H (400 * GAME_SCALE)   /* the intro's height, for the first frame */

static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;
static uint8_t      *indices;      /* one palette index a pixel */
static uint32_t     *pixels;       /* what the texture wants */
static int32_t       running;
static int32_t       holding;

/*
 * The one way out, and it is `_exit`. A DOS game had nothing to answer to; a
 * process on this machine has to die when it is told to, from wherever it
 * happens to be.
 *
 * SDL turns a signal into an `SDL_EVENT_QUIT`, which only reaches `sdl_pump`
 * when a frame is presented - so a run busy anywhere else ignored `timeout`'s
 * SIGTERM and stayed alive for hours, and the tools that drive the port left a
 * process behind on every call. Hence a plain handler as well.
 *
 * `_exit`, because two ways out of a program are two teardowns that will
 * differ: running SDL's shutdown and then `exit` takes whatever `atexit` has
 * registered over the same handles, on one path and not the other. The window
 * manager reclaims a window from a process that is gone.
 *
 * Ours, and not a transcription.
 */
void sdl_die(void)
{
    /*
     * Give the pointer back on the way out. A window that exits still holding
     * it leaves the user with no mouse and no obvious reason why - and this
     * path is `_exit`, so nothing else is going to run.
     */
    if (window)
        SDL_SetWindowRelativeMouseMode(window, false);
    _exit(0);
}

static void die_on_signal(int sig)
{
    (void)sig;
    sdl_die();
}

/*
 * OURS: the PC speaker, as a square wave through SDL's audio.
 *
 * The game's own driver - `sxovl_spkr.c`, transcribed from the STD: chunk of
 * SX.OVL - programs channel 2 of the 8253 and gates it onto the speaker, and
 * `io_on_speaker` hands the two facts that come out of that here: the tone in
 * hertz and whether the gate is closed. Everything below is the loudspeaker
 * the original had and this program does not.
 *
 * **A square wave and nothing cleverer.** That is what the hardware made: the
 * timer's output pin drove the cone directly, so the waveform is a square at
 * the divisor's frequency and the only volume is on or off. Half amplitude
 * because a full-scale square through modern speakers is unpleasant, which is
 * a judgement about listening rather than about the machine.
 *
 * The phase is kept as a fraction of a period and advanced per sample, so a
 * frequency that changes between buffers does not click: the wave carries on
 * from where it was rather than restarting.
 */
#define SPK_RATE 44100

static SDL_AudioStream *audio;
static double spk_hz;                   /* written by the guest's thread */
static int32_t spk_on;                  /* read by SDL's audio thread */
static double spk_phase;

static void SDLCALL feed_audio(void *userdata, SDL_AudioStream *stream,
                               int32_t additional, int32_t total)
{
    static int16_t buf[1024];
    double hz = spk_hz;
    int32_t on = spk_on;

    (void)userdata;
    (void)total;

    while (additional > 0) {
        int32_t want = additional > (int32_t)sizeof buf
                       ? (int32_t)sizeof buf : additional;
        int32_t n = want / (int32_t)sizeof buf[0];
        int32_t i;

        for (i = 0; i < n; i++) {
            if (!on || hz < 20.0 || hz > 20000.0) {
                buf[i] = 0;
                continue;
            }
            buf[i] = (spk_phase < 0.5) ? 8000 : -8000;
            spk_phase += hz / (double)SPK_RATE;
            if (spk_phase >= 1.0)
                spk_phase -= (double)(int32_t)spk_phase;
        }

        SDL_PutAudioStreamData(stream, buf, n * (int32_t)sizeof buf[0]);
        additional -= n * (int32_t)sizeof buf[0];
    }
}

/*
 * OURS: the OPL2's output, on its own stream.
 *
 * The chip runs at 49716 Hz - a YM3812 divides its 3.579545 MHz colourburst
 * crystal by 72 - and the port renders at that rate and lets SDL convert,
 * because resampling here would be a second place for the sound to be wrong.
 * See src/opl.h.
 *
 * Pulled rather than pushed: `opl_render` generates on demand from the
 * callback, which is what keeps it in step with the register writes the guest
 * is making rather than with anything this file decides.
 */
#include "src/opl.h"

/* The FM's level against the digitised stream - see `feed_opl`. */
#define FM_GAIN_NUM 1
#define FM_GAIN_DEN 2

static SDL_AudioStream *opl_stream;

static void SDLCALL feed_opl(void *userdata, SDL_AudioStream *stream,
                             int32_t additional, int32_t total)
{
    static int16_t buf[1024];
    int32_t i;

    (void)userdata;
    (void)total;

    while (additional > 0) {
        int32_t want = additional > (int32_t)sizeof buf
                       ? (int32_t)sizeof buf : additional;
        int32_t n = want / (int32_t)sizeof buf[0];

        if (n <= 0)
            break;

        opl_render(buf, (uint32_t)n);

        /*
         * OURS, and a judgement rather than a measurement of the original.
         *
         * A real Sound Blaster mixes FM and DAC through the card's mixer chip,
         * and this game never programs it - so there is no level in the
         * original to transcribe, and the two streams would otherwise both
         * arrive at whatever full scale their own format gives.
         *
         * Measured over the intro: the OPL runs at rms 752, peak 9166, and the
         * digitised blocks at rms 2603, peak 32768. On those numbers the
         * effects are the louder of the two - but the OPL's figure is over all
         * time, silence included, and the music plays *continuously* where an
         * effect is two tenths of a second. Sustained sound is heard as louder
         * than a transient of the same amplitude, which is why the music
         * dominates in practice.
         *
         * So the FM is attenuated here. The number is chosen by ear, and one
         * line is all there is to change if it wants to be different.
         */
        for (i = 0; i < n; i++)
            buf[i] = (int16_t)(buf[i] * FM_GAIN_NUM / FM_GAIN_DEN);

        SDL_PutAudioStreamData(stream, buf, n * (int32_t)sizeof buf[0]);
        additional -= n * (int32_t)sizeof buf[0];
    }
}

static void opl_open(void)
{
    SDL_AudioSpec in;

    if (opl_stream != NULL)
        return;

    in.format = SDL_AUDIO_S16;
    in.channels = 1;
    in.freq = (int)OPL_SAMPLE_RATE;

    opl_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                           &in, feed_opl, NULL);
    if (opl_stream != NULL)
        SDL_ResumeAudioStreamDevice(opl_stream);
}

/*
 * OURS: a block of PCM the Sound Blaster was handed, put on its own stream.
 *
 * A second stream rather than mixing by hand: SDL converts format and rate per
 * stream, and the card's sample rate is whatever DSP command 0x40 set, which
 * is not the device's. Both streams feed the same device and SDL sums them, so
 * a sound effect over the speaker's tone comes out as both - which is not a
 * combination any real machine made, and is the honest consequence of the port
 * being able to have two devices at once where the original had one.
 *
 * The stream is opened on the first block, because the rate is not known until
 * then, and reopened if the rate changes.
 */
static SDL_AudioStream *pcm;
static int32_t pcm_rate;

static void pcm_play(const uint8_t *buf, int32_t n, int32_t rate)
{
    if (pcm != NULL && rate != pcm_rate) {
        SDL_DestroyAudioStream(pcm);
        pcm = NULL;
    }

    if (pcm == NULL) {
        SDL_AudioSpec in;

        in.format = SDL_AUDIO_U8;       /* the card's own format */
        in.channels = 1;
        in.freq = rate;

        pcm = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        &in, NULL, NULL);
        if (pcm == NULL)
            return;

        pcm_rate = rate;
        SDL_ResumeAudioStreamDevice(pcm);
    }

    SDL_PutAudioStreamData(pcm, buf, n);
}

/*
 * OURS: what `io_on_speaker` calls. Two plain writes, read by the audio thread
 * without a lock - see the note beside the state in io.c for why that is the
 * same deferred question as the timer's and not a new one.
 */
static void speaker_set(double hz, int32_t on)
{
    spk_hz = hz;
    spk_on = on;
}

/*
 * OURS: open the audio device, and carry on without it if there is none.
 *
 * A machine with no sound card, or a build running headless, still has to play
 * the game - so this is not allowed to be a reason to fail. `io_on_speaker` is
 * only registered once a device is actually open, so a silent run costs
 * nothing but the silence.
 */
static void audio_open(void)
{
    SDL_AudioSpec spec;

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "no audio: %s (the game will be silent)\n",
                SDL_GetError());
        return;
    }

    spec.format = SDL_AUDIO_S16;
    spec.channels = 1;
    spec.freq = SPK_RATE;

    audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                      &spec, feed_audio, NULL);
    if (audio == NULL) {
        fprintf(stderr, "no audio stream: %s (the game will be silent)\n",
                SDL_GetError());
        return;
    }

    SDL_ResumeAudioStreamDevice(audio);
    io_on_speaker(speaker_set);
    io_on_pcm(pcm_play);
    opl_open();
}

int32_t sdl_open(void)
{
    signal(SIGTERM, die_on_signal);
    signal(SIGINT, die_on_signal);
    signal(SIGHUP, die_on_signal);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 0;
    }

    /*
     * **Utility, and not resizable.** A DOS game has one resolution and no
     * concept of being resized, and a plain resizable window gives a tiling
     * window manager no reason not to tile it - which is what happened, and it
     * stretches 640x400 into whatever cell it lands in. `SDL_WINDOW_UTILITY`
     * is the hint that says "float this", and dropping RESIZABLE says the same
     * thing again in the only terms some managers read.
     */
    if (!SDL_CreateWindowAndRenderer("The Incredible Machine", WIN_W, WIN_H,
                                     SDL_WINDOW_UTILITY,
                                     &window, &renderer)) {
        fprintf(stderr, "SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return 0;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        return 0;
    }

    /* The game's pixels are square and chunky; keep them so. */
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    /*
     * **No logical presentation.** There was a
     * `SDL_SetRenderLogicalPresentation(renderer, 640, 480, LETTERBOX)` here,
     * and it did two things neither of which was wanted once the window
     * started being sized to the picture.
     *
     * It made the renderer pretend its output was 640x480 - a 4:3 box - and
     * letterbox that into the real window. In a 1280x800 window the picture
     * came out 1066 wide with black bars either side, because 800 * 4/3 is
     * 1066, and 640 to 1066 is a scale of 1.666: every third column a pixel
     * wider than its neighbours. That is the horizontal squeeze and the loss
     * of sharpness, and it is why the reference frame and the window did not
     * look alike even though they agree byte for byte.
     *
     * It also stretched the game's 400 rows over 480 logical ones before any
     * of that.
     *
     * The window is now made an exact multiple of the picture, so the picture
     * is drawn straight into it and every source pixel is a whole square of
     * window pixels.
     */

    indices = malloc((size_t)W * H);
    pixels = malloc((size_t)W * H * sizeof *pixels);
    if (!indices || !pixels)
        return 0;

    running = 1;

    /* The speaker, last, because a machine with no sound card must still play. */
    audio_open();

    return 1;
}

/*
 * Compose the frame the CRTC would be scanning out and put it on the screen.
 *
 * The composition is `vga_compose`, which is the port's one answer to "what is
 * on the screen": it reads the planes through the same start address and
 * blanking line the guest set, and answers palette *indices*. The colours come
 * from the DAC separately, because an index is what a comparison against the
 * original needs and a colour is not.
 */
void sdl_present(void)
{
    uint8_t pal[768];
    int32_t i;

    if (!running)
        return;

    vga_compose(indices, W, H);
    vga_palette_rgb(pal);

    for (i = 0; i < W * H; i++) {
        const uint8_t *c = pal + 3 * indices[i];

        pixels[i] = (uint32_t)((c[0] << 16) | (c[1] << 8) | c[2]);
    }


    /*
     * Only the rows before the blanking line are picture, and the window is
     * made to fit exactly that many rows before they are drawn - so the source
     * rectangle and the window agree and every pixel is a whole number of
     * window pixels.
     *
     * **The game's own screens are doubled and the Sierra logo is not.** That
     * is a choice about this window and nothing to do with the original: the
     * game asks for 400 rows and the logo for 471, and `GAME_SCALE` applies to
     * the first and not the second. Doubling is exact - the texture filter is
     * nearest, so a doubled pixel is four identical pixels and no edge is
     * blurred.
     *
     * **The size is checked against the window, not against the last request.**
     * `SDL_SetWindowSize` is a request, and under Wayland it is answered a
     * frame or more later: `SDL_GetWindowSize` straight after the call still
     * reports the old size. Remembering what was last asked for and only
     * asking again when the line count changed lost the Sierra logo entirely -
     * the game programs 471 lines at the second frame and 400 at the
     * forty-first, the compositor was still catching up with the first request
     * when the second arrived, and the logo played out in a 400-row window
     * while the 471 landed just in time for the screen that did not want it.
     *
     * Comparing against the size the window actually has re-sends the request
     * until it is honoured, so a slow compositor costs a frame rather than the
     * whole screen.
     *
     * **And then it stops.** Asking every frame for as long as the sizes differ
     * would fight the person using the program: drag the window bigger and each
     * frame drags it back. So the asking is bounded - a second of frames after
     * the line count changes, which is far longer than a compositor needs and
     * shorter than anyone can resize a window in. After that the window is
     * theirs, and the picture is scaled into whatever size they chose.
     */
    {
        static int32_t asked_for = -1;   /* the line count last asked about */
        static int32_t asking;           /* frames left to keep asking for it */
        int32_t        vis = vga_visible_lines();
        int32_t        scale;
        int            want_w, want_h;   /* SDL's own type, at its edge */
        int            have_w = 0, have_h = 0;
        SDL_FRect      src;

        if (vis <= 0 || vis > H)
            vis = H;

        scale  = (vis == LOGO_LINES) ? 1 : GAME_SCALE;
        want_w = (int)(W * scale);
        want_h = (int)(vis * scale);

        /*
         * Until the game moves the blanking line the whole 480 is picture, and
         * that is the BIOS's state rather than anything the game asked for.
         * Resizing to it puts a 640x480 flicker between the window opening and
         * the logo's 471, so the window is simply left alone until the game has
         * said something about it.
         */
        if (vis >= H)
            asked_for = vis;
        else if (vis != asked_for) {
            asked_for = vis;
            asking = 60;
        }

        if (asking > 0) {
            SDL_GetWindowSize(window, &have_w, &have_h);
            if (have_w != want_w || have_h != want_h)
                SDL_SetWindowSize(window, want_w, want_h);
            else
                asking = 0;              /* honoured; the window is theirs now */
            if (asking > 0)
                asking--;
        }

        src.x = 0.0f;
        src.y = 0.0f;
        src.w = (float)W;
        src.h = (float)vis;

        SDL_UpdateTexture(texture, NULL, pixels, W * (int)sizeof *pixels);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, &src, NULL);
        SDL_RenderPresent(renderer);
    }

    sdl_pump();
}

/*
 * How many rows of the frame are picture, as `sdl_present` decides it. The
 * pump needs the same number to turn a window coordinate into a game one.
 */
static int32_t shown_lines(void)
{
    int32_t vis = vga_visible_lines();

    return (vis <= 0 || vis > H) ? H : vis;
}

/*
 * Let the window answer the desktop. A DOS game never had to do this, so there
 * is nothing here that corresponds to anything in the original - without it the
 * window is reported as not responding and cannot be closed.
 */
/*
 * Whether the window has the pointer, and where the game's pointer is.
 *
 * Kept here rather than asked of SDL each time because the two are not the
 * same question: SDL knows whether the host pointer is captured, and this is
 * the game's pointer, which carries on from where it was when the grab
 * changed.
 */
static int32_t grabbed;
static int32_t ptr_x, ptr_y;

static void set_grab(int32_t on)
{
    if (!window || grabbed == on)
        return;
    if (!SDL_SetWindowRelativeMouseMode(window, on != 0)) {
        fprintf(stderr, "SDL_SetWindowRelativeMouseMode: %s\n", SDL_GetError());
        return;
    }
    grabbed = on;
}

static void (*hotkey_hook)(int32_t id);

void sdl_on_hotkey(void (*fn)(int32_t id))
{
    hotkey_hook = fn;
}

/*
 * OURS: SDL's keycodes to the PC's set-1 scancodes, which is what the game
 * reads. `bios_read_key` hands back scancode-in-the-high-byte, and every table
 * in the game is indexed by that: 45 is X and 21 is Y for the two flip axes,
 * 0x2f is V, 0x1c is Enter, 0x0f is Tab.
 *
 * Only the keys the game looks at are here. A key with no row is not passed
 * on, which is right: the original's keyboard handler filled the same ring
 * from the same hardware and the game ignored the rest.
 */
static const struct { int32_t key; uint8_t scan; char ascii; } KEYMAP[] = {
    { SDLK_ESCAPE, 0x01, 27 },   { SDLK_1, 0x02, '1' },  { SDLK_2, 0x03, '2' },
    { SDLK_3, 0x04, '3' },       { SDLK_4, 0x05, '4' },  { SDLK_5, 0x06, '5' },
    { SDLK_6, 0x07, '6' },       { SDLK_7, 0x08, '7' },  { SDLK_8, 0x09, '8' },
    { SDLK_9, 0x0a, '9' },       { SDLK_0, 0x0b, '0' },
    { SDLK_MINUS, 0x0c, '-' },   { SDLK_EQUALS, 0x0d, '=' },
    { SDLK_BACKSPACE, 0x0e, 8 }, { SDLK_TAB, 0x0f, 9 },
    { SDLK_Q, 0x10, 'Q' }, { SDLK_W, 0x11, 'W' }, { SDLK_E, 0x12, 'E' },
    { SDLK_R, 0x13, 'R' }, { SDLK_T, 0x14, 'T' }, { SDLK_Y, 0x15, 'Y' },
    { SDLK_U, 0x16, 'U' }, { SDLK_I, 0x17, 'I' }, { SDLK_O, 0x18, 'O' },
    { SDLK_P, 0x19, 'P' }, { SDLK_RETURN, 0x1c, 13 },
    { SDLK_A, 0x1e, 'A' }, { SDLK_S, 0x1f, 'S' }, { SDLK_D, 0x20, 'D' },
    { SDLK_F, 0x21, 'F' }, { SDLK_G, 0x22, 'G' }, { SDLK_H, 0x23, 'H' },
    { SDLK_J, 0x24, 'J' }, { SDLK_K, 0x25, 'K' }, { SDLK_L, 0x26, 'L' },
    { SDLK_Z, 0x2c, 'Z' }, { SDLK_X, 0x2d, 'X' }, { SDLK_C, 0x2e, 'C' },
    { SDLK_V, 0x2f, 'V' }, { SDLK_B, 0x30, 'B' }, { SDLK_N, 0x31, 'N' },
    { SDLK_M, 0x32, 'M' }, { SDLK_SPACE, 0x39, ' ' },
    { SDLK_F1, 0x3b, 0 },  { SDLK_F2, 0x3c, 0 },  { SDLK_F3, 0x3d, 0 },
    { SDLK_F4, 0x3e, 0 },  { SDLK_F5, 0x3f, 0 },  { SDLK_F6, 0x40, 0 },
    { SDLK_F7, 0x41, 0 },  { SDLK_F8, 0x42, 0 },  { SDLK_F9, 0x43, 0 },
    { SDLK_F10, 0x44, 0 },
    { SDLK_UP, 0x48, 0 },  { SDLK_LEFT, 0x4b, 0 },
    { SDLK_RIGHT, 0x4d, 0 }, { SDLK_DOWN, 0x50, 0 },
};

/* OURS: the buttons as the events say they are, not as they are now. */
static uint16_t held_buttons;

/*
 * OURS: a release held back to the next pump, and why one is needed.
 *
 * **Events are drained once a frame, and that collapses time.** On the machine
 * this is reconstructing, the mouse driver interrupted on the press and again
 * on the release, and the 200 milliseconds a person holds a button sat between
 * the two - about fifty ticks of the game's own 236 Hz timer, which samples
 * the button level and is the only way the game learns anything happened.
 *
 * Here both events come out of one `SDL_PollEvent` loop microseconds apart, so
 * the level goes up and back down between two ticks and the timer sees
 * nothing. The click is not mistimed, it is *gone*, and a control that will
 * not answer a tap is one you end up holding down - which auto-repeats, which
 * is what this was reported as.
 *
 * So a release that arrives in the same pump as its own press is deferred to
 * the next one. That is not a debounce invented for the port; it restores the
 * minimum a press lasted on the hardware, and one pump is about eight of the
 * guest's ticks where the hardware gave it fifty. A release in any later pump
 * is delivered where it falls.
 */
static uint16_t deferred_release;

void sdl_pump(void)
{
    SDL_Event e;
    uint16_t pressed_this_pump = 0;

    if (deferred_release != 0) {
        held_buttons = (uint16_t)(held_buttons & ~deferred_release);
        deferred_release = 0;
        io_mouse_input(ptr_x, ptr_y, held_buttons);
    }

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT
            || (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE))
            sdl_die();

        /*
         * **Shift+F2 snapshots the machine.** Shifted so it cannot be reached
         * by a game that wants F2 for itself, and taken here rather than after
         * the guest has seen it because a developer key is not input: it is
         * consumed, and `continue` is what makes that true.
         */
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_F2
            && (SDL_GetModState() & SDL_KMOD_SHIFT)) {
            if (hotkey_hook)
                hotkey_hook(SDL_HOTKEY_SNAPSHOT);
            continue;
        }

        /*
         * **Ctrl+Alt hands the pointer back**, which is the gesture DOSBox
         * trained everyone to reach for. A grab without a release is a bug:
         * without this the window owns the mouse until the process ends.
         */
        if (e.type == SDL_EVENT_KEY_DOWN
            && (e.key.key == SDLK_LCTRL || e.key.key == SDLK_RCTRL
                || e.key.key == SDLK_LALT || e.key.key == SDLK_RALT)) {
            SDL_Keymod mod = SDL_GetModState();

            if ((mod & SDL_KMOD_CTRL) && (mod & SDL_KMOD_ALT))
                set_grab(0);
        }

        /*
         * The pointer, in the game's own pixels rather than the window's.
         *
         * **Grabbed, and relative.** A DOS game that uses the mouse owns it:
         * it hides the driver's pointer, sets its own range in its own
         * coordinates and draws its own cursor. A window that lets the host
         * pointer wander out while the game still thinks it is moving does not
         * reproduce that - the game's cursor stops at the edge of the desktop
         * instead of at the edge of the range the game set, and the two
         * disagree about where the pointer is. So the motion is taken as
         * `xrel`/`yrel` and accumulated here, which is also the shape the
         * original works in: it asks the driver for *mickeys* and sets the
         * mickey-to-pixel ratio itself.
         *
         * Ungrabbed, the absolute position is used instead, so the window is
         * still usable before the first click and after Ctrl+Alt.
         * `io_mouse_input` does the rest - the driver's quarter-pixel units,
         * the range the game fenced the pointer into, and whether the event is
         * one the game asked to hear about.
         */
        /*
         * **The keyboard, which the port did not have.** Shift+F2 and Ctrl+Alt
         * are taken above and never reach the game; everything else with a row
         * in KEYMAP goes into the BIOS ring the way a keyboard interrupt would
         * have put it there.
         */
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            size_t i;

            for (i = 0; i < sizeof KEYMAP / sizeof KEYMAP[0]; i++)
                if (KEYMAP[i].key == (int32_t)e.key.key) {
                    io_key_press((uint16_t)((KEYMAP[i].scan << 8)
                                            | (uint8_t)KEYMAP[i].ascii));
                    break;
                }
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !grabbed) {
            set_grab(1);
            continue;           /* the click that takes the pointer is not the
                                 * game's; DOSBox does the same */
        }

        if (e.type == SDL_EVENT_MOUSE_MOTION
            || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN
            || e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            uint16_t buttons;

            if (grabbed) {
                if (e.type == SDL_EVENT_MOUSE_MOTION) {
                    ptr_x += (int32_t)e.motion.xrel;
                    ptr_y += (int32_t)e.motion.yrel;
                }
            } else {
                float fx = 0.0f, fy = 0.0f;
                int   ww = W, wh = H;

                SDL_GetMouseState(&fx, &fy);
                SDL_GetWindowSize(window, &ww, &wh);
                ptr_x = (ww > 0) ? (int32_t)(fx * (float)W / (float)ww) : 0;
                ptr_y = (wh > 0)
                        ? (int32_t)(fy * (float)shown_lines() / (float)wh) : 0;
            }

            /*
             * Fenced to the picture. The game fences it again, in its own
             * units and to whatever range it set - this only stops the
             * accumulator running away while the host pointer is held against
             * the edge of nothing.
             */
            if (ptr_x < 0)
                ptr_x = 0;
            else if (ptr_x > W - 1)
                ptr_x = W - 1;
            if (ptr_y < 0)
                ptr_y = 0;
            else if (ptr_y > shown_lines() - 1)
                ptr_y = shown_lines() - 1;

            /*
             * **The state comes from the event, not from SDL_GetMouseState.**
             * That call answers about *now*, and these events are a queue: a
             * press and a release drained in the same pump both read whatever
             * the button happens to be doing at pump time, which is neither of
             * the two moments being reported. A click quicker than one frame
             * then arrives as two events that both say "up", and the guest
             * never sees it go down at all - so the tap does nothing, and the
             * only way to work the control is to hold it, which auto-repeats.
             *
             * `held_buttons` is stepped by the events themselves and a motion
             * event's own `state` re-syncs it, so press and release keep both
             * their order and their meaning.
             */
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                || e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                uint16_t bit = 0;

                if (e.button.button == SDL_BUTTON_LEFT)
                    bit = 1;
                else if (e.button.button == SDL_BUTTON_RIGHT)
                    bit = 2;

                if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    held_buttons |= bit;
                    pressed_this_pump |= bit;
                } else if (pressed_this_pump & bit) {
                    deferred_release |= bit;    /* see the note above */
                } else {
                    held_buttons = (uint16_t)(held_buttons & ~bit);
                }
            } else if (deferred_release == 0) {
                held_buttons = 0;
                if (e.motion.state & SDL_BUTTON_LMASK)
                    held_buttons |= 1;
                if (e.motion.state & SDL_BUTTON_RMASK)
                    held_buttons |= 2;
            }

            buttons = held_buttons;

            io_mouse_input(ptr_x, ptr_y, buttons);
        }
    }
}

/*
 * Hold the last frame on the screen until the window is closed. The port stops
 * at the first routine it has not got yet, and without this the window would
 * vanish with the process before anyone could see what had been drawn.
 */
void sdl_hold(void)
{
    /*
     * The frame the port stopped on used to be written here, under TIM_FRAME,
     * and the frames it presented under TIM_FRAMES. Both were developer flags
     * in the shipping binary, which is what the Makefile's rule forbids. The
     * first is `dev_final_frame` in devdump.c now, registered by devmain.c as
     * the abort hook; the second is gone, superseded by TIM_FLIPS, which names
     * a frame by the flip it was composed for so nothing has to be matched by
     * content.
     */

    /*
     * There is no "and then finish" case here any more. It existed because a
     * frame dump is a batch job and wants the file and the exit rather than a
     * window - but the dump has moved to `devtim`, which has no window to hold,
     * so what is left of this is only ever for a person to look at.
     */
    if (!running)
        return;

    /*
     * Holding is not presenting: without this the frame directory fills with
     * hundreds of copies of the frame the port stopped on, and
     * tools/compare_frames.py then reports a run as far longer than it was.
     */
    holding = 1;
    for (;;) {
        sdl_present();
        SDL_Delay(30);
    }
}
