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
#include <string.h>
#ifndef __TURBOC__
#include "hostlib.h"
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **This module's `_BSS`, 0x547a..0x567e, in the order the four are
 * defined below reversed**: Borland lays `_BSS` out in reverse order of
 * first mention, so the highest, DG5677, is defined first and DG547A, the
 * lowest, last. None of them is declared anywhere else, which would be a
 * mention of its own (docs/lessons.md).
 */

/*
 * **The critical-error vector**, at DGROUP 0x5677, and what failed.
 */
struct dg_5677 {
    void interrupt (far *crit_vec)(); /* +0x00  DOS's 24h, kept so it can be
                                            put back */
    uint16_t  failures;           /* +0x04 **or-ed, not set**: this layer accumulates its failures here */
} PACKED;

struct dg_5677 DG5677 DGROUP_BSS(0x5677);

/*
 * **The ten game files**, DGROUP 0x55c3..0x5677, 0xb4 bytes.
 */
struct machine_game_files {
    struct game_file files[0xa];  /* +0x00 [0xb4] */
} PACKED;

struct machine_game_files MACHINE_GAME_FILES DGROUP_BSS(0x55c3);

/*
 * **The eleven archives**, DGROUP 0x548f..0x55c3, 0x134 bytes. Index 0 is never where a search
 * begins - `find_entry_for_pointer` starts at 1 when `last_record` is clear.
 */
struct machine_archives {
    struct archive slot[0xb];     /* +0x00 [0x134] */
} PACKED;

struct machine_archives MACHINE_ARCHIVES DGROUP_WAS(0x548f);

/*
 * **The archives' lookup**, at DGROUP 0x547a: its one-entry cache, the
 * archive count, the name hash and the files it read. segment 0000's.
 */
struct dg_547a {
    FILE     *cache_key;      /* +0x00  the one-entry cache in front of find_entry_for_pointer: */
    struct game_file *cache_answer; /* +0x02  the pointer last asked about, and the answer */
    int16_t   archive_count;      /* +0x04  how many archives, accumulated; zero means none is open */
    uint16_t  last_record;        /* +0x06  where the search starts, so record 0 is never returned */
    /* **A 32-bit hash, not a pointer.** `hash_filename` splits its
       `uint32_t acc` across these two and `find_entry_for_pointer`
       compares the pair against each entry's first four bytes; the
       `_off`/`_seg` names its readers used were a misreading of a key. */
    uint32_t  name_hash;          /* +0x08  what hash_filename leaves for
                                            find_entry_for_pointer */
    uint8_t   open_immediate;     /* +0x0c  clear means try the file by name and close it again */
    /* **Reopen the archive even if it is the one already current.** Every
       reader forces the switch when it is set, and the switch clears it; the
       only thing that sets it is the two-instruction routine at image 0x09804,
       which nothing the port reaches calls. */
    uint8_t   reopen;          /* +0x0d */
    uint8_t   retry;              /* +0x0e  the loop around the loose-file open, for removable media */
    /* **An archive is being opened**, raised around the two retry loops that
       call `fopen` - the ones the missing-disk prompt belongs to. */
    uint8_t   opening;         /* +0x0f */
    uint8_t   scanned;            /* +0x10  the archives have been counted once */
    FILE     *file_used;      /* +0x11  the FILE it actually read from */
    FILE     *file_asked;     /* +0x13  and the one it was asked about */
} PACKED;

/* Not placed: the port's `vm_init`, which the hybrid runs as the machine
   layer, opens files through the archive lookup, and its count and its
   lists have to be the same side's. */
struct dg_547a DG547A DGROUP_WAS(0x547a);

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

