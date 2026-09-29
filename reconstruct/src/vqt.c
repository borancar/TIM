/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The quadtree walkers, which were written in assembly.** This file
 * corresponds to the second module of the original's code segment 248f,
 * image 0x25953..0x26198: `vqt_read_bits`, the two node walkers, their two
 * leaves and `far_copy`. It was part of `bitmaps.c` until 2026-09-26; the C
 * module ends at 0x25953 with a compiler's epilogue, and what follows is not
 * a compiler's - `vqt_read_bits` expanded in place as a macro, BP borrowed as
 * a pointer, `add sp,0x10a / pop bp` epilogues, and early exits that jump to
 * an epilogue placed *before* the routine's own entry.
 *
 * **So it is TASM source**, the `#ifdef __TURBOC__` block below, with the
 * host's transcription in the `#else`. The functions are in address order and each
 * carries the image offset it was read from, as everywhere else.
 *
 * The records it walks are `bitmaps.c`'s: `BITMAPS.walk`, the reader the
 * bitmap and screen loaders point it at.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler bc3.00
 */
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
extrn _redraw_cursor:far
extrn _BITMAP_COMPRESS:byte
extrn _VM_DRIVER:byte
extrn _VMDS:byte
VQT_TEXT segment byte public 'CODE'
assume cs:VQT_TEXT, ds:DGROUP
public _vqt_read_bits, _vqt_screen_node, _fill_screen_quadrant, _far_copy
public _vqt_node, _fill_quadrant

/* 0x25953 */
_vqt_read_bits proc near
        push bp
        mov bp, sp
        mov bx, word ptr [bp+4]
        mov bp, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp]
        mov dx, word ptr [bp+2]
        add word ptr [bp], cx
        adc word ptr [bp+2], 0
        mov cx, word ptr [bp+4]
        mov es, word ptr [bp+6]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        ret
L2599a:
        pop di
        pop si
        add sp, 2
        pop bp
        ret
_vqt_read_bits endp

/* 0x259a1 */
_vqt_screen_node proc near
        push bp
        mov bp, sp
        sub sp, 2
        push si
        push di
        mov si, word ptr [bp+8]
        mov di, word ptr [bp+0ah]
        mov ax, si
        or ax, di
        je L2599a
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 4
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        and ax, 0fh
        mov byte ptr [bp-2], al
        mov ax, di
        shr ax, 1
        push ax
        mov ax, si
        shr ax, 1
        push ax
        push word ptr [bp+6]
        push word ptr [bp+4]
        test byte ptr [bp-2], 8
        je L25a0e
        call _vqt_screen_node
        jmp short L25a1d
L25a0e:
        call _fill_screen_quadrant
        mov ax, word ptr DGROUP:_VMDS+14h
        push ax
        call FAR PTR _redraw_cursor
        add sp, 2
L25a1d:
        add sp, 8
        mov ax, di
        shr ax, 1
        push ax
        mov ax, si
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        push word ptr [bp+6]
        shr bx, 1
        add bx, word ptr [bp+4]
        push bx
        test byte ptr [bp-2], 4
        je L25a41
        call _vqt_screen_node
        jmp short L25a44
L25a41:
        call _fill_screen_quadrant
L25a44:
        add sp, 8
        mov ax, di
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        mov ax, si
        shr ax, 1
        push ax
        shr bx, 1
        add bx, word ptr [bp+6]
        push bx
        push word ptr [bp+4]
        test byte ptr [bp-2], 2
        je L25a68
        call _vqt_screen_node
        jmp short L25a6b
L25a68:
        call _fill_screen_quadrant
L25a6b:
        add sp, 8
        mov ax, di
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        mov ax, si
        mov cx, ax
        inc ax
        shr ax, 1
        push ax
        shr bx, 1
        add bx, word ptr [bp+6]
        push bx
        shr cx, 1
        add cx, word ptr [bp+4]
        push cx
        test byte ptr [bp-2], 1
        je L25a95
        call _vqt_screen_node
        jmp short L25a98
L25a95:
        call _fill_screen_quadrant
L25a98:
        add sp, 8
        pop di
        pop si
        add sp, 2
        pop bp
        ret
L25aa2:
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
_vqt_screen_node endp

