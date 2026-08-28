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
