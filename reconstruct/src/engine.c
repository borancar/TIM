
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
 * 0x1c250..0x20189 - what is left of it while its modules are split out from
 * the end: compbmp.c, timer.c, scale.c, polyclip.c, keyboard.c, text.c,
 * lowlevel.c, vidinit.c, vidload.c, fontload.c, bmpload.c and files.c hold
 * 0x20189..0x248fe. Functions are in address order and each carries the
 * image offset it was read from.
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
 * **The four resource handlers**, at DGROUP 0x357a, fourteen bytes apiece:
 * `prepare_resource_slot` sizes a slot from the first three words - the near
 * buffer, and the far one for a resource opened to read or otherwise;
 * `resource_read` dispatches on the near offset at +6 (0x3580) and
 * `open_resource` and `restart_resource_stream` on the one at +0xc (0x3586).
 * The two words between are the other two entries of the handler's table
 * and nothing in the port dispatches on them yet. Four is `prepare_resource_slot`'s
 * own bound, and the run ends at 0x35b2.
 */
struct res_handler {
    uint16_t  near_size;          /* +0x00 */
    uint16_t  far_size_read;      /* +0x02  when the mode string has an "r" */
    uint16_t  far_size;           /* +0x04  otherwise */
    uint16_t  read_off;           /* +0x06  the decoder: rle, lzw, ... */
    /* **The writing side's two hooks.** +0x08 is called where the port
       refuses with "flushing a resource opened for writing" - 0x1d7d3, inside
       `close_resource`'s write branch - and +0x0a where it refuses with
       "opening a resource for writing": 0x1d671 tests it for zero and 0x1d681
       calls it. Neither is reached by anything this port does, which is the
       same side of the resource layer `ENGINE_BIT_STATE` belongs to. */
    uint16_t  write_start_off;    /* +0x08 */
    uint16_t  write_open_off;     /* +0x0a */
    uint16_t  reset_off;          /* +0x0c  the restart */
} PACKED;

/* DGROUP 0x357a..0x35b2, 0x38 bytes. */
struct engine_res_handlers {
    struct res_handler type[4];   /* +0x00 [0x38] */
} PACKED;

struct engine_res_handlers ENGINE_RES_HANDLERS DGROUP_AT(0x357a) = {
    .type = {
        [0] = {
            .near_size = 0x0080,
            .read_off = 0x0001,
            .write_start_off = 0x007c,
        },
        [1] = {
            .near_size = 0x0080,
            .read_off = 0x0028,
            .write_start_off = 0x11bd,
        },
        [2] = {
            .near_size = 0x0080,
            .far_size_read = 0x3ab3,
            .far_size = 0x7566,
            .read_off = 0x0812,
            .write_start_off = 0x0ccb,
            .write_open_off = 0x0c4d,
            .reset_off = 0x0720,
        },
        [3] = {
            .near_size = 0x0080,
            .far_size_read = 0x2163,
            .far_size = 0x2163,
            .read_off = 0x25a2,
            .write_start_off = 0x235e,
            .write_open_off = 0x1958,
            .reset_off = 0x19c5,
        },
    },
};

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
 * **What the last palette fade was asked for, and how big a palette is**,
 * DGROUP 0x4460..0x4466, 0x06 bytes.
 */
struct engine_pen {
    uint16_t  fade_weight;          /* +0x00 [2] */
    uint16_t  fade_colour;          /* +0x02 [2] */
    /* **The palette's size in bytes for the depth in use**, out of the table
       below - 0x300 for 256 colours - and what the loader reads and the
       allocator doubles. */
    int16_t   palette_bytes;   /* +0x04 [2] */
} PACKED;

struct engine_pen ENGINE_PEN DGROUP_AT(0x4460) = { .fade_weight = 0x003f, .palette_bytes = 0x0300 };

/*
 * **How many bytes of palette each pixel depth has**, DGROUP 0x4466..0x4486,
 * 0x20 bytes: sixteen words indexed by the pixel shift, which `load_palette`
 * and `set_palette_pointer` file into `ENGINE_PEN.palette_bytes` and then read
 * that many bytes of file into the block. 0x300 is 256 colours of three bytes
 * and 0x30 is sixteen of three; the header called them pointers, which the
 * values are not. Up to 0x4486, where the chunk names begin.
 */
struct engine_palette_sizes {
    int16_t   size[16];           /* +0x00 [0x20] */
} PACKED;

struct engine_palette_sizes ENGINE_PALETTE_SIZES DGROUP_AT(0x4466) = {
    .size = {
        0x0000, 0x0102, 0x0011, 0x0011, 0x0102, 0x0300, 0x0000, 0x0300,
        0x0300, 0x0300, 0x0300, 0x0030, 0x0030, 0x0030, 0x0030, 0x0300,
    },
};

/*
 * **The polygon walker's two chains**, DGROUP 0x44d0..0x44de, 0x0e bytes.
 *
 * `draw_polygon` finds the topmost and the bottommost vertex, and the ring
 * between them is two chains - right from one to the other and left back
 * again. `top_at` and `bottom_at` are those two vertices as byte offsets into
 * the working arrays; `right_count` and `left_count` are how many points each
 * chain got; `at` is the cursor stepping through them an edge at a time and
 * `remaining` the count parked while an edge is drawn, because the original
 * needs the register.
 */
struct engine_polygon_chains {
    uint16_t  top_at;          /* +0x00 [2] */
    uint16_t  bottom_at;          /* +0x02 [2] */
    uint16_t  right_count;          /* +0x04 [2] */
    uint16_t  left_count;          /* +0x06 [2] */
    uint16_t  remaining;          /* +0x08 [2] */
    uint16_t  at;          /* +0x0a [2] */
    uint16_t  chain;              /* +0x0c [2]  0 is the left chain and 2 the right; a computed jmp on it */
} PACKED;

struct engine_polygon_chains ENGINE_POLYGON_CHAINS DGROUP_AT(0x44d0);

/*
 * **The polygon walker's own state**, DGROUP 0x44de..0x44ea, 0x0c bytes.
 *
 * `prev_x`/`prev_y` are the vertex before this one, which is how a repeated
 * point is dropped; `span_seg` is the span buffer's segment; `span_step` is
 * what `poly_walk` adds to its cursor beside the two, so one chain's spans
 * land in the buffer's left column and the other's in the right.
 *
 * The outline is a second pass: when its colour differs from the fill,
 * `outline_count` points of the ring are kept in `closed_x`/`closed_y` to be
 * drawn after; `second_pass` and `second_count` are the other copy, the one
 * kept when the two orderings came out exactly equal.
 */
struct engine_polygon_state {
    int16_t   prev_x;          /* +0x00 [2] */
    int16_t   prev_y;          /* +0x02 [2] */
    uint16_t  span_seg;          /* +0x04 [2] */
    uint16_t  outline_count;          /* +0x06 [2] */
    uint16_t  second_count;          /* +0x08 [2] */
    uint8_t   span_step;          /* +0x0a [1] */
    uint8_t   second_pass;          /* +0x0b [1] */
} PACKED;

struct engine_polygon_state ENGINE_POLYGON_STATE DGROUP_AT(0x44de);

/*
 * **The resource reader's flag bits and its handler index**, DGROUP 0x57ba..0x57bf, 0x05 bytes.
 */
struct engine_resource_flags {
    uint8_t   flags;              /* +0x00 [1]  bit 0x40 makes the copy happen at all; bit 0x20 picks 0x1cd2c */
    uint8_t   pad_57bb[1];        /* +0x01 [1] */
    /* **The stream the reader is on**, the same near pointer to a `file_rec`
       the selected resource holds; every read below goes through it. */
    dg_near_t file_ptr;           /* +0x02 [2] */
    uint8_t   handler;            /* +0x04 [1]  the low five bits of the byte, indexing a table of handlers */
} PACKED;

struct engine_resource_flags ENGINE_RESOURCE_FLAGS DGROUP_BSS(0x57ba);

/*
 * **The staging buffer `read_into_huge` reads through**, DGROUP
 * 0x5788..0x57ba, 0x32 bytes - the 0x32 that routine reads at a time, and
 * exactly the gap between `DG5768`'s end and the flags below. `game_fread`
 * reads into DGROUP, so a destination anywhere else is filled a bufferful at a
 * time through here.
 *
 * Zero in the image, which is why it is `DGROUP_BSS`.
 */
struct engine_read_staging {
    uint8_t   buf[0x32];          /* +0x00 [0x32] */
} PACKED;

struct engine_read_staging ENGINE_READ_STAGING DGROUP_BSS(0x5788);

/*
 * **The open resource streams**, a near pointer each, DGROUP 0x57c0..0x5888,
 * 0xc8 bytes. 0x64 is the bound `select_resource` and `open_resource_slot` test,
 * and a hundred words run exactly to `ENGINE_STREAM` at 0x5888.
 */
struct engine_resource_slots {
    dg_near_t slot_ptr[0x64];     /* +0x00 [0xc8] */
} PACKED;

struct engine_resource_slots ENGINE_RESOURCE_SLOTS DGROUP_BSS(0x57c0);

/*
 * **The compressed-stream reader's state**, DGROUP 0x5888..0x58b8, 0x30 bytes.
 *
 * From `n_bits` on it is the state of Unix `compress`'s LZW decoder, and those
 * fields take that program's names for them.
 */
