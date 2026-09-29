/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Loading bitmap lists**: the INF chunk, the list itself, freeing it, and
 * widening a one-bit plane to four.
 *
 * A module of the original's **code segment 1c25**, image 0x234d2..0x23b29,
 * split out of engine.c on 2026-09-27. Its literal pool is DGROUP
 * 0x4966..0x498e: the pad byte at 0x4965 ends the font loader's `_DATA`
 * before it, and files.c's begins after it. Functions are in address order
 * and each carries the image offset it was read from.
 *
 * **Borland C++ 2.0, `-mm -G -O`**, as files.c after it. Two of the image's
 * habits show here too: an assignment's value is tested in the type of what
 * was assigned rather than of the variable, and a `huge` pointer tested with
 * `!` is `or ax,dx` where one compared with 0 calls the runtime's `F_PCMP@`.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -G -O
 * JUDGE: data 0x4966..0x498e
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/* **A far pointer made by adding**: the segment widened to a long, shifted,
   and the offset added - the `cwd` and the `add`/`adc` the image has where
   `free_bitmaps` rebuilds a header's pointer. Ours in name only. */
#  define FAR_FROM_PAIR(seg, off) \
    ((void far *)(((int32_t)(seg) << 16) + (uint16_t)(off)))
#else
#  define FAR_FROM_PAIR(seg, off) MK_FP((seg), (off))
#endif

/*
 * **The scratch block, as this module sees it**: `huge`, where the modules
 * that only test it declare it `far`. `load_bitmap_list` steps it with the
 * runtime's huge add on the variable itself, which is a `huge` lvalue.
 */
#ifdef __TURBOC__
#  define SCRATCH (*(uint8_t huge **)&g_scratch_block)
#else
#  define SCRATCH g_scratch_block
#endif

/*
 * **The handle test that cannot fail.** The handle is an unsigned word
 * compared below 0, `or ax,ax / jae`, so the jump always goes on. The host's
 * handle is a pointer and has no such comparison. Ours.
 */
/*
 * **A far pointer masked as the long it is**: `and dx,0xfff0 / and ax,0xffff`.
 * The pointer has just been through a huge add, so its offset is 0..15 and
 * the mask clears it: the address rounded down to a paragraph. Ours.
 */
#ifdef __TURBOC__
#  define FAR_MASK(p, m) ((uint8_t far *)((uint32_t)(p) & (m)))
#else
/* On the host the linear address is the pointer, and the mask clears its
   low bits the same way. */
#  define FAR_MASK(p, m) \
    ((uint8_t *)((uintptr_t)(p) & ~(uintptr_t)(uint32_t)~(uint32_t)(m)))
#endif

#ifdef __TURBOC__
#  define HANDLE_BELOW_ZERO(h) ((uint16_t)(h) < 0)
#else
#  define HANDLE_BELOW_ZERO(h) ((void)(h), 0)
#endif

/*
 * 0x234d2
 *
 * Read a bitmap's `BMP:INF:` chunk into two allocations: an array of pointers,
 * NUL-terminated, and the ten-byte records it points at. Answers 1, or 0 with
 * everything freed again.
 *
 * The chunk begins with a count, and then two parallel streams of words - a
 * width and a height per record, or so the layout suggests. They are read into
 * one temporary block and threaded into the records afterwards, at +6 and +8.
 *
 * **How many rows are actually there is worked out from the chunk's size**, not
 * taken on trust: the size less the count word has to be at least four bytes
 * per record, and if it is not, only one row is read and every record gets the
 * same pair. That is what the two cursors advancing only when the count matches
 * is doing.
 *
 * The pointer array is `count + 1` pointers from `calloc`, so the
 * terminating null is already there before anything is written.
 *
 * Every failure after the first allocation goes through the same cleanup, which
 * frees the records, the array and the temporary in that order.
 */
uint16_t read_bmp_info(FILE *handle, register int16_t * count_at,
                       struct bitmap *** out)
{
    int16_t *a;
    int16_t *b;
    int16_t i;
    int16_t rows;
    struct bitmap **slot;
    uint8_t *tmp = NULL;
    register struct bitmap *hdr;

    *out = NULL;

