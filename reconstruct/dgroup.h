/*
 * The original's DGROUP: one 64 KB data segment.
 *
 * It is modelled as a **byte array**, not as a set of C globals, because the
 * game uses *near pointers* - a word in DGROUP holding an offset into DGROUP,
 * dereferenced as `[bx + 0x22]`. Named globals cannot express that; an array
 * can, and it is what the original actually has.
 *
 * Named variables are macros over the array rather than storage of their own,
 * so a name and a pointer dereference reach the same byte. Where a name is a
 * guess it says so; the offsets are read from the disassembly and are not.
 *
 * DGROUP is at image 0x2d3c0, so a DGROUP offset plus that is an image offset.
 *
 * This arrangement also makes verification stronger: tools/verify.py seeds the
 * **whole** segment before a call and compares the whole of it afterwards, so
 * a routine that touches state nobody declared is caught rather than missed.
 */
#ifndef DGROUP_H
#define DGROUP_H

#include <stdint.h>

/*
 * DGROUP is a **window into the guest's address space**, not storage of its
 * own. The game holds far pointers - `les bx, [0x546c]` - into blocks DOS gave
 * it, which are outside DGROUP entirely, so a DGROUP-only array cannot express
 * them. Real mode is a flat megabyte with segments as sixteen-byte units, and
 * that is what this models.
 */
#define GUEST_MEM_BYTES 0x100000
#define DGROUP_BYTES    0x10000

extern uint8_t  guest_mem[GUEST_MEM_BYTES];
extern uint32_t dgroup_base;        /* linear address of DGROUP */

#define dgroup      (guest_mem + dgroup_base)

/*
 * **Every one of these is `volatile`, and that is not caution.**
 *
 * The guest's memory is shared with the timer handler, which runs on a thread
 * of its own because that is what an interrupt is - see io.c. The game waits
 * for it by spinning on a word: `wait_and_latch_frame` sits on DGROUP 0x5754
 * until the handler sets it, and touches nothing else while it does. Without
 * `volatile` the compiler is entitled to read that word once, prove the loop
 * changes nothing, and spin for ever - which is exactly what the port did, and
 * it looked like the intro simply never advancing.
 *
 * It costs speed in the blitters, where these are read in tight loops. It is
 * still the right trade: the alternative is a port that works at -O0 and hangs
 * at -O2.
 */
#define DG8(off)    (*(volatile uint8_t  *)(dgroup + (off)))
#define DGS8(off)   (*(volatile int8_t   *)(dgroup + (off)))
#define DG16(off)   (*(volatile int16_t  *)(dgroup + (off)))
#define DG32(off)   (*(volatile int32_t  *)(dgroup + (off)))
#define DGU16(off)  (*(volatile uint16_t *)(dgroup + (off)))

/* A far pointer: segment and offset, as the hardware forms an address. */
#define FAR_PTR(seg, off) \
    (guest_mem + (((uint32_t)(uint16_t)(seg)) << 4) + (uint16_t)(off))
#define FAR8(seg, off)    (*(volatile uint8_t  *)FAR_PTR(seg, off))
#define FAR16(seg, off)   (*(volatile int16_t  *)FAR_PTR(seg, off))
#define FARU16(seg, off)  (*(volatile uint16_t *)FAR_PTR(seg, off))

/* A far pointer *stored* in DGROUP: offset first, then segment. */
#define DG_FAR_OFF(o)     DGU16(o)
#define DG_FAR_SEG(o)     DGU16((o) + 2)

/*
 * Set to 1 by the game's INT 08h handler by way of the code at image 0x0aa08,
 * cleared at 0x0ab17. The main loop at 0x0aaca spins until it is set, so it
 * paces the frame. The name is a guess from that behaviour.
 */

/* Read by the frame-presentation routine at 0x081cc to choose between three
 * paths. Names are guesses from that use. */

/*
 * A counter stepped by 0x0144e and wrapped from 0x2a00 back to 0x1c00. What it
 * counts is not established; the name says only where it lives.
 */

/*
 * ---------------------------------------------------------------------------
 * The video driver's data block, which lives **inside DGROUP** at offset
 * 0x3890.
 *
 * The driver loads its own data segment from `cs:[0x13a]` and keeps the byte
 * distance from DGROUP to it in `cs:[0x13c]` - and that distance is 0x3890.
 * So `driverDS:0x12` and `DGROUP:0x38a2` are the same word, which is why the
 * game's start-up at image 0x0e183 can write the driver's page segments
 * directly:
 *
 *     0e183  mov word ptr [0x38a4], 0xa000
 *     0e189  mov word ptr [0x38a2], 0xa820
 *
 * The clip box and the colours the rectangle routine at 0x20079 reads are the
 * same block seen from the game's side: DGROUP 0x3894 is driver 0x04. There is
 * one shared structure here, not two.
 * ---------------------------------------------------------------------------
 */
#define VMDS 0x3890

/*
 * ---------------------------------------------------------------------------
 * **The driver's block as a struct, named for the DGROUP offset it sits at.**
 *
 * NOT a transcription - the original has no struct declaration to copy, only
 * offsets off DS - but the *layout* is, and every field below carries the
 * offset it was read at with a `_Static_assert` next to the definition. That
 * assert is the point of doing it this way: a mistyped padding array or a
 * field the compiler decides to align moves everything after it, and silently
 * reading the wrong word is exactly the class of fault this project cannot
 * catch by looking at a screen.
 *
 * **The byte array stays underneath.** DGROUP is still `dgroup[]`, because the
 * game uses near pointers - a word in DGROUP holding an offset into DGROUP -
 * and a set of C globals cannot express that. This overlays a struct on the
 * bytes; a field and a pointer dereference still reach the same byte.
 *
 * Names ending `_ptr` hold an address, and the type says which kind: `dg_off_t`
 * is a near pointer, an offset into DGROUP, and `dg_seg_t` is a real-mode
 * segment. Everything else is the narrowest type the code actually uses -
 * `int16_t` where the original does signed compares, `uint8_t` for a flag byte.
 *
 * `unknown_XX` is a field whose purpose has not been established, named for its
 * offset so that it is obvious what is known and what is not. They are not
 * padding: the code reads and writes several of them.
 *
 * **Every one of these overlays is `packed`, and it is not superstition.**
 * DGROUP has words at odd addresses - the game's own state block starts at
 * 0x4e67 - so a `uint16_t` after a byte field lands on an odd offset, and a
 * compiler is entitled to align it and move everything after it. That happened
 * the first time `dg_52ed` was written: `last_key` is a byte at 0x52f1, the
 * word after it belongs at 0x52f2, and unpacked it went to 0x52f3 with the
 * five fields behind it following. The asserts caught all six, which is the
 * whole reason for having them; `packed` is what stops it happening again.
 * ---------------------------------------------------------------------------
 */
typedef uint16_t dg_off_t;      /* a near pointer: an offset into DGROUP */
typedef uint16_t dg_seg_t;      /* a real-mode segment */

/*
 * **Resolving between the two forms a near pointer has.** The game stores a
 * 16-bit offset into a segment; C wants an address. `dg_ptr` turns the game's
 * offset into something a routine can be handed, and `dg_off` turns an address
 * back into the offset the game would have stored - which is what lets a field
 * be a *field* even where the code needs its address.
 *
 * **Both take the base the offset is measured against, and that is not
 * ceremony.** This program has several offset spaces: DGROUP, the loaded sound
 * driver at `SX_SEG`, the sound module at `ASB_SEG:ASB_OFF`, the two code
 * segments `S1C25` and `SNDCS` that keep state inside themselves. An offset
 * only means something against one of them, and a helper that always
 * subtracted `dgroup` would answer confidently and wrongly for the other four.
 * Popcorn keeps a separate `global_off`, `assets_off`, `animations_off` and
 * `runtime_off` for the same reason; naming the base at the call site is the
 * same fact written once instead of four times.
 *
 * The font tables below are the worked example:
 * `dg_off(dgroup, &DG3890.font_table_34[si])` is what `game_fread` wants, and
 * it says which table where `(uint16_t)(0x38c4 + si)` did not.
 */
static inline uint8_t *dg_ptr(void *base, uint16_t off)
{
    return (uint8_t *)base + off;
}

/* `const volatile`, because the struct overlays are volatile - see the note on
 * DG8 above for why - and a plain `const void *` parameter would make every
 * call site discard the qualifier. */
/*
 * A 16-bit read and write through a **byte** pointer.
 *
 * The guest's records are packed and heap-allocated, so a field's address can
 * be odd and `int16_t *` into one is not something the compiler will hand out
 * - it says so, as -Waddress-of-packed-member. A routine that is passed the
 * address of a pair of words therefore takes `const volatile uint8_t *` and
 * reads through these, which assume nothing about alignment and say the
 * little-endian order the original depends on.
 *
 * A routine's *own* frame is a different case and keeps typed views: it is
 * declared `_Alignas(2)` here, so the even offsets inside it really are
 * aligned and `int16_t *` into it is honest.
 *
 * Ours. The original has no such routine; it addresses bytes and words with
 * the same instruction.
 */
