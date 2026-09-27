/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The interrupt stack switch**, code segment 0000 (`_TEXT`), image
 * 0x0b82c..0x0b859: hand-written assembly - it pops its own return address
 * and argument and pushes them back onto the stack it has just switched to,
 * which no C can say - so this is the host's transcription and is not
 * judged.
 *
 * It closes the assembly module of DOS helpers at 0x0b6b7 (`findfirst`,
 * `chdir`, `unlink` and the rest, whose `_DATA` is 0x2d4a..0x2d7e), whose
 * transcriptions are in borland_file.c beside the library they resemble.
 * Every other routine of segment 0000 is split out into its own file:
 * collide.c through cursor.c before this, mono.c, thunks.c and glue.c
 * after.
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

