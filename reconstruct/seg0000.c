/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file: it is
 * derived from someone else's executable.
 *
 * This file corresponds to the original's **code segment 0000**, image
 * 0x00000..0x0dff0. The binary is Borland C++ large model, so each translation
 * unit is its own code segment and this file mirrors one of them. Functions
 * are in address order and each carries the image offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x002be
 *
 * Subtract two fields of the structure that DGROUP 0x5400 points at from two
 * words beside it. What the structure is has not been established; only the
 * two fields it touches, at +0x22 and +0x24, and the fact that the pointer is
 * a **near** one - a DGROUP offset dereferenced as `[bx + 0x22]`, which is why
 * DGROUP has to be memory rather than a set of named globals.
 *
 * The pointer is re-read from DGROUP for the second field, exactly as here.
 */
void sub_002be(void)
{
    word_5414 = (int16_t)(word_5420 - DG16(ptr_5400 + 0x22));
    word_5402 = (int16_t)(word_541c - DG16(ptr_5400 + 0x24));
}

/*
 * 0x0144e
 *
 * Step the counter at DGROUP 0x4e87, wrapping 0x2a00 back to 0x1c00. What it
 * counts is not established; the range is 0x1c00..0x29ff.
 *
 * **The wrap is unverified.** It needs 10,752 calls to reach, and over the two
 * intro screens the routine is called 428 times with the counter never above
 * 0x1ab. The branch is transcribed from the disassembly and has never been
 * run against the original.
 */
void step_word_4e87(void)
{
    word_4e87++;
    if (word_4e87 == 0x2a00)
        word_4e87 = 0x1c00;
}

/*
 * 0x082c3
 *
 * Set the clip box: to the saved rectangle at DGROUP 0x52d7..0x52dd when the
 * mode word at 0x4e6b is any of seven values, and to a fixed one otherwise.
 *
 * The seven are single bits - 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000,
 * 0x8000 - but they are compared **for equality**, one at a time, not tested
 * as a mask, so a word with two of them set matches none. Transcribed as seven
 * compares rather than folded into a mask test.
 *
 * The saved rectangle is stored in descending order - 0x52dd is the left edge
 * and 0x52d7 the bottom - which is worth saying because it looks like a
 * transcription error otherwise.
 *
 * The fixed box is 0x110,0x48 to 0x20f,0xe7: 256 wide by 160 tall.
 */
void set_clip_for_mode(void)
{
    uint16_t mode = DGU16(0x4E6B);

    if (mode == 0x2000 || mode == 0x1000 || mode == 0x200 || mode == 0x8000
        || mode == 0x4000 || mode == 0x800 || mode == 0x400) {
        clip_left = DG16(0x52DD);
        clip_right = DG16(0x52DB);
        clip_top = DG16(0x52D9);
        clip_bottom = DG16(0x52D7);
    } else {
        clip_left = 0x110;
        clip_right = 0x20F;
        clip_top = 0x48;
        clip_bottom = 0xE7;
    }
}

/*
 * 0x08136
 *
 * Advance the button state for one frame. Three states live in DGROUP 0x5774
 * and the previous frame's is kept at 0x286e:
 *
 *   0  not pressed
 *   1  held
 *   2  the frame of a change
 *
 * It waits for the frame first, takes the two flag bits, and then walks the
 * state machine. The last test - a 2 while the previous frame was also 2
 * becomes a 1 - is what stops "changed" lasting two frames in a row.
 *
 * The third branch is `cmp [0x5774],0 / je`, so anything that is not zero
 * becomes 1: the state is normalised, not merely tested.
 */
