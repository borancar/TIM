/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Give a sequence to the first free voice and start it.**
 *
 * A module of the sound library, in 1.11 **code segment 2b98** on its own,
 * image 0x2b982..0x2ba6a. 1.00 linked the library's C into one segment,
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
 * 0x2b982
 *
 * Give a sequence to the first free voice and start it.
 *
 * Free means 0xff at +0x158 - the same mark `stop_all_voices` writes. The voice
 * then remembers the sequence twice: the pointer it was given, at +0x166, and
 * the record **after** it, at +0x16a, which is where playing begins.
 *
 * Three bytes of per-voice state are set from one of two places. When the table
 * at DGROUP 0x4a92 exists, +0x15d and +0x15c come out of it as a pair - two
 * bytes per index - and +0x15e is 0x7f. When it does not, the three come from
 * the caller instead: +0x15d from the third argument, +0x15c is 1, and +0x15e
 * is the index itself. So the table, when present, overrides what the caller
 * asked for.
 *
 * Answers the voice as a far pointer, or 0 if the sequence was null or every
 * voice was busy.
 */
struct sequence far *start_on_free_voice(const uint8_t far * source, uint16_t index,
                                         uint8_t byte_arg)
{
    struct sequence far *voice = NULL;
    int16_t i;

    if (source != NULL) {
        for (i = 0; i < 7; i++) {
            voice = g_sound_voice[i];

            if (voice->state == 0xff) {
                /* Which note data this voice is playing, and how far into
                   it - the second a segment beside the offset
                   `advance_record` stepped. */
                voice->source = source;
                voice->cursor = (uint8_t far *)advance_record(source);

                if (g_sound_bank.bank != 0) {
                    voice->looping = g_sound_bank.bank[index].loop;
                    voice->priority = g_sound_bank.bank[index].priority;
                    voice->volume = 0x7f;
                } else {
                    voice->looping = (uint8_t)byte_arg;
                    voice->priority = 1;
                    voice->volume = (uint8_t)index;
                }

                start_sequence_far(voice, 0);
                return voice;
            }
        }
    }

    return NULL;
}
