/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The host callback, which was written in assembly.** This file
 * corresponds to the fifth module of the original's code segment 2619, image
 * 0x29286..0x292f4: installing the callback cell at `cs:0x30f6`, and two ways
 * of calling through it. It sits between two C modules, and none of its
 * three routines is a compiler's: `set_sound_callback` saves AX round its
 * stores, and the callers save every register and the flags and keep their
 * answer in the code segment.
 *
 * **So it is TASM source**, `sound_call.asm`, with the host's transcription
 * here. It begins at 0x29286, with the cell itself: six bytes of the code
 * segment before the first routine. The functions are in address order and
 * each carries the image offset it was read from.
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2928c
 *
 * Install the host callback: a far pointer written into this module's own code
 * segment at `cs:0x30f6`, which is the cell `sound_callback` calls through.
 *
 * `AX` is pushed and popped around the two stores, so the caller's `AX`
 * survives - the routine has no return value of its own.
 */
void set_sound_callback(const uint8_t far * cb)
{
    g_sndcall.callback = cb;
}

/*
 * 0x292a1
 *
 * Call the host's sound callback, if one is installed, and answer what it
 * returned.
 *
 * The vector is the far pointer at the module's `cs:0x30f6` and it is only
 * called when the word at DGROUP 0x4aaa says a callback exists. With none
 * installed the routine still answers - AX is untouched from entry, so the
 * caller gets back whatever it passed in.
 *
 * The answer is parked at `cs:0x30fa` before the registers are popped and read
 * back afterwards, because the pops would otherwise destroy it. That is why a
 * routine that appears to return AX has a global in the middle of it.
 *
 * Everything is saved, flags included, because a callback is arbitrary code.
 * The port takes only the register input: the two stack arguments are read
 * solely on the path that calls the callback, and calling an arbitrary guest
 * function pointer is not something the port can do.
 */
uint16_t sound_callback(uint16_t ax, union sound_module_args * si)
{
    /*
     * `mov ax, 0x2d3c` loads DS two instructions before the test, and the
     * branch that skips the call lands *after* it - so with no module the
     * answer is that constant, which is a **relocation**: the program's DGROUP
     * segment, not the 0x2d3c the bytes read.
     */
    uint16_t answer = DGROUP_SEG;

    if (((int16_t)g_sound_bank.module_live) != 0)
        answer = call_sound_module(ax, si);

    g_sndcall.answer = (int16_t)answer;
    return (uint16_t)g_sndcall.answer;
}

/*
 * 0x292d9
 *
 * `sound_callback` without the answer: call through the callback cell if
 * DGROUP 0x4aaa says a callback exists, and keep nothing. It saves only SI
 * and DI, loads no DS of its own, and leaves AX as the callback left it.
 *
 * **Nothing calls it.** No call and no stored far pointer in the image names
 * 2619:3149; it is transcribed because its bytes are in the image. The name
 * is ours.
 */
void sound_callback_quiet(uint16_t ax, union sound_module_args * si)
{
    if (((int16_t)g_sound_bank.module_live) != 0)
        call_sound_module(ax, si);
}