struct machine_hash_order MACHINE_HASH_ORDER DGROUP_AT(0x28d2) = { { 0x00, 0x01, 0x06, 0x07 } };

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
    register struct game_file *si;
    register FILE *di;
    int16_t left;                       /* [bp-2] */
    char hdr[14];                       /* [bp-0x10] */

    if (DG547A.reopen != 0)
        make_file_current(0);
    load_archive_map();
    DG5677.failures = 0;
    if (DG547A.archive_count == 0)
        return fopen(name, mode);

    DG547A.file_used = 0;
    DG547A.file_asked = 0;
    for (si = &MACHINE_GAME_FILES.files[0], left = 0xa;
         left != 0 && si->in_use != 0; si++, left--)
        ;
    if (left == 0)
        return NULL;

    hash_filename(name);
    DG547A.opening = 1;
    do {
        DG547A.retry = 0;
        di = fopen(name, mode);
        if (DG4E67.file_op_active != 0)
            return di;
        if (DG547A.retry != 0 && VMDS.pixel_shift != 0)
            answer_carry_on(DG547A.last_record != 0 ? DG547A.last_record : 1);
    } while (DG547A.retry != 0);
    DG547A.opening = 0;

    if (di != NULL) {
        si->archive = 0;
        si->base = si->size = si->pos = 0;
        si->in_use = 1;
        si->stream = di;
    } else {
        if (!find_entry_for_pointer(si))
            return NULL;
        make_file_current(si->archive);
        seek_file_to(si->base + si->pos);
        di = MACHINE_ARCHIVES.slot[DG547A.last_record].stream;
        fread((uint8_t *)hdr, 0xd, 1, di);
        fread((uint8_t *)&si->size, 4, 1, di);
        MACHINE_ARCHIVES.slot[DG547A.last_record].pos = si->base = ftell(di);
        if (stricmp(hdr, name))
            return NULL;
        si->pos = 0;
        si->stream = 0;
        si->in_use = 1;
    }
    DG547A.open_immediate++;
    /* An archive entry: the game's FILE is its `game_file` record, and
       `archive_entry_for` tells the two apart by looking for it in the
       table. */
    return (FILE *)si;
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
    register struct game_file *si;
    register int16_t di;

    di = 0;
    if (file == NULL)
        return -1;
    if (DG547A.archive_count == 0
        || (si = archive_entry_for(file)) == NULL)
        di = fclose(file);
    else {
        archive_entry_for(NULL);
        if (si->stream != 0)
            di = fclose(si->stream);
        si->in_use = 0;
        DG547A.open_immediate--;
    }
    DG5677.failures |= di == -1;
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
#ifdef __TURBOC__
    /* The image saves SI and nothing uses it: a register variable the
       original declared and never used. */
    register int16_t unused;
