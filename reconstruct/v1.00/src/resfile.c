/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The resource files' public face, and the compressors behind it**: open,
 * close, read, write, seek and size a resource by handle, over the streams
 * resource.c keeps; and the writing side of types 1 and 2 - a run-length
 * coder, and the LZW coder of Unix `compress`, whose globals and routines
 * this keeps the shape and the names of: `hsize`, `checkpoint`, `in_count`,
 * `lmask`, `rmask`, `output`, `cl_block`, `cl_hash`. The game only ever
 * reads, so all of the writing side is uncalled.
 *
 * One module of the original's **code segment 1c25**, image
 * 0x1ce1f..0x1dba8. The coder's routines are `near` and the API's far; a far
 * callee defined earlier is reached with a bare `push cs / call`, which is
 * how `long_div` is placed here. Its `_DATA` is 0x35d6..0x3600 and its `_BSS`
 * 0x58b8..0x58d2. Both ends are assembly: the LZW decoder's module before it,
 * the LZSS one's after.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* DGROUP 0x35d6..0x3600. */
struct engine_compress_data {
    int16_t   hsize;              /* +0x00  the hash table's size, 5003 */
    int16_t   free_ent;           /* +0x02  the next free code */
    int16_t   clear_when_full;    /* +0x04  clear a full table at once, not when the ratio falls */
    int16_t   clear_flg;          /* +0x06  the next `output` resets the width */
    int32_t   ratio;              /* +0x08  in over out, in 256ths */
    int32_t   checkpoint;         /* +0x0c  in_count at the next measurement */
    int32_t   in_count;           /* +0x10  bytes taken in */
    int32_t   out_count;          /* +0x14  codes put out */
    uint8_t   lmask[9];           /* +0x18 */
    uint8_t   rmask[9];           /* +0x21 */
} PACKED;

/* DGROUP 0x58b8..0x58d2. */
struct engine_compress_state {
    int16_t   n_bits;             /* +0x00  the code width */
    uint8_t   buf[12];            /* +0x02  n_bits codes, packed */
    int16_t   ent;                /* +0x0e  the string so far, as a code */
    int16_t   maxcode;            /* +0x10  the largest code at this width */
    int32_t   bytes_out;          /* +0x12 */
    int16_t   offset;             /* +0x16  the bit offset into `buf` */
    uint8_t   first;              /* +0x18  the next byte starts the string */
    uint8_t   pad_19;             /* +0x19 */
} PACKED;

/*
 * **The LZW coder's settings and counters**, DGROUP 0x35d6..0x3600: the hash
 * table's size, the next free code, whether a full table is cleared at
 * once, the clear flag, the compression ratio last measured, when it is next
 * measured, the bytes taken in and the codes put out, and the two bit-mask
 * tables `output` packs codes with.
 */
struct engine_compress_data g_engine_compress = {
    5003, 0, 0, 0, 0, 10000, 1, 0,
    { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00 },
    { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff },
};

/*
 * **Where the coder is**, DGROUP 0x58b8..0x58d2: the code width, the twelve
 * bytes codes are packed into, the string so far, the largest code at this
 * width, the bytes put out, the bit offset into the pack, and whether the
 * next byte is the first.
 */
struct engine_compress_state g_engine_compress_state;

#define LZC g_engine_compress
#define LZS g_engine_compress_state

/* The hash table, `hsize` longs at the front of the stream's scratch block,
   and the code table right after it: `hsize * 4` is 0x4e2c. */
#define htabof(i)    (((int32_t huge *)g_engine_stream.scratch)[i])
#define codetabof(i) (*(int16_t huge *)((uint8_t huge *) \
                        ((int16_t huge *)g_engine_stream.scratch + (i)) + 0x4e2c))

/*
 * 0x1ce1f
 *
 * Start the LZW coder for a resource opened to write: counters to zero, the
 * code width to nine, the hash table cleared, and the next byte the first.
 * The table is cleared again only when the ratio falls - the flag at the end
 * is 0 - where `lzw_open_write`, the same routine, sets it. Nothing calls
 * this one; the handler table names the other.
 */
int16_t near lzw_open_write_ratio(void)
{
    LZS.offset = 0;
    LZS.bytes_out = 0;
    LZC.out_count = 0;
    LZC.clear_flg = 0;
    LZC.ratio = 0;
    LZC.in_count = 1;
    LZC.checkpoint = 10000;
    LZS.maxcode = (1 << (LZS.n_bits = 9)) - 1;
    LZC.free_ent = 0x101;
    cl_hash((int32_t)LZC.hsize);
    LZS.first = 1;
    LZC.clear_when_full = 0;
    return 0;
}

