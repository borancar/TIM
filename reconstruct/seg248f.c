/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 248f**, image
 * 0x248f0..0x26190. The segment was found by looking at where the binary's own
 * far calls land: 140 of them carry the segment 0x248f, and nothing between
 * 0x248f0 and the sound module at 0x26190 is reached any other way. Functions
 * are in address order and each carries the image offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x248fe
 *
 * Open the bit reader on a block of data, and answer the record - which is at
 * DGROUP 0x6402 and is the same four words `vqt_node` reads: a 32-bit bit
 * position, then the data as a far pointer.
 *
 * There is only one of them. DGROUP 0x6400 says whether it is in use and a
 * second open answers 0 rather than taking it away from the first.
 */
uint16_t open_bit_reader(uint16_t off, uint16_t seg)
{
    if (DGU16(0x6400) != 0)
        return 0;

    DGU16(0x6400) = 1;
    DGU16(0x6408) = seg;
    DGU16(0x6406) = off;
    DGU16(0x6404) = 0;
    DGU16(0x6402) = 0;

    return 0x6402;
}

/*
 * 0x24930
 *
 * Give the bit reader back.
 */
void close_bit_reader(void)
{
    DGU16(0x6400) = 0;
}

/*
 * 0x24e9a
 *
 * NOT TRANSCRIBED YET. Draw a bitmap held through the "BMP:OFF:" offset table.
 * 216 bytes, and unreached on every path the port is driven through.
 *
 * It was read once and not written, because the reading is not safe yet. It
 * normalises the header's `seg:off` into paragraphs and a remainder - `hdr[2]
 * >> 4` added to `hdr[0]`, `hdr[2] & 0xf` kept aside - and then hands
 * `open_bit_reader` **the sign word `cwd` just produced**, not the remainder,
 * which is stored at `[bp-4]` and never read again. Either the remainder is
 * genuinely dropped or the two arguments mean the opposite of what their names
 * here say, and nothing that can be *run* distinguishes the two. Writing the
 * plausible one would be exactly the trap this project is built to avoid.
 */
void draw_offset_bitmap(uint16_t hdr, int16_t x, int16_t y, uint16_t mode)
{
    (void)hdr; (void)x; (void)y; (void)mode;
    not_transcribed("0x24e9a, drawing an offset-table bitmap");
}

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
 * Any failure frees the list and answers 0; the record is closed only if this
 * routine opened it.
 */
