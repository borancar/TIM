/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Freeing records and stopping sequences.**
 *
 * The fifth module of the original's **code segment 2619**, image
 * 0x293c1..0x296b4 - the second of its modules in C; sound_device.c says how the
 * segment's boundaries are known. Functions are in address order and each
 * carries the image offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x293c1
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
 * DGROUP address, which is why the port needs a guest stack of its own - see
 * `dg_alloca` in dgroup.h.
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
     * **The previous link is a walking far pointer, and both ends of its walk
     * are host pointers.** It starts at this two-word scratch cell so that
     * the first unlink writes somewhere harmless, and then becomes each
     * record's link in turn - only ever written and read through, so a
     * pointer to the `struct far_ptr` it is, and the cell a C array.
     */
    struct sound_record far *cell;      /* [bp-0x1c], the two-word cell */
    struct sound_record far * far *link_at = &cell;
    /* The records' own links are pairs filed as DOS handed the blocks out,
       each starting a segment, so a pointer compares as the pair does. */
    struct sound_record *cur = SOUND_RECORD_PTR(DG4A82.records);
    int16_t found = 0;

    if (selector == 0 || selector == -2)
        stop_all_voices();

    while (cur != SOUND_RECORD_NONE) {
        int16_t match;

        if (selector == 0)
            match = 1;
        else if (cur->id == selector)
            match = 1;
        else if (selector == -1)
            match = (cur->flags & 1) != 0;
        else if (selector == -2)
            match = (cur->flags & 1) == 0;
        else
            match = 0;

        if (match) {
            found = 1;
            stop_sequences(cur->id);

            if (cur == SOUND_RECORD_PTR(DG4A82.records))
                DG4A82.records = far_of((uint8_t *)cur->next);

            *link_at = cur->next;

            if ((cur->flags & 1) != 0)
                free_for_kind(cur->data, 4);
            else
                free_for_kind(cur->data, 7);

            free_for_kind((uint8_t *)cur, 3);

            if (selector > 0)
                break;
        } else {
            link_at = &cur->next;
        }

        cur = *link_at;
    }

    return (uint16_t)found;
}

/*
 * 0x294ff
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
    struct sound_record *rec;

    if (selector == -1 || selector == 0) {
        rec = next_matching_record(-1);
        for (;;) {
            if (rec == SOUND_RECORD_NONE)
                break;

            rec->flags &= 0xffef;

            if (rec->sequence != NULL) {
                struct sequence *v = rec->sequence;

                follow_then_tick(v, 0);

                do {
                    v = rec->sequence;
                } while (v->state != 0xff);

                free_for_kind((uint8_t *)v, 2);
                rec->sequence = NULL;
                rec = SOUND_RECORD_NONE;
            } else {
                rec = next_matching_record(-3);
            }
        }

        if (selector == -1)
            return 1;
    }

    if (selector == -1 || selector == 0 || selector == -2) {
        rec = next_matching_record(-2);
        for (;;) {
            if (rec == SOUND_RECORD_NONE)
                break;

            rec->flags &= 0xffef;
            rec = next_matching_record(-3);
        }

        stop_all_voices();
        return 1;
    }

    rec = next_matching_record(selector);
    if (rec == SOUND_RECORD_NONE)
        return 0;

    rec->flags &= 0xffef;

    if ((rec->flags & 1) == 0) {
        stop_voice_playing(rec->data);
        return 1;
    }

    if (rec->sequence != NULL) {
        struct sequence *v = rec->sequence;

        follow_then_tick(v, 0);

        do {
            v = rec->sequence;
        } while (v->state != 0xff);

        free_for_kind((uint8_t *)v, 2);
        rec->sequence = NULL;
    }

    return 1;
}

/*
 * 0x296a1
 *
 * Set the master level and answer 1. The 1 is unconditional - nothing below
 * reports failure, so this cannot either.
 */
uint16_t set_master_level_ok(uint16_t level)
{
    set_master_level_far(level);
    return 1;
}