static inline int16_t dg_rd16(const volatile void *p)
{
    const volatile uint8_t *b = (const volatile uint8_t *)p;

    return (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

static inline void dg_wr16(volatile void *p, int16_t v)
{
    volatile uint8_t *b = (volatile uint8_t *)p;

    b[0] = (uint8_t)v;
    b[1] = (uint8_t)((uint16_t)v >> 8);
}

static inline uint16_t dg_off(const volatile void *base, const volatile void *p)
{
    return (uint16_t)((const volatile uint8_t *)p
                      - (const volatile uint8_t *)base);
}

struct dg_3890 {
    uint8_t   unknown_00;                   /* +0x00 */
    uint8_t   unknown_01;                   /* +0x01 */
    uint8_t   unknown_02;                   /* +0x02 */
    uint8_t   clip_enabled;                 /* +0x03 */
    int16_t   clip_left;                    /* +0x04 */
    int16_t   clip_right;                   /* +0x06 */
    int16_t   clip_top;                     /* +0x08 */
    int16_t   clip_bottom;                  /* +0x0a */
    uint8_t   fill_enabled;                 /* +0x0c */
    uint8_t   fill_colour;                  /* +0x0d */
    uint8_t   second_colour;                /* +0x0e */
    uint8_t   unknown_0f;                   /* +0x0f */
    uint16_t  unknown_10;                   /* +0x10 */
    dg_seg_t  page_back_ptr;                /* +0x12  being drawn into */
    dg_seg_t  page_front_ptr;               /* +0x14  on screen */
    dg_seg_t  page_src_ptr;                 /* +0x16  a copy's source */
    dg_seg_t  page_dst_ptr;                 /* +0x18  what drawing goes into */
    uint8_t   unknown_1a[2];                /* +0x1a */
    uint8_t   unknown_1c;                   /* +0x1c */
    int8_t    pixel_shift;                  /* +0x1d  bytes per pixel, as a
                                             * shift; signed, and read so */
    uint8_t   unknown_1e;                   /* +0x1e */
    uint8_t   unknown_1f;                   /* +0x1f */
    uint8_t   unknown_20;                   /* +0x20 */
    uint8_t   adapter;                      /* +0x21  0x10 is the VGA */
    uint16_t  line_colour;                  /* +0x22 */
    uint8_t   unknown_24[0x10];             /* +0x24 */
    /*
     * +0x34  the font's four per-slot tables, 0x14 apart, one byte per glyph
     * slot. The loader hands their *addresses* to `game_fread`, which takes a
     * DGROUP offset - so the call sites read `dg_off(&DG3890.font_table_34[si])`
     * rather than `0x38c4 + si`, which says the same thing and says which
     * table it is.
     */
    uint8_t   font_table_34[0x14];          /* +0x34  DGROUP 0x38c4 */
    uint8_t   font_table_48[0x14];          /* +0x48  DGROUP 0x38d8 */
    uint8_t   font_table_5c[0x14];          /* +0x5c  DGROUP 0x38ec */
    uint8_t   font_table_70[0x14];          /* +0x70  DGROUP 0x3900 */
    uint8_t   unknown_84[0x28];             /* +0x84 */
    /*
     * +0xac  the polygon clipper's four arrays, 0x28 bytes and so twenty
     * entries each. `clip_polygon` runs Sutherland and Hodgman's in two
     * passes: left and right out of `poly` into `work`, then top and bottom
     * back again, with `DG3A2C.clip_count` rewritten after each. The callers
     * hand the arrays' *addresses* to `poly_outline` and `poly_fill`, which
     * take DGROUP offsets, so those sites read `dg_off(dgroup, DG3890.poly_x)`.
     */
    int16_t   poly_x[20];                   /* +0xac   DGROUP 0x393c */
    int16_t   poly_y[20];                   /* +0xd4   DGROUP 0x3964 */
    int16_t   work_x[20];                   /* +0xfc   DGROUP 0x398c */
    int16_t   work_y[20];                   /* +0x124  DGROUP 0x39b4 */
    int16_t   closed_x[20];                 /* +0x14c  DGROUP 0x39dc, the closed
                                                       copy poly_fill walks */
    int16_t   closed_y[20];                 /* +0x174  DGROUP 0x3a04 */
    uint8_t   unknown_19c[2];               /* +0x19c  DG3A2C.clip_count */
    struct {
        dg_off_t off;
        dg_seg_t seg;
    } pal_copy_ptr;                         /* +0x19e  VGA:0x0f15's palette */
    uint8_t   unknown_1a2[0x51a];           /* +0x1a2 */
    uint16_t  dda_whole;                    /* +0x6bc */
    uint16_t  dda_frac;                     /* +0x6be */
    int16_t   dda_saved;                    /* +0x6c0 */
    uint16_t  dda_acc;                      /* +0x6c2 */
    uint8_t   line_mask;                    /* +0x6c4 */
    uint8_t   unknown_6c5[0x27];            /* +0x6c5 */
    uint16_t  screen_height;                /* +0x6ec  the mode's height, 480 */
    uint8_t   unknown_6ee[4];               /* +0x6ee */
    uint16_t  row_offset[480];              /* +0x6f2  measured: [y] == y * 80 */
} __attribute__((packed));

#define DG3890 (*(volatile struct dg_3890 *)(dgroup + VMDS))

#define DG_ASSERT_AT(type, field, off) \
    _Static_assert(__builtin_offsetof(type, field) == (off), \
                   #type "." #field " must sit at " #off)

DG_ASSERT_AT(struct dg_3890, clip_enabled,   0x03);
DG_ASSERT_AT(struct dg_3890, clip_left,      0x04);
DG_ASSERT_AT(struct dg_3890, clip_right,     0x06);
DG_ASSERT_AT(struct dg_3890, clip_top,       0x08);
DG_ASSERT_AT(struct dg_3890, clip_bottom,    0x0a);
DG_ASSERT_AT(struct dg_3890, fill_enabled,   0x0c);
DG_ASSERT_AT(struct dg_3890, fill_colour,    0x0d);
DG_ASSERT_AT(struct dg_3890, second_colour,  0x0e);
DG_ASSERT_AT(struct dg_3890, poly_x,         0xac);
DG_ASSERT_AT(struct dg_3890, poly_y,         0xd4);
DG_ASSERT_AT(struct dg_3890, work_x,         0xfc);
DG_ASSERT_AT(struct dg_3890, work_y,        0x124);
DG_ASSERT_AT(struct dg_3890, closed_x,      0x14c);
DG_ASSERT_AT(struct dg_3890, closed_y,      0x174);
DG_ASSERT_AT(struct dg_3890, page_back_ptr,  0x12);
DG_ASSERT_AT(struct dg_3890, page_front_ptr, 0x14);
DG_ASSERT_AT(struct dg_3890, page_src_ptr,   0x16);
DG_ASSERT_AT(struct dg_3890, page_dst_ptr,   0x18);
DG_ASSERT_AT(struct dg_3890, pixel_shift,    0x1d);
DG_ASSERT_AT(struct dg_3890, adapter,        0x21);
DG_ASSERT_AT(struct dg_3890, line_colour,    0x22);
DG_ASSERT_AT(struct dg_3890, font_table_34,  0x34);
DG_ASSERT_AT(struct dg_3890, font_table_48,  0x48);
DG_ASSERT_AT(struct dg_3890, font_table_5c,  0x5c);
DG_ASSERT_AT(struct dg_3890, font_table_70,  0x70);
DG_ASSERT_AT(struct dg_3890, pal_copy_ptr,   0x19e);
DG_ASSERT_AT(struct dg_3890, dda_whole,      0x6bc);
DG_ASSERT_AT(struct dg_3890, dda_frac,       0x6be);
DG_ASSERT_AT(struct dg_3890, dda_saved,      0x6c0);
DG_ASSERT_AT(struct dg_3890, dda_acc,        0x6c2);
DG_ASSERT_AT(struct dg_3890, line_mask,      0x6c4);
DG_ASSERT_AT(struct dg_3890, screen_height,  0x6ec);
DG_ASSERT_AT(struct dg_3890, row_offset,     0x6f2);

/* The names above are the struct's fields now; there are no macros for
 * them, because a macro named for a field re-expands inside `DG3890.field`
 * and the compiler says only "expected identifier". */

/*
 * The line drawer's own scratch, all inside the same block: the colour it is
 * drawing with, its current bit mask, and the four words its fixed-point DDA
 * keeps between rows. The original stores these rather than holding them in
 * registers, and the port has to as well or the memory comparison sees the
 * difference - which is how they were found.
 */

/* Where VGA:0x0f15 keeps the copy of the current palette. */

/* The byte offset of each scan line, indexed by y. Measured: [y] == y*80. */

/*
 * Measured over 2,108 calls to the rectangle routine while the intro screens
 * run: fill is always enabled, clipping always on, and the two colour bytes
 * always equal - which is the condition that skips the outline.
 */

/* Cleared, six words, by the routine at 0x166d6. Purpose not established. */
#define word_array_50bf(i) DG16(0x50bf + 2 * (i))

/*
 * ---------------------------------------------------------------------------
 * **Bare tables**: a run of same-sized entries at a fixed DGROUP address, with
 * no record around them. A pointer says what a macro over `DG16` cannot - the
 * element's width and that indexing is by element and not by byte - and none
 * of them claims a length, because nothing in the code states one.
 *
 * `rect_buffer` is the exception worth reading twice. Its entries are four
 * bytes and its index is **one-based**: entry 0 would sit on `DG5752.frame_flag`
 * and `DG5752.size_word`, and the only thing keeping the two apart is that
 * every caller guards on the index being non-zero. That is the original's
 * arrangement, not a mistake in the transcription.
 * ---------------------------------------------------------------------------
 */
/* 0x4d06: Borland's flags, one word per file handle */
#define HANDLE_FLAGS   ((volatile uint16_t *)(dgroup + 0x4d06))
/* 0x57c0: the open resource streams, a near pointer each */
#define RESOURCE_SLOTS ((volatile dg_off_t *)(dgroup + 0x57c0))
/* 0x3f82: a word per screen row, the row's base address */
#define ROW_BASE       ((volatile uint16_t *)(dgroup + 0x3f82))
/* 0x5956: the scaling table `scale_step` takes differences across */
#define SCALE_TABLE    ((volatile int16_t *)(dgroup + 0x5956))
/* 0x5754: a far pointer per saved rectangle, indexed from ONE */
#define RECT_BUFFER    ((volatile struct { dg_off_t off; dg_seg_t seg; } *) \
                        (dgroup + 0x5754))
/* 0x6414: the sequencer's seven voices, a far pointer each */
#define VOICES         ((volatile struct { dg_off_t off; dg_seg_t seg; } *) \
                        (dgroup + 0x6414))

/* A byte array indexed by the routine at 0x2147d, which returns its bit 0. */
#define byte_array_468c(i) DG8(0x468c + (i))

/*
 * A near pointer at DGROUP 0x5400 to a structure, and three words beside it,
 * all used by the routine at 0x002be. What the structure is has not been
 * established; only the offsets it touches are known.
 */
/* These are `DG53FC.list_ptr` and its neighbours now; see the struct. */

/*
 * DGROUP 0x4342 holds the *segment* of the block the game builds span lists in
 * - a separate allocation, not part of DGROUP. It is reached through
 * `FAR_PTR(span_buffer_seg, 0)`, in the guest's address space, exactly where
 * the original puts it.
 *
 * An earlier version gave the port an array of its own for this. It passed
 * every check until the verifier began comparing all of conventional memory
 * rather than only DGROUP, and then `fill_rect` and `vm_fill_spans` both
 * failed at once: the original's span list was being written somewhere the
 * port never touched.
 */
#define span_buffer_seg   DGU16(0x4342)

/*
 * The four holiday flags, set by `set_holiday_flags` from `dos_getdate` and
 * read by the parts bin: a kind is offered only on its own day. Three of them
 * gate a part and the fourth gates nothing at all.
 *
 * They are in no level - `TIM_LEVELSCAN` finds none of the three across all 87
 * - so the calendar is the only way to reach them, which is what `TIM_DATE`
 * is for.
 */
/* The four holiday flags are `DG4E67.holiday_*` now; see the struct. */


/*
 * ---------------------------------------------------------------------------
 * **The game's own state, at DGROUP 0x4e67.**
 *
 * Fifty-two consecutive words and 737 of the port's accesses - the largest
 * cluster `tools/dgrules.py` reports after the video driver's block. What each
 * one is comes from the routines that use it and is written beside it; where
 * it does not, the field keeps the `word_XXXX` name this header already used
 * for exactly that case, so the difference between known and guessed stays
 * visible.
 *
 * The types are the narrowest the code uses. `int16_t` where the original
 * compares signed - the three origin pairs are set to -8 by `round_setup` and
 * `game_play` tests its round number with `jle` - and `uint16_t` for the
 * counters and state words. `dg_off_t` marks the eight near pointers: five
 * region-list heads, two records kept beside them, and the bitmap lists.
 * ---------------------------------------------------------------------------
 */
struct dg_4e67 {
    uint16_t  round_kind;          /* +0x00  non-zero is freeform, zero loads a level */
    uint16_t  word_4e69;           /* +0x02 */
    uint16_t  state;               /* +0x04  the round and screen state machine's word */
    dg_off_t  region_kept_a_ptr;   /* +0x06  two records kept on their own as well */
    dg_off_t  region_kept_b_ptr;   /* +0x08 */
    dg_off_t  regions_a_ptr;       /* +0x0a  the five region lists, heads of */
    dg_off_t  regions_b_ptr;       /* +0x0c */
    dg_off_t  regions_c_ptr;       /* +0x0e */
    dg_off_t  regions_panel_ptr;   /* +0x10  the briefing's controls */
    dg_off_t  regions_play_ptr;    /* +0x12  the play screen's */
    int16_t   holiday_christmas;   /* +0x14  25 December - kind 34, the tree */
    int16_t   holiday_halloween;   /* +0x16  31 October  - kind 32, the pumpkin */
    int16_t   holiday_stpatrick;   /* +0x18  17 March    - read by nothing */
    int16_t   holiday_valentine;   /* +0x1a  14 February - kind 33, the heart */
    uint16_t  word_4e83;           /* +0x1c */
    uint16_t  file_op_active;      /* +0x1e  GUESS: 1 around the chdir a file dialog does */
    int16_t   word_4e87;           /* +0x20  stepped by 0x0144e, wrapped 0x2a00 to 0x1c00 */
    uint16_t  word_4e89;           /* +0x22 */
    uint16_t  redraw_a;            /* +0x24  five deferred redraws, one layer each; a */
    uint16_t  redraw_b;            /* +0x26  change asks for N frames and gets one a */
    uint16_t  redraw_c;            /* +0x28  frame. Counts, not flags - see */
    uint16_t  redraw_d;            /* +0x2a  game_screen_loop, which decrements each */
    uint16_t  redraw_e;            /* +0x2c  by one rather than clearing it */
    uint16_t  word_4e95;           /* +0x2e */
    uint16_t  word_4e97;           /* +0x30 */
    int16_t   origin_c_y;          /* +0x32  three origin pairs, y then x, all set to -8 */
    int16_t   origin_c_x;          /* +0x34  by round_setup; which is which role is not */
    int16_t   origin_b_y;          /* +0x36  established, only that the live one is the */
    int16_t   origin_b_x;          /* +0x38  third */
    int16_t   origin_y;            /* +0x3a  the play area's scroll origin: draw_part_clip */
    int16_t   origin_x;            /* +0x3c  takes world minus these to get screen */
    uint16_t  elapsed_ticks;       /* +0x3e  run_machine_loop accumulates the ticks a frame took */
    uint16_t  machine_frames;      /* +0x40  and counts its frames here */
    uint16_t  score_a;             /* +0x42  the pair finish_level banks for the password */
    uint16_t  score_b;             /* +0x44 */
    uint16_t  counter_lo;          /* +0x46  one 32-bit counter, low word first */
    uint16_t  counter_hi;          /* +0x48 */
    int16_t   word_4eb1;           /* +0x4a */
    int16_t   word_4eb3;           /* +0x4c */
    int16_t   password_puzzle;     /* +0x4e  the puzzle game_teardown prints a password for */
    int16_t   furthest_level;      /* +0x50  how far the player has reached; in tim.cfg */
    int16_t   level_count;         /* +0x52  how many L<n>.LEV there are */
    uint16_t  word_4ebb;           /* +0x54 */
    int16_t   round_number;        /* +0x56  the puzzle being played; round_setup loads it */
    int16_t   playing;             /* +0x58  game_play runs while this is non-zero */
    uint16_t  master_level;        /* +0x5a  the volume knob's setting; in tim.cfg */
    int16_t   word_4ec3;           /* +0x5c */
    int16_t   word_4ec5;           /* +0x5e */
    dg_off_t  icons_bmp_ptr;       /* +0x60  icons.bmp's list */
    dg_off_t  menu_bmp_ptr;        /* +0x62  gp_menu.bmp's */
    dg_off_t  bmp_4ecb_ptr;        /* +0x64 */
    dg_off_t  score2_bmp_ptr;      /* +0x66  score2.bmp's - draw_odometer_digit's strips */
} __attribute__((packed));

#define DG4E67 (*(volatile struct dg_4e67 *)(dgroup + 0x4e67))

DG_ASSERT_AT(struct dg_4e67, round_kind,         0x00);
DG_ASSERT_AT(struct dg_4e67, word_4e69,          0x02);
DG_ASSERT_AT(struct dg_4e67, state,              0x04);
DG_ASSERT_AT(struct dg_4e67, region_kept_a_ptr,  0x06);
DG_ASSERT_AT(struct dg_4e67, region_kept_b_ptr,  0x08);
DG_ASSERT_AT(struct dg_4e67, regions_a_ptr,      0x0a);
DG_ASSERT_AT(struct dg_4e67, regions_b_ptr,      0x0c);
DG_ASSERT_AT(struct dg_4e67, regions_c_ptr,      0x0e);
DG_ASSERT_AT(struct dg_4e67, regions_panel_ptr,  0x10);
DG_ASSERT_AT(struct dg_4e67, regions_play_ptr,   0x12);
DG_ASSERT_AT(struct dg_4e67, holiday_christmas,  0x14);
DG_ASSERT_AT(struct dg_4e67, holiday_halloween,  0x16);
DG_ASSERT_AT(struct dg_4e67, holiday_stpatrick,  0x18);
DG_ASSERT_AT(struct dg_4e67, holiday_valentine,  0x1a);
DG_ASSERT_AT(struct dg_4e67, word_4e83,          0x1c);
DG_ASSERT_AT(struct dg_4e67, file_op_active,     0x1e);
DG_ASSERT_AT(struct dg_4e67, word_4e87,          0x20);
DG_ASSERT_AT(struct dg_4e67, word_4e89,          0x22);
DG_ASSERT_AT(struct dg_4e67, redraw_a,           0x24);
DG_ASSERT_AT(struct dg_4e67, redraw_b,           0x26);
DG_ASSERT_AT(struct dg_4e67, redraw_c,           0x28);
DG_ASSERT_AT(struct dg_4e67, redraw_d,           0x2a);
DG_ASSERT_AT(struct dg_4e67, redraw_e,           0x2c);
DG_ASSERT_AT(struct dg_4e67, word_4e95,          0x2e);
DG_ASSERT_AT(struct dg_4e67, word_4e97,          0x30);
DG_ASSERT_AT(struct dg_4e67, origin_c_y,         0x32);
DG_ASSERT_AT(struct dg_4e67, origin_c_x,         0x34);
DG_ASSERT_AT(struct dg_4e67, origin_b_y,         0x36);
DG_ASSERT_AT(struct dg_4e67, origin_b_x,         0x38);
DG_ASSERT_AT(struct dg_4e67, origin_y,           0x3a);
DG_ASSERT_AT(struct dg_4e67, origin_x,           0x3c);
DG_ASSERT_AT(struct dg_4e67, elapsed_ticks,      0x3e);
DG_ASSERT_AT(struct dg_4e67, machine_frames,     0x40);
DG_ASSERT_AT(struct dg_4e67, score_a,            0x42);
DG_ASSERT_AT(struct dg_4e67, score_b,            0x44);
DG_ASSERT_AT(struct dg_4e67, counter_lo,         0x46);
DG_ASSERT_AT(struct dg_4e67, counter_hi,         0x48);
DG_ASSERT_AT(struct dg_4e67, word_4eb1,          0x4a);
DG_ASSERT_AT(struct dg_4e67, word_4eb3,          0x4c);
DG_ASSERT_AT(struct dg_4e67, password_puzzle,    0x4e);
DG_ASSERT_AT(struct dg_4e67, furthest_level,     0x50);
DG_ASSERT_AT(struct dg_4e67, level_count,        0x52);
DG_ASSERT_AT(struct dg_4e67, word_4ebb,          0x54);
DG_ASSERT_AT(struct dg_4e67, round_number,       0x56);
DG_ASSERT_AT(struct dg_4e67, playing,            0x58);
DG_ASSERT_AT(struct dg_4e67, master_level,       0x5a);
DG_ASSERT_AT(struct dg_4e67, word_4ec3,          0x5c);
DG_ASSERT_AT(struct dg_4e67, word_4ec5,          0x5e);
DG_ASSERT_AT(struct dg_4e67, icons_bmp_ptr,      0x60);
DG_ASSERT_AT(struct dg_4e67, menu_bmp_ptr,       0x62);
DG_ASSERT_AT(struct dg_4e67, bmp_4ecb_ptr,       0x64);
DG_ASSERT_AT(struct dg_4e67, score2_bmp_ptr,     0x66);

/*
 * **The parts the game is holding on to**, at DGROUP 0x50d3.
 */
struct dg_50d3 {
    dg_off_t  bin_list_ptr;       /* +0x00  the list draw_bin walks; defaults to &bin_head_ptr */
    dg_off_t  dragged_part_ptr;   /* +0x02  the part being dragged - drawn last, and not counted */
    dg_off_t  bin_head_ptr;       /* +0x04  the parts bin's list head */
    uint16_t  word_50d9;          /* +0x06 */
} __attribute__((packed));

#define DG50D3 (*(volatile struct dg_50d3 *)(dgroup + 0x50d3))

DG_ASSERT_AT(struct dg_50d3, bin_list_ptr,      0x00);
DG_ASSERT_AT(struct dg_50d3, dragged_part_ptr,  0x02);
DG_ASSERT_AT(struct dg_50d3, bin_head_ptr,      0x04);
DG_ASSERT_AT(struct dg_50d3, word_50d9,         0x06);

/*
 * **The pointer and its buttons, as the guest sees them**, at DGROUP 0x5768.
 */
struct dg_5768 {
    int16_t   button_accum_a;     /* +0x00  the two the timer handler accumulates into */
    int16_t   button_accum_b;     /* +0x02 */
    int16_t   pointer_a;          /* +0x04  the pair timer_callback keeps beside the buttons */
    int16_t   pointer_b;          /* +0x06 */
    uint16_t  word_5770;          /* +0x08 */
    uint16_t  button_right;       /* +0x0a  2 is a click; the intro leaves on either button */
    uint16_t  button_left;        /* +0x0c  2 is a click - the word every region reads */
    uint16_t  word_5776;          /* +0x0e */
    uint16_t  word_5778;          /* +0x10 */
    uint16_t  word_577a;          /* +0x12 */
    uint16_t  word_577c;          /* +0x14 */
    int16_t   word_577e;          /* +0x16 */
    int16_t   word_5780;          /* +0x18 */
    int16_t   pointer_y;          /* +0x1a  regions_handle_pointer tests a record's +8 and +0x0c */
    int16_t   pointer_x;          /* +0x1c  against these, and its +6 and +0x0a against x */
    uint16_t  word_5786;          /* +0x1e */
} __attribute__((packed));

#define DG5768 (*(volatile struct dg_5768 *)(dgroup + 0x5768))

DG_ASSERT_AT(struct dg_5768, button_accum_a,    0x00);
DG_ASSERT_AT(struct dg_5768, button_accum_b,    0x02);
DG_ASSERT_AT(struct dg_5768, pointer_a,         0x04);
DG_ASSERT_AT(struct dg_5768, pointer_b,         0x06);
DG_ASSERT_AT(struct dg_5768, word_5770,         0x08);
DG_ASSERT_AT(struct dg_5768, button_right,      0x0a);
DG_ASSERT_AT(struct dg_5768, button_left,       0x0c);
DG_ASSERT_AT(struct dg_5768, word_5776,         0x0e);
DG_ASSERT_AT(struct dg_5768, word_5778,         0x10);
DG_ASSERT_AT(struct dg_5768, word_577a,         0x12);
DG_ASSERT_AT(struct dg_5768, word_577c,         0x14);
DG_ASSERT_AT(struct dg_5768, word_577e,         0x16);
DG_ASSERT_AT(struct dg_5768, word_5780,         0x18);
DG_ASSERT_AT(struct dg_5768, pointer_y,         0x1a);
DG_ASSERT_AT(struct dg_5768, pointer_x,         0x1c);
DG_ASSERT_AT(struct dg_5768, word_5786,         0x1e);

/*
 * **The compressed-stream reader's state**, at DGROUP 0x5888.
 */
struct dg_5888 {
    uint8_t   flags;              /* +0x00  bit 0x20 chooses next_input_byte's path */
    uint8_t   pad_01;             /* +0x01 */
    dg_off_t  record_ptr;         /* +0x02  the record being read */
    uint16_t  word_588c;          /* +0x04 */
    uint16_t  word_588e;          /* +0x06 */
    uint16_t  word_5890;          /* +0x08 */
    uint16_t  word_5892;          /* +0x0a */
    uint16_t  word_5894;          /* +0x0c */
    uint16_t  word_5896;          /* +0x0e */
    int16_t   word_5898;          /* +0x10 */
    int16_t   word_589a;          /* +0x12 */
    int16_t   word_589c;          /* +0x14 */
    int16_t   word_589e;          /* +0x16 */
    int16_t   word_58a0;          /* +0x18 */
    uint8_t   byte_58a2;          /* +0x1a */
    uint8_t   pad_1b;             /* +0x1b */
    int16_t   word_58a4;          /* +0x1c */
    int16_t   word_58a6;          /* +0x1e */
    int16_t   word_58a8;          /* +0x20 */
    int16_t   word_58aa;          /* +0x22 */
    int16_t   word_58ac;          /* +0x24 */
    uint8_t   byte_58ae;          /* +0x26 */
    uint8_t   pad_27;             /* +0x27 */
    int16_t   word_58b0;          /* +0x28 */
    int16_t   word_58b2;          /* +0x2a */
    int16_t   word_58b4;          /* +0x2c */
    int16_t   word_58b6;          /* +0x2e */
} __attribute__((packed));

#define DG5888 (*(volatile struct dg_5888 *)(dgroup + 0x5888))

DG_ASSERT_AT(struct dg_5888, flags,             0x00);
DG_ASSERT_AT(struct dg_5888, record_ptr,        0x02);
DG_ASSERT_AT(struct dg_5888, word_588c,         0x04);
DG_ASSERT_AT(struct dg_5888, word_588e,         0x06);
DG_ASSERT_AT(struct dg_5888, word_5890,         0x08);
DG_ASSERT_AT(struct dg_5888, word_5892,         0x0a);
DG_ASSERT_AT(struct dg_5888, word_5894,         0x0c);
DG_ASSERT_AT(struct dg_5888, word_5896,         0x0e);
DG_ASSERT_AT(struct dg_5888, word_5898,         0x10);
DG_ASSERT_AT(struct dg_5888, word_589a,         0x12);
DG_ASSERT_AT(struct dg_5888, word_589c,         0x14);
DG_ASSERT_AT(struct dg_5888, word_589e,         0x16);
DG_ASSERT_AT(struct dg_5888, word_58a0,         0x18);
DG_ASSERT_AT(struct dg_5888, byte_58a2,         0x1a);
DG_ASSERT_AT(struct dg_5888, word_58a4,         0x1c);
DG_ASSERT_AT(struct dg_5888, word_58a6,         0x1e);
DG_ASSERT_AT(struct dg_5888, word_58a8,         0x20);
DG_ASSERT_AT(struct dg_5888, word_58aa,         0x22);
DG_ASSERT_AT(struct dg_5888, word_58ac,         0x24);
DG_ASSERT_AT(struct dg_5888, byte_58ae,         0x26);
DG_ASSERT_AT(struct dg_5888, word_58b0,         0x28);
DG_ASSERT_AT(struct dg_5888, word_58b2,         0x2a);
DG_ASSERT_AT(struct dg_5888, word_58b4,         0x2c);
DG_ASSERT_AT(struct dg_5888, word_58b6,         0x2e);

/*
 * **The structure the routine at 0x002be walks**, at DGROUP 0x53fc.
 */
struct dg_53fc {
    int16_t   word_53fc;          /* +0x00 */
    uint16_t  word_53fe;          /* +0x02 */
    dg_off_t  list_ptr;           /* +0x04  the part the collision sweep is
                                             working on; `resolve_collisions`
                                             sets it from `pick_by_flag` and
                                             every routine below reads part
                                             fields out of it */
    int16_t   word_5402;          /* +0x06 */
    int16_t   word_5404;          /* +0x08 */
    int16_t   word_5406;          /* +0x0a */
    int16_t   word_5408;          /* +0x0c */
    int16_t   word_540a;          /* +0x0e */
    int16_t   word_540c;          /* +0x10 */
    int16_t   word_540e;          /* +0x12 */
    int16_t   word_5410;          /* +0x14 */
    int16_t   word_5412;          /* +0x16 */
    int16_t   word_5414;          /* +0x18 */
    int16_t   word_5416;          /* +0x1a */
    int16_t   word_5418;          /* +0x1c */
    int16_t   word_541a;          /* +0x1e */
    int16_t   word_541c;          /* +0x20 */
    int16_t   word_541e;          /* +0x22 */
    int16_t   word_5420;          /* +0x24 */
    int16_t   word_5422;          /* +0x26 */
    int16_t   word_5424;          /* +0x28 */
    int16_t   word_5426;          /* +0x2a */
    uint16_t  word_5428;          /* +0x2c */
    int16_t   selected_level;     /* +0x2e  the puzzle picker's row; game_round copies it to round_number */
    int16_t   word_542c;          /* +0x30 */
} __attribute__((packed));

#define DG53FC (*(volatile struct dg_53fc *)(dgroup + 0x53fc))

DG_ASSERT_AT(struct dg_53fc, word_53fc,         0x00);
DG_ASSERT_AT(struct dg_53fc, word_53fe,         0x02);
DG_ASSERT_AT(struct dg_53fc, list_ptr,          0x04);
DG_ASSERT_AT(struct dg_53fc, word_5402,         0x06);
DG_ASSERT_AT(struct dg_53fc, word_5404,         0x08);
DG_ASSERT_AT(struct dg_53fc, word_5406,         0x0a);
DG_ASSERT_AT(struct dg_53fc, word_5408,         0x0c);
DG_ASSERT_AT(struct dg_53fc, word_540a,         0x0e);
DG_ASSERT_AT(struct dg_53fc, word_540c,         0x10);
DG_ASSERT_AT(struct dg_53fc, word_540e,         0x12);
DG_ASSERT_AT(struct dg_53fc, word_5410,         0x14);
DG_ASSERT_AT(struct dg_53fc, word_5412,         0x16);
DG_ASSERT_AT(struct dg_53fc, word_5414,         0x18);
DG_ASSERT_AT(struct dg_53fc, word_5416,         0x1a);
DG_ASSERT_AT(struct dg_53fc, word_5418,         0x1c);
DG_ASSERT_AT(struct dg_53fc, word_541a,         0x1e);
DG_ASSERT_AT(struct dg_53fc, word_541c,         0x20);
DG_ASSERT_AT(struct dg_53fc, word_541e,         0x22);
DG_ASSERT_AT(struct dg_53fc, word_5420,         0x24);
DG_ASSERT_AT(struct dg_53fc, word_5422,         0x26);
DG_ASSERT_AT(struct dg_53fc, word_5424,         0x28);
DG_ASSERT_AT(struct dg_53fc, word_5426,         0x2a);
DG_ASSERT_AT(struct dg_53fc, word_5428,         0x2c);
DG_ASSERT_AT(struct dg_53fc, selected_level,    0x2e);
DG_ASSERT_AT(struct dg_53fc, word_542c,         0x30);

/*
 * **The sound bank, its driver and its module**, at DGROUP 0x4a82.
 */
struct dg_4a82 {
    uint16_t  driver_number;      /* +0x00  install_driver_far's answer; load_sound_module looks it up */
    uint16_t  word_4a84;          /* +0x02  handed to configure_driver_far */
    uint16_t  word_4a86;          /* +0x04 */
    dg_off_t  records_ptr;        /* +0x06  the record list start_sound walks by hand */
    dg_off_t  records_tail_ptr;   /* +0x08 */
    uint16_t  timer_taken;        /* +0x0a  whether the timer was taken - 0x44ee says who has it */
    dg_off_t  tick_cb_off;        /* +0x0c  the timer callback; its segment is a relocation */
    dg_seg_t  tick_cb_seg;        /* +0x0e */
    dg_off_t  bank_ptr;           /* +0x10  the record +0x15c and +0x15d come out of */
    dg_off_t  driver_ptr;         /* +0x12  the loaded driver, installed by install_driver_far */
    uint16_t  word_4a96;          /* +0x14 */
    dg_off_t  module_off;         /* +0x16  the module: offset first, segment second, which is */
    dg_seg_t  module_seg;         /* +0x18  what the lcall [0x4a98] at 0x0bbde reads */
    uint16_t  load_error;         /* +0x1a  2 on the two failures that mean the resource was missing */
    uint16_t  identifier;         /* +0x1c  the identifier 0x7e takes instead of a constant */
    uint16_t  voice_word;         /* +0x1e  0 or -1 stops the walk; 0 or -2 means already on a voice */
    dg_off_t  directory_ptr;      /* +0x20  the payload directory */
    dg_seg_t  payload_seg;        /* +0x22  the segment the payloads are in */
    uint16_t  file;               /* +0x24  the file this module opened, if it did */
    uint16_t  file_kind;          /* +0x26  recorded beside the handle */
    uint16_t  module_live;        /* +0x28  the module is loaded and a callback exists */
    uint16_t  bank_choice;        /* +0x2a  chooses between load_sound_bank and its sibling */
    uint16_t  device;             /* +0x2c  the device number; 8 is recorded as 3 */
} __attribute__((packed));

#define DG4A82 (*(volatile struct dg_4a82 *)(dgroup + 0x4a82))

DG_ASSERT_AT(struct dg_4a82, driver_number,     0x00);
DG_ASSERT_AT(struct dg_4a82, word_4a84,         0x02);
DG_ASSERT_AT(struct dg_4a82, word_4a86,         0x04);
DG_ASSERT_AT(struct dg_4a82, records_ptr,       0x06);
DG_ASSERT_AT(struct dg_4a82, records_tail_ptr,  0x08);
DG_ASSERT_AT(struct dg_4a82, timer_taken,       0x0a);
DG_ASSERT_AT(struct dg_4a82, tick_cb_off,       0x0c);
DG_ASSERT_AT(struct dg_4a82, tick_cb_seg,       0x0e);
DG_ASSERT_AT(struct dg_4a82, bank_ptr,          0x10);
DG_ASSERT_AT(struct dg_4a82, driver_ptr,        0x12);
DG_ASSERT_AT(struct dg_4a82, word_4a96,         0x14);
DG_ASSERT_AT(struct dg_4a82, module_off,        0x16);
DG_ASSERT_AT(struct dg_4a82, module_seg,        0x18);
DG_ASSERT_AT(struct dg_4a82, load_error,        0x1a);
DG_ASSERT_AT(struct dg_4a82, identifier,        0x1c);
DG_ASSERT_AT(struct dg_4a82, voice_word,        0x1e);
DG_ASSERT_AT(struct dg_4a82, directory_ptr,     0x20);
DG_ASSERT_AT(struct dg_4a82, payload_seg,       0x22);
DG_ASSERT_AT(struct dg_4a82, file,              0x24);
DG_ASSERT_AT(struct dg_4a82, file_kind,         0x26);
DG_ASSERT_AT(struct dg_4a82, module_live,       0x28);
DG_ASSERT_AT(struct dg_4a82, bank_choice,       0x2a);
DG_ASSERT_AT(struct dg_4a82, device,            0x2c);

/*
 * **The rubber-band line and the machine's sound requests**, at DGROUP 0x52bd.
 */
struct dg_52bd {
    int16_t   band_x;             /* +0x00  the pointer in play-area coordinates */
    int16_t   band_y;             /* +0x02 */
    int16_t   anchor_x;           /* +0x04  the far part's anchor: its +0x1e and +0x20 plus +0x56, +0x57 */
    int16_t   anchor_y;           /* +0x06 */
    int16_t   band_colour;        /* +0x08  0xa where it would attach, -1 for no line */
    int16_t   drop_cursor;        /* +0x0a  0xa on every frame the hand is not already carrying */
    int16_t   word_52c9;          /* +0x0c */
    int16_t   fill_colour;        /* +0x0e  the colour the panel and the title box are filled in */
    int16_t   sound_request_0c;   /* +0x10  four request-and-acknowledge words: something sets */
    int16_t   sound_request_09;   /* +0x12  one to 2 and the loop below turns it to 1 and then */
    int16_t   sound_request_02;   /* +0x14  stops the sound. See run_machine_loop, which does all */
    int16_t   sound_request_01;   /* +0x16  four every frame */
    int16_t   music_now;          /* +0x18  the tune opened and started, remembered */
    int16_t   saved_clip_bottom;  /* +0x1a  the saved rectangle, stored in **descending** order - */
    int16_t   saved_clip_top;     /* +0x1c  0x52dd is the left edge and 0x52d7 the bottom, which */
    int16_t   saved_clip_right;   /* +0x1e  looks like a transcription error and is not */
    int16_t   saved_clip_left;    /* +0x20 */
    int16_t   word_52df;          /* +0x22 */
    union {                       /* +0x24  black.pal, stored the same way */
        int32_t  dword;
        struct { dg_off_t off; dg_seg_t seg; };
    } pal_black_ptr;
    union {                       /* +0x28  sierra.pal */
        int32_t  dword;
        struct { dg_off_t off; dg_seg_t seg; };
    } pal_sierra_ptr;
} __attribute__((packed));

#define DG52BD (*(volatile struct dg_52bd *)(dgroup + 0x52bd))

DG_ASSERT_AT(struct dg_52bd, band_x,            0x00);
DG_ASSERT_AT(struct dg_52bd, band_y,            0x02);
DG_ASSERT_AT(struct dg_52bd, anchor_x,          0x04);
DG_ASSERT_AT(struct dg_52bd, anchor_y,          0x06);
DG_ASSERT_AT(struct dg_52bd, band_colour,       0x08);
DG_ASSERT_AT(struct dg_52bd, drop_cursor,       0x0a);
DG_ASSERT_AT(struct dg_52bd, word_52c9,         0x0c);
DG_ASSERT_AT(struct dg_52bd, fill_colour,       0x0e);
DG_ASSERT_AT(struct dg_52bd, sound_request_0c,  0x10);
DG_ASSERT_AT(struct dg_52bd, sound_request_09,  0x12);
DG_ASSERT_AT(struct dg_52bd, sound_request_02,  0x14);
DG_ASSERT_AT(struct dg_52bd, sound_request_01,  0x16);
DG_ASSERT_AT(struct dg_52bd, music_now,         0x18);
DG_ASSERT_AT(struct dg_52bd, saved_clip_bottom, 0x1a);
DG_ASSERT_AT(struct dg_52bd, saved_clip_top,    0x1c);
DG_ASSERT_AT(struct dg_52bd, saved_clip_right,  0x1e);
DG_ASSERT_AT(struct dg_52bd, saved_clip_left,   0x20);
DG_ASSERT_AT(struct dg_52bd, word_52df,         0x22);
DG_ASSERT_AT(struct dg_52bd, pal_black_ptr,     0x24);
DG_ASSERT_AT(struct dg_52bd, pal_sierra_ptr,    0x28);

/*
 * **The palettes, the last key, and the art sets**, at DGROUP 0x52ed.
 */
struct dg_52ed {
    /*
     * +0x00  tim.pal. **A union, because the bytes are reached both ways.**
     * `game_startup` stores what `load_palette` answered with a single 32-bit
     * write, exactly as the original does, and `set_palette_pointer` and
     * `free_far_block` take the halves. Splitting it into two words alone was
     * a real bug: the 32-bit store landed on the offset and the segment was
     * lost, and the intro's palette went with it.
     */
    union {
        int32_t  dword;
        struct { dg_off_t off; dg_seg_t seg; };
    } pal_tim_ptr;
    uint8_t   last_key;           /* +0x04  the last key the screen loops took - a **byte**, which
                                   * the assert caught: 0x52f2 follows it at +0x05 */
    uint16_t  cursor_follows;     /* +0x05  restore_cursor_following is guarded by this */
    dg_off_t  panel_art_ptr;      /* +0x07  the art set the panel's pieces come out of */
    dg_off_t  cursor_art_ptr;     /* +0x09  mouse.bmp's list */
    uint16_t  word_52f8;          /* +0x0b */
    uint16_t  stop_requested;     /* +0x0d  game_teardown(0) raises it; the loops above read it */
    uint16_t  stack_floor;        /* +0x0f  what the stack is reserved below */
} __attribute__((packed));

#define DG52ED (*(volatile struct dg_52ed *)(dgroup + 0x52ed))

DG_ASSERT_AT(struct dg_52ed, pal_tim_ptr,       0x00);
DG_ASSERT_AT(struct dg_52ed, last_key,          0x04);
DG_ASSERT_AT(struct dg_52ed, cursor_follows,    0x05);
DG_ASSERT_AT(struct dg_52ed, panel_art_ptr,     0x07);
DG_ASSERT_AT(struct dg_52ed, cursor_art_ptr,    0x09);
DG_ASSERT_AT(struct dg_52ed, word_52f8,         0x0b);
DG_ASSERT_AT(struct dg_52ed, stop_requested,    0x0d);
DG_ASSERT_AT(struct dg_52ed, stack_floor,       0x0f);

/*
 * **The bitmap compressor's stream**, at DGROUP 0x63e2.
 */
struct dg_63e2 {
    uint16_t  pending_rows;       /* +0x00  counts rows, not pixels */
    dg_off_t  out_start_off;      /* +0x02  where the output started, and does not move */
    dg_seg_t  out_start_seg;      /* +0x04 */
    uint16_t  word_63e8;          /* +0x06 */
    uint16_t  word_63ea;          /* +0x08 */
    uint16_t  word_63ec;          /* +0x0a */
    dg_off_t  out_off;            /* +0x0c  where the next byte goes */
    dg_seg_t  out_seg;            /* +0x0e */
    uint16_t  word_63f2;          /* +0x10 */
    uint16_t  mode;               /* +0x12  0x243bf sets it; it chooses how the runs are written */
} __attribute__((packed));

#define DG63E2 (*(volatile struct dg_63e2 *)(dgroup + 0x63e2))

DG_ASSERT_AT(struct dg_63e2, pending_rows,      0x00);
DG_ASSERT_AT(struct dg_63e2, out_start_off,     0x02);
DG_ASSERT_AT(struct dg_63e2, out_start_seg,     0x04);
DG_ASSERT_AT(struct dg_63e2, word_63e8,         0x06);
DG_ASSERT_AT(struct dg_63e2, word_63ea,         0x08);
DG_ASSERT_AT(struct dg_63e2, word_63ec,         0x0a);
DG_ASSERT_AT(struct dg_63e2, out_off,           0x0c);
DG_ASSERT_AT(struct dg_63e2, out_seg,           0x0e);
DG_ASSERT_AT(struct dg_63e2, word_63f2,         0x10);
DG_ASSERT_AT(struct dg_63e2, mode,              0x12);

/*
 * **The file picker and the wrapped-text block**, at DGROUP 0x568f.
 */
struct dg_568f {
    int16_t   picker_mode;        /* +0x00  0x80 from the mode it was opened from, else 0 */
    int16_t   scroll;             /* +0x02  clamped on the way in, not on the way out */
    int16_t   entry_count;        /* +0x04 */
    int16_t   entry_size;         /* +0x06  0x16, which is where the block's size comes from */
    int16_t   word_5697;          /* +0x08 */
    dg_off_t  block_off;          /* +0x0a  allocated once and kept; a null pointer is the end */
    dg_seg_t  block_seg;          /* +0x0c */
    int16_t   word_569d;          /* +0x0e */
    uint8_t   word_569f;          /* +0x10  a byte: 0x56a0 follows at +0x11 */
    int16_t   text_height;        /* +0x11  the block's measured extents, which the centring uses */
    int16_t   text_width;         /* +0x13  the widest line, clamped to the box */
    int16_t   line_count;         /* +0x15  how many lines, for the table at 0x56a6 */
} __attribute__((packed));

#define DG568F (*(volatile struct dg_568f *)(dgroup + 0x568f))

DG_ASSERT_AT(struct dg_568f, picker_mode,       0x00);
DG_ASSERT_AT(struct dg_568f, scroll,            0x02);
DG_ASSERT_AT(struct dg_568f, entry_count,       0x04);
DG_ASSERT_AT(struct dg_568f, entry_size,        0x06);
DG_ASSERT_AT(struct dg_568f, word_5697,         0x08);
DG_ASSERT_AT(struct dg_568f, block_off,         0x0a);
DG_ASSERT_AT(struct dg_568f, block_seg,         0x0c);
DG_ASSERT_AT(struct dg_568f, word_569d,         0x0e);
DG_ASSERT_AT(struct dg_568f, word_569f,         0x10);
DG_ASSERT_AT(struct dg_568f, text_height,       0x11);
DG_ASSERT_AT(struct dg_568f, text_width,        0x13);
DG_ASSERT_AT(struct dg_568f, line_count,        0x15);

/*
 * **The moving parts**, at DGROUP 0x5179.
 */
struct dg_5179 {
    dg_off_t  moving_ptr;         /* +0x00  the objects gravity and the step passes walk */
    dg_off_t  moving_tail_ptr;    /* +0x02 */
} __attribute__((packed));

#define DG5179 (*(volatile struct dg_5179 *)(dgroup + 0x5179))

DG_ASSERT_AT(struct dg_5179, moving_ptr,        0x00);
DG_ASSERT_AT(struct dg_5179, moving_tail_ptr,   0x02);

/*
 * **The shape and part free lists**, at DGROUP 0x4e4e.
 */
struct dg_4e4e {
    dg_off_t  shape_free_off;     /* +0x00  the free list nodes come off */
    dg_seg_t  shape_free_seg;     /* +0x02 */
    dg_off_t  shapes_ptr;         /* +0x04  the shapes drawn over, put back in reverse */
    dg_off_t  shapes_tail_ptr;    /* +0x06 */
    dg_off_t  parts_free_ptr;     /* +0x08  the head; 0x4e58 is the queue folded onto it */
    dg_off_t  parts_queue_ptr;    /* +0x0a  what asked to move this frame */
    uint8_t   name_buf;           /* +0x0c  pick_file fills this and copies the answer out */
} __attribute__((packed));

#define DG4E4E (*(volatile struct dg_4e4e *)(dgroup + 0x4e4e))

DG_ASSERT_AT(struct dg_4e4e, shape_free_off,    0x00);
DG_ASSERT_AT(struct dg_4e4e, shape_free_seg,    0x02);
DG_ASSERT_AT(struct dg_4e4e, shapes_ptr,        0x04);
DG_ASSERT_AT(struct dg_4e4e, shapes_tail_ptr,   0x06);
DG_ASSERT_AT(struct dg_4e4e, parts_free_ptr,    0x08);
DG_ASSERT_AT(struct dg_4e4e, parts_queue_ptr,   0x0a);
DG_ASSERT_AT(struct dg_4e4e, name_buf,          0x0c);

/*
 * **The level's own settings and its two bonus counters**, at DGROUP 0x50af.
 */
struct dg_50af {
    int16_t   bonus_a;            /* +0x00  the two counters added into the 32-bit score at 0x4ead */
    int16_t   bonus_b;            /* +0x02 */
    int16_t   gravity;            /* +0x04  the knob's x is this * 0xa0 / 0x80 + 0x3d, as a long */
    int16_t   air;                /* +0x06  and this one * 0xa0 / 0x200 + 0x3d - a different divisor */
    int16_t   extent_y;           /* +0x08  the level's own extent, -8 for a machine with none */
    int16_t   extent_x;           /* +0x0a */
    int16_t   tune;               /* +0x0c  the level's tune; game_round reads it back from here */
    uint16_t  flip_options;       /* +0x0e  part_flip_options' answer, kept for the handles */
} __attribute__((packed));

#define DG50AF (*(volatile struct dg_50af *)(dgroup + 0x50af))

DG_ASSERT_AT(struct dg_50af, bonus_a,           0x00);
DG_ASSERT_AT(struct dg_50af, bonus_b,           0x02);
DG_ASSERT_AT(struct dg_50af, gravity,           0x04);
DG_ASSERT_AT(struct dg_50af, air,               0x06);
DG_ASSERT_AT(struct dg_50af, extent_y,          0x08);
DG_ASSERT_AT(struct dg_50af, extent_x,          0x0a);
DG_ASSERT_AT(struct dg_50af, tune,              0x0c);
DG_ASSERT_AT(struct dg_50af, flip_options,      0x0e);

/*
 * **The timer's own state**, at DGROUP 0x44ee.
 */
struct dg_44ee {
    uint8_t   installed;          /* +0x00  the flag that says the handler is in; 0x4a8c records who */
    int16_t   frame_budget;       /* +0x01  counts down from 0x2710; every frame spin waits on it */
    int16_t   word_44f1;          /* +0x03 */
    int16_t   divider_reload;     /* +0x05 */
    int16_t   divider;            /* +0x07  counts from the reload, and only then does the rest */
    uint16_t  slot_mask;          /* +0x09  which of the sixteen callback slots are in use */
} __attribute__((packed));

#define DG44EE (*(volatile struct dg_44ee *)(dgroup + 0x44ee))

DG_ASSERT_AT(struct dg_44ee, installed,         0x00);
DG_ASSERT_AT(struct dg_44ee, frame_budget,      0x01);
DG_ASSERT_AT(struct dg_44ee, word_44f1,         0x03);
DG_ASSERT_AT(struct dg_44ee, divider_reload,    0x05);
DG_ASSERT_AT(struct dg_44ee, divider,           0x07);
DG_ASSERT_AT(struct dg_44ee, slot_mask,         0x09);

/*
 * **The drawing re-entry guard and the frame flag**, at DGROUP 0x5752.
 */
struct dg_5752 {
    uint16_t  guard;              /* +0x00  raised across a redraw and put back; a nesting guard, not a lock */
    int16_t   frame_flag;         /* +0x02  what wait_and_latch_frame spins on, set by the INT 08h handler */
    int16_t   size_word;          /* +0x04  the size, or the driver's own if this is zero */
} __attribute__((packed));

#define DG5752 (*(volatile struct dg_5752 *)(dgroup + 0x5752))

DG_ASSERT_AT(struct dg_5752, guard,             0x00);
DG_ASSERT_AT(struct dg_5752, frame_flag,        0x02);
DG_ASSERT_AT(struct dg_5752, size_word,         0x04);

/*
 * **The belt's far end and the goal's frame counter**, at DGROUP 0x5456.
 */
struct dg_5456 {
    uint16_t  belt_far_end;       /* +0x00  the far end's +0x5a, stashed while it is detached */
    uint16_t  goal_frames;        /* +0x02  consecutive frames the goal has held; passing 0xc wins */
} __attribute__((packed));

#define DG5456 (*(volatile struct dg_5456 *)(dgroup + 0x5456))

DG_ASSERT_AT(struct dg_5456, belt_far_end,      0x00);
DG_ASSERT_AT(struct dg_5456, goal_frames,       0x02);

/*
 * **An interrupted match, and where it resumes**, at DGROUP 0x58e0.
 */
struct dg_58e0 {
    int16_t   interrupted;        /* +0x00  a match was cut short */
    int16_t   position;           /* +0x02  and these three are what it comes back to */
    int16_t   length;             /* +0x04 */
    int16_t   progress;           /* +0x06 */
} __attribute__((packed));

#define DG58E0 (*(volatile struct dg_58e0 *)(dgroup + 0x58e0))

DG_ASSERT_AT(struct dg_58e0, interrupted,       0x00);
DG_ASSERT_AT(struct dg_58e0, position,          0x02);
DG_ASSERT_AT(struct dg_58e0, length,            0x04);
DG_ASSERT_AT(struct dg_58e0, progress,          0x06);

/*
 * **The five-tick wait and the cursor iterator**, at DGROUP 0x6430.
 */
struct dg_6430 {
    int16_t   ticks_left;         /* +0x00  set to five; a callback steps it down each tick */
    dg_off_t  cursor_off;         /* +0x02  a static far pointer, with its selector beside it */
    dg_seg_t  cursor_seg;         /* +0x04 */
    int16_t   selector;           /* +0x06 */
} __attribute__((packed));

#define DG6430 (*(volatile struct dg_6430 *)(dgroup + 0x6430))

DG_ASSERT_AT(struct dg_6430, ticks_left,        0x00);
DG_ASSERT_AT(struct dg_6430, cursor_off,        0x02);
DG_ASSERT_AT(struct dg_6430, cursor_seg,        0x04);
DG_ASSERT_AT(struct dg_6430, selector,          0x06);

/*
 * **The Borland heap and its two stream flags**, at DGROUP 0x4e34.
 */
struct dg_4e34 {
    dg_off_t  first_block_ptr;    /* +0x00  the block chain runs from here to the topmost */
    dg_off_t  top_block_ptr;      /* +0x02  which is where a new block is cut from */
    dg_off_t  ring_cursor_ptr;    /* +0x04  first fit walks *backward* from here */
    uint8_t   pad_4e3a[2];
    int16_t   stdin_is_tty;       /* +0x08  the two flags remembering what isatty said */
    int16_t   stdout_is_tty;      /* +0x0a */
} __attribute__((packed));

#define DG4E34 (*(volatile struct dg_4e34 *)(dgroup + 0x4e34))

DG_ASSERT_AT(struct dg_4e34, first_block_ptr,   0x00);
DG_ASSERT_AT(struct dg_4e34, top_block_ptr,     0x02);
DG_ASSERT_AT(struct dg_4e34, ring_cursor_ptr,   0x04);
DG_ASSERT_AT(struct dg_4e34, stdin_is_tty,      0x08);
DG_ASSERT_AT(struct dg_4e34, stdout_is_tty,     0x0a);

/*
 * **The screen the driver reported**, at DGROUP 0x3f78.
 */
struct dg_3f78 {
    uint8_t   mode_kind;          /* +0x00  a byte saying which */
    uint8_t   pad_3f79[1];
    int16_t   screen_width;       /* +0x02  an extent past these is cut back to the edge */
    int16_t   screen_height;      /* +0x04  the copy-protection screen sets it to 0x18f first */
} __attribute__((packed));

#define DG3F78 (*(volatile struct dg_3f78 *)(dgroup + 0x3f78))

DG_ASSERT_AT(struct dg_3f78, mode_kind,         0x00);
DG_ASSERT_AT(struct dg_3f78, screen_width,      0x02);
DG_ASSERT_AT(struct dg_3f78, screen_height,     0x04);

/*
 * A pair of bytes the original moves as a word: an x and a y that are written
 * one at a time and copied together. `clone_part` is where the difference
 * shows - it copies +0x56, +0x6a and +0x6c with three 16-bit moves, and a
 * transcription that reads those as single bytes drops the y of each.
 *
 * Ours, as a name: the original has no type, only the width of the move.
 */
struct byte_pair {
    uint8_t x;                 /* +0x00 */
    uint8_t y;                 /* +0x01 */
} __attribute__((packed));

/*
 * ---------------------------------------------------------------------------
 * **A part**, the 0xa2-byte record the machine is made of.
 *
 * Not a fixed DGROUP address like the overlays above - the records are cut
 * from the near heap and reached through a 16-bit offset, so this is the shape
 * and `PART(p)` is how a routine that has been handed one looks at it. That is
 * the near-pointer case this header opens with, seen from the other side.
 *
 * The names come from `devdump.c`'s `dump_chain`, which has printed these
 * fields for long enough to be the project's own record of what they are, and
 * from the setups in parts.c. What neither names keeps `field_XX`.
 *
 * **Only the sites written `part` are converted.** The same record is also
 * walked through `si`, `di`, `rec` and `obj` - two and a half thousand more
 * accesses - and those names are the transcription's registers rather than a
 * claim about type. Converting them needs each site read, not a regex.
 * ---------------------------------------------------------------------------
 */
struct part {
    dg_off_t  link_ptr;        /* +0x00  the next part; `si = DGU16(si)` is the walk, */
    uint8_t   pad_02[2];
    uint16_t  kind;            /* +0x04  which of the fifty-odd components it is */
    uint16_t  flags_06;        /* +0x06  devdump prints these two as `f6` and `f8` */
    uint16_t  flags_08;        /* +0x08 */
    uint16_t  flags_0a;        /* +0x0a */
    uint16_t  form;            /* +0x0c  which shape a part with several is in */
    uint16_t  word_0e;         /* +0x0e */
    int16_t   word_10;         /* +0x10 */
    int16_t   direction;       /* +0x12  devdump prints it as `dir` */
    uint8_t   byte_14;         /* +0x14  a redraw countdown: set to a count and
                                         stepped down once per pass */
    uint8_t   pad_15[1];
    /* **The position in 9-bit fixed point**, and the reason the momentum reads
       here are 32 bits wide. `reset_machine` loads `pos_x` into the first and
       `pos_y` into the second and shifts each left 9; the physics integrates
       them and the whole part is `>> 9`. Half of each is written on its own
       where a routine has the high word of an `imul` to store, so both
       spellings are kept and they are the same four bytes. */
    union {
        int32_t   fx;          /* +0x16 */
        struct {
            uint16_t word_16;  /* +0x16 */
            int16_t  word_18;  /* +0x18 */
        };
    };
    union {
        int32_t   fy;          /* +0x1a */
        struct {
            int16_t  word_1a;  /* +0x1a */
            int16_t  word_1c;  /* +0x1c */
        };
    };
    int16_t   pos_x;           /* +0x1e  the part's position; the grab box at +0x56 is added to it */
    int16_t   pos_y;           /* +0x20 */
    int16_t   word_22;         /* +0x22 */
    int16_t   word_24;         /* +0x24 */
    uint16_t  word_26;         /* +0x26 */
    uint16_t  word_28;         /* +0x28  compared against pos_y, and taken from
                                         word_8e when the machine resets */
    int16_t   box_x;           /* +0x2a  the part's own box, which the pointer is tested against */
    int16_t   box_y;           /* +0x2c */
    uint16_t  word_2e;         /* +0x2e */
    uint8_t   pad_30[2];
    uint16_t  word_32;         /* +0x32 */
    uint8_t   pad_34[2];
    int16_t   vel_x;           /* +0x36  velocity, stepped by the movers */
    int16_t   word_38;         /* +0x38 */
    int16_t   weight;          /* +0x3a  devdump prints it as `wt` */
    uint16_t  momentum_lo;     /* +0x3c  one 32-bit momentum, low word first */
    uint16_t  momentum_hi;     /* +0x3e */
    uint16_t  word_40;         /* +0x40 */
    uint16_t  word_42;         /* +0x42 */
    /* **A word each, not a byte.** The part builder at machine_draw.c writes
       both with a 16-bit move out of the kind table at 0x296e/0x2970, and
       `DG16(si + 0x44) >> 4` turns one into a cell count; the `DG8` sites that
       gave them a byte width earlier are reading the low half of a value that
       never gets that large. */
    int16_t   width;           /* +0x44  one less than this is what the setups lay out */
    int16_t   height;          /* +0x46 */
    uint16_t  word_48;         /* +0x48 */
    uint8_t   pad_4a[2];
    uint16_t  word_4c;         /* +0x4c */
    uint8_t   pad_4e[2];
    uint16_t  word_50;         /* +0x50 */
    uint16_t  word_52;         /* +0x52 */
    uint16_t  word_54;         /* +0x54 */
    union {
        struct byte_pair grab;                        /* +0x56 */
        struct {
            uint8_t grab_x;    /* +0x56  the grab box */
            uint8_t grab_y;    /* +0x57 */
        };
    };
    uint16_t  word_58;         /* +0x58 */
    uint16_t  link_right;      /* +0x5a  the four neighbours part_setup_2068 files by direction */
    uint16_t  link_left;       /* +0x5c */
    uint16_t  link_down;       /* +0x5e */
    uint16_t  link_up;         /* +0x60 */
    uint16_t  linked_a;        /* +0x62  the two part numbers part_setup_3de5 turns into form bits */
    uint16_t  linked_b;        /* +0x64 */
    uint16_t  word_66;         /* +0x66 */
    uint16_t  word_68;         /* +0x68 */
    /* **The two attachment offsets, a byte pair each.** Written a byte at a
       time by the setups - `part_setup_1105` puts half the width in the first
       and zero in the second - and read as a pair by the belt routines, which
       index them: `refresh_link_geometry` adds `+0x6a + 2 * slot` to the
       part's x and `+0x6b + 2 * slot` to its y. `reverse_link_ends` swaps the
       two pairs with one 16-bit move, which is what `attach[0]` and
       `attach[1]` say and what four separate bytes cannot. */
    union {
        struct byte_pair attach[2];                   /* +0x6a */
        struct {
            uint8_t byte_6a;   /* +0x6a */
            uint8_t byte_6b;   /* +0x6b */
            uint8_t byte_6c;   /* +0x6c */
            uint8_t byte_6d;   /* +0x6d */
        };
    };
    uint8_t   pad_6e[4];
    uint8_t   byte_72;         /* +0x72 */
    uint8_t   byte_73;         /* +0x73 */
    uint16_t  word_74;         /* +0x74  a part; refile_overlapping_parts walks
                                         one or the other of this pair */
    uint16_t  word_76;         /* +0x76 */
    uint16_t  word_78;         /* +0x78 */
    uint16_t  word_7a;         /* +0x7a  written together by link_nearby_objects */
    uint16_t  word_7c;         /* +0x7c */
    uint8_t   byte_7e;         /* +0x7e */
    uint8_t   byte_7f;         /* +0x7f  a bucket number: the draw and refile
                                         walks compare it against the one they
                                         are filling */
    uint16_t  point_count;     /* +0x80  raised to 4 across part_finish and put back to 1 */
    dg_off_t  points_ptr;      /* +0x82  where a setup copies its connection points to */
    /* **The contact block.** `resolve_collisions` and `find_edge_contact` both
       reach it by taking the address `part + 0x84` and walking from there, and
       `apply_contact_friction` takes the same address off whichever part it
       was handed - which is why the five sit together and why the port used to
       call the address a `link`. +0x84 is the part being touched, the two
       bytes are cleared together, +0x88 goes to `angles_same_side`, and +0x8a
       gets the edge index the search stopped on. */
    uint16_t  word_84;         /* +0x84  the part this one is in contact with */
    uint8_t   byte_86;         /* +0x86  cleared with byte_87 */
    uint8_t   byte_87;         /* +0x87 */
    int16_t   word_88;         /* +0x88  the contact angle */
    uint16_t  word_8a;         /* +0x8a  the edge the contact was found on */
    uint16_t  word_8c;         /* +0x8c */
    uint16_t  word_8e;         /* +0x8e */
    uint16_t  word_90;         /* +0x90 */
    uint16_t  word_92;         /* +0x92  copied part to part by clone_part */
    uint16_t  word_94;         /* +0x94 */
    /* **These six words mean different things to different kinds of part, so
       none of them can carry a name.** `parts.c` runs `word_96` as a plain
       countdown - set to 0x1c, to 0x64, to 5, and stepped to zero - and steps
       `spin` up towards 0x14. `link_slack` reads the same six as two chains of
       three generations, end A in 0x96/0x98/0x9a and end B in 0x9c/0x9e/0xa0,
       holding a belt's rest length; `refresh_link_geometry` writes 0x96 and
       0x9c from `link_end_distance`, and `shift_state_history` ages both
       chains for every part whether or not it has a belt. `spin` is what
       devdump prints it as and is a guess about one kind, kept because
       renaming it would only move the guess. */
    uint16_t  word_96;         /* +0x96 */
    int16_t   word_98;         /* +0x98 */
    int16_t   word_9a;         /* +0x9a */
    int16_t   spin;            /* +0x9c */
    int16_t   word_9e;         /* +0x9e */
    int16_t   word_a0;         /* +0xa0 */
    uint8_t   pad_a2[0];
} __attribute__((packed));

#define PART(p) (*(volatile struct part *)(dgroup + (uint16_t)(p)))

DG_ASSERT_AT(struct part, link_ptr,       0x00);
DG_ASSERT_AT(struct part, kind,           0x04);
DG_ASSERT_AT(struct part, flags_06,       0x06);
DG_ASSERT_AT(struct part, flags_08,       0x08);
DG_ASSERT_AT(struct part, flags_0a,       0x0a);
DG_ASSERT_AT(struct part, form,           0x0c);
DG_ASSERT_AT(struct part, word_0e,        0x0e);
DG_ASSERT_AT(struct part, word_10,        0x10);
DG_ASSERT_AT(struct part, direction,      0x12);
DG_ASSERT_AT(struct part, byte_14,        0x14);
DG_ASSERT_AT(struct part, word_16,        0x16);
DG_ASSERT_AT(struct part, word_1a,        0x1a);
DG_ASSERT_AT(struct part, fx,             0x16);
DG_ASSERT_AT(struct part, word_18,        0x18);
DG_ASSERT_AT(struct part, fy,             0x1a);
DG_ASSERT_AT(struct part, word_1c,        0x1c);
DG_ASSERT_AT(struct part, pos_x,          0x1e);
DG_ASSERT_AT(struct part, pos_y,          0x20);
DG_ASSERT_AT(struct part, word_22,        0x22);
DG_ASSERT_AT(struct part, word_24,        0x24);
DG_ASSERT_AT(struct part, word_26,        0x26);
DG_ASSERT_AT(struct part, word_28,        0x28);
DG_ASSERT_AT(struct part, box_x,          0x2a);
DG_ASSERT_AT(struct part, box_y,          0x2c);
DG_ASSERT_AT(struct part, word_2e,        0x2e);
DG_ASSERT_AT(struct part, word_32,        0x32);
DG_ASSERT_AT(struct part, vel_x,          0x36);
DG_ASSERT_AT(struct part, word_38,        0x38);
DG_ASSERT_AT(struct part, weight,         0x3a);
DG_ASSERT_AT(struct part, momentum_lo,    0x3c);
DG_ASSERT_AT(struct part, momentum_hi,    0x3e);
DG_ASSERT_AT(struct part, word_40,        0x40);
DG_ASSERT_AT(struct part, word_42,        0x42);
DG_ASSERT_AT(struct part, width,          0x44);
DG_ASSERT_AT(struct part, height,         0x46);
DG_ASSERT_AT(struct part, word_48,        0x48);
DG_ASSERT_AT(struct part, word_4c,        0x4c);
DG_ASSERT_AT(struct part, word_50,        0x50);
DG_ASSERT_AT(struct part, word_52,        0x52);
DG_ASSERT_AT(struct part, word_54,        0x54);
DG_ASSERT_AT(struct part, grab_x,         0x56);
DG_ASSERT_AT(struct part, grab_y,         0x57);
DG_ASSERT_AT(struct part, word_58,        0x58);
DG_ASSERT_AT(struct part, link_right,     0x5a);
DG_ASSERT_AT(struct part, link_left,      0x5c);
DG_ASSERT_AT(struct part, link_down,      0x5e);
DG_ASSERT_AT(struct part, link_up,        0x60);
DG_ASSERT_AT(struct part, linked_a,       0x62);
DG_ASSERT_AT(struct part, linked_b,       0x64);
DG_ASSERT_AT(struct part, word_66,        0x66);
DG_ASSERT_AT(struct part, word_68,        0x68);
DG_ASSERT_AT(struct part, byte_6a,        0x6a);
DG_ASSERT_AT(struct part, byte_6b,        0x6b);
DG_ASSERT_AT(struct part, byte_6c,        0x6c);
DG_ASSERT_AT(struct part, byte_6d,        0x6d);
DG_ASSERT_AT(struct part, byte_72,        0x72);
DG_ASSERT_AT(struct part, byte_73,        0x73);
DG_ASSERT_AT(struct part, word_78,        0x78);
DG_ASSERT_AT(struct part, word_74,        0x74);
DG_ASSERT_AT(struct part, word_76,        0x76);
DG_ASSERT_AT(struct part, word_7a,        0x7a);
DG_ASSERT_AT(struct part, word_7c,        0x7c);
DG_ASSERT_AT(struct part, byte_7e,        0x7e);
DG_ASSERT_AT(struct part, byte_7f,        0x7f);
DG_ASSERT_AT(struct part, point_count,    0x80);
DG_ASSERT_AT(struct part, points_ptr,     0x82);
DG_ASSERT_AT(struct part, word_84,        0x84);
DG_ASSERT_AT(struct part, word_8a,        0x8a);
DG_ASSERT_AT(struct part, word_8c,        0x8c);
DG_ASSERT_AT(struct part, word_8e,        0x8e);
DG_ASSERT_AT(struct part, word_90,        0x90);
DG_ASSERT_AT(struct part, word_92,        0x92);
DG_ASSERT_AT(struct part, word_94,        0x94);
DG_ASSERT_AT(struct part, word_96,        0x96);
DG_ASSERT_AT(struct part, word_98,        0x98);
DG_ASSERT_AT(struct part, word_9a,        0x9a);
DG_ASSERT_AT(struct part, spin,           0x9c);
DG_ASSERT_AT(struct part, word_9e,        0x9e);
DG_ASSERT_AT(struct part, word_a0,        0xa0);
_Static_assert(sizeof(struct part) == 0xa2,
               "a part is 0xa2 bytes - game.c reads `n` of them off the near heap");

/*
 * **The level reader, the archive, and its one-entry cache**, at DGROUP 0x546c.
 */
struct dg_546c {
    dg_off_t  table_off;          /* +0x00  the far pointer the list reader allocates and frees */
    dg_seg_t  table_seg;          /* +0x02 */
    uint16_t  record_count;       /* +0x04  how many records of 0xa2 bytes came off the near heap */
    uint16_t  is_level;           /* +0x06  load_level sets it; save_machine zeroes it. It decides how much of a record is written and read */
    int16_t   version;            /* +0x08  the version gate: from 0x101 the file carries more */
    uint16_t  version_out;        /* +0x0a  written out beside it */
    uint16_t  error;              /* +0x0c  every writer checks it, and a file that fails to close is deleted */
    int16_t   cache_key;          /* +0x0e  the one-entry cache in front of find_entry_for_pointer: */
    int16_t   cache_answer;       /* +0x10  the pointer last asked about, and the answer */
    int16_t   archive_count;      /* +0x12  how many archives, accumulated; zero means none is open */
    uint16_t  last_record;        /* +0x14  where the search starts, so record 0 is never returned */
    int16_t   name_hash;          /* +0x16  what hash_filename leaves for find_entry_for_pointer */
    int16_t   word_5484;          /* +0x18 */
    uint8_t   open_immediate;     /* +0x1a  clear means try the file by name and close it again */
    uint8_t   byte_5487;          /* +0x1b */
    uint8_t   retry;              /* +0x1c  the loop around the loose-file open, for removable media */
    uint8_t   byte_5489;          /* +0x1d */
    uint8_t   scanned;            /* +0x1e  the archives have been counted once */
    int16_t   file_used;          /* +0x1f  the FILE it actually read from */
    int16_t   file_asked;         /* +0x21  and the one it was asked about */
} __attribute__((packed));

#define DG546C (*(volatile struct dg_546c *)(dgroup + 0x546c))

DG_ASSERT_AT(struct dg_546c, table_off,         0x00);
DG_ASSERT_AT(struct dg_546c, table_seg,         0x02);
DG_ASSERT_AT(struct dg_546c, record_count,      0x04);
DG_ASSERT_AT(struct dg_546c, is_level,          0x06);
DG_ASSERT_AT(struct dg_546c, version,           0x08);
DG_ASSERT_AT(struct dg_546c, version_out,       0x0a);
DG_ASSERT_AT(struct dg_546c, error,             0x0c);
DG_ASSERT_AT(struct dg_546c, cache_key,         0x0e);
DG_ASSERT_AT(struct dg_546c, cache_answer,      0x10);
DG_ASSERT_AT(struct dg_546c, archive_count,     0x12);
DG_ASSERT_AT(struct dg_546c, last_record,       0x14);
DG_ASSERT_AT(struct dg_546c, name_hash,         0x16);
DG_ASSERT_AT(struct dg_546c, word_5484,         0x18);
DG_ASSERT_AT(struct dg_546c, open_immediate,    0x1a);
DG_ASSERT_AT(struct dg_546c, byte_5487,         0x1b);
DG_ASSERT_AT(struct dg_546c, retry,             0x1c);
DG_ASSERT_AT(struct dg_546c, byte_5489,         0x1d);
DG_ASSERT_AT(struct dg_546c, scanned,           0x1e);
DG_ASSERT_AT(struct dg_546c, file_used,         0x1f);
DG_ASSERT_AT(struct dg_546c, file_asked,        0x21);

/*
 * **The mouse driver and the video mode the program found**, at DGROUP 0x48da.
 */
struct dg_48da {
    int16_t   gc_0_1;             /* +0x00  the graphics controller registers the cursor code saves: */
    int16_t   gc_4;               /* +0x02  0 and 1 here, 4 next, then 8 */
    int16_t   gc_8;               /* +0x04 */
    int16_t   seq_map_mask;       /* +0x06  and the sequencer's map mask */
    uint8_t   gc_3;               /* +0x08 */
    uint8_t   pad_48e3[3];
    uint8_t   word_48e6;          /* +0x0c */
    uint8_t   quarter_a;          /* +0x0d  a second pair, a quarter of each of the two */
    uint8_t   word_48e8;          /* +0x0e */
    uint8_t   quarter_b;          /* +0x0f */
    uint8_t   mouse_taken;        /* +0x10  whether the driver was taken; `neg al` branches on it */
    uint8_t   buttons;            /* +0x11  the byte timer_callback samples on the page flip */
    uint8_t   vector_hooked;      /* +0x12  the handler after this routine was installed */
    int16_t   vector_seg;         /* +0x13  vector 0's segment, from 0:2 - a load, not a store */
    int16_t   vector_off;         /* +0x15  and its offset, from 0:0 */
    uint8_t   pad_48f1[1];
    uint8_t   mode_found;         /* +0x18  the mode the program found the adapter in */
    uint8_t   mode_forced;        /* +0x19  a forced setting; 0xd is the one these screens take */
    dg_off_t  driver_off;         /* +0x1a  the video driver, as vm_init stored it */
    dg_seg_t  driver_seg;         /* +0x1c */
} __attribute__((packed));

#define DG48DA (*(volatile struct dg_48da *)(dgroup + 0x48da))

DG_ASSERT_AT(struct dg_48da, gc_0_1,            0x00);
DG_ASSERT_AT(struct dg_48da, gc_4,              0x02);
DG_ASSERT_AT(struct dg_48da, gc_8,              0x04);
DG_ASSERT_AT(struct dg_48da, seq_map_mask,      0x06);
DG_ASSERT_AT(struct dg_48da, gc_3,              0x08);
DG_ASSERT_AT(struct dg_48da, word_48e6,         0x0c);
DG_ASSERT_AT(struct dg_48da, quarter_a,         0x0d);
DG_ASSERT_AT(struct dg_48da, word_48e8,         0x0e);
DG_ASSERT_AT(struct dg_48da, quarter_b,         0x0f);
DG_ASSERT_AT(struct dg_48da, mouse_taken,       0x10);
DG_ASSERT_AT(struct dg_48da, buttons,           0x11);
DG_ASSERT_AT(struct dg_48da, vector_hooked,     0x12);
DG_ASSERT_AT(struct dg_48da, vector_seg,        0x13);
DG_ASSERT_AT(struct dg_48da, vector_off,        0x15);
DG_ASSERT_AT(struct dg_48da, mode_found,        0x18);
DG_ASSERT_AT(struct dg_48da, mode_forced,       0x19);
DG_ASSERT_AT(struct dg_48da, driver_off,        0x1a);
DG_ASSERT_AT(struct dg_48da, driver_seg,        0x1c);

/*
 * **The cursor, the fade, and the palette waiting to load**, at DGROUP 0x2d32.
 */
struct dg_2d32 {
    uint16_t  page;               /* +0x00  the page the middle call passes */
    uint16_t  screen_disturbed;   /* +0x02  the saved rectangles are put back when this says so */
    uint16_t  word_2d36;          /* +0x04 */
    uint16_t  word_2d38;          /* +0x06 */
    dg_off_t  pending_pal_off;    /* +0x08  a palette waiting to be loaded */
    dg_seg_t  pending_pal_seg;    /* +0x0a */
    uint16_t  cursor_off;         /* +0x0c  clear turns the whole cursor off - nothing is drawn */
    int16_t   delay_reload;       /* +0x0e  the delay counts down and is reloaded from here */
    uint16_t  read_driver;        /* +0x10  take the position from the driver rather than the last known */
    uint16_t  flag_2d44;          /* +0x12  what clear_flag_2d44 zeroes, and nothing else */
    int16_t   word_2d46;          /* +0x14 */
} __attribute__((packed));

#define DG2D32 (*(volatile struct dg_2d32 *)(dgroup + 0x2d32))

DG_ASSERT_AT(struct dg_2d32, page,              0x00);
DG_ASSERT_AT(struct dg_2d32, screen_disturbed,  0x02);
DG_ASSERT_AT(struct dg_2d32, word_2d36,         0x04);
DG_ASSERT_AT(struct dg_2d32, word_2d38,         0x06);
DG_ASSERT_AT(struct dg_2d32, pending_pal_off,   0x08);
DG_ASSERT_AT(struct dg_2d32, pending_pal_seg,   0x0a);
DG_ASSERT_AT(struct dg_2d32, cursor_off,        0x0c);
DG_ASSERT_AT(struct dg_2d32, delay_reload,      0x0e);
DG_ASSERT_AT(struct dg_2d32, read_driver,       0x10);
DG_ASSERT_AT(struct dg_2d32, flag_2d44,         0x12);
DG_ASSERT_AT(struct dg_2d32, word_2d46,         0x14);

/*
 * **The scratch block that is allocated to be freed**, at DGROUP 0x3576.
 */
struct dg_3576 {
    dg_off_t  scratch_off;        /* +0x00  picker_begin takes this if it is not null */
    dg_seg_t  scratch_seg;        /* +0x02 */
} __attribute__((packed));

#define DG3576 (*(volatile struct dg_3576 *)(dgroup + 0x3576))

DG_ASSERT_AT(struct dg_3576, scratch_off,       0x00);
DG_ASSERT_AT(struct dg_3576, scratch_seg,       0x02);

/*
 * **The bit buffer the decompressors read through**, at DGROUP 0x3600.
 */
struct dg_3600 {
    int16_t   bits;               /* +0x00  filled from the top; bits come off the **left** */
    uint8_t   bit_count;          /* +0x02  how many are in it */
} __attribute__((packed));

#define DG3600 (*(volatile struct dg_3600 *)(dgroup + 0x3600))

DG_ASSERT_AT(struct dg_3600, bits,              0x00);
DG_ASSERT_AT(struct dg_3600, bit_count,         0x02);

/*
 * **The three cached far pointers and the LZSS init flag**, at DGROUP 0x590a.
 */
struct dg_590a {
    dg_off_t  cache_a_off;        /* +0x00  the three records' far pointers, cached */
    dg_seg_t  cache_a_seg;        /* +0x02 */
    dg_off_t  cache_b_off;        /* +0x04 */
    dg_seg_t  cache_b_seg;        /* +0x06 */
    dg_off_t  cache_c_off;        /* +0x08  pointed at the record's own block */
    dg_seg_t  cache_c_seg;        /* +0x0a */
    uint8_t   pad_5916[2];
    int16_t   lzss_ready;         /* +0x0e  cleared so decompress_lzss builds its tree and fills its ring */
} __attribute__((packed));

#define DG590A (*(volatile struct dg_590a *)(dgroup + 0x590a))

DG_ASSERT_AT(struct dg_590a, cache_a_off,       0x00);
DG_ASSERT_AT(struct dg_590a, cache_a_seg,       0x02);
DG_ASSERT_AT(struct dg_590a, cache_b_off,       0x04);
DG_ASSERT_AT(struct dg_590a, cache_b_seg,       0x06);
DG_ASSERT_AT(struct dg_590a, cache_c_off,       0x08);
DG_ASSERT_AT(struct dg_590a, cache_c_seg,       0x0a);
DG_ASSERT_AT(struct dg_590a, lzss_ready,        0x0e);

/*
 * **The base the two indexes are taken from**, at DGROUP 0x628e.
 */
struct dg_628e {
    uint16_t  base;               /* +0x00  one `n` further on, less the one this indexes */
    uint16_t  word_6290;          /* +0x02 */
} __attribute__((packed));

#define DG628E (*(volatile struct dg_628e *)(dgroup + 0x628e))

DG_ASSERT_AT(struct dg_628e, base,              0x00);
DG_ASSERT_AT(struct dg_628e, word_6290,         0x02);

/*
 * **The polygon walker's two chains**, at DGROUP 0x44d0.
 */
struct dg_44d0 {
    uint16_t  word_44d0;          /* +0x00 */
    uint16_t  word_44d2;          /* +0x02 */
    uint16_t  word_44d4;          /* +0x04 */
    uint16_t  word_44d6;          /* +0x06 */
    uint16_t  word_44d8;          /* +0x08 */
    uint16_t  word_44da;          /* +0x0a */
    uint16_t  chain;              /* +0x0c  0 is the left chain and 2 the right; a computed jmp on it */
} __attribute__((packed));

#define DG44D0 (*(volatile struct dg_44d0 *)(dgroup + 0x44d0))

DG_ASSERT_AT(struct dg_44d0, word_44d0,         0x00);
DG_ASSERT_AT(struct dg_44d0, word_44d2,         0x02);
DG_ASSERT_AT(struct dg_44d0, word_44d4,         0x04);
DG_ASSERT_AT(struct dg_44d0, word_44d6,         0x06);
DG_ASSERT_AT(struct dg_44d0, word_44d8,         0x08);
DG_ASSERT_AT(struct dg_44d0, word_44da,         0x0a);
DG_ASSERT_AT(struct dg_44d0, chain,             0x0c);

/*
 * **The machine's own parts**, at DGROUP 0x521b.
 */
struct dg_521b {
    dg_off_t  parts_ptr;          /* +0x00  every part on the machine; 0x5179 is the moving ones */
    dg_off_t  parts_tail_ptr;     /* +0x02 */
} __attribute__((packed));

#define DG521B (*(volatile struct dg_521b *)(dgroup + 0x521b))

DG_ASSERT_AT(struct dg_521b, parts_ptr,         0x00);
DG_ASSERT_AT(struct dg_521b, parts_tail_ptr,    0x02);

/*
 * **Not established**, at DGROUP 0x0126.
 */
struct dg_0126 {
    uint8_t   word_0126;          /* +0x00 */
    uint8_t   byte_0127;          /* +0x01 */
    uint8_t   pad_0128[3];
    uint8_t   byte_012b;          /* +0x05 */
    uint8_t   byte_012c;          /* +0x06 */
} __attribute__((packed));

#define DG0126 (*(volatile struct dg_0126 *)(dgroup + 0x0126))

DG_ASSERT_AT(struct dg_0126, word_0126,         0x00);
DG_ASSERT_AT(struct dg_0126, byte_0127,         0x01);
DG_ASSERT_AT(struct dg_0126, byte_012b,         0x05);
DG_ASSERT_AT(struct dg_0126, byte_012c,         0x06);

/*
 * **Not established**, at DGROUP 0x1bca.
 */
struct dg_1bca {
    uint16_t  word_1bca;          /* +0x00 */
} __attribute__((packed));

#define DG1BCA (*(volatile struct dg_1bca *)(dgroup + 0x1bca))

DG_ASSERT_AT(struct dg_1bca, word_1bca,         0x00);

/*
 * **Not established**, at DGROUP 0x259c.
 */
struct dg_259c {
    uint16_t  word_259c;          /* +0x00 */
} __attribute__((packed));

#define DG259C (*(volatile struct dg_259c *)(dgroup + 0x259c))

DG_ASSERT_AT(struct dg_259c, word_259c,         0x00);

/*
 * **Not established**, at DGROUP 0x25d6.
 */
struct dg_25d6 {
    uint16_t  word_25d6;          /* +0x00 */
} __attribute__((packed));

#define DG25D6 (*(volatile struct dg_25d6 *)(dgroup + 0x25d6))

DG_ASSERT_AT(struct dg_25d6, word_25d6,         0x00);

/*
 * **Not established**, at DGROUP 0x260a.
 */
struct dg_260a {
    uint16_t  word_260a;          /* +0x00 */
} __attribute__((packed));

#define DG260A (*(volatile struct dg_260a *)(dgroup + 0x260a))

DG_ASSERT_AT(struct dg_260a, word_260a,         0x00);

/*
 * **Not established**, at DGROUP 0x2630.
 */
struct dg_2630 {
    uint16_t  word_2630;          /* +0x00 */
    uint16_t  word_2632;          /* +0x02 */
    uint16_t  word_2634;          /* +0x04 */
} __attribute__((packed));

#define DG2630 (*(volatile struct dg_2630 *)(dgroup + 0x2630))

DG_ASSERT_AT(struct dg_2630, word_2630,         0x00);
DG_ASSERT_AT(struct dg_2630, word_2632,         0x02);
DG_ASSERT_AT(struct dg_2630, word_2634,         0x04);

/*
 * **Not established**, at DGROUP 0x27ee.
 */
struct dg_27ee {
    uint16_t  word_27ee;          /* +0x00 */
} __attribute__((packed));

#define DG27EE (*(volatile struct dg_27ee *)(dgroup + 0x27ee))

DG_ASSERT_AT(struct dg_27ee, word_27ee,         0x00);

/*
 * **Not established**, at DGROUP 0x286e.
 */
struct dg_286e {
    int16_t   word_286e;          /* +0x00 */
} __attribute__((packed));

#define DG286E (*(volatile struct dg_286e *)(dgroup + 0x286e))

DG_ASSERT_AT(struct dg_286e, word_286e,         0x00);

/*
 * **Not established**, at DGROUP 0x28fa.
 */
struct dg_28fa {
    uint16_t  word_28fa;          /* +0x00 */
} __attribute__((packed));

#define DG28FA (*(volatile struct dg_28fa *)(dgroup + 0x28fa))

DG_ASSERT_AT(struct dg_28fa, word_28fa,         0x00);

/*
 * **Not established**, at DGROUP 0x2d76.
 */
struct dg_2d76 {
    uint8_t   word_2d76;          /* +0x00 */
    uint16_t  word_2d77;          /* +0x01 */
    uint16_t  word_2d79;          /* +0x03 */
    int16_t   word_2d7b;          /* +0x05 */
} __attribute__((packed));

#define DG2D76 (*(volatile struct dg_2d76 *)(dgroup + 0x2d76))

DG_ASSERT_AT(struct dg_2d76, word_2d76,         0x00);
DG_ASSERT_AT(struct dg_2d76, word_2d77,         0x01);
DG_ASSERT_AT(struct dg_2d76, word_2d79,         0x03);
DG_ASSERT_AT(struct dg_2d76, word_2d7b,         0x05);

/*
 * **The far-block table and the clipper's count**, at DGROUP 0x3a2c.
 */
struct dg_3a2c {
    uint16_t  clip_count;         /* +0x00  Sutherland and Hodgman's, rewritten after each edge */
    uint16_t  blocks_off;         /* +0x02  nine slots of four bytes, searched from 1 for a free one; */
    uint16_t  blocks_seg;         /* +0x04  the driver reaches the segment half as driverDS:0x1a0 */
} __attribute__((packed));

#define DG3A2C (*(volatile struct dg_3a2c *)(dgroup + 0x3a2c))

DG_ASSERT_AT(struct dg_3a2c, clip_count,        0x00);
DG_ASSERT_AT(struct dg_3a2c, blocks_off,        0x02);
DG_ASSERT_AT(struct dg_3a2c, blocks_seg,        0x04);

/*
 * **Not established**, at DGROUP 0x4342.
 */
struct dg_4342 {
    uint16_t  word_4342;          /* +0x00 */
    int16_t   word_4344;          /* +0x02 */
} __attribute__((packed));

#define DG4342 (*(volatile struct dg_4342 *)(dgroup + 0x4342))

DG_ASSERT_AT(struct dg_4342, word_4342,         0x00);
DG_ASSERT_AT(struct dg_4342, word_4344,         0x02);

/*
 * **Not established**, at DGROUP 0x4460.
 */
struct dg_4460 {
    uint16_t  word_4460;          /* +0x00 */
    uint16_t  word_4462;          /* +0x02 */
    int16_t   word_4464;          /* +0x04 */
} __attribute__((packed));

#define DG4460 (*(volatile struct dg_4460 *)(dgroup + 0x4460))

DG_ASSERT_AT(struct dg_4460, word_4460,         0x00);
DG_ASSERT_AT(struct dg_4460, word_4462,         0x02);
DG_ASSERT_AT(struct dg_4460, word_4464,         0x04);

/*
 * **Not established**, at DGROUP 0x44c2.
 */
struct dg_44c2 {
    uint16_t  word_44c2;          /* +0x00 */
    uint16_t  word_44c4;          /* +0x02 */
} __attribute__((packed));

#define DG44C2 (*(volatile struct dg_44c2 *)(dgroup + 0x44c2))

DG_ASSERT_AT(struct dg_44c2, word_44c2,         0x00);
DG_ASSERT_AT(struct dg_44c2, word_44c4,         0x02);

/*
 * **Not established**, at DGROUP 0x44de.
 */
struct dg_44de {
    int16_t   word_44de;          /* +0x00 */
    int16_t   word_44e0;          /* +0x02 */
    uint16_t  word_44e2;          /* +0x04 */
    uint16_t  word_44e4;          /* +0x06 */
    uint16_t  word_44e6;          /* +0x08 */
    uint8_t   byte_44e8;          /* +0x0a */
    uint8_t   byte_44e9;          /* +0x0b */
} __attribute__((packed));

#define DG44DE (*(volatile struct dg_44de *)(dgroup + 0x44de))

DG_ASSERT_AT(struct dg_44de, word_44de,         0x00);
DG_ASSERT_AT(struct dg_44de, word_44e0,         0x02);
DG_ASSERT_AT(struct dg_44de, word_44e2,         0x04);
DG_ASSERT_AT(struct dg_44de, word_44e4,         0x06);
DG_ASSERT_AT(struct dg_44de, word_44e6,         0x08);
DG_ASSERT_AT(struct dg_44de, byte_44e8,         0x0a);
DG_ASSERT_AT(struct dg_44de, byte_44e9,         0x0b);

/*
 * **Not established**, at DGROUP 0x458c.
 */
struct dg_458c {
    uint8_t   word_458c;          /* +0x00 */
    uint8_t   byte_458d;          /* +0x01 */
    uint16_t  word_458e;          /* +0x02 */
} __attribute__((packed));

#define DG458C (*(volatile struct dg_458c *)(dgroup + 0x458c))

DG_ASSERT_AT(struct dg_458c, word_458c,         0x00);
DG_ASSERT_AT(struct dg_458c, byte_458d,         0x01);
DG_ASSERT_AT(struct dg_458c, word_458e,         0x02);

/*
 * **Not established**, at DGROUP 0x4740.
 */
struct dg_4740 {
    uint16_t  word_4740;          /* +0x00 */
    uint16_t  word_4742;          /* +0x02 */
    uint16_t  word_4744;          /* +0x04 */
    uint16_t  word_4746;          /* +0x06 */
} __attribute__((packed));

#define DG4740 (*(volatile struct dg_4740 *)(dgroup + 0x4740))

DG_ASSERT_AT(struct dg_4740, word_4740,         0x00);
DG_ASSERT_AT(struct dg_4740, word_4742,         0x02);
DG_ASSERT_AT(struct dg_4740, word_4744,         0x04);
DG_ASSERT_AT(struct dg_4740, word_4746,         0x06);

/*
 * **Not established**, at DGROUP 0x48f8.
 */
struct dg_48f8 {
    uint16_t  word_48f8;          /* +0x00 */
    uint16_t  word_48fa;          /* +0x02 */
} __attribute__((packed));

#define DG48F8 (*(volatile struct dg_48f8 *)(dgroup + 0x48f8))

DG_ASSERT_AT(struct dg_48f8, word_48f8,         0x00);
DG_ASSERT_AT(struct dg_48f8, word_48fa,         0x02);

/*
 * **Not established**, at DGROUP 0x4bb8.
 */
struct dg_4bb8 {
    int16_t   word_4bb8;          /* +0x00 */
    int16_t   word_4bba;          /* +0x02 */
    int16_t   word_4bbc;          /* +0x04 */
    int16_t   word_4bbe;          /* +0x06 */
} __attribute__((packed));

#define DG4BB8 (*(volatile struct dg_4bb8 *)(dgroup + 0x4bb8))

DG_ASSERT_AT(struct dg_4bb8, word_4bb8,         0x00);
DG_ASSERT_AT(struct dg_4bb8, word_4bba,         0x02);
DG_ASSERT_AT(struct dg_4bb8, word_4bbc,         0x04);
DG_ASSERT_AT(struct dg_4bb8, word_4bbe,         0x06);

/*
 * **Not established**, at DGROUP 0x4bc6.
 */
struct dg_4bc6 {
    uint16_t  word_4bc6;          /* +0x00 */
    uint8_t   byte_4bc8;          /* +0x02 */
} __attribute__((packed));

#define DG4BC6 (*(volatile struct dg_4bc6 *)(dgroup + 0x4bc6))

DG_ASSERT_AT(struct dg_4bc6, word_4bc6,         0x00);
DG_ASSERT_AT(struct dg_4bc6, byte_4bc8,         0x02);

/*
 * **Not established**, at DGROUP 0x4bd6.
 */
struct dg_4bd6 {
    uint16_t  word_4bd6;          /* +0x00 */
    uint8_t   byte_4bd8;          /* +0x02 */
} __attribute__((packed));

#define DG4BD6 (*(volatile struct dg_4bd6 *)(dgroup + 0x4bd6))

DG_ASSERT_AT(struct dg_4bd6, word_4bd6,         0x00);
DG_ASSERT_AT(struct dg_4bd6, byte_4bd8,         0x02);

/*
 * **Not established**, at DGROUP 0x4d04.
 */
struct dg_4d04 {
    uint16_t  word_4d04;          /* +0x00 */
} __attribute__((packed));

#define DG4D04 (*(volatile struct dg_4d04 *)(dgroup + 0x4d04))

DG_ASSERT_AT(struct dg_4d04, word_4d04,         0x00);

/*
 * **Not established**, at DGROUP 0x4d2e.
 */
struct dg_4d2e {
    uint16_t  word_4d2e;          /* +0x00 */
    uint16_t  word_4d30;          /* +0x02 */
    uint8_t   pad_4d32[2];
    int16_t   word_4d34;          /* +0x06 */
} __attribute__((packed));

#define DG4D2E (*(volatile struct dg_4d2e *)(dgroup + 0x4d2e))

DG_ASSERT_AT(struct dg_4d2e, word_4d2e,         0x00);
DG_ASSERT_AT(struct dg_4d2e, word_4d30,         0x02);
DG_ASSERT_AT(struct dg_4d2e, word_4d34,         0x06);

/*
 * **Not established**, at DGROUP 0x4e99.
 */
struct dg_4e99 {
    int16_t   word_4e99;          /* +0x00 */
    int16_t   word_4e9b;          /* +0x02 */
    int16_t   word_4e9d;          /* +0x04 */
    int16_t   word_4e9f;          /* +0x06 */
} __attribute__((packed));

#define DG4E99 (*(volatile struct dg_4e99 *)(dgroup + 0x4e99))

DG_ASSERT_AT(struct dg_4e99, word_4e99,         0x00);
DG_ASSERT_AT(struct dg_4e99, word_4e9b,         0x02);
DG_ASSERT_AT(struct dg_4e99, word_4e9d,         0x04);
DG_ASSERT_AT(struct dg_4e99, word_4e9f,         0x06);

/*
 * **Not established**, at DGROUP 0x53ab.
 */
struct dg_53ab {
    uint8_t   word_53ab;          /* +0x00 */
    uint8_t   byte_53ac;          /* +0x01 */
    uint8_t   byte_53ad;          /* +0x02 */
    uint8_t   byte_53ae;          /* +0x03 */
} __attribute__((packed));

#define DG53AB (*(volatile struct dg_53ab *)(dgroup + 0x53ab))

DG_ASSERT_AT(struct dg_53ab, word_53ab,         0x00);
DG_ASSERT_AT(struct dg_53ab, byte_53ac,         0x01);
DG_ASSERT_AT(struct dg_53ab, byte_53ad,         0x02);
DG_ASSERT_AT(struct dg_53ab, byte_53ae,         0x03);

/*
 * **The critical-error vector and the picker's caret**, at DGROUP 0x5677.
 */
struct dg_5677 {
    dg_off_t  crit_vec_off;       /* +0x00  DOS's 24h, kept so it can be put back */
    dg_seg_t  crit_vec_seg;       /* +0x02 */
    uint16_t  failures;           /* +0x04  **or-ed, not set**: this layer accumulates its failures here */
    uint8_t   pad_567d[1];
    uint16_t  caret_blink;        /* +0x07  bumped on every pass; the caret is `*` */
    uint16_t  caret_blink_b;      /* +0x09  a different counter, and a different asterisk at 0x2954 */
} __attribute__((packed));

#define DG5677 (*(volatile struct dg_5677 *)(dgroup + 0x5677))

DG_ASSERT_AT(struct dg_5677, crit_vec_off,      0x00);
DG_ASSERT_AT(struct dg_5677, crit_vec_seg,      0x02);
DG_ASSERT_AT(struct dg_5677, failures,          0x04);
DG_ASSERT_AT(struct dg_5677, caret_blink,       0x07);
DG_ASSERT_AT(struct dg_5677, caret_blink_b,     0x09);

/*
 * ---------------------------------------------------------------------------
 * **What one video page has covered up.** `claim_page_slot` keeps two of these
 * - at DGROUP 0x56e6 and 0x5706, 0x20 apart, which is the size - and hands back
 * whichever already belongs to the page asked for, matching on bits 0xa800 of
 * the segment. With two pages in flight each needs its own record of what it
 * saved, because a restore has to put back what *that* page covered.
 *
 * The two blocks at +0x08 and +0x14 are the same twelve bytes twice, and that
 * is not a guess about their shape: `erase_object` and `restage_object_rect`
 * are line-for-line the same routine over the two, each testing bit 1 of the
 * flags byte, each handing x, y, w and h to `restore_rect_thunk` with the
 * buffer taken from the table at 0x5754 indexed by the field two words along,
 * and each falling back to `plot_pixel_clipped` with the colour byte when the
 * buffer index is zero.
 *
 * Field names are ours. The offsets, the size and the two-block shape are the
 * original's.
 * ---------------------------------------------------------------------------
 */
struct saved_rect {
    int16_t   x;               /* +0x00 */
    int16_t   y;               /* +0x02 */
    int16_t   w;               /* +0x04 */
    int16_t   h;               /* +0x06 */
    uint16_t  buf;             /* +0x08  index into the table at 0x5754, shifted
                                         left two; zero means a single pixel */
    uint8_t   pixel;           /* +0x0a  that pixel's colour */
    uint8_t   flags;           /* +0x0b  bit 1 says something is saved */
} __attribute__((packed));

_Static_assert(sizeof(struct saved_rect) == 0x0c, "a saved rect is twelve bytes");

struct page_slot {
    dg_seg_t  page;            /* +0x00  the page this slot belongs to */
    int16_t   word_02;         /* +0x02 */
    int16_t   word_04;         /* +0x04 */
    int16_t   word_06;         /* +0x06 */
    struct saved_rect obj;     /* +0x08  what the object covered */
    struct saved_rect cursor;  /* +0x14  what the cursor covered */
} __attribute__((packed));

DG_ASSERT_AT(struct page_slot, obj,           0x08);
DG_ASSERT_AT(struct page_slot, cursor,        0x14);
_Static_assert(sizeof(struct page_slot) == 0x20,
               "claim_page_slot strides by 0x20");

#define PAGESLOT(p) (*(volatile struct page_slot *)(dgroup + (uint16_t)(p)))

/*
 * **Not established**, at DGROUP 0x56e0.
 */
struct dg_56e0 {
    uint16_t  word_56e0;          /* +0x00 */
    int16_t   word_56e2;          /* +0x02 */
    int16_t   word_56e4;          /* +0x04 */
    struct page_slot slot[2];     /* +0x06  DGROUP 0x56e6 and 0x5706 */
} __attribute__((packed));

#define DG56E0 (*(volatile struct dg_56e0 *)(dgroup + 0x56e0))

DG_ASSERT_AT(struct dg_56e0, word_56e0,         0x00);
DG_ASSERT_AT(struct dg_56e0, word_56e2,         0x02);
DG_ASSERT_AT(struct dg_56e0, word_56e4,         0x04);
DG_ASSERT_AT(struct dg_56e0, slot,              0x06);

/*
 * **The drawing state saved across an interrupt**, at DGROUP 0x5726.
 */
struct dg_5726 {
    uint16_t  saved_a;            /* +0x00  seven values - the clip box and the two page segments. */
    int16_t   saved_b;            /* +0x02  0x5726's high half is always zero: it is restored as a byte */
    int16_t   saved_c;            /* +0x04 */
    int16_t   saved_d;            /* +0x06 */
    int16_t   saved_e;            /* +0x08 */
    uint16_t  saved_f;            /* +0x0a */
    uint16_t  saved_g;            /* +0x0c */
} __attribute__((packed));

#define DG5726 (*(volatile struct dg_5726 *)(dgroup + 0x5726))

DG_ASSERT_AT(struct dg_5726, saved_a,           0x00);
DG_ASSERT_AT(struct dg_5726, saved_b,           0x02);
DG_ASSERT_AT(struct dg_5726, saved_c,           0x04);
DG_ASSERT_AT(struct dg_5726, saved_d,           0x06);
DG_ASSERT_AT(struct dg_5726, saved_e,           0x08);
DG_ASSERT_AT(struct dg_5726, saved_f,           0x0a);
DG_ASSERT_AT(struct dg_5726, saved_g,           0x0c);

/*
 * **The palette request and the fade**, at DGROUP 0x5738.
 */
struct dg_5738 {
    dg_off_t  request_off;        /* +0x00  cleared when taken, so one request loads once */
    dg_seg_t  request_seg;        /* +0x02 */
    uint16_t  fade_mark;          /* +0x04  reset to zero by a load, which forces the fade to run; */
    int16_t   word_573e;          /* +0x06  the fade runs only while it differs from 0x5786 */
    int16_t   busy;               /* +0x08  non-zero suppresses the slot release, and everything waits on it */
} __attribute__((packed));

#define DG5738 (*(volatile struct dg_5738 *)(dgroup + 0x5738))

DG_ASSERT_AT(struct dg_5738, request_off,       0x00);
DG_ASSERT_AT(struct dg_5738, request_seg,       0x02);
DG_ASSERT_AT(struct dg_5738, fade_mark,         0x04);
DG_ASSERT_AT(struct dg_5738, word_573e,         0x06);
DG_ASSERT_AT(struct dg_5738, busy,              0x08);

/*
 * **The resource reader's flag bits and its handler index**, at DGROUP 0x57ba.
 */
struct dg_57ba {
    uint8_t   flags;              /* +0x00  bit 0x40 makes the copy happen at all; bit 0x20 picks 0x1cd2c */
    uint8_t   pad_57bb[1];
    uint16_t  word_57bc;          /* +0x02 */
    uint8_t   handler;            /* +0x04  the low five bits of the byte, indexing a table of handlers */
} __attribute__((packed));

#define DG57BA (*(volatile struct dg_57ba *)(dgroup + 0x57ba))

DG_ASSERT_AT(struct dg_57ba, flags,             0x00);
DG_ASSERT_AT(struct dg_57ba, word_57bc,         0x02);
DG_ASSERT_AT(struct dg_57ba, handler,           0x04);

/*
 * **Not established**, at DGROUP 0x58e8.
 */
struct dg_58e8 {
    uint16_t  word_58e8;          /* +0x00 */
    uint16_t  word_58ea;          /* +0x02 */
    int16_t   word_58ec;          /* +0x04 */
    int16_t   word_58ee;          /* +0x06 */
    int16_t   word_58f0;          /* +0x08 */
} __attribute__((packed));

#define DG58E8 (*(volatile struct dg_58e8 *)(dgroup + 0x58e8))

DG_ASSERT_AT(struct dg_58e8, word_58e8,         0x00);
DG_ASSERT_AT(struct dg_58e8, word_58ea,         0x02);
DG_ASSERT_AT(struct dg_58e8, word_58ec,         0x04);
DG_ASSERT_AT(struct dg_58e8, word_58ee,         0x06);
DG_ASSERT_AT(struct dg_58e8, word_58f0,         0x08);

/*
 * **Not established**, at DGROUP 0x5900.
 */
struct dg_5900 {
    uint16_t  word_5900;          /* +0x00 */
    int16_t   word_5902;          /* +0x02 */
} __attribute__((packed));

#define DG5900 (*(volatile struct dg_5900 *)(dgroup + 0x5900))

DG_ASSERT_AT(struct dg_5900, word_5900,         0x00);
DG_ASSERT_AT(struct dg_5900, word_5902,         0x02);

/*
 * **Not established**, at DGROUP 0x6176.
 */
struct dg_6176 {
    uint8_t   word_6176;          /* +0x00 */
} __attribute__((packed));

#define DG6176 (*(volatile struct dg_6176 *)(dgroup + 0x6176))

DG_ASSERT_AT(struct dg_6176, word_6176,         0x00);

/*
 * **The font table**, at DGROUP 0x618a.
 */
struct dg_618a {
    dg_off_t  fonts_off;          /* +0x00  eighteen slots; a font's body goes here with DGROUP as */
    dg_seg_t  fonts_seg;          /* +0x02  its segment, leaving the other two pointers null */
    int16_t   word_618e;          /* +0x04 */
    int16_t   word_6190;          /* +0x06 */
} __attribute__((packed));

#define DG618A (*(volatile struct dg_618a *)(dgroup + 0x618a))

DG_ASSERT_AT(struct dg_618a, fonts_off,         0x00);
DG_ASSERT_AT(struct dg_618a, fonts_seg,         0x02);
DG_ASSERT_AT(struct dg_618a, word_618e,         0x04);
DG_ASSERT_AT(struct dg_618a, word_6190,         0x06);

/*
 * **The font's width table**, at DGROUP 0x61da.
 */
struct dg_61da {
    dg_off_t  widths_off;         /* +0x00  `les bx,[0x61da]` loads the segment too, so the width */
    dg_seg_t  widths_seg;         /* +0x02  is a far read */
} __attribute__((packed));

#define DG61DA (*(volatile struct dg_61da *)(dgroup + 0x61da))

DG_ASSERT_AT(struct dg_61da, widths_off,        0x00);
DG_ASSERT_AT(struct dg_61da, widths_seg,        0x02);

/*
 * **Not established**, at DGROUP 0x622a.
 */
struct dg_622a {
    uint16_t  word_622a;          /* +0x00 */
    uint16_t  word_622c;          /* +0x02 */
} __attribute__((packed));

#define DG622A (*(volatile struct dg_622a *)(dgroup + 0x622a))

DG_ASSERT_AT(struct dg_622a, word_622a,         0x00);
DG_ASSERT_AT(struct dg_622a, word_622c,         0x02);

/*
 * **Not established**, at DGROUP 0x6400.
 */
struct dg_6400 {
    uint16_t  word_6400;          /* +0x00 */
    uint16_t  word_6402;          /* +0x02 */
    uint16_t  word_6404;          /* +0x04 */
    uint16_t  word_6406;          /* +0x06 */
    uint16_t  word_6408;          /* +0x08 */
    uint8_t   pad_640a[2];
    uint16_t  word_640c;          /* +0x0c */
} __attribute__((packed));

#define DG6400 (*(volatile struct dg_6400 *)(dgroup + 0x6400))

DG_ASSERT_AT(struct dg_6400, word_6400,         0x00);
DG_ASSERT_AT(struct dg_6400, word_6402,         0x02);
DG_ASSERT_AT(struct dg_6400, word_6404,         0x04);
DG_ASSERT_AT(struct dg_6400, word_6406,         0x06);
DG_ASSERT_AT(struct dg_6400, word_6408,         0x08);
DG_ASSERT_AT(struct dg_6400, word_640c,         0x0c);

/*
 * **Not established**, at DGROUP 0x6414.
 */
struct dg_6414 {
    uint16_t  word_6414;          /* +0x00 */
    uint16_t  word_6416;          /* +0x02 */
} __attribute__((packed));

#define DG6414 (*(volatile struct dg_6414 *)(dgroup + 0x6414))

DG_ASSERT_AT(struct dg_6414, word_6414,         0x00);
DG_ASSERT_AT(struct dg_6414, word_6416,         0x02);

/*
 * **The character being drawn**, at DGROUP 0x64c8.
 */
struct dg_64c8 {
    uint8_t   character;          /* +0x00  filed here before anything else, and it stays */
} __attribute__((packed));

#define DG64C8 (*(volatile struct dg_64c8 *)(dgroup + 0x64c8))

DG_ASSERT_AT(struct dg_64c8, character,         0x00);

/*
 * ---------------------------------------------------------------------------
 * **What is deliberately still a raw offset**, and why - about forty accesses
 * over twenty-odd offsets, down from 3,085.
 *
 * Four are word writes into the font's byte tables: `DG16(0x38d8) = 0x808`
 * sets two slots in one instruction, which is what the original does. Writing
 * `font_table_48[0] = 8; font_table_48[1] = 8;` would be two instructions and
 * a different transcription, so the raw form stays and says what the original
 * says.
 *
 * The rest are single accesses at offsets with no neighbour inside 0x40 - the
 * Borland runtime's `__brklvl` at 0x9c, the two current-directory buffers the
 * DOS wrappers are handed by offset, a handful of one-off words. A struct
 * whose only field is the offset it is named for tells a reader nothing the
 * offset did not, and `tools/dgrules.py --rule raw` lists them whenever that
 * stops being true.
 * ---------------------------------------------------------------------------
 */

/*
 * NOT a transcription: DGROUP's own segment number, which the original never
 * has to compute because it is sitting in SS and DS. A routine that takes the
 * address of a local and then treats it as a far pointer - `mov [bp-2],ss` -
 * needs the segment half, and this is where it comes from.
 */
#define DGROUP_SEG        ((uint16_t)(dgroup_base >> 4))

/*
 * The sound module keeps its state in **its own code segment**, segment 0x2619,
 * the same way the video driver keeps its data inside DGROUP. `SND8`/`SND16`
 * reach it. The image base is derived from `dgroup_base` because that is the
 * one thing tools/verify.py sets from the run it captured.
 *
 * The sound driver is a separate loaded block, and its address is not a
 * constant: the game holds a far pointer to it at the sound module's own
 * `cs:[0x1e7]`. `SX_SEG` reads that rather than hard-coding the 0x418f seen in
 * these runs, so the port follows the loader wherever it puts the driver.
 */
#define IMAGE_BASE  (dgroup_base - 0x2D3C0)
/*
 * Segment 0x1c25 keeps a little of its own state inside its code, the way the
 * sound module does - the saved timer vector, and the divisor table the tick
 * handler reads. `S1C8`/`S1C16` reach it.
 */
#define S1C25       (IMAGE_BASE + 0x1c250)
#define S1C8(off)   (*(uint8_t  *)(guest_mem + S1C25 + (off)))
#define S1C16(off)  (*(int16_t  *)(guest_mem + S1C25 + (off)))

#define SNDCS       (IMAGE_BASE + 0x26190)

#define SND8(off)   (*(uint8_t  *)(guest_mem + SNDCS + (off)))
#define SND16(off)  (*(int16_t  *)(guest_mem + SNDCS + (off)))

#define SX_SEG      (*(uint16_t *)(guest_mem + SNDCS + 0x1e9))
#define SX8(off)    (*(uint8_t  *)FAR_PTR(SX_SEG, (off)))
#define SX16(off)   (*(int16_t  *)FAR_PTR(SX_SEG, (off)))

/*
 * The **loaded sound module** is a second block, separate from the driver and
 * loaded before it: `setup_sound_device` puts its far pointer in DGROUP at
 * 0x4a98/0x4a9a - **the offset first and the segment second**, which is what
 * the `lcall [0x4a98]` at 0x0bbde reads - and every call goes through it. Like the driver it keeps all its state in its own code segment, so
 * `ASB8`/`ASB16` are to the module what `SX8`/`SX16` are to the driver.
 *
 * The name is the tag of the one module this port transcribes, `ASB:` - the
 * digitised-sound half of a Sound Blaster. The table at DGROUP 0x4a2e names
 * four more and the port has none of them.
 */
#define ASB_SEG     DG4A82.module_seg
#define ASB_OFF     DG4A82.module_off
#define ASB8(off)   (*(uint8_t  *)FAR_PTR(ASB_SEG, ASB_OFF + (off)))
#define ASB16(off)  (*(int16_t  *)FAR_PTR(ASB_SEG, ASB_OFF + (off)))
#define ASBU16(off) (*(uint16_t *)FAR_PTR(ASB_SEG, ASB_OFF + (off)))

/*
 * NOT a transcription: a stand-in for the guest's own stack frame.
 *
 * In the large model SS and DS are the same segment, so a local whose address
 * is taken - `lea ax,[bp-0x34]` - hands out an ordinary DGROUP offset, and a
 * routine that receives one cannot tell it from a pointer to a global. Several
 * transcribed routines build a structure on the stack and pass its offset to
 * another transcribed routine, and a C local cannot serve: it is not in
 * `guest_mem` and has no DGROUP offset at all.
 *
 * So the port carries a stack pointer of its own. `dg_enter` reserves bytes
 * below it and answers the offset of the low end; `dg_leave` gives them back.
 * tools/verify.py sets `guest_sp` to whatever the original's SP was at the
 * routine's entry, so the port's frame lands inside the range the verifier
 * already excludes from comparison - the bytes the call used as its stack.
 * Nothing is read back from a frame after `dg_leave`.
 */
extern uint16_t guest_sp;

void     dg_call(uint16_t bytes);
void     dg_uncall(uint16_t bytes);
uint16_t dg_enter(uint16_t bytes);
void     dg_leave(uint16_t bytes);

/*
 * **The sound module's own code segment, which is where it keeps its state**
 */
struct snd_cs {
    uint8_t   pad_0000[10];
    int16_t   word_000a;          /* +0x000a */
    uint8_t   pad_000c[60];
    int16_t   poll_table;         /* +0x0048  the table remove_sequence checks; **not** the playing table */
    int16_t   word_004a;          /* +0x004a */
    uint8_t   pad_004c[411];
    dg_off_t  driver_off;         /* +0x01e7  the cell every call far-calls through, with the */
    dg_seg_t  driver_seg;         /* +0x01e9  function number in BP */
    uint8_t   pad_01eb[12];
    int16_t   cursor_park;        /* +0x01f7  BP is the cursor, so it is parked here */
    uint8_t   busy;               /* +0x01f9  in on the way in, out on the way out; non-zero refuses the call */
    uint8_t   voice_lo;           /* +0x01fa  a sequence needs a voice inside this range */
    uint8_t   voice_hi;           /* +0x01fb */
    uint8_t   ch;                 /* +0x01fc  CH, and the device a table entry must match */
    uint8_t   own_voice;          /* +0x01fd  a channel with bit 1 at +0x134 is its own voice */
    uint8_t   bend_gate;          /* +0x01fe  non-zero drops a channel whose byte at cs:0x128 is not 0xff */
    uint8_t   cl;                 /* +0x01ff  CL */
    uint8_t   ah_high;            /* +0x0200  AH >> 4; clear marks the channel 0xfe and skips it */
    uint8_t   slot_high;          /* +0x0201  the sequence's slot times four */
    uint8_t   param_default;      /* +0x0202  the default function 11 is given */
    uint8_t   word_0203;          /* +0x0203 */
    uint8_t   voices_changed;     /* +0x0204  something changed which voice plays what */
    uint8_t   defer;              /* +0x0205  set leaves the new value in the pending array at cs:0x1c8 */
    uint8_t   scan_stopped;       /* +0x0206  at most two a call, so the scan stops where it is */
    uint8_t   pad_0207[2];
    uint8_t   muted;              /* +0x0209  set stops the muting and leaves the counters alone */
    uint8_t   pad_020a[2];
    uint8_t   scratch_mark;       /* +0x020c  0xff, set with the sixteen words at cs:0x108 */
    uint8_t   pad_020d[12009];
    dg_off_t  callback_off;       /* +0x30f6  the cell sound_callback calls through */
    dg_seg_t  callback_seg;       /* +0x30f8 */
    int16_t   answer;             /* +0x30fa  parked before the registers are popped and read back */
} __attribute__((packed));

#define SNDS (*(volatile struct snd_cs *)(guest_mem + SNDCS))

_Static_assert(__builtin_offsetof(struct snd_cs, word_000a) == 0x000a, "snd_cs.word_000a");
_Static_assert(__builtin_offsetof(struct snd_cs, poll_table) == 0x0048, "snd_cs.poll_table");
_Static_assert(__builtin_offsetof(struct snd_cs, word_004a) == 0x004a, "snd_cs.word_004a");
_Static_assert(__builtin_offsetof(struct snd_cs, driver_off) == 0x01e7, "snd_cs.driver_off");
_Static_assert(__builtin_offsetof(struct snd_cs, driver_seg) == 0x01e9, "snd_cs.driver_seg");
_Static_assert(__builtin_offsetof(struct snd_cs, cursor_park) == 0x01f7, "snd_cs.cursor_park");
_Static_assert(__builtin_offsetof(struct snd_cs, busy) == 0x01f9, "snd_cs.busy");
_Static_assert(__builtin_offsetof(struct snd_cs, voice_lo) == 0x01fa, "snd_cs.voice_lo");
_Static_assert(__builtin_offsetof(struct snd_cs, voice_hi) == 0x01fb, "snd_cs.voice_hi");
_Static_assert(__builtin_offsetof(struct snd_cs, ch) == 0x01fc, "snd_cs.ch");
_Static_assert(__builtin_offsetof(struct snd_cs, own_voice) == 0x01fd, "snd_cs.own_voice");
_Static_assert(__builtin_offsetof(struct snd_cs, bend_gate) == 0x01fe, "snd_cs.bend_gate");
_Static_assert(__builtin_offsetof(struct snd_cs, cl) == 0x01ff, "snd_cs.cl");
_Static_assert(__builtin_offsetof(struct snd_cs, ah_high) == 0x0200, "snd_cs.ah_high");
_Static_assert(__builtin_offsetof(struct snd_cs, slot_high) == 0x0201, "snd_cs.slot_high");
_Static_assert(__builtin_offsetof(struct snd_cs, param_default) == 0x0202, "snd_cs.param_default");
_Static_assert(__builtin_offsetof(struct snd_cs, word_0203) == 0x0203, "snd_cs.word_0203");
_Static_assert(__builtin_offsetof(struct snd_cs, voices_changed) == 0x0204, "snd_cs.voices_changed");
_Static_assert(__builtin_offsetof(struct snd_cs, defer) == 0x0205, "snd_cs.defer");
_Static_assert(__builtin_offsetof(struct snd_cs, scan_stopped) == 0x0206, "snd_cs.scan_stopped");
_Static_assert(__builtin_offsetof(struct snd_cs, muted) == 0x0209, "snd_cs.muted");
_Static_assert(__builtin_offsetof(struct snd_cs, scratch_mark) == 0x020c, "snd_cs.scratch_mark");
_Static_assert(__builtin_offsetof(struct snd_cs, callback_off) == 0x30f6, "snd_cs.callback_off");
_Static_assert(__builtin_offsetof(struct snd_cs, callback_seg) == 0x30f8, "snd_cs.callback_seg");
_Static_assert(__builtin_offsetof(struct snd_cs, answer) == 0x30fa, "snd_cs.answer");

/*
 * **The digitised-sound module's own code segment**
 */
struct asb_cs {
    uint8_t   pad_0000[52];
    uint8_t   word_0034;          /* +0x0034 */
    uint8_t   word_0035;          /* +0x0035 */
    uint8_t   word_0036;          /* +0x0036 */
    uint8_t   pad_0037[1];
    uint8_t   word_0038;          /* +0x0038 */
    uint8_t   word_0039;          /* +0x0039 */
    uint8_t   pad_003a[1];
    uint8_t   word_003b;          /* +0x003b */
    uint8_t   pad_003c[1];
    uint8_t   word_003d;          /* +0x003d */
    uint8_t   word_003e;          /* +0x003e */
    uint8_t   word_003f;          /* +0x003f */
    uint8_t   word_0040;          /* +0x0040 */
    uint8_t   word_0041;          /* +0x0041 */
    uint8_t   word_0042;          /* +0x0042 */
    uint8_t   word_0043;          /* +0x0043 */
    uint8_t   word_0044;          /* +0x0044 */
    uint8_t   word_0045;          /* +0x0045 */
    uint8_t   word_0046;          /* +0x0046 */
    uint8_t   word_0047;          /* +0x0047 */
    uint8_t   pad_0048[1];
    uint8_t   word_0049;          /* +0x0049 */
    uint8_t   pad_004a[2];
    uint8_t   half;               /* +0x004c  which half is current; `xor ...,1` flips it */
    uint8_t   nothing_to_report;  /* +0x004d */
    uint8_t   word_004e;          /* +0x004e */
    uint8_t   irq10_worth;        /* +0x004f  what later decides whether IRQ 10 is worth trying */
    uint8_t   pad_0050[2];
    uint8_t   word_0052;          /* +0x0052 */
    uint8_t   pad_0053[1];
    uint8_t   word_0054;          /* +0x0054 */
    uint8_t   pad_0055[1];
    int16_t   word_0056;          /* +0x0056 */
    uint16_t  word_0058;          /* +0x0058 */
    int16_t   word_005a;          /* +0x005a */
    int16_t   word_005c;          /* +0x005c */
    uint8_t   pad_005e[6];
    uint16_t  word_0064;          /* +0x0064 */
    uint16_t  word_0066;          /* +0x0066 */
    uint8_t   pad_0068[4];
    int16_t   word_006c;          /* +0x006c */
    int16_t   word_006e;          /* +0x006e */
    int16_t   word_0070;          /* +0x0070 */
    int16_t   word_0072;          /* +0x0072 */
    int16_t   word_0074;          /* +0x0074  zero in the module's own image; 0x07be writes 0x21 into it */
    int16_t   base;               /* +0x0076  the card's base port, which every other port is an offset from */
    int16_t   word_0078;          /* +0x0078 */
    int16_t   word_007a;          /* +0x007a */
    uint8_t   pad_007c[4];
    uint16_t  word_0080;          /* +0x0080 */
    uint16_t  word_0082;          /* +0x0082 */
    int16_t   word_0084;          /* +0x0084 */
    uint8_t   pad_0086[8];
    int16_t   word_008e;          /* +0x008e */
    int16_t   word_0090;          /* +0x0090 */
    int16_t   word_0092;          /* +0x0092 */
    int16_t   word_0094;          /* +0x0094 */
    int16_t   word_0096;          /* +0x0096 */
    int16_t   word_0098;          /* +0x0098 */
    int16_t   word_009a;          /* +0x009a */
    int16_t   word_009c;          /* +0x009c */
    int16_t   word_009e;          /* +0x009e */
    int16_t   word_00a0;          /* +0x00a0 */
    int16_t   word_00a2;          /* +0x00a2 */
    int16_t   word_00a4;          /* +0x00a4 */
    uint8_t   pad_00a6[1811];
    uint8_t   word_07b9;          /* +0x07b9 */
    uint8_t   word_07ba;          /* +0x07ba */
    uint8_t   word_07bb;          /* +0x07bb */
    uint8_t   word_07bc;          /* +0x07bc */
    uint8_t   word_07bd;          /* +0x07bd */
} __attribute__((packed));

#define ASBS (*(volatile struct asb_cs *)FAR_PTR(ASB_SEG, ASB_OFF))

_Static_assert(__builtin_offsetof(struct asb_cs, word_0034) == 0x0034, "asb_cs.word_0034");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0035) == 0x0035, "asb_cs.word_0035");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0036) == 0x0036, "asb_cs.word_0036");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0038) == 0x0038, "asb_cs.word_0038");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0039) == 0x0039, "asb_cs.word_0039");
_Static_assert(__builtin_offsetof(struct asb_cs, word_003b) == 0x003b, "asb_cs.word_003b");
_Static_assert(__builtin_offsetof(struct asb_cs, word_003d) == 0x003d, "asb_cs.word_003d");
_Static_assert(__builtin_offsetof(struct asb_cs, word_003e) == 0x003e, "asb_cs.word_003e");
_Static_assert(__builtin_offsetof(struct asb_cs, word_003f) == 0x003f, "asb_cs.word_003f");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0040) == 0x0040, "asb_cs.word_0040");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0041) == 0x0041, "asb_cs.word_0041");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0042) == 0x0042, "asb_cs.word_0042");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0043) == 0x0043, "asb_cs.word_0043");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0044) == 0x0044, "asb_cs.word_0044");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0045) == 0x0045, "asb_cs.word_0045");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0046) == 0x0046, "asb_cs.word_0046");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0047) == 0x0047, "asb_cs.word_0047");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0049) == 0x0049, "asb_cs.word_0049");
_Static_assert(__builtin_offsetof(struct asb_cs, half) == 0x004c, "asb_cs.half");
_Static_assert(__builtin_offsetof(struct asb_cs, nothing_to_report) == 0x004d, "asb_cs.nothing_to_report");
_Static_assert(__builtin_offsetof(struct asb_cs, word_004e) == 0x004e, "asb_cs.word_004e");
_Static_assert(__builtin_offsetof(struct asb_cs, irq10_worth) == 0x004f, "asb_cs.irq10_worth");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0052) == 0x0052, "asb_cs.word_0052");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0054) == 0x0054, "asb_cs.word_0054");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0056) == 0x0056, "asb_cs.word_0056");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0058) == 0x0058, "asb_cs.word_0058");
_Static_assert(__builtin_offsetof(struct asb_cs, word_005a) == 0x005a, "asb_cs.word_005a");
_Static_assert(__builtin_offsetof(struct asb_cs, word_005c) == 0x005c, "asb_cs.word_005c");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0064) == 0x0064, "asb_cs.word_0064");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0066) == 0x0066, "asb_cs.word_0066");
_Static_assert(__builtin_offsetof(struct asb_cs, word_006c) == 0x006c, "asb_cs.word_006c");
_Static_assert(__builtin_offsetof(struct asb_cs, word_006e) == 0x006e, "asb_cs.word_006e");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0070) == 0x0070, "asb_cs.word_0070");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0072) == 0x0072, "asb_cs.word_0072");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0074) == 0x0074, "asb_cs.word_0074");
_Static_assert(__builtin_offsetof(struct asb_cs, base) == 0x0076, "asb_cs.base");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0078) == 0x0078, "asb_cs.word_0078");
_Static_assert(__builtin_offsetof(struct asb_cs, word_007a) == 0x007a, "asb_cs.word_007a");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0080) == 0x0080, "asb_cs.word_0080");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0082) == 0x0082, "asb_cs.word_0082");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0084) == 0x0084, "asb_cs.word_0084");
_Static_assert(__builtin_offsetof(struct asb_cs, word_008e) == 0x008e, "asb_cs.word_008e");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0090) == 0x0090, "asb_cs.word_0090");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0092) == 0x0092, "asb_cs.word_0092");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0094) == 0x0094, "asb_cs.word_0094");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0096) == 0x0096, "asb_cs.word_0096");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0098) == 0x0098, "asb_cs.word_0098");
_Static_assert(__builtin_offsetof(struct asb_cs, word_009a) == 0x009a, "asb_cs.word_009a");
_Static_assert(__builtin_offsetof(struct asb_cs, word_009c) == 0x009c, "asb_cs.word_009c");
_Static_assert(__builtin_offsetof(struct asb_cs, word_009e) == 0x009e, "asb_cs.word_009e");
_Static_assert(__builtin_offsetof(struct asb_cs, word_00a0) == 0x00a0, "asb_cs.word_00a0");
_Static_assert(__builtin_offsetof(struct asb_cs, word_00a2) == 0x00a2, "asb_cs.word_00a2");
_Static_assert(__builtin_offsetof(struct asb_cs, word_00a4) == 0x00a4, "asb_cs.word_00a4");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07b9) == 0x07b9, "asb_cs.word_07b9");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07ba) == 0x07ba, "asb_cs.word_07ba");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07bb) == 0x07bb, "asb_cs.word_07bb");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07bc) == 0x07bc, "asb_cs.word_07bc");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07bd) == 0x07bd, "asb_cs.word_07bd");

