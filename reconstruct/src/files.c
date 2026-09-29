/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The plain screen loader, the file-record layer and the bitmap
 * compressor.**
 *
 * The last module of the original's **code segment 1c25**, image
 * 0x23b29..0x248fe - a paragraph past the segment's nominal end, where
 * segment 248f's first routine begins - split out of engine.c on
 * 2026-09-27. No call forces a boundary inside it, and its literal pool,
 * DGROUP 0x498e..0x49b9, is one run. Functions are in address order and
 * each carries the image offset it was read from.
 *
 * **Borland C++ 2.0, `-mm -G -O`**, like screenshot.c. Two-byte frames are
 * `dec sp / dec sp`, which BC++ 2.0 writes and TC++ 3.0 does not; `-G` is
 * the `inc sp / inc sp` after a one-word call and `-O` the missing jumps to
 * the epilogue. TC++ 1.01 does both, and assigns SI and DI the other way
 * round.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -G -O
 * JUDGE: data 0x498e..0x49b9
 */
#include <string.h>

#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * **This module's `MK_FP` is Turbo C 2.0's**: the segment widened to a long
 * and shifted, not Borland C++'s `_seg` addition. The widening is the `cwd`
 * the image has before every pointer it builds, and it is there because the
 * header's segment is a signed `int`. Ours, in the sense that the header it
 * came from is not known.
 */
#undef MK_FP
#define MK_FP(seg, ofs) ((void far *)(((uint32_t)(seg) << 16) | (uint16_t)(ofs)))
#endif

/*
 * The module's `_BSS`, 0x6292..0x63f6, is these three records. **Borland
 * C++ 2.0 orders `_BSS` by name, not by definition**: the order comes from
 * its symbol table, and moving the definitions changes nothing. So the third
 * is called `BITMAP_COMPRESS`, which that order puts after the other two as
 * the image has it; `ENGINE_BITMAP_COMPRESS` came out second. The names are
 * ours either way.
 */
/*
 * **The bitmap compressor's stream**, DGROUP 0x63e2..0x63f6, 0x14 bytes.
 */
struct engine_bitmap_compress {
    int16_t   pending_rows;       /* +0x00 [2]  counts rows, not pixels */
    /* **Pairs, and this record is why.** `compress_bitmap_list` measures how
       much it wrote as `out.seg - out_start.seg` paragraphs *plus*
       `out.off - out_start.off` bytes, subtracting the halves separately -
       which is a distance no single pointer can give. It also renormalises
       `out` by hand between bitmaps and steps its offset alone in between. */
    uint8_t far *out_start;     /* +0x02 [4]  where the output started, and
                                            does not move */
    uint16_t  block_paras;          /* +0x06 [2] */
    uint8_t far *src;           /* +0x08 [4]  the bitmap's pixels, read a byte at a time;
                                     only the offset steps */
    uint8_t far *out;           /* +0x0c [4]  where the next byte goes */
    uint8_t  *row_buffer;         /* +0x10 [2]  the row buffer compress_row works in, 0x7d0 bytes */
    int8_t    mode;               /* +0x12      0x243bf sets it, a byte; it chooses how the runs are written */
    uint8_t   pad_13;             /* +0x13 */
} PACKED;

struct engine_bitmap_compress BITMAP_COMPRESS;

/*
 * **The saved file record**, DGROUP 0x639e..0x63e2, 0x44 bytes. `seek_named_chunk` copies a
 * record here on the way in and `restore_file_record_from_saved` puts it back,
 * so a failed search leaves the file exactly as it found it.
 *
 * 0x44 bytes, which is the third independent measurement of a file record's
 * frame slot: `copy_file_record` moves 0x43, `load_bitmaps`' two buffers are
 * `[bp-0xa2]`..`[bp-0x5e]`..`[bp-0x1a]` - 0x44 apart either way - and this one
 * runs to `engine_bitmap_compress` exactly 0x44 on. The first of those three disagreed with
 * the other two for weeks, as `uint8_t saved_a[52]`, and only a sanitizer saw
 * it.
 */
struct engine_saved_file_record {
    struct open_file rec;         /* +0x00 [0x43]  what the struct copies move */
    uint8_t   pad_43;             /* +0x43 */
} PACKED;

struct engine_saved_file_record ENGINE_SAVED_FILE_RECORD;