/*
 * 0x1ce9d
 *
 * The same, with a full code table cleared as soon as it fills. Type 2's
 * open-for-writing in the handler table.
 */
int16_t near lzw_open_write(void)
{
    LZS.offset = 0;
    LZS.bytes_out = 0;
    LZC.out_count = 0;
    LZC.clear_flg = 0;
    LZC.ratio = 0;
    LZC.in_count = 1;
    LZC.checkpoint = 10000;
    LZS.maxcode = (1 << (LZS.n_bits = 9)) - 1;
    LZC.free_ent = 0x101;
    cl_hash((int32_t)LZC.hsize);
    LZS.first = 1;
    LZC.clear_when_full = 1;
    return 0;
}

/*
 * 0x1cf1b
 *
 * **Compress what is waiting in the spill ring** - `compress`'s main loop
 * over the bytes the writer has put there, from `spill_start` round to
 * `spill_end`. Each byte extends the current string if the pair is in the
 * hash table, and otherwise puts the string's code out and adds the pair,
 * or clears the table when it is full and the ratio says so. `final` puts
 * the last string out and the end code after it. Type 2's flush.
 */
int16_t near lzw_flush(int16_t final)
{
    int32_t fcode;
    int16_t disp;
    int16_t start;
    int16_t end;
    uint8_t *buf;
    register int16_t i = 0;
    register int16_t c;

    buf = g_engine_stream.spill;
    start = g_engine_stream.rec->spill_start;
    end = g_engine_stream.rec->spill_end;
    while ((start &= 0x7f) != end) {
        c = buf[start++];
        if (LZS.first) {
            LZS.ent = c;
            LZS.first = 0;
            continue;
        }
        LZC.in_count++;
        fcode = ((int32_t)c << 12) + LZS.ent;
        i = (c << 4) ^ LZS.ent;
        if (i == 0)
            disp = 1;
        else
            disp = LZC.hsize - i;
probe:
        if (htabof(i) == fcode) {
            LZS.ent = codetabof(i);
            continue;
        }
        if (htabof(i) < 0) {
            output(LZS.ent);
            LZC.out_count++;
            LZS.ent = c;
            if (LZC.free_ent < 0x1000) {
                codetabof(i) = LZC.free_ent++;
                htabof(i) = fcode;
            } else if (LZC.in_count >= LZC.checkpoint || LZC.clear_when_full)
                cl_block();
        } else {
            if ((i -= disp) < 0)
                i += LZC.hsize;
            goto probe;
        }
    }
    g_engine_stream.rec->spill_start = start;
    g_engine_stream.rec->spill_end = end;
    if (final) {
        output(LZS.ent);
        LZC.out_count++;
        output(-1);
    }
    return 0;
}

/*
 * 0x1d133
 *
 * `compress`'s `output`: pack one code of `n_bits` bits into the twelve-byte
 * buffer at the bit `offset`, and put the buffer out through
 * `put_output_byte` when it holds `n_bits` codes. A width change - the free
 * code past `maxcode`, or a clear - puts out what is packed first. A code of
 * -1 flushes the part-filled buffer at the end.
 */
void near output(int16_t code)
{
    int16_t r_off;
    int16_t i;
    register uint8_t *bp;
    register int16_t bits;

    r_off = LZS.offset;
    bits = LZS.n_bits;
    bp = LZS.buf;
    if (code >= 0) {
        bp += r_off >> 3;
        r_off &= 7;
        *bp = (*bp & LZC.rmask[r_off]) | ((code << r_off) & LZC.lmask[r_off]);
        bp++;
        bits -= 8 - r_off;
        code >>= 8 - r_off;
        if (bits >= 8) {
            *bp++ = code;
            code >>= 8;
            bits -= 8;
        }
        if (bits)
            *bp = code;
        LZS.offset += LZS.n_bits;
        if (LZS.offset == LZS.n_bits << 3) {
            bp = LZS.buf;
            bits = LZS.n_bits;
            LZS.bytes_out += bits;
            do
                put_output_byte(*bp++);
            while (--bits);
            LZS.offset = 0;
        }
        if (LZC.free_ent > LZS.maxcode || LZC.clear_flg) {
            if (LZS.offset > 0) {
                for (i = 0; i < LZS.n_bits; i++)
                    put_output_byte(LZS.buf[i]);
                LZS.bytes_out += LZS.n_bits;
            }
            LZS.offset = 0;
            if (LZC.clear_flg) {
                LZS.maxcode = (1 << (LZS.n_bits = 9)) - 1;
                LZC.clear_flg = 0;
            } else {
                LZS.n_bits++;
                if (LZS.n_bits == 12)
                    LZS.maxcode = 0x1000;
                else
                    LZS.maxcode = (1 << LZS.n_bits) - 1;
            }
        }
    } else {
        if (LZS.offset > 0)
            for (i = 0; i < (LZS.offset + 7) / 8; i++)
                put_output_byte(LZS.buf[i]);
        LZS.bytes_out += (LZS.offset + 7) / 8;
        LZS.offset = 0;
    }
}

