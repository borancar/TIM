/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The sound file: opening it, finding a record, starting and stopping the whole of sound.**
 *
 * The seventh module of the original's **code segment 2619**, image
 * 0x296b4..0x2a040 - the fifth of its modules in C; sound_device.c says how the
 * segment's boundaries are known. Functions are in address order and each
 * carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -G -Z
 * JUDGE: data 0x4a82..0x4ab4
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The sound bank, its driver and its module**, DGROUP 0x4a82..0x4ab0: this
 * module's `_DATA`, or sound_stop.c's - the two are adjacent and their bytes
 * would be the same either way; this module is the one that starts and ends
 * sound. The record is described in dgroup.h. Three fields start non-zero:
 * `voice_word` at -4, `bank_choice` at 1 and `device` at -2.
 */
struct sound_bank g_sound_bank = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 0, 0, 0, 0, 1, -2,
};

/*
 * DGROUP 0x4ab0..0x4ab4 - **two words nothing in the image names**: -2, and
 * 0x2b11, 11025, a sample rate. They are the last of the game's data - the
 * run-time library's begins at 0x4ab4 - and the objects linked after this
 * one, trig.c and atan2.c, have no `_DATA`; and this module's has no string
 * literals, which Borland would have put after them. So they are this
 * module's, defined after the record. What they were for is not known.
 */
struct dg_4ab0 g_dg4ab0 = { 0xfffe, 0x2b11 };

/*
 * 0x296b4
 *
 * Open a sound file and load either one record out of it or all of them.
 * Answers the handle it used, or 0.
 *
 * The first argument is a handle **or** a name: `file_record_valid` decides
 * which, and a name is opened here and remembered as ours to close - which is
 * what DGROUP 0x4aa8 records, beside the handle at 0x4aa6.
 *
 * Asking again for the same handle with a positive identifier short-circuits
 * to the search, so a second record out of an already-open file costs nothing.
 *
 * The directory is at offset 0xc of the file: a 32-bit size, then a block four
 * bytes longer read whole, whose first word must be 2. Its +6 is the number of
 * six-byte entries and +8 a byte handed to `read_record`; each entry is an
 * identifier and a 32-bit offset.
 *
 * A positive identifier loads one record, a non-positive one loads them all in
 * order. **The one-record search leaves its offset uninitialised when nothing
 * matches**, and the test that follows reads whatever the stack held; the port
 * keeps those two words on the guest stack for that reason rather than in C
 * locals.
 *
 * Every failure runs the same cleanup: close the file if this routine opened
 * it, free the directory, and throw away every record read so far.
 */
FILE *open_sound_file(char *name, int16_t id)
{
    int32_t found;
    uint32_t size;
    struct sound_dir_entry far *cur;
    int16_t si;

    /* A handle, or a name to open. */
    if (id != 0 && (FILE *)name == g_sound_bank.file && g_sound_bank.file != 0)
        goto search;

    if (g_sound_bank.file != (FILE *)name && g_sound_bank.file_kind != 0) {
        close_file_record(g_sound_bank.file);
        g_sound_bank.file = 0;
        g_sound_bank.file_kind = 0;
    }

    if (file_record_valid((FILE *)name) != 0) {
        g_sound_bank.file = (FILE *)name;
    } else {
        if ((g_sound_bank.file = open_file_record(name)) == 0)
            goto fail;
        g_sound_bank.file_kind = 1;
    }

    remove_and_free_records(0);

    game_fseek(g_sound_bank.file, 0xcL, 0);
    if (game_fread((uint8_t *)&size, 4, 1, g_sound_bank.file) != 1)
        goto fail;

    if (g_sound_bank.directory != 0)
        free_for_kind((uint8_t far *)g_sound_bank.directory, 0xa);

    /* The file's own directory image goes after the cursor, over `magic`
       onwards. */
    if ((g_sound_bank.directory = (struct sound_dir far *)alloc_for_kind(size + 4, 0xa))
        == 0)
        goto fail;
    if (fread_huge((uint8_t far *)&g_sound_bank.directory->magic, size, 1L, g_sound_bank.file)
        != 1L)
        goto fail;
    if (g_sound_bank.directory->magic != 2)
        goto fail;

    g_sound_bank.directory->cursor = g_sound_bank.directory->entry;

search:
    if (id > 0 && next_matching_record(id) != NULL)
        return g_sound_bank.file;

    cur = g_sound_bank.directory->cursor;

    if (id > 0) {
#ifndef __TURBOC__
        /* **The search leaves `found` unset when nothing matches**, and the
           test below reads whatever the stack held. The host starts it at
           the zero the test is written for. Ours. */
        found = 0;
#endif
        for (si = 0; g_sound_bank.directory->count > si; si++, cur++) {
            if (cur->id == id) {
                found = cur->at;
                break;
            }
        }

        if (game_fseek(g_sound_bank.file, found + 4, 0) != 0)
            goto fail;
        if (found == 0)
            goto fail;

        if (read_record(g_sound_bank.file, g_sound_bank.directory->kind) == 0)
            return 0;
        goto done;
    }

    for (si = 0; g_sound_bank.directory->count > si; si++, cur++) {
        if (game_fseek(g_sound_bank.file, cur->at + 4, 0) != 0)
            goto fail;
        if (read_record(g_sound_bank.file, g_sound_bank.directory->kind) == 0)
            goto fail;
    }

done:
    return g_sound_bank.file;

fail:
    if (g_sound_bank.file != 0 && g_sound_bank.file_kind != 0)
        close_file_record(g_sound_bank.file);

    if (g_sound_bank.directory != 0)
        free_for_kind((uint8_t far *)g_sound_bank.directory, 0xa);

    remove_and_free_records(0);

    g_sound_bank.file = 0;
    g_sound_bank.directory = 0;
    return 0;
}

