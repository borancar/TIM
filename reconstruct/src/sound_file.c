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
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

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
uint16_t open_sound_file(char *name, int16_t id)
{
    FILE *handle = (FILE *)name;         /* a handle, or a name to open */
    /* [bp-4]:[bp-2], one long: the matching record's file offset. The search
       below tests it against zero for "nothing matched", which is the value
       the original's author assumed the slot started at; the original never
       writes it before the search, so on a miss it read whatever the stack
       held. The port starts it at the zero the test is written for. */
    uint32_t found = 0;
    uint32_t size;               /* [bp-8]:[bp-6], one long */
    /* [bp-0xc]:[bp-0xa], the walk: one entry at a time, which is the
       original's `add si,6`. */
    const struct sound_dir_entry far *cur = NULL;
    /* The directory block itself. The original reloads `les bx,[0x4a98]`
       before each of the ten reads below; nothing changes it in between, and
       the `goto search` above skips the allocation, so it is taken again at
       that label. */
    struct sound_dir far *dir;
    int16_t si;
    uint16_t r = 0;

    if (id != 0 && handle == FILEREC_PTR(DG4A82.file_ptr) && DG4A82.file_ptr != 0)
        goto search;

    if (FILEREC_PTR(DG4A82.file_ptr) != handle && DG4A82.file_kind != 0)
        close_file_record(FILEREC_PTR(DG4A82.file_ptr));

    DG4A82.file_ptr = 0;
    DG4A82.file_kind = 0;

    if (file_record_valid(handle) != 0) {
        DG4A82.file_ptr = dg_near(dgroup, handle);
    } else {
        DG4A82.file_ptr = dg_near(dgroup, open_file_record(name));
        if (DG4A82.file_ptr == 0)
            goto fail;
        DG4A82.file_kind = 1;
    }

    remove_and_free_records(0);

    game_fseek(FILEREC_PTR(DG4A82.file_ptr), 0xc, 0);

    if (game_fread((uint8_t *)&size, 4, 1, FILEREC_PTR(DG4A82.file_ptr)) != 1)
        goto fail;

    if (DG4A82.directory != FAR_NULL_PTR)
        free_for_kind(DG4A82.directory, 0xa);

    /* `size + 4` as one long - the directory image goes after the cursor;
       the original adds the low word and carries into the high one by hand. */
    dir = (struct sound_dir *)(void *)
        alloc_for_kind(size + offsetof(struct sound_dir, magic), 0xa);
    DG4A82.directory = (uint8_t far *)dir;
    if ((uint8_t *)dir == FAR_NULL_PTR)
        goto fail;

    /* The file's own directory image lands after the cursor, over `magic`
       onwards. */
    if (fread_huge((uint8_t *)&dir->magic, size, 1,
                   FILEREC_PTR(DG4A82.file_ptr)) != 1)
        goto fail;

    if (dir->magic != 2)
        goto fail;

    /* Where the walk starts: the first entry. */
    dir->cursor = (uint8_t far *)&dir->entry[0];

search:
    dir = (struct sound_dir *)(void *)DG4A82.directory;
    if (id > 0 && next_matching_record(id) != SOUND_RECORD_NONE) {
        r = DG4A82.file_ptr;
        goto out;
    }

    cur = (const struct sound_dir_entry *)(void *)dir->cursor;

    if (id > 0) {
        for (si = 0; ; si++) {
            if (dir->count <= si)
                break;

            if (cur->id == id) {
                found = cur->at;
                break;
            }
            cur++;
        }

        {
            uint32_t at = found + 4;

            if (game_fseek(FILEREC_PTR(DG4A82.file_ptr), (int32_t)at, 0) != 0)
                goto fail;
        }

        if (found == 0)
            goto fail;

        {
            uint16_t ok;

            ok = read_record(FILEREC_PTR(DG4A82.file_ptr), dir->kind);
            if (ok == 0)
                goto out;
        }

        r = DG4A82.file_ptr;
        goto out;
    }

    for (si = 0; ; si++) {
        if (dir->count <= si)
            break;

        {
            /* The original loads the two words and adds four to the pair. */
            uint32_t at = cur->at + 4;

            if (game_fseek(FILEREC_PTR(DG4A82.file_ptr), (int32_t)at, 0) != 0)
                goto fail;
        }

        {
            uint16_t ok;

            ok = read_record(FILEREC_PTR(DG4A82.file_ptr), dir->kind);
            if (ok == 0)
                goto fail;
        }

        cur++;
    }

    r = DG4A82.file_ptr;
    goto out;

fail:
    if (DG4A82.file_ptr != 0 && DG4A82.file_kind != 0)
        close_file_record(FILEREC_PTR(DG4A82.file_ptr));

    if (DG4A82.directory != FAR_NULL_PTR)
        free_for_kind(DG4A82.directory, 0xa);

    remove_and_free_records(0);

    DG4A82.file_ptr = 0;
    DG4A82.directory = 0;
    r = 0;

out:
    return r;
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
    int16_t expect = 0, mask = 1;

    if (selector != -3) {
        SOUND_TICK_WAIT.selector = selector;
        SOUND_TICK_WAIT.cursor = DG4A82.records;
    } else if (SOUND_TICK_WAIT.cursor != SOUND_RECORD_NONE) {
        SOUND_TICK_WAIT.cursor = SOUND_TICK_WAIT.cursor->next;
    }

    if (SOUND_TICK_WAIT.selector == -2) {
        expect = 1;
    } else if (SOUND_TICK_WAIT.selector == -1) {
        /* mask 1, expect 0 - the defaults */
    } else if (SOUND_TICK_WAIT.selector == 0) {
        mask = 0;
        expect = 1;
    } else {
        /* Match on the identifier. */
        if (SOUND_TICK_WAIT.cursor == SOUND_RECORD_NONE || selector == -3) {
            SOUND_TICK_WAIT.cursor = NULL;
            return SOUND_RECORD_NONE;
        }

        for (;;) {
            if (SOUND_TICK_WAIT.cursor == SOUND_RECORD_NONE)
                break;
            if (SOUND_TICK_WAIT.cursor->id == selector)
                break;
            SOUND_TICK_WAIT.cursor = SOUND_TICK_WAIT.cursor->next;
        }
        return SOUND_TICK_WAIT.cursor;
    }

    while (SOUND_TICK_WAIT.cursor != SOUND_RECORD_NONE) {
        if ((((int16_t)SOUND_TICK_WAIT.cursor->flags & mask) ^ expect) != 0)
            break;
        SOUND_TICK_WAIT.cursor = SOUND_TICK_WAIT.cursor->next;
    }

    return SOUND_TICK_WAIT.cursor;
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
    struct sound_record *rec = DG4A82.records;

    while (rec != SOUND_RECORD_NONE) {
        if (rec->id == id)
            break;
        rec = rec->next;
    }

    if (rec == SOUND_RECORD_NONE)
        return 0;

    if ((rec->flags & 0x10) != 0)
        return 1;
    if (rec->data == FAR_NULL_PTR)
        return 1;
    if (rec->sequence != NULL)
        return 1;

    if ((rec->flags & 1) != 0) {
        const struct sound_record *other = DG4A82.records;

        while (other != SOUND_RECORD_NONE) {
            if ((other->flags & 1) != 0
                && other->sequence != NULL
                && other->id != id)
                stop_sequences(other->id);

            other = other->next;
        }

        if (((int16_t)DG4A82.voice_word) == 0 || ((int16_t)DG4A82.voice_word) == -1) {
            rec->flags |= 0x10;
            return 1;
        }

        {
            struct sequence *seq = create_sequence(rec->data);

            /* The sequence's pair, as the original files `create_sequence`'s
               DX:AX - a DOS block starting a segment. */
            rec->sequence = seq;
            if (seq == SEQUENCE_NONE)
                return 0;

            seq->loop = (uint8_t)((rec->flags & 2) ? 1 : 0);
            seq->priority = (uint8_t)rec->priority;

            if (load_and_start_sequence(seq, 0, 0x7f) == SEQUENCE_NONE)
                return 0;
            return 1;
        }
    }

    if (voice_playing(rec->data) != SEQUENCE_NONE)
        return 1;

    if (((int16_t)DG4A82.voice_word) == 0 || ((int16_t)DG4A82.voice_word) == -2) {
        if ((rec->flags & 2) != 0)
            rec->flags |= 0x10;
        return 1;
    }

    start_on_free_voice(rec->data,
                        0x7f,
                        (uint16_t)((rec->flags & 2) ? 1 : 0));
    return 1;
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

    if (DG4A82.driver != FAR_NULL_PTR
        || DG4A82.module != FAR_NULL_PTR)
        return 1;

    if (device == -1) {
        device = 2;
        si = 0;
    }

    if (setup_sound_device(device, module_index, callback, handle) == 0)
        return 0;

    if (si != 0 && (int16_t)(int8_t)TIMER.installed == 0) {
        timer_install(0xd);
        DG4A82.timer_taken = 1;
    }

    if (si != 0) {
        DG4A82.tick_handle = (int16_t)timer_add_callback(sound_service, 4);
        if (DG4A82.tick_handle == 0 && si != 0)
            return 0;
    } else if (si != 0) {
        return 0;
    }

    if (si != 0 && (DG4A82.module != FAR_NULL_PTR))
        DG4A82.module_handle = (int16_t)timer_add_callback(SOUND_MODULE_TICK, 2);

    alloc_voice_records();
    return 1;
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
    if (DG4A82.driver == FAR_NULL_PTR
        && DG4A82.module == FAR_NULL_PTR)
        return;

    remove_and_free_records(0);

    if (DG4A82.directory != FAR_NULL_PTR)
        free_for_kind(DG4A82.directory, 0xa);

    if (DG4A82.file_ptr != 0 && DG4A82.file_kind != 0)
        close_file_record(FILEREC_PTR(DG4A82.file_ptr));

    if (((int16_t)DG4A82.tick_handle) != 0) {
        timer_drop_callback(DG4A82.tick_handle);
        DG4A82.tick_handle = 0;
    }

    if (((int16_t)DG4A82.module_handle) != 0) {
        timer_drop_callback(DG4A82.module_handle);
        DG4A82.module_handle = 0;
    }

    if (((int16_t)DG4A82.timer_taken) != 0) {
        timer_remove();
        DG4A82.timer_taken = 0;
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
uint16_t read_record(FILE *file, uint16_t mode)
{
    /* The original reserves 0xe and then pushes SI and DI; the port used to
       reserve all 0x12 so a callee's frame cleared the saved registers too.
       An array's neighbours are its own bytes, so the size is the locals. */
    /* **One `long`, not two words.** The original reads four bytes into
       `[bp-4]` and then takes four off with a `sub`/`sbb` pair, and every use
       hands the whole thing to a routine that takes a 32-bit size. */
    int32_t len;
    int16_t out[2];
    /* **Two bytes read three times, at two widths.** `game_fread` fills it
       with a word once and with a single byte twice, all at offset 0, so it
       is a byte buffer with one widening read rather than a record - which
       is why `framify.py` refuses it and this one is spelled by hand. */
    uint8_t scratch[6];
    struct sound_record *rec;
    uint16_t kind;
    uint8_t *p;
    uint16_t r = 0;

    game_fread((uint8_t *)&len, 4, 1, file);
    game_fread(scratch, 2, 1, file);

    rec = (struct sound_record *)(void *)alloc_for_kind(sizeof(struct sound_record), 3);
    if (rec == SOUND_RECORD_NONE)
        goto out_;

    rec->id = *(int16_t *)(scratch);

    game_fread(scratch, 1, 1, file);
    rec->priority = *scratch;

    game_fread(scratch, 1, 1, file);
    rec->flags = *scratch;

    kind = (rec->flags & 1) ? 4 : 7;

    /* `sub ax,4 / sbb dx,0` - the borrow the two words needed by hand. */
    len -= 4;

    rec->data = NULL;

    /* Each payload is a block DOS handed out, so `far_of` files its own
       pair, as the original files the DX:AX it was answered. */
    if ((uint8_t)mode == 0x63) {
        p = alloc_for_kind((uint32_t)len, kind);
        rec->data = (p);

        if (p == FAR_NULL_PTR)
            goto fail;

        if (fread_huge(p, (uint32_t)len, 1, file) != 1)
            goto fail;
    } else if (((int16_t)DG4A82.bank_choice) != 0) {
        p = load_sound_bank(file, (uint32_t)len, (uint8_t *)out);

        rec->data = (p);
        if (p == FAR_NULL_PTR)
            goto fail;
    } else {
        p = load_resource_block(file, (uint32_t)len, (uint8_t *)out, kind);

        rec->data = (p);
        if (p == FAR_NULL_PTR)
            goto fail;
    }

    rec->next = DG4A82.records;
    rec->size = (uint16_t)out[0];

    DG4A82.records = rec;
    r = 1;
    goto out_;

fail:
    free_for_kind((uint8_t *)rec, 3);

out_:
    return r;
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
    uint8_t *blk;

    if (kind == 6 || kind == 8) {
        /* The near heap takes a word: 0x29f9d pushes `[bp+6]` alone. The
           pair is DS and the offset `malloc` answered, `mov [bp-2],ds` at
           0x29fab - so a refusal is DGROUP:0000, not 0000:0000. That pair is
           not normalised, so a caller filing it as `far_of` would file
           different bytes; nothing in the game asks for these two kinds. */
        blk = io_malloc((uint16_t)size);
        if (blk == NULL)
            blk = dgroup;
    } else {
        /* **The same Borland `long`, passed straight on.** 0x29fb7 pushes
           `[bp+8]` then `[bp+6]` into `dos_alloc_bytes` without touching
           either half. */
        blk = dos_alloc_bytes(size, 0, 0).ptr;
    }

    if (blk != FAR_NULL_PTR
        && (kind == 2 || kind == 3 || kind == 4 || kind == 7))
        far_memset(blk, 0, size);

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
        /* The near heap takes only the offset, `push [bp+6]`; the pair
           `alloc_for_kind` answered for these kinds is DS's, so the pointer
           is that offset in DGROUP. */
        io_free(blk);
        return;
    }
    dos_free_far(blk);
}
