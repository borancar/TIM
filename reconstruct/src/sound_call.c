/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The host callback, which was written in assembly.** This file
 * corresponds to the fifth module of the original's code segment 2619, image
 * 0x29286..0x292f4: installing the callback cell at `cs:0x30f6`, and two ways
 * of calling through it. It sits between two C modules, and none of its
 * three routines is a compiler's: `set_sound_callback` saves AX round its
 * stores, and the callers save every register and the flags and keep their
 * answer in the code segment.
 *
 * **So it is TASM source**, the `#ifdef __TURBOC__` block below, with the
 * host's transcription in the `#else`. It begins at 0x29286, with the cell itself: six
 * bytes of the code segment before the first routine. The functions are in address order and each
 * carries the image offset it was read from.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler tasm1.01
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
 * transcription is the `#else`. See glue.c for how the block reaches the
 * assembler.
 */
asm {
extrn _DG4A82:byte
SOUND_CALL_TEXT segment byte public 'CODE'
assume cs:SOUND_CALL_TEXT, ds:DGROUP
public _set_sound_callback, _sound_callback, _sound_callback_quiet
c_29286 label byte
        db 0h, 0h
c_29288 label byte
        db 0h, 0h
c_2928a label byte
        db 0h, 0h

/* 0x2928c */
_set_sound_callback proc far
        push bp
        mov bp, sp
        push ax
        mov ax, word ptr [bp+6]
        mov word ptr cs:c_29286, ax
        mov ax, word ptr [bp+8]
        mov word ptr cs:c_29288, ax
        pop ax
        pop bp
        retf
_set_sound_callback endp

/* 0x292a1 */
_sound_callback proc far
        push bp
        mov bp, sp
        push ds
        push es
        pushf
        push ax
        push cx
        push dx
        push bx
        push bp
        push si
        push di
        mov ax, DGROUP
        mov ds, ax
        cmp word ptr DGROUP:_DG4A82+28h, 0
        je L292c5
        mov si, word ptr [bp+8]
        mov ax, word ptr [bp+6]
        call dword ptr cs:c_29286
L292c5:
        mov word ptr cs:c_2928a, ax
        pop di
        pop si
        pop bp
        pop bx
        pop dx
        pop cx
        pop ax
        popf
        mov ax, word ptr cs:c_2928a
        pop es
        pop ds
        pop bp
        retf
_sound_callback endp

/* 0x292d9 */
_sound_callback_quiet proc far
        push bp
        mov bp, sp
        push si
        push di
        cmp word ptr DGROUP:_DG4A82+28h, 0
        je L292f0
        mov si, word ptr [bp+8]
        mov ax, word ptr [bp+6]
        call dword ptr cs:c_29286
L292f0:
        pop di
        pop si
        pop bp
        retf
_sound_callback_quiet endp
SOUND_CALL_TEXT ends
}
#else

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
    SNDCALL.callback = far_of(cb);
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

    if (((int16_t)DG4A82.module_live) != 0)
        answer = call_sound_module(ax, si);

    SNDCALL.answer = (int16_t)answer;
    return (uint16_t)SNDCALL.answer;
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
    if (((int16_t)DG4A82.module_live) != 0)
        call_sound_module(ax, si);
}
#endif
