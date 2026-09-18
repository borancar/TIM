/*
 * The original's DGROUP: one 64 KB data segment.
 *
 * It is modelled as a **byte array**, not as a set of C globals, because the
 * game uses *near pointers* - a word in DGROUP holding an offset into DGROUP,
 * dereferenced as `[bx + 0x22]`. Named globals cannot express that; an array
 * can, and it is what the original actually has.
 *
 * Named variables are objects the linker places *inside* the array - see
 * `DGROUP_AT` below - so a name and a pointer dereference reach the same byte.
 * Where a name is a guess it says so; the offsets are read from the
 * disassembly and are not.
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

/* Defined by the linker script `tools/genld.py` writes, not in C - see below. */
extern uint8_t  guest_mem[GUEST_MEM_BYTES];

/*
 * **Where the program sits, and what of it the port carries.** DOS loaded the
 * image at segment 0x0110 - the PSP at 0x0100 and its 0x10 paragraphs below it,
 * which is where the reference emulator puts it too - and DGROUP is at image
 * offset 0x2d3c0 of that, so linear 0x2e4c0. Borland's startup zeroes DGROUP
 * from 0x4e4e to 0x64ca (`rep stosb` at 0x000cd), so everything the image
 * gives DGROUP is below 0x4e4e.
 *
 * The port does not load the image. What the game needs of it is transcribed
 * as C objects, each marked with where it goes:
 *
 *   DGROUP_AT(off)       initialised DGROUP data, below DGROUP_INIT_END
 *   DGROUP_BSS(off)      DGROUP state the startup zeroes, from DGROUP_INIT_END
 *   SEGMENT_AT(seg, off) data a code segment keeps inside itself
 *
 * `tools/genld.py` reads those sections back out of the objects and writes the
 * linker script that lays `guest_mem` out with each object at its address, so
 * a DGROUP offset and a named object reach the same byte - which is what lets
 * the verifier seed and compare the whole segment - and the linker refuses two
 * that overlap. **An offset is written with four hex digits** (`0x0ea6`, not
 * `0xea6`): it becomes the section name, and the script matches it exactly.
 *
 * A segment the image relocated is the load segment plus the image's own
 * value, and is transcribed as `LOAD_SEG + 0x172c`, never as the sum.
 */
#define LOAD_SEG        0x0110u
#define IMG_DGROUP      0x2D3C0u
#define DGROUP_INIT_END 0x4e4e

/*
 * **`aligned(1)` is not decoration.** The x86-64 ABI lets the compiler assume a
 * global of sixteen bytes or more is on a sixteen-byte boundary, and GCC acts
 * on it: `reset_input_state` cleared `MACHINE_BUTTONS` with one `movaps`, which
 * faults on an address that is not - and a DGROUP offset usually is not. The
 * linker script's `SUBALIGN(1)` puts the object where it belongs; this is what
 * stops the code that uses it assuming otherwise. genld refuses a guest section
 * whose alignment is not 1.
 */
#define DGROUP_AT(off)       __attribute__((section(".guest.dgroup." #off), used, aligned(1)))
#define DGROUP_BSS(off)      __attribute__((section(".bss.guest.dgroup." #off), used, aligned(1)))
#define SEGMENT_AT(seg, off) __attribute__((section(".guest.seg." #seg "." #off), used, aligned(1)))

/* Declared in io.h, which this header deliberately does not include: `dg_near`
   below refuses a pointer that is not the guest's, and the refusal has to be
   loud. */
void port_abort(const char *msg);
extern uint32_t dgroup_base;        /* linear address of DGROUP */

#define dgroup      (guest_mem + dgroup_base)

/*
 * **The struct overlays are not `volatile`, and the three words that are say
 * so on their fields.** The guest's memory is shared with exactly one other thread, the
 * timer's, and that thread is the port's own doing - an interrupt on the
 * original suspends the game rather than running beside it. What `volatile`
 * buys is one thing: a loop that reads a word and does nothing else cannot
 * have the read hoisted out of it. The game has three such loops, and each
 * spins on a word the timer thread writes - `TIMER.frame_budget`,
 * `DG5752.frame_flag` and `SOUND_TICK_WAIT.ticks_left` - so those three fields are
 * `volatile`, where they are declared, and nothing else is. Every other
 * access, a blitter's included, is a plain read or write; where the two
 * threads race on it (see CLAUDE.md) `volatile` would not have helped, and
 * it was making every accessor and every prototype in the port say something
 * that was not true. Until 2026-09-12 all of them said it.
 */

/*
 * **No raw accessor macro is left** - the family that read DGROUP at a
 * constant offset. DGROUP is read through a field of a struct overlay, through
 * a typed pointer such as `VQTRD`, or through `dg_near_ptr(offset)`, and
 * that last one still reaches a byte by a computed number at dozens of sites:
 * it is what a typed view has not replaced yet, and `dgrules.py` does not
 * count it.
 *
 * They went one at a time, each when the last site using it had a field.
 * `DGS8` never had a caller: the eleven signed-byte sites wrote the cast out,
 * which is the better spelling, because the `cbw` is the original's and a
 * `(int8_t)` at the site says so. `DG16` and `DG32` went when the struct work
 * had taken their sites. `DG8` and `DGU16` went last, once the hybrid runner's
 * autoplay and the three macros built on `DGU16` - `DG_FAR_OFF`, `DG_FAR_SEG`
 * and `span_buffer_seg` - read the fields they were reading.
 * `tools/dgrules.py --rule raw` still looks for all five names, so one coming
 * back is a finding.
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
#define FP_LIN(p)         ((uint32_t)((const uint8_t *)(p) - guest_mem))
#define FP_SEG(p)         ((uint16_t)(FP_LIN(p) >> 4))
#define FP_OFF(p)         ((uint16_t)(FP_LIN(p) & 0xf))
#define FAR8(seg, off)    (*(uint8_t *)MK_FP(seg, off))
#define FAR16(seg, off)   (*(int16_t *)MK_FP(seg, off))
#define FARU16(seg, off)  (*(uint16_t *)MK_FP(seg, off))

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
 * Names ending `_ptr` hold an address, and the type says which kind: `dg_near_t`
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
typedef uint16_t dg_near_t;      /* a near pointer: an offset into DGROUP */

/* **A near pointer to a bitmap header**, which is what the game stores
   wherever it keeps one: two bytes, an offset into DGROUP, exactly
   `dg_near_t` and named for what it points at.

   It is not `struct bitmap *`. A host pointer is eight bytes and these live
   in guest memory two bytes apart, so a `struct bitmap **` over one of these
   arrays would stride four times too far from the second entry on - which is
   the whole reason `dg_near_t` exists. The typedef buys the name without
   touching the width. */
typedef dg_near_t bmp_ptr_t;
/* A list of these is stepped with `++`, which must be the original's `+ 2`. */
_Static_assert(sizeof(bmp_ptr_t) == 2, "bmp_ptr_t is a near pointer, one word");
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
 */
struct far_ptr {
    dg_near_t off;              /* +0x00 */
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
    dg_near_t off;              /* +0x02 */
} __attribute__((packed));

/*
 * **Normalise a far pointer**: carry the paragraphs out of the offset into the
 * segment and keep only the remainder, which is what `draw_bitmap` does to
 * every header before it draws and what `decode_vqt_list` does to reach the
 * first plane. Ours as a routine; the two lines are the original's.
 */
static inline struct far_ptr far_normalise(struct far_ptr p)
{
    return (struct far_ptr){ (dg_near_t)(p.off & 0x0f),
                             (dg_seg_t)(p.seg + (p.off >> 4)) };
}

/*
 * **The pair a pointer is filed as.** `FP_SEG` and `FP_OFF` as one value, for
 * the store into a `struct far_ptr` field - the counterpart of the `MK_FP` that
 * reads one. It is the **normalised** pair, which is what the original holds
 * wherever it has just stepped a huge pointer; see `FP_SEG`. Ours.
 */
static inline struct far_ptr far_of(const uint8_t *p)
{
    struct far_ptr r = { FP_OFF(p), FP_SEG(p) };

    return r;
}

/*
 * **The pointer a stored pair names** - `far_of` the other way round, and
 * `dg_near_ptr`'s far counterpart. Every read of a `struct far_ptr` field is
 * this `MK_FP`, and spelling both halves out at each one said nothing the
 * field's own name did not. The typed records have their own - `SEQUENCE_PTR`,
 * `SOUND_RECORD_PTR`. Ours.
 */
static inline uint8_t *dg_far_ptr(struct far_ptr p)
{
    return MK_FP(p.seg, p.off);
}

/*
 * **A pointer filed in another pointer's segment.** The original often holds a
 * segment and steps only the offset inside it - `advance_record` answers the
 * segment it was given beside a moved offset - and the pair it files is then
 * that segment with that offset, not the normalised pair `far_of` would give.
 * `from` is the pointer whose segment is kept; `p` is where the stepped offset
 * lands. Ours.
 */
static inline struct far_ptr far_stepped(const uint8_t *from, const uint8_t *p)
{
    struct far_ptr r = { (uint16_t)(FP_OFF(from) + (p - from)), FP_SEG(from) };

    return r;
}

/* The same, for the one record that stores the pair segment-first. */
static inline struct far_ptr_rev far_normalise_rev(struct far_ptr_rev p)
{
    return (struct far_ptr_rev){ (dg_seg_t)(p.seg + (p.off >> 4)),
                                 (dg_near_t)(p.off & 0x0f) };
}

/*
 * **The null far pointer**, 0000:0000. The game tests for it as
 * `(off | seg) == 0` - one `or` and a branch, which is the same question as
 * both halves being zero, and the port asks it as
 * `dg_far_ptr(p) == FAR_NULL_PTR`.
 *
 * **There was a `far_eq` here, comparing the two words**, and it was retired
 * on 2026-09-18 when every one of its 44 call sites became a pointer
 * comparison. The two are the same test wherever the pairs being compared were
 * filed the same way, which is every one of them: a record's own block, or a
 * pair copied from one table to another. Where an *exact* pair matters it is a
 * store, not a comparison, and `far_stepped` files those.
 *
 * Note that this is *not* a C null pointer: 0000:0000 is a real address in the
 * guest, the first byte of `guest_mem`, which is why `draw_string_body`'s
 * guard is `(str | seg) == 0` and not `str == NULL`.
 */
static const struct far_ptr FAR_NULL = { 0, 0 };

/*
 * **And the same null as a pointer**, for the routines that hold one rather
 * than a filed pair - a block DOS refused, a list that ends. It is
 * `dg_far_ptr(FAR_NULL)`, and it is **not** C's `NULL`: the guest's null is an
 * address, `guest_mem`'s first byte, which is why the tests against it are
 * written out rather than `!p`. The typed records say it their own way -
 * `SEQUENCE_NONE`, `PART_NONE`, `BMP_NONE`.
 */
#define FAR_NULL_PTR dg_far_ptr(FAR_NULL)

/* `dg_far_ptr` for the one record that stores the pair segment-first - a
   bitmap's pixels. */
static inline uint8_t *dg_far_ptr_rev(struct far_ptr_rev r)
{
    return MK_FP(r.seg, r.off);
}

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
 * offset into something a routine can be handed, and `dg_near` turns an address
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
 * `dg_near(dgroup, &VMDS.font_table_34[si])` is what `game_fread` wants, and
 * it says which table where `(uint16_t)(0x38c4 + si)` did not.
 */
static inline uint8_t *dg_ptr(void *base, uint16_t off)
{
    return (uint8_t *)base + off;
}

/*
 * **The pointer a stored near pointer names**, which is `dg_near_ptr(off)`
 * and was written that way at all 74 of them. `dg_near` is the store; this is
 * the read, and it names DGROUP the once rather than at every call - the base
 * argument earns its keep on `dg_ptr`, where the offset may belong to one of
 * the other four spaces, and says nothing on a field the game itself keeps as
 * a DGROUP offset. Ours.
 */
static inline uint8_t *dg_near_ptr(dg_near_t off)
{
    return dg_ptr(dgroup, off);
}

/* `const volatile`, because the struct overlays are volatile - see the note on
 * `volatile` above for why - and a plain `const void *` parameter would make every
 * call site discard the qualifier. */
/*
 * **A word in the guest's memory is read through `*(int16_t *)`**, and a
 * long through `*(int32_t *)`, wherever a routine is handed the address of one
 * rather than a field. The guest's records are packed and heap-allocated, so
 * the address can be odd; the host does the unaligned load, which is what the
 * original's `mov ax, [si+4]` did too. There used to be `dg_rd16`/`dg_wr16`
 * helpers here assembling the bytes by hand, retired on 2026-09-12: they said
 * the width where a cast says it as well, and a sanitizer's "misaligned"
 * report on such a read describes the model, not a defect.
 */

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
 * answered exactly rather than by a `dg_near` that could collide.
 */
static inline int dg_is_guest(const void *p)
{
    const uint8_t *b = (const uint8_t *)p;

    return b >= guest_mem && b < guest_mem + GUEST_MEM_BYTES;
}

/*
 * **The screen the driver reported**, at DGROUP 0x3f78 - which is the driver
 * block's own **+0x6e8**, so this struct and `struct vmds` describe the
 * same six bytes. They were written twice under two names: `game.c` set
 * `DG3F78.screen_height = 0x16f` in one routine and
 * `VMDS.screen_height = 0x18f` in another, and `set_full_clip` read
 * `VMDS.clip_bottom = DG3F78.screen_height - 1` with both names on one
 * line. `vmds` carries this as a field now, so there is one name.
 */
struct dg_3f78 {
    uint8_t   mode_kind;          /* +0x00  a byte saying which */
    uint8_t   pad_3f79[1];
    int16_t   screen_width;       /* +0x02  an extent past these is cut back to the edge */
    int16_t   screen_height;      /* +0x04  the copy-protection screen sets it to 0x18f first */
} __attribute__((packed));


/*
 * **An address back into the offset the guest holds it as.** The inverse of
 * `dg_ptr`, and a null pointer stays 0 because the guest's null is offset 0.
 *
 * **It refuses a pointer that is not the guest's.** `dg_near` takes a `void *`,
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
 *
 * **Only ever the value stored into a `dg_near_t` field named `_ptr`** - never
 * compared, cast, passed or returned; a comparison turns the stored field into
 * a pointer instead. `tools/check_dg_near.py` holds the code to that. `dg_near`
 * and `dg_near_t` were `dg_off` and `dg_off_t` until 2026-09-17.
 */
static inline dg_near_t dg_near(const void *base, const void *p)
{
    if (p == 0)
        return 0;

    if (!dg_is_guest(p))
        port_abort("dg_near on a pointer outside guest memory - a C local has "
                   "no offset the guest can hold");

    return (dg_near_t)((const uint8_t *)p
                       - (const uint8_t *)base);
}

/*
 * **A DGROUP object's far pointer**: DGROUP's segment and the object's near
 * pointer - what the original builds with `push ds` and an offset when a
 * routine that takes a far pointer is handed something of its own. `dg_near`'s
 * pair. The segment is DGROUP's whatever the offset is, so a null pointer is
 * DGROUP:0000, not the far null - `push ds` does not look at the offset. Ours,
 * like `dg_near`.
 */
static inline struct far_ptr dg_far(const void *base, const void *p)
{
    return (struct far_ptr){ dg_near(base, p),
                             (dg_seg_t)((uint32_t)((const uint8_t *)base - guest_mem) >> 4) };
}

/*
 * **The far-block table and the clipper's count**, at DGROUP 0x3a2c.
 */
struct dg_3a2c {
    uint16_t  clip_count;         /* +0x00  Sutherland and Hodgman's, rewritten after each edge */
    /* **Eleven slots of four bytes.** Slot 0 is `set_palette_pointer`'s, which
       the driver reaches on its own as driverDS:0x1a0 - the segment half
       alone, which is why each slot is a pair rather than a pointer. Slots 1
       to 9 are `load_palette`'s search and `free_far_block`'s walk. Slot 10
       takes the null a full table's store writes: the search stops at
       `di >= 0xa`, and the `cmp di,0xa / jl` at 0x1e9a4 guards only the
       *load* - failing it jumps straight to the store at 0x1eb4a, which files
       `[bx+0x3a2e]` with `di` still 10, at 0x3a56.

       Nothing else names those four bytes. After the table the game's code
       references nothing until 0x3b00 and the driver nothing until 0x3a70, and
       the image holds zeros throughout, so the storage is the table's. Eleven
       is what the code indexes; the 22 bytes after it are unreferenced too, so
       the source's array could have been larger, and nothing says so.

       It was `[9]` once, sized from a note that said "nine slots", and then
       `[10]` from a reading that took the `jl` for the store's guard. Both
       were sizes argued for rather than read off every store. */
    struct far_ptr blocks[11];    /* +0x02 */
} __attribute__((packed));


/*
 * **The video driver's data**, VMDS, at DGROUP 0x3890: the driver's own state,
 * which the game reads and writes through the fields below.
 */