void update_button_state(void)
{
    int16_t prev;

    wait_and_latch_frame();
    prev = DG16(0x5774);

    if (flag_bit_48ea(0))
        DG16(0x5774) = 1;
    if (flag_bit_48ea(1))
        DG16(0x5772) = 2;

    if (prev == 2 && DG16(0x286E) != 1) {
        DG16(0x5774) = 2;
    } else if (DG16(0x5774) == 1 && DG16(0x286E) == 0) {
        DG16(0x5774) = 2;
    } else if (DG16(0x5774) != 0) {
        DG16(0x5774) = 1;
    } else {
        DG16(0x5774) = 0;
    }

    if (DG16(0x5774) == 2 && DG16(0x286E) == 2)
        DG16(0x5774) = 1;

    DG16(0x286E) = DG16(0x5774);
}

/*
 * 0x0834b
 *
 * Set the clipping box to the whole visible screen: 0,0 to 639,399. The
 * bottom is 0x18f, which is the blanking line the CRTC is programmed with -
 * so the clip box is the *visible* 400 rows, not the 480 the mode scans.
 */
void set_clip_full_screen(void)
{
    clip_left = 0;
    clip_top = 0;
    clip_right = 0x27F;
    clip_bottom = 0x18F;
}

/*
 * 0x002dd
 *
 * Build the **swept** bounding box of the object at DGROUP 0x5400: the union
 * of where it is and where it was, which is what a dirty-rectangle redraw has
 * to repaint.
 *
 * The current box comes from the same fields `compute_bounds_53fe` uses -
 * +0x1e/+0x20 for the corner and +0x44/+0x46 for the extents - into
 * 0x5420/0x541c and 0x541e/0x541a, with the centre at 0x5418/0x5416. Then
 * `sub_002be` fills 0x5414 and 0x5402 with how far the object has moved, from
 * the previous position at +0x22/+0x24.
 *
 * The box is then stretched both ways: the near edges at 0x5412/0x5410 move
 * back to the old position if that was further back, and the far edges are
 * pushed out by the **absolute** movement - the branchless
 * `cwd / xor / sub` again.
 *
 * As in 0x00386, the pointer at 0x5400 is re-read before every field.
 */
void compute_swept_bounds_5400(void)
{
    int16_t d;

    DG16(0x5420) = DG16(DGU16(0x5400) + 0x1E);
    DG16(0x5412) = DG16(0x5420);
    DG16(0x541C) = DG16(DGU16(0x5400) + 0x20);
    DG16(0x5410) = DG16(0x541C);

    DG16(0x541E) = (int16_t)(DG16(0x5420) + DG16(DGU16(0x5400) + 0x44));
    DG16(0x541A) = (int16_t)(DG16(0x541C) + DG16(DGU16(0x5400) + 0x46));

    DG16(0x5418) = (int16_t)(DG16(0x5420)
                             + (int16_t)(DG16(DGU16(0x5400) + 0x44) >> 1));
    DG16(0x5416) = (int16_t)(DG16(0x541C)
                             + (int16_t)(DG16(DGU16(0x5400) + 0x46) >> 1));

    sub_002be();

    if (DG16(DGU16(0x5400) + 0x22) < DG16(0x5420))
        DG16(0x5412) = DG16(DGU16(0x5400) + 0x22);
    if (DG16(DGU16(0x5400) + 0x24) < DG16(0x541C))
        DG16(0x5410) = DG16(DGU16(0x5400) + 0x24);

    d = DG16(0x5414);
    if (d < 0)
        d = (int16_t)-d;
    DG16(0x541E) = (int16_t)(DG16(0x541E) + d);

    d = DG16(0x5402);
    if (d < 0)
        d = (int16_t)-d;
    DG16(0x541A) = (int16_t)(DG16(0x541A) + d);
}

