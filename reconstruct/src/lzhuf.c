/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **LZHUF** - Okumura and Yoshizaki's LZSS with adaptive Huffman coding,
 * type 3 of the resource handlers: the encoder's trees, the bit reader and
 * writer, the Huffman tree and its update, and the encoder and decoder.
 *
 * One module of the original's **code segment 1c25**, image
 * 0x1dba8..0x1e93c, with LZHUF.C's own data in its own order - `getbuf`,
 * `getlen`, `putbuf`, `putlen`, `p_len`, `p_code`, `d_code`, `d_len` - as
 * `_DATA` 0x3600..0x3886, and its `_BSS` 0x58d2..0x591a. **It is compiler
 * output edited by hand and assembled**: some routines are Borland C as it
 * stands (`InitTree` is BC++ 2.0 `-mm -k-` byte for byte), some keep a
 * frame and a `jmp` to the epilogue, and others are written by hand -
 * `GetBit`'s `cmp si,0`, `add sp,2` cleanups, and `DecodePosition`, whose
 * `ret` is a jump back into its caller. So it is TASM source, the `#ifdef
 * __TURBOC__` block below, with the host's transcription in the `#else`.
 * Its end is where the video driver's interface begins, vmiface.c, whose
 * data starts a paragraph past this module's.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler bc3.00
 */
#include <string.h>

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
_BSS segment word public 'BSS'
d_58d2 db 2 dup (?)
d_58d4 db 2 dup (?)
d_58d6 db 2 dup (?)
d_58d8 db 2 dup (?)
d_58da db 2 dup (?)
d_58dc db 2 dup (?)
d_58de db 2 dup (?)
d_58e0 db 2 dup (?)
d_58e2 db 2 dup (?)
d_58e4 db 2 dup (?)
d_58e6 db 2 dup (?)
d_58e8 db 2 dup (?)
d_58ea db 2 dup (?)
d_58ec db 2 dup (?)
d_58ee db 2 dup (?)
d_58f0 db 2 dup (?)
d_58f2 db 2 dup (?)
d_58f4 db 2 dup (?)
d_58f6 db 2 dup (?)
d_58f8 db 2 dup (?)
d_58fa db 2 dup (?)
d_58fc db 2 dup (?)
d_58fe db 2 dup (?)
d_5900 db 2 dup (?)
d_5902 db 2 dup (?)
d_5904 db 2 dup (?)
d_5906 db 2 dup (?)
d_5908 db 2 dup (?)
d_590a db 2 dup (?)
d_590c db 2 dup (?)
d_590e db 2 dup (?)
d_5910 db 2 dup (?)
d_5912 db 2 dup (?)
d_5914 db 2 dup (?)
d_5916 db 2 dup (?)
d_5918 db 2 dup (?)
_BSS ends

_DATA segment word public 'DATA'
d_3600 label byte
        db 0h, 0h
d_3602 label byte
        db 0h
d_3603 label byte
        db 0h, 0h
d_3605 label byte
        db 0h
d_3606 label byte
        db 3h, 4h, 4h, 4h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 6h, 6h, 6h, 6h
        db 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
        db 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
        db 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h
d_3646 label byte
        db 0h, 20h, 30h, 40h, 50h, 58h, 60h, 68h, 70h, 78h, 80h, 88h, 90h, 94h, 98h, 9ch
        db 0a0h, 0a4h, 0a8h, 0ach, 0b0h, 0b4h, 0b8h, 0bch, 0c0h, 0c2h, 0c4h, 0c6h, 0c8h, 0cah, 0cch, 0ceh
        db 0d0h, 0d2h, 0d4h, 0d6h, 0d8h, 0dah, 0dch, 0deh, 0e0h, 0e2h, 0e4h, 0e6h, 0e8h, 0eah, 0ech, 0eeh
        db 0f0h, 0f1h, 0f2h, 0f3h, 0f4h, 0f5h, 0f6h, 0f7h, 0f8h, 0f9h, 0fah, 0fbh, 0fch, 0fdh, 0feh, 0ffh
d_3686 label byte
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h
        db 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h
        db 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h
        db 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h
        db 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
        db 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 9h, 9h, 9h, 9h, 9h, 9h, 9h, 9h
        db 0ah, 0ah, 0ah, 0ah, 0ah, 0ah, 0ah, 0ah, 0bh, 0bh, 0bh, 0bh, 0bh, 0bh, 0bh, 0bh
        db 0ch, 0ch, 0ch, 0ch, 0dh, 0dh, 0dh, 0dh, 0eh, 0eh, 0eh, 0eh, 0fh, 0fh, 0fh, 0fh
        db 10h, 10h, 10h, 10h, 11h, 11h, 11h, 11h, 12h, 12h, 12h, 12h, 13h, 13h, 13h, 13h
        db 14h, 14h, 14h, 14h, 15h, 15h, 15h, 15h, 16h, 16h, 16h, 16h, 17h, 17h, 17h, 17h
        db 18h, 18h, 19h, 19h, 1ah, 1ah, 1bh, 1bh, 1ch, 1ch, 1dh, 1dh, 1eh, 1eh, 1fh, 1fh
        db 20h, 20h, 21h, 21h, 22h, 22h, 23h, 23h, 24h, 24h, 25h, 25h, 26h, 26h, 27h, 27h
        db 28h, 28h, 29h, 29h, 2ah, 2ah, 2bh, 2bh, 2ch, 2ch, 2dh, 2dh, 2eh, 2eh, 2fh, 2fh
        db 30h, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 3ah, 3bh, 3ch, 3dh, 3eh, 3fh
d_3786 label byte
        db 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h
        db 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h, 3h
        db 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h
        db 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h
        db 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h, 4h
        db 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h
        db 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h
        db 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h
        db 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h
        db 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h
        db 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h
        db 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h
        db 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
        db 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
        db 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
        db 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h
_DATA ends

extrn _dos_alloc_bytes:far
extrn _dos_free_far:far
extrn _ENGINE_STREAM:byte
LZHUF_TEXT segment byte public 'CODE'
assume cs:LZHUF_TEXT, ds:DGROUP
extrn _emit_byte:near
extrn _next_input_byte:near
extrn _put_output_byte:near
public _lzss_open_write, _lzss_reset, _init_tree, _insert_node
public _delete_node, _huff_get_bit, _huff_get_byte, _huff_putcode
public _huffman_start, _huffman_reconst, _huffman_update, _encode_char
public _encode_position, _encode_end, _decode_position, _lzss_flush
public _decompress_lzss

/* 0x1dba8 */
_lzss_open_write proc near
        mov word ptr DGROUP:d_58fe, 0
        mov word ptr DGROUP:d_3603, 0
        mov byte ptr DGROUP:d_3605, 0
        xor ax, ax
        push ax
        push ax
        mov dx, 2002h
        push ax
        push dx
        call FAR PTR _dos_alloc_bytes
        add sp, 8
        mov word ptr DGROUP:d_58f6, dx
        mov word ptr DGROUP:d_58f4, ax
        xor ax, ax
        push ax
        push ax
        mov dx, 2202h
        push ax
        push dx
        call FAR PTR _dos_alloc_bytes
        add sp, 8
        mov word ptr DGROUP:d_58fa, dx
        mov word ptr DGROUP:d_58f8, ax
        xor ax, ax
        push ax
        push ax
        mov dx, 2002h
        push ax
        push dx
        call FAR PTR _dos_alloc_bytes
        add sp, 8
        mov word ptr DGROUP:d_5908, dx
        mov word ptr DGROUP:d_5906, ax
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov ax, word ptr [bx+4]
        mov dx, word ptr [bx+2]
        mov word ptr DGROUP:d_5914, ax
        mov word ptr DGROUP:d_5912, dx
        xor ax, ax
        ret
_lzss_open_write endp

