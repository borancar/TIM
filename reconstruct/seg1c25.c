/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 1c25**, image
 * 0x1c250..0x248f0 - the largest of the game's own modules, 112 call targets.
 * Functions are in address order and each carries the image offset it was read
 * from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x1c6e3
 *
 * Does this NUL-terminated string contain the letter `r`?
 *
 * A **near** function - it ends in `ret`, not `retf` - so its argument sits at
 * [bp+4] and the string is a DGROUP offset. The loop tests for the terminator
 * before each character and steps the pointer before testing it, so an empty
 * string answers no without reading anything.
 */
int16_t string_contains_r(uint16_t str)
{
    while (DG8(str) != 0) {
        uint16_t at = str;
        str++;
        if (DG8(at) == 'r')
            return 1;
    }
    return 0;
}

/*
 * 0x220e9
 *
 * If the flag byte at DGROUP 0x48ea is set, store a quarter of each of the two
 * words at DGROUP 0x4740 and 0x4742 through the two near pointers passed in.
 * If it is clear, both are left alone - the routine writes nothing at all,
 * which a caller that did not initialise them would notice.
 *
 * The `neg`/`jae` pair again: carry is set exactly when the byte was non-zero.
 */
void read_pair_4740(uint16_t out_a, uint16_t out_b)
{
    if (DG8(0x48EA) == 0)
        return;
    DG16(out_a) = (int16_t)(DGU16(0x4740) >> 2);
    DG16(out_b) = (int16_t)(DGU16(0x4742) >> 2);
}

/*
 * 0x22161
 *
 * **Normalise a far pointer**: carry the paragraphs out of the offset into the
 * segment, leaving an offset of at most 15. AX holds the offset and DX the
 * segment; `AX >> 4` paragraphs move into DX and the offset is masked to four
 * bits.
 *
 * *This was first transcribed under the name `fixed_normalise`, as though AX
 * held a fixed-point fraction.* The behaviour is the same either way - it is
 * the same six instructions - but the name was wrong, and 0x222c6 is what
 * settled it: that routine calls this on the offset/segment halves of two far
 * pointers and then copies between them with `movsb`, which only makes sense
 * for pointer normalisation. A wrong name survives longer than a wrong line,
 * so it is corrected here and the correction recorded.
 *
 * A **near** routine taking and answering registers, so the port passes them
 * by reference. Both reads use the original AX, which is why the addition is
 * done before the mask.
 */
void normalise_far_ptr(uint16_t *off, uint16_t *seg)
{
    uint16_t ax = *off;

    *seg = (uint16_t)(*seg + (ax >> 4));
    *off = (uint16_t)(ax & 0x0F);
}

/*
 * 0x222c6
 *
 * Copy `count` bytes between two far pointers, normalising both first so that
 * each offset is under 16 and the segment carries the paragraphs.
 *
 * **The alignment step is dead code, in the original.** It reads
 *
 *     test di, 1
 *     jae  skip
 *     movsb
 *     dec  cx
 *
 * and `test` always clears the carry flag, so `jae` is always taken: the byte
 * that would have aligned the destination is never copied. It was presumably
 * meant to be `jz`. Transcribed as it behaves, not as it was meant, with the
 * dead branch recorded here rather than silently reinstated.
 *
 * The tail is `shr cx,1 / rep movsw / rcl cx,1 / rep movsb`: the shift puts the
 * odd bit into carry, the words are copied, and the rotate brings that bit back
 * into a count of 0 or 1 for the trailing byte. No compare anywhere.
 */
void far_memcpy(uint16_t dst_off, uint16_t dst_seg,
                uint16_t src_off, uint16_t src_seg, uint16_t count)
{
    uint16_t words;

    if (count == 0)
        return;

    normalise_far_ptr(&src_off, &src_seg);
    normalise_far_ptr(&dst_off, &dst_seg);

    words = (uint16_t)(count >> 1);
    while (words--) {
        FARU16(dst_seg, dst_off) = FARU16(src_seg, src_off);
        src_off = (uint16_t)(src_off + 2);
        dst_off = (uint16_t)(dst_off + 2);
    }
    if (count & 1)
        FAR8(dst_seg, dst_off) = FAR8(src_seg, src_off);
}

/*
 * NOT a transcription: the parity flag, worked out in C. The original gets it
 * from `or di,di` for free; see the routine below for why it is being used at
 * all.
 */
static int32_t low_byte_parity_even(uint16_t v)
{
    /* PF is set when the low eight bits hold an even number of set bits. */
    uint8_t b = (uint8_t)(v & 0xFF);
    int32_t n = 0;

    while (b) {
        n ^= 1;
        b = (uint8_t)(b & (b - 1));
    }
    return n == 0;
}