struct engine_stream {
    uint8_t   kind;               /* +0x00 [1]  the record's kind, copied by select_resource: the low five
                                     bits pick the handler, 0x20 reads from memory, 0x40 means
                                     opened for reading */
    uint8_t   pad_01;             /* +0x01 [1] */
    dg_near_t record_ptr;         /* +0x02 [2]  the record being read */
    uint8_t huge *scratch;      /* +0x04 [4]  the decompressor's block; every
                                     use is a `huge_add` from its base */
    uint16_t  wanted;             /* +0x08 [2]  how many bytes the caller still wants */
    dg_near_t spill_ptr;          /* +0x0a [2]  the record's work_ptr, the buffer a run that does not fit spills into */
    uint8_t huge *out;          /* +0x0c [4]  the decompression output cursor:
                                     `read_resource` normalises the caller's
                                     destination into it and three
                                     decompressors walk it */
    uint8_t huge *in;           /* +0x10 [4]  and where they are reading from */
    int16_t   written;            /* +0x14 [2]  what close_resource answers; the writing side counts into it
                                     - a name that is a guess, that side is not transcribed */
    int16_t   n_bits;             /* +0x16 [2]  the code width: 9 at a reset, one more when free_ent passes maxcode */
    int16_t   free_ent;           /* +0x18 [2]  the next free code: 0x101 at a reset, at most 0x1000 */
    uint8_t   resume;             /* +0x1a [1]  set when a request fills mid-string; the next call resumes at
                                     ENGINE_LZW_RESUME.scratch_at */
    uint8_t   pad_1b;             /* +0x1b [1] */
    int16_t   clear_flg;          /* +0x1c [2]  set by code 0x100; next_lzw_code resets the width and clears it */
    int16_t   oldcode;            /* +0x1e [2]  the previous code, prefix of the entry the next one adds */
    uint8_t huge *de_stack;     /* +0x20 [4]  the scratch block plus 0x3720, where decompress_lzw builds
                                     each string backwards */
    int16_t   finchar;            /* +0x24 [2]  the first byte of the last string, the new entry's suffix */
    uint8_t   first_code;         /* +0x26 [1]  set at a reset: the stream's first code is a literal */
    uint8_t   pad_27;             /* +0x27 [1] */
    int16_t   incode;             /* +0x28 [2]  the code just read, oldcode once its entry is added */
    int16_t   bit_pos;            /* +0x2a [2]  the bit position in the input window, n_bits per code */
    int16_t   bit_end;            /* +0x2c [2]  where whole codes stop in the window: bytes read * 8 less
                                     n_bits - 1; zero or less is the end of the input */
    int16_t   maxcode;            /* +0x2e [2]  the largest code at this width, 0x1000 at twelve bits */
} PACKED;

struct engine_stream ENGINE_STREAM DGROUP_WAS(0x5888);

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
 * 0x1c278
 *
 * Decompression type 1: plain run-length coding, and the first of the three
 * handlers the table at DGROUP 0x3580 dispatches to.
 *
 * Each token is one byte. Bit 7 clear means the low seven bits are a count of
 * literal bytes to copy; bit 7 set means they are a count and the **next** byte
 * is the value to repeat. A token of -1 - the end of the input - stops it, and
 * so does either emitter answering 0, which is how the output side says the
 * caller's request has been filled.
 *
 * Bit 0x20 at DGROUP 0x57ba selects a different routine entirely at 0x1cd2c.
 * That is not reached on these screens and is left as a stub.
 *
 * The answer is always 0 on the run-length path: nothing here reports how much
 * it produced, because 0x1c92b works that out from what is left at 0x5890.
 */
