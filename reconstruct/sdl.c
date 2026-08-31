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
    _exit(0);
}

static void die_on_signal(int sig)
{
    (void)sig;
    sdl_die();
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
     * Every presented frame, as indices and palette, when TIM_FRAMES names a
     * directory. That is what a frame-by-frame comparison against the original
     * needs: the original's captures come one per page flip, and the port's
     * come one per refresh, so the two are matched by content rather than by
     * number - see tools/compare_port.py.
     */
    {
        static const char *dir;
        static int32_t once, n;
        char path[512];
        FILE *f;

        if (!once) {
            once = 1;
            dir = getenv("TIM_FRAMES");
        }
        if (dir && !holding) {
            snprintf(path, sizeof path, "%s/f%05d.raw", dir, n);
            f = fopen(path, "wb");
            if (f) {
                fwrite(indices, 1, (size_t)W * H, f);
                fclose(f);
            }
            snprintf(path, sizeof path, "%s/f%05d.pal", dir, n);
            f = fopen(path, "wb");
            if (f) {
                fwrite(pal, 1, sizeof pal, f);
                fclose(f);
            }
            n++;
        }
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
 * Let the window answer the desktop. A DOS game never had to do this, so there
 * is nothing here that corresponds to anything in the original - without it the
 * window is reported as not responding and cannot be closed.
 */
void sdl_pump(void)
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT
            || (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE))
            sdl_die();
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
     * Write the frame out as palette *indices* if TIM_FRAME names a file. An
     * index is what a comparison against the original needs and a colour is
     * not: two different indices can be the same colour, and a PNG of the two
     * would agree where the planes do not. tools/diff_png.py takes it from
     * here.
     */
    const char *dump = getenv("TIM_FRAME");

    if (dump && indices) {
        FILE *f;

        vga_compose(indices, W, H);
        f = fopen(dump, "wb");
        if (f) {
            fwrite(indices, 1, (size_t)W * H, f);
            fclose(f);
            fprintf(stderr, "wrote %dx%d indices to %s\n", W, H, dump);
        }

        /*
         * And the DAC beside it, as 768 bytes of 8-bit RGB. The indices say
         * what was drawn; the palette says whether any of it is visible, and
         * the two questions are worth being able to ask separately - a frame
         * that is right and a screen that is black is a palette fault, not a
         * drawing one.
         */
        {
            char pal_path[512];
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
     * A frame dump is a batch job - tools/compare_port.py wants the file and
     * the exit, not a window to look at - so asking for one says "and then
     * finish". Holding is for a person.
     */
    if (dump)
        return;

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
