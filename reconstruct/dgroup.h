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

#include <stddef.h>
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

/* Declared in io.h, which this header deliberately does not include: `dg_off`
   below refuses a pointer that is not the guest's, and the refusal has to be
   loud. */
void port_abort(const char *msg);
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
#define DG8(off)    (*(volatile uint8_t *)(dgroup + (off)))
#define DG16(off)   (*(volatile int16_t *)(dgroup + (off)))
#define DG32(off)   (*(volatile int32_t *)(dgroup + (off)))
#define DGU16(off)  (*(volatile uint16_t *)(dgroup + (off)))

/*
 * **There is no `DGS8`, and a signed byte is read `(int8_t)DG8(off)`.**
 *
 * There was one from 030c859, the commit that first modelled DGROUP as
 * memory, and nothing ever called it in the two weeks since: every site that
 * wants a signed byte - eleven of them, in `machine.c`, `machine_draw.c` and
 * `engine.c` - had written the cast out by hand instead. That is the better
 * spelling anyway, because the sign extension is the *original's* and belongs
 * where the original does it: a byte read and a `cbw` are two steps, and
 * `(int16_t)(int8_t)DG8(hot)` in `place_object_for_draw` - shifting a box by
 * the signed offset pair the part-kind table holds at `hot` and `hot + 1` -
 * says both. `DGS8(hot)` would hide the widening inside the read.
 *
 * The asymmetry with `DG16`/`DGU16` is real and is not an oversight. A word
 * is read signed about as often as unsigned, so both spellings earn a name; a
 * byte is read unsigned almost always, so the exception is worth writing out.
 */

/*
 * A far pointer: segment and offset, as the hardware forms an address.
 *
 * **Borland's own name**, because the game was compiled against Borland and
 * this is that compiler's macro. It was `FAR_PTR` here for a long time, which
 * was ours and said the same thing in a spelling nobody who reads the original
 * would recognise.
 */
#define MK_FP(seg, off) \
    (guest_mem + (((uint32_t)(uint16_t)(seg)) << 4) + (uint16_t)(off))

/*
 * **And Borland's spelling for taking one apart.** `FP_SEG` and `FP_OFF`
 * answer the halves of what `MK_FP` built.
 *
 * `FP_SEG`/`FP_OFF` answer the **normalised** pair, the linear address split
 * at the paragraph, because that is the only pair a host pointer can be asked
 * for on its own - it does not remember which of the many `seg:off` pairs
 * addressing it the guest was holding, which is the caveat in tim.h.
 *
 * A routine that holds a fixed segment and steps only the offset - the usual
 * shape of a copy loop - wants the offset *within that segment*, which is not
 * one of these three and is not a Borland macro at all: in the original it is
 * simply the register. `out - MK_FP(seg, 0)` says it where it is needed, and
 * reads the same as the `back - scratch` beside it.
 */
#define FP_LIN(p)         ((uint32_t)((const volatile uint8_t *)(p) - guest_mem))
#define FP_SEG(p)         ((uint16_t)(FP_LIN(p) >> 4))
#define FP_OFF(p)         ((uint16_t)(FP_LIN(p) & 0xf))
#define FAR8(seg, off)    (*(volatile uint8_t *)MK_FP(seg, off))
#define FAR16(seg, off)   (*(volatile int16_t *)MK_FP(seg, off))
#define FARU16(seg, off)  (*(volatile uint16_t *)MK_FP(seg, off))

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

/* **A list's head cell**: the first part and the last, as the two near
   pointers a part itself begins with. The game hands a cell to `insert_sorted`
   and `read_list` as if it were a part - the record before the first, whose
   `next_ptr` is the list - so a part's `next_ptr` and `prev_ptr` sit where
   these do, and dgroup.h asserts it beside `struct part`. Three of them:
   `DG50D3.parts_bin`, `DG5179.moving_parts` and `DG521B.placed_parts`; the
   first part of a list is `PART_PTR(DG521B.placed_parts.next_ptr)`. */
struct list_node {
    dg_off_t next_ptr;
    dg_off_t prev_ptr;
} __attribute__((packed));

/* **A near pointer to a bitmap header**, which is what the game stores
   wherever it keeps one: two bytes, an offset into DGROUP, exactly
   `dg_off_t` and named for what it points at.

   It is not `struct bitmap *`. A host pointer is eight bytes and these live
   in guest memory two bytes apart, so a `struct bitmap **` over one of these
   arrays would stride four times too far from the second entry on - which is
   the whole reason `dg_off_t` exists. The typedef buys the name without
   touching the width. */
typedef dg_off_t bmp_ptr_t;
typedef uint16_t dg_seg_t;      /* a real-mode segment */

/*
 * **A far pointer as the guest stores one**: the offset first and the segment
 * second, which is the order every `seg:off` pair in DGROUP is written in and
 * the order `dos_alloc_bytes` and `huge_add_to` hand them about in.
 *
 * This is for a pair that *lives in guest memory*. A pair held in a routine's
 * own locals and stepped is a pointer and should be one - see the note in
 * CLAUDE.md about folding `_seg`/`_off` into `MK_FP` - so this type is for
 * the storing, not the walking.
 *
 * The `bitmap` header is the one record that stores the two the other way
 * round, and says so where it is declared.
 *
 * Three unions in this file spell the same two fields tag-less rather than
 * using this type, because they overlay an `int32_t` on the pair and an
 * anonymous member has to be a tag-less struct for `.off` to reach through.
 */
struct far_ptr {
    dg_off_t  off;              /* +0x00 */
    dg_seg_t  seg;              /* +0x02 */
} __attribute__((packed));

/*
 * **The other order.** One record in this program stores the two words the
 * opposite way round - the bitmap header - and the original settles it at
 * 0x2530b, which reads `[si+2]`, shifts it right four, adds it to `[si]` and
 * masks `[si+2]`: the segment is at +0 and the offset at +2. So it cannot be
 * a `far_ptr`, and saying it is would swap the two words silently.
 */
struct far_ptr_rev {
    dg_seg_t  seg;              /* +0x00 */
    dg_off_t  off;              /* +0x02 */
} __attribute__((packed));

/*
 * **Normalise a far pointer**: carry the paragraphs out of the offset into the
 * segment and keep only the remainder, which is what `draw_bitmap` does to
 * every header before it draws and what `decode_vqt_list` does to reach the
 * first plane. Ours as a routine; the two lines are the original's.
 */
static inline struct far_ptr far_normalise(struct far_ptr p)
{
    p.seg = (dg_seg_t)(p.seg + (p.off >> 4));
    p.off = (dg_off_t)(p.off & 0x0f);
    return p;
}

/* The same, for the one record that stores the pair segment-first. */
static inline struct far_ptr_rev far_normalise_rev(struct far_ptr_rev p)
{
    p.seg = (dg_seg_t)(p.seg + (p.off >> 4));
    p.off = (dg_off_t)(p.off & 0x0f);
    return p;
}

/*
 * **Are these the same far pointer?** The two *words*, which is what the
 * original compares - not the linear address, since many `seg:off` pairs
 * reach the same byte and the game never normalises before testing.
 */
static inline int far_eq(struct far_ptr a, struct far_ptr b)
{
    return a.off == b.off && a.seg == b.seg;
}

/*
 * **An address or a size, and only the caller knows which.** `dos_alloc_bytes`
 * answers a far pointer when it allocates and a *byte count* when it is asked
 * `(0xffff, 0xffff)`, which is how the game finds out how much memory is free -
 * the original's own `if` at the top of the routine is the fork. Neither type
 * is right for both, so the caller picks the member and the choice is written
 * at the site rather than guessed at by the signature.
 *
 * The two overlay exactly, and that is why `far_ptr` is `{off, seg}` and not
 * the other way round: the guest answers in DX:AX, so on a little-endian host
 * the low half of the 32-bit value sits where `off` is and the high half where
 * `seg` is. `.bytes` and `.ptr` are the same four bytes read two ways, which
 * is what the original does with DX:AX.
 */
union far_or_size {
    struct far_ptr ptr;         /* when it allocated */
    uint32_t       bytes;       /* when it was asked how much is free */
};

/*
 * **The null far pointer**, 0000:0000. The game tests for it as
 * `(off | seg) == 0` - one `or` and a branch, which is the same question as
 * both halves being zero and is what `far_eq(p, FAR_NULL)` asks.
 *
 * Note that this is *not* a C null pointer: 0000:0000 is a real address in the
 * guest, the first byte of `guest_mem`, which is why `draw_string_body`'s
 * guard is `(str | seg) == 0` and not `str == NULL`.
 */
static const struct far_ptr FAR_NULL = { 0, 0 };

/* And the conversion between the two orders, for a caller that wants the
   common one out of a bitmap header. */
static inline struct far_ptr far_of_rev(struct far_ptr_rev r)
{
    struct far_ptr p = { r.off, r.seg };

    return p;
}

static inline struct far_ptr_rev far_to_rev(struct far_ptr p)
{
    struct far_ptr_rev r = { p.seg, p.off };

    return r;
}

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

/*
 * The same for a 32-bit value. A routine's frame is `_Alignas(2)`, because
 * that is all a `[bp-N]` layout guarantees, so a long in it is two-aligned and
 * an `int32_t *` into it would be a stricter claim than the bytes support.
 */
static inline int32_t dg_rd32(const volatile void *p)
{
    const volatile uint8_t *b = (const volatile uint8_t *)p;

    return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8)
                     | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

static inline void dg_wr32(volatile void *p, int32_t v)
{
    volatile uint8_t *b = (volatile uint8_t *)p;
    uint32_t u = (uint32_t)v;

    b[0] = (uint8_t)u;        b[1] = (uint8_t)(u >> 8);
    b[2] = (uint8_t)(u >> 16); b[3] = (uint8_t)(u >> 24);
}

/*
 * **A null pointer is the offset zero**, and not the distance from the base to
 * address nothing. `string_chr` answers NULL where the original answers 0, and
 * every caller of it tests the answer against zero; taking the difference for
 * a null would hand back `-base` truncated, which is a large offset into
 * DGROUP and tests as *found*. The one number the original never uses as an
 * address is 0, which is why it can mean "no".
 */
/*
 * **Is this pointer the guest's memory at all?** Ours, and it answers a
 * question only the port can have: the original's addresses are all inside
 * its megabyte, and the port's are not - a frame that became a C array lives
 * on the host stack.
 *
 * It exists for the one shape where the guest tells a *handle* from an
 * *address* by comparing numbers. `load_bitmaps` asks whether its argument
 * matches an open file record's `file_ptr`; for a pointer outside guest
 * memory the answer is certainly no, and saying so is the original's own test
 * answered exactly rather than by a `dg_off` that could collide.
 */
static inline int dg_is_guest(const volatile void *p)
{
    const volatile uint8_t *b = (const volatile uint8_t *)p;

    return b >= guest_mem && b < guest_mem + GUEST_MEM_BYTES;
}

/*
 * **The screen the driver reported**, at DGROUP 0x3f78 - which is the driver
 * block's own **+0x6e8**, so this struct and `struct dg_3890` describe the
 * same six bytes. They were written twice under two names: `game.c` set
 * `DG3F78.screen_height = 0x16f` in one routine and
 * `DG3890.screen_height = 0x18f` in another, and `set_full_clip` read
 * `DG3890.clip_bottom = DG3F78.screen_height - 1` with both names on one
 * line. `dg_3890` carries this as a field now, so there is one name.
 */
struct dg_3f78 {
    uint8_t   mode_kind;          /* +0x00  a byte saying which */
    uint8_t   pad_3f79[1];
    int16_t   screen_width;       /* +0x02  an extent past these is cut back to the edge */
    int16_t   screen_height;      /* +0x04  the copy-protection screen sets it to 0x18f first */
} __attribute__((packed));

#define DG3F78 (*(volatile struct dg_3f78 *)(dgroup + 0x3f78))

/*
 * **An address back into the offset the guest holds it as.** The inverse of
 * `dg_ptr`, and a null pointer stays 0 because the guest's null is offset 0.
 *
 * **It refuses a pointer that is not the guest's.** `dg_off` takes a `void *`,
 * so the compiler will hand it anything - and a frame that became a C local
 * lives on the *host* stack, where the subtraction below is the distance
 * between two unrelated objects. Forty-one call sites were once "fixed" that
 * way in one sitting, every one of them wrong, with a clean build at the end
 * of it: `copy_file_record`, `read_bmp_info`, `far_memcpy` and the rest each
 * write through the address they are given, and would have written into
 * whatever that arithmetic pointed at.
 *
 * The compiler cannot see it and no comparison can either - a wild offset
 * corrupts guest memory somewhere else and the failure surfaces anywhere but
 * here. So the check is at the conversion, where the answer is still known to
 * be wrong. Ours; the original has no such routine, because every address it
 * has is inside its own megabyte.
 */
static inline uint16_t dg_off(const volatile void *base, const volatile void *p)
{
    if (p == 0)
        return 0;

    if (!dg_is_guest(p))
        port_abort("dg_off on a pointer outside guest memory - a C local has "
                   "no offset the guest can hold");

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
    struct far_ptr pal_copy_ptr;            /* +0x19e  VGA:0x0f15's palette */
    uint8_t   unknown_1a2[0x51a];           /* +0x1a2 */
    uint16_t  dda_whole;                    /* +0x6bc */
    uint16_t  dda_frac;                     /* +0x6be */
    int16_t   dda_saved;                    /* +0x6c0 */
    uint16_t  dda_acc;                      /* +0x6c2 */
    uint8_t   line_mask;                    /* +0x6c4 */
    uint8_t   unknown_6c5[0x23];            /* +0x6c5 */
    /*
     * +0x6e8  **DGROUP 0x3f78**, and the same six bytes `DG3F78` names. The
     * driver fills them in `vm_driver_init` and the game reads them all over;
     * before this they were two structs over one record, and the mode's
     * height had a name in each.
     */
    struct dg_3f78 screen;                  /* +0x6e8  DGROUP 0x3f78 */
    uint8_t   unknown_6ee[4];               /* +0x6ee */
    uint16_t  row_offset[480];              /* +0x6f2  measured: [y] == y * 80 */
} __attribute__((packed));

#define DG3890 (*(volatile struct dg_3890 *)(dgroup + VMDS))

#define DG_ASSERT_AT(type, field, off) \
    _Static_assert(__builtin_offsetof(type, field) == (off), \
                   #type "." #field " must sit at " #off)

DG_ASSERT_AT(struct dg_3f78, mode_kind,         0x00);
DG_ASSERT_AT(struct dg_3f78, screen_width,      0x02);
DG_ASSERT_AT(struct dg_3f78, screen_height,     0x04);

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
DG_ASSERT_AT(struct dg_3890, screen,         0x6e8);
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

/*
 * **The six drawing layers**, at DGROUP 0x50bf: a list head apiece, each a
 * chain of parts. `link_record_into_buckets` files a part on the layer, or
 * two, that its kind's `refile_level` names, threading it through the part's
 * `layer_next[0]` and `[1]`; `draw_machine` draws layer 5 down to layer 0
 * and `refile_overlapping_parts` walks the same chains; `clear_layer_heads`
 * (0x166d6) empties all six. Six is the extent every walker uses, and the
 * four words between here and 0x50d3 are not read as part of it.
 */
struct dg_50bf {
    dg_off_t  layer_head[6];      /* +0x00 */
} __attribute__((packed));

#define DG50BF (*(volatile struct dg_50bf *)(dgroup + 0x50bf))
_Static_assert(sizeof(struct dg_50bf) == 12, "six layer heads");

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
/* 0x5e56: one word per output row of a scaled blit - the source row's offset
   into its plane, as `blit_scaled_a` and `blit_scaled_b` work it out from the
   scaling table. The original also reaches it as `[bx+0x5e54]` with `bx` one
   entry higher, which is the same table one word lower: `ROW_OFFSETS[n - 1]`. */
#define ROW_OFFSETS    ((volatile uint16_t *)(dgroup + 0x5e56))
/* 0x5754: a far pointer per saved rectangle, indexed from ONE - slots 1 to 4 are
   the buffers `claim_buffer_slot` hands out, and slot 0 is never handed out */
#define RECT_BUFFER    ((volatile struct far_ptr *)(dgroup + 0x5754))
/* 0x6414: the sequencer's seven voices, a far pointer each */
#define VOICES         ((volatile struct far_ptr *)(dgroup + 0x6414))

/* A byte array indexed by the routine at 0x2147d, which returns its bit 0. */

/*
 * A near pointer at DGROUP 0x5400 to a structure, and three words beside it,
 * all used by the routine at 0x002be. What the structure is has not been
 * established; only the offsets it touches are known.
 */
/* These are `DG53FC.list_ptr` and its neighbours now; see the struct. */

/*
 * DGROUP 0x4342 holds the *segment* of the block the game builds span lists in
 * - a separate allocation, not part of DGROUP. It is reached through
 * `MK_FP(span_buffer_seg, 0)`, in the guest's address space, exactly where
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
    uint16_t  freeform;            /* +0x00  1 in freeform mode - the bin is unlimited and nothing is scored - 0 on a loaded level */
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
    /* **Two 32-bit scores.** `finish_level` copies `counter` into `score`
        a word at a time - `score_b = counter_hi; score_a = counter_lo` - and
        both are joined into an `int32_t` wherever they are read, by
        `score_to_code` and by the odometer. The `_a`/`_b` naming was the
        only thing that kept this pair from looking like the others. */
    int32_t   score;               /* +0x42  what finish_level banks for the
                                             password */
    int32_t   counter;             /* +0x46  the odometer's running total */
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
    dg_off_t  icons_bmp_ptr;      /* +0x60  icons.bmp's list */
    dg_off_t  menu_bmp_ptr;       /* +0x62  gp_menu.bmp's */
    dg_off_t  bmp_4ecb_ptr;       /* +0x64  gp_bord.bmp's */
    dg_off_t  score2_bmp_ptr;     /* +0x66  score2.bmp's - draw_odometer_digit's strips */
    /* **The level's title and hint**, read from the level file by
       `load_level` when it is a level and written back by `write_level`;
       the briefing draws the title over the panel and wraps the hint into
       the box. Eighty bytes for the title is the distance to the hint; the
       hint's extent is the gap to the next record at 0x50af, and the reader
       (`game_fread_string`, a length byte then the bytes) can put at most
       255 in it. */
    char      title[0x50];        /* +0x68  0x4ecf */
    char      hint[0x190];        /* +0xb8  0x4f1f, up to DG50AF */
} __attribute__((packed));

#define DG4E67 (*(volatile struct dg_4e67 *)(dgroup + 0x4e67))

DG_ASSERT_AT(struct dg_4e67, freeform,         0x00);
DG_ASSERT_AT(struct dg_4e67, title,            0x68);
DG_ASSERT_AT(struct dg_4e67, hint,             0xb8);
_Static_assert(sizeof(struct dg_4e67) == 0x248, "the hint runs up to DG50AF");
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
DG_ASSERT_AT(struct dg_4e67, score,              0x42);
DG_ASSERT_AT(struct dg_4e67, counter,            0x46);
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
    dg_off_t  bin_list_ptr;       /* +0x00  the list draw_bin walks; defaults to &parts_bin */
    dg_off_t  dragged_part_ptr;   /* +0x02  the part being dragged - drawn last, and not counted */
    /* **The parts bin: a doubly linked list's head and tail.** The level
       file fills it last, with `n_given` - the tools handed to the player,
       belts, ropes, pulleys, bellows, measured with TIM_LEVELSCAN over
       twenty levels - and `build_part_list` fills it with one of every
       kind for freeform. The game treats the head pair as a part whose
       `next_ptr` is the head: `insert_sorted` files its address into the
       first part's `prev_ptr`, and `unlink_part` writes back through that.
       `build_part_list` (0x1405b) and `read_list` clear both words; the
       tail is not otherwise written anywhere in the image. */
    struct list_node parts_bin;  /* +0x04  the head cell: next_ptr the first part, prev_ptr the last - cleared with it, never otherwise written */
} __attribute__((packed));

#define DG50D3 (*(volatile struct dg_50d3 *)(dgroup + 0x50d3))

DG_ASSERT_AT(struct dg_50d3, bin_list_ptr,      0x00);
DG_ASSERT_AT(struct dg_50d3, dragged_part_ptr,  0x02);
DG_ASSERT_AT(struct dg_50d3, parts_bin,       0x04);

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
    struct far_ptr scratch;       /* +0x04  the decompressor's block; every
                                     use is a `huge_add` from its base */
    uint16_t  word_5890;          /* +0x08 */
    uint16_t  word_5892;          /* +0x0a */
    struct far_ptr out;           /* +0x0c  the decompression output cursor:
                                     `read_resource` normalises the caller's
                                     destination into it and three
                                     decompressors walk it */
    struct far_ptr in;            /* +0x10  and where they are reading from */
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
DG_ASSERT_AT(struct dg_5888, scratch,           0x04);
DG_ASSERT_AT(struct dg_5888, word_5890,         0x08);
DG_ASSERT_AT(struct dg_5888, word_5892,         0x0a);
DG_ASSERT_AT(struct dg_5888, out,               0x0c);
DG_ASSERT_AT(struct dg_5888, in,                0x10);
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
    /* A far pointer: allocated and freed as one block, tested
       `(off != 0 || seg != 0)`, and handed to `configure_driver_far`. */
    struct far_ptr config;        /* +0x02  handed to configure_driver_far */
    /* **One far pointer, and "tail" was a misreading.** +0x06 is the offset
       and +0x08 the segment: every use pairs them - as `MK_FP(+8, +6)` to
       reach the head, and the node's own first four bytes are written back
       over both when one is unlinked. */
    struct far_ptr records;       /* +0x06  the record list start_sound
                                            walks by hand */
    uint16_t  timer_taken;        /* +0x0a  whether the timer was taken - 0x44ee says who has it */
    struct far_ptr tick_cb;       /* +0x0c  the timer callback; its segment
                                            is a relocation */
    dg_off_t  bank_ptr;           /* +0x10  the record +0x15c and +0x15d come out of */
    struct far_ptr driver;        /* +0x12  the loaded driver, installed by
                                            install_driver_far */
    struct far_ptr module;        /* +0x16  offset first, segment second,
                                            which is what the `lcall [0x4a98]`
                                            at 0x0bbde reads */
    uint16_t  load_error;         /* +0x1a  2 on the two failures that mean the resource was missing */
    uint16_t  identifier;         /* +0x1c  the identifier 0x7e takes instead of a constant */
    uint16_t  voice_word;         /* +0x1e  0 or -1 stops the walk; 0 or -2 means already on a voice */
    /* **One far pointer.** +0x20 is the offset and +0x22 the segment: the
       two were tested against zero together at four sites, built into a
       `struct far_ptr` by hand at three more, and assigned from one
       allocation's `.off` and `.seg`. The payload headers are reached as
       `directory.off + 4` and `+ 8`, which is a read at an offset and not
       the pointer being stepped. */
    struct far_ptr directory;     /* +0x20  the payload directory */
    uint16_t  file;               /* +0x24  the file this module opened, if it did */
    uint16_t  file_kind;          /* +0x26  recorded beside the handle */
    uint16_t  module_live;        /* +0x28  the module is loaded and a callback exists */
    uint16_t  bank_choice;        /* +0x2a  chooses between load_sound_bank and its sibling */
    uint16_t  device;             /* +0x2c  the device number; 8 is recorded as 3 */
} __attribute__((packed));