/*
 * **The open files**, DGROUP 0x6292..0x639e, 0x10c bytes: four `struct
 * open_file` records. `find_file_record` searches them downwards from index 3,
 * and four records of 0x43 bytes run exactly to `ENGINE_SAVED_FILE_RECORD`.
 */
struct engine_open_files {
    struct open_file rec[4];      /* +0x00 [0x10c] */
} PACKED;

struct engine_open_files ENGINE_OPEN_FILES;

/*
 * 0x23b29
 *
 * Load a screen that is *not* in the quadtree form: read its pixels a band at a
 * time and blit each band to the page as it arrives, so a 320x200 picture never
 * needs a 64 KB buffer.
 *
 * "SCR:DIM:" gives the size if it is there and 320x200 is assumed if it is not.
 * "SCR:BIN:" is the planar body, and it is required. Then, on an adapter that
 * DGROUP 0x38af selects, a second body: "SCR:VGA:" - kind 5 - or "SCR:AMG:" -
 * kind 6, which is expanded from one bit per pixel to four as each band lands.
 *
 * The band buffer is `(w / 2) * 128` bytes if the near heap will give it, and
 * is halved until it will, down to one row. `size / (w / 2)` is how many rows
 * fit in whatever was got, and the last band is short - which is what the
 * `imul` after each band is recomputing.
 *
 * The Amiga body is read at a quarter of the size, because the expansion turns
 * each byte into four.
 *
 * Answers the kind, so the caller can tell which of the three shapes it got.
 */
uint16_t load_screen_plain(char *name)
{
    int16_t res;
    int16_t opened;
    int16_t kind;
    uint16_t bytes;
    uint16_t band;
    uint8_t far *buf;
    int16_t w = 0x140;
    int16_t h = 0xc8;
    uint16_t half;
    register int16_t si;
    register int16_t di;

    /*
     * `restore_write_mode`, not `vm_reset_attributes`. The two are both "put
     * the VGA back", which is how the wrong one got written here once, and
     * the mistake is invisible on screen; the verifier saw it at once.
     */
    restore_write_mode();
    kind = 0;

    /* A handle, or a name to open - and the name's own slot then holds the
       handle. */
    if (file_record_valid((FILE *)name) == 0) {
        opened = 1;
        name = (char *)open_file_record(name);
    } else {
        opened = 0;
    }

    if (seek_named_chunk((FILE *)name, "SCR:DIM:", 0) != -1L) {
        game_fread((uint8_t *)&w, 1, 2, (FILE *)name);
        game_fread((uint8_t *)&h, 1, 2, (FILE *)name);
    }

    if (seek_named_chunk((FILE *)name, "SCR:BIN:", 0) != -1L) {
        if ((res = open_resource(0, (FILE *)name, "r",
                                 file_record_size((FILE *)name))) >= 0) {
            bytes = (half = w >> 1) << 7;

            /* The heap's answer widened with DS, halving the request until
               it is granted or would be less than a row pair. */
            do
                buf = (uint8_t far *)NEAR_ZERO(malloc_far(bytes));
            while (FAR_OF_NEAR_NULL(buf) && (bytes >>= 1) >= half);

            if (!FAR_OF_NEAR_NULL(buf)) {
                di = 0;
                si = bytes / half;
                band = bytes;
                if (si > h)
                    si = h;

                while (di < h) {
                    read_resource(res, buf, band);
                    blit_rows_thunk(buf, 0, di, half << 1, si);

                    di += si;
                    if (di + si > h) {
                        si = h - di;
                        band = si * half;
                    }
                }

                kind = 1;

                if (VMDS.vga_chunks != 0) {
                    close_resource(res);

                    if (seek_named_chunk((FILE *)name, "SCR:VGA:", 0) != -1L)
                        kind = 5;
                    else if (seek_named_chunk((FILE *)name, "SCR:AMG:", 0) != -1L)
                        kind = 6;

                    if (kind >= 5
                        && (res = open_resource(0, (FILE *)name, "r",
                                                file_record_size((FILE *)name))) >= 0) {
                        di = 0;
                        si = bytes / half;
                        if (kind == 6)
                            bytes >>= 2;
                        band = bytes;
                        if (si > h)
                            si = h;

                        while (di < h) {
                            read_resource(res, buf, band);

                            if (kind == 6)
                                expand_1bpp_to_4bpp(buf, buf, band);

                            blit_rows_alt_thunk(buf, 0, di, half << 1, si);

                            di += si;
                            if (di + si > h) {
                                si = h - di;
                                band = si * half;
                                if (kind == 6)
                                    band >>= 2;
                            }
                        }
                    }
                }

                free_far((uint8_t *)buf);
            }

            close_resource(res);
        }
    }

    if (opened != 0)
        close_file_record((FILE *)name);

    return kind;
}