/* 0x25aaa */
_fill_screen_quadrant proc near
        push bp
        mov bp, sp
        sub sp, 10ah
        push si
        push di
        mov ax, word ptr [bp+0ah]
        or ax, ax
        je L25aa2
        mov di, ax
        mov ax, word ptr [bp+8]
        or ax, ax
        je L25aa2
        mov si, ax
        cmp si, 1
        jne L25b39
        cmp di, 1
        jne L25b39
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov ch, al
        mov bx, word ptr [bp+6]
        shl bx, 1
        mov bx, word ptr [bx+3f82h]
        mov ax, word ptr [bp+4]
        mov cl, al
        shr ax, 1
        shr ax, 1
        add bx, ax
        and cl, 3
        mov dx, 3c4h
        mov ax, 102h
        shl ah, cl
        out dx, ax
        mov ax, word ptr DGROUP:_VMDS+18h
        mov es, ax
        mov byte ptr es:[bx], ch
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
L25b39:
        mov ax, si
        mov bx, di
        mul bl
        mov word ptr [bp-6], ax
        mov cx, 8
        or ah, ah
        jne L25b52
        xor cx, cx
        dec al
L25b4d:
        inc cx
        shr al, 1
        jne L25b4d
L25b52:
        push bp
        mov bx, cx
        mov bp, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp]
        mov dx, word ptr [bp+2]
        add word ptr [bp], cx
        adc word ptr [bp+2], 0
        mov cx, word ptr [bp+4]
        mov es, word ptr [bp+6]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        xor cx, cx
        mov word ptr [bp-4], ax
        or al, al
        je L25ba3
L25b9e:
        inc cx
        shr al, 1
        jne L25b9e
L25ba3:
        mov ax, word ptr [bp+4]
        mov di, ax
        add ax, word ptr [bp+8]
        mov word ptr [bp-8], ax
        mov ax, word ptr [bp+6]
        mov si, ax
        add ax, word ptr [bp+0ah]
        mov word ptr [bp-0ah], ax
        mov word ptr [bp-2], cx
        inc byte ptr [bp-4]
        mov ax, word ptr [bp-6]
        mov bx, ax
        shl bx, 1
        shl bx, 1
        shl bx, 1
        mul word ptr [bp-2]
        mov cx, word ptr [bp-4]
        shl cx, 1
        shl cx, 1
        shl cx, 1
        add ax, cx
        cmp bx, ax
        ja L25c53
L25bdc:
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov ch, al
        mov bx, si
        shl bx, 1
        mov bx, word ptr [bx+3f82h]
        mov ax, di
        mov cl, al
        shr ax, 1
        shr ax, 1
        add bx, ax
        and cl, 3
        mov dx, 3c4h
        mov ax, 102h
        shl ah, cl
        out dx, ax
        mov ax, word ptr DGROUP:_VMDS+18h
        mov es, ax
        mov byte ptr es:[bx], ch
        inc si
        cmp si, word ptr [bp-0ah]
        jl L25bdc
        mov si, word ptr [bp+6]
        inc di
        cmp di, word ptr [bp-8]
        jl L25bdc
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
L25c53:
        cmp byte ptr [bp-4], 1
        jne L25cbf
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov bx, word ptr [bp+6]
        shl bx, 1
        mov di, word ptr [bx+3f82h]
        mov bx, word ptr DGROUP:_VMDS+18h
        mov es, bx
        mov si, word ptr [bp+0ah]
L25ca3:
        mov cx, word ptr [bp+8]
        mov bx, word ptr [bp+4]
        push di
        mov ah, al
        call dword ptr DGROUP:_VM_DRIVER+2ch
        pop di
        add di, 50h
        dec si
        jne L25ca3
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
L25cbf:
        lea di, [bp-10ah]
L25cc3:
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov bx, ds
        mov es, bx
        stosb
        dec byte ptr [bp-4]
        jne L25cc3
        mov di, word ptr [bp+4]
L25d08:
        push bp
        mov bx, word ptr [bp-2]
        mov bp, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp]
        mov dx, word ptr [bp+2]
        add word ptr [bp], cx
        adc word ptr [bp+2], 0
        mov cx, word ptr [bp+4]
        mov es, word ptr [bp+6]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        lea bx, [bp-10ah]
        add bx, ax
        mov al, byte ptr [bx]
        mov ch, al
        mov bx, si
        shl bx, 1
        mov bx, word ptr [bx+3f82h]
        mov ax, di
        mov cl, al
        shr ax, 1
        shr ax, 1
        add bx, ax
        and cl, 3
        mov dx, 3c4h
        mov ax, 102h
        shl ah, cl
        out dx, ax
        mov ax, word ptr DGROUP:_VMDS+18h
        mov es, ax
        mov byte ptr es:[bx], ch
        inc si
        cmp si, word ptr [bp-0ah]
        jl L25d08
        mov si, word ptr [bp+6]
        inc di
        cmp di, word ptr [bp-8]
        jae L25d8e
        jmp L25d08
