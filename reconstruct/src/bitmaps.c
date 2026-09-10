/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Bitmap and screen files**: the formats a bitmap can be held in
 * and the code that loads and draws each of them.
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
dg_off_t open_bit_reader(struct far_ptr data)
{
    if (BITMAPS.in_use != 0)
        return 0;

    BITMAPS.in_use = 1;
    BITMAPS.data = data;
    BITMAPS.pos = 0;

    /* The address of the eight bytes above, not a handle - `mov ax, 0x6402`
       at 0x2492b. `dg_off(dgroup, &BITMAPS.pos)` is the same number and
       says which bytes it is. */
    return dg_off(dgroup, (const void *)&BITMAPS.pos);
}

/*
 * 0x24930
 *
 * Give the bit reader back.
 */
void close_bit_reader(void)
{
    BITMAPS.in_use = 0;
}

/*
 * 0x24e9a
 *
 * NOT TRANSCRIBED YET. Draw a bitmap held through the "BMP:OFF:" offset table.
 * 216 bytes.
 *
 * **Unreachable with this game's data, and now counted rather than assumed.**
 * `load_bitmaps` looks for "BMP:SCN:" first and takes the compressed form when
 * it is there; only when it is absent does it look for "BMP:OFF:", set the
 * 0xffff marker this draws, and then *require* a "BMP:VQT:" chunk. Across the
 * 162 extracted resources: 58 files carry OFF: and **all 58 carry SCN: as
 * well**, so the compressed branch always wins, and VQT: appears in **none**,
 * so the offset branch would fail before it drew anything even if it were
 * taken. RLE: is in one file, PARTBIN.BMP, and SCL: in none.
 *
 * That is why "unreached on every path the port is driven through" understated
 * it: it is not that nothing has happened to reach this, it is that no bitmap
 * the game ships can.
 *
 * It was read once and not written, because the reading is not safe yet. It
 * normalises the header's `seg:off` into paragraphs and a remainder - `bmp[2]
 * >> 4` added to `bmp[0]`, `bmp[2] & 0xf` kept aside - and then hands
 * `open_bit_reader` **the sign word `cwd` just produced**, not the remainder,
 * which is stored at `[bp-4]` and never read again. Either the remainder is
 * genuinely dropped or the two arguments mean the opposite of what their names
 * here say, and nothing that can be *run* distinguishes the two. Writing the
 * plausible one would be exactly the trap this project is built to avoid.
 */
