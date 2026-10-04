; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `trig.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `trig.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py with DS taken
; as the code segment (`--ds-is-cs`): the routines that work in 48 bits keep
; their scratch words there, 0x1681 and on, inside the zeroed run after
; `arctan_lookup`. The two tables are the same bytes as the C arrays in
; `trig.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: assembler bc3.00

_DATA segment word public 'DATA'
_DATA ends
_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
TRIG_TEXT segment byte public 'CODE'
assume cs:TRIG_TEXT, ds:TRIG_TEXT
public _long_mul_div, _scale_record_a, _scale_record_b, _mul16x16
public _mul_48, _angle_sin, _angle_cos, _arctan_ratio
public _arctan_lookup

; 0x2bb94
_long_mul_div proc near
	push bp
	mov bp, sp
	sub sp, 10h
	push si
	push di
	mov ax, [bp+0ch]
	mov dx, [bp+0eh]
	or dx, dx
	jns lmd_a_positive
	neg ax
	adc dx, 0
	neg dx
	mov [bp+0ch], ax
	mov [bp+0eh], dx
lmd_a_positive:
	mov bx, [bp+10h]
	mov cx, [bp+12h]
	or cx, cx
	jns lmd_b_positive
	neg bx
	adc cx, 0
	neg cx
	mov [bp+10h], bx
	mov [bp+12h], cx
lmd_b_positive:
	mov si, dx
	xor si, cx
	mov [bp-0ah], si
	cmp ax, bx
	jne lmd_divide
	cmp cx, dx
	jne lmd_divide
	or si, si
	je lmd_return
	mov ax, [bp+4]
	mov dx, [bp+6]
	neg ax
	adc dx, 0
	neg dx
	mov [bp+4], ax
	mov [bp+6], dx
	mov ax, [bp+8]
	mov dx, [bp+0ah]
	neg ax
	adc dx, 0
	neg dx
	mov [bp+8], ax
	mov [bp+0ah], dx
lmd_return:
	pop di
	pop si
	add sp, 10h
	pop bp
	ret
lmd_divide:
	or dx, dx
	jne lmd_normalise_bytes
	xchg dx, ax
	xchg bx, cx
	jmp short lmd_reciprocal
lmd_normalise_bytes:
	or dh, dh
	jne lmd_normalised
	mov ch, cl
	mov cl, bh
	mov bh, bl
	mov dh, dl
	mov dl, ah
	mov ah, al
	or dx, dx
	jmp short lmd_normalised
lmd_normalise_bit:
	shl bx, 1
	rcl cx, 1
	shl ax, 1
	rcl dx, 1
	or dx, dx
lmd_normalised:
	jns lmd_normalise_bit
lmd_reciprocal:
	mov ax, bx
	xchg dx, cx
	cmp cx, dx
	jbe lmd_return
	div cx
	mov [bp-2], ax
	mov [bp-6], ax
	xor ax, ax
	div cx
	mov [bp-4], ax
	mov [bp-8], ax
	mov ax, [bp+4]
	mov dx, [bp+6]
	or dx, dx
	jne lmd_x_nonzero
	or ax, ax
	je lmd_x_sign
lmd_x_nonzero:
	or dx, dx
	jns lmd_x_positive
	neg ax
	adc dx, 0
	neg dx
lmd_x_positive:
	mov bx, [bp-2]
	or bx, bx
	je lmd_x_short
	or dx, dx
	jne lmd_x_long
lmd_x_short:
	or dx, dx
	jne lmd_x_short_mul
	xchg ax, word ptr [bp - 4]
	xchg dx, word ptr [bp - 2]
lmd_x_short_mul:
	xchg dx, ax
	mul word ptr [bp-4]
	xor ax, ax
	xchg dx, ax
	jmp short lmd_x_sign
lmd_x_long:
	mov cx, dx
	mul word ptr [bp-2]
	mov si, ax
	mov di, dx
	mov ax, cx
	mul word ptr [bp-4]
	add si, ax
	mov si, 0
	adc di, dx
	adc si, 0
	mov ax, cx
	mul word ptr [bp-2]
	add ax, di
	adc dx, si
lmd_x_sign:
	mov bx, [bp+6]
	xor bx, [bp-0ah]
	jns lmd_y
	neg ax
	adc dx, 0
	neg dx
lmd_y:
	mov [bp+4], ax
	mov [bp+6], dx
	mov ax, [bp+8]
	mov dx, [bp+0ah]
	or dx, dx
	jne lmd_y_nonzero
	or ax, ax
	je lmd_y_sign
lmd_y_nonzero:
	or dx, dx
	jns lmd_y_positive
	neg ax
	adc dx, 0
	neg dx
lmd_y_positive:
	mov bx, [bp-6]
	or bx, bx
	je lmd_y_short
	or dx, dx
	jne lmd_y_long
lmd_y_short:
	or dx, dx
	jne lmd_y_short_mul
	xchg ax, word ptr [bp - 8]
	xchg dx, word ptr [bp - 6]
lmd_y_short_mul:
	xchg dx, ax
	mul word ptr [bp-8]
	xor ax, ax
	xchg dx, ax
	jmp short lmd_y_sign
lmd_y_long:
	mov cx, dx
	mul word ptr [bp-6]
	mov si, ax
	mov di, dx
	mov ax, cx
	mul word ptr [bp-8]
	add si, ax
	mov si, 0
	adc di, dx
	adc si, 0
	mov ax, cx
	mul word ptr [bp-6]
	add ax, di
	adc dx, si
lmd_y_sign:
	mov bx, [bp+0ah]
	xor bx, [bp-0ah]
	jns lmd_y_store
	neg ax
	adc dx, 0
	neg dx
lmd_y_store:
	mov [bp+8], ax
	mov [bp+0ah], dx
	pop di
	pop si
	add sp, 10h
	pop bp
	ret
unreached_far_routine label byte
	db 8bh, 0dch, 36h, 8bh, 5fh, 4h, 2eh, 0a1h, 0a0h, 9h, 99h, 1h, 7h, 11h, 57h, 2h
	db 2eh, 0a1h, 0a2h, 9h, 99h, 2bh, 47h, 4h, 1bh, 57h, 6h, 89h, 47h, 4h, 89h, 57h, 6h
	db 8bh, 0c3h, 0cbh
_long_mul_div endp

; 0x2bd52
_scale_record_a proc far
	push bp
	mov bp, sp
	push si
	mov si, [bp+6]
	mov bp, [bp+8]
	xor ax, ax
	push ax
	push word ptr cs:scale_divisor
	push word ptr [si+6]
	push word ptr [si+4]
	push word ptr [si+0ah]
	push word ptr [si+8]
	push word ptr [si+2]
	push word ptr [si]
	call _long_mul_div
	pop word ptr [bp]
	pop word ptr [bp+2]
	pop word ptr [bp+4]
	pop word ptr [bp+6]
	add sp, 8
	mov ax, bp
	pop si
	pop bp
	retf
_scale_record_a endp

; 0x2bd8c
_scale_record_b proc far
	push bp
	mov bp, sp
	push si
	mov si, [bp+6]
	xor ax, ax
	push ax
	push word ptr cs:scale_divisor
	push word ptr [si+6]
	push word ptr [si+4]
	push ax
	push ax
	push word ptr [si+2]
	push word ptr [si]
	call _long_mul_div
	pop ax
	pop dx
	add sp, 0ch
	pop si
	pop bp
	retf
_scale_record_b endp

; 0x2bdb3
_mul16x16 proc far
	mov bx, sp
	mov ax, [bx+4]
	imul word ptr [bx+6]
	retf
_mul16x16 endp

; 0x2bdbc
_mul_48 proc near
	push bx
	push si
	mov word ptr m48_x, ax
	mov word ptr m48_y, cx
	mov word ptr m48_z, dx
	mov si, bx
	mov dx, [si]
	mov bx, [si+2]
	mov ax, word ptr m48_x
	push di
	xor di, di
	or bx, bx
	jge m48_a_positive
	or di, 1
	neg bx
	neg dx
	sbb bx, 0
m48_a_positive:
	or ax, ax
	jge m48_a_mul
	xor di, 1
	neg ax
m48_a_mul:
	mov cx, ax
	mul dx
	xchg bx, dx
	xchg cx, ax
	mul dx
	add ax, bx
	adc dx, 0
	test di, 1
	je m48_a_done
	not cx
	not ax
	not dx
	add cx, 1
	adc ax, 0
	adc dx, 0
m48_a_done:
	pop di
	mov word ptr m48_sum_hi, dx
	mov word ptr m48_sum_mid, ax
	mov word ptr m48_sum_lo, cx
	mov dx, [si+4]
	mov bx, [si+6]
	mov ax, word ptr m48_y
	push di
	xor di, di
	or bx, bx
	jge m48_b_positive
	or di, 1
	neg bx
	neg dx
	sbb bx, 0
m48_b_positive:
	or ax, ax
	jge m48_b_mul
	xor di, 1
	neg ax
m48_b_mul:
	mov cx, ax
	mul dx
	xchg bx, dx
	xchg cx, ax
	mul dx
	add ax, bx
	adc dx, 0
	test di, 1
	je m48_b_done
	not cx
	not ax
	not dx
	add cx, 1
	adc ax, 0
	adc dx, 0
m48_b_done:
	pop di
	add cx, word ptr m48_sum_lo
	adc ax, word ptr m48_sum_mid
	adc dx, word ptr m48_sum_hi
	mov bx, word ptr m48_z
	or bx, bx
	je m48_return
	mov word ptr m48_sum_hi, dx
	mov word ptr m48_sum_mid, ax
	mov word ptr m48_sum_lo, cx
	mov ax, bx
	mov dx, [si+8]
	mov bx, [si+0ah]
	push di
	xor di, di
	or bx, bx
	jge m48_c_positive
	or di, 1
	neg bx
	neg dx
	sbb bx, 0
m48_c_positive:
	or ax, ax
	jge m48_c_mul
	xor di, 1
	neg ax
m48_c_mul:
	mov cx, ax
	mul dx
	xchg bx, dx
	xchg cx, ax
	mul dx
	add ax, bx
	adc dx, 0
	test di, 1
	je m48_c_done
	not cx
	not ax
	not dx
	add cx, 1
	adc ax, 0
	adc dx, 0
m48_c_done:
	pop di
	add cx, word ptr m48_sum_lo
	adc ax, word ptr m48_sum_mid
	adc dx, word ptr m48_sum_hi
m48_return:
	pop si
	pop bx
	ret
unreached_rows label byte
	db 56h, 57h, 8bh, 0f3h, 8bh, 0f9h, 8bh, 0d8h, 8ah, 44h, 12h, 0ah, 0c0h, 75h, 10h, 6h
	db 8ch, 0d8h, 8eh, 0c0h, 8bh, 0f3h, 0b9h, 6h, 0h, 0f3h, 0a5h, 7h, 0e9h, 0a6h, 0h
unreached_rows_scaled:
	dec al
	je unreached_rows_two
	jmp short unreached_rows_three
unreached_rows_two:
	mov ax, [si]
	mov cx, [si+6]
	mov dx, [si+0ch]
	call _mul_48
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	mov [di], ax
	mov [di+2], dx
	mov ax, [si+2]
	mov cx, [si+8]
	mov dx, [si+0eh]
	call _mul_48
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	mov [di+4], ax
	mov [di+6], dx
	mov ax, [bx+8]
	mov dx, [bx+0ah]
	mov [di+8], ax
	mov [di+0ah], dx
	jmp short unreached_rows_return
unreached_rows_three:
	mov ax, [si]
	mov cx, [si+6]
	mov dx, [si+0ch]
	call _mul_48
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	mov [di], ax
	mov [di+2], dx
	mov ax, [si+2]
	mov cx, [si+8]
	mov dx, [si+0eh]
	call _mul_48
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	mov [di+4], ax
	mov [di+6], dx
	mov ax, [si+4]
	mov cx, [si+0ah]
	mov dx, [si+10h]
	call _mul_48
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	shl cx, 1
	rcl ax, 1
	rcl dx, 1
	mov [di+8], ax
	mov [di+0ah], dx
unreached_rows_return:
	pop di
	pop si
	ret
mul_48_pad label byte
	db 0h
_mul_48 endp

; 0x2bfa0
_angle_sin proc far
	mov bx, sp
	mov ax, ss:[bx+4]
	add ax, 0c000h
	shr ax, 1
	shr ax, 1
	shr ax, 1
	shr ax, 1
	test ax, 800h
	je sin_index
	neg ax
	add ax, 1000h
sin_index:
	mov bx, ax
	shl bx, 1
	mov ax, word ptr cs:cosine_table[bx]
	retf
_angle_sin endp

; 0x2bfc5
_angle_cos proc far
	mov bx, sp
	mov ax, ss:[bx+4]
	shr ax, 1
	shr ax, 1
	shr ax, 1
	shr ax, 1
	test ax, 800h
	je cos_index
	neg ax
	add ax, 1000h
cos_index:
	mov bx, ax
	shl bx, 1
	mov ax, word ptr cs:cosine_table[bx]
	retf
arctan_table_pad label byte
	db 0h
arctan_table label byte
	db 0h, 0h, 1h, 0h, 2h, 0h, 3h, 0h, 5h, 0h, 6h, 0h, 7h, 0h, 8h, 0h
	db 0ah, 0h, 0bh, 0h, 0ch, 0h, 0eh, 0h, 0fh, 0h, 10h, 0h, 11h, 0h, 13h, 0h
	db 14h, 0h, 15h, 0h, 16h, 0h, 18h, 0h, 19h, 0h, 1ah, 0h, 1bh, 0h, 1dh, 0h
	db 1eh, 0h, 1fh, 0h, 21h, 0h, 22h, 0h, 23h, 0h, 24h, 0h, 26h, 0h, 27h, 0h
	db 28h, 0h, 29h, 0h, 2bh, 0h, 2ch, 0h, 2dh, 0h, 2fh, 0h, 30h, 0h, 31h, 0h
	db 32h, 0h, 34h, 0h, 35h, 0h, 36h, 0h, 37h, 0h, 39h, 0h, 3ah, 0h, 3bh, 0h
	db 3ch, 0h, 3eh, 0h, 3fh, 0h, 40h, 0h, 41h, 0h, 43h, 0h, 44h, 0h, 45h, 0h
	db 47h, 0h, 48h, 0h, 49h, 0h, 4ah, 0h, 4ch, 0h, 4dh, 0h, 4eh, 0h, 4fh, 0h
	db 51h, 0h, 52h, 0h, 53h, 0h, 54h, 0h, 56h, 0h, 57h, 0h, 58h, 0h, 59h, 0h
	db 5bh, 0h, 5ch, 0h, 5dh, 0h, 5eh, 0h, 60h, 0h, 61h, 0h, 62h, 0h, 63h, 0h
	db 65h, 0h, 66h, 0h, 67h, 0h, 68h, 0h, 6ah, 0h, 6bh, 0h, 6ch, 0h, 6dh, 0h
	db 6eh, 0h, 70h, 0h, 71h, 0h, 72h, 0h, 73h, 0h, 75h, 0h, 76h, 0h, 77h, 0h
	db 78h, 0h, 7ah, 0h, 7bh, 0h, 7ch, 0h, 7dh, 0h, 7eh, 0h, 80h, 0h, 81h, 0h
	db 82h, 0h, 83h, 0h, 85h, 0h, 86h, 0h, 87h, 0h, 88h, 0h, 89h, 0h, 8bh, 0h
	db 8ch, 0h, 8dh, 0h, 8eh, 0h, 90h, 0h, 91h, 0h, 92h, 0h, 93h, 0h, 94h, 0h
	db 96h, 0h, 97h, 0h, 98h, 0h, 99h, 0h, 9ah, 0h, 9ch, 0h, 9dh, 0h, 9eh, 0h
	db 9fh, 0h, 0a0h, 0h, 0a2h, 0h, 0a3h, 0h, 0a4h, 0h, 0a5h, 0h, 0a6h, 0h, 0a8h, 0h
	db 0a9h, 0h, 0aah, 0h, 0abh, 0h, 0ach, 0h, 0aeh, 0h, 0afh, 0h, 0b0h, 0h, 0b1h, 0h
	db 0b2h, 0h, 0b3h, 0h, 0b5h, 0h, 0b6h, 0h, 0b7h, 0h, 0b8h, 0h, 0b9h, 0h, 0bah, 0h
	db 0bch, 0h, 0bdh, 0h, 0beh, 0h, 0bfh, 0h, 0c0h, 0h, 0c1h, 0h, 0c3h, 0h, 0c4h, 0h
	db 0c5h, 0h, 0c6h, 0h, 0c7h, 0h, 0c8h, 0h, 0cah, 0h, 0cbh, 0h, 0cch, 0h, 0cdh, 0h
	db 0ceh, 0h, 0cfh, 0h, 0d0h, 0h, 0d2h, 0h, 0d3h, 0h, 0d4h, 0h, 0d5h, 0h, 0d6h, 0h
	db 0d7h, 0h, 0d8h, 0h, 0dah, 0h, 0dbh, 0h, 0dch, 0h, 0ddh, 0h, 0deh, 0h, 0dfh, 0h
	db 0e0h, 0h, 0e2h, 0h, 0e3h, 0h, 0e4h, 0h, 0e5h, 0h, 0e6h, 0h, 0e7h, 0h, 0e8h, 0h
	db 0e9h, 0h, 0eah, 0h, 0ech, 0h, 0edh, 0h, 0eeh, 0h, 0efh, 0h, 0f0h, 0h, 0f1h, 0h
	db 0f2h, 0h, 0f3h, 0h, 0f4h, 0h, 0f6h, 0h, 0f7h, 0h, 0f8h, 0h, 0f9h, 0h, 0fah, 0h
	db 0fbh, 0h, 0fch, 0h, 0fdh, 0h, 0feh, 0h, 0ffh, 0h, 1h, 1h, 2h, 1h, 3h, 1h
	db 4h, 1h, 5h, 1h, 6h, 1h, 7h, 1h, 8h, 1h, 9h, 1h, 0ah, 1h, 0bh, 1h
	db 0ch, 1h, 0dh, 1h, 0eh, 1h, 10h, 1h, 11h, 1h, 12h, 1h, 13h, 1h, 14h, 1h
	db 15h, 1h, 16h, 1h, 17h, 1h, 18h, 1h, 19h, 1h, 1ah, 1h, 1bh, 1h, 1ch, 1h
	db 1dh, 1h, 1eh, 1h, 1fh, 1h, 20h, 1h, 21h, 1h, 22h, 1h, 23h, 1h, 25h, 1h
	db 26h, 1h, 27h, 1h, 28h, 1h, 29h, 1h, 2ah, 1h, 2bh, 1h, 2ch, 1h, 2dh, 1h
	db 2eh, 1h, 2fh, 1h, 30h, 1h, 31h, 1h, 32h, 1h, 33h, 1h, 34h, 1h, 35h, 1h
	db 36h, 1h, 37h, 1h, 38h, 1h, 39h, 1h, 3ah, 1h, 3bh, 1h, 3ch, 1h, 3dh, 1h
	db 3eh, 1h, 3fh, 1h, 40h, 1h, 41h, 1h, 42h, 1h, 43h, 1h, 44h, 1h, 45h, 1h
	db 46h, 1h, 47h, 1h, 48h, 1h, 49h, 1h, 4ah, 1h, 4bh, 1h, 4ch, 1h, 4dh, 1h
	db 4eh, 1h, 4eh, 1h, 4fh, 1h, 50h, 1h, 51h, 1h, 52h, 1h, 53h, 1h, 54h, 1h
	db 55h, 1h, 56h, 1h, 57h, 1h, 58h, 1h, 59h, 1h, 5ah, 1h, 5bh, 1h, 5ch, 1h
	db 5dh, 1h, 5eh, 1h, 5fh, 1h, 60h, 1h, 61h, 1h, 61h, 1h, 62h, 1h, 63h, 1h
	db 64h, 1h, 65h, 1h, 66h, 1h, 67h, 1h, 68h, 1h, 69h, 1h, 6ah, 1h, 6bh, 1h
	db 6ch, 1h, 6dh, 1h, 6dh, 1h, 6eh, 1h, 6fh, 1h, 70h, 1h, 71h, 1h, 72h, 1h
	db 73h, 1h, 74h, 1h, 75h, 1h, 76h, 1h, 77h, 1h, 77h, 1h, 78h, 1h, 79h, 1h
	db 7ah, 1h, 7bh, 1h, 7ch, 1h, 7dh, 1h, 7eh, 1h, 7fh, 1h, 7fh, 1h, 80h, 1h
	db 81h, 1h, 82h, 1h, 83h, 1h, 84h, 1h, 85h, 1h, 86h, 1h, 86h, 1h, 87h, 1h
	db 88h, 1h, 89h, 1h, 8ah, 1h, 8bh, 1h, 8ch, 1h, 8ch, 1h, 8dh, 1h, 8eh, 1h
	db 8fh, 1h, 90h, 1h, 91h, 1h, 92h, 1h, 92h, 1h, 93h, 1h, 94h, 1h, 95h, 1h
	db 96h, 1h, 97h, 1h, 97h, 1h, 98h, 1h, 99h, 1h, 9ah, 1h, 9bh, 1h, 9ch, 1h
	db 9ch, 1h, 9dh, 1h, 9eh, 1h, 9fh, 1h, 0a0h, 1h, 0a1h, 1h, 0a1h, 1h, 0a2h, 1h
	db 0a3h, 1h, 0a4h, 1h, 0a5h, 1h, 0a5h, 1h, 0a6h, 1h, 0a7h, 1h, 0a8h, 1h, 0a9h, 1h
	db 0a9h, 1h, 0aah, 1h, 0abh, 1h, 0ach, 1h, 0adh, 1h, 0adh, 1h, 0aeh, 1h, 0afh, 1h
	db 0b0h, 1h, 0b1h, 1h, 0b1h, 1h, 0b2h, 1h, 0b3h, 1h, 0b4h, 1h, 0b5h, 1h, 0b5h, 1h
	db 0b6h, 1h, 0b7h, 1h, 0b8h, 1h, 0b8h, 1h, 0b9h, 1h, 0bah, 1h, 0bbh, 1h, 0bch, 1h
	db 0bch, 1h, 0bdh, 1h, 0beh, 1h, 0bfh, 1h, 0bfh, 1h, 0c0h, 1h, 0c1h, 1h, 0c2h, 1h
	db 0c2h, 1h, 0c3h, 1h, 0c4h, 1h, 0c5h, 1h, 0c5h, 1h, 0c6h, 1h, 0c7h, 1h, 0c8h, 1h
	db 0c8h, 1h, 0c9h, 1h, 0cah, 1h, 0cbh, 1h, 0cbh, 1h, 0cch, 1h, 0cdh, 1h, 0ceh, 1h
	db 0ceh, 1h, 0cfh, 1h, 0d0h, 1h, 0d0h, 1h, 0d1h, 1h, 0d2h, 1h, 0d3h, 1h, 0d3h, 1h
	db 0d4h, 1h, 0d5h, 1h, 0d6h, 1h, 0d6h, 1h, 0d7h, 1h, 0d8h, 1h, 0d8h, 1h, 0d9h, 1h
	db 0dah, 1h, 0dbh, 1h, 0dbh, 1h, 0dch, 1h, 0ddh, 1h, 0ddh, 1h, 0deh, 1h, 0dfh, 1h
	db 0dfh, 1h, 0e0h, 1h, 0e1h, 1h, 0e2h, 1h, 0e2h, 1h, 0e3h, 1h, 0e4h, 1h, 0e4h, 1h
	db 0e5h, 1h, 0e6h, 1h, 0e6h, 1h, 0e7h, 1h, 0e8h, 1h, 0e8h, 1h, 0e9h, 1h, 0eah, 1h
	db 0eah, 1h, 0ebh, 1h, 0ech, 1h, 0edh, 1h, 0edh, 1h, 0eeh, 1h, 0efh, 1h, 0efh, 1h
	db 0f0h, 1h, 0f1h, 1h, 0f1h, 1h, 0f2h, 1h, 0f3h, 1h, 0f3h, 1h, 0f4h, 1h, 0f4h, 1h
	db 0f5h, 1h, 0f6h, 1h, 0f6h, 1h, 0f7h, 1h, 0f8h, 1h, 0f8h, 1h, 0f9h, 1h, 0fah, 1h
	db 0fah, 1h, 0fbh, 1h, 0fch, 1h, 0fch, 1h, 0fdh, 1h, 0feh, 1h, 0feh, 1h, 0ffh, 1h
_angle_cos endp

; 0x2c3e8
_arctan_ratio proc far
	push bp
	mov bp, sp
	push si
	push di
	mov ax, [bp+6]
	mov bx, [bp+8]
	mov si, 1
	cmp ax, 0
	jge atr_x_positive
	neg si
	neg ax
atr_x_positive:
	mov di, 1
	cmp bx, 0
	jge atr_y_positive
	neg di
	neg bx
atr_y_positive:
	cmp ax, 0
	jne atr_x_nonzero
	cmp bx, 0
	jne atr_quarter
	mov ax, 0
	jmp short atr_return
atr_quarter:
	mov ax, 400h
	jmp short atr_octant
atr_x_nonzero:
	cmp bx, 0
	jne atr_general
	xor ax, ax
	jmp short atr_octant
atr_general:
	cmp ax, bx
	jne atr_unequal
	mov ax, 200h
	jmp short atr_octant
atr_unequal:
	jb atr_x_smaller
	xor dh, dh
	mov dl, bh
	mov bh, bl
	xor bl, bl
	xchg bx, ax
	shl ax, 1
	rcl dx, 1
	div bx
	mov bx, ax
	shl bx, 1
	mov ax, word ptr cs:arctan_table[bx]
	jmp short atr_octant
atr_x_smaller:
	xor dh, dh
	mov dl, ah
	mov ah, al
	xor al, al
	shl ax, 1
	rcl dx, 1
	div bx
	mov bx, ax
	shl bx, 1
	mov ax, 400h
	sub ax, word ptr cs:arctan_table[bx]
atr_octant:
	cmp si, 0
	jg atr_y_sign
	mov bx, 800h
	sub bx, ax
	mov ax, bx
atr_y_sign:
	cmp di, 0
	jg atr_scale
	mov bx, 1000h
	sub bx, ax
	mov ax, bx
atr_scale:
	shl ax, 1
	shl ax, 1
	shl ax, 1
	shl ax, 1
atr_return:
	pop di
	pop si
	pop bp
	retf
_arctan_ratio endp

; 0x2c48b
_arctan_lookup proc far
	push bp
	mov bp, sp
	mov bx, [bp+6]
	shl bx, 1
	mov ax, word ptr cs:arctan_table[bx]
	pop bp
	retf
trig_unused_a label byte
	db 146 dup (0h)
scale_divisor label byte
	db 0h, 0h, 0h, 0h
trig_unused_b label byte
	db 0h, 0h
trig_unused_c label byte
	db 3289 dup (0h)
m48_x label byte
	db 0h, 0h
m48_y label byte
	db 0h, 0h
m48_z label byte
	db 0h, 0h
m48_sum_hi label byte
	db 0h, 0h
m48_sum_mid label byte
	db 0h, 0h
m48_sum_lo label byte
	db 2752 dup (0h)
	db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh
	db 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
	db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0feh, 0feh, 0feh, 0feh, 0feh, 0feh, 0feh, 0feh
	db 0feh, 0feh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0h, 0h, 0h, 0h, 0h, 0h, 1h, 1h
	db 1h, 1h, 1h, 1h, 1h, 1h, 1h, 1h, 0fdh, 0fdh, 0fdh, 0fdh, 0fdh, 0fdh, 0fdh, 0fdh
	db 0feh, 0feh, 0feh, 0feh, 0ffh, 0ffh, 0ffh, 0ffh, 0h, 0h, 0h, 0h, 1h, 1h, 1h, 1h
	db 2h, 2h, 2h, 2h, 2h, 2h, 2h, 2h, 0fch, 0fch, 0fch, 0fch, 0fch, 0fch, 0fdh, 0fdh
	db 0fdh, 0fdh, 0feh, 0feh, 0feh, 0feh, 0ffh, 0ffh, 0h, 0h, 1h, 1h, 1h, 1h, 2h, 2h
	db 2h, 2h, 3h, 3h, 3h, 3h, 3h, 3h, 0fbh, 0fbh, 0fbh, 0fbh, 0fbh, 0fbh, 0fch, 0fch
	db 0fdh, 0fdh, 0feh, 0feh, 0ffh, 0ffh, 0ffh, 0ffh, 0h, 0h, 1h, 1h, 2h, 2h, 3h, 3h
	db 3h, 3h, 4h, 4h, 4h, 4h, 4h, 4h, 0fah, 0fah, 0fah, 0fah, 0fah, 0fah, 0fbh, 0fbh
	db 0fch, 0fch, 0fdh, 0fdh, 0feh, 0feh, 0ffh, 0ffh, 0h, 0h, 1h, 1h, 2h, 2h, 3h, 3h
	db 4h, 4h, 5h, 5h, 5h, 5h, 5h, 5h, 0f9h, 0f9h, 0f9h, 0f9h, 0f9h, 0fah, 0fah, 0fbh
	db 0fch, 0fch, 0fdh, 0fdh, 0feh, 0feh, 0ffh, 0ffh, 0h, 0h, 1h, 1h, 2h, 2h, 3h, 3h
	db 4h, 5h, 5h, 6h, 6h, 6h, 6h, 6h, 0f8h, 0f8h, 0f8h, 0f8h, 0f8h, 0f9h, 0f9h, 0fah
	db 0fbh, 0fbh, 0fch, 0fdh, 0fdh, 0feh, 0ffh, 0ffh, 0h, 0h, 1h, 2h, 2h, 3h, 4h, 4h
	db 5h, 6h, 6h, 7h, 7h, 7h, 7h, 7h
cosine_table label byte
	db 0h, 40h, 0ffh, 3fh, 0ffh, 3fh, 0ffh, 3fh, 0ffh, 3fh, 0ffh, 3fh, 0ffh, 3fh, 0ffh, 3fh
	db 0feh, 3fh, 0feh, 3fh, 0feh, 3fh, 0fdh, 3fh, 0fdh, 3fh, 0fch, 3fh, 0fch, 3fh, 0fbh, 3fh
	db 0fbh, 3fh, 0fah, 3fh, 0f9h, 3fh, 0f9h, 3fh, 0f8h, 3fh, 0f7h, 3fh, 0f6h, 3fh, 0f5h, 3fh
	db 0f4h, 3fh, 0f3h, 3fh, 0f2h, 3fh, 0f1h, 3fh, 0f0h, 3fh, 0efh, 3fh, 0eeh, 3fh, 0edh, 3fh
	db 0ech, 3fh, 0ebh, 3fh, 0e9h, 3fh, 0e8h, 3fh, 0e7h, 3fh, 0e5h, 3fh, 0e4h, 3fh, 0e2h, 3fh
	db 0e1h, 3fh, 0dfh, 3fh, 0deh, 3fh, 0dch, 3fh, 0dah, 3fh, 0d8h, 3fh, 0d7h, 3fh, 0d5h, 3fh
	db 0d3h, 3fh, 0d1h, 3fh, 0cfh, 3fh, 0cdh, 3fh, 0cbh, 3fh, 0c9h, 3fh, 0c7h, 3fh, 0c5h, 3fh
	db 0c3h, 3fh, 0c1h, 3fh, 0bfh, 3fh, 0bch, 3fh, 0bah, 3fh, 0b8h, 3fh, 0b5h, 3fh, 0b3h, 3fh
	db 0b1h, 3fh, 0aeh, 3fh, 0ach, 3fh, 0a9h, 3fh, 0a6h, 3fh, 0a4h, 3fh, 0a1h, 3fh, 9eh, 3fh
	db 9ch, 3fh, 99h, 3fh, 96h, 3fh, 93h, 3fh, 90h, 3fh, 8dh, 3fh, 8ah, 3fh, 87h, 3fh
	db 84h, 3fh, 81h, 3fh, 7eh, 3fh, 7bh, 3fh, 78h, 3fh, 74h, 3fh, 71h, 3fh, 6eh, 3fh
	db 6ah, 3fh, 67h, 3fh, 64h, 3fh, 60h, 3fh, 5dh, 3fh, 59h, 3fh, 55h, 3fh, 52h, 3fh
	db 4eh, 3fh, 4ah, 3fh, 47h, 3fh, 43h, 3fh, 3fh, 3fh, 3bh, 3fh, 37h, 3fh, 33h, 3fh
	db 2fh, 3fh, 2bh, 3fh, 27h, 3fh, 23h, 3fh, 1fh, 3fh, 1bh, 3fh, 17h, 3fh, 13h, 3fh
	db 0eh, 3fh, 0ah, 3fh, 6h, 3fh, 1h, 3fh, 0fdh, 3eh, 0f8h, 3eh, 0f4h, 3eh, 0efh, 3eh
	db 0ebh, 3eh, 0e6h, 3eh, 0e1h, 3eh, 0ddh, 3eh, 0d8h, 3eh, 0d3h, 3eh, 0ceh, 3eh, 0cah, 3eh
	db 0c5h, 3eh, 0c0h, 3eh, 0bbh, 3eh, 0b6h, 3eh, 0b1h, 3eh, 0ach, 3eh, 0a7h, 3eh, 0a1h, 3eh
	db 9ch, 3eh, 97h, 3eh, 92h, 3eh, 8ch, 3eh, 87h, 3eh, 82h, 3eh, 7ch, 3eh, 77h, 3eh
	db 71h, 3eh, 6ch, 3eh, 66h, 3eh, 61h, 3eh, 5bh, 3eh, 55h, 3eh, 50h, 3eh, 4ah, 3eh
	db 44h, 3eh, 3eh, 3eh, 38h, 3eh, 33h, 3eh, 2dh, 3eh, 27h, 3eh, 21h, 3eh, 1bh, 3eh
	db 14h, 3eh, 0eh, 3eh, 8h, 3eh, 2h, 3eh, 0fch, 3dh, 0f5h, 3dh, 0efh, 3dh, 0e9h, 3dh
	db 0e2h, 3dh, 0dch, 3dh, 0d6h, 3dh, 0cfh, 3dh, 0c9h, 3dh, 0c2h, 3dh, 0bbh, 3dh, 0b5h, 3dh
	db 0aeh, 3dh, 0a7h, 3dh, 0a1h, 3dh, 9ah, 3dh, 93h, 3dh, 8ch, 3dh, 85h, 3dh, 7eh, 3dh
	db 77h, 3dh, 70h, 3dh, 69h, 3dh, 62h, 3dh, 5bh, 3dh, 54h, 3dh, 4dh, 3dh, 45h, 3dh
	db 3eh, 3dh, 37h, 3dh, 2fh, 3dh, 28h, 3dh, 21h, 3dh, 19h, 3dh, 12h, 3dh, 0ah, 3dh
	db 2h, 3dh, 0fbh, 3ch, 0f3h, 3ch, 0ech, 3ch, 0e4h, 3ch, 0dch, 3ch, 0d4h, 3ch, 0cch, 3ch
	db 0c5h, 3ch, 0bdh, 3ch, 0b5h, 3ch, 0adh, 3ch, 0a5h, 3ch, 9dh, 3ch, 95h, 3ch, 8ch, 3ch
	db 84h, 3ch, 7ch, 3ch, 74h, 3ch, 6ch, 3ch, 63h, 3ch, 5bh, 3ch, 53h, 3ch, 4ah, 3ch
	db 42h, 3ch, 39h, 3ch, 31h, 3ch, 28h, 3ch, 20h, 3ch, 17h, 3ch, 0eh, 3ch, 6h, 3ch
	db 0fdh, 3bh, 0f4h, 3bh, 0ebh, 3bh, 0e2h, 3bh, 0dah, 3bh, 0d1h, 3bh, 0c8h, 3bh, 0bfh, 3bh
	db 0b6h, 3bh, 0adh, 3bh, 0a3h, 3bh, 9ah, 3bh, 91h, 3bh, 88h, 3bh, 7fh, 3bh, 75h, 3bh
	db 6ch, 3bh, 63h, 3bh, 59h, 3bh, 50h, 3bh, 47h, 3bh, 3dh, 3bh, 34h, 3bh, 2ah, 3bh
	db 20h, 3bh, 17h, 3bh, 0dh, 3bh, 3h, 3bh, 0fah, 3ah, 0f0h, 3ah, 0e6h, 3ah, 0dch, 3ah
	db 0d2h, 3ah, 0c8h, 3ah, 0beh, 3ah, 0b4h, 3ah, 0aah, 3ah, 0a0h, 3ah, 96h, 3ah, 8ch, 3ah
	db 82h, 3ah, 78h, 3ah, 6dh, 3ah, 63h, 3ah, 59h, 3ah, 4fh, 3ah, 44h, 3ah, 3ah, 3ah
	db 2fh, 3ah, 25h, 3ah, 1ah, 3ah, 10h, 3ah, 5h, 3ah, 0fbh, 39h, 0f0h, 39h, 0e5h, 39h
	db 0dah, 39h, 0d0h, 39h, 0c5h, 39h, 0bah, 39h, 0afh, 39h, 0a4h, 39h, 99h, 39h, 8eh, 39h
	db 83h, 39h, 78h, 39h, 6dh, 39h, 62h, 39h, 57h, 39h, 4ch, 39h, 41h, 39h, 35h, 39h
	db 2ah, 39h, 1fh, 39h, 13h, 39h, 8h, 39h, 0fdh, 38h, 0f1h, 38h, 0e6h, 38h, 0dah, 38h
	db 0cfh, 38h, 0c3h, 38h, 0b7h, 38h, 0ach, 38h, 0a0h, 38h, 94h, 38h, 89h, 38h, 7dh, 38h
	db 71h, 38h, 65h, 38h, 59h, 38h, 4dh, 38h, 41h, 38h, 35h, 38h, 29h, 38h, 1dh, 38h
	db 11h, 38h, 5h, 38h, 0f9h, 37h, 0edh, 37h, 0e0h, 37h, 0d4h, 37h, 0c8h, 37h, 0bbh, 37h
	db 0afh, 37h, 0a3h, 37h, 96h, 37h, 8ah, 37h, 7dh, 37h, 71h, 37h, 64h, 37h, 57h, 37h
	db 4bh, 37h, 3eh, 37h, 31h, 37h, 25h, 37h, 18h, 37h, 0bh, 37h, 0feh, 36h, 0f1h, 36h
	db 0e5h, 36h, 0d8h, 36h, 0cbh, 36h, 0beh, 36h, 0b1h, 36h, 0a4h, 36h, 96h, 36h, 89h, 36h
	db 7ch, 36h, 6fh, 36h, 62h, 36h, 54h, 36h, 47h, 36h, 3ah, 36h, 2ch, 36h, 1fh, 36h
	db 12h, 36h, 4h, 36h, 0f7h, 35h, 0e9h, 35h, 0dch, 35h, 0ceh, 35h, 0c0h, 35h, 0b3h, 35h
	db 0a5h, 35h, 97h, 35h, 89h, 35h, 7ch, 35h, 6eh, 35h, 60h, 35h, 52h, 35h, 44h, 35h
	db 36h, 35h, 28h, 35h, 1ah, 35h, 0ch, 35h, 0feh, 34h, 0f0h, 34h, 0e2h, 34h, 0d4h, 34h
	db 0c6h, 34h, 0b7h, 34h, 0a9h, 34h, 9bh, 34h, 8ch, 34h, 7eh, 34h, 70h, 34h, 61h, 34h
	db 53h, 34h, 44h, 34h, 36h, 34h, 27h, 34h, 19h, 34h, 0ah, 34h, 0fbh, 33h, 0edh, 33h
	db 0deh, 33h, 0cfh, 33h, 0c1h, 33h, 0b2h, 33h, 0a3h, 33h, 94h, 33h, 85h, 33h, 76h, 33h
	db 67h, 33h, 58h, 33h, 49h, 33h, 3ah, 33h, 2bh, 33h, 1ch, 33h, 0dh, 33h, 0feh, 32h
	db 0eeh, 32h, 0dfh, 32h, 0d0h, 32h, 0c1h, 32h, 0b1h, 32h, 0a2h, 32h, 93h, 32h, 83h, 32h
	db 74h, 32h, 64h, 32h, 55h, 32h, 45h, 32h, 36h, 32h, 26h, 32h, 16h, 32h, 7h, 32h
	db 0f7h, 31h, 0e7h, 31h, 0d8h, 31h, 0c8h, 31h, 0b8h, 31h, 0a8h, 31h, 98h, 31h, 88h, 31h
	db 79h, 31h, 69h, 31h, 59h, 31h, 49h, 31h, 38h, 31h, 28h, 31h, 18h, 31h, 8h, 31h
	db 0f8h, 30h, 0e8h, 30h, 0d8h, 30h, 0c7h, 30h, 0b7h, 30h, 0a7h, 30h, 96h, 30h, 86h, 30h
	db 76h, 30h, 65h, 30h, 55h, 30h, 44h, 30h, 34h, 30h, 23h, 30h, 13h, 30h, 2h, 30h
	db 0f1h, 2fh, 0e1h, 2fh, 0d0h, 2fh, 0bfh, 2fh, 0afh, 2fh, 9eh, 2fh, 8dh, 2fh, 7ch, 2fh
	db 6bh, 2fh, 5ah, 2fh, 49h, 2fh, 38h, 2fh, 28h, 2fh, 16h, 2fh, 5h, 2fh, 0f4h, 2eh
	db 0e3h, 2eh, 0d2h, 2eh, 0c1h, 2eh, 0b0h, 2eh, 9fh, 2eh, 8dh, 2eh, 7ch, 2eh, 6bh, 2eh
	db 5ah, 2eh, 48h, 2eh, 37h, 2eh, 25h, 2eh, 14h, 2eh, 3h, 2eh, 0f1h, 2dh, 0e0h, 2dh
	db 0ceh, 2dh, 0bch, 2dh, 0abh, 2dh, 99h, 2dh, 88h, 2dh, 76h, 2dh, 64h, 2dh, 52h, 2dh
	db 41h, 2dh, 2fh, 2dh, 1dh, 2dh, 0bh, 2dh, 0f9h, 2ch, 0e8h, 2ch, 0d6h, 2ch, 0c4h, 2ch
	db 0b2h, 2ch, 0a0h, 2ch, 8eh, 2ch, 7ch, 2ch, 6ah, 2ch, 57h, 2ch, 45h, 2ch, 33h, 2ch
	db 21h, 2ch, 0fh, 2ch, 0fch, 2bh, 0eah, 2bh, 0d8h, 2bh, 0c6h, 2bh, 0b3h, 2bh, 0a1h, 2bh
	db 8eh, 2bh, 7ch, 2bh, 6ah, 2bh, 57h, 2bh, 45h, 2bh, 32h, 2bh, 20h, 2bh, 0dh, 2bh
	db 0fah, 2ah, 0e8h, 2ah, 0d5h, 2ah, 0c2h, 2ah, 0b0h, 2ah, 9dh, 2ah, 8ah, 2ah, 77h, 2ah
	db 65h, 2ah, 52h, 2ah, 3fh, 2ah, 2ch, 2ah, 19h, 2ah, 6h, 2ah, 0f3h, 29h, 0e0h, 29h
	db 0cdh, 29h, 0bah, 29h, 0a7h, 29h, 94h, 29h, 81h, 29h, 6eh, 29h, 5ah, 29h, 47h, 29h
	db 34h, 29h, 21h, 29h, 0eh, 29h, 0fah, 28h, 0e7h, 28h, 0d4h, 28h, 0c0h, 28h, 0adh, 28h
	db 99h, 28h, 86h, 28h, 72h, 28h, 5fh, 28h, 4bh, 28h, 38h, 28h, 24h, 28h, 11h, 28h
	db 0fdh, 27h, 0eah, 27h, 0d6h, 27h, 0c2h, 27h, 0afh, 27h, 9bh, 27h, 87h, 27h, 73h, 27h
	db 5fh, 27h, 4ch, 27h, 38h, 27h, 24h, 27h, 10h, 27h, 0fch, 26h, 0e8h, 26h, 0d4h, 26h
	db 0c0h, 26h, 0ach, 26h, 98h, 26h, 84h, 26h, 70h, 26h, 5ch, 26h, 48h, 26h, 34h, 26h
	db 1fh, 26h, 0bh, 26h, 0f7h, 25h, 0e3h, 25h, 0cfh, 25h, 0bah, 25h, 0a6h, 25h, 92h, 25h
	db 7dh, 25h, 69h, 25h, 54h, 25h, 40h, 25h, 2ch, 25h, 17h, 25h, 3h, 25h, 0eeh, 24h
	db 0dah, 24h, 0c5h, 24h, 0b0h, 24h, 9ch, 24h, 87h, 24h, 73h, 24h, 5eh, 24h, 49h, 24h
	db 34h, 24h, 20h, 24h, 0bh, 24h, 0f6h, 23h, 0e1h, 23h, 0cdh, 23h, 0b8h, 23h, 0a3h, 23h
	db 8eh, 23h, 79h, 23h, 64h, 23h, 4fh, 23h, 3ah, 23h, 25h, 23h, 10h, 23h, 0fbh, 22h
	db 0e6h, 22h, 0d1h, 22h, 0bch, 22h, 0a7h, 22h, 92h, 22h, 7dh, 22h, 67h, 22h, 52h, 22h
	db 3dh, 22h, 28h, 22h, 12h, 22h, 0fdh, 21h, 0e8h, 21h, 0d2h, 21h, 0bdh, 21h, 0a8h, 21h
	db 92h, 21h, 7dh, 21h, 68h, 21h, 52h, 21h, 3dh, 21h, 27h, 21h, 12h, 21h, 0fch, 20h
	db 0e7h, 20h, 0d1h, 20h, 0bbh, 20h, 0a6h, 20h, 90h, 20h, 7bh, 20h, 65h, 20h, 4fh, 20h
	db 39h, 20h, 24h, 20h, 0eh, 20h, 0f8h, 1fh, 0e2h, 1fh, 0cdh, 1fh, 0b7h, 1fh, 0a1h, 1fh
	db 8bh, 1fh, 75h, 1fh, 5fh, 1fh, 49h, 1fh, 34h, 1fh, 1eh, 1fh, 8h, 1fh, 0f2h, 1eh
	db 0dch, 1eh, 0c6h, 1eh, 0b0h, 1eh, 99h, 1eh, 83h, 1eh, 6dh, 1eh, 57h, 1eh, 41h, 1eh
	db 2bh, 1eh, 15h, 1eh, 0feh, 1dh, 0e8h, 1dh, 0d2h, 1dh, 0bch, 1dh, 0a6h, 1dh, 8fh, 1dh
	db 79h, 1dh, 63h, 1dh, 4ch, 1dh, 36h, 1dh, 20h, 1dh, 9h, 1dh, 0f3h, 1ch, 0dch, 1ch
	db 0c6h, 1ch, 0afh, 1ch, 99h, 1ch, 83h, 1ch, 6ch, 1ch, 55h, 1ch, 3fh, 1ch, 28h, 1ch
	db 12h, 1ch, 0fbh, 1bh, 0e5h, 1bh, 0ceh, 1bh, 0b7h, 1bh, 0a1h, 1bh, 8ah, 1bh, 73h, 1bh
	db 5dh, 1bh, 46h, 1bh, 2fh, 1bh, 18h, 1bh, 2h, 1bh, 0ebh, 1ah, 0d4h, 1ah, 0bdh, 1ah
	db 0a6h, 1ah, 8fh, 1ah, 79h, 1ah, 62h, 1ah, 4bh, 1ah, 34h, 1ah, 1dh, 1ah, 6h, 1ah
	db 0efh, 19h, 0d8h, 19h, 0c1h, 19h, 0aah, 19h, 93h, 19h, 7ch, 19h, 65h, 19h, 4eh, 19h
	db 37h, 19h, 20h, 19h, 8h, 19h, 0f1h, 18h, 0dah, 18h, 0c3h, 18h, 0ach, 18h, 95h, 18h
	db 7dh, 18h, 66h, 18h, 4fh, 18h, 38h, 18h, 20h, 18h, 9h, 18h, 0f2h, 17h, 0dah, 17h
	db 0c3h, 17h, 0ach, 17h, 94h, 17h, 7dh, 17h, 66h, 17h, 4eh, 17h, 37h, 17h, 1fh, 17h
	db 8h, 17h, 0f1h, 16h, 0d9h, 16h, 0c2h, 16h, 0aah, 16h, 93h, 16h, 7bh, 16h, 64h, 16h
	db 4ch, 16h, 34h, 16h, 1dh, 16h, 5h, 16h, 0eeh, 15h, 0d6h, 15h, 0beh, 15h, 0a7h, 15h
	db 8fh, 15h, 77h, 15h, 60h, 15h, 48h, 15h, 30h, 15h, 19h, 15h, 1h, 15h, 0e9h, 14h
	db 0d1h, 14h, 0bah, 14h, 0a2h, 14h, 8ah, 14h, 72h, 14h, 5ah, 14h, 43h, 14h, 2bh, 14h
	db 13h, 14h, 0fbh, 13h, 0e3h, 13h, 0cbh, 13h, 0b3h, 13h, 9bh, 13h, 83h, 13h, 6ch, 13h
	db 54h, 13h, 3ch, 13h, 24h, 13h, 0ch, 13h, 0f4h, 12h, 0dch, 12h, 0c4h, 12h, 0ach, 12h
	db 94h, 12h, 7bh, 12h, 63h, 12h, 4bh, 12h, 33h, 12h, 1bh, 12h, 3h, 12h, 0ebh, 11h
	db 0d3h, 11h, 0bbh, 11h, 0a2h, 11h, 8ah, 11h, 72h, 11h, 5ah, 11h, 42h, 11h, 2ah, 11h
	db 11h, 11h, 0f9h, 10h, 0e1h, 10h, 0c9h, 10h, 0b0h, 10h, 98h, 10h, 80h, 10h, 68h, 10h
	db 4fh, 10h, 37h, 10h, 1fh, 10h, 6h, 10h, 0eeh, 0fh, 0d6h, 0fh, 0bdh, 0fh, 0a5h, 0fh
	db 8ch, 0fh, 74h, 0fh, 5ch, 0fh, 43h, 0fh, 2bh, 0fh, 12h, 0fh, 0fah, 0eh, 0e2h, 0eh
	db 0c9h, 0eh, 0b1h, 0eh, 98h, 0eh, 80h, 0eh, 67h, 0eh, 4fh, 0eh, 36h, 0eh, 1eh, 0eh
	db 5h, 0eh, 0edh, 0dh, 0d4h, 0dh, 0bch, 0dh, 0a3h, 0dh, 8bh, 0dh, 72h, 0dh, 59h, 0dh
	db 41h, 0dh, 28h, 0dh, 10h, 0dh, 0f7h, 0ch, 0deh, 0ch, 0c6h, 0ch, 0adh, 0ch, 95h, 0ch
	db 7ch, 0ch, 63h, 0ch, 4bh, 0ch, 32h, 0ch, 19h, 0ch, 1h, 0ch, 0e8h, 0bh, 0cfh, 0bh
	db 0b6h, 0bh, 9eh, 0bh, 85h, 0bh, 6ch, 0bh, 54h, 0bh, 3bh, 0bh, 22h, 0bh, 9h, 0bh
	db 0f1h, 0ah, 0d8h, 0ah, 0bfh, 0ah, 0a6h, 0ah, 8dh, 0ah, 75h, 0ah, 5ch, 0ah, 43h, 0ah
	db 2ah, 0ah, 11h, 0ah, 0f9h, 9h, 0e0h, 9h, 0c7h, 9h, 0aeh, 9h, 95h, 9h, 7ch, 9h
	db 64h, 9h, 4bh, 9h, 32h, 9h, 19h, 9h, 0h, 9h, 0e7h, 8h, 0ceh, 8h, 0b5h, 8h
	db 9ch, 8h, 84h, 8h, 6bh, 8h, 52h, 8h, 39h, 8h, 20h, 8h, 7h, 8h, 0eeh, 7h
	db 0d5h, 7h, 0bch, 7h, 0a3h, 7h, 8ah, 7h, 71h, 7h, 58h, 7h, 3fh, 7h, 26h, 7h
	db 0dh, 7h, 0f4h, 6h, 0dbh, 6h, 0c2h, 6h, 0a9h, 6h, 90h, 6h, 77h, 6h, 5eh, 6h
	db 45h, 6h, 2ch, 6h, 13h, 6h, 0fah, 5h, 0e1h, 5h, 0c8h, 5h, 0afh, 5h, 96h, 5h
	db 7dh, 5h, 64h, 5h, 4bh, 5h, 32h, 5h, 19h, 5h, 0h, 5h, 0e7h, 4h, 0ceh, 4h
	db 0b5h, 4h, 9ch, 4h, 83h, 4h, 6ah, 4h, 51h, 4h, 37h, 4h, 1eh, 4h, 5h, 4h
	db 0ech, 3h, 0d3h, 3h, 0bah, 3h, 0a1h, 3h, 88h, 3h, 6fh, 3h, 56h, 3h, 3dh, 3h
	db 23h, 3h, 0ah, 3h, 0f1h, 2h, 0d8h, 2h, 0bfh, 2h, 0a6h, 2h, 8dh, 2h, 74h, 2h
	db 5bh, 2h, 41h, 2h, 28h, 2h, 0fh, 2h, 0f6h, 1h, 0ddh, 1h, 0c4h, 1h, 0abh, 1h
	db 92h, 1h, 78h, 1h, 5fh, 1h, 46h, 1h, 2dh, 1h, 14h, 1h, 0fbh, 0h, 0e2h, 0h
	db 0c9h, 0h, 0afh, 0h, 96h, 0h, 7dh, 0h, 64h, 0h, 4bh, 0h, 32h, 0h, 19h, 0h
	db 0h, 0h, 0e6h, 0ffh, 0cdh, 0ffh, 0b4h, 0ffh, 9bh, 0ffh, 82h, 0ffh, 69h, 0ffh, 50h, 0ffh
	db 36h, 0ffh, 1dh, 0ffh, 4h, 0ffh, 0ebh, 0feh, 0d2h, 0feh, 0b9h, 0feh, 0a0h, 0feh, 87h, 0feh
	db 6dh, 0feh, 54h, 0feh, 3bh, 0feh, 22h, 0feh, 9h, 0feh, 0f0h, 0fdh, 0d7h, 0fdh, 0beh, 0fdh
	db 0a4h, 0fdh, 8bh, 0fdh, 72h, 0fdh, 59h, 0fdh, 40h, 0fdh, 27h, 0fdh, 0eh, 0fdh, 0f5h, 0fch
	db 0dch, 0fch, 0c2h, 0fch, 0a9h, 0fch, 90h, 0fch, 77h, 0fch, 5eh, 0fch, 45h, 0fch, 2ch, 0fch
	db 13h, 0fch, 0fah, 0fbh, 0e1h, 0fbh, 0c8h, 0fbh, 0aeh, 0fbh, 95h, 0fbh, 7ch, 0fbh, 63h, 0fbh
	db 4ah, 0fbh, 31h, 0fbh, 18h, 0fbh, 0ffh, 0fah, 0e6h, 0fah, 0cdh, 0fah, 0b4h, 0fah, 9bh, 0fah
	db 82h, 0fah, 69h, 0fah, 50h, 0fah, 37h, 0fah, 1eh, 0fah, 5h, 0fah, 0ech, 0f9h, 0d3h, 0f9h
	db 0bah, 0f9h, 0a1h, 0f9h, 88h, 0f9h, 6fh, 0f9h, 56h, 0f9h, 3dh, 0f9h, 24h, 0f9h, 0bh, 0f9h
	db 0f2h, 0f8h, 0d9h, 0f8h, 0c0h, 0f8h, 0a7h, 0f8h, 8eh, 0f8h, 75h, 0f8h, 5ch, 0f8h, 43h, 0f8h
	db 2ah, 0f8h, 11h, 0f8h, 0f8h, 0f7h, 0dfh, 0f7h, 0c6h, 0f7h, 0adh, 0f7h, 94h, 0f7h, 7bh, 0f7h
	db 63h, 0f7h, 4ah, 0f7h, 31h, 0f7h, 18h, 0f7h, 0ffh, 0f6h, 0e6h, 0f6h, 0cdh, 0f6h, 0b4h, 0f6h
	db 9bh, 0f6h, 83h, 0f6h, 6ah, 0f6h, 51h, 0f6h, 38h, 0f6h, 1fh, 0f6h, 6h, 0f6h, 0eeh, 0f5h
	db 0d5h, 0f5h, 0bch, 0f5h, 0a3h, 0f5h, 8ah, 0f5h, 72h, 0f5h, 59h, 0f5h, 40h, 0f5h, 27h, 0f5h
	db 0eh, 0f5h, 0f6h, 0f4h, 0ddh, 0f4h, 0c4h, 0f4h, 0abh, 0f4h, 93h, 0f4h, 7ah, 0f4h, 61h, 0f4h
	db 49h, 0f4h, 30h, 0f4h, 17h, 0f4h, 0feh, 0f3h, 0e6h, 0f3h, 0cdh, 0f3h, 0b4h, 0f3h, 9ch, 0f3h
	db 83h, 0f3h, 6ah, 0f3h, 52h, 0f3h, 39h, 0f3h, 21h, 0f3h, 8h, 0f3h, 0efh, 0f2h, 0d7h, 0f2h
	db 0beh, 0f2h, 0a6h, 0f2h, 8dh, 0f2h, 74h, 0f2h, 5ch, 0f2h, 43h, 0f2h, 2bh, 0f2h, 12h, 0f2h
	db 0fah, 0f1h, 0e1h, 0f1h, 0c9h, 0f1h, 0b0h, 0f1h, 98h, 0f1h, 7fh, 0f1h, 67h, 0f1h, 4eh, 0f1h
	db 36h, 0f1h, 1dh, 0f1h, 5h, 0f1h, 0edh, 0f0h, 0d4h, 0f0h, 0bch, 0f0h, 0a3h, 0f0h, 8bh, 0f0h
	db 73h, 0f0h, 5ah, 0f0h, 42h, 0f0h, 29h, 0f0h, 11h, 0f0h, 0f9h, 0efh, 0e0h, 0efh, 0c8h, 0efh
	db 0b0h, 0efh, 97h, 0efh, 7fh, 0efh, 67h, 0efh, 4fh, 0efh, 36h, 0efh, 1eh, 0efh, 6h, 0efh
	db 0eeh, 0eeh, 0d5h, 0eeh, 0bdh, 0eeh, 0a5h, 0eeh, 8dh, 0eeh, 75h, 0eeh, 5dh, 0eeh, 44h, 0eeh
	db 2ch, 0eeh, 14h, 0eeh, 0fch, 0edh, 0e4h, 0edh, 0cch, 0edh, 0b4h, 0edh, 9ch, 0edh, 84h, 0edh
	db 6bh, 0edh, 53h, 0edh, 3bh, 0edh, 23h, 0edh, 0bh, 0edh, 0f3h, 0ech, 0dbh, 0ech, 0c3h, 0ech
	db 0abh, 0ech, 93h, 0ech, 7ch, 0ech, 64h, 0ech, 4ch, 0ech, 34h, 0ech, 1ch, 0ech, 4h, 0ech
	db 0ech, 0ebh, 0d4h, 0ebh, 0bch, 0ebh, 0a5h, 0ebh, 8dh, 0ebh, 75h, 0ebh, 5dh, 0ebh, 45h, 0ebh
	db 2eh, 0ebh, 16h, 0ebh, 0feh, 0eah, 0e6h, 0eah, 0cfh, 0eah, 0b7h, 0eah, 9fh, 0eah, 88h, 0eah
	db 70h, 0eah, 58h, 0eah, 41h, 0eah, 29h, 0eah, 11h, 0eah, 0fah, 0e9h, 0e2h, 0e9h, 0cbh, 0e9h
	db 0b3h, 0e9h, 9bh, 0e9h, 84h, 0e9h, 6ch, 0e9h, 55h, 0e9h, 3dh, 0e9h, 26h, 0e9h, 0eh, 0e9h
	db 0f7h, 0e8h, 0e0h, 0e8h, 0c8h, 0e8h, 0b1h, 0e8h, 99h, 0e8h, 82h, 0e8h, 6bh, 0e8h, 53h, 0e8h
	db 3ch, 0e8h, 25h, 0e8h, 0dh, 0e8h, 0f6h, 0e7h, 0dfh, 0e7h, 0c7h, 0e7h, 0b0h, 0e7h, 99h, 0e7h
	db 82h, 0e7h, 6ah, 0e7h, 53h, 0e7h, 3ch, 0e7h, 25h, 0e7h, 0eh, 0e7h, 0f7h, 0e6h, 0dfh, 0e6h
	db 0c8h, 0e6h, 0b1h, 0e6h, 9ah, 0e6h, 83h, 0e6h, 6ch, 0e6h, 55h, 0e6h, 3eh, 0e6h, 27h, 0e6h
	db 10h, 0e6h, 0f9h, 0e5h, 0e2h, 0e5h, 0cbh, 0e5h, 0b4h, 0e5h, 9dh, 0e5h, 86h, 0e5h, 70h, 0e5h
	db 59h, 0e5h, 42h, 0e5h, 2bh, 0e5h, 14h, 0e5h, 0fdh, 0e4h, 0e7h, 0e4h, 0d0h, 0e4h, 0b9h, 0e4h
	db 0a2h, 0e4h, 8ch, 0e4h, 75h, 0e4h, 5eh, 0e4h, 48h, 0e4h, 31h, 0e4h, 1ah, 0e4h, 4h, 0e4h
	db 0edh, 0e3h, 0d7h, 0e3h, 0c0h, 0e3h, 0aah, 0e3h, 93h, 0e3h, 7ch, 0e3h, 66h, 0e3h, 50h, 0e3h
	db 39h, 0e3h, 23h, 0e3h, 0ch, 0e3h, 0f6h, 0e2h, 0dfh, 0e2h, 0c9h, 0e2h, 0b3h, 0e2h, 9ch, 0e2h
	db 86h, 0e2h, 70h, 0e2h, 59h, 0e2h, 43h, 0e2h, 2dh, 0e2h, 17h, 0e2h, 1h, 0e2h, 0eah, 0e1h
	db 0d4h, 0e1h, 0beh, 0e1h, 0a8h, 0e1h, 92h, 0e1h, 7ch, 0e1h, 66h, 0e1h, 4fh, 0e1h, 39h, 0e1h
	db 23h, 0e1h, 0dh, 0e1h, 0f7h, 0e0h, 0e1h, 0e0h, 0cbh, 0e0h, 0b6h, 0e0h, 0a0h, 0e0h, 8ah, 0e0h
	db 74h, 0e0h, 5eh, 0e0h, 48h, 0e0h, 32h, 0e0h, 1dh, 0e0h, 7h, 0e0h, 0f1h, 0dfh, 0dbh, 0dfh
	db 0c6h, 0dfh, 0b0h, 0dfh, 9ah, 0dfh, 84h, 0dfh, 6fh, 0dfh, 59h, 0dfh, 44h, 0dfh, 2eh, 0dfh
	db 18h, 0dfh, 3h, 0dfh, 0edh, 0deh, 0d8h, 0deh, 0c2h, 0deh, 0adh, 0deh, 97h, 0deh, 82h, 0deh
	db 6dh, 0deh, 57h, 0deh, 42h, 0deh, 2dh, 0deh, 17h, 0deh, 2h, 0deh, 0edh, 0ddh, 0d7h, 0ddh
	db 0c2h, 0ddh, 0adh, 0ddh, 98h, 0ddh, 82h, 0ddh, 6dh, 0ddh, 58h, 0ddh, 43h, 0ddh, 2eh, 0ddh
	db 19h, 0ddh, 4h, 0ddh, 0efh, 0dch, 0dah, 0dch, 0c5h, 0dch, 0b0h, 0dch, 9bh, 0dch, 86h, 0dch
	db 71h, 0dch, 5ch, 0dch, 47h, 0dch, 32h, 0dch, 1eh, 0dch, 9h, 0dch, 0f4h, 0dbh, 0dfh, 0dbh
	db 0cbh, 0dbh, 0b6h, 0dbh, 0a1h, 0dbh, 8ch, 0dbh, 78h, 0dbh, 63h, 0dbh, 4fh, 0dbh, 3ah, 0dbh
	db 25h, 0dbh, 11h, 0dbh, 0fch, 0dah, 0e8h, 0dah, 0d3h, 0dah, 0bfh, 0dah, 0abh, 0dah, 96h, 0dah
	db 82h, 0dah, 6dh, 0dah, 59h, 0dah, 45h, 0dah, 30h, 0dah, 1ch, 0dah, 8h, 0dah, 0f4h, 0d9h
	db 0e0h, 0d9h, 0cbh, 0d9h, 0b7h, 0d9h, 0a3h, 0d9h, 8fh, 0d9h, 7bh, 0d9h, 67h, 0d9h, 53h, 0d9h
	db 3fh, 0d9h, 2bh, 0d9h, 17h, 0d9h, 3h, 0d9h, 0efh, 0d8h, 0dbh, 0d8h, 0c7h, 0d8h, 0b3h, 0d8h
	db 0a0h, 0d8h, 8ch, 0d8h, 78h, 0d8h, 64h, 0d8h, 50h, 0d8h, 3dh, 0d8h, 29h, 0d8h, 15h, 0d8h
	db 2h, 0d8h, 0eeh, 0d7h, 0dbh, 0d7h, 0c7h, 0d7h, 0b4h, 0d7h, 0a0h, 0d7h, 8dh, 0d7h, 79h, 0d7h
	db 66h, 0d7h, 52h, 0d7h, 3fh, 0d7h, 2bh, 0d7h, 18h, 0d7h, 5h, 0d7h, 0f1h, 0d6h, 0deh, 0d6h
	db 0cbh, 0d6h, 0b8h, 0d6h, 0a5h, 0d6h, 91h, 0d6h, 7eh, 0d6h, 6bh, 0d6h, 58h, 0d6h, 45h, 0d6h
	db 32h, 0d6h, 1fh, 0d6h, 0ch, 0d6h, 0f9h, 0d5h, 0e6h, 0d5h, 0d3h, 0d5h, 0c0h, 0d5h, 0adh, 0d5h
	db 9ah, 0d5h, 88h, 0d5h, 75h, 0d5h, 62h, 0d5h, 4fh, 0d5h, 3dh, 0d5h, 2ah, 0d5h, 17h, 0d5h
	db 5h, 0d5h, 0f2h, 0d4h, 0dfh, 0d4h, 0cdh, 0d4h, 0bah, 0d4h, 0a8h, 0d4h, 95h, 0d4h, 83h, 0d4h
	db 71h, 0d4h, 5eh, 0d4h, 4ch, 0d4h, 39h, 0d4h, 27h, 0d4h, 15h, 0d4h, 3h, 0d4h, 0f0h, 0d3h
	db 0deh, 0d3h, 0cch, 0d3h, 0bah, 0d3h, 0a8h, 0d3h, 95h, 0d3h, 83h, 0d3h, 71h, 0d3h, 5fh, 0d3h
	db 4dh, 0d3h, 3bh, 0d3h, 29h, 0d3h, 17h, 0d3h, 6h, 0d3h, 0f4h, 0d2h, 0e2h, 0d2h, 0d0h, 0d2h
	db 0beh, 0d2h, 0adh, 0d2h, 9bh, 0d2h, 89h, 0d2h, 77h, 0d2h, 66h, 0d2h, 54h, 0d2h, 43h, 0d2h
	db 31h, 0d2h, 1fh, 0d2h, 0eh, 0d2h, 0fch, 0d1h, 0ebh, 0d1h, 0dah, 0d1h, 0c8h, 0d1h, 0b7h, 0d1h
	db 0a5h, 0d1h, 94h, 0d1h, 83h, 0d1h, 72h, 0d1h, 60h, 0d1h, 4fh, 0d1h, 3eh, 0d1h, 2dh, 0d1h
	db 1ch, 0d1h, 0bh, 0d1h, 0fah, 0d0h, 0e9h, 0d0h, 0d7h, 0d0h, 0c7h, 0d0h, 0b6h, 0d0h, 0a5h, 0d0h
	db 94h, 0d0h, 83h, 0d0h, 72h, 0d0h, 61h, 0d0h, 50h, 0d0h, 40h, 0d0h, 2fh, 0d0h, 1eh, 0d0h
	db 0eh, 0d0h, 0fdh, 0cfh, 0ech, 0cfh, 0dch, 0cfh, 0cbh, 0cfh, 0bbh, 0cfh, 0aah, 0cfh, 9ah, 0cfh
	db 89h, 0cfh, 79h, 0cfh, 69h, 0cfh, 58h, 0cfh, 48h, 0cfh, 38h, 0cfh, 27h, 0cfh, 17h, 0cfh
	db 7h, 0cfh, 0f7h, 0ceh, 0e7h, 0ceh, 0d7h, 0ceh, 0c7h, 0ceh, 0b6h, 0ceh, 0a6h, 0ceh, 96h, 0ceh
	db 86h, 0ceh, 77h, 0ceh, 67h, 0ceh, 57h, 0ceh, 47h, 0ceh, 37h, 0ceh, 27h, 0ceh, 18h, 0ceh
	db 8h, 0ceh, 0f8h, 0cdh, 0e9h, 0cdh, 0d9h, 0cdh, 0c9h, 0cdh, 0bah, 0cdh, 0aah, 0cdh, 9bh, 0cdh
	db 8bh, 0cdh, 7ch, 0cdh, 6ch, 0cdh, 5dh, 0cdh, 4eh, 0cdh, 3eh, 0cdh, 2fh, 0cdh, 20h, 0cdh
	db 11h, 0cdh, 1h, 0cdh, 0f2h, 0cch, 0e3h, 0cch, 0d4h, 0cch, 0c5h, 0cch, 0b6h, 0cch, 0a7h, 0cch
	db 98h, 0cch, 89h, 0cch, 7ah, 0cch, 6bh, 0cch, 5ch, 0cch, 4dh, 0cch, 3eh, 0cch, 30h, 0cch
	db 21h, 0cch, 12h, 0cch, 4h, 0cch, 0f5h, 0cbh, 0e6h, 0cbh, 0d8h, 0cbh, 0c9h, 0cbh, 0bbh, 0cbh
	db 0ach, 0cbh, 9eh, 0cbh, 8fh, 0cbh, 81h, 0cbh, 73h, 0cbh, 64h, 0cbh, 56h, 0cbh, 48h, 0cbh
	db 39h, 0cbh, 2bh, 0cbh, 1dh, 0cbh, 0fh, 0cbh, 1h, 0cbh, 0f3h, 0cah, 0e5h, 0cah, 0d7h, 0cah
	db 0c9h, 0cah, 0bbh, 0cah, 0adh, 0cah, 9fh, 0cah, 91h, 0cah, 83h, 0cah, 76h, 0cah, 68h, 0cah
	db 5ah, 0cah, 4ch, 0cah, 3fh, 0cah, 31h, 0cah, 23h, 0cah, 16h, 0cah, 8h, 0cah, 0fbh, 0c9h
	db 0edh, 0c9h, 0e0h, 0c9h, 0d3h, 0c9h, 0c5h, 0c9h, 0b8h, 0c9h, 0abh, 0c9h, 9dh, 0c9h, 90h, 0c9h
	db 83h, 0c9h, 76h, 0c9h, 69h, 0c9h, 5bh, 0c9h, 4eh, 0c9h, 41h, 0c9h, 34h, 0c9h, 27h, 0c9h
	db 1ah, 0c9h, 0eh, 0c9h, 1h, 0c9h, 0f4h, 0c8h, 0e7h, 0c8h, 0dah, 0c8h, 0ceh, 0c8h, 0c1h, 0c8h
	db 0b4h, 0c8h, 0a8h, 0c8h, 9bh, 0c8h, 8eh, 0c8h, 82h, 0c8h, 75h, 0c8h, 69h, 0c8h, 5ch, 0c8h
	db 50h, 0c8h, 44h, 0c8h, 37h, 0c8h, 2bh, 0c8h, 1fh, 0c8h, 12h, 0c8h, 6h, 0c8h, 0fah, 0c7h
	db 0eeh, 0c7h, 0e2h, 0c7h, 0d6h, 0c7h, 0cah, 0c7h, 0beh, 0c7h, 0b2h, 0c7h, 0a6h, 0c7h, 9ah, 0c7h
	db 8eh, 0c7h, 82h, 0c7h, 76h, 0c7h, 6bh, 0c7h, 5fh, 0c7h, 53h, 0c7h, 48h, 0c7h, 3ch, 0c7h
	db 30h, 0c7h, 25h, 0c7h, 19h, 0c7h, 0eh, 0c7h, 2h, 0c7h, 0f7h, 0c6h, 0ech, 0c6h, 0e0h, 0c6h
	db 0d5h, 0c6h, 0cah, 0c6h, 0beh, 0c6h, 0b3h, 0c6h, 0a8h, 0c6h, 9dh, 0c6h, 92h, 0c6h, 87h, 0c6h
	db 7ch, 0c6h, 71h, 0c6h, 66h, 0c6h, 5bh, 0c6h, 50h, 0c6h, 45h, 0c6h, 3ah, 0c6h, 2fh, 0c6h
	db 25h, 0c6h, 1ah, 0c6h, 0fh, 0c6h, 4h, 0c6h, 0fah, 0c5h, 0efh, 0c5h, 0e5h, 0c5h, 0dah, 0c5h
	db 0d0h, 0c5h, 0c5h, 0c5h, 0bbh, 0c5h, 0b0h, 0c5h, 0a6h, 0c5h, 9ch, 0c5h, 92h, 0c5h, 87h, 0c5h
	db 7dh, 0c5h, 73h, 0c5h, 69h, 0c5h, 5fh, 0c5h, 55h, 0c5h, 4bh, 0c5h, 41h, 0c5h, 37h, 0c5h
	db 2dh, 0c5h, 23h, 0c5h, 19h, 0c5h, 0fh, 0c5h, 5h, 0c5h, 0fch, 0c4h, 0f2h, 0c4h, 0e8h, 0c4h
	db 0dfh, 0c4h, 0d5h, 0c4h, 0cbh, 0c4h, 0c2h, 0c4h, 0b8h, 0c4h, 0afh, 0c4h, 0a6h, 0c4h, 9ch, 0c4h
	db 93h, 0c4h, 8ah, 0c4h, 80h, 0c4h, 77h, 0c4h, 6eh, 0c4h, 65h, 0c4h, 5ch, 0c4h, 52h, 0c4h
	db 49h, 0c4h, 40h, 0c4h, 37h, 0c4h, 2eh, 0c4h, 25h, 0c4h, 1dh, 0c4h, 14h, 0c4h, 0bh, 0c4h
	db 2h, 0c4h, 0f9h, 0c3h, 0f1h, 0c3h, 0e8h, 0c3h, 0dfh, 0c3h, 0d7h, 0c3h, 0ceh, 0c3h, 0c6h, 0c3h
	db 0bdh, 0c3h, 0b5h, 0c3h, 0ach, 0c3h, 0a4h, 0c3h, 9ch, 0c3h, 93h, 0c3h, 8bh, 0c3h, 83h, 0c3h
	db 7bh, 0c3h, 73h, 0c3h, 6ah, 0c3h, 62h, 0c3h, 5ah, 0c3h, 52h, 0c3h, 4ah, 0c3h, 42h, 0c3h
	db 3ah, 0c3h, 33h, 0c3h, 2bh, 0c3h, 23h, 0c3h, 1bh, 0c3h, 13h, 0c3h, 0ch, 0c3h, 4h, 0c3h
	db 0fdh, 0c2h, 0f5h, 0c2h, 0edh, 0c2h, 0e6h, 0c2h, 0deh, 0c2h, 0d7h, 0c2h, 0d0h, 0c2h, 0c8h, 0c2h
	db 0c1h, 0c2h, 0bah, 0c2h, 0b2h, 0c2h, 0abh, 0c2h, 0a4h, 0c2h, 9dh, 0c2h, 96h, 0c2h, 8fh, 0c2h
	db 88h, 0c2h, 81h, 0c2h, 7ah, 0c2h, 73h, 0c2h, 6ch, 0c2h, 65h, 0c2h, 5eh, 0c2h, 58h, 0c2h
	db 51h, 0c2h, 4ah, 0c2h, 44h, 0c2h, 3dh, 0c2h, 36h, 0c2h, 30h, 0c2h, 29h, 0c2h, 23h, 0c2h
	db 1dh, 0c2h, 16h, 0c2h, 10h, 0c2h, 0ah, 0c2h, 3h, 0c2h, 0fdh, 0c1h, 0f7h, 0c1h, 0f1h, 0c1h
	db 0ebh, 0c1h, 0e4h, 0c1h, 0deh, 0c1h, 0d8h, 0c1h, 0d2h, 0c1h, 0cch, 0c1h, 0c7h, 0c1h, 0c1h, 0c1h
	db 0bbh, 0c1h, 0b5h, 0c1h, 0afh, 0c1h, 0aah, 0c1h, 0a4h, 0c1h, 9eh, 0c1h, 99h, 0c1h, 93h, 0c1h
	db 8eh, 0c1h, 88h, 0c1h, 83h, 0c1h, 7dh, 0c1h, 78h, 0c1h, 73h, 0c1h, 6dh, 0c1h, 68h, 0c1h
	db 63h, 0c1h, 5eh, 0c1h, 58h, 0c1h, 53h, 0c1h, 4eh, 0c1h, 49h, 0c1h, 44h, 0c1h, 3fh, 0c1h
	db 3ah, 0c1h, 35h, 0c1h, 31h, 0c1h, 2ch, 0c1h, 27h, 0c1h, 22h, 0c1h, 1eh, 0c1h, 19h, 0c1h
	db 14h, 0c1h, 10h, 0c1h, 0bh, 0c1h, 7h, 0c1h, 2h, 0c1h, 0feh, 0c0h, 0f9h, 0c0h, 0f5h, 0c0h
	db 0f1h, 0c0h, 0ech, 0c0h, 0e8h, 0c0h, 0e4h, 0c0h, 0e0h, 0c0h, 0dch, 0c0h, 0d8h, 0c0h, 0d4h, 0c0h
	db 0d0h, 0c0h, 0cch, 0c0h, 0c8h, 0c0h, 0c4h, 0c0h, 0c0h, 0c0h, 0bch, 0c0h, 0b8h, 0c0h, 0b5h, 0c0h
	db 0b1h, 0c0h, 0adh, 0c0h, 0aah, 0c0h, 0a6h, 0c0h, 0a2h, 0c0h, 9fh, 0c0h, 9bh, 0c0h, 98h, 0c0h
	db 95h, 0c0h, 91h, 0c0h, 8eh, 0c0h, 8bh, 0c0h, 87h, 0c0h, 84h, 0c0h, 81h, 0c0h, 7eh, 0c0h
	db 7bh, 0c0h, 78h, 0c0h, 75h, 0c0h, 72h, 0c0h, 6fh, 0c0h, 6ch, 0c0h, 69h, 0c0h, 66h, 0c0h
	db 63h, 0c0h, 61h, 0c0h, 5eh, 0c0h, 5bh, 0c0h, 59h, 0c0h, 56h, 0c0h, 53h, 0c0h, 51h, 0c0h
	db 4eh, 0c0h, 4ch, 0c0h, 4ah, 0c0h, 47h, 0c0h, 45h, 0c0h, 43h, 0c0h, 40h, 0c0h, 3eh, 0c0h
	db 3ch, 0c0h, 3ah, 0c0h, 38h, 0c0h, 36h, 0c0h, 34h, 0c0h, 32h, 0c0h, 30h, 0c0h, 2eh, 0c0h
	db 2ch, 0c0h, 2ah, 0c0h, 28h, 0c0h, 27h, 0c0h, 25h, 0c0h, 23h, 0c0h, 21h, 0c0h, 20h, 0c0h
	db 1eh, 0c0h, 1dh, 0c0h, 1bh, 0c0h, 1ah, 0c0h, 18h, 0c0h, 17h, 0c0h, 16h, 0c0h, 14h, 0c0h
	db 13h, 0c0h, 12h, 0c0h, 11h, 0c0h, 10h, 0c0h, 0fh, 0c0h, 0eh, 0c0h, 0dh, 0c0h, 0ch, 0c0h
	db 0bh, 0c0h, 0ah, 0c0h, 9h, 0c0h, 8h, 0c0h, 7h, 0c0h, 6h, 0c0h, 6h, 0c0h, 5h, 0c0h
	db 4h, 0c0h, 4h, 0c0h, 3h, 0c0h, 3h, 0c0h, 2h, 0c0h, 2h, 0c0h, 1h, 0c0h, 1h, 0c0h
	db 1h, 0c0h, 0h, 0c0h, 0h, 0c0h, 0h, 0c0h, 0h, 0c0h, 0h, 0c0h, 0h, 0c0h, 0h, 0c0h
	db 0h, 0c0h, 0h
_arctan_lookup endp
TRIG_TEXT ends
end
