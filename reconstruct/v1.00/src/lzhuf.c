/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
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
 * JUDGE: structs resource=res engine_stream=strm
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
INCLUDE STRUCTS.ASH
_BSS segment word public 'BSS'
enc_waiting db 2 dup (?)
enc_i db 2 dup (?)
enc_c db 2 dup (?)
enc_len db 2 dup (?)
enc_r db 2 dup (?)
enc_s db 2 dup (?)
enc_last_match_length db 2 dup (?)
match_interrupted db 2 dup (?)
match_position db 2 dup (?)
match_length db 2 dup (?)
match_progress db 2 dup (?)
lzss_ring_pos db 2 dup (?)
lzss_count_lo db 2 dup (?)
lzss_count_hi db 2 dup (?)
lzss_size_lo db 2 dup (?)
lzss_size_hi db 2 dup (?)
enc_match_length db 2 dup (?)
lzh_lson_off db 2 dup (?)
lzh_lson_seg db 2 dup (?)
lzh_rson_off db 2 dup (?)
lzh_rson_seg db 2 dup (?)
enc_match_position db 2 dup (?)
enc_started db 2 dup (?)
huff_son_off db 2 dup (?)
huff_son_seg db 2 dup (?)
huff_len db 2 dup (?)
lzh_dad_off db 2 dup (?)
lzh_dad_seg db 2 dup (?)
huff_freq_off db 2 dup (?)
huff_freq_seg db 2 dup (?)
huff_prnt_off db 2 dup (?)
huff_prnt_seg db 2 dup (?)
lzss_ring_off db 2 dup (?)
lzss_ring_seg db 2 dup (?)
huff_code db 2 dup (?)
lzss_ready db 2 dup (?)
_BSS ends

_DATA segment word public 'DATA'
lzh_getbuf label byte
        db 0h, 0h
lzh_getlen label byte
        db 0h
lzh_putbuf label byte
        db 0h, 0h
lzh_putlen label byte
        db 0h
lzh_p_len label byte
        db 3h, 4h, 4h, 4h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 6h, 6h, 6h, 6h
        db 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
        db 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
        db 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h
lzh_p_code label byte
        db 0h, 20h, 30h, 40h, 50h, 58h, 60h, 68h, 70h, 78h, 80h, 88h, 90h, 94h, 98h, 9ch
        db 0a0h, 0a4h, 0a8h, 0ach, 0b0h, 0b4h, 0b8h, 0bch, 0c0h, 0c2h, 0c4h, 0c6h, 0c8h, 0cah, 0cch, 0ceh
        db 0d0h, 0d2h, 0d4h, 0d6h, 0d8h, 0dah, 0dch, 0deh, 0e0h, 0e2h, 0e4h, 0e6h, 0e8h, 0eah, 0ech, 0eeh
        db 0f0h, 0f1h, 0f2h, 0f3h, 0f4h, 0f5h, 0f6h, 0f7h, 0f8h, 0f9h, 0fah, 0fbh, 0fch, 0fdh, 0feh, 0ffh
lzh_d_code label byte
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
lzh_d_len label byte
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
extrn _g_engine_stream:byte
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
        mov word ptr DGROUP:enc_started, 0
        mov word ptr DGROUP:lzh_putbuf, 0
        mov byte ptr DGROUP:lzh_putlen, 0
        xor ax, ax
        push ax
        push ax
        mov dx, 2002h
        push ax
        push dx
        call FAR PTR _dos_alloc_bytes
        add sp, 8
        mov word ptr DGROUP:lzh_lson_seg, dx
        mov word ptr DGROUP:lzh_lson_off, ax
        xor ax, ax
        push ax
        push ax
        mov dx, 2202h
        push ax
        push dx
        call FAR PTR _dos_alloc_bytes
        add sp, 8
        mov word ptr DGROUP:lzh_rson_seg, dx
        mov word ptr DGROUP:lzh_rson_off, ax
        xor ax, ax
        push ax
        push ax
        mov dx, 2002h
        push ax
        push dx
        call FAR PTR _dos_alloc_bytes
        add sp, 8
        mov word ptr DGROUP:lzh_dad_seg, dx
        mov word ptr DGROUP:lzh_dad_off, ax
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov ax, word ptr [bx+res_scratch+2]
        mov dx, word ptr [bx+res_scratch]
        mov word ptr DGROUP:lzss_ring_seg, ax
        mov word ptr DGROUP:lzss_ring_off, dx
        xor ax, ax
        ret
_lzss_open_write endp

/* 0x1dc15 */
_lzss_reset proc near
        mov word ptr DGROUP:lzss_ready, 0
        mov word ptr DGROUP:lzh_getbuf, 0
        mov byte ptr DGROUP:lzh_getlen, 0
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov ax, word ptr [bx+res_scratch+2]
        mov dx, word ptr [bx+res_scratch]
        mov word ptr DGROUP:lzss_ring_seg, ax
        mov word ptr DGROUP:lzss_ring_off, dx
        xor ax, ax
        ret
_lzss_reset endp

/* 0x1dc3a */
_init_tree proc near
        push si
        mov si, 1001h
        jmp short init_rson_test
init_rson_loop:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        mov word ptr es:[bx], 1000h
        inc si