/* 0x1dc15 */
_lzss_reset proc near
        mov word ptr DGROUP:d_5918, 0
        mov word ptr DGROUP:d_3600, 0
        mov byte ptr DGROUP:d_3602, 0
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov ax, word ptr [bx+4]
        mov dx, word ptr [bx+2]
        mov word ptr DGROUP:d_5914, ax
        mov word ptr DGROUP:d_5912, dx
        xor ax, ax
        ret
_lzss_reset endp

/* 0x1dc3a */
_init_tree proc near
        push si
        mov si, 1001h
        jmp short L1dc50
L1dc40:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        mov word ptr es:[bx], 1000h
        inc si
L1dc50:
        cmp si, 1100h
        jle L1dc40
        xor si, si
        jmp short L1dc6a
L1dc5a:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov word ptr es:[bx], 1000h
        inc si
L1dc6a:
        cmp si, 1000h
        jl L1dc5a
        pop si
        ret
_init_tree endp

/* 0x1dc72 */
_insert_node proc near
        push bp
        mov bp, sp
        sub sp, 6
        push si
        push di
        mov cx, 1
        mov ax, word ptr DGROUP:d_5914
        mov dx, word ptr DGROUP:d_5912
        add dx, word ptr [bp+4]
        mov word ptr [bp-2], ax
        mov word ptr [bp-4], dx
        les bx, dword ptr [bp-4]
        mov al, byte ptr es:[bx]
        mov ah, 0
        add ax, 1001h
        mov si, ax
        mov dx, word ptr [bp+4]
        shl dx, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, dx
        mov ax, 1000h
        mov word ptr es:[bx], ax
        les bx, dword ptr DGROUP:d_58f8
        add bx, dx
        mov word ptr es:[bx], ax
        mov word ptr DGROUP:d_58f2, 0
L1dcba:
        or cx, cx
        jl L1dcea
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        cmp word ptr es:[bx], 1000h
        je L1dcd4
        mov si, word ptr es:[bx]
        jmp short L1dce8
L1dcd4:
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov word ptr es:[bx], si
        jmp L1de4b
L1dce8:
        jmp short L1dd14
L1dcea:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        cmp word ptr es:[bx], 1000h
        je L1dd00
        mov si, word ptr es:[bx]
        jmp short L1dd14
L1dd00:
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov word ptr es:[bx], si
        jmp L1de4b
L1dd14:
        mov di, 1
        jmp short L1dd35
L1dd19:
        les bx, dword ptr [bp-4]
        mov cl, byte ptr es:[bx+di]
        mov ch, 0
        les bx, dword ptr DGROUP:d_5912
        add bx, si
        add bx, di
        mov dl, byte ptr es:[bx]
        mov dh, 0
        sub cx, dx
        or cx, cx
        jne L1dd3a
        inc di
L1dd35:
        cmp di, 3ch
        jl L1dd19
L1dd3a:
        cmp di, 2
        jle L1dd7b
        cmp di, word ptr DGROUP:d_58f2
        jle L1dd5a
        mov ax, word ptr [bp+4]
        sub ax, si
        and ax, 0fffh
        dec ax
        mov word ptr DGROUP:d_58fc, ax
        mov word ptr DGROUP:d_58f2, di
        cmp di, 3ch
        jge L1dd7e
L1dd5a:
        cmp di, word ptr DGROUP:d_58f2
        jne L1dd7b
        mov ax, word ptr [bp+4]
        sub ax, si
        and ax, 0fffh
        dec ax
        mov word ptr [bp-6], ax
        cmp ax, word ptr DGROUP:d_58fc
        jb L1dd75
        jmp L1dcba
L1dd75:
        mov ax, word ptr [bp-6]
        mov word ptr DGROUP:d_58fc, ax
L1dd7b:
        jmp L1dcba
L1dd7e:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, word ptr [bp+4]
        shl bx, 1
        add bx, word ptr DGROUP:d_5906
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, word ptr [bp+4]
        shl bx, 1
        add bx, word ptr DGROUP:d_58f4
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, word ptr [bp+4]
        shl bx, 1
        add bx, word ptr DGROUP:d_58f8
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        cmp word ptr es:[bx], si
        jne L1de21
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        jmp short L1de3c
L1de21:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
L1de3c:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov word ptr es:[bx], 1000h
L1de4b:
        pop di
        pop si
        mov sp, bp
        pop bp
        ret
_insert_node endp

/* 0x1de51 */
_delete_node proc near
        push bp
        mov bp, sp
        push si
        push di
        mov di, word ptr [bp+4]
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne L1de6d
        jmp L1dfd2
L1de6d:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne L1de8e
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        mov si, word ptr es:[bx]
        jmp L1df81
L1de8e:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne L1deaf
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        mov si, word ptr es:[bx]
        jmp L1df81
L1deaf:
        mov si, word ptr es:[bx]
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne L1dec6
        jmp L1df51
L1dec6:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
L1ded0:
        mov si, word ptr es:[bx]
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne L1ded0
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, si
        shl dx, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, dx
        mov dx, word ptr es:[bx]
        shl dx, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, si
        shl dx, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, dx
        mov dx, word ptr es:[bx]
        shl dx, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, si
        shl bx, 1
        add bx, word ptr DGROUP:d_58f4
        mov word ptr es:[bx], ax
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov word ptr es:[bx], si
L1df51:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, si
        shl bx, 1
        add bx, word ptr DGROUP:d_58f8
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov word ptr es:[bx], si
L1df81:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, si
        shl bx, 1
        add bx, word ptr DGROUP:d_5906
        mov word ptr es:[bx], ax
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f8
        add bx, ax
        cmp word ptr es:[bx], di
        jne L1dfab
        mov word ptr es:[bx], si
        jmp short L1dfc3
L1dfab:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:d_58f4
        add bx, ax
        mov word ptr es:[bx], si
L1dfc3:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_5906
        add bx, ax
        mov word ptr es:[bx], 1000h
L1dfd2:
        pop di
        pop si
        pop bp
        ret
_delete_node endp

/* 0x1dfd6 */
_huff_get_bit proc near
        push si
        cmp byte ptr DGROUP:d_3602, 8
        ja L1dff4
        call _next_input_byte
        mov ah, 0
        mov cl, 8
        sub cl, byte ptr DGROUP:d_3602
        shl ax, cl
        or word ptr DGROUP:d_3600, ax
        add byte ptr DGROUP:d_3602, 8
L1dff4:
        mov si, word ptr DGROUP:d_3600
        shl word ptr DGROUP:d_3600, 1
        sub byte ptr DGROUP:d_3602, 1
        xor ax, ax
        cmp si, 0
        jge L1e009
        inc ax
L1e009:
        pop si
        ret
_huff_get_bit endp

/* 0x1e00b */
_huff_get_byte proc near
        push bp
        mov bp, sp
        push si
        jmp short L1e02a
L1e011:
        call _next_input_byte
        mov ah, 0
        mov cl, 8
        sub cl, byte ptr DGROUP:d_3602
        shl ax, cl
        or word ptr DGROUP:d_3600, ax
        mov al, byte ptr DGROUP:d_3602
        add al, 8
        mov byte ptr DGROUP:d_3602, al
L1e02a:
        cmp byte ptr DGROUP:d_3602, 8
        jbe L1e011
        mov si, word ptr DGROUP:d_3600
        mov cl, 8
        shl word ptr DGROUP:d_3600, cl
        mov al, byte ptr DGROUP:d_3602
        add al, 0f8h
        mov byte ptr DGROUP:d_3602, al
        mov ax, si
        mov cl, 8
        shr ax, cl
        jmp short L1e04b
L1e04b:
        pop si
        pop bp
        ret
_huff_get_byte endp

