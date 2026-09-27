/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The game's files**: opening, reading, seeking and writing a file that
 * may be a loose file or an entry in one of the resource archives, the
 * archives' name map and its hash, and the DOS critical-error handler that
 * stands in while they are read.
 *
 * One module of the original's **code segment 0000** (`_TEXT`), image
 * 0x08fc3..0x09e4c. From 0x08fc3 to 0x09afe its routines reach each other
 * with bare `push cs / call`, and its `_DATA` is 0x28d2..0x28ec, which TLINK
 * put after levels.c's. **Its end is ours**: nothing reaches back across
 * 0x09afe..0x09e4c, and the file ends where fstring.c's far string helpers
 * begin. The rectangle routine at 0x09c23, which nothing calls, is here only
 * because nothing says it is anywhere else.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 * JUDGE: data 0x28d2..0x28ec
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **Which four characters of a filename its hash is made of**, at DGROUP
 * 0x28d2: `hash_filename` folds the bytes at these positions of the padded
 * name - 0, 1, 6, 7 in the image - into a long.
 *
 * DGROUP 0x28d2..0x28d6, 0x04 bytes.
 */
struct machine_hash_order {
    uint8_t   hash_order[4];      /* +0x00 [4] */
} PACKED;

struct machine_hash_order MACHINE_HASH_ORDER DGROUP_AT(0x28d2) = { .hash_order = { 0x00, 0x01, 0x06, 0x07 } };

/*
 * **The resource map's name and three modes**, DGROUP 0x28d6..0x28ec:
 * "RESOURCE.MAP" and "rb" three times, one for each of the routines at
 * 0x0964b, 0x09a7f and 0x09b02 that opens a file.
 */
struct machine_resource_map_names {
    char      resource_map[13];   /* +0x00  "RESOURCE.MAP" */
    char      mode_rb_a[3];       /* +0x0d  "rb" */
    char      mode_rb_b[3];       /* +0x10  "rb" */
    char      mode_rb_c[3];       /* +0x13  "rb" */
} PACKED;

struct machine_resource_map_names MACHINE_RESOURCE_MAP_NAMES DGROUP_AT(0x28d6) = {
    .resource_map = "RESOURCE.MAP",
    .mode_rb_a = "rb",
    .mode_rb_b = "rb",
    .mode_rb_c = "rb",
};

/*
 * **The eleven archives**, DGROUP 0x548f..0x55c3, 0x134 bytes. Index 0 is never where a search
 * begins - `find_entry_for_pointer` starts at 1 when `last_record` is clear.
 */
struct machine_archives {
    struct archive slot[0xb];     /* +0x00 [0x134] */
} PACKED;

struct machine_archives MACHINE_ARCHIVES DGROUP_WAS(0x548f);

/*
 * **The ten game files**, DGROUP 0x55c3..0x5677, 0xb4 bytes.
 */
struct machine_game_files {
    struct game_file files[0xa];  /* +0x00 [0xb4] */
} PACKED;

struct machine_game_files MACHINE_GAME_FILES DGROUP_BSS(0x55c3);

/*
 * NOT a transcription: a boundary the port chose. The enclosing routine
 * contains three inlined copies of this loop, and this is the loop, lifted out
 * so the port writes it once. Its address is given below rather than here,
 * because an address on the first line of a block *is* the provenance mark.
 *
 * Walk each list once, from its head to either a null entry or a match. The
 * body matches 0x09904..0x09941 instruction for instruction - the 0x1c-strided
 * table at 0x54a7/0x54a9, the `(off | seg) == 0` test, the compare against the
 * wanted pair, the `off += 8` - but **the enclosing routine is not
 * transcribed**: 0x098e0 takes one argument at bp+6, reads what it is looking
 * for out of the globals at 0x5482 and 0x5484 rather than from arguments, and
 * carries on past this loop at 0x09941. Its signature here is the helper's,
 * not the original's.
 *
 * It carried a bare `0x098e0` before, which files a routine as transcribed
 * under `tests/provenance.py`, and this is not one.
 */
void scan_entry_list(int16_t idx, uint32_t want,
                     const struct archive_entry **at)
{
    *at = (const struct archive_entry *)(void *)MACHINE_ARCHIVES.slot[idx].list;

    for (;;) {
        /* Each entry opens with its 32-bit key; a zero one ends the list.
           The original steps the offset eight bytes inside the list's
           segment, and the list is sized in a word, so it cannot leave it. */
        uint32_t key = (*at)->key;

        if (key == 0 || key == want)
            return;
        (*at)++;
    }
}