int16_t decompress_rle(void)
{
    int16_t di = 1;

    if ((ENGINE_RESOURCE_FLAGS.flags & 0x20) == 0) {
        not_transcribed("0x1cd2c, the other type-1 path");
        return 0;
    }

    while (di != 0) {
        int16_t si = next_input_byte();

        if (si == -1)
            break;

        if ((si & 0x80) != 0)
            di = emit_fill_run((uint16_t)next_input_byte(),
                               (uint16_t)(si & 0x7f));
        else
            di = emit_literal_run((uint16_t)(si & 0x7f));
    }

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
 * most 0x32 bytes and then `far_memcpy`s them out.
 *
 * The destination is advanced by `huge_add_to` on **its own argument slot** -
 * `lea ax,[bp+4]` - so the far pointer the caller passed by value is stepped in
 * place as a huge pointer; the port's parameter is one, stepped the same way.
 *
 * The loop ends on a short read as well as on the count running out, and the
 * answer is 0 either way: nothing here reports how much it managed.
 */
int16_t read_into_huge(uint8_t far * dst, uint16_t count)
{
    int16_t si = (int16_t)count;
    int16_t di = 1;

    while (si != 0 && di > 0) {
        uint16_t n = (uint16_t)(si > 0x32 ? 0x32 : si);

        di = (int16_t)game_fread(ENGINE_READ_STAGING.buf, 1, n,
                                 FILEREC_PTR(ENGINE_RESOURCE_FLAGS.file_ptr));
        si = (int16_t)(si - di);

        far_memcpy(dst, ENGINE_READ_STAGING.buf, (uint16_t)di);

        dst += di;
    }
    return 0;
}

/*
 * 0x1c3e6
 *
 * Read up to `count` bytes of the compressed stream into DGROUP, and answer how
 * many. This is what fills the bit buffer the LZW code reader works out of.
 *
 * How much is left is `+0xe:+0x10` minus `+0xa:+0xc` on the record at DGROUP
 * 0x588a, and the request is cut down to it - a 32-bit comparison that is
 * signed on the high half and unsigned on the low, which is the compiler
 * comparing a `long` against a zero-extended `int`.
 *
 * The position advances by what will be taken **before** anything is taken, and
 * then the same bit 0x20 at DGROUP 0x5888 that `next_input_byte` uses chooses
 * between the file and a block already in memory - there through
 * `huge_add_to` on the cursor at 0x5898.
 */
int16_t read_input_block(uint8_t *dst, uint16_t count)
{
    uint16_t rec = ENGINE_STREAM.record_ptr;
    /* `sub`/`sbb` on the two halves - one 32-bit subtract, and **signed**,
       because the compare below is. */
    int32_t  rem = (int32_t)(RESOURCE_PTR(rec)->end - RESOURCE_PTR(rec)->in);
    uint32_t n;

    if (rem == 0)
        return 0;

    /* `xor ax,ax / cmp ax,[bp-2] / jl / jg / cmp di,[bp-4] / jbe` at 0x1c416:
       one 32-bit compare of `count` against `rem`, signed on the high word
       and unsigned on the low - the compiler putting a zero-extended `int`
       beside a `long`. **The smaller is taken**, which is the request being
       cut down to what is left.
    
       Written as two halves this read as `rem_hi > 0 || (rem_hi == 0 &&
       count > rem_lo)` choosing `rem`, which has the arms of the `min` the
       wrong way round for every `rem` above 0xffff: `jl` at 0x1c41b goes to
       0x1c42c, which is `mov ax,di` - the count. The two spellings agree
       while a chunk's remainder fits in a word, and nothing here has ever
       given it one that does not. */
    n = ((int32_t)count < rem) ? count : (uint32_t)rem;

    RESOURCE_PTR(rec)->in += n;

    if ((ENGINE_STREAM.kind & 0x20) != 0)
        return (int16_t)game_fread(dst, 1, (uint16_t)n,
                                   FILEREC_PTR(ENGINE_RESOURCE_FLAGS.file_ptr));

    far_memcpy(dst,
               ENGINE_STREAM.in, (uint16_t)n);
    ENGINE_STREAM.in += (int32_t)n;

    return (int16_t)n;
}

/*
 * 0x1c493
 *
 * Deliver a run of `n` literal bytes to the output.
 *
 * The output has two states and DGROUP 0x5890 - what the caller of 0x1c92b
 * still wants - decides between them. While the run fits, the bytes go to the
 * destination huge pointer at 0x5894, which is then stepped by `huge_add_to`,
 * and the answer is 1 meaning "keep going". Once it does not fit they spill
 * into the small buffer at 0x5892 with a count at the record's +0x1a, and the
 * answer is 0.
 *
 * The input position at the record's +0xa:+0xc advances by `n` **before**
 * either, so it counts what was consumed rather than what was delivered.
 *
 * Bit 0x40 at DGROUP 0x57ba is what makes the write happen at all; without it
 * the bytes are skipped in the file instead, by seeking forward over them. That
 * branch is not reached on these screens.
 *
 * The spill writes at the **start** of the buffer, not at the count it has just
 * increased - unlike 0x1c51e, which offsets by the old count. Transcribed as it
 * stands; nothing reaches it here either.
 */
int16_t emit_literal_run(uint16_t n)
{
    uint16_t rec = ENGINE_STREAM.record_ptr;

    RESOURCE_PTR(rec)->in += n;

    if (ENGINE_STREAM.wanted < n) {
        rec = ENGINE_STREAM.record_ptr;
        RESOURCE_PTR(rec)->spill_end = (uint8_t)(RESOURCE_PTR(rec)->spill_end + n);
        read_into_huge(dg_near_ptr(ENGINE_STREAM.spill_ptr), n);
        return 0;
    }

    if ((ENGINE_RESOURCE_FLAGS.flags & 0x40) != 0)
        read_into_huge(ENGINE_STREAM.out, n);
    else
        game_fseek(FILEREC_PTR(ENGINE_RESOURCE_FLAGS.file_ptr), n, 1);

    ENGINE_STREAM.wanted = (int16_t)(ENGINE_STREAM.wanted - n);
    ENGINE_STREAM.out += (int32_t)n;

    return 1;
}

/*
 * 0x1c51e
 *
 * Deliver a run of `n` copies of one byte - the other half of the run-length
 * pair, and the same two output states as `emit_literal_run`, reached the same
 * way and answering the same 1 or 0.
 *
 * Nothing is read here, so nothing advances the input position: that was done
 * by the two `next_input_byte` calls the caller made to get the length and the
 * value.
 *
 * The spill offsets by the record's +0x1a **before** adding to it, which is
 * what 0x1c493's spill does not do. Neither is reached on these screens.
 */
int16_t emit_fill_run(uint16_t value, uint16_t n)
{
    uint16_t rec;

    if (ENGINE_STREAM.wanted < n) {
        rec = ENGINE_STREAM.record_ptr;
        far_memset(dg_near_ptr((uint16_t)(ENGINE_STREAM.spill_ptr + RESOURCE_PTR(rec)->spill_end)),
                   value, (uint32_t)(int16_t)n);
        rec = ENGINE_STREAM.record_ptr;
        RESOURCE_PTR(rec)->spill_end = (uint8_t)(RESOURCE_PTR(rec)->spill_end + n);
        return 0;
    }

    if ((ENGINE_RESOURCE_FLAGS.flags & 0x40) != 0)
        far_memset(ENGINE_STREAM.out, value,
                   (uint32_t)(int16_t)n);

    ENGINE_STREAM.wanted = (int16_t)(ENGINE_STREAM.wanted - n);
    ENGINE_STREAM.out += (int32_t)(int16_t)n;

    return 1;
}

/*
 * 0x1c5a3
 *
 * Deliver one byte - `emit_literal_run` and `emit_fill_run` written for a run
 * of exactly one, with the same two states and the same answers.
 *
 * The spill half is reached here, unlike in the other two, and it reads the
 * record's +0x1a and increments it in one instruction - `mov al,[bx+0x1a]` then
 * `inc byte [bx+0x1a]` - so the byte lands at the old count.
 */
int16_t emit_byte(uint16_t value)
{
    if (ENGINE_STREAM.wanted >= 1) {
        if ((ENGINE_RESOURCE_FLAGS.flags & 0x40) != 0)
            *ENGINE_STREAM.out = (uint8_t)value;

        ENGINE_STREAM.out += 1;
        ENGINE_STREAM.wanted = (int16_t)(ENGINE_STREAM.wanted - 1);
        return 1;
    }

    {
        uint16_t rec = ENGINE_STREAM.record_ptr;
        uint8_t n = RESOURCE_PTR(rec)->spill_end;

        RESOURCE_PTR(rec)->spill_end = (uint8_t)(n + 1);
        dg_near_ptr(ENGINE_STREAM.spill_ptr)[n] = (uint8_t)value;
        return 0;
    }
}

/*
 * 0x1c970
 *
 * Reset the LZW state for a new stream: the whole 0x3aa1-byte block cleared,
 * the code width back to nine with its limit at 0x1ff, the first 0x100 codes
 * made into single-byte strings - prefix zero, suffix the code itself - and the
 * next free code set to 0x101, one past the clear code.
 *
 * Every one of those writes goes through the huge-pointer add, because the
 * block is far and the tables run past a segment: the prefixes at twice the
 * code and the suffixes at 0x2720 plus it.
 *
 * The scratch pointer at DGROUP 0x58a8 is set to 0x3720 into the block, which
 * is where `decompress_lzw` builds each decoded string.
 *
 * The flag at 0x58ae says the next code read is the first, and 0x58a2 that no
 * byte is left over.
 */
void lzw_reset(void)
{
    int16_t i;
    /* The dictionary block; `huge_add` reaches into it from the start. */
    uint8_t *scratch = ENGINE_STREAM.scratch;

    far_memset(scratch, 0, 0x3aa1);

    ENGINE_STREAM.n_bits = 9;
    ENGINE_STREAM.maxcode = (int16_t)((1 << 9) - 1);

    for (i = 0xff; i >= 0; i--) {
        *(uint16_t *)(scratch + (int32_t)i * 2) = 0;

        scratch[(int32_t)i + 0x2720] = (uint8_t)i;
    }

    ENGINE_STREAM.free_ent = 0x101;
    ENGINE_STREAM.clear_flg = 0;
    ENGINE_STREAM.first_code = 1;
    ENGINE_STREAM.resume = 0;
    ENGINE_STREAM.bit_pos = 0;
    ENGINE_STREAM.bit_end = 0;

    /* `huge_add` answers the normalised pair, which is `far_of`'s. */
    ENGINE_STREAM.de_stack = scratch + 0x3720;
}

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
                uint16_t rec;

                ENGINE_STREAM.out = out;
                ENGINE_LZW_RESUME.scratch_at = (int16_t)(back - scratch);

                rec = ENGINE_STREAM.record_ptr;
                {
                    uint16_t n = RESOURCE_PTR(rec)->spill_end;

                    /* 0x1cc0a is `inc word ptr [si+0x1a]`: a carry out of the
                       end lands in the start. */
                    if (++RESOURCE_PTR(rec)->spill_end == 0)
                        RESOURCE_PTR(rec)->spill_start++;
                    dg_near_ptr(ENGINE_STREAM.spill_ptr)[n] = al;
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
 * 0x1c92b
 *
 * Deliver the next `count` bytes of a resource, decompressing as needed, and
 * answer how many were actually delivered.
 *
 * Everything the three decompressors do is bookkeeping around DGROUP 0x5890,
 * which starts as what was asked for and is counted down as bytes are
 * produced; what came out is the difference. `resource_advance` runs first to
 * hand over anything left over from the last call, and again afterwards if the
 * request is still not full.
 *
 * The handler is chosen by the byte at DGROUP 0x57be, indexing a table at
 * DGROUP 0x3580 **fourteen bytes to the entry** with the near offset first.
 * Which entries are live was measured by hooking the indirect call, not read
 * off the table: exactly three, and the port maps their offsets back to the
 * routines rather than pretending to know the whole table.
 *
 * The two words at the record's +0x16:+0x18 are a running total of everything
 * this resource has produced.
 *
 * The first argument is not read. `resource_advance` takes none, and the handle
 * it would name is reached through a global.
 */
int16_t resource_read(FILE *handle, uint16_t count)
{
    uint16_t rec;
    int16_t got;

    (void)handle;

    ENGINE_STREAM.wanted = (int16_t)count;
    resource_advance();

    if (((int16_t)ENGINE_STREAM.wanted) != 0) {
        uint16_t entry = ENGINE_RES_HANDLERS.type[ENGINE_RESOURCE_FLAGS.handler].read_off;

        switch (entry) {
        case 0x0028:                    /* image 0x1c278 */
            decompress_rle();
            break;
        case 0x0812:                    /* image 0x1ca62 */
            decompress_lzw();
            break;
        case 0x25a2:                    /* image 0x1e7f2 */
            decompress_lzss();
            break;
        default:
            not_transcribed("a handler in the table at DGROUP 0x3580");
            break;
        }

        if (((int16_t)ENGINE_STREAM.wanted) != 0)
            resource_advance();
    }

    got = (int16_t)(count - ENGINE_STREAM.wanted);

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->pos += (uint16_t)got;

    return got;
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
 * 0x1c649
 *
 * Select a resource by handle and unpack its entry into the globals the rest of
 * the loader reads. A **** call, so its argument is at [bp+4].
 *
 * The table at DGROUP 0x57c0 holds 0x64 near pointers, one per handle, and a
 * handle outside 0..0x63 or naming a null entry answers 0. Note the low bound
 * is a *signed* test, so a negative handle is rejected rather than wrapping.
 *
 * The entry's byte at +0x20 is both a flag set and a small number: the whole
 * byte goes to 0x5888, its low five bits to 0x57be, and bit 0x20 selects
 * between two ways of finding the data.
 *
 * With bit 0x20 set the resource is already somewhere known and only its +6 is
 * kept. Without it, the data lies at a 32-bit offset from a far pointer - +6/+8
 * is the base and +0xa/+0xc the offset - and the two are added and normalised.
 * The original does that through the runtime's huge-pointer add at 0x0bf0a,
 * which folds the sum down until the offset is a single nibble, and then hands
 * the answer to `normalise_far_ptr_far`; so does the port.
 */
int16_t select_resource(int16_t handle)
{
    uint16_t entry;

    if (handle < 0 || handle >= 0x64)
        return 0;

    entry = ENGINE_RESOURCE_SLOTS.slot_ptr[handle];
    ENGINE_STREAM.record_ptr = (int16_t)entry;
    if (entry == 0)
        return 0;

    ENGINE_STREAM.scratch = RESOURCE_PTR(entry)->scratch;
    ENGINE_STREAM.spill_ptr = (int16_t)RESOURCE_PTR(entry)->work_ptr;

    ENGINE_STREAM.kind = RESOURCE_PTR(entry)->kind;
    ENGINE_RESOURCE_FLAGS.handler = (uint8_t)(ENGINE_STREAM.kind & 0x1f);

    if ((ENGINE_STREAM.kind & 0x20) != 0) {
        ENGINE_RESOURCE_FLAGS.file_ptr = RESOURCE_PTR(entry)->data.file_ptr;
        ENGINE_RESOURCE_FLAGS.flags = 0x20;
        return 1;
    }

    ENGINE_RESOURCE_FLAGS.flags = 0;
    ENGINE_STREAM.in = normalise_far_ptr_far(RESOURCE_PTR(entry)->data.ptr
                                             + RESOURCE_PTR(entry)->in);
    return 1;
}
/*
 * 0x1c389
 *
 * The next byte of whatever is being decompressed, or -1 at the end.
 *
 * There are two sources and the bit 0x20 at DGROUP 0x5888 chooses between them:
 * the resource file through `game_fgetc`, or a block already in memory, walked
 * by the huge pointer at DGROUP 0x5898 with `huge_post_add`.
 *
 * Either way the position at +0xa:+0xc of the record at DGROUP 0x588a is
 * stepped first, and the end test compares it against +0xe:+0x10 - so the count
 * is kept by the record and not by the source.
 *
 * The byte is zero-extended: `cbw` then `and ax,0xff`, which is the compiler
 * widening a `char` and then masking the sign back off.
 */
int16_t next_input_byte(void)
{
    uint16_t rec = ENGINE_STREAM.record_ptr;

    if (RESOURCE_PTR(rec)->in == RESOURCE_PTR(rec)->end)
        return -1;

    RESOURCE_PTR(rec)->in++;

    if ((ENGINE_STREAM.kind & 0x20) != 0)
        return game_fgetc(FILEREC_PTR(ENGINE_RESOURCE_FLAGS.file_ptr));

    /* 0x5898 is `ENGINE_STREAM.in`, the read cursor the decompressors
       walk, stepped as a huge pointer. */
    return (int16_t)(*ENGINE_STREAM.in++ & 0xff);
}

/*
 * 0x1c6e3
 *
 * Does this NUL-terminated string contain the letter `r`?
 *
 * A **** function - it ends in `ret`, not `retf` - so its argument sits at
 * [bp+4] and the string is a DGROUP offset. The loop tests for the terminator
 * before each character and steps the pointer before testing it, so an empty
 * string answers no without reading anything.
 */
int16_t string_contains_r(const char *str)
{
    const char *s = str;

    while (*s != 0) {
        const char *at = s;
        s++;
        if (*at == 'r')
            return 1;
    }
    return 0;
}
/*
 * 0x1c705
 *
 * Free a pointer unless it is null - the whole routine.
 *
 * A **** call taking a near pointer, so its argument sits at [bp+4] rather
 * than the [bp+6] a far routine would use.
 *
 * The free itself is the C runtime's, which the port does not have; see
 * `io_malloc` in io.c for why it refuses rather than pretending.
 *
 * **Measured: the free path is reached on these screens**, so it cannot be
 * verified by exercising only the other branch. It is checked properly now
 * that the runtime's own allocator is transcribed.
 */
void free_if_set(uint16_t p)
{
    if (p != 0)
        io_free(dg_near_ptr(p));
}
/*
 * 0x1c71a
 *
 * Close a resource slot: give back everything it holds and clear its entry in
 * the table at DGROUP 0x57c0. Always answers -1.
 *
 * The record's +0 is a `calloc`ed block and goes through `free_if_set`. Its
 * +2:+4 is a far block from DOS, and that is only freed when there is **no**
 * shared block at DGROUP 0x3576 - when there is, the record was pointed at it
 * rather than given one of its own, and freeing it would take the shared one
 * away.
 *
 * The record itself is freed last, through the same `free_if_set`, and the slot
 * is zeroed whether or not there was anything in it.
 */
int16_t close_resource_slot(uint16_t slot)
{
    uint16_t rec;

    rec = ENGINE_RESOURCE_SLOTS.slot_ptr[slot];
    ENGINE_STREAM.record_ptr = (int16_t)rec;

    if (rec != 0) {
        free_if_set(RESOURCE_PTR(rec)->work_ptr);

        rec = ENGINE_STREAM.record_ptr;
        if (RESOURCE_PTR(rec)->scratch != FAR_NULL_PTR
            && DG3576.scratch == FAR_NULL_PTR)
            dos_free_far(RESOURCE_PTR(rec)->scratch);
    }

    free_if_set(ENGINE_STREAM.record_ptr);
    ENGINE_RESOURCE_SLOTS.slot_ptr[slot] = 0;

    return -1;
}

/*
 * 0x1c783
 *
 * Take a resource slot. Answers its number, or -1 when all hundred are in use
 * or the record cannot be allocated.
 *
 * The table at DGROUP 0x57c0 is a hundred words and a zero means free. The
 * record is 0x21 bytes from `calloc`, so it starts cleared - which matters,
 * because `close_resource_slot` frees whatever pointers it finds in it.
 */
int16_t open_resource_slot(void)
{
    int16_t si;
    struct resource *rec;

    for (si = 0; si < 0x64; si++) {
        if (ENGINE_RESOURCE_SLOTS.slot_ptr[si] == 0)
            break;
    }

    if (si == 0x64)
        return -1;

    rec = (struct resource *)(void *)heap_calloc_far(1, sizeof(struct resource));
    ENGINE_STREAM.record_ptr = dg_near(dgroup, rec);
    if (rec == NULL)
        return -1;

    ENGINE_RESOURCE_SLOTS.slot_ptr[si] = dg_near(dgroup, rec);
    return si;
}

/*
 * 0x1c7d5
 *
 * Give a slot the working memory its decompression type needs. Answers 0, or -1
 * for a type above 3 or an allocation that failed.
 *
 * The sizes come from the same table the handlers do - fourteen bytes to the
 * entry, based at DGROUP **0x357a**, with the near handler offset six bytes
 * into it. That is where the 0x3580 the dispatcher in 0x1c92b uses comes from.
 *
 * Each entry holds two pairs of sizes and `string_contains_r` on the caller's
 * string chooses between them: a match takes the near size from +0 and the far
 * size from +2, no match takes a default near size of 0x80 and the far size
 * from +4.
 *
 * The near part is `calloc`ed. The far part is only allocated when there is no
 * shared block at DGROUP 0x3576; when there is, the record is pointed at that
 * one instead, which is the arrangement `close_resource_slot` has to know
 * about.
 */
int16_t prepare_resource_slot(int16_t type, char *name)
{
    uint16_t near_size = 0x80;
    uint16_t far_size;
    uint16_t rec;

    if (type > 3)
        return -1;

    if (string_contains_r(name) != 0) {
        near_size = ENGINE_RES_HANDLERS.type[type].near_size;
        far_size = ENGINE_RES_HANDLERS.type[type].far_size_read;
    } else {
        far_size = ENGINE_RES_HANDLERS.type[type].far_size;
    }

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->work_ptr = dg_near(dgroup, heap_calloc_far(1, near_size));
    if (RESOURCE_PTR(rec)->work_ptr == 0)
        return -1;

    if (far_size != 0) {
        if (DG3576.scratch != FAR_NULL_PTR) {
            rec = ENGINE_STREAM.record_ptr;
            RESOURCE_PTR(rec)->scratch = DG3576.scratch;
            ENGINE_STREAM.scratch = DG3576.scratch;
        } else {
            uint8_t far *p = dos_alloc_bytes(far_size, 0, 0).ptr;

            rec = ENGINE_STREAM.record_ptr;
            RESOURCE_PTR(rec)->scratch = p;
            ENGINE_STREAM.scratch = p;
        }

        rec = ENGINE_STREAM.record_ptr;
        if (RESOURCE_PTR(rec)->scratch == FAR_NULL_PTR)
            return -1;
    }

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->kind = (uint8_t)type;
    return 0;
}

/*
 * 0x1c8a7
 *
 * Hand over the next run of bytes from the selected resource, up to whatever
 * the caller still wants. A **** call taking nothing: everything is in the
 * globals `select_resource` set up.
 *
 * The entry's bytes at +0x1a and +0x1b are an end and a start, and their
 * difference is what is available. If that is more than the outstanding count
 * at 0x5890, only that much is taken and the start is advanced - by the **low
 * byte** of the count, because the start is a byte and the count is a word.
 * Otherwise the run is exhausted and both bytes are zeroed.
 *
 * The copy happens only with bit 0x40 set at 0x57ba. Without it the counters
 * still move, so a caller can walk a resource without reading it - which is how
 * a seek is done here.
 *
 * The destination far pointer at 0x5894 is advanced by the same amount through
 * the runtime's in-place huge-pointer add at 0x0be82, `huge_add_to`, which the
 * port calls too.
 */
void resource_advance(void)
{
    uint16_t entry = ENGINE_STREAM.record_ptr;
    uint16_t di = RESOURCE_PTR(entry)->spill_start;
    uint16_t si = (uint16_t)(RESOURCE_PTR(entry)->spill_end - di);

    if (si > ENGINE_STREAM.wanted) {
        si = ENGINE_STREAM.wanted;
        RESOURCE_PTR(entry)->spill_start = (uint8_t)(RESOURCE_PTR(entry)->spill_start + (uint8_t)si);
    } else {
        RESOURCE_PTR(entry)->spill_end = 0;
        RESOURCE_PTR(entry)->spill_start = 0;
    }

    if (si == 0)
        return;

    if ((ENGINE_RESOURCE_FLAGS.flags & 0x40) != 0)
        far_memcpy(ENGINE_STREAM.out,
                   dg_near_ptr((uint16_t)(ENGINE_STREAM.spill_ptr + di)), si);

    ENGINE_STREAM.wanted = (int16_t)(ENGINE_STREAM.wanted - si);

    ENGINE_STREAM.out += (int32_t)si;
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
    uint16_t rec;
    int16_t type;
    int32_t pos;

    (void)unused;

    slot = open_resource_slot();
    if (slot == -1)
        return -1;

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->data.file_ptr = dg_near(dgroup, file);

    pos = game_ftell(file);
    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->start = (uint32_t)pos;

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->in = 5;

    if (string_contains_r(name) == 0) {
        not_transcribed("0x1d633, opening a resource for writing");
        return -1;
    }

    type = (int16_t)(game_fgetc(file) & 0xff);
    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->kind = (uint8_t)type;

    if (prepare_resource_slot(type, name) == -1) {
        game_fseek(file, -1, 1);          /* 0xffff:0xffff is -1 */
        close_resource_slot((uint16_t)slot);
        return -1;
    }

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->end = size;

    game_fread((uint8_t *)&RESOURCE_PTR(ENGINE_STREAM.record_ptr)->size,
               1, 4, file);

    {
        uint16_t entry = ENGINE_RES_HANDLERS.type[type].reset_off;

        if (entry != 0) {
            switch (entry) {
            case 0x0720:                /* image 0x1c970 */
                lzw_reset();
                break;
            case 0x19c5:                /* image 0x1dc15 */
                lzss_reset();
                break;
            default:
                not_transcribed("a reset in the table at DGROUP 0x3586");
                break;
            }
        }
    }

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->kind = (uint8_t)(RESOURCE_PTR(rec)->kind | 0x40);

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->kind = (uint8_t)(RESOURCE_PTR(rec)->kind | 0x20);
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
    uint16_t rec;

    if (select_resource(handle) == 0)
        return -1;

    rec = ENGINE_STREAM.record_ptr;
    return RESOURCE_PTR(rec)->size;
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
    uint16_t rec;
    /* The target. Every comparison against it below is **signed** on the high
       word and unsigned on the low, which is one signed 32-bit compare - the
       original's `cmp hi / jg / jl / cmp lo / ja`. */
    int32_t t = 0;

    if (select_resource(handle) == 0)
        return -1;

    rec = ENGINE_STREAM.record_ptr;

    if (whence == 1)
        t = RESOURCE_PTR(rec)->pos;
    else if (whence == 2)
        t = RESOURCE_PTR(rec)->size;

    t += by;

    rec = ENGINE_STREAM.record_ptr;
    if (RESOURCE_PTR(rec)->pos == t)
        return t;

    if (RESOURCE_PTR(rec)->pos > t) {
        /*
         * Backwards. The stream is started over - its answer is not looked at
         * - and the position is then 0, so the target *is* the distance left
         * to skip and needs no subtracting. A target at the start or before it
         * is already reached.
         */
        restart_resource_stream(handle);

        if (t <= 0)
            return 0;
    } else if (RESOURCE_PTR(rec)->size > t) {
        t -= RESOURCE_PTR(rec)->pos;
    } else {
        t = RESOURCE_PTR(rec)->size - RESOURCE_PTR(rec)->pos;
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

        rec = ENGINE_STREAM.record_ptr;
        ENGINE_STREAM.in = normalise_far_ptr_far(RESOURCE_PTR(rec)->data.ptr
                                                 + RESOURCE_PTR(rec)->in);
    }

    rec = ENGINE_STREAM.record_ptr;
    return RESOURCE_PTR(rec)->pos;
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
    uint16_t rec;

    if (select_resource(handle) == 0 || (ENGINE_STREAM.kind & 0x40) == 0)
        return -1;

    {
        uint16_t entry = ENGINE_RES_HANDLERS.type[ENGINE_RESOURCE_FLAGS.handler].reset_off;

        if (entry != 0) {
            switch (entry) {
            case 0x0720:                /* image 0x1c970 */
                lzw_reset();
                break;
            case 0x19c5:                /* image 0x1dc15 */
                lzss_reset();
                break;
            default:
                not_transcribed("a reset in the table at DGROUP 0x3586");
                break;
            }
        }
    }

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->in = 5;

    rec = ENGINE_STREAM.record_ptr;
    if (RESOURCE_PTR(rec)->kind & 0x20) {
        uint32_t at = RESOURCE_PTR(rec)->start + 5;

        game_fseek(FILEREC_PTR(ENGINE_RESOURCE_FLAGS.file_ptr), (int32_t)at, 0);
    } else {
        ENGINE_STREAM.in = normalise_far_ptr_far(RESOURCE_PTR(rec)->data.ptr + 5);
    }

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->pos = 0;

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->spill_start = 0;

    rec = ENGINE_STREAM.record_ptr;
    RESOURCE_PTR(rec)->spill_end = 0;

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
    uint16_t rec = ENGINE_STREAM.record_ptr;

    ENGINE_DECOMPRESS_CACHE.lzss_ready = 0;
    ENGINE_BIT_BUFFER.bits = 0;
    ENGINE_BIT_BUFFER.bit_count = 0;

    ENGINE_DECOMPRESS_CACHE.cache_c = RESOURCE_PTR(rec)->scratch;

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
    uint16_t rec = ENGINE_STREAM.record_ptr;
    uint16_t far *freq, far *prnt, far *son;
    int16_t i, j;

    /* Three places inside the scratch block, each in the block's own
       segment - the offset steps and the segment does not. */
    {
        uint8_t far *scratch = RESOURCE_PTR(rec)->scratch;

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
        uint16_t rec;
        int16_t i;

        ENGINE_MATCH_RESUME.interrupted = 0;
        huffman_start();

        for (i = 0; i < 0xfc4; i++)
            ring[i] = 0x20;

        ENGINE_LZSS_STATE.ring_pos = 0xfc4;
        ENGINE_LZSS_STATE.count = 0;

        rec = ENGINE_STREAM.record_ptr;
        ENGINE_LZSS_STATE.size = RESOURCE_PTR(rec)->size;
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

/*
 * 0x1ec36
 *
 * Fade a run of palette entries towards a colour, through the driver's vector
 * at DGROUP 0x43ce - which is VGA:0x0f57, read out of a running machine
 * because nothing in the image writes that word.
 *
 * The size is filed at DGROUP 0x4460 and 0x4462 first, in the order the
 * arguments are *not* in - the colour to 0x4460 and the weight to 0x4462 -
 * before both are passed on unchanged.
 */
void fade_palette_run(uint16_t first, uint16_t count, uint16_t colour,
                      uint16_t weight)
{
    ENGINE_PEN.fade_weight = weight;
    ENGINE_PEN.fade_colour = colour;

    vm_blend_palette(first, count, colour, (uint8_t)weight);
}

/*
 * 0x1e967
 *
 * Load a palette and keep it. Takes either a resource name or an already-open
 * file record - `file_record_valid` tells the two apart, and a name is opened
 * here and closed again before returning. Answers the far pointer to the block
 * it allocated, and files that pointer in the table at DGROUP 0x3a2e.
 *
 * That table is eleven slots of four bytes, offset at 0x3a2e and segment at
 * 0x3a30, searched from 1 to 9 for one whose four bytes are zero. When none is
 * free the search ends with the index at 10, and the routine files a null
 * pointer into slot 10 and answers null - the table's last slot, which only
 * this store ever reaches. See `VMDS.palettes.blocks` for why that is storage and not
 * an overrun.
 *
 * The palette's length and the chunk name are both chosen by the byte at
 * DGROUP 0x38ad, through the word tables at 0x4466 and 0x44a2. If that chunk is
 * not in the file and DGROUP 0x38af is set, it falls back to a "PAL:AMG:"
 * chunk: 32 Amiga colour words, 4 bits per component, each expanded to the
 * VGA's 6 by masking to four bits and shifting up two. That fills 96 bytes of
 * the 768 and the remaining 672 are zeroed, which is where the 256-entry size
 * comes from.
 *
 * The pointer goes back in **DX:AX**, `mov dx,[bp-8] / mov ax,[bp-0xa]` at
 * 0x1eb5e - the segment in DX and the offset in AX, which is a
 * `struct far_ptr` and is answered as one.
 */
uint8_t far *load_palette(char *name)
{
    FILE *file = (FILE *)name;          /* a handle, or a name to open */
    /* `sub sp,0x34a`, and both halves of it are Borland locals. */
    _Alignas(2) uint8_t buf[0x300];             /* [bp-0x30a] */
    _Alignas(2) int16_t amg[0x20];              /* [bp-0x34a] */

    uint8_t *blk = FAR_NULL_PTR;                 /* [bp-0xa], [bp-8] */
    uint16_t opened;                            /* [bp-2] */
    int16_t di;
    int32_t size;

    ENGINE_PEN.palette_bytes = ENGINE_PALETTE_SIZES.size[(int16_t)VMDS.pixel_shift];

    di = 1;
    for (;;) {
        if (dg_far_ptr(VMDS.palettes.blocks[di]) == FAR_NULL_PTR)
            break;
        if (di >= 0xa)
            break;
        di++;
    }

    if (di < 0xa) {
        int32_t chunk;

        if (file_record_valid(file) == 0) {
            opened = 1;
            file = open_file_record(name);
        } else {
            opened = 0;
        }

        /* Which palette chunk this adapter wants; entry 0 is the empty
           string, which `seek_named_chunk` refuses. */
        chunk = seek_named_chunk(
            file,
            PALCHUNK.by_adapter[(int16_t)VMDS.pixel_shift],
            0);

        if (chunk != -1) {
            size = ENGINE_PEN.palette_bytes;                /* the `cwd` sign-extends it */
            blk = dos_alloc_bytes(size, 0, 0).ptr;

            if (blk != FAR_NULL_PTR) {
                game_fread(buf, 1, (uint16_t)ENGINE_PEN.palette_bytes, file);
                size = ENGINE_PEN.palette_bytes;
                huge_move(blk, buf, (uint32_t)size);
            }
        } else if (VMDS.vga_chunks != 0) {
            chunk = seek_named_chunk(file, PALCHUNK.pal_amg, 0);

            if (chunk != -1
                && game_fread((uint8_t *)amg, 1, 0x40, file) != 0) {
                size = ENGINE_PEN.palette_bytes;
                blk = dos_alloc_bytes(size, 0, 0).ptr;

                if (blk != FAR_NULL_PTR) {
                    uint8_t far *p = blk;                     /* [bp-4] */
                    int16_t si;

                    for (si = 0; si < 0x20; si++) {
                        int16_t w = amg[si];

                        *p++ = (uint8_t)(((w >> 8) & 0xf) << 2);
                        *p++ = (uint8_t)(((w >> 4) & 0xf) << 2);
                        *p++ = (uint8_t)((w & 0xf) << 2);
                    }
                    for (si = 0; si < 0x2a0; si++)
                        *p++ = 0;
                }
            }
        }

        if (opened != 0)
            close_file_record(file);
    }

    /* A block DOS gave out starts a segment, so its pair is the one the
       original files - and a null one is 0000:0000 either way. */
    VMDS.palettes.blocks[di] = far_of(blk);

    return blk;
}

/*
 * 0x1eb6a
 *
 * Set the current palette, or answer the one already set.
 *
 * It first makes sure a buffer exists: the byte at `VMDS.pixel_shift` - the driver's
 * own mode number, sign extended - indexes a table of sizes at DGROUP 0x4466,
 * and if the far pointer at 0x3a2e is still null a block of twice that many
 * bytes is allocated for it.
 *
 * Then, with a null argument it answers the pointer it last stored; with a
 * real one it stores it, hands it to the driver at VGA:0x0f15 through the
 * vector at DGROUP 0x4396, and answers it back.
 *
 * The pointer is passed and answered offset-first, in AX, with the segment in
 * DX - the usual far-pointer convention here.
 */
uint8_t far *set_palette_pointer(uint8_t far * h)
{
    int16_t idx = VMDS.pixel_shift;

    ENGINE_PEN.palette_bytes = ENGINE_PALETTE_SIZES.size[idx];

    if (dg_far_ptr(VMDS.palettes.blocks[0]) == FAR_NULL_PTR
        && ENGINE_PEN.palette_bytes != 0) {
        int16_t bytes = (int16_t)(ENGINE_PEN.palette_bytes * 2);
        /* The high half was `bytes < 0 ? 0xFFFF : 0` - a `cwd`, sign-extending
           the count to the long the allocator takes. */
        VMDS.palettes.blocks[0] = far_of(dos_alloc_bytes((uint32_t)bytes, 0, 0).ptr);
    }

    if (h == FAR_NULL_PTR)
        return PALCHUNK.palette_ptr;

    PALCHUNK.palette_ptr = (h);
    vm_load_palette(h);
    return h;
}

/*
 * 0x20079
 *
 * Fill a rectangle, clipped, and optionally outline it.
 *
 * The fill is done by turning the rectangle into a **span list** - the first
 * row, the row count, then one `x1, x2` pair per row, all identical - and
 * handing it to the driver at VGA:0x0be6 through the vector at DGROUP 0x43b2.
 * That is a general span filler being used for the simplest possible case,
 * which is why a solid rectangle costs one entry per scan line.
 *
 * Clipping shrinks the rectangle in place against the box at DGROUP
 * 0x3894..0x389a, and the original x and y are pushed before that and popped
 * back afterwards, because the outline below wants the unclipped ones.
 *
 * The outline at 0x2013f is **not transcribed**. It draws four lines through
 * 0x21e34 with a stack-reuse trick - each call pushes only the arguments that
 * differ from the last and relies on the rest still being there - which has no
 * honest expression in C without modelling the stack. It is never reached on
 * the intro screens: over 2,108 calls the two border bytes at DGROUP
 * 0x389d/0x389e - the driver's own colour bytes, seen through DGROUP - were
 * always equal, which is the condition that skips it.
 */
void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t right = (int16_t)(x + w - 1);
    int16_t bottom = (int16_t)(y + h - 1);

    if (VMDS.fill_enabled != 0) {
        int16_t cx = x, cy = y, cw = w, ch = h;

        if (VMDS.clip_enabled != 0) {
            int16_t d = (int16_t)(cx - VMDS.clip_left);
            if (d < 0) {
                cx = (int16_t)(cx - d);
                cw = (int16_t)(cw + d);
            }
            d = (int16_t)(cy - VMDS.clip_top);
            if (d < 0) {
                cy = (int16_t)(cy - d);
                ch = (int16_t)(ch + d);
            }
            d = (int16_t)(VMDS.clip_right - right);
            if (d < 0)
                cw = (int16_t)(cw + d);
            d = (int16_t)(VMDS.clip_bottom - bottom);
            if (d < 0)
                ch = (int16_t)(ch + d);
        }

        if (cw > 0 && ch > 0) {
            uint8_t *p = MK_FP(DG4342.span_buffer_seg, 0);
            int16_t n = ch;
            int16_t x2 = (int16_t)(cx + cw - 1);

            *p++ = (uint8_t)(cy & 0xFF);
            *p++ = (uint8_t)((uint16_t)cy >> 8);
            *p++ = (uint8_t)(ch & 0xFF);
            *p++ = (uint8_t)((uint16_t)ch >> 8);
            do {
                *p++ = (uint8_t)(cx & 0xFF);
                *p++ = (uint8_t)((uint16_t)cx >> 8);
                *p++ = (uint8_t)(x2 & 0xFF);
                *p++ = (uint8_t)((uint16_t)x2 >> 8);
            } while (--n);

            vm_fill_spans(MK_FP(DG4342.span_buffer_seg, 0));
        }
    }

    if (VMDS.fill_enabled != 0 && VMDS.fill_colour == VMDS.second_colour)
        return;
    not_transcribed("0x2013f, the rectangle outline");
}

/*
 * 0x20185
 *
 * A thunk - `ljmp [0x44ea]` - and 0x44ea was measured pointing at the
 * instruction after it, `draw_compressed_body` at 0x20189, so the vector
 * exists to be repointed and not to reach another module. The port calls
 * the body. Assembly, and which module it ends or starts is not settled: the
 * `nop` before it pads the assembly `fill_rect` ends, and the body after it
 * is C (compbmp.c).
 */
void draw_compressed_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    draw_compressed_body(bmp, x, y, mode);
}

/*
 * 0x1ebdc
 *
 * Free one far block from the table of eleven at DGROUP 0x3a2e, found by its
 * address rather than by an index: the pair passed in is compared against each
 * entry and the one that matches is freed and zeroed.
 *
 * The walk is slots 1 to 9: entry 0 is `set_palette_pointer`'s, and entry 10
 * only ever holds the null `load_palette` files when the table is full. A null
 * argument does nothing at all.
 */
void free_far_block(uint8_t far * h)
{
    int16_t i;

    if (h == FAR_NULL_PTR)
        return;

    for (i = 1; i < 10; i++) {
        uint8_t far *block = dg_far_ptr(VMDS.palettes.blocks[i]);

        if (block != h)
            continue;

        dos_free_far(block);
        VMDS.palettes.blocks[i] = FAR_NULL;
    }
}

/*
 * 172c:3312, image 0x1f562
 *
 * One edge of the polygon, walked down the scanlines, writing the x it reaches
 * on each into the span buffer.
 *
 * `x` steps by `step` every row plus the carry out of a fractional accumulator
 * that `frac` is added to - a fixed-point DDA rather than Bresenham's error
 * term - and `di` walks the buffer four bytes a row, which is one pair of
 * span ends.
 *
 * **The original is a computed jump into an unrolled loop.** It works out
 * `0x3e28 - 7 * count` and jumps there, landing exactly `count` copies of the
 * seven-byte body from the end - about four hundred of them, which is most of
 * this routine's 3,674 bytes. That is a speed device with no observable
 * difference, so the port writes the loop.
 */
void poly_walk(uint8_t far * span, int16_t x, int16_t frac, int16_t step,
               int16_t acc, int16_t count, uint16_t di)
{
    int16_t di_step = (int8_t)ENGINE_POLYGON_STATE.span_step;

    di = (uint16_t)((di << 2) + ENGINE_POLYGON_CHAINS.chain);

    while (count-- > 0) {
        uint32_t t;

        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);

        t = (uint32_t)(uint16_t)acc + (uint32_t)(uint16_t)frac;
        acc = (int16_t)t;
        x = (int16_t)(x + step + (int16_t)(t >> 16));
    }
}

/*
 * 172c:3015, image 0x1f265 - an edge with no run at all.
 *
 * Both ends have the same x, so every row gets it: no fractional part and no
 * step. The two ends are put in top-to-bottom order first.
 */
void poly_edge_vertical(uint8_t far * span, int16_t x,
                        int16_t y1, int16_t y2)
{
    if (y2 <= y1) {
        int16_t t = y1;

        y1 = y2;
        y2 = t;
    }

    ENGINE_POLYGON_STATE.span_step = 2;
    poly_walk(span, x, 0, 0, 0, (int16_t)(y2 - y1 + 1), (uint16_t)y1);
}

/*
 * 172c:316f, image 0x1f3bf - an edge at exactly 45 degrees.
 *
 * One across for every one down, so again no fractional part: the step is 1 or
 * -1 by which way the x runs.
 */
void poly_edge_diagonal(uint8_t far * span, int16_t x1, int16_t x2,
                        int16_t y1, int16_t y2)
{
    /*
     * The ends are put top-first, so the swap is the one that happens when the
     * first end is *below* the second. Testing it the other way round leaves
     * the ends bottom-first, and the count below - `y2 - y1 + 1` - then comes
     * out zero or negative and the edge is not written at all. A polygon whose
     * side is at exactly 45 degrees loses that side, which is why the fault
     * hid: 45 is a special case of its own, and everything shallower or
     * steeper goes elsewhere.
     */
    if (y1 >= y2) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    ENGINE_POLYGON_STATE.span_step = 2;
    poly_walk(span, x1, 0, (x1 < x2) ? 1 : -1, 0,
              (int16_t)(-(int16_t)(y1 - y2) + 1), (uint16_t)y1);
}

/*
 * 172c:3031, image 0x1f281 - an edge steeper than 45 degrees.
 *
 * Bresenham's, written out: the error starts at `2 * dx - dy`, a step that
 * takes the error non-negative moves x by one and adds `2 * (dx - dy)`, and
 * anything else adds `2 * dx`.
 *
 * The original unrolls the body six times and picks the direction by which way
 * the rows run - `di` four bytes forward or four back - which is the same two
 * loops the port writes as one with a signed step.
 */
void poly_edge_steep(uint8_t far * span, int16_t x1, int16_t x2,
                     int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e1, e2, x, count, sign;
    uint16_t di;

    if (x1 >= x2) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)((y1 << 2) + ENGINE_POLYGON_CHAINS.chain);

    dy = (int16_t)(y1 - y2);
    sign = (dy >= 0) ? 0 : -1;
    if (dy < 0)
        dy = (int16_t)-dy;

    dx = (int16_t)(x2 - x1);
    e2 = (int16_t)(dx * 2);
    x = x1;
    err = (int16_t)(e2 - dy);
    e1 = (int16_t)((int16_t)((dx - dy) * 2) ^ e2);
    count = (int16_t)(dy + 1);

    /*
     * `sign` is the sign the absolute value above threw away: zero means the
     * rows run backwards through the buffer, which the original reaches by a
     * second copy of the whole unrolled body.
     */
    while (count-- > 0) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + (sign == 0 ? -4 : 4));

        /*
         * `cmp bh,0x80 / sbb dx,dx`: all ones when the error is not negative.
         * It steps x, and it picks the increment out of `e1`, which holds
         * the two increments xor'd together: `mask & e1 ^ e2` is
         * `2 * (dx - dy)` on a step and `2 * dx` otherwise. Adding `e1`
         * itself - which this did until 2026-09-27 - put S15's machine 21
         * flips off the original's.
         */
        {
            int16_t mask = (int16_t)(err >= 0 ? -1 : 0);

            x = (int16_t)(x - mask);
            err = (int16_t)(err + ((mask & e1) ^ e2));
        }
    }
}

