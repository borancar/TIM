/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Open a sound file and load one record out of it, or all of them.**
 *
 * A module of the sound library, in 1.11 **code segment 2ae1** on its own,
 * image 0x2ae14..0x2b0bf. 1.00 linked the library's C into one segment,
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
 * 0x2ae14
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
