/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **LZHUF** - Okumura and Yoshizaki's LZSS with adaptive Huffman coding,
 * type 3 of the resource handlers: the encoder's trees, the bit reader and
 * writer, the Huffman tree and its update, and the encoder and decoder.
 *
 * One module of the original's **code segment 1c25**, image 0x1dba8..0x1e93c,
 * with LZHUF.C's own data in its own order - `getbuf`, `getlen`, `putbuf`,
 * `putlen`, `p_len`, `p_code`, `d_code`, `d_len` - as `_DATA` 0x3600..0x3886,
 * and its `_BSS` 0x58d2..0x591a. **It is compiler output edited by hand and
 * assembled**: some routines are Borland C as it stands (`InitTree` is BC++
 * 2.0 `-mm -k-` byte for byte), some keep a frame and a `jmp` to the
 * epilogue, and others are written by hand - `GetBit`'s `cmp si,0`, `add
 * sp,2` cleanups, and `DecodePosition`, whose `ret` is a jump back into its
 * caller. So it is TASM source, `lzhuf.asm`, with the host's transcription
 * here. Its end is where the video driver's interface begins, vmiface.c,
 * whose data starts a paragraph past this module's.
 */
#include <string.h>

#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The bit buffer the decompressors read through**, DGROUP 0x3600..0x3603, 0x03 bytes.
 */
struct engine_bit_buffer {
    int16_t   bits;               /* +0x00 [2]  filled from the top; bits come off the **left** */
    uint8_t   bit_count;          /* +0x02 [1]  how many are in it */
} PACKED;

struct engine_bit_buffer g_engine_bit_buffer;

/*
 * **The Huffman coder's position tables**, DGROUP 0x3603..0x3686: three zero
 * bytes, then sixty-four code lengths and sixty-four codes - the shape of
 * LZHUF's `p_len` and `p_code`, which are the *encoder's* half, next to the
 * decoder's `g_engine_huffman_positions`. **Not established** that anything
 * reads them.
 */
struct engine_huffman_codes {
    uint8_t   pad_3603[3];        /* +0x00 */
    uint8_t   len[64];            /* +0x03  0x3606 */
    uint8_t   code[64];           /* +0x43  0x3646 */
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
 * **The Huffman position tables**, DGROUP 0x3686..0x3886, 0x200 bytes: for each code byte
 * `decode_position` reads, the high bits of the position at 0x3686 and the
 * length at 0x3786. 256 bytes each, up to 0x3886.
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
 * **An interrupted match, and where it resumes**, DGROUP 0x58e0..0x58e8, 0x08 bytes.
 */
struct engine_match_resume {
    int16_t   interrupted;        /* +0x00 [2]  a match was cut short */
    int16_t   position;           /* +0x02 [2]  and these three are what it comes back to */
    int16_t   length;             /* +0x04 [2] */
    int16_t   progress;           /* +0x06 [2] */
} PACKED;

struct engine_match_resume g_engine_match_resume;

/*
 * **The LZSS decoder's progress**, DGROUP 0x58e8..0x58f2, 0x0a bytes.
 * `decompress_lzss` produces bytes until `count` reaches `size`: the listing
 * steps `count` with `add`/`adc` and compares the two with `jge` on the high
 * words and `jae` on the low, which is one signed 32-bit compare.
 */
struct engine_lzss_state {
    uint16_t  ring_pos;          /* +0x00 [2] */
    int32_t   count;              /* +0x02 [4]  bytes produced so far */
    int32_t   size;               /* +0x06 [4]  the record's size, copied at the start */
} PACKED;

struct engine_lzss_state g_engine_lzss_state;

/*
 * **The son table's far pointer**, DGROUP 0x5900..0x5904. Two words that are
 * one pointer - the offset at 0x5900 and the segment at 0x5902, a far
 * pointer's own order - filed by `huffman_start` beside the other two tables
 * it caches at 0x590a and 0x590e.
 */
struct engine_huffman_tree {
    uint16_t far *son;          /* +0x00 [4] */
} PACKED;

struct engine_huffman_tree g_engine_huffman_tree;

/*
 * **The three cached far pointers and the LZSS init flag**, DGROUP 0x590a..0x591a, 0x10 bytes.
 */
struct engine_decompress_cache {
    /* Three cached far pointers into the decompressor's block, and all three
       tables live in that one block: the original reads them as
       `[bx + si]` with the table's offset in BX, which is why it keeps the
       pairs rather than one pointer. The port takes each as the word table it
       is - see `HUFF_TABLE` - and the pairs stay because the guest stores
       them. */
    uint16_t far *cache_a;      /* +0x00 [4]  the three records' pointers */
    uint16_t far *cache_b;      /* +0x04 [4] */
    uint8_t far *cache_c;       /* +0x08 [4]  the record's own block */
    uint8_t   pad_5916[2];        /* +0x0c [2] */
    int16_t   lzss_ready;         /* +0x0e [2]  cleared so decompress_lzss builds its tree and fills its ring */
} PACKED;

struct engine_decompress_cache g_engine_decompress_cache;

/*
 * 0x1dba8
 *
 * The LZSS compressor's start, for a resource opened to write; assembly. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near lzss_open_write(void)
{
    not_transcribed("0x1dba8, lzss_open_write");
    return 0;
}

/*
 * 0x1dc15
 *
 * Reset the LZSS state for a new stream. Eight instructions: clear the
 * initialised flag at DGROUP 0x5918 so `decompress_lzss` builds its tree and
 * fills its ring on the next call, empty the bit buffer at 0x3600, and point
 * 0x5912 at the record's own block.
 *
 * Answers 0, which is what the dispatch that reaches it expects of all of them.
 */
int16_t lzss_reset(void)
{
    struct resource *rec = g_engine_stream.rec;

    g_engine_decompress_cache.lzss_ready = 0;
    g_engine_bit_buffer.bits = 0;
    g_engine_bit_buffer.bit_count = 0;

    g_engine_decompress_cache.cache_c = rec->scratch;

    return 0;
}

/*
 * 0x1dc3a
 *
 * LZHUF's `InitTree`: the encoder's binary trees emptied, every root and every parent `NIL`. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source in `lzhuf.asm` is the original's.
 */
void init_tree(void)
{
    not_transcribed("0x1dc3a");
}

/*
 * 0x1dc72
 *
 * LZHUF's `InsertNode`: string `r` into the encoder's trees, keeping the longest match. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source in `lzhuf.asm` is the original's.
 */
void insert_node(int16_t r)
{
    not_transcribed("0x1dc72");
    (void)r;
}

/*
 * 0x1de51
 *
 * LZHUF's `DeleteNode`: node `p` out of the encoder's trees. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source in `lzhuf.asm` is the original's.
 */
void delete_node(int16_t p)
{
    not_transcribed("0x1de51");
    (void)p;
}

/*
 * 0x1dfd6
 *
 * One bit of the type-3 stream, as 0 or 1.
 *
 * The buffer is a word at DGROUP 0x3600 filled from the top, with the number of
 * bits in it at 0x3602. Bits come off the **left**: the answer is the sign of
 * the word, and the word is then shifted up by one.
 *
 * A refill happens whenever there are eight or fewer bits, not when the buffer
 * is empty, so there is always a whole byte's headroom. `next_input_byte`
 * answers -1 at the end of the input and only its low byte is taken, so the
 * stream runs on into 0xff bytes rather than stopping - which is what the
 * decoder above expects, since it stops on a symbol and not on the input.
 */
int16_t huff_get_bit(void)
{
    int16_t si;

    if (g_engine_bit_buffer.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - g_engine_bit_buffer.bit_count));
        g_engine_bit_buffer.bits = (int16_t)(((uint16_t)g_engine_bit_buffer.bits) | ax);
        g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count + 8);
    }

    si = g_engine_bit_buffer.bits;
    g_engine_bit_buffer.bits = (int16_t)(((uint16_t)g_engine_bit_buffer.bits) << 1);
    g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count - 1);

    return (int16_t)(si < 0 ? 1 : 0);
}

/*
 * 0x1e00b
 *
 * Eight bits of the same stream, as a byte. The same buffer at DGROUP 0x3600
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
 * **The three tables of the adaptive tree**, each a word array inside the one
 * scratch block - the frequencies at +0x103b, the parents at +0x1523 and the
 * sons at +0x1c7d - and each cached as its own far pointer so the routines
 * below can take it as the table it is. The original reaches an entry as
 * `[bx + si]` with the table's offset in BX and the index doubled by hand;
 * a `uint16_t *` says the same thing and indexes by the entry.
 */

/*
 * 0x1e04e
 *
 * LZHUF's `Putcode`: `len` bits of `code` into the output, a byte at a time through `put_output_byte`. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source in `lzhuf.asm` is the original's.
 */
void huff_putcode(int16_t len, uint16_t code)
{
    not_transcribed("0x1e04e");
    (void)len;
    (void)code;
}

/*
 * 0x1e0b3
 *
 * Build the adaptive Huffman tree that decompression type 3 decodes with.
 *
 * Three arrays live inside the record at DGROUP 0x588a, at +0x103b, +0x1523
 * and +0x1c7d, and their far pointers are cached at DGROUP 0x590a, 0x590e and
 * 0x5900. They abut exactly: 0x274 words of frequency, then the parent array,
 * then the son array.
 *
 * The shape is LZHUF's, and the constants say so: 0x13a symbols, a tree of
 * 0x273 nodes with the root at 0x272 - 314, 627 and 626. Every leaf starts with
 * a frequency of 1 and is hung under a parent built by pairing leaves upward,
 * and the two sentinels at the end - a frequency of 0xffff past the root and a
 * parent of 0 at it - are what stop the update walk later.
 *
 * The identification is from the structure and the constants, not from any
 * source: this is transcribed from the bytes like everything else here.
 */
void huffman_start(void)
{
    struct resource *rec = g_engine_stream.rec;
    uint16_t far *freq, far *prnt, far *son;
    int16_t i, j;

    /* Three places inside the scratch block, each in the block's own
       segment - the offset steps and the segment does not. */
    {
        uint8_t far *scratch = rec->scratch;

        g_engine_decompress_cache.cache_a = (uint16_t far *)(scratch + 0x103b);
        g_engine_decompress_cache.cache_b = (uint16_t far *)(scratch + 0x1523);
        g_engine_huffman_tree.son         = (uint16_t far *)(scratch + 0x1c7d);
    }

    freq = g_engine_decompress_cache.cache_a;
    prnt = g_engine_decompress_cache.cache_b;
    son  = g_engine_huffman_tree.son;

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
 * 0x1e1af
 *
 * Halve every frequency and rebuild the tree, when the root's count would
 * overflow. LZHUF's `reconst`.
 *
 * **It is reached by a jump, not a call.** `huffman_update` jumps here and this
 * ends with a jump back into the middle of it, having built and torn down its
 * own frame in between. The port makes it an ordinary function, which is what
 * it is everywhere except in the two instructions that connect them.
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
    uint16_t *freq = g_engine_decompress_cache.cache_a;
    uint16_t *prnt = g_engine_decompress_cache.cache_b;
    uint16_t *son = g_engine_huffman_tree.son;
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
 * 0x1e338
 *
 * Count one symbol and keep the tree ordered. LZHUF's `update`.
 *
 * The walk starts at the symbol's leaf parent and goes up to the root, adding
 * one at each step. When a node's new count passes its right-hand neighbour's
 * the two are swapped - frequencies, sons, and both parent links each, since an
 * internal node's two children share a parent entry.
 *
 * A root count of 0x8000 sends it to `huffman_reconst` first, which is why
 * frequencies never overflow.
 */
void huffman_update(uint16_t c)
{
    uint16_t *freq = g_engine_decompress_cache.cache_a;
    uint16_t *prnt = g_engine_decompress_cache.cache_b;
    uint16_t *son = g_engine_huffman_tree.son;

    if (freq[0x272] == 0x8000)
        huffman_reconst();

    c = prnt[c + 0x273];

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

}

/*
 * 0x1e445
 *
 * LZHUF's `EncodeChar`: a symbol's code walked up the Huffman tree and put out. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source in `lzhuf.asm` is the original's.
 */
void encode_char(uint16_t c)
{
    not_transcribed("0x1e445");
    (void)c;
}

/*
 * 0x1e4a7
 *
 * LZHUF's `EncodePosition`: a match position, its top six bits coded and the rest raw. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source in `lzhuf.asm` is the original's.
 */
void encode_position(uint16_t c)
{
    not_transcribed("0x1e4a7");
    (void)c;
}

/*
 * 0x1e4e7
 *
 * LZHUF's `EncodeEnd`: whatever bits are left put out. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source in `lzhuf.asm` is the original's.
 */
void encode_end(void)
{
    not_transcribed("0x1e4e7");
}

/*
 * 0x1e561
 *
 * Decode a match position: twelve bits, of which the top six come out of a
 * table and the bottom six are read raw.
 *
 * A byte is taken first and used to index two 256-entry tables in DGROUP - the
 * code at 0x3686 and the length at 0x3786 - which between them say how many
 * further bits the position needs. Those bits are shifted into the byte one at
 * a time, and only the low six of the result survive; the table's code supplies
 * the rest, shifted up by six.
 *
 * **It is reached by a jump, not a call**, and ends with a jump back into
 * 0x1e7f2 rather than a `ret` - the same arrangement as `huffman_reconst`. The
 * port makes it an ordinary function. That is also why the verifier cannot
 * check it on its own: there is no return for the harness to watch for. Every
 * one of `decompress_lzss`'s 226 verified calls runs it.
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
 * 0x1e5ae
 *
 * The LZSS compressor's pass over the spill ring. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near lzss_flush(int16_t final)
{
    (void)final;
    not_transcribed("0x1e5ae, lzss_flush");
    return 0;
}

/*
 * 0x1e7f2
 *
 * Decompression type 3: LZSS over a 4096-byte ring, with the literals and match
 * lengths adaptively Huffman coded and the match positions coded by
 * `decode_position`. The third and busiest of the handlers the table at DGROUP
 * 0x3580 dispatches to.
 *
 * Like `decompress_lzw` it can stop in the middle and be called again, and for
 * the same reason: `emit_byte` answers 0 once the caller's request is full. The
 * flag at DGROUP 0x58e0 says a match was interrupted, and the position, length
 * and progress at 0x58e2, 0x58e4 and 0x58e6 are what it comes back to.
 *
 * The first call also initialises: the tree, the ring filled with 0xfc4 spaces
 * and the write position set past them, and the total to produce read from the
 * record's +0x12:+0x14. 0x5918 is what makes that happen once.
 *
 * A symbol below 0x100 is a literal. Anything else is a match, whose length is
 * the symbol less 0xfd and whose source is the ring position that far back,
 * masked to twelve bits. Every byte produced goes to the output *and* back into
 * the ring, which is why a match may read bytes it has just written.
 *
 * Two blocks of this routine are placed out of line by the compiler and are
 * folded back in here: the symbol decode at 0x1e52d, which walks the tree from
 * the root a bit at a time, and 0x1e561 above.
 *
 * The answer is always 0.
 */
int16_t decompress_lzss(void)
{
    /* **The ring**, 0x1000 bytes at the front of the record's own block: the
       window the matches are copied out of, indexed everywhere below by a
       position masked to 0xfff. */
    uint8_t far * ring = g_engine_decompress_cache.cache_c;
    uint16_t di = 0;
    int16_t si;

    if (g_engine_decompress_cache.lzss_ready == 0) {
        struct resource *rec;
        int16_t i;

        g_engine_match_resume.interrupted = 0;
        huffman_start();

        for (i = 0; i < 0xfc4; i++)
            ring[i] = 0x20;

        g_engine_lzss_state.ring_pos = 0xfc4;
        g_engine_lzss_state.count = 0;

        rec = g_engine_stream.rec;
        g_engine_lzss_state.size = rec->size;
        g_engine_decompress_cache.lzss_ready = 1;
    }

    for (;;) {
        /* 0x1e91d - is there still something to produce? */
        if (g_engine_lzss_state.count >= g_engine_lzss_state.size)
            return 0;

        if (g_engine_match_resume.interrupted == 0) {
            /* 0x1e52d - one symbol, walked out of the tree bit by bit. */
            const uint16_t *son = g_engine_huffman_tree.son;

            di = son[0x272];          /* the root */
            while (di < 0x273)
                di = son[di + huff_get_bit()];

            di -= 0x273;
            huffman_update(di);

            if (di < 0x100) {
                /* 0x1e849 - a literal. */
                si = emit_byte(di);

                ring[g_engine_lzss_state.ring_pos] = (uint8_t)di;
                g_engine_lzss_state.ring_pos = (int16_t)((g_engine_lzss_state.ring_pos + 1) & 0xfff);
                g_engine_lzss_state.count = (int32_t)((uint32_t)g_engine_lzss_state.count + 1);

                if (si == 0)
                    return 0;
                continue;
            }

            /* 0x1e89c - a match. */
            {
                uint16_t pos = (uint16_t)decode_position();

                g_engine_match_resume.position = (int16_t)((g_engine_lzss_state.ring_pos - pos - 1) & 0xfff);
                g_engine_match_resume.length = (int16_t)(di + 0xff03);
                g_engine_match_resume.progress = 0;
            }
        }

        g_engine_match_resume.interrupted = 0;

        while (g_engine_match_resume.progress < g_engine_match_resume.length) {
            uint16_t b = ring[(((uint16_t)g_engine_match_resume.position)
                               + ((uint16_t)g_engine_match_resume.progress))
                              & 0xfff];

            si = emit_byte(b);

            ring[g_engine_lzss_state.ring_pos] = (uint8_t)b;
            g_engine_lzss_state.ring_pos = (int16_t)((g_engine_lzss_state.ring_pos + 1) & 0xfff);
            g_engine_lzss_state.count = (int32_t)((uint32_t)g_engine_lzss_state.count + 1);

            g_engine_match_resume.progress = (int16_t)(((uint16_t)g_engine_match_resume.progress) + 1);

            if (si == 0) {
                g_engine_match_resume.interrupted = 1;
                return 0;
            }
        }
    }
}

