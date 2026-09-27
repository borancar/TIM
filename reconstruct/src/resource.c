/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The resource streams**: a hundred slots, each a record naming where a
 * resource's compressed bytes are - a file, or a block in memory - and the
 * reader that hands them to a decoder and the decoded bytes to the caller.
 * The stored and run-length decoders are here; the LZW and LZSS ones, and
 * the writing side of all four, are the modules after it, which reach this
 * one's globals and are reached through its handler table.
 *
 * One module of the original's **code segment 1c25**, image
 * 0x1c251..0x1ca46: its routines are `near`, reached with a bare `call`,
 * and the runtime's long and huge helpers are far calls. Its `_DATA` is
 * 0x3576..0x35b2 and its `_BSS` 0x5788..0x58b8. The front is the byte after
 * vgadac.c's last `retf`; the end is where the LZW decoder's assembly
 * begins.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **The shared scratch block**, DGROUP 0x3576: a far block the picker and
 * the bitmap loader may leave here, which a resource slot borrows rather
 * than allocating one of its own.
 */
struct dg_3576 DG3576 DGROUP_WAS(0x3576) = { 0 };

/*
 * **The four resource handlers**, at DGROUP 0x357a, fourteen bytes apiece
 * in the image: the sizes of the near work buffer and of the far scratch
 * block for reading and otherwise, and four near routines - the decoder,
 * the writing side's flush and open, and the reset a stream is started
 * with. A null is a type without one. Type 0 is stored, 1 run-length, 2
 * LZW and 3 LZSS.
 *
 * `lzw_reset` answers nothing; the table holds it as the others are held,
 * and nothing reads what the call answers.
 */
struct engine_res_handlers ENGINE_RES_HANDLERS DGROUP_WAS(0x357a) = {
    {
        { 0x0080, 0x0000, 0x0000, decompress_store, store_flush, 0, 0 },
        { 0x0080, 0x0000, 0x0000, decompress_rle, rle_flush, 0, 0 },
        { 0x0080, 0x3ab3, 0x7566, decompress_lzw, lzw_flush,
          lzw_open_write, (res_fn)lzw_reset },
        { 0x0080, 0x2163, 0x2163, decompress_lzss, lzss_flush,
          lzss_open_write, lzss_reset },
    }
};

/*
 * **This module's `_BSS`, 0x5788..0x58b8.** Borland lays `_BSS` out in
 * reverse order of first mention; dgroup.h mentions the four from the
 * highest down, and they are defined here in the same order.
 */
struct engine_stream ENGINE_STREAM DGROUP_WAS(0x5888);
struct engine_resource_slots ENGINE_RESOURCE_SLOTS DGROUP_WAS(0x57c0);
struct engine_resource_flags ENGINE_RESOURCE_FLAGS DGROUP_WAS(0x57ba);
struct engine_read_staging ENGINE_READ_STAGING DGROUP_BSS(0x5788);

/*
 * 0x1c251
 *
 * Decompression type 0: **stored**. Each byte of the input goes straight to
 * the output until the input ends or the request is filled - the handler
 * the table at DGROUP 0x3580 names first, and nothing selects it: every
 * resource in the game is compressed.
 */
int16_t near decompress_store(void)
{
    register int16_t more;
    register int16_t c;

    more = 1;
    while (more != 0 && (c = next_input_byte()) != -1)
        more = emit_byte(c);
    return 0;
}

/*
 * 0x1c278
 *
 * Decompression type 1: plain run-length coding.
 *
 * Each token is one byte. Bit 7 clear means the low seven bits are a count of
 * literal bytes to copy; bit 7 set means they are a count and the **next** byte
 * is the value to repeat. A token of -1 - the end of the input - stops it, and
 * so does either emitter answering 0, which is how the output side says the
 * caller's request has been filled.
 *
 * Bit 0x20 of `ENGINE_RESOURCE_FLAGS.flags` - a resource read from a file
 * rather than memory - clear, a resource in memory, hands the whole job to
 * the assembly at 0x1cd2c instead, and answers
 * what that does.
 */
int16_t near decompress_rle(void)
{
    int16_t token;
    int16_t more;

    more = 1;
    if (!(ENGINE_RESOURCE_FLAGS.flags & 0x20))
        return rle_from_memory();

    while (more != 0 && (token = next_input_byte()) != -1) {
        if (token & 0x80)
            more = emit_fill_run(next_input_byte(), token & 0x7f);
        else
            more = emit_literal_run(token & 0x7f);
    }
    return 0;
}