/* 0x1e04e */
_huff_putcode proc near
        push bp
        mov bp, sp
        push si
        mov si, word ptr [bp+6]
        mov ax, si
        mov cl, byte ptr DGROUP:d_3605
        shr ax, cl
        or word ptr DGROUP:d_3603, ax
        mov al, byte ptr DGROUP:d_3605
        add al, byte ptr [bp+4]
        mov byte ptr DGROUP:d_3605, al
        cmp al, 8
        jb L1e0b0
        mov ax, word ptr DGROUP:d_3603
        mov cl, 8
        shr ax, cl
        push ax
        call _put_output_byte
        add sp, 2
        mov al, byte ptr DGROUP:d_3605
        add al, 0f8h
        mov byte ptr DGROUP:d_3605, al
        cmp al, 8
        jb L1e0aa
        push word ptr DGROUP:d_3603
        call _put_output_byte
        add sp, 2
        mov al, byte ptr DGROUP:d_3605
        add al, 0f8h
        mov byte ptr DGROUP:d_3605, al
        mov cl, byte ptr [bp+4]
        sub cl, byte ptr DGROUP:d_3605
        mov ax, si
        shl ax, cl
        mov word ptr DGROUP:d_3603, ax
        jmp short L1e0b0
L1e0aa:
        mov cl, 8
        shl word ptr DGROUP:d_3603, cl
L1e0b0:
        pop si
        pop bp
        ret
_huff_putcode endp

/* 0x1e0b3 */
_huffman_start proc near
        push bp
        mov bp, sp
        push si
        push di
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov ax, word ptr [bx+4]
        mov dx, word ptr [bx+2]
        add dx, 103bh
        mov word ptr DGROUP:d_590c, ax
        mov word ptr DGROUP:d_590a, dx
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov ax, word ptr [bx+4]
        mov dx, word ptr [bx+2]
        add dx, 1523h
        mov word ptr DGROUP:d_5910, ax
        mov word ptr DGROUP:d_590e, dx
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov ax, word ptr [bx+4]
        mov dx, word ptr [bx+2]
        add dx, 1c7dh
        mov word ptr DGROUP:d_5902, ax
        mov word ptr DGROUP:d_5900, dx
        xor si, si
        jmp short L1e12d
L1e0fb:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        mov word ptr es:[bx], 1
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov ax, si
        add ax, 273h
        mov word ptr es:[bx], ax
        mov ax, si
        add ax, 273h
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov word ptr es:[bx], si
        inc si
L1e12d:
        cmp si, 13ah
        jl L1e0fb
        xor si, si
        mov di, 13ah
        jmp short L1e18f
L1e13a:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, si
        inc dx
        shl dx, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, dx
        add ax, word ptr es:[bx]
        mov dx, di
        shl dx, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov word ptr es:[bx], si
        mov ax, si
        inc ax
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov ax, di
        mov word ptr es:[bx], ax
        mov dx, si
        shl dx, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, dx
        mov word ptr es:[bx], ax
        inc si
        inc si
        inc di
L1e18f:
        cmp di, 272h
        jle L1e13a
        les bx, dword ptr DGROUP:d_590a
        mov word ptr es:[bx+4e6h], 0ffffh
        les bx, dword ptr DGROUP:d_590e
        mov word ptr es:[bx+4e4h], 0
        pop di
        pop si
        pop bp
        ret
_huffman_start endp

/* 0x1e1af */
_huffman_reconst proc near
        push bp
        mov bp, sp
        sub sp, 6
        push si
        push di
        xor cx, cx
        xor si, si
        jmp short L1e207
L1e1bd:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        cmp word ptr es:[bx], 273h
        jl L1e206
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        mov ax, word ptr es:[bx]
        inc ax
        shr ax, 1
        mov dx, cx
        shl dx, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, cx
        shl dx, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, dx
        mov word ptr es:[bx], ax
        inc cx
L1e206:
        inc si
L1e207:
        cmp si, 273h
        jl L1e1bd
        xor si, si
        mov cx, 13ah
        jmp L1e2dc
L1e215:
        mov ax, si
        inc ax
        mov di, ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, di
        shl dx, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, dx
        add ax, word ptr es:[bx]
        mov dx, cx
        shl dx, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, dx
        mov word ptr es:[bx], ax
        mov word ptr [bp-4], ax
        mov ax, cx
        dec ax
        mov di, ax
        jmp short L1e24c
L1e24b:
        dec di
L1e24c:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        mov ax, word ptr es:[bx]
        cmp ax, word ptr [bp-4]
        ja L1e24b
        inc di
        mov ax, cx
        sub ax, di
        shl ax, 1
        mov word ptr [bp-6], ax
        mov ax, word ptr [bp-6]
        dec ax
        mov word ptr [bp-2], ax
        jmp short L1e2b6
L1e271:
        mov ax, di
        add ax, word ptr [bp-2]
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, di
        add dx, word ptr [bp-2]
        inc dx
        shl dx, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, di
        add ax, word ptr [bp-2]
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, di
        add dx, word ptr [bp-2]
        inc dx
        shl dx, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, dx
        mov word ptr es:[bx], ax
        dec word ptr [bp-2]
L1e2b6:
        cmp word ptr [bp-2], 0
        jge L1e271
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        mov ax, word ptr [bp-4]
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov word ptr es:[bx], si
        inc si
        inc si
        inc cx
L1e2dc:
        cmp cx, 273h
        jge L1e2e5
        jmp L1e215
L1e2e5:
        xor si, si
        jmp short L1e32a
L1e2e9:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov ax, word ptr es:[bx]
        mov di, ax
        cmp ax, 273h
        jl L1e30c
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov word ptr es:[bx], si
        jmp short L1e329
L1e30c:
        mov ax, di
        inc ax
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov ax, si
        mov word ptr es:[bx], ax
        mov dx, di
        shl dx, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, dx
        mov word ptr es:[bx], ax
L1e329:
        inc si
L1e32a:
        cmp si, 273h
        jl L1e2e9
        pop di
        pop si
        mov sp, bp
        pop bp
        jmp short L1e350
c_1e337 db 90h
_huffman_reconst endp

/* 0x1e338 */
_huffman_update proc near
        push bp
        mov bp, sp
        sub sp, 4
        push si
        push di
        les bx, dword ptr DGROUP:d_590a
        cmp word ptr es:[bx+4e4h], 8000h
        jne L1e350
        jmp _huffman_reconst
L1e350:
        mov ax, word ptr [bp+4]
        add ax, 273h
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp+4], ax
L1e364:
        mov ax, word ptr [bp+4]
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        inc word ptr es:[bx]
        mov ax, word ptr es:[bx]
        mov word ptr [bp-2], ax
        mov si, word ptr [bp+4]
        inc si
        add bx, 2
        mov ax, word ptr es:[bx]
        cmp ax, word ptr [bp-2]
        jb L1e38a
        jmp L1e427
L1e38a:
        inc si
        add bx, 2
        mov ax, word ptr es:[bx]
        cmp ax, word ptr [bp-2]
        jb L1e38a
        dec si
        sub bx, 2
        mov ax, word ptr es:[bx]
        mov dx, word ptr [bp+4]
        shl dx, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_590a
        add bx, ax
        mov ax, word ptr [bp-2]
        mov word ptr es:[bx], ax
        mov ax, word ptr [bp+4]
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp-4], ax
        mov ax, word ptr [bp-4]
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov word ptr es:[bx], si
        cmp word ptr [bp-4], 273h
        jge L1e3e7
        add bx, 2
        mov word ptr es:[bx], si
L1e3e7:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov di, word ptr es:[bx]
        mov ax, word ptr [bp-4]
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        cmp di, 273h
        jge L1e416
        add bx, 2
        mov word ptr es:[bx], ax
L1e416:
        mov ax, word ptr [bp+4]
        shl ax, 1
        les bx, dword ptr DGROUP:d_5900
        add bx, ax
        mov word ptr es:[bx], di
        mov word ptr [bp+4], si
