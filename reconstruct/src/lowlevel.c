/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The machine, in assembly**: DOS memory, the mouse, huge-pointer
 * arithmetic, the divide-error trap, clipped single pixels, and three thunks
 * into the video driver.
 *
 * This file corresponds to the original's **code segment 1c25**, image
 * 0x21b44..0x22483, split out of engine.c on 2026-09-27; dosmem.c holds
 * 0x21ab5..0x21b44, the module `far_memset` is not in. All of it is
 * hand-written: register arguments and answers, `iret`, near helpers that
 * answer in the carry, frames no compiler builds. **So it is TASM source**,
 * the `#ifdef __TURBOC__` block below, with the host's transcription in the
 * `#else`. The functions are in address order and each carries the image
 * offset it was read from.
 *
 * **It is probably several modules, and only one boundary is proven.**
 * Measured 2026-09-27 over every call into, out of and inside the range, and
 * every DGROUP word each routine names:
 *
 *   - `dos_alloc_bytes` (0x21abd) calls `far_memset` (0x22300) through
 *     TLINK's `nop / push cs / call`, so the two are in different modules.
 *     That is the only call between the groups below.
 *   - Inside a group the calls are bare `push cs / call` (TASM's own, so the
 *     same module) or near: the joystick (0x21b44..0x21e34) calls 0x21d19,
 *     0x21b44 and 0x21bab; `mouse_init` calls `mouse_set_ranges`;
 *     `mouse_event` calls the two VGA save routines; and the huge-pointer
 *     helpers (0x22161..0x22394) near-call 0x22161 and 0x22190.
 *   - The data follows the code: the joystick's block at 0x4724..0x4740,
 *     starting on the word after the text module's; the mouse's
 *     0x4740..0x48ec, one run (0x4748..0x48d8 is the stack `mouse_event`
 *     switches to, `mov sp,0x48d8`); the divide trap's 0x48ec..0x48f1; then
 *     `vm_init`'s module at 0x48f2. No word is padded between the joystick,
 *     the mouse and the divide trap, and the thunks, the DOS memory routines,
 *     the line clipper, the huge-pointer helpers and the clipped pixels have
 *     no data at all.
 *   - No routine is padded to an alignment: each starts on the byte after
 *     the previous one's return.
 *
 * So the groups are plain - thunks and DOS memory, joystick, line clipper,
 * mouse, huge pointers, divide trap, clipped pixels and a thunk - but
 * nothing yet says which neighbours shared a file. One file stands for the
 * range until something does.
 *
 * **0x21b44..0x21e34 is not transcribed**: a joystick driver - port 0x201,
 * timed with `loop` - that nothing on the paths the port runs calls.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler bc3.00
 */
#include <string.h>

#ifndef __TURBOC__
#include "hostlib.h"
#endif
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
 * transcription is the `#else`. See glue.c for how the block reaches the
 * assembler.
 */
asm {
_DATA segment word public 'DATA'
d_4724 label byte
        db 0h
d_4725 label byte
        db 0h
d_4726 label byte
        db 0h
d_4727 label byte
        db 0h, 0h
d_4729 label byte
        db 0h, 0h
d_472b label byte
        db 0h, 0h
d_472d label byte
        db 0h, 0h
d_472f label byte
        db 0h, 0h
d_4731 label byte
        db 0h, 0h
d_4733 label byte
        db 0h, 0h
d_4735 label byte
        db 0h, 0h
d_4737 label byte
        db 0h, 0h
d_4739 label byte
        db 0h, 0h
d_473b label byte
        db 0h, 0h
d_473d label byte
        db 0h, 0h, 0h
d_4740 label byte
        db 0h, 0h
d_4742 label byte
        db 0h, 0h
d_4744 label byte
        db 0h, 0h
d_4746 label byte
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h
d_48da label byte
        db 0h, 0h
d_48dc label byte
        db 0h, 0h
d_48de label byte
        db 0h, 0h
d_48e0 label byte
        db 0h, 0h
d_48e2 label byte
        db 0h, 0h, 0h, 0h
d_48e6 label byte
        db 2h
d_48e7 label byte
        db 40h
d_48e8 label byte
        db 1h
d_48e9 label byte
        db 41h
d_48ea label byte
        db 0h
d_48eb label byte
        db 0h
d_48ec label byte
        db 0h
d_48ed label byte
        db 0h, 0h
d_48ef label byte
        db 0h, 0h, 0h
_DATA ends

extrn _g_vm_driver:byte
extrn _g_timer:byte
extrn _g_vmds:byte
LOWLEVEL_TEXT segment byte public 'CODE'
assume cs:LOWLEVEL_TEXT, ds:DGROUP
public _joy_time_axes, _joy_scale_axis, _joy_init, _joy_read
public _joy_direction, _joy_button, _joy_axis, _clip_and_draw_line
public _mouse_init, _mouse_set_ranges, _mouse_set_user_handler, _mouse_event
public _mouse_save_vga, _mouse_restore_vga, _remove_mouse, _read_mouse_pointer
public _mouse_move_to, _read_mouse_button, _normalise_pointer, _huge_add_positive
public _huge_move, _far_memcpy, _far_memset, _far_ptr_compare
public _normalise_pointer_far, _install_divide_trap, _divide_error_handler, _restore_int0_vector
public _read_pixel_clipped, _plot_pixel_clipped, _restore_rect_thunk

/* 0x237ce */
_joy_time_axes proc near
        mov dx, 201h
        pushf
        push bp
        cli
        mov bp, word ptr DGROUP:d_4739
        mov cx, 190h
L21b51:
        mov ax, cx
        mov cx, bp
L21b55:
        loop L21b55
        mov cx, ax
        in al, dx
        test al, bl
        loopne L21b51
        jcxz L21b60
L21b60:
        mov cx, 190h
        xor al, al
        out dx, al
        xor si, si
        mov di, si
        test bl, 3
        je L21b8b
L21b6f:
        mov ax, cx
        mov cx, bp
L21b73:
        loop L21b73
        mov cx, ax
        in al, dx
        and al, bl
        je L21b88
        shr al, 1
        adc si, 0
        shr al, 1
        adc di, 0
        loop L21b6f
L21b88:
        pop bp
        popf
        ret
L21b8b:
        mov ax, cx
        mov cx, bp
L21b8f:
        loop L21b8f
        mov cx, ax
        in al, dx
        and al, bl
        je L21b88
        shr al, 1
        shr al, 1
        shr al, 1
        adc si, 0
        shr al, 1
        adc di, 0
        loop L21b8b
        pop bp
        popf
        ret
_joy_time_axes endp

/* 0x23835 */
_joy_scale_axis proc near
        push di
        sub ax, bx
        or ax, ax
        jge L21bb9
        mov di, 1
        neg ax
        jmp short L21bbc
L21bb9:
        mov di, 0
L21bbc:
        or cx, cx
        je L21bc8
        xor dx, dx
        mul cx
        mov al, ah
        mov ah, dl
L21bc8:
        cmp ax, 7fh
        jbe L21bd0
        mov ax, 7fh
L21bd0:
        cmp al, 8
        jae L21bd6
        xor al, al
L21bd6:
        xor ah, ah
        test di, 1
        je L21be0
        neg ax
L21be0:
        pop di
        ret
_joy_scale_axis endp

/* 0x2386c */
_joy_init proc far
        push si
        push di
        cli
        mov al, 36h
        out 43h, al
        xor al, al
        out 40h, al
        out 40h, al
        mov dx, 201h
        mov cx, 3e8h
        out 43h, al
        in al, 40h
        mov ah, al
        in al, 40h
        xchg al, ah
        mov si, ax
L21c01:
        mov ax, ax
        mov ax, ax
        nop
        nop
        nop
        nop
        mov ax, ax
        in al, dx
        test al, al
        loop L21c01
        mov al, 6
        out 43h, al
        in al, 40h
        mov ah, al
        in al, 40h
        xchg al, ah
        mov di, ax
        mov bx, word ptr DGROUP:_g_timer+3h
        mov al, 36h
        out 43h, al
        mov al, bl
        out 40h, al
        mov al, bh
        out 40h, al
        sti
        sub si, di
        xor dx, dx
        mov ax, 6fcch
        div si
        cmp ax, 0
        jne L21c40
        mov ax, 1
L21c40:
        mov word ptr DGROUP:d_4739, ax
        mov bx, 3
        call _joy_time_axes
        mov ax, si
        mov word ptr DGROUP:d_4727, ax
        cmp ax, 190h
        rcr byte ptr DGROUP:d_4726, 1
        mov ax, di
        mov word ptr DGROUP:d_4729, ax
        cmp ax, 190h
        rcr byte ptr DGROUP:d_4726, 1
        mov al, byte ptr DGROUP:d_4726
        and al, 0c0h
        cmp al, 0c0h
        jne L21c71
        mov byte ptr DGROUP:d_4724, 1
        jmp short L21c76
L21c71:
        mov byte ptr DGROUP:d_4724, 0
L21c76:
        or si, si
        je L21c86
        mov cx, si
        mov ax, 7f00h
        xor dx, dx
        div cx
        mov word ptr DGROUP:d_472f, ax
L21c86:
        or di, di
        je L21c96
        mov cx, di
        mov ax, 7f00h
        xor dx, dx
        div cx
        mov word ptr DGROUP:d_4731, ax
L21c96:
        mov bx, 0ch
        call _joy_time_axes
        mov ax, si
        cmp ax, 190h
        mov word ptr DGROUP:d_472b, ax
        rcr byte ptr DGROUP:d_4726, 1
        mov ax, di
        mov word ptr DGROUP:d_472d, ax
        cmp ax, 190h
        rcr byte ptr DGROUP:d_4726, 1
        mov al, byte ptr DGROUP:d_4726
        and al, 0c0h
        cmp al, 0c0h
        jne L21cc4
        mov byte ptr DGROUP:d_4725, 1
        jmp short L21cc9
L21cc4:
        mov byte ptr DGROUP:d_4725, 0
L21cc9:
        or si, si
        je L21cd9
        mov cx, si
        mov ax, 7f00h
        xor dx, dx
        div cx
        mov word ptr DGROUP:d_4733, ax
L21cd9:
        or di, di
        je L21ce9
        mov cx, di
        mov ax, 7f00h
        xor dx, dx
        div cx
        mov word ptr DGROUP:d_4735, ax
L21ce9:
        mov dx, 201h
        out dx, al
        mov cx, 14h
L21cf0:
        loop L21cf0
        in al, dx
        test al, 3
        jne L21cfc
        mov byte ptr DGROUP:d_4724, 0
L21cfc:
        test al, 0ch
        jne L21d05
        mov byte ptr DGROUP:d_4725, 0
L21d05:
        mov cl, 4
        shr byte ptr DGROUP:d_4726, cl
        xor ax, ax
        mov al, byte ptr DGROUP:d_4725
        shl al, 1
        or al, byte ptr DGROUP:d_4724
        pop di
        pop si
        retf
_joy_init endp

/* 0x239a3 */
_joy_read proc far
        push bp
        mov bp, sp
        push si
        push di
        mov bx, word ptr [bp+6]
        cmp bx, 0
        jne L21d54
        xor ah, ah
        mov al, byte ptr DGROUP:d_4724
        or al, al
        je L21d8d
        mov bx, 3
        call _joy_time_axes
        mov bx, word ptr DGROUP:d_4727
        mov cx, word ptr DGROUP:d_472f
        mov ax, si
        call _joy_scale_axis
        mov word ptr DGROUP:d_4737, ax
        mov bx, word ptr DGROUP:d_4729
        mov cx, word ptr DGROUP:d_4731
        mov ax, di
        call _joy_scale_axis
        jmp short L21d80
L21d54:
        xor ah, ah
        mov al, byte ptr DGROUP:d_4725
        or al, al
        je L21d8d
        mov bx, 0ch
        call _joy_time_axes
        mov bx, word ptr DGROUP:d_472b
        mov cx, word ptr DGROUP:d_4733
        mov ax, si
        call _joy_scale_axis
        mov word ptr DGROUP:d_4737, ax
        mov bx, word ptr DGROUP:d_472d
        mov cx, word ptr DGROUP:d_4735
        mov ax, di
        call _joy_scale_axis
L21d80:
        mov si, word ptr [bp+0ah]
        mov di, word ptr [bp+8]
        mov word ptr [si], ax
        mov ax, word ptr DGROUP:d_4737
        mov word ptr [di], ax
L21d8d:
        pop di
        pop si
        pop bp
        retf
_joy_read endp

/* 0x23a1b */
_joy_direction proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+6]
        or ax, ax
        je L21da0
        mov al, byte ptr DGROUP:d_4725
        jmp short L21da3