/*
 * 0x1c2cc
 *
 * **Drain the spill ring to the output**, a byte at a time through
 * `put_output_byte`: from the record's `spill_start` round to its
 * `spill_end`, the index wrapping at 0x80. Type 0's flush in the handler
 * table - the writing side, which nothing reaches.
 */
int16_t near store_flush(void)
{
    uint16_t i;
    register uint8_t *buf;

    i = ENGINE_STREAM.rec->spill_start;
    buf = ENGINE_STREAM.spill;
    while (ENGINE_STREAM.rec->spill_end != i) {
        put_output_byte(buf[i++]);
        i &= 0x7f;
    }
    ENGINE_STREAM.rec->spill_start = i;
    return 0;
}

/*
 * 0x1c319
 *
 * Copy `count` bytes out of the current resource into a huge pointer, through
 * `ENGINE_READ_STAGING`, the 0x32-byte staging buffer at DGROUP 0x5788.
 *
 * The buffer is why this is a loop at all: `game_fread` reads into DGROUP, and
 * the destination is a huge pointer that may be anywhere, so each pass reads at
 * most 0x32 bytes and then `far_memcpy`s them out. The destination is the
 * caller's copy, stepped in place.
 *
 * The loop ends on a short read as well as on the count running out, and the
 * answer is 0 either way: nothing here reports how much it managed.
 */
int16_t near read_into_huge(uint8_t huge *dst, uint16_t count)
{
    uint16_t n;
    int16_t got;

    got = 1;
    while (count > 0 && got > 0) {
        n = count > 0x32 ? 0x32 : count;
        count -= got = game_fread(ENGINE_READ_STAGING.buf, 1, n,
                                  ENGINE_RESOURCE_FLAGS.file);
        far_memcpy(dst, ENGINE_READ_STAGING.buf, got);
        dst += got;
    }
    return 0;
}

/*
 * 0x1c389
 *
 * The next byte of whatever is being decompressed, or -1 at the end.
 *
 * There are two sources and the bit 0x20 at DGROUP 0x5888 chooses between them:
 * the resource file through `game_fgetc`, or a block already in memory, walked
 * by the huge pointer at DGROUP 0x5898.
 *
 * Either way the position at +0xa:+0xc of the record is stepped first, and
 * the end test compares it against +0xe:+0x10 - so the count is kept by the
 * record and not by the source.
 *
 * The byte is zero-extended: `cbw` then `and ax,0xff`, which is the compiler
 * widening a `char` and then masking the sign back off.
 */
int16_t near next_input_byte(void)
{
    if (ENGINE_STREAM.rec->in == ENGINE_STREAM.rec->end)
        return -1;
    ENGINE_STREAM.rec->in++;
    if (ENGINE_STREAM.kind & 0x20)
        return game_fgetc(ENGINE_RESOURCE_FLAGS.file);
    return *ENGINE_STREAM.in++ & 0xff;
}

/*
 * 0x1c3e6
 *
 * Read up to `count` bytes of the compressed stream into DGROUP, and answer how
 * many. This is what fills the bit buffer the LZW code reader works out of.
 *
 * What is left is `end - in` on the record, one signed 32-bit subtract, and
 * the request is cut down to it: `count` beside a `long` is zero-extended and
 * the compare is signed on the high word, unsigned on the low.
 *
 * The position advances by what will be taken **before** anything is taken,
 * and then bit 0x20 of the stream's kind chooses between the file and a block
 * already in memory.
 */
int16_t near read_input_block(uint8_t *dst, uint16_t count)
{
    int32_t rem;

    if ((rem = ENGINE_STREAM.rec->end - ENGINE_STREAM.rec->in) == 0)
        return 0;
    rem = count > rem ? rem : count;
    ENGINE_STREAM.rec->in += rem;
    if (ENGINE_STREAM.kind & 0x20)
        return game_fread(dst, 1, (uint16_t)rem, ENGINE_RESOURCE_FLAGS.file);
    far_memcpy(dst, (uint8_t huge *)ENGINE_STREAM.in, (uint16_t)rem);
    ENGINE_STREAM.in += rem;
    return (int16_t)rem;
}

