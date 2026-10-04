; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `lzw.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `lzw.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `lzw.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs resource=res engine_stream=strm
; JUDGE: assembler bc3.00

_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
_DATA segment word public 'DATA'
        db 0h, 1h, 3h, 7h, 0fh, 1fh, 3fh, 7fh, 0ffh, 0h
lzw_code_buf label byte
        db 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch, 0ch
lzw_rmask label byte
        db 0h, 1h, 3h, 7h, 0fh, 1fh, 3fh, 7fh, 0ffh
lzw_resume_src label byte
        db 0h, 0h, 0h
rle_spill_at label byte
        db 0h, 0h
_DATA ends

extrn _g_engine_resource_flags:byte
extrn _g_engine_stream:byte
LZW_TEXT segment byte public 'CODE'
assume cs:LZW_TEXT, ds:DGROUP
extrn _emit_byte:near
extrn _read_input_block:near
public _decompress_lzw, _next_lzw_code, _rle_from_memory
lzw_first_code:
        mov byte ptr DGROUP:_g_engine_stream+strm_first_code, 0
        mov bp, es
        call _next_lzw_code
        mov word ptr DGROUP:_g_engine_stream+strm_oldcode, ax
        mov word ptr DGROUP:_g_engine_stream+strm_finchar, ax
        push ax
        call _emit_byte
        add sp, 2
        mov es, bp
        jmp short lzw_next_code
lzw_pad_90 db 90h

; 0x1ca62
_decompress_lzw proc near
        push bp
        push si
        push di
        mov ax, word ptr DGROUP:_g_engine_stream+strm_scratch+2
        add ax, 372h
        mov es, ax
        cmp byte ptr DGROUP:_g_engine_stream+strm_resume, 0
        je lzw_fresh
        mov cx, word ptr DGROUP:_g_engine_stream+strm_wanted
        inc cx
        mov bp, es
        les di, dword ptr DGROUP:_g_engine_stream+strm_output
        mov al, byte ptr DGROUP:_g_engine_resource_flags
        mov si, word ptr DGROUP:lzw_resume_src
        mov byte ptr DGROUP:_g_engine_stream+strm_resume, 0
        mov dx, ds
        mov ds, bp
        mov bx, 2
        test al, 40h
        je lzw_resume_skip
        jmp lzw_copy_next
lzw_resume_skip:
        jmp lzw_skip_next
lzw_fresh:
        cmp byte ptr DGROUP:_g_engine_stream+strm_first_code, 0
        jne lzw_first_code
lzw_next_code:
        mov bp, es
        call _next_lzw_code
        mov es, bp
        cmp ax, 0
        jl lzw_end
        cmp ax, 100h
        jne lzw_code
        mov bp, es
        les di, dword ptr DGROUP:_g_engine_stream+strm_scratch
        mov ax, di
        mov cx, 100h
        rep stosw
        inc ax
        mov word ptr DGROUP:_g_engine_stream+strm_clear_flg, ax
        xchg ah, al
        mov word ptr DGROUP:_g_engine_stream+strm_free_ent, ax
        call _next_lzw_code
        mov es, bp
        cmp ax, 0
        jge lzw_code
lzw_end:
        pop di
        pop si
        pop bp
        ret
lzw_code:
        sub di, di
        mov si, ax
        mov word ptr DGROUP:_g_engine_stream+strm_incode, ax
        cmp ax, word ptr DGROUP:_g_engine_stream+strm_free_ent
        jl lzw_unwind
        mov ax, word ptr DGROUP:_g_engine_stream+strm_finchar
        stosb
        mov si, word ptr DGROUP:_g_engine_stream+strm_oldcode
lzw_unwind:
        mov dx, ds
        mov ax, word ptr DGROUP:_g_engine_stream+strm_scratch+2
        mov ds, ax
        mov cx, 100h
        mov bx, 2720h
lzw_chain:
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        cmp si, cx
        jl lzw_chain_end
        mov al, byte ptr [bx+si]
        stosb
        shl si, 1
        mov si, word ptr [si]
        jmp short lzw_chain
lzw_chain_end:
        mov al, byte ptr [bx+si]
        stosb
        mov ds, dx
        mov ah, 0
        mov word ptr DGROUP:_g_engine_stream+strm_finchar, ax
        mov cx, word ptr DGROUP:_g_engine_stream+strm_wanted
        inc cx
        mov si, di
        dec si
        mov bp, es
        les di, dword ptr DGROUP:_g_engine_stream+strm_output
        mov al, byte ptr DGROUP:_g_engine_resource_flags
        mov dx, ds
        mov ds, bp
        mov bx, 2
        test al, 40h
        je lzw_skip
lzw_copy:
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
lzw_copy_next:
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        lodsb
        dec cx
        je lzw_wanted_met
        stosb
        sub si, bx
        jl lzw_string_done
        jmp short lzw_copy
lzw_skip:
        lodsb
        dec cx
        je lzw_wanted_met
        inc di
lzw_skip_next:
        sub si, bx
        jl lzw_string_done
        jmp short lzw_skip
lzw_wanted_met:
        mov ds, dx
        mov word ptr DGROUP:_g_engine_stream+strm_output, di
        mov word ptr DGROUP:lzw_resume_src, si
        mov si, word ptr DGROUP:_g_engine_stream+strm_rec
        mov bl, byte ptr [si+res_spill_end]
        inc word ptr [si+res_spill_end]
        sub bh, bh
        mov si, word ptr DGROUP:_g_engine_stream+strm_spill
        mov byte ptr [bx+si], al
        sub ax, ax
        mov word ptr DGROUP:_g_engine_stream+strm_wanted, ax
        inc ax
        mov byte ptr DGROUP:_g_engine_stream+strm_resume, al
        pop di
        pop si
        pop bp
        ret
lzw_string_done:
        mov ax, ds
        mov es, ax
        mov ds, dx
        dec cx
        mov word ptr DGROUP:_g_engine_stream+strm_wanted, cx
        mov word ptr DGROUP:_g_engine_stream+strm_output, di
        mov ax, word ptr DGROUP:_g_engine_stream+strm_free_ent
        cmp ax, 1000h
        jge lzw_table_full
        mov di, ax
        shl di, 1
        mov ax, word ptr DGROUP:_g_engine_stream+strm_oldcode
        mov bx, es
        mov bp, es
        sub bx, 372h
        mov es, bx
        stosw
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_g_engine_stream+strm_free_ent, ax
        add di, 271fh
        mov ax, word ptr DGROUP:_g_engine_stream+strm_finchar
        stosb
        mov es, bp
lzw_table_full:
        mov ax, word ptr DGROUP:_g_engine_stream+strm_incode
        mov word ptr DGROUP:_g_engine_stream+strm_oldcode, ax
        jmp lzw_next_code
_decompress_lzw endp

; 0x1cc65
_next_lzw_code proc near
        mov ax, word ptr DGROUP:_g_engine_stream+strm_free_ent
        cmp ax, word ptr DGROUP:_g_engine_stream+strm_maxcode
        jg code_widen
        cmp word ptr DGROUP:_g_engine_stream+strm_clear_flg, 0
        jne code_clear
        mov ax, word ptr DGROUP:_g_engine_stream+strm_bit_pos
        cmp ax, word ptr DGROUP:_g_engine_stream+strm_bit_end
        jge code_refill
code_extract:
        mov si, offset DGROUP:lzw_code_buf
        mov bx, word ptr DGROUP:_g_engine_stream+strm_n_bits
        mov ch, al
        mov dx, ax
        add ax, bx
        mov word ptr DGROUP:_g_engine_stream+strm_bit_pos, ax
        shr dx, 1
        shr dx, 1
        shr dx, 1
        add si, dx
        and ch, 7
        lodsb
        mov cl, ch
        shr ax, cl
        mov dx, ax
        sub ch, 8
        neg ch
        sub bl, ch
        cmp bl, 8
        jl code_last_bits
        lodsb
        mov cl, ch
        shl ax, cl
        or dx, ax
        add ch, 8
        sub bl, 8
code_last_bits:
        sub ax, ax
        mov al, byte ptr lzw_rmask[bx]
        and al, byte ptr [si]
        mov cl, ch
        shl ax, cl
        or ax, dx
        ret
code_widen:
        mov cx, word ptr DGROUP:_g_engine_stream+strm_n_bits
        inc cx
        mov word ptr DGROUP:_g_engine_stream+strm_n_bits, cx
        mov ax, 1000h
        cmp cl, 0ch
        je code_set_max
        mov ax, 1
        shl ax, cl
        dec ax
code_set_max:
        mov word ptr DGROUP:_g_engine_stream+strm_maxcode, ax
        cmp word ptr DGROUP:_g_engine_stream+strm_clear_flg, 0
        je code_refill
code_clear:
        mov ax, 9
        mov word ptr DGROUP:_g_engine_stream+strm_n_bits, ax
        mov ax, 1ffh
        mov word ptr DGROUP:_g_engine_stream+strm_maxcode, ax
        mov word ptr DGROUP:_g_engine_stream+strm_clear_flg, 0
code_refill:
        mov si, word ptr DGROUP:_g_engine_stream+strm_n_bits
        push si
        mov ax, 35bch
        push ax
        call _read_input_block
        add sp, 4
        sub bx, bx
        cmp ax, bx
        jle code_eof
        mov word ptr DGROUP:_g_engine_stream+strm_bit_pos, bx
        shl ax, 1
        shl ax, 1
        shl ax, 1
        dec si
        sub ax, si
        mov word ptr DGROUP:_g_engine_stream+strm_bit_end, ax
        mov ax, bx
        jmp code_extract
code_eof:
        mov word ptr DGROUP:_g_engine_stream+strm_bit_end, ax
        mov ax, 0ffffh
        ret
_next_lzw_code endp

; 0x1cd2c
_rle_from_memory proc near
        push bp
        push si
        push di
        sub cx, cx
        mov bx, cx
        mov si, word ptr DGROUP:_g_engine_stream+strm_rec
        mov cl, byte ptr [si+res_spill_end]
        mov ax, word ptr DGROUP:_g_engine_stream+strm_spill
        add ax, cx
        mov word ptr DGROUP:rle_spill_at, ax
        mov ax, word ptr [si+res_in_end]
        mov dx, word ptr [si+res_in_end+2]
        sub ax, word ptr [si+res_in_pos]
        sbb dx, word ptr [si+res_in_pos+2]
        mov bp, 0ffffh
        jne rle_start
        mov bp, ax
rle_start:
        les di, dword ptr DGROUP:_g_engine_stream+strm_output
        mov dx, word ptr DGROUP:_g_engine_stream+strm_wanted
        mov al, byte ptr DGROUP:_g_engine_resource_flags
        lds si, dword ptr DGROUP:_g_engine_stream+strm_input
        test al, 40h
        je rle_skip
rle_copy:
        sub cx, cx
        cmp bx, bp
        je rle_input_done
        lodsb
        shl al, 1
        jae rle_literal
        shr al, 1
        mov cl, al
        lodsb
        add bx, 2
        sub dx, cx
        jb rle_fill_spill
        mov ah, al
        test di, 1
        je rle_fill_words
        stosb
        dec cx
rle_fill_words:
        shr cx, 1
        rep stosw
        rcl cx, 1
        rep stosb
        jmp short rle_copy
rle_literal:
        shr al, 1
        mov cl, al
        add bx, cx
        inc bx
        sub dx, cx
        jb rle_literal_spill
        test di, 1
        je rle_literal_words
        movsb
        dec cx
rle_literal_words:
        shr cx, 1
        rep movsw
        rcl cx, 1
        rep movsb
        jmp short rle_copy
rle_input_done:
        mov bp, cx
        jmp short rle_return
rle_pad_90 db 90h
rle_skip:
        sub cx, cx
        cmp bx, bp
        je rle_input_done
        lodsb
        shl al, 1
        jae rle_skip_literal
        shr al, 1
        mov cl, al
        lodsb
        add bx, 2
        sub dx, cx
        jae rle_skip
rle_fill_spill:
        add dx, cx
        mov bp, ss
        mov es, bp
        mov word ptr ss:[5894h], di
        mov di, word ptr ss:[35d4h]
        mov bp, cx
        rep stosb
rle_return:
        mov ax, ss
        mov ds, ax
        mov si, word ptr DGROUP:_g_engine_stream+strm_rec
        mov ax, bp
        add byte ptr [si+res_spill_end], al
        mov word ptr DGROUP:_g_engine_stream+strm_wanted, dx
        add word ptr [si+res_in_pos], bx
        adc word ptr [si+res_in_pos+2], 0
        pop di
        pop si
        pop bp
        ret
rle_skip_literal:
        shr al, 1
        mov cl, al
        add bx, cx
        inc bx
        add si, cx
        sub dx, cx
        jae rle_skip
rle_literal_spill:
        add dx, cx
        mov bp, ss
        mov es, bp
        mov word ptr ss:[5894h], di
        mov di, word ptr ss:[35d4h]
        mov bp, cx
        rep movsb
        jmp short rle_return
_rle_from_memory endp
LZW_TEXT ends
end