init_rson_test:
        cmp si, 1100h
        jle init_rson_loop
        xor si, si
        jmp short init_dad_test
init_dad_loop:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov word ptr es:[bx], 1000h
        inc si
init_dad_test:
        cmp si, 1000h
        jl init_dad_loop
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
        mov ax, word ptr DGROUP:lzss_ring_seg
        mov dx, word ptr DGROUP:lzss_ring_off
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
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, dx
        mov ax, 1000h
        mov word ptr es:[bx], ax
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, dx
        mov word ptr es:[bx], ax
        mov word ptr DGROUP:enc_match_length, 0
insert_step:
        or cx, cx
        jl insert_left
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        cmp word ptr es:[bx], 1000h
        je insert_new_right
        mov si, word ptr es:[bx]
        jmp short insert_go_compare
insert_new_right:
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov word ptr es:[bx], si
        jmp insert_return
insert_go_compare:
        jmp short insert_compare
insert_left:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        cmp word ptr es:[bx], 1000h
        je insert_new_left
        mov si, word ptr es:[bx]
        jmp short insert_compare
insert_new_left:
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov word ptr es:[bx], si
        jmp insert_return
insert_compare:
        mov di, 1
        jmp short insert_compare_test
insert_compare_loop:
        les bx, dword ptr [bp-4]
        mov cl, byte ptr es:[bx+di]
        mov ch, 0
        les bx, dword ptr DGROUP:lzss_ring_off
        add bx, si
        add bx, di
        mov dl, byte ptr es:[bx]
        mov dh, 0
        sub cx, dx
        or cx, cx
        jne insert_compared
        inc di
insert_compare_test:
        cmp di, 3ch
        jl insert_compare_loop
insert_compared:
        cmp di, 2
        jle insert_next
        cmp di, word ptr DGROUP:enc_match_length
        jle insert_same_length
        mov ax, word ptr [bp+4]
        sub ax, si
        and ax, 0fffh
        dec ax
        mov word ptr DGROUP:enc_match_position, ax
        mov word ptr DGROUP:enc_match_length, di
        cmp di, 3ch
        jge insert_replace
insert_same_length:
        cmp di, word ptr DGROUP:enc_match_length
        jne insert_next
        mov ax, word ptr [bp+4]
        sub ax, si
        and ax, 0fffh
        dec ax
        mov word ptr [bp-6], ax
        cmp ax, word ptr DGROUP:enc_match_position
        jb insert_nearer
        jmp insert_step
insert_nearer:
        mov ax, word ptr [bp-6]
        mov word ptr DGROUP:enc_match_position, ax
insert_next:
        jmp insert_step
insert_replace:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, word ptr [bp+4]
        shl bx, 1
        add bx, word ptr DGROUP:lzh_dad_off
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, word ptr [bp+4]
        shl bx, 1
        add bx, word ptr DGROUP:lzh_lson_off
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, word ptr [bp+4]
        shl bx, 1
        add bx, word ptr DGROUP:lzh_rson_off
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        cmp word ptr es:[bx], si
        jne insert_parent_left
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        jmp short insert_unlink
insert_parent_left:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
insert_unlink:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov word ptr es:[bx], 1000h
insert_return:
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
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne delete_present
        jmp delete_return
delete_present:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne delete_has_right
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        mov si, word ptr es:[bx]
        jmp delete_link_parent
delete_has_right:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne delete_two_children
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        mov si, word ptr es:[bx]
        jmp delete_link_parent
delete_two_children:
        mov si, word ptr es:[bx]
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne delete_find_max
        jmp delete_take_right
delete_find_max:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
delete_max_loop:
        mov si, word ptr es:[bx]
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        cmp word ptr es:[bx], 1000h
        jne delete_max_loop
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, si
        shl dx, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, dx
        mov dx, word ptr es:[bx]
        shl dx, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, si
        shl dx, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, dx
        mov dx, word ptr es:[bx]
        shl dx, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, si
        shl bx, 1
        add bx, word ptr DGROUP:lzh_lson_off
        mov word ptr es:[bx], ax
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov word ptr es:[bx], si
delete_take_right:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, si
        shl bx, 1
        add bx, word ptr DGROUP:lzh_rson_off
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov word ptr es:[bx], si
delete_link_parent:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov bx, si
        shl bx, 1
        add bx, word ptr DGROUP:lzh_dad_off
        mov word ptr es:[bx], ax
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_rson_off
        add bx, ax
        cmp word ptr es:[bx], di
        jne delete_parent_left
        mov word ptr es:[bx], si
        jmp short delete_unlink
delete_parent_left:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov ax, word ptr es:[bx]
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_lson_off
        add bx, ax
        mov word ptr es:[bx], si
delete_unlink:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:lzh_dad_off
        add bx, ax
        mov word ptr es:[bx], 1000h
delete_return:
        pop di
        pop si
        pop bp
        ret
_delete_node endp

/* 0x1dfd6 */
_huff_get_bit proc near
        push si
        cmp byte ptr DGROUP:lzh_getlen, 8
        ja bit_take
        call _next_input_byte
        mov ah, 0
        mov cl, 8
        sub cl, byte ptr DGROUP:lzh_getlen
        shl ax, cl
        or word ptr DGROUP:lzh_getbuf, ax
        add byte ptr DGROUP:lzh_getlen, 8