/*
 * 0x1c493
 *
 * Deliver a run of `n` literal bytes to the output.
 *
 * The output has two states and `ENGINE_STREAM.wanted` - what the caller of
 * `resource_read` still wants - decides between them. While the run fits,
 * the bytes go to the destination huge pointer, which is then stepped, and
 * the answer is 1 meaning "keep going". Once it does not fit they spill into
 * the record's small buffer, counted by its `spill_end`, and the answer is 0.
 *
 * The input position advances by `n` **before** either, so it counts what was
 * consumed rather than what was delivered.
 *
 * Bit 0x40 of the reader's flags is what makes the write happen at all;
 * without it the bytes are skipped in the file instead, by seeking forward
 * over them.
 *
 * The spill writes at the **start** of the buffer, not at the count it has
 * just increased - unlike `emit_fill_run`, which offsets by the old count.
 */
int16_t near emit_literal_run(uint16_t n)
{
    ENGINE_STREAM.rec->in += n;
    if (ENGINE_STREAM.wanted >= n) {
        if (ENGINE_RESOURCE_FLAGS.flags & 0x40)
            read_into_huge(ENGINE_STREAM.out, n);
        else
            game_fseek(ENGINE_RESOURCE_FLAGS.file, (uint32_t)n, 1);
        ENGINE_STREAM.wanted -= n;
        ENGINE_STREAM.out += n;
        return 1;
    } else {
        ENGINE_STREAM.rec->spill_end += n;
        read_into_huge(ENGINE_STREAM.spill, n);
        return 0;
    }
}

/*
 * 0x1c51e
 *
 * Deliver a run of `n` copies of one byte - the other half of the run-length
 * pair, and the same two output states as `emit_literal_run`, answering the
 * same 1 or 0.
 *
 * Nothing is read here, so nothing advances the input position: that was done
 * by the two `next_input_byte` calls the caller made to get the length and the
 * value. The spill offsets by the record's `spill_end` **before** adding to
 * it, which `emit_literal_run`'s does not do.
 */
int16_t near emit_fill_run(uint16_t value, int16_t n)
{
    if (ENGINE_STREAM.wanted >= n) {
        if (ENGINE_RESOURCE_FLAGS.flags & 0x40)
            far_memset(ENGINE_STREAM.out, value, (int32_t)n);
        ENGINE_STREAM.wanted -= n;
        ENGINE_STREAM.out += n;
        return 1;
    } else {
        far_memset(ENGINE_STREAM.spill + ENGINE_STREAM.rec->spill_end,
                   value, (int32_t)n);
        ENGINE_STREAM.rec->spill_end += n;
        return 0;
    }
}

/*
 * 0x1c5a3
 *
 * Deliver one byte - `emit_literal_run` and `emit_fill_run` written for a run
 * of exactly one, with the same two states and the same answers. The spill
 * reads the record's `spill_end` and increments it in one instruction, so the
 * byte lands at the old count.
 */
int16_t near emit_byte(uint16_t value)
{
    if (ENGINE_STREAM.wanted >= 1) {
        if (ENGINE_RESOURCE_FLAGS.flags & 0x40)
            *ENGINE_STREAM.out = value;
        ENGINE_STREAM.out++;
        ENGINE_STREAM.wanted--;
        return 1;
    } else {
        ENGINE_STREAM.spill[ENGINE_STREAM.rec->spill_end++] = value;
        return 0;
    }
}

/*
 * 0x1c5f5
 *
 * **Write one byte of a resource being written**: counted at DGROUP 0x589c,
 * then to the file with `game_fputc`, or into memory at the record's data
 * plus its position, which steps. The writing side's byte sink, which
 * nothing reaches.
 */
int16_t near put_output_byte(int16_t c)
{
    ENGINE_STREAM.written++;
    if (ENGINE_STREAM.kind & 0x20)
        return game_fputc(c, ENGINE_RESOURCE_FLAGS.file);
    else
        return ENGINE_STREAM.rec->data.ptr[ENGINE_STREAM.rec->in++] = c;
}

/*
 * 0x1c649
 *
 * Select a resource by handle and unpack its record into the globals the
 * rest of the loader reads. A handle outside 0..0x63, signed, or naming an
 * empty slot answers 0.
 *
 * The record's `kind` is both a flag set and a small number: the whole byte
 * goes to the stream, its low five bits choose the handler, and bit 0x20
 * selects between two ways of finding the data - a file, whose record is
 * kept, or memory, the data pointer plus the position added as a huge
 * pointer and normalised.
 */