/*
 * 0x08fc3
 *
 * Takes one argument, ignores it, and answers 1. Six instructions: a frame,
 * `mov ax, 1`, and a jump to the epilogue that goes nowhere.
 *
 * Both its callers are on a failure path in the file layer - one of them after
 * a failed open, having first checked three flags - and it sits immediately
 * before `game_fopen` in the same source file. That reads like the routine
 * that would have asked the user what to do about a disk error, left answering
 * "carry on" in the shipped build. **That is a reading of where it is called
 * from, not something the bytes say**: what the bytes say is that it answers 1.
 */
int16_t answer_carry_on(uint16_t what)
{
    (void)what;
    return 1;
}

/*
 * 0x08fcd
 *
 * The game's own `fopen`. Answers one of the ten 0x12-byte archive-entry
 * blocks at DGROUP 0x55c3, not a `FILE` - which is why every one of
 * `game_fread`, `game_fgetc`, `game_fseek` and `game_ftell` starts by asking
 * `archive_entry_for` whether the thing it was handed is one of these.
 *
 * With no archive loaded it is a plain forward to the runtime's `fopen` and the
 * caller gets a real `FILE`.
 *
 * Otherwise a free block is found - +0xe is the in-use flag - and the name
 * hashed, which leaves the hash at DGROUP 0x5482 for `find_entry_for_pointer`
 * to look up. Then the **loose file is tried first**: if a real file of that
 * name exists it is opened and the block simply carries its `FILE` at +0x10.
 * That is how a patched or unpacked file overrides the archive.
 *
 * Failing that the archive is searched. The entry's own header is read - a
 * 13-byte name and a 4-byte size - and the name compared case-insensitively
 * against the one asked for, which is the check that the hash found the right
 * file rather than a colliding one. The entry's data then starts where `ftell`
 * says the file now is.
 *
 * The retry loop around the loose-file open exists for removable media: 0x5488
 * is set by the critical-error handler and 0x38ad says whether to prompt. Both
 * are dead here.
 */
FILE *game_fopen(char *name, const char *mode)
{
    char hdr[16];
    struct game_file *si;
    struct file_rec *di;
    int16_t left;
    FILE *r = NULL;

    if (DG547A.reopen != 0)
        make_file_current(0);

    load_archive_map();
    DG5677.failures = 0;

    if (DG547A.archive_count == 0) {
        r = borland_fopen(name, mode);
        goto out;
    }

    DG547A.file_used_ptr = 0;
    DG547A.file_asked_ptr = 0;

    si = &MACHINE_GAME_FILES.files[0];
    for (left = 0xa; left != 0; left--) {
        if (si->in_use == 0)
            break;
        si++;
    }

    if (left == 0)
        goto out;

    hash_filename(name);

    DG547A.opening = 1;

    for (;;) {
        DG547A.retry = 0;
        di = borland_fopen(name, mode);

        if (((int16_t)DG4E67.file_op_active) != 0) {
            r = di;
            goto out;
        }
        if (DG547A.retry != 0 && ((uint8_t)VMDS.pixel_shift) != 0)
            not_transcribed("0x08fc3, the prompt for a missing disk");
        if (DG547A.retry == 0)
            break;
    }

    DG547A.opening = 0;

    if (di != NULL) {
        si->archive = 0;
        si->pos = 0;
        si->size = 0;
        si->base = 0;
        si->in_use = 1;
        si->stream_ptr = dg_near(dgroup, di);
        goto found;
    }

    if (find_entry_for_pointer(si) == 0)
        goto out;

    make_file_current(si->archive);

    {
        uint32_t at = si->base + si->pos;
        int32_t pos;
        struct archive *a;

        seek_file_to(at);

        di = FILEREC_PTR(MACHINE_ARCHIVES.slot[DG547A.last_record].stream_ptr);

        borland_fread((uint8_t *)hdr, 0xd, 1, di);
        borland_fread((uint8_t *)&si->size, 4, 1, di);

        pos = borland_ftell(di);
        si->base = (uint32_t)pos;

        a = &MACHINE_ARCHIVES.slot[DG547A.last_record];
        a->pos = (uint32_t)pos;
    }

    if (string_compare_nocase(hdr, name) != 0)
        goto out;

    si->pos = 0;
    si->stream_ptr = 0;
    si->in_use = 1;

found:
    DG547A.open_immediate = (uint8_t)(DG547A.open_immediate + 1);
    /* An archive entry: the game's FILE is its `game_file` record, and
       `archive_entry_for` tells the two apart by looking for it in the
       table. */
    r = (FILE *)si;

out:
    return r;
}