L1e427:
        mov ax, word ptr [bp+4]
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp+4], ax
        or ax, ax
        je L1e43f
        jmp L1e364
L1e43f:
        pop di
        pop si
        mov sp, bp
        pop bp
        ret
_huffman_update endp

/* 0x1e445 */
_encode_char proc near
        push bp
        mov bp, sp
        dec sp
        dec sp
        push si
        push di
        xor si, si
        mov ax, word ptr [bp+4]
        add ax, 273h
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp-2], ax
        xor di, di
L1e464:
        shr si, 1
        test word ptr [bp-2], 1
        je L1e471
        add si, 8000h
L1e471:
        inc di
        mov ax, word ptr [bp-2]
        shl ax, 1
        les bx, dword ptr DGROUP:d_590e
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp-2], ax
        cmp ax, 272h
        jne L1e464
        push si
        push di
        call _huff_putcode
        add sp, 4
        mov word ptr DGROUP:d_5916, si
        mov word ptr DGROUP:d_5904, di
        push word ptr [bp+4]
        call _huffman_update
        add sp, 2
        pop di
        pop si
        mov sp, bp
        pop bp
        ret
_encode_char endp

/* 0x1e4a7 */
_encode_position proc near
        push bp
        mov bp, sp
        push si
        push di
        mov di, word ptr [bp+4]
        mov ax, di
        mov cl, 6
        shr ax, cl
        mov si, ax
        mov al, byte ptr d_3646[si]
        mov ah, 0
        mov cl, 8
        shl ax, cl
        push ax
        mov al, byte ptr d_3606[si]
        mov ah, 0
        push ax
        call _huff_putcode
        add sp, 4
        mov ax, di
        and ax, 3fh
        mov cl, 0ah
        shl ax, cl
        push ax
        mov ax, 6
        push ax
        call _huff_putcode
        add sp, 4
        pop di
        pop si
        pop bp
        ret
_encode_position endp

/* 0x1e4e7 */
_encode_end proc near
        cmp byte ptr DGROUP:d_3605, 0
        je L1e4fc
        mov ax, word ptr DGROUP:d_3603
        mov cl, 8
        shr ax, cl
        push ax
        call _put_output_byte
        add sp, 2
L1e4fc:
        push word ptr DGROUP:d_58f6
        push word ptr DGROUP:d_58f4
        call FAR PTR _dos_free_far
        add sp, 4
        push word ptr DGROUP:d_58fa
        push word ptr DGROUP:d_58f8
        call FAR PTR _dos_free_far
        add sp, 4
        push word ptr DGROUP:d_5908
        push word ptr DGROUP:d_5906
        call FAR PTR _dos_free_far
        add sp, 4
        ret
L1e52d:
        les bx, dword ptr DGROUP:d_5900
        mov di, word ptr es:[bx+4e4h]
        jmp short L1e54d
c_1e538 db 0a1h, 2h, 59h
c_1e53b db 8eh, 0c0h
L1e53d:
        call _huff_get_bit
        mov bx, ax
        add bx, di
        shl bx, 1
        add bx, word ptr DGROUP:d_5900
        mov di, word ptr es:[bx]
L1e54d:
        cmp di, 273h
        jb L1e53d
        sub di, 273h
        push di
        call _huffman_update
        add sp, 2
        jmp L1e849
_encode_end endp

/* 0x1e561 */
_decode_position proc near
        push bp
        mov bp, sp
        dec sp
        dec sp
        push si
        push di
        call _huff_get_byte
        mov si, ax
        mov al, byte ptr d_3686[si]
        mov ah, 0
        mov cl, 6
        shl ax, cl
        mov word ptr [bp-2], ax
        mov al, byte ptr d_3786[si]
        mov ah, 0
        mov di, ax
        dec di
        dec di
        jmp short L1e591
L1e586:
        call _huff_get_bit
        mov dx, si
        shl dx, 1
        add dx, ax
        mov si, dx
L1e591:
        mov ax, di
        dec di
        or ax, ax
        jne L1e586
        mov ax, si
        and ax, 3fh
        push ax
        mov ax, word ptr [bp-2]
        pop dx
        or ax, dx
        jmp short L1e5a6
L1e5a6:
        pop di
        pop si
        mov sp, bp
        pop bp
        jmp L1e89c
_decode_position endp

/* 0x1e5ae */
_lzss_flush proc near
        push bp
        mov bp, sp
        sub sp, 4
        push si
        push di
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov al, byte ptr [bx+1bh]
        mov ah, 0
        mov word ptr [bp-2], ax
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov al, byte ptr [bx+1ah]
        mov ah, 0
        mov di, ax
        mov si, word ptr DGROUP:_ENGINE_STREAM+0ah
        mov ax, word ptr [bp-2]
        mov word ptr [bp-4], ax
        cmp word ptr DGROUP:d_58fe, 0
        je L1e5e1
        jmp L1e69b
L1e5e1:
        cmp word ptr DGROUP:d_58d2, 0
        jne L1e617
        call _huffman_start
        call _init_tree
        mov word ptr DGROUP:d_58dc, 0
        mov word ptr DGROUP:d_58da, 0fc4h
        push di
        les di, dword ptr DGROUP:d_5912
        add di, word ptr DGROUP:d_58d4
        mov cx, word ptr DGROUP:d_58da
        sub cx, word ptr DGROUP:d_58d4
        mov al, 20h
        cld
        rep stosb
        pop di
        mov word ptr DGROUP:d_58d8, 0
L1e617:
        mov word ptr DGROUP:d_58d2, 0
        jmp short L1e63e
L1e61f:
        mov bx, word ptr [bp-4]
        mov al, byte ptr [bx+si]
        les bx, dword ptr DGROUP:d_5912
        add bx, word ptr DGROUP:d_58da
        add bx, word ptr DGROUP:d_58d8
        mov byte ptr es:[bx], al
        inc word ptr [bp-4]
        and word ptr [bp-4], 7fh
        inc word ptr DGROUP:d_58d8
L1e63e:
        cmp word ptr DGROUP:d_58d8, 3ch
        jge L1e64a
        cmp word ptr [bp-4], di
        jne L1e61f
L1e64a:
        cmp word ptr [bp-4], di
        jne L1e66a
        cmp word ptr [bp+4], 0
        jne L1e66a
        mov word ptr DGROUP:d_58d2, 1
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov al, byte ptr [bp-4]
        mov byte ptr [bx+1bh], al
        xor ax, ax
        jmp L1e7ec
L1e66a:
        mov word ptr DGROUP:d_58d4, 1
        jmp short L1e684
L1e672:
        mov ax, word ptr DGROUP:d_58da
        sub ax, word ptr DGROUP:d_58d4
        push ax
        call _insert_node
        add sp, 2
        inc word ptr DGROUP:d_58d4
L1e684:
        cmp word ptr DGROUP:d_58d4, 3ch
        jle L1e672
        push word ptr DGROUP:d_58da
        call _insert_node
        add sp, 2
        mov word ptr DGROUP:d_58fe, 1
L1e69b:
        cmp word ptr DGROUP:d_58d2, 0
        jne L1e6f7
        mov ax, word ptr DGROUP:d_58f2
        cmp ax, word ptr DGROUP:d_58d8
        jle L1e6b1
        mov ax, word ptr DGROUP:d_58d8
        mov word ptr DGROUP:d_58f2, ax
L1e6b1:
        cmp word ptr DGROUP:d_58f2, 2
        jg L1e6d4
        mov word ptr DGROUP:d_58f2, 1
        les bx, dword ptr DGROUP:d_5912
        add bx, word ptr DGROUP:d_58da
        mov al, byte ptr es:[bx]
        mov ah, 0
        push ax
        call _encode_char
        add sp, 2
        jmp short L1e6eb