bit_take:
        mov si, word ptr DGROUP:lzh_getbuf
        shl word ptr DGROUP:lzh_getbuf, 1
        sub byte ptr DGROUP:lzh_getlen, 1
        xor ax, ax
        cmp si, 0
        jge bit_return
        inc ax
bit_return:
        pop si
        ret
_huff_get_bit endp

/* 0x1e00b */
_huff_get_byte proc near
        push bp
        mov bp, sp
        push si
        jmp short byte_need
byte_fill:
        call _next_input_byte
        mov ah, 0
        mov cl, 8
        sub cl, byte ptr DGROUP:lzh_getlen
        shl ax, cl
        or word ptr DGROUP:lzh_getbuf, ax
        mov al, byte ptr DGROUP:lzh_getlen
        add al, 8
        mov byte ptr DGROUP:lzh_getlen, al
byte_need:
        cmp byte ptr DGROUP:lzh_getlen, 8
        jbe byte_fill
        mov si, word ptr DGROUP:lzh_getbuf
        mov cl, 8
        shl word ptr DGROUP:lzh_getbuf, cl
        mov al, byte ptr DGROUP:lzh_getlen
        add al, 0f8h
        mov byte ptr DGROUP:lzh_getlen, al
        mov ax, si
        mov cl, 8
        shr ax, cl
        jmp short byte_return
byte_return:
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
        mov cl, byte ptr DGROUP:lzh_putlen
        shr ax, cl
        or word ptr DGROUP:lzh_putbuf, ax
        mov al, byte ptr DGROUP:lzh_putlen
        add al, byte ptr [bp+4]
        mov byte ptr DGROUP:lzh_putlen, al
        cmp al, 8
        jb putcode_return
        mov ax, word ptr DGROUP:lzh_putbuf
        mov cl, 8
        shr ax, cl
        push ax
        call _put_output_byte
        add sp, 2
        mov al, byte ptr DGROUP:lzh_putlen
        add al, 0f8h
        mov byte ptr DGROUP:lzh_putlen, al
        cmp al, 8
        jb putcode_shift
        push word ptr DGROUP:lzh_putbuf
        call _put_output_byte
        add sp, 2
        mov al, byte ptr DGROUP:lzh_putlen
        add al, 0f8h
        mov byte ptr DGROUP:lzh_putlen, al
        mov cl, byte ptr [bp+4]
        sub cl, byte ptr DGROUP:lzh_putlen
        mov ax, si
        shl ax, cl
        mov word ptr DGROUP:lzh_putbuf, ax
        jmp short putcode_return
putcode_shift:
        mov cl, 8
        shl word ptr DGROUP:lzh_putbuf, cl
putcode_return:
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
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov ax, word ptr [bx+res_scratch+2]
        mov dx, word ptr [bx+res_scratch]
        add dx, 103bh
        mov word ptr DGROUP:huff_freq_seg, ax
        mov word ptr DGROUP:huff_freq_off, dx
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov ax, word ptr [bx+res_scratch+2]
        mov dx, word ptr [bx+res_scratch]
        add dx, 1523h
        mov word ptr DGROUP:huff_prnt_seg, ax
        mov word ptr DGROUP:huff_prnt_off, dx
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov ax, word ptr [bx+res_scratch+2]
        mov dx, word ptr [bx+res_scratch]
        add dx, 1c7dh
        mov word ptr DGROUP:huff_son_seg, ax
        mov word ptr DGROUP:huff_son_off, dx
        xor si, si
        jmp short start_leaves
start_leaf:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        mov word ptr es:[bx], 1
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov ax, si
        add ax, 273h
        mov word ptr es:[bx], ax
        mov ax, si
        add ax, 273h
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov word ptr es:[bx], si
        inc si
start_leaves:
        cmp si, 13ah
        jl start_leaf
        xor si, si
        mov di, 13ah
        jmp short start_nodes
start_node:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, si
        inc dx
        shl dx, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, dx
        add ax, word ptr es:[bx]
        mov dx, di
        shl dx, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov word ptr es:[bx], si
        mov ax, si
        inc ax
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov ax, di
        mov word ptr es:[bx], ax
        mov dx, si
        shl dx, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, dx
        mov word ptr es:[bx], ax
        inc si
        inc si
        inc di
start_nodes:
        cmp di, 272h
        jle start_node
        les bx, dword ptr DGROUP:huff_freq_off
        mov word ptr es:[bx+4e6h], 0ffffh
        les bx, dword ptr DGROUP:huff_prnt_off
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
        jmp short rebuild_collect_loop
rebuild_collect:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        cmp word ptr es:[bx], 273h
        jl rebuild_collect_next
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        mov ax, word ptr es:[bx]
        inc ax
        shr ax, 1
        mov dx, cx
        shl dx, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, cx
        shl dx, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, dx
        mov word ptr es:[bx], ax
        inc cx
rebuild_collect_next:
        inc si
rebuild_collect_loop:
        cmp si, 273h
        jl rebuild_collect
        xor si, si
        mov cx, 13ah
        jmp rebuild_node_loop