#define DG4A82 (*(volatile struct dg_4a82 *)(dgroup + 0x4a82))

DG_ASSERT_AT(struct dg_4a82, driver_number,     0x00);
DG_ASSERT_AT(struct dg_4a82, config,            0x02);
DG_ASSERT_AT(struct dg_4a82, records,           0x06);
DG_ASSERT_AT(struct dg_4a82, timer_taken,       0x0a);
DG_ASSERT_AT(struct dg_4a82, tick_cb,       0x0c);
DG_ASSERT_AT(struct dg_4a82, bank_ptr,          0x10);
DG_ASSERT_AT(struct dg_4a82, driver,            0x12);
DG_ASSERT_AT(struct dg_4a82, module,        0x16);
DG_ASSERT_AT(struct dg_4a82, load_error,        0x1a);
DG_ASSERT_AT(struct dg_4a82, identifier,        0x1c);
DG_ASSERT_AT(struct dg_4a82, voice_word,        0x1e);
DG_ASSERT_AT(struct dg_4a82, directory,         0x20);
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
        struct far_ptr ptr;
        struct { dg_off_t off; dg_seg_t seg; };
    } pal_black_ptr;
    union {                       /* +0x28  sierra.pal */
        int32_t  dword;
        struct far_ptr ptr;
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
        struct far_ptr ptr;
        struct { dg_off_t off; dg_seg_t seg; };
    } pal_tim_ptr;
    uint8_t   last_key;           /* +0x04  the last key the screen loops took - a **byte**, which
                                   * the assert caught: 0x52f2 follows it at +0x05 */
    uint16_t  cursor_follows;     /* +0x05  restore_cursor_following is guarded by this */
    dg_off_t  panel_art_ptr;     /* +0x07  the art set the panel's pieces come out of */
    dg_off_t  cursor_art_ptr;    /* +0x09  mouse.bmp's list */
    uint16_t  word_52f8;          /* +0x0b */
    uint16_t  stop_requested;     /* +0x0d  game_teardown(0) raises it; the loops above read it */
    uint16_t  stack_floor;        /* +0x0f  what the stack is reserved below */
} __attribute__((packed));

#define DG52ED (*(volatile struct dg_52ed *)(dgroup + 0x52ed))

/*
 * **The machine file the picker chose**, at DGROUP 0x52fe. `pick_file` copies
 * its answer here and `load_animation` and `save_machine` read it back.
 *
 * Thirteen bytes: `dg_52ed` ends at 0x52fe and `dg_530b` begins thirteen on,
 * and the picker's own buffer is capped at thirteen by the `0x0d` it hands
 * `picker_type`.
 */
struct dg_52fe {
    char      name[0xd];          /* +0x00 */
} __attribute__((packed));

#define DG52FE (*(volatile struct dg_52fe *)(dgroup + 0x52fe))

DG_ASSERT_AT(struct dg_52fe, name,              0x00);

/*
 * **The intro's credit roll**, at DGROUP 0x2370 - where each of the animated
 * pieces is put and which bitmap it is. `game_intro` walks it two entries at a
 * time, drawing a pair on each frame it is given, and stops on an entry whose
 * x is zero.
 *
 * Sixty-two entries and that terminator, measured out of the image; the y is
 * an offset from 0x19f, which is where the strip sits on the screen.
 */
struct intro_step {
    int16_t   x;                  /* +0x00  zero ends the roll */
    int16_t   y;                  /* +0x02  0x19f is added before drawing */
    int16_t   bitmap;             /* +0x04  an index into the intro's list */
} __attribute__((packed));

struct dg_2370 {
    struct intro_step step[63];   /* +0x00 */
} __attribute__((packed));

#define DG2370 (*(volatile struct dg_2370 *)(dgroup + 0x2370))

DG_ASSERT_AT(struct intro_step, x,              0x00);
DG_ASSERT_AT(struct intro_step, y,              0x02);
DG_ASSERT_AT(struct intro_step, bitmap,         0x04);
DG_ASSERT_AT(struct dg_2370, step,              0x00);

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
/*
 * **The saved file record**, at DGROUP 0x639e. `seek_named_chunk` copies a
 * record here on the way in and `restore_file_record_from_saved` puts it back,
 * so a failed search leaves the file exactly as it found it.
 *
 * 0x44 bytes, which is the third independent measurement of a file record's
 * frame slot: `copy_file_record` moves 0x43, `load_bitmaps`' two buffers are
 * `[bp-0xa2]`..`[bp-0x5e]`..`[bp-0x1a]` - 0x44 apart either way - and this one
 * runs to `dg_63e2` exactly 0x44 on. The first of those three disagreed with
 * the other two for weeks, as `uint8_t saved_a[52]`, and only a sanitizer saw
 * it.
 */
struct dg_639e {
    uint8_t   record[0x44];       /* +0x00 */
} __attribute__((packed));

#define DG639E (*(volatile struct dg_639e *)(dgroup + 0x639e))

DG_ASSERT_AT(struct dg_639e, record,            0x00);

struct dg_63e2 {
    uint16_t  pending_rows;       /* +0x00  counts rows, not pixels */
    /* **Pairs, and this record is why.** `compress_bitmap_list` measures how
       much it wrote as `out.seg - out_start.seg` paragraphs *plus*
       `out.off - out_start.off` bytes, subtracting the halves separately -
       which is a distance no single pointer can give. It also renormalises
       `out` by hand between bitmaps and steps its offset alone in between. */
    struct far_ptr out_start;     /* +0x02  where the output started, and
                                            does not move */
    uint16_t  word_63e8;          /* +0x06 */
    uint16_t  word_63ea;          /* +0x08 */
    uint16_t  word_63ec;          /* +0x0a */
    struct far_ptr out;           /* +0x0c  where the next byte goes */
    uint16_t  word_63f2;          /* +0x10 */
    uint16_t  mode;               /* +0x12  0x243bf sets it; it chooses how the runs are written */
} __attribute__((packed));

#define DG63E2 (*(volatile struct dg_63e2 *)(dgroup + 0x63e2))

DG_ASSERT_AT(struct dg_63e2, pending_rows,      0x00);
DG_ASSERT_AT(struct dg_63e2, out_start,         0x02);
DG_ASSERT_AT(struct dg_63e2, word_63e8,         0x06);
DG_ASSERT_AT(struct dg_63e2, word_63ea,         0x08);
DG_ASSERT_AT(struct dg_63e2, word_63ec,         0x0a);
DG_ASSERT_AT(struct dg_63e2, out,               0x0c);
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
    struct far_ptr block;         /* +0x0a  allocated once and kept; a null
                                     pointer is the end */
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

/*
 * **The wrapped text's line starts**, at DGROUP 0x56a6 - a near pointer into
 * the caller's own string for each line `wrap_text_to_box` decided on, and
 * `DG568F.line_count` of them.
 *
 * Nine words, settled from three directions that agree. The wrapper caps the
 * box at seven line heights, so seven lines can start inside it and one more
 * is written before the height is re-tested; `draw_wrapped_text` finds a
 * line's end by reading the *next* entry, so the table needs one past the
 * last; and the saved-rectangle slots begin at 0x56b8, which is nine words on.
 */
struct dg_56a6 {
    dg_off_t  line[9];            /* +0x00 */
} __attribute__((packed));

#define DG56A6 (*(volatile struct dg_56a6 *)(dgroup + 0x56a6))

DG_ASSERT_AT(struct dg_56a6, line,              0x00);

/*
 * **The twenty saved-rectangle slots**, at DGROUP 0x56b8. Each is a near
 * pointer to the head of a chain of records, or zero for an empty slot;
 * `find_saved_rect_slot` walks all twenty and `restore_saved_rect_lists`
 * counts down every record on every chain. Twenty words end at 0x56e0, where
 * the free list is.
 *
 * A slot is handed around as a pointer to its word - `find_saved_rect_slot`
 * answers one, or NULL - and the records on a chain are `struct
 * rect_list_entry`.
 */
struct dg_56b8 {
    dg_off_t  slot[0x14];         /* +0x00 */
} __attribute__((packed));

#define DG56B8 (*(volatile struct dg_56b8 *)(dgroup + 0x56b8))

DG_ASSERT_AT(struct dg_56b8, slot,              0x00);
DG_ASSERT_AT(struct dg_568f, word_5697,         0x08);
DG_ASSERT_AT(struct dg_568f, block,             0x0a);
DG_ASSERT_AT(struct dg_568f, word_569d,         0x0e);
DG_ASSERT_AT(struct dg_568f, word_569f,         0x10);
DG_ASSERT_AT(struct dg_568f, text_height,       0x11);
DG_ASSERT_AT(struct dg_568f, text_width,        0x13);
DG_ASSERT_AT(struct dg_568f, line_count,        0x15);

/*
 * **The moving parts**, at DGROUP 0x5179.
 */
struct dg_5179 {
    /* **The moving parts: a doubly linked list's head and tail**, the second
       list the level file fills (`n_moving`) - balls, balloons, buckets,
       rockets - and what gravity and the step passes walk. The pair is read
       the way the bin's at 0x50d7 is; see `parts_bin`. */
    struct list_node moving_parts;  /* +0x00  the head cell: next_ptr the first part, prev_ptr the last - cleared with it, never otherwise written */
} __attribute__((packed));

#define DG5179 (*(volatile struct dg_5179 *)(dgroup + 0x5179))

DG_ASSERT_AT(struct dg_5179, moving_parts,    0x00);

/*
 * **The shape and part free lists**, at DGROUP 0x4e4e.
 */
struct dg_4e4e {
    struct far_ptr shape_free;    /* +0x00  the free list nodes come off */
    dg_off_t  shapes_ptr;         /* +0x04  the shapes drawn over, put back in reverse */
    dg_off_t  shapes_tail_ptr;    /* +0x06 */
    dg_off_t  parts_free_ptr;     /* +0x08  the head; 0x4e58 is the queue folded onto it */
    dg_off_t  parts_queue_ptr;    /* +0x0a  what asked to move this frame */
    char      name_buf[0xd];      /* +0x0c  pick_file fills this and copies the
                                     answer out; `validate_filename` reads it
                                     back. Thirteen bytes - an 8.3 name and its
                                     NUL - which is what is left before
                                     `dg_4e67` */
} __attribute__((packed));

#define DG4E4E (*(volatile struct dg_4e4e *)(dgroup + 0x4e4e))

DG_ASSERT_AT(struct dg_4e4e, shape_free,        0x00);
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
    /* **The sixteen slots themselves**: a far pointer apiece from +0x0b and
       a countdown-and-period pair apiece from +0x4b. `timer_add_callback`
       takes the first clear bit of `slot_mask` and files all three;
       `timer_tick` counts every live slot down, calls it at zero and reloads
       it from its period. Sixteen is the mask's width, which is the extent
       both walkers use. */
    struct far_ptr callback[16];  /* +0x0b  0x44f9 */
    struct {
        int16_t left;             /* +0x00  counts down to the call */
        int16_t period;           /* +0x02  what it reloads from */
    } tick[16];                   /* +0x4b  0x4539 */
} __attribute__((packed));

#define DG44EE (*(volatile struct dg_44ee *)(dgroup + 0x44ee))

DG_ASSERT_AT(struct dg_44ee, installed,         0x00);
DG_ASSERT_AT(struct dg_44ee, frame_budget,      0x01);
DG_ASSERT_AT(struct dg_44ee, word_44f1,         0x03);
DG_ASSERT_AT(struct dg_44ee, divider_reload,    0x05);
DG_ASSERT_AT(struct dg_44ee, divider,           0x07);
DG_ASSERT_AT(struct dg_44ee, slot_mask,         0x09);
DG_ASSERT_AT(struct dg_44ee, callback,          0x0b);
DG_ASSERT_AT(struct dg_44ee, tick,              0x4b);
_Static_assert(sizeof(struct dg_44ee) == 0x8b, "the timer state ends at 0x4579");

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
 * **The text the player types on the puzzle screen**, at DGROUP 0x542e - a
 * password or a score code - which `picker_type` fills to 0x19 characters
 * and `password_to_level`, `score_code_to_score` and `puzzle_draw_password`
 * read. Forty bytes, up to DG5456.
 */
struct dg_542e {
    char typed[0x28];             /* +0x00 */
} __attribute__((packed));

#define DG542E (*(volatile struct dg_542e *)(dgroup + 0x542e))
_Static_assert(sizeof(struct dg_542e) == 0x28, "the typed text ends at DG5456");

/*
 * **The belt's far end and the goal's frame counter**, at DGROUP 0x5456.
 */
struct dg_5456 {
    uint16_t  belt_far_end;       /* +0x00  the far end's +0x5a, stashed while it is detached */
    /* **Ten words, and the original reuses them.** `goal_test_1b89` at 0x01bb4
       does `inc word ptr [0x5458]` - a plain count of frames the goal has
       held, and passing 0xc wins. `goal_test_1552` at 0x015bf does
       `cmp word ptr [bx + 0x5458], 0` with `bx` twice the mouse-cage count:
       a per-cage table of which have been set going, zeroed ten wide by
       `clear_machine`. Both are the binary's; the counter is element 0.
       It was declared as the one word, which is the `blocks[9]` shape. */
    union {
        uint16_t  goal_frames;    /* +0x02 */
        uint16_t  cage_ran[10];   /* +0x02 .. +0x15 */
    };
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
    struct far_ptr cursor;        /* +0x02  a static far pointer, with its
                                            selector beside it */
    int16_t   selector;           /* +0x06 */
} __attribute__((packed));

#define DG6430 (*(volatile struct dg_6430 *)(dgroup + 0x6430))

DG_ASSERT_AT(struct dg_6430, ticks_left,        0x00);
DG_ASSERT_AT(struct dg_6430, cursor,        0x02);
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
 * **A part's point table**: a run of `byte_pair` at a constant DGROUP offset,
 * which the `part_setup_*` routines copy into the part's own `points_ptr`.
 * They are of different lengths and are not one array - and one routine reads
 * its table's address out of a table of addresses indexed by the part's form -
 * so the offset stays at the call site and only its *type* is stated here.
 */
/* Not `volatile`, and neither is `POINT16_TABLE` below: a point table is
   constant data in the image - the `const` says nothing writes it - and the
   timer handler reaches no part or part data at all. See `PARTP` in full. */
#define POINT_TABLE(off) \
    ((const struct byte_pair *)(dgroup + (uint16_t)(off)))

/*
 * **A table of near pointers**, whatever they point at and whatever indexes
 * them. Two shapes use it and they have nothing in common but this: the
 * `part_setup_*` routines read a point table's address out of one, indexed by
 * a part's form - `POINT_TABLE(OFF_TABLE(0x33e6)[form])`, and 0x33e6 holds
 * 0x33ce, 0x33d6, 0x33de, three point tables 8 bytes apart - and
 * `load_palette` reads a chunk name out of one indexed by the adapter, where
 * 0x44a2 holds 0x44a1, 0x4498, 0x448f, which are "", "PAL:CGA:" and
 * "PAL:EGA:".
 *
 * It was called `FORM_TABLE` while only the first was known. The name said the
 * index rather than the shape, so the second use had a choice between a second
 * macro for one shape - the trap recorded in CLAUDE.md, met twice already in
 * this file - and a raw accessor. The index belongs at the call site, which is
 * where it is written.
 *
 * **A base is not always where its table starts, and these sit next to
 * tables of a different kind.** The compiler folds the first index into the
 * address, so `DG16(0x3384 + 2 * form)` with the form pinned to 8..10 reads
 * 0x3394 onward and `0x3384` is nothing but `0x3394 - 16`. That address is
 * also the fourth entry of the offsets at 0x338c, which a different routine
 * reads off its own base. The two tables are **adjacent, not overlapping** -
 * measured after each site's form range was read - so three reads in
 * `parts.c` keep the original's own base and index rather than being given a
 * name that would have to pick a base the original never mentions. Each says
 * which words it reads and what they are.
 */
#define OFF_TABLE(off) \
    ((const volatile dg_off_t *)(dgroup + (uint16_t)(off)))

/*
 * ---------------------------------------------------------------------------
 * **A string in DGROUP**, so a routine that takes one takes an ordinary
 * pointer into `guest_mem`.
 *
 * They are data and not code, measured rather than assumed: DGROUP's
 * initialised image starts at **0x2d3c0** - `docs/executable.md` derives that
 * from the Borland startup loading DS - and runs to the end of the 0x345f0-byte
 * image, 0x7230 bytes with nothing above it. The highest transcribed routine is
 * `atan2_long` at 0x2d296, **298 bytes below** the boundary, so code and data
 * are adjacent rather than far apart and "it looks like data" would not have
 * settled it. Each of these strings occurs **exactly once** in the whole image,
 * at its DGROUP offset: "BMP:SCN:" only at 0x31d86, which is 0x2d3c0 + 0x49c6.
 * ---------------------------------------------------------------------------
 */
#define STR(off) ((const uint8_t *)(dgroup + (uint16_t)(off)))

/*
 * ---------------------------------------------------------------------------
 * **The chunk paths**, at DGROUP 0x4966: the names `seek_named_chunk` walks a
 * file's nesting along. Each is two four-character tags with no separator,
 * which is what its "non-zero multiple of four" check is about.
 *
 * They are transcribed as what they are - a run of NUL-terminated strings the
 * compiler laid down in the order the routines that use them appear - so the
 * fopen modes it emitted between them are fields here too, and the run ends
 * exactly where the sound device's tag table begins.
 *
 * "BMP:OFF:" is here **twice**, at 0x49cf and 0x49e1: two copies of one
 * literal, and `load_bitmaps` pushes a different one at each of its two call
 * sites.
 * ---------------------------------------------------------------------------
 */
struct chunk_names {
    char bmp_inf[9];        /* +0x00  0x4966  "BMP:INF:" */
    char bmp_bin[9];        /* +0x09  0x496f  "BMP:BIN:" */
    char mode_r_a[2];       /* +0x12  0x4978  "r" */
    char bmp_vga[9];        /* +0x14  0x497a  "BMP:VGA:" */
    char bmp_amg[9];        /* +0x1d  0x4983  "BMP:AMG:" */
    char mode_r_b[2];       /* +0x26  0x498c  "r" */
    char scr_dim[9];        /* +0x28  0x498e  "SCR:DIM:" */
    char scr_bin[9];        /* +0x31  0x4997  "SCR:BIN:" */
    char mode_r_c[2];       /* +0x3a  0x49a0  "r" */
    char scr_vga[9];        /* +0x3c  0x49a2  "SCR:VGA:" */
    char scr_amg[9];        /* +0x45  0x49ab  "SCR:AMG:" */
    char mode_r_d[2];       /* +0x4e  0x49b4  "r" */
    char mode_rb[3];        /* +0x50  0x49b6  "rb" */
    /* +0x53  0x49b9. Not strings: `06 00` and then five words that read as
       far pointers - 3e29:1c25, 61fd:1c25, and 1063 - which is another
       module's data sharing the region. Named as the gap it is. */
    uint8_t pad_49b9[13];
    char bmp_scn[9];        /* +0x60  0x49c6  "BMP:SCN:" */
    char bmp_off[9];        /* +0x69  0x49cf  "BMP:OFF:" */
    char bmp_vqt[9];        /* +0x72  0x49d8  "BMP:VQT:" */
    char bmp_off_b[9];      /* +0x7b  0x49e1  "BMP:OFF:" - the second copy */
    char bmp_rle[9];        /* +0x84  0x49ea  "BMP:RLE:" */
    char bmp_scl[9];        /* +0x8d  0x49f3  "BMP:SCL:" */
    uint8_t pad_49fc[2];
    char scr_vqt[9];        /* +0x98  0x49fe  "SCR:VQT:" */
    uint8_t pad_4a07[1];
    char ssm_000[9];        /* +0xa2  0x4a08  "SSM:000:" */
    uint8_t pad_4a11[1];
    /* +0xac  0x4a12. **Not a constant: a buffer.** The image holds
       `53 53 4d 3a 20 20 20 20 20 00` - "SSM:" and *five* spaces, which is
       nine characters and would fail the multiple-of-four check.
       `setup_sound_device` writes a four-character tag **and its NUL** over
       the spaces at +4 first, so the path is eight when it is walked and the
       fifth space is the room that NUL needs. */
    char ssm_tag[10];
} __attribute__((packed));

/* **Not `volatile`.** These are the compiler's string literals; nothing
   writes them. The two buffers among them are filled by `string_copy_far`,
   which takes an offset, so even those are not written through this. */
#define CHUNK (*(struct chunk_names *)(dgroup + 0x4966))

DG_ASSERT_AT(struct chunk_names, bmp_inf,   0x00);
DG_ASSERT_AT(struct chunk_names, bmp_bin,   0x09);
DG_ASSERT_AT(struct chunk_names, bmp_vga,   0x14);
DG_ASSERT_AT(struct chunk_names, bmp_amg,   0x1d);
DG_ASSERT_AT(struct chunk_names, scr_dim,   0x28);
DG_ASSERT_AT(struct chunk_names, scr_bin,   0x31);
DG_ASSERT_AT(struct chunk_names, scr_vga,   0x3c);
DG_ASSERT_AT(struct chunk_names, scr_amg,   0x45);
DG_ASSERT_AT(struct chunk_names, mode_rb,   0x50);
DG_ASSERT_AT(struct chunk_names, bmp_scn,   0x60);
DG_ASSERT_AT(struct chunk_names, bmp_off,   0x69);
DG_ASSERT_AT(struct chunk_names, bmp_vqt,   0x72);
DG_ASSERT_AT(struct chunk_names, bmp_off_b, 0x7b);
DG_ASSERT_AT(struct chunk_names, bmp_rle,   0x84);
DG_ASSERT_AT(struct chunk_names, bmp_scl,   0x8d);
DG_ASSERT_AT(struct chunk_names, scr_vqt,   0x98);
DG_ASSERT_AT(struct chunk_names, ssm_000,   0xa2);
DG_ASSERT_AT(struct chunk_names, ssm_tag,   0xac);
_Static_assert(sizeof(struct chunk_names) == 0xb6,
               "the run ends at 0x4a1c, where the device tag table begins");

/*
 * **The palette pointer table**, at DGROUP 0x4466: sixteen words, one per
 * pixel shift, which `load_palette` and `set_palette_pointer` file into
 * `DG4460.word_4464`. Up to 0x4486, where the chunk names begin.
 */