uint16_t load_bitmaps(uint16_t name)
{
    uint16_t fp = dg_enter(0xa2);
    uint16_t saved_a = (uint16_t)(fp + 0x44);   /* [bp-0x5e] */
    uint16_t saved_b = fp;                      /* [bp-0xa2] */
    uint16_t count_at = (uint16_t)(fp + 0x8e);  /* [bp-4]    */
    uint16_t list_at = (uint16_t)(fp + 0x90);   /* [bp-2]    */
    uint16_t offset_at = (uint16_t)(fp + 0x8e - 0x10); /* [bp-0x14] */

    uint16_t di = name;
    uint16_t opened = 0;                        /* [bp-8]  */
    uint16_t blk_seg = 0, blk_off = 0;          /* [bp-0xa], [bp-0xc] */
    uint16_t kind = 0;                          /* [bp-0x1a] */
    uint16_t i;
    uint32_t r;

    DGU16(list_at) = 0;

    if (file_record_valid(di) == 0) {
        opened = 1;
        di = open_file_record(di);
        if (di == 0)
            goto fail;
    }

    copy_file_record(saved_a, di);

    if (seek_named_chunk(di, 0x49c6, 0) != 0xffffffffu) {      /* "BMP:SCN:" */
        copy_file_record(saved_b, di);
        restore_file_record_from(saved_a);

        if (read_bmp_info(di, count_at, list_at) == 0)
            goto fail;

        set_field_4_of_each(0xfffe, DGU16(list_at));
        restore_file_record_from(saved_b);
        kind = 0;
    } else {
        if (seek_named_chunk(di, 0x49cf, 0) == 0xffffffffu)    /* "BMP:OFF:" */
            goto planar;

        game_fread(count_at - 0x16, 2, 1, di);   /* [bp-0x1a], the kind */
        kind = DGU16((uint16_t)(count_at - 0x16));

        restore_file_record_from(saved_a);

        if (read_bmp_info(di, count_at, list_at) == 0)
            goto fail;

        set_field_4_of_each(0xffff, DGU16(list_at));

        if (seek_named_chunk(di, 0x49d8, 0) == 0xffffffffu)    /* "BMP:VQT:" */
            goto fail;
    }

    if (kind == 0) {
        uint32_t size = file_record_size(di);
        uint32_t blk = dos_alloc_bytes((uint16_t)size, (uint16_t)(size >> 16),
                                       0, 0);

        blk_seg = (uint16_t)(blk >> 16);
        blk_off = (uint16_t)blk;
        if (blk == 0)
            goto fail;

        read_far(blk_off, blk_seg, (uint16_t)size, (uint16_t)(size >> 16), di);

        if (seek_named_chunk(di, 0x49e1, 0) == 0xffffffffu) {  /* "BMP:OFF:" */
            dos_free_far(blk_off, blk_seg);
            goto fail;
        }

        for (i = 0; i < DGU16(count_at); i++) {
            uint16_t si;
            uint32_t p;

            if (game_fread(offset_at, 4, 1, di) != 1) {
                dos_free_far(blk_off, blk_seg);
                goto fail;
            }

            p = huge_add(blk_off, blk_seg,
                         (int32_t)(((uint32_t)DGU16((uint16_t)(offset_at + 2))
                                    << 16) | DGU16(offset_at)));

            si = DGU16((uint16_t)(DGU16(list_at) + 2 * i));
            DGU16(si) = (uint16_t)(p >> 16);
            DGU16((uint16_t)(si + 2)) = (uint16_t)p;
        }
    } else {
        uint16_t fp2 = dg_enter(4);
        uint32_t blk;

        r = vm_bitmap_list_size(DGU16(list_at), count_at - 2);
        blk = dos_alloc_bytes((uint16_t)r, (uint16_t)(r >> 16), 0, 0);

        blk_seg = (uint16_t)(blk >> 16);
        blk_off = (uint16_t)blk;
        if (blk == 0) {
            dg_leave(4);
            goto fail;
        }

        set_field_4_of_each(0xfffc, DGU16(list_at));

        DGU16(fp2) = blk_off;
        DGU16((uint16_t)(fp2 + 2)) = blk_seg;

        for (i = 0; i < DGU16(count_at); i++) {
            uint16_t si = DGU16((uint16_t)(DGU16(list_at) + 2 * i));

            DGU16(si) = DGU16((uint16_t)(fp2 + 2));
            DGU16((uint16_t)(si + 2)) = DGU16(fp2);

            huge_add_to(fp2, DGROUP_SEG,
                        (uint16_t)(DG16((uint16_t)(si + 6))
                                   * DG16((uint16_t)(si + 8))));
        }
        dg_leave(4);

        decode_vqt_list(di, DGU16(list_at));
    }
    goto loaded;

planar:
    DGU16(list_at) = load_bitmap_list(di);

loaded:
    DGU16(count_at) = count_list(DGU16(list_at));

    if (seek_named_chunk(di, 0x49ea, 0) != 0xffffffffu)        /* "BMP:RLE:" */
        compress_bitmap_list(DGU16(list_at), 0x10);

    if (seek_named_chunk(di, 0x49f3, 0) != 0xffffffffu)        /* "BMP:SCL:" */
        set_field_4_of_each(0xfffd, DGU16(list_at));

    goto out;

fail:
    free_bitmaps_thunk(DGU16(list_at));
    DGU16(list_at) = 0;

out:
    if (opened != 0)
        close_file_record(di);

    {
        uint16_t answer = DGU16(list_at);

        dg_leave(0xa2);
        return answer;
    }
}

/*
 * 0x252b4
 *
 * Walk a null-terminated array of near pointers and write the same word into
 * each target's +4.
 *
 * The array is the second argument and the word the first, which is the order
 * the compiler pushed them and not the order it reads them.
 */