/*
 * 0x23df2
 *
 * Find the open-file record with a given handle, or NULL.
 *
 * The four records of `ENGINE_OPEN_FILES`, with the handle at each one's +0.
 *
 * The search runs **downwards from index 3**: `si` is loaded with 4 and the
 * loop jumps straight to its test, which decrements before comparing, so all
 * four records are looked at and the highest matching slot wins if two ever
 * held the same handle.
 */
struct open_file *near find_file_record(FILE *handle)
{
    int16_t i = 4;

    while (--i >= 0) {
        if (ENGINE_OPEN_FILES.rec[i].file == handle)
            return &ENGINE_OPEN_FILES.rec[i];
    }

    return NULL;
}

/*
 * 0x23e23
 *
 * Put a file record back to how it starts: all 0x43 bytes cleared **except**
 * the handle at +0 and the 32-bit value at +0x1b:+0x1d, which are saved into
 * locals across the clear and written back - and then the file rewound.
 *
 * Saving those two rather than clearing around them is what makes the routine
 * usable on a record that is being reused as well as one being made.
 */
void near reset_file_record(struct open_file *rec)
{
    register char *p = (char *)rec;
    register int16_t n = sizeof *rec;
    FILE *handle = rec->file;
    uint32_t keep = rec->bound[0];

    while (--n >= 0)
        *p++ = 0;

    rec->bound[0] = keep;
    game_rewind(rec->file = handle);
}

/*
 * 0x23e70
 *
 * Compare two strings for at most `n` characters, answering 1 if they agree and
 * 0 if they do not.
 *
 * The end test comes **before** the count test, so two strings that both end
 * agree however small `n` is - including zero. Running the count out with both
 * still going also answers 1, which is what makes this a prefix comparison
 * rather than a full one.
 */
int16_t near string_equal_upto(const char * a, const char * b, uint16_t n)
{
    while ((*a != 0 || *b != 0) && n-- != 0) {
        if (*a++ != *b++)
            return 0;
    }
    return 1;
}

/*
 * 0x23ea8
 *
 * Copy a file record out to the caller: 0x43 bytes from the record with the
 * given handle. Answers the destination, or 0 for a null destination, a null
 * handle, or a handle that names no record.
 */
struct open_file *copy_file_record(struct open_file *dst, FILE *handle)
{
    struct open_file *rec;

    if (handle == 0 || dst == NULL || (rec = find_file_record(handle)) == NULL)
        return NULL;

    *dst = *rec;
    return dst;
}

/*
 * 0x23ee4
 *
 * Copy a file record **in** from the caller: 0x43 bytes over the record whose
 * handle is the first word of what was handed in, and then the file seeked to
 * where the copy says it was.
 *
 * The counterpart of `copy_file_record`, and the pair is how a caller saves and
 * restores a position without the record's own fields moving under it.
 */
int16_t restore_file_record_from(const struct open_file *src)
{
    struct open_file *rec;

    if (src == NULL || src->file == 0
        || (rec = find_file_record(src->file)) == NULL)
        return 0;

    *rec = *src;
    game_fseek(rec->file, (int32_t)rec->pos, 0);
    return 1;
}

/*
 * 0x23f2c
 *
 * Open a file through the resource manager: take a free record, open the file,
 * measure it, and answer the handle. 0 if there is no free record or the file
 * is not there.
 *
 * The size is found by seeking to the end and asking where that is, and stored
 * at +0x1b:+0x1d **with bit 31 set** - `or dx,0x8000` - which is a mark of some
 * kind rather than part of the length; nothing here reads it back.
 *
 * `reset_file_record` then clears the rest of the record and rewinds the file,
 * which is why the seek to the end costs nothing.
 */
FILE *open_file_record(char *name)
{
    struct open_file *rec;

    if ((rec = find_file_record(0)) == NULL)
        return 0;

    if ((rec->file = (game_fopen(name, "rb"))) == 0)
        return 0;

    game_fseek(rec->file, 0L, 2);
    rec->bound[0] = game_ftell(rec->file) | 0x80000000L;

    reset_file_record(rec);
    return rec->file;
}

