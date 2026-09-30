/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Stop every voice that is playing.**
 *
 * A module of the sound library, in 1.11 **code segment 2b29** on its own,
 * image 0x2b29e..0x2b2d0. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2b29e
 *
 * Retire every voice that is still marked as playing. The seven-entry table at
 * DGROUP 0x6414 again, the 0xff at +0x158 as the mark, and
 * `retire_and_tick_far` as the retirement - the same three pieces as
 * `stop_voice_playing`, over all of them rather than one.
 *
 * It is called before the voices are allocated, when every entry is a far
 * null, and then it reads and writes the interrupt table's byte at 0000:0158
 * - the first pass marks it 0xff and the other six find it so. `ZERO_PAGE`
 * gives the host the same bytes.
 */
void stop_all_voices(void)
{
    int16_t i;

    for (i = 0; i < 7; i++) {
        if (ZERO_PAGE(g_sound_voice[i])->state != 0xff) {
            retire_and_tick_far(ZERO_PAGE(g_sound_voice[i]));
            ZERO_PAGE(g_sound_voice[i])->state = 0xff;
        }
    }
    (void)i;    /* see free_voice_records.c */
}
