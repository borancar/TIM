/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Stop the voice playing a block of note data.**
 *
 * A module of the sound library, in 1.11 **code segment 2b92** on its own,
 * image 0x2b929..0x2b982. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2b929
 *
 * Stop whichever voice is playing a given sequence. The same seven-entry table
 * at DGROUP 0x6414 that `voice_playing` searches, and the same match on the far
 * pointer at +0x166 - but this one does not test +0x158 first, so a voice
 * already marked stopped is retired and marked again.
 *
 * It returns after the first match: nothing here handles a second voice on the
 * same sequence, which is the assumption that a sequence has one.
 */
void stop_voice_playing(const uint8_t far * source)
{
    int16_t i;
    struct sequence far *v;

    for (i = 0; i < 7; i++) {
        v = g_sound_voice[i];

        /* Which note data this voice is playing - see `voice_playing`. */
        if (v->source == source) {
            retire_and_tick_far(v);
            v->state = 0xff;
            return;
        }
    }
    (void)i;    /* see free_voice_records.c */
}
