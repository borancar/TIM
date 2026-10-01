/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Allocate the seven voices' records.**
 *
 * A module of the sound library, in 1.11 **code segment 287d** on its own,
 * image 0x287d6..0x28850. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: compiler bc3.10
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x287d6
 *
 * Allocate the seven voice records - 0x17a bytes each, kind 2 - and put them in
 * the table at DGROUP 0x6414.
 *
 * A table that is **already** filled is refused with 0, not accepted as work
 * already done: the test is on the first entry only, and the answer is the
 * failure code. So this is called once.
 *
 * Each record is marked free with 0xff at +0x158 and given a far pointer at +8
 * to its own +0x16a. If any allocation fails the whole table is handed back
 * through `free_voice_records` - including the entries this loop has not
 * reached, which are whatever they were before.
 */
uint16_t alloc_voice_records(void)
{
    int16_t i;
    struct sequence far *voice;

    if (g_sound_voice[0] == NULL) {
        for (i = 0; i < 7; i++) {
            if ((g_sound_voice[i] = (struct sequence far *)
                     alloc_for_kind(sizeof(struct sequence), 2))
                == NULL) {
                free_voice_records();
                return 0;
            }

            /* `cursor_at` is where the record's own `cursor` is. */
            voice = g_sound_voice[i];
            voice->state = 0xff;
            voice->cursor_at = &voice->cursor;
        }
        return 1;
    }

    return 0;
}