/*
 * **Segment 1c25, which keeps the displaced timer vector inside its own code**
 */
struct s1c_cs {
    uint8_t   pad_0000[17517];
    dg_off_t  old_int8_off;       /* +0x446d  the INT 08h vector timer_install displaced */
    dg_seg_t  old_int8_seg;       /* +0x446f */
    uint8_t   pad_4471[2507];
    int16_t   word_4e3c;          /* +0x4e3c */
    int16_t   word_4e3e;          /* +0x4e3e */
    int16_t   word_4e40;          /* +0x4e40 */
    int16_t   word_4e42;          /* +0x4e42 */
    uint8_t   pad_4e44[4437];
    int16_t   word_5f99;          /* +0x5f99 */
    int16_t   word_5f9b;          /* +0x5f9b */
} __attribute__((packed));

#define S1CS (*(volatile struct s1c_cs *)(guest_mem + S1C25))

_Static_assert(__builtin_offsetof(struct s1c_cs, old_int8_off) == 0x446d, "s1c_cs.old_int8_off");
_Static_assert(__builtin_offsetof(struct s1c_cs, old_int8_seg) == 0x446f, "s1c_cs.old_int8_seg");
_Static_assert(__builtin_offsetof(struct s1c_cs, word_4e3c) == 0x4e3c, "s1c_cs.word_4e3c");
_Static_assert(__builtin_offsetof(struct s1c_cs, word_4e3e) == 0x4e3e, "s1c_cs.word_4e3e");
_Static_assert(__builtin_offsetof(struct s1c_cs, word_4e40) == 0x4e40, "s1c_cs.word_4e40");
_Static_assert(__builtin_offsetof(struct s1c_cs, word_4e42) == 0x4e42, "s1c_cs.word_4e42");
_Static_assert(__builtin_offsetof(struct s1c_cs, word_5f99) == 0x5f99, "s1c_cs.word_5f99");
_Static_assert(__builtin_offsetof(struct s1c_cs, word_5f9b) == 0x5f9b, "s1c_cs.word_5f9b");

