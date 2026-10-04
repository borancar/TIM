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
 * 0x1ee50..0x1f5eb: its routines are `near`, reached with a bare `call`,
 * and the runtime's long and huge helpers are far calls. Its `_DATA` is
 * 0x31a0..0x31dc and its `_BSS` 0x53a2..0x54ce. The front is the byte after
 * vgadac.c's last `retf`; the end is where the LZW decoder's assembly
 * begins. 1.11's is Borland C++ 3.0 without `-O` where 1.00's was 2.0, and
 * `emit_byte` steps its huge cursor in inline `asm` the compiler assembled
 * itself: compiled through TASM, three other routines come
 * out with jumps and an `xchg` the image does not have.
 *
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The shared scratch block**, DGROUP 0x3576: a far block the picker and
 * the bitmap loader may leave here, which a resource slot borrows rather
 * than allocating one of its own.
 */
uint8_t far *g_scratch_block = 0;

/*
 * **The four resource handlers**, at DGROUP 0x31a4, fourteen bytes apiece
 * in the image: the sizes of the near work buffer and of the far scratch
 * block for reading and otherwise, and four near routines - the decoder,
 * the writing side's flush and open, and the reset a stream is started
 * with. A null is a type without one. Type 0 is stored, 1 run-length, 2
 * LZW and 3 LZSS. 1.11 has no writing side: the flushes and opens that
 * 1.00 named here are nulls, and so is every size for writing.
 *
 * `lzw_reset` answers nothing; the table holds it as the others are held,
 * and nothing reads what the call answers.
 */
struct engine_res_handlers g_engine_res_handlers = {
    {
        { 0x0080, 0x0000, 0x0000, decompress_store, 0, 0, 0 },
        { 0x0080, 0x0000, 0x0000, decompress_rle, 0, 0, 0 },
        { 0x0080, 0x3ab3, 0x0000, decompress_lzw, 0, 0, (res_fn)lzw_reset },
        { 0x0080, 0x2164, 0x0000, decompress_lzss, 0, 0, lzss_reset },
    }
};

/*
 * **This module's `_BSS`, 0x53a2..0x54ce.** Borland lays `_BSS` out in
 * reverse order of first mention; dgroup.h mentions them from the highest
 * down, and they are defined here in the same order.
 */
int16_t   g_lzw_n_bits;
int16_t   g_lzw_maxcode;
int16_t   g_lzw_free_ent;
int16_t   g_lzw_clear_flg;
int16_t   g_lzw_finchar;
int16_t   g_lzw_oldcode;
int16_t   g_lzw_incode;
uint8_t huge *g_lzw_de_stack;
int16_t   g_lzw_bit_pos;
int16_t   g_lzw_bit_end;
uint8_t   g_lzw_first_code;
uint8_t   g_lzw_resume;
uint8_t   g_pad_54b5;
struct resource *g_stream_rec;
uint8_t  *g_stream_spill;
uint8_t   g_resource_handler;
FILE     *g_resource_file;
uint8_t huge *g_stream_out;
uint8_t huge *g_stream_scratch;
struct resource *g_resource_slots[0x64];
int16_t   g_stream_written;
uint16_t  g_stream_wanted;
uint8_t   g_resource_flags;
uint8_t   g_stream_kind;
char huge *g_stream_in;
struct engine_read_staging g_engine_read_staging;

/*
 * 0x1ee50
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
 * 0x1ee77
 *
 * Decompression type 1: plain run-length coding.
 *
 * Each token is one byte. Bit 7 clear means the low seven bits are a count of
 * literal bytes to copy; bit 7 set means they are a count and the **next** byte
 * is the value to repeat. A token of -1 - the end of the input - stops it, and
 * so does either emitter answering 0, which is how the output side says the
 * caller's request has been filled.
 *
 * Bit 0x20 of `g_resource_flags` - a resource read from a file
 * rather than memory - clear, a resource in memory, hands the whole job to
 * the assembly at 0x1f8d1, `rle_from_memory`, instead, and answers
 * what that does.
 */
int16_t near decompress_rle(void)
{
    int16_t token;
    int16_t more;

    more = 1;
    if (!(g_resource_flags & 0x20))
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
 * 0x1eec9
 *
 * Copy `count` bytes out of the current resource into a huge pointer, through
 * `g_engine_read_staging`, the 0x32-byte staging buffer at DGROUP 0x53a2.
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
        count -= got = game_fread(g_engine_read_staging.buf, 1, n,
                                  g_resource_file);
        far_memcpy(dst, g_engine_read_staging.buf, got);
        dst += got;
    }
    return 0;
}

