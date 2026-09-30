/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Start the sequence a record identifier names.**
 *
 * A module of the sound library, in 1.11 **code segment 2b41** on its own,
 * image 0x2b418..0x2b608. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2b418
 *
 * Start the sequence with a given identifier, loading it if it is not loaded
 * yet. Answers 1 for "it is playing or there is nothing to do", 0 for a
 * failure to load.
 *
 * The list at DGROUP 0x4a88 is walked by hand rather than through
 * `next_matching_record`, because that iterator has one shared cursor and this
 * routine walks the list a second time inside itself.
 *
 * Three conditions each mean there is nothing to do, and all three answer 1:
 * the 0x10 bit already set at +0x12, no source at +4, or something already
 * loaded at +0xe.
 *
 * Then the two families part. A record with bit 0 set - music - first stops
 * **every other** loaded record of the same family, so only one plays at a
 * time. If DGROUP 0x4aa0 is 0 or -1 it stops there and marks 0x10 without
 * loading anything, which is how a disabled device still leaves the game
 * believing the music started. Otherwise `create_sequence` builds it from the
 * source at +4, two bytes are copied into the built sequence at +0x15c and
 * +0x15d - the second from +0xc, the first from bit 1 of +0x12 - and
 * `load_and_start_sequence` starts it at level 0x7f.
 *
 * A record without bit 0 - an effect - asks `voice_playing` whether its source
 * is already on a voice, and answers 1 if it is. If not, 0x4aa0 being 0 or -2
 * is again the disabled case, and otherwise `start_on_free_voice` places it,
 * again at 0x7f, with bit 1 of +0x12 as the byte argument.
 */
uint16_t start_sequence_by_id(int16_t id)
{
    struct sound_record far *rec;
    struct sound_record far *other;

    for (rec = g_sound_bank.records; rec != NULL && rec->id != id;
         rec = rec->next)
        ;

    if (rec == NULL)
        return 0;

    if ((rec->flags & 0x10) != 0 || rec->data == NULL
        || rec->sequence != NULL)
        return 1;

    if ((rec->flags & 1) != 0) {
        for (other = g_sound_bank.records; other != NULL;
             other = other->next) {
            if ((other->flags & 1) != 0 && other->sequence != NULL
                && other->id != id)
                stop_sequences(other->id);
        }

        if (g_sound_bank.voice_word == 0 || g_sound_bank.voice_word == -1) {
            rec->flags |= 0x10;
            return 1;
        }

        if ((rec->sequence = create_sequence(rec->data)) != NULL) {
            rec->sequence->loop = (rec->flags & 2) != 0;
            rec->sequence->priority = (uint8_t)rec->priority;

            if (load_and_start_sequence(rec->sequence, 0, 0x7f) != NULL)
                return 1;
        }
    } else {
        if (voice_playing(rec->data) == NULL) {
            if (g_sound_bank.voice_word == 0 || g_sound_bank.voice_word == -2) {
                if ((rec->flags & 2) != 0) {
                    rec->flags |= 0x10;
                    return 1;
                }
                return 1;
            }

            {
                int16_t loop = (rec->flags & 2) ? 1 : 0;

                start_on_free_voice(rec->data, 0x7f, loop);
            }
            return 1;
        }
        return 1;
    }

    return 0;
}
