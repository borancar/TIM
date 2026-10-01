/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Read one record of a sound file into memory.**
 *
 * A module of the sound library, in 1.11 **code segment 27e2** on its own,
 * image 0x27e22..0x27ffe. 1.00 linked the library's C into one segment,
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
 * 0x27e22
 *
 * Read one record's header out of a file, load whatever it points at, and put
 * the record on the front of the list at DGROUP 0x4a88. Answers 1, or 0 if
 * anything failed.
 *
 * The header is four fields read one after another: a 32-bit length, then an
 * identifier word into +0xa, then two bytes into +0xc and +0x12. Bit 0 of that
 * last one is what makes the payload kind 4 rather than kind 7 - the same two
 * kinds `remove_and_free_records` frees by.
 *
 * The length then has **four taken off it**, because the identifier and the two
 * bytes were part of it.
 *
 * Where the payload comes from is three cases. A second argument of 0x63 means
 * it is raw: a block of its own and `fread_huge` straight into it. Otherwise
 * DGROUP 0x4aac chooses between `load_sound_bank`, which selects a record for
 * the configured device, and `load_resource_block`, which takes the resource
 * whole.
 *
 * Any failure frees the record as kind 3 and answers 0; the payload's own
 * pointer is left where it was written, which is null on every path that gets
 * there.
 */
uint16_t read_record(FILE *file, uint8_t mode)
{
    int32_t len;
    uint32_t out;
    struct sound_record far *rec;
    int16_t scratch;
    uint16_t kind;

    game_fread((uint8_t *)&len, 4, 1, file);
    game_fread((uint8_t *)&scratch, 2, 1, file);

    if ((rec = (struct sound_record far *)alloc_for_kind(sizeof(struct sound_record), 3))
        == NULL)
        return 0;

    rec->id = scratch;

    /* Two single bytes into the same word, each widened as a byte. */
    game_fread((uint8_t *)&scratch, 1, 1, file);
    rec->priority = *(uint8_t *)&scratch;

    game_fread((uint8_t *)&scratch, 1, 1, file);
    rec->flags = *(uint8_t *)&scratch;

    kind = (rec->flags & 1) ? 4 : 7;

    len = len - 4;

    rec->data = 0;

    if (mode == 0x63) {
        if ((rec->data = alloc_for_kind(len, kind)) == NULL
            || fread_huge(rec->data, len, 1L, file) != 1L) {
            free_for_kind((uint8_t far *)rec, 3);
            return 0;
        }
    } else if (g_sound_bank.bank_choice != 0) {
        if ((rec->data = load_sound_bank(file, len, (uint8_t *)&out, kind))
            == NULL) {
            free_for_kind((uint8_t far *)rec, 3);
            return 0;
        }
    } else {
        if ((rec->data = load_resource_block(file, len, (uint8_t *)&out, kind))
            == NULL) {
            free_for_kind((uint8_t far *)rec, 3);
            return 0;
        }
    }

    rec->next = g_sound_bank.records;
    rec->size = (uint16_t)out;
    g_sound_bank.records = rec;
    return 1;
}