/*
 * 0x00386
 *
 * Derive a rectangle and its centre from the structure that DGROUP 0x53fe
 * points at, into six words at DGROUP 0x5404..0x540e:
 *
 *     0x540e = left    = [+0x1e]        0x540a = top     = [+0x20]
 *     0x540c = right   = left + [+0x44] 0x5408 = bottom  = top + [+0x46]
 *     0x5406 = mid x   = left + [+0x44]/2
 *     0x5404 = mid y   = top  + [+0x46]/2
 *
 * so +0x44 and +0x46 are a width and a height. The halving is an **arithmetic**
 * shift, so a negative extent rounds toward negative infinity rather than
 * toward zero - which is not the same as dividing by two in C, and is why it
 * is written as a shift here.
 *
 * The pointer is re-read from DGROUP before every field, six times over. That
 * is what the original does and it is transcribed that way; it matters if
 * anything else can change 0x53fe in between.
 */
void compute_bounds_53fe(void)
{
    DG16(0x540E) = DG16(DGU16(0x53FE) + 0x1E);
    DG16(0x540A) = DG16(DGU16(0x53FE) + 0x20);
    DG16(0x540C) = (int16_t)(DG16(0x540E) + DG16(DGU16(0x53FE) + 0x44));
    DG16(0x5408) = (int16_t)(DG16(0x540A) + DG16(DGU16(0x53FE) + 0x46));
    DG16(0x5406) = (int16_t)(DG16(0x540E)
                             + (int16_t)(DG16(DGU16(0x53FE) + 0x44) >> 1));
    DG16(0x5404) = (int16_t)(DG16(0x540A)
                             + (int16_t)(DG16(DGU16(0x53FE) + 0x46) >> 1));
}

/*
 * 0x004d1
 *
 * Reduce a 16-bit angle to one of four directions. Two exact values are
 * answered directly - 0x2000 gives 0 and 0xa000 gives 2 - and everything else
 * is rotated by an eighth of a turn and shifted down to its top two bits.
 *
 * The shift is **arithmetic** (`sar`), not logical, and then masked to two
 * bits, so the sign makes no difference to the result; it is transcribed as
 * written rather than simplified to a logical shift.
 */
int16_t angle_to_quadrant(int16_t angle)
{
    if ((uint16_t)angle == 0x2000)
        return 0;
    if ((uint16_t)angle == 0xA000)
        return 2;
    return (int16_t)(((int16_t)(angle + 0x2000) >> 14) & 3);
}

/*
 * 0x02bcc
 *
 * Clamp the two signed words at +0x36 and +0x38 of a record to plus or minus
 * a limit that depends on the record's kind.
 *
 * The kind is the word at +4, and it indexes a table of 0x3a-byte entries
 * starting at DGROUP 0xea6; the limit is the word at +0x0a of that entry. The
 * negative bound is computed as `0 - limit` each time rather than kept, so a
 * limit of 0 pins the field to 0.
 *
 * The entry address is recomputed into BX before every single access in the
 * original - six times - and that is transcribed rather than hoisted.
 */
void clamp_record_pair(uint16_t rec)
{
    uint16_t entry = (uint16_t)(0xEA6 + (uint16_t)(DG16(rec + 4) * 0x3A));

    if (DG16(rec + 0x38) > DG16(entry + 0x0A))
        DG16(rec + 0x38) = DG16(entry + 0x0A);
    else if (DG16(rec + 0x38) < (int16_t)(0 - DG16(entry + 0x0A)))
        DG16(rec + 0x38) = (int16_t)(0 - DG16(entry + 0x0A));

    if (DG16(rec + 0x36) > DG16(entry + 0x0A))
        DG16(rec + 0x36) = DG16(entry + 0x0A);
    else if (DG16(rec + 0x36) < (int16_t)(0 - DG16(entry + 0x0A)))
        DG16(rec + 0x36) = (int16_t)(0 - DG16(entry + 0x0A));
}

/*
 * 0x03a61
 *
 * Is `node` on the chain hanging off `rec`? Only records whose type word at
 * +4 is 0x11 have such a chain; anything else answers no without looking.
 * The chain is linked through the word at +0x78, by **near** pointer.
 */
