/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Bitmap and screen files**: the formats a bitmap can be held in and the
 * code that loads each of them - segment 248f's third module, image
 * 0x24f72..0x25953. Functions are in address order and each carries the
 * image offset it was read from.

 * **Segment 248f holds four modules, and this is one.** In the medium model
 * each file's code goes into a segment named for it unless it is told
 * otherwise, and these four were told the same name: three C files and the
 * assembly module in vqt.c, in that order. The boundaries are the image's
 * own evidence. Every far call from one routine of the segment to another is
 * `nop / push cs / call` - TLINK's rewrite of a `9A` - where Borland C++
 * calls a routine *defined earlier in the same file* with a bare
 * `push cs / call`, as it does 411 times elsewhere in the image. So
 * `draw_offset_bitmap` is not in `open_bit_reader`'s file, and `draw_bitmap`
 * is not in `draw_offset_bitmap`'s. The data agrees: each file's `_DATA` is
 * its own run of DGROUP.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -G -Z
 * JUDGE: data 0x49c6..0x4a07
 *
 * **Its data** is its string literals, DGROUP 0x49c6..0x4a07, in the order
 * the code names them, and one word nothing names.
 *
 * What it shares with the rest of the program - a bitmap header and its list,
 * a file record, a block from DOS - is still read through the port's macros,
 * which are casts under TCC, until those records convert.
 */
/* This module declares `blit_scaled_a` its own way - near; see
   `draw_bitmap_scaled_248f`. */
#define TIM_BITMAPS_C
#include "tim.h"
#include "io.h"
#include "dgroup.h"

#ifndef __TURBOC__
/* `decode_vqt_list` counts bitmaps into a word it never reads: the
   original's, and a warning only on the host. */
#  pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#ifdef __TURBOC__
void near blit_scaled_a(struct bitmap *bmp, int16_t x, int16_t y,
                        int16_t a, int16_t b, int16_t c);
#  define blit_scaled_a_near blit_scaled_a
#endif

/*
 * 0x24f72
 *
 * Load a bitmap file, whichever of four shapes it is in, and answer the list.
 * The start-up calls it for "cp.bmp" and "gp_bord.bmp"; it is the door that
 * `load_bitmap_list` is only one road out of.
 *
 * The chunk names decide, in this order:
 *
 *   "BMP:SCN:"  a screen: the headers are read and every bitmap's field 4 set
 *               to 0xfffe, and the pixels are already where they belong.
 *   "BMP:OFF:"  a table of 32-bit offsets, one a bitmap, into one block read
 *               whole; each header is pointed at its own offset within it. The
 *               field-4 marker here is 0xffff.
 *   "BMP:VQT:"  the quadtree form, decoded by `decode_vqt_list` into a block
 *               the driver sized; marker 0xfffc.
 *   none of them - `load_bitmap_list`, the planar form.
 *
 * And then two more that modify whatever was loaded: "BMP:RLE:" compresses the
 * lot in place at sixteen colours, and "BMP:SCL:" sets field 4 to 0xfffd.
 *
 * The file record is copied aside and put back around the header read, because
 * `read_bmp_info` moves the position and the chunk search afterwards has to
 * start where it did.
 *
 * `name` is either an open file record or a filename: `file_record_valid`
 * says which, by comparing it with the four records. Any failure frees the
 * list and answers 0; the record is closed only if this routine opened it.
 *
 * **The offsets are added to a `huge` pointer**, so TCC normalises each sum
 * through the runtime (`F_PADD@`, `F_PADA@`) and the header is filed the
 * normalised pair - segment first, as a header keeps it.
 */
struct bmp_set *load_bitmaps(char *name)
{
    bmp_ptr_t *list;
    int16_t count;
    int16_t i;
    int16_t opened;
    uint8_t huge *block;
    uint8_t huge *p;
    int32_t offset;
    int32_t size;
    int16_t kind;
    uint8_t saved_a[0x44];
    uint8_t saved_b[0x44];
    FILE *file;
    struct bitmap *hdr;

