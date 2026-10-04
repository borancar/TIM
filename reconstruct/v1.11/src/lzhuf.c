/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **LZHUF** - Okumura and Yoshizaki's LZSS with adaptive Huffman coding,
 * type 3 of the resource handlers, reading side only: the bit reader, the
 * symbol decoder with its tree update, the Huffman tree and its rebuild,
 * and the decoder. 1.00's encoder - the trees, the bit writer and the
 * flush - is gone from 1.11.
 *
 * One module of the original's **code segment 1c25**, image 0x20015..0x205c6,
 * with LZHUF.C's own data in its own order - `getbuf`, `getlen`, `p_len`,
 * `p_code`, `d_code`, `d_len` - as `_DATA` 0x3200..0x3483, and its `_BSS`
 * 0x54ce..0x54f2. **It is compiler output edited by hand and assembled**:
 * most routines are Borland C as it stands, with the `jmp` to the epilogue of
 * a build without `-O`, but the two in front are written by hand -
 * `huff_get_bit` has no frame, and `decode_char` walks the tree with the bit
 * reader inlined and then updates it with DS on the tables' segment, reaching
 * `huffman_reconst` with DS put back. So it is TASM source, `lzhuf.asm`, with
 * the host's transcription here. Its end is where the video driver's
 * interface begins, vmiface.c.
 */
#include <string.h>

#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The bit buffer the decompressor reads through**, LZHUF's `getbuf` and
 * `getlen`, DGROUP 0x3200..0x3203.
 */
struct engine_bit_buffer {
    int16_t   bits;               /* +0x00 [2]  filled from the top; bits come off the **left** */
    uint8_t   bit_count;          /* +0x02 [1]  how many are in it */
} PACKED;

struct engine_bit_buffer g_engine_bit_buffer;

/*
 * **The Huffman coder's position tables**, DGROUP 0x3203..0x3283: sixty-four
 * code lengths and sixty-four codes - LZHUF's `p_len` and `p_code`, the
 * *encoder's* half, next to the decoder's `g_engine_huffman_positions`.
 * Nothing in 1.11 reads them: the encoder that did is gone.
 */
struct engine_huffman_codes {
    uint8_t   len[64];            /* +0x00  0x3203 */
    uint8_t   code[64];           /* +0x40  0x3243 */
} PACKED;

struct engine_huffman_codes g_engine_huffman_codes = {
    .len = {
        0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    },
    .code = {
        0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80,
        0x88, 0x90, 0x94, 0x98, 0x9c, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4,
        0xb8, 0xbc, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0,
        0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6,
        0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    },
};

/*
 * **The Huffman position tables**, DGROUP 0x3283..0x3483, 0x200 bytes: for each code byte
 * `decode_position` reads, the high bits of the position at 0x3283 and the
 * length at 0x3383. 256 bytes each, up to 0x3483.
 */
struct engine_huffman_positions {
    uint8_t   high[256];          /* +0x00 [0x100] */
    uint8_t   len[256];           /* +0x100 [0x100] */
} PACKED;

struct engine_huffman_positions g_engine_huffman_positions = {
    .high = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09,
        0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0a,
        0x0a, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e,
        0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x10, 0x10, 0x10, 0x11,
        0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
        0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16,
        0x16, 0x17, 0x17, 0x17, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1a, 0x1a,
        0x1b, 0x1b, 0x1c, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f, 0x1f, 0x20,
        0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25,
        0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 0x29, 0x29, 0x2a, 0x2a, 0x2b,
        0x2b, 0x2c, 0x2c, 0x2d, 0x2d, 0x2e, 0x2e, 0x2f, 0x2f, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c,
        0x3d, 0x3e, 0x3f,
    },
    .len = {
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08,
    },
};

/*
 * **An interrupted match, and where it resumes**, DGROUP 0x54ce..0x54d6.
 */