int16_t near select_resource(int16_t handle)
{
    if (handle < 0 || handle >= 0x64
        || (ENGINE_STREAM.rec = ENGINE_RESOURCE_SLOTS.slot[handle]) == NULL)
        return 0;

    ENGINE_STREAM.scratch = ENGINE_STREAM.rec->scratch;
    ENGINE_STREAM.spill = ENGINE_STREAM.rec->work;
    ENGINE_RESOURCE_FLAGS.handler =
        (ENGINE_STREAM.kind = ENGINE_STREAM.rec->kind) & 0x1f;
    if (ENGINE_STREAM.kind & 0x20) {
        ENGINE_RESOURCE_FLAGS.file = ENGINE_STREAM.rec->data.file;
        ENGINE_RESOURCE_FLAGS.flags = 0x20;
    } else {
        ENGINE_RESOURCE_FLAGS.flags = 0;
        ENGINE_STREAM.in = (char huge *)normalise_far_ptr_far(
            (uint8_t huge *)(ENGINE_STREAM.rec->data.ptr + ENGINE_STREAM.rec->in));
    }
    return 1;
}

/*
 * 0x1c6e3
 *
 * Does this NUL-terminated string contain the letter `r`? The mode string
 * of an open, asking whether the resource is to be read.
 */
int16_t near string_contains_r(const char *s)
{
    while (*s)
        if (*s++ == 'r')
            return 1;
    return 0;
}

/*
 * 0x1c705
 *
 * Free a pointer unless it is null - the whole routine.
 */
void near free_if_set(void *p)
{
    if (p)
        heap_free_far(p);
}

/*
 * 0x1c71a
 *
 * Close a resource slot: give back everything it holds and clear its entry.
 * Always answers -1.
 *
 * The record's work buffer is `calloc`ed and goes through `free_if_set`. Its
 * scratch block is from DOS, and is only freed when there is **no** shared
 * block at DGROUP 0x3576 - when there is, the record was pointed at it rather
 * than given one of its own, and freeing it would take the shared one away.
 * The record itself is freed last, and the slot is zeroed whether or not
 * there was anything in it.
 */
int16_t near close_resource_slot(int16_t slot)
{
    if ((ENGINE_STREAM.rec = ENGINE_RESOURCE_SLOTS.slot[slot]) != NULL) {
        free_if_set(ENGINE_STREAM.rec->work);
        if (ENGINE_STREAM.rec->scratch != NULL && !DG3576.scratch)
            dos_free_far(ENGINE_STREAM.rec->scratch);
    }
    free_if_set(ENGINE_STREAM.rec);
    ENGINE_RESOURCE_SLOTS.slot[slot] = NULL;
    return -1;
}

/*
 * 0x1c783
 *
 * Take a resource slot. Answers its number, or -1 when all hundred are in use
 * or the record cannot be allocated. The record is 0x21 bytes from `calloc`,
 * so it starts cleared - which matters, because `close_resource_slot` frees
 * whatever pointers it finds in it.
 */
int16_t near open_resource_slot(void)
{
    int16_t i;

    for (i = 0; i < 0x64; i++)
        if (ENGINE_RESOURCE_SLOTS.slot[i] == NULL)
            break;
    if (i == 0x64)
        return -1;
    if ((ENGINE_STREAM.rec = (struct resource *)heap_calloc_far(1, sizeof(struct resource))) == NULL)
        return -1;
    ENGINE_RESOURCE_SLOTS.slot[i] = ENGINE_STREAM.rec;
    return i;
}

/*
 * 0x1c7d5
 *
 * Give a slot the working memory its decompression type needs. Answers 0, or
 * -1 for a type above 3 or an allocation that failed.
 *
 * Each handler holds two pairs of sizes, and `string_contains_r` on the mode
 * chooses between them: reading takes the near size and the far size for
 * reading, anything else a near size of 0x80 and the other far size. The near
 * part is `calloc`ed. The far part is only allocated when there is no shared
 * block at DGROUP 0x3576; when there is, the record is pointed at that one
 * instead, which is the arrangement `close_resource_slot` has to know about.
 */
int16_t near prepare_resource_slot(int16_t type, char *mode)
{
    uint16_t near_size;
    struct res_handler *h;
    uint16_t far_size;

    if (type > 3)
        return -1;
    h = &ENGINE_RES_HANDLERS.type[type];
    near_size = 0x80;
    if (string_contains_r(mode)) {
        near_size = h->near_size;
        far_size = h->far_size_read;
    } else
        far_size = h->far_size;

    if ((ENGINE_STREAM.rec->work = (uint8_t *)heap_calloc_far(1, near_size)) == NULL)
        return -1;
    if (far_size) {
        /* Compared as the huge pointer the original held it as. */
        if ((uint8_t huge *)DG3576.scratch != NULL)
            ENGINE_STREAM.scratch = ENGINE_STREAM.rec->scratch = DG3576.scratch;
        else
            ENGINE_STREAM.scratch = ENGINE_STREAM.rec->scratch =
                DOS_ALLOC_PTR(DOS_ALLOC((uint32_t)far_size, 0));
        if (!ENGINE_STREAM.rec->scratch)
            return -1;
    }
    ENGINE_STREAM.rec->kind = type;
    return 0;
}