L21da0:
        mov al, byte ptr DGROUP:d_4724
L21da3:
        or al, al
        je L21de8
        mov ax, 473dh
        push ax
        mov ax, 473bh
        push ax
        mov ax, word ptr [bp+6]
        push ax
        push cs
        call near ptr _joy_read
        add sp, 6
        xor ax, ax
        mov bx, word ptr DGROUP:d_473b
        cmp bx, -1eh
        jge L21dca
        or ax, 4
        jmp short L21dd2
L21dca:
        cmp bx, 1eh
        jl L21dd2
        or ax, 8
L21dd2:
        mov bx, word ptr DGROUP:d_473d
        cmp bx, -1eh
        jge L21de0
        or ax, 1
        jmp short L21de8
L21de0:
        cmp bx, 1eh
        jl L21de8
        or ax, 2
L21de8:
        pop bp
        retf
_joy_direction endp

/* 0x23a74 */
_joy_button proc far
        push bp
        mov bp, sp
        mov cx, word ptr [bp+6]
        and cx, 3
        mov dx, 201h
        in al, dx
        add cl, 4
        shr al, cl
        and al, 1
        xor ah, ah
        xor al, 1
        pop bp
        retf
_joy_button endp

/* 0x23a8e */
_joy_axis proc far
        push bp
        mov bp, sp
        push si
        push di
        mov bx, 1
        mov cx, word ptr [bp+6]
        shl bx, cl
        call _joy_time_axes
        mov cx, word ptr [bp+6]
        test cl, 1
        je L21e1e
        mov si, di
L21e1e:
        mov di, cx
        shl di, 1
        mov bx, word ptr d_4727[di]
        mov cx, word ptr d_472f[di]
        mov ax, si
        call _joy_scale_axis
        sti
        pop di
        pop si
        pop bp
        retf
_joy_axis endp

/* 0x23abe */
_clip_and_draw_line proc far
        push bp
        mov bp, sp
        push si
        push di
        push es
        mov ax, word ptr DGROUP:_g_vmds+18h
        mov es, ax
        mov bx, word ptr [bp+6]
        mov cx, word ptr [bp+8]
        mov si, word ptr [bp+0ah]
        mov di, word ptr [bp+0ch]
        mov al, byte ptr DGROUP:_g_vmds+3h
        or al, al
        jne L21e55
        jmp L21f08
L21e55:
        mov ax, word ptr DGROUP:_g_vmds+8h
        cmp cx, ax
        jl L21e66
        cmp di, ax
        jge L21e81
        xchg bx, si
        xchg cx, di
        jmp short L21e6a
L21e66:
        cmp di, ax
        jl L21e96
L21e6a:
        mov bp, di
        sub bp, cx
        sub ax, cx
        mov cx, ax
        mov ax, si
        sub ax, bx
        imul cx
        idiv bp
        add bx, ax
        mov ax, word ptr DGROUP:_g_vmds+8h
        mov cx, ax
L21e81:
        mov ax, word ptr DGROUP:_g_vmds+4h
        cmp bx, ax
        jl L21e92
        cmp si, ax
        jge L21eb0
        xchg bx, si
        xchg cx, di
        jmp short L21e99
L21e92:
        cmp si, ax
        jge L21e99
L21e96:
        jmp L21f18
L21e99:
        mov bp, si
        sub bp, bx
        sub ax, bx
        mov bx, ax
        mov ax, di
        sub ax, cx
        imul bx
        idiv bp
        add cx, ax
        mov ax, word ptr DGROUP:_g_vmds+4h
        mov bx, ax
L21eb0:
        mov ax, word ptr DGROUP:_g_vmds+0ah
        cmp cx, ax
        ja L21ec1
        cmp di, ax
        jbe L21edc
        xchg bx, si
        xchg cx, di
        jmp short L21ec5
L21ec1:
        cmp di, ax
        ja L21e96