struct dg_4466 {
    int16_t   pointer[16];        /* +0x00 */
} __attribute__((packed));

#define DG4466 (*(volatile struct dg_4466 *)(dgroup + 0x4466))
_Static_assert(sizeof(struct dg_4466) == 0x20, "the palette pointers end at 0x4486");

/*
 * ---------------------------------------------------------------------------
 * **The palette chunk names and the table that chooses between them**, at
 * DGROUP 0x4486 - and the table is *inside* the run, four strings then
 * sixteen offsets into them, indexed by the adapter's pixel shift.
 *
 * Entry 0 is the empty string at 0x44a1, which is the NUL that ends
 * "PAL:CGA:". `seek_named_chunk` refuses a zero-length path, so shift 0 asks
 * for no palette chunk at all.
 * ---------------------------------------------------------------------------
 */
struct pal_chunk_names {
    char pal_vga[9];          /* +0x00  0x4486  "PAL:VGA:" */
    char pal_ega[9];          /* +0x09  0x448f  "PAL:EGA:" */
    char pal_cga[9];          /* +0x12  0x4498  "PAL:CGA:" */
    char none[1];             /* +0x1b  0x44a1  "" */
    dg_off_t by_adapter[16];  /* +0x1c  0x44a2  which of the four, by shift */
    uint8_t  pad_44c2[4];
    char pal_amg[9];          /* +0x40  0x44c6  "PAL:AMG:" */
} __attribute__((packed));

#define PALCHUNK (*(struct pal_chunk_names *)(dgroup + 0x4486))

DG_ASSERT_AT(struct pal_chunk_names, pal_vga,    0x00);
DG_ASSERT_AT(struct pal_chunk_names, pal_ega,    0x09);
DG_ASSERT_AT(struct pal_chunk_names, pal_cga,    0x12);
DG_ASSERT_AT(struct pal_chunk_names, none,       0x1b);
DG_ASSERT_AT(struct pal_chunk_names, by_adapter, 0x1c);
DG_ASSERT_AT(struct pal_chunk_names, pal_amg,    0x40);

/*
 * ---------------------------------------------------------------------------
 * **The overlay chunk name and the adapter tags**, at DGROUP 0x4919. The same
 * shape as `CHUNK.ssm_tag`: `4f 56 4c 3a 20 20 20 20 20 00` is "OVL:" and
 * five spaces, and the caller copies a four-character tag and its NUL over the
 * spaces at +4 before the seek.
 *
 * The tags follow it, eleven of five bytes. The code reaches them only through
 * `ADAPTER_TAGS`, whose table sits *below* this at 0x4901 and one of whose
 * entries - "BAD:" - points below that again, so they are transcribed as the
 * run they are rather than named one by one.
 * ---------------------------------------------------------------------------
 */
struct ovl_chunk_names {
    char ovl_tag[10];        /* +0x00  0x4919  "OVL:" + room for the tag */
    char adapter_tag[11][5]; /* +0x0a  0x4923  CGA: EGA: TAN: HER: MCG: EVA:
                                               VGA: EVG: HVG: HEG: NEW: */
} __attribute__((packed));

#define OVLCHUNK (*(struct ovl_chunk_names *)(dgroup + 0x4919))

DG_ASSERT_AT(struct ovl_chunk_names, ovl_tag,     0x00);
DG_ASSERT_AT(struct ovl_chunk_names, adapter_tag, 0x0a);

/* The four-character tags the two buffers above are completed from. The
   tables hold offsets rather than the tags themselves, which is why these
   stay `OFF_TABLE` and not a run of `char[5]`. */
#define ADAPTER_TAGS    OFF_TABLE(0x48ff)  /* [si], si from 1: "CGA:" on */
#define DEVICE_TAGS     OFF_TABLE(0x4a1c)  /* "STD:" "TAN:" "ADL:" ... */
#define MODULE_TAGS     OFF_TABLE(0x4a2e)  /* "ASB:" "APS:" "ATD:" ... */

/* A signed 16-bit point: a part's position, box and size generations, a
   belt's and a rope's corners. Declared here because `struct part` is the
   first to hold one. */
struct point16 {
    int16_t x;                 /* +0x00 */
    int16_t y;                 /* +0x02 */
} __attribute__((packed));

/*
 * ---------------------------------------------------------------------------
 * **A part**, the 0xa2-byte record the machine is made of.
 *
 * Not a fixed DGROUP address like the overlays above - the records are cut
 * from the near heap and reached through a 16-bit offset, so this is the shape
 * and `PART_PTR(p)` is how a routine that has been handed one looks at it. That is
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
    /* **The next part on its list, and the previous.** The lists' heads are
       `struct list_node` cells - `parts_bin`, `moving_parts`, `placed_parts`
       - and the game treats a head as a part whose only fields are these two: `insert_sorted`
       files the head's *address* into the first part's `prev_ptr`, and
       `unlink_part` writes `PART_PTR(prev_ptr)->next_ptr` without asking whether
       that names a head or a part. One `mov` for both in the original. */
    dg_off_t  next_ptr;        /* +0x00 */
    /* **The bin list's back-link, not padding.** `insert_sorted` writes it - and
       writes the next node's back to `rec` - and `bin_part_at_index` walks it
       to step backwards from the sentinel at 0x50d7. */
    dg_off_t  prev_ptr;        /* +0x02 */
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
    /* **Three generations each of the position, the box and the size**, a
       `point16` triple apiece with the newest first. `shift_state_history`
       ages every triple with two 32-bit moves, `reset_machine` seeds the
       older two of the box and the size from the newest, and
       `add_record_shapes` hands generation 2 or 3 of the box and the size
       *together* to `alloc_shape` - which is what pairs the three. The words
       keep their old names in the struct beside each triple: the triple is
       what the moves say, the names are what the comparisons say. */
    union {
        struct point16 pos[3];     /* +0x1e  gen 1 at +0x1e, 2 at +0x22, 3 at +0x26 */
        struct {
            int16_t   pos_x;       /* +0x1e  the part's position; the grab box at +0x56 is added to it */
            int16_t   pos_y;       /* +0x20 */
            int16_t   word_22;     /* +0x22 */
            int16_t   word_24;     /* +0x24 */
            uint16_t  word_26;     /* +0x26 */
            uint16_t  word_28;     /* +0x28  compared against pos_y, and taken from
                                             word_8e when the machine resets */
        };
    };
    union {
        struct point16 box[3];     /* +0x2a  gen 1 at +0x2a, 2 at +0x2e, 3 at +0x32 */
        struct {
            int16_t   box_x;       /* +0x2a  the part's own box, which the pointer is tested against */
            int16_t   box_y;       /* +0x2c */
            uint16_t  word_2e;     /* +0x2e */
            uint8_t   pad_30[2];
            uint16_t  word_32;     /* +0x32 */
            uint8_t   pad_34[2];
        };
    };
    int16_t   vel_x;           /* +0x36  velocity, stepped by the movers */
    int16_t   word_38;         /* +0x38 */
    int16_t   weight;          /* +0x3a  devdump prints it as `wt` */
    /* **One 32-bit momentum**, and both spellings are the same four bytes -
       the same shape as `fx` above. `part_step_*` reads and writes it whole
       with `DG32(si + 0x3c)`; the halves are named because other routines
       store one at a time. A field that is only the low word here would be a
       two-byte read where the original makes a four-byte one, which is the
       defect that stopped three levels solving once already. */
    union {
        int32_t   momentum;    /* +0x3c */
        struct {
            uint16_t momentum_lo;  /* +0x3c  low word first */
            uint16_t momentum_hi;  /* +0x3e */
        };
    };
    uint16_t  word_40;         /* +0x40 */
    uint16_t  word_42;         /* +0x42 */
    /* **A word each, not a byte.** The part builder at machine_draw.c writes
       both with a 16-bit move out of the kind table at 0x296e/0x2970, and
       `DG16(si + 0x44) >> 4` turns one into a cell count; the `DG8` sites that
       gave them a byte width earlier are reading the low half of a value that
       never gets that large. */
    union {
        struct point16 size[3];    /* +0x44  gen 1 at +0x44, 2 at +0x48, 3 at +0x4c */
        struct {
            int16_t   width;       /* +0x44  one less than this is what the setups lay out */
            int16_t   height;      /* +0x46 */
            uint16_t  word_48;     /* +0x48 */
            uint8_t   pad_4a[2];
            uint16_t  word_4c;     /* +0x4c */
            uint8_t   pad_4e[2];
        };
    };
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
    /* **Six links, and the array is the fact.** `part_setup_2068` files four
       of them by direction and `part_setup_3de5` writes the last two, so the
       port had them as four named words plus a separate pair - and three
       `part_step_*` routines walk `+0x5a + 2 * i` with **i from 4 to 6**,
       which reaches 0x62 and 0x64. That is one six-word array indexed past
       its named half, not two tables that happen to be adjacent.
       `-Warray-bounds` is what said so, on `link[4]`, the moment the raw
       accessor became a field. */
    union {
        uint16_t link[6];                             /* +0x5a */
        struct {
            uint16_t link_right;   /* +0x5a  the four neighbours by direction */
            uint16_t link_left;    /* +0x5c */
            uint16_t link_down;    /* +0x5e */
            uint16_t link_up;      /* +0x60 */
            uint16_t linked_a;     /* +0x62  the pair part_setup_3de5 turns */
            uint16_t linked_b;     /* +0x64  into form bits */
        };
    };
    /* **The belt records this part is an end of**, indexed the same way as
       `link` above - `cut_belts` writes `+0x66 + 2 * slot`. A kind-0xa
       carrier's own belt is always the first, which is why the singular
       spelling is the one most of the port uses. */
    union {
        uint16_t belt_ptr[2];                         /* +0x66 */
        struct {
            uint16_t word_66;      /* +0x66 */
            uint16_t word_68;      /* +0x68 */
        };
    };
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
    /* **The next part on each of the two drawing layers this part is filed
       on** - `link_record_into_buckets` writes `[i]` for the layer its
       kind's `refile_level[i]` names, and `byte_7f` keeps which layer `[0]`
       is, so the walkers pick the half that matches the layer they are on. */
    union {
        dg_off_t  layer_next[2];                      /* +0x74 */
        struct {
            uint16_t  word_74; /* +0x74 */
            uint16_t  word_76; /* +0x76 */
        };
    };
    /* **The next part in a chain, and only after something builds one.** Five
       routines zero it on the head and then thread parts on by insertion -
       `collect_carried`, `link_nearby_objects`, `link_objects_in_range`,
       `link_objects_crossing` and `link_objects_at_point`. Everything else
       only walks it, so a step routine that reads it is reading whatever the
       last of those five left; it means nothing before one has run. */
    dg_off_t  next_linked_ptr; /* +0x78 */
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
    dg_off_t  contact_ptr;     /* +0x84  the part this one is in contact with */
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

/*
 * **A part record is not `volatile`, and it is the one record that says so.**
 *
 * The reason every other accessor here is volatile is the timer handler, which
 * runs on a thread of its own and shares DGROUP with the main thread - see the
 * note above `DG8`. What it touches is a short list: the clip box, the two page
 * pointers, its own guards, and the two words the frame spins on. It reaches no
 * part record. `timer_callback` goes to `redraw_cursor` and then `draw_cursor`,
 * and neither that nor `restage_object_rect`, `save_or_restore_draw_state` or
 * the rect thunks under it touches a `PART` at all.
 *
 * So the qualifier would buy nothing and cost a good deal: a part is read in
 * the tightest loops in the game - `machine.c` alone reaches one at 397 sites.
 * `struct bitmap` is already spelled this way for the same reason, which is
 * why `BMPP` reads as it does.
 */
/* **A part offset of 0 is NULL.** The guest's null near pointer is DGROUP
   offset 0, and a walk that ends on it ends on a C null pointer here rather
   than on a `struct part` overlaid on the segment's first bytes - so a list
   loop reads `p != NULL`, and reading through a null part faults instead of
   reading whatever is at DGROUP 0. A function rather than a macro so the
   offset is evaluated once. */
static inline struct part *PART_PTR(uint16_t p)
{
    return p != 0 ? (struct part *)(dgroup + p) : NULL;
}
_Static_assert(__builtin_offsetof(struct part, next_ptr) == __builtin_offsetof(struct list_node, next_ptr)
               && __builtin_offsetof(struct part, prev_ptr) == __builtin_offsetof(struct list_node, prev_ptr),
               "a list's head cell is read through the part layout, so the links have to line up");

DG_ASSERT_AT(struct part, next_ptr,       0x00);
DG_ASSERT_AT(struct part, prev_ptr,       0x02);
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
DG_ASSERT_AT(struct part, pos,            0x1e);
DG_ASSERT_AT(struct part, pos_x,          0x1e);
DG_ASSERT_AT(struct part, pos_y,          0x20);
DG_ASSERT_AT(struct part, word_22,        0x22);
DG_ASSERT_AT(struct part, word_24,        0x24);
DG_ASSERT_AT(struct part, word_26,        0x26);
DG_ASSERT_AT(struct part, word_28,        0x28);
DG_ASSERT_AT(struct part, box,            0x2a);
DG_ASSERT_AT(struct part, box_x,          0x2a);
DG_ASSERT_AT(struct part, box_y,          0x2c);
DG_ASSERT_AT(struct part, word_2e,        0x2e);
DG_ASSERT_AT(struct part, word_32,        0x32);
DG_ASSERT_AT(struct part, vel_x,          0x36);
DG_ASSERT_AT(struct part, word_38,        0x38);
DG_ASSERT_AT(struct part, weight,         0x3a);
DG_ASSERT_AT(struct part, momentum,       0x3c);
DG_ASSERT_AT(struct part, momentum_lo,    0x3c);
DG_ASSERT_AT(struct part, momentum_hi,    0x3e);
DG_ASSERT_AT(struct part, word_40,        0x40);
DG_ASSERT_AT(struct part, word_42,        0x42);
DG_ASSERT_AT(struct part, size,           0x44);
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
DG_ASSERT_AT(struct part, link,           0x5a);
DG_ASSERT_AT(struct part, link_right,     0x5a);
DG_ASSERT_AT(struct part, link_left,      0x5c);
DG_ASSERT_AT(struct part, link_down,      0x5e);
DG_ASSERT_AT(struct part, link_up,        0x60);
DG_ASSERT_AT(struct part, linked_a,       0x62);
DG_ASSERT_AT(struct part, linked_b,       0x64);
DG_ASSERT_AT(struct part, belt_ptr,       0x66);
DG_ASSERT_AT(struct part, word_66,        0x66);
DG_ASSERT_AT(struct part, word_68,        0x68);
DG_ASSERT_AT(struct part, byte_6a,        0x6a);
DG_ASSERT_AT(struct part, byte_6b,        0x6b);
DG_ASSERT_AT(struct part, byte_6c,        0x6c);
DG_ASSERT_AT(struct part, byte_6d,        0x6d);
DG_ASSERT_AT(struct part, byte_72,        0x72);
DG_ASSERT_AT(struct part, byte_73,        0x73);
DG_ASSERT_AT(struct part, next_linked_ptr, 0x78);
DG_ASSERT_AT(struct part, layer_next,     0x74);
DG_ASSERT_AT(struct part, word_74,        0x74);
DG_ASSERT_AT(struct part, word_76,        0x76);
DG_ASSERT_AT(struct part, word_7a,        0x7a);
DG_ASSERT_AT(struct part, word_7c,        0x7c);
DG_ASSERT_AT(struct part, byte_7e,        0x7e);
DG_ASSERT_AT(struct part, byte_7f,        0x7f);
DG_ASSERT_AT(struct part, point_count,    0x80);
DG_ASSERT_AT(struct part, points_ptr,     0x82);
DG_ASSERT_AT(struct part, contact_ptr,    0x84);
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
    struct far_ptr table;         /* +0x00  the far pointer the list reader
                                     allocates and frees */
    uint16_t  record_count;       /* +0x04  how many records of 0xa2 bytes came off the near heap */
    uint16_t  is_level;           /* +0x06  load_level sets it; save_machine zeroes it. It decides how much of a record is written and read */
    int16_t   version;            /* +0x08  the version gate: from 0x101 the file carries more */
    uint16_t  version_out;        /* +0x0a  written out beside it */
    uint16_t  error;              /* +0x0c  every writer checks it, and a file that fails to close is deleted */
    int16_t   cache_key;          /* +0x0e  the one-entry cache in front of find_entry_for_pointer: */
    int16_t   cache_answer;       /* +0x10  the pointer last asked about, and the answer */
    int16_t   archive_count;      /* +0x12  how many archives, accumulated; zero means none is open */
    uint16_t  last_record;        /* +0x14  where the search starts, so record 0 is never returned */
    /* **A 32-bit hash, not a pointer.** `hash_filename` splits its
       `uint32_t acc` across these two and `find_entry_for_pointer`
       compares the pair against each entry's first four bytes; the
       `_off`/`_seg` names its readers used were a misreading of a key. */
    uint32_t  name_hash;          /* +0x16  what hash_filename leaves for
                                            find_entry_for_pointer */
    uint8_t   open_immediate;     /* +0x1a  clear means try the file by name and close it again */
    uint8_t   byte_5487;          /* +0x1b */
    uint8_t   retry;              /* +0x1c  the loop around the loose-file open, for removable media */
    uint8_t   byte_5489;          /* +0x1d */
    uint8_t   scanned;            /* +0x1e  the archives have been counted once */
    int16_t   file_used;          /* +0x1f  the FILE it actually read from */
    int16_t   file_asked;         /* +0x21  and the one it was asked about */
} __attribute__((packed));

#define DG546C (*(volatile struct dg_546c *)(dgroup + 0x546c))

DG_ASSERT_AT(struct dg_546c, table,             0x00);
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
DG_ASSERT_AT(struct dg_546c, open_immediate,    0x1a);
DG_ASSERT_AT(struct dg_546c, byte_5487,         0x1b);
DG_ASSERT_AT(struct dg_546c, retry,             0x1c);
DG_ASSERT_AT(struct dg_546c, byte_5489,         0x1d);
DG_ASSERT_AT(struct dg_546c, scanned,           0x1e);
DG_ASSERT_AT(struct dg_546c, file_used,         0x1f);
DG_ASSERT_AT(struct dg_546c, file_asked,        0x21);

/*
 * ---------------------------------------------------------------------------
 * **A game file**, the 0x12-byte record `game_fopen` hands back and every
 * `game_f*` routine takes. There are ten of them at DGROUP 0x55c3 and the
 * table's extent is settled from both ends: the eleven 0x1c-byte archive
 * records above it end at 0x55c3, and `DG5677` begins exactly ten records
 * later.
 *
 * A file the archive knows about is described by the four fields below and
 * read through the archive's own handle; a loose file has `stream` set instead
 * and every routine hands the work straight to the stdio layer. `in_use` is
 * what `game_fopen` looks for a clear one of and `game_fclose` clears.
 *
 * The three 32-bit quantities are kept as their halves because that is how the
 * routines do the arithmetic - `game_fread` subtracts `pos` from `size` with
 * an explicit borrow, and `game_fseek` compares the two a half at a time.
 *
 * Field names are ours; the offsets and the size are the original's.
 * ---------------------------------------------------------------------------
 */
struct game_file {
    uint16_t  archive;         /* +0x00  which archive holds it, an index into
                                  the 0x1c-byte records at DGROUP 0x548f */
    /* **Three Borland `long`s**, measured rather than inferred from the
       naming: 0x092b2 steps `pos` with `add [di+0xa],ax / adc [di+0xc],0`
       and 0x09270 adds it to `base` with `add dx,[di+0xa] / adc ax,[di+0xc]`.
       `open_game_file` reads four bytes straight into `size` with one
       `stdio_fread`, which settles that one on its own.

       Every comparison against them is **unsigned**, where `struct
       resource`'s are signed. That difference is the original's. */
    uint32_t  base;            /* +0x02  where the entry's data starts in that
                                         archive, past its 17-byte header */
    uint32_t  size;            /* +0x06  the entry's size, out of that header */
    uint32_t  pos;             /* +0x0a  how far into the entry the reader is;
                                         `base + pos` is where to seek */
    uint16_t  in_use;          /* +0x0e  the slot is taken */
    dg_off_t  stream;          /* +0x10  the loose file, when there is one */
} __attribute__((packed));

#define GAME_FILE_PTR(p) ((volatile struct game_file *)(dgroup + (uint16_t)(p)))

DG_ASSERT_AT(struct game_file, archive,         0x00);
DG_ASSERT_AT(struct game_file, base,            0x02);
DG_ASSERT_AT(struct game_file, size,            0x06);
DG_ASSERT_AT(struct game_file, pos,             0x0a);
DG_ASSERT_AT(struct game_file, in_use,          0x0e);
DG_ASSERT_AT(struct game_file, stream,          0x10);

/*
 * **The ten game files**, at DGROUP 0x55c3.
 */
struct dg_55c3 {
    struct game_file files[0xa];  /* +0x00 */
} __attribute__((packed));

#define DG55C3 (*(volatile struct dg_55c3 *)(dgroup + 0x55c3))

DG_ASSERT_AT(struct dg_55c3, files,             0x00);

/*
 * ---------------------------------------------------------------------------
 * **An archive**, the 0x1c-byte record `load_archive_map` fills in from
 * `RESOURCE.MAP`. Eleven of them from DGROUP 0x548f, ending exactly where the
 * `game_file` table above begins - and `free_archive_lists` loops `i <= 10`,
 * which is the same eleven counted from the other side.
 *
 * Only one archive is open at a time: `make_file_current` closes the last
 * one's `stream` before opening this one's, and `last_record` says which. The
 * `pos` pair is what DOS is *believed* to be at, so `seek_file_to` can decline
 * a seek it has already made - measured at 319 seeks out of 18,930 calls.
 *
 * `name` is passed straight to `stdio_fopen`, so the record's own first byte is
 * the filename; 13 bytes is what the map file stores.
 *
 * Field names are ours; the offsets and the size are the original's.
 * ---------------------------------------------------------------------------
 */
struct archive {
    char      name[0xd];       /* +0x00  read out of RESOURCE.MAP, and handed
                                  to `fopen` as it stands */
    uint8_t   pad_0d[1];
    uint16_t  index;           /* +0x0e  its own index, written by the loader */
    dg_off_t  stream;          /* +0x10  open only while it is the current one */
    uint32_t  pos;             /* +0x12  where DOS is believed to be */
    uint8_t   pad_16[2];
    struct far_ptr list;       /* +0x18  the eight-byte entries the map read:
                                  a hash and an offset each, ending on an
                                  all-zero hash */
} __attribute__((packed));

#define ARCHIVE_PTR(p) ((volatile struct archive *)(dgroup + (uint16_t)(p)))

