/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Build a sequence record around a block of note data.**
 *
 * A module of the sound library, in 1.11 **code segment 281d** on its own,
 * image 0x281d1..0x28256. 1.00 linked the library's C into one segment,
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
 * 0x281d1
 *
 * Build a sequence record around a block of note data, and answer it as a far
 * pointer - or a null one if there was no room.
 *
 * The record is 0x17a bytes of kind 2, so `alloc_for_kind` zeroes it; every
 * field not written below is therefore known to be zero rather than merely
 * assumed so.
 *
 * The source pointer is kept at +0x166, and +0x16a gets the same pointer
 * stepped past the first record by `advance_record` - the segment half is
 * carried across unchanged, because that routine only moves the offset.
 *
 * +8 is then made to point at **+0x16a of the record itself**, so the cursor
 * the sequencer follows lives inside the record and starts at the second entry.
 * That is why the record's own segment is stored beside it at +0xa.
 *
 * +0x15e is set to 0x7f, which `sequencer_tick` reads as "use the default"
 * where it feeds `scale_byte_pair`, and the two words at +0x172 are cleared
 * again although the allocation already did it.
 */
struct sequence far *create_sequence(const uint8_t far * src)
{
    struct sequence far *seq;

    if ((seq = (struct sequence far *)alloc_for_kind(sizeof(struct sequence), 2))
        != NULL) {
        /* `cursor` and `cursor_at` are stepped inside their blocks' segments:
           `advance_record` and the `+ 0x16a` move the offset alone. */
        seq->source = src;
        seq->cursor = (uint8_t far *)advance_record(src);
        seq->cursor_at = &seq->cursor;

        seq->volume = 0x7f;
        seq->next = 0;

        return seq;
    }

    return NULL;
}