L21ec5:
        mov bp, di
        sub bp, cx
        sub ax, cx
        mov cx, ax
        mov ax, si
        sub ax, bx
        imul cx
        idiv bp
        add bx, ax
        mov ax, word ptr DGROUP:_g_vmds+0ah
        mov cx, ax
L21edc:
        mov ax, word ptr DGROUP:_g_vmds+6h
        cmp bx, ax
        ja L21eed
        cmp si, ax
        jbe L21f08
        xchg bx, si
        xchg cx, di
        jmp short L21ef1
L21eed:
        cmp si, ax
        ja L21e96
L21ef1:
        mov bp, si
        sub bp, bx
        sub ax, bx
        mov bx, ax
        mov ax, di
        sub ax, cx
        imul bx
        idiv bp
        add cx, ax
        mov ax, word ptr DGROUP:_g_vmds+6h
        mov bx, ax
L21f08:
        mov dx, si
        mov si, di
        cmp bx, dx
        jbe L21f14
        xchg bx, dx
        xchg cx, si
L21f14:
        call dword ptr DGROUP:_g_vm_driver+0ch
L21f18:
        pop es
        pop di
        pop si
        pop bp
        retf
_clip_and_draw_line endp

/* 0x23ba7 */
_mouse_init proc far
        sub ax, ax
        cmp byte ptr DGROUP:d_48ea, al
        jne L21f8c
        int 33h
        neg ax
        mov byte ptr DGROUP:d_48ea, al
        jae L21f8c
        mov ax, 4
        mov cx, 7fffh
        mov dx, cx
        int 33h
        mov ax, 1
        int 33h
        mov ax, 2
        int 33h
        mov ax, 0fh
        mov cx, 8
        mov dx, 8
        int 33h
        mov ax, 4
        xor cx, cx
        mov dx, cx
        int 33h
        push word ptr DGROUP:_g_vmds+6ech
        push word ptr DGROUP:_g_vmds+6eah
        xor ax, ax
        push ax
        push ax
        push cs
        call near ptr _mouse_set_ranges
        add sp, 8
        mov ax, 0ch
        mov cx, 1fh
        push cs
        pop es
        mov dx, 5d7fh
        int 33h
        mov al, byte ptr DGROUP:_g_vmds+1dh
        cmp al, 8
        jne L21f89
        mov al, byte ptr DGROUP:d_48e7
        mov byte ptr DGROUP:d_48e6, al
        mov al, byte ptr DGROUP:d_48e9
        mov byte ptr DGROUP:d_48e8, al
L21f89:
        mov ax, 1
L21f8c:
        retf
_mouse_init endp

/* 0x23c17 */
_mouse_set_ranges proc far
        push bp
        mov bp, sp
        mov ax, 7
        mov cx, word ptr [bp+6]
        mov dx, cx
        add dx, word ptr [bp+0ah]
        shl cx, 1
        shl cx, 1
        dec dx
        shl dx, 1
        shl dx, 1
        int 33h
        mov ax, 8
        mov cx, word ptr [bp+8]
        mov dx, cx
        add dx, word ptr [bp+0ch]
        shl cx, 1
        shl cx, 1
        dec dx
        shl dx, 1
        shl dx, 1
        int 33h
        pop bp
        retf
_mouse_set_ranges endp

/* 0x23c48 */
_mouse_set_user_handler proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+6]
        mov word ptr DGROUP:d_4744, ax
        mov ax, word ptr [bp+8]
        mov word ptr DGROUP:d_4746, ax
        pop bp
        retf
_mouse_set_user_handler endp

/* 0x23c59 */
_mouse_event proc far
        push ds
        push es
        push bp
        push si
        mov bp, sp
        mov si, ss
        mov ax, DGROUP
        cli
        mov sp, 48d8h
        mov ss, ax
        sti
        mov ds, ax
        mov byte ptr DGROUP:d_48eb, bl
        mov word ptr DGROUP:d_4740, cx
        mov word ptr DGROUP:d_4742, dx
        mov ax, word ptr DGROUP:d_4744
        or ax, word ptr DGROUP:d_4746
        je L22004
        push cs
        call near ptr _mouse_save_vga
        call dword ptr DGROUP:d_4744
        push cs
        call near ptr _mouse_restore_vga
L22004:
        cli
        mov ss, si
        mov sp, bp
        sti
        pop si
        pop bp
        pop es
        pop ds
        retf
_mouse_event endp

/* 0x23c99 */
_mouse_save_vga proc far
        mov dx, 3ceh
        in al, dx
        mov ah, al
        xor al, al
        out dx, al
        inc dx
        in al, dx
        mov word ptr DGROUP:d_48da, ax
        dec dx
        mov al, 1
        out dx, al
        inc dx
        in al, dx
        mov ah, al
        xor al, al
        out dx, al
        dec dx
        mov al, 4
        out dx, al
        inc dx
        in al, dx
        mov word ptr DGROUP:d_48dc, ax
        dec dx
        mov al, 5
        out dx, al
        inc dx
        in al, dx
        mov ah, al
        mov al, byte ptr DGROUP:d_48e8
        out dx, al
        mov di, 0ffffh
        mov bx, 0a000h
        mov es, bx
        stosb
        mov al, byte ptr DGROUP:d_48e6
        out dx, al
        dec dx
        mov al, 8
        out dx, al
        inc dx
        in al, dx
        mov word ptr DGROUP:d_48de, ax
        mov al, 0ffh
        out dx, al
        dec dx
        mov al, 3
        out dx, al
        inc dx
        in al, dx
        mov byte ptr DGROUP:d_48e2, al
        xor al, al
        out dx, al
        mov dx, 3c4h
        in al, dx
        mov ah, al
        mov al, 2
        out dx, al
        inc dx
        in al, dx
        mov word ptr DGROUP:d_48e0, ax
        mov al, 0fh
        out dx, al
        retf
_mouse_save_vga endp

/* 0x23cfe */
_mouse_restore_vga proc far
        mov dx, 3c4h
        mov al, 2
        out dx, al
        mov ax, word ptr DGROUP:d_48e0
        inc dx
        out dx, al
        dec dx
        mov al, ah
        out dx, al
        mov dx, 3ceh
        mov al, 3
        out dx, al
        mov al, byte ptr DGROUP:d_48e2
        inc dx
        out dx, al
        dec dx
        mov al, 8
        out dx, al
        mov ax, word ptr DGROUP:d_48de
        inc dx
        out dx, al
        dec dx
        mov al, 5
        out dx, al
        inc dx
        mov al, byte ptr DGROUP:d_48e8
        out dx, al
        mov di, 0ffffh
        mov bx, 0a000h
        mov es, bx
        mov al, byte ptr es:[di]
        mov al, ah
        out dx, al
        dec dx
        mov al, 4
        out dx, al
        mov ax, word ptr DGROUP:d_48dc
        inc dx
        out dx, al
        dec dx
        mov al, 1
        out dx, al
        mov al, ah
        inc dx
        out dx, al
        dec dx
        xor al, al
        out dx, al
        mov ax, word ptr DGROUP:d_48da
        inc dx
        out dx, al
        dec dx
        mov al, ah
        out dx, al
        retf
_mouse_restore_vga endp

/* 0x23d57 */
_remove_mouse proc far
        xor ax, ax
        cmp byte ptr DGROUP:d_48ea, al
        je L220e8
        mov byte ptr DGROUP:d_48ea, al
        int 33h
        mov ax, 0ch
        xor cx, cx
        mov dx, cx
        mov es, cx
        int 33h
        mov ax, 1
L220e8:
        retf
_remove_mouse endp

/* 0x23d73 */
_read_mouse_pointer proc far
        push bp
        mov bp, sp
        sub cx, cx
        mov dx, cx
        mov ax, cx
        mov al, byte ptr DGROUP:d_48ea
        neg ax
        jae L22111
        mov bx, word ptr [bp+6]
        mov ax, word ptr DGROUP:d_4740
        shr ax, 1
        shr ax, 1
        mov word ptr [bx], ax
        mov bx, word ptr [bp+8]
        mov ax, word ptr DGROUP:d_4742
        shr ax, 1
        shr ax, 1
        mov word ptr [bx], ax
L22111:
        pop bp
        retf
_read_mouse_pointer endp