/*
 * 0x1d2c4
 *
 * A `long` divided by a `long` - the runtime's divide, as a far routine of
 * its own. `cl_block` is its only caller.
 */
int32_t long_div(int32_t a, int32_t b)
{
    return a / b;
}

/*
 * 0x1d2dc
 *
 * `compress`'s `cl_block`: measure the ratio of bytes in to bytes out, in
 * eighths of a bit, and when it has fallen since last time clear the hash
 * table and put the clear code out. The next measurement is 10000 bytes on.
 */
void near cl_block(void)
{
    int32_t rat;

    LZC.checkpoint = LZC.in_count + 10000;
    if (LZC.in_count <= 0x007fffffL)
        rat = long_div(LZC.in_count << 8, LZS.bytes_out);
    else if ((rat = LZS.bytes_out >> 8) == 0)
        rat = 0x7fffffffL;
    else
        rat = long_div(LZC.in_count, rat);
    if (rat > LZC.ratio)
        LZC.ratio = rat;
    else {
        LZC.ratio = 0;
        cl_hash((int32_t)LZC.hsize);
        LZC.free_ent = 0x101;
        LZC.clear_flg = 1;
        output(0x100);
    }
}

/*
 * 0x1d3bf
 *
 * `compress`'s `cl_hash`: set `hsize` entries of the hash table to -1, the
 * empty slot.
 */
void near cl_hash(int32_t hsize)
{
    int32_t huge *p;

    p = (int32_t huge *)g_engine_stream.scratch;
    while (--hsize >= 0) {
        *p = -1;
        p++;
    }
}

/*
 * 0x1d40d
 *
 * **Run-length code what is waiting in the spill ring** - type 1's flush,
 * the coder `decompress_rle` undoes. From the start it finds the first run
 * of three or more equal bytes; the literals before it go out as a count
 * byte and the bytes, and the run as the count with bit 7 set and the value
 * once. Unless `final`, a stretch that reaches the end of what is waiting
 * with no run in it is left for the next call, which may extend it.
 */
void near rle_flush(int16_t final)
{
    uint8_t start;
    uint8_t end;
    int16_t avail;
    int16_t run;
    int16_t j;
    uint8_t flag;
    uint8_t mark;
    register uint8_t *buf;
    register int16_t last;

    buf = g_engine_stream.spill;
    start = g_engine_stream.rec->spill_start;
    end = g_engine_stream.rec->spill_end;
    while ((avail = (end - start) & 0x7f) != 0) {
        last = -1;
        j = start;
        run = 1;
        do {
            if (buf[j] == last)
                run++;
            else {
                if (run >= 3)
                    break;
                run = 1;
            }
            last = buf[j];
            j++;
        } while ((j &= 0x7f) != end);
        flag = 0;
        if (run >= 3) {
            mark = (j - run) & 0x7f;
            if (mark == start) {
                flag = 0x80;
                mark = j;
            }
        } else
            mark = end;
        run = (mark - start) & 0x7f;
        if (run == avail && run < 0x7f && !final)
            break;
        put_output_byte(run | flag);
        if (flag & 0x80) {
            put_output_byte(last);
            start = (start + run) & 0x7f;
        } else
            while (run--) {
                put_output_byte(buf[start++]);
                start &= 0x7f;
            }
    }
    g_engine_stream.rec->spill_start = start;
    g_engine_stream.rec->spill_end = end;
}