L25d8e:
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
_fill_screen_quadrant endp

/* 0x25d96 */
_far_copy proc near
        push bp
        mov bp, sp
        push ds
        push si
        push di
        les di, dword ptr [bp+4]
        lds si, dword ptr [bp+8]
        mov cx, word ptr [bp+0ch]
        shr cx, 1
        rep movsw
        jae L25dac
        movsb
L25dac:
        pop di
        pop si
        pop ds
        pop bp
        ret
L25db1:
        pop di
        pop si
        add sp, 2
        pop bp
        ret
_far_copy endp

/* 0x25db8 */
_vqt_node proc near
        push bp
        mov bp, sp
        sub sp, 2
        push si
        push di
        mov si, word ptr [bp+8]
        mov di, word ptr [bp+0ah]
        mov ax, si
        or ax, di
        je L25db1
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 4
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        and ax, 0fh
        mov byte ptr [bp-2], al
        mov ax, di
        shr ax, 1
        push ax
        mov ax, si
        shr ax, 1
        push ax
        push word ptr [bp+6]
        push word ptr [bp+4]
        test byte ptr [bp-2], 8
        je L25e25
        call _vqt_node
        jmp short L25e28
L25e25:
        call _fill_quadrant
L25e28:
        add sp, 8
        mov ax, di
        shr ax, 1
        push ax
        mov ax, si
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        push word ptr [bp+6]
        shr bx, 1
        add bx, word ptr [bp+4]
        push bx
        test byte ptr [bp-2], 4
        je L25e4c
        call _vqt_node
        jmp short L25e4f
L25e4c:
        call _fill_quadrant
L25e4f:
        add sp, 8
        mov ax, di
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        mov ax, si
        shr ax, 1
        push ax
        shr bx, 1
        add bx, word ptr [bp+6]
        push bx
        push word ptr [bp+4]
        test byte ptr [bp-2], 2
        je L25e73
        call _vqt_node
        jmp short L25e76
L25e73:
        call _fill_quadrant
L25e76:
        add sp, 8
        mov ax, di
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        mov ax, si
        mov cx, ax
        inc ax
        shr ax, 1
        push ax
        shr bx, 1
        add bx, word ptr [bp+6]
        push bx
        shr cx, 1
        add cx, word ptr [bp+4]
        push cx
        test byte ptr [bp-2], 1
        je L25ea0
        call _vqt_node
        jmp short L25ea3
L25ea0:
        call _fill_quadrant
L25ea3:
        add sp, 8
        pop di
        pop si
        add sp, 2
        pop bp
        ret
L25ead:
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
_vqt_node endp

/* 0x25eb5 */
_fill_quadrant proc near
        push bp
        mov bp, sp
        sub sp, 10ah
        push si
        push di
        mov ax, word ptr [bp+0ah]
        or ax, ax
        je L25ead
        mov di, ax
        mov ax, word ptr [bp+8]
        or ax, ax
        je L25ead
        mov si, ax
        cmp si, 1
        jne L25f3f
        cmp di, 1
        jne L25f3f
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov di, word ptr [bp+4]
        mov dl, al
        mov cx, di
        mov di, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        lea bx, [di+18h]
        mov ax, word ptr [bp+6]
        shl ax, 1
        add bx, ax
        mov bx, word ptr [bx]
        mov ax, cx
        add bx, ax
        les di, dword ptr [di+8]
        add di, bx
        mov al, dl
        stosb
        mov di, cx
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
L25f3f:
        mov ax, si
        mov bx, di
        mul bl
        mov word ptr [bp-6], ax
        mov cx, 8
        or ah, ah
        jne L25f58
        xor cx, cx
        dec al
L25f53:
        inc cx
        shr al, 1
        jne L25f53
L25f58:
        push bp
        mov bx, cx
        mov bp, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp]
        mov dx, word ptr [bp+2]
        add word ptr [bp], cx
        adc word ptr [bp+2], 0
        mov cx, word ptr [bp+4]
        mov es, word ptr [bp+6]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        xor cx, cx
        mov word ptr [bp-4], ax
        or al, al
        je L25fa9
L25fa4:
        inc cx
        shr al, 1
        jne L25fa4
