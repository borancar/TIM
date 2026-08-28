/*
 * Developer entry point. NOT a transcription, and NOT part of what ships.
 *
 * Everything a comparison needs lives here so that main.c stays what the
 * original's start-up was. tools/ calls this binary, never ./tim.
 *
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