/* 0x23d9d */
_mouse_move_to proc far
        push bp
        mov bp, sp
        mov al, byte ptr DGROUP:d_48ea
        neg al
        jae L2213a
        mov ax, 4
        mov cx, word ptr [bp+6]
        shl cx, 1
        shl cx, 1
        mov word ptr DGROUP:d_4740, cx
        mov dx, word ptr [bp+8]
        shl dx, 1
        shl dx, 1
        mov word ptr DGROUP:d_4742, dx
        int 33h
        mov al, 1
L2213a:
        mov ah, 0
        pop bp
        retf
_mouse_move_to endp

/* 0x23dc8 */
_read_mouse_button proc far
        push bp
        mov bp, sp
        sub bx, bx
        mov bl, byte ptr DGROUP:d_48ea
        neg bx
        jae L2215a
        mov bl, byte ptr DGROUP:d_48eb
        xor bh, bh
        mov ax, word ptr [bp+6]
        neg ax
        jae L2215a
        shr bx, 1
L2215a:
        mov ax, bx
        and ax, 1
        pop bp
        retf
_read_mouse_button endp

/* 0x23deb */
_normalise_pointer proc near
        push cx
        mov cx, ax
        and ax, 0fh
        shr cx, 1
        shr cx, 1
        shr cx, 1
        shr cx, 1
        add dx, cx
        pop cx
        ret
c_22173 db 0e8h, 0ebh, 0ffh
c_22176 db 0f6h, 0c6h, 0f0h
c_22179 db 74h, 8h
c_2217b db 5h, 0f0h, 0ffh
c_2217e db 81h, 0eah, 0ffh, 0fh
c_22182 db 0c3h
L22183:
        shl dx, 1
        shl dx, 1
        shl dx, 1
        shl dx, 1
        add ax, dx
        sub dx, dx
        ret
_normalise_pointer endp

/* 0x23e1a */
_huge_add_positive proc near
        add ax, bx
        sbb bx, bx
        and bx, 1000h
        add dx, bx
        mov bx, cx
        mov cl, 5
        clc
        rcr bx, cl
        add dx, bx
        ret
c_221a4 db 0f7h, 0dbh
c_221a6 db 3h, 0c3h
c_221a8 db 1bh, 0dbh
c_221aa db 0f7h, 0d3h
c_221ac db 81h, 0e3h, 0h, 10h
c_221b0 db 2bh, 0d3h
c_221b2 db 0e8h, 0ach, 0ffh
c_221b5 db 8bh, 0d9h
c_221b7 db 0b1h, 5h
c_221b9 db 0f8h
c_221ba db 0d3h, 0dbh
c_221bc db 2bh, 0d3h
c_221be db 0c3h
c_221bf db 0e3h, 12h
c_221c1 db 0f7h, 0c7h, 1h, 0h
c_221c5 db 75h, 2h
c_221c7 db 0a4h
c_221c8 db 49h
L221c9:
        dec si
        dec di
        shr cx, 1
        rep movsw
        rcl cx, 1
        inc si
        inc di
L221d3:
        rep movsb
        ret
c_221d6 db 0e3h, 10h
c_221d8 db 0f7h, 0c7h, 1h, 0h
c_221dc db 74h, 2h
c_221de db 0a4h
c_221df db 49h
L221e0:
        shr cx, 1
        rep movsw
        rcl cx, 1
        rep movsb
L221e8:
        ret
c_221e9 db 0h, 0h
c_221eb db 0h, 0h
_huge_add_positive endp

/* 0x23e77 */
_huge_move proc far
        push bp
        mov bp, sp
        sub sp, 4
        push si
        push di
        push ds
        mov word ptr cs:c_221e9, 5f11h
        mov word ptr cs:c_221eb, 5f86h
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        mov word ptr [bp-4], ax
        mov word ptr [bp-2], dx
        call _normalise_pointer
        mov word ptr [bp+6], ax
        mov word ptr [bp+8], dx
        mov ax, word ptr [bp+0ah]
        mov dx, word ptr [bp+0ch]
        call _normalise_pointer
        cmp dx, word ptr [bp+8]
        ja L2226c
        jb L22233
        cmp ax, word ptr [bp+6]
        ja L2226c
        jb L22233
        jmp L222b8
L22233:
        std
        mov word ptr cs:c_221e9, 5f23h
        mov word ptr cs:c_221eb, 5f6fh
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        mov bx, word ptr [bp+0eh]
        mov cx, word ptr [bp+10h]
        sub bx, 1
        sbb cx, 0
        js L222b8
        push cx
        push bx
        call _huge_add_positive
        mov word ptr [bp+6], ax
        mov word ptr [bp+8], dx
        mov ax, word ptr [bp+0ah]
        mov dx, word ptr [bp+0ch]
        pop bx
        pop cx
        call _huge_add_positive
L2226c:
        mov ds, dx
        mov si, ax
        mov es, word ptr [bp+8]
        mov di, word ptr [bp+6]
L22276:
        sub cx, cx
        mov bx, 7d00h
        cmp word ptr [bp+10h], cx
        jne L2228d
        mov ax, word ptr [bp+0eh]
        cmp ax, cx
        je L222b8
        cmp ax, bx
        jg L2228d
        mov bx, ax
L2228d:
        mov cx, bx
        mov ax, si
        mov dx, ds
        call word ptr cs:c_221e9
        mov si, ax
        mov ds, dx
        mov ax, di
        mov dx, es
        call word ptr cs:c_221e9
        mov di, ax
        mov es, dx
        call word ptr cs:c_221eb
        sub ax, ax
        sub word ptr [bp+0eh], bx
        sbb word ptr [bp+10h], ax
        jmp short L22276
L222b8:
        mov dx, word ptr [bp-2]
        mov ax, word ptr [bp-4]
        cld
        pop ds
        pop di
        pop si
        mov sp, bp
        pop bp
        retf
_huge_move endp

/* 0x23f50 */
_far_memcpy proc far
        push bp
        mov bp, sp
        mov cx, word ptr [bp+0eh]
        jcxz L222fe
        push si
        push di
        push ds
        mov ax, word ptr [bp+0ah]
        mov dx, word ptr [bp+0ch]
        call _normalise_pointer
        mov ds, dx
        mov si, ax
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        call _normalise_pointer
        mov es, dx
        mov di, ax
        test di, 1
        jae L222f3
        movsb
        dec cx
L222f3:
        shr cx, 1
        rep movsw
        rcl cx, 1
        rep movsb
        pop ds
        pop di
        pop si
L222fe:
        pop bp
        retf
_far_memcpy endp

/* 0x23f8a */
_far_memset proc far
        push bp
        mov bp, sp
        push si
        push di
        cld
        mov di, word ptr [bp+6]
        mov ax, word ptr [bp+8]
        mov es, ax
        mov ax, word ptr [bp+0ah]
        mov ah, al
        mov si, ax
L22315:
        sub bx, bx
        mov cx, 7d00h
        cmp word ptr [bp+0eh], bx
        jne L2232c
        mov ax, word ptr [bp+0ch]
        cmp ax, bx
        je L22356
        cmp ax, cx
        jg L2232c
        mov cx, ax
L2232c:
        mov bx, cx
        mov ax, di
        mov dx, es
        call _normalise_pointer
        mov di, ax
        mov es, dx
        mov ax, si
        cmp cx, 0ah
        jl L2234c
        or di, di
        jp L22346
        stosb
        dec cx
L22346:
        shr cx, 1
        rep stosw
        rcl cl, 1
L2234c:
        rep stosb
        sub word ptr [bp+0ch], bx
        sbb word ptr [bp+0eh], cx
        jmp short L22315
L22356:
        pop di
        pop si
        pop bp
        retf
_far_memset endp

/* 0x23fe4 */
_far_ptr_compare proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        call _normalise_pointer
        mov bx, ax
        mov cx, dx
        mov ax, word ptr [bp+0ah]
        mov dx, word ptr [bp+0ch]
        call _normalise_pointer
        cmp cx, dx
        jb L2237d
        ja L22381
        cmp bx, ax
        ja L22381
L2237d:
        sbb ax, ax
        pop bp
        retf
L22381:
        mov ax, 1
        pop bp
        retf
_far_ptr_compare endp

/* 0x24010 */
_normalise_pointer_far proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        call _normalise_pointer
        pop bp
        retf
_normalise_pointer_far endp