/*
 * 0x0917f
 *
 * The game's own `fclose`, and the last of the set over the archive.
 *
 * A null `FILE` is -1 before anything else. With the archive open and an entry
 * for this stream, the close is against the entry rather than the stream: the
 * one-entry cache is thrown away by looking up handle 0, whatever `FILE` the
 * entry carries at +0x10 is closed, its +0xe cleared, and the count of open
 * resources at DGROUP 0x5486 dropped by one.
 *
 * A stream with no entry is closed directly through the runtime, and skips both
 * the entry bookkeeping and the open count. That is the path a **saved
 * machine** takes: it was opened by name and never came out of the archive.
 *
 * A failure sets bit 0 of DGROUP 0x567b, which is where this layer collects
 * whether anything went wrong.
 */
int16_t game_fclose(FILE *file)
{
    struct game_file *si = GAME_FILE_NONE;
    int16_t di = 0;

    if (file == 0)
        return -1;

    if (DG547A.archive_count != 0)
        si = archive_entry_for(file);

    if (si == GAME_FILE_NONE) {
        di = borland_fclose(file);
        DG5677.failures = (int16_t)(DG5677.failures | (di == -1 ? 1 : 0));
        return di;
    }

    archive_entry_for(0);

    if (si->stream_ptr != 0)
        di = borland_fclose(FILEREC_PTR(si->stream_ptr));

    si->in_use = 0;
    DG547A.open_immediate = (uint8_t)(DG547A.open_immediate - 1);

    DG5677.failures = (int16_t)(DG5677.failures | (di == -1 ? 1 : 0));
    return di;
}

/*
 * 0x091ef
 *
 * The game's own `fread`. Everything that reads a resource comes through here,
 * and it decides between the loose file and the archive.
 *
 * With the archive closed - DGROUP 0x547e zero - or with no entry for this
 * `FILE`, it is a plain forward to the runtime's `fread` and nothing else
 * happens. An entry whose +0x10 holds a `FILE` of its own is forwarded the same
 * way, with that one substituted.
 *
 * Otherwise the read is against a stretch of the archive, and three things have
 * to happen that a plain `fread` would not do.
 *
 * The request is **clamped** to what is left of the entry, by taking whole
 * items off the count until `size * count` fits in `+6:+8` minus the position
 * at `+0xa:+0xc`. The comparison is 32-bit with the request's high half a
 * constant zero, which is why the first branch of it can never be taken.
 *
 * Then the archive file is made current and seeked to the entry's base plus the
 * position, and the `FILE` to read from is looked up in the table at DGROUP
 * 0x549f, 0x1c bytes per open archive.
 *
 * Afterwards the entry's position and the archive's own running total at
 * 0x54a1 both advance by what was actually read - `n * size`, not what was
 * asked for.
 */
uint16_t game_fread(uint8_t * buf, uint16_t size, uint16_t count,
                    FILE *file)
{
    struct game_file *di = GAME_FILE_NONE;

    if (DG547A.archive_count != 0)
        di = archive_entry_for(file);

    if (di == GAME_FILE_NONE)
        return borland_fread(buf, size, count, file);

    if (di->stream_ptr != 0)
        return borland_fread(buf, size, count, FILEREC_PTR(di->stream_ptr));

    {
        uint16_t bytes = (uint16_t)((int16_t)size * (int16_t)count);
        uint16_t n, got;

        for (;;) {
            if (bytes == 0)
                break;

            uint32_t left = di->size - di->pos;

            if ((left >> 16) != 0)
                break;
            if (bytes <= (uint16_t)left)
                break;

            count--;
            bytes = (uint16_t)(bytes - size);
        }

        make_file_current(di->archive);

        {
            uint32_t at = di->base + di->pos;

            seek_file_to(at);
        }

        file = FILEREC_PTR(MACHINE_ARCHIVES.slot[di->archive].stream_ptr);

        n = borland_fread(buf, size, count, file);

        got = (uint16_t)((int16_t)n * (int16_t)size);

        di->pos += got;

        {
            struct archive *a = &MACHINE_ARCHIVES.slot[di->archive];

            a->pos += got;
        }

        return n;
    }
}

/*
 * 0x092dc
 *
 * The game's own `fseek`, and `game_fread`'s counterpart: the same choice
 * between the loose file and the archive, the same substitution when an entry
 * carries its own `FILE`.
 *
 * Against an archive entry there is no seeking to do on the file at all - only
 * the entry's own position at +0xa:+0xc is moved, and the archive file is
 * seeked when something is actually read. So the three whences are worked out
 * here in 32-bit arithmetic:
 *
 *   0  the offset as given
 *   1  the offset plus the position now
 *   2  the entry's size minus the offset, or zero if the offset is larger
 *
 * and the result is clamped to the entry's size, so seeking past the end parks
 * at the end rather than reporting an error. The answer is 0 either way.
 */
