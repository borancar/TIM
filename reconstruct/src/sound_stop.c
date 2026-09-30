/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Stopping sound**: the whole of it, one sequence, or the records a selector names, and the five-tick wait.
 *
 * The sixth module of the original's **code segment 2619**, image
 * 0x292f4..0x296b4 - the fourth of its modules in C; sound_device.c says how the
 * segment's boundaries are known. Functions are in address order and each
 * carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -G -Z
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* The five-tick wait, DGROUP 0x6430; the record is described in dgroup.h. */
struct sound_tick_wait g_sound_tick_wait;

/*
 * 0x292f4 (1.00's; not yet placed in 1.11)
 *
 * Shut the sound down: silence the driver, let whatever is playing finish, and
 * give both blocks back.
 *
 * How it waits depends on whether the sequencer's timer callback is
 * registered - DGROUP 0x4a8e. With it registered the tick is running and
 * `delay_five_ticks` is enough; without it nothing is driving the sequencer, so
 * `sound_service` is called twice by hand instead.
 *
 * `silence_driver_far` is called with **no arguments at all**, which is safe
 * only because it reads none - the same dead argument 0x2846a has.
 *
 * The loaded module is told to stop through its own dispatcher at 0x0bbc6, a
 * call into a block that is not part of this binary. Not reached here, and left
 * as a stub.
 */
void stop_sound(void)
{
    if (g_sound_bank.driver != NULL) {
        silence_driver_far();

        if (((int16_t)g_sound_bank.tick_handle) == 0) {
            sound_service();
            sound_service();
        } else {
            delay_five_ticks();
        }
    }

    if (g_sound_bank.module != NULL) {
        stop_loaded_module();
    }

    if (g_sound_bank.driver != NULL) {
        free_for_kind(g_sound_bank.driver, 1);
        g_sound_bank.driver = 0;
    }

    if (g_sound_bank.module != NULL) {
        free_for_kind(g_sound_bank.module, 1);
        g_sound_bank.module = 0;
    }
}

/*
 * 0x2b68f
 *
 * Wait five timer ticks. A counter at DGROUP 0x6430 is set to five, a callback
 * registered at four ticks a time, and the routine **spins** until the callback
 * has counted it down; then the slot is given back.
 *
 * The far pointer it registers is this module's own `cs:0x3228`, which is
 * `tick_delay` below.
 *
 * The spin only ends because the timer interrupt runs the callback, so in the
 * port it ends only when something drives the timer - the same standing as
 * `wait_and_latch_frame`. Nothing reaches it on these screens.
 */
void delay_five_ticks(void)
{
    uint16_t handle;

    g_sound_tick_wait.ticks_left = 5;

    handle = timer_add_callback(tick_delay, 4);

    while (g_sound_tick_wait.ticks_left > 0)
        ;

    timer_drop_callback(handle);
}

/*
 * 0x293b8 (1.00's; not yet placed in 1.11)
 *
 * The callback `delay_five_ticks` registers: one instruction of work, counting
 * DGROUP 0x6430 down by one each tick.
 */
void tick_delay(void)
{
    g_sound_tick_wait.ticks_left--;
}

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

/*
 * 0x2ad2c (1.00's; not yet placed in 1.11)
 *
 * Set the master level and answer 1. The 1 is unconditional - nothing below
 * reports failure, so this cannot either.
 */
uint16_t set_master_level_ok(uint16_t level)
{
    set_master_level_far(level);
    return 1;
}