L25fa9:
        mov ax, word ptr [bp+4]
        mov di, ax
        add ax, word ptr [bp+8]
        mov word ptr [bp-8], ax
        mov ax, word ptr [bp+6]
        mov si, ax
        add ax, word ptr [bp+0ah]
        mov word ptr [bp-0ah], ax
        mov word ptr [bp-2], cx
        inc byte ptr [bp-4]
        mov ax, word ptr [bp-6]
        mov bx, ax
        shl bx, 1
        shl bx, 1
        shl bx, 1
        mul word ptr [bp-2]
        mov cx, word ptr [bp-4]
        shl cx, 1
        shl cx, 1
        shl cx, 1
        add ax, cx
        cmp bx, ax
        ja L26052
L25fe2:
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov dl, al
        mov cx, di
        mov di, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        lea bx, [di+18h]
        mov ax, si
        shl ax, 1
        add bx, ax
        mov bx, word ptr [bx]
        mov ax, cx
        add bx, ax
        les di, dword ptr [di+8]
        add di, bx
        mov al, dl
        stosb
        mov di, cx
        inc si
        cmp si, word ptr [bp-0ah]
        jl L25fe2
        mov si, word ptr [bp+6]
        inc di
        cmp di, word ptr [bp-8]
        jl L25fe2
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
L26052:
        cmp byte ptr [bp-4], 1
        jne L260c9
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov byte ptr [bp-2], al
        mov dx, word ptr [bp+6]
L26096:
        mov si, word ptr [bp+4]
        mov cx, word ptr [bp+8]
L2609c:
        mov di, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        lea bx, [di+18h]
        mov ax, dx
        shl ax, 1
        add bx, ax
        mov bx, word ptr [bx]
        mov ax, si
        add bx, ax
        les di, dword ptr [di+8]
        add di, bx
        mov al, byte ptr [bp-2]
        stosb
        inc si
        loop L2609c
        inc dx
        dec word ptr [bp+0ah]
        jne L26096
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
L260c9:
        lea di, [bp-10ah]
L260cd:
        mov bx, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, word ptr [bx]
        mov dx, word ptr [bx+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+2], cx
        mov cx, word ptr [bx+4]
        mov es, word ptr [bx+6]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov bx, ds
        mov es, bx
        stosb
        dec byte ptr [bp-4]
        jne L260cd
        mov di, word ptr [bp+4]
L26112:
        push bp
        mov bx, word ptr [bp-2]
        mov bp, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp]
        mov dx, word ptr [bp+2]
        add word ptr [bp], cx
        adc word ptr [bp+2], 0
        mov cx, word ptr [bp+4]
        mov es, word ptr [bp+6]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        lea bx, [bp-10ah]
        add bx, ax
        mov al, byte ptr [bx]
        mov dl, al
        mov cx, di
        mov di, word ptr DGROUP:_BITMAP_COMPRESS+2ah
        lea bx, [di+18h]
        mov ax, si
        shl ax, 1
        add bx, ax
        mov bx, word ptr [bx]
        mov ax, cx
        add bx, ax
        les di, dword ptr [di+8]
        add di, bx
        mov al, dl
        stosb
        mov di, cx
        inc si
        cmp si, word ptr [bp-0ah]
        jl L26112
        mov si, word ptr [bp+6]
        inc di
        cmp di, word ptr [bp-8]
        jge L26190
        jmp short L26112
L26190:
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
_fill_quadrant endp
VQT_TEXT ends
}
#else

/*
 * 0x25953
 *
 * **Read `bits` bits** from the reader `BITMAPS.walk` names, and step its
 * position past them. `vqt_read_fn`'s only target.
 *
 * The same read `vqt_node` makes for its four: a word at `data.off + (pos >>
 * 3)`, the offset stepped inside the segment, shifted down by the position's
 * low three bits. The mask is `0xff00 rol bits` with the high byte cleared,
 * which is `(1 << bits) - 1` for the eight counts that make sense and 0 for a
 * count of 0; the rotate is written out so the other counts give what `rol`
 * gives. The position is a 32-bit add of the whole 16-bit count.
 *
 * A **** routine: it reuses BP as the record pointer and restores it. Reached
 * from the mirrored quadtree only, which this game's data never draws. The
 * name is ours.
 */
uint16_t near vqt_read_bits(uint16_t bits)
{
    struct vqt_reader *rd = BITMAPS.walk;
    uint16_t turn = (uint16_t)((bits & 0x1f) % 16);
    uint16_t mask = (uint16_t)(((uint16_t)(0xff00u << turn)
                                | (uint16_t)(0xff00u >> ((16 - turn) & 15)))
                               & 0x00ff);
    uint32_t pos = (uint32_t)rd->pos;
    uint16_t word;

    rd->pos = (int32_t)(pos + bits);
    word = (uint16_t)(rd->data[pos >> 3] | (rd->data[(pos >> 3) + 1] << 8));
    return (uint16_t)((word >> (pos & 7)) & mask);
}

