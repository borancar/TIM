
/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The engine under the game**, and the least single-subject file
 * here: four decompressors, the resource archive, palettes and fonts and
 * text, bitmap and polygon drawing, the keyboard, the mouse, the timer, DOS
 * memory, and the interface to the VGA driver in VM.OVL. It is named for
 * the layer it is rather than for one thing it does, because it does not do
 * one thing.
 *
 * This file corresponds to the original's **code segment 1c25**, image
 * 0x1c250..0x1e967 - what is left of it while its modules are split out from
 * the end: palette.c, polygon.c, compbmp.c, timer.c, scale.c, polyclip.c,
 * keyboard.c, text.c, lowlevel.c, vidinit.c, vidload.c, fontload.c,
 * bmpload.c and files.c hold 0x1e967..0x248fe. Functions are in address
 * order and each carries the image offset it was read from.
 */
#include <string.h>

#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * The DGROUP records only this file reads or writes. Each is laid over the
 * DGROUP byte array at the address its macro names, like the shared ones in
 * dgroup.h; they are declared here because nothing else uses them.
 */


/*
 * **Nine bit masks**, DGROUP 0x35b2..0x35bc, `(1 << n) - 1` for n from 0 to 8
 * - the same nine as `ENGINE_LZW_MASKS` - and a zero byte. **Not established**
 * which routine reads these; the port's decoders read the other copy.
 */
struct engine_bit_masks {
    uint8_t   mask[9];            /* +0x00 */
    uint8_t   pad_35bb;           /* +0x09 */
} PACKED;

struct engine_bit_masks ENGINE_BIT_MASKS DGROUP_AT(0x35b2) = { .mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff } };


/*
 * **The bit reader's input window**, DGROUP 0x35bc..0x35c8, 0x0c bytes: `next_lzw_code` has
 * `read_input_block` fill it and takes its codes out of it a byte at a time,
 * from the bit position ENGINE_STREAM keeps. Twelve bytes, up to the mask table.
 */
struct engine_lzw_window {
    uint8_t   window[12];         /* +0x00 [0xc] */
} PACKED;

struct engine_lzw_window ENGINE_LZW_WINDOW DGROUP_AT(0x35bc) = {
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

struct engine_lzw_masks ENGINE_LZW_MASKS DGROUP_AT(0x35c8) = { .mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff } };

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

struct engine_lzw_resume ENGINE_LZW_RESUME DGROUP_AT(0x35d1);

/*
 * **The resource *writer's* state**, DGROUP 0x35d3..0x3600, 0x2d bytes: the
 * image sets 0x138b, 10000 and 1, and two nine-byte runs read as the left and
 * right bit masks, 0xff down to 0 and 0 up to 0xff.
 *
 * **Nothing in the port touches any of it, and that is not an omission.**
 * Every instruction in the image that names these is between 0x1cd2c and
 * 0x1d54e - the gap between `next_lzw_code` and `open_resource` - which is the
 * compressing side, and this port does not transcribe it: `engine_stream`'s
 * `written` says the same thing. That side keeps its own `n_bits` and
 * `maxcode` too, at 0x58b8 and 0x58c8, where the reader's are at 0x589e and
 * 0x58b6.
 *
 * **Three of the four words were two `long`s**, which the image says twice
 * over. `in_count` is incremented `add [0x35e6],1 / adc [0x35e8],0` at
 * 0x1cf68, once per byte the compressor takes, so it is 32 bits at 0x35e6 and
 * the two bytes above it are its high half, not padding. `checkpoint` is
 * written from it at 0x1d2e5 - `dx = [0x35e6] / ax = [0x35e8] / add dx,0x2710
 * / adc ax,0`, then the pair back into 0x35e2 and 0x35e4 - so it is 32 bits at
 * 0x35e2, and the routine that does it is the one 0x1d0b8 calls when the
 * 32-bit comparison of the two says the count has caught up. The image's 10000
 * and 1 are those two `long`s' initialisers, not two words that happen to sit
 * beside zeroes.
 *
 * `hash_size` is the third: 5003, and 0x1d0de adds it to the probe when
 * stepping back off the bottom of the table, which is what a table's size is
 * for. The pad above the counters keeps two more the same routines use - the
 * next free code at 0x35d8, 0x101 at reset and stepped to 0x1000, and the flag
 * at 0x35da that 0x1d0cd tests before letting the table be cleared - unnamed
 * because nothing here needs them and a writer this port does not transcribe
 * is the only thing that would.
 */