void set_field_4_of_each(uint16_t value, uint16_t list)
{
    while (DGU16(list) != 0) {
        DG16((uint16_t)(DGU16(list) + 4)) = (int16_t)value;
        list = (uint16_t)(list + 2);
    }
}

/*
 * 0x252d0
 *
 * A thunk from this module into `free_bitmaps` in segment 1c25, which is a
 * `push`, an `lcall` and nothing else. It exists because the two are different
 * translation units and the call has to be far.
 */
void free_bitmaps_thunk(uint16_t list)
{
    free_bitmaps(list);
}

/*
 * 0x252e0
 *
 * Count the entries in a null-terminated array of words. A null array answers
 * 0 without looking at it, which is what the test before the loop is for.
 */
uint16_t count_list(uint16_t list)
{
    uint16_t n = 0;

    if (list == 0)
        return 0;

    while (DGU16((uint16_t)(list + 2 * n)) != 0)
        n++;

    return n;
}

/*
 * 0x25300
 *
 * Draw one bitmap, choosing how by the marker its loader left in field 4.
 *
 * The header's far pointer is normalised first - paragraphs out of the offset
 * and into the segment - and *written back*, so a bitmap drawn twice is
 * normalised once. Then:
 *
 *   0xfffd  scaled, through the driver at VGA:0x271b, and with three arguments
 *           rather than four
 *   0xfffe  compressed, by 0x20185 - the form `compress_bitmap_list` writes
 *   0xffff  the offset-table form, by 0x24e9a
 *   other   plain planar, through the driver's structured blit at VGA:0x1707 -
 *           and "other" is not a fall-through for the unexpected, it is the
 *           ordinary case: an uncompressed bitmap's field 4 holds the offset of
 *           its mask, which is a small number and not a marker at all.
 */