/*
 * 0x259a1
 *
 * The quadtree walk again, but for a whole *screen* rather than a bitmap: the
 * same four-bit code, the same halves, the same reader record at DGROUP 0x640c,
 * and `fill_screen_quadrant` where `vqt_node` has its own leaf. The original
 * has the two written out separately rather than sharing one, and the port
 * keeps them apart for the same reason - they are two routines at two
 * addresses.
 *
 * One thing is not symmetric and is not a slip in the reading: **only the first
 * quadrant redraws the cursor** after its fill. The other three do not. That
 * keeps the pointer on top while a screen paints itself in without paying for a
 * redraw at every leaf.
 */
void near vqt_screen_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    struct vqt_reader *rd;
    uint16_t code;
    uint32_t pos;

    if ((w | h) == 0)
        return;

    rd = BITMAPS.walk;
    pos = (uint32_t)rd->pos;
    rd->pos = (int32_t)(pos + 4);

    /* Four bits at `pos`, read as a word so a nibble can straddle a byte. */
    code = (uint16_t)(((uint16_t)(rd->data[pos >> 3]
                                  | (rd->data[(pos >> 3) + 1] << 8))
                       >> (pos & 7)) & 0x0f);

    if (code & 8) {
        vqt_screen_node(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
    } else {
        fill_screen_quadrant(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
        redraw_cursor(VMDS.page_front);
    }

    if (code & 4)
        vqt_screen_node((uint16_t)(x + (w >> 1)), y,
                        (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));
    else
        fill_screen_quadrant((uint16_t)(x + (w >> 1)), y,
                             (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));

    if (code & 2)
        vqt_screen_node(x, (uint16_t)(y + (h >> 1)),
                        (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_screen_quadrant(x, (uint16_t)(y + (h >> 1)),
                             (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));

    if (code & 1)
        vqt_screen_node((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                        (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_screen_quadrant((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                             (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
}

/*
 * 0x25aaa
 *
 * **The screen quadtree's leaf: paint one rectangle straight onto the page**
 * from what the bit stream says next. `fill_quadrant`'s arithmetic, byte for
 * byte, with a different destination: not a plane buffer but video memory,
 * one plane to a pixel.
 *
 * A pixel at x goes into the byte `VMDS.row_offset[y] + (x >> 2)` of the page
 * `VMDS.page_dst`, and the plane is chosen for each write with the
 * Sequencer's map mask - `mov ax,0x102 / shl ah,cl / out dx,ax` with CL the
 * low two bits of x, so plane `1 << (x & 3)`. The three places that write a
 * pixel each spell that out, and so does this.
 *
 * The shape is `fill_quadrant`'s: either dimension 0 paints nothing; a 1 by 1
 * leaf is one 8-bit read written; otherwise an 8-bit `area` of the low bytes,
 * a bit count of at least 1, a palette size stepped as a byte, and the same
 * unsigned 16-bit test choosing raw 8-bit pixels (x outer, y inner) or a
 * palette. The reads are `vqt_read_bits` written out in place, as there.
 *
 * Two things differ. A **one-colour** palette fills each row with one far call
 * through DGROUP 0x436e, `VM_DRIVER.entry[10]`, the driver's span fill at
 * VGA:0x034f - registers AX the colour in both halves, BX x, CX w, ES:DI the
 * row - stepping DI by 0x50 a row. `vm_init` is the only writer of that table,
 * so this calls `vm_span` directly. And the **palette loop's x test is
 * unsigned**, `jae` at 0x25d89, where every other loop here is signed.
 *
 * The early exits jump to the epilogue before the entry, at 0x25aa2. Reached
 * only through a "BMP:VQT:" chunk, and the game ships none - see the count
 * beside `draw_offset_bitmap` at 0x24e9a, and `vqt_screen_node`, its only
 * caller. Nothing has run this transcription.
 */
void near fill_screen_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint8_t palette[0x100];       /* [bp-0x10a] */
    uint16_t area;                /* [bp-6] */
    uint16_t bits;                /* cx */
    uint16_t n;                   /* [bp-4], stepped and counted as a byte */
    uint16_t index_bits;          /* [bp-2] */
    int16_t x1, y1;               /* [bp-8], [bp-0xa] */
    int16_t xi, yi;               /* di, si */
    uint16_t count, i, row, rows;
    uint16_t at;
    uint8_t colour, al;

    if (h == 0)
        return;
    if (w == 0)
        return;

    if (w == 1 && h == 1) {
        colour = (uint8_t)vqt_read_bits(8);
        at = (uint16_t)(VMDS.row_offset[y] + (x >> 2));
        io_out16(PORT_SEQ_INDEX,
                 (uint16_t)(((uint16_t)(uint8_t)(1 << (x & 3)) << 8) | 0x02));
        vga_write((uint16_t)(vga_seg_offset(VMDS.page_dst) + at), colour);
        return;
    }

    area = (uint16_t)((uint8_t)w * (uint8_t)h);     /* `mul bl` */

    bits = 8;
    if ((area >> 8) == 0) {
        bits = 0;
        al = (uint8_t)((uint8_t)area - 1);
        do {
            bits++;
            al >>= 1;
        } while (al != 0);
    }

    n = vqt_read_bits(bits);

    index_bits = 0;
    al = (uint8_t)n;
    while (al != 0) {
        index_bits++;
        al >>= 1;
    }

    xi = (int16_t)x;
    x1 = (int16_t)(x + w);
    yi = (int16_t)y;
    y1 = (int16_t)(y + h);

    n = (uint16_t)((n & 0xff00) | (uint8_t)(n + 1));   /* `inc byte ptr [bp-4]` */

    if ((uint16_t)(area << 3)
        <= (uint16_t)(area * index_bits + (uint16_t)(n << 3))) {
        do {
            do {
                colour = (uint8_t)vqt_read_bits(8);
                at = (uint16_t)(VMDS.row_offset[(uint16_t)yi]
                                + ((uint16_t)xi >> 2));
                io_out16(PORT_SEQ_INDEX,
                         (uint16_t)(((uint16_t)(uint8_t)(1 << (xi & 3)) << 8)
                                    | 0x02));
                vga_write((uint16_t)(vga_seg_offset(VMDS.page_dst) + at),
                          colour);
                yi++;
            } while (yi < y1);
            yi = (int16_t)y;
            xi++;
        } while (xi < x1);
        return;
    }

    if ((uint8_t)n == 1) {
        colour = (uint8_t)vqt_read_bits(8);
        row = VMDS.row_offset[y];                   /* di */
        rows = h;                                     /* si */
        do {
            vm_span((uint16_t)((colour << 8) | colour), x, (int16_t)w,
                    MK_FP((uint16_t)VMDS.page_dst, row));
            row = (uint16_t)(row + 0x50);
        } while (--rows != 0);
        return;
    }

    count = (uint8_t)n;
    i = 0;
    do {
        palette[i++] = (uint8_t)vqt_read_bits(8);
        count = (uint8_t)(count - 1);
    } while (count != 0);                             /* `dec byte ptr [bp-4]` */

    xi = (int16_t)x;
    do {
        do {
            colour = palette[vqt_read_bits(index_bits)];
            at = (uint16_t)(VMDS.row_offset[(uint16_t)yi]
                            + ((uint16_t)xi >> 2));
            io_out16(PORT_SEQ_INDEX,
                     (uint16_t)(((uint16_t)(uint8_t)(1 << (xi & 3)) << 8)
                                | 0x02));
            vga_write((uint16_t)(vga_seg_offset(VMDS.page_dst) + at),
                      colour);
            yi++;
        } while (yi < y1);
        yi = (int16_t)y;
        xi++;
    } while ((uint16_t)xi < (uint16_t)x1);            /* `jae`, unsigned */
}

/*
 * 0x25d96
 *
 * A far block move, destination first: words then a trailing byte, the odd
 * count carried out of `shr cx,1` in the carry flag.
 *
 * This is the third routine in the port that does this - `far_memcpy` at
 * 0x222c6 and `far_move` at 0x0bd2e are the others - and all three differ in
 * argument order or in which register holds the count. They are separate
 * routines in the original and stay separate here.
 *
 * **Both ends are `far` pointers and the 64K wrap is given up.** `rep movsw`
 * steps SI and DI as 16-bit registers, so both ends wrap inside their segment
 * on the original; a host pointer cannot, and the `far` tag says which kind
 * this is without giving the host that behaviour. The destination held out
 * longest, as a `seg:off` pair with `(uint16_t)(dst_off + i)` doing the wrap
 * explicitly.
 *
 * What makes giving it up sound is the same measurement that settled it for
 * `far_move` and `far_memcpy`: instrumented on 2026-09-09 across the intro to
 * flip 200 and a full level14 run, neither end ever reached
 * `off + count > 0x10000`. For either to wrap it would have to start above
 * 0xC000 in DGROUP with a large count, and the copy would then run into
 * DGROUP from offset 0, which is a bug rather than a behaviour.
 *
 * Under Borland the tag brings the wrap back, because there `far` pointer
 * arithmetic *is* 16-bit offset arithmetic - so the source says the right
 * thing for both compilers and only the host gives the behaviour up.
 */
void near far_copy(uint8_t far *dst, const uint8_t far *src,
              uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        dst[i] = src[i];
}

/*
 * 0x25db8
 *
 * One node of the quadtree the "BMP:VQT:" chunk is: read four bits, and for
 * each quadrant either recurse into this again or hand it to `fill_quadrant` to
 * be painted.
 *
 * The bit reader is the record at DGROUP 0x640c: a 32-bit bit position at +0
 * and the data as a far pointer at +4. Four bits are taken and the position
 * advanced, the byte is found by shifting the position right three, a *word* is
 * read from there and shifted down by the position's low three bits - reading a
 * word rather than a byte is what lets a code straddle a byte boundary without
 * any special case.
 *
 * The four halves are `w >> 1` and `(w + 1) >> 1`, so an odd width puts the
 * extra column in the right-hand pair and an odd height the extra row in the
 * bottom pair. Bit 8 of the code is the top-left quadrant, then 4, 2 and 1
 * clockwise - and a set bit means subdivide, a clear one means fill.
 *
 * A zero width *and* height ends the recursion; either alone does not.
 *
 * A **** routine, and it shares the epilogue three bytes above its own
 * entry for the early return.
 */
void near vqt_node(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    struct vqt_reader *rd;
    uint16_t code;
    uint32_t pos;

    if ((w | h) == 0)
        return;

    rd = BITMAPS.walk;
    pos = (uint32_t)rd->pos;
    rd->pos = (int32_t)(pos + 4);

    /* Four bits at `pos`, read as a word so a nibble can straddle a byte. The
       offset is stepped inside the segment, which is why `data` stays a pair. */
    code = (uint16_t)(((uint16_t)(rd->data[pos >> 3]
                                  | (rd->data[(pos >> 3) + 1] << 8))
                       >> (pos & 7)) & 0x0f);

    if (code & 8)
        vqt_node(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));
    else
        fill_quadrant(x, y, (uint16_t)(w >> 1), (uint16_t)(h >> 1));

    if (code & 4)
        vqt_node((uint16_t)(x + (w >> 1)), y,
                 (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));
    else
        fill_quadrant((uint16_t)(x + (w >> 1)), y,
                      (uint16_t)((w + 1) >> 1), (uint16_t)(h >> 1));

    if (code & 2)
        vqt_node(x, (uint16_t)(y + (h >> 1)),
                 (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_quadrant(x, (uint16_t)(y + (h >> 1)),
                      (uint16_t)(w >> 1), (uint16_t)((h + 1) >> 1));

    if (code & 1)
        vqt_node((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                 (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
    else
        fill_quadrant((uint16_t)(x + (w >> 1)), (uint16_t)(y + (h >> 1)),
                      (uint16_t)((w + 1) >> 1), (uint16_t)((h + 1) >> 1));
}

/*
 * 0x25eb5
 *
 * **The quadtree's leaf: paint one rectangle of the bitmap** from what the bit
 * stream says next. Every pixel goes into the first plane of the reader record
 * `BITMAPS.walk` names, at `plane[0] + row[y] + x` - a 16-bit offset
 * inside the plane's segment - and the other three planes are not touched.
 *
 * Either dimension 0 paints nothing; a 1 by 1 leaf is one byte read and
 * written. Anything larger starts with a palette size:
 *
 *   - `area` is `mul bl`, the **low bytes** of w and h multiplied, an 8-bit
 *     product;
 *   - `bits` is 8 for an area of 256 or more, and otherwise the bit length of
 *     `area - 1` **but at least 1** - this `dec`/`shr` loop has no `je` before
 *     it, where 0x24c55's has;
 *   - `n` is read with `bits`, `index_bits` is the bit length of `n`, and then
 *     `n` is stepped as a **byte**, `inc byte ptr [bp-4]`, so 255 becomes 0;
 *   - if `area * 8` is above `area * index_bits + n * 8`, unsigned 16-bit, a
 *     palette pays. Otherwise every pixel is 8 bits raw, x outer and y inner.
 *
 * A palette of one colour paints the rectangle with it, rows counted down on
 * `h` and each row a `loop` over `w`. A larger one is read into the frame -
 * `[bp-0x10a]`, 0x100 bytes, counted down on the byte `n`, so an `n` that
 * wrapped to 0 reads all 256 - and each pixel is an `index_bits` index into it.
 * The table's address never leaves the routine, so it is a C array.
 *
 * **The reads are 0x25953 written out in place.** The instructions at
 * 0x25f58..0x25f9a and 0x26112..0x26155 are `vqt_read_bits`'s, byte for byte,
 * but for loading the count from CX or `[bp-2]` rather than from its argument,
 * so they are called as it here. The 8-bit reads - 0x25eda, 0x25fe2, 0x26058,
 * 0x260cd - step the position the same way and keep AL without the mask,
 * which is the same byte.
 *
 * The early exits jump to an epilogue **before** the entry, at 0x25ead, and
 * the last one to an epilogue at 0x26190..0x26197. So the routine is 739 bytes,
 * not the 1,853 this comment once gave - and its last eight bytes lie past the
 * 0x26190 this file's header gives as the end of the segment.
 *
 * **Reached only through a "BMP:VQT:" chunk, and the game ships none.** See the
 * count beside `draw_offset_bitmap` at 0x24e9a: zero of the 162 extracted
 * resources carry VQT:, so `vqt_node`, its only caller, is never entered by
 * this data. Nothing has run this transcription.
 */
void near fill_quadrant(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint8_t palette[0x100];       /* [bp-0x10a] */
    struct vqt_reader *rd;
    uint16_t area;                /* [bp-6] */
    uint16_t bits;                /* cx */
    uint16_t n;                   /* [bp-4], stepped and counted as a byte */
    uint16_t index_bits;          /* [bp-2] */
    int16_t x1, y1;               /* [bp-8], [bp-0xa] */
    int16_t xi, yi;               /* di, si */
    uint16_t count, i;            /* cx in the one-colour rows */
    uint8_t colour, al;

    if (h == 0)
        return;
    if (w == 0)
        return;

    if (w == 1 && h == 1) {
        colour = (uint8_t)vqt_read_bits(8);
        rd = BITMAPS.walk;
        rd->plane[0][(uint16_t)rd->row[y] + x] = colour;
        return;
    }

    area = (uint16_t)((uint8_t)w * (uint8_t)h);     /* `mul bl` */

    bits = 8;
    if ((area >> 8) == 0) {
        bits = 0;
        al = (uint8_t)((uint8_t)area - 1);
        do {
            bits++;
            al >>= 1;
        } while (al != 0);
    }

    n = vqt_read_bits(bits);

    index_bits = 0;
    al = (uint8_t)n;
    while (al != 0) {
        index_bits++;
        al >>= 1;
    }

    xi = (int16_t)x;
    x1 = (int16_t)(x + w);
    yi = (int16_t)y;
    y1 = (int16_t)(y + h);

    n = (uint16_t)((n & 0xff00) | (uint8_t)(n + 1));   /* `inc byte ptr [bp-4]` */

    if ((uint16_t)(area << 3)
        <= (uint16_t)(area * index_bits + (uint16_t)(n << 3))) {
        do {
            do {
                colour = (uint8_t)vqt_read_bits(8);
                rd = BITMAPS.walk;
                rd->plane[0][(uint16_t)rd->row[yi]
                                         + (uint16_t)xi] = colour;
                yi++;
            } while (yi < y1);
            yi = (int16_t)y;
            xi++;
        } while (xi < x1);
        return;
    }

    if ((uint8_t)n == 1) {
        colour = (uint8_t)vqt_read_bits(8);
        yi = (int16_t)y;                              /* dx */
        do {
            xi = (int16_t)x;
            count = w;
            do {
                rd = BITMAPS.walk;
                rd->plane[0][(uint16_t)rd->row[yi]
                                         + (uint16_t)xi] = colour;
                xi++;
            } while (--count != 0);                   /* `loop` */
            yi++;
        } while (--h != 0);
        return;
    }

    count = (uint8_t)n;
    i = 0;
    do {
        palette[i++] = (uint8_t)vqt_read_bits(8);
        count = (uint8_t)(count - 1);
    } while (count != 0);                             /* `dec byte ptr [bp-4]` */

    xi = (int16_t)x;
    do {
        do {
            colour = palette[vqt_read_bits(index_bits)];
            rd = BITMAPS.walk;
            rd->plane[0][(uint16_t)rd->row[yi]
                                     + (uint16_t)xi] = colour;
            yi++;
        } while (yi < y1);
        yi = (int16_t)y;
        xi++;
    } while (xi < x1);
}
#endif