/* 0x2401e */
_install_divide_trap proc far
        push ax
        push es
        mov byte ptr DGROUP:d_48ec, 1
        sub ax, ax
        mov es, ax
        mov ax, word ptr es:[0]
        mov word ptr DGROUP:d_48ef, ax
        mov ax, word ptr es:[2]
        mov word ptr DGROUP:d_48ed, ax
        cli
        mov word ptr es:[0], offset _divide_error_handler
        mov word ptr es:[2], cs
        sti
        pop es
        pop ax
        retf
_install_divide_trap endp

/* 0x24048 */
_divide_error_handler proc near
        push bp
        mov bp, sp
        push bx
        push es
        push ax
        mov ax, word ptr [bp+2]
        mov bx, ax
        mov ax, word ptr [bp+4]
        mov es, ax
        mov ax, word ptr es:[bx]
        and ax, 0feh
        cmp ax, 0f6h
        je L223e1
        sub bx, 2
        mov ax, bx
        mov word ptr [bp+2], ax
L223e1:
        mov ax, word ptr es:[bx]
        test ax, 1
        je L223f0
        pop ax
        sar dx, 1
        rcr ax, 1
        jmp short L223f3
L223f0:
        pop ax
        sar ax, 1
L223f3:
        pop es
        pop bx
        pop bp
        iret
_divide_error_handler endp

/* 0x24081 */
_restore_int0_vector proc far
        push ax
        push es
        xor ax, ax
        cmp byte ptr DGROUP:d_48ec, al
        je L22418
        mov byte ptr DGROUP:d_48ec, al
        sub ax, ax
        mov es, ax
        cli
        mov ax, word ptr DGROUP:d_48ef
        mov ax, word ptr es:[0]
        mov ax, word ptr DGROUP:d_48ed
        mov ax, word ptr es:[2]
        sti
L22418:
        pop es
        pop ax
        retf
_restore_int0_vector endp

/* 0x240a5 */
_read_pixel_clipped proc far
        push bp
        mov bp, sp
        cmp byte ptr DGROUP:_g_vmds+3h, 0
        je L22443
        mov ax, word ptr [bp+6]
        cmp ax, word ptr DGROUP:_g_vmds+4h
        jl L22448
        cmp ax, word ptr DGROUP:_g_vmds+6h
        jg L22448
        mov ax, word ptr [bp+8]
        cmp ax, word ptr DGROUP:_g_vmds+8h
        jl L22448
        cmp ax, word ptr DGROUP:_g_vmds+0ah
        jg L22448
L22443:
        pop bp
        jmp dword ptr DGROUP:_g_vm_driver+58h
L22448:
        pop bp
        mov ax, 0ffffh
        retf
_read_pixel_clipped endp

/* 0x240d7 */
_plot_pixel_clipped proc far
        push bp
        mov bp, sp
        cmp byte ptr DGROUP:_g_vmds+3h, 0
        je L22475
        mov ax, word ptr [bp+6]
        cmp ax, word ptr DGROUP:_g_vmds+4h
        jl L2247a
        cmp ax, word ptr DGROUP:_g_vmds+6h
        jg L2247a
        mov ax, word ptr [bp+8]
        cmp ax, word ptr DGROUP:_g_vmds+8h
        jl L2247a
        cmp ax, word ptr DGROUP:_g_vmds+0ah
        jg L2247a
L22475:
        pop bp
        jmp dword ptr DGROUP:_g_vm_driver+5ch
L2247a:
        pop bp
        mov ax, 0ffffh
        retf
_plot_pixel_clipped endp

/* 0x24109 */
_restore_rect_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+20h
_restore_rect_thunk endp
LOWLEVEL_TEXT ends
}
#else


/*
 * **The mouse's position and the game's handler**, DGROUP 0x4740..0x4748,
 * 0x08 bytes. The position is kept at four times the pixel, which is what the
 * driver's own units are.
 */
struct engine_mouse {
    uint16_t  mouse_x;            /* +0x00 [2]  four times the pixel x: `mouse_move_to` stores `x << 2`
                                               and `read_mouse_pointer` answers `>> 2` */
    uint16_t  mouse_y;            /* +0x02 [2]  the same for y */
    void (far *mouse_handler_fn)(void); /* +0x04 [4]  the game's own handler, called by `mouse_event`;
                                               nothing in the image sets it */
} PACKED;

struct engine_mouse g_engine_mouse;

/*
 * **The cursor code's, the mouse's and the divide trap's data**, DGROUP
 * 0x48da..0x48f2, in address order.
 *
 * The graphics controller's registers the cursor code saves - 0 and 1, 4,
 * then 8 - the sequencer's map mask, and register 3.
 */
int16_t g_saved_gc_0_1;                  /* DGROUP 0x48da */
int16_t g_saved_gc_4;                    /* DGROUP 0x48dc */
int16_t g_saved_gc_8;                    /* DGROUP 0x48de */
int16_t g_saved_seq_map_mask;            /* DGROUP 0x48e0 */
uint8_t g_saved_gc_3;                    /* DGROUP 0x48e2 */

/*
 * **Two graphics-controller mode bytes**, both written to GC register 5 by the
 * cursor code: VGA write mode 2 - a byte selects a colour - and write mode 1,
 * the latch copy the routine parks with. The `_256` pair is the same two with
 * bit 6 set, the 256-colour shift; `mouse_init` copies them over the first
 * pair when the driver reports eight bits a pixel.
 */
uint8_t g_gc_mode_fill = 0x02;           /* DGROUP 0x48e6 */
uint8_t g_gc_mode_fill_256 = 0x40;       /* DGROUP 0x48e7 */
uint8_t g_gc_mode_copy = 0x01;           /* DGROUP 0x48e8 */
uint8_t g_gc_mode_copy_256 = 0x41;       /* DGROUP 0x48e9 */

/* Whether the mouse driver was taken - `neg al` branches on it - and the
   button byte `timer_callback` samples on the page flip. */
uint8_t g_mouse_taken;                   /* DGROUP 0x48ea */
uint8_t g_mouse_buttons;                 /* DGROUP 0x48eb */

/* Whether `install_divide_trap` has run, and the vector 0 it replaced: from
   0:0 and 0:2, a load and not a store, and stored **segment first** at an odd
   offset. */
uint8_t g_divide_hooked;                 /* DGROUP 0x48ec */
void interrupt (far *g_old_divide_vector)();   /* DGROUP 0x48ed */

/*
 * 0x237ce
 *
 * **Time the joystick's axes**: fire the one-shots and count until the chosen bits fall, into SI and DI. A near routine with register arguments. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source above is the original's.
 */
void joy_time_axes(void)
{
    not_transcribed("0x21b44, the joystick driver");
}

/*
 * 0x23835
 *
 * **Scale one axis** against its calibration: AX less BX, times CX, clamped to 127 with a dead zone of 8, the sign put back. A near routine with register arguments. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source above is the original's.
 */
int16_t joy_scale_axis(void)
{
    not_transcribed("0x21bab, the joystick driver");
    return 0;
}

/*
 * 0x2386c
 *
 * **Find and calibrate the joysticks**: time the 8253 against the port to size the delay loop, then take each stick's centre. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source above is the original's.
 */
int16_t joy_init(void)
{
    not_transcribed("0x21be2, the joystick driver");
    return 0;
}

/*
 * 0x239a3
 *
 * **Read a stick's two axes**, scaled. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source above is the original's.
 */
void joy_read(int16_t stick, int16_t *x, int16_t *y)
{
    not_transcribed("0x21d19, the joystick driver");
    (void)stick; (void)x; (void)y;
}

/*
 * 0x23a1b
 *
 * **A stick as four direction bits**: left 4, right 8, up 1, down 2, past thirty either way. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source above is the original's.
 */
uint16_t joy_direction(int16_t stick)
{
    not_transcribed("0x21d91, the joystick driver");
    (void)stick;
    return 0;
}

/*
 * 0x23a74
 *
 * **Is button `n` down?** Bit 4+n of port 0x201, inverted. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source above is the original's.
 */
uint16_t joy_button(uint16_t n)
{
    not_transcribed("0x21dea, the joystick driver");
    (void)n;
    return 0;
}

/*
 * 0x23a8e
 *
 * **One axis, scaled.** Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source above is the original's.
 */
int16_t joy_axis(uint16_t n)
{
    not_transcribed("0x21e04, the joystick driver");
    (void)n;
    return 0;
}