/*
 * 0x1d54e
 *
 * **Open a resource in a file** and answer its handle, or -1. The file is
 * at the resource's header: a type byte and the decoded size.
 *
 * Reading (an "r" in the mode), the type is taken from the file, the slot
 * given the memory that type needs, the compressed size recorded as the
 * caller gives it and the decoded size read, and the type's reset run.
 * Writing, the type is the caller's first argument: it is written, then
 * four bytes of header to be filled in at close, and the type's
 * open-for-writing run. Either way the stream reads from a file.
 *
 * `open_resource_slot` is handed the mode, which it does not read.
 */
int16_t open_resource(int16_t type, FILE *file, char *mode, int32_t size)
{
    int16_t slot;
    int32_t header;

    if ((slot = open_resource_slot(mode)) == -1)
        return -1;
    g_engine_stream.rec->data.file = file;
    g_engine_stream.rec->start = game_ftell(file);
    g_engine_stream.rec->in = 5;
    if (string_contains_r(mode)) {
        if (prepare_resource_slot(type = g_engine_stream.rec->kind = game_fgetc(file),
                                  mode) == -1) {
            game_fseek(file, -1L, 1);
            return close_resource_slot(slot);
        }
        g_engine_stream.rec->end = size;
        game_fread((uint8_t *)&g_engine_stream.rec->size, 1, 4, file);
        if (g_engine_res_handlers.type[type].reset)
            g_engine_res_handlers.type[type].reset();
        g_engine_stream.rec->kind |= 0x40;
    } else {
        if (prepare_resource_slot(type, mode) == -1)
            return close_resource_slot(slot);
        game_fputc(type, file);
        game_fwrite((uint8_t *)&header, 1, 4, file);
        if (g_engine_res_handlers.type[type].open_write)
            g_engine_res_handlers.type[type].open_write();
    }
    g_engine_stream.rec->kind |= 0x20;
    return slot;
}

/*
 * 0x1d698
 *
 * **Open a resource in memory**: `open_resource` for a block the caller
 * holds, the header read from or written to its front. Nothing calls it.
 */
int16_t open_resource_mem(int16_t type, char huge *data, char *mode, int32_t size)
{
    int16_t slot;

    if ((slot = open_resource_slot(mode)) == -1)
        return -1;
    g_engine_stream.rec->data.ptr = data;
    g_engine_stream.rec->kind = type;
    g_engine_stream.rec->in = 5;
    if (string_contains_r(mode)) {
        if (prepare_resource_slot(type = g_engine_stream.rec->kind = *data++,
                                  mode) == -1)
            return close_resource_slot(slot);
        far_memcpy((uint8_t *)&g_engine_stream.rec->size, (uint8_t huge *)data, 4);
        g_engine_stream.rec->end = size;
        if (g_engine_res_handlers.type[type].reset)
            g_engine_res_handlers.type[type].reset();
        g_engine_stream.rec->kind |= 0x40;
    } else {
        if (prepare_resource_slot(type, mode) == -1)
            return close_resource_slot(slot);
        *g_engine_stream.rec->data.ptr = type;
    }
    return slot;
}

/*
 * 0x1d798
 *
 * **Close a resource**, answering the bytes the writing side put out - zero
 * for one opened to read. Writing, the type's flush runs with `final` set,
 * and the decoded size goes into the header: back over it in the file, or
 * into the block's front.
 */
int16_t close_resource(int16_t handle)
{
    if (!select_resource(handle))
        return -1;
    g_engine_stream.written = 0;
    if (!(g_engine_stream.kind & 0x40)) {
        g_engine_res_handlers.type[g_engine_resource_flags.handler].flush(1);
        if (g_engine_stream.kind & 0x20) {
            game_fseek(g_engine_resource_flags.file, g_engine_stream.rec->start + 1, 0);
            game_fwrite((uint8_t *)&g_engine_stream.rec->size, 4, 1,
                        g_engine_resource_flags.file);
            game_fseek(g_engine_resource_flags.file, 0L, 2);
        } else
            far_memcpy((uint8_t huge *)(g_engine_stream.rec->data.ptr + 1),
                       (uint8_t *)&g_engine_stream.rec->size, 4);
    }
    close_resource_slot(handle);
    return g_engine_stream.written;
}

/*
 * 0x1d868
 *
 * **Read** `count` bytes of a resource to `dst`, answering how many, or -1
 * for a handle that names nothing. The destination is normalised into the
 * stream's output cursor, and bit 0x40 is what tells the emitters to write
 * rather than skip.
 */