DG_ASSERT_AT(struct archive, name,              0x00);
DG_ASSERT_AT(struct archive, index,             0x0e);
DG_ASSERT_AT(struct archive, stream,            0x10);
DG_ASSERT_AT(struct archive, pos,               0x12);
DG_ASSERT_AT(struct archive, list,              0x18);

/*
 * **The eleven archives**, at DGROUP 0x548f. Index 0 is never where a search
 * begins - `find_entry_for_pointer` starts at 1 when `last_record` is clear.
 */
struct dg_548f {
    struct archive slot[0xb];     /* +0x00 */
} __attribute__((packed));

#define DG548F (*(volatile struct dg_548f *)(dgroup + 0x548f))

DG_ASSERT_AT(struct dg_548f, slot,              0x00);

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
    /* **Segment first**, which is `far_ptr_rev` and not `far_ptr` - and at
       an odd offset, which the packed record allows. */
    struct far_ptr_rev vector;    /* +0x13  vector 0, from 0:0 and 0:2 - a
                                            load, not a store */
    uint8_t   pad_48f1[1];
    uint8_t   mode_found;         /* +0x18  the mode the program found the adapter in */
    uint8_t   mode_forced;        /* +0x19  a forced setting; 0xd is the one these screens take */
    struct far_ptr driver;        /* +0x1a  the video driver, as vm_init
                                            stored it */
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
DG_ASSERT_AT(struct dg_48da, vector,        0x13);
DG_ASSERT_AT(struct dg_48da, mode_found,        0x18);
DG_ASSERT_AT(struct dg_48da, mode_forced,       0x19);
DG_ASSERT_AT(struct dg_48da, driver,            0x1a);

/*
 * **Which page pointers the saved-rect lists are restored between**, at
 * DGROUP 0x2d0a: pairs of addresses of the driver's page words at
 * 0x38a0..0x38a4 (and of the word at 0x2d08), walked by
 * `restore_saved_rect_lists` from pair 0 - or from pair 1 alone - until
 * the next pair's second word is 0. Nine pairs and the terminating pair
 * fill the run to 0x2d32.
 */
struct dg_2d0a {
    struct {
        dg_off_t src;             /* +0x00  the address of a page word */
        dg_off_t dst;             /* +0x02 */
    } pair[10];                   /* +0x00 */
} __attribute__((packed));

#define DG2D0A (*(volatile struct dg_2d0a *)(dgroup + 0x2d0a))
_Static_assert(sizeof(struct dg_2d0a) == 0x28, "the page pairs end at 0x2d32");

/*
 * **The cursor, the fade, and the palette waiting to load**, at DGROUP 0x2d32.
 */
struct dg_2d32 {
    uint16_t  page;               /* +0x00  the page the middle call passes */
    uint16_t  screen_disturbed;   /* +0x02  the saved rectangles are put back when this says so */
    uint16_t  word_2d36;          /* +0x04 */
    uint16_t  word_2d38;          /* +0x06 */
    struct far_ptr pending_pal;   /* +0x08  a palette waiting to be loaded */
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
DG_ASSERT_AT(struct dg_2d32, pending_pal,   0x08);
DG_ASSERT_AT(struct dg_2d32, cursor_off,        0x0c);
DG_ASSERT_AT(struct dg_2d32, delay_reload,      0x0e);
DG_ASSERT_AT(struct dg_2d32, read_driver,       0x10);
DG_ASSERT_AT(struct dg_2d32, flag_2d44,         0x12);
DG_ASSERT_AT(struct dg_2d32, word_2d46,         0x14);

/*
 * **The scratch block that is allocated to be freed**, at DGROUP 0x3576.
 */
struct dg_3576 {
    struct far_ptr scratch;       /* +0x00  picker_begin takes this if it is
                                     not null */
} __attribute__((packed));

#define DG3576 (*(volatile struct dg_3576 *)(dgroup + 0x3576))

DG_ASSERT_AT(struct dg_3576, scratch,           0x00);

/*
 * **The Huffman position tables**, at DGROUP 0x3686: for each code byte
 * `decode_position` reads, the high bits of the position at 0x3686 and the
 * length at 0x3786. 256 bytes each, up to 0x3886.
 */
struct dg_3686 {
    uint8_t   high[256];          /* +0x00 */
    uint8_t   len[256];           /* +0x100 */
} __attribute__((packed));

#define DG3686 (*(volatile struct dg_3686 *)(dgroup + 0x3686))
DG_ASSERT_AT(struct dg_3686, len, 0x100);

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
    /* Three cached far pointers into the decompressor's block. `a` and `b`
       share a segment - the Huffman tables are read as
       `FARU16(cache_a.seg, cache_b.off + n)` - and every walk steps an
       offset alone, so the pairs are stored rather than dereferenced. */
    struct far_ptr cache_a;       /* +0x00  the three records' pointers */
    struct far_ptr cache_b;       /* +0x04 */
    struct far_ptr cache_c;       /* +0x08  the record's own block */
    uint8_t   pad_5916[2];
    int16_t   lzss_ready;         /* +0x0e  cleared so decompress_lzss builds its tree and fills its ring */
} __attribute__((packed));

#define DG590A (*(volatile struct dg_590a *)(dgroup + 0x590a))

DG_ASSERT_AT(struct dg_590a, cache_a,           0x00);
DG_ASSERT_AT(struct dg_590a, cache_b,           0x04);
DG_ASSERT_AT(struct dg_590a, cache_c,           0x08);
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
    /* **The placed parts: a doubly linked list's head and tail**, the first
       list the level file fills (`n_machine`) and on most levels the largest
       - the scenery: platforms, ramps, pipes, conveyors. 0x5179 holds the
       moving ones. The pair is read the way the bin's at 0x50d7 is; see
       `parts_bin`. */
    struct list_node placed_parts;  /* +0x00  the head cell: next_ptr the first part, prev_ptr the last - cleared with it, never otherwise written */
} __attribute__((packed));

#define DG521B (*(volatile struct dg_521b *)(dgroup + 0x521b))

DG_ASSERT_AT(struct dg_521b, placed_parts,    0x00);

/*
 * ---------------------------------------------------------------------------
 * **Two of Borland's runtime globals**, at DGROUP 0x0094.
 *
 * `errno` is at +0x00, and `io_error` (0x0dcf2) says so in its own comment: it
 * maps a DOS code through the table at 0x4d36 and files the answer here, with
 * `_doserrno` going to 0x4d34. The near heap writes 8 - ENOMEM - into it on
 * both paths where it refuses to come within 0x200 bytes of the stack.
 *
 * `brklvl` is at +0x08, the near heap's break: `brk_set` (0x0c7c8) writes it
 * and `heap_sbrk` (0x0c7e6) moves it and answers where it was, which is the
 * Unix convention and what makes the caller's block start at the answer.
 *
 * The six bytes between them are read by nothing transcribed here, so what
 * they hold is not established.
 *
 * The field is `err_no` rather than `errno` because `errno` is a macro in
 * standard C and a struct member cannot carry that name.
 * ---------------------------------------------------------------------------
 */
struct dg_0094 {
    int16_t   err_no;             /* +0x00  `errno` */
    uint8_t   pad_0096[6];
    uint16_t  brklvl;             /* +0x08  the near heap's break */
} __attribute__((packed));

#define DG0094 (*(volatile struct dg_0094 *)(dgroup + 0x0094))

DG_ASSERT_AT(struct dg_0094, err_no,            0x00);
DG_ASSERT_AT(struct dg_0094, brklvl,            0x08);

/*
 * **The runtime's own file names**, at DGROUP 0xaa: the configuration, the
 * two overlays, the palettes, the font, the cursor and the two panel bitmaps
 * `game_startup` opens, in the order Borland filed them. Typed from the
 * image; the names are ours, from the text. The run ends at the master-level
 * table at 0x116.
 *
 * **A name the game opens stays in DGROUP.** `game_fopen` hands it to
 * `hash_filename`, which uppercases it in place - so after the first open the
 * bytes at 0xf5 read "CP.BMP", in the original and in the port alike, and
 * the verifier compares them. A C string literal is read-only and would
 * fault there. What is only ever read - a mode, "RESOURCE.CFG", which goes
 * to `stdio_fopen` and not through the hash - is a literal at its call site.
 */
struct dg_00aa {
    char resource_cfg[13];   /* +0x00  0x00aa "RESOURCE.CFG" (a literal where it is read) */
    char rb[3];              /* +0x0d  0x00b7 "rb" */
    char vm_ovl[7];          /* +0x10  0x00ba "vm.ovl" */
    char tim_pal[8];         /* +0x17  0x00c1 "tim.pal" */
    char sierra_pal[11];     /* +0x1f  0x00c9 "sierra.pal" */
    char black_pal[10];      /* +0x2a  0x00d4 "black.pal" */
    char memofnt8_fnt[13];   /* +0x34  0x00de "memofnt8.fnt" */
    char mouse_bmp[10];      /* +0x41  0x00eb "mouse.bmp" */
    char cp_bmp[7];          /* +0x4b  0x00f5 "cp.bmp"       game_startup */
    char gp_bord_bmp[12];    /* +0x52  0x00fc "gp_bord.bmp"  game_startup */
    char sx_ovl[7];          /* +0x5e  0x0108 "sx.ovl" */
    char tim_sx[7];          /* +0x65  0x010f "tim.sx"       game_startup */
} __attribute__((packed));

#define DG00AA (*(volatile struct dg_00aa *)(dgroup + 0x00aa))
DG_ASSERT_AT(struct dg_00aa, cp_bmp,            0x4b);
DG_ASSERT_AT(struct dg_00aa, tim_sx,            0x65);
_Static_assert(sizeof(struct dg_00aa) == 0x6c, "the runtime's file names end at the master-level table at 0x116");

/*
 * **The master-level table**, at DGROUP 0x116: a word per master level, 0 to
 * 6, which `game_startup` and the two level-change states hand to
 * `set_master_level_ok`. 0, 3, 5, 8, 10, 13, 15 in the image; seven words,
 * up to the static draw step at 0x124.
 */
struct dg_0116 {
    uint16_t  master_level_ok[7]; /* +0x00 */
} __attribute__((packed));

#define DG0116 (*(volatile struct dg_0116 *)(dgroup + 0x0116))
_Static_assert(sizeof(struct dg_0116) == 14, "the master-level table ends at 0x124");

/*
 * **A draw step**, the record a part's draw list is a chain of: which
 * frames to draw at what offsets, and on which level. `draw_part` walks
 * the chain the kind's `bitmaps2_ptr` names for the form when bit 12 of
 * `flags_08` is set; otherwise it fills in **the one static step at DGROUP
 * 0x124** - the level, the form as its first frame, the kind's hot spot as
 * its first offset - and walks that. The image holds the static step with
 * its `next` 0 and `frame[1]` 0xff, which is how a frame list ends; four
 * frames and four offsets is `draw_part`'s own bound, so the record is
 * fifteen bytes. The struct declared here before, `dg_0126`, was this
 * record based two bytes late, which is why its first field was a "word"
 * one byte wide.
 */
struct draw_step {
    dg_off_t  next;               /* +0x00 */
    uint8_t   level;              /* +0x02  drawn on this level only, unless the part is carried */
    uint8_t   frame[4];           /* +0x03  indices into the kind's bitmap set; 0xff ends the list */
    struct byte_pair offset[4];   /* +0x07  each frame's offset from the part, signed bytes */
} __attribute__((packed));

#define DRAWSTEP_PTR(p) ((volatile struct draw_step *)(dgroup + (uint16_t)(p)))
#define DG0124 (*DRAWSTEP_PTR(0x0124))

DG_ASSERT_AT(struct draw_step, level,  0x02);
DG_ASSERT_AT(struct draw_step, frame,  0x03);
DG_ASSERT_AT(struct draw_step, offset, 0x07);
_Static_assert(sizeof(struct draw_step) == 15, "a draw step: four frames and their offsets");

/*
 * **The game's message texts**, at DGROUP 0x1bcc: the two startup complaints
 * and the goodbye, the copy-protection prompt, every message box's title and
 * body, the picker's and the puzzle screen's labels and buttons, the level-
 * complete texts, and the path separator at the end - the one byte
 * `DG1BCA.path_sep_ptr` points at.
 * Typed from the image, one array per literal in the order Borland filed
 * them; the names are ours, from the text. The run ends at 0x2370.
 *
 * **The bodies are fields and the titles are literals**, and the line
 * between them is a write. A message box's body goes through
 * `draw_wrapped_text`, whose `measure_word` puts a NUL at the end of each
 * word and takes it back - a write into the text, which in the original lands
 * here in DGROUP and which a C string literal, being read-only, would fault
 * on. A title or a button label is only drawn, so it is a literal at its
 * call site and its field here is the layout's record of where it was.
 */
struct dg_1bcc {
    char not_enough_free_memory[26];  /* +0x000 0x1bcc '\n\nNOT ENOUGH FREE MEMORY\n' */
    char you_need_at_least[74];       /* +0x01a 0x1be6 "\nYou need at least 550k of free memory to run 'The Incredible Machine'.\n\n" */
    char unable_to_initialize_vm[25]; /* +0x064 0x1c30 'Unable to initialize vm.' */
    char thanks_for_playing[85];      /* +0x07d 0x1c49 "\n\nThanks for playing 'The Incredible Machine'.\nThe last password given to you was:  " */
    char please_select_in_order[57];  /* +0x0d2 0x1c9e 'Please select, in order, the three parts listed on page ' */
    char of_the_users_manual[23];     /* +0x10b 0x1cd7 " of the user's manual." */
    char version_number[15];          /* +0x122 0x1cee 'VERSION NUMBER' */
    char this_is_version[50];         /* +0x131 0x1cfd "This is version 1.00 of 'The Incredible Machine.'" */
    char memory_low[11];              /* +0x163 0x1d2f 'MEMORY LOW' */
    char memory_is_getting_low[61];   /* +0x16e 0x1d3a 'Memory is getting low.  You can only place a few more parts.' */
    char out_of_memory[14];           /* +0x1ab 0x1d77 'OUT OF MEMORY' */
    char you_cant_place_any[32];      /* +0x1b9 0x1d85 "You can't place any more parts." */
    char quit_game[10];               /* +0x1d9 0x1da5 'QUIT GAME' */
    char quit_body[40];               /* +0x1e3 0x1daf 'Are you sure you want to quit the game?' */
    char restart_level[14];           /* +0x20b 0x1dd7 'RESTART LEVEL' */
    char restart_body[65];            /* +0x219 0x1de5 'Are you sure you want to clear all parts and restart this level?' */
    char freeform_mode[14];           /* +0x25a 0x1e26 'FREEFORM MODE' */
    char freeform_body[46];           /* +0x268 0x1e34 'Are you sure you want to enter freeform mode?' */
    char leave_freeform_mode[20];     /* +0x296 0x1e62 'LEAVE FREEFORM MODE' */
    char leave_freeform_body[46];     /* +0x2aa 0x1e76 'Are you sure you want to leave freeform mode?' */
    char cant_change_gravity[21];     /* +0x2d8 0x1ea4 "CAN'T CHANGE GRAVITY" */
    char gravity_body[73];            /* +0x2ed 0x1eb9 'You are only allowed to change the gravitational force in freeform mode.' */
    char cant_change_air_pressure[26];/* +0x336 0x1f02 "CAN'T CHANGE AIR PRESSURE" */
    char air_pressure_body[66];       /* +0x350 0x1f1c 'You are only allowed to change the air pressure in freeform mode.' */
    char overwrite_file[15];          /* +0x392 0x1f5e 'OVERWRITE FILE' */
    char overwrite_body[51];          /* +0x3a1 0x1f6d 'File already exists.  Do you want to overwrite it?' */
    char file_error[11];              /* +0x3d4 0x1fa0 'FILE ERROR' */
    char cant_open_for_saving[37];    /* +0x3df 0x1fab 'Unable to open that file for saving.' */
    char cant_open_for_loading[38];   /* +0x404 0x1fd0 'Unable to open that file for loading.' */
    char disk_write_protected[89];    /* +0x42a 0x1ff6 'Disk is write protected or there is not enough memory on that disk to save this machine.' */
    char path_error[11];              /* +0x483 0x204f 'PATH ERROR' */
    char path_error_body[28];         /* +0x48e 0x205a 'Unable to choose that path.' */
    char wrong_format[13];            /* +0x4aa 0x2076 'WRONG FORMAT' */
    char wrong_format_body[65];       /* +0x4b7 0x2083 "That file has not been saved in 'The Incredible Machine' format." */
    char need_password[14];           /* +0x4f8 0x20c4 'NEED PASSWORD' */
    char need_password_body[68];      /* +0x506 0x20d2 'You need to enter the correct password in order to try this puzzle.' */
    char bad_password[13];            /* +0x54a 0x2116 'BAD PASSWORD' */
    char bad_password_body[30];       /* +0x557 0x2123 'That is not a valid password.' */
    char score_code_invalid[19];      /* +0x575 0x2141 'SCORE CODE INVALID' */
    char score_code_body[61];         /* +0x588 0x2154 'That score code is invalid.  Your score will be set to zero.' */
    char parent_dir[13];              /* +0x5c5 0x2191 '<PARENT DIR>' */
    char load_machine[13];            /* +0x5d2 0x219e 'LOAD MACHINE' */
    char save_machine[13];            /* +0x5df 0x21ab 'SAVE MACHINE' */
    char load[5];                     /* +0x5ec 0x21b8 'LOAD' */
    char save[5];                     /* +0x5f1 0x21bd 'SAVE' */
    char cancel[7];                   /* +0x5f6 0x21c2 'CANCEL' */
    char file_name[11];               /* +0x5fd 0x21c9 'File Name:' */
    char freeform_mode_title[14];     /* +0x608 0x21d4 'FREEFORM MODE' */
    char puzzle_prefix[8];            /* +0x616 0x21e2 'PUZZLE ' */
    char completed[12];               /* +0x61e 0x21ea ' COMPLETED!' */
    char total_bonus_points[21];      /* +0x62a 0x21f6 'Total bonus points: ' */
    char new_password[13];            /* +0x63f 0x220b 'New Password' */
    char empty[1];                    /* +0x64c 0x2218 '' */
    char click_button_to_continue[27];/* +0x64d 0x2219 '(click button to continue)' */
    char replay_solution[16];         /* +0x668 0x2234 'REPLAY SOLUTION' */
    char replay_body[82];             /* +0x678 0x2244 'Do you want to advance to the next puzzle or replay your solution to this puzzle?' */
    char select_puzzle[14];           /* +0x6ca 0x2296 'SELECT PUZZLE' */
    char password[9];                 /* +0x6d8 0x22a4 'PASSWORD' */
    char solved_all_puzzles[19];      /* +0x6e1 0x22ad 'SOLVED ALL PUZZLES' */
    char freeform_hint[70];           /* +0x6f4 0x22c0 'You can create any type of machine that you wish to in freeform mode.' */
    char solved_all_body[104];        /* +0x73a 0x2306 'Wow!!  INCREDIBLE Job!!!  You have solved all of the puzzles!!  Advance will take you to freeform mode.' */
    char path_sep[2];                 /* +0x7a2 0x236e '\\' */
} __attribute__((packed));

#define DG1BCC (*(volatile struct dg_1bcc *)(dgroup + 0x1bcc))
_Static_assert(sizeof(struct dg_1bcc) == 0x7a4, "DG1BCC ends at 0x2370");

/*
 * **Not established**, at DGROUP 0x1bca.
 */
struct dg_1bca {
    uint16_t  path_sep_ptr;      /* a near pointer to the "\\" at 0x236e, `DG1BCC.path_sep` */          /* +0x00 */
} __attribute__((packed));

#define DG1BCA (*(volatile struct dg_1bca *)(dgroup + 0x1bca))

DG_ASSERT_AT(struct dg_1bca, path_sep_ptr,         0x00);

/*
 * **The intro's file names**, at DGROUP 0x254a - the Sierra screen, the corners,
 * the two animations and the icon set `game_intro` loads.
 * Typed from the image, one array per literal in the order Borland filed
 * them; the names are ours, from the text. The run ends at 0x258c.
 */
struct dg_254a {
    char sierra_bmp[11];              /* +0x000 0x254a 'sierra.bmp' */
    char sierra_scr[11];              /* +0x00b 0x2555 'sierra.scr' */
    char corners_bmp[12];             /* +0x016 0x2560 'corners.bmp' */
    char title_gkc[10];               /* +0x022 0x256c 'title.gkc' */
    char credits_gkc[12];             /* +0x02c 0x2576 'credits.gkc' */
    char icons_bmp[10];               /* +0x038 0x2582 'icons.bmp' */
} __attribute__((packed));

#define DG254A (*(volatile struct dg_254a *)(dgroup + 0x254a))
_Static_assert(sizeof(struct dg_254a) == 0x42, "DG254A ends at 0x258c");

/*
 * **The four quadrants' unit steps**, at DGROUP 0x258c: `dx` is 0, -1, 0, 1
 * and `dy` is -1, 0, 1, 0 in the image, and `find_edge_contact` and its
 * reversed twin index both by the quadrant. Eight words, up to 0x259c.
 */
struct dg_258c {
    int16_t   dx[4];              /* +0x00 */
    int16_t   dy[4];              /* +0x08 */
} __attribute__((packed));

#define DG258C (*(volatile struct dg_258c *)(dgroup + 0x258c))
DG_ASSERT_AT(struct dg_258c, dy,                0x08);
_Static_assert(sizeof(struct dg_258c) == 0x10, "the quadrant steps end at 0x259c");

/*
 * **The copy-protection answers**, at DGROUP 0x24ea: three rows of sixteen
 * part numbers, one row per icon the page asks for, which
 * `copy_protect_screen` compares against the three the player picked. The
 * rows are 0x20 apart in the code that read them, which is the sixteen.
 */
struct dg_24ea {
    int16_t   answer[3][16];      /* +0x00  [icon][page] */
} __attribute__((packed));

#define DG24EA (*(volatile struct dg_24ea *)(dgroup + 0x24ea))
_Static_assert(sizeof(struct dg_24ea) == 0x60, "the answers end at 0x254a");

/*
 * **Not established**, at DGROUP 0x259c.
 */
struct dg_259c {
    uint16_t  word_259c;          /* +0x00  which of the message box's buttons the tab key is on */
    int16_t   stop_x[2];          /* +0x02  their x; the y is always 0xde. 232 and 360 in the image */
} __attribute__((packed));

#define DG259C (*(volatile struct dg_259c *)(dgroup + 0x259c))

DG_ASSERT_AT(struct dg_259c, word_259c,         0x00);

/*
 * **The menu strip's animation tables**, at DGROUP 0x25a2, as
 * `draw_machine_layer_f` reads them: by frame, which of the menu bitmaps to
 * draw and where; and for frames past the fourth, where the four-frame
 * sprite goes. The names are ours; the extents are the routine's bounds
 * and the run ends exactly at 0x25d6.
 */