int16_t game_fseek(FILE *file, int32_t off, int16_t whence)
{
    struct game_file *si = GAME_FILE_NONE;

    if (DG547A.archive_count != 0)
        si = archive_entry_for(file);

    if (si == GAME_FILE_NONE)
        return borland_fseek(file, off, whence);

    if (si->stream_ptr != 0)
        return borland_fseek(FILEREC_PTR(si->stream_ptr), off, whence);

    /* Every comparison here is **unsigned** over the pair, which is the
       original's `cmp hi / ja / jb / cmp lo / ja` and not the signed shape
       `resource_seek` uses. */
    if (whence == 1) {
        off = (int32_t)((uint32_t)off + si->pos);
    } else if (whence == 2) {
        if (si->size > (uint32_t)off)
            off = (int32_t)(si->size - (uint32_t)off);
        else
            off = 0;
    }

    if (si->size < (uint32_t)off)
        off = (int32_t)si->size;

    si->pos = (uint32_t)off;
    return 0;
}

/*
 * 0x093a2
 *
 * The game's own `ftell`, and the third of the trio over the archive - the same
 * choice between the loose file and the archive as `game_fread` and
 * `game_fseek`, and the same substitution when an entry carries its own `FILE`.
 *
 * Inside an archive entry the answer is the entry's own position at +0xa:+0xc,
 * not the file's, so a caller sees the resource as if it were a file of its
 * own.
 */
int32_t game_ftell(FILE *file)
{
    struct game_file *si = GAME_FILE_NONE;

    if (DG547A.archive_count != 0)
        si = archive_entry_for(file);

    if (si == GAME_FILE_NONE)
        return borland_ftell(file);

    if (si->stream_ptr != 0)
        return borland_ftell(FILEREC_PTR(si->stream_ptr));

    return (int32_t)si->pos;
}

/*
 * 0x093e0
 *
 * `rewind` over the archive: `game_fseek` to nought from the start, and
 * nothing else. Five pushes and a call.
 */
void game_rewind(FILE *file)
{
    game_fseek(file, 0, 0);
}

/*
 * 0x093f6
 *
 * The game's own `fgetc`, and `game_fread`'s shape one byte at a time: the same
 * choice between the loose file and the archive, the same substitution when an
 * entry carries a `FILE` at +0x10, the same table at DGROUP 0x549f.
 *
 * The two DGROUP cells it sets on the way through - 0x548d with the `FILE` it
 * was asked about and 0x548b with the one it actually read from - are written
 * on every path, including the plain one, so something downstream reads them.
 *
 * At the end of an entry it answers -1 without touching the file at all. The
 * test is a 32-bit compare of the position at +0xa:+0xc against the size at
 * +6:+8, written high half first.
 *
 * Both the entry position and the archive's running total advance by one.
 */
int16_t game_fgetc(FILE *file)
{
    struct game_file *si = GAME_FILE_NONE;

    DG547A.file_asked_ptr = dg_near(dgroup, file);

    if (DG547A.archive_count != 0)
        si = archive_entry_for(file);

    if (si == GAME_FILE_NONE) {
        DG547A.file_used_ptr = dg_near(dgroup, file);
        return borland_fgetc(file);
    }

    if (si->stream_ptr != 0) {
        DG547A.file_used_ptr = si->stream_ptr;
        return borland_fgetc(FILEREC_PTR(si->stream_ptr));
    }

    if (si->pos >= si->size)
        return -1;

    make_file_current(si->archive);

    {
        uint32_t at = si->base + si->pos;
        int16_t got;
        struct archive *a;

        seek_file_to(at);

        file = FILEREC_PTR(MACHINE_ARCHIVES.slot[si->archive].stream_ptr);
        DG547A.file_used_ptr = dg_near(dgroup, file);
        got = borland_fgetc(file);

        si->pos++;

        a = &MACHINE_ARCHIVES.slot[si->archive];
        a->pos++;

        return got;
    }
}

/*
 * 0x094fb
 *
 * **`fwrite`, through the archive layer.** A pointer, an element size, a count
 * and a file, answering how many elements went - and every caller in the
 * machine writer compares that with 1.
 *
 * **A file may be an entry in the resource archive rather than a file of its
 * own**, and when the archive is in use at DGROUP 0x547e this asks
 * `archive_entry_for` first. An entry writes to the handle at its +0x10; an
 * entry without one writes *nothing* and answers zero, which the callers then
 * read as a short write. A file that is not an entry at all falls through to
 * the plain path, and so does everything when the archive is not in use.
 *
 * The failure mark at DGROUP 0x567b is **or-ed, not set**: it accumulates
 * across every write anyone does rather than describing this one. That is a
 * different thing from the machine writer's own 0x5478, which is per-file and
 * checked before each field - the two exist together and neither is the other.
 */
