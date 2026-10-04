/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The LZW decoder and the run-length decoder over memory** - type 2 of the
 * resource handlers, the code reader under it, and type 1 for a resource
 * already in memory. Hand-written assembly: the decoder stops in the middle
 * of a string and resumes there on the next call, with DS moved onto the
 * dictionary; so it is TASM source, `lzw.asm`, with the host's transcription
 * here.
 *
 * One module of the original's **code segment 1c25**, image
 * 0x1ca46..0x1ce1f - the decoder's own head comes before its entry at
 * 0x1ca62 - with its `_DATA` 0x35b2..0x35d6. Its ends are resource.c's last
 * routine and resfile.c's first.
 */
#include <string.h>

#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **Nine bit masks**, DGROUP 0x35b2..0x35bc, `(1 << n) - 1` for n from 0 to 8
 * - the same nine as `g_engine_lzw_masks` - and a zero byte. **Not established**
 * which routine reads these; the port's decoders read the other copy.
 */
struct engine_bit_masks {
    uint8_t   mask[9];            /* +0x00 */
    uint8_t   pad_35bb;           /* +0x09 */
} PACKED;

struct engine_bit_masks g_engine_bit_masks = { .mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff } };

/*
 * **The bit reader's input window**, DGROUP 0x35bc..0x35c8, 0x0c bytes: `next_lzw_code` has
 * `read_input_block` fill it and takes its codes out of it a byte at a time,
 * from the bit position g_engine_stream keeps. Twelve bytes, up to the mask table.
 */
struct engine_lzw_window {
    uint8_t   window[12];         /* +0x00 [0xc] */
    /* OURS: a code that ends on the window's last byte reads one more and
       masks it with `mask[0]`, which is 0 - in DGROUP that byte is the mask
       table's first. The host's object has to have it. */
    uint8_t   over;
} PACKED;

struct engine_lzw_window g_engine_lzw_window = {
    .window = {
        0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
        0x0c,
    },
};

/*
 * **The LZW mask table**, DGROUP 0x35c8..0x35d1, 0x09 bytes: 0, 1, 3, 7, ..., 0xff, indexed
 * by how many bits are still wanted. Nine bytes, up to 0x35d1.
 */
struct engine_lzw_masks {
    uint8_t   mask[9];            /* +0x00 [9] */
} PACKED;

struct engine_lzw_masks g_engine_lzw_masks = { .mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff } };

/*
 * **Where the LZW string had got to**, DGROUP 0x35d1..0x35d3, 0x02 bytes.
 *
 * `decompress_lzw` copies a decoded string out of its scratch buffer
 * backwards, and a request that fills mid-string has to resume there next
 * time. This is that position, as an offset into the scratch buffer, saved
 * beside the byte at DGROUP 0x58a2 that says a resume is pending.
 */
struct engine_lzw_resume {
    int16_t   scratch_at;         /* +0x00 [2] */
} PACKED;

struct engine_lzw_resume g_engine_lzw_resume;

/*
 * **Three bytes before the LZW coder's data**, DGROUP 0x35d3..0x35d6: the
 * assembly at 0x1cd2c keeps a near pointer at 0x35d4. The rest of the
 * writer's state is resfile.c's.
 */
struct engine_bit_state {
    uint8_t   pad_35d3[3];        /* +0x00 */
} PACKED;

struct engine_bit_state g_engine_bit_state;

/*
 * 0x1ca62
 *
 * Decompression type 2: LZW, hand-written assembly, and the only routine here
 * that has to be able to **stop in the middle** and be called again.
 *
 * The dictionary is the block at DGROUP 0x588c:0x588e - prefix codes as words
 * from offset 0, suffix bytes from 0x2720 - and 0x372 paragraphs above it, at
 * offset 0x3720, is a scratch area the decoded string is built in.
 *
 * A code is expanded by walking the prefix chain, which produces the string
 * **backwards**, so it is written forward into scratch and then copied out
 * backwards. That is what the `sub si,2` after each `lodsb` is doing: one
 * forward from the load, two back, net one back.
 *
 * A code at or above the next free one is the case where the string being
 * decoded is the one about to be defined; the last byte emitted goes down
 * first and the previous code is expanded behind it.
 *
 * Code 0x100 clears the dictionary - 0x200 bytes filled with the *offset* of
 * the dictionary, which is zero in practice but transcribed as written - and
 * arms the reset flag `next_lzw_code` reads.
 *
 * Both copy loops are unrolled ten times in the original and are written here
 * as the loops they are; nothing depends on the unrolling.
 *
 * The suspend at 0x1cbf9 is the interesting part. When the caller's request
 * fills up mid-string, the scratch position is parked at DGROUP 0x35d1, the
 * byte that did not fit is spilled into the small buffer at 0x5892, and 0x58a2
 * is set. The next call comes back in at the middle of whichever copy loop it
 * left, with the destination, the count and the scratch position restored -
 * which is why this is not written as a plain loop over codes.
 *
 * Answers 1 when it suspended, and whatever `next_lzw_code` answered - a
 * negative - at the end of the input.
 */