struct dg_25a2 {
    uint16_t  picture[6];         /* +0x00  a bitmap index in menu_bmp_ptr's set */
    int16_t   picture_x[6];       /* +0x0c */
    int16_t   picture_y[6];       /* +0x18 */
    int16_t   sprite_x[4];        /* +0x24  by the frame modulo four */
    int16_t   sprite_y[4];        /* +0x2c */
} __attribute__((packed));

#define DG25A2 (*(volatile struct dg_25a2 *)(dgroup + 0x25a2))
DG_ASSERT_AT(struct dg_25a2, sprite_x,          0x24);
_Static_assert(sizeof(struct dg_25a2) == 0x34, "the animation tables end at 0x25d6");

/*
 * **Not established**, at DGROUP 0x25d6.
 */
struct dg_25d6 {
    uint16_t  word_25d6;          /* +0x00 */
} __attribute__((packed));

#define DG25D6 (*(volatile struct dg_25d6 *)(dgroup + 0x25d6))

DG_ASSERT_AT(struct dg_25d6, word_25d6,         0x00);

/*
 * **The message box's button labels and the panel's bitmaps**, at DGROUP 0x25d8.
 * Typed from the image, one array per literal in the order Borland filed
 * them; the names are ours, from the text. The run ends at 0x260a.
 */
struct dg_25d8 {
    char continue_btn[9];             /* +0x000 0x25d8 'CONTINUE' */
    char yes[4];                      /* +0x009 0x25e1 'YES' */
    char no[3];                       /* +0x00d 0x25e5 'NO' */
    char score1_bmp[11];              /* +0x010 0x25e8 'score1.bmp' */
    char gp_menu_bmp[12];             /* +0x01b 0x25f3 'gp_menu.bmp' */
    char score2_bmp[11];              /* +0x027 0x25ff 'score2.bmp' */
} __attribute__((packed));

#define DG25D8 (*(volatile struct dg_25d8 *)(dgroup + 0x25d8))
_Static_assert(sizeof(struct dg_25d8) == 0x32, "DG25D8 ends at 0x260a");

/*
 * **Not established**, at DGROUP 0x260a.
 */
struct dg_260a {
    uint16_t  word_260a;          /* +0x00  which of the puzzle screen's five tab stops */
    int16_t   stop_x[5];          /* +0x02  where `puzzle_tab` parks the pointer */
    int16_t   stop_y[5];          /* +0x0c */
} __attribute__((packed));

#define DG260A (*(volatile struct dg_260a *)(dgroup + 0x260a))

DG_ASSERT_AT(struct dg_260a, word_260a,         0x00);

/*
 * **Not established**, at DGROUP 0x2630.
 */
struct dg_2630 {
    uint16_t  word_2630;          /* +0x00 */
    /* **The goal tests, one far pointer per round**, from 0x2632 up to
       0x27ee: `check_goal` calls `[round_number]`. Entry 0 is 0000:0000 in
       the image, and its two words are also the bin-repeat counters game.c
       steps - the same bytes under two names, which is why this is a union
       and not a claim that one of the readings is wrong. */
    union {
        struct far_ptr goal_test[111];    /* +0x02 */
        struct {
            uint16_t  word_2632;  /* +0x02 */
            uint16_t  word_2634;  /* +0x04 */
        };
    };
} __attribute__((packed));

#define DG2630 (*(volatile struct dg_2630 *)(dgroup + 0x2630))

DG_ASSERT_AT(struct dg_2630, word_2630,         0x00);
DG_ASSERT_AT(struct dg_2630, word_2632,         0x02);
DG_ASSERT_AT(struct dg_2630, word_2634,         0x04);
DG_ASSERT_AT(struct dg_2630, goal_test,         0x02);
_Static_assert(sizeof(struct dg_2630) == 0x1be, "the goal tests end at 0x27ee");

/*
 * **Not established**, at DGROUP 0x27ee.
 */
struct dg_27ee {
    uint16_t  word_27ee;          /* +0x00  which of the eleven tab stops on the play screen */
    int16_t   stop_x[9];          /* +0x02  stops 9 and 10 take x from the two knobs instead */
    int16_t   stop_y[11];         /* +0x14  and its eleventh word, at 0x2816, is also the
                                            first of the level table below, which nothing
                                            reads as that */
} __attribute__((packed));

#define DG27EE (*(volatile struct dg_27ee *)(dgroup + 0x27ee))

DG_ASSERT_AT(struct dg_27ee, word_27ee,         0x00);

/*
 * **Where each master level's marker is drawn**, at DGROUP 0x2818: an x per
 * level from 1 to 6, which `paint_panel_e` reads as `0x2816 + 2 * level`.
 * The word before it, at 0x2816, is the last of the tab stops above.
 */
struct dg_2818 {
    int16_t   level_x[6];         /* +0x00  level 1 first */
} __attribute__((packed));

#define DG2818 (*(volatile struct dg_2818 *)(dgroup + 0x2818))

/*
 * **The level screens' string literals**, at DGROUP 0x2824 - Borland files a
 * copy of every literal beside the routine that uses it, which is why "*.TIM"
 * is here twice. Named by their users; the bytes are the image's, and the
 * run ends at the hot spots at 0x284a.
 */
struct dg_2824 {
    char ff_lev[7];               /* +0x00  0x2824  "ff.lev"   screen_state_0400 */
    char tim_filter_load[6];      /* +0x07  0x282b  "*.TIM"    screen_state_0100's pick_file */
    char tim_filter_save[6];      /* +0x0d  0x2831  "*.TIM"    screen_state_0080's */
    char title_sep[3];            /* +0x13  0x2837  ": "       paint_panel_frame */
    char replay[7];               /* +0x16  0x283a  "REPLAY"   finish_level's two buttons */
    char advance[8];              /* +0x1d  0x2841  "ADVANCE" */
    uint8_t pad_2849[1];
} __attribute__((packed));

#define DG2824 (*(volatile struct dg_2824 *)(dgroup + 0x2824))
_Static_assert(sizeof(struct dg_2824) == 0x26, "the level screens' literals end at 0x284a");

/*
 * **The cursors' hot spots**, at DGROUP 0x284a: y for the nine cursors,
 * then x - y before x, the way `set_cursor` takes them. `select_cursor`
 * reads both by cursor number and the run ends at 0x286e.
 */
struct dg_284a {
    int16_t   hot_y[9];           /* +0x00 */
    int16_t   hot_x[9];           /* +0x12 */
} __attribute__((packed));

#define DG284A (*(volatile struct dg_284a *)(dgroup + 0x284a))
DG_ASSERT_AT(struct dg_284a, hot_x,             0x12);
_Static_assert(sizeof(struct dg_284a) == 0x24, "the hot spots end at 0x286e");

/*
 * **Not established**, at DGROUP 0x286e.
 */
struct dg_286e {
    int16_t   word_286e;          /* +0x00 */
} __attribute__((packed));

#define DG286E (*(volatile struct dg_286e *)(dgroup + 0x286e))

DG_ASSERT_AT(struct dg_286e, word_286e,         0x00);

/*
 * **The file names and modes**, at DGROUP 0x2870: one "rb", "wb", "l", ".lev",
 * "password.txt" or "tim.cfg" per call site, in the order the routines that
 * open them sit in the segment. The two at 0x287d and 0x287f have no reader
 * in the port. The run ends at the hash order at 0x28d2.
 *
 * The four names are fields because `game_fopen` uppercases a name in place
 * through `hash_filename` - see `dg_00aa`. The modes and the "l" and ".lev"
 * pieces are only read, and are literals where they are used.
 */
struct dg_2870 {
    char rb_read_level[3];        /* +0x00  0x2870  read_level */
    char wb_write_level[3];       /* +0x03  0x2873  write_level */
    char l_load_level[2];         /* +0x06  0x2876  load_level builds "l<n>.lev" */
    char lev_load_level[5];       /* +0x08  0x2878 */
    char l_287d[2];               /* +0x0d  0x287d  no reader in the port */
    char lev_287f[5];             /* +0x0f  0x287f */
    char rb_is_machine_file[3];   /* +0x14  0x2884  is_machine_file */
    char l_count_levels[2];       /* +0x17  0x2887  count_level_files */
    char lev_count_levels[5];     /* +0x19  0x2889 */
    char rb_count_levels[3];      /* +0x1e  0x288e */
    char l_puzzle_title[2];       /* +0x21  0x2891  get_puzzle_title */
    char lev_puzzle_title[5];     /* +0x23  0x2893 */
    char rb_puzzle_title[3];      /* +0x28  0x2898 */
    char password_txt_level[13];  /* +0x2b  0x289b  password_to_level */
    char rb_password_level[3];    /* +0x38  0x28a8 */
    char password_txt_line[13];   /* +0x3b  0x28ab  read_password_line */
    char rb_password_line[3];     /* +0x48  0x28b8 */
    char tim_cfg_read[8];         /* +0x4b  0x28bb  read_tim_cfg */
    char rb_tim_cfg[3];           /* +0x53  0x28c3 */
    char tim_cfg_write[8];        /* +0x56  0x28c6  sub_12bed, which writes it */
    char wb_tim_cfg[3];           /* +0x5e  0x28ce */
    uint8_t pad_28d1[1];
} __attribute__((packed));

#define DG2870 (*(volatile struct dg_2870 *)(dgroup + 0x2870))
_Static_assert(sizeof(struct dg_2870) == 0x62, "the file names end at the hash order at 0x28d2");

/*
 * **Which four characters of a filename its hash is made of**, at DGROUP
 * 0x28d2: `hash_filename` folds the bytes at these positions of the padded
 * name - 0, 1, 6, 7 in the image - into a long.
 */
struct dg_28d2 {
    uint8_t   hash_order[4];      /* +0x00 */
} __attribute__((packed));

#define DG28D2 (*(volatile struct dg_28d2 *)(dgroup + 0x28d2))

/*
 * **The characters a filename may not contain**, at DGROUP 0x28ec: fourteen
 * of them, `*` `/` `,` `-` `[` `]` `&` `@` `^` `%` `?` `(` `)` `:`, which
 * `validate_filename` tests one by one. The run ends at 0x28fa.
 */
struct dg_28ec {
    uint8_t   forbidden[14];      /* +0x00 */
} __attribute__((packed));

#define DG28EC (*(volatile struct dg_28ec *)(dgroup + 0x28ec))
_Static_assert(sizeof(struct dg_28ec) == 14, "the forbidden characters end at 0x28fa");

/*
 * **Not established**, at DGROUP 0x28fa.
 */
struct dg_28fa {
    uint16_t  word_28fa;          /* +0x00  which of the picker's seven tab stops */
    int16_t   stop_x[7];          /* +0x02  where `picker_tab` parks the pointer */
    int16_t   stop_y[7];          /* +0x10 */
} __attribute__((packed));

#define DG28FA (*(volatile struct dg_28fa *)(dgroup + 0x28fa))

DG_ASSERT_AT(struct dg_28fa, word_28fa,         0x00);

/*
 * **The name the last `findfirst`/`findnext` answered**, at DGROUP 0x2d4a:
 * thirteen bytes `dos_find_to_dgroup` copies out of the DTA and
 * `dos_find_name` answers. The word before it and the 0x1f bytes after, up to
 * DG2D76, are not established.
 */
struct dg_2d48 {
    uint16_t  word_2d48;          /* +0x00 */
    char      find_name[13];      /* +0x02  0x2d4a */
    uint8_t   unread_2d57[0x1f];  /* +0x0f */
} __attribute__((packed));

#define DG2D48 (*(volatile struct dg_2d48 *)(dgroup + 0x2d48))
DG_ASSERT_AT(struct dg_2d48, find_name,         0x02);
_Static_assert(sizeof(struct dg_2d48) == 0x2e, "the find name's run ends at DG2D76");

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
    /* **Ten slots of four bytes**, of which nine are searched: `load_palette`
       walks `di` from 1 and stops at `di >= 0xa`, then files into slot `di`,
       so index 9 is written; `free_far_block` walks `i < 10`; and slot 0 is
       `set_palette_pointer`'s, which the driver reaches on its own as
       driverDS:0x1a0 - the segment half alone, which is why each slot is a
       pair rather than a pointer.

       It was declared `[9]` because the note above said "nine slots", which
       is what the *search* covers. Sized from the prose rather than from the
       loop, the array was four bytes short of the slot `di == 9` writes. */
    struct far_ptr blocks[10];    /* +0x02 */
} __attribute__((packed));

#define DG3A2C (*(volatile struct dg_3a2c *)(dgroup + 0x3a2c))

DG_ASSERT_AT(struct dg_3a2c, clip_count,        0x00);
DG_ASSERT_AT(struct dg_3a2c, blocks,            0x02);

/*
 * **Not established**, at DGROUP 0x4342.
 */
struct dg_4342 {
    uint16_t  word_4342;          /* +0x00 */
    int16_t   word_4344;          /* +0x02 */
    /* **Fifty far pointers into the video driver**, at 0x4346: `vm_init`
       copies a hundred words of the driver's own table from its +0x13e and
       then writes the driver's segment over every second one, which is what
       fifty `far_ptr`s filled word by word looks like. Up to 0x440e. */
    struct far_ptr font[50];      /* +0x04 */
} __attribute__((packed));

#define DG4342 (*(volatile struct dg_4342 *)(dgroup + 0x4342))

DG_ASSERT_AT(struct dg_4342, word_4342,         0x00);
DG_ASSERT_AT(struct dg_4342, word_4344,         0x02);
DG_ASSERT_AT(struct dg_4342, font,              0x04);
_Static_assert(sizeof(struct dg_4342) == 0xcc, "the driver pointers end at 0x440e");

/*
 * **The picker's string literals**, at DGROUP 0x2918: the ".TIM" extension it
 * forces, the eleven reserved DOS device names `validate_filename` refuses,
 * and the wildcards and dot entries its directory walk uses - two copies of
 * "*.*" and of "..", one per call. Named by their users; the run ends at
 * 0x2967. `reserved_names` in game.c needs these as offsets in a static
 * initialiser, which is what DG2918_OFF is for.
 */
struct dg_2918 {
    char tim_ext[4];              /* +0x00  0x2918  "TIM"   pick_file's force_extension */
    char con[4];                  /* +0x04  0x291c */
    char aux[4];                  /* +0x08  0x2920 */
    char com1[5];                 /* +0x0c  0x2924 */
    char com2[5];                 /* +0x11  0x2929 */
    char com3[5];                 /* +0x16  0x292e */
    char com4[5];                 /* +0x1b  0x2933 */
    char prn[4];                  /* +0x20  0x2938 */
    char lpt1[5];                 /* +0x24  0x293c */
    char lpt2[5];                 /* +0x29  0x2941 */
    char nul[4];                  /* +0x2e  0x2946 */
    char null[5];                 /* +0x32  0x294a */
    char rb_validate[3];          /* +0x37  0x294f  validate_filename's open */
    char star_name[2];            /* +0x3a  0x2952  picker_draw_name */
    char star_filename[2];        /* +0x3c  0x2954  picker_draw_filename */
    char all_files_first[4];      /* +0x3e  0x2956  sub_13a8a's dos_findfirst */
    char dot[2];                  /* +0x42  0x295a */
    char dotdot[3];               /* +0x44  0x295c */
    char all_files_next[4];       /* +0x47  0x295f  its dos_findnext */
    char dotdot_2963[3];          /* +0x4b  0x2963  what listing_to_name answers for the parent entry */
    uint8_t pad_2966[1];
} __attribute__((packed));

#define DG2918 (*(volatile struct dg_2918 *)(dgroup + 0x2918))
#define DG2918_OFF(field) ((uint16_t)(0x2918 + __builtin_offsetof(struct dg_2918, field)))
_Static_assert(sizeof(struct dg_2918) == 0x4f, "the picker's literals end at 0x2967");

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
    /* **The keyboard's tables**, as `keyboard_isr` reads them. The extents
       are the ISR's own bounds - it drops any scancode at or above 0x59
       before touching a table, and walks the PCjr remap eleven wide - and
       the record ends where `DG471B` begins. The two pads are bytes nothing
       in the port reads. What `held` holds is a reading: the ISR files the
       scancode's upper bits there on a press and clears the slot on the
       matching release. */
    uint8_t   held[2];            /* +0x04  0x4590 */
    uint8_t   pad_4592[0x48];
    uint8_t   ascii[0x59];        /* +0x4e  0x45da  scancode to character */
    uint8_t   shifted[0x59];      /* +0xa7  0x4633  the same with shift down */
    uint8_t   state[0x59];        /* +0x100 0x468c  a bit per key: down */
    uint8_t   pad_46e5[0x20];
    uint8_t   pcjr_from[0x0b];    /* +0x179 0x4705  the PCjr's scancodes ... */
    uint8_t   pcjr_to[0x0b];      /* +0x184 0x4710  ... and what they stand for */
} __attribute__((packed));

#define DG458C (*(volatile struct dg_458c *)(dgroup + 0x458c))

/*
 * **The stride shift table**, at DGROUP 0x457a: `blit_scaled_b` shifts a
 * bitmap's width by the entry the driver's pixel shift selects - read as
 * `mov al, [bx+0x457a]` with a sign-extended byte in `bx`, so the index can
 * be negative and the table is only known to start here. Fourteen bytes,
 * then four the port never reads, up to DG458C.
 */
struct dg_457a {
    uint8_t   stride_shift[14];   /* +0x00  ff 02 03 01 ff 00 ff 00 00 03 01 03 03 03 */
    uint8_t   bytes_4588[4];      /* +0x0e */
} __attribute__((packed));

#define DG457A (*(volatile struct dg_457a *)(dgroup + 0x457a))
_Static_assert(sizeof(struct dg_457a) == 0x12, "the stride shifts end at DG458C");

DG_ASSERT_AT(struct dg_458c, word_458c,         0x00);
DG_ASSERT_AT(struct dg_458c, byte_458d,         0x01);
DG_ASSERT_AT(struct dg_458c, word_458e,         0x02);
DG_ASSERT_AT(struct dg_458c, held,              0x04);
DG_ASSERT_AT(struct dg_458c, ascii,             0x4e);
DG_ASSERT_AT(struct dg_458c, shifted,           0xa7);
DG_ASSERT_AT(struct dg_458c, state,             0x100);
DG_ASSERT_AT(struct dg_458c, pcjr_from,         0x179);
_Static_assert(sizeof(struct dg_458c) == 0x18f, "the keyboard record ends at 0x471b");

/*
 * **The text colour map**, at DGROUP 0x471e: `draw_char` maps a glyph
 * pixel value below five through it. The image holds 0, 1, 2, 3, 4.
 */
struct dg_471e {
    uint8_t   colour[5];          /* +0x00 */
} __attribute__((packed));

#define DG471E (*(volatile struct dg_471e *)(dgroup + 0x471e))

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
    /* **One far pointer**: the block `load_video_driver` reads the adapter's
       driver into. +0x00 is the offset and +0x02 the segment - every use
       pairs them, as `huge_equal(off, seg, 0, 0)` against null, as the
       destination of `read_resource`, and as the `(seg << 16) | off` the
       routine answers. */
    struct far_ptr block;         /* +0x00 */
} __attribute__((packed));

#define DG48F8 (*(volatile struct dg_48f8 *)(dgroup + 0x48f8))

DG_ASSERT_AT(struct dg_48f8, block,             0x00);

/*
 * **Borland's `_ctype` table**, at DGROUP 0x4ab7: a class byte per character,
 * 0x101 of them, up to DG4BB8. `to_lower` tests bit 2, upper case, and is the
 * one reader in the port.
 */
struct dg_4ab7 {
    uint8_t   ctype[0x101];       /* +0x00 */
} __attribute__((packed));

#define DG4AB7 (*(volatile struct dg_4ab7 *)(dgroup + 0x4ab7))
_Static_assert(sizeof(struct dg_4ab7) == 0x101, "the ctype table ends at DG4BB8");

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
    /* Borland's `_dosErrorToSV`: the errno for each DOS error code, 0x59
       entries, -1 where there is none. `io_error` clamps a code to 0x58 and
       reads through here. The string "TMP" follows at 0x4d90. */
    int8_t    errno_map[0x59];    /* +0x08  0x4d36 */
} __attribute__((packed));

#define DG4D2E (*(volatile struct dg_4d2e *)(dgroup + 0x4d2e))
DG_ASSERT_AT(struct dg_4d2e, errno_map,         0x08);
_Static_assert(sizeof(struct dg_4d2e) == 0x61, "the errno map ends before the TMP string at 0x4d90");

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
    struct far_ptr crit_vec;      /* +0x00  DOS's 24h, kept so it can be
                                            put back */
    uint16_t  failures;           /* +0x04 **or-ed, not set**: this layer accumulates its failures here */
    uint8_t   pad_567d[1];
    uint16_t  caret_blink;        /* +0x07  bumped on every pass; the caret is `*` */
    uint16_t  caret_blink_b;      /* +0x09  a different counter, and a different asterisk at 0x2954 */
} __attribute__((packed));

#define DG5677 (*(volatile struct dg_5677 *)(dgroup + 0x5677))

DG_ASSERT_AT(struct dg_5677, crit_vec,      0x00);
DG_ASSERT_AT(struct dg_5677, failures,          0x04);
DG_ASSERT_AT(struct dg_5677, caret_blink,       0x07);

/*
 * **The shared name buffer**, at DGROUP 0x5682. `listing_to_name` strips a
 * listing record's `<`, `>` and spaces into it and answers its address, so the
 * caller has a near string it can hand to `strcpy`.
 *
 * Thirteen bytes, which is what a DOS 8.3 name and its NUL take - and what is
 * left between `dg_5677`, which ends at 0x5682, and `dg_568f`.
 */
struct dg_5682 {
    char      name[0xd];          /* +0x00 */
} __attribute__((packed));

#define DG5682 (*(volatile struct dg_5682 *)(dgroup + 0x5682))

DG_ASSERT_AT(struct dg_5682, name,              0x00);
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

#define PAGESLOT_PTR(p) ((volatile struct page_slot *)(dgroup + (uint16_t)(p)))

/*
 * **The two page slots**, at DGROUP 0x56e6.
 *
 * `claim_page_slot` walks two of them at a stride of 0x20, which is
 * `sizeof(struct page_slot)`, and matches on the top bits of the record's
 * first field - the page it belongs to. It answers the slot's own offset, so
 * the callers keep taking a `PAGESLOT`.
 */
struct dg_56e6 {
    struct page_slot slots[2];   /* +0x00 */
} __attribute__((packed));

#define DG56E6 (*(volatile struct dg_56e6 *)(dgroup + 0x56e6))

DG_ASSERT_AT(struct dg_56e6, slots,             0x00);

/*
 * **Not established**, at DGROUP 0x56e0.
 */