/*
 * 0x23abe
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

    if (g_vmds.clip_enabled != 0) {
        /* top */
        edge = g_vmds.clip_top;
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
        edge = g_vmds.clip_left;
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
        edge = g_vmds.clip_bottom;
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
        edge = g_vmds.clip_right;
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

/*
 * 0x23ba7
 *
 * Start the mouse, once. The flag at DGROUP 0x48ea says whether it has already
 * been done, and a second call answers 0 without touching anything.
 *
 * INT 33h AX=0 resets the driver and answers 0xffff if one is installed. The
 * original turns that into the flag with `neg ax`, which makes 1 from 0xffff
 * and 0 from 0, and sets carry for any non-zero answer - so the `jae` that
 * follows is "no mouse, give up", and the flag is written either way.
 *
 * With a driver there it sets the cursor far off-screen, shows and immediately
 * hides it, sets the mickeys-per-pixel to 8 by 8, puts the cursor at the
 * origin, limits it to the screen the game recorded at DGROUP 0x3f7a and
 * 0x3f7c, and installs the handler at 0x21fcf for the 0x1f events. None of
 * that is in guest memory.
 *
 * The two bytes it copies at the end are: on adapter 8 - the byte at DGROUP
 * 0x38ad, the same one that chooses the palette length - the cursor's hot spot
 * is taken from a second pair at 0x48e7 and 0x48e9.
 */
uint16_t mouse_init(void)
{
    uint16_t present;

    if (g_mouse_taken != 0)
        return 0;

    present = io_mouse_reset();
    g_mouse_taken = (uint8_t)(-(int16_t)present);

    if (present == 0)
        return 0;

    io_mouse_move_to(0x7fff, 0x7fff);
    io_mouse_show();
    io_mouse_hide();
    io_mouse_set_speed(8, 8);
    io_mouse_move_to(0, 0);

    mouse_set_ranges(0, 0, ((uint16_t)g_vmds.screen.screen_width), ((uint16_t)g_vmds.screen.screen_height));

    io_mouse_set_handler(0x1f, (void (far *)(void))mouse_event);

    if (((uint8_t)g_vmds.pixel_shift) == 8) {
        g_gc_mode_fill = g_gc_mode_fill_256;
        g_gc_mode_copy = g_gc_mode_copy_256;
    }

    return 1;
}

/*
 * 0x23c17
 *
 * Set how far the cursor may travel, from an origin and a size in cells: INT
 * 33h AX=7 for the horizontal range and AX=8 for the vertical. Both ends are
 * multiplied by four - the same cell-to-pixel scale `mouse_move_to` uses - and
 * the far end has one subtracted before scaling, so a size of `n` cells ends at
 * the last pixel of the `n`th rather than the first pixel of the next.
 *
 * It writes nothing to memory: the ranges live in the mouse driver.
 */
void mouse_set_ranges(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    io_mouse_set_x_range((uint16_t)(x << 2), (uint16_t)((x + w - 1) << 2));
    io_mouse_set_y_range((uint16_t)(y << 2), (uint16_t)((y + h - 1) << 2));
}

/*
 * 0x23c48
 *
 * Remember the game's own mouse handler, as a far pointer in DGROUP 0x4744 and
 * 0x4746. `mouse_event` below calls it after it has recorded the event, and
 * calls nothing when both words are zero.
 *
 * **Nothing in the image calls this**, and the only other references to either
 * word are the two inside `mouse_event`. Searched for both, as a far call and
 * as a near one, and as any instruction with those displacements: three sites
 * for 0x4744 and two for 0x4746, all of them here. So the pointer is never
 * set, the branch in `mouse_event` is never taken, and the two routines that
 * save and restore the VGA around it never run. Transcribed because the code
 * is there and the branch has to be right if it is ever reached; marked
 * unreachable because it is.
 */
void mouse_set_user_handler(void (far *h)(void))
{
    g_engine_mouse.mouse_handler_fn = h;
}

/*
 * 0x23c59
 *
 * **The mouse driver's callback.** INT 33h AX=0x0c installs this, and the
 * driver calls it on every event the mask asked for, with the button state in
 * `BL` and the position in `CX` and `DX`.
 *
 * It records all three in DGROUP - the buttons at 0x48eb, which is the byte
 * `read_mouse_button` answers from and therefore the *only* way a click reaches
 * the game - and then, if a handler is installed, saves the VGA, calls it, and
 * puts the VGA back.
 *
 * The first thing it does is switch to a stack of its own inside DGROUP, at
 * 0x48d8, with interrupts off across the two writes that change `ss` and `sp`
 * together. That is machine, not program: an interrupt arriving between them
 * would run on half a stack. The port has one stack and no interrupts to
 * arrive, so the switch is **not transcribed** - it is the one part of this
 * routine with nothing to correspond to. Everything it does to memory is here.
 */
void mouse_event(uint16_t buttons, uint16_t x, uint16_t y)
{
    g_mouse_buttons = (uint8_t)buttons;
    g_engine_mouse.mouse_x = x;
    g_engine_mouse.mouse_y = y;

    if (g_engine_mouse.mouse_handler_fn == NULL)
        return;

    mouse_save_vga();
    /* `lcall [0x4744]`. Nothing sets the pointer - see
       `mouse_set_user_handler` - so this is never reached. */
    g_engine_mouse.mouse_handler_fn();
    mouse_restore_vga();
}

/*
 * 0x23c99
 *
 * NEVER REACHED: only `mouse_event` calls this, on the branch that needs a
 * user handler, and nothing installs one. See `mouse_set_user_handler`.
 *
 * Save the VGA state the game's own mouse handler is about to disturb, into
 * DGROUP: graphics controller 0 and 1 at 0x48da, 4 at 0x48dc, 8 at 0x48de,
 * 3 at 0x48e2, and the sequencer's map mask at 0x48e0.
 *
 * Every one is read the same way - index the register, read the data port,
 * keep the old index in `ah` so it can go back - and then set to the value a
 * plain write needs: the set/reset registers to 0, the bit mask to 0xff, the
 * function select to 0, and the map mask to 0xf.
 *
 * The **read-modify-write latch** is the odd part. Between writing register 5
 * and restoring it the routine does `stosb` to `a000:ffff`, and on the way back
 * out reads the same byte. That is not a pixel: writing a byte through the VGA
 * loads the four latches from it, and reading one does the same, so this is how
 * the handler's own read-modify-write state is parked and picked up again. The
 * byte it uses is the very last in the aperture, which no mode this game runs
 * displays.
 *
 * Ours only in that the port has no latch to park: `vga_write`/`vga_read` in
 * io.c model the latches, so the same two accesses do the same thing here.
 */
void mouse_save_vga(void)
{
    uint16_t v;

    io_out8(PORT_GC_INDEX, 0);
    v = (uint16_t)(io_in8(PORT_GC_INDEX) << 8);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    g_saved_gc_0_1 = (int16_t)v;
    io_out8(PORT_GC_INDEX, 0);
    io_out8(PORT_GC_DATA, 0);

    io_out8(PORT_GC_INDEX, 1);
    v = (uint16_t)(io_in8(PORT_GC_DATA) << 8);
    io_out8(PORT_GC_DATA, 0);

    io_out8(PORT_GC_INDEX, 4);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    g_saved_gc_4 = (int16_t)v;

    io_out8(PORT_GC_INDEX, 5);
    v = (uint16_t)(io_in8(PORT_GC_DATA) << 8);
    io_out8(PORT_GC_DATA, g_gc_mode_copy);
    vga_write(0xffff, g_gc_mode_copy);          /* park the latches */
    io_out8(PORT_GC_DATA, g_gc_mode_fill);

    io_out8(PORT_GC_INDEX, 8);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    g_saved_gc_8 = (int16_t)v;
    io_out8(PORT_GC_DATA, 0xff);

    io_out8(PORT_GC_INDEX, 3);
    g_saved_gc_3 = io_in8(PORT_GC_DATA);
    io_out8(PORT_GC_DATA, 0);

    v = (uint16_t)(io_in8(PORT_SEQ_INDEX) << 8);
    io_out8(PORT_SEQ_INDEX, 2);
    v = (uint16_t)(v | io_in8(PORT_SEQ_DATA));
    g_saved_seq_map_mask = (int16_t)v;
    io_out8(PORT_SEQ_DATA, 0x0f);
}

