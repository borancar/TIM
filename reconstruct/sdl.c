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

void sdl_pump(void)
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT
            || (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE))
            sdl_die();

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
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !grabbed) {
            set_grab(1);
            continue;           /* the click that takes the pointer is not the
                                 * game's; DOSBox does the same */
        }

        if (e.type == SDL_EVENT_MOUSE_MOTION
            || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN
            || e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            uint32_t held = SDL_GetMouseState(NULL, NULL);
            uint16_t buttons = 0;

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

            if (held & SDL_BUTTON_LMASK)
                buttons |= 1;
            if (held & SDL_BUTTON_RMASK)
                buttons |= 2;

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