void draw_offset_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    (void)bmp; (void)x; (void)y; (void)mode;
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
uint16_t load_bitmaps(uint8_t * name)
{
    /* **68 bytes each, and `saved_a` was 52.** `copy_file_record` writes 0x43
       into both, so every call ran fifteen bytes past this one - silently,
       because what is above it in the frame is the port's own locals and the
       original's `[bp-0x5e]` to `[bp-0x1a]` is 0x44 whichever end it is
       measured from. AddressSanitizer caught it on the first run of a build
       that had never linked; see the note in CLAUDE.md. */
    uint8_t saved_a[68];                      /* [bp-0x5e] */
    uint8_t saved_b[68];                      /* [bp-0xa2] */
    int16_t count_at;  /* [bp-4]    */
    int16_t list_at;   /* [bp-2]    */
    int16_t offset_at[7]; /* [bp-0x14] */
    /* Two more slots the original addresses as `count_at` less a constant
       rather than by name: [bp-0x1a] is the kind and [bp-6] the size
       `vm_bitmap_list_size` writes. 0x8e - 0x16 and 0x8e - 2. */
    int16_t kind_at[3];   /* [bp-0x1a] */
    int16_t size_at;   /* [bp-6]    */

    /*
     * `name` is either an open file record's handle or the address of a
     * filename - the original tells them apart by asking `file_record_valid`
     * whether the number matches one of four `file_ptr`s. A pointer outside
     * guest memory cannot be a handle, so `dg_is_guest` answers that half
     * exactly; measured on 2026-09-09 the test never fires at all, on any of
     * the ten call sites.
     */
    uint16_t as_handle = dg_is_guest(name) ? dg_off(dgroup, name) : 0;
    uint16_t di = as_handle;
    uint16_t opened = 0;                        /* [bp-8]  */
    struct far_ptr block = {0, 0};              /* [bp-0xc], [bp-0xa] */
    uint16_t kind = 0;                          /* [bp-0x1a] */
    uint16_t i;
    uint32_t r;

    list_at = (int16_t)0;

    if (as_handle == 0 || file_record_valid(as_handle) == 0) {
        opened = 1;
        di = open_file_record(name);
        if (di == 0)
            goto fail;
    }

    copy_file_record(saved_a, di);

    if (seek_named_chunk(di, 0x49c6, 0) != 0xffffffffu) {      /* "BMP:SCN:" */
        copy_file_record(saved_b, di);
        restore_file_record_from(saved_a);

        if (read_bmp_info(di, (uint8_t *)&count_at,
                          (uint8_t *)&list_at) == 0)
            goto fail;

        set_field_4_of_each(0xfffe, BMPLIST((uint16_t)list_at));
        restore_file_record_from(saved_b);
        kind = 0;
    } else {
        if (seek_named_chunk(di, 0x49cf, 0) == 0xffffffffu)    /* "BMP:OFF:" */
            goto planar;

        game_fread((uint8_t *)kind_at, 2, 1, di);
        kind = (uint16_t)kind_at[0];

        restore_file_record_from(saved_a);

        if (read_bmp_info(di, (uint8_t *)&count_at,
                          (uint8_t *)&list_at) == 0)
            goto fail;

        set_field_4_of_each(0xffff, BMPLIST((uint16_t)list_at));

        if (seek_named_chunk(di, 0x49d8, 0) == 0xffffffffu)    /* "BMP:VQT:" */
            goto fail;
    }

    if (kind == 0) {
        uint32_t size = file_record_size(di);
        block = dos_alloc_bytes((uint16_t)size,
                                (uint16_t)(size >> 16), 0, 0).ptr;
        if (far_eq(block, FAR_NULL))
            goto fail;

        read_far(MK_FP(block.seg, block.off), (int32_t)size, di);

        if (seek_named_chunk(di, 0x49e1, 0) == 0xffffffffu) {  /* "BMP:OFF:" */
            dos_free_far(block);
            goto fail;
        }

        for (i = 0; i < (uint16_t)count_at; i++) {
            uint16_t si;
            struct far_ptr p;

            if (game_fread((uint8_t *)offset_at, 4, 1, di) != 1) {
                dos_free_far(block);
                goto fail;
            }

            p = huge_add(block,
                         (int32_t)(((uint32_t)(uint16_t)offset_at[1]
                                    << 16) | (uint16_t)offset_at[0]));

            si = BMPLIST((uint16_t)list_at)[i];
            BMP(si).data = far_to_rev(p);
        }
    } else {
        /* As in `read_far`: four bytes for `huge_add_to` to step, and
           nothing but that call sees the address. */
        struct far_ptr fp2;


        r = vm_bitmap_list_size((uint16_t)list_at, (uint8_t *)&size_at);
        block = dos_alloc_bytes((uint16_t)r, (uint16_t)(r >> 16), 0, 0).ptr;
        if (far_eq(block, FAR_NULL))
            goto fail;

        set_field_4_of_each(0xfffc, BMPLIST((uint16_t)list_at));

        fp2 = block;

        for (i = 0; i < (uint16_t)count_at; i++) {
            uint16_t si = BMPLIST((uint16_t)list_at)[i];

            BMP(si).data = far_to_rev(fp2);

            huge_add_to(&fp2,
                        (uint16_t)(BMP(si).width
                                   * BMP(si).height));
        }

        decode_vqt_list(di, (uint16_t)list_at);
    }
    goto loaded;

planar:
    list_at = (int16_t)load_bitmap_list(di);

loaded:
    count_at = (int16_t)count_list(BMPLIST((uint16_t)list_at));

    if (seek_named_chunk(di, 0x49ea, 0) != 0xffffffffu)        /* "BMP:RLE:" */
        compress_bitmap_list((uint16_t)list_at, 0x10);

    if (seek_named_chunk(di, 0x49f3, 0) != 0xffffffffu)        /* "BMP:SCL:" */
        set_field_4_of_each(0xfffd, BMPLIST((uint16_t)list_at));

    goto out;

fail:
    free_bitmaps_thunk(BMPLIST((uint16_t)list_at));
    list_at = (int16_t)0;

out:
    if (opened != 0)
        close_file_record(di);

    {
        uint16_t answer = (uint16_t)list_at;
        return answer;
    }
}