struct dg_56e0 {
    /* The free list of `rect_list_entry` records. Only ever appended to -
       here **and in the original**: the builder that fills it, 0x0a05f, is
       reached only from the creator at 0x0a0d7, and nothing in the image
       calls that. See `struct rect_list_entry`. */
    dg_off_t  rect_free_ptr;      /* +0x00 */
    int16_t   word_56e2;          /* +0x02 */
    int16_t   word_56e4;          /* +0x04 */
    struct page_slot slot[2];     /* +0x06  DGROUP 0x56e6 and 0x5706 */
} __attribute__((packed));

#define DG56E0 (*(volatile struct dg_56e0 *)(dgroup + 0x56e0))

DG_ASSERT_AT(struct dg_56e0, rect_free_ptr,     0x00);
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
 * **The four object buffers `claim_buffer_slot` hands out**: a taken flag
 * apiece at 0x5734; the buffers themselves are `RECT_BUFFER[1..4]`, the far
 * pointers at 0x5758 up to `DG5768`. Four is the routine's own bound.
 */
struct dg_5734 {
    uint8_t   used[4];            /* +0x00 */
} __attribute__((packed));

#define DG5734 (*(volatile struct dg_5734 *)(dgroup + 0x5734))

/*
 * **The palette request and the fade**, at DGROUP 0x5738.
 */
struct dg_5738 {
    struct far_ptr request;       /* +0x00  cleared when taken, so one
                                            request loads once */
    uint16_t  fade_mark;          /* +0x04  reset to zero by a load, which forces the fade to run; */
    int16_t   word_573e;          /* +0x06  the fade runs only while it differs from 0x5786 */
    int16_t   busy;               /* +0x08  non-zero suppresses the slot release, and everything waits on it */
} __attribute__((packed));

#define DG5738 (*(volatile struct dg_5738 *)(dgroup + 0x5738))

DG_ASSERT_AT(struct dg_5738, request,       0x00);
DG_ASSERT_AT(struct dg_5738, fade_mark,         0x04);
DG_ASSERT_AT(struct dg_5738, word_573e,         0x06);
DG_ASSERT_AT(struct dg_5738, busy,              0x08);

/*
 * **The two buttons' state machines**, at DGROUP 0x5742 - eight bytes each,
 * and `reset_input_state` clears both as two blocks of four words. Sixteen
 * bytes end at 0x5752, which is the guard the clear holds across itself.
 *
 * `button_state` is the whole of what reads them; the field names are its
 * comment.
 */
struct button {
    int16_t   state;              /* +0x00  0 up, 2 pressed, 4 clicked, 8 held */
    int16_t   was_down;           /* +0x02  what the driver said last time */
    int16_t   presses;            /* +0x04  what tells a click from a double one */
    int16_t   delay;              /* +0x06  reloaded from 0x2d40 on every change */
} __attribute__((packed));

struct dg_5742 {
    struct button button[2];      /* +0x00 */
} __attribute__((packed));

#define DG5742 (*(volatile struct dg_5742 *)(dgroup + 0x5742))

DG_ASSERT_AT(struct button, state,              0x00);
DG_ASSERT_AT(struct button, was_down,           0x02);
DG_ASSERT_AT(struct button, presses,            0x04);
DG_ASSERT_AT(struct button, delay,              0x06);
DG_ASSERT_AT(struct dg_5742, button,            0x00);

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
 * ---------------------------------------------------------------------------
 * **A fifth font table**, at DGROUP 0x627a, one byte per slot.
 *
 * `load_font` reads a compressed font's header as single bytes into parallel
 * arrays indexed by the slot - 0x38c4, 0x38d8, 0x38ec and 0x3900, which are
 * `DG3890.font_table_34` and its three neighbours, and this one. Those four
 * are `uint8_t[0x14]`, and `DG628E` starts at 0x628e, so this is twenty slots
 * as well.
 *
 * What it holds is the row the underline is drawn on: `draw_char` tests
 * `DG3890.unknown_02 & 8` and then this against the row it is about to draw,
 * blanking that pixel. The name is a **reading** of that one use.
 *
 * Element 0 doubles as the current font's value - `select_font` copies the
 * chosen slot's byte down into it - which is what the two bare reads are.
 * ---------------------------------------------------------------------------
 */
struct dg_627a {
    uint8_t   underline_row[0x14];   /* +0x00  one per font slot */
} __attribute__((packed));

#define DG627A (*(volatile struct dg_627a *)(dgroup + 0x627a))

DG_ASSERT_AT(struct dg_627a, underline_row,     0x00);

/*
 * ---------------------------------------------------------------------------
 * **The two directories the game holds on to**, at DGROUP 0x530b.
 *
 * Both are filled at startup by `dos_get_cur_dir`, which writes a drive letter,
 * a colon and a backslash before the path - so byte 0 of each is the drive, and
 * `dos_setdisk(DG8(...))` is handing over that letter.
 *
 * `screen_state_0100` is where the pair earns its keep: it changes to
 * `picker_dir`, lets `pick_file` wander wherever the player likes, saves where
 * the picker ended up back into `picker_dir`, and then changes to `game_dir` to
 * put the process back. So the picker remembers its own place and the game
 * keeps its own.
 *
 * Three of them, eighty bytes each: 0x530b, 0x535b and 0x53ab. The third is
 * the one the picker actually navigates - `path_join` and `path_up` walk it,
 * `path_is_root` tests it, `dos_chdir` follows it and `picker_type` types into
 * it with a width of 0x50, which is where that size is stated outright.
 * `picker_draw_name`'s comment calls it the name field.
 *
 * An earlier version of this comment said 0x53ab was "the next object" after
 * the two. It is the third member of the same run.
 * ---------------------------------------------------------------------------
 */
struct dg_530b {
    char      picker_dir[0x50];   /* +0x00  DGROUP 0x530b */
    char      game_dir[0x50];     /* +0x50  DGROUP 0x535b */
    char      path_field[0x50];   /* +0xa0  DGROUP 0x53ab */
} __attribute__((packed));

#define DG530B (*(volatile struct dg_530b *)(dgroup + 0x530b))

DG_ASSERT_AT(struct dg_530b, picker_dir,        0x00);
DG_ASSERT_AT(struct dg_530b, game_dir,          0x50);
DG_ASSERT_AT(struct dg_530b, path_field,        0xa0);

/*
 * **The driver's page hook**, at DGROUP 0x3f72.
 *
 * Non-zero makes the three blitters call the vector at DGROUP 0x43b6 between
 * taking the destination page and reading the clip. That vector is the
 * driver's do-nothing stub, so the page comes back as it went in - and the
 * port keeps the guard so a build whose 0x3f72 is *set* is not silently the
 * same as one whose is clear. The name is a reading of that one use.
 */
struct dg_3f72 {
    int16_t   page_hook;          /* +0x00 */
} __attribute__((packed));

#define DG3F72 (*(volatile struct dg_3f72 *)(dgroup + 0x3f72))

DG_ASSERT_AT(struct dg_3f72, page_hook,         0x00);

/*
 * **The PCjr keyboard flag**, at DGROUP 0x471b.
 *
 * `install_keyboard` clears it and then calls `detect_pcjr`; the path that
 * would set it is 0x210f3, which is a stub here. Both readers are PCjr
 * keyboard quirks - one remaps scancode 0x29 to 0x48, the other treats Caps
 * and Num as keys that never report a release.
 */
struct dg_471b {
    uint8_t   pcjr_keyboard;      /* +0x00 */
} __attribute__((packed));

#define DG471B (*(volatile struct dg_471b *)(dgroup + 0x471b))

DG_ASSERT_AT(struct dg_471b, pcjr_keyboard,     0x00);

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
    uint16_t  word_08;            /* +0x08 */
    uint16_t  word_0a;            /* +0x0a */
    uint16_t  reset_off;          /* +0x0c  the restart */
} __attribute__((packed));

struct dg_357a {
    struct res_handler type[4];   /* +0x00 */
} __attribute__((packed));

#define DG357A (*(volatile struct dg_357a *)(dgroup + 0x357a))
DG_ASSERT_AT(struct res_handler, read_off,  0x06);
DG_ASSERT_AT(struct res_handler, reset_off, 0x0c);
_Static_assert(sizeof(struct dg_357a) == 0x38, "four handlers end at 0x35b2");

/*
 * **The LZW mask table**, at DGROUP 0x35c8: 0, 1, 3, 7, ..., 0xff, indexed
 * by how many bits are still wanted. Nine bytes, up to 0x35d1.
 */
struct dg_35c8 {
    uint8_t   mask[9];            /* +0x00 */
} __attribute__((packed));

#define DG35C8 (*(volatile struct dg_35c8 *)(dgroup + 0x35c8))

/*
 * **The bit reader's input window**, at DGROUP 0x35bc: `next_lzw_code` has
 * `read_input_block` fill it and takes its codes out of it a byte at a time,
 * from the bit position DG5888 keeps. Twelve bytes, up to the mask table.
 */
struct dg_35bc {
    uint8_t   window[12];         /* +0x00 */
} __attribute__((packed));

#define DG35BC (*(volatile struct dg_35bc *)(dgroup + 0x35bc))
_Static_assert(sizeof(struct dg_35bc) == 0x0c, "the input window ends at DG35C8");
_Static_assert(sizeof(struct dg_35c8) == 9, "the mask table ends at 0x35d1");

/*
 * **Where the LZW string had got to**, at DGROUP 0x35d1.
 *
 * `decompress_lzw` copies a decoded string out of its scratch buffer
 * backwards, and a request that fills mid-string has to resume there next
 * time. This is that position, as an offset into the scratch buffer, saved
 * beside the byte at DGROUP 0x58a2 that says a resume is pending.
 */
struct dg_35d1 {
    int16_t   scratch_at;         /* +0x00 */
} __attribute__((packed));

#define DG35D1 (*(volatile struct dg_35d1 *)(dgroup + 0x35d1))

DG_ASSERT_AT(struct dg_35d1, scratch_at,        0x00);

/*
 * **The sound module's name template**, at DGROUP 0x4a08.
 *
 * `load_sound_module` builds the name in place: the eight characters
 * `SSM:000:` with the three digits overwritten from the number it was given -
 * hundreds, tens and units, each from its own division. Those digits are bytes
 * 4, 5 and 6, which is what the three raw accessors at 0x4a0c..0x4a0e were.
 */
struct dg_4a08 {
    char      module_name[9];     /* +0x00  "SSM:000:" and its terminator */
} __attribute__((packed));

#define DG4A08 (*(volatile struct dg_4a08 *)(dgroup + 0x4a08))

DG_ASSERT_AT(struct dg_4a08, module_name,       0x00);

/*
 * **The interrupt's own stack**, at DGROUP 0x317e.
 *
 * `isr_stack_switch` files `SS:SP` here on the way in so the handler can run
 * on a private stack and put the interrupted one back on the way out. The port
 * does not switch stacks - it has no single SP to switch - but it writes both
 * words, because anything else is free to read them.
 */
struct dg_317e {
    uint16_t  saved_ss;           /* +0x00 */
    uint16_t  saved_sp;           /* +0x02 */
} __attribute__((packed));

#define DG317E (*(volatile struct dg_317e *)(dgroup + 0x317e))

DG_ASSERT_AT(struct dg_317e, saved_ss,          0x00);
DG_ASSERT_AT(struct dg_317e, saved_sp,          0x02);

/*
 * **Which chunk a font lives in**, at DGROUP 0x495c.
 *
 * `load_font` hands it to `seek_named_chunk`, which takes the DGROUP offset of
 * an eight-character name - so this word holds that offset rather than the
 * name. Nothing in the port writes it: the value comes in with the image.
 */
struct dg_495c {
    dg_off_t  font_chunk_name;    /* +0x00  offset of the name to seek */
} __attribute__((packed));

#define DG495C (*(volatile struct dg_495c *)(dgroup + 0x495c))

DG_ASSERT_AT(struct dg_495c, font_chunk_name,   0x00);

/*
 * **The shortest run worth encoding**, at DGROUP 0x49ba.
 *
 * `compress_row` counts a run of equal bytes and emits it as a run only when
 * it reaches this; anything shorter goes out as literals. Nothing in the port
 * writes it either - it comes in with the image.
 */
struct dg_49ba {
    int16_t   min_run;            /* +0x00 */
} __attribute__((packed));

#define DG49BA (*(volatile struct dg_49ba *)(dgroup + 0x49ba))

DG_ASSERT_AT(struct dg_49ba, min_run,           0x00);

/*
 * **Each font slot's kind**, at DGROUP 0x6176, one byte per slot for the
 * twenty slots `FONTSLOT` holds: `load_font` writes 0 for a plain bitmap
 * font, 2 for the 0xfe header, and the negated header byte for 0xfd and
 * 0xff. Slot 0 is the *selected* font's copy - `set_font` writes
 * `kind[slot]` into it the way it copies `font_table_34[slot]` into
 * `font_table_34[0]` - and the drawing routines test bit 0 of that.
 */
struct dg_6176 {
    uint8_t   kind[0x14];         /* +0x00 */
} __attribute__((packed));

#define DG6176 (*(volatile struct dg_6176 *)(dgroup + 0x6176))

DG_ASSERT_AT(struct dg_6176, kind,              0x00);
_Static_assert(sizeof(struct dg_6176) == 0x14, "twenty font slots, up to FONTSLOT at 0x618a");

/*
 * **The font table**, at DGROUP 0x618a.
 *
 * Both pairs are set from the same place - `vm_init` files the BIOS's answer
 * to INT 10h AX=1130h into each - and only the first is written again
 * afterwards, when a font is loaded into DGROUP. So the second keeps whatever
 * the BIOS said, which is what its name records.
 */
struct dg_618a {
    struct far_ptr fonts;         /* +0x00  a font's body goes here with
                                            DGROUP as its segment */
    struct far_ptr bios_fonts;    /* +0x04  the BIOS font pointer as INT 10h
                                            AX=1130h answered it */
} __attribute__((packed));

#define DG618A (*(volatile struct dg_618a *)(dgroup + 0x618a))

DG_ASSERT_AT(struct dg_618a, fonts,             0x00);
DG_ASSERT_AT(struct dg_618a, bios_fonts,        0x04);

/* **The eighteen font slots and their width tables**, which is what the two
   fields above are the first of: `load_font` writes `0x618a + 4 * slot` and
   `0x61da + 4 * slot`, so `DG618A.fonts` and `FONTSLOT[0]` are one object
   under two names, and `DG61DA.widths` and `WIDTHSLOT[0]` likewise. */
#define FONTSLOT  ((volatile struct far_ptr *)(dgroup + 0x618a))
#define WIDTHSLOT ((volatile struct far_ptr *)(dgroup + 0x61da))

/*
 * **The font's width table**, at DGROUP 0x61da.
 */
struct dg_61da {
    struct far_ptr widths;        /* +0x00  `les bx,[0x61da]` loads the
                                            segment too, so the width is a
                                            far read */
} __attribute__((packed));

#define DG61DA (*(volatile struct dg_61da *)(dgroup + 0x61da))

DG_ASSERT_AT(struct dg_61da, widths,            0x00);

/*
 * **The third font slot table**, at DGROUP 0x622a - the one between the
 * widths at 0x61da and the bodies at 0x618a. `load_font_data` files three far
 * pointers into one block per font: the widths at its base, this one two
 * bytes per glyph on, and the body one byte per glyph after that. `load_font`
 * reads all three the same way, `0x622a + 4 * slot`.
 *
 * What the middle table *holds* is still not established; that it is a slot
 * table of far pointers is.
 */
struct dg_622a {
    struct far_ptr slot;          /* +0x00 */
} __attribute__((packed));

#define DG622A (*(volatile struct dg_622a *)(dgroup + 0x622a))
#define MIDSLOT ((volatile struct far_ptr *)(dgroup + 0x622a))

DG_ASSERT_AT(struct dg_622a, slot,              0x00);

/*
 * **Not established**, at DGROUP 0x6400.
 */
/*
 * ---------------------------------------------------------------------------
 * **`bitmaps.c`'s own state**, at DGROUP 0x6400 - and it is named for the
 * module rather than the address because that is what it is: every one of its
 * fourteen uses is in that one translation unit, which in the original means
 * this run of DGROUP *is* that unit's statics. DGROUP is one segment shared by
 * the whole program, but each unit's own data sits in a contiguous piece of it.
 *
 * **The eight bytes at +0x02 are the singleton bit reader**, and
 * `open_bit_reader` answers `0x6402` - their address - rather than a handle.
 * They are a `vqt_reader` **truncated after `data`**: no plane table at +0x08
 * and no row table at +0x18. That is not an oversight in the transcription, it
 * is the defect the quadtree format has - `load_screen_vqt` puts this reader in
 * `reader` for `vqt_node` to walk, and the leaf then reads a plane table that
 * is not there. `VQT` occurs zero times in the four shipped archives, which is
 * the measured half of why nobody noticed.
 *
 * Field names are ours; the offsets are the original's.
 * ---------------------------------------------------------------------------
 */
typedef struct {
    uint16_t       in_use;        /* +0x00  a second open answers 0 */
    uint32_t       pos;           /* +0x02  the singleton's bit position,
                                     stepped four bits at a time. One
                                     Borland `long`: `vqt_node` loads it
                                     `mov ax,[bx] / mov dx,[bx+2]` and steps
                                     it `add cx,4 / adc cx,0`. */
    struct far_ptr data;          /* +0x06  and the block it reads */
    uint8_t        pad_640a[2];
    dg_off_t       reader;        /* +0x0c  which reader the vqt walk uses -
                                     the singleton above, or the frame
                                     `decode_vqt_list` files here */
} __attribute__((packed)) bitmaps_t;

#define BITMAPS (*(volatile bitmaps_t *)(dgroup + 0x6400))

DG_ASSERT_AT(bitmaps_t, in_use,                 0x00);
DG_ASSERT_AT(bitmaps_t, pos,                    0x02);
DG_ASSERT_AT(bitmaps_t, data,                   0x06);
DG_ASSERT_AT(bitmaps_t, reader,                 0x0c);

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
#define S1C8(off)   (*(uint8_t *)(guest_mem + S1C25 + (off)))
#define S1C16(off)  (*(int16_t *)(guest_mem + S1C25 + (off)))

#define SNDCS       (IMAGE_BASE + 0x26190)

#define SND8(off)   (*(uint8_t *)(guest_mem + SNDCS + (off)))
#define SND16(off)  (*(int16_t *)(guest_mem + SNDCS + (off)))

#define SX_SEG      (*(uint16_t *)(guest_mem + SNDCS + 0x1e9))
#define SX8(off)    (*(uint8_t *)MK_FP(SX_SEG, (off)))
#define SX16(off)   (*(int16_t *)MK_FP(SX_SEG, (off)))

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
#define ASB_SEG     DG4A82.module.seg
#define ASB_OFF     DG4A82.module.off
#define ASB8(off)   (*(uint8_t *)MK_FP(ASB_SEG, ASB_OFF + (off)))
#define ASB16(off)  (*(int16_t *)MK_FP(ASB_SEG, ASB_OFF + (off)))
#define ASBU16(off) (*(uint16_t *)MK_FP(ASB_SEG, ASB_OFF + (off)))

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
 * So the port carries a stack pointer of its own. `dg_alloca` reserves bytes
 * below it and answers the offset of the low end; `dg_free` gives them back.
 * They were `dg_enter`/`dg_leave` until 2026-09-10, which read as though a
 * routine were being entered rather than as what they are: `alloca` in the
 * guest's stack instead of the host's.
 *
 * **Where these bytes land is not matched against the original, and is not
 * meant to be.** Only the global DGROUP is compared. `tools/verify.py` seeds
 * `guest_sp` from the original's SP at the routine's entry, which it does at
 * the same moment it seeds memory - so a frame the routine reserves for itself
 * happens to fall where the original's `sub sp` put it, and inside the range
 * the comparison skips. That is a convenience of the one routine under test,
 * not a property of the model: the port stopped accounting for the words a
 * *call* pushes when `dg_call`/`dg_uncall` went, so a frame reserved deeper
 * down sits higher than the original's, and nothing depends on it not to.
 * What the model does require is that nothing is read back from a frame after
 * `dg_free`.
 */
extern uint16_t guest_sp;

uint16_t dg_alloca(uint16_t bytes);
void     dg_free(uint16_t bytes);

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
    struct far_ptr driver;        /* +0x01e7  the cell every call far-calls
                                              through, with the function
                                              number in BP */
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
    struct far_ptr callback;      /* +0x30f6  the cell sound_callback calls
                                              through */
    int16_t   answer;             /* +0x30fa  parked before the registers are popped and read back */
} __attribute__((packed));

#define SNDS (*(volatile struct snd_cs *)(guest_mem + SNDCS))

_Static_assert(__builtin_offsetof(struct snd_cs, word_000a) == 0x000a, "snd_cs.word_000a");
_Static_assert(__builtin_offsetof(struct snd_cs, poll_table) == 0x0048, "snd_cs.poll_table");
_Static_assert(__builtin_offsetof(struct snd_cs, word_004a) == 0x004a, "snd_cs.word_004a");
_Static_assert(__builtin_offsetof(struct snd_cs, driver) == 0x01e7, "snd_cs.driver");
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
_Static_assert(__builtin_offsetof(struct snd_cs, callback) == 0x30f6, "snd_cs.callback");
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

#define ASBS (*(volatile struct asb_cs *)MK_FP(ASB_SEG, ASB_OFF))

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
    struct far_ptr old_int8;      /* +0x446d  the INT 08h vector
                                              timer_install displaced */
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

_Static_assert(__builtin_offsetof(struct s1c_cs, old_int8) == 0x446d, "s1c_cs.old_int8");
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

#define SXSPKR (*(volatile struct sx_spkr *)MK_FP(SX_SEG, 0))

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

#define SXADL (*(volatile struct sx_adl *)MK_FP(SX_SEG, 0))

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

#define SXSBP (*(volatile struct sx_sbp *)MK_FP(SX_SEG, 0))

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
  union {
    struct {
    dg_off_t  link_ptr;        /* +0x00  the next record on this list */
    uint16_t  mask;            /* +0x02  and-ed with the state word at 0x4e6b */
    uint16_t  word_04;         /* +0x04 */
    int16_t   x0;              /* +0x06  the rectangle, inclusive at both ends */
    int16_t   y0;              /* +0x08 */
    int16_t   x1;              /* +0x0a */
    int16_t   y1;              /* +0x0c */
    uint16_t  cursor;          /* +0x0e  which cursor while the pointer is in it */
    uint16_t  code;            /* +0x10  written into the state word on a click */
    /* Two far *code* pointers, and both are tested `(off | seg) != 0` -
       which is `far_eq(h, FAR_NULL)` and not a null-pointer test. */
    struct far_ptr hover;      /* +0x12  called whenever the pointer is
                                         inside */
    struct far_ptr click;      /* +0x16  and this one on the click itself */
    } __attribute__((packed));
    /* The same thirteen words as `build_screen_regions` fills them, from the
       table it carries: word[1] is `mask`, word[12] the segment of `click`. */
    uint16_t  word[13];
  };
} __attribute__((packed));
_Static_assert(sizeof(struct region) == 0x1a, "a region record is thirteen words");