struct engine_bit_state {
    uint8_t   pad_35d3[3];        /* +0x00 */
    uint16_t  hash_size;          /* +0x03  0x35d6 */
    uint8_t   pad_35d8[10];       /* +0x05  0x35d8..0x35e1 */
    int32_t   checkpoint;         /* +0x0f  0x35e2  in_count + 10000, last set */
    int32_t   in_count;           /* +0x13  0x35e6  bytes taken in */
    uint8_t   pad_35ea[4];        /* +0x17 */
    uint8_t   left_mask[9];       /* +0x1b  0x35ee */
    uint8_t   right_mask[9];      /* +0x24  0x35f7 */
} PACKED;

struct engine_bit_state ENGINE_BIT_STATE DGROUP_AT(0x35d3) = {
    .hash_size = 0x138b,
    .checkpoint = 10000,
    .in_count = 1,
    .left_mask = { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 },
    .right_mask = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff },
};


/*
 * **The bit buffer the decompressors read through**, DGROUP 0x3600..0x3603, 0x03 bytes.
 */
struct engine_bit_buffer {
    int16_t   bits;               /* +0x00 [2]  filled from the top; bits come off the **left** */
    uint8_t   bit_count;          /* +0x02 [1]  how many are in it */
} PACKED;

struct engine_bit_buffer ENGINE_BIT_BUFFER DGROUP_AT(0x3600);

/*
 * **The Huffman coder's position tables**, DGROUP 0x3603..0x3686: three zero
 * bytes, then sixty-four code lengths and sixty-four codes - the shape of
 * LZHUF's `p_len` and `p_code`, which are the *encoder's* half, next to the
 * decoder's `ENGINE_HUFFMAN_POSITIONS`. **Not established** that anything
 * reads them.
 */
struct engine_huffman_codes {
    uint8_t   pad_3603[3];        /* +0x00 */
    uint8_t   len[64];            /* +0x03  0x3606 */
    uint8_t   code[64];           /* +0x43  0x3646 */
} PACKED;

struct engine_huffman_codes ENGINE_HUFFMAN_CODES DGROUP_AT(0x3603) = {
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

struct engine_huffman_positions ENGINE_HUFFMAN_POSITIONS DGROUP_AT(0x3686) = {
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

struct engine_match_resume ENGINE_MATCH_RESUME DGROUP_BSS(0x58e0);

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

struct engine_lzss_state ENGINE_LZSS_STATE DGROUP_BSS(0x58e8);

/*
 * **The son table's far pointer**, DGROUP 0x5900..0x5904. Two words that are
 * one pointer - the offset at 0x5900 and the segment at 0x5902, which is
 * `far_ptr`'s own order - filed by `huffman_start` beside the other two tables
 * it caches at 0x590a and 0x590e.
 */
struct engine_huffman_tree {
    uint16_t far *son;          /* +0x00 [4] */
} PACKED;

struct engine_huffman_tree ENGINE_HUFFMAN_TREE DGROUP_WAS(0x5900);

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

struct engine_decompress_cache ENGINE_DECOMPRESS_CACHE DGROUP_WAS(0x590a);










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
     * either a DOS block, which starts a segment, or `DG3576.scratch`, which
     * is masked to a paragraph with `& 0xfff0` and then normalised where it is
     * built. Every use here reads that invariant, and the clear loop below
     * writes the offset itself into the dictionary, which is what makes it
     * visible.
     */
    uint8_t far * block = ENGINE_STREAM.scratch;
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

    if (ENGINE_STREAM.resume != 0) {
        cx = (uint16_t)(ENGINE_STREAM.wanted + 1);
        out = (uint8_t far *)ENGINE_STREAM.out;
        back = scratch + (uint16_t)ENGINE_LZW_RESUME.scratch_at;
        copying = (ENGINE_RESOURCE_FLAGS.flags & 0x40) != 0;
        ENGINE_STREAM.resume = 0;
        goto step_back;
    }