uint16_t game_fwrite(const uint8_t * ptr, uint16_t size, uint16_t count,
                     FILE *file)
{
    uint16_t n;

    if (((uint16_t)DG547A.archive_count) != 0) {
        struct game_file *entry = archive_entry_for(file);

        if (entry != GAME_FILE_NONE) {
            if (entry->stream_ptr != 0)
                n = borland_fwrite(ptr, size, count,
                              FILEREC_PTR(entry->stream_ptr));
            else
                n = 0;

            DG5677.failures |= (uint16_t)(n != count ? 1 : 0);
            return n;
        }
    }

    n = borland_fwrite(ptr, size, count, file);

    DG5677.failures |= (uint16_t)(n != count ? 1 : 0);
    return n;
}

/*
 * 0x09571
 *
 * **`fputc`, through the archive layer** - `game_fwrite`'s shape for one
 * byte. With the archive in use at DGROUP 0x547e an entry writes through the
 * handle at its +0x10, and an entry without one writes nothing and answers
 * -1; a file that is not an entry, and everything when the archive is not in
 * use, goes to `borland_fputc` directly. The answer is `fputc`'s, and the
 * failure mark at 0x567b is or-ed with "it was -1", once, on every path -
 * the original computes that at one exit and this does it on each, which is
 * the same word either way.
 *
 * **Nothing the port runs reaches it.** Its two callers in the image are the
 * resource writer: `open_resource`'s write branch, which is a stub here, and
 * the byte writer at 0x1c5f5, which is not transcribed. The game never writes
 * a resource in play, so this is read rather than measured, and its spec will
 * say "never called".
 */
int16_t game_fputc(int16_t c, FILE *file)
{
    int16_t n;

    if (((uint16_t)DG547A.archive_count) != 0) {
        struct game_file *entry = archive_entry_for(file);

        if (entry != GAME_FILE_NONE) {
            if (entry->stream_ptr != 0)
                n = borland_fputc(c, FILEREC_PTR(entry->stream_ptr));
            else
                n = -1;

            DG5677.failures |= (uint16_t)(n == -1 ? 1 : 0);
            return n;
        }
    }

    n = borland_fputc(c, file);

    DG5677.failures |= (uint16_t)(n == -1 ? 1 : 0);
    return n;
}

/*
 * 0x095cf
 *
 * Give a file a buffer, whether it is a loose file or one inside the archive.
 *
 * The difference is **which file gets buffered**, not how. A file the archive
 * knows about has a record from `archive_entry_for`, and that record's +0x10
 * is the handle of the archive it lives in - so the buffer goes on the archive
 * rather than on the caller's own handle, which is the one that will actually
 * be read from. A loose file, or a record with no archive behind it, gets the
 * buffer on itself.
 *
 * DGROUP 0x547e is whether the archive is in use at all; with it clear the
 * lookup is skipped.
 */
void game_setbuf(FILE *file, uint8_t *buf)
{
    struct game_file *rec = GAME_FILE_NONE;

    if (((uint16_t)DG547A.archive_count) != 0)
        rec = archive_entry_for(file);

    if (rec == GAME_FILE_NONE) {
        borland_setbuf(file, buf);
        return;
    }

    if (rec->stream_ptr != 0)
        borland_setbuf(FILEREC_PTR(rec->stream_ptr), buf);
}

/*
 * 0x0960f
 *
 * Load `RESOURCE.MAP`, which is what tells the game where everything in the
 * archives is. Runs once - DGROUP 0x548a is the flag that says so.
 *
 * Before opening anything it takes over **INT 24h**, DOS's critical-error
 * handler, keeping the old vector at DGROUP 0x5677. That is what stops a
 * missing disk from aborting the program, and the handler it installs is at
 * 0x9bdf. The segment pushed for it reads 0x0000 in the image and is a
 * relocation; the port works it out from where the program is.
 *
 * The file itself is four bytes into the table at DGROUP 0x28d2 - the byte
 * offsets `hash_filename` packs, so the hash function is **defined by the
 * file**, not by the program - then a count of archives, and then for each
 * archive a 13-byte name into its 0x1c-byte record at DGROUP 0x548f, a count of
 * entries, and a block of eight bytes per entry holding a hash and an offset.
 *
 * The block is one entry longer than the count, which leaves room for the
 * terminator the lookup relies on.
 *
 * The count of archives **accumulates** into DGROUP 0x547e, and the first index
 * of this map is worked out from it afterwards, so a second map would append
 * rather than replace.
 */