/*
 * 0x23cfe
 *
 * NEVER REACHED, for the same reason as `mouse_save_vga`.
 *
 * Put back what `mouse_save_vga` took, in the reverse order, index register
 * last so the card is left selecting whatever it was selecting before. The
 * latch byte at `a000:ffff` is read rather than written this time, which
 * reloads the latches from it.
 */
void mouse_restore_vga(void)
{
    uint16_t v;

    io_out8(PORT_SEQ_INDEX, 2);
    v = (uint16_t)g_saved_seq_map_mask;
    io_out8(PORT_SEQ_DATA, (uint8_t)v);
    io_out8(PORT_SEQ_INDEX, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 3);
    io_out8(PORT_GC_DATA, g_saved_gc_3);

    io_out8(PORT_GC_INDEX, 8);
    v = (uint16_t)g_saved_gc_8;
    io_out8(PORT_GC_DATA, (uint8_t)v);

    io_out8(PORT_GC_INDEX, 5);
    io_out8(PORT_GC_DATA, g_gc_mode_copy);
    (void)vga_read(0xffff);                  /* pick the latches back up */
    io_out8(PORT_GC_DATA, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 4);
    v = (uint16_t)g_saved_gc_4;
    io_out8(PORT_GC_DATA, (uint8_t)v);

    io_out8(PORT_GC_INDEX, 1);
    io_out8(PORT_GC_DATA, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 0);
    v = (uint16_t)g_saved_gc_0_1;
    io_out8(PORT_GC_DATA, (uint8_t)v);
    io_out8(PORT_GC_INDEX, (uint8_t)(v >> 8));
}

/*
 * 0x23d57
 *
 * **Let the mouse go.** The flag at DGROUP 0x48ea says the driver was taken
 * over; clearing it, INT 33h AX=0 resets the driver and AX=0x0C with ES:DX
 * zero takes the event handler off it. Answers 1 if it did the work.
 */
int16_t remove_mouse(void)
{
    if (g_mouse_taken == 0)
        return 0;

    g_mouse_taken = 0;

    io_mouse_reset();
    io_mouse_set_handler(0, NULL);

    return 1;
}

/*
 * 0x23d73
 *
 * If the flag byte at DGROUP 0x48ea is set, store a quarter of each of the two
 * words at DGROUP 0x4740 and 0x4742 through the two near pointers passed in.
 * If it is clear, both are left alone - the routine writes nothing at all,
 * which a caller that did not initialise them would notice.
 *
 * The `neg`/`jae` pair again: carry is set exactly when the byte was non-zero.
 */
void read_mouse_pointer(int16_t *x, int16_t *y)
{
    if (g_mouse_taken == 0)
        return;
    *x = (int16_t)(g_engine_mouse.mouse_x >> 2);
    *y = (int16_t)(g_engine_mouse.mouse_y >> 2);
}

/*
 * 0x23d9d
 *
 * Put the mouse cursor at a given cell: INT 33h AX=4, with the position
 * multiplied by four into DGROUP 0x4740 and 0x4742.
 *
 * The two DGROUP words are written whether or not the driver is there, but only
 * when the flag at 0x48ea says it is - `neg al` sets carry for any non-zero
 * byte, and `jae` skips everything on a zero one. Answers 1 when it moved the
 * cursor and 0 when there was no mouse.
 *
 * The interrupt itself is not reproduced: the port has no mouse driver, and the
 * call leaves nothing in guest memory to compare. What it writes to DGROUP is
 * what anything else can see.
 */
uint16_t mouse_move_to(uint16_t x, uint16_t y)
{
    if (g_mouse_taken == 0)
        return 0;

    g_engine_mouse.mouse_x = (int16_t)(x << 2);
    g_engine_mouse.mouse_y = (int16_t)(y << 2);

    /*
     * `mov ax,4 / int 0x33` - the driver's "set cursor position", with the
     * quartered coordinates already in CX and DX. The port had the two DGROUP
     * words and not the call, so the pointer was never actually warped.
     */
    io_mouse_move_to((uint16_t)(x << 2), (uint16_t)(y << 2));

    return 1;
}

/*
 * 0x23dc8
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
int16_t read_mouse_button(uint16_t which)
{
    uint16_t v = g_mouse_taken;

    if (v == 0)
        return 0;

    v = g_mouse_buttons;
    if (which != 0)
        v >>= 1;
    return (int16_t)(v & 1);
}

/*
 * 0x23deb
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
 * A near routine taking and answering registers - AX and DX, the offset and
 * the segment of one far pointer - so the port passes that pointer by
 * reference: the segment plus the offset's paragraphs, and the offset's low
 * four bits.
 */
void normalise_pointer(uint8_t far **p)
{
    /* Carry the offset's paragraphs into the segment. A host pointer is one
       address, and normalising changes nothing about it. */
    (void)p;
}

/*
 * 0x23e1a
 *
 * Add a signed 32-bit byte count to a far pointer, the offset in `AX` and
 * segment in `DX`, the count in `CX:BX`. This is the **positive** door; the
 * negative one is at 0x221a4 and normalises afterwards, which this does not.
 *
 * The carry out of the offset add becomes 0x1000 paragraphs on the segment,
 * built without a branch: `sbb bx,bx` makes -1 or 0 and `and bx,0x1000` picks
 * the bit.
 *
 * The high half is folded in with **one `rcr bx,5`** after a `clc`, which is a
 * seventeen-bit rotate: the result is `(cx >> 5) | ((cx & 0xf) << 12)`. The
 * second term is the `cx * 0x1000` the arithmetic wants; the first is a
 * leftover that is zero only while `cx` is under 32, which for a count under
 * two megabytes it is. Transcribed as the rotate it is rather than as the
 * multiply it stands for.
 */
uint8_t far *huge_add_positive(uint8_t far *p, uint32_t delta)
{
    uint16_t lo = (uint16_t)delta;              /* BX */
    uint16_t hi = (uint16_t)(delta >> 16);      /* CX */
    uint32_t sum = (uint32_t)FP_OFF(p) + lo;
    dg_seg_t seg = FP_SEG(p);

    if (sum > 0xffff)
        seg = seg + 0x1000;

    /* The rotate takes the high word alone, which is why it is split out of
       the count here rather than shifted as a whole. */
    seg = seg + (uint16_t)((hi >> 5) | ((hi & 0xf) << 12));

    return MK_FP(seg, (uint16_t)sum);
}

/*
 * 0x23e77
 *
 * Copy `count` bytes between two far pointers, with a **32-bit** count, safe
 * when the two overlap. Answers the destination it was given, unnormalised.
 *
 * The original is this written for an 8086: it normalises both pointers,
 * compares them, and dispatches through a pair of function pointers it stores
 * at `cs:[0x5f99]` and `cs:[0x5f9b]` - the normaliser (0x22161 going up,
 * 0x22173 going down) and the copy loop (0x221d6 up, 0x221bf down, the latter
 * under `std`). It works in chunks of at most 0x7d00 bytes, renormalising at
 * the top of each so an offset can never carry past 64K, and each loop copies
 * one byte first where that aligns the destination and then moves words.
 *
 * **That machinery is not reconstructed.** All of it - the indirect calls, the
 * word moves, the chunking, the alignment step - is there to make the copy fast
 * on a 16-bit machine. The port has a flat address space and none of those
 * costs, and what the two artefacts have to agree on is which bytes end up
 * where, which is exactly `memmove`'s contract.
 *
 * So the port takes two pointers and a count. The original's own `seg:off`
 * arithmetic was the *only* thing that forced its source to be a guest
 * address - it never read the source, it indexed memory by the linear address
 * it computed - and with that gone `load_palette` hands it an ordinary C
 * array.
 *
 * One thing does not survive the change. The routine answered the destination
 * pair it was given, unnormalised, in DX:AX; a pointer cannot spell an
 * unnormalised pair, so it answers the destination pointer and `routines.def`
 * says RET_NONE. Nothing reads the answer: `load_palette` is the only caller
 * and drops it, and `syms.c` does not dispatch this routine.
 *
 * The two dispatch words *do* survive, and getting them wrong was the first
 * thing the verifier said - `0x5f11/0x5f86` against `0x5f23/0x5f6f`, two bytes
 * of a 578-byte write. They are written from the same comparison the original
 * makes. They are *data* that happens to sit in a code segment, the same as
 * the saved timer vector further up this module. Nothing reads them back.
 */