    if (ENGINE_STREAM.first_code != 0) {
        /* 0x1ca46 - the first code of a stream is a literal. */
        ENGINE_STREAM.first_code = 0;
        code = next_lzw_code();
        ENGINE_STREAM.oldcode = code;
        ENGINE_STREAM.finchar = code;
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
            uint16_t p = FP_OFF(ENGINE_STREAM.scratch);
            int16_t i;

            for (i = 0; i < 0x100; i++)
                prefix[i] = p;

            ENGINE_STREAM.clear_flg = (int16_t)(p + 1);
            ENGINE_STREAM.free_ent = (int16_t)(((p + 1) << 8) | ((p + 1) >> 8));

            code = next_lzw_code();
            if (code < 0)
                return code;
        }

        in = scratch;
        si = (uint16_t)code;
        ENGINE_STREAM.incode = code;

        if ((int16_t)si >= ENGINE_STREAM.free_ent) {
            *in++ = (uint8_t)((uint16_t)ENGINE_STREAM.finchar);
            si = ((uint16_t)ENGINE_STREAM.oldcode);
        }

        while (si >= 0x100) {
            *in++ = suffix[si];
            si = prefix[si];
        }

        al = suffix[si];
        *in++ = al;
        ENGINE_STREAM.finchar = al;

        cx = (uint16_t)(ENGINE_STREAM.wanted + 1);
        back = in - 1;
        out = (uint8_t far *)ENGINE_STREAM.out;
        copying = (ENGINE_RESOURCE_FLAGS.flags & 0x40) != 0;