rebuild_node:
        mov ax, si
        inc ax
        mov di, ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, di
        shl dx, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, dx
        add ax, word ptr es:[bx]
        mov dx, cx
        shl dx, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, dx
        mov word ptr es:[bx], ax
        mov word ptr [bp-4], ax
        mov ax, cx
        dec ax
        mov di, ax
        jmp short rebuild_find
rebuild_find_back:
        dec di
rebuild_find:
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        mov ax, word ptr es:[bx]
        cmp ax, word ptr [bp-4]
        ja rebuild_find_back
        inc di
        mov ax, cx
        sub ax, di
        shl ax, 1
        mov word ptr [bp-6], ax
        mov ax, word ptr [bp-6]
        dec ax
        mov word ptr [bp-2], ax
        jmp short rebuild_shift_test
rebuild_shift_loop:
        mov ax, di
        add ax, word ptr [bp-2]
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, di
        add dx, word ptr [bp-2]
        inc dx
        shl dx, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, di
        add ax, word ptr [bp-2]
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov dx, di
        add dx, word ptr [bp-2]
        inc dx
        shl dx, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, dx
        mov word ptr es:[bx], ax
        dec word ptr [bp-2]
rebuild_shift_test:
        cmp word ptr [bp-2], 0
        jge rebuild_shift_loop
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        mov ax, word ptr [bp-4]
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov word ptr es:[bx], si
        inc si
        inc si
        inc cx
rebuild_node_loop:
        cmp cx, 273h
        jge rebuild_links
        jmp rebuild_node
rebuild_links:
        xor si, si
        jmp short rebuild_link_loop
rebuild_link:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov di, ax
        cmp ax, 273h
        jl rebuild_link_pair
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov word ptr es:[bx], si
        jmp short rebuild_link_next
rebuild_link_pair:
        mov ax, di
        inc ax
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov ax, si
        mov word ptr es:[bx], ax
        mov dx, di
        shl dx, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, dx
        mov word ptr es:[bx], ax
rebuild_link_next:
        inc si
rebuild_link_loop:
        cmp si, 273h
        jl rebuild_link
        pop di
        pop si
        mov sp, bp
        pop bp
        jmp short update_climb
rebuild_pad db 90h
_huffman_reconst endp

/* 0x1e338 */
_huffman_update proc near
        push bp
        mov bp, sp
        sub sp, 4
        push si
        push di
        les bx, dword ptr DGROUP:huff_freq_off
        cmp word ptr es:[bx+4e4h], 8000h
        jne update_climb
        jmp _huffman_reconst
update_climb:
        mov ax, word ptr [bp+4]
        add ax, 273h
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp+4], ax
update_node:
        mov ax, word ptr [bp+4]
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        inc word ptr es:[bx]
        mov ax, word ptr es:[bx]
        mov word ptr [bp-2], ax
        mov si, word ptr [bp+4]
        inc si
        add bx, 2
        mov ax, word ptr es:[bx]
        cmp ax, word ptr [bp-2]
        jb update_find
        jmp update_up
update_find:
        inc si
        add bx, 2
        mov ax, word ptr es:[bx]
        cmp ax, word ptr [bp-2]
        jb update_find
        dec si
        sub bx, 2
        mov ax, word ptr es:[bx]
        mov dx, word ptr [bp+4]
        shl dx, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, dx
        mov word ptr es:[bx], ax
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_freq_off
        add bx, ax
        mov ax, word ptr [bp-2]
        mov word ptr es:[bx], ax
        mov ax, word ptr [bp+4]
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp-4], ax
        mov ax, word ptr [bp-4]
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov word ptr es:[bx], si
        cmp word ptr [bp-4], 273h
        jge update_swap_other
        add bx, 2
        mov word ptr es:[bx], si
update_swap_other:
        mov ax, si
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov di, word ptr es:[bx]
        mov ax, word ptr [bp-4]
        mov word ptr es:[bx], ax
        mov ax, di
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov ax, word ptr [bp+4]
        mov word ptr es:[bx], ax
        cmp di, 273h
        jge update_swapped
        add bx, 2
        mov word ptr es:[bx], ax
update_swapped:
        mov ax, word ptr [bp+4]
        shl ax, 1
        les bx, dword ptr DGROUP:huff_son_off
        add bx, ax
        mov word ptr es:[bx], di
        mov word ptr [bp+4], si
update_up:
        mov ax, word ptr [bp+4]
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp+4], ax
        or ax, ax
        je update_return
        jmp update_node
update_return:
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
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp-2], ax
        xor di, di
encode_char_bit:
        shr si, 1
        test word ptr [bp-2], 1
        je encode_char_up
        add si, 8000h
encode_char_up:
        inc di
        mov ax, word ptr [bp-2]
        shl ax, 1
        les bx, dword ptr DGROUP:huff_prnt_off
        add bx, ax
        mov ax, word ptr es:[bx]
        mov word ptr [bp-2], ax
        cmp ax, 272h
        jne encode_char_bit
        push si
        push di
        call _huff_putcode
        add sp, 4
        mov word ptr DGROUP:huff_code, si
        mov word ptr DGROUP:huff_len, di
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
        mov al, byte ptr lzh_p_code[si]
        mov ah, 0
        mov cl, 8
        shl ax, cl
        push ax
        mov al, byte ptr lzh_p_len[si]
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
        cmp byte ptr DGROUP:lzh_putlen, 0
        je encode_end_free
        mov ax, word ptr DGROUP:lzh_putbuf
        mov cl, 8
        shr ax, cl
        push ax
        call _put_output_byte
        add sp, 2