    if (seek_named_chunk(handle, "BMP:INF:", 0) == -1L)
        return 0;

    if (game_fread((uint8_t *)count_at, 2, 1, handle) != 1)
        return 0;

    /* The list, and one run of headers for the whole of it: `list[0]` is
       the run's first byte, which is why `free_bitmap_list` gives it back
       as a heap block. */
    if ((*out = (struct bitmap **)calloc_far((*count_at + 1) * sizeof **out, 1)) == NULL)
        goto fail;
    if (((*out)[0] =
             (struct bitmap *)calloc_far(sizeof(struct bitmap), *count_at)) == NULL)
        goto fail;

    /* A width and a height per bitmap if the chunk holds that many, and
       otherwise one pair for all of them. */
    if (file_record_size(handle) - 2 < (int32_t)(*count_at * 4))
        rows = 1;
    else
        rows = *count_at;

    if ((tmp = malloc_far(rows * 4)) == NULL)
        goto fail;
    if (game_fread(tmp, rows * 4, 1, handle) != 1)
        goto fail;

    /* `rows` widths then `rows` heights, straight out of the file. `tmp`
       is a near-heap block, so it is even and a word pointer is safe. */
    a = (int16_t *)tmp;
    b = (int16_t *)tmp + rows;
    hdr = (*out)[0];
    slot = *out;

    for (i = 0; *count_at > i; i++) {
        *slot = hdr;
        hdr->width = *a;
        hdr->height = *b;

        if (*count_at == rows) {
            a++;
            b++;
        }

        hdr++;
        slot++;
    }

    /* The null. With `*count_at` of zero the loop does not run and this
       goes over `list[0]`, as the original's cursor does too. */
    *slot = 0;
    free_far(tmp);
    return 1;

fail:
    if (tmp != NULL)
        free_far(tmp);

    if (*out != NULL) {
        if ((*out)[0] != 0)
            free_far((uint8_t *)(*out)[0]);
        free_far((uint8_t *)*out);
    }

    return 0;
}

/*
 * 0x2367c
 *
 * Load a bitmap list. Takes a resource name or an open file record, answers the
 * list it built, and gives every block back on any failure.
 *
 * The shape is: read the header - the number of bitmaps and the list of headers
 * - ask the driver how much room the planar form needs, take that from DOS,
 * read the "BMP:BIN:" chunk into it, and hand the whole thing to
 * `vm_load_bitmap_list` to convert in place. Then, if DGROUP 0x38af is set,
 * look for a "BMP:VGA:" or "BMP:AMG:" chunk and read *that* through the driver
 * as well - 5 for VGA, 6 for Amiga, and the Amiga one is expanded from one bit
 * per pixel to four first, in place and backwards.
 *
 * Three things in it are worth naming:
 *
 *   **A dead branch.** After opening the record it tests `or ax,ax` and then
 *   `jae`, and `or` always clears carry, so the failure jump is never taken. It
 *   is the same slip as the alignment step in `far_memcpy`, and it is
 *   transcribed as it behaves.
 *
 *   **A scratch block that is allocated to be freed.** If DGROUP 0x3576 is null
 *   it asks the near heap for 0x3cc4 bytes, frees them at once, and then asks
 *   for 0x3ac4 - which is how a program of this era makes sure the smaller
 *   block lands at the top of the largest hole. The pointer it keeps is then
 *   pushed up to the next paragraph boundary.
 *
 *   **A retry loop that halves.** The second read's buffer starts at 0x7fff
 *   bytes and the request is halved until DOS can satisfy it, so a machine with
 *   less memory reads in smaller pieces rather than failing.
 *
 * The driver call at vector 0x4382 is `vm_nothing` on this adapter, and nine
 * words are pushed at 0x4382 and 0x437e where five are read.
 */
struct bitmap **load_bitmap_list(char *name)
{
    struct bitmap **list;
    uint8_t huge *blk;
    uint8_t huge *walk;
    uint8_t huge *tmp;
    uint8_t *scratch;
    int16_t count;
    int16_t got;
    int16_t size;
    int16_t opened;
    int16_t kind;
    int32_t want;
    register FILE *si = (FILE *)name;
    register int16_t res;