        for (;;) {
            al = *back++;
            if (--cx == 0) {
                /* 0x1cbf9 - the caller's request is full mid-string. */
                struct resource *rec;

                ENGINE_STREAM.out = out;
                ENGINE_LZW_RESUME.scratch_at = (int16_t)(back - scratch);

                rec = ENGINE_STREAM.rec;
                {
                    uint16_t n = rec->spill_end;

                    /* 0x1cc0a is `inc word ptr [si+0x1a]`: a carry out of the
                       end lands in the start. */
                    if (++rec->spill_end == 0)
                        rec->spill_start++;
                    ENGINE_STREAM.spill[n] = al;
                }

                ENGINE_STREAM.wanted = 0;
                ENGINE_STREAM.resume = 1;
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
        ENGINE_STREAM.wanted = (int16_t)cx;
        ENGINE_STREAM.out = out;

        if (ENGINE_STREAM.free_ent < 0x1000) {
            uint16_t next = ((uint16_t)ENGINE_STREAM.free_ent);

            prefix[next] = ((uint16_t)ENGINE_STREAM.oldcode);
            ENGINE_STREAM.free_ent = (int16_t)(next + 1);
            suffix[next] = (uint8_t)((uint16_t)ENGINE_STREAM.finchar);
        }

        ENGINE_STREAM.oldcode = ENGINE_STREAM.incode;
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

    if ((int16_t)((uint16_t)ENGINE_STREAM.free_ent) > ENGINE_STREAM.maxcode) {
        uint16_t cx = (uint16_t)(((uint16_t)ENGINE_STREAM.n_bits) + 1);

        ENGINE_STREAM.n_bits = (int16_t)cx;
        if ((uint8_t)cx == 0xc)
            ENGINE_STREAM.maxcode = 0x1000;
        else
            ENGINE_STREAM.maxcode = (int16_t)((1 << (cx & 0xff)) - 1);

        if (ENGINE_STREAM.clear_flg != 0) {
            ENGINE_STREAM.n_bits = 9;
            ENGINE_STREAM.maxcode = 0x1ff;
            ENGINE_STREAM.clear_flg = 0;
        }
    } else if (ENGINE_STREAM.clear_flg != 0) {
        ENGINE_STREAM.n_bits = 9;
        ENGINE_STREAM.maxcode = 0x1ff;
        ENGINE_STREAM.clear_flg = 0;
    } else if (ENGINE_STREAM.bit_pos < ENGINE_STREAM.bit_end) {
        goto extract;
    }

    {
        uint16_t width = ((uint16_t)ENGINE_STREAM.n_bits);
        int16_t n = read_input_block(ENGINE_LZW_WINDOW.window, width);

        if (n <= 0) {
            ENGINE_STREAM.bit_end = n;
            return -1;
        }

        ENGINE_STREAM.bit_pos = 0;
        ENGINE_STREAM.bit_end = (int16_t)((n << 3) - (width - 1));
    }

extract:
    bitpos = ((uint16_t)ENGINE_STREAM.bit_pos);
    bl = (uint8_t)((uint16_t)ENGINE_STREAM.n_bits);
    ch = (uint8_t)bitpos;

    ENGINE_STREAM.bit_pos = (int16_t)(bitpos + ((uint16_t)ENGINE_STREAM.n_bits));

    in = &ENGINE_LZW_WINDOW.window[bitpos >> 3];
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

    ax = ENGINE_LZW_MASKS.mask[bl];
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

/*
 * 0x1ce9d
 *
 * The LZW compressor's start, for a resource opened to write. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near lzw_open_write(void)
{
    not_transcribed("0x1ce9d, lzw_open_write");
    return 0;
}

/*
 * 0x1cf1b
 *
 * The LZW compressor's pass over the spill ring. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near lzw_flush(void)
{
    not_transcribed("0x1cf1b, lzw_flush");
    return 0;
}

/*
 * 0x1d40d
 *
 * The run-length compressor's pass over the spill ring. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near rle_flush(void)
{
    not_transcribed("0x1d40d, rle_flush");
    return 0;
}

/*
 * 0x1d54e
 *
 * Open a resource for reading and answer its slot, or -1.
 *
 * The slot is taken, given the `FILE` it will read from at +6 and that file's
 * current position at +0x1c:+0x1e, and started at offset 5 - past the header
 * this routine is about to consume.
 *
 * `string_contains_r` on the name chooses between two paths, and only one is
 * reached here: the read one, which takes the decompression type from the next
 * byte of the file, gives the slot the memory that type needs, records the
 * caller's size at +0xe:+0x10, reads four more bytes into the record at +0x12,
 * and then calls the type's **reset** through a third pointer in the same
 * fourteen-byte table - at DGROUP 0x3586, which is entry+12.
 *
 * Which resets that dispatch reaches was measured, not read off the table: type
 * 2 to `lzw_reset` and type 3 to `lzss_reset`, and a null entry skips it, which
 * is how types 0 and 1 need nothing done.
 *
 * The other path - the writing one, with its own reset at 0x3584 - is not
 * reached and is left as a stub.
 *
 * The stray `push [bp+0xa]` before the `game_fgetc` is not a leak: the compiler
 * leaves the name on the stack across that call so it can serve as
 * `prepare_resource_slot`'s second argument, and cleans both afterwards.
 */
int16_t open_resource(uint16_t unused, FILE *file, char *name,
                      uint32_t size)
{
    int16_t slot;
    struct resource *rec;
    int16_t type;
    int32_t pos;

    (void)unused;

    slot = open_resource_slot();
    if (slot == -1)
        return -1;

    rec = ENGINE_STREAM.rec;
    rec->data.file = file;

    pos = game_ftell(file);
    rec = ENGINE_STREAM.rec;
    rec->start = (uint32_t)pos;

    rec = ENGINE_STREAM.rec;
    rec->in = 5;

    if (string_contains_r(name) == 0) {
        not_transcribed("0x1d633, opening a resource for writing");
        return -1;
    }

    type = (int16_t)(game_fgetc(file) & 0xff);
    rec = ENGINE_STREAM.rec;
    rec->kind = (uint8_t)type;

    if (prepare_resource_slot(type, name) == -1) {
        game_fseek(file, -1, 1);          /* 0xffff:0xffff is -1 */
        close_resource_slot((uint16_t)slot);
        return -1;
    }

    rec = ENGINE_STREAM.rec;
    rec->end = size;

    game_fread((uint8_t *)&ENGINE_STREAM.rec->size,
               1, 4, file);

    if (ENGINE_RES_HANDLERS.type[type].reset)
        ENGINE_RES_HANDLERS.type[type].reset();

    rec = ENGINE_STREAM.rec;
    rec->kind = (uint8_t)(rec->kind | 0x40);

    rec = ENGINE_STREAM.rec;
    rec->kind = (uint8_t)(rec->kind | 0x20);
    return slot;
}

/*
 * 0x1d798
 *
 * Close a resource. Answers what is left at DGROUP 0x589c, which the writing
 * side counts into and the reading side leaves at zero.
 *
 * Bit 0x40 at DGROUP 0x5888 - set when the resource was opened for reading -
 * sends it straight to `close_resource_slot`. Everything else is the writing
 * side: a flush through a second pointer in the fourteen-byte table, at
 * 0x3582, then the four bytes at the record's +0x12 written back over the
 * header. Not reached on these screens, and left as a stub.
 */
int16_t close_resource(int16_t handle)
{
    if (select_resource(handle) == 0)
        return -1;

    ENGINE_STREAM.written = 0;

    if ((ENGINE_STREAM.kind & 0x40) == 0) {
        not_transcribed("0x1d7c1, flushing a resource opened for writing");
        return -1;
    }

    close_resource_slot((uint16_t)handle);
    return ENGINE_STREAM.written;
}

/*
 * 0x1d868
 *
 * Read a resource into memory. Answers what `resource_read` answered, or -1 if
 * the handle names nothing.
 *
 * Three things happen before the read. The resource is selected, which is what
 * makes DGROUP 0x588a and the rest point at it; the destination is
 * **normalised** into the pair at 0x5894, because the decompressors step it
 * with huge-pointer arithmetic that assumes it is; and bit 0x40 is set at 0x57ba,
 * which is what tells the emitters to write rather than skip.
 */
int16_t read_resource(int16_t handle, uint8_t far * dst, uint16_t count)
{
    if (select_resource(handle) == 0)
        return -1;

    /*
     * OURS, and a refusal rather than a fallback. The pair below is not a way
     * of writing the destination down, it is the **decompression cursor**:
     * fourteen sites walk it, `huge_add_to` steps it, and `decompress_lzw` and
     * `decompress_lzss` renormalise it in place, writing `ENGINE_STREAM.out`.
     * So the destination has to be somewhere the guest can address.
     *
     * A pointer signature accepts a C local where the `seg:off` pair refused
     * one, and that is how this was got wrong before: handed a one-byte frame
     * local, `check_sound` answered a single run of blocks against fifty-five
     * and printed an empty sample list, with nothing saying why. It aborts
     * now, and `dg_is_guest` is the exact question - an address outside the
     * guest's megabyte has no pair, and every address inside it has one.
     */
    if (!dg_is_guest(dst))
        port_abort("read_resource: a destination outside guest memory has no "
                   "seg:off for the decompression cursor at DGROUP 0x5894");

    /*
     * `normalise_far_ptr_far` answers `seg + (off >> 4)` and `off & 0xf`,
     * which is the linear address split at the paragraph - and that is exactly
     * what Borland's `FP_SEG`/`FP_OFF` answer for a pointer, so the round trip
     * is not needed.
     */
    ENGINE_STREAM.out = dst;

    ENGINE_RESOURCE_FLAGS.flags = (uint8_t)(ENGINE_RESOURCE_FLAGS.flags | 0x40);

    return resource_read(FILEREC_PTR((uint16_t)handle), count);
}

/*
 * 0x1d95f
 *
 * The size of a resource, as a far value in DX:AX, or -1 for a handle that
 * names nothing. It is the pair at the record's +0x12:+0x14 - the four bytes
 * `open_resource` read out of the header.
 */
int32_t resource_size(int16_t handle)
{
    struct resource *rec;

    if (select_resource(handle) == 0)
        return -1;

    rec = ENGINE_STREAM.rec;
    return rec->size;
}

/*
 * 0x1d983
 *
 * Seek within a resource, answering the position reached as a far value in
 * DX:AX, or -1 if the handle names nothing.
 *
 * A compressed stream cannot be seeked, so this **skips by decompressing**.
 * The target is worked out from the whence - 0 from the start, 1 from the
 * position at the record's +0x16:+0x18, 2 from the size at +0x12:+0x14 - and
 * then the difference is read in chunks of at most 0x7d00 bytes and thrown
 * away, which is what `read_resource` not having set bit 0x40 at 0x57ba makes
 * happen.
 *
 * A target already reached returns at once. A target *behind* the position
 * needs the stream restarted through `restart_resource_stream`, after which
 * the position is 0 and the target itself is the distance to skip - nothing on
 * these screens seeks backwards, so that path is transcribed and unexercised.
 *
 * A target past the end is clamped to it, and each chunk re-normalises the
 * source pointer at 0x5898 from the record's own far pointer plus its offset.
 */
int32_t resource_seek(int16_t handle, int32_t by, int16_t whence)
{
    struct resource *rec;
    /* The target. Every comparison against it below is **signed** on the high
       word and unsigned on the low, which is one signed 32-bit compare - the
       original's `cmp hi / jg / jl / cmp lo / ja`. */
    int32_t t = 0;

    if (select_resource(handle) == 0)
        return -1;

    rec = ENGINE_STREAM.rec;

    if (whence == 1)
        t = rec->pos;
    else if (whence == 2)
        t = rec->size;

    t += by;

    rec = ENGINE_STREAM.rec;
    if (rec->pos == t)
        return t;

    if (rec->pos > t) {
        /*
         * Backwards. The stream is started over - its answer is not looked at
         * - and the position is then 0, so the target *is* the distance left
         * to skip and needs no subtracting. A target at the start or before it
         * is already reached.
         */
        restart_resource_stream(handle);

        if (t <= 0)
            return 0;
    } else if (rec->size > t) {
        t -= rec->pos;
    } else {
        t = rec->size - rec->pos;
    }

    for (;;) {
        uint16_t n;
        int16_t got;

        if (t >= 0x7d00)
            n = 0x7d00;
        else
            n = (uint16_t)t;

        got = resource_read(FILEREC_PTR((uint16_t)handle), n);

        t -= (uint16_t)got;

        if (t == 0)
            break;

        rec = ENGINE_STREAM.rec;
        ENGINE_STREAM.in = (char huge *)normalise_far_ptr_far(
            (uint8_t huge *)(rec->data.ptr + rec->in));
    }

    rec = ENGINE_STREAM.rec;
    return rec->pos;
}

/*
 * 0x1dae6
 *
 * Put a resource stream back to its beginning, so a seek backwards can then
 * skip forwards to where it wants.
 *
 * Only a stream still marked readable at 0x5888 bit 0x40 can be restarted;
 * anything else answers -1. The decompressor for the current type at DGROUP
 * 0x57be is reset through the third pointer of its fourteen-byte entry at
 * 0x3586 - the same dispatch `open_resource` uses to start one - and a null
 * entry means the type needs nothing done.
 *
 * Then the record goes back to where `open_resource` left it: offset 5, past
 * the header. A stream read from a file seeks that file to its own start at
 * +0x1c plus 5; one read from memory rebuilds the cursor at 0x5898 from the
 * record's block at +6 plus 5. Either way the position at +0x16 and the two
 * bytes at +0x1a - whatever the decompressor had part-read - go to zero.
 */
int16_t restart_resource_stream(int16_t handle)
{
    struct resource *rec;

    if (select_resource(handle) == 0 || (ENGINE_STREAM.kind & 0x40) == 0)
        return -1;

    if (ENGINE_RES_HANDLERS.type[ENGINE_RESOURCE_FLAGS.handler].reset)
        ENGINE_RES_HANDLERS.type[ENGINE_RESOURCE_FLAGS.handler].reset();

    rec = ENGINE_STREAM.rec;
    rec->in = 5;

    rec = ENGINE_STREAM.rec;
    if (rec->kind & 0x20) {
        uint32_t at = rec->start + 5;

        game_fseek(ENGINE_RESOURCE_FLAGS.file, (int32_t)at, 0);
    } else {
        ENGINE_STREAM.in = (char huge *)normalise_far_ptr_far((uint8_t huge *)(rec->data.ptr + 5));
    }

    rec = ENGINE_STREAM.rec;
    rec->pos = 0;

    rec = ENGINE_STREAM.rec;
    rec->spill_start = 0;

    rec = ENGINE_STREAM.rec;
    rec->spill_end = 0;

    return 0;
}

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
    struct resource *rec = ENGINE_STREAM.rec;

    ENGINE_DECOMPRESS_CACHE.lzss_ready = 0;
    ENGINE_BIT_BUFFER.bits = 0;
    ENGINE_BIT_BUFFER.bit_count = 0;

    ENGINE_DECOMPRESS_CACHE.cache_c = rec->scratch;

    return 0;
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

    if (ENGINE_BIT_BUFFER.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - ENGINE_BIT_BUFFER.bit_count));
        ENGINE_BIT_BUFFER.bits = (int16_t)(((uint16_t)ENGINE_BIT_BUFFER.bits) | ax);
        ENGINE_BIT_BUFFER.bit_count = (uint8_t)(ENGINE_BIT_BUFFER.bit_count + 8);
    }