#define REGION_PTR(p) ((volatile struct region *)(dgroup + (uint16_t)(p)))

DG_ASSERT_AT(struct region, link_ptr,   0x00);
DG_ASSERT_AT(struct region, mask,       0x02);
DG_ASSERT_AT(struct region, x0,         0x06);
DG_ASSERT_AT(struct region, y0,         0x08);
DG_ASSERT_AT(struct region, x1,         0x0a);
DG_ASSERT_AT(struct region, y1,         0x0c);
DG_ASSERT_AT(struct region, cursor,     0x0e);
DG_ASSERT_AT(struct region, code,       0x10);
DG_ASSERT_AT(struct region, hover,      0x12);
DG_ASSERT_AT(struct region, click,      0x16);
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
    uint16_t  word_0e;         /* +0x0e  the record's own offset, filed by setup_streams */
} __attribute__((packed));

#define FILEREC_PTR(p) ((volatile struct file_rec *)(dgroup + (uint16_t)(p)))

/*
 * **Borland's streams**, at DGROUP 0x4bc4.
 *
 * Twenty `struct file_rec`, which is what the two routines that walk the table
 * say: `flush_all_streams` counts 0x14 of them at a stride of 0x10, and
 * `find_free_stream` bounds itself with `DG4D04.word_4d04 << 4` - the count
 * times the stride. The fields they read are already named on that struct -
 * `+2` is `flags` and `+4` is `handle`, which is tested signed because -1
 * means no handle.
 */
struct dg_4bc4 {
    struct file_rec streams[0x14];   /* +0x00 */
} __attribute__((packed));

#define DG4BC4 (*(volatile struct dg_4bc4 *)(dgroup + 0x4bc4))

DG_ASSERT_AT(struct dg_4bc4, streams,           0x00);

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
struct open_file {
    dg_off_t file_ptr;         /* +0x00  the Borland FILE this slot is for */
    uint8_t  path[0x19];       /* +0x02  the chunk tags walked into, four
                                         characters each, NUL-terminated */
    /*
     * +0x1b  **one 32-bit file offset each**, where that chunk ends, with
     * **bit 31 marking a container** - the bit `open_file_record` sets on
     * `bound[0]` and `read_record`'s walk masks off before comparing. Held
     * as two words it needed the flag taken off the high half at four sites
     * and the halves put back together at two more.
     *
     * **The original held it as a `long` too**, and its own instructions say
     * so. At 0x24136 the chunk-exhausted test reads both halves and masks
     * them - `and dx, 0xffff` then `and ax, 0x7fff` - and the first of those
     * masks a 16-bit register with 0xffff, which does nothing. It is only
     * there as the low half of one 32-bit `& 0x7fffffff`. What follows is
     * `cmp`/`jne` twice, an **equality** - which is why folding it to
     * `== pos` is exact, where an ordering compare on halves would not be.
     */
    uint32_t bound[7];         /* +0x1b */
    int16_t  depth;            /* +0x37  how far in, in bytes: a multiple of 4,
                                         and the walk gives up at 0x18 */
    int16_t  word_39;          /* +0x39  how many matches to skip */
    uint32_t pos;              /* +0x3b  the position, which restore_file_record
                                         seeks back to */
    uint32_t size;             /* +0x3f  the current chunk's size; what
                                         file_record_size answers */
} __attribute__((packed));

DG_ASSERT_AT(struct open_file, path,          0x02);
DG_ASSERT_AT(struct open_file, bound,         0x1b);
DG_ASSERT_AT(struct open_file, depth,         0x37);
DG_ASSERT_AT(struct open_file, word_39,       0x39);
DG_ASSERT_AT(struct open_file, pos,           0x3b);
DG_ASSERT_AT(struct open_file, size,          0x3f);
_Static_assert(sizeof(struct open_file) == 0x43,
               "an open file is what find_file_record strides by");

#define OPENFILE_PTR(p) ((volatile struct open_file *)(dgroup + (uint16_t)(p)))

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
    bmp_ptr_t bmp[];
} __attribute__((packed));

#define BMPSET_PTR(p) ((volatile struct bmp_set *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A bitmap header**, the record every entry of a `bmp_set` points at and the
 * only thing `draw_bitmap` is ever handed.
 *
 * **The segment comes first and the offset second**, which is the reverse of
 * every other far pair in this program. Three sites say so independently:
 * `draw_bitmap` normalises with `seg += off >> 4; off &= 0x0f`,
 * `vm_blit_bitmap` names them, and `free_bitmap_list` hands them to
 * `dos_free_far(off, seg)` in that order.
 *
 * `mask_off` is where the mask starts *in the same segment* - the driver takes
 * `(mask_off - off) >> 2` as its plane step - and three values are sentinels
 * instead of an offset, which is how `draw_bitmap` picks a drawing routine:
 *
 *     0xfffc  quadtree     set by the "BMP:VQT:" path, which `decode_vqt_list`
 *                          then decodes into a plain block
 *     0xfffd  scaled       blit_scaled_thunk        ("BMP:SCL:")
 *     0xfffe  compressed   draw_compressed_bitmap   ("BMP:SCN:", "BMP:RLE:")
 *     0xffff  offset       draw_offset_bitmap       ("BMP:OFF:")
 *     other   plain        blit_bitmap_thunk, and the value is a real offset
 *
 * `draw_bitmap`'s switch has a case for the last three and **not** for 0xfffc,
 * which therefore reaches the plain blitter. `load_bitmaps`' own comment lists
 * all four markers; whether a decoded quadtree has its field put back before
 * it is drawn is not established here.
 *
 * The pixels are a separate far block, so a header is a fixed near record and
 * nothing indexes an array of them - which is why no size is claimed here.
 * Nothing in the port reads past `height`.
 *
 * Field names are ours; the offsets are the original's.
 * ---------------------------------------------------------------------------
 */
struct bitmap {
    struct far_ptr_rev data;      /* +0x00  the pixel block, segment first */
    uint16_t  mask_off;           /* +0x04  the mask, or a sentinel above */
    int16_t   width;              /* +0x06  also the row stride */
    int16_t   height;             /* +0x08 */
} __attribute__((packed));

_Static_assert(sizeof(struct bitmap) == 0xa,
               "a bitmap header is the 0xa read_bmp_info calloc's one of "
               "per bitmap, and the 0xa it steps its cursor by");


/* **The same header as a pointer**, for the routines that take one rather
   than reach for a field. `draw_bitmap` and the four it dispatches to had a
   `uint16_t hdr` and did `BMP_PTR(hdr)->` throughout; the header is what they are
   handed and `struct bitmap *` says so.

   It is not `volatile`, unlike `BMP`. That qualifier is on the record because
   another thread draws through the same DGROUP - see the timer note in
   CLAUDE.md - and it belongs where a field is *read*, not on an argument a
   caller hands across. A `volatile` parameter here would only mean every one
   of the hundred-odd call sites casting into it. */
#define BMP_PTR(p) ((struct bitmap *)(dgroup + (uint16_t)(p)))

/* A **bitmap list**: a null-terminated array of near pointers to the above.
   `BMPSET_PTR(p)` and `BMPLIST(p)` are two views of one object - the first for a
   set whose entries are known by number, `bmp[0x25]`, the second for a list
   walked to its null. Same bytes, same element type, two names because the
   code reaches them two ways.
   Every loader in `bitmaps.c` answers one of these and the walks over it -
   count, free, set the sentinel, point each header at its pixels - are all
   this indexing. The name is ours; the shape is the loop's. */
#define BMPLIST(p) ((bmp_ptr_t *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **The quadtree bit reader**, the record `decode_vqt_list` builds on its own
 * stack and files into `DG6400.word_640c` for `vqt_node`, `vqt_screen_node`
 * and `fill_quadrant` to fetch back out.
 *
 * `pos` is a bit position, stepped four at a time and read as one 32-bit
 * count; `data` is the compressed block; `plane` is one far pointer per plane,
 * each a quarter of the bitmap on from the last; and `row` is `height` entries
 * of row offset, which is why the record is sized from the frame rather than
 * fixed - `sub sp,0x1ca` leaves 0x1b2 bytes for it, or 217 rows.
 *
 * Field names are ours; the offsets are the original's.
 * ---------------------------------------------------------------------------
 */
struct vqt_reader {
    uint32_t        pos;          /* +0x00  the bit position, one Borland
                                     `long` - see `bitmaps_t.pos` */
    struct far_ptr  data;         /* +0x04  the compressed block */
    struct far_ptr  plane[4];     /* +0x08  one per plane */
    int16_t         row[];        /* +0x18  `height` row offsets */
} __attribute__((packed));

#define VQTRD(p) ((volatile struct vqt_reader *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A sound-record node**, the eight bytes `read_sound_records` allocates one
 * of per record and threads onto a list ordered by `key`.
 *
 * These live in *far* memory rather than DGROUP - `alloc_for_kind(8, 0, 9)`
 * hands them out - so they are reached through a far pointer and there is no
 * `DG*` macro for them. The size is not a reading: it is the 8 that allocation
 * asks for, and `read_resource(handle, node, 4)` fills the first four bytes
 * while the loader zeroes the link.
 *
 * Field names are ours; the offsets are the original's.
 * ---------------------------------------------------------------------------
 */
struct sound_node {
    uint16_t       key;         /* +0x00  insert_by_key orders the list on it */
    uint16_t       length;      /* +0x02  summed to size the index block */
    struct far_ptr next;        /* +0x04  null-terminated, both halves zero */
} __attribute__((packed));

DG_ASSERT_AT(struct sound_node, key,            0x00);
DG_ASSERT_AT(struct sound_node, length,         0x02);
DG_ASSERT_AT(struct sound_node, next,           0x04);

/* One of these through the far pointer that reaches it. Not a `DG*` macro:
   they are not in DGROUP. */
#define NODE(p) ((volatile struct sound_node far *)MK_FP((p).seg, (p).off))

DG_ASSERT_AT(struct vqt_reader, pos,            0x00);
DG_ASSERT_AT(struct vqt_reader, data,           0x04);
DG_ASSERT_AT(struct vqt_reader, plane,          0x08);
DG_ASSERT_AT(struct vqt_reader, row,            0x18);

DG_ASSERT_AT(struct bitmap, data,               0x00);
DG_ASSERT_AT(struct bitmap, mask_off,           0x04);
DG_ASSERT_AT(struct bitmap, width,              0x06);
DG_ASSERT_AT(struct bitmap, height,             0x08);

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
/*
 * **A part's point table, in words.** The same thing `POINT_TABLE` names, for
 * the three tables `part_setup_40f0` reads: their entries are four bytes with
 * the coordinate at +0 and +2, and the image says why - every other byte is
 * zero, so they are `point16` and not `byte_pair`. The setup takes each with a
 * byte move, which is a low-byte read of a word and what the original does.
 *
 * **Also indexed by a part's form**, at 0x339a, 0x340a and 0x3416 - three
 * entries each, `{0x72,0} {0x72,5} {0x72,10}` at 0x340a - which is the same
 * table shape reached with a different index. It very nearly got a second name
 * for that; one shape, one macro.
 */
#define POINT16_TABLE(off) \
    ((const struct point16 *)(dgroup + (uint16_t)(off)))

/*
 * **The part shape tables**, DGROUP 0x3182 .. 0x3576.
 *
 * Every outline a part is built from lives in this one run: `part_setup_*`
 * copies one into the part's own `points_ptr`, and which one it copies is the
 * part's kind and form. `dg_317e` ends at 0x3182 and `DG3576` begins at
 * 0x3576, so the region is bounded on both sides and this struct covers it
 * with nothing left over.
 *
 * Three kinds of table are interleaved, and the type of each is what the code
 * that reads it says:
 *
 *   `s_` a run of `byte_pair`  - an outline, copied a point at a time;
 *   `p_` a run of `point16`    - the same shape in words, where every other
 *                                byte is zero, read with a byte move;
 *   `o_` a run of `dg_off_t`   - **offsets of the tables above**, indexed by
 *                                a part's form, so one kind reaches several
 *                                outlines.
 *
 * **The extents are measured, not assumed.** Each table runs to the next
 * object in the region; an offset table's length is how many of its words are
 * offsets into this region. The sizes then account for all 1012 bytes with no
 * hole, which is the check - a wrong length would leave one. That matters
 * here: `DG3A2C.blocks` was declared `[9]` from a sentence when the loop
 * wrote index 9, and this is the same shape of claim.
 *
 * The names carry the hex because nothing better is known. What each outline
 * *is* would come from the part it belongs to, and that is not established.
 *
 * **One run nothing in the port reads**, kept as bytes rather than given a
 * type on the strength of its values alone: 0x34ba reads as four `point16` -
 * (22,15) (39,15) (0,15) (16,15). Typed when something is found that reaches
 * it. Two others were typed that way: 0x31e6 is six signed words the boxing
 * glove reads as its reach, and 0x3394 three the jack-in-the-box reads - both
 * reached with the first form folded into the address, `[bx+0x31e8]` and
 * `[bx+0x3384]`, so the index at the site carries the fold. And 0x3330 is
 * five bytes the conveyor reads by width step, not the tail of the gun's
 * point pairs above it, which the gun copies seven of.
 */
struct part_shapes {
    struct byte_pair  s_3182[8];              /* 0x000  0x3182  8 pairs */
    struct byte_pair  s_3192[6];              /* 0x010  0x3192  6 pairs */
    struct byte_pair  s_319e[6];              /* 0x01c  0x319e  6 pairs */
    struct byte_pair  s_31aa[6];              /* 0x028  0x31aa  6 pairs */
    dg_off_t          o_31b6[3];              /* 0x034  0x31b6  3 offsets */
    struct byte_pair  s_31bc[6];              /* 0x03a  0x31bc  6 pairs */
    struct byte_pair  s_31c8[6];              /* 0x046  0x31c8  6 pairs */
    struct byte_pair  s_31d4[6];              /* 0x052  0x31d4  6 pairs */
    dg_off_t          o_31e0[3];              /* 0x05e  0x31e0  3 offsets */
    int16_t           glove_reach[6];         /* 0x064  0x31e6  -32 -82 0 80 130 0: how far the
                                                 boxing glove reaches, `part_step_boxing_glove` */
    struct byte_pair  s_31f2[6];              /* 0x070  0x31f2  6 pairs */
    struct byte_pair  s_31fe[6];              /* 0x07c  0x31fe  6 pairs */
    struct byte_pair  s_320a[6];              /* 0x088  0x320a  6 pairs */
    struct byte_pair  s_3216[6];              /* 0x094  0x3216  6 pairs */
    struct byte_pair  s_3222[4];              /* 0x0a0  0x3222  4 pairs */
    struct byte_pair  s_322a[4];              /* 0x0a8  0x322a  4 pairs */
    struct byte_pair  s_3232[8];              /* 0x0b0  0x3232  8 pairs */
    struct byte_pair  s_3242[8];              /* 0x0c0  0x3242  8 pairs */
    struct byte_pair  s_3252[5];              /* 0x0d0  0x3252  5 pairs */
    struct byte_pair  s_325c[5];              /* 0x0da  0x325c  5 pairs */
    struct byte_pair  s_3266[7];              /* 0x0e4  0x3266  7 pairs */
    struct byte_pair  s_3274[7];              /* 0x0f2  0x3274  7 pairs */
    struct byte_pair  s_3282[7];              /* 0x100  0x3282  7 pairs */
    struct byte_pair  s_3290[5];              /* 0x10e  0x3290  5 pairs */
    struct byte_pair  s_329a[5];              /* 0x118  0x329a  5 pairs */
    struct byte_pair  s_32a4[5];              /* 0x122  0x32a4  5 pairs */
    struct byte_pair  s_32ae[5];              /* 0x12c  0x32ae  5 pairs */
    struct byte_pair  s_32b8[4];              /* 0x136  0x32b8  4 pairs */
    struct byte_pair  s_32c0[4];              /* 0x13e  0x32c0  4 pairs */
    struct byte_pair  s_32c8[5];              /* 0x146  0x32c8  5 pairs */
    struct byte_pair  s_32d2[5];              /* 0x150  0x32d2  5 pairs */
    struct point16    p_32dc[8];              /* 0x15a  0x32dc  8 points */
    struct byte_pair  s_32fc[6];              /* 0x17a  0x32fc  6 pairs */
    struct byte_pair  s_3308[6];              /* 0x186  0x3308  6 pairs */
    struct byte_pair  s_3314[7];              /* 0x192  0x3314  7 pairs */
    struct byte_pair  s_3322[7];              /* 0x1a0  0x3322  7 pairs, the gun's points */
    uint8_t           conveyor_grab_x[5];     /* 0x1ae  0x3330  9 23 38 44 59: the grab x by width step,
                                                 `part_settle_conveyor` */
    uint8_t           unread_3335[1];         /* 0x1b3  0x3335 */
    struct byte_pair  s_3336[7];              /* 0x1b4  0x3336  7 pairs */
    struct byte_pair  s_3344[4];              /* 0x1c2  0x3344  4 pairs */
    struct byte_pair  s_334c[4];              /* 0x1ca  0x334c  4 pairs */
    struct byte_pair  s_3354[4];              /* 0x1d2  0x3354  4 pairs */
    struct byte_pair  s_335c[4];              /* 0x1da  0x335c  4 pairs */
    dg_off_t          o_3364[4];              /* 0x1e2  0x3364  4 offsets */
    struct byte_pair  s_336c[4];              /* 0x1ea  0x336c  4 pairs */
    struct byte_pair  s_3374[4];              /* 0x1f2  0x3374  4 pairs */
    struct byte_pair  s_337c[4];              /* 0x1fa  0x337c  4 pairs */
    struct byte_pair  s_3384[4];              /* 0x202  0x3384  4 pairs */
    dg_off_t          o_338c[4];              /* 0x20a  0x338c  4 offsets */
    int16_t           jack_reach[3];          /* 0x212  0x3394  -21 -34 -59: how far the jack-in-the-box
                                                 reaches by form, `part_step_jack_in_the_box` */
    struct point16    p_339a[4];              /* 0x218  0x339a  4 points */
    struct byte_pair  s_33aa[9];              /* 0x228  0x33aa  9 pairs */
    struct byte_pair  s_33bc[9];              /* 0x23a  0x33bc  9 pairs */
    struct byte_pair  s_33ce[4];              /* 0x24c  0x33ce  4 pairs */
    struct byte_pair  s_33d6[4];              /* 0x254  0x33d6  4 pairs */
    struct byte_pair  s_33de[4];              /* 0x25c  0x33de  4 pairs */
    dg_off_t          o_33e6[3];              /* 0x264  0x33e6  3 offsets */
    struct byte_pair  s_33ec[4];              /* 0x26a  0x33ec  4 pairs */
    struct byte_pair  s_33f4[4];              /* 0x272  0x33f4  4 pairs */
    struct byte_pair  s_33fc[4];              /* 0x27a  0x33fc  4 pairs */
    dg_off_t          o_3404[3];              /* 0x282  0x3404  3 offsets */
    struct point16    p_340a[3];              /* 0x288  0x340a  3 points */
    struct point16    p_3416[3];              /* 0x294  0x3416  3 points */
    struct byte_pair  s_3422[8];              /* 0x2a0  0x3422  8 pairs */
    struct byte_pair  s_3432[8];              /* 0x2b0  0x3432  8 pairs */
    struct byte_pair  s_3442[8];              /* 0x2c0  0x3442  8 pairs */
    struct byte_pair  s_3452[8];              /* 0x2d0  0x3452  8 pairs */
    struct byte_pair  s_3462[8];              /* 0x2e0  0x3462  8 pairs */
    struct byte_pair  s_3472[8];              /* 0x2f0  0x3472  8 pairs */
    struct byte_pair  s_3482[8];              /* 0x300  0x3482  8 pairs */
    dg_off_t          o_3492[2];              /* 0x310  0x3492  2 offsets */
    struct byte_pair  s_3496[8];              /* 0x314  0x3496  8 pairs */
    struct byte_pair  s_34a6[8];              /* 0x324  0x34a6  8 pairs */
    dg_off_t          o_34b6[2];              /* 0x334  0x34b6  2 offsets */
    uint8_t           unread_34ba[16];        /* 0x338  0x34ba  16 bytes */
    struct point16    p_34ca[3];              /* 0x348  0x34ca  3 points */
    struct point16    p_34d6[3];              /* 0x354  0x34d6  3 points */
    struct point16    p_34e2[8];              /* 0x360  0x34e2  8 points */
    struct point16    p_3502[8];              /* 0x380  0x3502  8 points */
    struct point16    p_3522[21];             /* 0x3a0  0x3522  21 points */
} __attribute__((packed));

#define PARTSHAPES (*(const struct part_shapes *)(dgroup + 0x3182))

_Static_assert(sizeof(struct part_shapes) == 0x3f4,
               "the shape tables run from 0x3182 to DG3576");
