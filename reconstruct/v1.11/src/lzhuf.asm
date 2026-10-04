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
; JUDGE: structs resource=res
; JUDGE: assembler bc3.00

DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
_BSS segment word public 'BSS'
match_interrupted db 2 dup (?)
match_position db 2 dup (?)
match_length db 2 dup (?)
match_progress db 2 dup (?)
lzss_ring_pos db 2 dup (?)
lzss_count_lo db 2 dup (?)
lzss_count_hi db 2 dup (?)
lzss_size_lo db 2 dup (?)
lzss_size_hi db 2 dup (?)
huff_son_off db 2 dup (?)
huff_son_seg db 2 dup (?)
huff_prnt_off db 2 dup (?)
huff_prnt_seg db 2 dup (?)
huff_freq_off db 2 dup (?)
huff_freq_seg db 2 dup (?)
lzss_ready db 2 dup (?)
lzss_ring_off db 2 dup (?)
lzss_ring_seg db 2 dup (?)
_BSS ends

_DATA segment word public 'DATA'
lzh_getbuf label byte
	db 0h, 0h
lzh_getlen label byte
	db 0h, 3h, 4h, 4h, 4h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 5h, 6h, 6h, 6h
	db 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
	db 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h, 7h
	db 7h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h, 8h
	db 8h, 0h, 20h, 30h, 40h, 50h, 58h, 60h, 68h, 70h, 78h, 80h, 88h, 90h, 94h, 98h
	db 9ch, 0a0h, 0a4h, 0a8h, 0ach, 0b0h, 0b4h, 0b8h, 0bch, 0c0h, 0c2h, 0c4h, 0c6h, 0c8h, 0cah, 0cch
	db 0ceh, 0d0h, 0d2h, 0d4h, 0d6h, 0d8h, 0dah, 0dch, 0deh, 0e0h, 0e2h, 0e4h, 0e6h, 0e8h, 0eah, 0ech
	db 0eeh, 0f0h, 0f1h, 0f2h, 0f3h, 0f4h, 0f5h, 0f6h, 0f7h, 0f8h, 0f9h, 0fah, 0fbh, 0fch, 0fdh, 0feh
	db 0ffh
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

extrn _g_stream_rec:byte
LZHUF_TEXT segment byte public 'CODE'
assume cs:LZHUF_TEXT, ds:DGROUP
extrn _emit_byte:near
extrn _next_input_byte:near
public _huff_get_bit, _decode_char, _lzss_reset, _huff_get_byte
public _huffman_start, _huffman_reconst, _decode_position, _decompress_lzss

; 0x20015
_huff_get_bit proc near
	cmp byte ptr DGROUP:lzh_getlen, 8
	ja bit_take
	call _next_input_byte
	xor ah, ah
	mov cl, 8
	sub cl, byte ptr DGROUP:lzh_getlen
	shl ax, cl
	or word ptr DGROUP:lzh_getbuf, ax
	add byte ptr DGROUP:lzh_getlen, 8
bit_take:
	mov ax, word ptr DGROUP:lzh_getbuf
	shl word ptr DGROUP:lzh_getbuf, 1
	dec byte ptr DGROUP:lzh_getlen
	cwd
	neg dx
	mov ax, dx
	ret
_huff_get_bit endp

; 0x20043
_decode_char proc near
	push si
	push di
	les di, dword ptr DGROUP:huff_son_off
	mov si, es:[di+4e4h]
	cmp si, 273h
	jae char_leaf
char_walk:
	cmp byte ptr DGROUP:lzh_getlen, 8
	ja char_bit
	call _next_input_byte
	xor ah, ah
	mov cl, 8
	sub cl, byte ptr DGROUP:lzh_getlen
	shl ax, cl
	or word ptr DGROUP:lzh_getbuf, ax
	add byte ptr DGROUP:lzh_getlen, 8
char_bit:
	mov ax, word ptr DGROUP:lzh_getbuf
	shl word ptr DGROUP:lzh_getbuf, 1
	dec byte ptr DGROUP:lzh_getlen
	cwd
	neg dx
	mov bx, dx
	add bx, si
	shl bx, 1
	mov si, es:[bx+di]
	cmp si, 273h
	jb char_walk
char_leaf:
	sub si, 273h
	push bp
	push si
	push ds
	mov bp, si
	shl bp, 1
	lds si, dword ptr DGROUP:huff_freq_off
	cmp word ptr [si+4e4h], 8000h
	jne char_update
	push ds
	mov ax, ss
	mov ds, ax
	call _huffman_reconst
	pop ds