L1e6d4:
        mov ax, word ptr DGROUP:d_58f2
        add ax, 0fdh
        push ax
        call _encode_char
        add sp, 2
        push word ptr DGROUP:d_58fc
        call _encode_position
        add sp, 2
L1e6eb:
        mov ax, word ptr DGROUP:d_58f2
        mov word ptr DGROUP:d_58de, ax
        mov word ptr DGROUP:d_58d4, 0
L1e6f7:
        mov word ptr DGROUP:d_58d2, 0
        jmp short L1e765
L1e6ff:
        mov bx, word ptr [bp-4]
        mov al, byte ptr [bx+si]
        mov ah, 0
        mov word ptr DGROUP:d_58d6, ax
        inc word ptr [bp-4]
        and word ptr [bp-4], 7fh
        push word ptr DGROUP:d_58dc
        call _delete_node
        add sp, 2
        les bx, dword ptr DGROUP:d_5912
        add bx, word ptr DGROUP:d_58dc
        mov al, byte ptr DGROUP:d_58d6
        mov byte ptr es:[bx], al
        cmp word ptr DGROUP:d_58dc, 3bh
        jge L1e743
        mov bx, word ptr DGROUP:d_58dc
        mov es, word ptr DGROUP:d_5914
        add bx, word ptr DGROUP:d_5912
        mov al, byte ptr DGROUP:d_58d6
        mov byte ptr es:[bx+1000h], al
L1e743:
        inc word ptr DGROUP:d_58dc
        and word ptr DGROUP:d_58dc, 0fffh
        inc word ptr DGROUP:d_58da
        and word ptr DGROUP:d_58da, 0fffh
        push word ptr DGROUP:d_58da
        call _insert_node
        add sp, 2
        inc word ptr DGROUP:d_58d4
L1e765:
        mov ax, word ptr DGROUP:d_58d4
        cmp ax, word ptr DGROUP:d_58de
        jge L1e775
        cmp word ptr [bp-4], di
        je L1e775
        jmp short L1e6ff
L1e775:
        cmp word ptr [bp-4], di
        jne L1e7c4
        cmp word ptr [bp+4], 0
        jne L1e7c4
        mov word ptr DGROUP:d_58d2, 1
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov al, byte ptr [bp-4]
        mov byte ptr [bx+1bh], al
        xor ax, ax
        jmp short L1e7ec
c_1e794 db 0ebh, 2eh
L1e796:
        push word ptr DGROUP:d_58dc
        call _delete_node
        add sp, 2
        mov ax, word ptr DGROUP:d_58dc
        inc ax
        and ax, 0fffh
        mov word ptr DGROUP:d_58dc, ax
        mov ax, word ptr DGROUP:d_58da
        inc ax
        and ax, 0fffh
        mov word ptr DGROUP:d_58da, ax
        dec word ptr DGROUP:d_58d8
        je L1e7c4
        push word ptr DGROUP:d_58da
        call _insert_node
        add sp, 2
L1e7c4:
        mov ax, word ptr DGROUP:d_58d4
        inc word ptr DGROUP:d_58d4
        cmp ax, word ptr DGROUP:d_58de
        jl L1e796
        cmp word ptr DGROUP:d_58d8, 0
        jle L1e7db
        jmp L1e69b
L1e7db:
        call _encode_end
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov al, byte ptr [bp-4]
        mov byte ptr [bx+1bh], al
        xor ax, ax
        jmp short L1e7ec
L1e7ec:
        pop di
        pop si
        mov sp, bp
        pop bp
        ret
_lzss_flush endp

/* 0x1e7f2 */
_decompress_lzss proc near
        push si
        push di
        cmp word ptr DGROUP:d_5918, 0
        je L1e7fe
        jmp L1e91d
L1e7fe:
        mov word ptr DGROUP:d_58e0, 0
        call _huffman_start
        les di, dword ptr DGROUP:d_5912
        mov cx, 0fc4h
        mov al, 20h
        cld
        rep stosb
        mov word ptr DGROUP:d_58e8, 0fc4h
        mov word ptr DGROUP:d_58ec, 0
        mov word ptr DGROUP:d_58ea, 0
        mov bx, word ptr DGROUP:_ENGINE_STREAM+2h
        mov ax, word ptr [bx+14h]
        mov dx, word ptr [bx+12h]
        mov word ptr DGROUP:d_58f0, ax
        mov word ptr DGROUP:d_58ee, dx
        mov word ptr DGROUP:d_5918, 1
        jmp L1e91d
L1e83f:
        cmp word ptr DGROUP:d_58e0, 0
        jne L1e882
        jmp L1e52d
L1e849:
        cmp di, 100h
        jge L1e882
        push di
        call _emit_byte
        add sp, 2
        mov si, ax
        les bx, dword ptr DGROUP:d_5912
        add bx, word ptr DGROUP:d_58e8
        mov ax, di
        mov byte ptr es:[bx], al
        inc word ptr DGROUP:d_58e8
        and word ptr DGROUP:d_58e8, 0fffh
        add word ptr DGROUP:d_58ea, 1
        adc word ptr DGROUP:d_58ec, 0
        or si, si
        jne L1e882
        xor ax, ax
        jmp L1e939
L1e882:
        cmp di, 100h
        jge L1e892
        cmp word ptr DGROUP:d_58e0, 0
        jne L1e892
        jmp L1e91d
L1e892:
        cmp word ptr DGROUP:d_58e0, 0
        jne L1e8b9
        jmp _decode_position
L1e89c:
        mov dx, word ptr DGROUP:d_58e8
        sub dx, ax
        dec dx
        and dx, 0fffh
        mov word ptr DGROUP:d_58e2, dx
        mov ax, di
        add ax, 0ff03h
        mov word ptr DGROUP:d_58e4, ax
        mov word ptr DGROUP:d_58e6, 0
L1e8b9:
        mov word ptr DGROUP:d_58e0, 0
        jmp short L1e914
L1e8c1:
        mov ax, word ptr DGROUP:d_58e2
        add ax, word ptr DGROUP:d_58e6
        and ax, 0fffh
        les bx, dword ptr DGROUP:d_5912
        add bx, ax
        mov al, byte ptr es:[bx]
        mov ah, 0
        mov di, ax
        push di
        call _emit_byte
        add sp, 2
        mov si, ax
        les bx, dword ptr DGROUP:d_5912
        add bx, word ptr DGROUP:d_58e8
        mov ax, di
        mov byte ptr es:[bx], al
        inc word ptr DGROUP:d_58e8
        and word ptr DGROUP:d_58e8, 0fffh
        add word ptr DGROUP:d_58ea, 1
        adc word ptr DGROUP:d_58ec, 0
        inc word ptr DGROUP:d_58e6
        or si, si
        jne L1e914
        mov word ptr DGROUP:d_58e0, 1
        xor ax, ax
        jmp short L1e939
L1e914:
        mov ax, word ptr DGROUP:d_58e6
        cmp ax, word ptr DGROUP:d_58e4
        jl L1e8c1
L1e91d:
        mov ax, word ptr DGROUP:d_58ec
        cmp ax, word ptr DGROUP:d_58f0
        jge L1e929
        jmp L1e83f
L1e929:
        jne L1e937
        mov ax, word ptr DGROUP:d_58ea
        cmp ax, word ptr DGROUP:d_58ee
        jae L1e937
        jmp L1e83f
L1e937:
        xor ax, ax
L1e939:
        pop di
        pop si
        ret
_decompress_lzss endp

LZHUF_TEXT ends
}
#else

/*
 * **The bit buffer the decompressors read through**, DGROUP 0x3600..0x3603, 0x03 bytes.
 */
struct engine_bit_buffer {
    int16_t   bits;               /* +0x00 [2]  filled from the top; bits come off the **left** */
    uint8_t   bit_count;          /* +0x02 [1]  how many are in it */
} PACKED;