struct engine_match_resume {
    int16_t   interrupted;        /* +0x00 [2]  a match was cut short */
    int16_t   position;           /* +0x02 [2]  and these three are what it comes back to */
    int16_t   length;             /* +0x04 [2] */
    int16_t   progress;           /* +0x06 [2] */
} PACKED;

struct engine_match_resume g_engine_match_resume;

/*
 * **The LZSS decoder's progress**, DGROUP 0x54d6..0x54e0.
 * `decompress_lzss` produces bytes until `count` reaches `size`: it steps
 * `count` with `add`/`adc` and compares the two with `jge` on the high words
 * and `jae` on the low, which is one signed 32-bit compare.
 */
struct engine_lzss_state {
    uint16_t  ring_pos;          /* +0x00 [2] */
    int32_t   count;              /* +0x02 [4]  bytes produced so far */
    int32_t   size;               /* +0x06 [4]  the record's size, copied at the start */
} PACKED;

struct engine_lzss_state g_engine_lzss_state;

/*
 * **The adaptive tree's three tables and the LZSS state**, DGROUP
 * 0x54e0..0x54f2: LZHUF's `son`, `prnt` and `freq`, each a far pointer into
 * the one scratch block - at +0x1c7e, +0x1524 and +0x103c - and cached as
 * its own pointer so the routines below can take it as the table it is. The
 * original reaches an entry as `[bx + si]` with the table's offset in BX and
 * the index doubled by hand; a `uint16_t *` says the same thing and indexes
 * by the entry. Then the flag that makes `decompress_lzss` build its tree
 * and fill its ring, and the ring itself, the front of the block.
 */
struct engine_decompress_cache {
    uint16_t far *son;          /* +0x00 [4]  0x54e0 */
    uint16_t far *prnt;         /* +0x04 [4]  0x54e4 */
    uint16_t far *freq;         /* +0x08 [4]  0x54e8 */
    int16_t   lzss_ready;       /* +0x0c [2]  0x54ec */
    uint8_t far *ring;          /* +0x0e [4]  0x54ee: the record's own block */
} PACKED;

struct engine_decompress_cache g_engine_decompress_cache;

/*
 * 0x20015
 *
 * One bit of the type-3 stream, as 0 or 1. Hand-written: no frame.
 *
 * The buffer is a word at DGROUP 0x3200 filled from the top, with the number of
 * bits in it at 0x3202. Bits come off the **left**: the answer is the sign of
 * the word, and the word is then shifted up by one.
 *
 * A refill happens whenever there are eight or fewer bits, not when the buffer
 * is empty, so there is always a whole byte's headroom. `next_input_byte`
 * answers -1 at the end of the input and only its low byte is taken, so the
 * stream runs on into 0xff bytes rather than stopping - which is what the
 * decoder expects, since it stops on a symbol and not on the input.
 */
int16_t huff_get_bit(void)
{
    int16_t bits;

    if (g_engine_bit_buffer.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - g_engine_bit_buffer.bit_count));
        g_engine_bit_buffer.bits = (int16_t)(((uint16_t)g_engine_bit_buffer.bits) | ax);
        g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count + 8);
    }

    bits = g_engine_bit_buffer.bits;
    g_engine_bit_buffer.bits = (int16_t)(((uint16_t)g_engine_bit_buffer.bits) << 1);
    g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count - 1);

    return (int16_t)(bits < 0 ? 1 : 0);
}

/*
 * 0x20043
 *
 * One symbol, walked out of the tree from the root a bit at a time, then
 * counted: LZHUF's `DecodeChar` with its `update` folded in. Hand-written:
 * the bit reader is `huff_get_bit` inlined, and the update runs with DS on
 * the tables' segment.
 *
 * The update walks from the symbol's leaf up to the root, adding one at each
 * step. When a node's new count passes its right-hand neighbour's the two are
 * swapped - frequencies, sons, and both parent links each, since an internal
 * node's two children share a parent entry. A root count of 0x8000 sends it
 * to `huffman_reconst` first, which is why frequencies never overflow.
 */