/*
 * **The PC speaker driver**, laid over whatever `SX_SEG` points at.
 *
 * One struct per driver, and that is the point: `SX_SEG` is whichever
 * chunk the loader put there, and the three have different layouts. A
 * single overlay would be right for one of them and quietly wrong for the
 * other two - they share only 0x188d, and that by coincidence.
 */
struct sx_spkr {
    uint8_t   pad_0000[828];
    int16_t   word_033c;          /* +0x033c */
    uint8_t   pad_033e[4];
    uint8_t   byte_0342;          /* +0x0342 */
    uint8_t   byte_0343;          /* +0x0343 */
    uint8_t   byte_0344;          /* +0x0344 */
    uint8_t   byte_0345;          /* +0x0345 */
    uint8_t   byte_0346;          /* +0x0346 */
    uint8_t   byte_0347;          /* +0x0347 */
    uint8_t   byte_0348;          /* +0x0348 */
    uint8_t   byte_0349;          /* +0x0349 */
} __attribute__((packed));

#define SXSPKR (*(volatile struct sx_spkr *)FAR_PTR(SX_SEG, 0))

_Static_assert(__builtin_offsetof(struct sx_spkr, word_033c) == 0x033c, "sx_spkr.word_033c");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0342) == 0x0342, "sx_spkr.byte_0342");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0343) == 0x0343, "sx_spkr.byte_0343");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0344) == 0x0344, "sx_spkr.byte_0344");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0345) == 0x0345, "sx_spkr.byte_0345");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0346) == 0x0346, "sx_spkr.byte_0346");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0347) == 0x0347, "sx_spkr.byte_0347");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0348) == 0x0348, "sx_spkr.byte_0348");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0349) == 0x0349, "sx_spkr.byte_0349");