/*
 * 172c:3196, image 0x1f3e6 - an edge shallower than 45 degrees,
 * the right chain's.
 *
 * Walked *along* rather than down: one row covers several columns, so the loop
 * runs over x and only writes when the error says the row has changed. Unrolled
 * eight times in the original, with the "catch up the error" chain unrolled
 * eight times inside that; a speed device with no observable difference, so the
 * port writes the loop.
 *
 * This is the one the **right** chain takes, where DGROUP 0x44dc is 2: it
 * writes the second slot of each row, `y * 4 + 2`, and counts x **down**.
 *
 * This and `poly_edge_shallow_left` are **not** one routine with a flag: they
 * differ in three
 * places - which way the ends are put in order, which of the row's two slots
 * is written, and which way x counts - and the original has two of them, each
 * its own entry reached by a computed `jmp` on DGROUP 0x44dc. Written as one
 * function with a flag they could not be told apart by the verifier, and the
 * coverage tool counted neither.
 */
void poly_edge_shallow_right(uint8_t far * span, int16_t x1, int16_t x2,
                             int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e, x, count, di_step;
    uint16_t di;

    if ((x1 <= x2)) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)(((y1 << 1) + 1) << 1);   /* the row's second slot */
    di_step = 2;

    dy = (int16_t)(y2 - y1);
    if (dy < 0) {
        dy = (int16_t)-dy;
        di_step = -6;
    }

    count = dy;

    dx = (int16_t)(x2 - x1);
    if (dx > 0)
        dx = (int16_t)-dx;

    err = dx;
    e = (int16_t)((dx + dy) * 2);
    dy = (int16_t)(dy * 2);
    err = (int16_t)(err + dy);

    x = x1;

    /*
     * The first end is written outside the loop, and the *only* thing between
     * it and the second is one step of x and a catch-up: the error is not
     * advanced by `e` yet. Folding that first write into the loop adds an
     * `err += e` that the original does not do there, and on a shallow edge
     * `e` is negative, so the catch-up steps x an extra column or two and
     * every end after it is wrong by that much.
     *
     * `stosw` advances DI by two of its own accord and the original adds its
     * 2 or -6 on top, so a row costs four bytes either way - the same slot of
     * the next row, or of the one before. Adding only the 2 or -6 lands on the
     * *other* slot of the row just written, which leaves the right ends of a
     * whole run of rows unset and the driver fills those to the clip's right
     * edge: a stripe from wherever the polygon was to x=639.
     */
    *(int16_t *)(void *)(span + di) = x;
    di = (uint16_t)(di + 2 + di_step);
    x = (int16_t)(x - 1);

    while (err < 0) {
        x = (int16_t)(x - 1);
        err = (int16_t)(err + dy);
    }

    for (;;) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);
        x = (int16_t)(x - 1);

        if (--count == 0)
            break;

        err = (int16_t)(err + e);
        while (err < 0) {
            x = (int16_t)(x - 1);
            err = (int16_t)(err + dy);
        }
    }
}