struct vmds {
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
     * DGROUP offset - so the call sites read `dg_near(&VMDS.font_table_34[si])`
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
     * take DGROUP offsets, so those sites read `dg_near(dgroup, VMDS.poly_x)`.
     */
    int16_t   poly_x[20];                   /* +0xac   DGROUP 0x393c */
    int16_t   poly_y[20];                   /* +0xd4   DGROUP 0x3964 */
    int16_t   work_x[20];                   /* +0xfc   DGROUP 0x398c */
    int16_t   work_y[20];                   /* +0x124  DGROUP 0x39b4 */
    int16_t   closed_x[20];                 /* +0x14c  DGROUP 0x39dc, the closed
                                                       copy poly_fill walks */
    int16_t   closed_y[20];                 /* +0x174  DGROUP 0x3a04 */
    /* **The game's clip count and palette slots, inside the driver's data.**
       The game reaches them at DGROUP 0x3a2c; the driver reaches slot 0 on its
       own as VGA:0x0f15's palette, driverDS:0x19e. One block, two readers. */
    struct dg_3a2c palettes;                /* +0x19c  DGROUP 0x3a2c */
    uint8_t   unknown_1ca[0x4f2];           /* +0x1ca */
    uint16_t  dda_whole;                    /* +0x6bc */
    uint16_t  dda_frac;                     /* +0x6be */
    int16_t   dda_saved;                    /* +0x6c0 */
    uint16_t  dda_acc;                      /* +0x6c2 */
    uint8_t   line_mask;                    /* +0x6c4 */
    uint8_t   unknown_6c5[0x1d];            /* +0x6c5 */
    /* **The page hook**, DGROUP 0x3f72, which the game reads. Non-zero makes
       the three blitters call the vector at DGROUP 0x43b6 between taking the
       destination page and reading the clip. That vector is the driver's
       do-nothing stub, so the page comes back as it went in - and the port
       keeps the guard so a build whose 0x3f72 is *set* is not silently the
       same as one whose is clear. The name is a reading of that one use. */
    int16_t   page_hook;                    /* +0x6e2 */
    uint8_t   unknown_6e4[4];               /* +0x6e4 */
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

extern struct vmds VMDS;

#define DG_ASSERT_AT(type, field, off) \
    _Static_assert(__builtin_offsetof(type, field) == (off), \
                   #type "." #field " must sit at " #off)

DG_ASSERT_AT(struct dg_3f78, mode_kind,         0x00);
DG_ASSERT_AT(struct dg_3f78, screen_width,      0x02);
DG_ASSERT_AT(struct dg_3f78, screen_height,     0x04);

DG_ASSERT_AT(struct vmds, clip_enabled,      0x03);
DG_ASSERT_AT(struct vmds, clip_left,         0x04);
DG_ASSERT_AT(struct vmds, clip_right,        0x06);
DG_ASSERT_AT(struct vmds, clip_top,          0x08);
DG_ASSERT_AT(struct vmds, clip_bottom,       0x0a);
DG_ASSERT_AT(struct vmds, fill_enabled,      0x0c);
DG_ASSERT_AT(struct vmds, fill_colour,       0x0d);
DG_ASSERT_AT(struct vmds, second_colour,     0x0e);
DG_ASSERT_AT(struct vmds, poly_x,            0xac);
DG_ASSERT_AT(struct vmds, poly_y,            0xd4);
DG_ASSERT_AT(struct vmds, work_x,            0xfc);
DG_ASSERT_AT(struct vmds, work_y,            0x124);
DG_ASSERT_AT(struct vmds, closed_x,          0x14c);
DG_ASSERT_AT(struct vmds, closed_y,          0x174);
DG_ASSERT_AT(struct vmds, page_back_ptr,     0x12);
DG_ASSERT_AT(struct vmds, page_front_ptr,    0x14);
DG_ASSERT_AT(struct vmds, page_src_ptr,      0x16);
DG_ASSERT_AT(struct vmds, page_dst_ptr,      0x18);
DG_ASSERT_AT(struct vmds, pixel_shift,       0x1d);
DG_ASSERT_AT(struct vmds, adapter,           0x21);
DG_ASSERT_AT(struct vmds, line_colour,       0x22);
DG_ASSERT_AT(struct vmds, font_table_34,     0x34);
DG_ASSERT_AT(struct vmds, font_table_48,     0x48);
DG_ASSERT_AT(struct vmds, font_table_5c,     0x5c);
DG_ASSERT_AT(struct vmds, font_table_70,     0x70);
DG_ASSERT_AT(struct dg_3a2c, clip_count,        0x00);
DG_ASSERT_AT(struct dg_3a2c, blocks,            0x02);
DG_ASSERT_AT(struct vmds, palettes,          0x19c);
DG_ASSERT_AT(struct vmds, page_hook,         0x6e2);
DG_ASSERT_AT(struct vmds, dda_whole,         0x6bc);
DG_ASSERT_AT(struct vmds, dda_frac,          0x6be);
DG_ASSERT_AT(struct vmds, dda_saved,         0x6c0);
DG_ASSERT_AT(struct vmds, dda_acc,           0x6c2);
DG_ASSERT_AT(struct vmds, line_mask,         0x6c4);
DG_ASSERT_AT(struct vmds, screen,            0x6e8);
DG_ASSERT_AT(struct vmds, row_offset,        0x6f2);

/* The names above are the struct's fields now; there are no macros for
 * them, because a macro named for a field re-expands inside `VMDS.field`
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
    dg_near_t layer_head_ptr[6];  /* +0x00 */
} __attribute__((packed));

extern struct dg_50bf DG50BF;
_Static_assert(sizeof(struct dg_50bf) == 12, "six layer heads");

/*
 * ---------------------------------------------------------------------------
 * **Bare tables**: a run of same-sized entries at a fixed DGROUP address, with
 * no record around them. A pointer says what a raw word accessor cannot - the
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

/* A byte array indexed by the routine at 0x2147d, which returns its bit 0. */

/*
 * A near pointer at DGROUP 0x5400 to a structure, and three words beside it,
 * all used by the routine at 0x002be. What the structure is has not been
 * established; only the offsets it touches are known.
 */
/* These are `DG53FC.list_ptr` and its neighbours now; see the struct. */

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
 * counters and state words. `dg_near_t` marks the eight near pointers: five
 * region-list heads, two records kept beside them, and the bitmap lists.
 * ---------------------------------------------------------------------------
 */
struct dg_4e67 {
    uint16_t  freeform;            /* +0x00  1 in freeform mode - the bin is unlimited and nothing is scored - 0 on a loaded level */
    /* **Which handle the pointer is on**, and the only word that says what a
       click in the play area will do. 0 is nothing, 1 to 8 are the handles
       `part_handle_at_pointer` answers - the two flips, the four resize
       corners and the two ends - and 9 is "carrying a part". Bit 0x8000 says
       the handle is engaged, so `tool & 0x7fff` is the handle and the bit is
       the drag. `cursor_for_tool` turns the nine into cursor numbers, which is
       where the name comes from. */
    uint16_t  tool;            /* +0x02 */
    uint16_t  state;               /* +0x04  the round and screen state machine's word */
    dg_near_t region_kept_a_ptr;   /* +0x06  two records kept on their own as well */
    dg_near_t region_kept_b_ptr;   /* +0x08 */
    dg_near_t regions_a_ptr;       /* +0x0a  the five region lists, heads of */
    dg_near_t regions_b_ptr;       /* +0x0c */
    dg_near_t regions_c_ptr;       /* +0x0e */
    dg_near_t regions_panel_ptr;   /* +0x10  the briefing's controls */
    dg_near_t regions_play_ptr;    /* +0x12  the play screen's */
    int16_t   holiday_christmas;   /* +0x14  25 December - kind 34, the tree */
    int16_t   holiday_halloween;   /* +0x16  31 October  - kind 32, the pumpkin */
    int16_t   holiday_stpatrick;   /* +0x18  17 March    - read by nothing */
    int16_t   holiday_valentine;   /* +0x1a  14 February - kind 33, the heart */
    /* **The "memory is getting low" box has been shown.** Set with the box and
       cleared again only when the largest free block climbs back over 0x1770,
       which is the hysteresis that stops a machine hovering near the edge
       being told twice. */
    uint16_t  memory_warned;   /* +0x1c */
    uint16_t  file_op_active;      /* +0x1e  GUESS: 1 around the chdir a file dialog does */
    /* **Frames the loop that is running has run.** `step_loop_frames` adds one
       a frame from the intro's loop and from `run_machine_loop`, `round_setup`
       clears it, and `draw_machine_layer_f` clears it on its way in - which is
       what freezes the bin's header animation at frame 0, the one thing that
       reads it as a phase. The other reader is `goal_test_puzzle_70`, which
       wants 0x134 of them before it will pass, so on that puzzle it is
       elapsed time. Not `machine_frames`: that one `clear_machine` resets at
       every start and this one only a new round does. */
    int16_t   loop_frames;         /* +0x20  wraps 0x2a00 to 0x1c00 */
    /* **A countdown for the carried part's icon**, the same shape as a part's
       own `redraw_count`: the editor loop draws the icon and steps it down
       while it is not zero. */
    uint16_t  redraw_carried;  /* +0x22 */
    uint16_t  redraw_a;            /* +0x24  five deferred redraws, one layer each; a */
    uint16_t  redraw_b;            /* +0x26  change asks for N frames and gets one a */
    uint16_t  redraw_c;            /* +0x28  frame. Counts, not flags - see */
    uint16_t  redraw_d;            /* +0x2a  game_screen_loop, which decrements each */
    uint16_t  redraw_e;            /* +0x2c  by one rather than clearing it */
    /* **Where in the part the player took hold of it**: the pointer less the
       part's own origin, filed when a part is picked up and subtracted again
       every frame, so a part grabbed by its corner stays held by its corner.
       The y is first, which is the order the original writes them in. */
    uint16_t  drag_offset_y;   /* +0x2e */
    uint16_t  drag_offset_x;   /* +0x30 */
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
    /* **How far each bonus counter has rolled**, 0 to 0x15 - one digit cell -
       and back to 0 with one off the counter's value. `start_counters` puts
       the first at -4, which is four steps of nothing before it moves, and
       `step_counters` draws the band only while the scroll is positive. The
       second reel's is never armed in the shipped game; see `step_counters`
       and STATUS.md. */
    int16_t   bonus_2_scroll;  /* +0x4a */
    int16_t   bonus_1_scroll;  /* +0x4c */
    int16_t   password_puzzle;     /* +0x4e  the puzzle game_teardown prints a password for */
    int16_t   furthest_level;      /* +0x50  how far the player has reached; in tim.cfg */
    int16_t   level_count;         /* +0x52  how many L<n>.LEV there are */
    /* **Written once and never read**, and that is the whole of what is known:
       `round_setup` stores 0 here, and the two bytes of this offset occur
       exactly once in the image - that store. A dead store of the original's,
       kept because DGROUP is compared with the original's memory. */
    uint16_t  word_4ebb;       /* +0x54 */
    int16_t   round_number;        /* +0x56  the puzzle being played; round_setup loads it */
    int16_t   playing;             /* +0x58  game_play runs while this is non-zero */
    uint16_t  master_level;        /* +0x5a  the volume knob's setting; in tim.cfg */
    /* **The cursor showing, and the one the hourglass replaced.**
       `select_cursor` returns at once when the number it is given is already
       in `cursor`, `wait_cursor` files the outgoing one in `saved_cursor`
       unless it is the hourglass itself, and `restore_cursor` selects what is
       there. */
    int16_t   saved_cursor;    /* +0x5c */
    int16_t   cursor;          /* +0x5e */
    dg_near_t icons_bmp_ptr;      /* +0x60  icons.bmp's list */
    dg_near_t menu_bmp_ptr;       /* +0x62  gp_menu.bmp's */
    dg_near_t bmp_4ecb_ptr;       /* +0x64  gp_bord.bmp's */
    dg_near_t score2_bmp_ptr;     /* +0x66  score2.bmp's - draw_odometer_digit's strips */
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

extern struct dg_4e67 DG4E67;

DG_ASSERT_AT(struct dg_4e67, freeform,          0x00);
DG_ASSERT_AT(struct dg_4e67, title,             0x68);
DG_ASSERT_AT(struct dg_4e67, hint,              0xb8);
_Static_assert(sizeof(struct dg_4e67) == 0x248, "the hint runs up to DG50AF");
DG_ASSERT_AT(struct dg_4e67, tool,              0x02);
DG_ASSERT_AT(struct dg_4e67, state,             0x04);
DG_ASSERT_AT(struct dg_4e67, region_kept_a_ptr, 0x06);
DG_ASSERT_AT(struct dg_4e67, region_kept_b_ptr, 0x08);
DG_ASSERT_AT(struct dg_4e67, regions_a_ptr,     0x0a);
DG_ASSERT_AT(struct dg_4e67, regions_b_ptr,     0x0c);
DG_ASSERT_AT(struct dg_4e67, regions_c_ptr,     0x0e);
DG_ASSERT_AT(struct dg_4e67, regions_panel_ptr, 0x10);
DG_ASSERT_AT(struct dg_4e67, regions_play_ptr,  0x12);
DG_ASSERT_AT(struct dg_4e67, holiday_christmas, 0x14);
DG_ASSERT_AT(struct dg_4e67, holiday_halloween, 0x16);
DG_ASSERT_AT(struct dg_4e67, holiday_stpatrick, 0x18);
DG_ASSERT_AT(struct dg_4e67, holiday_valentine, 0x1a);
DG_ASSERT_AT(struct dg_4e67, memory_warned,     0x1c);
DG_ASSERT_AT(struct dg_4e67, file_op_active,    0x1e);
DG_ASSERT_AT(struct dg_4e67, loop_frames,       0x20);
DG_ASSERT_AT(struct dg_4e67, redraw_carried,    0x22);
DG_ASSERT_AT(struct dg_4e67, redraw_a,          0x24);
DG_ASSERT_AT(struct dg_4e67, redraw_b,          0x26);
DG_ASSERT_AT(struct dg_4e67, redraw_c,          0x28);
DG_ASSERT_AT(struct dg_4e67, redraw_d,          0x2a);
DG_ASSERT_AT(struct dg_4e67, redraw_e,          0x2c);
DG_ASSERT_AT(struct dg_4e67, drag_offset_y,     0x2e);
DG_ASSERT_AT(struct dg_4e67, drag_offset_x,     0x30);
DG_ASSERT_AT(struct dg_4e67, origin_c_y,        0x32);
DG_ASSERT_AT(struct dg_4e67, origin_c_x,        0x34);
DG_ASSERT_AT(struct dg_4e67, origin_b_y,        0x36);
DG_ASSERT_AT(struct dg_4e67, origin_b_x,        0x38);
DG_ASSERT_AT(struct dg_4e67, origin_y,          0x3a);
DG_ASSERT_AT(struct dg_4e67, origin_x,          0x3c);
DG_ASSERT_AT(struct dg_4e67, elapsed_ticks,     0x3e);
DG_ASSERT_AT(struct dg_4e67, machine_frames,    0x40);
DG_ASSERT_AT(struct dg_4e67, score,             0x42);
DG_ASSERT_AT(struct dg_4e67, counter,           0x46);
DG_ASSERT_AT(struct dg_4e67, bonus_2_scroll,    0x4a);
DG_ASSERT_AT(struct dg_4e67, bonus_1_scroll,    0x4c);
DG_ASSERT_AT(struct dg_4e67, password_puzzle,   0x4e);
DG_ASSERT_AT(struct dg_4e67, furthest_level,    0x50);
DG_ASSERT_AT(struct dg_4e67, level_count,       0x52);
DG_ASSERT_AT(struct dg_4e67, word_4ebb,         0x54);
DG_ASSERT_AT(struct dg_4e67, round_number,      0x56);
DG_ASSERT_AT(struct dg_4e67, playing,           0x58);
DG_ASSERT_AT(struct dg_4e67, master_level,      0x5a);
DG_ASSERT_AT(struct dg_4e67, saved_cursor,      0x5c);
DG_ASSERT_AT(struct dg_4e67, cursor,            0x5e);
DG_ASSERT_AT(struct dg_4e67, icons_bmp_ptr,     0x60);
DG_ASSERT_AT(struct dg_4e67, menu_bmp_ptr,      0x62);
DG_ASSERT_AT(struct dg_4e67, bmp_4ecb_ptr,      0x64);
DG_ASSERT_AT(struct dg_4e67, score2_bmp_ptr,    0x66);

/*
 * **The pointer and its buttons, as the guest sees them**, at DGROUP 0x5768.
 */
struct dg_5768 {
    int16_t   button_accum_a;     /* +0x00  the two the timer handler accumulates into */
    int16_t   button_accum_b;     /* +0x02 */
    int16_t   cursor_y;           /* +0x04  the live pointer `timer_callback` moves and clamps to the
                                     screen, y first as the original files it: this is what the
                                     cursor is drawn at, and `wait_and_latch_frame` copies the pair
                                     into `pointer_x`/`pointer_y` below for the frame's regions */
    int16_t   cursor_x;           /* +0x06 */
    dg_near_t cursor_bitmap_ptr;  /* +0x08  the mouse cursor's bitmap, 0 for none - set_cursor */
    uint16_t  button_right;       /* +0x0a  2 is a click; the intro leaves on either button */
    uint16_t  button_left;        /* +0x0c  2 is a click - the word every region reads */
    /* **The pointer as the driver last reported it** - the mouse driver's own
       x and y, or a copy of `cursor_x`/`cursor_y` when the driver is not being
       read. **Nothing reads them**: the two bytes of each offset occur exactly
       twice in the image and both are this store, so they are named by address
       because there is nothing else to name them from. */
    uint16_t  word_5776;       /* +0x0e  y */
    uint16_t  word_5778;       /* +0x10  x */
    /* **A pointer move waiting to be made**, x and y, which `redraw_cursor_all`
       performs and clears. Nothing in the image ever stores a non-zero pair
       here - three references each, all in that one routine - so the request
       is never made; the routine that would make it is transcribed whole. */
    uint16_t  pending_move_y;  /* +0x12 */
    uint16_t  pending_move_x;  /* +0x14 */
    /* **The cursor bitmap's hot spot**, subtracted from the pointer to place
       the bitmap - `draw_cursor` does `cursor_x - hot_x, cursor_y - hot_y` -
       and the keyboard's pointer steps clamp against the same pair. Which way
       round was measured rather than read: with the hourglass up the bitmap is
       16 by 20 and the pair is 8 and 10. `set_cursor` takes them in this
       order and zeroes both when the cursor is turned off, so an old offset
       cannot outlive its bitmap. */
    int16_t   hot_y;           /* +0x16 */
    int16_t   hot_x;           /* +0x18 */
    int16_t   pointer_y;          /* +0x1a  regions_handle_pointer tests a record's +8 and +0x0c */
    int16_t   pointer_x;          /* +0x1c  against these, and its +6 and +0x0a against x */
    /* **How far the palette should be faded** towards the colour, the weight
       `fade_palette_run` takes; `MACHINE_PALETTE_FADE.fade_mark` is how far it
       has been, and `redraw_cursor_all` runs a fade whenever the two differ.
       Nothing in the image writes it - four references, all reads - so it
       holds what the image put there and the fade the two would drive never
       runs. */
    uint16_t  fade_weight;     /* +0x1e */
} __attribute__((packed));

extern struct dg_5768 DG5768;

DG_ASSERT_AT(struct dg_5768, button_accum_a,    0x00);
DG_ASSERT_AT(struct dg_5768, button_accum_b,    0x02);
DG_ASSERT_AT(struct dg_5768, cursor_y,          0x04);
DG_ASSERT_AT(struct dg_5768, cursor_x,          0x06);
DG_ASSERT_AT(struct dg_5768, cursor_bitmap_ptr, 0x08);
DG_ASSERT_AT(struct dg_5768, button_right,      0x0a);
DG_ASSERT_AT(struct dg_5768, button_left,       0x0c);
DG_ASSERT_AT(struct dg_5768, word_5776,         0x0e);
DG_ASSERT_AT(struct dg_5768, word_5778,         0x10);
DG_ASSERT_AT(struct dg_5768, pending_move_y,         0x12);
DG_ASSERT_AT(struct dg_5768, pending_move_x,         0x14);
DG_ASSERT_AT(struct dg_5768, hot_y,         0x16);
DG_ASSERT_AT(struct dg_5768, hot_x,         0x18);
DG_ASSERT_AT(struct dg_5768, pointer_y,         0x1a);
DG_ASSERT_AT(struct dg_5768, pointer_x,         0x1c);
DG_ASSERT_AT(struct dg_5768, fade_weight,         0x1e);

/*
 * **The structure the routine at 0x002be walks**, at DGROUP 0x53fc.
 */
struct dg_53fc {
    /* **The collision sweep's own block**, and the two parts it is working on.
       `resolve_collisions` sets `list_ptr` from `pick_by_flag` and walks every
       other part into `other_ptr`; `compute_swept_bounds_5400` fills the first
       part's boxes and `compute_bounds_53fe` the second's, and the overlap
       tests below read them as two rectangles. A block rather than locals
       because the original's routines take no arguments and reach these by
       offset - which is why DGROUP has to be memory. */
    /* The part `list_ptr` was already touching when the sweep began: copied
       out of its `contact_ptr`, tested for zero as "there was one", and put
       into `other_ptr` to be retried first. `angles_same_side` answers no
       while it is zero. */
    int16_t   contact_ptr;        /* +0x00 */
    dg_near_t other_ptr;          /* +0x02  the part `list_ptr` is being tested against */
    dg_near_t list_ptr;           /* +0x04  the part the collision sweep is
                                             working on; `resolve_collisions`
                                             sets it from `pick_by_flag` and
                                             every routine below reads part
                                             fields out of it */
    /* **How far `list_ptr` moved this frame**, current position less previous,
       which `compute_swept_bounds_5400` then adds to the far edges as an
       absolute value. */
    int16_t   moved_y;            /* +0x06 */
    /* **`other_ptr`'s box**, from `compute_bounds_53fe`: its position, its
       position plus its size, and the two centres - the halving an arithmetic
       shift, so a negative extent rounds down rather than toward zero. */
    int16_t   other_mid_y;        /* +0x08 */
    int16_t   other_mid_x;        /* +0x0a */
    int16_t   other_bottom;       /* +0x0c */
    int16_t   other_top;          /* +0x0e */
    int16_t   other_right;        /* +0x10 */
    int16_t   other_left;         /* +0x12 */
    /* **`list_ptr`'s swept box**: where it is and where it was, in one
       rectangle. The near edges fall back to the previous position when that
       was further back and the far edges are pushed out by the distance
       moved, which is what a dirty-rectangle redraw has to repaint. */
    int16_t   swept_top;          /* +0x14 */
    int16_t   swept_left;         /* +0x16 */
    int16_t   moved_x;            /* +0x18 */
    /* The centres of `list_ptr`'s box before the sweep stretched it. */
    int16_t   mid_y;              /* +0x1a */
    int16_t   mid_x;              /* +0x1c */
    int16_t   swept_bottom;       /* +0x1e */
    /* Where `list_ptr` is this frame - the corner the swept box starts from. */
    int16_t   cur_y;              /* +0x20 */
    int16_t   swept_right;        /* +0x22 */
    int16_t   cur_x;              /* +0x24 */
    /* **The contact being argued about**: the angle copied out of `list_ptr`'s
       `contact_angle` and the quadrant `angle_to_quadrant` puts it in.
       `angles_same_side` refuses any angle from another quadrant before it
       compares. */
    int16_t   contact_quadrant;   /* +0x26 */
    int16_t   contact_angle;      /* +0x28 */
    /* **Which way `list_ptr` is travelling**, `object_delta_angle` of its last
       two positions, refreshed at every step of the search. */
    int16_t   travel_angle;       /* +0x2a */
    /* Ours in name only: the password field's cursor blink, stepped every time
       the puzzle picker redraws the line and showing a star while bit 3 is
       set. */
    uint16_t  password_blink;     /* +0x2c */
    int16_t   selected_level;     /* +0x2e  the puzzle picker's row; game_round copies it to round_number */
    /* **The first puzzle the list shows**, from `puzzle_page_of_score`, and
       the arrows page it by 0x15 - the twenty-one rows a page holds - with 1
       as the floor and the level count as the ceiling. */
    int16_t   puzzle_page;        /* +0x30 */
} __attribute__((packed));

extern struct dg_53fc DG53FC;

DG_ASSERT_AT(struct dg_53fc, contact_ptr,       0x00);
DG_ASSERT_AT(struct dg_53fc, other_ptr,         0x02);
DG_ASSERT_AT(struct dg_53fc, list_ptr,          0x04);
DG_ASSERT_AT(struct dg_53fc, moved_y,           0x06);
DG_ASSERT_AT(struct dg_53fc, other_mid_y,       0x08);
DG_ASSERT_AT(struct dg_53fc, other_mid_x,       0x0a);
DG_ASSERT_AT(struct dg_53fc, other_bottom,      0x0c);
DG_ASSERT_AT(struct dg_53fc, other_top,         0x0e);
DG_ASSERT_AT(struct dg_53fc, other_right,       0x10);
DG_ASSERT_AT(struct dg_53fc, other_left,        0x12);
DG_ASSERT_AT(struct dg_53fc, swept_top,         0x14);
DG_ASSERT_AT(struct dg_53fc, swept_left,        0x16);
DG_ASSERT_AT(struct dg_53fc, moved_x,           0x18);
DG_ASSERT_AT(struct dg_53fc, mid_y,             0x1a);
DG_ASSERT_AT(struct dg_53fc, mid_x,             0x1c);
DG_ASSERT_AT(struct dg_53fc, swept_bottom,      0x1e);
DG_ASSERT_AT(struct dg_53fc, cur_y,             0x20);
DG_ASSERT_AT(struct dg_53fc, swept_right,       0x22);
DG_ASSERT_AT(struct dg_53fc, cur_x,             0x24);
DG_ASSERT_AT(struct dg_53fc, contact_quadrant,  0x26);
DG_ASSERT_AT(struct dg_53fc, contact_angle,     0x28);
DG_ASSERT_AT(struct dg_53fc, travel_angle,      0x2a);
DG_ASSERT_AT(struct dg_53fc, password_blink,    0x2c);
DG_ASSERT_AT(struct dg_53fc, selected_level,    0x2e);
DG_ASSERT_AT(struct dg_53fc, puzzle_page,       0x30);

/*
 * **One entry of the table `bank_ptr` points at**: two bytes per index -
 * `start_on_free_voice` doubles the index with `shl ax,1` - read one at a time
 * through AL and never moved as a word. The names are guesses from what the
 * sequencer does with the two voice bytes they land in: `step_sequence` loops a
 * finished sequence instead of removing it while +0x15d is set, and
 * `start_sequence` orders the playing table by +0x15c, descending.
 */
struct sound_bank_entry {
    uint8_t loop;              /* +0x00  -> voice +0x15d */
    uint8_t priority;          /* +0x01  -> voice +0x15c */
} __attribute__((packed));
DG_ASSERT_AT(struct sound_bank_entry, loop,              0x00);
DG_ASSERT_AT(struct sound_bank_entry, priority,          0x01);

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
    /* **Two timer handles, not a far pointer.** `timer_add_callback` answers a
       slot number, and these are the two the sound module holds - the
       sequencer's tick at SNDCS:0x193e and the loaded module's at
       IMAGE_BASE:0xbba6. They are set, tested and dropped one at a time and
       are never paired into an address; the field was a `struct far_ptr`
       until 2026-09-18, which said the opposite of what the code does. */
    int16_t   tick_handle;        /* +0x0c  the sequencer's */
    int16_t   module_handle;      /* +0x0e  the loaded module's */
    dg_near_t bank_ptr;           /* +0x10  a table of struct sound_bank_entry,
                                            what a voice's +0x15c and +0x15d
                                            come out of */
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
    dg_near_t file_ptr;           /* +0x24  the file this module opened, if it did */
    uint16_t  file_kind;          /* +0x26  recorded beside the handle */
    uint16_t  module_live;        /* +0x28  the module is loaded and a callback exists */
    uint16_t  bank_choice;        /* +0x2a  chooses between load_sound_bank and its sibling */
    uint16_t  device;             /* +0x2c  the device number; 8 is recorded as 3 */
} __attribute__((packed));

extern struct dg_4a82 DG4A82;

DG_ASSERT_AT(struct dg_4a82, driver_number,     0x00);
DG_ASSERT_AT(struct dg_4a82, config,            0x02);
DG_ASSERT_AT(struct dg_4a82, records,           0x06);
DG_ASSERT_AT(struct dg_4a82, timer_taken,       0x0a);
DG_ASSERT_AT(struct dg_4a82, tick_handle,       0x0c);
DG_ASSERT_AT(struct dg_4a82, module_handle,     0x0e);
DG_ASSERT_AT(struct dg_4a82, bank_ptr,          0x10);
DG_ASSERT_AT(struct dg_4a82, driver,            0x12);
DG_ASSERT_AT(struct dg_4a82, module,            0x16);
DG_ASSERT_AT(struct dg_4a82, load_error,        0x1a);
DG_ASSERT_AT(struct dg_4a82, identifier,        0x1c);
DG_ASSERT_AT(struct dg_4a82, voice_word,        0x1e);
DG_ASSERT_AT(struct dg_4a82, directory,         0x20);
DG_ASSERT_AT(struct dg_4a82, file_ptr,          0x24);
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
    /* **The colour the parts bin's column is cleared to**, 0x0b, filed once by
       `game_setup` and read only by `draw_machine_layer_a`, which puts it in
       both of the driver's fill colours before its two `fill_rect`s. */
    int16_t   bin_colour;      /* +0x0c */
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
    /* **The font handle for "memofnt8.fnt"**, what `load_font` answered at
       start-up; `set_font` takes it and `game_teardown` gives its slot back. */
    int16_t   memo_font;       /* +0x22 */
    struct far_ptr pal_black_ptr; /* +0x24  black.pal, as pal_tim_ptr */
    struct far_ptr pal_sierra_ptr;/* +0x28  sierra.pal */
} __attribute__((packed));

extern struct dg_52bd DG52BD;

DG_ASSERT_AT(struct dg_52bd, band_x,            0x00);
DG_ASSERT_AT(struct dg_52bd, band_y,            0x02);
DG_ASSERT_AT(struct dg_52bd, anchor_x,          0x04);
DG_ASSERT_AT(struct dg_52bd, anchor_y,          0x06);
DG_ASSERT_AT(struct dg_52bd, band_colour,       0x08);
DG_ASSERT_AT(struct dg_52bd, drop_cursor,       0x0a);
DG_ASSERT_AT(struct dg_52bd, bin_colour,         0x0c);
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
DG_ASSERT_AT(struct dg_52bd, memo_font,         0x22);
DG_ASSERT_AT(struct dg_52bd, pal_black_ptr,     0x24);
DG_ASSERT_AT(struct dg_52bd, pal_sierra_ptr,    0x28);

/*
 * **The palettes, the last key, and the art sets**, at DGROUP 0x52ed.
 */
struct dg_52ed {
    /*
     * +0x00  tim.pal: the far pointer `load_palette` answers, stored whole and
     * read whole by `set_palette_pointer` and `free_far_block`.
     */
    struct far_ptr pal_tim_ptr;
    uint8_t   last_key;           /* +0x04  the last key the screen loops took - a **byte**, which
                                   * the assert caught: 0x52f2 follows it at +0x05 */
    uint16_t  cursor_follows;     /* +0x05  restore_cursor_following is guarded by this */
    dg_near_t panel_art_ptr;     /* +0x07  the art set the panel's pieces come out of */
    dg_near_t cursor_art_ptr;    /* +0x09  mouse.bmp's list */
    dg_near_t tim_sx_ptr;         /* +0x0b  tim.sx's file record, which open_sound_file reads the sounds from */
    uint16_t  stop_requested;     /* +0x0d  game_teardown(0) raises it; the loops above read it */
    uint16_t  stack_floor;        /* +0x0f  what the stack is reserved below */
} __attribute__((packed));

extern struct dg_52ed DG52ED;

/*
 * **The machine file the picker chose**, at DGROUP 0x52fe. `pick_file` copies
 * its answer here and `load_animation` and `save_machine` read it back.
 *
 * Thirteen bytes: `dg_52ed` ends at 0x52fe and `game_directories` begins thirteen on,
 * and the picker's own buffer is capped at thirteen by the `0x0d` it hands
 * `picker_type`.
 */
