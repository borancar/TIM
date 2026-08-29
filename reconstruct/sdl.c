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

#include <SDL3/SDL.h>

#include "io.h"
#include "sdl.h"

/*
 * 640x400, which is what the game actually displays: it sets BIOS mode 0x12
 * and then moves the CRTC's blanking line up, so the card still scans 480 and
 * the bottom eighty rows are blank. See docs/executable.md.
 */
#define W 640
#define H 400

static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;
static uint8_t      *indices;      /* one palette index a pixel */
static uint32_t     *pixels;       /* what the texture wants */
static int32_t       running;

int32_t sdl_open(void)
{
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
    if (!SDL_CreateWindowAndRenderer("The Incredible Machine", W, H,
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

    /* The game's pixels are square-ish and chunky; keep them so. */
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(renderer, W, H,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

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
        if (dir) {
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

    SDL_UpdateTexture(texture, NULL, pixels, W * (int)sizeof *pixels);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

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
            || (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)) {
            sdl_close();
            exit(0);
        }
    }
}

void sdl_close(void)
{
    if (!running)
        return;
    running = 0;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
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
    for (;;) {
        sdl_present();
        SDL_Delay(30);
    }
}