uint8_t far * huge_move(uint8_t far * dst, const uint8_t far * src, uint32_t count)
{
    /* The original's dispatch offsets - `normalise_pointer` at 0x22161 and the
       forward copy at 0x221d6 - stored for the comparison's sake only: this
       body calls neither through them. Overwritten below if the copy has to
       go down. */
    g_s1c_huge_move.normalise_off = 0x5f11;
    g_s1c_huge_move.copy_off = 0x5f86;

    /* The original compares the two linear addresses; the host's pointers
       are them. */
    if (src < (const uint8_t far *)dst) {
        /* The normalise that steps back a paragraph, and the backward copy. */
        g_s1c_huge_move.normalise_off = 0x5f23;
        g_s1c_huge_move.copy_off = 0x5f6f;
    }

    memmove((void *)(uintptr_t)dst, (const void *)(uintptr_t)src, count);

    return dst;
}

/*
 * 0x23f50
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
void far_memcpy(uint8_t far * dst, const uint8_t far * src, uint16_t count)
{
    uint16_t words;

    if (count == 0)
        return;

    /*
     * The original normalises both pairs and then steps the offsets 16-bit,
     * which is `rep movsw` on a machine with segments. A host pointer is that
     * normalised address already, and the wrap goes with the pair - measured
     * across the intro, the briefing, the picker and all twenty-eight level
     * snapshots, neither end ever reached `off + count > 0x10000`.
     *
     * The words-then-a-byte shape is kept: it decides which byte of an odd
     * count is copied last, and the word is read at an odd address the
     * way the guest's unaligned `movsw` does.
     */
    words = (uint16_t)(count >> 1);
    while (words--) {
        *(int16_t *)(dst) = *(int16_t *)(src);
        src += 2;
        dst += 2;
    }
    if (count & 1)
        *dst = *src;
}

/*
 * 0x23f8a
 *
 * Set `count` bytes to the low byte of `value`, starting at a far pointer.
 *
 * **The machinery is not reconstructed**, on the same reasoning as `huge_move`
 * above: the original works in chunks of at most 0x7d00 bytes, renormalising
 * the pointer at the top of each so an offset can never carry past 64K, writes
 * one byte first where that makes the destination even, and then moves words
 * with both halves of the value. Every part of that is there to make the fill
 * fast on a 16-bit machine, and none of it changes which bytes end up holding
 * what - which is `memset`'s contract.
 *
 * The offset stepping was the only thing that made the destination a guest
 * address: `off = (uint16_t)(off + 2)` is a 16-bit add, and the renormalisation
 * is what keeps it from wrapping. Neither survives as a pointer and neither
 * needs to.
 */
void far_memset(uint8_t far * dst, uint16_t value, uint32_t count)
{
    memset((void *)(uintptr_t)dst, (int)(value & 0xff), count);
}

/*
 * 0x23fe4
 *
 * **Compare two far pointers**, each normalised first: -1 below, 0 equal,
 * 1 above. Nothing calls it.
 */
int16_t far_ptr_compare(const uint8_t far *a, const uint8_t far *b)
{
    if (a < b)
        return -1;
    return a == b ? 0 : 1;
}

/*
 * 0x24010
 *
 * The far-callable face of `normalise_pointer` at 0x22161: load the pointer
 * into AX and DX, call the near routine, and let its registers be the result.
 * So this answers a normalised far pointer in DX:AX, like any other far
 * routine returning a long.
 */
uint8_t far *normalise_pointer_far(uint8_t far *p)
{
    normalise_pointer(&p);
    return p;
}

/*
 * 0x2401e
 *
 * Take over INT 0, the divide-by-zero trap. The old vector is kept at DGROUP
 * 0x48ed and the new one points at 0x616e in this code segment - the handler
 * that begins immediately after this routine. DGROUP 0x48ec records that it
 * has been done.
 *
 * The original writes the vector table directly, at absolute 0; the port
 * hands the handler to `setvect`, whose table stands for it.
 *
 * The two halves are stored the other way round from how they are read: the
 * offset from 0:0 goes to 0x48ef and the segment from 0:2 to 0x48ed, so the
 * saved pair is segment-first.
 */
void install_divide_trap(void)
{
    g_divide_hooked = 1;

    g_old_divide_vector = getvect(0);

    setvect(0, (void interrupt (far *)())divide_error_handler);
}

/*
 * 0x24048
 *
 * **The divide-error handler** `install_divide_trap` puts at INT 0: it steps
 * the saved IP over the faulting `div` and returns. An interrupt handler;
 * the host takes no INT 0, so it has no body to run. A stub, which aborts.
 */
void divide_error_handler(void)
{
    not_transcribed("0x223be, the divide-error handler");
}

/*
 * 0x24081
 *
 * **A restore that restores nothing**, and it is the original's and not a
 * transcription slip.
 *
 * The flag at DGROUP 0x48ec is cleared, and then two pairs of instructions
 * that look like they put vector 0 back - `mov ax,[0x48ef]` then
 * `mov ax,es:[0]`, and the same for [0x48ed] and es:[2] - are **both loads**.
 * The bytes are `a1 ef 48 26 a1 00 00`; a store would be `26 a3`. So the saved
 * words are read into AX and thrown away, and the vector at 0000:0000 is read
 * and thrown away too. Nothing in memory changes.
 *
 * Written out as the flag clear it is, with the loads left off because a load
 * into a register nothing reads is not something C can express and not
 * something anything can observe.
 */
void restore_int0_vector(void)
{
    if (g_divide_hooked == 0)
        return;

    g_divide_hooked = 0;
}

/*
 * 0x240a5
 *
 * Read a pixel if it is inside the driver's clip window, and answer -1 if it
 * was not.
 *
 * The same guard as `plot_pixel_clipped` at 0x2244d, on the same four inclusive
 * bounds and the same on/off byte at 0x3893, differing only in taking two
 * arguments instead of three and jumping through DGROUP 0x439a. The two sit
 * next to each other in the image and are plainly one pair.
 *
 * -1 for "outside" is not distinguishable from a legitimately read colour only
 * because colours here are 0..15; any answer above that is the clip.
 */
int16_t read_pixel_clipped(int16_t x, int16_t y)
{
    if (g_vmds.clip_enabled != 0) {
        if (x < g_vmds.clip_left)
            return -1;
        if (x > g_vmds.clip_right)
            return -1;
        if (y < g_vmds.clip_top)
            return -1;
        if (y > g_vmds.clip_bottom)
            return -1;
    }

    return (int16_t)vm_read_pixel(x, y);
}

/*
 * 0x240d7
 *
 * Plot a pixel if it is inside the driver's clip window, and answer -1 if it
 * was not.
 *
 * The window is four words in the driver's data block - 0x3894 and 0x3896 for
 * x, 0x3898 and 0x389a for y - and both bounds are **inclusive**, tested with
 * `jl` and `jg`. The byte at 0x3893 switches the whole test off, and with it
 * zero any coordinate is passed straight through.
 *
 * The call into the driver is an `ljmp` through DGROUP 0x439e, not a call, so
 * the driver runs on this frame, sees the same three arguments, and returns
 * directly to whoever called here. The third argument is the colour and is
 * never looked at on the way past.
 *
 * That also means the answer on the drawn path is not chosen: it is whatever
 * the driver left in AX, which is 0xff08. Only the clipped path returns a
 * deliberate value.
 */
int16_t plot_pixel_clipped(int16_t x, int16_t y, int16_t colour)
{
    if (g_vmds.clip_enabled != 0) {
        if (x < g_vmds.clip_left)
            return -1;
        if (x > g_vmds.clip_right)
            return -1;
        if (y < g_vmds.clip_top)
            return -1;
        if (y > g_vmds.clip_bottom)
            return -1;
    }

    return (int16_t)vm_plot_pixel(x, y, (uint8_t)colour);
}

/*
 * 0x24109
 *
 * A thunk into the video driver: `ljmp [0x4362]`, which is `vm_restore_rect`.
 * Same arrangement as 0x2149a.
 */
void restore_rect_thunk(const uint8_t far * buf, int16_t x,
                        int16_t y, int16_t w, int16_t h)
{
    vm_restore_rect(buf, x, y, w, h);
}
#endif