/*
 * 0x1ef3a
 *
 * The next byte of whatever is being decompressed, or -1 at the end.
 *
 * There are two sources and bit 0x20 of `g_stream_kind` chooses between
 * them: the resource file through `game_fgetc`, or a block already in
 * memory, walked by the huge pointer `g_stream_in`.
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
    if (g_stream_rec->in_pos == g_stream_rec->in_end)
        return -1;
    g_stream_rec->in_pos++;
    if (g_stream_kind & 0x20)
        return game_fgetc(g_resource_file);
    return *g_stream_in++ & 0xff;
}

/*
 * 0x1ef97
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

    if ((rem = g_stream_rec->in_end - g_stream_rec->in_pos) == 0)
        return 0;
    rem = count > rem ? rem : count;
    g_stream_rec->in_pos += rem;
    if (g_stream_kind & 0x20)
        return game_fread(dst, 1, (uint16_t)rem, g_resource_file);
    far_memcpy(dst, (uint8_t huge *)g_stream_in, (uint16_t)rem);
    g_stream_in += rem;
    return (int16_t)rem;
}

/*
 * 0x1f044
 *
 * Deliver a run of `n` literal bytes to the output.
 *
 * The output has two states and `g_stream_wanted` - what the caller of
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
    g_stream_rec->in_pos += n;
    if (g_stream_wanted >= n) {
        if (g_resource_flags & 0x40)
            read_into_huge(g_stream_out, n);
        else
            game_fseek(g_resource_file, (uint32_t)n, 1);
        g_stream_wanted -= n;
        g_stream_out += n;
        return 1;
    } else {
        g_stream_rec->spill_end += n;
        read_into_huge(g_stream_spill, n);
        return 0;
    }
}

/*
 * 0x1f0cf
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
    if (g_stream_wanted >= n) {
        if (g_resource_flags & 0x40)
            far_memset(g_stream_out, value, (int32_t)n);
        g_stream_wanted -= n;
        g_stream_out += n;
        return 1;
    } else {
        far_memset(g_stream_spill + g_stream_rec->spill_end,
                   value, (int32_t)n);
        g_stream_rec->spill_end += n;
        return 0;
    }
}

/*
 * 0x1f154
 *
 * Deliver one byte - `emit_literal_run` and `emit_fill_run` written for a run
 * of exactly one, with the same two states and the same answers. The spill
 * reads the record's `spill_end` and increments it in one instruction, so the
 * byte lands at the old count.
 */
int16_t near emit_byte(uint16_t value)
{
    if ((int16_t)g_stream_wanted >= 1) {
#ifdef __TURBOC__
        asm les bx, g_stream_out
        if (g_resource_flags & 0x40) {
            asm mov al, byte ptr value
            asm mov es:[bx], al
        }
        asm inc bx
        asm jne stepped
        asm add word ptr g_stream_out+2, 1000h
stepped:
        asm mov word ptr g_stream_out, bx
#else
        if (g_resource_flags & 0x40)
            *g_stream_out = value;
        g_stream_out++;
#endif
        g_stream_wanted--;
        return 1;
    }
    g_stream_spill[g_stream_rec->spill_end++] = value;
    return 0;
}

/*
 * 0x1f1a2
 *
 * **Write one byte of a resource being written**: counted in `g_stream_written`,
 * then to the file with `game_fputc`, or into memory at the record's data
 * plus its position, which steps. The writing side's byte sink, which
 * nothing reaches.
 */
int16_t near put_output_byte(int16_t c)
{
    g_stream_written++;
    if (g_stream_kind & 0x20)
        return game_fputc(c, g_resource_file);
    else
        return g_stream_rec->data.ptr[g_stream_rec->in_pos++] = c;
}

/*
 * 0x1f1f6
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
        || (g_stream_rec = g_resource_slots[handle]) == NULL)
        return 0;

    g_stream_scratch = g_stream_rec->scratch;
    g_stream_spill = g_stream_rec->work;
    g_resource_handler =
        (g_stream_kind = g_stream_rec->kind) & 0x1f;
    if (g_stream_kind & 0x20) {
        g_resource_file = g_stream_rec->data.file;
        g_resource_flags = 0x20;
    } else {
        g_resource_flags = 0;
        g_stream_in = (char huge *)normalise_pointer_far(
            (uint8_t huge *)(g_stream_rec->data.ptr + g_stream_rec->in_pos));
    }
    return 1;
}

/*
 * 0x1f290
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
 * 0x1f2b2
 *
 * Free a pointer unless it is null - the whole routine.
 */