/*
 * 0x252b4
 *
 * Walk a bitmap list and write the same word into every header's `mask_off`,
 * which is the +4 the original writes and the field a loader leaves its
 * marker in.
 *
 * The array is the second argument and the word the first, which is the order
 * the compiler pushed them and not the order it reads them.
 */
void set_field_4_of_each(uint16_t value, dg_off_t * list)
{
    dg_off_t *p = list;

    while (*p != 0) {
        BMPP(*p)->mask_off = value;
        p++;
    }
}

/*
 * 0x252d0
 *
 * A thunk from this module into `free_bitmaps` in segment 1c25, which is a
 * `push`, an `lcall` and nothing else. It exists because the two are different
 * translation units and the call has to be far.
 */
void free_bitmaps_thunk(dg_off_t * list)
{
    free_bitmaps(list);
}

/*
 * 0x252e0
 *
 * Count the entries in a null-terminated array of words. A null array answers
 * 0 without looking at it, which is what the test before the loop is for.
 *
 * The array is what the routine is handed, so it is spelled as one. The
 * original's test is `list == 0` on the offset, and offset 0 is `dgroup`
 * itself rather than a C null pointer - `dg_off` answers 0 for both, which is
 * why the guard is written through it and not as `list == NULL`.
 */
uint16_t count_list(dg_off_t * list)
{
    uint16_t n = 0;

    if (dg_off(dgroup, list) == 0)
        return 0;

    while (list[n] != 0)
        n++;

    return n;
}

/*
 * 0x25300
 *
 * Draw one bitmap, choosing how by the marker its loader left in `mask_off`.
 *
 * **It takes the header, not its offset.** The guest pushes one word and the
 * original does `BMP(hdr).` throughout; what that word names is a
 * `struct bitmap`, so the port takes one and the four routines below it do
 * too. `BMPP` is the offset a caller still holds turned into it.
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
 *           ordinary case: an uncompressed bitmap's `mask_off` holds the
 *           offset of its mask, which is a small number and not a marker at
 *           all.
 */