DG_ASSERT_AT(struct part_shapes, s_3182,        0x000);
DG_ASSERT_AT(struct part_shapes, s_3192,        0x010);
DG_ASSERT_AT(struct part_shapes, s_319e,        0x01c);
DG_ASSERT_AT(struct part_shapes, s_31aa,        0x028);
DG_ASSERT_AT(struct part_shapes, o_31b6,        0x034);
DG_ASSERT_AT(struct part_shapes, s_31bc,        0x03a);
DG_ASSERT_AT(struct part_shapes, s_31c8,        0x046);
DG_ASSERT_AT(struct part_shapes, s_31d4,        0x052);
DG_ASSERT_AT(struct part_shapes, o_31e0,        0x05e);
DG_ASSERT_AT(struct part_shapes, glove_reach,   0x064);
DG_ASSERT_AT(struct part_shapes, s_31f2,        0x070);
DG_ASSERT_AT(struct part_shapes, s_31fe,        0x07c);
DG_ASSERT_AT(struct part_shapes, s_320a,        0x088);
DG_ASSERT_AT(struct part_shapes, s_3216,        0x094);
DG_ASSERT_AT(struct part_shapes, s_3222,        0x0a0);
DG_ASSERT_AT(struct part_shapes, s_322a,        0x0a8);
DG_ASSERT_AT(struct part_shapes, s_3232,        0x0b0);
DG_ASSERT_AT(struct part_shapes, s_3242,        0x0c0);
DG_ASSERT_AT(struct part_shapes, s_3252,        0x0d0);
DG_ASSERT_AT(struct part_shapes, s_325c,        0x0da);
DG_ASSERT_AT(struct part_shapes, s_3266,        0x0e4);
DG_ASSERT_AT(struct part_shapes, s_3274,        0x0f2);
DG_ASSERT_AT(struct part_shapes, s_3282,        0x100);
DG_ASSERT_AT(struct part_shapes, s_3290,        0x10e);
DG_ASSERT_AT(struct part_shapes, s_329a,        0x118);
DG_ASSERT_AT(struct part_shapes, s_32a4,        0x122);
DG_ASSERT_AT(struct part_shapes, s_32ae,        0x12c);
DG_ASSERT_AT(struct part_shapes, s_32b8,        0x136);
DG_ASSERT_AT(struct part_shapes, s_32c0,        0x13e);
DG_ASSERT_AT(struct part_shapes, s_32c8,        0x146);
DG_ASSERT_AT(struct part_shapes, s_32d2,        0x150);
DG_ASSERT_AT(struct part_shapes, p_32dc,        0x15a);
DG_ASSERT_AT(struct part_shapes, s_32fc,        0x17a);
DG_ASSERT_AT(struct part_shapes, s_3308,        0x186);
DG_ASSERT_AT(struct part_shapes, s_3314,        0x192);
DG_ASSERT_AT(struct part_shapes, s_3322,        0x1a0);
DG_ASSERT_AT(struct part_shapes, conveyor_grab_x, 0x1ae);
DG_ASSERT_AT(struct part_shapes, s_3336,        0x1b4);
DG_ASSERT_AT(struct part_shapes, s_3344,        0x1c2);
DG_ASSERT_AT(struct part_shapes, s_334c,        0x1ca);
DG_ASSERT_AT(struct part_shapes, s_3354,        0x1d2);
DG_ASSERT_AT(struct part_shapes, s_335c,        0x1da);
DG_ASSERT_AT(struct part_shapes, o_3364,        0x1e2);
DG_ASSERT_AT(struct part_shapes, s_336c,        0x1ea);
DG_ASSERT_AT(struct part_shapes, s_3374,        0x1f2);
DG_ASSERT_AT(struct part_shapes, s_337c,        0x1fa);
DG_ASSERT_AT(struct part_shapes, s_3384,        0x202);
DG_ASSERT_AT(struct part_shapes, o_338c,        0x20a);
DG_ASSERT_AT(struct part_shapes, jack_reach,    0x212);
DG_ASSERT_AT(struct part_shapes, p_339a,        0x218);
DG_ASSERT_AT(struct part_shapes, s_33aa,        0x228);
DG_ASSERT_AT(struct part_shapes, s_33bc,        0x23a);
DG_ASSERT_AT(struct part_shapes, s_33ce,        0x24c);
DG_ASSERT_AT(struct part_shapes, s_33d6,        0x254);
DG_ASSERT_AT(struct part_shapes, s_33de,        0x25c);
DG_ASSERT_AT(struct part_shapes, o_33e6,        0x264);
DG_ASSERT_AT(struct part_shapes, s_33ec,        0x26a);
DG_ASSERT_AT(struct part_shapes, s_33f4,        0x272);
DG_ASSERT_AT(struct part_shapes, s_33fc,        0x27a);
DG_ASSERT_AT(struct part_shapes, o_3404,        0x282);
DG_ASSERT_AT(struct part_shapes, p_340a,        0x288);
DG_ASSERT_AT(struct part_shapes, p_3416,        0x294);
DG_ASSERT_AT(struct part_shapes, s_3422,        0x2a0);
DG_ASSERT_AT(struct part_shapes, s_3432,        0x2b0);
DG_ASSERT_AT(struct part_shapes, s_3442,        0x2c0);
DG_ASSERT_AT(struct part_shapes, s_3452,        0x2d0);
DG_ASSERT_AT(struct part_shapes, s_3462,        0x2e0);
DG_ASSERT_AT(struct part_shapes, s_3472,        0x2f0);
DG_ASSERT_AT(struct part_shapes, s_3482,        0x300);
DG_ASSERT_AT(struct part_shapes, o_3492,        0x310);
DG_ASSERT_AT(struct part_shapes, s_3496,        0x314);
DG_ASSERT_AT(struct part_shapes, s_34a6,        0x324);
DG_ASSERT_AT(struct part_shapes, o_34b6,        0x334);
DG_ASSERT_AT(struct part_shapes, unread_34ba,   0x338);
DG_ASSERT_AT(struct part_shapes, p_34ca,        0x348);
DG_ASSERT_AT(struct part_shapes, p_34d6,        0x354);
DG_ASSERT_AT(struct part_shapes, p_34e2,        0x360);
DG_ASSERT_AT(struct part_shapes, p_3502,        0x380);
DG_ASSERT_AT(struct part_shapes, p_3522,        0x3a0);

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

#define BELT_PTR(p) ((volatile struct belt *)(dgroup + (uint16_t)(p)))

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

#define ROPE_PTR(p) ((volatile struct rope *)(dgroup + (uint16_t)(p)))

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

/* Not `volatile`, for the reason `PARTP` gives: a point array is a part's
   own data, reached only from one, and the timer handler touches neither. */
#define POINTS(p) ((struct part_point *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A part kind**, the 0x3a-byte record at DGROUP 0x0ea6 that every part of
 * that kind shares. `PART_PTR(x)->kind` is the index: eighteen sites compute
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
    /* **The velocity clamp, not padding.** `clamp_record_pair` bounds a part's
       `vel_x` and `word_38` to plus and minus this. */
    int16_t   max_speed;       /* +0x0a */
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
    /* **Two level bounds, not padding.** `refile_overlapping_parts` compares a
       draw level against each, with 0xff meaning no limit. The name is a
       reading of that comparison and nothing more. */
    uint8_t   refile_level[2]; /* +0x1c */
    uint16_t  point_count;     /* +0x1e */
    uint16_t  word_20;         /* +0x20 */
    /* **Five hooks, not three**, each a far pointer the game calls through.
       Two of them were inside a `pad_20[10]` until the dispatchers were
       typed: `part_step` calls `[bx + 0x0ecc]` and `part_hit` calls
       `[bx + 0x0ec8]`, where `bx` is `kind * 0x3a` - so those are this
       record's +0x26 and +0x22, the compiler having folded 0x0ea6 into the
       displacement. */
    struct far_ptr hit;        /* +0x22  `call_part_hook(.., "hit")` */
    struct far_ptr step;       /* +0x26  `call_part_hook(.., "step")` */
    struct far_ptr setup;      /* +0x2a */
    struct far_ptr flip;       /* +0x2e */
    struct far_ptr settle;     /* +0x32 */
    struct far_ptr drive;      /* +0x36  the drive hook - the one `part_drive` calls with seven arguments */
} __attribute__((packed));

DG_ASSERT_AT(struct part_kind, weight,        0x02);
DG_ASSERT_AT(struct part_kind, word_04,       0x04);
DG_ASSERT_AT(struct part_kind, word_06,       0x06);
DG_ASSERT_AT(struct part_kind, gravity,       0x08);
DG_ASSERT_AT(struct part_kind, max_speed,     0x0a);
DG_ASSERT_AT(struct part_kind, max_w,         0x0c);
DG_ASSERT_AT(struct part_kind, max_h,         0x0e);
DG_ASSERT_AT(struct part_kind, min_w,         0x10);
DG_ASSERT_AT(struct part_kind, min_h,         0x12);
DG_ASSERT_AT(struct part_kind, bitmaps_ptr,   0x14);
DG_ASSERT_AT(struct part_kind, bitmaps2_ptr,  0x16);
DG_ASSERT_AT(struct part_kind, word_18,       0x18);
DG_ASSERT_AT(struct part_kind, word_1a,       0x1a);
DG_ASSERT_AT(struct part_kind, refile_level,  0x1c);
DG_ASSERT_AT(struct part_kind, point_count,   0x1e);
DG_ASSERT_AT(struct part_kind, hit,          0x22);
DG_ASSERT_AT(struct part_kind, step,         0x26);
DG_ASSERT_AT(struct part_kind, setup,   0x2a);
DG_ASSERT_AT(struct part_kind, flip,   0x2e);
DG_ASSERT_AT(struct part_kind, settle,   0x32);
DG_ASSERT_AT(struct part_kind, drive,   0x36);
_Static_assert(sizeof(struct part_kind) == 0x3a,
               "a part kind is what free_part_bitmap strides by");

/*
 * **The part kinds by name**, so `make_part(KIND_DYNAMITE)` says which part
 * it is building where `make_part(0x13)` did not.
 *
 * `#define` and not an `enum`, because a part's `kind` field is **two
 * bytes** and a C enum is an `int`. Giving one a fixed underlying type is
 * C23; this compiler accepts `enum part_kind_id : uint16_t` even under
 * `-std=c11`, which is exactly the reason not to write it - the layout of
 * `struct part` would then depend on an extension rather than on the
 * original.
 *
 * The names come from the manual and the game's own menus - see the table
 * above `PARTKIND`. Three are not in either and are named from what the
 * code does with them: the gun fires a **bullet**, a burst leaves a **blast**
 * of shreds behind, and a cut belt leaves two **anchors** at the cut.
 *
 * Kinds 51 to 54 have no icon, no name and no initialiser, so they get no
 * constant. 55, 56 and 57 do have initialisers and still have no name, so
 * they carry their numbers - a name replaces one the moment it is found.
 */
#define KIND_BOWLING_BALL       0
#define KIND_BRICK_PLATFORM     1
#define KIND_RAMP               2
#define KIND_SEESAW             3
#define KIND_BALLOON            4
#define KIND_CONVEYOR           5
#define KIND_MOUSE_CAGE         6
#define KIND_PULLEY             7
#define KIND_BELT               8
#define KIND_BASKETBALL         9
#define KIND_ROPE              10
#define KIND_BIRD_CAGE         11
#define KIND_POKEY             12
#define KIND_JACK_IN_THE_BOX   13
#define KIND_GEAR              14
#define KIND_BOB_THE_FISH      15
#define KIND_BELLOW            16
#define KIND_BUCKET            17
#define KIND_CANNON            18
#define KIND_DYNAMITE          19
#define KIND_BULLET            20
#define KIND_ELECTRIC_PLUG     21
#define KIND_DYNAMITE_PLUNGER  22
#define KIND_HOOK              23
#define KIND_FAN               24
#define KIND_FLASHLIGHT        25
#define KIND_GENERATOR         26
#define KIND_GUN               27
#define KIND_BASEBALL          28
#define KIND_LIGHT             29
#define KIND_MAGNIFYING_GLASS  30
#define KIND_MONKEY            31
#define KIND_PUMPKIN           32
#define KIND_HEART_BALLOON     33
#define KIND_CHRISTMAS_TREE    34
#define KIND_BOXING_GLOVE      35
#define KIND_ROCKET            36
#define KIND_SCISSORS          37
#define KIND_SOLAR_PANEL       38
#define KIND_TRAMPOLINE        39
#define KIND_WINDMILL          40
#define KIND_BLAST             41
#define KIND_MORT_THE_MOUSE    42
#define KIND_CANNON_BALL       43
#define KIND_TENNIS_BALL       44
#define KIND_CANDLE            45
#define KIND_PIPE              46
#define KIND_CORNER_PIPE       47
#define KIND_WOODEN_PLATFORM   48
#define KIND_ANCHOR            49
#define KIND_MOTOR             50
/* No icon and no name, but real: 55, 56 and 57 carry initialisers, and
   `part_setup_1105` serves 55 and 57 and tells the two apart. They carry
   their numbers until something says what they are. */
#define KIND_55                55
#define KIND_56                56
#define KIND_57                57
/*
 * **What each kind is**, from the manual and from the game's own menus -
 * the game's word wins where the two differ, which is why kind 6 is the
 * mouse cage rather than the manual's mouse motor, 12 is pokey, 15 is bob
 * the fish and 31 is the monkey.
 *
 * This is the only place the mapping is written down, and it is not
 * derivable from the binary: the kind table holds pointers and sizes, not
 * names. The icons `TIM_PARTPICS` draws are indexed by kind and are the way
 * to check one - kind 37 draws a pair of scissors.
 *
 * Kinds 20, 41, 49 and 51..57 have no icon and no name; the last of those
 * still have initialisers, which is why `part_init_14ca0` and its two
 * neighbours keep their addresses for names.
 *
 *     0 bowling_ball          1 brick_platform        2 ramp
 *     3 seesaw                4 balloon               5 conveyor
 *     6 mouse_cage            7 pulley                8 belt
 *     9 basketball           10 rope                 11 bird_cage
 *    12 pokey                13 jack_in_the_box      14 gear
 *    15 bob_the_fish         16 bellow               17 bucket
 *    18 cannon               19 dynamite             21 electric_plug
 *    22 dynamite_plunger     23 hook                 24 fan
 *    25 flashlight           26 generator            27 gun
 *    28 baseball             29 light                30 magnifying_glass
 *    31 monkey               32 pumpkin              33 heart_balloon
 *    34 christmas_tree       35 boxing_glove         36 rocket
 *    37 scissors             38 solar_panel          39 trampoline
 *    40 windmill             42 mort_the_mouse       43 cannon_ball
 *    44 tennis_ball          45 candle               46 pipe
 *    47 corner_pipe          48 wooden_platform      50 motor
 */
/* the record for a kind, and the record at an address a routine was handed */
#define PARTKIND_AT_PTR(p) ((volatile struct part_kind *)(dgroup + (uint16_t)(p)))
#define PARTKIND_PTR(k)    PARTKIND_AT_PTR(0x0ea6 + 0x3a * (uint16_t)(k))

/*
 * ---------------------------------------------------------------------------
 * **A move-queue node**, eight bytes: `game.c` builds twenty of them with
 * `heap_calloc_far(1, 8)` and threads them on `DG4E4E.parts_free_ptr`;
 * `queue_part` moves one to `parts_queue_ptr`, sorted by the part's momentum
 * high word then low. `queue_part` used to read these through `PART_PTR()`, and
 * the field names lined up by offset - +4 was `kind` in one line and `lo` in
 * the next, +6 `flags_06` and `hi` - which is the same bytes under two types
 * with nothing able to object. The momentum halves are the part's own
 * `momentum_lo`/`momentum_hi`, copied in at +0x3c/+0x3e.
 * ---------------------------------------------------------------------------
 */
struct queue_node {
    dg_off_t  next;            /* +0x00 */
    dg_off_t  part;            /* +0x02  the part that asked to move */
    uint16_t  momentum_lo;     /* +0x04 */
    int16_t   momentum_hi;     /* +0x06  compared signed; the sort key */
} __attribute__((packed));

DG_ASSERT_AT(struct queue_node, part,         0x02);
DG_ASSERT_AT(struct queue_node, momentum_lo,  0x04);
DG_ASSERT_AT(struct queue_node, momentum_hi,  0x06);
_Static_assert(sizeof(struct queue_node) == 8, "a queue node is what heap_calloc_far(1, 8) makes");

#define QNODE_PTR(p) ((struct queue_node *)(dgroup + (uint16_t)(p)))

/*
 * ---------------------------------------------------------------------------
 * **A saved-rectangle list entry**, 0x1a bytes, chained through +0x18 on one
 * of the twenty heads in `DG56B8.slot[]` and returned whole to
 * `DG56E0.rect_free_ptr`.
 *
 * **Its creator is dead code in the shipped binary.** The record is filled
 * at 0x0a0d7 - the arguments are the fields in order, `[bp+6..0xc]` into
 * +0..+6, `[bp+0xe]` into +0xc, `[bp+0x10]`/`[bp+0x12]` into +8/+0xa,
 * `[bp+0x14]` into +0xe, `[bp+0x16]`/`[bp+0x18]` into +0x14/+0x16, and
 * `imul` of w by h into +0x10 - and that routine's one caller is 0x0a717,
 * which nothing calls: no near or far call to it in the image, no 2- or
 * 4-byte occurrence of its address as data, and the recursive code map from
 * the entry point reaches both readers and none of the six save-side
 * routines (0x0a05f builds the pool five at a time with
 * `heap_calloc_far(n, 0x1a)` and counts them at 0x56b6; 0x0a4bf discards
 * every slot; 0x0a5a1 tears the pool down; 0x0a5d8 reads the count). So the
 * slots are empty in the original too, the port dropped nothing, and the
 * readers below walk what the original walks: nothing.
 *
 * Names come from the creator's own stores where it has them - `area` is the
 * `imul` at 0x0a2ec, `block_head` is the 1 the builder writes at 0x0a098 on the
 * first record of each block it allocates and the teardown tests `& 1` to free
 * a whole block at once - and from the readers otherwise. `refcount` is a reading:
 * `restore_saved_rect_lists` steps it every frame and `find_saved_rect_slot`
 * matches it against a caller-supplied 0.
 * ---------------------------------------------------------------------------
 */
struct rect_list_entry {
    int16_t   x;               /* +0x00  in eight-pixel columns */
    int16_t   y;               /* +0x02 */
    int16_t   w;               /* +0x04  in eight-pixel columns */
    int16_t   h;               /* +0x06 */
    dg_off_t  page_src;        /* +0x08 */
    dg_off_t  page_dst;        /* +0x0a */
    uint16_t  mode;            /* +0x0c  1 copies the rect, 4 restores it from `buf` */
    int16_t   refcount;             /* +0x0e  how many hold the slot; stepped down once a frame, reusable at 0 */
    uint16_t  area;            /* +0x10  w * h, from the creator's imul */
    uint16_t  block_head;      /* +0x12  1 on the first record of each heap block */
    struct far_ptr buf;        /* +0x14  the saved pixels, for mode 4 */
    dg_off_t  next;            /* +0x18 */
} __attribute__((packed));

DG_ASSERT_AT(struct rect_list_entry, page_src, 0x08);
DG_ASSERT_AT(struct rect_list_entry, mode,     0x0c);
DG_ASSERT_AT(struct rect_list_entry, refcount,      0x0e);
DG_ASSERT_AT(struct rect_list_entry, buf,      0x14);
DG_ASSERT_AT(struct rect_list_entry, next,     0x18);
_Static_assert(sizeof(struct rect_list_entry) == 0x1a, "a rect list entry is 0x1a bytes");

#define RECTENT_PTR(p) ((struct rect_list_entry *)(dgroup + (uint16_t)(p)))

/*
 * **How many rect records the pool holds**, at DGROUP 0x56b6, just below the
 * twenty slot heads. Written only by the dead pool builder at 0x0a05f and
 * read only by the dead getter at 0x0a5d8; declared so the word has a name
 * and so nothing else is laid over it.
 */
struct dg_56b6 {
    uint16_t  rect_pool_count;    /* +0x00 */
} __attribute__((packed));

#define DG56B6 (*(volatile struct dg_56b6 *)(dgroup + 0x56b6))

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
    struct far_ptr init;       /* +0x0c  the kind's init routine, called
                                         far */
} __attribute__((packed));

DG_ASSERT_AT(struct part_template, flags_0a,  0x02);
DG_ASSERT_AT(struct part_template, word_50,   0x04);
DG_ASSERT_AT(struct part_template, word_52,   0x06);
DG_ASSERT_AT(struct part_template, width,     0x08);
DG_ASSERT_AT(struct part_template, height,    0x0a);
DG_ASSERT_AT(struct part_template, init,  0x0c);
_Static_assert(sizeof(struct part_template) == 0x10,
               "a part template is what make_part strides by");

#define PARTTMPL_PTR(n) ((volatile struct part_template *) \
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
    struct far_ptr scratch;    /* +0x02  the far scratch block, which
                                  lzss_reset caches */
    /* **Polymorphic, which is why this pair is not a `far_ptr`.** It is a
       file handle on one path and the two halves of a far pointer on another,
       and `read_resource_block` builds a `struct far_ptr` from it at the point
       of use rather than the field claiming to be one. A union of the two -
       `struct { uint16_t handle; }` against `struct far_ptr ptr;` - would say
       it properly and is worth doing once which path sets which is written
       down; until then the call sites carry the compound literal and this
       comment carries the reason. */
    uint16_t  word_06;         /* +0x06  a file handle, or the low half of a far pointer */
    uint16_t  word_08;         /* +0x08 */
    /* **Three Borland `long`s.** `read_input_block` takes `end - in` with a
       borrow and compares the two as wholes; `next_input_byte` steps `in`
       with a carry; `open_resource` splits a `uint32_t` into `end` and
       `resource_tell` joins `in` back into one. */
    uint32_t  in;              /* +0x0a  how far into the compressed input
                                         the reader is */
    uint32_t  end;             /* +0x0e  where the compressed input ends */
    uint32_t  size;            /* +0x12  what resource_seek measures from for
                                         SEEK_END */
    uint32_t  pos;             /* +0x16  and what it measures from for
                                         SEEK_CUR. Stepped with a carry by
                                         `read_resource`, subtracted from
                                         `size` with a borrow, and compared
                                         against the target **signed** - the
                                         original's `cmp hi / jg / jl / cmp
                                         lo / ja` over the pair. */
    union {
        /* the run counter. Mostly a byte, but one site increments it 16 bits
           wide, so the carry into +0x1b is the original's and is kept */
        uint16_t word_1a;      /* +0x1a */
        struct {
            uint8_t byte_1a;   /* +0x1a */
            uint8_t byte_1b;   /* +0x1b */
        };
    };
    uint32_t  start;           /* +0x1c  where in the file the resource begins,
                                         from game_ftell at open */
    uint8_t   kind;            /* +0x20  the type prepare_resource_slot was given */
} __attribute__((packed));

DG_ASSERT_AT(struct resource, scratch,       0x02);
DG_ASSERT_AT(struct resource, word_06,       0x06);
DG_ASSERT_AT(struct resource, word_08,       0x08);
DG_ASSERT_AT(struct resource, in,            0x0a);
DG_ASSERT_AT(struct resource, end,           0x0e);
DG_ASSERT_AT(struct resource, size,          0x12);
DG_ASSERT_AT(struct resource, pos,           0x16);
DG_ASSERT_AT(struct resource, word_1a,       0x1a);
DG_ASSERT_AT(struct resource, byte_1b,       0x1b);
DG_ASSERT_AT(struct resource, start,         0x1c);
DG_ASSERT_AT(struct resource, kind,          0x20);
_Static_assert(sizeof(struct resource) == 0x21,
               "a resource is what heap_calloc_far(1, 0x21) makes");

#define RESOURCE_PTR(p) ((volatile struct resource *)(dgroup + (uint16_t)(p)))

DG_ASSERT_AT(struct file_rec, left,     0x00);
DG_ASSERT_AT(struct file_rec, flags,    0x02);
DG_ASSERT_AT(struct file_rec, handle,   0x04);
DG_ASSERT_AT(struct file_rec, buf_size, 0x06);
DG_ASSERT_AT(struct file_rec, read_ptr, 0x0a);

#endif /* DGROUP_H */