int16_t chain_contains(uint16_t rec, uint16_t node)
{
    uint16_t p;

    if (DG16(rec + 4) != 0x11)
        return 0;

    p = DGU16(rec + 0x78);
    while (p != 0) {
        if (p == node)
            return 1;
        p = DGU16(p + 0x78);
    }
    return 0;
}

/*
 * 0x03d2e
 *
 * Step the second word of each pair in a four-word record one further from the
 * first: if `[+4]` is above `[+0]` it goes up, if below it goes down, and if
 * equal it is left alone. The same for `[+6]` against `[+2]`.
 *
 * The two words are compared by subtraction and the *difference* tested, not
 * the values, so this is transcribed as a difference rather than as a compare.
 * What the record is has not been established - a pair of coordinates and a
 * pair of limits would fit, but that is inference.
 */
void step_pair_apart(uint16_t rec)
{
    int16_t d = (int16_t)(DG16(rec + 4) - DG16(rec));

    if (d > 0)
        DG16(rec + 4)++;
    else if (d < 0)
        DG16(rec + 4)--;

    d = (int16_t)(DG16(rec + 6) - DG16(rec + 2));
    if (d > 0)
        DG16(rec + 6)++;
    else if (d < 0)
        DG16(rec + 6)--;
}

/*
 * 0x04b53
 *
 * Are two points within 140 of each other in both axes?
 *
 * The absolute value is the branchless `cwd / xor ax,dx / sub ax,dx`: sign
 * extend into DX, exclusive-or, subtract. Both axes must pass; the first
 * failure answers 0 immediately.
 */
int16_t points_within_140(uint16_t a, uint16_t b)
{
    int16_t d = (int16_t)(DG16(a) - DG16(b));

    if (d < 0)
        d = (int16_t)-d;
    if (d > 0x8C)
        return 0;

    d = (int16_t)(DG16(a + 2) - DG16(b + 2));
    if (d < 0)
        d = (int16_t)-d;
    if (d > 0x8C)
        return 0;

    return 1;
}

/*
 * 0x07283
 *
 * Recompute a record's velocity from how far it has moved, then clamp it.
 *
 * The position is at +0x1e and +0x20 - the same pair `compute_bounds_53fe`
 * reads as the left and top edges - and +0x22 and +0x24 hold where it was, so
 * the difference is the step taken. That difference is then shifted **left**
 * by `9 - shift`, which turns a whole-pixel step into the fixed-point velocity
 * the rest of the code works in: a smaller `shift` argument means a bigger
 * result.
 *
 * The two axes are independent, chosen by bits 0 and 1 of the last argument,
 * so either can be left alone. It finishes by calling `clamp_record_pair`,
 * which clamps exactly the two fields written here - which is what identifies
 * +0x36 and +0x38 as a velocity pair rather than anything else.
 */
void update_velocity(uint16_t rec, uint8_t shift_x, uint8_t shift_y,
                     uint16_t which)
{
    if (which & 1) {
        DG16(rec + 0x36) = (int16_t)(DG16(rec + 0x1E) - DG16(rec + 0x22));
        DG16(rec + 0x36) = (int16_t)((uint16_t)DG16(rec + 0x36)
                                     << (uint8_t)(9 - shift_x));
    }
    if (which & 2) {
        DG16(rec + 0x38) = (int16_t)(DG16(rec + 0x20) - DG16(rec + 0x24));
        DG16(rec + 0x38) = (int16_t)((uint16_t)DG16(rec + 0x38)
                                     << (uint8_t)(9 - shift_y));
    }
    clamp_record_pair(rec);
}

/*
 * 0x07b3e
 *
 * Splice the whole of one list onto the front of another and empty the first.
 *
 * The list at DGROUP 0x4e58 is walked to its last node - the link is the first
 * word of each node - that node is pointed at the head of the list at DGROUP
 * 0x4e56, and 0x4e56 is then pointed at what 0x4e58 held. Returning a batch of
 * nodes to a free list in one move, by the shape of it, though the names are
 * not established.
 */