struct dg_52fe {
    char      name[0xd];          /* +0x00 */
} __attribute__((packed));

extern struct dg_52fe DG52FE;

DG_ASSERT_AT(struct dg_52fe, name,              0x00);

DG_ASSERT_AT(struct dg_52ed, pal_tim_ptr,       0x00);
DG_ASSERT_AT(struct dg_52ed, last_key,          0x04);
DG_ASSERT_AT(struct dg_52ed, cursor_follows,    0x05);
DG_ASSERT_AT(struct dg_52ed, panel_art_ptr,     0x07);
DG_ASSERT_AT(struct dg_52ed, cursor_art_ptr,    0x09);
DG_ASSERT_AT(struct dg_52ed, tim_sx_ptr,        0x0b);
DG_ASSERT_AT(struct dg_52ed, stop_requested,    0x0d);
DG_ASSERT_AT(struct dg_52ed, stack_floor,       0x0f);

/*
 * ---------------------------------------------------------------------------
 * **A shape**, one of the 180 twenty-four-byte blocks `game_startup` takes
 * from DOS and chains through their own first four bytes. A shape is a piece
 * of the screen something was drawn over, kept until it has been put back:
 * `alloc_shape` pops one off the free list and fills it in, `replay_shapes`
 * walks the used list, redraws each and pops it back.
 *
 * The block is 0x18 bytes and every one of them is accounted for here.
 * ---------------------------------------------------------------------------
 */
struct shape {
    struct far_ptr next;       /* +0x00  the link, in the block's own first four bytes */
    uint8_t        flags;      /* +0x04  bit 0 the second fill colour, bit 2 a belt length */
    uint8_t        replays;    /* +0x05  how many passes of `replay_shapes` it waits for:
                                         1 or 2 at every call site, stepped down once per
                                         pass, and put back and freed when it reaches 0 -
                                         so a 2 sits out one pass. `alloc_shape` reads the
                                         same byte a second way, a 1 being the c origin and
                                         anything else the b origin */
    int16_t        x1;         /* +0x06  the first point, less that origin */
    int16_t        y1;         /* +0x08 */
    int16_t        x2;         /* +0x0a  the second point - an extent, without bit 2 */
    int16_t        y2;         /* +0x0c */
    int16_t        width;      /* +0x0e  a belt's width; half of it widens the box below */
    int16_t        left;       /* +0x10  the box the dirty-rectangle tests read */
    int16_t        right;      /* +0x12 */
    int16_t        top;        /* +0x14 */
    int16_t        bottom;     /* +0x16 */
} __attribute__((packed));

_Static_assert(sizeof(struct shape) == 0x18, "game_startup allocates 0x18 bytes");
DG_ASSERT_AT(struct shape, flags,             0x04);
DG_ASSERT_AT(struct shape, x1,                0x06);
DG_ASSERT_AT(struct shape, width,             0x0e);
DG_ASSERT_AT(struct shape, left,              0x10);
DG_ASSERT_AT(struct shape, bottom,            0x16);

#define SHAPE_PTR(fp) ((struct shape *)(void *)dg_far_ptr(fp))

/*
 * **The shape and part free lists**, at DGROUP 0x4e4e.
 */
struct dg_4e4e {
    struct far_ptr shape_free;    /* +0x00  the free list nodes come off */
    struct far_ptr shapes;        /* +0x04  the shapes drawn over, put back in reverse.
                                            **One far pointer**, not two near ones: the
                                            offset is at +0x04 and the segment at +0x06,
                                            which is what `alloc_shape` files there */
    dg_near_t parts_free_ptr;     /* +0x08  the head; 0x4e58 is the queue folded onto it */
    dg_near_t parts_queue_ptr;    /* +0x0a  what asked to move this frame */
    char      name_buf[0xd];      /* +0x0c  pick_file fills this and copies the
                                     answer out; `validate_filename` reads it
                                     back. Thirteen bytes - an 8.3 name and its
                                     NUL - which is what is left before
                                     `dg_4e67` */
} __attribute__((packed));

extern struct dg_4e4e DG4E4E;

DG_ASSERT_AT(struct dg_4e4e, shape_free,        0x00);
DG_ASSERT_AT(struct dg_4e4e, shapes,            0x04);
DG_ASSERT_AT(struct dg_4e4e, parts_free_ptr,    0x08);
DG_ASSERT_AT(struct dg_4e4e, parts_queue_ptr,   0x0a);
DG_ASSERT_AT(struct dg_4e4e, name_buf,          0x0c);

/*
 * **The level's own settings and its two bonus counters**, at DGROUP 0x50af.
 */
struct dg_50af {
    int16_t   bonus_1;            /* +0x00  the left bonus counter on the play screen, drawn at x 0x184;
                                     `step_counters` walks it down into the 32-bit score at 0x4ead
                                     first, and `finish_level` adds the two as words for the total */
    int16_t   bonus_2;            /* +0x02  the counter to its right, drawn at x 0x238. **Never walked
                                     down**: `step_counters` guards it on a state its only caller
                                     excludes, and nothing arms its scroll while it holds a value -
                                     so it shows the level's second bonus and stays there until the
                                     teardown zeroes it. `finish_level` still adds it to the first */
    int16_t   gravity;            /* +0x04  the knob's x is this * 0xa0 / 0x80 + 0x3d, as a long */
    int16_t   air;                /* +0x06  and this one * 0xa0 / 0x200 + 0x3d - a different divisor */
    int16_t   extent_y;           /* +0x08  the level's own extent, -8 for a machine with none */
    int16_t   extent_x;           /* +0x0a */
    int16_t   tune;               /* +0x0c  the level's tune; game_round reads it back from here */
    uint16_t  flip_options;       /* +0x0e  part_flip_options' answer, kept for the handles */
} __attribute__((packed));

extern struct dg_50af DG50AF;

DG_ASSERT_AT(struct dg_50af, bonus_1,           0x00);
DG_ASSERT_AT(struct dg_50af, bonus_2,           0x02);
DG_ASSERT_AT(struct dg_50af, gravity,           0x04);
DG_ASSERT_AT(struct dg_50af, air,               0x06);
DG_ASSERT_AT(struct dg_50af, extent_y,          0x08);
DG_ASSERT_AT(struct dg_50af, extent_x,          0x0a);
DG_ASSERT_AT(struct dg_50af, tune,              0x0c);
DG_ASSERT_AT(struct dg_50af, flip_options,      0x0e);

/*
 * **The timer's own state**, at DGROUP 0x44ee.
 */
struct timer {
    uint8_t   installed;          /* +0x00  the flag that says the handler is in; 0x4a8c records who */
    volatile int16_t frame_budget; /* +0x01  counts down from 0x2710; every frame spin waits on it.
                                     **volatile**: `timer_tick` writes it on the timer thread and the
                                     spin reads it with nothing between the reads */
    /* **The divisor programmed into the 8253**, 0xffff / rate, kept beside the
       rate itself. */
    int16_t   divisor;         /* +0x03 */
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

extern struct timer TIMER;

DG_ASSERT_AT(struct timer, installed,         0x00);
DG_ASSERT_AT(struct timer, frame_budget,      0x01);
DG_ASSERT_AT(struct timer, divisor,         0x03);
DG_ASSERT_AT(struct timer, divider_reload,    0x05);
DG_ASSERT_AT(struct timer, divider,           0x07);
DG_ASSERT_AT(struct timer, slot_mask,         0x09);
DG_ASSERT_AT(struct timer, callback,          0x0b);
DG_ASSERT_AT(struct timer, tick,              0x4b);
_Static_assert(sizeof(struct timer) == 0x8b, "the timer state ends at 0x4579");

/*
 * **The wrapped text's line starts**, DGROUP 0x56a6..0x56b8, 0x12 bytes - a near pointer into
 * the caller's own string for each line `wrap_text_to_box` decided on, and
 * `GAME_PICKER_TEXT.line_count` of them.
 *
 * Nine words, settled from three directions that agree. The wrapper caps the
 * box at seven line heights, so seven lines can start inside it and one more
 * is written before the height is re-tested; `draw_wrapped_text` finds a
 * line's end by reading the *next* entry, so the table needs one past the
 * last; and the saved-rectangle slots begin at 0x56b8, which is nine words on.
 */
struct game_text_lines {
    dg_near_t line_ptr[9];        /* +0x00 [0x12] */
} __attribute__((packed));

extern struct game_text_lines GAME_TEXT_LINES;
_Static_assert(sizeof(struct game_text_lines) == 0x12, "DGROUP 0x56a6..0x56b8, 0x12 bytes");
DG_ASSERT_AT(struct game_text_lines, line_ptr,          0x00);



/*
 * **The drawing re-entry guard and the frame flag**, at DGROUP 0x5752.
 */
struct dg_5752 {
    uint16_t  guard;              /* +0x00  raised across a redraw and put back; a nesting guard, not a lock */
    volatile int16_t frame_flag;  /* +0x02  what wait_and_latch_frame spins on, set by the INT 08h handler
                                     - on the timer thread, which is why this one is **volatile** */
    int16_t   size_word;          /* +0x04  the size, or the driver's own if this is zero */
} __attribute__((packed));

extern struct dg_5752 DG5752;

DG_ASSERT_AT(struct dg_5752, guard,             0x00);
DG_ASSERT_AT(struct dg_5752, frame_flag,        0x02);
DG_ASSERT_AT(struct dg_5752, size_word,         0x04);

/*
 * **The belt's far end and the goal tests' state**, at DGROUP 0x5456.
 */
struct dg_5456 {
    dg_near_t belt_far_end_ptr;   /* +0x00  the far end's +0x5a, stashed while it is detached */
    /* **Ten words the goal tests keep between frames**, and what each means
       depends on the test. `goal_test_puzzles_19_48` at 0x01bb4 does
       `inc word ptr [0x5458]` - a count of frames the goal has held, and
       passing 0xc wins. `goal_test_puzzle_78` at 0x015bf does
       `cmp word ptr [bx + 0x5458], 0` with `bx` twice the mouse-cage count: a
       flag per cage. The ten is `clear_machine`'s, which zeroes them with
       `cmp si, 0xa / jl` over a word stride; nothing yet says the table is no
       longer than that. */
    uint16_t  goal_condition[10]; /* +0x02 .. +0x15 */
} __attribute__((packed));

extern struct dg_5456 DG5456;

DG_ASSERT_AT(struct dg_5456, belt_far_end_ptr,  0x00);
DG_ASSERT_AT(struct dg_5456, goal_condition,    0x02);

/*
 * **The Borland heap and its two stream flags**, at DGROUP 0x4e34.
 */
struct dg_4e34 {
    dg_near_t first_block_ptr;    /* +0x00  the block chain runs from here to the topmost */
    dg_near_t top_block_ptr;      /* +0x02  which is where a new block is cut from */
    dg_near_t ring_cursor_ptr;    /* +0x04  first fit walks *backward* from here */
    uint8_t   cr[2];              /* +0x06  "\r", which `borland_fputc` writes before a newline in text mode */
    int16_t   stdin_is_tty;       /* +0x08  the two flags remembering what isatty said */
    int16_t   stdout_is_tty;      /* +0x0a */
    dg_near_t realcvt_ptr;        /* +0x0c  0x4e40: where `%e`, `%f` and `%g` go -
                                     `float_formats_missing` in this program */
} __attribute__((packed));

extern struct dg_4e34 DG4E34;

DG_ASSERT_AT(struct dg_4e34, first_block_ptr,   0x00);
DG_ASSERT_AT(struct dg_4e34, top_block_ptr,     0x02);

/*
 * **A near-heap block header**: the four bytes *below* every pointer
 * `heap_malloc` answers, which is why `heap_free` takes 4 off before it looks.
 * The size is always even and its low bit is the in-use flag - set by `inc`,
 * cleared by `dec`, and masked off with 0xfffe wherever the size is walked.
 * `prev_ptr` is the block below by address, for coalescing; the chain runs
 * from `DG4E34.first_block_ptr` up to `top_block_ptr`.
 *
 * The two ring links exist only while the block is free: they are the first
 * four bytes of its own payload, which is why nothing smaller than eight
 * bytes is ever cut, and why a block in use has them overwritten by whatever
 * the caller stored. `DG4E34.ring_cursor_ptr` is where the ring is entered
 * and first fit walks it through `back_ptr`. Layout from the disassembly of
 * Borland's allocator at 0x0c8ca..0x0cbdd; the header only, not the game.
 */
struct heap_block {
    uint16_t  size;            /* +0x00  even; bit 0 is the in-use flag */
    dg_near_t prev_ptr;        /* +0x02  the block below, by address */
    dg_near_t fwd_ptr;         /* +0x04  free ring, forward - payload otherwise */
    dg_near_t back_ptr;        /* +0x06  free ring, backward - payload otherwise */
} __attribute__((packed));

#define HEAPBLK_PTR(p) ((struct heap_block *)(dgroup + (uint16_t)(p)))

/*
 * **What `heapwalk` fills in**, Borland's `struct heapinfo`: the block's payload
 * - the pointer `malloc` answered, kept as the near pointer the original keeps -
 * its size without the in-use bit, and that bit on its own. The walk takes the
 * pointer back to find the next block, so a zero one starts from the first.
 */
struct heapinfo {
    dg_near_t block_ptr;       /* +0x00 */
    uint16_t  size;            /* +0x02 */
    uint16_t  in_use;          /* +0x04 */
} __attribute__((packed));

/* **No heap block**, as a pointer - see `PART_NONE`. */
#define HEAPBLK_NONE HEAPBLK_PTR(0)

/* **`heap_sbrk` refused**, as a pointer: the -1 the original answers in AX,
   `mov ax,0xffff` at 0x0c812, kept as DGROUP:FFFF rather than turned into NULL,
   for the reason `PART_NONE` gives. */
#define HEAP_SBRK_FAIL ((uint8_t *)(dgroup + 0xffff))

DG_ASSERT_AT(struct heap_block, size,              0x00);
DG_ASSERT_AT(struct heap_block, prev_ptr,          0x02);
DG_ASSERT_AT(struct heap_block, fwd_ptr,           0x04);
DG_ASSERT_AT(struct heap_block, back_ptr,          0x06);
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
struct point8 {
    uint8_t x;                 /* +0x00 */
    uint8_t y;                 /* +0x01 */
} __attribute__((packed));

/*
 * **A part's point table**: a run of `point8` at a constant DGROUP offset,
 * which the `part_setup_*` routines copy into the part's own `points_ptr`.
 * They are of different lengths and are not one array - and one routine reads
 * its table's address out of a table of addresses indexed by the part's form -
 * so the offset stays at the call site and only its *type* is stated here.
 */
/* Not `volatile`, and neither is `POINT16_TABLE` below: a point table is
   constant data in the image - the `const` says nothing writes it - and the
   timer handler reaches no part or part data at all. See `PARTP` in full. */
#define POINT_TABLE(off) \
    ((const struct point8 *)(dgroup + (uint16_t)(off)))

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
    ((const dg_near_t *)(dgroup + (uint16_t)(off)))

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
    uint8_t pad_49b9[1];    /* +0x53  0x49b9 */
} __attribute__((packed));

/* **Not `volatile`.** These are the compiler's string literals; nothing
   writes them. The two buffers among them are filled by `string_copy_far`,
   which takes an offset, so even those are not written through this. */
extern struct chunk_names CHUNK;

DG_ASSERT_AT(struct chunk_names, bmp_inf,           0x00);
DG_ASSERT_AT(struct chunk_names, bmp_bin,           0x09);
DG_ASSERT_AT(struct chunk_names, bmp_vga,           0x14);
DG_ASSERT_AT(struct chunk_names, bmp_amg,           0x1d);
DG_ASSERT_AT(struct chunk_names, scr_dim,           0x28);
DG_ASSERT_AT(struct chunk_names, scr_bin,           0x31);
DG_ASSERT_AT(struct chunk_names, scr_vga,           0x3c);
DG_ASSERT_AT(struct chunk_names, scr_amg,           0x45);
DG_ASSERT_AT(struct chunk_names, mode_rb,           0x50);
_Static_assert(sizeof(struct chunk_names) == 0x54,
               "the first run ends at 0x49ba, where DG49BA's code pointers begin");

/*
 * **The rest of the chunk names**, from 0x49c6, after the twelve bytes of
 * `DG49BA` - another module's data sharing the region, three code pointers
 * the offset-table bitmap draws through. The run was one struct with those
 * twelve bytes as a gap in it until the data became objects the linker places,
 * and two objects cannot share bytes.
 */
struct chunk_names2 {
    char bmp_scn[9];        /* +0x00  0x49c6  "BMP:SCN:" */
    char bmp_off[9];        /* +0x09  0x49cf  "BMP:OFF:" */
    char bmp_vqt[9];        /* +0x12  0x49d8  "BMP:VQT:" */
    char bmp_off_b[9];      /* +0x1b  0x49e1  "BMP:OFF:" - the second copy */
    char bmp_rle[9];        /* +0x24  0x49ea  "BMP:RLE:" */
    char bmp_scl[9];        /* +0x2d  0x49f3  "BMP:SCL:" */
    uint8_t pad_49fc[2];
    char scr_vqt[9];        /* +0x38  0x49fe  "SCR:VQT:" */
    uint8_t pad_4a07[1];
    /* **The sound module's name template**, not only a constant:
       `load_sound_module` builds the name in place, writing the three digits
       at +4, +5 and +6 - hundreds, tens and units, each from its own division
       - over "000". */
    char ssm_000[9];        /* +0x42  0x4a08  "SSM:000:" */
    uint8_t pad_4a11[1];
    /* +0x4c  0x4a12. **Not a constant: a buffer.** The image holds
       `53 53 4d 3a 20 20 20 20 20 00` - "SSM:" and *five* spaces, which is
       nine characters and would fail the multiple-of-four check.
       `setup_sound_device` writes a four-character tag **and its NUL** over
       the spaces at +4 first, so the path is eight when it is walked and the
       fifth space is the room that NUL needs. */
    char ssm_tag[10];
} __attribute__((packed));

extern struct chunk_names2 CHUNK2;

DG_ASSERT_AT(struct chunk_names2, bmp_off,           0x09);
DG_ASSERT_AT(struct chunk_names2, bmp_vqt,           0x12);
DG_ASSERT_AT(struct chunk_names2, bmp_off_b,         0x1b);
DG_ASSERT_AT(struct chunk_names2, bmp_rle,           0x24);
DG_ASSERT_AT(struct chunk_names2, bmp_scl,           0x2d);
DG_ASSERT_AT(struct chunk_names2, scr_vqt,           0x38);
DG_ASSERT_AT(struct chunk_names2, ssm_000,           0x42);
DG_ASSERT_AT(struct chunk_names2, ssm_tag,           0x4c);
_Static_assert(sizeof(struct chunk_names2) == 0x56,
               "the run ends at 0x4a1c, where the device tag table begins");

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
    dg_near_t by_adapter[16];  /* +0x1c  0x44a2  which of the four, by shift */
    struct far_ptr palette_ptr; /* +0x3c  0x44c2  the palette `set_palette_pointer` last stored, answered back when it is passed a null */
    char pal_amg[9];          /* +0x40  0x44c6  "PAL:AMG:" */
} __attribute__((packed));

extern struct pal_chunk_names PALCHUNK;

DG_ASSERT_AT(struct pal_chunk_names, pal_vga,           0x00);
DG_ASSERT_AT(struct pal_chunk_names, pal_ega,           0x09);
DG_ASSERT_AT(struct pal_chunk_names, pal_cga,           0x12);
DG_ASSERT_AT(struct pal_chunk_names, none,              0x1b);
DG_ASSERT_AT(struct pal_chunk_names, by_adapter,        0x1c);
DG_ASSERT_AT(struct pal_chunk_names, pal_amg,           0x40);

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

extern struct ovl_chunk_names OVLCHUNK;

DG_ASSERT_AT(struct ovl_chunk_names, ovl_tag,           0x00);
DG_ASSERT_AT(struct ovl_chunk_names, adapter_tag,       0x0a);

/* The four-character tags the two buffers above are completed from. The
   tables hold offsets rather than the tags themselves, which is why these
   stay `OFF_TABLE` and not a run of `char[5]`. */
/*
 * **The adapter tags**, DGROUP 0x48fc..0x4919: "BAD:" and then twelve near
 * pointers to the tags `load_named_chunk` puts after "OVL:". The game indexes
 * `[si*2 + 0x48ff]` with `si` from 1 - the compiler folding the first index
 * into the base - so entry 1 is `tag[0]`, at 0x4901. The sixth points back at
 * "BAD:".
 */
struct adapter_tags {
    char      bad[5];              /* +0x00  0x48fc  "BAD:" */
    dg_near_t tag[12];             /* +0x05  0x4901  entries 1 to 12 */
} __attribute__((packed));
_Static_assert(sizeof(struct adapter_tags) == 0x1d, "the tags end at 0x4919, where OVL: starts");
extern struct adapter_tags ADAPTER_TAGS;

/*
 * **The sound device and module tags**, DGROUP 0x4a1c..0x4a82: nine near
 * pointers indexed by device, five by module, the fourteen tags they point
 * at, and two "r" modes `load_sound_driver` and `load_sound_module` open
 * with.
 */
struct sound_tags {
    dg_near_t device[9];           /* +0x00  0x4a1c  "STD:" "TAN:" "ADL:" ... */
    dg_near_t module[5];           /* +0x12  0x4a2e  "ASB:" "APS:" "ATD:" ... */
    char      tag[14][5];          /* +0x1c  0x4a38  the fourteen, in that order */
    char      mode_r_a[2];         /* +0x62  0x4a7e  "r" */
    char      mode_r_b[2];         /* +0x64  0x4a80  "r" */
} __attribute__((packed));
_Static_assert(sizeof(struct sound_tags) == 0x66, "the tags end at 0x4a82, where DG4A82 starts");
extern struct sound_tags SOUND_TAGS;

/* A signed 16-bit point: a part's position, box and size generations, a
   belt's and a rope's corners. Declared here because `struct part` is the
   first to hold one. */
struct point16 {
    int16_t x;                 /* +0x00 */
    int16_t y;                 /* +0x02 */
} __attribute__((packed));

/* The other pair a shape is filed with: its extent. `alloc_shape` takes an
   origin and an extent as two records of two words each. */