/*
 * 172c:3251, image 0x1f4a1 - an edge shallower than 45 degrees,
 * the left chain's.
 *
 * Walked *along* rather than down: one row covers several columns, so the loop
 * runs over x and only writes when the error says the row has changed. Unrolled
 * eight times in the original, with the "catch up the error" chain unrolled
 * eight times inside that; a speed device with no observable difference, so the
 * port writes the loop.
 *
 * This is the one the **left** chain takes, where DGROUP 0x44dc is 0: it
 * writes the first slot of each row, `y * 4`, and counts x **up**.
 *
 * This and `poly_edge_shallow_right` are **not** one routine with a flag: they
 * differ in three
 * places - which way the ends are put in order, which of the row's two slots
 * is written, and which way x counts - and the original has two of them, each
 * its own entry reached by a computed `jmp` on DGROUP 0x44dc. Written as one
 * function with a flag they could not be told apart by the verifier, and the
 * coverage tool counted neither.
 */
void poly_edge_shallow_left(uint8_t far * span, int16_t x1, int16_t x2,
                            int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e, x, count, di_step;
    uint16_t di;

    if ((x1 >= x2)) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)(y1 << 2);                /* the row's first slot */
    di_step = 2;

    dy = (int16_t)(y2 - y1);
    if (dy < 0) {
        dy = (int16_t)-dy;
        di_step = -6;
    }

    count = dy;

    dx = (int16_t)(x2 - x1);
    if (dx > 0)
        dx = (int16_t)-dx;

    err = dx;
    e = (int16_t)((dx + dy) * 2);
    dy = (int16_t)(dy * 2);
    err = (int16_t)(err + dy);

    x = x1;

    /*
     * The first end is written outside the loop, and the *only* thing between
     * it and the second is one step of x and a catch-up: the error is not
     * advanced by `e` yet. Folding that first write into the loop adds an
     * `err += e` that the original does not do there, and on a shallow edge
     * `e` is negative, so the catch-up steps x an extra column or two and
     * every end after it is wrong by that much.
     *
     * `stosw` advances DI by two of its own accord and the original adds its
     * 2 or -6 on top, so a row costs four bytes either way - the same slot of
     * the next row, or of the one before. Adding only the 2 or -6 lands on the
     * *other* slot of the row just written, which leaves the right ends of a
     * whole run of rows unset and the driver fills those to the clip's right
     * edge: a stripe from wherever the polygon was to x=639.
     */
    *(int16_t *)(void *)(span + di) = x;
    di = (uint16_t)(di + 2 + di_step);
    x = (int16_t)(x + 1);

    while (err < 0) {
        x = (int16_t)(x + 1);
        err = (int16_t)(err + dy);
    }

    for (;;) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);
        x = (int16_t)(x + 1);

        if (--count == 0)
            break;

        err = (int16_t)(err + e);
        while (err < 0) {
            x = (int16_t)(x + 1);
            err = (int16_t)(err + dy);
        }
    }
}