int16_t read_resource(int16_t handle, uint8_t far *dst, uint16_t count)
{
    if (!select_resource(handle))
        return -1;
    g_engine_stream.out = normalise_pointer_far(dst);
    g_engine_resource_flags.flags |= 0x40;
    return resource_read(handle, count);
}

/*
 * 0x1d8a4
 *
 * **Write** `count` bytes to a resource opened for writing: into the spill
 * ring a ring's worth at a time, the type's flush after each. Answers the
 * bytes the flushes put out. Nothing calls it.
 */
int16_t write_resource(int16_t handle, uint8_t huge *src, uint16_t count)
{
    uint8_t huge *p;
    int16_t end;
    int16_t stop;
    register uint8_t *buf;

    if (!select_resource(handle))
        return -1;
    g_engine_stream.written = 0;
    g_engine_stream.rec->size += count;
    buf = g_engine_stream.rec->work;
    p = src;
    while (count) {
        end = g_engine_stream.rec->spill_end;
        stop = (g_engine_stream.rec->spill_start - 1) & 0x7f;
        do {
            buf[end] = *p;
            p++;
            end++;
            count--;
            end &= 0x7f;
        } while (end != stop && count);
        g_engine_stream.rec->spill_end = end & 0x7f;
        g_engine_res_handlers.type[g_engine_resource_flags.handler].flush(0);
    }
    return g_engine_stream.written;
}

/*
 * 0x1d95f
 *
 * The decoded size of a resource, from its header, or -1 for a handle that
 * names nothing.
 */
int32_t resource_size(int16_t handle)
{
    if (!select_resource(handle))
        return -1L;
    return g_engine_stream.rec->size;
}

/*
 * 0x1d983
 *
 * **Seek** within a resource, answering the position reached, or -1 for a
 * handle that names nothing.
 *
 * A compressed stream cannot be seeked, so this skips by decoding: the
 * target from the whence - 0 the start, 1 the position, 2 the size - and the
 * distance to it read in chunks of at most 0x7d00 and thrown away, which is
 * what not setting bit 0x40 makes happen. A target behind the position
 * restarts the stream first, and one past the end is clamped to it. Each
 * chunk re-derives the input cursor from the record's own.
 */
int32_t resource_seek(int16_t handle, int32_t by, int16_t whence)
{
    int32_t t;

    if (!select_resource(handle))
        return -1L;
    t = 0;
    switch (whence) {
    case 1:
        t = g_engine_stream.rec->pos;
        break;
    case 2:
        t = g_engine_stream.rec->size;
        break;
    }
    t += by;
    if (g_engine_stream.rec->pos == t)
        return t;
    if (g_engine_stream.rec->pos > t) {
        restart_resource_stream(handle);
        if (t <= 0)
            return 0L;
    } else if (g_engine_stream.rec->size <= t)
        t = g_engine_stream.rec->size - g_engine_stream.rec->pos;
    else
        t -= g_engine_stream.rec->pos;
    while ((t -= (uint16_t)resource_read(handle, t < 0x7d00L ? (uint16_t)t : 0x7d00)) != 0)
        g_engine_stream.in = (char huge *)normalise_pointer_far(
            (uint8_t huge *)(g_engine_stream.rec->data.ptr + g_engine_stream.rec->in));
    return g_engine_stream.rec->pos;
}

/*
 * 0x1dae6
 *
 * **Put a resource stream back to its beginning**, so a seek backwards can
 * then skip forwards. Only a stream opened to read can be; anything else
 * answers -1. The type's reset runs, and the record goes back to where
 * `open_resource` left it: past the five-byte header, in the file or in
 * memory, with nothing decoded and nothing spilled.
 */
int16_t restart_resource_stream(int16_t handle)
{
    if (!select_resource(handle) || !(g_engine_stream.kind & 0x40))
        return -1;
    if (g_engine_res_handlers.type[g_engine_resource_flags.handler].reset)
        g_engine_res_handlers.type[g_engine_resource_flags.handler].reset();
    g_engine_stream.rec->in = 5;
    if (g_engine_stream.rec->kind & 0x20)
        game_fseek(g_engine_resource_flags.file, g_engine_stream.rec->start + 5, 0);
    else
        g_engine_stream.in = (char huge *)normalise_pointer_far(
            (uint8_t huge *)(g_engine_stream.rec->data.ptr + 5));
    g_engine_stream.rec->spill_end = g_engine_stream.rec->spill_start =
        g_engine_stream.rec->pos = 0;
    return 0;
}