struct extent16 {
    int16_t width;             /* +0x00 */
    int16_t height;            /* +0x02 */
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
    /* **The next part on its list, and the previous.** A list's head is a
       part itself - `parts_bin`, `moving_parts`, `placed_parts`, each a whole
       `struct part` - and the game treats it as the record before the first:
       `insert_sorted` files the head's *address* into the first part's
       `prev_ptr`, and `unlink_part` writes `PART_PTR(prev_ptr)->next_ptr`
       without asking whether that names a head or a part. One `mov` for both
       in the original. A list ends on a `next_ptr` of 0 - `or si,si` at
       0x126e4 and 0x14d8c - and only `prev_ptr` leads back to a head. */
    dg_near_t next_ptr;        /* +0x00 */
    /* **The bin list's back-link, not padding.** `insert_sorted` writes it - and
       writes the next node's back to `rec` - and `bin_part_at_index` walks it
       to step backwards from the sentinel at 0x50d7. */
    dg_near_t prev_ptr;        /* +0x02 */
    uint16_t  kind;            /* +0x04  which of the fifty-odd components it is */
    uint16_t  flags_06;        /* +0x06  devdump prints these two as `f6` and `f8` */
    uint16_t  flags_08;        /* +0x08 */
    uint16_t  flags_0a;        /* +0x0a */
    /* **The form, and the two generations behind it** - the same
       three-generation shape as `pos`, `box` and `size` below, aged by
       `shift_state_history` with `form_prev2 = form_prev; form_prev = form`.
       Kept as three names rather than an array because `form` is read at
       several hundred sites and the older two at a handful: what reads them
       asks "has the form changed since last frame" (`form != form_prev`) or
       "has it been still for two" (`form_prev == form_prev2`). */
    uint16_t  form;            /* +0x0c  which shape a part with several is in */
    uint16_t  form_prev;       /* +0x0e */
    int16_t   form_prev2;      /* +0x10 */
    int16_t   direction;       /* +0x12  devdump prints it as `dir` */
    /* **A redraw countdown**: `mark_part_shapes` sets it to a count and the
       draw walks in `machine_draw.c` step it down once per pass, redrawing
       the part while it is not zero. The carried part is stepped separately
       and ahead of the rest. */
    uint8_t   redraw_count;    /* +0x14 */
    uint8_t   pad_15[1];
    /* **The position in 9-bit fixed point**, and the reason the momentum reads
       here are 32 bits wide. `reset_machine` loads `pos_x` into the first and
       `pos_y` into the second and shifts each left 9; the physics integrates
       them and the whole part is `>> 9`. **Each is one Borland `long`.** A
       routine storing one writes the high word from `q >> 16` and the low word
       from `q`, and that pair of moves is a 32-bit store rather than two
       fields - so there are no word halves to reach for. */
    int32_t   fx;              /* +0x16 */
    int32_t   fy;              /* +0x1a */
    /* **Three generations each of the position, the box and the size**, a
       `point16` triple apiece with the newest first. `shift_state_history`
       ages every triple with two 32-bit moves, `reset_machine` seeds the
       older two of the box and the size from the newest, and
       `add_record_shapes` hands generation 2 or 3 of the box and the size
       *together* to `alloc_shape` - which is what pairs the three. All three
       are only their triples, so every use names the generation it reads.

       **The position is only the triple**, so every use says which
       generation it reads: `pos[0]` is where the part is now - the grab box at
       +0x56 is added to it - `pos[1]` the generation before, and `pos[2]` the
       one before that, which the settle tests compare against `pos[0]` and
       `reset_machine` seeds from +0x8c/+0x8e. */
    struct point16 pos[3];         /* +0x1e  gen 1 at +0x1e, 2 at +0x22, 3 at +0x26 */
    /* **The box is only the triple too.** `box[0]` is the part's own box, the
       one the pointer is tested against; `box[1]` and `box[2]` are the older
       generations `shift_state_history` ages it into. */
    struct point16 box[3];         /* +0x2a  gen 1 at +0x2a, 2 at +0x2e, 3 at +0x32 */
    /* **The velocity, both axes**, in the same ninths as `fx`/`fy`:
       `integrate_object` does `fx += vel_x; fy += vel_y` and nothing else adds
       to either. The pair is clamped, halved on a bounce and rotated together
       by `rotate_point`; the cannon fires by writing both at once. */
    int16_t   vel_x;           /* +0x36 */
    int16_t   vel_y;           /* +0x38 */
    int16_t   weight;          /* +0x3a  devdump prints it as `wt` */
    /* **One 32-bit momentum**, and both spellings are the same four bytes -
       the same shape as `fx` above. `part_step_*` reads and writes it whole
       with `DG32(si + 0x3c)`; the halves are named because other routines
       store one at a time. A field that is only the low word here would be a
       two-byte read where the original makes a four-byte one, which is the
       defect that stopped three levels solving once already. */
    int32_t   momentum;        /* +0x3c  one Borland `long` */
    /* **The extent a flipped part mirrors within** - a name that is a guess.
       `place_object_for_draw` lays a mirrored hot point at `mirror_size.width
       - hot.x - size[0].width`, and the same for y. `make_part`,
       `reset_machine` and `read_record_fields` copy `size[0]` in;
       `carried_part_grow`, `carried_part_shrink` and `run_drag_frame` copy
       `set_size` in; `clone_part` copies it as one four-byte unit. Signed, as
       `size` is: nothing reads it unsigned. */
    struct extent16 mirror_size;   /* +0x40 */
    /* **A word each, not a byte.** The part builder at machine_draw.c writes
       both with a 16-bit move out of the kind table at 0x296e/0x2970, and
       `DG16(si + 0x44) >> 4` turns one into a cell count; the `DG8` sites that
       gave them a byte width earlier are reading the low half of a value that
       never gets that large. */
    /* **And the size**, as an extent: `size[0]` is the part's width and
       height now - one less than the width is what the setups lay out - and
       `size[1]`, `size[2]` the older generations. **Signed**, as the original
       reads them: every read of +0x44 or +0x46 that says anything about sign
       is a `sar` or a signed jump, and none is `shr`, `ja` or `jb`. */
    struct extent16 size[3];       /* +0x44  gen 1 at +0x44, 2 at +0x48, 3 at +0x4c */
    /* **The size the player set** - a name that is a guess. It starts as the
       template's, `carried_part_grow` and `carried_part_shrink` step it by
       0x10 within the kind's limits - comparing it `jle`/`jge`, signed -
       `set_object_extent` copies it into `size[0]`, and it is one of the
       fields a machine file saves and loads. */
    struct extent16 set_size;      /* +0x50 */
    /* The rope this part is tied to: the 0x38-byte record `heap_calloc_far`
       gives a kind-8 part, and both ends' parts point at it too. 0 when none. */
    dg_near_t rope_ptr;        /* +0x54 */
    struct point8 grab;        /* +0x56  the grab box's corner */
    /* **And the grab box's size**, one word used for both extents:
       `draw_part_selection` takes it as the width and, for a belt, as the
       height unless half the height is smaller. Only the kinds that are
       grabbed by a handle rather than by their body set it - the conveyor
       0x0e, the mouse cage 0x0c, and three more. */
    uint16_t  grab_size;       /* +0x58 */
    /* **Six links in one array.** `part_setup_2068` files four of them by
       direction and `part_setup_3de5` writes the last two, and three
       `part_step_*` routines walk `+0x5a + 2 * i` with **i from 4 to 6**,
       which reaches 0x62 and 0x64. `[0]` to `[3]` are the neighbours by
       direction - right, left, down, up - and `[4]`, `[5]` the pair
       `part_setup_3de5` turns into form bits. */
    dg_near_t link_ptr[6];     /* +0x5a */
    /* **The belt records this part is an end of**, indexed the same way as
       `link_ptr` above - `cut_belts` writes `+0x66 + 2 * slot`. A kind-0xa
       carrier's own belt is always the first. */
    dg_near_t belt_ptr[2];     /* +0x66 */
    /* **The two attachment offsets, a byte pair each.** Written a byte at a
       time by the setups - `part_setup_1105` puts half the width in the first
       and zero in the second - and read as a pair by the belt routines, which
       index them: `refresh_link_geometry` adds `+0x6a + 2 * slot` to the
       part's x and `+0x6b + 2 * slot` to its y. `reverse_link_ends` swaps the
       two pairs with one 16-bit move, which is what `attach[0]` and
       `attach[1]` say and what four separate bytes cannot. */
    struct point8 attach[2];   /* +0x6a */
    uint8_t   pad_6e[4];
    /* **Where this kind is held** - the point a gripper, a rope or the line
       drawn from a host reaches for, as an offset from the part's own
       position. `grab_distance` measures a gripper's own edge against
       `pos[0] + hold` and says so in as many words; `draw_part_extra` draws to
       the same point, and `link_objects_at_point` tests it against a box.
       Mirrored by the setups: the cannon puts it at 0x3e when bit 4 of +8 is
       set and at 1 when it is not, which is the same point on a part facing
       the other way. */
    struct point8 hold;        /* +0x72 */
    /* **The next part on each of the two drawing layers this part is filed
       on** - `link_record_into_buckets` writes `[i]` for the layer its
       kind's `refile_level[i]` names, and `layer_slot` keeps which layer `[0]`
       is, so the walkers pick the half that matches the layer they are on. */
    dg_near_t layer_next_ptr[2];   /* +0x74 */
    /* **The next part in a chain, and only after something builds one.** Five
       routines zero it on the head and then thread parts on by insertion -
       `collect_carried`, `link_nearby_objects`, `link_objects_in_range`,
       `link_objects_crossing` and `link_objects_at_point`. Everything else
       only walks it, so a step routine that reads it is reading whatever the
       last of those five left; it means nothing before one has run. */
    dg_near_t next_linked_ptr; /* +0x78 */
    /* **How far this part is from the one that collected it**, on each axis,
       and only `link_nearby_objects` ever writes them - the other four
       chain-builders leave whatever was there. The value is the smaller of the
       two edge distances with its sign kept: positive when this part is to the
       right of (or below) the collector with a gap between them, negative when
       it overlaps, and never zero, because the routine substitutes 1 and -1.
       The part hooks read them as the direction and the reach of a push. */
    int16_t   link_dx;         /* +0x7a */
    int16_t   link_dy;         /* +0x7c */
    /* **Which of the host's two slots this part sits in** - 0 or 1, the index
       into the host's `link_ptr[4]`/`link_ptr[5]`, where `link_ptr[4]` of this
       part names the host. `rehome_carried_part` reads it to empty the slot it
       is leaving and writes it when a new host is found. */
    uint8_t   host_slot;       /* +0x7e */
    /* **Which layer `layer_next_ptr[0]` is filed on** - the slot number
       `link_record_into_buckets` used for `[0]` - so the draw and refile walks
       can tell which of a part's two links belongs to the layer they are
       filling. */
    uint8_t   layer_slot;      /* +0x7f */
    uint16_t  point_count;     /* +0x80  raised to 4 across part_finish and put back to 1 */
    dg_near_t points_ptr;      /* +0x82  where a setup copies its connection points to */
    /* **The contact block.** `resolve_collisions` and `find_edge_contact` both
       reach it by taking the address `part + 0x84` and walking from there, and
       `apply_contact_friction` takes the same address off whichever part it
       was handed - which is why the five sit together and why the port used to
       call the address a `link`. +0x84 is the part being touched, the two
       bytes are cleared together, +0x88 goes to `angles_same_side`, and +0x8a
       gets the edge index the search stopped on. */
    dg_near_t contact_ptr;     /* +0x84  the part this one is in contact with */
    /* **Two flags that break a tie, and which is which is not settled.** Both
       are cleared when the contact is taken and one of the two is set from the
       swept test's `if (x0 > x1)` and `if (v > out[0])` - so they say which
       side of the contact the object came down on. The only readers are
       `bounce_off_contact` and `apply_contact_friction`, and both use them the
       same way: a `contact_angle` of 0 or 0x8000 - a flat edge, which gives no
       direction - is nudged by +0x1000 when `byte_86` is clear and by -0x1000
       when `byte_87` is. So one of them names each direction, and nothing this
       project has measured says which, which is why neither is named. */
    uint8_t   byte_86;         /* +0x86 */
    uint8_t   byte_87;         /* +0x87 */
    /* **The angle of the edge being touched**, written as the edge's own angle
       turned by 0x8000 - the normal pointing back at this part - and read by
       `angles_same_side`, `bounce_off_contact` and `apply_contact_friction`. */
    int16_t   contact_angle;   /* +0x88 */
    /* **Which edge of the other part it is**, the index the search stopped on
       (`i - 1` or `j - 1` of the point walk). The part hooks read it as the
       face they are resting against, and `(contact_edge + 4) & 7` for the
       opposite one. */
    uint16_t  contact_edge;    /* +0x8a */
    /* **The state a reset returns the part to**, and the state a machine file
       records. `reset_machine` copies all five back - the position into all
       three generations of `pos` and into `fx`/`fy`, and the other three
       straight - `write_part_list` writes them and `read_record_fields` reads
       them back, and the editor writes them when a part is put down or a
       settle hook finishes: every `part->form = X` in a settle is followed by
       `part->start_form = X`. `make_part` sets the position to -1,-1, which is
       what a part that has never been placed carries. */
    uint16_t  start_x;         /* +0x8c */
    uint16_t  start_y;         /* +0x8e */
    uint16_t  start_form;      /* +0x90 */
    uint16_t  start_direction; /* +0x92 */
    uint16_t  start_flags;     /* +0x94  the flags at +8 as they were placed */
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
/* **A part is an offset into DS, and 0 is an offset like any other.** The
   original holds a part as a near pointer - one word, an offset into DS - and
   the links at +0 and +2 are those words. The `dg_` structs name offsets in
   that segment: the three list heads are DS:0x50d7, DS:0x5179 and DS:0x521b,
   `write_level` hands each to `write_part_list` as one word, and the walk
   reads through it with the default DS. A list ends on a `next_ptr` of 0,
   and the original tests the *offset* for that - `or si,si` at 0x126e4,
   0x14d8c and 0x00f9b - so the port does too, never this pointer. DS:0 is
   the Borland banner, and where the original reads a part at 0 without a
   test, as `compute_link_endpoints` does for a rope's empty end, this reads
   the same bytes. A function rather than a macro so the offset is evaluated
   once. */
static inline struct part *PART_PTR(uint16_t p)
{
    return (struct part *)(dgroup + p);
}

/* **The end of a part list, as a pointer.** A walk held as a `struct part *`
   cannot end on NULL - `PART_PTR(0)` is DS:0, the Borland banner - so it ends
   on this, which is the same address the original's `or si,si` decides on. */
#define PART_NONE PART_PTR(0)

DG_ASSERT_AT(struct part, next_ptr,          0x00);
DG_ASSERT_AT(struct part, prev_ptr,          0x02);
DG_ASSERT_AT(struct part, kind,              0x04);
DG_ASSERT_AT(struct part, flags_06,          0x06);
DG_ASSERT_AT(struct part, flags_08,          0x08);
DG_ASSERT_AT(struct part, flags_0a,          0x0a);
DG_ASSERT_AT(struct part, form,              0x0c);
DG_ASSERT_AT(struct part, form_prev,         0x0e);
DG_ASSERT_AT(struct part, form_prev2,        0x10);
DG_ASSERT_AT(struct part, direction,         0x12);
DG_ASSERT_AT(struct part, redraw_count,      0x14);
DG_ASSERT_AT(struct part, fx,                0x16);
DG_ASSERT_AT(struct part, fy,                0x1a);
DG_ASSERT_AT(struct part, pos,               0x1e);
DG_ASSERT_AT(struct part, box,               0x2a);
DG_ASSERT_AT(struct part, vel_x,             0x36);
DG_ASSERT_AT(struct part, vel_y,             0x38);
DG_ASSERT_AT(struct part, weight,            0x3a);
DG_ASSERT_AT(struct part, momentum,          0x3c);
DG_ASSERT_AT(struct part, mirror_size,       0x40);
DG_ASSERT_AT(struct part, size,              0x44);
DG_ASSERT_AT(struct part, set_size,          0x50);
DG_ASSERT_AT(struct part, rope_ptr,          0x54);
DG_ASSERT_AT(struct part, grab,              0x56);
DG_ASSERT_AT(struct part, grab_size,         0x58);
DG_ASSERT_AT(struct part, link_ptr,          0x5a);
DG_ASSERT_AT(struct part, belt_ptr,          0x66);
DG_ASSERT_AT(struct part, attach,            0x6a);
DG_ASSERT_AT(struct part, hold,              0x72);
DG_ASSERT_AT(struct part, next_linked_ptr,   0x78);
DG_ASSERT_AT(struct part, layer_next_ptr,    0x74);
DG_ASSERT_AT(struct part, link_dx,           0x7a);
DG_ASSERT_AT(struct part, link_dy,           0x7c);
DG_ASSERT_AT(struct part, host_slot,         0x7e);
DG_ASSERT_AT(struct part, layer_slot,           0x7f);
DG_ASSERT_AT(struct part, point_count,       0x80);
DG_ASSERT_AT(struct part, points_ptr,        0x82);
DG_ASSERT_AT(struct part, contact_ptr,       0x84);
DG_ASSERT_AT(struct part, contact_edge,      0x8a);
DG_ASSERT_AT(struct part, start_x,           0x8c);
DG_ASSERT_AT(struct part, start_y,           0x8e);
DG_ASSERT_AT(struct part, start_form,        0x90);
DG_ASSERT_AT(struct part, start_direction,   0x92);
DG_ASSERT_AT(struct part, start_flags,       0x94);
DG_ASSERT_AT(struct part, word_96,           0x96);
DG_ASSERT_AT(struct part, word_98,           0x98);
DG_ASSERT_AT(struct part, word_9a,           0x9a);
DG_ASSERT_AT(struct part, spin,              0x9c);
DG_ASSERT_AT(struct part, word_9e,           0x9e);
DG_ASSERT_AT(struct part, word_a0,           0xa0);
_Static_assert(sizeof(struct part) == 0xa2,
               "a part is 0xa2 bytes - game.c reads `n` of them off the near heap");

/*
 * **The parts the game is holding on to**, at DGROUP 0x50d3.
 *
 * Here rather than in address order because `parts_bin` is a `struct part`,
 * and a member needs its type complete.
 */
struct dg_50d3 {
    dg_near_t bin_list_ptr;       /* +0x00  the list draw_bin walks; defaults to &parts_bin */
    dg_near_t dragged_part_ptr;   /* +0x02  the part being dragged - drawn last, and not counted */
    /* **The parts bin: a doubly linked list's head, and the head is a whole
       part.** The level file fills it last, with `n_given` - the tools
       handed to the player, belts, ropes, pulleys, bellows, measured with
       TIM_LEVELSCAN over twenty levels - and `build_part_list` fills it with
       one of every kind for freeform.

       **0xa2 bytes, not the two links.** The three heads sit exactly one part
       apart - 0x50d7, 0x5179, 0x521b, and `DG52BD` begins at 0x52bd - with
       nothing else declared between them, and the game reads a head through
       the part layout: `bin_list_ptr` is set to 0x50d7 (0x10d7f, 0x123ac,
       0x140f3) and `bin_part_at_index` reads `kind` through it.

       `next_ptr` and `prev_ptr` are the only fields written by name:
       `build_part_list` clears both (`xor ax,ax` at 0x14063), `free_all_lists`
       clears `next_ptr` (0x14d66), and `insert_sorted` files a head's address
       into its first part's `prev_ptr`. A head never points at itself. */
    struct part parts_bin;        /* +0x04 */
} __attribute__((packed));

extern struct dg_50d3 DG50D3;

DG_ASSERT_AT(struct dg_50d3, bin_list_ptr,      0x00);
DG_ASSERT_AT(struct dg_50d3, dragged_part_ptr,  0x02);
DG_ASSERT_AT(struct dg_50d3, parts_bin,         0x04);
_Static_assert(sizeof(struct dg_50d3) == 0x5179 - 0x50d3,
               "the bin's head part ends where the moving list's head begins");

/*
 * **The moving parts**, at DGROUP 0x5179.
 */
struct dg_5179 {
    /* **The moving parts: a doubly linked list's head**, the second list the
       level file fills (`n_moving`) - balls, balloons, buckets, rockets - and
       what gravity and the step passes walk. A whole part, read the way the
       bin's at 0x50d7 is; see `parts_bin`. */
    struct part moving_parts;     /* +0x00 */
} __attribute__((packed));

extern struct dg_5179 DG5179;

DG_ASSERT_AT(struct dg_5179, moving_parts,      0x00);
_Static_assert(sizeof(struct dg_5179) == 0x521b - 0x5179,
               "the moving list's head part ends where the placed list's head begins");

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
    dg_near_t cache_key_ptr;      /* +0x0e  the one-entry cache in front of find_entry_for_pointer: */
    dg_near_t cache_answer_ptr;   /* +0x10  the pointer last asked about, and the answer */
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
    dg_near_t file_used_ptr;      /* +0x1f  the FILE it actually read from */
    dg_near_t file_asked_ptr;     /* +0x21  and the one it was asked about */
} __attribute__((packed));

extern struct dg_546c DG546C;

/* The block `DG546C.table` points at, as the far array of near pointers it is:
   one a part, indexed by part number, `n * 4` bytes allocated for `n`. */
struct part_table {
    dg_near_t part_ptr[];
} __attribute__((packed));
#define PART_TABLE ((struct part_table *)dg_far_ptr(DG546C.table))

DG_ASSERT_AT(struct dg_546c, table,             0x00);
DG_ASSERT_AT(struct dg_546c, record_count,      0x04);
DG_ASSERT_AT(struct dg_546c, is_level,          0x06);
DG_ASSERT_AT(struct dg_546c, version,           0x08);
DG_ASSERT_AT(struct dg_546c, version_out,       0x0a);
DG_ASSERT_AT(struct dg_546c, error,             0x0c);
DG_ASSERT_AT(struct dg_546c, cache_key_ptr,     0x0e);
DG_ASSERT_AT(struct dg_546c, cache_answer_ptr,  0x10);
DG_ASSERT_AT(struct dg_546c, archive_count,     0x12);
DG_ASSERT_AT(struct dg_546c, last_record,       0x14);
DG_ASSERT_AT(struct dg_546c, name_hash,         0x16);
DG_ASSERT_AT(struct dg_546c, open_immediate,    0x1a);
DG_ASSERT_AT(struct dg_546c, byte_5487,         0x1b);
DG_ASSERT_AT(struct dg_546c, retry,             0x1c);
DG_ASSERT_AT(struct dg_546c, byte_5489,         0x1d);
DG_ASSERT_AT(struct dg_546c, scanned,           0x1e);
DG_ASSERT_AT(struct dg_546c, file_used_ptr,     0x1f);
DG_ASSERT_AT(struct dg_546c, file_asked_ptr,    0x21);

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
       `borland_fread`, which settles that one on its own.

       Every comparison against them is **unsigned**, where `struct
       resource`'s are signed. That difference is the original's. */
    uint32_t  base;            /* +0x02  where the entry's data starts in that
                                         archive, past its 17-byte header */
    uint32_t  size;            /* +0x06  the entry's size, out of that header */
    uint32_t  pos;             /* +0x0a  how far into the entry the reader is;
                                         `base + pos` is where to seek */
    uint16_t  in_use;          /* +0x0e  the slot is taken */
    dg_near_t stream_ptr;      /* +0x10  the loose file, when there is one */
} __attribute__((packed));

#define GAME_FILE_PTR(p) ((struct game_file *)(dgroup + (uint16_t)(p)))

/* **No game file**, as a pointer - see `PART_NONE`. */
#define GAME_FILE_NONE GAME_FILE_PTR(0)

DG_ASSERT_AT(struct game_file, archive,           0x00);
DG_ASSERT_AT(struct game_file, base,              0x02);
DG_ASSERT_AT(struct game_file, size,              0x06);
DG_ASSERT_AT(struct game_file, pos,               0x0a);
DG_ASSERT_AT(struct game_file, in_use,            0x0e);
DG_ASSERT_AT(struct game_file, stream_ptr,        0x10);

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
 * `name` is passed straight to `borland_fopen`, so the record's own first byte is
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
    dg_near_t stream_ptr;      /* +0x10  open only while it is the current one */
    uint32_t  pos;             /* +0x12  where DOS is believed to be */
    uint8_t   pad_16[2];
    struct far_ptr list;       /* +0x18  the eight-byte entries the map read:
                                  a hash and an offset each, ending on an
                                  all-zero hash */
} __attribute__((packed));

#define ARCHIVE_PTR(p) ((struct archive *)(dgroup + (uint16_t)(p)))

/*
 * OURS, as a type: one entry of the archive index `archive.list` points at and
 * `scan_entry_list` walks - the name's 32-bit key, then where the entry's data
 * starts. `load_archive_map` fills them from RESOURCE.MAP, a `long` each, and
 * steps its cursor by eight. **Packed**, because the index hands back whatever
 * offset the entry sits at and `+ 4` can be odd; that is why the base used to
 * be read as two 16-bit words. A member of a packed struct is read correctly
 * at any address, the key included.
 */
struct archive_entry {
    uint32_t key;     /* +0x00 */
    uint32_t base;    /* +0x04 */
} __attribute__((packed));
_Static_assert(sizeof(struct archive_entry) == 8, "load_archive_map steps its cursor by 8");

DG_ASSERT_AT(struct archive, name,              0x00);
DG_ASSERT_AT(struct archive, index,             0x0e);
DG_ASSERT_AT(struct archive, stream_ptr,        0x10);
DG_ASSERT_AT(struct archive, pos,               0x12);
DG_ASSERT_AT(struct archive, list,              0x18);

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
    /* **Two graphics-controller mode bytes**, both written to GC register 5 by
       the cursor code: 2 and 1 in the image, which are VGA write mode 2 - a
       byte selects a colour - and write mode 1, the latch copy the routine
       parks with. The second pair is the same two with bit 6 set, 0x40 and
       0x41, which is the 256-colour shift; `mouse_init` copies them over the
       first pair when the driver reports eight bits a pixel. */
    uint8_t   gc_mode_fill;    /* +0x0c */
    uint8_t   quarter_a;       /* +0x0d */
    uint8_t   gc_mode_copy;    /* +0x0e */
    uint8_t   quarter_b;       /* +0x0f */
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

extern struct dg_48da DG48DA;