encode_end_free:
        push word ptr DGROUP:lzh_lson_seg
        push word ptr DGROUP:lzh_lson_off
        call FAR PTR _dos_free_far
        add sp, 4
        push word ptr DGROUP:lzh_rson_seg
        push word ptr DGROUP:lzh_rson_off
        call FAR PTR _dos_free_far
        add sp, 4
        push word ptr DGROUP:lzh_dad_seg
        push word ptr DGROUP:lzh_dad_off
        call FAR PTR _dos_free_far
        add sp, 4
        ret
decode_char:
        les bx, dword ptr DGROUP:huff_son_off
        mov di, word ptr es:[bx+4e4h]
        jmp short decode_char_test
decode_char_dead_seg db 0a1h, 2h, 59h
decode_char_dead_es db 8eh, 0c0h
decode_char_step:
        call _huff_get_bit
        mov bx, ax
        add bx, di
        shl bx, 1
        add bx, word ptr DGROUP:huff_son_off
        mov di, word ptr es:[bx]
decode_char_test:
        cmp di, 273h
        jb decode_char_step
        sub di, 273h
        push di
        call _huffman_update
        add sp, 2
        jmp lzss_char
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
        mov al, byte ptr lzh_d_code[si]
        mov ah, 0
        mov cl, 6
        shl ax, cl
        mov word ptr [bp-2], ax
        mov al, byte ptr lzh_d_len[si]
        mov ah, 0
        mov di, ax
        dec di
        dec di
        jmp short position_bits_left
position_bit:
        call _huff_get_bit
        mov dx, si
        shl dx, 1
        add dx, ax
        mov si, dx
position_bits_left:
        mov ax, di
        dec di
        or ax, ax
        jne position_bit
        mov ax, si
        and ax, 3fh
        push ax
        mov ax, word ptr [bp-2]
        pop dx
        or ax, dx
        jmp short position_return
position_return:
        pop di
        pop si
        mov sp, bp
        pop bp
        jmp lzss_position
_decode_position endp

/* 0x1e5ae */
_lzss_flush proc near
        push bp
        mov bp, sp
        sub sp, 4
        push si
        push di
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov al, byte ptr [bx+res_spill_start]
        mov ah, 0
        mov word ptr [bp-2], ax
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov al, byte ptr [bx+res_spill_end]
        mov ah, 0
        mov di, ax
        mov si, word ptr DGROUP:_g_engine_stream+strm_spill
        mov ax, word ptr [bp-2]
        mov word ptr [bp-4], ax
        cmp word ptr DGROUP:enc_started, 0
        je enc_begin
        jmp enc_loop
enc_begin:
        cmp word ptr DGROUP:enc_waiting, 0
        jne enc_fill
        call _huffman_start
        call _init_tree
        mov word ptr DGROUP:enc_s, 0
        mov word ptr DGROUP:enc_r, 0fc4h
        push di
        les di, dword ptr DGROUP:lzss_ring_off
        add di, word ptr DGROUP:enc_i
        mov cx, word ptr DGROUP:enc_r
        sub cx, word ptr DGROUP:enc_i
        mov al, 20h
        cld
        rep stosb
        pop di
        mov word ptr DGROUP:enc_len, 0
enc_fill:
        mov word ptr DGROUP:enc_waiting, 0
        jmp short enc_fill_test
enc_fill_loop:
        mov bx, word ptr [bp-4]
        mov al, byte ptr [bx+si]
        les bx, dword ptr DGROUP:lzss_ring_off
        add bx, word ptr DGROUP:enc_r
        add bx, word ptr DGROUP:enc_len
        mov byte ptr es:[bx], al
        inc word ptr [bp-4]
        and word ptr [bp-4], 7fh
        inc word ptr DGROUP:enc_len
enc_fill_test:
        cmp word ptr DGROUP:enc_len, 3ch
        jge enc_filled
        cmp word ptr [bp-4], di
        jne enc_fill_loop
enc_filled:
        cmp word ptr [bp-4], di
        jne enc_insert_prefix
        cmp word ptr [bp+4], 0
        jne enc_insert_prefix
        mov word ptr DGROUP:enc_waiting, 1
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov al, byte ptr [bp-4]
        mov byte ptr [bx+res_spill_start], al
        xor ax, ax
        jmp enc_return
enc_insert_prefix:
        mov word ptr DGROUP:enc_i, 1
        jmp short enc_insert_test
enc_insert_loop:
        mov ax, word ptr DGROUP:enc_r
        sub ax, word ptr DGROUP:enc_i
        push ax
        call _insert_node
        add sp, 2
        inc word ptr DGROUP:enc_i
enc_insert_test:
        cmp word ptr DGROUP:enc_i, 3ch
        jle enc_insert_loop
        push word ptr DGROUP:enc_r
        call _insert_node
        add sp, 2
        mov word ptr DGROUP:enc_started, 1