/*
 * **The AdLib driver**, laid over whatever `SX_SEG` points at.
 *
 * One struct per driver, and that is the point: `SX_SEG` is whichever
 * chunk the loader put there, and the three have different layouts. A
 * single overlay would be right for one of them and quietly wrong for the
 * other two - they share only 0x188d, and that by coincidence.
 */
struct sx_adl {
    uint8_t   pad_0000[55];
    int16_t   word_0037;          /* +0x0037 */
    int16_t   word_0039;          /* +0x0039 */
    int16_t   word_003b;          /* +0x003b */
    uint8_t   pad_003d[224];
    uint8_t   byte_011d;          /* +0x011d */
    uint8_t   byte_011e;          /* +0x011e */
    uint8_t   byte_011f;          /* +0x011f */
    uint8_t   pad_0120[175];
    uint8_t   byte_01cf;          /* +0x01cf */
    uint8_t   pad_01d0[64];
    uint8_t   byte_0210;          /* +0x0210  which slots do not own an operator */
    uint8_t   pad_0211[17];
    uint8_t   byte_0222;          /* +0x0222  the channel each slot is on */
    uint8_t   pad_0223[17];
    uint8_t   byte_0234;          /* +0x0234  the channel table */
    uint8_t   pad_0235[317];
    int16_t   word_0372;          /* +0x0372  the patch bank the driver was given */
    uint8_t   pad_0374[5396];
    uint8_t   byte_1888;          /* +0x1888 */
    uint8_t   byte_1889;          /* +0x1889 */
    uint8_t   byte_188a;          /* +0x188a */
    uint8_t   pad_188b[1];
    uint8_t   byte_188c;          /* +0x188c */
    int16_t   word_188d;          /* +0x188d */
} __attribute__((packed));