char_update:
	mov di, ss:[54e4h]
	mov bp, ds:[bp+di+4e6h]
	shl bp, 1
update_count:
	inc word ptr ds:[bp+si]
	mov ax, ds:[bp+si]
	cmp ax, ds:[bp+si+2]
	jbe update_parent
	mov bx, bp
update_find_slot:
	add bx, 2
	cmp [bx+si+2], ax
	jb update_find_slot
	mov cx, [bx+si]
	mov ds:[bp+si], cx
	mov [bx+si], ax
	mov si, ss:[54e0h]
	mov ax, ds:[bp+si]
	shl ax, 1
	shr bx, 1
	xchg bx, ax
	mov [bx+di], ax
	cmp bx, 4e6h
	jge update_son_prnt
	mov [bx+di+2], ax
update_son_prnt:
	shl ax, 1
	shr bx, 1
	xchg bx, ax
	mov cx, [bx+si]
	mov [bx+si], ax
	mov dx, bx
	mov bx, cx
	shl bx, 1
	shr bp, 1
	mov [bx+di], bp
	cmp bx, 4e6h
	jge update_prnt_done
	mov [bx+di+2], bp
update_prnt_done:
	shl bp, 1
	mov ds:[bp+si], cx
	mov bp, dx
	mov si, ss:[54e8h]
update_parent:
	mov bp, ds:[bp+di]
	or bp, bp
	je update_done
	shl bp, 1
	jmp short update_count
update_done:
	pop ds
	pop si
	pop bp
	mov ax, si
	pop di
	pop si
	ret
_decode_char endp

; 0x2012a
_lzss_reset proc near
	push bp
	mov bp, sp
	mov word ptr DGROUP:lzss_ready, 0
	mov word ptr DGROUP:lzh_getbuf, 0
	mov byte ptr DGROUP:lzh_getlen, 0
	mov bx, word ptr DGROUP:_g_stream_rec
	mov ax, [bx+res_scratch+2]
	mov dx, [bx+res_scratch]
	mov word ptr DGROUP:lzss_ring_seg, ax
	mov word ptr DGROUP:lzss_ring_off, dx
	xor ax, ax
	jmp short reset_return
reset_return:
	pop bp
	ret
_lzss_reset endp

; 0x20155
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

; 0x20198
_huffman_start proc near
	push bp
	mov bp, sp
	push si
	push di
	mov bx, word ptr DGROUP:_g_stream_rec
	mov di, [bx+res_scratch]
	mov bx, word ptr DGROUP:_g_stream_rec
	mov ax, [bx+res_scratch+2]
	mov word ptr DGROUP:huff_son_seg, ax
	mov word ptr DGROUP:huff_prnt_seg, ax
	mov word ptr DGROUP:huff_freq_seg, ax
	mov ax, di
	add ax, 103ch
	mov word ptr DGROUP:huff_freq_off, ax
	mov ax, di
	add ax, 1524h
	mov word ptr DGROUP:huff_prnt_off, ax
	mov ax, di
	add ax, 1c7eh
	mov word ptr DGROUP:huff_son_off, ax
	xor cx, cx
	jmp short start_leaves
start_leaf:
	mov ax, cx
	shl ax, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, ax
	mov word ptr es:[bx], 1
	mov ax, cx
	shl ax, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, ax
	mov ax, cx
	add ax, 273h
	mov es:[bx], ax
	mov ax, cx
	add ax, 273h
	shl ax, 1
	les bx, dword ptr DGROUP:huff_prnt_off
	add bx, ax
	mov es:[bx], cx
	inc cx
start_leaves:
	cmp cx, 13ah
	jl start_leaf
	xor cx, cx
	mov si, 13ah
	jmp short start_nodes
start_node:
	mov ax, cx
	shl ax, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, ax
	mov ax, es:[bx]
	mov dx, cx
	inc dx
	shl dx, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, dx
	add ax, es:[bx]
	mov dx, si
	shl dx, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, dx
	mov es:[bx], ax
	mov ax, si
	shl ax, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, ax
	mov es:[bx], cx
	mov ax, cx
	inc ax
	shl ax, 1
	les bx, dword ptr DGROUP:huff_prnt_off
	add bx, ax
	mov ax, si
	mov es:[bx], ax
	mov dx, cx
	shl dx, 1
	les bx, dword ptr DGROUP:huff_prnt_off
	add bx, dx
	mov es:[bx], ax
	add cx, 2
	inc si