#endif
    register struct game_file *di;
    uint16_t n;                         /* [bp-2] */
    uint16_t bytes;                     /* [bp-4] */

    if (DG547A.archive_count == 0
        || (di = archive_entry_for(file)) == NULL)
        return fread(buf, size, count, file);
    if (di->stream != 0)
        return fread(buf, size, count, di->stream);

    for (bytes = size * count;
         bytes != 0 && (uint32_t)bytes > di->size - di->pos;
         count--, bytes -= size)
        ;
    make_file_current(di->archive);
    seek_file_to(di->base + di->pos);
    file = MACHINE_ARCHIVES.slot[di->archive].stream;
    n = fread(buf, size, count, file);
    bytes = n * size;
    di->pos += bytes;
    MACHINE_ARCHIVES.slot[di->archive].pos += bytes;
    return n;
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
int16_t game_fseek(FILE *file, int32_t off, register int16_t whence)
{
    register struct game_file *si;

    if (DG547A.archive_count == 0
        || (si = archive_entry_for(file)) == NULL)
        return fseek(file, off, whence);
    if (si->stream != 0)
        return fseek(si->stream, off, whence);

    /* Every comparison here is **unsigned** over the pair, which is the
       original's `cmp hi / ja / jb / cmp lo / ja` and not the signed shape
       `resource_seek` uses. */
    if (whence == 1)
        off += si->pos;
    else if (whence == 2) {
        if (si->size <= (uint32_t)off)
            off = 0;
        else
            off = si->size - off;
    }
    if (si->size < (uint32_t)off)
        off = si->size;
    si->pos = off;
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
int32_t game_ftell(register FILE *file)
{
    register struct game_file *si;

    if (DG547A.archive_count == 0
        || (si = archive_entry_for(file)) == NULL)
        return ftell(file);
    if (si->stream != 0)
        return ftell(si->stream);
    else
        return si->pos;
}

/*
 * 0x093e0
 *
 * `rewind` over the archive: `game_fseek` to nought from the start, and
 * nothing else. Five pushes and a call.
 */
void game_rewind(FILE *file)
{
    game_fseek(file, 0L, 0);
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
int16_t game_fgetc(register FILE *file)
{
    register struct game_file *si;
    int16_t got;                        /* [bp-2] */

    DG547A.file_asked = file;
    if (DG547A.archive_count == 0
        || (si = archive_entry_for(file)) == NULL)
        return fgetc((DG547A.file_used = file));
    if (si->stream != 0)
        return fgetc((DG547A.file_used = si->stream));
    if (si->pos >= si->size)
        return -1;
    make_file_current(si->archive);
    seek_file_to(si->base + si->pos);
    file = MACHINE_ARCHIVES.slot[si->archive].stream;
    got = fgetc((DG547A.file_used = file));
    si->pos++;
    MACHINE_ARCHIVES.slot[si->archive].pos++;
    return got;
}

/*
 * 0x094a8
 *
 * **Is a game file at its end?** A loose file answers as Borland's `feof`
 * does, the stream's end-of-file bit, 0x20 of its flags; so does an archive
 * member with a stream of its own. Any other archive member is at its end
 * when its position has reached its size.
 *
 * Nothing in the image calls it, near or far, so the descent map never
 * reached it and the port never had it. It is transcribed because the module
 * holds it. The name is ours.
 */
int16_t game_feof(register FILE *file)
{
    register struct game_file *si;

    if (DG547A.archive_count == 0
        || (si = archive_entry_for(file)) == NULL)
        return feof(file);
    if (si->stream != 0)
        return feof(si->stream);
    else
        return si->pos >= si->size;
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
    register struct game_file *si;
    register uint16_t di;
    const uint8_t *p;                   /* [bp-2] */

    p = ptr;
    if (DG547A.archive_count == 0
        || (si = archive_entry_for(file)) == NULL)
        di = fwrite(p, size, count, file);
    else if (si->stream != 0)
        di = fwrite(p, size, count, si->stream);
    else
        di = 0;
    DG5677.failures |= di != count;
    return di;
}

/*
 * 0x09571
 *
 * **`fputc`, through the archive layer** - `game_fwrite`'s shape for one
 * byte. With the archive in use at DGROUP 0x547e an entry writes through the
 * handle at its +0x10, and an entry without one writes nothing and answers
 * -1; a file that is not an entry, and everything when the archive is not in
 * use, goes to `fputc` directly. The answer is `fputc`'s, and the
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
    register struct game_file *si;
    register int16_t di;

    if (DG547A.archive_count == 0
        || (si = archive_entry_for(file)) == NULL)
        di = fputc(c, file);
    else if (si->stream != 0)
        di = fputc(c, si->stream);
    else
        di = -1;
    DG5677.failures |= di == -1;
    return di;
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
void game_setbuf(register FILE *file, uint8_t *buf)
{
    register struct game_file *si;

    if (DG547A.archive_count == 0
        || (si = archive_entry_for(file)) == NULL)
        setbuf(file, buf);
    else if (si->stream != 0)
        setbuf(si->stream, buf);
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
    register FILE *si;
    register int16_t di;
    struct archive *a;                  /* [bp-2] */
    struct archive_entry far *e;        /* [bp-6] */
    int16_t count;                      /* [bp-8] */
    uint32_t lo;                        /* [bp-0xc] the entry's key */
    uint32_t hi;                        /* [bp-0x10] and where its data starts */
    char *name;                         /* [bp-0x12] */

    if (DG547A.scanned == 0) {
        DG5677.crit_vec = getvect(0x24);
        setvect(0x24, (void interrupt (far *)())crit_error_handler);
        DG547A.scanned = 1;
        name = "RESOURCE.MAP";
        if ((si = fopen(name, "rb")) != NULL) {
            fread((uint8_t *)MACHINE_HASH_ORDER.hash_order, 4, 1, si);
            fread((uint8_t *)&count, 2, 1, si);
            DG547A.archive_count += count;
            for (di = DG547A.archive_count - count + 1; di <= DG547A.archive_count;
                 di++) {
                a = &MACHINE_ARCHIVES.slot[di];
                fread((uint8_t *)a->name, 0xd, 1, si);
                fread((uint8_t *)&count, 2, 1, si);
                /* **Zeroed**, which is what writes the terminator: the block is
                   one entry longer than the count and the lookup stops on an
                   all-zero key. */
                e = (struct archive_entry far *)
                    DOS_ALLOC_PTR(DOS_ALLOC((uint16_t)((count + 1) << 3), 1));
                a->list = (uint8_t far *)e;
                a->index = di;
                while (count--) {
                    fread((uint8_t *)&lo, 4, 1, si);
                    fread((uint8_t *)&hi, 4, 1, si);
                    e->key = lo;
                    e->base = hi;
                    e++;
                }
            }
            fclose(si);
        }
    }
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
    register int16_t si;

    for (si = 0; si <= 10; si++)
        if (MACHINE_ARCHIVES.slot[si].list != NULL) {
            dos_free_far(MACHINE_ARCHIVES.slot[si].list);
            MACHINE_ARCHIVES.slot[si].list = NULL;
        }
    if (DG5677.crit_vec) {
        setvect(0x24, DG5677.crit_vec);
        DG5677.crit_vec = 0;
    }
    DG547A.scanned = 0;
}

/*
 * 0x09803
 *
 * **Ask for the archives to be reopened**: set the byte at DGROUP 0x5487
 * that `game_fopen` tests before it opens anything, and nothing else.
 *
 * Nothing in the image calls it, near or far. It is transcribed because the
 * module holds it. The name is ours.
 */
void request_archive_reopen(void)
{
    DG547A.reopen = 1;
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
    register uint8_t *si;
    register int16_t di;
    int32_t acc;                        /* [bp-4] */
    uint16_t sum;                       /* [bp-6] */
    uint16_t eor;                       /* [bp-8] */
    char buf[14];                       /* [bp-0x16] */

    if (name == NULL)
        return DG547A.name_hash = 0;

    sum = eor = 0;
    for (si = (uint8_t *)name; *si != 0; si++) {
        if (*si >= 'a' && *si <= 'z')
            *si ^= 0x20;
        sum += *si;
        eor ^= *si;
        if (*si == '\\' || *si == ':') {
            sum = eor = 0;
            name = (char *)si + 1;
        }
    }
    strncpy(buf, name, 0xd);
    acc = 0;
    for (di = 0; di < 4; di++)
        acc = (int32_t)((uint32_t)acc << 8)
              + (uint8_t)buf[MACHINE_HASH_ORDER.hash_order[di]];
    acc += (int16_t)(sum * eor);
    return DG547A.name_hash = acc;
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
int16_t find_entry_for_pointer(register struct game_file *out)
{
    int16_t idx;
    int16_t fwd;
    const struct archive_entry far *at; /* [bp-4] */
    uint32_t want;                      /* [bp-8] */
    int16_t back;                       /* [bp-0xa] */

    /* Each walk below starts a list and steps over its eight-byte entries to
       the key it wants or to the zero key that ends it. The original writes
       it out three times, and so does this. */
    want = DG547A.name_hash;
    if ((idx = DG547A.last_record) == 0)
        idx = 1;
    for (at = (const struct archive_entry far *)MACHINE_ARCHIVES.slot[idx].list;
         at->key != 0 && at->key != want; at++)
        ;
    fwd = DG547A.last_record + 1;
    back = DG547A.last_record - 1;
    while (at->key != want && (back > 0 || fwd <= DG547A.archive_count)) {
        if (fwd <= DG547A.archive_count) {
            idx = fwd++;
            for (at = (const struct archive_entry far *)MACHINE_ARCHIVES.slot[idx].list;
                 at->key != 0 && at->key != want; at++)
                ;
        }
        if (at->key != want && back > 0) {
            idx = back--;
            for (at = (const struct archive_entry far *)MACHINE_ARCHIVES.slot[idx].list;
                 at->key != 0 && at->key != want; at++)
                ;
        }
    }
    if (at->key == want) {
        out->archive = idx;
        out->base = at->base;
        out->size = out->pos = 0;
        return 1;
    } else
        return 0;
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
void make_file_current(register uint16_t index)
{
    register struct archive *si;
    int16_t missing;                    /* [bp-2] */

    /* Set when the archive *cannot* be opened: `fclose` of a failed
       `fopen` is what answers non-zero. */
    missing = 0;
    if (!DG547A.open_immediate && index != 0
        && fclose(fopen((const char *)MACHINE_ARCHIVES.slot[index].name,
                                        "rb")))
        missing = 1;

    if (index != DG547A.last_record || missing != 0 || DG547A.reopen != 0) {
        si = &MACHINE_ARCHIVES.slot[DG547A.last_record];
        if (si->stream != 0) {
            fclose(si->stream);
            si->stream = 0;
        }
        DG547A.last_record = index;
        si = &MACHINE_ARCHIVES.slot[DG547A.last_record];
        if (index != 0) {
            DG547A.opening = 1;
            while ((si->stream = fopen((const char *)si->name, "rb")) == 0)
                if (VMDS.pixel_shift != 0)
                    answer_carry_on(index);
            DG547A.opening = 0;
        }
        si->pos = 0;
        archive_entry_for(NULL);
        DG547A.reopen = 0;
    }
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
    register struct archive *si;

    si = &MACHINE_ARCHIVES.slot[DG547A.last_record];
    if (si->pos != at) {
        fseek(si->stream, at, 0);
        si->pos = at;
    }
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
    register struct game_file *si;
    int16_t n;

    if (file == NULL) {
        DG547A.cache_key = 0;
        DG547A.cache_answer = 0;
        return NULL;
    }
    if (DG547A.archive_count == 0)
        return NULL;
    if (file == DG547A.cache_key)
        return DG547A.cache_answer;
    DG547A.cache_key = file;
    for (si = &MACHINE_GAME_FILES.files[0], n = 0xa; n != 0 && (FILE *)si != file;
         si++, n--)
        ;
    if (n == 0 || si->in_use == 0) {
        si = NULL;
        DG547A.cache_key = 0;
    }
    return (DG547A.cache_answer = si);
}

/*
 * 0x09bdf
 *
 * **DOS's critical-error handler while the game's files are open**, the
 * vector `load_archive_map` installs at 24h. An `interrupt` function: DOS's
 * answer is the AX it pops, so it is the `ax` below that is set. With a file
 * operation of the game's own in progress, and whenever an archive is being
 * opened, the answer is 3, fail the call; otherwise 1, retry. Either way it
 * notes that the call went wrong, in the two bytes `game_fopen` and
 * `make_file_current` look at.
 *
 * The host has no DOS to raise it, so there it is never called; under the
 * hybrid the original's own runs.
 */
void interrupt crit_error_handler(uint16_t bp, uint16_t di, uint16_t si,
                                  uint16_t ds, uint16_t es, uint16_t dx,
                                  uint16_t cx, uint16_t bx, uint16_t ax)
{
    if (DG4E67.file_op_active != 0)
        ax = 3;
    else
        ax = DG547A.opening ? 3 : 1;
    DG547A.retry = 1;
    DG547A.reopen = 1;
#ifndef __TURBOC__
    (void)ax;   /* ours: on the host the answer goes nowhere */
#endif
}

/*
 * 0x09c23
 *
 * **A rectangle drawn by XOR**, pixel by pixel through the driver's clipped
 * read and write: the outline in the second colour, and the inside in the
 * fill colour when filling is on - so drawing it twice takes it away again.
 * Drawn from the destination page into itself, the source page being set to
 * it for the while and put back after.
 *
 * With clipping on, each edge is drawn only if it is inside the clip box, and
 * the rectangle is cut down to the box. **A cut-down left or top edge is set
 * to the clip-enabled byte, 1, not to the clip box's edge**: that is what the
 * image does, and it is transcribed so.
 *
 * Nothing in the image calls it, near or far. It is transcribed because the
 * module holds it. The name is ours.
 */
void draw_xor_rect(register int16_t x, int16_t y, int16_t w, int16_t h)
{
    register int16_t si;
    int16_t yy;                         /* [bp-2] */
    int16_t outline;                    /* [bp-4] */
    int16_t fill;                       /* [bp-6] */
    int16_t end;                        /* [bp-8] */
    uint16_t saved;                     /* [bp-0xa] */
    int16_t left_in;                    /* [bp-0xc] */
    int16_t right_in;                   /* [bp-0xe] */
    int16_t top_in;                     /* [bp-0x10] */
    int16_t bottom_in;                  /* [bp-0x12] */
    int16_t x_end;                      /* [bp-0x14] */
    int16_t y_end;                      /* [bp-0x16] */

    if (!VMDS.second_colour && !VMDS.fill_colour)
        return;
    saved = VMDS.page_src;
    VMDS.page_src = VMDS.page_dst;
    outline = VMDS.second_colour;
    fill = VMDS.fill_colour;
    if (VMDS.clip_enabled) {
        end = x + w - 1;
        left_in = x >= VMDS.clip_left && x <= VMDS.clip_right;
        right_in = end >= VMDS.clip_left && end <= VMDS.clip_right;
        if (x < VMDS.clip_left) {
            w -= VMDS.clip_left - x;
            x = VMDS.clip_enabled;
        }
        if (x > VMDS.clip_right) {
            w -= x - VMDS.clip_right;
            x = VMDS.clip_right;
        }
        end = y + h - 1;
        top_in = y >= VMDS.clip_top && y <= VMDS.clip_bottom;
        bottom_in = end >= VMDS.clip_top && end <= VMDS.clip_bottom;
        if (y < VMDS.clip_top) {
            h -= VMDS.clip_top - y;
            y = VMDS.clip_enabled;
        }
        if (y > VMDS.clip_bottom) {
            h -= y - VMDS.clip_bottom;
            y = VMDS.clip_bottom;
        }
    } else
        left_in = right_in = top_in = bottom_in = 1;

    x_end = x + w - 1;
    y_end = y + h - 1;
    if (VMDS.fill_enabled)
        for (yy = y + 1; yy < y_end; yy++)
            for (si = x + 1; si < x_end; si++)
                plot_pixel_clipped(si, yy, read_pixel_clipped(si, yy) ^ fill);
    for (si = y; si <= y_end; si++) {
        if (left_in)
            plot_pixel_clipped(x, si, read_pixel_clipped(x, si) ^ outline);
        if (right_in)
            plot_pixel_clipped(x_end, si, read_pixel_clipped(x_end, si) ^ outline);
    }
    for (si = x + 1; si < x_end; si++) {
        if (top_in)
            plot_pixel_clipped(si, y, read_pixel_clipped(si, y) ^ outline);
        if (bottom_in)
            plot_pixel_clipped(si, y_end, read_pixel_clipped(si, y_end) ^ outline);
    }
    VMDS.page_src = saved;
}
