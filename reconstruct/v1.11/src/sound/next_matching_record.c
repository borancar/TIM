/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Walk the loaded records, one family or one identifier at a time.**
 *
 * A module of the sound library, in 1.11 **code segment 280b** on its own,
 * image 0x280b5..0x28194. 1.00 linked the library's C into one segment,
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
 * **The walk's cursor and selector**, DGROUP 0x5fec..0x5ff2: this module's
 * `_BSS`, where 1.00 kept them beside the five-tick wait in one record.
 * Borland lays `_BSS` out in reverse order of first mention, so the
 * selector is defined first.
 */
static int16_t g_sound_record_selector;
static struct sound_record far *g_sound_record_cursor;

/*
 * 0x280b5
 *
 * Walk the record list and answer the next one matching a selector, as a far
 * pointer in DX:AX. The cursor is a **static** far pointer at DGROUP 0x6432,
 * with the selector remembered beside it at 0x6436, so this is an iterator with
 * one shared position rather than a search - two overlapping walks would tread
 * on each other.
 *
 * A selector of -3 means "continue": the cursor steps on and the remembered
 * selector is reused. Anything else starts again from the list head at 0x4a88
 * and is remembered.
 *
 * Three selectors filter on the flag word at each record's +0x12, and they are
 * expressed as a mask and an expected value rather than as three tests:
 *
 *   -1  mask 1, expect 0 - records with bit 0 set
 *   -2  mask 1, expect 1 - records with bit 0 clear
 *    0  mask 0, expect 1 - every record, since `0 ^ 1` is never zero
 *
 * Any other selector matches on the identifier at +0xa instead.
 *
 * **An identifier search cannot be continued.** Reaching that branch with the
 * argument -3 clears the cursor and answers nothing - and the cursor has
 * already stepped on by then, so the record after a match is skipped as well as
 * unreported. Whether that is deliberate because identifiers are unique, or an
 * oversight, is not established; it is transcribed as it stands.
 *
 * Running off the end answers a null far pointer, and the two selector families
 * differ in whether they also *clear* the cursor: the flag walk leaves it at
 * null naturally, the identifier walk writes zeros explicitly on the paths that
 * give up early.
 */
struct sound_record far *next_matching_record(int16_t selector)
{
    int16_t expect = 0;
    int16_t mask = 1;

    if (selector != -3) {
        g_sound_record_selector = selector;
        g_sound_record_cursor = g_sound_bank.records;
    } else if (g_sound_record_cursor != NULL) {
        g_sound_record_cursor = g_sound_record_cursor->next;
    }

    switch (g_sound_record_selector) {
    case 0:
        mask = 0;
        /* falls through */
    case -2:
        expect = 1;
        /* falls through */
    case -1:
        while (g_sound_record_cursor != NULL) {
            if (((g_sound_record_cursor->flags & mask) ^ expect) != 0)
                return g_sound_record_cursor;
            g_sound_record_cursor = g_sound_record_cursor->next;
        }
        break;

    default:
        /* Match on the identifier. */
        if (g_sound_record_cursor != NULL && selector != -3) {
            while (g_sound_record_cursor != NULL
                   && g_sound_record_cursor->id != selector)
                g_sound_record_cursor = g_sound_record_cursor->next;
        } else {
            g_sound_record_cursor = 0;
        }
    }

    return g_sound_record_cursor;
}