/*
 * 0x23f90
 *
 * Put a file record back to the copy saved at DGROUP 0x639e and seek the file
 * to where that copy says it was. Always answers -1.
 *
 * This is `seek_named_chunk`'s failure path: it takes the snapshot on the way
 * in and comes back here whenever the walk cannot go on, so a failed search
 * leaves the record exactly as it found it.
 */
int32_t near restore_file_record(struct open_file *rec)
{
    *rec = ENGINE_SAVED_FILE_RECORD.rec;
    game_fseek(rec->file, (int32_t)rec->pos, 0);
    return -1L;
}

/*
 * 0x23fc2
 *
 * Walk into a file's nested chunks along a path of four-character names, and
 * answer where the wanted one starts - as a far value in DX:AX - or -1.
 *
 * The path is a string of four-character names with no separators, so its
 * length must be a non-zero multiple of four; anything else is refused before
 * the file is touched.
 *
 * The record carries the whole walk: **the depth in bytes** at +0x37, a stack
 * of six chunk-end positions from +0x1b - indexed by that depth, which is why
 * it is a multiple of four - the current position at +0x3b, the current chunk's
 * size at +0x3f, and the path walked so far from +2. Six levels is the limit,
 * and a depth reaching 0x18 gives up.
 *
 * Bit 15 of a stored chunk end says the chunk is a container: with it set the
 * walk descends, reading a four-character name and a four-byte size and pushing
 * the new end; with it clear the chunk is data and is skipped over.
 *
 * The `+0x39` field remembers how many matches a previous call already went
 * past, so asking for a later index continues from there rather than starting
 * again - and asking for an earlier one resets the record and starts over.
 *
 * There is a dead test in the middle: `cmp word [si+0x3f],0` followed by `jb`,
 * which on an unsigned comparison against zero can never be taken. It is
 * transcribed as the nothing it does.
 */
int32_t seek_named_chunk(FILE *handle, const char * path,
                          int16_t index)
{
    int16_t keep;
    register struct open_file *rec;
    register int16_t len;

    if (handle == 0 || (rec = find_file_record(handle)) == NULL)
        return -1L;

    len = -1;
    while (path[++len] != 0)
        ;

    if (len == 0 || (len & 3) != 0)
        return -1L;

    ENGINE_SAVED_FILE_RECORD.rec = *rec;

    if (string_equal_upto(path, (const char *)rec->path, 0x19) != 0) {
        if (index == 0 && (uint32_t)game_ftell(rec->file) == rec->pos)
            goto at_position;

        if (index == -1) {
            game_fseek(rec->file, rec->pos, 0);
            goto at_position;
        }

        if (rec->matched != 0) {
            if (index != 0) {
                keep = index;
                if (rec->matched < index) {
                    index -= rec->matched;
                } else if (rec->matched > index) {
                    reset_file_record(rec);
                } else {
                    game_fseek(rec->file, rec->pos, 0);
                    goto at_position;
                }
            } else {
                keep = rec->matched + (index = 1);
            }
        } else {
            if ((keep = index) != 0)
                reset_file_record(rec);
            else
                index = 1;
        }
    } else {
        if (index > 0) {
            reset_file_record(rec);
            keep = index;
        } else {
            index = 1;
            keep = 0;
        }
    }

    /* Step over whatever chunk the record is sitting on. */
    if ((rec->bound[rec->depth >> 2] & 0x80000000L) == 0)
        rec->pos += rec->size;
    game_fseek(rec->file, rec->pos, 0);

    while (index-- != 0) {
        for (;;) {
            /* Has this chunk run out? */
            if ((rec->bound[rec->depth >> 2] & 0x7fffffffL) == rec->pos) {
                if (rec->depth == 0)
                    return restore_file_record(rec);
                rec->depth -= 4;
                continue;
            }

            /* A data chunk is skipped over. */
            if ((rec->bound[rec->depth >> 2] & 0x80000000L) == 0) {
                rec->pos += rec->size;
                game_fseek(rec->file, rec->pos, 0);
                continue;
            }

            /* A container is descended into. */
            if (game_fread(&rec->path[rec->depth], 1, 4,
                           rec->file) != 4)
                return restore_file_record(rec);

            if ((rec->depth += 4) >= 0x18)
                return restore_file_record(rec);

            rec->path[rec->depth] = 0;
            rec->pos += 8;

            if (game_fread((uint8_t *)&rec->size, 4, 1,
                           rec->file) != 1)
                return restore_file_record(rec);

            rec->bound[rec->depth >> 2] = rec->pos + rec->size;

            /* The container flag is taken off the size here rather than
               masked at every read. */
            rec->size &= 0x7fffffffL;

            if (rec->size < 0 || rec->size >= (rec->bound[0] & 0x7fffffffL))
                return restore_file_record(rec);

            if (rec->depth == len
                && string_equal_upto((const char *)rec->path, path, len) != 0)
                break;
        }
    }

    rec->matched = keep;

at_position:
    return rec->pos;
}