/*
 * 0x21abd
 *
 * Allocate memory from DOS, given a **32-bit byte count**, and answer a far
 * pointer to it in DX:AX - always at offset 0, since DOS hands out whole
 * paragraphs.
 *
 * The size is turned into paragraphs by shifting the pair right four times
 * with `shr`/`rcr`, and rounded **up** if any of the low four bits were set -
 * the remainder is tested from a copy taken before the shifting.
 *
 * A size of 0xffffffff is not a request but a question: it calls DOS with
 * 0xffff paragraphs, which always fails, and converts the largest-free figure
 * DOS reports back into bytes. So one routine both allocates and asks how much
 * there is, told apart by its argument.
 *
 * Bit 0 of the flags asks for the block to be zeroed, which it does through
 * `far_memset` at 0x22300.
 *
 * The DOS call itself is IO - see io.h - and is primed by the verifier with
 * what DOS actually answered, because the port has no arena of its own.
 */
uint32_t dos_alloc_bytes(uint16_t size_lo, uint16_t size_hi, uint16_t flags)
{
    uint16_t paras_lo, paras_hi, remainder, seg, largest;
    int32_t failed;

    if (size_hi == size_lo && size_hi == 0xFFFF) {
        /* The "how much is free" question. */
        io_dos_alloc(0xFFFF, &largest, &failed);
        {
            uint32_t bytes = (uint32_t)largest << 4;
            return bytes;
        }
    }

    remainder = (uint16_t)(size_lo & 0x0F);
    paras_hi = size_hi;
    paras_lo = size_lo;
    {
        int32_t i;
        for (i = 0; i < 4; i++) {
            paras_lo = (uint16_t)((paras_lo >> 1) | ((paras_hi & 1) << 15));
            paras_hi = (uint16_t)(paras_hi >> 1);
        }
    }
    if (remainder != 0)
        paras_lo = (uint16_t)(paras_lo + 1);

    seg = io_dos_alloc(paras_lo, &largest, &failed);
    if (failed)
        return 0;

    if (flags & 1)
        far_memset(0, seg, 0, size_lo, size_hi);

    return (uint32_t)seg << 16;
}

/*
 * 0x22300
 *
 * Fill memory through a far pointer, with a **32-bit** count, in chunks of at
 * most 0x7d00 bytes so that a chunk can never carry the offset past 64K. The
 * pointer is renormalised at the top of every chunk, and the count is reduced
 * by `sub`/`sbb` across both halves.
 *
 * **The alignment test is wrong, in the original.** It reads
 *
 *     or  di, di
 *     jp  skip
 *     stosb
 *
 * and `jp` is jump-if-**parity**, not jump-if-odd: `or di,di` sets PF from the
 * parity of DI's low byte, which has nothing to do with whether the address is
 * even. So the byte that would align the destination is stored or not
 * according to how many bits are set in the low half of the address. It is
 * transcribed as it behaves - the parity of the low byte - rather than as the
 * alignment test it was meant to be. `far_memcpy` at 0x222c6 has a bug in the
 * same place, differently: there the branch is always taken.
 *
 * A chunk shorter than ten bytes skips the word-fill entirely and is done with
 * `rep stosb`.
 */
void far_memset(uint16_t off, uint16_t seg, uint16_t value,
                uint16_t count_lo, uint16_t count_hi)
{
    uint16_t pair = (uint16_t)((value & 0xFF) | ((value & 0xFF) << 8));

    for (;;) {
        uint16_t chunk = 0x7D00;
        uint16_t taken, cx;

        if (count_hi == 0) {
            if (count_lo == 0)
                return;
            if ((int16_t)count_lo <= (int16_t)0x7D00)
                chunk = count_lo;
        }
        taken = chunk;

        normalise_far_ptr(&off, &seg);
        cx = chunk;

        if ((int16_t)cx >= 0x0A) {
            if (!low_byte_parity_even(off)) {
                FAR8(seg, off) = (uint8_t)value;
                off = (uint16_t)(off + 1);
                cx--;
            }
            {
                uint16_t words = (uint16_t)(cx >> 1);
                uint16_t odd = (uint16_t)(cx & 1);

                while (words--) {
                    FARU16(seg, off) = pair;
                    off = (uint16_t)(off + 2);
                }
                cx = odd;
            }
        }
        while (cx--) {
            FAR8(seg, off) = (uint8_t)value;
            off = (uint16_t)(off + 1);
        }

        {
            uint32_t total = ((uint32_t)count_hi << 16) | count_lo;
            total -= taken;
            count_lo = (uint16_t)total;
            count_hi = (uint16_t)(total >> 16);
        }
    }
}

/*
 * 0x22386
 *
 * The far-callable face of `normalise_far_ptr` at 0x22161: load the pointer
 * into AX and DX, call the near routine, and let its registers be the result.
 * So this answers a normalised far pointer in DX:AX, like any other far
 * routine returning a long.
 */
uint32_t normalise_far_ptr_far(uint16_t off, uint16_t seg)
{
    normalise_far_ptr(&off, &seg);
    return ((uint32_t)seg << 16) | off;
}

/*
 * 0x2213e
 *
 * Answer bit 0 of one of two flag bytes, or 0 if the first of them is clear.
 *
 * The original tests with `neg` and `jae`: `neg` leaves the carry flag set
 * exactly when its operand was non-zero, which is how this reads a byte and
 * branches on it without a compare. When DGROUP 0x48ea is zero the negation
 * leaves zero and the final AND answers 0; otherwise the byte at 0x48eb is
 * taken, shifted right once if the argument is non-zero, and its bit 0
 * returned.
 */
