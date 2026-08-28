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
 * 0x0d0a3
 *
 * Find the first free slot in the table of 16-byte records at DGROUP 0x4bc4.
 * A slot is free when the **signed** byte at +4 is negative; the number of
 * records in use is at DGROUP 0x4d04, and the end is computed inside the loop
 * rather than once, exactly as here.
 *
 * A near function - it ends in `ret`. It answers the slot's DGROUP offset, or
 * 0 if the walk ran off the end.
 */
uint16_t find_free_slot_4bc4(void)
{
    uint16_t si = 0x4BC4;

    while (DGS8(si + 4) >= 0) {
        uint16_t end = (uint16_t)(0x4BC4 + (uint16_t)(DGU16(0x4D04) << 4));
        uint16_t prev = si;

        si = (uint16_t)(si + 16);
        if (end <= prev)
            break;
    }
    return (DGS8(si + 4) < 0) ? si : 0;
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