/*
 * 0x242af
 *
 * The size of an open file, as a far value in DX:AX, or -1 for a handle that
 * names nothing. It is the pair at the record's +0x3f:+0x41.
 *
 * A handle of zero is refused before the search, which is what makes zero mean
 * "no file" throughout this layer.
 */
int32_t file_record_size(FILE *handle)
{
    struct open_file *rec;

    if (handle == 0 || (rec = find_file_record(handle)) == NULL)
        return -1L;

    return rec->size;
}

/*
 * 0x242d9
 *
 * Close an open file: clear the record's handle at +0 and close the stream.
 * Answers 1, or 0 for a handle of zero or one that names no record.
 *
 * The record is released by zeroing its +0 alone - `find_file_record` reads
 * nothing else to decide a slot is free - so the rest of the 0x43 bytes are
 * left as they were until the slot is taken again.
 */
int16_t close_file_record(FILE *handle)
{
    register struct open_file *rec;

    if (handle == 0 || (rec = find_file_record(handle)) == NULL)
        return 0;

    rec->file = 0;
    game_fclose(handle);
    return 1;
}

/*
 * 0x24308
 *
 * Whether a handle names an open file: 1 or 0. `find_file_record` answers the
 * record and this throws it away, which is the whole routine.
 */
int16_t file_record_valid(FILE *handle)
{
    return (int16_t)(find_file_record(handle) != NULL);
}

/*
 * 0x24320
 *
 * Planar to chunky, one byte a pixel: the reverse of the driver's
 * `vm_chunky_to_planar`, done in ordinary memory rather than through the card.
 *
 * The four planes are not four pointers but one, `count` bytes apart - the
 * layout `vm_load_bitmap_list` leaves behind - so the routine builds the other
 * three by adding the stride three times and shares their segment. Then for
 * each bit from 0x80 down it gathers that bit out of all four and writes the
 * nibble as a whole byte, which is why the destination is eight times the size
 * of one plane.
 *
 * `count` is decremented when the mask wraps rather than once a pixel, so it
 * counts source bytes and the loop runs eight times for each.
 *
 * A **** routine: its first argument is at [bp+4], not [bp+6].
 */
void near planes_to_chunky(uint8_t far * dst, const uint8_t far * src,
                      uint16_t count)
{
    /* Four plane cursors into one block, `count` apart; the first is `src`
       itself. The four planes are one allocation and cannot straddle a
       segment, so offset arithmetic is pointer arithmetic. */
    const uint8_t far *p1;
    const uint8_t far *p2;
    const uint8_t far *p3;
    uint8_t mask = 0x80;
    register uint8_t v;

    p1 = src + count;
    p2 = p1 + count;
    p3 = p2 + count;

    while (count != 0) {
        v = 0;
        if ((*src & mask) != 0)
            v |= 1;
        if ((*p1 & mask) != 0)
            v |= 2;
        if ((*p2 & mask) != 0)
            v |= 4;
        if ((*p3 & mask) != 0)
            v |= 8;

        *dst++ = v;

        if (!(mask >>= 1)) {
            count--;
            mask = 0x80;
            src++;
            p1++;
            p2++;
            p3++;
        }
    }
}