struct engine_bit_buffer ENGINE_BIT_BUFFER;

/*
 * **The Huffman coder's position tables**, DGROUP 0x3603..0x3686: three zero
 * bytes, then sixty-four code lengths and sixty-four codes - the shape of
 * LZHUF's `p_len` and `p_code`, which are the *encoder's* half, next to the
 * decoder's `ENGINE_HUFFMAN_POSITIONS`. **Not established** that anything
 * reads them.
 */
struct engine_huffman_codes {
    uint8_t   pad_3603[3];        /* +0x00 */
    uint8_t   len[64];            /* +0x03  0x3606 */
    uint8_t   code[64];           /* +0x43  0x3646 */
} PACKED;

struct engine_huffman_codes ENGINE_HUFFMAN_CODES = {
    .len = {
        0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    },
    .code = {
        0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80,
        0x88, 0x90, 0x94, 0x98, 0x9c, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4,
        0xb8, 0xbc, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0,
        0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6,
        0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
        0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    },
};


/*
 * **The Huffman position tables**, DGROUP 0x3686..0x3886, 0x200 bytes: for each code byte
 * `decode_position` reads, the high bits of the position at 0x3686 and the
 * length at 0x3786. 256 bytes each, up to 0x3886.
 */
struct engine_huffman_positions {
    uint8_t   high[256];          /* +0x00 [0x100] */
    uint8_t   len[256];           /* +0x100 [0x100] */
} PACKED;

struct engine_huffman_positions ENGINE_HUFFMAN_POSITIONS = {
    .high = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09,
        0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0a,
        0x0a, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e,
        0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x10, 0x10, 0x10, 0x11,
        0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
        0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16,
        0x16, 0x17, 0x17, 0x17, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1a, 0x1a,
        0x1b, 0x1b, 0x1c, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f, 0x1f, 0x20,
        0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25,
        0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 0x29, 0x29, 0x2a, 0x2a, 0x2b,
        0x2b, 0x2c, 0x2c, 0x2d, 0x2d, 0x2e, 0x2e, 0x2f, 0x2f, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c,
        0x3d, 0x3e, 0x3f,
    },
    .len = {
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08,
    },
};





/*
 * **An interrupted match, and where it resumes**, DGROUP 0x58e0..0x58e8, 0x08 bytes.
 */
struct engine_match_resume {
    int16_t   interrupted;        /* +0x00 [2]  a match was cut short */
    int16_t   position;           /* +0x02 [2]  and these three are what it comes back to */
    int16_t   length;             /* +0x04 [2] */
    int16_t   progress;           /* +0x06 [2] */
} PACKED;

struct engine_match_resume ENGINE_MATCH_RESUME;

/*
 * **The LZSS decoder's progress**, DGROUP 0x58e8..0x58f2, 0x0a bytes.
 * `decompress_lzss` produces bytes until `count` reaches `size`: the listing
 * steps `count` with `add`/`adc` and compares the two with `jge` on the high
 * words and `jae` on the low, which is one signed 32-bit compare.
 */
struct engine_lzss_state {
    uint16_t  ring_pos;          /* +0x00 [2] */
    int32_t   count;              /* +0x02 [4]  bytes produced so far */
    int32_t   size;               /* +0x06 [4]  the record's size, copied at the start */
} PACKED;

struct engine_lzss_state ENGINE_LZSS_STATE;

/*
 * **The son table's far pointer**, DGROUP 0x5900..0x5904. Two words that are
 * one pointer - the offset at 0x5900 and the segment at 0x5902, a far
 * pointer's own order - filed by `huffman_start` beside the other two tables
 * it caches at 0x590a and 0x590e.
 */
struct engine_huffman_tree {
    uint16_t far *son;          /* +0x00 [4] */
} PACKED;

struct engine_huffman_tree ENGINE_HUFFMAN_TREE;

/*
 * **The three cached far pointers and the LZSS init flag**, DGROUP 0x590a..0x591a, 0x10 bytes.
 */
struct engine_decompress_cache {
    /* Three cached far pointers into the decompressor's block, and all three
       tables live in that one block: the original reads them as
       `[bx + si]` with the table's offset in BX, which is why it keeps the
       pairs rather than one pointer. The port takes each as the word table it
       is - see `HUFF_TABLE` - and the pairs stay because the guest stores
       them. */
    uint16_t far *cache_a;      /* +0x00 [4]  the three records' pointers */
    uint16_t far *cache_b;      /* +0x04 [4] */
    uint8_t far *cache_c;       /* +0x08 [4]  the record's own block */
    uint8_t   pad_5916[2];        /* +0x0c [2] */
    int16_t   lzss_ready;         /* +0x0e [2]  cleared so decompress_lzss builds its tree and fills its ring */
} PACKED;

struct engine_decompress_cache ENGINE_DECOMPRESS_CACHE;

/*
 * 0x1dba8
 *
 * The LZSS compressor's start, for a resource opened to write; assembly. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near lzss_open_write(void)
{
    not_transcribed("0x1dba8, lzss_open_write");
    return 0;
}

/*
 * 0x1dc15
 *
 * Reset the LZSS state for a new stream. Eight instructions: clear the
 * initialised flag at DGROUP 0x5918 so `decompress_lzss` builds its tree and
 * fills its ring on the next call, empty the bit buffer at 0x3600, and point
 * 0x5912 at the record's own block.
 *
 * Answers 0, which is what the dispatch that reaches it expects of all of them.
 */
int16_t lzss_reset(void)
{
    struct resource *rec = ENGINE_STREAM.rec;

    ENGINE_DECOMPRESS_CACHE.lzss_ready = 0;
    ENGINE_BIT_BUFFER.bits = 0;
    ENGINE_BIT_BUFFER.bit_count = 0;

    ENGINE_DECOMPRESS_CACHE.cache_c = rec->scratch;

    return 0;
}

/*
 * 0x1dc3a
 *
 * LZHUF's `InitTree`: the encoder's binary trees emptied, every root and every parent `NIL`. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void init_tree(void)
{
    not_transcribed("0x1dc3a");
}

/*
 * 0x1dc72
 *
 * LZHUF's `InsertNode`: string `r` into the encoder's trees, keeping the longest match. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void insert_node(int16_t r)
{
    not_transcribed("0x1dc72");
    (void)r;
}

/*
 * 0x1de51
 *
 * LZHUF's `DeleteNode`: node `p` out of the encoder's trees. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void delete_node(int16_t p)
{
    not_transcribed("0x1de51");
    (void)p;
}

/*
 * 0x1dfd6
 *
 * One bit of the type-3 stream, as 0 or 1.
 *
 * The buffer is a word at DGROUP 0x3600 filled from the top, with the number of
 * bits in it at 0x3602. Bits come off the **left**: the answer is the sign of
 * the word, and the word is then shifted up by one.
 *
 * A refill happens whenever there are eight or fewer bits, not when the buffer
 * is empty, so there is always a whole byte's headroom. `next_input_byte`
 * answers -1 at the end of the input and only its low byte is taken, so the
 * stream runs on into 0xff bytes rather than stopping - which is what the
 * decoder above expects, since it stops on a symbol and not on the input.
 */
int16_t huff_get_bit(void)
{
    int16_t si;

    if (ENGINE_BIT_BUFFER.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - ENGINE_BIT_BUFFER.bit_count));
        ENGINE_BIT_BUFFER.bits = (int16_t)(((uint16_t)ENGINE_BIT_BUFFER.bits) | ax);
        ENGINE_BIT_BUFFER.bit_count = (uint8_t)(ENGINE_BIT_BUFFER.bit_count + 8);
    }

    si = ENGINE_BIT_BUFFER.bits;
    ENGINE_BIT_BUFFER.bits = (int16_t)(((uint16_t)ENGINE_BIT_BUFFER.bits) << 1);
    ENGINE_BIT_BUFFER.bit_count = (uint8_t)(ENGINE_BIT_BUFFER.bit_count - 1);

    return (int16_t)(si < 0 ? 1 : 0);
}