    si = ENGINE_BIT_BUFFER.bits;
    ENGINE_BIT_BUFFER.bits = (int16_t)(((uint16_t)ENGINE_BIT_BUFFER.bits) << 1);
    ENGINE_BIT_BUFFER.bit_count = (uint8_t)(ENGINE_BIT_BUFFER.bit_count - 1);

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

    while (ENGINE_BIT_BUFFER.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - ENGINE_BIT_BUFFER.bit_count));
        ENGINE_BIT_BUFFER.bits = (int16_t)(((uint16_t)ENGINE_BIT_BUFFER.bits) | ax);
        ENGINE_BIT_BUFFER.bit_count = (uint8_t)(ENGINE_BIT_BUFFER.bit_count + 8);
    }

    si = ((uint16_t)ENGINE_BIT_BUFFER.bits);
    ENGINE_BIT_BUFFER.bits = (int16_t)(si << 8);
    ENGINE_BIT_BUFFER.bit_count = (uint8_t)(ENGINE_BIT_BUFFER.bit_count - 8);

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
    struct resource *rec = ENGINE_STREAM.rec;
    uint16_t far *freq, far *prnt, far *son;
    int16_t i, j;

    /* Three places inside the scratch block, each in the block's own
       segment - the offset steps and the segment does not. */
    {
        uint8_t far *scratch = rec->scratch;

        ENGINE_DECOMPRESS_CACHE.cache_a = (uint16_t far *)(scratch + 0x103b);
        ENGINE_DECOMPRESS_CACHE.cache_b = (uint16_t far *)(scratch + 0x1523);
        ENGINE_HUFFMAN_TREE.son         = (uint16_t far *)(scratch + 0x1c7d);
    }

    freq = ENGINE_DECOMPRESS_CACHE.cache_a;
    prnt = ENGINE_DECOMPRESS_CACHE.cache_b;
    son  = ENGINE_HUFFMAN_TREE.son;

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
    uint16_t *freq = ENGINE_DECOMPRESS_CACHE.cache_a;
    uint16_t *prnt = ENGINE_DECOMPRESS_CACHE.cache_b;
    uint16_t *son = ENGINE_HUFFMAN_TREE.son;
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
    uint16_t *freq = ENGINE_DECOMPRESS_CACHE.cache_a;
    uint16_t *prnt = ENGINE_DECOMPRESS_CACHE.cache_b;
    uint16_t *son = ENGINE_HUFFMAN_TREE.son;

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
    uint16_t high = (uint16_t)(ENGINE_HUFFMAN_POSITIONS.high[si] << 6);
    int16_t n = (int16_t)(ENGINE_HUFFMAN_POSITIONS.len[si] - 2);

    while (n-- != 0)
        si = (uint16_t)(2 * si + huff_get_bit());

    return (int16_t)(high | (si & 0x3f));
}