    file = (FILE *)name;
    list = 0;
    block = FAR_NULL_PTR;
    opened = 0;
    if (file_record_valid(file) == 0) {
        opened = 1;
        if ((file = open_file_record((char *)file)) == 0)
            goto fail;
    }
    copy_file_record(saved_a, file);
    if (seek_named_chunk(file, "BMP:SCN:", 0) != -1L) {
        copy_file_record(saved_b, file);
        restore_file_record_from(saved_a);
        if (read_bmp_info(file, (uint16_t *)&count, &list) == 0)
            goto fail;
        set_field_4_of_each(0xfffe, list);
        restore_file_record_from(saved_b);
        kind = 0;
    } else {
        if (seek_named_chunk(file, "BMP:OFF:", 0) == -1L)
            goto planar;
        game_fread((uint8_t *)&kind, 2, 1, file);
        restore_file_record_from(saved_a);
        if (read_bmp_info(file, (uint16_t *)&count, &list) == 0)
            goto fail;
        set_field_4_of_each(0xffff, list);
        if (seek_named_chunk(file, "BMP:VQT:", 0) == -1L)
            goto fail;
    }
    if (kind == 0) {
        size = file_record_size(file);
        if (FAR_IS_NULL(block = DOS_ALLOC_PTR(DOS_ALLOC(size, 0))))
            goto fail;
        read_far(block, size, file);
        if (seek_named_chunk(file, "BMP:OFF:", 0) == -1L) {
            dos_free_far(block);
            goto fail;
        }
        for (i = 0; i < count; i++) {
            if (game_fread((uint8_t *)&offset, 4, 1, file) != 1) {
                dos_free_far(block);
                goto fail;
            }
            p = block + offset;
            hdr = BMP_PTR(list[i]);
            hdr->data.seg = FP_SEG(p);
            hdr->data.off = FP_OFF(p);
        }
    } else {
        size = VM_VECTOR(13, vm_list_size_fn)(list, (uint8_t *)&i);
        if (FAR_IS_NULL(block = DOS_ALLOC_PTR(DOS_ALLOC(size, 0))))
            goto fail;
        set_field_4_of_each(0xfffc, list);
        for (i = 0; i < count; i++) {
            hdr = BMP_PTR(list[i]);
            hdr->data.seg = FP_SEG(block);
            hdr->data.off = FP_OFF(block);
            block += (uint16_t)(hdr->width * hdr->height);
        }
        decode_vqt_list(file, list);
    }
    goto loaded;
planar:
    list = load_bitmap_list((char *)file)->bmp_ptr;
loaded:
    count = count_list(list);
    if (seek_named_chunk(file, "BMP:RLE:", 0) != -1L)
        compress_bitmap_list(list, 0x10);
    if (seek_named_chunk(file, "BMP:SCL:", 0) != -1L)
        set_field_4_of_each(0xfffd, list);
    goto out;
fail:
    free_bitmaps_thunk(list);
    list = 0;
out:
    if (opened != 0)
        close_file_record(file);
    return (struct bmp_set *)list;
}

/*
 * 0x252b4
 *
 * Walk a bitmap list and write the same word into every header's `mask_off`,
 * which is the +4 the original writes and the field a loader leaves its
 * marker in.
 *
 * **The word is held in DX**, which no C variable is ever given - Borland's
 * pseudo-register `_DX`. The host keeps it in a local.
 */
void near set_field_4_of_each(uint16_t value, bmp_ptr_t *list)
{
    bmp_ptr_t *p;

#ifdef __TURBOC__
    _DX = value;
    for (p = list; *p != 0; p++)
        BMP_PTR(*p)->mask_off = _DX;
#else
    for (p = list; *p != 0; p++)
        BMP_PTR(*p)->mask_off = value;
#endif
}

/*
 * 0x252d0
 *
 * A thunk from this module into `free_bitmaps` in segment 1c25, which is a
 * `push`, an `lcall` and nothing else. It exists because the two are different
 * translation units and the call has to be far.
 */
void free_bitmaps_thunk(bmp_ptr_t *list)
{
    free_bitmaps(list);
}

/*
 * 0x252e0
 *
 * Count the entries in a null-terminated array of words. A null array answers
 * 0 without looking at it, which is what the test before the loop is for.
 *
 * **In CX and DX**, Borland's pseudo-registers: the list in CX and the count
 * in DX, and no frame beyond BP. The host keeps them in locals.
 */