void draw_bitmap(uint16_t hdr, int16_t x, int16_t y, uint16_t mode)
{
    DGU16(hdr) = (uint16_t)(DGU16(hdr) + (DGU16((uint16_t)(hdr + 2)) >> 4));
    DGU16((uint16_t)(hdr + 2)) = (uint16_t)(DGU16((uint16_t)(hdr + 2)) & 0x0f);

    switch (DGU16((uint16_t)(hdr + 4))) {
    case 0xfffd:
        blit_scaled_thunk(hdr, x, y);
        return;
    case 0xfffe:
        draw_compressed_bitmap(hdr, x, y, mode);
        return;
    case 0xffff:
        draw_offset_bitmap(hdr, x, y, mode);
        return;
    default:
        blit_bitmap_thunk(hdr, x, y, mode);
        return;
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
 * picture is in video memory by then, not in it.
 */
uint16_t load_screen(uint16_t name)
{
    uint16_t fp = dg_enter(0x4e);
    uint16_t saved = fp;                    /* [bp-0x4e] */

    uint16_t si = name;
    uint16_t opened = 0;                    /* [bp-2]  */
    uint16_t blk_seg = 0, blk_off = 0;      /* [bp-4], [bp-6] */
    uint16_t di = 0;

    if (file_record_valid(si) == 0) {
        opened = 1;
        si = open_file_record(si);
        if (si == 0) {
            di = 0xffff;
            goto out;
        }
    }

    copy_file_record(saved, si);

    if (seek_named_chunk(si, 0x49fe, 0) == 0xffffffffu) {   /* "SCR:VQT:" */
        restore_file_record_from(saved);
        di = load_screen_plain(si);
        goto close;
    }

    {
        uint32_t size = file_record_size(si);
        uint32_t blk = dos_alloc_bytes((uint16_t)size, (uint16_t)(size >> 16),
                                       0, 0);

        blk_seg = (uint16_t)(blk >> 16);
        blk_off = (uint16_t)blk;
        if (blk == 0) {
            di = 0xffff;
            goto out;
        }

        read_far(blk_off, blk_seg, (uint16_t)size, (uint16_t)(size >> 16), si);
    }

    DGU16(0x640c) = open_bit_reader(blk_off, blk_seg);
    if (DGU16(0x640c) == 0) {
        di = 0xffff;
        goto out;
    }

    clear_flag_2d44();
    vqt_screen_node(0, 0, 0x140, 0xc8);
    set_flag_2d44();
    close_bit_reader();

close:
    if (opened != 0)
        close_file_record(si);

out:
    if (huge_equal(blk_off, blk_seg, 0, 0) == 0)
        dos_free_far(blk_off, blk_seg);

    dg_leave(0x4e);
    return di;
}

/*
 * 0x2551a
 *
 * Read a 32-bit count of bytes from a file into a far destination, through a
 * bounce buffer, because the read below it takes a **near** buffer and a
 * 16-bit count.
 *
 * The buffer is as big as the near heap will give it: it asks for 0x4000 and
 * halves down to 0x800, then steps down by 0x100 at a time, and if even that
 * fails it falls back to 0x100 bytes of its own stack. So a machine with a full
 * heap reads in small pieces rather than failing.
 *
 * The destination is renormalised every 0x10000/buffer reads - which is what
 * the divide at the top is for - by adding a whole segment to the pointer and
 * starting the offset again, so a destination longer than 64 KB is written
 * without the offset ever wrapping.
 *
 * A short read ends it, whatever the count still says. A **near** routine.
 */
void read_far(uint16_t dst_off, uint16_t dst_seg,
              uint16_t count_lo, uint16_t count_hi, uint16_t file)
{
    uint16_t fp = dg_enter(0x10a);
    uint16_t fallback = fp;             /* [bp-0x10a], 0x100 bytes */

    uint16_t buf;                       /* [bp-6]   */
    int16_t si = 0x4000;
    int16_t per_segment;                /* [bp-8]   */
    int16_t left_in_segment;            /* [bp-0xa] */
    uint16_t walk_off, walk_seg;        /* [bp-4], [bp-2] */
    uint16_t ptr_off = dst_off, ptr_seg = dst_seg;
    uint32_t remaining = ((uint32_t)count_hi << 16) | count_lo;

    for (;;) {
        if (si == 0)
            break;
        buf = heap_malloc_far((uint16_t)si);
        if (buf != 0)
            break;
        if (si > 0x800)
            si = (int16_t)(si >> 1);
        else
            si = (int16_t)(si - 0x100);
    }

    if (si == 0) {
        buf = fallback;
        si = 0x100;
    }

    per_segment = (count_hi != 0)
                  ? (int16_t)long_divide(0x00010000L, si)
                  : 0;
    left_in_segment = per_segment;

    walk_seg = ptr_seg;
    walk_off = ptr_off;

    while (remaining != 0) {
        uint16_t want = (uint16_t)(((int32_t)si <= (int32_t)remaining)
                                   ? (uint16_t)si : (uint16_t)remaining);
        uint16_t got = game_fread(buf, 1, want, file);

        if (got == 0)
            break;

        far_copy(walk_off, walk_seg, buf, DGROUP_SEG, got);

        walk_off = (uint16_t)(walk_off + got);
        remaining -= got;

        if (per_segment != 0 && --left_in_segment == 0) {
            uint16_t fp2 = dg_enter(4);

            DGU16(fp2) = ptr_off;
            DGU16((uint16_t)(fp2 + 2)) = ptr_seg;
            huge_add_to(fp2, DGROUP_SEG, 0x00010000L);
            ptr_off = DGU16(fp2);
            ptr_seg = DGU16((uint16_t)(fp2 + 2));
            dg_leave(4);

            left_in_segment = per_segment;
            walk_seg = ptr_seg;
            walk_off = ptr_off;
        }
    }

    if (buf != 0 && buf != fallback)
        heap_free_far(buf);

    dg_leave(0x10a);
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
 * Then, for each bitmap: the record at DGROUP 0x640c is filled in with where
 * the four planes go - each one a quarter of the pixels apart - and where each
 * row starts, and `vqt_node` walks the quadtree that paints it. Afterwards the
 * bits consumed are rounded up to whole bytes, the unread tail of the buffer is
 * slid down to the front, and as much as will fit is read in behind it.
 *
 * The header's far pointer is normalised on the way in - paragraphs out of the
 * offset and into the segment - so a bitmap whose planes cross a segment
 * boundary is addressed the same way as one that does not.
 *
 * A **near** routine.
 */
void decode_vqt_list(uint16_t file, uint16_t list)
{
    uint16_t fp = dg_enter(0x1ca);
    uint16_t rd = fp;                       /* [bp-0x1ca], the reader record */
    uint16_t cur = (uint16_t)(fp + 0x1c0);  /* [bp-0xa]/[bp-8], walked by
                                             * huge_add_to, so it needs a real
                                             * DGROUP address */
    uint16_t at = list;                     /* [bp-2]  */
    uint32_t largest = 0;                   /* [bp-0x20] */
    uint32_t free_bytes, file_left;
    uint32_t buffer;                        /* [bp-0x18]/[bp-0x1a] */
    uint16_t blk_seg = 0, blk_off = 0;      /* [bp-0xc], [bp-0xe] */
    uint16_t index = 0;                     /* [bp-0x12] */
    uint16_t si;

    while (DGU16(at) != 0) {
        uint16_t hdr = DGU16(at);
        uint32_t need = buffer_size_thunk(DGU16((uint16_t)(hdr + 6)),
                                          DGU16((uint16_t)(hdr + 8)))
                        & 0xffffu;

        if (largest < need)
            largest = need;

        at = (uint16_t)(at + 2);
    }

    free_bytes = dos_alloc_bytes(0xffff, 0xffff, 0, 0);
    file_left = file_record_size(file);
    buffer = free_bytes;

    if (file_left <= free_bytes) {
        buffer = file_left;
        largest = 0;
    }

    if (largest <= buffer) {
        uint32_t blk = dos_alloc_bytes((uint16_t)buffer, (uint16_t)(buffer >> 16),
                                       0, 0);

        blk_seg = (uint16_t)(blk >> 16);
        blk_off = (uint16_t)blk;
        if (blk == 0)
            goto no_block;
        goto have_block;
    }

no_block:
    if ((DGU16(0x3576) | DGU16(0x3578)) == 0)
        goto done;
    if (largest > 0x3ab4)
        goto done;

    blk_seg = DGU16(0x3578);
    blk_off = DGU16(0x3576);
    buffer = 0x3ab4;

have_block:
    DGU16(0x640c) = rd;
    DGU16(rd) = 0;
    DGU16((uint16_t)(rd + 2)) = 0;
    DGU16((uint16_t)(rd + 4)) = blk_off;
    DGU16((uint16_t)(rd + 6)) = blk_seg;

    read_far(blk_off, blk_seg, (uint16_t)buffer, (uint16_t)(buffer >> 16), file);
    file_left -= buffer;

    at = list;

    while ((si = DGU16(at)) != 0) {
        uint32_t used;
        uint16_t plane_off, plane_seg;
        uint16_t row;
        int16_t i;
        uint32_t quarter;

        plane_seg = (uint16_t)(DGU16(si)
                               + (DGU16((uint16_t)(si + 2)) >> 4));
        plane_off = (uint16_t)(DGU16((uint16_t)(si + 2)) & 0x0f);

        quarter = (uint32_t)(uint16_t)((int16_t)(DG16((uint16_t)(si + 6))
                                                 * DG16((uint16_t)(si + 8)))
                                       >> 2);

        for (i = 0; i < 4; i++) {
            DGU16((uint16_t)(rd + 4 * i + 0x0a)) = plane_seg;
            DGU16((uint16_t)(rd + 4 * i + 0x08)) = plane_off;
            plane_off = (uint16_t)(plane_off + quarter);
        }

        row = 0;
        for (i = 0; DG16((uint16_t)(si + 8)) > i; i++) {
            DGU16((uint16_t)(rd + 2 * i + 0x18)) = row;
            row = (uint16_t)(row + DG16((uint16_t)(si + 6)));
        }

        vqt_node(0, 0, DGU16((uint16_t)(si + 6)), DGU16((uint16_t)(si + 8)));

        used = ((uint32_t)DGU16((uint16_t)(rd + 2)) << 16) | DGU16(rd);
        used = (uint32_t)long_shift_right((int32_t)(used + 7), 3);

        DGU16(rd) = 0;
        DGU16((uint16_t)(rd + 2)) = 0;

        DGU16(cur) = DGU16((uint16_t)(rd + 4));
        DGU16((uint16_t)(cur + 2)) = DGU16((uint16_t)(rd + 6));

        if (file_left != 0) {
            uint32_t p = huge_add(DGU16(cur), DGU16((uint16_t)(cur + 2)),
                                  (int32_t)used);
            uint32_t chunk;

            far_copy(DGU16(cur), DGU16((uint16_t)(cur + 2)),
                     (uint16_t)p, (uint16_t)(p >> 16),
                     (uint16_t)((uint16_t)buffer - (uint16_t)used));

            huge_add_to(cur, DGROUP_SEG, (int32_t)(buffer - used));

            chunk = (used >= file_left) ? file_left : used;
            if (chunk > buffer)
                chunk = buffer;

            read_far(DGU16(cur), DGU16((uint16_t)(cur + 2)),
                     (uint16_t)chunk, (uint16_t)(chunk >> 16), file);
            file_left -= chunk;
        } else {
            uint32_t p = huge_add(DGU16(cur), DGU16((uint16_t)(cur + 2)),
                                  (int32_t)used);

            DGU16((uint16_t)(rd + 6)) = (uint16_t)(p >> 16);
            DGU16((uint16_t)(rd + 4)) = (uint16_t)p;
        }

        at = (uint16_t)(at + 2);
        index++;
    }

    if (blk_seg != DGU16(0x3578) || blk_off != DGU16(0x3576))
        dos_free_far(blk_off, blk_seg);

done:
    (void)index;
    dg_leave(0x1ca);
}

/*
 * 0x259a1
 *
 * The quadtree walk again, but for a whole *screen* rather than a bitmap: the
 * same four-bit code, the same halves, the same reader record at DGROUP 0x640c,
 * and `fill_screen_quadrant` where `vqt_node` has its own leaf. The original
 * has the two written out separately rather than sharing one, and the port
 * keeps them apart for the same reason - they are two routines at two
 * addresses.
 *
 * One thing is not symmetric and is not a slip in the reading: **only the first
 * quadrant redraws the cursor** after its fill. The other three do not. That
 * keeps the pointer on top while a screen paints itself in without paying for a
 * redraw at every leaf.
 */
void vqt_screen_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t rd, code;
    uint32_t pos;
    uint16_t data_off, data_seg;

    if ((w | h) == 0)
        return;

    rd = DGU16(0x640c);
    pos = ((uint32_t)DGU16((uint16_t)(rd + 2)) << 16) | DGU16(rd);

    DGU16(rd) = (uint16_t)(pos + 4);
    DGU16((uint16_t)(rd + 2)) = (uint16_t)((pos + 4) >> 16);

    data_off = DGU16((uint16_t)(rd + 4));
    data_seg = DGU16((uint16_t)(rd + 6));

    code = (uint16_t)((FARU16(data_seg, (uint16_t)(data_off + (pos >> 3)))
                       >> (pos & 7)) & 0x0f);

    if (code & 8) {
        vqt_screen_node(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
    } else {
        fill_screen_quadrant(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
        redraw_cursor(DGU16(0x38a4));
    }

    if (code & 4)
        vqt_screen_node((uint16_t)(x + (w >> 1)), y,
                        (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));
    else
        fill_screen_quadrant((uint16_t)(x + (w >> 1)), y,
                             (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));

    if (code & 2)
        vqt_screen_node(x, (uint16_t)(y + (h >> 1)),
                        (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_screen_quadrant(x, (uint16_t)(y + (h >> 1)),
                             (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));

    if (code & 1)
        vqt_screen_node((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                        (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_screen_quadrant((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                             (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
}

/*
 * 0x25aaa
 *
 * NOT TRANSCRIBED YET. The screen quadtree's leaf: paint one rectangle from
 * what the bit stream says next.
 */
void fill_screen_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    (void)x; (void)y; (void)w; (void)h;
    not_transcribed("0x25aaa, the screen quadtree's leaf");
}

/*
 * 0x25d96
 *
 * A far block move, destination first: words then a trailing byte, the odd
 * count carried out of `shr cx,1` in the carry flag.
 *
 * This is the third routine in the port that does this - `far_memcpy` at
 * 0x222c6 and `far_move` at 0x0bd2e are the others - and all three differ in
 * argument order or in which register holds the count. They are separate
 * routines in the original and stay separate here.
 */
void far_copy(uint16_t dst_off, uint16_t dst_seg, uint16_t src_off,
              uint16_t src_seg, uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        *FAR_PTR(dst_seg, (uint16_t)(dst_off + i)) =
            *FAR_PTR(src_seg, (uint16_t)(src_off + i));
}
/*
 * 0x25db8
 *
 * One node of the quadtree the "BMP:VQT:" chunk is: read four bits, and for
 * each quadrant either recurse into this again or hand it to `fill_quadrant` to
 * be painted.
 *
 * The bit reader is the record at DGROUP 0x640c: a 32-bit bit position at +0
 * and the data as a far pointer at +4. Four bits are taken and the position
 * advanced, the byte is found by shifting the position right three, a *word* is
 * read from there and shifted down by the position's low three bits - reading a
 * word rather than a byte is what lets a code straddle a byte boundary without
 * any special case.
 *
 * The four halves are `w >> 1` and `(w + 1) >> 1`, so an odd width puts the
 * extra column in the right-hand pair and an odd height the extra row in the
 * bottom pair. Bit 8 of the code is the top-left quadrant, then 4, 2 and 1
 * clockwise - and a set bit means subdivide, a clear one means fill.
 *
 * A zero width *and* height ends the recursion; either alone does not.
 *
 * A **near** routine, and it shares the epilogue three bytes above its own
 * entry for the early return.
 */
void vqt_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t rd, code;
    uint32_t pos;
    uint16_t data_off, data_seg;

    if ((w | h) == 0)
        return;

    rd = DGU16(0x640c);
    pos = ((uint32_t)DGU16((uint16_t)(rd + 2)) << 16) | DGU16(rd);

    DGU16(rd) = (uint16_t)(pos + 4);
    DGU16((uint16_t)(rd + 2)) = (uint16_t)((pos + 4) >> 16);

    data_off = DGU16((uint16_t)(rd + 4));
    data_seg = DGU16((uint16_t)(rd + 6));

    code = (uint16_t)((FARU16(data_seg, (uint16_t)(data_off + (pos >> 3)))
                       >> (pos & 7)) & 0x0f);

    if (code & 8)
        vqt_node(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
    else
        fill_quadrant(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));

    if (code & 4)
        vqt_node((uint16_t)(x + (w >> 1)), y,
                 (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));
    else
        fill_quadrant((uint16_t)(x + (w >> 1)), y,
                      (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));

    if (code & 2)
        vqt_node(x, (uint16_t)(y + (h >> 1)),
                 (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_quadrant(x, (uint16_t)(y + (h >> 1)),
                      (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));

    if (code & 1)
        vqt_node((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                 (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_quadrant((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                      (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
}

/*
 * 0x25eb5
 *
 * NOT TRANSCRIBED YET. The leaf of the quadtree: paint one rectangle of the
 * bitmap from what the bit stream says next. 1,853 bytes, and the largest
 * single routine still stubbed.
 *
 * It was described here as standing in the way of the port's first frame. It
 * no longer is: the port draws the intro, the copy-protection screen and the
 * whole level-one briefing without reaching it, and `tools/check_briefing.py`
 * measures that at 0 of 307,200 pixels. **Nothing the port is driven through
 * today calls it** - not the panel, not the picker, not the puzzle screen, not
 * a save - so it is unreached rather than blocking, and a transcription of it
 * could not be verified against anything.
 */
void fill_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    not_transcribed("0x25eb5, the quadtree leaf");
}