int16_t decode_char(void)
{
    uint16_t far *son = g_engine_decompress_cache.son;
    uint16_t far *prnt = g_engine_decompress_cache.prnt;
    uint16_t far *freq = g_engine_decompress_cache.freq;
    uint16_t sym, c;

    sym = son[0x272];               /* the root */
    while (sym < 0x273)
        sym = son[sym + huff_get_bit()];
    sym -= 0x273;

    if (freq[0x272] == 0x8000)
        huffman_reconst();

    c = prnt[sym + 0x273];

    do {
        uint16_t k = (uint16_t)(freq[c] + 1);
        uint16_t l = (uint16_t)(c + 1);

        freq[c] = k;

        if (freq[l] < k) {
            uint16_t i, j;

            while (freq[l] < k)
                l++;
            l--;

            freq[c] = freq[l];
            freq[l] = k;

            i = son[c];
            prnt[i] = l;
            if (i < 0x273)
                prnt[i + 1] = l;

            j = son[l];
            son[l] = i;
            prnt[j] = c;
            if (j < 0x273)
                prnt[j + 1] = c;
            son[c] = j;

            c = l;
        }

        c = prnt[c];
    } while (c != 0);

    return (int16_t)sym;
}

/*
 * 0x2012a
 *
 * Reset the LZSS state for a new stream: clear the flag at DGROUP 0x54ec so
 * `decompress_lzss` builds its tree and fills its ring on the next call,
 * empty the bit buffer at 0x3200, and point the ring at the record's own
 * block.
 *
 * Answers 0, which is what the dispatch that reaches it expects of all of them.
 */
int16_t lzss_reset(void)
{
    g_engine_decompress_cache.lzss_ready = 0;
    g_engine_bit_buffer.bits = 0;
    g_engine_bit_buffer.bit_count = 0;

    g_engine_decompress_cache.ring = g_stream_rec->scratch;

    return 0;
}

/*
 * 0x20155
 *
 * Eight bits of the same stream, as a byte. The same buffer at DGROUP 0x3200
 * and the same refill rule, but the refill is a **loop** here: taking eight
 * bits at once can need two bytes in, where `huff_get_bit` never needs more
 * than one.
 */
int16_t huff_get_byte(void)
{
    uint16_t si;

    while (g_engine_bit_buffer.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - g_engine_bit_buffer.bit_count));
        g_engine_bit_buffer.bits = (int16_t)(((uint16_t)g_engine_bit_buffer.bits) | ax);
        g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count + 8);
    }

    si = ((uint16_t)g_engine_bit_buffer.bits);
    g_engine_bit_buffer.bits = (int16_t)(si << 8);
    g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count - 8);

    return (int16_t)(si >> 8);
}

/*
 * 0x20198
 *
 * Build the adaptive Huffman tree: LZHUF's `StartHuff`.
 *
 * The three tables live inside the record's scratch block, at +0x103c,
 * +0x1524 and +0x1c7e, and their far pointers are cached at DGROUP 0x54e8,
 * 0x54e4 and 0x54e0. They abut exactly: 0x274 words of frequency, then the
 * parent array, then the son array.
 *
 * 0x13a symbols, a tree of 0x273 nodes with the root at 0x272 - 314, 627
 * and 626. Every leaf starts with a frequency of 1 and is hung under a parent
 * built by pairing leaves upward, and the two sentinels at the end - a
 * frequency of 0xffff past the root and a parent of 0 at it - are what stop
 * the update walk later.
 */