    kind = 0;
    opened = 0;
    list = NULL;
    tmp = blk = 0;
    scratch = NULL;
    res = 0;

    /* A handle, or a name to open. */
    if (file_record_valid(si) == 0) {
        opened = 1;
        if (HANDLE_BELOW_ZERO(si = open_file_record((char *)si)))
            goto done;
    }

    if (read_bmp_info(si, &count, &list) == 0)
        goto done;

    want = ((vm_list_size_fn)VM_DRIVER.entry[13])(list, (uint8_t *)&size);

    if (!(blk = DOS_ALLOC_PTR(DOS_ALLOC(want, 0))))
        goto done;
    if (size != 0 && !(tmp = DOS_ALLOC_PTR(DOS_ALLOC((int32_t)size, 0))))
        goto done;

    /* A paragraph-aligned scratch block from the near heap, sixteen bytes
       into what it answered, if nothing has one yet. */
    if (g_scratch_block == NULL) {
        if ((scratch = malloc_far(0x3cc4)) != NULL) {
            free_far(scratch);
            if ((scratch = malloc_far(0x3ac4)) != NULL) {
                g_scratch_block = (uint8_t far *)NEAR_ZERO(scratch);
                SCRATCH += 0x10;
                g_scratch_block = normalise_pointer_far(
                    FAR_MASK(g_scratch_block, 0xfffffff0L));
            }
        }
    }

    if (seek_named_chunk(si, "BMP:BIN:", 0) == -1L)
        goto done;
    if ((res = open_resource(0, si, "r", file_record_size(si))) < 0)
        goto done;

    walk = blk;
    while (read_resource(res, walk, 0x7fff) == 0x7fff)
        walk += 0x7fff;

    ((vm_load_list_fn)VM_DRIVER.entry[14])(list, blk, resource_size(res), tmp, want);

    close_resource(res);
    kind = 1;

    if (VMDS.vga_chunks == 0)
        goto done;

    if (seek_named_chunk(si, "BMP:VGA:", 0) != -1L)
        kind = 5;
    else if (seek_named_chunk(si, "BMP:AMG:", 0) != -1L)
        kind = 6;

    if (kind < 5)
        goto done;
    if ((res = open_resource(0, si, "r", file_record_size(si))) < 0)
        goto done;

    /* Halve the request until DOS grants one; a signed shift of the long. */
    for (want = 0x7fffL; !(tmp = DOS_ALLOC_PTR(DOS_ALLOC(want, 0))); want >>= 1)
        ;

    walk = blk;
    while ((got = read_resource(res, tmp, (uint16_t)want)) > 0) {
        if (kind == 6) {
            expand_1bpp_to_4bpp(tmp, tmp, got);
            got <<= 2;
        }

        ((vm_chunk_fn)VM_DRIVER.entry[15])(tmp, walk, got);
        walk += want << 1;
    }

    close_resource(res);

done:
    if (tmp != NULL)
        dos_free_far(tmp);

    if (scratch != NULL) {
        free_far(scratch);
        g_scratch_block = 0;
    }

    if (kind == 0) {
        if (blk != NULL)
            dos_free_far(blk);
        if (res != 0)
            close_resource(res);
        free_bitmap_list(list);
        list = NULL;
    }

    if (opened != 0)
        close_file_record(si);

#ifdef __TURBOC__
    return (struct bitmap **)list;
#else
    return list != NULL ? (struct bitmap **)list : NULL;
#endif
}

/*
 * 0x23a18
 *
 * Give back a bitmap list: the block its first word points at, and then the
 * list itself.
 *
 * **The first read is through an unchecked pointer, and that is the
 * original.** 0x23a1f is `cmp word ptr [si], 0` before 0x23a2d tests `si`
 * itself, so the list is dereferenced before it is known to be there, and a
 * null list reads DGROUP:0000 - `NEAR_ZERO` gives the host the same read.
 */
void free_bitmap_list(struct bitmap ** list)
{
    if (NEAR_ZERO(list)[0] != 0)
        free_far((uint8_t *)list[0]);

    if (list != NULL)
        free_far((uint8_t *)list);
}