uint16_t count_list(bmp_ptr_t *list)
{
#ifdef __TURBOC__
    _CX = (uint16_t)list;
    _DX = 0;
    if (_CX != 0)
        while (((bmp_ptr_t *)_CX)[_DX] != 0)
            _DX++;
    return _DX;
#else
    uint16_t n = 0;

    if (list != NULL && list != BMPLIST(0))
        while (list[n] != 0)
            n++;
    return n;
#endif
}

/*
 * 0x25300
 *
 * Draw one bitmap, choosing how by the marker its loader left in `mask_off`.
 *
 * The header's far pointer is normalised first - paragraphs out of the offset
 * and into the segment - and *written back*, so a bitmap drawn twice is
 * normalised once. Then:
 *
 *   0xffff  the offset-table form, by `draw_offset_bitmap`
 *   0xfffe  compressed, by `draw_compressed_bitmap` - the form
 *           `compress_bitmap_list` writes
 *   0xfffd  scaled, through the driver's scaled blit, with three arguments
 *           rather than four
 *   other   plain planar, through the driver's structured blit - and "other"
 *           is not a fall-through for the unexpected, it is the ordinary
 *           case: an uncompressed bitmap's `mask_off` holds the offset of its
 *           mask, which is a small number and not a marker at all.
 *
 * The cases are in that order in the source: the compare chain is sorted,
 * the bodies are not.
 */
void draw_bitmap(struct bitmap *bmp, int16_t x, int16_t y, uint16_t mode)
{
    bmp->data.seg = bmp->data.seg + (bmp->data.off >> 4);
    bmp->data.off &= 0xf;
    switch (bmp->mask_off) {
    case 0xffff:
        draw_offset_bitmap(bmp, x, y, mode);
        return;
    case 0xfffe:
        draw_compressed_bitmap(bmp, x, y, mode);
        return;
    case 0xfffd:
        blit_scaled_thunk(bmp, x, y);
        return;
    default:
        blit_bitmap_thunk(bmp, x, y, mode);
        return;
    }
}

/*
 * 0x2537d
 *
 * **`draw_bitmap_scaled`'s twin, and nothing calls it.** The same
 * normalisation and the same choice as the one at 0x0b9c9 - nothing for the
 * scaled and offset-table forms, `blit_scaled_a` for the compressed one and
 * `blit_scaled_b` for the rest - but the three trailing arguments go on in
 * the order they came, where 0x0b9c9 moves `mode` ahead of the size: it was
 * written against another declaration of the two blitters.
 *
 * And that declaration made `blit_scaled_a` **near**. The call is `call`
 * with the offset `blit_scaled_a` has in its own segment, 0x655c, which TLINK
 * resolved inside this one - linear 0x2ae4c, the middle of segment 2a04's
 * cosine table. It never ran: no instruction and no table reaches 0x2537d,
 * so the port answers that case by stopping rather than by guessing. The
 * name is ours.
 */
void draw_bitmap_scaled_248f(struct bitmap *bmp, int16_t x, int16_t y,
                             int16_t a, int16_t b, int16_t c)
{
    bmp->data.seg = bmp->data.seg + (bmp->data.off >> 4);
    bmp->data.off &= 0xf;
    switch (bmp->mask_off) {
    case 0xffff:
        break;
    case 0xfffe:
#ifdef __TURBOC__
        blit_scaled_a_near(bmp, x, y, a, b, c);
#else
        port_abort("draw_bitmap_scaled_248f: its near call reaches 248f:655c, "
                   "inside segment 2a04's cosine table");
#endif
        break;
    case 0xfffd:
        break;
    default:
        blit_scaled_b(bmp, x, y, (uint16_t)a, b, c);
        break;
    }
}