DG_ASSERT_AT(struct dg_48da, gc_0_1,            0x00);
DG_ASSERT_AT(struct dg_48da, gc_4,              0x02);
DG_ASSERT_AT(struct dg_48da, gc_8,              0x04);
DG_ASSERT_AT(struct dg_48da, seq_map_mask,      0x06);
DG_ASSERT_AT(struct dg_48da, gc_3,              0x08);
DG_ASSERT_AT(struct dg_48da, gc_mode_fill,         0x0c);
DG_ASSERT_AT(struct dg_48da, quarter_a,         0x0d);
DG_ASSERT_AT(struct dg_48da, gc_mode_copy,         0x0e);
DG_ASSERT_AT(struct dg_48da, quarter_b,         0x0f);
DG_ASSERT_AT(struct dg_48da, mouse_taken,       0x10);
DG_ASSERT_AT(struct dg_48da, buttons,           0x11);
DG_ASSERT_AT(struct dg_48da, vector_hooked,     0x12);
DG_ASSERT_AT(struct dg_48da, vector,            0x13);
DG_ASSERT_AT(struct dg_48da, mode_found,        0x18);
DG_ASSERT_AT(struct dg_48da, mode_forced,       0x19);
DG_ASSERT_AT(struct dg_48da, driver,            0x1a);

/*
 * **The scratch block that is allocated to be freed**, at DGROUP 0x3576.
 */
struct dg_3576 {
    struct far_ptr scratch;       /* +0x00  picker_begin takes this if it is
                                     not null */
} __attribute__((packed));

extern struct dg_3576 DG3576;

DG_ASSERT_AT(struct dg_3576, scratch,           0x00);

/*
 * **The machine's own parts**, at DGROUP 0x521b.
 */
struct dg_521b {
    /* **The placed parts: a doubly linked list's head**, the first list the
       level file fills (`n_machine`) and on most levels the largest - the
       scenery: platforms, ramps, pipes, conveyors. 0x5179 holds the moving
       ones. A whole part, read the way the bin's at 0x50d7 is; see
       `parts_bin`. */
    struct part placed_parts;     /* +0x00 */
} __attribute__((packed));

extern struct dg_521b DG521B;

DG_ASSERT_AT(struct dg_521b, placed_parts,      0x00);
_Static_assert(sizeof(struct dg_521b) == 0x52bd - 0x521b,
               "the placed list's head part ends where DG52BD begins");

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
    dg_near_t brklvl_ptr;         /* +0x08  the near heap's break */
} __attribute__((packed));

extern struct dg_0094 DG0094;

DG_ASSERT_AT(struct dg_0094, err_no,            0x00);
DG_ASSERT_AT(struct dg_0094, brklvl_ptr,        0x08);

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
    dg_near_t next;               /* +0x00 */
    uint8_t   level;              /* +0x02  drawn on this level only, unless the part is carried */
    uint8_t   frame[4];           /* +0x03  indices into the kind's bitmap set; 0xff ends the list */
    struct point8 offset[4];   /* +0x07  each frame's offset from the part, signed bytes */
} __attribute__((packed));

#define DRAWSTEP_PTR(p) ((struct draw_step *)(dgroup + (uint16_t)(p)))

/* **No draw step**, as a pointer - see `PART_NONE`. */
#define DRAWSTEP_NONE DRAWSTEP_PTR(0)
/* **The one draw step `draw_part` builds itself**, at DGROUP 0x0124, for a
   part whose kind has no step table of its own. */
extern struct draw_step DG0124;

DG_ASSERT_AT(struct draw_step, level,             0x02);
DG_ASSERT_AT(struct draw_step, frame,             0x03);
DG_ASSERT_AT(struct draw_step, offset,            0x07);
_Static_assert(sizeof(struct draw_step) == 15, "a draw step: four frames and their offsets");

/*
 * **The game's message texts**, at DGROUP 0x1bcc: the two startup complaints
 * and the goodbye, the copy-protection prompt, every message box's title and
 * body, the picker's and the puzzle screen's labels and buttons, the level-
 * complete texts, and the path separator at the end - the one byte
 * `GAME_PATH_SEP.path_sep_ptr` points at.
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

extern struct dg_1bcc DG1BCC;
_Static_assert(sizeof(struct dg_1bcc) == 0x7a4, "DG1BCC ends at 0x2370");

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

extern struct dg_254a DG254A;
_Static_assert(sizeof(struct dg_254a) == 0x42, "DG254A ends at 0x258c");

/*
 * **Not established**, at DGROUP 0x2630.
 */
struct dg_2630 {
    dg_near_t belt_anchor_ptr;    /* +0x00  the part a belt being placed is anchored to */
    /* The two bin-scroll repeat counters `bin_scroll_back` and its twin step.
       `check_goal`'s `lcall [bx + 0x2632]` with `bx` four times the round
       would make them entry 0 of the goal table, but the round is never 0 -
       `game_setup` starts it at 1 and nothing brings it lower - so they are
       only ever these two words, 0000:0000 in the image. */
    uint16_t  back_held;       /* +0x02  how long the bin's back arrow has been held */
    uint16_t  forward_held;    /* +0x04  and the forward one */
    /* **The goal tests, one far pointer per puzzle from 1**, up to 0x27ee. */
    struct far_ptr goal_test[110];    /* +0x06 */
} __attribute__((packed));

extern struct dg_2630 DG2630;

DG_ASSERT_AT(struct dg_2630, belt_anchor_ptr,   0x00);
DG_ASSERT_AT(struct dg_2630, back_held,         0x02);
DG_ASSERT_AT(struct dg_2630, forward_held,         0x04);
DG_ASSERT_AT(struct dg_2630, goal_test,         0x06);
_Static_assert(sizeof(struct dg_2630) == 0x1be, "the goal tests end at 0x27ee");


/*
 * **The span buffer and the driver's vectors**, at DGROUP 0x4342.
 */
struct dg_4342 {
    /* **The segment of the block the game builds span lists in** - a separate
       allocation, not part of DGROUP, reached as `MK_FP(span_buffer_seg, 0)`
       in the guest's address space, exactly where the original puts it.
       `vm_init` sizes it `screen_height * 4 + 0x20`, a header and one span
       per row, and files the block's segment plus one - it frees at this
       minus one.

       An earlier version gave the port an array of its own for this. It
       passed every check until the verifier began comparing all of
       conventional memory rather than only DGROUP, and then `fill_rect` and
       `vm_fill_spans` both failed at once: the original's span list was being
       written somewhere the port never touched. */
    uint16_t  span_buffer_seg;    /* +0x00 */
    /* **Adapter detection is allowed**: `detect_adapter` answers 0 without
       asking anything when this is clear. The image holds 1 and nothing in it
       writes the offset, so it is on and stays on. */
    int16_t   detect_allowed;  /* +0x02 */
    /* **Fifty far pointers into the video driver**, at 0x4346: `vm_init`
       copies a hundred words of the driver's own table from its +0x13e and
       then writes the driver's segment over every second one, which is what
       fifty `far_ptr`s filled word by word looks like. Up to 0x440e. */
    struct far_ptr font[50];      /* +0x04 */
} __attribute__((packed));

extern struct dg_4342 DG4342;

DG_ASSERT_AT(struct dg_4342, span_buffer_seg,   0x00);
DG_ASSERT_AT(struct dg_4342, detect_allowed,         0x02);
DG_ASSERT_AT(struct dg_4342, font,              0x04);
_Static_assert(sizeof(struct dg_4342) == 0xcc, "the driver pointers end at 0x440e");

/*
 * **Scan codes**, set 1, as the keyboard sends them: what `bios_read_key()`
 * answers in its high byte and what indexes the key-down table above. The
 * game reads keys two ways - a screen that wants a *key* takes the high
 * byte and compares one of these; the picker, which wants a *character*,
 * takes the low byte and compares ASCII. The keypad's digits double as an
 * eight-way pad in `timer_callback`: 7 8 9 are Home Up PgUp, 1 2 3 End Down
 * PgDn, and 5 and Ins stand in for the buttons.
 */
#define SC_TAB    0x0f
#define SC_R      0x13
#define SC_Y      0x15
#define SC_ENTER  0x1c
#define SC_A      0x1e
#define SC_C      0x2e
#define SC_V      0x2f
#define SC_N      0x31
#define SC_ALT    0x38
#define SC_SPACE  0x39
#define SC_HOME   0x47
#define SC_UP     0x48
#define SC_PGUP   0x49
#define SC_LEFT   0x4b
#define SC_KP5    0x4c
#define SC_RIGHT  0x4d
#define SC_END    0x4f
#define SC_DOWN   0x50
#define SC_PGDN   0x51
#define SC_INS    0x52

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

extern struct dg_5677 DG5677;

DG_ASSERT_AT(struct dg_5677, crit_vec,          0x00);
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
    dg_near_t bitmap_ptr;      /* +0x02  the bitmap the slot was staged for, re-staged when the cursor's changes */
    int16_t   x;               /* +0x04  where the cursor's bitmap is drawn, unclipped */
    int16_t   y;               /* +0x06 */
    struct saved_rect obj;     /* +0x08  what the object covered */
    struct saved_rect cursor;  /* +0x14  what the cursor covered */
} __attribute__((packed));

DG_ASSERT_AT(struct page_slot, obj,               0x08);
DG_ASSERT_AT(struct page_slot, cursor,            0x14);
_Static_assert(sizeof(struct page_slot) == 0x20,
               "claim_page_slot strides by 0x20");

#define PAGESLOT_PTR(p) ((struct page_slot *)(dgroup + (uint16_t)(p)))

/* **No page slot**, as a pointer - see `PART_NONE`. */
#define PAGESLOT_NONE PAGESLOT_PTR(0)

/*
 * **The shortest run worth encoding**, at DGROUP 0x49ba.
 *
 * `compress_row` counts a run of equal bytes and emits it as a run only when
 * it reaches this; anything shorter goes out as literals. Nothing in the port
 * writes it either - it comes in with the image.
 */
struct dg_49ba {
    int16_t   min_run;            /* +0x00 */
    /* **Three code pointers the offset-table bitmap draws through**, and
       nothing in the image writes the first or the last: they come in with
       the data segment. `draw_offset_bitmap` repoints `plot_fn` before a
       draw - at the driver's plot, `DG4342.font[22]`, when the bitmap is
       wholly inside the clip box, and back at `plot_pixel_clipped` when it
       is not. Names are ours. */
    struct far_ptr fill_fn;       /* +0x02  1c25:3e29, `fill_rect` */
    struct far_ptr plot_fn;       /* +0x06  1c25:61fd, `plot_pixel_clipped` */
    uint16_t  read_fn;            /* +0x0a  near, 248f:1063, `vqt_read_bits` */
} __attribute__((packed));

extern struct dg_49ba DG49BA;

DG_ASSERT_AT(struct dg_49ba, min_run,           0x00);
DG_ASSERT_AT(struct dg_49ba, fill_fn,           0x02);
DG_ASSERT_AT(struct dg_49ba, plot_fn,           0x06);
DG_ASSERT_AT(struct dg_49ba, read_fn,           0x0a);

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
    uint16_t       draw_flags;    /* +0x0a  `draw_offset_bitmap`'s mode:
                                     bit 1 mirrors x, bit 0 mirrors y */
    dg_near_t      reader_ptr;    /* +0x0c  which reader the vqt walk uses -
                                     the singleton above, or the frame
                                     `decode_vqt_list` files here */
    uint16_t       pixel_fn;      /* +0x0e  near, 248f: what a fill loop reads
                                     a colour through - `DG49BA.read_fn`, or
                                     0x004b, `read_palette_pixel` */
    uint16_t       fill_fn;       /* +0x10  near, 248f: 0x0275, 0x02c4 or
                                     0x0313 for a mirrored fill, 0 for none */
    uint8_t        plot_zero;     /* +0x12  plot colour 0 rather than skip it;
                                     only ever cleared */
    uint8_t        pad_6413;
} __attribute__((packed)) bitmaps_t;

extern bitmaps_t BITMAPS;

DG_ASSERT_AT(bitmaps_t, in_use,                 0x00);
DG_ASSERT_AT(bitmaps_t, pos,                    0x02);
DG_ASSERT_AT(bitmaps_t, data,                   0x06);
DG_ASSERT_AT(bitmaps_t, draw_flags,             0x0a);
DG_ASSERT_AT(bitmaps_t, reader_ptr,                0x0c);
DG_ASSERT_AT(bitmaps_t, pixel_fn,               0x0e);
DG_ASSERT_AT(bitmaps_t, fill_fn,                0x10);
DG_ASSERT_AT(bitmaps_t, plot_zero,              0x12);

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
 * the same way the video driver keeps its data inside DGROUP - `struct snd_cs`,
 * below. The image base is derived from `dgroup_base` because that is the
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
 * **The sound module's own code segment, which is where it keeps its state** -
 * two data blocks inside segment 2619's code: 0x0008..0x020d, between a
 * routine's `ret` and the next routine's `push bp`, and the six bytes at
 * 0x30f6. Each is placed there as its own object, with what the image holds;
 * the field comments are offsets in the segment.
 */
struct snd_cs {
    struct far_ptr playing[16];   /* +0x0008  the sequences playing, packed from the front, null-ended */
    struct far_ptr polled[16];    /* +0x0048  the sequences parked to be polled; **not** the playing table */
    struct far_ptr voice_sequence[16]; /* +0x0088  which sequence each voice plays, null for none */
    uint8_t   unknown_00c8[64];   /* +0x00c8  not read or written by the port */
    int16_t   scratch[16];        /* +0x0108  init_sequence_params' sixteen words */
    /* The tick's per-voice arrays, sixteen bytes each - see `sequencer_tick`. */
    uint8_t   voice_held[16];     /* +0x0128  the request each voice plays now, 0xff free */
    uint8_t   voice_keep_own[16]; /* +0x0138  the request must keep its own voice number */
    uint8_t   voice_cost[16];     /* +0x0148  what the request costs */
    uint8_t   voice_gives_back[16]; /* +0x0158  what dropping it gives back */
    uint8_t   voice_request[16];  /* +0x0168  the request this tick, 0xff none */
    uint8_t   saved_keep_own[16]; /* +0x0178  the four above, snapshotted per sequence */
    uint8_t   saved_cost[16];     /* +0x0188 */
    uint8_t   saved_gives_back[16]; /* +0x0198 */
    uint8_t   saved_request[16];  /* +0x01a8 */
    uint8_t   voice_channel[16];  /* +0x01b8  the channel each voice was last given, 0x0f none */
    uint8_t   pending_volume[16]; /* +0x01c8  a volume deferred for flush_pending_volumes, 0xff none */
    uint8_t   pad_01d8[15];
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
    /* **The running total parked across a sequence**: `sound_service` puts it
       here before it walks the channels and reads it back on the path that
       abandons one, so a sequence that fails leaves the total as it found
       it. */
    uint8_t   saved_total;     /* +0x0203 */
    uint8_t   voices_changed;     /* +0x0204  something changed which voice plays what */
    uint8_t   defer;              /* +0x0205  set leaves the new value in the pending array at cs:0x1c8 */
    uint8_t   scan_stopped;       /* +0x0206  at most two a call, so the scan stops where it is */
    uint8_t   pad_0207[2];
    uint8_t   muted;              /* +0x0209  set stops the muting and leaves the counters alone */
    uint8_t   pad_020a[2];
    uint8_t   scratch_mark;       /* +0x020c  0xff, set with the sixteen words at cs:0x108 */
} __attribute__((packed));

extern struct snd_cs SNDS;

struct snd_cs_call {
    struct far_ptr callback;      /* +0x30f6  the cell sound_callback calls
                                              through */
    int16_t   answer;             /* +0x30fa  parked before the registers are popped and read back */
} __attribute__((packed));

extern struct snd_cs_call SNDCALL;

_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, polled) == 0x0048, "snd_cs.polled");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_sequence) == 0x0088, "snd_cs.voice_sequence");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, scratch) == 0x0108, "snd_cs.scratch");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_held) == 0x0128, "snd_cs.voice_held");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_keep_own) == 0x0138, "snd_cs.voice_keep_own");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_cost) == 0x0148, "snd_cs.voice_cost");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_gives_back) == 0x0158, "snd_cs.voice_gives_back");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_request) == 0x0168, "snd_cs.voice_request");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, saved_keep_own) == 0x0178, "snd_cs.saved_keep_own");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, saved_cost) == 0x0188, "snd_cs.saved_cost");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, saved_gives_back) == 0x0198, "snd_cs.saved_gives_back");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, saved_request) == 0x01a8, "snd_cs.saved_request");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_channel) == 0x01b8, "snd_cs.voice_channel");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, pending_volume) == 0x01c8, "snd_cs.pending_volume");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, driver) == 0x01e7, "snd_cs.driver");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, cursor_park) == 0x01f7, "snd_cs.cursor_park");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, busy) == 0x01f9, "snd_cs.busy");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_lo) == 0x01fa, "snd_cs.voice_lo");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voice_hi) == 0x01fb, "snd_cs.voice_hi");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, ch) == 0x01fc, "snd_cs.ch");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, own_voice) == 0x01fd, "snd_cs.own_voice");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, bend_gate) == 0x01fe, "snd_cs.bend_gate");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, cl) == 0x01ff, "snd_cs.cl");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, ah_high) == 0x0200, "snd_cs.ah_high");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, slot_high) == 0x0201, "snd_cs.slot_high");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, param_default) == 0x0202, "snd_cs.param_default");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, saved_total) == 0x0203, "snd_cs.saved_total");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, voices_changed) == 0x0204, "snd_cs.voices_changed");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, defer) == 0x0205, "snd_cs.defer");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, scan_stopped) == 0x0206, "snd_cs.scan_stopped");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, muted) == 0x0209, "snd_cs.muted");
_Static_assert(0x0008 + __builtin_offsetof(struct snd_cs, scratch_mark) == 0x020c, "snd_cs.scratch_mark");
_Static_assert(sizeof(struct snd_cs) == 0x0205, "the data ends at 0x020d, where the next routine starts");
_Static_assert(__builtin_offsetof(struct snd_cs_call, answer) == 0x0004, "snd_cs_call.answer");

/*
 * **The digitised-sound module's own code segment**
 */
struct asb_cs {
    uint8_t   pad_0000[52];
    /* **The two halves of a sample**, because a block that crosses a 64K DMA
       page has to be handed over in two: the page byte, the offset and the
       length of each, with `half` saying which is current. A sample that does
       not cross has `length_b` zero and is played in one. */
    uint8_t   page_a;          /* +0x0034 */
    uint8_t   page_b;          /* +0x0035 */
    uint8_t   word_0036;          /* +0x0036 */
    uint8_t   pad_0037[1];
    /* The DSP answered 2.00 or later, which `asb_probe_version` takes off the
       version it read; 3.00 or later also sets `irq10_worth`. */
    uint8_t   dsp_v2;          /* +0x0038 */
    /* The page, offset and length now programmed into the DMA controller, out
       of whichever half `asb_arm_block` chose. */
    uint8_t   page;            /* +0x0039 */
    uint8_t   pad_003a[1];
    uint8_t   word_003b;          /* +0x003b */
    uint8_t   pad_003c[1];
    uint8_t   word_003d;          /* +0x003d */
    /* **Four "busy" flags**, one per vector the module chains, each raised
       across the handler it replaced and lowered again; `asb_safe_to_call` ORs
       them with the two DOS flags to answer whether it is safe to go near DOS
       or the BIOS. */
    uint8_t   busy_int09;      /* +0x003e */
    uint8_t   busy_int0d;      /* +0x003f */
    uint8_t   busy_int74;      /* +0x0040 */
    uint8_t   word_0041;          /* +0x0041 */
    uint8_t   word_0042;          /* +0x0042 */
    uint8_t   busy_int10;      /* +0x0043 */
    uint8_t   word_0044;          /* +0x0044 */
    /* **The card's IRQ**, and above 7 means the slave PIC - which is what
       chooses `pic_port` and whether the interrupt is acknowledged at 0xa0 as
       well as 0x20. */
    uint8_t   irq;             /* +0x0045 */
    /* **Looping**: `looping` is what the caller asked for and `looped` is
       raised when the handler rearms the block rather than stopping, which is
       the high byte of what function 4 answers. */
    uint8_t   looped;          /* +0x0046 */
    uint8_t   looping;         /* +0x0047 */
    uint8_t   pad_0048[1];
    uint8_t   word_0049;          /* +0x0049 */
    uint8_t   pad_004a[2];
    uint8_t   half;               /* +0x004c  which half is current; `xor ...,1` flips it */
    uint8_t   nothing_to_report;  /* +0x004d */
    /* What `asb_hook_irq` answered when the vector was taken, and what
       `asb_unhook_irq` is given to put it back. */
    uint8_t   irq_saved;       /* +0x004e */
    uint8_t   irq10_worth;        /* +0x004f  what later decides whether IRQ 10 is worth trying */
    uint8_t   pad_0050[2];
    uint8_t   word_0052;          /* +0x0052 */
    uint8_t   pad_0053[1];
    /* **The shutdown guard**: once it is 1 `asb_shutdown` does nothing, which
       is what lets the interrupt handler stop the last block and the game stop
       it again afterwards. Function 4 answers `id` 0xffff while it is set. */
    uint8_t   stopped;         /* +0x0054 */
    uint8_t   pad_0055[1];
    int16_t   length_a;        /* +0x0056 */
    uint16_t  offset_a;        /* +0x0058 */
    int16_t   length_b;        /* +0x005a  zero when the sample fits in one */
    int16_t   offset_b;        /* +0x005c */
    uint8_t   pad_005e[6];
    /* **Where in the sample this block began**, high word then low, which
       function 4 adds the bytes the DMA controller has already taken to. */
    uint16_t  pos_hi;          /* +0x0064 */
    uint16_t  pos_lo;          /* +0x0066 */
    uint8_t   pad_0068[4];
    /* The length this block started with, kept so function 4 can subtract the
       DMA controller's remaining count and say how far in it is. */
    int16_t   block_length;    /* +0x006c */
    int16_t   length;          /* +0x006e */
    int16_t   offset;          /* +0x0070 */
    /* What function 4 answers as the sound's identifier; `asb_play` zeroes
       it. */
    int16_t   id;              /* +0x0072 */
    /* **The PIC's mask port**, 0x21 for the master and 0xa1 for the slave,
       chosen from `irq`. Zero in the module's own image; the install path
       writes it. */
    int16_t   pic_port;        /* +0x0074 */
    int16_t   base;               /* +0x0076  the card's base port, which every other port is an offset from */
    /* The rate the caller asked for - 0x2b11, 11025 Hz, is what install
       leaves - against `dsp_rate`, the time constant actually written. */
    int16_t   rate;            /* +0x0078 */
    /* A DOS handle the module opens, 0xffff for none, closed on shutdown. */
    int16_t   file_handle;     /* +0x007a */
    uint8_t   pad_007c[4];
    /* **The position past which function 4 answers "finished"**, high word
       then low, and the rate as the DSP's own time constant. */
    uint16_t  limit_hi;        /* +0x0080 */
    uint16_t  limit_lo;        /* +0x0082 */
    int16_t   dsp_rate;        /* +0x0084 */
    uint8_t   pad_0086[8];
    /* **Two far pointers, four words.** `asb_safe_to_call` reads a byte
       through each and ORs them: INT 21h AH=34h answers the InDOS flag's
       address, and the byte below it is the critical-error flag. Which field
       holds which is read off `asb_install`, where the pair at +0x0092 is set
       one byte above the pair at +0x008e - so the **names are that reading**,
       not something the code states. */
    struct far_ptr criterr;       /* +0x008e */
    struct far_ptr indos;         /* +0x0092 */
    struct far_ptr old_int0d;     /* +0x0096  the vectors asb_install displaces */
    struct far_ptr old_int74;     /* +0x009a */
    struct far_ptr old_int10;     /* +0x009e */
    struct far_ptr old_int09;     /* +0x00a2 */
    uint8_t   pad_00a6[1811];
    uint8_t   word_07b9;          /* +0x07b9 */
    uint8_t   word_07ba;          /* +0x07ba */
    uint8_t   word_07bb;          /* +0x07bb */
    uint8_t   word_07bc;          /* +0x07bc */
    uint8_t   word_07bd;          /* +0x07bd */
} __attribute__((packed));

#define ASBS (*(struct asb_cs *)MK_FP(ASB_SEG, ASB_OFF))

