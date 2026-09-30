/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Stop the sequences a selector names.**
 *
 * A module of the sound library, in 1.11 **code segment 2b6c** on its own,
 * image 0x2b6cd..0x2b86d. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2b6cd
 *
 * Stop sequences. Which ones is the selector, and it is the same vocabulary
 * `next_matching_record` uses: -1 for the ones with bit 0 of +0x12 set, -2 for
 * the ones without, 0 for both, and anything else names one by its identifier.
 *
 * Stopping a record of the first family means letting its voice finish -
 * `follow_then_tick`, then a **busy wait** on that voice's +0x158 until it
 * reads 0xff - and then giving the voice back as kind 2 and clearing +0xe. The
 * wait is not a spin against an interrupt: `follow_then_tick` runs the
 * sequencer itself, and it is the sequencer that writes the 0xff.
 *
 * That branch then abandons the walk by clearing the cursor - one record of
 * that family is stopped per call, not all of them - while a record with
 * nothing at +0xe simply has its 0x10 bit cleared and the walk continues.
 *
 * The second family is only ever cleared of 0x10; what actually silences it is
 * the `stop_all_voices` at the end. A selector of 0 does the first family and
 * then falls into the second, which is why the `-1` case returns early and the
 * `0` case does not.
 *
 * An identifier selects one record and takes whichever of the two paths its
 * bit 0 says, and answers 0 when there is no such record - the only path here
 * that reports failure.
 */
uint16_t stop_sequences(int16_t selector)
{
    struct sound_record far *rec;

    switch (selector) {
    case -1:
    case 0:
        rec = next_matching_record(-1);
        while (rec != NULL) {
            rec->flags &= 0xffef;

            if (rec->sequence != NULL) {
                follow_then_tick(rec->sequence, 0);

                /* The timer retires it; this waits for that. */
                while (rec->sequence->state != 0xff)
                    ;

                free_for_kind((uint8_t far *)rec->sequence, 2);
                rec->sequence = 0;
                rec = NULL;
            } else {
                rec = next_matching_record(-3);
            }
        }

        if (selector == -1)
            return 1;
        /* falls through */

    case -2:
        rec = next_matching_record(-2);
        while (rec != NULL) {
            rec->flags &= 0xffef;
            rec = next_matching_record(-3);
        }

        stop_all_voices();
        return 1;

    default:
        if ((rec = next_matching_record(selector)) != NULL) {
            rec->flags &= 0xffef;

            if ((rec->flags & 1) != 0) {
                if (rec->sequence != NULL) {
                    follow_then_tick(rec->sequence, 0);

                    while (rec->sequence->state != 0xff)
                        ;

                    free_for_kind((uint8_t far *)rec->sequence, 2);
                    rec->sequence = 0;
                }
            } else {
                stop_voice_playing(rec->data);
            }
            return 1;
        }
        return 0;
    }
}