/*
 * 0x1c8a7
 *
 * Hand over the next run of bytes from the spill buffer, up to whatever the
 * caller still wants.
 *
 * The record's `spill_end` and `spill_start` are an end and a start, and
 * their difference is what is available. If that is more than is wanted, only
 * that much is taken and the start is advanced - by the **low byte** of the
 * count, because the start is a byte and the count is a word. Otherwise the
 * buffer is drained and both bytes are zeroed.
 *
 * The copy happens only with bit 0x40 of the reader's flags. Without it the
 * counters still move, so a caller can walk a resource without reading it -
 * which is how a seek is done here.
 */
void near resource_advance(void)
{
    uint16_t n;
    uint16_t start;

    start = ENGINE_STREAM.rec->spill_start;
    n = ENGINE_STREAM.rec->spill_end - start;
    if (n > ENGINE_STREAM.wanted)
        ENGINE_STREAM.rec->spill_start += n = ENGINE_STREAM.wanted;
    else
        ENGINE_STREAM.rec->spill_start = ENGINE_STREAM.rec->spill_end = 0;
    if (n != 0) {
        if (ENGINE_RESOURCE_FLAGS.flags & 0x40)
            far_memcpy(ENGINE_STREAM.out, ENGINE_STREAM.spill + start, n);
        ENGINE_STREAM.wanted -= n;
        ENGINE_STREAM.out += n;
    }
}

/*
 * 0x1c92b
 *
 * Read `count` bytes of the selected resource: first whatever the spill
 * buffer holds, then the handler's decoder, then the spill again for what the
 * decoder left there. Answers how many were delivered, and adds them to the
 * record's `pos`.
 *
 * The first argument is not read.
 */
int16_t near resource_read(FILE *handle, uint16_t count)
{
    ENGINE_STREAM.wanted = count;
    resource_advance();
    if (ENGINE_STREAM.wanted != 0) {
        ENGINE_RES_HANDLERS.type[ENGINE_RESOURCE_FLAGS.handler].read();
        if (ENGINE_STREAM.wanted != 0)
            resource_advance();
    }
    count -= ENGINE_STREAM.wanted;
    ENGINE_STREAM.rec->pos += count;
    return count;
}

/*
 * 0x1c970
 *
 * Reset the LZW state for a new stream: the whole 0x3aa1-byte block cleared,
 * the code width back to nine with its limit at 0x1ff, the first 0x100 codes
 * made into single-byte strings - prefix zero, suffix the code itself - and
 * the next free code set to 0x101, one past the clear code.
 *
 * Every one of those writes goes through the huge-pointer add, because the
 * block is far and the tables run past a segment: the prefixes at twice the
 * code and the suffixes at 0x2720 plus it. The string stack is set to 0x3720
 * into the block, which is where `decompress_lzw` builds each decoded string.
 */
void near lzw_reset(void)
{
    /* Declared and never used, which is still a pushed SI. */
    register int16_t unused;
    int16_t i;

    (void)unused;
    far_memset(ENGINE_STREAM.scratch, 0, 0x3aa1L);
    ENGINE_STREAM.maxcode = (1 << (ENGINE_STREAM.n_bits = 9)) - 1;
    for (i = 0xff; i >= 0; i--) {
        ((int16_t huge *)ENGINE_STREAM.scratch)[i] = 0;
        *(ENGINE_STREAM.scratch + i + 0x2720L) = i & 0xff;
    }
    ENGINE_STREAM.free_ent = 0x101;
    ENGINE_STREAM.clear_flg = 0;
    ENGINE_STREAM.first_code = 1;
    ENGINE_STREAM.resume = 0;
    ENGINE_STREAM.bit_pos = 0;
    ENGINE_STREAM.bit_end = 0;
    ENGINE_STREAM.de_stack = ENGINE_STREAM.scratch + 0x3720L;
}

/*
 * 0x1ca3c
 *
 * An empty routine: `push bp / mov bp,sp / pop bp / ret`, and nothing calls
 * it.
 */
void near resource_nothing_1(void)
{
}

/*
 * 0x1ca41
 *
 * The same, again.
 */
void near resource_nothing_2(void)
{
}
