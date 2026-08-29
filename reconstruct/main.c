/*
 * The Incredible Machine - reconstruction. The program's entry point.
 *
 * NOT a transcription: this is the port's own start-up. It is deliberately
 * almost empty, because a DOS game has no command line - it starts, shows its
 * screens, and plays. Every developer flag lives in devmain.c and links into a
 * separate binary, so nothing a comparison depends on can become part of what
 * ships.
 *
 * What is here is what DOS did before the program's first instruction: put the
 * image in memory, relocate it, and give the program a stack and an arena. Then
 * `game_main`, which is the game's own `main` at image 0x0dfff.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dgroup.h"
#include "io.h"
#include "tim.h"

/* Where the recovered image and its relocation table are, unless TIM_DIR says
 * otherwise. They are build products of tools/unlzexe.py, not game files. */
#define DEFAULT_OUT "out"

int main(void)
{
    const char *dir = getenv("TIM_DIR");
    char img[512], exe[512];

    if (!dir)
        dir = DEFAULT_OUT;

    snprintf(img, sizeof img, "%s/TIM.img", dir);
    snprintf(exe, sizeof exe, "%s/TIM.unpacked.exe", dir);

    io_reset();

    if (!io_load_program(img, exe)) {
        fprintf(stderr,
                "cannot read %s and %s - run tools/unlzexe.py first, or set "
                "TIM_DIR\n", img, exe);
        return 1;
    }

    /*
     * What the C runtime does between the loader and `main`: its init table at
     * DGROUP 0x4e48 has a single entry, and it is `setup_streams`. Without it
     * every stream looks already-open and no file can be read.
     */
    setup_streams();

    return (int)game_main();
}