void splice_list_4e58_onto_4e56(void)
{
    uint16_t last, next;

    if (DGU16(0x4E58) == 0)
        return;

    last = DGU16(0x4E58);
    next = DGU16(last);
    while (next != 0) {
        last = next;
        next = DGU16(next);
    }

    DGU16(last) = DGU16(0x4E56);
    DGU16(0x4E56) = DGU16(0x4E58);
    DGU16(0x4E58) = 0;
}

/*
 * 0x03d67
 *
 * Is `v` between `a` and `b`, whichever way round they are?
 *
 * The bounds are ordered with a **signed** compare, and the containment test
 * is then the unsigned-difference trick: `v - lo <= hi - lo` is true exactly
 * when v lies in the range, and false by wrapping when it is below `lo`. The
 * two compares are of different signedness in the original and are transcribed
 * that way.
 */
int16_t value_between(uint16_t v, uint16_t a, uint16_t b)
{
    if ((int16_t)a > (int16_t)b)
        return (uint16_t)(v - b) <= (uint16_t)(a - b) ? 1 : 0;
    return (uint16_t)(v - a) <= (uint16_t)(b - a) ? 1 : 0;
}

/*
 * 0x004fd
 *
 * Decide which side of a range a value falls on, and set one of two flag bytes
 * accordingly - or both, when the value is inside the range.
 *
 * `range` is a record whose bounds are at +0 and +4; `out` is a record whose
 * flags are the bytes at +2 and +3. The containment test is `value_between` at
 * 0x03d67, which handles either ordering, so the side test below has to handle
 * both orderings too - and it does, by asking which bound is the lower one
 * first. All four compares here are **signed**.
 */
void set_side_flags(uint16_t range, int16_t v, uint16_t out)
{
    if (value_between((uint16_t)v, DGU16(range), DGU16(range + 4))) {
        DG8(out + 2) = 1;
        DG8(out + 3) = 1;
        return;
    }

    if (DG16(range) >= DG16(range + 4)) {
        if (DG16(range + 4) <= v)
            DG8(out + 3) = 1;
        else
            DG8(out + 2) = 1;
    } else {
        if (DG16(range) <= v)
            DG8(out + 2) = 1;
        else
            DG8(out + 3) = 1;
    }
}

/*
 * 0x05b65
 *
 * Pick the first of three words that is both non-zero and enabled by its bit
 * in the argument: 0x2000 selects DGROUP 0x521b, 0x1000 selects 0x5179, and
 * 0x0800 selects 0x50d7. If none qualifies the answer is 0.
 *
 * The order is the priority, and each test is "the slot is filled **and** the
 * caller asked for it" - a slot that is empty is skipped even when its bit is
 * set.
 */
int16_t pick_by_flag(uint16_t flags)
{
    if (DG16(0x521B) != 0 && (flags & 0x2000))
        return DG16(0x521B);
    if (DG16(0x5179) != 0 && (flags & 0x1000))
        return DG16(0x5179);
    if (DG16(0x50D7) != 0 && (flags & 0x0800))
        return DG16(0x50D7);
    return 0;
}

/*
 * 0x05ba7
 *
 * Choose a value for a record: its own word at +0 if that is set, otherwise
 * one of the shared slots, chosen by the record's flag word at +6 together
 * with the caller's flags.
 *
 * The 0x2000 case defers to `pick_by_flag` at 0x05b65 with the caller's flags,
 * so the record decides *whether* to look and the caller decides *which* slot.
 * The 0x1000 case does not defer: it requires the caller's 0x800 as well, and
 * reads DGROUP 0x50d7 directly - the same slot `pick_by_flag`'s third case
 * reads, reached by a different route.
 */