int16_t flag_bit_48ea(uint16_t which)
{
    uint16_t v = DG8(0x48EA);

    if (v == 0)
        return 0;

    v = DG8(0x48EB);
    if (which != 0)
        v >>= 1;
    return (int16_t)(v & 1);
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

    if (fill_enabled != 0) {
        int16_t cx = x, cy = y, cw = w, ch = h;

        if (clip_enabled != 0) {
            int16_t d = (int16_t)(cx - clip_left);
            if (d < 0) {
                cx = (int16_t)(cx - d);
                cw = (int16_t)(cw + d);
            }
            d = (int16_t)(cy - clip_top);
            if (d < 0) {
                cy = (int16_t)(cy - d);
                ch = (int16_t)(ch + d);
            }
            d = (int16_t)(clip_right - right);
            if (d < 0)
                cw = (int16_t)(cw + d);
            d = (int16_t)(clip_bottom - bottom);
            if (d < 0)
                ch = (int16_t)(ch + d);
        }

        if (cw > 0 && ch > 0) {
            uint8_t *p = FAR_PTR(span_buffer_seg, 0);
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

            vm_fill_spans(span_buffer_seg, 0);
        }
    }

    if (fill_enabled != 0 && vga_fill_colour == vga_second_colour)
        return;
    not_transcribed("0x2013f, the rectangle outline");
}

/*
 * 0x2147d
 *
 * Return bit 0 of the byte at DGROUP 0x468c + index.
 *
 * It runs with interrupts disabled, so the array is something an interrupt
 * handler also writes - a keyboard or timer flag, most likely, though that is
 * inference and not established.
 *
 * It does **not** push BP: it saves it in DX and points BP at the stack, so
 * its argument is at [bp+4] rather than the usual [bp+6]. Transcribed as an
 * ordinary parameter, since the port has no BP to preserve.
 */
int16_t bit0_of_468c(uint16_t index)
{
    return (int16_t)(byte_array_468c(index) & 1);
}

/*
 * 0x21e34
 *
 * Clip a line to the clip box and hand what is left to the driver's line
 * drawer through the vector at DGROUP 0x434e.
 *
 * Four stages - top, left, bottom, right - each the same shape: if both ends
 * are outside the edge the line is dropped entirely; if both are inside the
 * stage is skipped; otherwise the two ends are **swapped** so the outside one
 * is first, and it is moved onto the edge by interpolation.
 *
 * The interpolation is a 32-bit intermediate: `imul` makes a 32-bit product in
 * DX:AX and `idiv` divides it, so a long multiply is essential here and doing
 * it in 16 bits would overflow on a long line.
 *
 * **The first two stages compare signed and the last two unsigned** - `jl`/
 * `jge` against top and left, `ja`/`jbe` against bottom and right. That is not
 * a slip to tidy: once a line has been clipped to the top and left edges its
 * coordinates cannot be negative, so unsigned compares are safe and shorter.
 * Transcribed with the same signedness.
 *
 * BP is used as a scratch register for the divisor, which destroys the frame
 * pointer - safe only because every argument has already been loaded into a
 * register by then.
 *
 * Finally the ends are ordered by x, swapping both coordinates together, so
 * the drawer always receives them left to right.
 */
void clip_and_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    int16_t edge, t;

    if (clip_enabled != 0) {
        /* top */
        edge = clip_top;
        if (y1 < edge) {
            if (y2 < edge)
                return;
        } else if (y2 >= edge) {
            goto left;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        x1 = (int16_t)(x1 + (int16_t)(((int32_t)(x2 - x1) * (edge - y1))
                                      / (y2 - y1)));
        y1 = edge;

left:
        edge = clip_left;
        if (x1 < edge) {
            if (x2 < edge)
                return;
        } else if (x2 >= edge) {
            goto bottom;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        y1 = (int16_t)(y1 + (int16_t)(((int32_t)(y2 - y1) * (edge - x1))
                                      / (x2 - x1)));
        x1 = edge;

bottom:
        edge = clip_bottom;
        if ((uint16_t)y1 > (uint16_t)edge) {
            if ((uint16_t)y2 > (uint16_t)edge)
                return;
        } else if ((uint16_t)y2 <= (uint16_t)edge) {
            goto right;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        x1 = (int16_t)(x1 + (int16_t)(((int32_t)(x2 - x1) * (edge - y1))
                                      / (y2 - y1)));
        y1 = edge;

right:
        edge = clip_right;
        if ((uint16_t)x1 > (uint16_t)edge) {
            if ((uint16_t)x2 > (uint16_t)edge)
                return;
        } else if ((uint16_t)x2 <= (uint16_t)edge) {
            goto draw;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        y1 = (int16_t)(y1 + (int16_t)(((int32_t)(y2 - y1) * (edge - x1))
                                      / (x2 - x1)));
        x1 = edge;
    }

draw:
    if ((uint16_t)x1 > (uint16_t)x2) {
        t = x1; x1 = x2; x2 = t;
        t = y1; y1 = y2; y2 = t;
    }
    vm_draw_line(x1, y1, x2, y2);
}