#define SXADL (*(volatile struct sx_adl *)FAR_PTR(SX_SEG, 0))

_Static_assert(__builtin_offsetof(struct sx_adl, word_0037) == 0x0037, "sx_adl.word_0037");
_Static_assert(__builtin_offsetof(struct sx_adl, word_0039) == 0x0039, "sx_adl.word_0039");
_Static_assert(__builtin_offsetof(struct sx_adl, word_003b) == 0x003b, "sx_adl.word_003b");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_011d) == 0x011d, "sx_adl.byte_011d");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_011e) == 0x011e, "sx_adl.byte_011e");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_011f) == 0x011f, "sx_adl.byte_011f");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_01cf) == 0x01cf, "sx_adl.byte_01cf");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_0210) == 0x0210, "sx_adl.byte_0210");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_0222) == 0x0222, "sx_adl.byte_0222");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_0234) == 0x0234, "sx_adl.byte_0234");
_Static_assert(__builtin_offsetof(struct sx_adl, word_0372) == 0x0372, "sx_adl.word_0372");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_1888) == 0x1888, "sx_adl.byte_1888");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_1889) == 0x1889, "sx_adl.byte_1889");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_188a) == 0x188a, "sx_adl.byte_188a");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_188c) == 0x188c, "sx_adl.byte_188c");
_Static_assert(__builtin_offsetof(struct sx_adl, word_188d) == 0x188d, "sx_adl.word_188d");