/*
 * 0x1e5ae
 *
 * The LZSS compressor's pass over the spill ring. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near lzss_flush(void)
{
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
    uint8_t far * ring = ENGINE_DECOMPRESS_CACHE.cache_c;
    uint16_t di = 0;
    int16_t si;

    if (ENGINE_DECOMPRESS_CACHE.lzss_ready == 0) {
        struct resource *rec;
        int16_t i;

        ENGINE_MATCH_RESUME.interrupted = 0;
        huffman_start();

        for (i = 0; i < 0xfc4; i++)
            ring[i] = 0x20;

        ENGINE_LZSS_STATE.ring_pos = 0xfc4;
        ENGINE_LZSS_STATE.count = 0;

        rec = ENGINE_STREAM.rec;
        ENGINE_LZSS_STATE.size = rec->size;
        ENGINE_DECOMPRESS_CACHE.lzss_ready = 1;
    }

    for (;;) {
        /* 0x1e91d - is there still something to produce? */
        if (ENGINE_LZSS_STATE.count >= ENGINE_LZSS_STATE.size)
            return 0;

        if (ENGINE_MATCH_RESUME.interrupted == 0) {
            /* 0x1e52d - one symbol, walked out of the tree bit by bit. */
            const uint16_t *son = ENGINE_HUFFMAN_TREE.son;

            di = son[0x272];          /* the root */
            while (di < 0x273)
                di = son[di + huff_get_bit()];

            di -= 0x273;
            huffman_update(di);

            if (di < 0x100) {
                /* 0x1e849 - a literal. */
                si = emit_byte(di);

                ring[ENGINE_LZSS_STATE.ring_pos] = (uint8_t)di;
                ENGINE_LZSS_STATE.ring_pos = (int16_t)((ENGINE_LZSS_STATE.ring_pos + 1) & 0xfff);
                ENGINE_LZSS_STATE.count = (int32_t)((uint32_t)ENGINE_LZSS_STATE.count + 1);

                if (si == 0)
                    return 0;
                continue;
            }

            /* 0x1e89c - a match. */
            {
                uint16_t pos = (uint16_t)decode_position();

                ENGINE_MATCH_RESUME.position = (int16_t)((ENGINE_LZSS_STATE.ring_pos - pos - 1) & 0xfff);
                ENGINE_MATCH_RESUME.length = (int16_t)(di + 0xff03);
                ENGINE_MATCH_RESUME.progress = 0;
            }
        }

        ENGINE_MATCH_RESUME.interrupted = 0;

        while (ENGINE_MATCH_RESUME.progress < ENGINE_MATCH_RESUME.length) {
            uint16_t b = ring[(((uint16_t)ENGINE_MATCH_RESUME.position)
                               + ((uint16_t)ENGINE_MATCH_RESUME.progress))
                              & 0xfff];

            si = emit_byte(b);

            ring[ENGINE_LZSS_STATE.ring_pos] = (uint8_t)b;
            ENGINE_LZSS_STATE.ring_pos = (int16_t)((ENGINE_LZSS_STATE.ring_pos + 1) & 0xfff);
            ENGINE_LZSS_STATE.count = (int32_t)((uint32_t)ENGINE_LZSS_STATE.count + 1);

            ENGINE_MATCH_RESUME.progress = (int16_t)(((uint16_t)ENGINE_MATCH_RESUME.progress) + 1);

            if (si == 0) {
                ENGINE_MATCH_RESUME.interrupted = 1;
                return 0;
            }
        }
    }
}