/*
 * 172c:2f69, image 0x1f219
 *
 * Draw the outline: one `clip_and_draw_line` per side, from two arrays of
 * points. The second half of the routine is the same again with the vertical
 * window and every y halved, which is the mode where a row is two scan lines -
 * the byte at DGROUP 0x3f78 says which.
 */
void poly_outline(int16_t *xs, int16_t *ys, int16_t n)
{
    if (VMDS.screen.mode_kind == 0) {
        while (n-- > 0) {
            clip_and_draw_line(xs[0], ys[0],
                               xs[1],
                               ys[1]);
            xs++;
            ys++;
        }
        return;
    }

    VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top >> 1);
    VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom >> 1);

    while (n-- > 0) {
        clip_and_draw_line(xs[0], (int16_t)(ys[0] >> 1),
                           xs[1],
                           (int16_t)(ys[1] >> 1));
        xs++;
        ys++;
    }

    VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top << 1);
    VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom << 1);
}

/*
 * 172c:2b9d, image 0x1eded
 *
 * Fill a polygon, and outline it if the two colours differ.
 *
 * Six DGROUP arrays do the work: 0x393c and 0x3964 hold the points as given,
 * 0x398c and 0x39b4 the ones actually used, and 0x39dc and 0x3a04 a closed copy
 * kept for the outline pass - the fill destroys the working pair.
 *
 * With fewer than three points, or with filling off at DGROUP 0x389c, there is
 * nothing to fill and it draws the outline and stops.
 *
 * The fill itself is the classic one. The points are walked backwards, dropping
 * any that repeat the last, and the topmost and bottommost are found on the way
 * - by y, and by x when two share a y, which is what makes the choice
 * unambiguous. If those two turn out to have the same y the whole thing is one
 * horizontal line and goes straight to `clip_and_draw_line`.
 *
 * Otherwise the two edges leaving the top vertex are compared to see which way
 * round the polygon is wound - by slope, and the comparison is done with two
 * divisions rather than a cross product so it cannot overflow - and the arrays
 * are reversed if it is the wrong way. Then the outline is split into a left
 * chain and a right chain, each walked edge by edge into a buffer of span ends,
 * and the whole buffer handed to the driver's span filler in one call.
 *
 * This routine is hand-written assembly - BP is a general register throughout,
 * and most of its 3,674 bytes are an unrolled loop entered by computed jump -
 * so the port follows its registers rather than pretending it was compiled.
 */
