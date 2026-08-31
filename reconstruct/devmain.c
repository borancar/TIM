/*
 * Developer entry point. NOT a transcription, and NOT part of what ships.
 *
 * Everything a comparison needs lives here so that main.c stays what the
 * original's start-up was. tools/ calls this binary, never ./tim.
 *
 * With no options it **runs the game**, which is what a frame-by-frame
 * comparison needs and what this binary could not do until now: it reset the
 * machine, composed one empty frame and stopped, so every tool here drove
 * `./tim` instead - and `./tim` had to carry the developer hooks for them,
 * which is exactly what the arrangement exists to prevent. The hooks are in
 * `devdump.c` and link only here now; `tim` gets `devstub.c`.
 *
 * No SDL, deliberately. The window is registered in main.c rather than inside
 * io.c so that this binary need not link it, and nothing a comparison reads
 * comes from the window: `dev_flip_dump` composes the frame itself, from the
 * same planes, on the guest's own page flip. So a run here is headless without
 * being a different run.
 *
 *   (no options)  run the game. TIM_CLICK, TIM_POINTER, TIM_FLIPS and
 *                 TIM_FLIPHASH steer it and are documented in devdump.c.
 *   --raw FILE    write the composed frame as 8-bit palette indices, which is
 *                 what a comparison against the original's video memory needs.
 *                 A picture would throw away the index a pixel had, and two
 *                 different indices can share a colour.
 *   --lines N     program the CRTC blanking line, as the game does.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "tim.h"

#define W 640
#define H 480

int main(int argc, char **argv)
{
    const char *raw = NULL;
    int32_t lines = 399;

    for (int32_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--raw") && i + 1 < argc)
            raw = argv[++i];
        else if (!strcmp(argv[i], "--lines") && i + 1 < argc)
            lines = (int32_t)strtol(argv[++i], NULL, 0);
        else {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return 2;
        }
    }

    io_reset();

    if (!raw) {
        /* The same start-up main.c does, without the window. */
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
        setup_streams();
        io_set_timer(timer_tick);
        game_main();
        return 0;
    }

    vm_set_display_lines((uint16_t)lines);

    if (raw) {
        uint8_t *fb = malloc((size_t)W * H);
        if (!fb)
            return 1;
        vga_compose(fb, W, H);
        FILE *f = fopen(raw, "wb");
        if (!f) {
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