start_nodes:
	cmp si, 272h
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

; 0x20285
_huffman_reconst proc near
	push bp
	mov bp, sp
	sub sp, 6
	push si
	push di
	xor di, di
	xor cx, cx
	jmp short rebuild_collect_loop
rebuild_collect:
	mov ax, cx
	shl ax, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, ax
	cmp word ptr es:[bx], 273h
	jl rebuild_collect_next
	mov ax, cx
	shl ax, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, ax
	mov ax, es:[bx]
	inc ax
	shr ax, 1
	mov dx, di
	shl dx, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, dx
	mov es:[bx], ax
	mov ax, cx
	shl ax, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, ax
	mov ax, es:[bx]
	mov dx, di
	shl dx, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, dx
	mov es:[bx], ax
	inc di
rebuild_collect_next:
	inc cx
rebuild_collect_loop:
	cmp cx, 273h
	jl rebuild_collect
	xor cx, cx
	mov di, 13ah
	jmp rebuild_node_loop
rebuild_node:
	mov ax, cx
	inc ax
	mov si, ax
	mov ax, cx
	shl ax, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, ax
	mov ax, es:[bx]
	mov dx, si
	shl dx, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, dx
	add ax, es:[bx]
	mov dx, di
	shl dx, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, dx
	mov es:[bx], ax
	mov [bp-4], ax
	mov ax, di
	dec ax
	mov si, ax
	jmp short rebuild_find
rebuild_find_back:
	dec si
rebuild_find:
	mov ax, si
	shl ax, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, ax
	mov ax, es:[bx]
	cmp ax, [bp-4]
	ja rebuild_find_back
	inc si
	mov ax, di
	sub ax, si
	shl ax, 1
	mov [bp-6], ax
	mov ax, [bp-6]
	dec ax
	mov [bp-2], ax
	jmp short rebuild_shift_loop
rebuild_shift:
	mov ax, si
	add ax, [bp-2]
	shl ax, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, ax
	mov ax, es:[bx]
	mov dx, si
	add dx, [bp-2]
	inc dx
	shl dx, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, dx
	mov es:[bx], ax
	mov ax, si
	add ax, [bp-2]
	shl ax, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, ax
	mov ax, es:[bx]
	mov dx, si
	add dx, [bp-2]
	inc dx
	shl dx, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, dx
	mov es:[bx], ax
	dec word ptr [bp-2]
rebuild_shift_loop:
	cmp word ptr [bp-2], 0
	jge rebuild_shift
	mov ax, si
	shl ax, 1
	les bx, dword ptr DGROUP:huff_freq_off
	add bx, ax
	mov ax, [bp-4]
	mov es:[bx], ax
	mov ax, si
	shl ax, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, ax
	mov es:[bx], cx
	add cx, 2
	inc di
rebuild_node_loop:
	cmp di, 273h
	jge rebuild_links
	jmp rebuild_node
rebuild_links:
	xor cx, cx
	jmp short rebuild_link_loop
rebuild_link:
	mov ax, cx
	shl ax, 1
	les bx, dword ptr DGROUP:huff_son_off
	add bx, ax
	mov ax, es:[bx]
	mov si, ax
	cmp ax, 273h
	jl rebuild_link_node
	mov ax, si
	shl ax, 1
	les bx, dword ptr DGROUP:huff_prnt_off
	add bx, ax
	mov es:[bx], cx
	jmp short rebuild_link_next
rebuild_link_node:
	mov ax, si
	inc ax
	shl ax, 1
	les bx, dword ptr DGROUP:huff_prnt_off
	add bx, ax
	mov ax, cx
	mov es:[bx], ax
	mov dx, si
	shl dx, 1
	les bx, dword ptr DGROUP:huff_prnt_off
	add bx, dx
	mov es:[bx], ax
rebuild_link_next:
	inc cx
rebuild_link_loop:
	cmp cx, 273h
	jl rebuild_link
	pop di
	pop si
	mov sp, bp
	pop bp
	ret
_huffman_reconst endp

; 0x2040d
_decode_position proc near
	push bp
	mov bp, sp
	sub sp, 2
	push si
	push di
	call _huff_get_byte
	mov si, ax
	mov al, byte ptr lzh_d_code[si]
	mov ah, 0
	mov cl, 6
	shl ax, cl
	mov [bp-2], ax
	mov al, byte ptr lzh_d_len[si]
	mov ah, 0
	mov di, ax
	sub di, 2
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
	mov ax, [bp-2]
	pop dx
	or ax, dx
	jmp short position_return