/*
 * **The Sound Blaster Pro driver**, laid over whatever `SX_SEG` points at.
 *
 * One struct per driver, and that is the point: `SX_SEG` is whichever
 * chunk the loader put there, and the three have different layouts. A
 * single overlay would be right for one of them and quietly wrong for the
 * other two - they share only 0x188d, and that by coincidence.
 */
struct sx_sbp {
    uint8_t   pad_0000[44];
    int16_t   word_002c;          /* +0x002c */
    int16_t   word_002e;          /* +0x002e */
    int16_t   word_0030;          /* +0x0030 */
    int16_t   word_0032;          /* +0x0032 */
    int16_t   word_0034;          /* +0x0034 */
    int16_t   word_0036;          /* +0x0036 */
    int16_t   word_0038;          /* +0x0038 */
    int16_t   word_003a;          /* +0x003a */
    int16_t   word_003c;          /* +0x003c */
    int16_t   word_003e;          /* +0x003e */
    int16_t   word_0040;          /* +0x0040 */
    uint8_t   pad_0042[224];
    uint8_t   byte_0122;          /* +0x0122 */
    uint8_t   byte_0123;          /* +0x0123 */
    uint8_t   byte_0124;          /* +0x0124 */
    uint8_t   pad_0125[175];
    uint8_t   byte_01d4;          /* +0x01d4 */
    uint8_t   pad_01d5[45];
    uint8_t   byte_0202;          /* +0x0202 */
    uint8_t   pad_0203[372];
    int16_t   word_0377;          /* +0x0377 */
    uint8_t   pad_0379[5396];
    uint8_t   byte_188d;          /* +0x188d */
    uint8_t   byte_188e;          /* +0x188e */
    uint8_t   byte_188f;          /* +0x188f */
    uint8_t   pad_1890[1];
    uint8_t   byte_1891;          /* +0x1891 */
    int16_t   word_1892;          /* +0x1892 */
} __attribute__((packed));

#define SXSBP (*(volatile struct sx_sbp *)FAR_PTR(SX_SEG, 0))

_Static_assert(__builtin_offsetof(struct sx_sbp, word_002c) == 0x002c, "sx_sbp.word_002c");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_002e) == 0x002e, "sx_sbp.word_002e");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_0030) == 0x0030, "sx_sbp.word_0030");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_0032) == 0x0032, "sx_sbp.word_0032");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_0034) == 0x0034, "sx_sbp.word_0034");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_0036) == 0x0036, "sx_sbp.word_0036");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_0038) == 0x0038, "sx_sbp.word_0038");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_003a) == 0x003a, "sx_sbp.word_003a");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_003c) == 0x003c, "sx_sbp.word_003c");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_003e) == 0x003e, "sx_sbp.word_003e");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_0040) == 0x0040, "sx_sbp.word_0040");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_0122) == 0x0122, "sx_sbp.byte_0122");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_0123) == 0x0123, "sx_sbp.byte_0123");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_0124) == 0x0124, "sx_sbp.byte_0124");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_01d4) == 0x01d4, "sx_sbp.byte_01d4");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_0202) == 0x0202, "sx_sbp.byte_0202");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_0377) == 0x0377, "sx_sbp.word_0377");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_188d) == 0x188d, "sx_sbp.byte_188d");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_188e) == 0x188e, "sx_sbp.byte_188e");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_188f) == 0x188f, "sx_sbp.byte_188f");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_1891) == 0x1891, "sx_sbp.byte_1891");
_Static_assert(__builtin_offsetof(struct sx_sbp, word_1892) == 0x1892, "sx_sbp.word_1892");

/*
 * ---------------------------------------------------------------------------
 * **A screen region**, the 0x1a-byte record `build_screen_regions` cuts
 * thirty-six of onto five lists.
 *
 * Every field is read off `regions_handle_pointer`, which is the only routine
 * that walks one: it tests +0x02 against the state word, the pointer's x
 * against +0x06 and +0x0a and its y against +0x08 and +0x0c, calls the far
 * pointer at +0x12 whenever the pointer is inside, takes the cursor from
 * +0x0e, and on a click calls +0x16 and writes +0x10 into the state. The link
 * is +0x00, which is what `si = DGU16(si)` walks.
 *
 * The size is the one CLAUDE.md records, and the table in machine.c prints its
 * columns in the same order.
 * ---------------------------------------------------------------------------
 */
struct region {
    dg_off_t  link_ptr;        /* +0x00  the next record on this list */
    uint16_t  mask;            /* +0x02  and-ed with the state word at 0x4e6b */
    uint16_t  word_04;         /* +0x04 */
    int16_t   x0;              /* +0x06  the rectangle, inclusive at both ends */
    int16_t   y0;              /* +0x08 */
    int16_t   x1;              /* +0x0a */
    int16_t   y1;              /* +0x0c */
    uint16_t  cursor;          /* +0x0e  which cursor while the pointer is in it */
    uint16_t  code;            /* +0x10  written into the state word on a click */
    dg_off_t  hover_off;       /* +0x12  called whenever the pointer is inside */
    dg_seg_t  hover_seg;       /* +0x14 */
    dg_off_t  click_off;       /* +0x16  and this one on the click itself */
    dg_seg_t  click_seg;       /* +0x18 */
} __attribute__((packed));

#define REGION(p) (*(volatile struct region *)(dgroup + (uint16_t)(p)))

DG_ASSERT_AT(struct region, link_ptr,   0x00);
DG_ASSERT_AT(struct region, mask,       0x02);
DG_ASSERT_AT(struct region, x0,         0x06);
DG_ASSERT_AT(struct region, y0,         0x08);
DG_ASSERT_AT(struct region, x1,         0x0a);
DG_ASSERT_AT(struct region, y1,         0x0c);
DG_ASSERT_AT(struct region, cursor,     0x0e);
DG_ASSERT_AT(struct region, code,       0x10);
DG_ASSERT_AT(struct region, hover_off,  0x12);
DG_ASSERT_AT(struct region, click_off,  0x16);
_Static_assert(sizeof(struct region) == 0x1a,
               "a region is 0x1a bytes - build_screen_regions cuts thirty-six");

/*
 * ---------------------------------------------------------------------------
 * **A `FILE`**, the Borland stream structure - twenty of which the runtime
 * keeps, and which `flush_all` walks.
 *
 * The fields are the ones `buffered_read`'s own note lists: +0 the bytes left
 * in the buffer, +2 the flags, +4 the DOS handle, +6 the buffer size, +0xa the
 * read pointer. The handle is a *byte* and is read signed in three places,
 * which is what makes -1 mean "no handle".
 * ---------------------------------------------------------------------------
 */
struct file_rec {
    int16_t   left;            /* +0x00  bytes still in the buffer */
    uint16_t  flags;           /* +0x02  0x40 is the one buffered_read tests */
    uint8_t   handle;          /* +0x04  the DOS handle, read signed for -1 */
    uint8_t   pad_05;          /* +0x05 */
    uint16_t  buf_size;        /* +0x06 */
    uint16_t  word_08;         /* +0x08 */
    dg_off_t  read_ptr;        /* +0x0a  where the next byte comes from */
    uint16_t  word_0c;         /* +0x0c */
    uint16_t  word_0e;         /* +0x0e */
} __attribute__((packed));

#define FILEREC(p) (*(volatile struct file_rec *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **An open file**, the 0x43-byte record the game keeps four of at DGROUP
 * 0x6292 - not Borland's `FILE`, which is `struct file_rec` above and is what
 * `+0x00` points at.
 *
 * `find_file_record` gives the stride and `copy_file_record` the size: both say
 * 0x43, one as `0x6292 + 0x43 * i` and the other as a `far_move` of that many
 * bytes.
 *
 * `seek_named_chunk` determines the middle, which no other routine touches.
 * Descending into a container it reads the four-character tag to
 * `si + depth + 2`, steps `depth` by 4, terminates the string with a zero at
 * the new `si + depth + 2`, and files the chunk's end in `si + depth + 0x1b`.
 * So the two arrays share one index: `path` is the chunk names walked into,
 * written end to end the way "BMP:SCN:" reads, and `bound` is where each of
 * those chunks stops. `depth` is in bytes and the routine gives up at 0x18,
 * which is what makes both arrays seven entries and the record end where it
 * does.
 *
 * `open_file_record` sets `bound[0]` to the file's own length with 0x8000 set
 * in the high word, and `reset_file_record` clears all 0x43 bytes *except*
 * that entry and the pointer at +0x00 - which is what lets it be used on a
 * record being reused as well as one being made.
 *
 * Field names are ours; the offsets and the size are the original's.
 * ---------------------------------------------------------------------------
 */