void draw_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    bmp->data = far_normalise_rev(bmp->data);

    switch (bmp->mask_off) {
    case 0xfffd:
        blit_scaled_thunk(bmp, x, y);
        return;
    case 0xfffe:
        draw_compressed_bitmap(bmp, x, y, mode);
        return;
    case 0xffff:
        draw_offset_bitmap(bmp, x, y, mode);
        return;
    default:
        blit_bitmap_thunk(bmp, x, y, mode);
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
    uint8_t saved[78];                    /* [bp-0x4e] */

    uint16_t si = name;
    uint16_t opened = 0;                    /* [bp-2]  */
    struct far_ptr block = {0, 0};          /* [bp-6], [bp-4] */
    uint16_t di = 0;

    if (file_record_valid(si) == 0) {
        opened = 1;
        si = open_file_record(dg_ptr(dgroup, si));
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
        block = dos_alloc_bytes((uint16_t)size,
                                (uint16_t)(size >> 16), 0, 0).ptr;
        if (far_eq(block, FAR_NULL)) {
            di = 0xffff;
            goto out;
        }

        read_far(MK_FP(block.seg, block.off), (int32_t)size, si);
    }

    BITMAPS.reader = open_bit_reader(block);
    if (BITMAPS.reader == 0) {
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
    if (huge_equal(block.off, block.seg, 0, 0) == 0)
        dos_free_far(block);
    return di;
}

/*
 * 0x2551a
 *
 * Read a 32-bit count of bytes from a file into a far destination, through a
 * bounce buffer, because the read below it takes a **** buffer and a
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
 * A short read ends it, whatever the count still says. A **** routine.
 */
void read_far(uint8_t far *dst, int32_t count, uint16_t file)
{
    /* The only slot of this frame that is not already a C local below - the
       other ten bytes are `buf`, `per_segment`, `left_in_segment` and the two
       walk words, each carrying its own `[bp-N]`. */
    _Alignas(2) uint8_t fallback[0x100];        /* [bp-0x10a] */

    /* [bp-6], a heap block or `fallback`. The heap hands its blocks out
       `volatile`, which this one is not: it is a private bounce buffer, read
       and written by this routine alone between the `game_fread` that fills
       it and the `far_copy` that empties it. The cast drops the qualifier
       once, here, rather than carrying it through both calls. */
    uint8_t *  buf;
    int16_t si = 0x4000;
    int16_t per_segment;                /* [bp-8]   */
    int16_t left_in_segment;            /* [bp-0xa] */
    struct far_ptr walk;                /* [bp-4], [bp-2] */
    /* The parameter is a plain far pointer; the walk below needs the pair,
       because it crosses 64K boundaries with `huge_add_to`. `FP_SEG`/`FP_OFF`
       answer the normalised pair, which is what `huge_add_to` keeps it in
       anyway. */
    struct far_ptr ptr = { FP_OFF(dst), FP_SEG(dst) };
    /* One Borland `long`, pushed as [bp+8] and [bp+0xa]: the loop compares
       `si` against it with `cwd / cmp dx,[bp+0xa] / jg / cmp ax,[bp+8] / jbe`,
       which is a signed 32-bit compare and not two word tests. */
    int32_t remaining = count;

    for (;;) {
        if (si == 0)
            break;
        buf = (uint8_t *)heap_malloc_far((uint16_t)si);
        if (buf != NULL)
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

    /* Only the *high word* is tested - `xor ax,ax / or ax,[bp+0xa]` - so
       this is not `count >= 0x10000` however much it reads like it. */
    per_segment = ((uint16_t)((uint32_t)count >> 16) != 0)
                  ? (int16_t)long_divide(0x00010000L, si)
                  : 0;
    left_in_segment = per_segment;

    walk = ptr;

    while (remaining != 0) {
        uint16_t want = (uint16_t)(((int32_t)si <= remaining)
                                   ? (uint16_t)si : (uint16_t)remaining);
        uint16_t got = game_fread(buf, 1, want, file);

        if (got == 0)
            break;

        far_copy(MK_FP(walk.seg, walk.off), buf, got);

        walk.off = (uint16_t)(walk.off + got);
        remaining -= got;

        if (per_segment != 0 && --left_in_segment == 0) {
            /* The four bytes the original reserves so `huge_add_to` has a
               variable to step; nothing but that call sees the address, so
               unlike this routine's outer frame it is a local. */
            huge_add_to(&ptr, 0x00010000L);

            left_in_segment = per_segment;
            walk = ptr;
        }
    }

    if (buf != NULL && buf != fallback)
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
 * A **** routine.
 */
void decode_vqt_list(uint16_t file, uint16_t list)
{
    /*
     * `sub sp,0x1ca`. The reader record is at the bottom of it and the named
     * locals sit above; both are Borland locals, so the frame is a C array.
     *
     * **The reader record keeps the guest's stack, and one slot is why.**
     * `BITMAPS.reader = dg_off(dgroup, rd)` files the record's address into
     * a guest word that `vqt_node`, `vqt_screen_node` and `fill_quadrant`
     * fetch back out and write through, so the address has to be one the guest
     * can hold. The frame was a C array for a while and that word then took
     * the distance to somewhere outside guest memory; `dg_off` refuses such a
     * pointer now, which turned a wrong number into an abort and is what made
     * this worth putting back rather than leaving.
     *
     * The original has a defect of its own here and it is a different one: the
     * word outlives the call, holding a stack offset into a frame that has
     * been given back, and `load_screen_vqt` puts the *other* reader in it -
     * the eight-byte singleton at 0x6402, which has no plane table at +0x08
     * and no row table at +0x18 for the leaf to read. Reserving the frame does
     * not fix that and is not meant to.
     *
     * Nothing exercises either. `VQT` does not occur once in the four shipped
     * `RESOURCE.00*` archives - `BMP:` occurs 61 times and `SCR:` once - so
     * the quadtree form is a code path the release has no data for, and the
     * routine is entered zero times by the intro and by all 28 level
     * snapshots. Boran's reading is that it was broken in the original too and
     * that is why it never shipped. The archives are the measured half of
     * that; the rest is a reading and this comment is not evidence for it.
     */
    /* **The reader record is the guest's, and has to be.** Its address is
       filed into `BITMAPS.reader` for `vqt_node`, `vqt_screen_node` and
       `fill_quadrant` to fetch back out and write through, and a C array has
       no DGROUP address to file. This is `framify.py --in-dgroup`'s shape: the
       original's `sub sp,0x1ca` is reserved and `rd` is a typed pointer into
       it, so the body reads `rd->plane[i].seg` while the bytes stay the
       guest's. The other locals are ordinary C ones - only this slot is
       walled. */
    volatile struct vqt_reader *rd =
        VQTRD(dg_alloca(0x1ca));                      /* [bp-0x1ca] */

    /* [bp-0xa]/[bp-8], the far pointer `huge_add_to` steps. Its comment used
       to say it needed a real DGROUP address; that stopped being true when
       `huge_add_to` took a pointer, and nothing else looks at it. */
    struct far_ptr cur;
    dg_off_t *at = BMPLIST(list);      /* [bp-2]  */
    uint32_t largest = 0;                   /* [bp-0x20] */
    uint32_t free_bytes, file_left;
    uint32_t buffer;                        /* [bp-0x18]/[bp-0x1a] */
    struct far_ptr block = {0, 0};          /* [bp-0xe], [bp-0xc] */
    uint16_t index = 0;                     /* [bp-0x12] */
    uint16_t si;

    while (*at != 0) {
        uint16_t hdr = *at;
        uint32_t need = buffer_size_thunk((uint16_t)BMP(hdr).width,
                                          (uint16_t)BMP(hdr).height)
                        & 0xffffu;

        if (largest < need)
            largest = need;

        at++;
    }

    free_bytes = dos_alloc_bytes(0xffff, 0xffff, 0, 0).bytes;
    file_left = file_record_size(file);
    buffer = free_bytes;

    if (file_left <= free_bytes) {
        buffer = file_left;
        largest = 0;
    }

    if (largest <= buffer) {
        block = dos_alloc_bytes((uint16_t)buffer,
                                (uint16_t)(buffer >> 16), 0, 0).ptr;
        if (far_eq(block, FAR_NULL))
            goto no_block;
        goto have_block;
    }

no_block:
    if (far_eq(DG3576.scratch, FAR_NULL))
        goto done;
    if (largest > 0x3ab4)
        goto done;

    block = DG3576.scratch;
    buffer = 0x3ab4;

have_block:
    BITMAPS.reader = dg_off(dgroup, rd);
    rd->pos = 0;
    rd->data = block;

    read_far(MK_FP(block.seg, block.off), (int32_t)buffer, file);
    file_left -= buffer;

    at = BMPLIST(list);

    while ((si = *at) != 0) {
        uint32_t used;
        /* **Stepped as a pair, on purpose.** `quarter` goes onto the offset
           and the segment stays put, without renormalising - so each of the
           four values is a different `seg:off` for the same linear address
           and all four are *stored*. That is the one shape a host pointer
           cannot carry, which is why this stays two words rather than
           becoming a `MK_FP`. */
        struct far_ptr plane;
        uint16_t row;
        int16_t i;
        uint32_t quarter;

        plane = far_normalise(far_of_rev(BMP(si).data));

        quarter = (uint32_t)(uint16_t)((int16_t)(BMP(si).width
                                                 * BMP(si).height)
                                       >> 2);

        for (i = 0; i < 4; i++) {
            rd->plane[i] = plane;
            plane.off = (uint16_t)(plane.off + quarter);
        }

        row = 0;
        for (i = 0; BMP(si).height > i; i++) {
            rd->row[i] = (int16_t)row;
            row = (uint16_t)(row + BMP(si).width);
        }

        vqt_node(0, 0, (uint16_t)BMP(si).width, (uint16_t)BMP(si).height);

        used = rd->pos;
        used = (uint32_t)long_shift_right((int32_t)(used + 7), 3);

        rd->pos = 0;

        cur = rd->data;

        if (file_left != 0) {
            struct far_ptr p = huge_add(cur, (int32_t)used);
            uint32_t chunk;

            far_copy(MK_FP(cur.seg, cur.off), MK_FP(p.seg, p.off),
                     (uint16_t)((uint16_t)buffer - (uint16_t)used));

            huge_add_to(&cur, (int32_t)(buffer - used));

            chunk = (used >= file_left) ? file_left : used;
            if (chunk > buffer)
                chunk = buffer;

            read_far(MK_FP(cur.seg, cur.off), (int32_t)chunk, file);
            file_left -= chunk;
        } else {
            rd->data = huge_add(cur, (int32_t)used);
        }

        at++;
        index++;
    }

    if (!far_eq(block, DG3576.scratch))
        dos_free_far(block);

done:
    (void)index;
    dg_free(0x1ca);
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
    volatile struct vqt_reader *rd;
    uint16_t code;
    uint32_t pos;

    if ((w | h) == 0)
        return;

    rd = VQTRD(BITMAPS.reader);
    pos = rd->pos;
    rd->pos = pos + 4;

    /* Four bits at `pos`, read as a word so a nibble can straddle a byte. The
       offset is stepped inside the segment, which is why `data` stays a pair. */
    code = (uint16_t)((FARU16(rd->data.seg,
                              (uint16_t)(rd->data.off + (pos >> 3)))
                       >> (pos & 7)) & 0x0f);

    if (code & 8) {
        vqt_screen_node(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
    } else {
        fill_screen_quadrant(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
        redraw_cursor(DG3890.page_front_ptr);
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
 *
 * **Reached only through a "BMP:VQT:" chunk, and the game ships none.** See the
 * count beside `draw_offset_bitmap` at 0x24e9a: zero of the 162 extracted
 * resources carry VQT:, so neither quadtree leaf can be entered by this data.
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
 *
 * **Both ends are `far` pointers and the 64K wrap is given up.** `rep movsw`
 * steps SI and DI as 16-bit registers, so both ends wrap inside their segment
 * on the original; a host pointer cannot, and the `far` tag says which kind
 * this is without giving the host that behaviour. The destination held out
 * longest, as a `seg:off` pair with `(uint16_t)(dst_off + i)` doing the wrap
 * explicitly.
 *
 * What makes giving it up sound is the same measurement that settled it for
 * `far_move` and `far_memcpy`: instrumented on 2026-09-09 across the intro to
 * flip 200 and a full level14 run, neither end ever reached
 * `off + count > 0x10000`. For either to wrap it would have to start above
 * 0xC000 in DGROUP with a large count, and the copy would then run into
 * DGROUP from offset 0, which is a bug rather than a behaviour.
 *
 * Under Borland the tag brings the wrap back, because there `far` pointer
 * arithmetic *is* 16-bit offset arithmetic - so the source says the right
 * thing for both compilers and only the host gives the behaviour up.
 */
void far_copy(uint8_t far *dst, const uint8_t far *src,
              uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        dst[i] = src[i];
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
 * A **** routine, and it shares the epilogue three bytes above its own
 * entry for the early return.
 */
void vqt_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    volatile struct vqt_reader *rd;
    uint16_t code;
    uint32_t pos;

    if ((w | h) == 0)
        return;

    rd = VQTRD(BITMAPS.reader);
    pos = rd->pos;
    rd->pos = pos + 4;

    /* Four bits at `pos`, read as a word so a nibble can straddle a byte. The
       offset is stepped inside the segment, which is why `data` stays a pair. */
    code = (uint16_t)((FARU16(rd->data.seg,
                              (uint16_t)(rd->data.off + (pos >> 3)))
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
 *
 * **Reached only through a "BMP:VQT:" chunk, and the game ships none.** See the
 * count beside `draw_offset_bitmap` at 0x24e9a: zero of the 162 extracted
 * resources carry VQT:, so neither quadtree leaf can be entered by this data.
 */
void fill_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    not_transcribed("0x25eb5, the quadtree leaf");
}