/*
 * 0x1e00b
 *
 * Eight bits of the same stream, as a byte. The same buffer at DGROUP 0x3600
 * and the same refill rule, but the refill is a **loop** here: taking eight
 * bits at once can need two bytes in, where `huff_get_bit` never needs more
 * than one.
 */
int16_t huff_get_byte(void)
{
    uint16_t si;

    while (ENGINE_BIT_BUFFER.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - ENGINE_BIT_BUFFER.bit_count));
        ENGINE_BIT_BUFFER.bits = (int16_t)(((uint16_t)ENGINE_BIT_BUFFER.bits) | ax);
        ENGINE_BIT_BUFFER.bit_count = (uint8_t)(ENGINE_BIT_BUFFER.bit_count + 8);
    }

    si = ((uint16_t)ENGINE_BIT_BUFFER.bits);
    ENGINE_BIT_BUFFER.bits = (int16_t)(si << 8);
    ENGINE_BIT_BUFFER.bit_count = (uint8_t)(ENGINE_BIT_BUFFER.bit_count - 8);

    return (int16_t)(si >> 8);
}

/*
 * **The three tables of the adaptive tree**, each a word array inside the one
 * scratch block - the frequencies at +0x103b, the parents at +0x1523 and the
 * sons at +0x1c7d - and each cached as its own far pointer so the routines
 * below can take it as the table it is. The original reaches an entry as
 * `[bx + si]` with the table's offset in BX and the index doubled by hand;
 * a `uint16_t *` says the same thing and indexes by the entry.
 */

/*
 * 0x1e04e
 *
 * LZHUF's `Putcode`: `len` bits of `code` into the output, a byte at a time through `put_output_byte`. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void huff_putcode(int16_t len, uint16_t code)
{
    not_transcribed("0x1e04e");
    (void)len;
    (void)code;
}

/*
 * 0x1e0b3
 *
 * Build the adaptive Huffman tree that decompression type 3 decodes with.
 *
 * Three arrays live inside the record at DGROUP 0x588a, at +0x103b, +0x1523
 * and +0x1c7d, and their far pointers are cached at DGROUP 0x590a, 0x590e and
 * 0x5900. They abut exactly: 0x274 words of frequency, then the parent array,
 * then the son array.
 *
 * The shape is LZHUF's, and the constants say so: 0x13a symbols, a tree of
 * 0x273 nodes with the root at 0x272 - 314, 627 and 626. Every leaf starts with
 * a frequency of 1 and is hung under a parent built by pairing leaves upward,
 * and the two sentinels at the end - a frequency of 0xffff past the root and a
 * parent of 0 at it - are what stop the update walk later.
 *
 * The identification is from the structure and the constants, not from any
 * source: this is transcribed from the bytes like everything else here.
 */
void huffman_start(void)
{
    struct resource *rec = ENGINE_STREAM.rec;
    uint16_t far *freq, far *prnt, far *son;
    int16_t i, j;

    /* Three places inside the scratch block, each in the block's own
       segment - the offset steps and the segment does not. */
    {
        uint8_t far *scratch = rec->scratch;

        ENGINE_DECOMPRESS_CACHE.cache_a = (uint16_t far *)(scratch + 0x103b);
        ENGINE_DECOMPRESS_CACHE.cache_b = (uint16_t far *)(scratch + 0x1523);
        ENGINE_HUFFMAN_TREE.son         = (uint16_t far *)(scratch + 0x1c7d);
    }

    freq = ENGINE_DECOMPRESS_CACHE.cache_a;
    prnt = ENGINE_DECOMPRESS_CACHE.cache_b;
    son  = ENGINE_HUFFMAN_TREE.son;

    for (i = 0; i < 0x13a; i++) {
        freq[i] = 1;
        son[i] = (uint16_t)(i + 0x273);
        prnt[i + 0x273] = (uint16_t)i;
    }

    i = 0;
    for (j = 0x13a; j <= 0x272; j++) {
        freq[j] = (uint16_t)(freq[i] + freq[i + 1]);
        son[j] = (uint16_t)i;
        prnt[i + 1] = (uint16_t)j;
        prnt[i] = (uint16_t)j;
        i += 2;
    }

    freq[0x273] = 0xffff;    /* the root's guard, `freq + 0x4e6` */
    prnt[0x272] = 0;
}

/*
 * 0x1e1af
 *
 * Halve every frequency and rebuild the tree, when the root's count would
 * overflow. LZHUF's `reconst`.
 *
 * **It is reached by a jump, not a call.** `huffman_update` jumps here and this
 * ends with a jump back into the middle of it, having built and torn down its
 * own frame in between. The port makes it an ordinary function, which is what
 * it is everywhere except in the two instructions that connect them.
 *
 * Three passes. The leaves are collected to the front with their frequencies
 * rounded up and halved; the internal nodes are rebuilt by pairing and each
 * insertion point found by walking back until the frequency fits; then every
 * parent link is written from the son array.
 *
 * **The shift in the second pass moves twice as many entries as it should.**
 * The count is `(j - k) * 2`, which is right as a *byte* count for a `memmove`
 * and wrong as the element count this loop uses it as, so it walks up to
 * `2j - k` instead of `j`. Nothing is lost by it: every entry above `j` is
 * recomputed by a later turn of the outer loop before anything reads it. It is
 * transcribed as it behaves.
 */
void huffman_reconst(void)
{
    uint16_t *freq = ENGINE_DECOMPRESS_CACHE.cache_a;
    uint16_t *prnt = ENGINE_DECOMPRESS_CACHE.cache_b;
    uint16_t *son = ENGINE_HUFFMAN_TREE.son;
    int16_t i, j, k, n;

    j = 0;
    for (i = 0; i < 0x273; i++) {
        if (son[i] >= 0x273) {
            freq[j] = (uint16_t)((freq[i] + 1) >> 1);
            son[j] = son[i];
            j++;
        }
    }

    i = 0;
    for (j = 0x13a; j < 0x273; j++) {
        uint16_t f = (uint16_t)(freq[i] + freq[i + 1]);

        freq[j] = f;

        for (k = (int16_t)(j - 1); freq[k] > f; k--)
            ;
        k++;

        for (n = (int16_t)((j - k) * 2 - 1); n >= 0; n--) {
            freq[k + n + 1] = freq[k + n];
            son[k + n + 1] = son[k + n];
        }

        freq[k] = f;
        son[k] = (uint16_t)i;
        i += 2;
    }

    for (i = 0; i < 0x273; i++) {
        uint16_t c = son[i];

        if (c >= 0x273) {
            prnt[c] = (uint16_t)i;
        } else {
            prnt[c + 1] = (uint16_t)i;
            prnt[c] = (uint16_t)i;
        }
    }
}

/*
 * 0x1e338
 *
 * Count one symbol and keep the tree ordered. LZHUF's `update`.
 *
 * The walk starts at the symbol's leaf parent and goes up to the root, adding
 * one at each step. When a node's new count passes its right-hand neighbour's
 * the two are swapped - frequencies, sons, and both parent links each, since an
 * internal node's two children share a parent entry.
 *
 * A root count of 0x8000 sends it to `huffman_reconst` first, which is why
 * frequencies never overflow.
 */