void load_archive_map(void)
{
    int16_t count;               /* [bp-8] */
    uint32_t lo;                 /* [bp-0xc] the entry's key */
    uint32_t hi;                 /* [bp-0x10] and where its data starts */
    struct file_rec *file;
    int16_t di;

    if (DG547A.scanned != 0) {
        return;
    }

    DG5677.crit_vec = dos_getvect(0x24);

    dos_setvect(0x24, (struct far_ptr){ 0x9bdf, (uint16_t)(IMAGE_BASE >> 4) });
    DG547A.scanned = 1;

    file = borland_fopen(MACHINE_RESOURCE_MAP_NAMES.resource_map, MACHINE_RESOURCE_MAP_NAMES.mode_rb_a);
    if (file == 0) {
        return;
    }

    borland_fread((uint8_t *)MACHINE_HASH_ORDER.hash_order, 4, 1, file);
    borland_fread((uint8_t *)&count, 2, 1, file);

    DG547A.archive_count = (int16_t)(DG547A.archive_count + count);
    di = (int16_t)(DG547A.archive_count - count + 1);

    for (; di <= DG547A.archive_count; di++) {
        struct archive *a = &MACHINE_ARCHIVES.slot[di];
        struct archive_entry *e;

        borland_fread((uint8_t *)a->name, 0xd, 1, file);
        borland_fread((uint8_t *)&count, 2, 1, file);

        /* **Zeroed**, which is what writes the terminator: the block is one
           entry longer than the count and the lookup stops on an all-zero key.
           The flags are the fourth argument - `push 1`, then `push 0` for the
           third - and the port had the two the other way round, so the block
           was not cleared and the terminator was whatever the memory held. */
        a->list = (dos_alloc_bytes((uint16_t)((count + 1) << 3), 0, 1).ptr);
        a->index = (uint16_t)di;

        /* The original steps the far pointer at [bp-6] by eight, one entry. */
        e = (struct archive_entry *)a->list;
        while (count != 0) {
            count--;

            borland_fread((uint8_t *)&lo, 4, 1, file);
            borland_fread((uint8_t *)&hi, 4, 1, file);

            e->key = lo;
            e->base = hi;
            e++;
        }
    }

    borland_fclose(file);
}

/*
 * 0x09784
 *
 * Free each archive's list of entries - the far pointer at +0x18 of the eleven
 * 0x1c-byte records from DGROUP 0x548f, which `load_archive_map` allocated -
 * and then put DOS's critical-error vector 24h back from the pair kept at
 * 0x5677.
 *
 * The loop is `si <= 10`, so eleven and not ten, which is what says the table
 * has eleven records rather than the ten the `game_file` table has. Each
 * pointer is tested as a pair before it is freed, and zeroed after.
 *
 * It was called `free_sound_slots` for as long as its stride was the only
 * thing read about it: 0x54a7 is a record's +0x18 and not a table of its own,
 * and there is no sound anywhere near it.
 */
void free_archive_lists(void)
{
    int16_t i;

    for (i = 0; i <= 10; i++) {
        struct archive *a = &MACHINE_ARCHIVES.slot[i];

        if (a->list == NULL)
            continue;

        dos_free_far(a->list);

        a->list = NULL;
    }

    if (dg_far_ptr(DG5677.crit_vec) != FAR_NULL_PTR) {
        dos_setvect(0x24, DG5677.crit_vec);
        DG5677.crit_vec = FAR_NULL;
    }

    DG547A.scanned = 0;
}

/*
 * 0x0980d
 *
 * Hash a filename, answering the hash in DX:AX and leaving it at DGROUP
 * 0x5482 as well. A null name answers zero and stores zero.
 *
 * The name is **uppercased in place**, in the caller's own buffer, and any
 * `\\` or `:` restarts the two running values and moves the start of the name
 * past it - so only the last path component counts and the caller's pointer is
 * left pointing at it.
 *
 * Two things are accumulated over the name: a sum and an exclusive-or. Then the
 * last component is copied into a 13-byte buffer, padded with zeros, and four
 * of its bytes - at the offsets in the table at DGROUP 0x28d2 - are packed into
 * a 32-bit value eight bits at a time. The sum times the exclusive-or is added
 * to that, **as a 16-bit product sign-extended**: the `imul` computes 32 bits
 * and the `cwd` after it throws the top half away, which is the compiler
 * treating the result as an `int`.
 */