/*
 * 0x29966
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
        g_sound_tick_wait.selector = selector;
        g_sound_tick_wait.cursor = g_sound_bank.records;
    } else if (g_sound_tick_wait.cursor != NULL) {
        g_sound_tick_wait.cursor = g_sound_tick_wait.cursor->next;
    }

    switch (g_sound_tick_wait.selector) {
    case 0:
        mask = 0;
        /* falls through */
    case -2:
        expect = 1;
        /* falls through */
    case -1:
        while (g_sound_tick_wait.cursor != NULL) {
            if (((g_sound_tick_wait.cursor->flags & mask) ^ expect) != 0)
                return g_sound_tick_wait.cursor;
            g_sound_tick_wait.cursor = g_sound_tick_wait.cursor->next;
        }
        break;

    default:
        /* Match on the identifier. */
        if (g_sound_tick_wait.cursor != NULL && selector != -3) {
            while (g_sound_tick_wait.cursor != NULL
                   && g_sound_tick_wait.cursor->id != selector)
                g_sound_tick_wait.cursor = g_sound_tick_wait.cursor->next;
        } else {
            g_sound_tick_wait.cursor = 0;
        }
    }

    return g_sound_tick_wait.cursor;
}

/*
 * 0x29a49
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

/*
 * 0x29c3b
 *
 * Start the sound system. Answers 1 if it came up, 0 if it did not - and 1
 * again, immediately, if either the driver at DGROUP 0x4a94 or the module at
 * 0x4a98 is already loaded, so this cannot run twice.
 *
 * A device of -1 means "no sound": the device becomes 2 and the flag that
 * drives everything after is cleared, so `setup_sound_device` still runs but
 * nothing is installed on the back of it.
 *
 * With sound wanted, three things follow. The timer is taken over at rate 0xd
 * unless something already has it - DGROUP 0x44ee - and 0x4a8c records that.
 * The sequencer's own tick is registered as a callback at rate 4, keeping its
 * slot at 0x4a8e. And a third
 * callback goes to the loaded module's own dispatcher, at 0x0bba6 in segment 0,
 * but only if that module loaded - which it does not here.
 *
 * `alloc_voice_records` is last, and its answer is not looked at.
 */
uint16_t start_sound(int16_t device, int16_t module_index, uint16_t callback,
                     FILE *handle)
{
    int16_t si = 1;

    if (g_sound_bank.driver != NULL || g_sound_bank.module != NULL)
        return 1;

    if (device == -1) {
        device = 2;
        si = 0;
    }

    if (setup_sound_device(device, module_index, callback, handle) != 0) {
        if (si != 0 && !(int8_t)g_timer.installed) {
            timer_install(0xd);
            g_sound_bank.timer_taken = 1;
        }

        if ((si != 0
             && (g_sound_bank.tick_handle = (int16_t)timer_add_callback(sound_service, 4)) != 0)
            || si == 0) {
            if (si != 0 && g_sound_bank.module != NULL)
                g_sound_bank.module_handle =
                    (int16_t)timer_add_callback(SOUND_MODULE_TICK, 2);

            alloc_voice_records();
            return 1;
        }
    }

    return 0;
}