/*
 * 0x253e7
 *
 * Load a screen - a whole 320x200 image rather than a sprite - and paint it.
 *
 * A "SCR:VQT:" chunk means the quadtree form: the file is read whole into a
 * block from DOS, the bit reader is opened on it, the cursor is pinned so it
 * does not smear as the picture arrives, and `vqt_screen_node` paints the lot
 * as one node covering 0x140 by 0xc8. Then the cursor is released and the
 * reader closed.
 *
 * Without that chunk it falls back to `load_screen_plain` and the file record's
 * position is put back first, which is why the record was copied aside before
 * the chunk search.
 *
 * Answers -1 on any failure, and the block is freed on every path - the
 * picture is in video memory by then, not in it. The block is `huge`, which
 * is why the test that frees it is the runtime's `F_PCMP@`.
 */
uint16_t load_screen(char *name)
{
    int16_t opened;
    uint8_t huge *block;
    int32_t size;
    uint8_t saved[0x44];
    FILE *file;
    uint16_t result;

    file = (FILE *)name;
    block = FAR_NULL_PTR;
    opened = 0;
    result = 0;
#ifdef __TURBOC__
    /* **Two bytes of this module's literal pool that no instruction
       names**, DGROUP 0x49fc: a "\0" between `load_bitmaps`' literals and
       this routine's. Borland C++ pools a module's literals after its
       variables, in the order the source names them, and keeps a literal
       whose code the optimiser drops. What the dropped code was cannot be
       told from the bytes; this is the least it can have been. */
    if (0)
        (void)"\0";
#endif
    if (file_record_valid(file) == 0) {
        opened = 1;
        if ((file = open_file_record((char *)file)) == 0)
            goto fail;
    }
    copy_file_record(saved, file);
    if (seek_named_chunk(file, "SCR:VQT:", 0) != -1L) {
        size = file_record_size(file);
        if (FAR_IS_NULL(block = DOS_ALLOC_PTR(DOS_ALLOC(size, 0))))
            goto fail;
        read_far(block, size, file);
        if ((BITMAPS.walk = open_bit_reader(block)) != 0) {
            cursor_redraw_off();
            vqt_screen_node(0, 0, 0x140, 0xc8);
            cursor_redraw_on();
            close_bit_reader();
        } else
            goto fail;
    } else {
        restore_file_record_from(saved);
        result = load_screen_plain((char *)file);
        goto close;
fail:
        result = 0xffff;
    }
close:
    if (opened != 0)
        close_file_record(file);
    if (block != FAR_NULL_PTR)
        dos_free_far(block);
    return result;
}

/*
 * 0x2551a
 *
 * Read a 32-bit count of bytes from a file into a far destination, through a
 * bounce buffer, because the read below it takes a near buffer and a 16-bit
 * count.
 *
 * The buffer is as big as the near heap will give it: it asks for 0x4000 and
 * halves down to 0x800, then steps down by 0x100 at a time, and if even that
 * fails it falls back to 0x100 bytes of its own stack. So a machine with a full
 * heap reads in small pieces rather than failing.
 *
 * The destination is renormalised every 0x10000/buffer reads - which is what
 * the divide at the top is for - by adding a whole segment to the pointer and
 * starting the offset again, so a destination longer than 64 KB is written
 * without the offset ever wrapping. Only the count's high word decides
 * whether that is needed: `count & 0xffff0000`.
 *
 * A short read ends it, whatever the count still says.
 */
void near read_far(uint8_t huge *dst, int32_t count, FILE *file)
{
    uint8_t far *walk;
    uint8_t *buf;
    int16_t per_segment;
    uint16_t left;
    uint8_t fallback[0x100];
    int16_t size;
    int16_t got;

    size = 0x4000;
    while (size != 0 && (buf = heap_malloc_far(size)) == 0) {
        if (size > 0x800)
            size >>= 1;
        else
            size -= 0x100;
    }
    if (size == 0) {
        buf = fallback;
        size = 0x100;
    }
    if ((count & 0xffff0000L) != 0)
        per_segment = (int16_t)(0x10000L / size);
    else
        per_segment = 0;
    left = per_segment;
    walk = dst;
    while (count != 0) {
        got = size > count ? (int16_t)count : size;
        got = game_fread(buf, 1, got, file);
        if (got == 0)
            break;
        far_copy(walk, buf, got);
        walk += got;
        count -= got;
        if (per_segment != 0 && --left == 0) {
            dst += 0x10000L;
            left = per_segment;
            walk = dst;
        }
    }
    if (buf != 0 && fallback != buf)
        heap_free_far(buf);
}

