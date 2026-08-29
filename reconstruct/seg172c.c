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
 * The **part setups**, fourteen of the thirty-nine.
 *
 * Each writes a list of byte pairs into the four-bytes-per-bitmap array the
 * initialiser allocated at +0x82 - an x and a y at +0 and +1 of each slot, the
 * other two bytes left alone - and then calls one routine with the part.
 *
 * The pairs are the part's connection points: where a rod or a rope may be
 * attached to each of its bitmaps. Fourteen setups are exactly that and nothing
 * else, and they go in as the table they are, each row carrying its own offset
 * in this segment.
 *
 * The other twenty-five do more - loops, tests on the part's flags - and are
 * not in here. `part_setup` names any it does not have.
 */
static const struct {
    uint16_t off;               /* this setup's offset in segment 172c */
    uint16_t finish;            /* the routine it ends by calling */
    uint8_t  n;                 /* how many pairs */
    uint8_t  xy[2 * 16];
} part_setups[13] = {
    { 0x0001, 0x05d1e,  8, { 0x08, 0x00, 0x17, 0x00, 0x1f, 0x08, 0x1f, 0x17, 0x17, 0x1f, 0x08, 0x1f, 0x00, 0x17, 0x00, 0x08 } },
    { 0x0065, 0x05d1e,  8, { 0x07, 0x00, 0x0f, 0x00, 0x16, 0x08, 0x16, 0x0f, 0x0e, 0x16, 0x08, 0x16, 0x00, 0x0f, 0x00, 0x08 } },
    { 0x00c9, 0x05d1e,  8, { 0x03, 0x00, 0x0b, 0x00, 0x0e, 0x04, 0x0e, 0x0a, 0x0b, 0x0e, 0x03, 0x0e, 0x00, 0x0a, 0x00, 0x04 } },
    { 0x07b2, 0x05d1e,  6, { 0x00, 0x13, 0x0a, 0x28, 0x19, 0x28, 0x24, 0x14, 0x1b, 0x2f, 0x08, 0x2f } },
    { 0x0950, 0x05d1e,  3, { 0x08, 0x1f, 0x0e, 0x16, 0x15, 0x1f } },
    { 0x0f70, 0x05d1e, 12, { 0x00, 0x18, 0x13, 0x00, 0x19, 0x00, 0x2d, 0x18, 0x2d, 0x3f, 0x2b, 0x3f, 0x2b, 0x1a, 0x22, 0x10, 0x0b, 0x10, 0x02, 0x1a, 0x02, 0x3f, 0x00, 0x3f } },
    { 0x24d0, 0x05d1e,  4, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x295d, 0x05d1e,  4, { 0x00, 0x00, 0x1f, 0x00, 0x1f, 0x1f, 0x00, 0x1f } },
    { 0x2ee1, 0x05d1e,  4, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x346f, 0x05d1e,  5, { 0x00, 0x06, 0x0c, 0x00, 0x17, 0x06, 0x17, 0x0a, 0x00, 0x0a } },
    { 0x3737, 0x05d1e,  4, { 0x04, 0x00, 0x0a, 0x00, 0x0e, 0x33, 0x00, 0x33 } },
    { 0x3f72, 0x05d1e,  4, { 0x00, 0x0b, 0x2f, 0x0b, 0x2f, 0x1b, 0x00, 0x1b } },
    { 0x496f, 0x05d1e,  3, { 0x08, 0x2f, 0x12, 0x11, 0x1c, 0x2f } },
};

/*
 * NOT a transcription: reach one part's setup by its offset in this segment.
 *
 * The original arrives by `lcall` through a relocated far pointer, which the
 * port cannot do, so the offset is dispatched here the same way the region and
 * timer handlers are. An offset with no row yet aborts and names itself, which
 * is how the ones a screen actually needs get found - the title screen reaches
 * far fewer than the thirty-nine.
 */
void part_setup(uint16_t off, uint16_t part)
{
    int32_t i;

    for (i = 0; i < 13; i++) {
        uint16_t si;
        int32_t k;

        if (part_setups[i].off != off)
            continue;

        si = DGU16((uint16_t)(part + 0x82));
        for (k = 0; k < part_setups[i].n; k++) {
            DG8((uint16_t)(si + 4 * k)) = part_setups[i].xy[2 * k];
            DG8((uint16_t)(si + 4 * k + 1)) = part_setups[i].xy[2 * k + 1];
        }

        part_finish(part_setups[i].finish, part);
        return;
    }

    {
        static char what[64];

        snprintf(what, sizeof what, "the part setup at 172c:%04x", off);
        not_transcribed(what);
    }
}

/*
 * NOT a transcription: reach the routine a setup ends by calling.
 *
 * All thirteen of the setups above end the same way, at 0x05d1e, so this could
 * be one call - it is a dispatcher anyway, because the setups that are not in
 * the table yet end at other addresses and this is where they will arrive.
 */
void part_finish(uint16_t off, uint16_t part)
{
    switch (off) {
    case 0x5d1e:
        part_finish_angles(part);
        return;
    default:
        break;
    }

    {
        static char what[64];

        snprintf(what, sizeof what, "the part finish at %#07x", off);
        (void)part;
        not_transcribed(what);
    }
}