_Static_assert(__builtin_offsetof(struct asb_cs, page_a) == 0x0034, "asb_cs.page_a");
_Static_assert(__builtin_offsetof(struct asb_cs, page_b) == 0x0035, "asb_cs.page_b");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0036) == 0x0036, "asb_cs.word_0036");
_Static_assert(__builtin_offsetof(struct asb_cs, dsp_v2) == 0x0038, "asb_cs.dsp_v2");
_Static_assert(__builtin_offsetof(struct asb_cs, page) == 0x0039, "asb_cs.page");
_Static_assert(__builtin_offsetof(struct asb_cs, word_003b) == 0x003b, "asb_cs.word_003b");
_Static_assert(__builtin_offsetof(struct asb_cs, word_003d) == 0x003d, "asb_cs.word_003d");
_Static_assert(__builtin_offsetof(struct asb_cs, busy_int09) == 0x003e, "asb_cs.busy_int09");
_Static_assert(__builtin_offsetof(struct asb_cs, busy_int0d) == 0x003f, "asb_cs.busy_int0d");
_Static_assert(__builtin_offsetof(struct asb_cs, busy_int74) == 0x0040, "asb_cs.busy_int74");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0041) == 0x0041, "asb_cs.word_0041");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0042) == 0x0042, "asb_cs.word_0042");
_Static_assert(__builtin_offsetof(struct asb_cs, busy_int10) == 0x0043, "asb_cs.busy_int10");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0044) == 0x0044, "asb_cs.word_0044");
_Static_assert(__builtin_offsetof(struct asb_cs, irq) == 0x0045, "asb_cs.irq");
_Static_assert(__builtin_offsetof(struct asb_cs, looped) == 0x0046, "asb_cs.looped");
_Static_assert(__builtin_offsetof(struct asb_cs, looping) == 0x0047, "asb_cs.looping");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0049) == 0x0049, "asb_cs.word_0049");
_Static_assert(__builtin_offsetof(struct asb_cs, half) == 0x004c, "asb_cs.half");
_Static_assert(__builtin_offsetof(struct asb_cs, nothing_to_report) == 0x004d, "asb_cs.nothing_to_report");
_Static_assert(__builtin_offsetof(struct asb_cs, irq_saved) == 0x004e, "asb_cs.irq_saved");
_Static_assert(__builtin_offsetof(struct asb_cs, irq10_worth) == 0x004f, "asb_cs.irq10_worth");
_Static_assert(__builtin_offsetof(struct asb_cs, word_0052) == 0x0052, "asb_cs.word_0052");
_Static_assert(__builtin_offsetof(struct asb_cs, stopped) == 0x0054, "asb_cs.stopped");
_Static_assert(__builtin_offsetof(struct asb_cs, length_a) == 0x0056, "asb_cs.length_a");
_Static_assert(__builtin_offsetof(struct asb_cs, offset_a) == 0x0058, "asb_cs.offset_a");
_Static_assert(__builtin_offsetof(struct asb_cs, length_b) == 0x005a, "asb_cs.length_b");
_Static_assert(__builtin_offsetof(struct asb_cs, offset_b) == 0x005c, "asb_cs.offset_b");
_Static_assert(__builtin_offsetof(struct asb_cs, pos_hi) == 0x0064, "asb_cs.pos_hi");
_Static_assert(__builtin_offsetof(struct asb_cs, pos_lo) == 0x0066, "asb_cs.pos_lo");
_Static_assert(__builtin_offsetof(struct asb_cs, block_length) == 0x006c, "asb_cs.block_length");
_Static_assert(__builtin_offsetof(struct asb_cs, length) == 0x006e, "asb_cs.length");
_Static_assert(__builtin_offsetof(struct asb_cs, offset) == 0x0070, "asb_cs.offset");
_Static_assert(__builtin_offsetof(struct asb_cs, id) == 0x0072, "asb_cs.id");
_Static_assert(__builtin_offsetof(struct asb_cs, pic_port) == 0x0074, "asb_cs.pic_port");
_Static_assert(__builtin_offsetof(struct asb_cs, base) == 0x0076, "asb_cs.base");
_Static_assert(__builtin_offsetof(struct asb_cs, rate) == 0x0078, "asb_cs.rate");
_Static_assert(__builtin_offsetof(struct asb_cs, file_handle) == 0x007a, "asb_cs.file_handle");
_Static_assert(__builtin_offsetof(struct asb_cs, limit_hi) == 0x0080, "asb_cs.limit_hi");
_Static_assert(__builtin_offsetof(struct asb_cs, limit_lo) == 0x0082, "asb_cs.limit_lo");
_Static_assert(__builtin_offsetof(struct asb_cs, dsp_rate) == 0x0084, "asb_cs.dsp_rate");
_Static_assert(__builtin_offsetof(struct asb_cs, criterr) == 0x008e, "asb_cs.criterr");
_Static_assert(__builtin_offsetof(struct asb_cs, indos) == 0x0092, "asb_cs.indos");
_Static_assert(__builtin_offsetof(struct asb_cs, old_int0d) == 0x0096, "asb_cs.old_int0d");
_Static_assert(__builtin_offsetof(struct asb_cs, old_int74) == 0x009a, "asb_cs.old_int74");
_Static_assert(__builtin_offsetof(struct asb_cs, old_int10) == 0x009e, "asb_cs.old_int10");
_Static_assert(__builtin_offsetof(struct asb_cs, old_int09) == 0x00a2, "asb_cs.old_int09");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07b9) == 0x07b9, "asb_cs.word_07b9");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07ba) == 0x07ba, "asb_cs.word_07ba");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07bb) == 0x07bb, "asb_cs.word_07bb");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07bc) == 0x07bc, "asb_cs.word_07bc");
_Static_assert(__builtin_offsetof(struct asb_cs, word_07bd) == 0x07bd, "asb_cs.word_07bd");

/*
 * **Segment 1c25, which keeps the displaced vectors inside its own code** -
 * three data cells, each placed at its offset in the segment. All are zero in
 * the image; the routines that install the handlers fill them.
 */
struct s1c_timer {
    struct far_ptr old_int8;      /* +0x446d  the INT 08h vector
                                              timer_install displaced */
} __attribute__((packed));

struct s1c_keyboard {
    struct far_ptr old_int9;      /* +0x4e3c  the INT 09h vector
                                              install_keyboard displaced */
    struct far_ptr old_int1c;     /* +0x4e40  and the INT 1Ch one */
} __attribute__((packed));

struct s1c_words {
    int16_t   word_5f99;          /* +0x5f99 */
    int16_t   word_5f9b;          /* +0x5f9b */
} __attribute__((packed));

extern struct s1c_timer    S1C_TIMER;
extern struct s1c_keyboard S1C_KEYBOARD;
extern struct s1c_words    S1C_WORDS;

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
    /* **The pitch bend as MIDI sent it**, both halves - `(msb << 7) | lsb` -
       kept only so `sx_query` can hand it back. What the bend is *computed*
       from throws the low seven bits away. */
    int16_t   bend_value;      /* +0x033c */
    uint8_t   pad_033e[4];
    /* **The bend in quarter semitones and its direction** (1 up), which
       `sx_apply_bend` adds to or subtracts from the note's table index. A
       full-scale bend is 47 of them, near enough an octave. */
    uint8_t   bend;            /* +0x0342 */
    uint8_t   bend_up;         /* +0x0343 */
    /* **The note on the speaker**, 0 for silence. `sx_stop_note` ignores a
       request to stop any other note, which is what lets a voice taken over by
       a later note be released harmlessly. */
    uint8_t   note;            /* +0x0344 */
    /* **Three things that can veto a note**, all tested together in
       `sx_note_on`: the master level (function 12, 0 to 0xf, and setting it to
       zero silences), the on/off switch (function 13, which stores only 0 or
       1), and MIDI controller 7's own flag. */
    uint8_t   level;           /* +0x0345 */
    uint8_t   enabled;         /* +0x0346 */
    uint8_t   volume_on;       /* +0x0347 */
    /* **The one channel the speaker listens to**, claimed and released through
       controller 0x4b, 0xff for nobody. One speaker sounds one note, so every
       request for another channel is dropped. */
    uint8_t   channel;         /* +0x0348 */
    /* Function 11 stores CL here and answers what was there; **nothing else in
       the driver reads it**, so what it is for is not established. */
    uint8_t   byte_0349;       /* +0x0349 */
} __attribute__((packed));

#define SXSPKR (*(struct sx_spkr *)MK_FP(SX_SEG, 0))

_Static_assert(__builtin_offsetof(struct sx_spkr, bend_value) == 0x033c, "sx_spkr.bend_value");
_Static_assert(__builtin_offsetof(struct sx_spkr, bend) == 0x0342, "sx_spkr.bend");
_Static_assert(__builtin_offsetof(struct sx_spkr, bend_up) == 0x0343, "sx_spkr.bend_up");
_Static_assert(__builtin_offsetof(struct sx_spkr, note) == 0x0344, "sx_spkr.note");
_Static_assert(__builtin_offsetof(struct sx_spkr, level) == 0x0345, "sx_spkr.level");
_Static_assert(__builtin_offsetof(struct sx_spkr, enabled) == 0x0346, "sx_spkr.enabled");
_Static_assert(__builtin_offsetof(struct sx_spkr, volume_on) == 0x0347, "sx_spkr.volume_on");
_Static_assert(__builtin_offsetof(struct sx_spkr, channel) == 0x0348, "sx_spkr.channel");
_Static_assert(__builtin_offsetof(struct sx_spkr, byte_0349) == 0x0349, "sx_spkr.byte_0349");

/*
 * **The AdLib driver**, laid over whatever `SX_SEG` points at.
 *
 * One struct per driver, and that is the point: `SX_SEG` is whichever
 * chunk the loader put there, and the three have different layouts. A
 * single overlay would be right for one of them and quietly wrong for the
 * other two - they share only 0x188d, and that by coincidence.
 */
/*
 * One patch as the bank holds it: the two operators' thirteen bytes each, and
 * their connection bytes at the end rather than in the runs - which is why
 * `adl_write_operator` is handed a run and a connection separately.
 */
struct adl_patch {
    uint8_t   op[2][13];          /* +0x00 and +0x0d */
    uint8_t   connect[2];         /* +0x1a */
} __attribute__((packed));

_Static_assert(sizeof(struct adl_patch) == 28, "the bank is indexed by 28");

struct sx_adl {
    uint8_t   pad_0000[55];
    /* **The three port numbers `adl_write` uses**: the register select, the
       one it reads 0x21 times as the chip's settling delay, and the data
       port. They are variables rather than constants, which is why searching
       this driver's bytes for 0x0388 finds them in what looks like a table. */
    int16_t   reg_port;        /* +0x0037 */
    int16_t   wait_port;       /* +0x0039 */
    int16_t   data_port;       /* +0x003b */
    uint8_t   pad_003d[224];
    /* Function 11 stores CL here and answers what was there, and nothing else
       in the driver reads it - the same shape, and the same silence about what
       it means, as `SPKR:0x349`. */
    uint8_t   byte_011d;       /* +0x011d */
    /* **The master level and the switch above it**, functions 12 and 13, the
       same pair every driver in this family carries: the level scales every
       note's output and setting it rewrites all nine sounding voices, and a
       clear `enabled` forces the level to zero at the point it is applied. */
    uint8_t   enabled;         /* +0x011e */
    uint8_t   level;           /* +0x011f */
    uint8_t   pad_0120[175];
    /* **The most recently used voice**: the last entry of the nine-entry
       rotation at ADL:0x1c7, which `adl_touch_voice` shifts a voice to the end
       of so the next allocation takes the one used longest ago. */
    uint8_t   voice_mru;       /* +0x01cf */
    uint8_t   pad_01d0[64];
    /* **Three tables of eighteen**, one entry per operator the OPL2 has -
       nine channels of two - and the driver indexes all three by the slot.
       They were a byte each with a seventeen-byte pad, which is the same
       memory and said nothing about the shape. */
    uint8_t   no_operator[18];    /* +0x0210  set for a slot that owns none */
    uint8_t   op_reg[18];         /* +0x0222  the register offset this slot's
                                              operator answers to - the OPL2's
                                              0,1,2,8,9,0xa,0x10 layout, not a
                                              straight index */
    uint8_t   chan_reg[18];       /* +0x0234  and the offset its channel uses */
    uint8_t   pad_0246[300];
    int16_t   bank_bytes;          /* +0x0372  how many bytes of bank were copied in */
    /* **The patch bank**, at +0x374, which `adl_init` copies in from the file
       the game hands it. A patch is 28 bytes - two operator runs of thirteen
       and the two connection bytes kept apart at the end - and the driver
       reaches one as `0x374 + program * 28`.

       192 of them is what fits: the largest program the code can ask for is
       0x58 + 0x65 = 0xbd, and 192 entries end at +0x1874, which leaves exactly
       the six bytes before the default operator run at +0x187a. */
    struct adl_patch patch[192];  /* +0x0374 */
    uint8_t   pad_1874[6];
    uint8_t   default_op[13];     /* +0x187a  the run adl_reset writes to every
                                              slot */
    uint8_t   pad_1887[1];
    /* **The chip's own global bits**, each held here and written to the OPL2
       when one of them changes: note select in register 8, and the tremolo and
       vibrato depths in the top two bits of register 0xBD. */
    uint8_t   note_select;     /* +0x1888 */
    uint8_t   am_depth;        /* +0x1889 */
    uint8_t   vib_depth;       /* +0x188a */
    uint8_t   pad_188b[1];
    /* The rhythm bits of the same register, which this game leaves clear: it
       uses the nine melodic voices and no percussion mode. */
    uint8_t   rhythm;          /* +0x188c */
    /* Register 1, which on an OPL2 is the wave-select enable; `adl_reset`
       writes 0x20 here and passes it on. */
    int16_t   wave_select;     /* +0x188d */
} __attribute__((packed));

#define SXADL (*(struct sx_adl *)MK_FP(SX_SEG, 0))

_Static_assert(__builtin_offsetof(struct sx_adl, reg_port) == 0x0037, "sx_adl.reg_port");
_Static_assert(__builtin_offsetof(struct sx_adl, wait_port) == 0x0039, "sx_adl.wait_port");
_Static_assert(__builtin_offsetof(struct sx_adl, data_port) == 0x003b, "sx_adl.data_port");
_Static_assert(__builtin_offsetof(struct sx_adl, byte_011d) == 0x011d, "sx_adl.byte_011d");
_Static_assert(__builtin_offsetof(struct sx_adl, enabled) == 0x011e, "sx_adl.enabled");
_Static_assert(__builtin_offsetof(struct sx_adl, level) == 0x011f, "sx_adl.level");
_Static_assert(__builtin_offsetof(struct sx_adl, voice_mru) == 0x01cf, "sx_adl.voice_mru");
_Static_assert(__builtin_offsetof(struct sx_adl, no_operator) == 0x0210, "sx_adl.no_operator");
_Static_assert(__builtin_offsetof(struct sx_adl, op_reg) == 0x0222, "sx_adl.op_reg");
_Static_assert(__builtin_offsetof(struct sx_adl, chan_reg) == 0x0234, "sx_adl.chan_reg");
_Static_assert(__builtin_offsetof(struct sx_adl, bank_bytes) == 0x0372, "sx_adl.bank_bytes");
_Static_assert(__builtin_offsetof(struct sx_adl, patch) == 0x0374, "sx_adl.patch");
_Static_assert(__builtin_offsetof(struct sx_adl, default_op) == 0x187a, "sx_adl.default_op");
_Static_assert(__builtin_offsetof(struct sx_adl, note_select) == 0x1888, "sx_adl.note_select");
_Static_assert(__builtin_offsetof(struct sx_adl, am_depth) == 0x1889, "sx_adl.am_depth");
_Static_assert(__builtin_offsetof(struct sx_adl, vib_depth) == 0x188a, "sx_adl.vib_depth");
_Static_assert(__builtin_offsetof(struct sx_adl, rhythm) == 0x188c, "sx_adl.rhythm");
_Static_assert(__builtin_offsetof(struct sx_adl, wave_select) == 0x188d, "sx_adl.wave_select");

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
    /* **Eleven port numbers**, because a Sound Blaster Pro's base is
       configurable where an AdLib's is not. Each write is an index, five reads
       of the index port as the chip's settling time, the data byte, and
       thirty-three reads of the wait port. The first three are the plain OPL
       pair; then the left and the right bank, which on a Pro 1.0 are two
       physically separate YM3812s and on an OPL3 the one chip's two banks; and
       last the mixer, which is the part an AdLib does not have and is how this
       driver places a voice left or right. */
    int16_t   reg_port;        /* +0x002c */
    int16_t   wait_port;       /* +0x002e */
    int16_t   data_port;       /* +0x0030 */
    int16_t   left_reg_port;   /* +0x0032 */
    int16_t   left_wait_port;  /* +0x0034 */
    int16_t   left_data_port;  /* +0x0036 */
    int16_t   right_reg_port;  /* +0x0038 */
    int16_t   right_wait_port; /* +0x003a */
    int16_t   right_data_port; /* +0x003c */
    int16_t   mixer_reg_port;  /* +0x003e */
    int16_t   mixer_data_port; /* +0x0040 */
    uint8_t   pad_0042[224];
    /* Function 11's byte, which this driver stores and nothing here reads -
       the same shape as `ADL:0x11d` and `SPKR:0x349`. */
    uint8_t   byte_0122;       /* +0x0122 */
    /* **The FM switch and the master level**, functions 13 and 12. On this card
       the level is not an OPL register but the mixer's FM volume at 0x26, both
       nibbles at once; switching off writes silence to the mixer and keeps the
       setting here, so `level` always holds the real one. */
    uint8_t   enabled;         /* +0x0123 */
    uint8_t   level;           /* +0x0124 */
    uint8_t   pad_0125[175];
    /* **The most recently used voice**: the last of the nine at SBP:0x1cc,
       which the rotation shifts a voice to the end of. */
    uint8_t   voice_mru;       /* +0x01d4 */
    uint8_t   pad_01d5[45];
    /* **Whether the two banks take the stereo bits** - 0x20 for left and 0x10
       for right on registers 0xc0..0xc8. It is zero in the shipped driver, a
       build-time constant saying "this is not an OPL3", and the branch that
       tests it is transcribed whole because that is what the original runs. */
    uint8_t   opl3;            /* +0x0202 */
    uint8_t   pad_0203[372];
    /* How many bytes of patch bank `sbp_init` copies in. */
    int16_t   bank_bytes;      /* +0x0377 */
    uint8_t   pad_0379[5396];
    /* **The chip's global bits**, the same five this family's other drivers
       keep: note select in register 8, the tremolo and vibrato depths and the
       rhythm bits in 0xBD, and register 1's wave-select enable. */
    uint8_t   note_select;     /* +0x188d */
    uint8_t   am_depth;        /* +0x188e */
    uint8_t   vib_depth;       /* +0x188f */
    uint8_t   pad_1890[1];
    uint8_t   rhythm;          /* +0x1891 */
    int16_t   wave_select;     /* +0x1892 */
} __attribute__((packed));

#define SXSBP (*(struct sx_sbp *)MK_FP(SX_SEG, 0))

_Static_assert(__builtin_offsetof(struct sx_sbp, reg_port) == 0x002c, "sx_sbp.reg_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, wait_port) == 0x002e, "sx_sbp.wait_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, data_port) == 0x0030, "sx_sbp.data_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, left_reg_port) == 0x0032, "sx_sbp.left_reg_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, left_wait_port) == 0x0034, "sx_sbp.left_wait_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, left_data_port) == 0x0036, "sx_sbp.left_data_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, right_reg_port) == 0x0038, "sx_sbp.right_reg_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, right_wait_port) == 0x003a, "sx_sbp.right_wait_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, right_data_port) == 0x003c, "sx_sbp.right_data_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, mixer_reg_port) == 0x003e, "sx_sbp.mixer_reg_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, mixer_data_port) == 0x0040, "sx_sbp.mixer_data_port");
_Static_assert(__builtin_offsetof(struct sx_sbp, byte_0122) == 0x0122, "sx_sbp.byte_0122");
_Static_assert(__builtin_offsetof(struct sx_sbp, enabled) == 0x0123, "sx_sbp.enabled");
_Static_assert(__builtin_offsetof(struct sx_sbp, level) == 0x0124, "sx_sbp.level");
_Static_assert(__builtin_offsetof(struct sx_sbp, voice_mru) == 0x01d4, "sx_sbp.voice_mru");
_Static_assert(__builtin_offsetof(struct sx_sbp, opl3) == 0x0202, "sx_sbp.opl3");
_Static_assert(__builtin_offsetof(struct sx_sbp, bank_bytes) == 0x0377, "sx_sbp.bank_bytes");
_Static_assert(__builtin_offsetof(struct sx_sbp, note_select) == 0x188d, "sx_sbp.note_select");
_Static_assert(__builtin_offsetof(struct sx_sbp, am_depth) == 0x188e, "sx_sbp.am_depth");
_Static_assert(__builtin_offsetof(struct sx_sbp, vib_depth) == 0x188f, "sx_sbp.vib_depth");
_Static_assert(__builtin_offsetof(struct sx_sbp, rhythm) == 0x1891, "sx_sbp.rhythm");
_Static_assert(__builtin_offsetof(struct sx_sbp, wave_select) == 0x1892, "sx_sbp.wave_select");

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
    dg_near_t link_ptr;        /* +0x00  the next record on this list */
    uint16_t  mask;            /* +0x02  and-ed with the state word at 0x4e6b */
    /* **The region's own number**, which its handler reads and nothing else
       does. Measured on the play screen: the five bin rows carry 0 to 4 -
       the slot each one is - and every other region on that screen carries
       0. `region_cursor_bin` and its click handler pass it straight to
       `bin_part_at_index`. */
    uint16_t  slot;            /* +0x04 */
    int16_t   x0;              /* +0x06  the rectangle, inclusive at both ends */
    int16_t   y0;              /* +0x08 */
    int16_t   x1;              /* +0x0a */
    int16_t   y1;              /* +0x0c */
    uint16_t  cursor;          /* +0x0e  which cursor while the pointer is in it */
    uint16_t  code;            /* +0x10  written into the state word on a click */
    /* Two far *code* pointers, and both are tested `(off | seg) != 0` -
       which is `dg_far_ptr(h) != FAR_NULL_PTR` and not a C null test. */
    struct far_ptr hover;      /* +0x12  called whenever the pointer is
                                         inside */
    struct far_ptr click;      /* +0x16  and this one on the click itself */
} __attribute__((packed));
_Static_assert(sizeof(struct region) == 0x1a, "a region record is thirteen words");

#define REGION_PTR(p) ((struct region *)(dgroup + (uint16_t)(p)))

/* **No region**, as a pointer - see `PART_NONE`. */
#define REGION_NONE REGION_PTR(0)

DG_ASSERT_AT(struct region, link_ptr,          0x00);
DG_ASSERT_AT(struct region, mask,              0x02);
DG_ASSERT_AT(struct region, x0,                0x06);
DG_ASSERT_AT(struct region, y0,                0x08);
DG_ASSERT_AT(struct region, x1,                0x0a);
DG_ASSERT_AT(struct region, y1,                0x0c);
DG_ASSERT_AT(struct region, cursor,            0x0e);
DG_ASSERT_AT(struct region, code,              0x10);
DG_ASSERT_AT(struct region, hover,             0x12);
DG_ASSERT_AT(struct region, click,             0x16);
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
/* **Borland's `FILE`**, sixteen bytes, with Borland's own field names:
   `level` is the bytes still buffered, negative while writing; `fd` the DOS
   handle, read signed for -1; `hold` the one-byte buffer an unbuffered stream
   reads into; `buffer` and `curp` the buffer and the cursor into it as DGROUP
   offsets; `token` the record's own offset, which `borland_fclose` and
   `borland_setvbuf` check before believing the pointer. The stream routines
   in borland_file.c take a pointer to one and nothing outside them looks
   inside; a routine that files a stream in DGROUP keeps `dg_near` of it and
   gets it back with `FILEREC_PTR`. */
struct file_rec {
    int16_t   level;           /* +0x00  bytes still in the buffer */
    uint16_t  flags;           /* +0x02  0x40 is the one buffered_read tests */
    uint8_t   fd;              /* +0x04  the DOS handle, read signed for -1 */
    uint8_t   hold;            /* +0x05  the unbuffered stream's one byte */
    uint16_t  bsize;           /* +0x06 */
    dg_near_t buffer_ptr;      /* +0x08 */
    dg_near_t curp_ptr;        /* +0x0a  where the next byte comes from */
    uint16_t  istemp;          /* +0x0c */
    dg_near_t token_ptr;       /* +0x0e  the record's own offset, filed by setup_streams */
} __attribute__((packed));

/* A stream offset of 0 is NULL - the failed open, which every caller tests. */
static inline struct file_rec *FILEREC_PTR(uint16_t p)
{
    return p != 0 ? (struct file_rec *)(dgroup + p) : NULL;
}

/* **`FILE` is Borland's, in the game.** The game's translation units include
   no host <stdio.h> - what they need of the host's console goes through io.h -
   so the standard name is free, and the game's file routines are written
   against it: a routine that opens, reads, seeks or closes takes and answers
   `FILE *`, and another stdio could be put under the name. A *host* unit -
   io.c, sdl.c, the dev*.c files, the hybrid - needs the host's <stdio.h> and
   defines `TIM_HOST`, so this typedef is skipped there; those units read the
   game's prototypes with the host's `FILE`, which is a pointer either way, and
   none of them hands the game one. */