int16_t pick_for_record(uint16_t rec, uint16_t flags)
{
    if (DG16(rec) != 0)
        return DG16(rec);

    if (DG16(rec + 6) & 0x2000)
        return pick_by_flag(flags);

    if ((DG16(rec + 6) & 0x1000) && (flags & 0x800))
        return DG16(0x50D7);

    return 0;
}

/*
 * 0x06f43
 *
 * Say which of two fields of a structure matches a value: 0 for the field at
 * +0x5a, 1 for the one at +0x5c, and -1 for neither. The structure is reached
 * by a **near** pointer - a DGROUP offset - so it is indexed off DGROUP here.
 */
int16_t match_field_5a_5c(int16_t value, uint16_t obj)
{
    if (DG16(obj + 0x5A) == value)
        return 0;
    if (DG16(obj + 0x5C) == value)
        return 1;
    return -1;
}

/*
 * 0x06f68
 *
 * Given a record reached by a **near** pointer, answer the word at +4 if the
 * word at +2 matches, and the word at +2 itself if it does not. A null record
 * answers 0.
 *
 * Both the "matched" and "did not match" paths funnel through one `jmp` to the
 * epilogue, which is why the disassembly has three jumps to reach two results.
 */
int16_t select_field_2_or_4(int16_t key, uint16_t rec)
{
    if (rec == 0)
        return 0;
    if (DG16(rec + 2) == key)
        return DG16(rec + 4);
    return DG16(rec + 2);
}

/*
 * 0x081cc
 *
 * Present the frame. Three paths, chosen by two DGROUP flags: an optional
 * call to the routine at 0x0e34a first, then either a hook at 0x0b078 or the
 * driver's page flip. Called from sixteen places.
 */
void present_frame(uint16_t wait_retrace)
{
    if (present_hook_a != 0)
        sub_0e34a(1);

    if (present_hook_b != 0)
        sub_0b078();
    else
        vm_show_page(wait_retrace);
}

/*
 * 0x08f77
 *
 * Program the CRTC to blank after `lines` scan lines. The count is ten bits
 * and the hardware spreads it over three registers: the low eight in Start
 * Vertical Blank, bit 8 in Overflow bit 3, bit 9 in Maximum Scan Line bit 5.
 * Overflow and Maximum Scan Line are read back first so the other timing bits
 * the BIOS put there survive.
 *
 * Called with 0x1d6 (470) for the Sierra logo and 0x18f (399) for the game's
 * own 640x400 screens. Vertical Display End is never touched, so the CRTC goes
 * on scanning 480 lines and simply blanks the tail - which is what lets two
 * 640x400 pages fit in one 64 KB plane.
 */
void vm_set_display_lines(uint16_t lines)
{
    uint8_t v;

    io_out8(PORT_CRTC_INDEX, 0x15);
    io_out8(PORT_CRTC_DATA, (uint8_t)(lines & 0xFF));

    io_out8(PORT_CRTC_INDEX, 0x07);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xF7) | (((lines >> 8) & 1) << 3));
    io_out8(PORT_CRTC_DATA, v);

    io_out8(PORT_CRTC_INDEX, 0x09);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xDF) | (((lines >> 8) & 2) << 4));
    io_out8(PORT_CRTC_DATA, v);
}

/*
 * 0x0aaca
 *
 * Wait for the frame, then latch the input state for the frame about to be
 * drawn and clear the accumulators.
 *
 * The wait is the spin on `frame_pending` that the INT 08h handler releases -
 * and it is guarded, so when DGROUP 0x44ee is clear the routine does not wait
 * at all. That spin measured at 64% of all basic block executions under an
 * emulator paced on the host clock; see STATUS.md.
 *
 * The pair at 0x5782/0x5784 is filled either from `read_pair_4740` or from the
 * two words at 0x576c/0x576e, and the pair at 0x5768/0x576a is moved into
 * 0x5772/0x5774 and zeroed - accumulated since the last frame, then handed
 * over and reset, which is what a frame boundary looks like.
 */