position_return:
	pop di
	pop si
	mov sp, bp
	pop bp
	ret
_decode_position endp

; 0x2045a
_decompress_lzss proc near
	push bp
	mov bp, sp
	sub sp, 2
	push si
	cmp word ptr DGROUP:lzss_ready, 0
	jne lzss_go
	mov word ptr DGROUP:match_interrupted, 0
	call _huffman_start
	mov word ptr DGROUP:match_position, 0
	jmp short lzss_ring_clear_loop
lzss_ring_clear:
	les bx, dword ptr DGROUP:lzss_ring_off
	add bx, word ptr DGROUP:match_position
	mov byte ptr es:[bx], 20h
	inc word ptr DGROUP:match_position
lzss_ring_clear_loop:
	cmp word ptr DGROUP:match_position, 0fc4h
	jl lzss_ring_clear
	mov word ptr DGROUP:lzss_ring_pos, 0fc4h
	mov word ptr DGROUP:lzss_count_hi, 0
	mov word ptr DGROUP:lzss_count_lo, 0
	mov bx, word ptr DGROUP:_g_stream_rec
	mov ax, [bx+res_size+2]
	mov dx, [bx+res_size]
	mov word ptr DGROUP:lzss_size_hi, ax
	mov word ptr DGROUP:lzss_size_lo, dx
	mov word ptr DGROUP:lzss_ready, 1
lzss_go:
	jmp lzss_more
lzss_next:
	cmp word ptr DGROUP:match_interrupted, 0
	jne lzss_not_literal
	call _decode_char
	mov [bp-2], ax
	cmp word ptr [bp-2], 100h
	jge lzss_not_literal
	push word ptr [bp-2]
	call _emit_byte
	pop cx
	mov si, ax
	les bx, dword ptr DGROUP:lzss_ring_off
	add bx, word ptr DGROUP:lzss_ring_pos
	mov al, [bp-2]
	mov es:[bx], al
	inc word ptr DGROUP:lzss_ring_pos
	and word ptr DGROUP:lzss_ring_pos, 0fffh
	add word ptr DGROUP:lzss_count_lo, 1
	adc word ptr DGROUP:lzss_count_hi, 0
	or si, si
	jne lzss_not_literal
lzss_stop:
	xor ax, ax
	jmp lzss_return
lzss_not_literal:
	cmp word ptr [bp-2], 100h
	jge lzss_match
	cmp word ptr DGROUP:match_interrupted, 0
	jne lzss_match
	jmp lzss_more
lzss_match:
	cmp word ptr DGROUP:match_interrupted, 0
	jne lzss_match_resume
	call _decode_position
	mov dx, word ptr DGROUP:lzss_ring_pos
	sub dx, ax
	dec dx
	and dx, 0fffh
	mov word ptr DGROUP:match_position, dx
	mov ax, [bp-2]
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
	mov al, es:[bx]
	mov ah, 0
	mov [bp-2], ax
	push word ptr [bp-2]
	call _emit_byte
	pop cx
	mov si, ax
	les bx, dword ptr DGROUP:lzss_ring_off
	add bx, word ptr DGROUP:lzss_ring_pos
	mov al, [bp-2]
	mov es:[bx], al
	inc word ptr DGROUP:lzss_ring_pos
	and word ptr DGROUP:lzss_ring_pos, 0fffh
	add word ptr DGROUP:lzss_count_lo, 1
	adc word ptr DGROUP:lzss_count_hi, 0
	inc word ptr DGROUP:match_progress
	or si, si
	jne lzss_copy_loop
	mov word ptr DGROUP:match_interrupted, 1
	jmp lzss_stop
lzss_copy_loop:
	mov ax, word ptr DGROUP:match_progress
	cmp ax, word ptr DGROUP:match_length
	jl lzss_copy
lzss_more:
	mov ax, word ptr DGROUP:lzss_count_hi
	mov dx, word ptr DGROUP:lzss_count_lo
	cmp ax, word ptr DGROUP:lzss_size_hi
	jge lzss_more_low
	jmp lzss_next
lzss_more_low:
	jne lzss_done
	cmp dx, word ptr DGROUP:lzss_size_lo
	jae lzss_done
	jmp lzss_next
lzss_done:
	jmp lzss_stop
lzss_return:
	pop si
	mov sp, bp
	pop bp
	ret
_decompress_lzss endp
LZHUF_TEXT ends
end
