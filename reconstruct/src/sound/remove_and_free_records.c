/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Unlink records from the loaded list and give them back.**
 *
 * A module of the sound library, in 1.11 **code segment 2b0b** on its own,
 * image 0x2b0bf..0x2b1f8. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2b0bf
 *
 * Unlink records from the list at DGROUP 0x4a88 and give them back. The
 * selector is the one `next_matching_record` uses - 0 for every record, -1 and
 * -2 for the two families, anything else an identifier - but the matching is
 * open-coded here rather than borrowed, because this walk has to hold on to the
 * *previous* node and the shared iterator cannot.
 *
 * `stop_all_voices` runs first, but only for 0 and -2. An identifier or -1 does
 * not silence anything up front; each record is stopped individually by
 * `stop_sequences` as it is reached.
 *
 * The previous link starts as a **local**: `mov [bp-2],ss` puts the stack
 * segment beside the offset of a two-word cell at `bp-0x1c`, so the first
 * unlink writes into that scratch rather than into a real node, and reading it
 * back gives the next record. Since SS is DGROUP the cell is an ordinary
 * DGROUP address.
 *
 * The list head is fixed up separately, by comparing against it rather than by
 * treating it as another link.
 *
 * Each record costs three frees: its +4 pointer as kind 4 or kind 7 depending
 * on bit 0 of +0x12, and the record itself as kind 3.
 *
 * A positive selector stops after the first match. The two families and 0 walk
 * to the end.
 */
uint16_t remove_and_free_records(int16_t selector)
{
    /*
     * **A whole record on the stack, for its link.** `prev` starts at it so
     * that the first unlink writes somewhere harmless, and then follows the
     * list one record behind `cur`.
     */
    struct sound_record far *prev;
    struct sound_record far *cur;
    struct sound_record dummy;
    int16_t found;

    found = 0;
    prev = &dummy;
    cur = g_sound_bank.records;

    if (selector == 0 || selector == -2)
        stop_all_voices();

    while (cur != NULL) {
        if (selector == 0 || cur->id == selector
            || (selector == -1 && (cur->flags & 1) != 0)
            || (selector == -2 && (cur->flags & 1) == 0)) {
            found = 1;
            stop_sequences(cur->id);

            if (cur == g_sound_bank.records)
                g_sound_bank.records = cur->next;

            prev->next = cur->next;

            if ((cur->flags & 1) != 0)
                free_for_kind(cur->data, 4);
            else
                free_for_kind(cur->data, 7);

            free_for_kind((uint8_t far *)cur, 3);

            if (found != 0 && selector > 0)
                return found;
        } else {
            prev = cur;
        }

        cur = prev->next;
    }

    return found;
}