void wait_and_latch_frame(void)
{
    if (DG8(0x44EE) != 0) {
        while (frame_pending())
            ;
    }

    if (DG16(0x2D42) != 0) {
        read_pair_4740(0x5784, 0x5782);
    } else {
        DG16(0x5784) = DG16(0x576E);
        DG16(0x5782) = DG16(0x576C);
    }

    DG16(0x5774) = DG16(0x576A);
    DG16(0x5772) = DG16(0x5768);
    DG16(0x5768) = 0;
    DG16(0x576A) = 0;
    frame_flag = 0;
}

/*
 * 0x0b429
 *
 * Find the entry in the two-slot table at DGROUP 0x56e6 whose top bits match,
 * and claim it. The slots are 0x20 bytes apart, so the second is at 0x5706 -
 * and those are exactly the two words the initialisation below fills with the
 * driver's back and front pages, once, the first time through.
 *
 * The match is on bits **0xa800** only, not on the whole word, so a slot
 * matches a page that differs from it in the low bits. Answers the slot's
 * DGROUP offset, or 0 if neither matched.
 *
 * A `want` of 0 means "the page currently being drawn into".
 */
uint16_t claim_page_slot(uint16_t want)
{
    uint16_t si;
    int16_t i;

    if (DG16(0x2D46) != 0) {
        DGU16(0x56E6) = vga_page_back;
        DGU16(0x5706) = vga_page_front;
        DG16(0x2D46) = 0;
    }

    if (want == 0)
        want = vga_page_back;

    si = 0x56E6;
    for (i = 0; i < 2; i++, si = (uint16_t)(si + 0x20)) {
        if ((want & 0xA800) == (DGU16(si) & 0xA800)) {
            DGU16(si) = want;
            return si;
        }
    }
    return 0;
}

/*
 * 0x0b47f
 *
 * Save the driver's drawing state, or put it back: a non-zero argument saves,
 * zero restores. The state is the clip box, whether clipping is on, and the
 * two page segments - seven values, kept at DGROUP 0x5726..0x5732.
 *
 * `clip_enabled` is a byte and is saved **zero-extended into a word**, then
 * restored as a byte, so the high half of 0x5726 is always zero. Transcribed
 * with the same widths rather than made symmetrical.
 */
void save_or_restore_draw_state(int16_t save)
{
    if (save != 0) {
        DGU16(0x5726) = clip_enabled;
        DG16(0x5728) = clip_left;
        DG16(0x572A) = clip_right;
        DG16(0x572C) = clip_top;
        DG16(0x572E) = clip_bottom;
        DGU16(0x5732) = vga_page_dst;
        DGU16(0x5730) = vga_page_src;
    } else {
        clip_enabled = DG8(0x5726);
        clip_left = DG16(0x5728);
        clip_right = DG16(0x572A);
        clip_top = DG16(0x572C);
        clip_bottom = DG16(0x572E);
        vga_page_dst = DGU16(0x5732);
        vga_page_src = DGU16(0x5730);
    }
}

/*
 * 0x0b078
 *
 * NOT TRANSCRIBED YET. Reached from the frame-presentation routine at 0x081cc
 * when DGROUP 0x52f2 is set. The address is known and the body is not read, so
 * it aborts rather than doing nothing: a silent no-op here would be a missing
 * frame that looks like a blitter fault.
 */
void sub_0b078(void)
{
    not_transcribed("0x0b078");
}

/*
 * 0x0b4e2
 *
 * Non-zero while `frame_flag` is still clear. The original is
 * `neg ax / sbb ax,ax / inc ax`, which is Borland's idiom for `ax = (ax == 0)`.
 *
 * The caller at 0x0aaca spins on this waiting for the INT 08h handler to set
 * the flag; that spin was 64% of all basic block executions under an emulator
 * paced on the host clock. See STATUS.md.
 */
int16_t frame_pending(void)
{
    return (int16_t)(frame_flag == 0);
}