/*
 * 0x23a3c
 *
 * Give back everything a bitmap list owns: the block its first header points
 * at, and then the list itself through `free_bitmap_list`.
 *
 * **It does not walk the list, and it does not need to.** 0x23a48 is
 * `mov di, [si]` - the first entry and no other, with no loop anywhere in the
 * routine - because a whole list is *three* allocations rather than two per
 * bitmap, and this pair of routines gives back exactly those three:
 *
 *   the list      `read_bmp_info`: `calloc_far((count + 1) * 2, 1)`,
 *                 count words and a null - freed by `free_bitmap_list`
 *   the headers   `read_bmp_info`: `calloc_far(0xa, count)`, one run of
 *                 `struct bitmap`, and `list[0]` is its first byte - which is
 *                 why `free_bitmap_list` frees `list[0]` as a heap block
 *   the pixels    one `dos_alloc_bytes` for every bitmap in the list, and the
 *                 first header's `data` is its base - which is what this
 *                 routine frees
 *
 * All three loaders agree about that last one: `vm_load_bitmap_list` starts at
 * the block it was handed and steps `off` per bitmap, so header 0 keeps the
 * base; `decode_vqt_list`'s branch of `load_bitmaps` sets `fp2 = block` before
 * its loop. The "BMP:OFF:" branch is the one that would not hold - header 0's
 * data is `block + offsets[0]` out of the file - and it is unreachable with
 * this game's data, for the reasons counted beside `draw_offset_bitmap`.
 *
 * The far pointer is read out of the header the way `vm_load_bitmap_list` wrote
 * it - segment at +0 and offset at +2 - and the `cwd` and `adc` around that read
 * are a 32-bit expression the compiler emitted and then had no use for: `cwd`
 * sets DX and the next instruction clears it, and the `adc` adds a carry that
 * `add dx, [di+2]` cannot produce. Transcribed as the two words it reads.
 */
void free_bitmaps(register struct bitmap ** list)
{
    if (list != NULL) {
        register struct bitmap *hdr = list[0];

        dos_free_far(FAR_FROM_PAIR((dg_sseg_t)hdr->data_seg, hdr->data_off));
        free_bitmap_list(list);
    }
}

/*
 * 0x23a6a
 *
 * How many entries a null-terminated list of near pointers has. A null list is
 * zero rather than a fault.
 */
uint16_t count_list_entries(struct bitmap ** list)
{
    uint16_t n = 0;

    if (list != NULL) {
        while (list[n] != 0)
            n++;
    }

    return n;
}

/*
 * 0x23a8a
 *
 * Expand one bit per pixel into four, **backwards**, so the source and the
 * destination may be the same block: a set bit becomes colour 1 and a clear one
 * colour 0. Two source bits share a destination byte - the even bit in the low
 * nibble and the odd one in the high - so `count` source bytes make `count * 4`
 * destination bytes, which is why both pointers are first walked to their last
 * byte and then stepped down.
 *
 * The mask that separates the two cases is `test si, 0xaa`: `si` walks 1, 2, 4
 * ... 0x80, and the bits of 0xaa are the odd positions. The odd one *ors* its
 * 0x10 into the byte the even one wrote and then moves the pointer; the even
 * one *assigns*, which is what clears whatever was in that byte before.
 *
 * The source byte is sign-extended before the test - `cbw` - which cannot
 * matter while `si` stays under 0x100, and is transcribed rather than tidied
 * away.
 *
 * Both far pointers live in the caller's argument slots and are walked in
 * place: `huge_add_to` takes the address *of* the pointer.
 */
void expand_1bpp_to_4bpp(const uint8_t huge * src, uint8_t huge * dst,
                         uint16_t count)
{
    int8_t byte;
    register int16_t si;

    /* Both walk down from their last byte, stepped as huge pointers. */
    src += count - 1;
    dst += count * 4 - 1;

    while (count-- != 0) {
        byte = *src;
        src--;

        for (si = 1; (si & 0xff) != 0; si <<= 1) {
            if ((si & 0xaa) != 0) {
                *dst |= (si & byte) ? 0x10 : 0x00;
                dst--;
            } else {
                *dst = (si & byte) ? 0x01 : 0x00;
            }
        }
    }
}