/*
 * 0x243bf
 *
 * Compress a whole list of bitmaps **in place**, over the pixels they came
 * from, and shrink the block down to what the compressed form needed. Answers
 * that size in bytes.
 *
 * Two far pointers run through DGROUP: 0x63e4 is where the output started and
 * does not move, and 0x63ee is where the next byte goes. The second is
 * renormalised at the top of every bitmap - paragraphs carried into the
 * segment, the offset masked to four bits - and the normalised value is what
 * goes back into the header afterwards, so each bitmap's header ends up
 * pointing at its own compressed data. The 0xfffe written to header+4 replaces
 * the mask pointer that `vm_load_bitmap_list` put there; a compressed bitmap
 * carries its transparency in the stream instead.
 *
 * On the adapter that DGROUP 0x38af selects the pixels are already chunky and
 * are compressed where they lie. Anywhere else they are still four planes, so
 * each bitmap is turned chunky into a block from DOS first, compressed out of
 * that, and the block given back - which is why `planes_to_chunky` divides the
 * count by eight: it is told the size in *pixels* and one plane byte is eight
 * of them.
 *
 * The size is measured as the two pointers' difference in segments and bytes
 * and handed to INT 21h AH=4Ah, which is the only place the port has to grow a
 * DOS arena that can shrink a block.
 */
int32_t compress_bitmap_list(struct bitmap **list, uint8_t colours)
{
    uint8_t far *at;
    int16_t seg;
    uint16_t resize_seg;
    uint16_t pixels;
    uint8_t far *blk;
    register struct bitmap **si;
    register int16_t di;

    BITMAP_COMPRESS.mode = colours - 1;
    BITMAP_COMPRESS.row_buffer = malloc_far(0x7d0);

    si = list;

    /* The first bitmap's own pixels, which is where the output begins. */
    BITMAP_COMPRESS.out = BITMAP_COMPRESS.out_start =
        MK_FP((int16_t)list[0]->data_seg, list[0]->data_off);

    while (*si != 0) {
        /* Normalise, and remember where this bitmap's own data begins. The
           shift is *signed*, which is the original's `sar`. */
        seg = FP_SEG(BITMAP_COMPRESS.out);
        di = FP_OFF(BITMAP_COMPRESS.out);
        at = BITMAP_COMPRESS.out = MK_FP(seg + (di >> 4), di & 0x0f);

        if (!(int8_t)VMDS.vga_chunks) {
            pixels = (*si)->width * (*si)->height;
            blk = DOS_ALLOC_PTR(DOS_ALLOC(pixels, 0));

            pixels >>= 3;

            planes_to_chunky(blk,
                             MK_FP((int16_t)(*si)->data_seg, (*si)->data_off),
                             pixels);

            (*si)->data_seg = FP_SEG(blk);
            (*si)->data_off = FP_OFF(blk);

            compress_bitmap(*si);

            dos_free_far(blk);
        } else {
            compress_bitmap(*si);
        }

        (*si)->data_seg = FP_SEG(at);
        (*si)->data_off = FP_OFF(at);
        (*si)->mask_off = 0xfffe;

        si++;
    }

    seg = FP_SEG(BITMAP_COMPRESS.out) - FP_SEG(BITMAP_COMPRESS.out_start);
    di = FP_OFF(BITMAP_COMPRESS.out) - FP_OFF(BITMAP_COMPRESS.out_start);
    BITMAP_COMPRESS.block_paras = seg + ((di + 0x0f) >> 4);

    /* Shrink the block to what the compressed form needed: INT 21h AH=4Ah on
       the first bitmap's segment. */
    resize_seg = list[0]->data_seg;
#ifdef __TURBOC__
    _BX = BITMAP_COMPRESS.block_paras;
    _AX = resize_seg;
    _ES = _AX;
    _AH = 0x4a;
    geninterrupt(0x21);
#else
    io_dos_resize(resize_seg, BITMAP_COMPRESS.block_paras);
#endif

    free_far(BITMAP_COMPRESS.row_buffer);

    return (seg << 4) + di;
}

/*
 * 0x2451f
 *
 * Emit one value into the compressed bitmap being written at the far pointer in
 * DGROUP 0x63ee, flushing whatever run is pending at DGROUP 0x63e2 first.
 *
 * There are two shapes and they do not meet. With a run pending and a
 * **negative** value, the value is negated and written as its low six bits and
 * then bits 6 to 8, or a zero byte if those are empty, and the rest of the run
 * is padded with zeroes. With a run pending and a value that is not negative,
 * the whole run becomes zero bytes and the count is cleared. Only the second
 * falls through to the tail.
 *
 * The tail writes 0x7f for every 0x3f the value is over, and then the remainder
 * with 0x40 set - a run-length byte and a literal, which is what makes 0x7f the
 * longest run this format can say in one byte.
 *
 * A **** routine: its argument is at [bp+4].
 */