void near free_if_set(void *p)
{
    if (p)
        free_far(p);
}

/*
 * 0x1f2c7
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
    if ((g_stream_rec = g_resource_slots[slot]) != NULL) {
        free_if_set(g_stream_rec->work);
        if (g_stream_rec->scratch != NULL && !g_scratch_block)
            dos_free_far(g_stream_rec->scratch);
    }
    free_if_set(g_stream_rec);
    g_resource_slots[slot] = NULL;
    return -1;
}

/*
 * 0x1f330
 *
 * Take a resource slot. Answers its number, or -1 when all hundred are in use
 * or the record cannot be allocated. The record is 0x21 bytes from `calloc`,
 * so it starts cleared - which matters, because `close_resource_slot` frees
 * whatever pointers it finds in it.
 */
int16_t near open_resource_slot(char *mode)
{
    int16_t i;

    for (i = 0; i < 0x64; i++)
        if (g_resource_slots[i] == NULL)
            break;
    if (i == 0x64)
        return -1;
    if ((g_stream_rec = (struct resource *)calloc_far(1, sizeof(struct resource))) == NULL)
        return -1;
    g_resource_slots[i] = g_stream_rec;
    return i;
}

/*
 * 0x1f37f
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
    h = &g_engine_res_handlers.type[type];
    near_size = 0x80;
    if (string_contains_r(mode)) {
        near_size = h->near_size;
        far_size = h->far_size_read;
    } else
        far_size = h->far_size;

    if ((g_stream_rec->work = (uint8_t *)calloc_far(1, near_size)) == NULL)
        return -1;
    if (far_size) {
        /* Compared as the huge pointer the original held it as. */
        if ((uint8_t huge *)g_scratch_block != NULL)
            g_stream_scratch = g_stream_rec->scratch = g_scratch_block;
        else
            g_stream_scratch = g_stream_rec->scratch =
                dos_alloc_bytes((uint32_t)far_size, 0);
        if (!g_stream_rec->scratch)
            return -1;
    }
    g_stream_rec->kind = type;
    return 0;
}

/*
 * 0x1f44c
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

    start = g_stream_rec->spill_start;
    n = g_stream_rec->spill_end - start;
    if (n > g_stream_wanted)
        g_stream_rec->spill_start += n = g_stream_wanted;
    else
        g_stream_rec->spill_start = g_stream_rec->spill_end = 0;
    if (n != 0) {
        if (g_resource_flags & 0x40)
            far_memcpy(g_stream_out, g_stream_spill + start, n);
        g_stream_wanted -= n;
        g_stream_out += n;
    }
}

/*
 * 0x1f4cf
 *
 * Read `count` bytes of the selected resource: first whatever the spill
 * buffer holds, then the handler's decoder, then the spill again for what the
 * decoder left there. Answers how many were delivered, and adds them to the
 * record's `pos`.
 *
 * The first argument is not read.
 */
int16_t near resource_read(int16_t handle, uint16_t count)
{
    g_stream_wanted = count;
    resource_advance();
    if (g_stream_wanted != 0) {
        g_engine_res_handlers.type[g_resource_handler].read();
        if (g_stream_wanted != 0)
            resource_advance();
    }
    count -= g_stream_wanted;
    g_stream_rec->pos += count;
    return count;
}

/*
 * 0x1f514
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
    far_memset(g_stream_scratch, 0, 0x3aa1L);
    g_lzw_maxcode = (1 << (g_lzw_n_bits = 9)) - 1;
    for (i = 0xff; i >= 0; i--) {
        ((int16_t huge *)g_stream_scratch)[i] = 0;
        *(g_stream_scratch + i + 0x2720L) = i & 0xff;
    }
    g_lzw_free_ent = 0x101;
    g_lzw_clear_flg = 0;
    g_lzw_first_code = 1;
    g_lzw_resume = 0;
    g_lzw_bit_pos = 0;
    g_lzw_bit_end = 0;
    g_lzw_de_stack = g_stream_scratch + 0x3720L;
}

/*
 * 0x1f5e1
 *
 * An empty routine: `push bp / mov bp,sp / pop bp / ret`, and nothing calls
 * it.
 */
void near resource_nothing_1(void)
{
}

/*
 * 0x1f5e6
 *
 * The same, again.
 */
void near resource_nothing_2(void)
{
}