enc_loop:
        cmp word ptr DGROUP:enc_waiting, 0
        jne enc_shift
        mov ax, word ptr DGROUP:enc_match_length
        cmp ax, word ptr DGROUP:enc_len
        jle enc_match_clamped
        mov ax, word ptr DGROUP:enc_len
        mov word ptr DGROUP:enc_match_length, ax
enc_match_clamped:
        cmp word ptr DGROUP:enc_match_length, 2
        jg enc_emit_match
        mov word ptr DGROUP:enc_match_length, 1
        les bx, dword ptr DGROUP:lzss_ring_off
        add bx, word ptr DGROUP:enc_r
        mov al, byte ptr es:[bx]
        mov ah, 0
        push ax
        call _encode_char
        add sp, 2
        jmp short enc_emitted
enc_emit_match:
        mov ax, word ptr DGROUP:enc_match_length
        add ax, 0fdh
        push ax
        call _encode_char
        add sp, 2
        push word ptr DGROUP:enc_match_position
        call _encode_position
        add sp, 2
enc_emitted:
        mov ax, word ptr DGROUP:enc_match_length
        mov word ptr DGROUP:enc_last_match_length, ax
        mov word ptr DGROUP:enc_i, 0
enc_shift:
        mov word ptr DGROUP:enc_waiting, 0
        jmp short enc_shift_test
enc_shift_loop:
        mov bx, word ptr [bp-4]
        mov al, byte ptr [bx+si]
        mov ah, 0
        mov word ptr DGROUP:enc_c, ax
        inc word ptr [bp-4]
        and word ptr [bp-4], 7fh
        push word ptr DGROUP:enc_s
        call _delete_node
        add sp, 2
        les bx, dword ptr DGROUP:lzss_ring_off
        add bx, word ptr DGROUP:enc_s
        mov al, byte ptr DGROUP:enc_c
        mov byte ptr es:[bx], al
        cmp word ptr DGROUP:enc_s, 3bh
        jge enc_shift_unmirrored
        mov bx, word ptr DGROUP:enc_s
        mov es, word ptr DGROUP:lzss_ring_seg
        add bx, word ptr DGROUP:lzss_ring_off
        mov al, byte ptr DGROUP:enc_c
        mov byte ptr es:[bx+1000h], al
enc_shift_unmirrored:
        inc word ptr DGROUP:enc_s
        and word ptr DGROUP:enc_s, 0fffh
        inc word ptr DGROUP:enc_r
        and word ptr DGROUP:enc_r, 0fffh
        push word ptr DGROUP:enc_r
        call _insert_node
        add sp, 2
        inc word ptr DGROUP:enc_i
enc_shift_test:
        mov ax, word ptr DGROUP:enc_i
        cmp ax, word ptr DGROUP:enc_last_match_length
        jge enc_shifted
        cmp word ptr [bp-4], di
        je enc_shifted
        jmp short enc_shift_loop
enc_shifted:
        cmp word ptr [bp-4], di
        jne enc_tail_test
        cmp word ptr [bp+4], 0
        jne enc_tail_test
        mov word ptr DGROUP:enc_waiting, 1
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov al, byte ptr [bp-4]
        mov byte ptr [bx+res_spill_start], al
        xor ax, ax
        jmp short enc_return
enc_dead_jmp db 0ebh, 2eh
enc_tail_loop:
        push word ptr DGROUP:enc_s
        call _delete_node
        add sp, 2
        mov ax, word ptr DGROUP:enc_s
        inc ax
        and ax, 0fffh
        mov word ptr DGROUP:enc_s, ax
        mov ax, word ptr DGROUP:enc_r
        inc ax
        and ax, 0fffh
        mov word ptr DGROUP:enc_r, ax
        dec word ptr DGROUP:enc_len
        je enc_tail_test
        push word ptr DGROUP:enc_r
        call _insert_node
        add sp, 2
enc_tail_test:
        mov ax, word ptr DGROUP:enc_i
        inc word ptr DGROUP:enc_i
        cmp ax, word ptr DGROUP:enc_last_match_length
        jl enc_tail_loop
        cmp word ptr DGROUP:enc_len, 0
        jle enc_done
        jmp enc_loop
enc_done:
        call _encode_end
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov al, byte ptr [bp-4]
        mov byte ptr [bx+res_spill_start], al
        xor ax, ax
        jmp short enc_return
enc_return:
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
        cmp word ptr DGROUP:lzss_ready, 0
        je lzss_start
        jmp lzss_more
lzss_start:
        mov word ptr DGROUP:match_interrupted, 0
        call _huffman_start
        les di, dword ptr DGROUP:lzss_ring_off
        mov cx, 0fc4h
        mov al, 20h
        cld
        rep stosb
        mov word ptr DGROUP:lzss_ring_pos, 0fc4h
        mov word ptr DGROUP:lzss_count_hi, 0
        mov word ptr DGROUP:lzss_count_lo, 0
        mov bx, word ptr DGROUP:_g_engine_stream+strm_rec
        mov ax, word ptr [bx+res_size+2]
        mov dx, word ptr [bx+res_size]
        mov word ptr DGROUP:lzss_size_hi, ax
        mov word ptr DGROUP:lzss_size_lo, dx
        mov word ptr DGROUP:lzss_ready, 1
        jmp lzss_more
