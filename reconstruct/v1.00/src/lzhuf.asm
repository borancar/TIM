; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `lzhuf.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `lzhuf.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `lzhuf.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs resource=res engine_stream=strm
; JUDGE: assembler bc3.00

DGROUP group _DATA,_BSS
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

; 0x1dba8
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

; 0x1dc15
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

; 0x1dc3a
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

; 0x1dc72
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

; 0x1de51
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

; 0x1dfd6
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

; 0x1e00b
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

; 0x1e04e
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

; 0x1e0b3
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

; 0x1e1af
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

; 0x1e338
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

; 0x1e445
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

; 0x1e4a7
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

; 0x1e4e7
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

; 0x1e561
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

; 0x1e5ae
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

; 0x1e7f2
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
end