int32_t hash_filename(char *name)
{
    char buf[22];
    char *si;
    uint16_t sum = 0, eor = 0;
    uint32_t acc = 0;
    int16_t i;

    if (name == NULL) {
        DG547A.name_hash = 0;
        return 0;
    }

    si = name;
    while (*si != 0) {
        uint8_t c;

        if (*si >= 'a' && *si <= 'z')
            *si = (char)(*si ^ 0x20);

        c = (uint8_t)*si;
        sum = (uint16_t)(sum + c);
        eor ^= c;

        if (*si == '\\' || *si == ':') {
            eor = 0;
            sum = 0;
            name = si + 1;
        }
        si++;
    }

    string_copy_padded(buf, name, 0xd);

    for (i = 0; i < 4; i++) {
        uint8_t c = (uint8_t)buf[MACHINE_HASH_ORDER.hash_order[i]];

        acc = long_shift_left(acc, 8) + c;
    }

    acc = (uint32_t)((int32_t)acc
                     + (int32_t)(int16_t)(sum * eor));

    DG547A.name_hash = acc;
    return (int32_t)acc;
}

/*
 * 0x098e0
 *
 * Find which archive holds a file, and answer whether one does.
 *
 * What is looked for is not an argument - it is the **filename hash** at
 * 0x5482, which `hash_filename` leaves there. Records are 0x1c bytes from
 * 0x54a7, and each one's first field is a far pointer to the list of
 * eight-byte entries `load_archive_map` read: the hash at +0 and the file's
 * offset within the archive at +4, ending at an all-zero hash. 0x5480 holds the
 * record last used and 0x547e the highest valid index.
 *
 * The search starts at 0x5480 - or at 1 if that is zero, so record 0 is never
 * where a search begins - and then spirals outward one index at a time,
 * forward first and backward second, re-reading both globals on every pass.
 * That is a locality bet: the pointer being asked about is usually in the
 * record that was just used.
 *
 * Each step scans a whole list, so a step can stop on a null entry rather than
 * a match; the outer loop re-checks and keeps going while either direction has
 * indices left. Whichever record was scanned last is the one reported, so the
 * final check decides between a real hit and having simply run out.
 *
 * On success the `game_file` the caller passes takes the archive index and the
 * entry's offset, and has its size and position zeroed. The original writes
 * those four as **four 16-bit stores**, high half first - the port had them as
 * two `DG32`, which is byte for byte the same only because the value is zero.
 */
int16_t find_entry_for_pointer(struct game_file *out)
{
    uint32_t want = DG547A.name_hash;
    const struct archive_entry *at;
    int16_t idx, fwd, back;

    idx = ((int16_t)DG547A.last_record);
    if (idx == 0)
        idx = 1;
    scan_entry_list(idx, want, &at);

    fwd = (int16_t)(((int16_t)DG547A.last_record) + 1);
    back = (int16_t)(((int16_t)DG547A.last_record) - 1);

    for (;;) {
        if (at->key == want)
            break;
        if (back <= 0 && fwd > DG547A.archive_count)
            break;

        if (fwd <= DG547A.archive_count) {
            idx = fwd++;
            scan_entry_list(idx, want, &at);
        }

        if (at->key == want)
            continue;
        if (back <= 0)
            continue;
        idx = back--;
        scan_entry_list(idx, want, &at);
    }

    if (at->key != want)
        return 0;

    out->archive = (uint16_t)idx;
    out->base = at->base;
    out->pos = 0;
    out->size = 0;
    return 1;
}

/*
 * 0x09a62
 *
 * Make a given resource file the open one, opening it and closing whatever was
 * open before.
 *
 * The first thing it does is a **file-exists test written as an open and an
 * immediate close**: with the flag at 0x5486 clear it tries the file by name
 * and shuts it again, keeping only whether that worked. That is the loose-file
 * probe - the game asks whether a real file is there before settling for the
 * packed copy - and finding one forces a reopen even when the same index is
 * already current.
 *
 * Without that, an index that is already current returns immediately, which is
 * why the routine is cheap enough to call before every read: 18,930 calls, 26
 * of which reach DOS.
 *
 * The open itself retries forever. A failure calls the prompt at 0x08fc3 - but
 * only while 0x38ad is set - and tries again, which is how a program on
 * removable media asks for the right disk. With 0x38ad clear it spins on
 * `fopen` with nothing to change the answer.
 *
 * Afterwards the believed file position at +0x12 is zeroed, because a freshly
 * opened file is at nought, and `archive_entry_for(0)` is called to throw away
 * the one-entry cache - the `FILE` pointers it remembers are about to be stale.
 *
 * The fast path above is the common one - 18,930 calls against 26 that open
 * anything - but every occurrence the harness samples is off it, which is why
 * this needed the runtime's own `fopen` before it could be checked at all.
 */
