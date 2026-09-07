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
#define frame_flag        DG16(0x5754)

/* Read by the frame-presentation routine at 0x081cc to choose between three
 * paths. Names are guesses from that use. */
#define present_hook_a    DG16(0x52fa)
#define present_hook_b    DG16(0x52f2)

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
    uint8_t   unknown_84[0x11a];            /* +0x84 */
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
};

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

/* A byte array indexed by the routine at 0x2147d, which returns its bit 0. */
#define byte_array_468c(i) DG8(0x468c + (i))

/*
 * A near pointer at DGROUP 0x5400 to a structure, and three words beside it,
 * all used by the routine at 0x002be. What the structure is has not been
 * established; only the offsets it touches are known.
 */
#define ptr_5400          DGU16(0x5400)
#define word_5402         DG16(0x5402)
#define word_5414         DG16(0x5414)
#define word_541c         DG16(0x541c)
#define word_5420         DG16(0x5420)

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
};

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
#define ASB_SEG     DGU16(0x4a9a)
#define ASB_OFF     DGU16(0x4a98)
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

#endif /* DGROUP_H */