void huffman_update(uint16_t c)
{
    uint16_t *freq = ENGINE_DECOMPRESS_CACHE.cache_a;
    uint16_t *prnt = ENGINE_DECOMPRESS_CACHE.cache_b;
    uint16_t *son = ENGINE_HUFFMAN_TREE.son;

    if (freq[0x272] == 0x8000)
        huffman_reconst();

    c = prnt[c + 0x273];

    do {
        uint16_t k = (uint16_t)(freq[c] + 1);
        uint16_t l = (uint16_t)(c + 1);

        freq[c] = k;

        if (freq[l] < k) {
            uint16_t i, j;

            while (freq[l] < k)
                l++;
            l--;

            freq[c] = freq[l];
            freq[l] = k;

            i = son[c];
            prnt[i] = l;
            if (i < 0x273)
                prnt[i + 1] = l;

            j = son[l];
            son[l] = i;
            prnt[j] = c;
            if (j < 0x273)
                prnt[j + 1] = c;
            son[c] = j;

            c = l;
        }

        c = prnt[c];
    } while (c != 0);

}

/*
 * 0x1e445
 *
 * LZHUF's `EncodeChar`: a symbol's code walked up the Huffman tree and put out. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void encode_char(uint16_t c)
{
    not_transcribed("0x1e445");
    (void)c;
}

/*
 * 0x1e4a7
 *
 * LZHUF's `EncodePosition`: a match position, its top six bits coded and the rest raw. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void encode_position(uint16_t c)
{
    not_transcribed("0x1e4a7");
    (void)c;
}

/*
 * 0x1e4e7
 *
 * LZHUF's `EncodeEnd`: whatever bits are left put out. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void encode_end(void)
{
    not_transcribed("0x1e4e7");
}

/*
 * 0x1e561
 *
 * Decode a match position: twelve bits, of which the top six come out of a
 * table and the bottom six are read raw.
 *
 * A byte is taken first and used to index two 256-entry tables in DGROUP - the
 * code at 0x3686 and the length at 0x3786 - which between them say how many
 * further bits the position needs. Those bits are shifted into the byte one at
 * a time, and only the low six of the result survive; the table's code supplies
 * the rest, shifted up by six.
 *
 * **It is reached by a jump, not a call**, and ends with a jump back into
 * 0x1e7f2 rather than a `ret` - the same arrangement as `huffman_reconst`. The
 * port makes it an ordinary function. That is also why the verifier cannot
 * check it on its own: there is no return for the harness to watch for. Every
 * one of `decompress_lzss`'s 226 verified calls runs it.
 */
int16_t decode_position(void)
{
    uint16_t si = (uint16_t)huff_get_byte();
    uint16_t high = (uint16_t)(ENGINE_HUFFMAN_POSITIONS.high[si] << 6);
    int16_t n = (int16_t)(ENGINE_HUFFMAN_POSITIONS.len[si] - 2);

    while (n-- != 0)
        si = (uint16_t)(2 * si + huff_get_bit());

    return (int16_t)(high | (si & 0x3f));
}

/*
 * 0x1e5ae
 *
 * The LZSS compressor's pass over the spill ring. NOT TRANSCRIBED YET: a stub, which aborts.
 */
int16_t near lzss_flush(int16_t final)
{
    (void)final;
    not_transcribed("0x1e5ae, lzss_flush");
    return 0;
}

/*
 * 0x1e7f2
 *
 * Decompression type 3: LZSS over a 4096-byte ring, with the literals and match
 * lengths adaptively Huffman coded and the match positions coded by
 * `decode_position`. The third and busiest of the handlers the table at DGROUP
 * 0x3580 dispatches to.
 *
 * Like `decompress_lzw` it can stop in the middle and be called again, and for
 * the same reason: `emit_byte` answers 0 once the caller's request is full. The
 * flag at DGROUP 0x58e0 says a match was interrupted, and the position, length
 * and progress at 0x58e2, 0x58e4 and 0x58e6 are what it comes back to.
 *
 * The first call also initialises: the tree, the ring filled with 0xfc4 spaces
 * and the write position set past them, and the total to produce read from the
 * record's +0x12:+0x14. 0x5918 is what makes that happen once.
 *
 * A symbol below 0x100 is a literal. Anything else is a match, whose length is
 * the symbol less 0xfd and whose source is the ring position that far back,
 * masked to twelve bits. Every byte produced goes to the output *and* back into
 * the ring, which is why a match may read bytes it has just written.
 *
 * Two blocks of this routine are placed out of line by the compiler and are
 * folded back in here: the symbol decode at 0x1e52d, which walks the tree from
 * the root a bit at a time, and 0x1e561 above.
 *
 * The answer is always 0.
 */
int16_t decompress_lzss(void)
{
    /* **The ring**, 0x1000 bytes at the front of the record's own block: the
       window the matches are copied out of, indexed everywhere below by a
       position masked to 0xfff. */
    uint8_t far * ring = ENGINE_DECOMPRESS_CACHE.cache_c;
    uint16_t di = 0;
    int16_t si;

    if (ENGINE_DECOMPRESS_CACHE.lzss_ready == 0) {
        struct resource *rec;
        int16_t i;

        ENGINE_MATCH_RESUME.interrupted = 0;
        huffman_start();

        for (i = 0; i < 0xfc4; i++)
            ring[i] = 0x20;

        ENGINE_LZSS_STATE.ring_pos = 0xfc4;
        ENGINE_LZSS_STATE.count = 0;

        rec = ENGINE_STREAM.rec;
        ENGINE_LZSS_STATE.size = rec->size;
        ENGINE_DECOMPRESS_CACHE.lzss_ready = 1;
    }

    for (;;) {
        /* 0x1e91d - is there still something to produce? */
        if (ENGINE_LZSS_STATE.count >= ENGINE_LZSS_STATE.size)
            return 0;

        if (ENGINE_MATCH_RESUME.interrupted == 0) {
            /* 0x1e52d - one symbol, walked out of the tree bit by bit. */
            const uint16_t *son = ENGINE_HUFFMAN_TREE.son;

            di = son[0x272];          /* the root */
            while (di < 0x273)
                di = son[di + huff_get_bit()];

            di -= 0x273;
            huffman_update(di);

            if (di < 0x100) {
                /* 0x1e849 - a literal. */
                si = emit_byte(di);

                ring[ENGINE_LZSS_STATE.ring_pos] = (uint8_t)di;
                ENGINE_LZSS_STATE.ring_pos = (int16_t)((ENGINE_LZSS_STATE.ring_pos + 1) & 0xfff);
                ENGINE_LZSS_STATE.count = (int32_t)((uint32_t)ENGINE_LZSS_STATE.count + 1);

                if (si == 0)
                    return 0;
                continue;
            }

            /* 0x1e89c - a match. */
            {
                uint16_t pos = (uint16_t)decode_position();

                ENGINE_MATCH_RESUME.position = (int16_t)((ENGINE_LZSS_STATE.ring_pos - pos - 1) & 0xfff);
                ENGINE_MATCH_RESUME.length = (int16_t)(di + 0xff03);
                ENGINE_MATCH_RESUME.progress = 0;
            }
        }

        ENGINE_MATCH_RESUME.interrupted = 0;

        while (ENGINE_MATCH_RESUME.progress < ENGINE_MATCH_RESUME.length) {
            uint16_t b = ring[(((uint16_t)ENGINE_MATCH_RESUME.position)
                               + ((uint16_t)ENGINE_MATCH_RESUME.progress))
                              & 0xfff];

            si = emit_byte(b);

            ring[ENGINE_LZSS_STATE.ring_pos] = (uint8_t)b;
            ENGINE_LZSS_STATE.ring_pos = (int16_t)((ENGINE_LZSS_STATE.ring_pos + 1) & 0xfff);
            ENGINE_LZSS_STATE.count = (int32_t)((uint32_t)ENGINE_LZSS_STATE.count + 1);

            ENGINE_MATCH_RESUME.progress = (int16_t)(((uint16_t)ENGINE_MATCH_RESUME.progress) + 1);

            if (si == 0) {
                ENGINE_MATCH_RESUME.interrupted = 1;
                return 0;
            }
        }
    }
}

#endif