/*
 * 0x1e940
 *
 * A thunk into the video driver: `ljmp [0x43ba]`, which is `vm_blit_bitmap`.
 * It jumps rather than calls, so the driver returns to this routine's caller
 * and reads that caller's arguments off the stack unchanged.
 */
void blit_bitmap_thunk(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    vm_blit_bitmap(bmp, x, y, mode);
}

/*
 * 0x1e944
 *
 * A thunk into the video driver: `ljmp [0x43ca]`, which is VGA:0x271b. Same
 * arrangement as 0x1e940 - it takes three arguments rather than four, because
 * that is what its caller pushed.
 */
void blit_scaled_thunk(struct bitmap * bmp, int16_t x, int16_t y)
{
    vm_blit_scaled(bmp, x, y);
}

/*
 * 0x1e94c
 *
 * Put the graphics controller back the way the rest of the code expects it,
 * after a routine that changed it to draw. Write mode 2, every bit of the bit
 * mask, every plane of the map mask - the same three registers `vm_blit_bitmap`
 * restores in its epilogue, and the same values.
 *
 * On any adapter but 0x10 it does nothing at all: the whole body is behind that
 * test, and the routine is two `retf`s in a row in the image because the second
 * one is a separate one-byte routine.
 */
void restore_write_mode(void)
{
    if (VMDS.adapter != 0x10)
        return;

    io_out16(PORT_GC_INDEX, 0x0205);            /* write mode 2 */
    io_out16(PORT_GC_INDEX, 0xff08);            /* bit mask: every bit */
    io_out16(PORT_SEQ_INDEX, 0x0f02);           /* map mask: every plane */
}