struct chunk_bound {
    uint16_t lo;               /* +0x00 */
    uint16_t hi;               /* +0x02  the top bit is a flag, masked before use */
} __attribute__((packed));

struct open_file {
    dg_off_t file_ptr;         /* +0x00  the Borland FILE this slot is for */
    uint8_t  path[0x19];       /* +0x02  the chunk tags walked into, four
                                         characters each, NUL-terminated */
    struct chunk_bound bound[7]; /* +0x1b  where each of those chunks ends */
    int16_t  depth;            /* +0x37  how far in, in bytes: a multiple of 4,
                                         and the walk gives up at 0x18 */
    int16_t  word_39;          /* +0x39  how many matches to skip */
    uint16_t pos_lo;           /* +0x3b  the position, which restore_file_record
                                         seeks back to */
    uint16_t pos_hi;           /* +0x3d */
    uint16_t size_lo;          /* +0x3f  the current chunk's size; what
                                         file_record_size answers */
    uint16_t size_hi;          /* +0x41 */
} __attribute__((packed));

DG_ASSERT_AT(struct open_file, path,          0x02);
DG_ASSERT_AT(struct open_file, bound,         0x1b);
DG_ASSERT_AT(struct open_file, depth,         0x37);
DG_ASSERT_AT(struct open_file, word_39,       0x39);
DG_ASSERT_AT(struct open_file, pos_lo,        0x3b);
DG_ASSERT_AT(struct open_file, pos_hi,        0x3d);
DG_ASSERT_AT(struct open_file, size_lo,       0x3f);
DG_ASSERT_AT(struct open_file, size_hi,       0x41);
_Static_assert(sizeof(struct open_file) == 0x43,
               "an open file is what find_file_record strides by");

#define OPENFILE(p) (*(volatile struct open_file *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A bitmap set**: the list `load_bitmaps` answers, one near pointer per
 * bitmap in the file.
 *
 * `read_bmp_info` fills it and hands back a count beside it, so the length is
 * the file's and not a constant - hence a flexible array rather than named
 * fields. There is no field name to give an entry, either: three of these are
 * live at once and they hold different art.
 *
 *     DG52ED.panel_art_ptr   "cp.bmp"       the panel's own pieces
 *     DG4E67.bmp_4ecb_ptr    "gp_bord.bmp"  the play screen's border
 *     DG4E67.menu_bmp_ptr    "gp_menu.bmp"  the menu strip
 *
 * So entry 10 means whatever the file it came out of put there, and the type
 * is the whole of what is worth saying: **every word in this list is a near
 * pointer to a bitmap header**, which is why each is passed straight to
 * `draw_bitmap` and to nothing else.
 *
 * Entry `n` is at `+2n`, which is how a site here reads back against the
 * disassembly - `bmp[0x25]` is `[si+0x4a]`.
 * ---------------------------------------------------------------------------
 */
struct bmp_set {
    dg_off_t bmp[];
} __attribute__((packed));

#define BMPSET(p) (*(volatile struct bmp_set *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A belt**, the 0x2c-byte record a part hangs off `word_66` and `word_68`.
 *
 * The size is not a reading: `machine_draw.c` builds one with
 * `heap_calloc_far(1, 0x2c)` and writes the part straight into `+0x00`, which
 * is also what makes that field the owner rather than a list link.
 *
 * The three endpoint blocks are `link_end_distance`'s, which picks its base
 * from a generation argument - 0x14 for 3, 0x1c for 2, 0x24 for 1 - and then
 * indexes `base + 4 * end`. So each block is a pair of points, end A then end
 * B, and the record ends exactly where the third block does.
 *
 * Three more routines settle the shape rather than suggest it.
 * `reverse_link_ends` swaps end A with end B in all three blocks at once -
 * 0x14 with 0x18, 0x1c with 0x20, 0x24 with 0x28, and each y beside it - so
 * the blocks really are `[generation][end]` and not anything else that happens
 * to be six words apart. `shift_state_history` ages them, gen 2 into gen 1 and
 * gen 3 into gen 2, with one 32-bit move per point, and ages `+0x0e`,
 * `+0x10`, `+0x12` the same way, which is what makes those three a scalar's
 * history rather than three fields. And `read_record_fields`, reading a level
 * in, copies `+0x02` into `+0x06`, `+0x04` into `+0x08`, `+0x0a` into `+0x0c`
 * and `+0x0b` into `+0x0d`: the second of each pair is where the file's own
 * attachment is kept.
 *
 * `refresh_link_geometry` names the ends: it takes the part at `+0x02`, adds
 * that part's `+0x2a`/`+0x2c` position to the attachment offset it finds at
 * `+0x6a + 2 * slot_a`, and stores the result in `+0x14`/`+0x16`. `+0x04` and
 * `slot_b` do the same for end B into `+0x18`/`+0x1a`.
 *
 * **A part's `+0x54` is a different record and must not be moved onto this
 * one.** `shift_state_history` is where the two stand side by side: kind 8
 * takes `word_54` and ages four chains whose generations are 0x10 apart, kinds
 * 7 and 0xa take `word_66` and age this one's, whose generations are 8 apart.
 * `compute_link_endpoints` reads the `+0x54` record's parts at `+0x04` and
 * `+0x06` and writes coordinates over `+0x08` to `+0x16`, so its `+0x0a` is a
 * word where a belt has two bytes. It has not been read yet.
 *
 * Field names are ours, from what the routines above do with them; the
 * offsets and the size are the original's.
 * ---------------------------------------------------------------------------
 */
struct point16 {
    int16_t x;                 /* +0x00 */
    int16_t y;                 /* +0x02 */
} __attribute__((packed));

struct belt {
    dg_off_t  owner_ptr;       /* +0x00  the part this belt hangs off */
    dg_off_t  end_a_ptr;       /* +0x02  the part end A is attached to */
    dg_off_t  end_b_ptr;       /* +0x04  the part end B is attached to */
    dg_off_t  home_a_ptr;      /* +0x06  the attachment the level file gave */
    dg_off_t  home_b_ptr;      /* +0x08 */
    uint8_t   slot_a;          /* +0x0a  which of end A's four +0x5a directions */
    uint8_t   slot_b;          /* +0x0b  the same for end B */
    uint8_t   home_slot_a;     /* +0x0c  the slot the level file gave */
    uint8_t   home_slot_b;     /* +0x0d */
    int16_t   v[3];            /* +0x0e  a scalar with the same three
                                         generations as `pt`, newest first */
    struct point16 pt[3][2];  /* +0x14  three generations of both ends:
                                         gen 3 at +0x14, gen 2 at +0x1c,
                                         gen 1 at +0x24 */
} __attribute__((packed));

DG_ASSERT_AT(struct belt, end_a_ptr,       0x02);
DG_ASSERT_AT(struct belt, end_b_ptr,       0x04);
DG_ASSERT_AT(struct belt, home_a_ptr,      0x06);
DG_ASSERT_AT(struct belt, home_b_ptr,      0x08);
DG_ASSERT_AT(struct belt, slot_a,          0x0a);
DG_ASSERT_AT(struct belt, slot_b,          0x0b);
DG_ASSERT_AT(struct belt, home_slot_a,     0x0c);
DG_ASSERT_AT(struct belt, home_slot_b,     0x0d);
DG_ASSERT_AT(struct belt, v,               0x0e);
DG_ASSERT_AT(struct belt, pt,              0x14);
_Static_assert(sizeof(struct belt) == 0x2c,
               "a belt is what heap_calloc_far(1, 0x2c) makes");

#define BELT(p) (*(volatile struct belt *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A rope**, the 0x38-byte record a kind-8 part hangs off `word_54` - not a
 * belt, which is `struct belt` above and hangs off `word_66`.
 * `shift_state_history` is where the two stand side by side and is what tells
 * them apart: kind 8 ages four chains whose generations are 0x10 bytes apart,
 * kinds 7 and 0xa age a belt's two, whose generations are 8 apart.
 *
 * `clone_part` gives the size with `heap_calloc_far(1, 0x38)` and writes the
 * part straight into +0x02, which is what makes that field the owner.
 *
 * The geometry is four points and not two, and the arithmetic closes exactly:
 * three generations of four points at four bytes each is 0x30, and +0x08 plus
 * 0x30 is the record's end. `compute_link_endpoints` fills the first
 * generation - the two parts' own attachment positions into points 0 and 1,
 * and each offset by half the part's +0x58 into points 2 and 3, along whichever
 * axis the two ends are further apart on. So a rope is drawn as a quadrilateral
 * and a belt as a line, which is why one keeps four corners and the other two.
 *
 * Field names are ours; the offsets and the size are the original's.
 * ---------------------------------------------------------------------------
 */
struct rope {
    uint16_t  word_00;         /* +0x00 */
    dg_off_t  owner_ptr;       /* +0x02  the part this rope hangs off */
    dg_off_t  end_a_ptr;       /* +0x04  the part end A is attached to */
    dg_off_t  end_b_ptr;       /* +0x06  the part end B is attached to */
    struct point16 pt[3][4];   /* +0x08  three generations of four corners:
                                         gen 3 at +0x08, gen 2 at +0x18,
                                         gen 1 at +0x28 */
} __attribute__((packed));

DG_ASSERT_AT(struct rope, owner_ptr,       0x02);
DG_ASSERT_AT(struct rope, end_a_ptr,       0x04);
DG_ASSERT_AT(struct rope, end_b_ptr,       0x06);
DG_ASSERT_AT(struct rope, pt,              0x08);
_Static_assert(sizeof(struct rope) == 0x38,
               "a rope is what clone_part makes with heap_calloc_far(1, 0x38)");

#define ROPE(p) (*(volatile struct rope *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A part's connection points**, the array `part_init` makes with
 * `heap_calloc_far(point_count, 4)` and hangs off `points_ptr`. Four bytes
 * each, and the setups fill the first two a byte at a time out of a table of
 * pairs while `part_finish_angles` computes the third.
 *
 * That routine is what says the third is a word and not two bytes: it takes
 * point `n` and point `n + 1`, widens their two bytes each into a scratch
 * quad, and stores `0xc000 - atan2` of the difference back into `+0x02` with a
 * 16-bit move. So the record is a byte, a byte, and the angle from this point
 * to the next.
 *
 * The names are ours; the four-byte stride is `heap_calloc_far`'s.
 * ---------------------------------------------------------------------------
 */
struct part_point {
    uint8_t   x;               /* +0x00  an offset from the part's own position */
    uint8_t   y;               /* +0x01 */
    int16_t   angle;           /* +0x02  towards the next point */
} __attribute__((packed));

_Static_assert(sizeof(struct part_point) == 4,
               "part_init allocates four bytes a point");

#define POINTS(p) ((volatile struct part_point *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A part kind**, the 0x3a-byte record at DGROUP 0x0ea6 that every part of
 * that kind shares. `PART(x).kind` is the index: eighteen sites compute
 * `0x0ea6 + 0x3a * kind` and read a field out of the answer, and three of them
 * hold the address in a local first.
 *
 * The stride is what `free_part_bitmap` and `load_part_bitmaps` walk -
 * `0x0eba + 0x3a * n` for the bitmap list, which is this record's +0x14 - and
 * the fields below are the ones anything reads.
 *
 *   +0x02  `reset_machine` copies it into the part's own `weight`.
 *   +0x08  the gravity direction, which `apply_contact_friction` reads as the
 *          normal load - one field, two routines, and the note in
 *          `integrate_object` already says they are the same one.
 *   +0x14  a bitmap set, indexed by the part's form; +0x16 is a second one.
 *   +0x1e  the connection-point count `part_init` allocates from.
 *
 * Nothing reads +0x20 upward, so where the record's 0x3a bytes go after that
 * is not known. The names are ours.
 * ---------------------------------------------------------------------------
 */
struct part_kind {
    uint16_t  word_00;         /* +0x00 */
    int16_t   weight;          /* +0x02 */
    int16_t   word_04;         /* +0x04  bounce_pair reads it beside the weight */
    int16_t   word_06;         /* +0x06  apply_contact_friction reads it four times */
    int16_t   gravity;         /* +0x08  the normal load, same field */
    uint8_t   pad_0a[2];       /* +0x0a */
    /* the size limits the + and - keys stop at. `carried_part_grow` compares
       the part's +0x50 against the first and its +0x52 against the second,
       picking the axis the same way `carried_part_shrink` does against the
       other pair - which is why the four are a maximum and a minimum per axis
       and not four unrelated words */
    int16_t   max_w;           /* +0x0c */
    int16_t   max_h;           /* +0x0e */
    int16_t   min_w;           /* +0x10 */
    int16_t   min_h;           /* +0x12 */
    dg_off_t  bitmaps_ptr;     /* +0x14  a bmp_set, indexed by form */
    dg_off_t  bitmaps2_ptr;    /* +0x16  a second one */
    uint16_t  word_18;         /* +0x18 */
    uint16_t  word_1a;         /* +0x1a */
    uint8_t   pad_1c[2];       /* +0x1c */
    uint16_t  point_count;     /* +0x1e */
    uint8_t   pad_20[10];      /* +0x20 */
    /* three of the kind's hooks, each a far pointer the game calls through:
       `call_part_setup`, `call_part_flip` and `call_part_hook(.., "settle")` */
    dg_off_t  setup_off;       /* +0x2a */
    dg_seg_t  setup_seg;       /* +0x2c */
    dg_off_t  flip_off;        /* +0x2e */
    dg_seg_t  flip_seg;        /* +0x30 */
    dg_off_t  settle_off;      /* +0x32 */
    dg_seg_t  settle_seg;      /* +0x34 */
    uint8_t   pad_36[4];       /* +0x36  nothing reads these */
} __attribute__((packed));

DG_ASSERT_AT(struct part_kind, weight,        0x02);
DG_ASSERT_AT(struct part_kind, word_04,       0x04);
DG_ASSERT_AT(struct part_kind, word_06,       0x06);
DG_ASSERT_AT(struct part_kind, gravity,       0x08);
DG_ASSERT_AT(struct part_kind, max_w,         0x0c);
DG_ASSERT_AT(struct part_kind, max_h,         0x0e);
DG_ASSERT_AT(struct part_kind, min_w,         0x10);
DG_ASSERT_AT(struct part_kind, min_h,         0x12);
DG_ASSERT_AT(struct part_kind, bitmaps_ptr,   0x14);
DG_ASSERT_AT(struct part_kind, bitmaps2_ptr,  0x16);
DG_ASSERT_AT(struct part_kind, word_18,       0x18);
DG_ASSERT_AT(struct part_kind, word_1a,       0x1a);
DG_ASSERT_AT(struct part_kind, point_count,   0x1e);
DG_ASSERT_AT(struct part_kind, setup_off,   0x2a);
DG_ASSERT_AT(struct part_kind, setup_seg,   0x2c);
DG_ASSERT_AT(struct part_kind, flip_off,   0x2e);
DG_ASSERT_AT(struct part_kind, flip_seg,   0x30);
DG_ASSERT_AT(struct part_kind, settle_off,   0x32);
DG_ASSERT_AT(struct part_kind, settle_seg,   0x34);
_Static_assert(sizeof(struct part_kind) == 0x3a,
               "a part kind is what free_part_bitmap strides by");

/* the record for a kind, and the record at an address a routine was handed */
#define PARTKIND_AT(p) (*(volatile struct part_kind *)(dgroup + (uint16_t)(p)))
#define PARTKIND(k)    PARTKIND_AT(0x0ea6 + 0x3a * (uint16_t)(k))

/*
 * ---------------------------------------------------------------------------
 * **A part template**, sixteen bytes per kind at DGROUP 0x2966. `make_part`
 * indexes it with the kind shifted left four and copies six of its eight words
 * straight into the new part, then calls the far pointer in the last two.
 *
 * Every field is named for where it is copied to, because that is the only
 * thing the table's own routine does with it. The stride is `n << 4` and only
 * `make_part` reads the table at all.
 * ---------------------------------------------------------------------------
 */
struct part_template {
    uint16_t  flags_06;        /* +0x00  goes to the part's +0x06 */
    uint16_t  flags_0a;        /* +0x02  ... +0x0a */
    uint16_t  word_50;         /* +0x04  ... +0x50 */
    uint16_t  word_52;         /* +0x06  ... +0x52 */
    int16_t   width;           /* +0x08  ... +0x44 */
    int16_t   height;          /* +0x0a  ... +0x46 */
    dg_off_t  init_off;        /* +0x0c  the kind's init routine, called far */
    dg_seg_t  init_seg;        /* +0x0e */
} __attribute__((packed));

DG_ASSERT_AT(struct part_template, flags_0a,  0x02);
DG_ASSERT_AT(struct part_template, word_50,   0x04);
DG_ASSERT_AT(struct part_template, word_52,   0x06);
DG_ASSERT_AT(struct part_template, width,     0x08);
DG_ASSERT_AT(struct part_template, height,    0x0a);
DG_ASSERT_AT(struct part_template, init_off,  0x0c);
DG_ASSERT_AT(struct part_template, init_seg,  0x0e);
_Static_assert(sizeof(struct part_template) == 0x10,
               "a part template is what make_part strides by");

#define PARTTMPL(n) (*(volatile struct part_template *) \
                     (dgroup + 0x2966 + 0x10 * (uint16_t)(n)))

/*
 * ---------------------------------------------------------------------------
 * **A resource stream**, the 0x21-byte record `open_resource_slot` makes and
 * files in the table at DGROUP 0x57c0. `DG5888.record_ptr` points at whichever
 * one is selected, and sixteen routines in engine.c read it through that.
 *
 * The size is `heap_calloc_far(1, 0x21)` and the last field is the byte at
 * +0x20, so nothing here is a guess about where the record ends.
 *
 * **Four 32-bit quantities, each kept as two words with the carry written
 * out.** `next_input_byte` steps +0x0a and adds one to +0x0c when it wraps;
 * `read_input_block` subtracts +0x0a from +0x0e with an explicit borrow;
 * `resource_seek` takes +0x16/+0x18 for `whence == 1` and +0x12/+0x14 for
 * `whence == 2`. So they are pairs of words and not `uint32_t`: the code does
 * the arithmetic a word at a time, and saying `uint32_t` would describe a
 * routine nobody wrote.
 *
 * +0x06 and +0x08 are a reading and are named as one. `open_resource` writes a
 * file handle into +0x06 alone, and `restart_resource_stream` hands the pair
 * to `huge_add(..., 5)` as if it were a far pointer - which is consistent,
 * because `DG5888.flags & 0x20` is the bit that chooses between reading the
 * stream from a file and reading it out of memory, and these two routines are
 * on opposite sides of it.
 *
 * Field names below the offsets are ours; the offsets and the size are the
 * original's.
 * ---------------------------------------------------------------------------
 */
struct resource {
    dg_off_t  work_ptr;        /* +0x00  the near buffer prepare_resource_slot makes */
    dg_off_t  scratch_off;     /* +0x02  the far scratch block, which lzss_reset caches */
    dg_seg_t  scratch_seg;     /* +0x04 */
    uint16_t  word_06;         /* +0x06  a file handle, or the low half of a far pointer */
    uint16_t  word_08;         /* +0x08 */
    uint16_t  in_lo;           /* +0x0a  how far into the compressed input the reader is */
    uint16_t  in_hi;           /* +0x0c */
    uint16_t  end_lo;          /* +0x0e  where the compressed input ends */
    uint16_t  end_hi;          /* +0x10 */
    uint16_t  size_lo;         /* +0x12  the size resource_seek measures from for SEEK_END */
    uint16_t  size_hi;         /* +0x14 */
    uint16_t  pos_lo;          /* +0x16  the position it measures from for SEEK_CUR */
    uint16_t  pos_hi;          /* +0x18 */
    union {
        /* the run counter. Mostly a byte, but one site increments it 16 bits
           wide, so the carry into +0x1b is the original's and is kept */
        uint16_t word_1a;      /* +0x1a */
        struct {
            uint8_t byte_1a;   /* +0x1a */
            uint8_t byte_1b;   /* +0x1b */
        };
    };
    uint16_t  start_lo;        /* +0x1c  where in the file the resource begins,
                                         from game_ftell at open */
    uint16_t  start_hi;        /* +0x1e */
    uint8_t   kind;            /* +0x20  the type prepare_resource_slot was given */
} __attribute__((packed));

DG_ASSERT_AT(struct resource, scratch_off,   0x02);
DG_ASSERT_AT(struct resource, scratch_seg,   0x04);
DG_ASSERT_AT(struct resource, word_06,       0x06);
DG_ASSERT_AT(struct resource, word_08,       0x08);
DG_ASSERT_AT(struct resource, in_lo,         0x0a);
DG_ASSERT_AT(struct resource, in_hi,         0x0c);
DG_ASSERT_AT(struct resource, end_lo,        0x0e);
DG_ASSERT_AT(struct resource, end_hi,        0x10);
DG_ASSERT_AT(struct resource, size_lo,       0x12);
DG_ASSERT_AT(struct resource, size_hi,       0x14);
DG_ASSERT_AT(struct resource, pos_lo,        0x16);
DG_ASSERT_AT(struct resource, pos_hi,        0x18);
DG_ASSERT_AT(struct resource, word_1a,       0x1a);
DG_ASSERT_AT(struct resource, byte_1b,       0x1b);
DG_ASSERT_AT(struct resource, start_lo,      0x1c);
DG_ASSERT_AT(struct resource, start_hi,      0x1e);
DG_ASSERT_AT(struct resource, kind,          0x20);
_Static_assert(sizeof(struct resource) == 0x21,
               "a resource is what heap_calloc_far(1, 0x21) makes");

#define RESOURCE(p) (*(volatile struct resource *)(dgroup + (uint16_t)(p)))

DG_ASSERT_AT(struct file_rec, left,     0x00);
DG_ASSERT_AT(struct file_rec, flags,    0x02);
DG_ASSERT_AT(struct file_rec, handle,   0x04);
DG_ASSERT_AT(struct file_rec, buf_size, 0x06);
DG_ASSERT_AT(struct file_rec, read_ptr, 0x0a);

#endif /* DGROUP_H */