void huffman_start(void)
{
    uint16_t far *freq, far *prnt, far *son;
    int16_t i, j;

    /* Three places inside the scratch block, each in the block's own
       segment - the offset steps and the segment does not. */
    {
        uint8_t far *scratch = g_stream_rec->scratch;

        g_engine_decompress_cache.freq = (uint16_t far *)(scratch + 0x103c);
        g_engine_decompress_cache.prnt = (uint16_t far *)(scratch + 0x1524);
        g_engine_decompress_cache.son  = (uint16_t far *)(scratch + 0x1c7e);
    }

    freq = g_engine_decompress_cache.freq;
    prnt = g_engine_decompress_cache.prnt;
    son  = g_engine_decompress_cache.son;

    for (i = 0; i < 0x13a; i++) {
        freq[i] = 1;
        son[i] = (uint16_t)(i + 0x273);
        prnt[i + 0x273] = (uint16_t)i;
    }

    i = 0;
    for (j = 0x13a; j <= 0x272; j++) {
        freq[j] = (uint16_t)(freq[i] + freq[i + 1]);
        son[j] = (uint16_t)i;
        prnt[i + 1] = (uint16_t)j;
        prnt[i] = (uint16_t)j;
        i += 2;
    }

    freq[0x273] = 0xffff;    /* the root's guard, `freq + 0x4e6` */
    prnt[0x272] = 0;
}

/*
 * 0x20285
 *
 * Halve every frequency and rebuild the tree, when the root's count would
 * overflow. LZHUF's `reconst`.
 *
 * Three passes. The leaves are collected to the front with their frequencies
 * rounded up and halved; the internal nodes are rebuilt by pairing and each
 * insertion point found by walking back until the frequency fits; then every
 * parent link is written from the son array.
 *
 * **The shift in the second pass moves twice as many entries as it should.**
 * The count is `(j - k) * 2`, which is right as a *byte* count for a `memmove`
 * and wrong as the element count this loop uses it as, so it walks up to
 * `2j - k` instead of `j`. Nothing is lost by it: every entry above `j` is
 * recomputed by a later turn of the outer loop before anything reads it. It is
 * transcribed as it behaves.
 */
void huffman_reconst(void)
{
    uint16_t far *freq = g_engine_decompress_cache.freq;
    uint16_t far *prnt = g_engine_decompress_cache.prnt;
    uint16_t far *son = g_engine_decompress_cache.son;
    int16_t i, j, k, n;

    j = 0;
    for (i = 0; i < 0x273; i++) {
        if (son[i] >= 0x273) {
            freq[j] = (uint16_t)((freq[i] + 1) >> 1);
            son[j] = son[i];
            j++;
        }
    }

    i = 0;
    for (j = 0x13a; j < 0x273; j++) {
        uint16_t f = (uint16_t)(freq[i] + freq[i + 1]);

        freq[j] = f;

        for (k = (int16_t)(j - 1); freq[k] > f; k--)
            ;
        k++;

        for (n = (int16_t)((j - k) * 2 - 1); n >= 0; n--) {
            freq[k + n + 1] = freq[k + n];
            son[k + n + 1] = son[k + n];
        }

        freq[k] = f;
        son[k] = (uint16_t)i;
        i += 2;
    }

    for (i = 0; i < 0x273; i++) {
        uint16_t c = son[i];

        if (c >= 0x273) {
            prnt[c] = (uint16_t)i;
        } else {
            prnt[c + 1] = (uint16_t)i;
            prnt[c] = (uint16_t)i;
        }
    }
}

/*
 * 0x2040d
 *
 * Decode a match position: twelve bits, of which the top six come out of a
 * table and the bottom six are read raw. LZHUF's `DecodePosition`.
 *
 * A byte is taken first and used to index two 256-entry tables in DGROUP - the
 * code at 0x3283 and the length at 0x3383 - which between them say how many
 * further bits the position needs. Those bits are shifted into the byte one at
 * a time, and only the low six of the result survive; the table's code supplies
 * the rest, shifted up by six.
 */
int16_t decode_position(void)
{
    uint16_t si = (uint16_t)huff_get_byte();
    uint16_t high = (uint16_t)(g_engine_huffman_positions.high[si] << 6);
    int16_t n = (int16_t)(g_engine_huffman_positions.len[si] - 2);

    while (n-- != 0)
        si = (uint16_t)(2 * si + huff_get_bit());

    return (int16_t)(high | (si & 0x3f));
}

