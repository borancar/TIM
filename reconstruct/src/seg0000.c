/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **What is left of code segment 0000** (`_TEXT`), image 0x0a78e..0x0dff0,
 * while its modules are split out from the front: the screen and the
 * pointer's regions, sound selection, the heap checks, the resource files,
 * drawing pages and rectangles, and the rest. Everything before it is in
 * collide.c through rects.c. Several modules are still in here - calls
 * that reach back bare, and `_DATA` in link order, put boundaries near
 * 0x08136, 0x08fc3, 0x0a05f and 0x0aa76 - so the name is the segment's
 * until each is split out and named.
 *
 * Not yet judged: each module gets its markers as it is split out.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **The interrupt's own stack**, DGROUP 0x317e..0x3182, 0x04 bytes.
 *
 * `isr_stack_switch` files `SS:SP` here on the way in so the handler can run
 * on a private stack and put the interrupted one back on the way out. The port
 * does not switch stacks - it has no single SP to switch - but it writes both
 * words, because anything else is free to read them.
 */
struct machine_isr_stack {
    uint16_t  saved_ss;           /* +0x00 [2] */
    uint16_t  saved_sp;           /* +0x02 [2] */
} PACKED;

struct machine_isr_stack MACHINE_ISR_STACK DGROUP_AT(0x317e);

/*
 * 0x0b82c
 *
 * Switch the interrupt handler onto a stack of its own, and back: a non-zero
 * argument saves SS:SP at DGROUP 0x317e and puts SP at 0x2e7c inside DGROUP, a
 * zero one puts the saved pair back. The entry at 0x0b84b is the second half
 * reached directly.
 *
 * It does this by popping its own return address and argument off the stack,
 * changing SS:SP, and pushing them back - the only way to return onto a stack
 * you have just swapped.
 *
 * **The switch itself means nothing here.** The port's handler runs on a real
 * thread with a real stack of its own, which is what the private stack was for.
 * The two DGROUP words are still written, because anything else can read them.
 */
void isr_stack_switch(int16_t to_private)
{
    if (to_private != 0) {
        MACHINE_ISR_STACK.saved_ss = DGROUP_SEG;
        MACHINE_ISR_STACK.saved_sp = guest_sp;
        return;
    }

    /* The restore half at 0x0b84b: the saved pair goes back into SS:SP. */
}

/*
 * 0x0b859
 *
 * Set the mouse's mickeys-per-pixel, INT 33h AX=0x0f, the same value for both
 * axes: the argument goes into CX and DX alike. The start-up asks for 3.
 *
 * Nothing of it is in guest memory, so the port sends it to the IO boundary and
 * there is nothing here for the two artefacts to disagree about.
 */
void mouse_set_speed(uint16_t mickeys)
{
    io_mouse_set_speed(mickeys, mickeys);
}

/*
 * 0x0b93d
 *
 * `fread` into a **huge** pointer, one byte at a time, answering how many whole
 * items came in.
 *
 * A byte at a time because the destination may cross a segment: each one is
 * stored through the far pointer and then `huge_add_to` steps and renormalises
 * it - reached here by its near door at 0x0be7f.
 *
 * The count is `size * count` as a 32-bit product, and the answer is the bytes
 * actually read divided by the size, which is why a partial last item does not
 * count. The loop stops on the count running out or on `game_fgetc` answering
 * -1, and the test is made **before** the decrement, so a count of zero reads
 * nothing.
 */
uint32_t fread_huge(uint8_t far * dst, uint32_t size, uint32_t count,
                    FILE *file)
{
    /* `dst` is the [bp-8] pair `huge_add_to` steps - the caller's copy, taken
       by value, which a huge pointer's `++` is. */
    uint32_t total = long_multiply(count, size);
    uint32_t got = 0;

    while (total != 0) {
        int16_t c;

        total--;

        c = game_fgetc(file);

        if (c == -1)
            break;

        *dst++ = (uint8_t)c;
        got++;
    }
    return ulong_divide(got, size);
}

/*
 * 0x0b9c9
 *
 * Draw one bitmap scaled - the same choice `draw_bitmap` makes, from the same
 * marker in field 4, and the same normalisation of the header's far pointer
 * written back into it.
 *
 * Two of the four forms have no scaled blitter: 0xfffd, the already-scaled
 * one, and 0xffff, the offset-table one, both simply do nothing here. That is
 * the original's own silence and not a gap - a bitmap in either form is never
 * asked to be drawn scaled.
 */
void draw_bitmap_scaled(struct bitmap *hdr, int16_t x, int16_t y,
                        int16_t w, int16_t h, uint16_t mode)
{
    hdr->data = far_normalise_rev(hdr->data);

    switch (hdr->mask_off) {
    case 0xfffd:
    case 0xffff:
        return;
    case 0xfffe:
        blit_scaled_a(hdr, x, y, mode, w, h);
        return;
    default:
        blit_scaled_b(hdr, x, y, mode, w, h);
        return;
    }
}
