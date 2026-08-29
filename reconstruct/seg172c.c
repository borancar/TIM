#include <stdio.h>
/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 172c**, image
 * 0x172c0..0x1c250. Functions are in address order and each carries the image
 * offset it was read from.
 *
 * This is where the parts live: each of the machine's fifty-odd components has
 * its own setup routine here, reached from the initialiser table in seg14de.c
 * through a far pointer the loader relocates.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * NOT a transcription: reach one part's setup by its offset in this segment.
 *
 * The original arrives by `lcall` through a relocated far pointer, which the
 * port cannot do, so the offset is dispatched here the same way the region and
 * timer handlers are. An offset with no case yet aborts and names itself, which
 * is how the ones a screen actually needs get found - the title screen reaches
 * far fewer than the forty-two the table holds.
 */
void part_setup(uint16_t off, uint16_t part)
{
    (void)part;

    switch (off) {
    default:
        break;
    }

    {
        static char what[64];

        snprintf(what, sizeof what, "the part setup at 172c:%04x", off);
        not_transcribed(what);
    }
}