/*
 * 0x2045a
 *
 * Decompression type 3: LZSS over a 4096-byte ring, with the literals and match
 * lengths adaptively Huffman coded and the match positions coded by
 * `decode_position`. The busiest of the handlers the table at DGROUP 0x31a4
 * dispatches to.
 *
 * Like `decompress_lzw` it can stop in the middle and be called again, and for
 * the same reason: `emit_byte` answers 0 once the caller's request is full. The
 * flag at DGROUP 0x54ce says a match was interrupted, and the position, length
 * and progress at 0x54d0, 0x54d2 and 0x54d4 are what it comes back to.
 *
 * The first call also initialises: the tree, the ring filled with 0xfc4 spaces
 * - counted in the match position's own word - and the write position set past
 * them, and the total to produce read from the record's +0x12:+0x14. 0x54ec
 * is what makes that happen once.
 *
 * A symbol below 0x100 is a literal. Anything else is a match, whose length is
 * the symbol less 0xfd and whose source is the ring position that far back,
 * masked to twelve bits. Every byte produced goes to the output *and* back into
 * the ring, which is why a match may read bytes it has just written.
 *
 * The answer is always 0.
 */
int16_t decompress_lzss(void)
{
    /* **The ring**, 0x1000 bytes at the front of the record's own block: the
       window the matches are copied out of, indexed everywhere below by a
       position masked to 0xfff. */
    uint8_t far *ring;
    int16_t c = 0;
    int16_t more;

    if (g_engine_decompress_cache.lzss_ready == 0) {
        g_engine_match_resume.interrupted = 0;
        huffman_start();

        ring = g_engine_decompress_cache.ring;
        for (g_engine_match_resume.position = 0;
             g_engine_match_resume.position < 0xfc4;
             g_engine_match_resume.position++)
            ring[g_engine_match_resume.position] = ' ';

        g_engine_lzss_state.ring_pos = 0xfc4;
        g_engine_lzss_state.count = 0;
        g_engine_lzss_state.size = g_stream_rec->size;
        g_engine_decompress_cache.lzss_ready = 1;
    }
    ring = g_engine_decompress_cache.ring;

    while (g_engine_lzss_state.count < g_engine_lzss_state.size) {
        if (g_engine_match_resume.interrupted == 0) {
            c = decode_char();
            if (c < 0x100) {
                more = emit_byte((uint16_t)c);

                ring[g_engine_lzss_state.ring_pos] = (uint8_t)c;
                g_engine_lzss_state.ring_pos = (uint16_t)((g_engine_lzss_state.ring_pos + 1) & 0xfff);
                g_engine_lzss_state.count++;

                if (more == 0)
                    return 0;
            }
        }
        if (c < 0x100 && g_engine_match_resume.interrupted == 0)
            continue;

        if (g_engine_match_resume.interrupted == 0) {
            g_engine_match_resume.position =
                (int16_t)((g_engine_lzss_state.ring_pos - decode_position() - 1) & 0xfff);
            g_engine_match_resume.length = (int16_t)(c + 0xff03);
            g_engine_match_resume.progress = 0;
        }

        g_engine_match_resume.interrupted = 0;

        while (g_engine_match_resume.progress < g_engine_match_resume.length) {
            c = ring[((uint16_t)g_engine_match_resume.position
                      + (uint16_t)g_engine_match_resume.progress) & 0xfff];

            more = emit_byte((uint16_t)c);

            ring[g_engine_lzss_state.ring_pos] = (uint8_t)c;
            g_engine_lzss_state.ring_pos = (uint16_t)((g_engine_lzss_state.ring_pos + 1) & 0xfff);
            g_engine_lzss_state.count++;
            g_engine_match_resume.progress++;

            if (more == 0) {
                g_engine_match_resume.interrupted = 1;
                return 0;
            }
        }
    }
    return 0;
}