/*
 * 0x29cf6
 *
 * Take the whole sound system down, in the reverse order `start_sound` built
 * it up. Does nothing at all if neither the driver nor the module is loaded.
 *
 * Everything is released and its slot zeroed as it goes: the records and their
 * payloads, the directory at DGROUP 0x4aa2, the file at 0x4aa6 if this module
 * opened it, the two timer callbacks at 0x4a8e and 0x4a90, and the timer itself
 * if 0x4a8c says it was taken. Then the voice records, and `stop_sound` last.
 *
 * `free_voice_records` answers whether it found a table to free, and that
 * answer is ignored - so a system that never allocated one comes down just as
 * quietly.
 */
void shutdown_sound(void)
{
    if (g_sound_bank.driver == NULL
        && g_sound_bank.module == NULL)
        return;

    remove_and_free_records(0);

    if (g_sound_bank.directory != 0)
        free_for_kind((uint8_t far *)g_sound_bank.directory, 0xa);

    if (g_sound_bank.file != 0 && g_sound_bank.file_kind != 0)
        close_file_record(g_sound_bank.file);

    if (((int16_t)g_sound_bank.tick_handle) != 0) {
        timer_drop_callback(g_sound_bank.tick_handle);
        g_sound_bank.tick_handle = 0;
    }

    if (((int16_t)g_sound_bank.module_handle) != 0) {
        timer_drop_callback(g_sound_bank.module_handle);
        g_sound_bank.module_handle = 0;
    }

    if (((int16_t)g_sound_bank.timer_taken) != 0) {
        timer_remove();
        g_sound_bank.timer_taken = 0;
    }

    free_voice_records();
    stop_sound();
}

/*
 * 0x29da0
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

/*
 * 0x29f89
 *
 * Allocate a block for the sound module, choosing where from by a `kind`
 * argument, and zero it for some kinds but not others.
 *
 * Kinds 6 and 8 come from the C runtime's own heap - a near pointer, with the
 * data segment supplied as the segment half - and everything else from DOS
 * through `dos_alloc_bytes`. The two are not interchangeable: only the DOS path
 * can hand back more than a segment, and only the heap path gives a pointer the
 * runtime can later free.
 *
 * Kinds 2, 3, 4 and 7 are then zeroed with `far_memset`. Note that 6 and 8 are
 * not among them, so a heap block comes back holding whatever was there - and
 * the zeroing is skipped entirely when the allocation failed, which is the only
 * thing the null check guards.
 *
 * `malloc` is not transcribed. The runtime's heap is a deliberate non-goal, and
 * the port refuses rather than inventing a pointer it could not also give a
 * block header to; see `io_malloc`. Kinds 6 and 8 are not reached on the
 * screens checked, so the rest of this verifies.
 */
uint8_t far *alloc_for_kind(uint32_t size, uint16_t kind)
{
    uint8_t far *blk;
    uint8_t *p;

    if (kind == 6 || kind == 8) {
        /* The near heap takes a word, and its answer is widened with DS - so
           a refusal is DGROUP:0000, not the far null. Nothing in the game
           asks for these two kinds. */
        p = malloc_far((uint16_t)size);
        blk = (uint8_t far *)NEAR_ZERO(p);
    } else {
        blk = dos_alloc_bytes(size, 0);
    }

    if (blk != NULL
        && (kind == 2 || kind == 3 || kind == 4 || kind == 7))
        far_memset(blk, 0, size);

    if (blk == NULL)
        g_sound_bank.load_error = 1;

    return blk;
}

/*
 * 0x2a017
 *
 * Release a block the sound module allocated, and the exact counterpart of
 * `alloc_for_kind` at 0x29f89: the same `kind` argument picks the same two
 * places, kinds 6 and 8 going back to the C runtime's heap and everything else
 * to DOS.
 *
 * The kind is not stored with the block, so it is the caller's job to release
 * one with the same kind it asked for. Passing the wrong one hands a heap
 * pointer to DOS or a DOS segment to `free`, and nothing here would notice.
 *
 * `free` is not transcribed, for the reason `io_malloc` gives; the DOS path is
 * the one these screens take.
 */
void free_for_kind(uint8_t far * blk, uint16_t kind)
{
    if (kind == 6 || kind == 8) {
        /* The near heap takes only the offset. */
        free_far((uint8_t *)blk);
        return;
    } else {
        dos_free_far(blk);
    }
    /* The image's: this `return` and the first one each get an epilogue of
       their own, and the function's own is left after them, unreached. */
    return;
}