lzss_next:
        cmp word ptr DGROUP:match_interrupted, 0
        jne lzss_not_literal
        jmp decode_char
lzss_char:
        cmp di, 100h
        jge lzss_not_literal
        push di
        call _emit_byte
        add sp, 2
        mov si, ax
        les bx, dword ptr DGROUP:lzss_ring_off
        add bx, word ptr DGROUP:lzss_ring_pos
        mov ax, di
        mov byte ptr es:[bx], al
        inc word ptr DGROUP:lzss_ring_pos
        and word ptr DGROUP:lzss_ring_pos, 0fffh
        add word ptr DGROUP:lzss_count_lo, 1
        adc word ptr DGROUP:lzss_count_hi, 0
        or si, si
        jne lzss_not_literal
        xor ax, ax
        jmp lzss_return
lzss_not_literal:
        cmp di, 100h
        jge lzss_match
        cmp word ptr DGROUP:match_interrupted, 0
        jne lzss_match
        jmp lzss_more
lzss_match:
        cmp word ptr DGROUP:match_interrupted, 0
        jne lzss_match_resume
        jmp _decode_position
lzss_position:
        mov dx, word ptr DGROUP:lzss_ring_pos
        sub dx, ax
        dec dx
        and dx, 0fffh
        mov word ptr DGROUP:match_position, dx
        mov ax, di
        add ax, 0ff03h
        mov word ptr DGROUP:match_length, ax
        mov word ptr DGROUP:match_progress, 0
lzss_match_resume:
        mov word ptr DGROUP:match_interrupted, 0
        jmp short lzss_copy_loop
lzss_copy:
        mov ax, word ptr DGROUP:match_position
        add ax, word ptr DGROUP:match_progress
        and ax, 0fffh
        les bx, dword ptr DGROUP:lzss_ring_off
        add bx, ax
        mov al, byte ptr es:[bx]
        mov ah, 0
        mov di, ax
        push di
        call _emit_byte
        add sp, 2
        mov si, ax
        les bx, dword ptr DGROUP:lzss_ring_off
        add bx, word ptr DGROUP:lzss_ring_pos
        mov ax, di
        mov byte ptr es:[bx], al
        inc word ptr DGROUP:lzss_ring_pos
        and word ptr DGROUP:lzss_ring_pos, 0fffh
        add word ptr DGROUP:lzss_count_lo, 1
        adc word ptr DGROUP:lzss_count_hi, 0
        inc word ptr DGROUP:match_progress
        or si, si
        jne lzss_copy_loop
        mov word ptr DGROUP:match_interrupted, 1
        xor ax, ax
        jmp short lzss_return
lzss_copy_loop:
        mov ax, word ptr DGROUP:match_progress
        cmp ax, word ptr DGROUP:match_length
        jl lzss_copy
lzss_more:
        mov ax, word ptr DGROUP:lzss_count_hi
        cmp ax, word ptr DGROUP:lzss_size_hi
        jge lzss_more_low
        jmp lzss_next
lzss_more_low:
        jne lzss_done
        mov ax, word ptr DGROUP:lzss_count_lo
        cmp ax, word ptr DGROUP:lzss_size_lo
        jae lzss_done
        jmp lzss_next
lzss_done:
        xor ax, ax
lzss_return:
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

struct engine_bit_buffer g_engine_bit_buffer;

/*
 * **The Huffman coder's position tables**, DGROUP 0x3603..0x3686: three zero
 * bytes, then sixty-four code lengths and sixty-four codes - the shape of
 * LZHUF's `p_len` and `p_code`, which are the *encoder's* half, next to the
 * decoder's `g_engine_huffman_positions`. **Not established** that anything
 * reads them.
 */
struct engine_huffman_codes {
    uint8_t   pad_3603[3];        /* +0x00 */
    uint8_t   len[64];            /* +0x03  0x3606 */
    uint8_t   code[64];           /* +0x43  0x3646 */
} PACKED;

struct engine_huffman_codes g_engine_huffman_codes = {
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

struct engine_huffman_positions g_engine_huffman_positions = {
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

struct engine_match_resume g_engine_match_resume;

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

struct engine_lzss_state g_engine_lzss_state;

/*
 * **The son table's far pointer**, DGROUP 0x5900..0x5904. Two words that are
 * one pointer - the offset at 0x5900 and the segment at 0x5902, a far
 * pointer's own order - filed by `huffman_start` beside the other two tables
 * it caches at 0x590a and 0x590e.
 */
struct engine_huffman_tree {
    uint16_t far *son;          /* +0x00 [4] */
} PACKED;

struct engine_huffman_tree g_engine_huffman_tree;

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

struct engine_decompress_cache g_engine_decompress_cache;

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
    struct resource *rec = g_engine_stream.rec;

    g_engine_decompress_cache.lzss_ready = 0;
    g_engine_bit_buffer.bits = 0;
    g_engine_bit_buffer.bit_count = 0;

    g_engine_decompress_cache.cache_c = rec->scratch;

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