int16_t decompress_lzw(void)
{
    /*
     * **The scratch block, and the three things inside it.** The dictionary is
     * a word per code at +0 and a byte per code at +0x2720; the buffer the
     * string is built into, forwards, and then read out of backwards, is at
     * +0x3720 - which the original reaches as the segment plus 0x372
     * paragraphs, because the block's offset is zero.
     *
     * **It is zero by construction, not by luck.** `select_resource` files
     * either a DOS block, which starts a segment, or `g_scratch_block`, which
     * is masked to a paragraph with `& 0xfff0` and then normalised where it is
     * built. Every use here reads that invariant, and the clear loop below
     * writes the offset itself into the dictionary, which is what makes it
     * visible.
     */
    uint8_t far * block = g_engine_stream.scratch;
    uint16_t *prefix = (uint16_t *)(void *)block;
    uint8_t far * suffix = block + 0x2720;
    uint8_t far * scratch = block + 0x3720;
    uint8_t far *in, *back;
    /* The output cursor: `di` against the segment the caller chose, walked
       with `inc di` and filed back into DGROUP 0x5894 - a far pointer, whose
       arithmetic is the offset's. The scratch index that shared the `di`
       register is `in` above, which is a different thing entirely. */
    uint8_t far * out;
    uint16_t si, cx;
    int16_t code;
    uint8_t al = 0;
    int16_t copying;

    if (g_engine_stream.resume != 0) {
        cx = (uint16_t)(g_engine_stream.wanted + 1);
        out = (uint8_t far *)g_engine_stream.output;
        back = scratch + (uint16_t)g_engine_lzw_resume.scratch_at;
        copying = (g_engine_resource_flags.flags & 0x40) != 0;
        g_engine_stream.resume = 0;
        goto step_back;
    }

    if (g_engine_stream.first_code != 0) {
        /* 0x1ca46 - the first code of a stream is a literal. */
        g_engine_stream.first_code = 0;
        code = next_lzw_code();
        g_engine_stream.oldcode = code;
        g_engine_stream.finchar = code;
        emit_byte((uint16_t)code);
    }

    for (;;) {
        code = next_lzw_code();
        if (code < 0)
            return code;

        if (code == 0x100) {
            /* The block's own offset, written into every entry it clears -
               zero, as the note on `block` says, and written as the field
               rather than as a 0 because that is what the original stores. */
            uint16_t p = FP_OFF(g_engine_stream.scratch);
            int16_t i;

            for (i = 0; i < 0x100; i++)
                prefix[i] = p;

            g_engine_stream.clear_flg = (int16_t)(p + 1);
            g_engine_stream.free_ent = (int16_t)(((p + 1) << 8) | ((p + 1) >> 8));

            code = next_lzw_code();
            if (code < 0)
                return code;
        }

        in = scratch;
        si = (uint16_t)code;
        g_engine_stream.incode = code;

        if ((int16_t)si >= g_engine_stream.free_ent) {
            *in++ = (uint8_t)((uint16_t)g_engine_stream.finchar);
            si = ((uint16_t)g_engine_stream.oldcode);
        }

        while (si >= 0x100) {
            *in++ = suffix[si];
            si = prefix[si];
        }

        al = suffix[si];
        *in++ = al;
        g_engine_stream.finchar = al;

        cx = (uint16_t)(g_engine_stream.wanted + 1);
        back = in - 1;
        out = (uint8_t far *)g_engine_stream.output;
        copying = (g_engine_resource_flags.flags & 0x40) != 0;

        for (;;) {
            al = *back++;
            if (--cx == 0) {
                /* 0x1cbf9 - the caller's request is full mid-string. */
                struct resource *rec;

                g_engine_stream.output = out;
                g_engine_lzw_resume.scratch_at = (int16_t)(back - scratch);

                rec = g_engine_stream.rec;
                {
                    uint16_t n = rec->spill_end;

                    /* 0x1cc0a is `inc word ptr [si+0x1a]`: a carry out of the
                       end lands in the start. */
                    if (++rec->spill_end == 0)
                        rec->spill_start++;
                    g_engine_stream.spill[n] = al;
                }

                g_engine_stream.wanted = 0;
                g_engine_stream.resume = 1;
                return 1;
            }

            if (copying)
                *out = al;
            out++;

step_back:
            /* one forward from the load, two back, net one back - and the
               original's `js` on a 16-bit offset is this pointer stepping
               below the buffer it started at. */
            back -= 2;
            if (back < scratch)
                break;
        }

        /* 0x1cc22 - this code is done and the dictionary can grow. */
        cx--;
        g_engine_stream.wanted = (int16_t)cx;
        g_engine_stream.output = out;

        if (g_engine_stream.free_ent < 0x1000) {
            uint16_t next = ((uint16_t)g_engine_stream.free_ent);

            prefix[next] = ((uint16_t)g_engine_stream.oldcode);
            g_engine_stream.free_ent = (int16_t)(next + 1);
            suffix[next] = (uint8_t)((uint16_t)g_engine_stream.finchar);
        }

        g_engine_stream.oldcode = g_engine_stream.incode;
    }
}