void near emit_packed_value(register int16_t value)
{
    if (BITMAP_COMPRESS.pending_rows != 0) {
        if (value < 0) {
            value = -value;
            *BITMAP_COMPRESS.out++ = (uint8_t)(value & 0x3f);

            value = (value & 0x1c0) >> 6;
            if (value != 0)
                *BITMAP_COMPRESS.out++ = (uint8_t)(value & 0x3f);

            while (--BITMAP_COMPRESS.pending_rows)
                *BITMAP_COMPRESS.out++ = 0;
            return;
        }

        while (BITMAP_COMPRESS.pending_rows-- != 0)
            *BITMAP_COMPRESS.out++ = 0;
        BITMAP_COMPRESS.pending_rows = 0;
    }

    while (value > 0x3f) {
        *BITMAP_COMPRESS.out++ = 0x7f;
        value -= 0x3f;
    }

    *BITMAP_COMPRESS.out++ = (uint8_t)(0x40 | value);
}

/*
 * 0x245b9
 *
 * Write a run of literal pixels into the compressed bitmap at DGROUP 0x63ee: a
 * marker byte of the count with 0xc0 set, and then the pixels themselves.
 *
 * How they are written depends on DGROUP 0x63f4, which `0x243bf` sets to one
 * less than the number of colours - 0x0f for sixteen. At sixteen colours two
 * pixels share a byte, high nibble first, so an odd count is rounded up and the
 * pad pixel is zeroed in the *source* buffer first, before the marker's count
 * is incremented. Otherwise a pixel is a byte and they go out unchanged.
 *
 * The count is a byte and is compared zero-extended, so a run is at most 255
 * pixels. A **** routine: its arguments are at [bp+4] and [bp+6].
 */
void near write_literal_run(register uint8_t count, uint8_t * buf)
{
    uint8_t v;
    register int16_t si;

    *BITMAP_COMPRESS.out++ = (uint8_t)(count | 0xc0);

    if ((count & 1) != 0) {
        buf[count] = 0;
        count++;
    }

    if (BITMAP_COMPRESS.mode == 0x0f) {
        for (si = 0; count > si; si += 2) {
            v = (uint8_t)((buf[si] << 4) | buf[si + 1]);
            *BITMAP_COMPRESS.out++ = v;
        }
    } else {
        for (si = 0; count > si; si++)
            *BITMAP_COMPRESS.out++ = buf[si];
    }
}

/*
 * 0x24639
 *
 * Compress one row of chunky pixels into the bitmap being written at DGROUP
 * 0x63ee. This is the other half of the format `emit_packed_value` and
 * `emit_literal_run` write bytes for, and between them the three tags are:
 *
 *   0x40 | n   a value, written by `emit_packed_value`
 *   0x80 | n   `n` copies of the byte that follows - a run
 *   0xc0 | n   `n` literal pixels, packed two to a byte at sixteen colours
 *
 * so a count never exceeds 0x3f and a longer run goes out as repeated 0xbf
 * pairs with 0x3f each.
 *
 * The decision is one threshold: DGROUP 0x49ba is the shortest run worth
 * encoding as one, and anything shorter is added to a literal buffer instead.
 * That buffer is flushed when it reaches 0x3f, when a run interrupts it, and
 * at the end of the row.
 *
 * A **** routine, and it walks its own `remaining` argument down - which
 * nothing can see, because the caller pops it.
 */
void near compress_row(uint8_t *src, int16_t remaining)
{
    uint8_t value;
    uint8_t run = 0;
    uint8_t literals = 0;
    uint8_t buf[0x100];
    register uint8_t *si;

    while (remaining > 0) {
        si = src;
        run = 1;
        value = *si++;
        while (*si++ == value)
            run++;

        if ((int16_t)run >= DG49BA.min_run) {
            if ((int16_t)run > remaining)
                run = (uint8_t)remaining;

            if (literals != 0) {
                write_literal_run(literals, buf);
                literals = 0;
            }

            remaining -= run;
            src += run;

            while (run > 0x3f) {
                run += 0xc1;                    /* less 0x3f */
                *BITMAP_COMPRESS.out++ = 0xbf;
                *BITMAP_COMPRESS.out++ = value;
            }

            if (run != 0) {
                *BITMAP_COMPRESS.out++ = (uint8_t)(0x80 | run);
                *BITMAP_COMPRESS.out++ = value;
            }
            run = 0;
        } else {
            remaining--;
            buf[literals] = value;
            literals++;
            src++;
        }

        if (literals == 0x3f) {
            write_literal_run(literals, buf);
            literals = 0;
        }
    }

    if (literals != 0)
        write_literal_run(literals, buf);
}