#ifdef TIM_HOST
#include <stdio.h>          /* the host's FILE, for the host's own units */
#else
typedef struct file_rec FILE;
#endif

/* Not volatile: the streams are Borland's and nothing on the timer thread
   touches one, so `&BORLAND_STREAMS.streams[i]` is the `struct file_rec *` the stream
   routines take. */

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
    dg_near_t file_ptr;         /* +0x00  the Borland FILE this slot is for */
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
    /* **How many matches this record has already gone past**, so asking for a
       later index continues from there; an earlier one resets the record. */
    int16_t  matched;          /* +0x39 */
    uint32_t pos;              /* +0x3b  the position, which restore_file_record
                                         seeks back to */
    int32_t  size;             /* +0x3f  the current chunk's size; what
                                         file_record_size answers. A signed
                                         long: seek_named_chunk tests it
                                         `< 0` with `jl`, and against the
                                         outer bound with `jb`, where the
                                         unsigned bound makes it unsigned */
} __attribute__((packed));

DG_ASSERT_AT(struct open_file, path,              0x02);
DG_ASSERT_AT(struct open_file, bound,             0x1b);
DG_ASSERT_AT(struct open_file, depth,             0x37);
DG_ASSERT_AT(struct open_file, matched,           0x39);
DG_ASSERT_AT(struct open_file, pos,               0x3b);
DG_ASSERT_AT(struct open_file, size,              0x3f);
_Static_assert(sizeof(struct open_file) == 0x43,
               "an open file is what find_file_record strides by");


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
 * disassembly - `bmp_ptr[0x25]` is `[si+0x4a]`.
 * ---------------------------------------------------------------------------
 */
struct bmp_set {
    bmp_ptr_t bmp_ptr[];
} __attribute__((packed));

#define BMPSET_PTR(p) ((struct bmp_set *)(dgroup + (uint16_t)(p)))

/* **No bitmap set**, as a pointer - see `PART_NONE`. */
#define BMPSET_NONE BMPSET_PTR(0)

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

/* **No bitmap**, as a pointer - see `PART_NONE`. */
#define BMP_NONE BMP_PTR(0)

/* A **bitmap list**: a null-terminated array of near pointers to the above.
   `BMPSET_PTR(p)` and `BMPLIST(p)` are two views of one object - the first for a
   set whose entries are known by number, `bmp_ptr[0x25]`, the second for a list
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

#define VQTRD(p) ((struct vqt_reader *)(dgroup + (uint16_t)(p)))

/* **The mirrored quadtree leaf's palette**, as `VQTRD` is the reader: up to
   256 colour bytes that `vqt_flip_leaf` reads into the bottom of its own
   frame and files the offset of at `BITMAPS_FLIP_STATE.palette`. `p` is that offset.

   The original indexes it as `add bx,ax` on the 16-bit offset; indexing the
   pointer instead only differs if the table straddles the end of DGROUP, and
   a 0x110-byte frame on the guest's stack cannot. */
#define VQTPAL(p) ((uint8_t *)(dgroup + (uint16_t)(p)))

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

DG_ASSERT_AT(struct sound_node, key,               0x00);
DG_ASSERT_AT(struct sound_node, length,            0x02);
DG_ASSERT_AT(struct sound_node, next,              0x04);

/* One of these through the far pointer that reaches it. Not a `DG*` macro:
   they are not in DGROUP. */
#define SOUND_NODE_PTR(fp) ((struct sound_node *)(void *)dg_far_ptr(fp))

/*
 * ---------------------------------------------------------------------------
 * **A sequence**, the 0x17a-byte record of kind 2 that `create_sequence` builds
 * around a block of note data and `alloc_voice_records` makes seven of for the
 * voices - the same record either way, which is why a voice and a sequence are
 * read through the same offsets. It lives in a DOS block, not in DGROUP.
 *
 * The layout is the offsets the sound module reads and writes; the sizes of
 * the channel tables are `start_sequence`'s loop, sixteen channels where it
 * sets entry 0xf on its own and fifteen where it does not, and each table ends
 * where the next begins. Names come from what the code does with a field, and
 * where that is only a reading the comment says **guessed**; the per-channel
 * byte tables `start_sequence` fills with 0 or 0xff and nothing here names are
 * spelled by their offset.
 *
 * The far pointers are **stored** pairs, and three of them are a segment beside
 * an offset stepped inside it - `cursor` is `source` past its first record, and
 * `cursor_at` is this record's own `cursor` - so they are filed as that
 * segment and offset, never renormalised.
 * ---------------------------------------------------------------------------
 */
/*
 * **A sequence's per-channel state**, +0x0bc to +0x152 of a sequence: one word
 * table and eight byte tables, each read by the mapped MIDI channel - the
 * original's `es:[bx+2*si+0xbc]` and `es:[bx+si+0x107]`.
 *
 * **Fifteen entries each, and the channel runs to 15.** The offsets fix the
 * width - the tables are 0x0f apart - and the index is a channel's low nibble,
 * so channel 15 reads the first entry of the table after: `pan[15]` is
 * `volume[0]`, and `byte_143[15]` is `loop_count`'s low byte. That is the
 * original's own arithmetic, and it is kept.
 */
struct sequence_channels {
    /* **What each of the fifteen channels is set to**, and `tick_program_voice`
       is where the names come from: it sends each of these to the driver as
       the controller beside it when a channel is given a voice. The three that
       start 0xff are filled from the sequence's own header the first time a
       channel is met - bytes 1, 4, 8 and 0xb of it - so 0xff means "not set
       yet" rather than a value. */
    uint16_t       bend[15];            /* +0x00  0x2000 at start, the centre; top bit the sustain pedal, controller 0x40 */
    uint8_t        byte_0da[15];        /* +0x1e  0xff at start; the low nibble goes out as controller 0x4b and the high one is read on its own */
    uint8_t        modulation[15];      /* +0x2d  controller 1 */
    uint8_t        pan[15];             /* +0x3c  controller 0x0a */
    uint8_t        volume[15];          /* +0x4b  controller 7, scaled by the sequence's own volume */
    uint8_t        program[15];         /* +0x5a  the program change */
    uint8_t        note[15];            /* +0x69  controller 0x4e, the note to retrigger */
    uint8_t        byte_134[15];        /* +0x78  0 at start; bits 1 and 2, set while the channels are placed */
    uint8_t        byte_143[15];        /* +0x87  0 at start */
} __attribute__((packed));

_Static_assert(sizeof(struct sequence_channels) == 0x96, "+0x0bc to +0x152 of a sequence");

struct sequence {
    uint8_t        unknown_000[8];      /* +0x000  not read or written by the port */
    struct far_ptr cursor_at;           /* +0x008  where the cursor lives: this record's `cursor` */
    uint16_t       position[16];        /* +0x00c  each channel's place in the event data */
    uint16_t       position_saved[16];  /* +0x02c  its shadow, which a checkpoint copies */
    uint16_t       delay[16];           /* +0x04c  ticks to each channel's next event */
    uint16_t       delay_saved[16];     /* +0x06c */
    uint8_t        byte_08c[16];        /* +0x08c  0xff at start */
    uint8_t        status[16];          /* +0x09c  running status */
    uint8_t        status_saved[16];    /* +0x0ac */
    struct sequence_channels ch;        /* +0x0bc  each channel's controllers */
    uint16_t       loop_count;          /* +0x152  bumped by controller 0x60 */
    uint16_t       ticks;               /* +0x154  bumped every step */
    uint16_t       ticks_saved;         /* +0x156  and restored from here on a loop */
    uint8_t        state;               /* +0x158  0xff free or stopped, 0xfe a fade arrived */
    uint8_t        mode;                /* +0x159  1, or 2 when started with the flag */
    uint8_t        byte_15a;            /* +0x15a  what a rewind must match; with `loop` 0, the end */
    uint8_t        byte_15b;            /* +0x15b */
    uint8_t        priority;            /* +0x15c  a sound bank entry's second byte */
    uint8_t        loop;                /* +0x15d  and its first */
    uint8_t        volume;              /* +0x15e  0x7f for the default */
    uint8_t        device_value;        /* +0x15f  controller 0x50's, 0x7f for the default */
    uint8_t        fade_target;         /* +0x160  top bit: remove the sequence on arrival */
    uint8_t        fade_period;         /* +0x161  ticks between fade steps */
    uint8_t        fade_countdown;      /* +0x162 */
    uint8_t        fade_step;           /* +0x163  the largest step, 0 for no fade */
    uint8_t        skip;                /* +0x164  set: the tick leaves the sequence alone */
    uint8_t        poll;                /* +0x165  how the host is asked about it */
    struct far_ptr source;              /* +0x166  the note data */
    struct far_ptr cursor;              /* +0x16a  the record being played */
    uint8_t        unknown_16e[4];      /* +0x16e */
    struct far_ptr next;                /* +0x172  a chain `follow_far_chain` walks */
    uint8_t        unknown_176[4];      /* +0x176 */
} __attribute__((packed));

_Static_assert(sizeof(struct sequence) == 0x17a, "create_sequence allocates 0x17a bytes");

/* **No sequence**, and **no node**, as pointers: 0000:0000, the null far
   pointer the guest tests with `or ax,dx` - see `PART_NONE` for why a
   sentinel and not NULL. */
#define SEQUENCE_NONE   ((struct sequence *)(void *)FAR_NULL_PTR)
#define SEQUENCE_PTR(fp) ((struct sequence *)(void *)dg_far_ptr((fp)))
#define SOUND_NODE_NONE ((struct sound_node *)(void *)FAR_NULL_PTR)

/*
 * ---------------------------------------------------------------------------
 * **A sound file's directory**, the block `open_sound_file` allocates and
 * fills from the file's own bytes: four bytes of cursor of its own making,
 * and then the file's directory image read in at +4.
 *
 * The walk steps `entry` by one record at a time - the original's `add si,6` -
 * and the cursor at +0 is where it left off, filed as this block's segment
 * beside its ninth byte, which is `entry[0]`.
 * ---------------------------------------------------------------------------
 */
struct sound_dir_entry {
    int16_t   id;              /* +0x00  what start_sequence_by_id asks for */
    uint32_t  at;              /* +0x02  where the record is in the file; the
                                         original loads it as two words */
} __attribute__((packed));

_Static_assert(sizeof(struct sound_dir_entry) == 6, "the walk steps by six");

struct sound_dir {
    struct far_ptr cursor;     /* +0x00  where the walk is, filed by open_sound_file */
    uint16_t  magic;           /* +0x04  2, or the file is not one of these */
    int16_t   count;           /* +0x06  how many entries follow */
    uint8_t   kind;            /* +0x08  handed to read_record as its mode */
    struct sound_dir_entry entry[1];   /* +0x09  `count` of them */
} __attribute__((packed));

DG_ASSERT_AT(struct sound_dir, magic,             0x04);
DG_ASSERT_AT(struct sound_dir, count,             0x06);
DG_ASSERT_AT(struct sound_dir, kind,              0x08);
DG_ASSERT_AT(struct sound_dir, entry,             0x09);

/*
 * ---------------------------------------------------------------------------
 * **A sound record**, the 0x14 bytes of kind 3 `read_record` makes for each
 * entry of a sound file and puts on the front of the list at `DG4A82.records`.
 * It lives in a DOS block, not in DGROUP.
 *
 * The header's fields are what `read_record` reads into it - the identifier, a
 * byte into +0xc, a byte into +0x12 - and what it loads is kept at +4. Bit 0 of
 * the flags says the payload is note data to be **sequenced** - kind 4, and
 * `start_sequence_by_id` builds a sequence for it and keeps it at +0xe - and a
 * clear bit a sample for a voice, kind 7. Bit 1 is copied into the sequence's
 * or the voice's loop byte, and bit 4 marks a start asked for while the device
 * could not take it. Those three names are readings of the code, **guessed**.
 * ---------------------------------------------------------------------------
 */
struct sound_record {
    struct far_ptr next;                /* +0x00  the next record, newest first */
    struct far_ptr data;                /* +0x04  what it loaded */
    uint16_t       size;                /* +0x08  the low word of the loaded size */
    int16_t        id;                  /* +0x0a  what `start_sequence_by_id` finds */
    uint16_t       priority;            /* +0x0c  a byte, copied into a sequence's */
    struct far_ptr sequence;            /* +0x0e  the sequence built for it, while one is */
    uint16_t       flags;               /* +0x12  bit 0 sequenced, bit 1 loop, bit 4 start pending */
} __attribute__((packed));

_Static_assert(sizeof(struct sound_record) == 0x14, "read_record allocates 0x14 bytes");
DG_ASSERT_AT(struct sound_record, data,              0x04);
DG_ASSERT_AT(struct sound_record, id,                0x0a);
DG_ASSERT_AT(struct sound_record, sequence,          0x0e);
DG_ASSERT_AT(struct sound_record, flags,             0x12);

#define SOUND_RECORD_NONE ((struct sound_record *)(void *)FAR_NULL_PTR)
#define SOUND_RECORD_PTR(fp) ((struct sound_record *)(void *)dg_far_ptr((fp)))
DG_ASSERT_AT(struct sequence, cursor_at,         0x008);
DG_ASSERT_AT(struct sequence, position,          0x00c);
DG_ASSERT_AT(struct sequence, delay,             0x04c);
DG_ASSERT_AT(struct sequence, byte_08c,          0x08c);
DG_ASSERT_AT(struct sequence, ch,                0x0bc);
DG_ASSERT_AT(struct sequence, ch.byte_0da,      0x0da);
DG_ASSERT_AT(struct sequence, ch.byte_143,      0x143);
DG_ASSERT_AT(struct sequence, loop_count,        0x152);
DG_ASSERT_AT(struct sequence, state,             0x158);
DG_ASSERT_AT(struct sequence, priority,          0x15c);
DG_ASSERT_AT(struct sequence, volume,            0x15e);
DG_ASSERT_AT(struct sequence, fade_target,       0x160);
DG_ASSERT_AT(struct sequence, poll,              0x165);
DG_ASSERT_AT(struct sequence, source,            0x166);
DG_ASSERT_AT(struct sequence, cursor,            0x16a);
DG_ASSERT_AT(struct sequence, next,              0x172);

DG_ASSERT_AT(struct vqt_reader, pos,               0x00);
DG_ASSERT_AT(struct vqt_reader, data,              0x04);
DG_ASSERT_AT(struct vqt_reader, plane,             0x08);
DG_ASSERT_AT(struct vqt_reader, row,               0x18);

DG_ASSERT_AT(struct bitmap, data,              0x00);
DG_ASSERT_AT(struct bitmap, mask_off,          0x04);
DG_ASSERT_AT(struct bitmap, width,             0x06);
DG_ASSERT_AT(struct bitmap, height,            0x08);

/*
 * ---------------------------------------------------------------------------
 * **A belt**, the 0x2c-byte record a part hangs off `belt_ptr[0]` and `belt_ptr[1]`.
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
 * takes `rope_ptr` and ages four chains whose generations are 0x10 apart, kinds
 * 7 and 0xa take `belt_ptr[0]` and age this one's, whose generations are 8 apart.
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
 * zero, so they are `point16` and not `point8`. The setup takes each with a
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
 * part's kind and form. `machine_isr_stack` ends at 0x3182 and `DG3576` begins at
 * 0x3576, so the region is bounded on both sides and this struct covers it
 * with nothing left over.
 *
 * Three kinds of table are interleaved, and the type of each is what the code
 * that reads it says:
 *
 *   `s_` a run of `point8`  - an outline, copied a point at a time;
 *   `p_` a run of `point16`    - the same shape in words, where every other
 *                                byte is zero, read with a byte move;
 *   `o_` a run of `dg_near_t`   - **offsets of the tables above**, indexed by
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
    struct point8  s_3182[8];              /* 0x000  0x3182  8 pairs */
    struct point8  s_3192[6];              /* 0x010  0x3192  6 pairs */
    struct point8  s_319e[6];              /* 0x01c  0x319e  6 pairs */
    struct point8  s_31aa[6];              /* 0x028  0x31aa  6 pairs */
    dg_near_t         o_31b6[3];              /* 0x034  0x31b6  3 offsets */
    struct point8  s_31bc[6];              /* 0x03a  0x31bc  6 pairs */
    struct point8  s_31c8[6];              /* 0x046  0x31c8  6 pairs */
    struct point8  s_31d4[6];              /* 0x052  0x31d4  6 pairs */
    dg_near_t         o_31e0[3];              /* 0x05e  0x31e0  3 offsets */
    int16_t           glove_reach[6];         /* 0x064  0x31e6  -32 -82 0 80 130 0: how far the
                                                 boxing glove reaches, `part_step_boxing_glove` */
    struct point8  s_31f2[6];              /* 0x070  0x31f2  6 pairs */
    struct point8  s_31fe[6];              /* 0x07c  0x31fe  6 pairs */
    struct point8  s_320a[6];              /* 0x088  0x320a  6 pairs */
    struct point8  s_3216[6];              /* 0x094  0x3216  6 pairs */
    struct point8  s_3222[4];              /* 0x0a0  0x3222  4 pairs */
    struct point8  s_322a[4];              /* 0x0a8  0x322a  4 pairs */
    struct point8  s_3232[8];              /* 0x0b0  0x3232  8 pairs */
    struct point8  s_3242[8];              /* 0x0c0  0x3242  8 pairs */
    struct point8  s_3252[5];              /* 0x0d0  0x3252  5 pairs */
    struct point8  s_325c[5];              /* 0x0da  0x325c  5 pairs */
    struct point8  s_3266[7];              /* 0x0e4  0x3266  7 pairs */
    struct point8  s_3274[7];              /* 0x0f2  0x3274  7 pairs */
    struct point8  s_3282[7];              /* 0x100  0x3282  7 pairs */
    struct point8  s_3290[5];              /* 0x10e  0x3290  5 pairs */
    struct point8  s_329a[5];              /* 0x118  0x329a  5 pairs */
    struct point8  s_32a4[5];              /* 0x122  0x32a4  5 pairs */
    struct point8  s_32ae[5];              /* 0x12c  0x32ae  5 pairs */
    struct point8  s_32b8[4];              /* 0x136  0x32b8  4 pairs */
    struct point8  s_32c0[4];              /* 0x13e  0x32c0  4 pairs */
    struct point8  s_32c8[5];              /* 0x146  0x32c8  5 pairs */
    struct point8  s_32d2[5];              /* 0x150  0x32d2  5 pairs */
    struct point16    p_32dc[8];              /* 0x15a  0x32dc  8 points */
    struct point8  s_32fc[6];              /* 0x17a  0x32fc  6 pairs */
    struct point8  s_3308[6];              /* 0x186  0x3308  6 pairs */
    struct point8  s_3314[7];              /* 0x192  0x3314  7 pairs */
    struct point8  s_3322[7];              /* 0x1a0  0x3322  7 pairs, the gun's points */
    uint8_t           conveyor_grab_x[5];     /* 0x1ae  0x3330  9 23 38 44 59: the grab x by width step,
                                                 `part_settle_conveyor` */
    uint8_t           unread_3335[1];         /* 0x1b3  0x3335 */
    struct point8  s_3336[7];              /* 0x1b4  0x3336  7 pairs */
    struct point8  s_3344[4];              /* 0x1c2  0x3344  4 pairs */
    struct point8  s_334c[4];              /* 0x1ca  0x334c  4 pairs */
    struct point8  s_3354[4];              /* 0x1d2  0x3354  4 pairs */
    struct point8  s_335c[4];              /* 0x1da  0x335c  4 pairs */
    dg_near_t         o_3364[4];              /* 0x1e2  0x3364  4 offsets */
    struct point8  s_336c[4];              /* 0x1ea  0x336c  4 pairs */
    struct point8  s_3374[4];              /* 0x1f2  0x3374  4 pairs */
    struct point8  s_337c[4];              /* 0x1fa  0x337c  4 pairs */
    struct point8  s_3384[4];              /* 0x202  0x3384  4 pairs */
    dg_near_t         o_338c[4];              /* 0x20a  0x338c  4 offsets */
    int16_t           jack_reach[3];          /* 0x212  0x3394  -21 -34 -59: how far the jack-in-the-box
                                                 reaches by form, `part_step_jack_in_the_box` */
    struct point16    p_339a[4];              /* 0x218  0x339a  4 points */
    struct point8  s_33aa[9];              /* 0x228  0x33aa  9 pairs */
    struct point8  s_33bc[9];              /* 0x23a  0x33bc  9 pairs */
    struct point8  s_33ce[4];              /* 0x24c  0x33ce  4 pairs */
    struct point8  s_33d6[4];              /* 0x254  0x33d6  4 pairs */
    struct point8  s_33de[4];              /* 0x25c  0x33de  4 pairs */
    dg_near_t         o_33e6[3];              /* 0x264  0x33e6  3 offsets */
    struct point8  s_33ec[4];              /* 0x26a  0x33ec  4 pairs */
    struct point8  s_33f4[4];              /* 0x272  0x33f4  4 pairs */
    struct point8  s_33fc[4];              /* 0x27a  0x33fc  4 pairs */
    dg_near_t         o_3404[3];              /* 0x282  0x3404  3 offsets */
    struct point16    p_340a[3];              /* 0x288  0x340a  3 points */
    struct point16    p_3416[3];              /* 0x294  0x3416  3 points */
    struct point8  s_3422[8];              /* 0x2a0  0x3422  8 pairs */
    struct point8  s_3432[8];              /* 0x2b0  0x3432  8 pairs */
    struct point8  s_3442[8];              /* 0x2c0  0x3442  8 pairs */
    struct point8  s_3452[8];              /* 0x2d0  0x3452  8 pairs */
    struct point8  s_3462[8];              /* 0x2e0  0x3462  8 pairs */
    struct point8  s_3472[8];              /* 0x2f0  0x3472  8 pairs */
    struct point8  s_3482[8];              /* 0x300  0x3482  8 pairs */
    dg_near_t         o_3492[2];              /* 0x310  0x3492  2 offsets */
    struct point8  s_3496[8];              /* 0x314  0x3496  8 pairs */
    struct point8  s_34a6[8];              /* 0x324  0x34a6  8 pairs */
    dg_near_t         o_34b6[2];              /* 0x334  0x34b6  2 offsets */
    /* **The scissors' blade**, a segment of four words - x0, y0, x1, y1 - once
       as it stands and once mirrored; `part_step_scissors` picks one by the flip
       bit and `cut_belts` cuts every belt that crosses it. */
    int16_t           cut_line[2][4];         /* 0x338  0x34ba */
    struct point16    p_34ca[3];              /* 0x348  0x34ca  3 points */
    struct point16    p_34d6[3];              /* 0x354  0x34d6  3 points */
    struct point16    p_34e2[8];              /* 0x360  0x34e2  8 points */
    struct point16    p_3502[8];              /* 0x380  0x3502  8 points */
    struct point16    p_3522[8];              /* 0x3a0  0x3522  8 points */
    /* **The seesaw's shaft by form**, a segment of four words each, which
       `part_step_seesaw` hands `link_objects_crossing`. */
    int16_t           shaft_line[3][4];       /* 0x3c0  0x3542 */
} __attribute__((packed));

extern struct part_shapes PARTSHAPES;

_Static_assert(sizeof(struct part_shapes) == 0x3d8,
               "the shape tables run from 0x3182 to the IFF chunk names at 0x355a");

/*
 * **The IFF chunk names**, DGROUP 0x355a..0x3576, and the mode the file is
 * written with - the ILBM writer's literals, which follow the shape tables
 * rather than belonging to them. They were the tail of `p_3522`'s 21 points
 * until the six strings were read as what they are.
 */