/*
 * 0x1cc65
 *
 * The next LZW code, 9 to 12 bits wide, out of a bit buffer at DGROUP 0x35bc.
 * Answers -1 at the end of the input.
 *
 * Three things can happen before a code is extracted, and they fall through
 * into one another:
 *
 *   the next free code has passed the width's limit at 0x58b6, so the width at
 *   0x589e goes up by one and the limit with it - `1 << width` minus one,
 *   except at twelve bits where it is 0x1000 rather than 0xfff;
 *
 *   0x58a4 says the dictionary is to be reset, so the width goes back to nine
 *   and the limit to 0x1ff;
 *
 *   the bit buffer is empty, so `read_input_block` fills it with `width` bytes.
 *
 * A widening or a reset **always** refills, discarding whatever bits were left.
 * That is not a mistake: the compressor pads to a byte boundary when the width
 * changes, which is what makes the two ends agree.
 *
 * The buffer holds at most twelve bytes, so the bit position is under 108 and
 * the `shr ax,cl` that follows `lodsb` is shifting a byte even though AH still
 * holds the high half of the position `add` - it is zero every time.
 *
 * The mask table at DGROUP 0x35c8 is indexed by how many bits are still wanted.
 */
int16_t next_lzw_code(void)
{
    uint16_t bitpos;
    uint16_t ax, dx;
    const uint8_t *in;
    uint8_t ch, bl;

    if ((int16_t)((uint16_t)g_engine_stream.free_ent) > g_engine_stream.maxcode) {
        uint16_t cx = (uint16_t)(((uint16_t)g_engine_stream.n_bits) + 1);

        g_engine_stream.n_bits = (int16_t)cx;
        if ((uint8_t)cx == 0xc)
            g_engine_stream.maxcode = 0x1000;
        else
            g_engine_stream.maxcode = (int16_t)((1 << (cx & 0xff)) - 1);

        if (g_engine_stream.clear_flg != 0) {
            g_engine_stream.n_bits = 9;
            g_engine_stream.maxcode = 0x1ff;
            g_engine_stream.clear_flg = 0;
        }
    } else if (g_engine_stream.clear_flg != 0) {
        g_engine_stream.n_bits = 9;
        g_engine_stream.maxcode = 0x1ff;
        g_engine_stream.clear_flg = 0;
    } else if (g_engine_stream.bit_pos < g_engine_stream.bit_end) {
        goto extract;
    }

    {
        uint16_t width = ((uint16_t)g_engine_stream.n_bits);
        int16_t n = read_input_block(g_engine_lzw_window.window, width);

        if (n <= 0) {
            g_engine_stream.bit_end = n;
            return -1;
        }

        g_engine_stream.bit_pos = 0;
        g_engine_stream.bit_end = (int16_t)((n << 3) - (width - 1));
    }

extract:
    bitpos = ((uint16_t)g_engine_stream.bit_pos);
    bl = (uint8_t)((uint16_t)g_engine_stream.n_bits);
    ch = (uint8_t)bitpos;

    g_engine_stream.bit_pos = (int16_t)(bitpos + ((uint16_t)g_engine_stream.n_bits));

    in = &g_engine_lzw_window.window[bitpos >> 3];
    ch &= 7;

    ax = *in;
    in++;
    ax = (uint16_t)(ax >> ch);
    dx = ax;

    ch = (uint8_t)(-(int8_t)(ch - 8));
    bl = (uint8_t)(bl - ch);

    if ((int8_t)bl >= 8) {
        ax = *in;
        in++;
        ax = (uint16_t)(ax << ch);
        dx |= ax;
        ch = (uint8_t)(ch + 8);
        bl = (uint8_t)(bl - 8);
    }

    ax = g_engine_lzw_masks.mask[bl];
    ax &= *in;
    ax = (uint16_t)(ax << ch);

    return (int16_t)(ax | dx);
}

/*
 * 0x1cd2c
 *
 * Decompression type 1 from a resource in memory, hand-written assembly. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near rle_from_memory(void)
{
    not_transcribed("0x1cd2c, rle_from_memory");
    return 0;
}
