/*
 * The Incredible Machine - reconstruction. The program's entry point.
 *
 * NOT a transcription: this is the port's own start-up. It is deliberately
 * almost empty, because a DOS game has no command line - it starts, shows its
 * screens, and plays. Every developer flag lives in devmain.c and links into a
 * separate binary, so nothing a comparison depends on can become part of what
 * ships.
 */
#include "io.h"
#include "tim.h"

int main(void)
{
    io_reset();
    /*
     * The game proper is not wired up yet. Until it is, say so rather than
     * opening a window on an empty frame - see STATUS.md for what is done.
     */
    return 0;
}