/*
 * 0x25639
 *
 * Read a "BMP:VQT:" chunk and decode every bitmap in the list out of it.
 *
 * It buys the biggest buffer it can and then reads the file through it as a
 * sliding window, which is the whole shape of the routine:
 *
 *   - one pass over the list to find the largest single bitmap, because the
 *     buffer has to hold at least that much;
 *   - if the whole file fits in free memory, take the file's size instead and
 *     forget the largest, since nothing will ever have to slide;
 *   - failing that, fall back to the scratch block at DGROUP 0x3576, which is
 *     0x3ab4 bytes and is refused if the largest bitmap will not fit in it.
 *
 * Then, for each bitmap: the reader in this routine's frame is filled in with
 * where the four planes go - each one a quarter of the pixels apart - and
 * where each row starts, and `vqt_node` walks the quadtree that paints it.
 * Afterwards the bits consumed are rounded up to whole bytes, the unread tail
 * of the buffer is slid down to the front, and as much as will fit is read in
 * behind it.
 *
 * **The reader outlives the call**: its address is filed at `BITMAPS.walk`,
 * a word nothing clears, and `load_screen` later puts the *other* reader
 * there - the singleton, which has no plane or row table. The format is
 * never exercised: `VQT` does not occur once in the four shipped
 * `RESOURCE.00*` archives.
 */
void near decode_vqt_list(FILE *file, bmp_ptr_t *list)
{
    bmp_ptr_t *at;
    uint8_t far *p;
    uint8_t huge *cur;
    uint8_t far *block;
    int16_t row;
    int16_t index;
    int32_t file_left;
    int32_t buffer;
    int32_t chunk;
    uint16_t largest;
    uint16_t n;
    struct vqt_reader reader;
    struct bitmap *hdr;
    int16_t i;

    index = 0;
    at = list;
    largest = 0;
    while (*at != 0) {
        chunk = buffer_size_thunk(BMP_PTR(*at)->width, BMP_PTR(*at)->height);
        if (largest < chunk)
            largest = (uint16_t)chunk;
        at++;
    }
    buffer = DOS_ALLOC_BYTES(DOS_ALLOC(-1L, 0));
    file_left = file_record_size(file);
    if (file_left <= buffer) {
        buffer = file_left;
        largest = 0;
    }
    if (largest > buffer
        || FAR_IS_NULL(block = DOS_ALLOC_PTR(DOS_ALLOC(buffer, 0)))) {
        if (DG3576.scratch != FAR_NULL_PTR && largest <= 0x3ab4) {
            block = DG3576.scratch;
            buffer = 0x3ab4;
        } else
            return;
    }
    BITMAPS.walk = &reader;
    BITMAPS.walk->pos = 0;
    BITMAPS.walk->data = block;
    read_far(block, buffer, file);
    file_left -= buffer;
    at = list;
    while ((hdr = BMP_PTR(*at)) != BMP_NONE) {
        row = hdr->data.seg + (hdr->data.off >> 4);
        p = FAR_OF_LONG(row, hdr->data.off & 0xf);
        n = (hdr->width * hdr->height) >> 2;
        for (i = 0; i < 4; i++) {
            BITMAPS.walk->plane[i] = p;
            p += n;
        }
        for (row = i = 0; hdr->height > i; i++) {
            BITMAPS.walk->row[i] = row;
            row += hdr->width;
        }
        vqt_node(0, 0, hdr->width, hdr->height);
        n = (uint16_t)((BITMAPS.walk->pos + 7) >> 3);
        BITMAPS.walk->pos = 0;
        cur = BITMAPS.walk->data;
        if (file_left != 0) {
            p = cur + n;
            far_copy(cur, p, (uint16_t)buffer - n);
            cur += buffer - n;
            chunk = n < file_left ? n : file_left;
            if (chunk > buffer)
                chunk = buffer;
            read_far(cur, chunk, file);
            file_left -= chunk;
        } else
            BITMAPS.walk->data = cur + n;
        at++;
        index++;
    }
    if (block != DG3576.scratch)
        dos_free_far(block);
}