void make_file_current(uint16_t index)
{
    struct archive *a;
    int16_t exists = 0;

    if (DG547A.open_immediate == 0 && index != 0) {
        struct file_rec *f = borland_fopen((const char *)MACHINE_ARCHIVES.slot[index].name,
                                 MACHINE_RESOURCE_MAP_NAMES.mode_rb_b);

        borland_fclose(f);
        if (f != 0)
            exists = 1;
    }

    if (index == DG547A.last_record && exists == 0 && DG547A.reopen == 0)
        return;

    a = &MACHINE_ARCHIVES.slot[DG547A.last_record];
    if (a->stream_ptr != 0) {
        borland_fclose(FILEREC_PTR(a->stream_ptr));
        a->stream_ptr = 0;
    }

    DG547A.last_record = (int16_t)index;
    a = &MACHINE_ARCHIVES.slot[DG547A.last_record];

    if (index != 0) {
        DG547A.opening = 1;
        for (;;) {
            struct file_rec *f = borland_fopen((const char *)a->name,
                                     MACHINE_RESOURCE_MAP_NAMES.mode_rb_c);

            a->stream_ptr = dg_near(dgroup, f);
            if (f != 0)
                break;
            if (((uint8_t)VMDS.pixel_shift) != 0)
                not_transcribed("0x08fc3, the prompt for a missing disk");
        }
        DG547A.opening = 0;
    }

    a->pos = 0;

    archive_entry_for(0);
    DG547A.reopen = 0;
}

/*
 * 0x09b38
 *
 * Put a file at a given position, without asking DOS if it is already there.
 *
 * The record is the 0x1c-byte entry at DGROUP 0x548f selected by 0x5480 - the
 * same table `find_entry_for_pointer` walks, four bytes lower. Its +0x12 holds
 * the position DOS is believed to be at, as a 32-bit value, and a seek to that
 * same place does nothing at all.
 *
 * That cache is why the loader can afford to ask for a seek before every read:
 * measured over a run, 18,930 calls reach DOS 319 times. The archive is read
 * forward, so the believed position is nearly always right.
 *
 * The 319 that do reach DOS go through the runtime's own `fseek`, always from
 * the start of the file. That used to be a stand-in in io.c that did nothing,
 * with the caller verified only on occurrences where the buffer does not move;
 * it is the real routine now.
 */
void seek_file_to(uint32_t at)
{
    struct archive *a = &MACHINE_ARCHIVES.slot[DG547A.last_record];

    if (a->pos == at)
        return;

    borland_fseek(FILEREC_PTR(a->stream_ptr), (int32_t)at, 0);

    a->pos = at;
}

/*
 * 0x09b7c
 *
 * Find the archive entry standing in for an open file, or answer null if the
 * file is a real one.
 *
 * This is the pivot of the loader's two-way lookup: the game asks for each
 * resource as a loose file first and falls back to the packed archive, and this
 * is what tells the two apart afterwards. Everything above it - `fread`,
 * `fseek`, `ftell` - checks here before deciding whether to touch DOS.
 *
 * The table is ten entries of 0x12 bytes at DGROUP 0x55c3, keyed by the `FILE`
 * pointer itself, and there is a **one-entry cache** in front of it: 0x547a
 * holds the last pointer asked about and 0x547c the answer. A repeat question
 * is answered without walking anything, which matters because the read path
 * asks on every call.
 *
 * A null pointer clears the cache and answers null - that is how the cache is
 * invalidated when a file is closed.
 *
 * Two things end the walk with no match: running out of entries, and finding
 * one whose +0xe is zero. The second is checked **after** the loop rather than
 * inside it, so an entry matching the pointer but not yet open is found and
 * then rejected. Both paths also clear 0x547a, so the negative answer is not
 * cached - only positive ones are.
 */
struct game_file *archive_entry_for(FILE *file)
{
    struct game_file *si;
    int16_t n;

    if (file == 0) {
        DG547A.cache_key_ptr = 0;
        DG547A.cache_answer_ptr = 0;
        return GAME_FILE_NONE;
    }

    if (DG547A.archive_count == 0)
        return GAME_FILE_NONE;

    if (file == FILEREC_PTR(DG547A.cache_key_ptr))
        return GAME_FILE_PTR(DG547A.cache_answer_ptr);

    DG547A.cache_key_ptr = dg_near(dgroup, file);

    si = &MACHINE_GAME_FILES.files[0];
    n = 0xa;
    while (n != 0 && (FILE *)si != file) {
        si++;
        n--;
    }

    if (n == 0 || si->in_use == 0) {
        si = GAME_FILE_NONE;
        DG547A.cache_key_ptr = 0;
    }

    DG547A.cache_answer_ptr = dg_near(dgroup, si);
    return si;
}