struct iff_chunk_names {
    char      form[5];            /* +0x00  "FORM" */
    char      ilbm[5];            /* +0x05  "ILBM" */
    char      bmhd[5];            /* +0x0a  "BMHD" */
    char      cmap[5];            /* +0x0f  "CMAP" */
    char      body[5];            /* +0x14  "BODY" */
    char      mode_wb[3];         /* +0x19  "wb" */
} __attribute__((packed));
_Static_assert(sizeof(struct iff_chunk_names) == 0x1c, "ends at 0x3576, DG3576");
extern struct iff_chunk_names IFF_CHUNK_NAMES;
DG_ASSERT_AT(struct part_shapes, s_3182,            0x000);
DG_ASSERT_AT(struct part_shapes, s_3192,            0x010);
DG_ASSERT_AT(struct part_shapes, s_319e,            0x01c);
DG_ASSERT_AT(struct part_shapes, s_31aa,            0x028);
DG_ASSERT_AT(struct part_shapes, o_31b6,            0x034);
DG_ASSERT_AT(struct part_shapes, s_31bc,            0x03a);
DG_ASSERT_AT(struct part_shapes, s_31c8,            0x046);
DG_ASSERT_AT(struct part_shapes, s_31d4,            0x052);
DG_ASSERT_AT(struct part_shapes, o_31e0,            0x05e);
DG_ASSERT_AT(struct part_shapes, glove_reach,       0x064);
DG_ASSERT_AT(struct part_shapes, s_31f2,            0x070);
DG_ASSERT_AT(struct part_shapes, s_31fe,            0x07c);
DG_ASSERT_AT(struct part_shapes, s_320a,            0x088);
DG_ASSERT_AT(struct part_shapes, s_3216,            0x094);
DG_ASSERT_AT(struct part_shapes, s_3222,            0x0a0);
DG_ASSERT_AT(struct part_shapes, s_322a,            0x0a8);
DG_ASSERT_AT(struct part_shapes, s_3232,            0x0b0);
DG_ASSERT_AT(struct part_shapes, s_3242,            0x0c0);
DG_ASSERT_AT(struct part_shapes, s_3252,            0x0d0);
DG_ASSERT_AT(struct part_shapes, s_325c,            0x0da);
DG_ASSERT_AT(struct part_shapes, s_3266,            0x0e4);
DG_ASSERT_AT(struct part_shapes, s_3274,            0x0f2);
DG_ASSERT_AT(struct part_shapes, s_3282,            0x100);
DG_ASSERT_AT(struct part_shapes, s_3290,            0x10e);
DG_ASSERT_AT(struct part_shapes, s_329a,            0x118);
DG_ASSERT_AT(struct part_shapes, s_32a4,            0x122);
DG_ASSERT_AT(struct part_shapes, s_32ae,            0x12c);
DG_ASSERT_AT(struct part_shapes, s_32b8,            0x136);
DG_ASSERT_AT(struct part_shapes, s_32c0,            0x13e);
DG_ASSERT_AT(struct part_shapes, s_32c8,            0x146);
DG_ASSERT_AT(struct part_shapes, s_32d2,            0x150);
DG_ASSERT_AT(struct part_shapes, p_32dc,            0x15a);
DG_ASSERT_AT(struct part_shapes, s_32fc,            0x17a);
DG_ASSERT_AT(struct part_shapes, s_3308,            0x186);
DG_ASSERT_AT(struct part_shapes, s_3314,            0x192);
DG_ASSERT_AT(struct part_shapes, s_3322,            0x1a0);
DG_ASSERT_AT(struct part_shapes, conveyor_grab_x,   0x1ae);
DG_ASSERT_AT(struct part_shapes, s_3336,            0x1b4);
DG_ASSERT_AT(struct part_shapes, s_3344,            0x1c2);
DG_ASSERT_AT(struct part_shapes, s_334c,            0x1ca);
DG_ASSERT_AT(struct part_shapes, s_3354,            0x1d2);
DG_ASSERT_AT(struct part_shapes, s_335c,            0x1da);
DG_ASSERT_AT(struct part_shapes, o_3364,            0x1e2);
DG_ASSERT_AT(struct part_shapes, s_336c,            0x1ea);
DG_ASSERT_AT(struct part_shapes, s_3374,            0x1f2);
DG_ASSERT_AT(struct part_shapes, s_337c,            0x1fa);
DG_ASSERT_AT(struct part_shapes, s_3384,            0x202);
DG_ASSERT_AT(struct part_shapes, o_338c,            0x20a);
DG_ASSERT_AT(struct part_shapes, jack_reach,        0x212);
DG_ASSERT_AT(struct part_shapes, p_339a,            0x218);
DG_ASSERT_AT(struct part_shapes, s_33aa,            0x228);
DG_ASSERT_AT(struct part_shapes, s_33bc,            0x23a);
DG_ASSERT_AT(struct part_shapes, s_33ce,            0x24c);
DG_ASSERT_AT(struct part_shapes, s_33d6,            0x254);
DG_ASSERT_AT(struct part_shapes, s_33de,            0x25c);
DG_ASSERT_AT(struct part_shapes, o_33e6,            0x264);
DG_ASSERT_AT(struct part_shapes, s_33ec,            0x26a);
DG_ASSERT_AT(struct part_shapes, s_33f4,            0x272);
DG_ASSERT_AT(struct part_shapes, s_33fc,            0x27a);
DG_ASSERT_AT(struct part_shapes, o_3404,            0x282);
DG_ASSERT_AT(struct part_shapes, p_340a,            0x288);
DG_ASSERT_AT(struct part_shapes, p_3416,            0x294);
DG_ASSERT_AT(struct part_shapes, s_3422,            0x2a0);
DG_ASSERT_AT(struct part_shapes, s_3432,            0x2b0);
DG_ASSERT_AT(struct part_shapes, s_3442,            0x2c0);
DG_ASSERT_AT(struct part_shapes, s_3452,            0x2d0);
DG_ASSERT_AT(struct part_shapes, s_3462,            0x2e0);
DG_ASSERT_AT(struct part_shapes, s_3472,            0x2f0);
DG_ASSERT_AT(struct part_shapes, s_3482,            0x300);
DG_ASSERT_AT(struct part_shapes, o_3492,            0x310);
DG_ASSERT_AT(struct part_shapes, s_3496,            0x314);
DG_ASSERT_AT(struct part_shapes, s_34a6,            0x324);
DG_ASSERT_AT(struct part_shapes, o_34b6,            0x334);
DG_ASSERT_AT(struct part_shapes, cut_line,          0x338);
DG_ASSERT_AT(struct part_shapes, p_34ca,            0x348);
DG_ASSERT_AT(struct part_shapes, p_34d6,            0x354);
DG_ASSERT_AT(struct part_shapes, p_34e2,            0x360);
DG_ASSERT_AT(struct part_shapes, p_3502,            0x380);
DG_ASSERT_AT(struct part_shapes, p_3522,            0x3a0);
DG_ASSERT_AT(struct part_shapes, shaft_line,        0x3c0);

struct belt {
    dg_near_t owner_ptr;       /* +0x00  the part this belt hangs off */
    dg_near_t end_a_ptr;       /* +0x02  the part end A is attached to */
    dg_near_t end_b_ptr;       /* +0x04  the part end B is attached to */
    dg_near_t home_a_ptr;      /* +0x06  the attachment the level file gave */
    dg_near_t home_b_ptr;      /* +0x08 */
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

DG_ASSERT_AT(struct belt, end_a_ptr,         0x02);
DG_ASSERT_AT(struct belt, end_b_ptr,         0x04);
DG_ASSERT_AT(struct belt, home_a_ptr,        0x06);
DG_ASSERT_AT(struct belt, home_b_ptr,        0x08);
DG_ASSERT_AT(struct belt, slot_a,            0x0a);
DG_ASSERT_AT(struct belt, slot_b,            0x0b);
DG_ASSERT_AT(struct belt, home_slot_a,       0x0c);
DG_ASSERT_AT(struct belt, home_slot_b,       0x0d);
DG_ASSERT_AT(struct belt, v,                 0x0e);
DG_ASSERT_AT(struct belt, pt,                0x14);
_Static_assert(sizeof(struct belt) == 0x2c,
               "a belt is what heap_calloc_far(1, 0x2c) makes");

#define BELT_PTR(p) ((struct belt *)(dgroup + (uint16_t)(p)))

/* **No belt**, as a pointer - the same address an offset of 0 names. See
   `PART_NONE`: every accessor here is `dgroup + off`, so a walk held as a
   typed pointer ends on this and never on NULL. */
#define BELT_NONE BELT_PTR(0)

/*
 * ---------------------------------------------------------------------------
 * **A rope**, the 0x38-byte record a kind-8 part hangs off `rope_ptr` - not a
 * belt, which is `struct belt` above and hangs off `belt_ptr[0]`.
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
    /* **Nothing in the port reads or writes it**, and a rope is a heap record
       reached through a pointer, so the image cannot be searched for a use
       the way a DGROUP offset can. Named for what is known. */
    uint16_t  _pad_00;         /* +0x00 */
    dg_near_t owner_ptr;       /* +0x02  the part this rope hangs off */
    dg_near_t end_a_ptr;       /* +0x04  the part end A is attached to */
    dg_near_t end_b_ptr;       /* +0x06  the part end B is attached to */
    struct point16 pt[3][4];   /* +0x08  three generations of four corners:
                                         gen 3 at +0x08, gen 2 at +0x18,
                                         gen 1 at +0x28 */
} __attribute__((packed));

DG_ASSERT_AT(struct rope, owner_ptr,         0x02);
DG_ASSERT_AT(struct rope, end_a_ptr,         0x04);
DG_ASSERT_AT(struct rope, end_b_ptr,         0x06);
DG_ASSERT_AT(struct rope, pt,                0x08);
_Static_assert(sizeof(struct rope) == 0x38,
               "a rope is what clone_part makes with heap_calloc_far(1, 0x38)");

#define ROPE_PTR(p) ((struct rope *)(dgroup + (uint16_t)(p)))

/* **No rope**, as a pointer - see `BELT_NONE` and `PART_NONE`. */
#define ROPE_NONE ROPE_PTR(0)

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

/* **No point list**, as a pointer - see `PART_NONE`. */
#define POINTS_NONE POINTS(0)

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
    /* **The kind's density**, and what the sign of its gravity comes from.
       `recompute_kind_physics` bends the world's gravity setting into a
       `base` and compares this against it: heavier falls, lighter rises,
       equal gets none. Measured against the table itself - the balloon is 9
       where that `base` is 33 at the default gravity, and the balloon is the
       part that goes up; the bowling ball is 2832. */
    uint16_t  density;         /* +0x00 */
    int16_t   weight;          /* +0x02 */
    /* **How bouncy and how grippy the material is**, both taken from the two
       kinds in contact rather than from one: `bounce_pair` takes the
       *smaller* bounce of the two and multiplies the velocity by it, and
       `apply_contact_friction` takes the *larger* grip - except against a
       running conveyor, which forces 0x100. */
    int16_t   bounce;          /* +0x04 */
    int16_t   grip;            /* +0x06 */
    int16_t   gravity;         /* +0x08  the normal load, same field */
    /* **The velocity clamp, not padding.** `clamp_record_pair` bounds a part's
       `vel_x` and `vel_y` to plus and minus this. */
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
    dg_near_t bitmaps_ptr;     /* +0x14  a bmp_set, indexed by form */
    dg_near_t bitmaps2_ptr;    /* +0x16  a second one */
    /* **Two tables the form indexes**, each a DGROUP offset or 0 for none: the
       hot spot of each form as a `point8`, which `place_object_for_draw` adds
       to the position and mirrors within `mirror_size` when the part is
       flipped, and the size of each form as a `point16`, which
       `set_object_extent` takes in preference to the bitmap's own. */
    dg_near_t hotspots_ptr;    /* +0x18 */
    dg_near_t sizes_ptr;       /* +0x1a */
    /* **Two level bounds, not padding.** `refile_overlapping_parts` compares a
       draw level against each, with 0xff meaning no limit. The name is a
       reading of that comparison and nothing more. */
    uint8_t   refile_level[2]; /* +0x1c */
    uint16_t  point_count;     /* +0x1e */
    /* **Where the kind sorts in the parts bin.** `insert_sorted` walks the bin
       until it meets a kind with a larger one, so the bin's order is this
       number and not the kind's. The moving list sorts by weight instead, and
       the placed list not at all. */
    uint16_t  priority;        /* +0x20 */
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

DG_ASSERT_AT(struct part_kind, weight,            0x02);
DG_ASSERT_AT(struct part_kind, bounce,           0x04);
DG_ASSERT_AT(struct part_kind, grip,           0x06);
DG_ASSERT_AT(struct part_kind, gravity,           0x08);
DG_ASSERT_AT(struct part_kind, max_speed,         0x0a);
DG_ASSERT_AT(struct part_kind, max_w,             0x0c);
DG_ASSERT_AT(struct part_kind, max_h,             0x0e);
DG_ASSERT_AT(struct part_kind, min_w,             0x10);
DG_ASSERT_AT(struct part_kind, min_h,             0x12);
DG_ASSERT_AT(struct part_kind, bitmaps_ptr,       0x14);
DG_ASSERT_AT(struct part_kind, bitmaps2_ptr,      0x16);
DG_ASSERT_AT(struct part_kind, hotspots_ptr,           0x18);
DG_ASSERT_AT(struct part_kind, sizes_ptr,           0x1a);
DG_ASSERT_AT(struct part_kind, refile_level,      0x1c);
DG_ASSERT_AT(struct part_kind, point_count,       0x1e);
DG_ASSERT_AT(struct part_kind, hit,               0x22);
DG_ASSERT_AT(struct part_kind, step,              0x26);
DG_ASSERT_AT(struct part_kind, setup,             0x2a);
DG_ASSERT_AT(struct part_kind, flip,              0x2e);
DG_ASSERT_AT(struct part_kind, settle,            0x32);
DG_ASSERT_AT(struct part_kind, drive,             0x36);
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
/*
 * **The kind table**: one `struct part_kind` per kind, from DGROUP 0x0ea6 to
 * 0x1bca, where the strings start. Fifty-eight records, which is what
 * `free_all_part_bitmaps` walks - 0 to 0x39 - and what the image holds before
 * the text. The original reaches a record as `imul 0x3a` then `add ax, 0xea6`,
 * or with the base folded into the displacement when it reads one field -
 * `[bx + 0xec6]` is `word_20` - so the index is the only thing it computes.
 *
 * Its contents are transcribed in dgroup.c, at that address.
 */
#define PART_KIND_COUNT 58
extern struct part_kind PART_KINDS[PART_KIND_COUNT];

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
    dg_near_t next_ptr;        /* +0x00 */
    dg_near_t part;            /* +0x02  the part that asked to move */
    int32_t   momentum;        /* +0x04  the sort key: one Borland `long`,
                                         compared as `jg`/`jl` on the high
                                         word and `jae`/`ja` on the low */
} __attribute__((packed));

DG_ASSERT_AT(struct queue_node, part,              0x02);
DG_ASSERT_AT(struct queue_node, momentum,          0x04);
_Static_assert(sizeof(struct queue_node) == 8, "a queue node is what heap_calloc_far(1, 8) makes");

#define QNODE_PTR(p) ((struct queue_node *)(dgroup + (uint16_t)(p)))

/* **No queue node**, as a pointer - see `PART_NONE`. */
#define QNODE_NONE QNODE_PTR(0)

/*
 * ---------------------------------------------------------------------------
 * **A saved-rectangle list entry**, 0x1a bytes, chained through +0x18 on one
 * of the twenty heads in `MACHINE_RECT_SLOTS.slot[]` and returned whole to
 * `MACHINE_RECT_FREE.rect_free_ptr`.
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
    dg_seg_t  page_src;        /* +0x08  the pages the rect is restored between: */
    dg_seg_t  page_dst;        /* +0x0a  0xa000, 0xa800 or 0xa820, or 0xffff for mode 4 */
    uint16_t  mode;            /* +0x0c  1 copies the rect, 4 restores it from `buf` */
    int16_t   refcount;             /* +0x0e  how many hold the slot; stepped down once a frame, reusable at 0 */
    uint16_t  area;            /* +0x10  w * h, from the creator's imul */
    uint16_t  block_head;      /* +0x12  1 on the first record of each heap block */
    struct far_ptr buf;        /* +0x14  the saved pixels, for mode 4 */
    dg_near_t next_ptr;        /* +0x18 */
} __attribute__((packed));

DG_ASSERT_AT(struct rect_list_entry, page_src,          0x08);
DG_ASSERT_AT(struct rect_list_entry, mode,              0x0c);
DG_ASSERT_AT(struct rect_list_entry, refcount,          0x0e);
DG_ASSERT_AT(struct rect_list_entry, buf,               0x14);
DG_ASSERT_AT(struct rect_list_entry, next_ptr,          0x18);
_Static_assert(sizeof(struct rect_list_entry) == 0x1a, "a rect list entry is 0x1a bytes");

#define RECTENT_PTR(p) ((struct rect_list_entry *)(dgroup + (uint16_t)(p)))

/* **No rect list entry**, as a pointer - see `PART_NONE`. */
#define RECTENT_NONE RECTENT_PTR(0)

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
    struct extent16 set_size;  /* +0x04  ... the part's set_size at +0x50 */
    struct extent16 size;      /* +0x08  ... the part's size[0] at +0x44 */
    struct far_ptr init;       /* +0x0c  the kind's init routine, called
                                         far */
} __attribute__((packed));

DG_ASSERT_AT(struct part_template, flags_0a,          0x02);
DG_ASSERT_AT(struct part_template, set_size,          0x04);
DG_ASSERT_AT(struct part_template, size,              0x08);
DG_ASSERT_AT(struct part_template, init,              0x0c);
_Static_assert(sizeof(struct part_template) == 0x10,
               "a part template is what make_part strides by");

/* The templates, one per kind, and the two words after them that nothing is
   known to read. */
extern struct part_template PART_TEMPLATES[PART_KIND_COUNT];

/*
 * ---------------------------------------------------------------------------
 * **A resource stream**, the 0x21-byte record `open_resource_slot` makes and
 * files in the table at DGROUP 0x57c0. `ENGINE_STREAM.record_ptr` points at whichever
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
 * because `ENGINE_STREAM.kind & 0x20` is the bit that chooses between reading the
 * stream from a file and reading it out of memory, and these two routines are
 * on opposite sides of it.
 *
 * Field names below the offsets are ours; the offsets and the size are the
 * original's.
 * ---------------------------------------------------------------------------
 */
struct resource {
    dg_near_t work_ptr;        /* +0x00  the near buffer prepare_resource_slot makes */
    struct far_ptr scratch;    /* +0x02  the far scratch block, which
                                  lzss_reset caches */
    /* **Where the resource's data lies.** Without bit 0x20 of `kind` it is a
       far pointer into memory: `select_resource`, `resource_seek` and
       `restart_resource_stream` add to it through the runtime's huge add at
       0x0bf0a and normalise the answer. With bit 0x20 the resource is read from
       a file and only the offset word is used - it holds the file record's
       near pointer, which `open_resource` files there and `select_resource`
       copies to 0x57bc. Two readings of the same four bytes, chosen by the
       kind, so a union. */
    union {
        struct far_ptr data;   /* +0x06 */
        dg_near_t file_ptr;    /* +0x06  with bit 0x20: the file record */
    };
    /* **Three Borland `long`s.** `read_input_block` takes `end - in` with a
       borrow and compares the two as wholes; `next_input_byte` steps `in`
       with a carry; `open_resource` splits a `uint32_t` into `end` and
       `resource_tell` joins `in` back into one. */
    uint32_t  in;              /* +0x0a  how far into the compressed input
                                         the reader is */
    uint32_t  end;             /* +0x0e  where the compressed input ends */
    int32_t   size;            /* +0x12  what resource_seek measures from for
                                         SEEK_END */
    int32_t   pos;             /* +0x16  and what it measures from for
                                         SEEK_CUR. Stepped with a carry by
                                         `read_resource`, subtracted from
                                         `size` with a borrow, and compared
                                         against the target **signed** - the
                                         original's `cmp hi / jg / jl / cmp
                                         lo / ja` over the pair. */
    /* **Two bytes indexing the spill buffer** at `work_ptr`, where a run that
       does not fit the caller's request goes - names that are guesses. The
       emitters and both decompressors advance the end; `resource_advance`
       hands `end - start` over, advances the start, and zeroes both once the
       buffer is drained. Every access in the original is byte-wide except
       `inc word ptr [si+0x1a]` at 0x1cc0a in `decompress_lzw`, whose carry out
       of the end lands in the start - spelled out as a carry at that site. */
    uint8_t   spill_end;       /* +0x1a */
    uint8_t   spill_start;     /* +0x1b */
    uint32_t  start;           /* +0x1c  where in the file the resource begins,
                                         from game_ftell at open */
    uint8_t   kind;            /* +0x20  the type prepare_resource_slot was given */
} __attribute__((packed));

DG_ASSERT_AT(struct resource, scratch,           0x02);
DG_ASSERT_AT(struct resource, data,              0x06);
DG_ASSERT_AT(struct resource, in,                0x0a);
DG_ASSERT_AT(struct resource, end,               0x0e);
DG_ASSERT_AT(struct resource, size,              0x12);
DG_ASSERT_AT(struct resource, pos,               0x16);
DG_ASSERT_AT(struct resource, spill_end,         0x1a);
DG_ASSERT_AT(struct resource, spill_start,       0x1b);
DG_ASSERT_AT(struct resource, start,             0x1c);
DG_ASSERT_AT(struct resource, kind,              0x20);
_Static_assert(sizeof(struct resource) == 0x21,
               "a resource is what heap_calloc_far(1, 0x21) makes");

#define RESOURCE_PTR(p) ((struct resource *)(dgroup + (uint16_t)(p)))

/* **No resource slot**, as a pointer - see `PART_NONE`. */
#define RESOURCE_NONE RESOURCE_PTR(0)

DG_ASSERT_AT(struct file_rec, level,             0x00);
DG_ASSERT_AT(struct file_rec, flags,             0x02);
DG_ASSERT_AT(struct file_rec, fd,                0x04);
DG_ASSERT_AT(struct file_rec, bsize,             0x06);
DG_ASSERT_AT(struct file_rec, curp_ptr,          0x0a);

/*
 * **The level screens' string literals**, DGROUP 0x2824..0x284a, 0x26 bytes - Borland files a
 * copy of every literal beside the routine that uses it, which is why "*.TIM"
 * is here twice. Named by their users; the bytes are the image's, and the
 * run ends at the hot spots at 0x284a.
 */
struct game_level_strings {
    char ff_lev[7];               /* +0x00 [7]  "ff.lev"   screen_state_0400 */
    char tim_filter_load[6];      /* +0x07 [6]  "*.TIM"    screen_state_0100's pick_file */
    char tim_filter_save[6];      /* +0x0d [6]  "*.TIM"    screen_state_0080's */
    char title_sep[3];            /* +0x13 [3]  ": "       paint_panel_frame */
    char replay[7];               /* +0x16 [7]  "REPLAY"   finish_level's two buttons */
    char advance[8];              /* +0x1d [8]  "ADVANCE" */
    uint8_t pad_2849[1];          /* +0x25 [1] */
} __attribute__((packed));

extern struct game_level_strings GAME_LEVEL_STRINGS;

/*
 * ---------------------------------------------------------------------------
 * **The rest of the image's DGROUP data**: what no struct above covers, typed
 * as far as it is read. Everything here is **Not established** unless its
 * comment says otherwise, and the names are ours.
 * ---------------------------------------------------------------------------
 */

/* DGROUP 0x2d06..0x2d0a, after the part templates: two words. */
struct dg_2d06 {
    /* **Nothing in the port touches either word**, and the image holds no
       instruction that names 0x2d06 or 0x2d08 - so whatever reads them, if
       anything does, reaches them through a pointer. The image's own bytes are
       the initialisers below and `check_image_data` holds them to that. */
    uint16_t  _pad_2d06;       /* +0x00 */
    uint16_t  _pad_2d08;           /* +0x02 */
} __attribute__((packed));
extern struct dg_2d06 DG2D06;

/*
 * DGROUP 0x440e..0x4460: twenty far pointers after DG4342's, and two bytes.
 * `vm_driver_init(0x3890, 0x4412, DGROUP_SEG)` hands the driver the table from
 * the second, so the last nineteen are the driver's; what the first is is not
 * known. Segment 1c25 and segment 0000 are both code.
 */
struct dg_440e {
    struct far_ptr ptr_440e;       /* +0x00 */
    struct far_ptr driver_table[19]; /* +0x04  0x4412 */
    uint8_t   pad_445e[2];         /* +0x50 */
} __attribute__((packed));
_Static_assert(sizeof(struct dg_440e) == 0x52, "ends at 0x4460, ENGINE_PEN");
extern struct dg_440e DG440E;

/* DGROUP 0x44ea..0x44ee: one far pointer, into segment 1c25's code. */
extern struct far_ptr DG44EA;

/* DGROUP 0x4ab0..0x4ab4: two words - the second is 0x2b11, 11025, which is a
   sample rate, and that is all that is known. */
struct dg_4ab0 {
    /* **The same shape**: nothing in the port touches them and the image names
       neither offset. 0x2b11 has the look of a DGROUP offset and 0xfffe of a
       -2, which is as far as the evidence goes. */
    uint16_t  _pad_4ab0;       /* +0x00 */
    uint16_t  _pad_4ab2;           /* +0x02 */
} __attribute__((packed));
extern struct dg_4ab0 DG4AB0;

#endif /* DGROUP_H */