void draw_polygon(int16_t n, const int16_t *xs, const int16_t *ys)
{
    uint16_t seg;
    /* The span buffer's first byte. The edge routines step a 16-bit offset
       inside it, exactly as the original steps `di` against a segment, so the
       base and the offset stay apart; `seg` itself is still filed at
       0x44e2 below. */
    uint8_t far * span;
    int16_t ax, bx, cx, dx, si, di, bp;
    int16_t i;

    ENGINE_POLYGON_STATE.span_seg = 0;
    ENGINE_POLYGON_STATE.second_pass = 0;

    if (n >= 0) {
        VMDS.palettes.clip_count = (uint16_t)n;
        for (i = 0; i < n; i++) {
            VMDS.poly_x[i] = xs[i];
            VMDS.poly_y[i] = ys[i];
        }
    }

    if (n < 2)
        goto out;

    if (n == 2) {
        poly_outline(VMDS.poly_x, VMDS.poly_y, 1);
        goto out;
    }

    if (VMDS.fill_enabled == 0) {
        /* Filling is off: close the ring and draw it as lines. */
        n = (int16_t)VMDS.palettes.clip_count;
        VMDS.poly_x[n] = ((uint16_t)VMDS.poly_x[0]);
        VMDS.poly_y[n] = ((uint16_t)VMDS.poly_y[0]);
        poly_outline(VMDS.poly_x, VMDS.poly_y, n);
        goto out;
    }

    if (VMDS.second_colour != VMDS.fill_colour) {
        n = (int16_t)VMDS.palettes.clip_count;
        ENGINE_POLYGON_STATE.outline_count = (uint16_t)n;

        for (i = 0; i < n; i++) {
            VMDS.closed_x[i] = ((uint16_t)VMDS.poly_x[i]);
            VMDS.closed_y[i] = ((uint16_t)VMDS.poly_y[i]);
        }
        VMDS.closed_x[n] = ((uint16_t)VMDS.poly_x[0]);
        VMDS.closed_y[n] = ((uint16_t)VMDS.poly_y[0]);
    }

    if (VMDS.clip_enabled != 0)
        clip_polygon();

    n = (int16_t)VMDS.palettes.clip_count;
    if (n < 2)
        goto out;
    if (n == 2) {
        poly_outline(VMDS.poly_x, VMDS.poly_y, 1);
        goto out;
    }

    si = (int16_t)((n - 1) * 2);
    ENGINE_POLYGON_STATE.prev_y = ((uint16_t)VMDS.poly_y[0]);
    dx = 0x7fff;
    bx = (int16_t)0x8001;
    ENGINE_POLYGON_STATE.prev_x = ((uint16_t)VMDS.poly_x[0]);
    bp = dx;
    cx = bx;
    di = 0;
    ENGINE_POLYGON_CHAINS.top_at = 0;
    ENGINE_POLYGON_CHAINS.bottom_at = 0;

    for (; si >= 0; si -= 2) {
        ax = VMDS.poly_y[si >> 1];

        if (ax == ENGINE_POLYGON_STATE.prev_y
            && VMDS.poly_x[si >> 1] == ENGINE_POLYGON_STATE.prev_x)
            continue;

        ENGINE_POLYGON_STATE.prev_y = ax;
        VMDS.work_y[di >> 1] = ax;

        /*
         * The tie-breaks go opposite ways, and which way is not a matter of
         * taste: the topmost keeps the point *further* right of two on the
         * same row and the bottommost the one further left, so the two chains
         * leave the vertices from opposite corners. Reading either the other
         * way round picks a different vertex to start from, and the fill can
         * still come out right - it did, pixel for pixel - while the arrays
         * the routine leaves behind do not match, which is how it was caught.
         */
        if (ax < dx
            || (ax == dx && VMDS.poly_x[si >> 1] > cx)) {
            ENGINE_POLYGON_CHAINS.top_at = (uint16_t)di;
            dx = ax;
            cx = VMDS.poly_x[si >> 1];
        }

        if (ax > bx
            || (ax == bx && VMDS.poly_x[si >> 1] <= bp)) {
            ENGINE_POLYGON_CHAINS.bottom_at = (uint16_t)di;
            bx = ax;
            bp = VMDS.poly_x[si >> 1];
        }

        ax = VMDS.poly_x[si >> 1];
        ENGINE_POLYGON_STATE.prev_x = ax;
        VMDS.work_x[di >> 1] = ax;
        di += 2;
    }

    if (dx == bx) {
        /* Every point on one row: one line, and nothing to fill. */
        if (VMDS.screen.mode_kind == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top >> 1);
            VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top << 1);
            VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom << 1);
        }
        goto out;
    }

    ax = (int16_t)((uint16_t)di >> 1);
    if (ax < 2)
        goto out;

    if (ax == 2) {
        if (VMDS.screen.mode_kind == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top >> 1);
            VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top << 1);
            VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom << 1);
        }
        goto out;
    }

    cx = di;
    VMDS.palettes.clip_count = (uint16_t)ax;

    /*
     * Which way round is it wound? Compare the slopes of the two edges leaving
     * the top vertex. A zero rise is turned into one with a huge run so the
     * comparison still means something, and the two slopes are compared as
     * quotient-then-remainder rather than by cross-multiplying, because the
     * product would not fit.
     */
    si = (int16_t)ENGINE_POLYGON_CHAINS.top_at;
    di = (int16_t)(si + 2);
    if (di >= cx)
        di = 0;

    dx = (int16_t)(VMDS.work_x[di >> 1] - VMDS.work_x[si >> 1]);
    bp = (int16_t)(VMDS.work_y[di >> 1] - VMDS.work_y[si >> 1]);
    if (bp == 0) {
        bp = 1;
        dx = (dx >= 0) ? 0x7fff : (int16_t)-0x7fff;
    }

    di = (int16_t)(si - 2);
    if (di < 0)
        di = (int16_t)(di + cx);

    ax = (int16_t)(VMDS.work_x[di >> 1] - VMDS.work_x[si >> 1]);
    bx = (int16_t)(VMDS.work_y[di >> 1] - VMDS.work_y[si >> 1]);
    if (bx == 0) {
        bx = 1;
        if (ax < 0) {
            ax = (int16_t)0x8001;
            goto ax_negative;
        }
        ax = (int16_t)-(int16_t)0x8001;
    }

    if (ax < 0)
        goto ax_negative;

    if (dx <= 0)
        goto reverse;
    goto compare;