    if (g_engine_bit_buffer.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - g_engine_bit_buffer.bit_count));
        g_engine_bit_buffer.bits = (int16_t)(((uint16_t)g_engine_bit_buffer.bits) | ax);
        g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count + 8);
    }

    si = g_engine_bit_buffer.bits;
    g_engine_bit_buffer.bits = (int16_t)(((uint16_t)g_engine_bit_buffer.bits) << 1);
    g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count - 1);

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

    while (g_engine_bit_buffer.bit_count <= 8) {
        uint16_t ax = (uint16_t)(next_input_byte() & 0xff);

        ax = (uint16_t)(ax << (8 - g_engine_bit_buffer.bit_count));
        g_engine_bit_buffer.bits = (int16_t)(((uint16_t)g_engine_bit_buffer.bits) | ax);
        g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count + 8);
    }

    si = ((uint16_t)g_engine_bit_buffer.bits);
    g_engine_bit_buffer.bits = (int16_t)(si << 8);
    g_engine_bit_buffer.bit_count = (uint8_t)(g_engine_bit_buffer.bit_count - 8);

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
    struct resource *rec = g_engine_stream.rec;
    uint16_t far *freq, far *prnt, far *son;
    int16_t i, j;

    /* Three places inside the scratch block, each in the block's own
       segment - the offset steps and the segment does not. */
    {
        uint8_t far *scratch = rec->scratch;

        g_engine_decompress_cache.cache_a = (uint16_t far *)(scratch + 0x103b);
        g_engine_decompress_cache.cache_b = (uint16_t far *)(scratch + 0x1523);
        g_engine_huffman_tree.son         = (uint16_t far *)(scratch + 0x1c7d);
    }

    freq = g_engine_decompress_cache.cache_a;
    prnt = g_engine_decompress_cache.cache_b;
    son  = g_engine_huffman_tree.son;

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
    uint16_t *freq = g_engine_decompress_cache.cache_a;
    uint16_t *prnt = g_engine_decompress_cache.cache_b;
    uint16_t *son = g_engine_huffman_tree.son;
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
    uint16_t *freq = g_engine_decompress_cache.cache_a;
    uint16_t *prnt = g_engine_decompress_cache.cache_b;
    uint16_t *son = g_engine_huffman_tree.son;

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
    uint16_t high = (uint16_t)(g_engine_huffman_positions.high[si] << 6);
    int16_t n = (int16_t)(g_engine_huffman_positions.len[si] - 2);

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
    uint8_t far * ring = g_engine_decompress_cache.cache_c;
    uint16_t di = 0;
    int16_t si;

    if (g_engine_decompress_cache.lzss_ready == 0) {
        struct resource *rec;
        int16_t i;

        g_engine_match_resume.interrupted = 0;
        huffman_start();

        for (i = 0; i < 0xfc4; i++)
            ring[i] = 0x20;

        g_engine_lzss_state.ring_pos = 0xfc4;
        g_engine_lzss_state.count = 0;

        rec = g_engine_stream.rec;
        g_engine_lzss_state.size = rec->size;
        g_engine_decompress_cache.lzss_ready = 1;
    }

    for (;;) {
        /* 0x1e91d - is there still something to produce? */
        if (g_engine_lzss_state.count >= g_engine_lzss_state.size)
            return 0;

        if (g_engine_match_resume.interrupted == 0) {
            /* 0x1e52d - one symbol, walked out of the tree bit by bit. */
            const uint16_t *son = g_engine_huffman_tree.son;

            di = son[0x272];          /* the root */
            while (di < 0x273)
                di = son[di + huff_get_bit()];

            di -= 0x273;
            huffman_update(di);

            if (di < 0x100) {
                /* 0x1e849 - a literal. */
                si = emit_byte(di);

                ring[g_engine_lzss_state.ring_pos] = (uint8_t)di;
                g_engine_lzss_state.ring_pos = (int16_t)((g_engine_lzss_state.ring_pos + 1) & 0xfff);
                g_engine_lzss_state.count = (int32_t)((uint32_t)g_engine_lzss_state.count + 1);

                if (si == 0)
                    return 0;
                continue;
            }

            /* 0x1e89c - a match. */
            {
                uint16_t pos = (uint16_t)decode_position();

                g_engine_match_resume.position = (int16_t)((g_engine_lzss_state.ring_pos - pos - 1) & 0xfff);
                g_engine_match_resume.length = (int16_t)(di + 0xff03);
                g_engine_match_resume.progress = 0;
            }
        }

        g_engine_match_resume.interrupted = 0;

        while (g_engine_match_resume.progress < g_engine_match_resume.length) {
            uint16_t b = ring[(((uint16_t)g_engine_match_resume.position)
                               + ((uint16_t)g_engine_match_resume.progress))
                              & 0xfff];

            si = emit_byte(b);

            ring[g_engine_lzss_state.ring_pos] = (uint8_t)b;
            g_engine_lzss_state.ring_pos = (int16_t)((g_engine_lzss_state.ring_pos + 1) & 0xfff);
            g_engine_lzss_state.count = (int32_t)((uint32_t)g_engine_lzss_state.count + 1);

            g_engine_match_resume.progress = (int16_t)(((uint16_t)g_engine_match_resume.progress) + 1);

            if (si == 0) {
                g_engine_match_resume.interrupted = 1;
                return 0;
            }
        }
    }
}

#endif