/*
 * 0x24757
 *
 * Compress one bitmap, row by row, into the stream at DGROUP 0x63ee. This is
 * what drives `compress_row`, `write_literal_run` and `emit_packed_value`; the
 * header is at [si], its pixels at [si+2] with the segment at [si], and its
 * width and height at [si+6] and [si+8].
 *
 * Two things happen before any pixel is written.
 *
 * It reserves **one byte** at the front of the stream and fills it in last:
 * the smallest non-zero pixel value in the whole bitmap. At sixteen colours on
 * an adapter that wants it, that means a first pass over every pixel to find
 * it, and every pixel written afterwards has it subtracted - so a bitmap that
 * uses colours 8 to 15 is stored as 0 to 7 and the byte says where it started.
 * Anywhere else the byte is 1 and the subtraction is a no-op.
 *
 * And DGROUP 0x63e2 counts *rows*, not pixels: it is incremented once a row and
 * flushed as zero bytes when a row turns out to have content. Together with
 * 0x63e6, which counts transparent pixels forward and then has the row width
 * subtracted from it, that is how a run of blank rows costs almost nothing.
 * The count going negative is not a fault - it is the signal
 * `emit_packed_value` reads to tell "so many transparent" from "so many rows".
 *
 * A **** routine.
 */
void near compress_bitmap(register struct bitmap *bmp)
{
    register int16_t di = 0;            /* pixels waiting in the row buffer */
    int16_t x;
    int16_t y;
    int16_t blanks = 0;                 /* and it does go negative */
    uint8_t least = 0xff;
    char v;
    char *at;
    uint8_t far *hdr;
    char rowbuf[0x140];

    BITMAP_COMPRESS.pending_rows = 0;
    BITMAP_COMPRESS.block_paras = 0;

    BITMAP_COMPRESS.src = MK_FP((int16_t)bmp->data_seg, bmp->data_off);

    if (BITMAP_COMPRESS.mode == 0x0f && VMDS.vga_chunks != 0) {
        for (y = 0; bmp->height > y; y++) {
            for (x = 0; bmp->width > x; x++) {
                v = *BITMAP_COMPRESS.src++;
                if (v != 0 && (uint8_t)v < least)
                    least = v;
            }
        }
    } else {
        least = 1;
    }

    BITMAP_COMPRESS.src = MK_FP((int16_t)bmp->data_seg, bmp->data_off);

    hdr = BITMAP_COMPRESS.out++;

    for (y = 0; bmp->height > y; y++) {
        at = rowbuf;
        far_memcpy((uint8_t far *)rowbuf, BITMAP_COMPRESS.src, bmp->width);
        BITMAP_COMPRESS.src += bmp->width;

        for (x = 0; bmp->width > x; x++) {
            v = *at++;
            if (!v) {
                if (di != 0) {
                    compress_row(BITMAP_COMPRESS.row_buffer, di);
                    di = 0;
                }
                blanks++;
            } else {
                v = (uint8_t)((v - least) & BITMAP_COMPRESS.mode);
                BITMAP_COMPRESS.row_buffer[di] = v;
                di++;

                if (blanks != 0) {
                    emit_packed_value(blanks);
                    blanks = 0;
                } else if (BITMAP_COMPRESS.pending_rows != 0) {
                    while (BITMAP_COMPRESS.pending_rows-- != 0)
                        *BITMAP_COMPRESS.out++ = 0;
                    BITMAP_COMPRESS.pending_rows = 0;
                }
            }
        }

        if (di != 0) {
            compress_row(BITMAP_COMPRESS.row_buffer, di);
            di = 0;
        }

        blanks -= bmp->width;
        BITMAP_COMPRESS.pending_rows++;
    }

    if (di != 0)
        compress_row(BITMAP_COMPRESS.row_buffer, di);

    emit_packed_value(0);

    *hdr = least;
}