ax_negative:
    if (dx >= 0)
        goto keep;

    dx = (int16_t)-dx;
    ax = (int16_t)-ax;
    {
        int16_t t = dx;

        dx = ax;
        ax = t;
        t = bx;
        bx = bp;
        bp = t;
    }

compare:
    /*
     * Each edge is a rise over a run and each keeps its own pair: `dx` goes
     * with `bp`, `ax` with `bx`, and the `xchg` above swaps the two pairs
     * whole rather than breaking them up. Dividing one edge's rise by the
     * other's run compares nothing, and the winding then comes out backwards
     * on the polygons where the two slopes happen to straddle - which is a
     * fill built from the wrong chains.
     */
    {
        uint16_t q2 = (uint16_t)ax / (uint16_t)bx;
        uint16_t r2 = (uint16_t)ax % (uint16_t)bx;
        uint16_t q1 = (uint16_t)dx / (uint16_t)bp;
        uint16_t r1 = (uint16_t)dx % (uint16_t)bp;

        if (q1 > q2)
            goto keep;
        if (q1 < q2)
            goto reverse;

        /*
         * Equal whole parts, so the remainders decide - each shifted up a
         * word and divided by its own run again, which is the original's way
         * of getting another sixteen bits of the quotient without a 32-bit
         * divide it has no instruction for.
         */
        {
            uint16_t f1 = (uint16_t)(((uint32_t)r1 << 16) / (uint16_t)bp);
            uint16_t f2 = (uint16_t)(((uint32_t)r2 << 16) / (uint16_t)bx);

            if (f1 < f2)
                goto reverse;
            if (f1 > f2)
                goto keep;
        }
    }

    /* Exactly equal: keep a copy for a second pass and go on. */
    ENGINE_POLYGON_STATE.second_pass = 1;
    ENGINE_POLYGON_STATE.second_count = (uint16_t)cx;
    for (i = 0; i < cx; i += 2) {
        VMDS.closed_x[i >> 1] = VMDS.work_x[i >> 1];
        VMDS.closed_y[i >> 1] = VMDS.work_y[i >> 1];
    }

keep:
    for (i = 0; i < cx; i += 2) {
        VMDS.poly_x[i >> 1] = VMDS.work_x[i >> 1];
        VMDS.poly_y[i >> 1] = VMDS.work_y[i >> 1];
    }
    goto chains;

reverse:
    for (i = 0; i < cx; i += 2) {
        VMDS.poly_x[(cx - 2 - i) >> 1] = VMDS.work_x[i >> 1];
        VMDS.poly_y[(cx - 2 - i) >> 1] = VMDS.work_y[i >> 1];
    }
    ENGINE_POLYGON_CHAINS.top_at = (uint16_t)(cx - 2 - (int16_t)ENGINE_POLYGON_CHAINS.top_at);
    ENGINE_POLYGON_CHAINS.bottom_at = (uint16_t)(cx - 2 - (int16_t)ENGINE_POLYGON_CHAINS.bottom_at);

chains:
    /* The right chain: from the bottom vertex up to the top. */
    dx = VMDS.poly_y[ENGINE_POLYGON_CHAINS.bottom_at >> 1];
    si = (int16_t)ENGINE_POLYGON_CHAINS.top_at;
    di = 0;
    for (;;) {
        VMDS.work_x[di >> 1] = VMDS.poly_x[si >> 1];
        ax = VMDS.poly_y[si >> 1];
        VMDS.work_y[di >> 1] = ax;
        di += 2;
        if (ax >= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    ENGINE_POLYGON_CHAINS.right_count = (uint16_t)((uint16_t)di >> 1);

    /* The left chain: from the top vertex down to the bottom. */
    dx = VMDS.poly_y[ENGINE_POLYGON_CHAINS.top_at >> 1];
    si = (int16_t)ENGINE_POLYGON_CHAINS.bottom_at;
    for (;;) {
        VMDS.work_x[di >> 1] = VMDS.poly_x[si >> 1];
        ax = VMDS.poly_y[si >> 1];
        VMDS.work_y[di >> 1] = ax;
        di += 2;
        if (ax <= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    ENGINE_POLYGON_CHAINS.left_count = (uint16_t)(((uint16_t)di >> 1) - ENGINE_POLYGON_CHAINS.right_count);

    seg = DG4342.span_buffer_seg;
    span = MK_FP(seg, 0);

    ENGINE_POLYGON_CHAINS.chain = 2;
    ENGINE_POLYGON_CHAINS.at = 0;
    ax = (int16_t)ENGINE_POLYGON_CHAINS.right_count;

    for (;;) {
        ax--;
        if (ax == 0) {
            if (ENGINE_POLYGON_CHAINS.chain != 0) {
                ENGINE_POLYGON_CHAINS.at += 2;
                ENGINE_POLYGON_CHAINS.chain = 0;
                ax = (int16_t)ENGINE_POLYGON_CHAINS.left_count;
                continue;
            }
            break;
        }

        ENGINE_POLYGON_CHAINS.remaining = (uint16_t)ax;

        si = (int16_t)ENGINE_POLYGON_CHAINS.at;
        ENGINE_POLYGON_CHAINS.at = (uint16_t)(si + 2);

        {
            int16_t x1 = VMDS.work_x[si >> 1];
            int16_t x2 = VMDS.work_x[(si >> 1) + 1];
            int16_t y1 = VMDS.work_y[si >> 1];
            int16_t y2 = VMDS.work_y[(si >> 1) + 1];
            int16_t adx = (int16_t)(x1 - x2);
            int16_t ady;

            if (adx < 0)
                adx = (int16_t)-adx;

            if (adx == 0) {
                poly_edge_vertical(span, x1, y1, y2);
            } else {
                ady = (int16_t)(y1 - y2);
                if (ady < 0)
                    ady = (int16_t)-ady;

                if (ady == 0) {
                    /* One row: write whichever end the side wants. */
                    int16_t lo = (x1 < x2) ? x1 : x2;
                    int16_t hi = (x1 < x2) ? x2 : x1;
                    uint16_t at = (uint16_t)((y1 << 2) + ENGINE_POLYGON_CHAINS.chain);

                    *(int16_t *)(void *)(span + at) =
                        (ENGINE_POLYGON_CHAINS.chain == 0) ? lo : hi;
                } else if (adx < ady) {
                    poly_edge_steep(span, x1, x2, y1, y2);
                } else if (adx > ady) {
                    /* `cmp [0x44dc],0; jne 0x1f3e6; je 0x1f4a1`. */
                    if (ENGINE_POLYGON_CHAINS.chain != 0)
                        poly_edge_shallow_right(span, x1, x2, y1, y2);
                    else
                        poly_edge_shallow_left(span, x1, x2, y1, y2);
                } else {
                    poly_edge_diagonal(span, x1, x2, y1, y2);
                }
            }
        }

        ax = (int16_t)ENGINE_POLYGON_CHAINS.remaining;
    }

    /* Hand the whole buffer to the driver's span filler in one call. */
    {
        int16_t top = VMDS.poly_y[ENGINE_POLYGON_CHAINS.top_at >> 1];
        int16_t bottom = VMDS.poly_y[ENGINE_POLYGON_CHAINS.bottom_at >> 1];
        /* The list starts four words before the first row's pair: the
           first row and the row count, in the segment below `seg`. */
        /* The paragraph below the buffer - `seg - 1` - is where the list's
           own header lives. */
        uint8_t *spans = span - 0x10 + (uint16_t)((top << 2) + 0x0c);
        int16_t rows = (int16_t)(bottom - top + 1);

        ENGINE_POLYGON_STATE.span_seg = seg;

        spans[0] = (uint8_t)top;
        spans[1] = (uint8_t)((uint16_t)top >> 8);
        spans[2] = (uint8_t)rows;
        spans[3] = (uint8_t)((uint16_t)rows >> 8);

        vm_fill_spans(spans);
    }

    if (VMDS.second_colour != VMDS.fill_colour)
        poly_outline(VMDS.closed_x, VMDS.closed_y, (int16_t)ENGINE_POLYGON_STATE.outline_count);

out:
    if (ENGINE_POLYGON_STATE.second_pass != 0) {
        /* The second pass, for a polygon whose two top edges had one slope. */
        ENGINE_POLYGON_STATE.second_pass = 0;
        cx = (int16_t)ENGINE_POLYGON_STATE.second_count;
        for (i = 0; i < cx; i += 2) {
            VMDS.work_x[i >> 1] = ((uint16_t)VMDS.closed_x[i >> 1]);
            VMDS.work_y[i >> 1] = ((uint16_t)VMDS.closed_y[i >> 1]);
        }
        goto reverse;
    }
}
