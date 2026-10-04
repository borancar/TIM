; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `polygon.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `polygon.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `polygon.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs vmds=vmds vm_driver=vmdrv VM_SLOT_*
; JUDGE: assembler bc3.00

_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
_DATA segment word public 'DATA'
poly_top_at label byte
        db 0h, 0h
poly_bottom_at label byte
        db 0h, 0h
poly_right_count label byte
        db 0h, 0h
poly_left_count label byte
        db 0h, 0h
poly_remaining label byte
        db 0h, 0h
poly_at label byte
        db 0h, 0h
poly_chain label byte
        db 0h, 0h
poly_prev_x label byte
        db 0h, 0h
poly_prev_y label byte
        db 0h, 0h
poly_span_seg label byte
        db 0h, 0h
poly_outline_count label byte
        db 0h, 0h
poly_second_count label byte
        db 0h, 0h
poly_span_step label byte
        db 0h
poly_second_pass label byte
        db 0h
_DATA ends

extrn _clip_and_draw_line:far
extrn _clip_polygon:far
extrn _g_vm_driver:byte
extrn _g_compressed_body_vector:byte
extrn _g_vmds:byte
POLYGON_TEXT segment byte public 'CODE'
assume cs:POLYGON_TEXT, ds:DGROUP
public _draw_polygon, _poly_outline, _poly_edge_vertical, _poly_edge_steep
public _poly_edge_diagonal, _poly_edge_shallow_right, _poly_edge_shallow_left, _poly_walk
public _fill_rect, _draw_compressed_bitmap

; 0x20a77
_draw_polygon proc far
        push bp
        mov bp, sp
        push si
        push di
        push es
        sub ax, ax
        mov word ptr DGROUP:poly_span_seg, ax
        mov byte ptr DGROUP:poly_second_pass, al
        mov ax, ds
        mov es, ax
        mov ax, word ptr [bp+6]
        or ax, ax
        js poly_count_set
        mov word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count, ax
        mov cx, ax
        mov si, word ptr [bp+8]
        mov di, offset DGROUP:_g_vmds+vmds_poly_x
        rep movsw
        mov cx, ax
        mov si, word ptr [bp+0ah]
        mov di, offset DGROUP:_g_vmds+vmds_poly_y
        rep movsw
poly_count_set:
        cmp ax, 2
        jg poly_enough
        jl poly_too_few
poly_two_points:
        mov si, offset DGROUP:_g_vmds+vmds_poly_x
        mov di, offset DGROUP:_g_vmds+vmds_poly_y
        mov bp, 1
        call _poly_outline
poly_too_few:
        jmp poly_second_pass_check
poly_enough:
        mov al, byte ptr DGROUP:_g_vmds+vmds_fill_enabled
        or al, al
        jne poly_fill
        mov ax, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        mov bx, ax
        mov bp, ax
        shl bx, 1
        mov si, offset DGROUP:_g_vmds+vmds_poly_x
        mov di, offset DGROUP:_g_vmds+vmds_poly_y
        mov ax, word ptr [si]
        mov word ptr [bx+si], ax
        mov ax, word ptr [di]
        mov word ptr [bx+di], ax
        call _poly_outline
        jmp poly_second_pass_check
poly_fill:
        mov al, byte ptr DGROUP:_g_vmds+vmds_second_colour
        cmp al, byte ptr DGROUP:_g_vmds+vmds_fill_colour
        je poly_clip
        mov ax, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        mov word ptr DGROUP:poly_outline_count, ax
        mov bx, ax
        dec bx
        mov si, offset DGROUP:_g_vmds+vmds_poly_x
        mov di, offset DGROUP:_g_vmds+vmds_closed_x
        lodsw
        stosw
        mov cx, bx
        rep movsw
        stosw
        mov si, offset DGROUP:_g_vmds+vmds_poly_y
        mov di, offset DGROUP:_g_vmds+vmds_closed_y
        lodsw
        stosw
        mov cx, bx
        rep movsw
        stosw
poly_clip:
        mov al, byte ptr DGROUP:_g_vmds+vmds_clip_enabled
        or al, al
        je poly_clipped
        call FAR PTR _clip_polygon
poly_clipped:
        mov ax, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        cmp ax, 2
        je poly_two_points
        jl poly_too_few
        dec ax
        shl ax, 1
        mov si, ax
        mov ax, word ptr DGROUP:_g_vmds+vmds_poly_y
        mov word ptr DGROUP:poly_prev_y, ax
        mov dx, 7fffh
        mov bx, 8001h
        mov ax, word ptr DGROUP:_g_vmds+vmds_poly_x
        mov word ptr DGROUP:poly_prev_x, ax
        mov bp, dx
        mov cx, bx
        sub di, di
        sub ax, ax
        mov word ptr DGROUP:poly_top_at, ax
        mov word ptr DGROUP:poly_bottom_at, ax
poly_scan_point:
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        cmp ax, word ptr DGROUP:poly_prev_y
        jne poly_new_y
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        cmp ax, word ptr DGROUP:poly_prev_x
        je poly_scan_next
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
poly_new_y:
        mov word ptr DGROUP:poly_prev_y, ax
        mov word ptr _g_vmds[di+vmds_work_y], ax
        cmp ax, dx
        jg poly_check_bottom
        jl poly_new_top
        cmp word ptr _g_vmds[si+vmds_poly_x], cx
        jle poly_check_bottom
poly_new_top:
        mov word ptr DGROUP:poly_top_at, di
        mov dx, ax
        mov cx, word ptr _g_vmds[si+vmds_poly_x]
poly_check_bottom:
        cmp ax, bx
        jl poly_keep_point
        jg poly_new_bottom
        cmp word ptr _g_vmds[si+vmds_poly_x], bp
        jg poly_keep_point
poly_new_bottom:
        mov word ptr DGROUP:poly_bottom_at, di
        mov bx, ax
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
poly_keep_point:
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        mov word ptr DGROUP:poly_prev_x, ax
        mov word ptr _g_vmds[di+vmds_work_x], ax
        add di, 2
poly_scan_next:
        sub si, 2
        jge poly_scan_point
        cmp dx, bx
        jne poly_count_kept
poly_flat_line:
        cmp byte ptr DGROUP:_g_vmds+vmds_screen+vm_screen_mode_kind, 0
        jne poly_flat_line_halved
        push dx
        push cx
        push bx
        push bp
        call FAR PTR _clip_and_draw_line
        add sp, 8
poly_flat_done:
        jmp poly_second_pass_check
poly_flat_line_halved:
        shr word ptr DGROUP:_g_vmds+vmds_clip_top, 1
        shr word ptr DGROUP:_g_vmds+vmds_clip_bottom, 1
        sar dx, 1
        push dx
        push cx
        sar bx, 1
        push bx
        push bp
        call FAR PTR _clip_and_draw_line
        add sp, 8
        shl word ptr DGROUP:_g_vmds+vmds_clip_top, 1
        shl word ptr DGROUP:_g_vmds+vmds_clip_bottom, 1
        jmp poly_second_pass_check
poly_count_kept:
        mov ax, di
        shr ax, 1
        cmp ax, 2
        je poly_flat_line
        jl poly_flat_done
        mov cx, di
        mov word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count, ax
        mov ax, ds
        mov es, ax
        mov ax, word ptr DGROUP:poly_top_at
        mov si, ax
        mov di, si
        add di, 2
        cmp di, cx
        sbb ax, ax
        and di, ax
        mov dx, word ptr _g_vmds[di+vmds_work_x]
        sub dx, word ptr _g_vmds[si+vmds_work_x]
        mov bp, word ptr _g_vmds[di+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        jne poly_prev_edge
        inc bp
        or dx, dx
        mov dx, 7fffh
        jns poly_prev_edge
        neg dx
poly_prev_edge:
        mov di, si
        sub di, 2
        jge poly_prev_delta
        add di, cx
poly_prev_delta:
        mov ax, word ptr _g_vmds[di+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        mov bx, word ptr _g_vmds[di+vmds_work_y]
        sub bx, word ptr _g_vmds[si+vmds_work_y]
        jne poly_order_signs
        inc bx
        or ax, ax
        mov ax, 8001h
        js poly_order_swap
        neg ax
poly_order_signs:
        or ax, ax
        js poly_order_swap
        or dx, dx
        jle poly_reverse
        jmp short poly_compare_slopes
poly_order_swap:
        or dx, dx
        jge poly_keep_order
        neg dx
        neg ax
        xchg dx, ax
        xchg bp, bx
poly_compare_slopes:
        mov si, ax
        mov di, dx
        sub dx, dx
        div bx
        xchg di, ax
        mov si, dx
        sub dx, dx
        div bp
        cmp ax, di
        ja poly_keep_order
        jb poly_reverse
        sub ax, ax
        div bp
        mov di, ax
        sub ax, ax
        mov dx, si
        div bx
        cmp di, ax
        jb poly_reverse
        ja poly_keep_order
        mov byte ptr DGROUP:poly_second_pass, 1
        mov word ptr DGROUP:poly_second_count, cx
        mov dx, cx
        mov si, offset DGROUP:_g_vmds+vmds_work_x
        mov di, offset DGROUP:_g_vmds+vmds_closed_x
        shr cx, 1
        rep movsw
        mov cx, dx
        mov si, offset DGROUP:_g_vmds+vmds_work_y
        mov di, offset DGROUP:_g_vmds+vmds_closed_y
        shr cx, 1
        rep movsw
        mov cx, dx
poly_keep_order:
        mov si, offset DGROUP:_g_vmds+vmds_work_x
        mov di, offset DGROUP:_g_vmds+vmds_poly_x
        mov dx, cx
        shr cx, 1
        rep movsw
        mov si, offset DGROUP:_g_vmds+vmds_work_y
        mov di, offset DGROUP:_g_vmds+vmds_poly_y
        mov cx, dx
        shr cx, 1
        rep movsw
        mov cx, dx
        jmp short poly_chains
poly_reverse:
        mov dx, cx
        mov si, offset DGROUP:_g_vmds+vmds_work_x
        mov di, offset DGROUP:_g_vmds+vmds_poly_x
        add di, dx
        shr cx, 1
poly_reverse_x:
        lodsw
        dec di
        dec di
        mov word ptr [di], ax
        loop poly_reverse_x
        mov si, offset DGROUP:_g_vmds+vmds_work_y
        mov di, offset DGROUP:_g_vmds+vmds_poly_y
        add di, dx
        mov cx, dx
        shr cx, 1
poly_reverse_y:
        lodsw
        dec di
        dec di
        mov word ptr [di], ax
        loop poly_reverse_y
        mov cx, dx
        sub dx, 2
        mov ax, dx
        sub ax, word ptr DGROUP:poly_top_at
        mov word ptr DGROUP:poly_top_at, ax
        mov ax, dx
        sub ax, word ptr DGROUP:poly_bottom_at
        mov word ptr DGROUP:poly_bottom_at, ax
poly_chains:
        mov ax, word ptr DGROUP:poly_bottom_at
        mov bx, ax
        mov dx, word ptr _g_vmds[bx+vmds_poly_y]
        mov ax, word ptr DGROUP:poly_top_at
        mov si, ax
        sub di, di
        jmp short poly_right_point
poly_right_next:
        add si, 2
        cmp si, cx
        sbb ax, ax
        and si, ax
poly_right_point:
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        mov word ptr _g_vmds[di+vmds_work_x], ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        cmp ax, dx
        jl poly_right_next
        mov ax, di
        shr ax, 1
        mov word ptr DGROUP:poly_right_count, ax
        mov ax, word ptr DGROUP:poly_top_at
        mov bx, ax
        mov dx, word ptr _g_vmds[bx+vmds_poly_y]
        mov ax, word ptr DGROUP:poly_bottom_at
        mov si, ax
        jmp short poly_left_point
poly_left_next:
        add si, 2
        cmp si, cx
        sbb ax, ax
        and si, ax
poly_left_point:
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        mov word ptr _g_vmds[di+vmds_work_x], ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        cmp ax, dx
        jg poly_left_next
        mov ax, di
        shr ax, 1
        sub ax, word ptr DGROUP:poly_right_count
        mov word ptr DGROUP:poly_left_count, ax
        mov ax, word ptr DGROUP:_g_vm_driver
        mov es, ax
        mov word ptr DGROUP:poly_chain, 2
        mov word ptr DGROUP:poly_at, 0
        mov ax, word ptr DGROUP:poly_right_count
        jmp short poly_edge
poly_edge_next:
        mov ax, word ptr DGROUP:poly_remaining
poly_edge:
        dec ax
        je poly_chain_done
        mov word ptr DGROUP:poly_remaining, ax
        mov ax, word ptr DGROUP:poly_at
        mov si, ax
        add ax, 2
        mov word ptr DGROUP:poly_at, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        mov bx, ax
        mov bp, word ptr _g_vmds[si+vmds_work_x+2*1]
        mov cx, word ptr _g_vmds[si+vmds_work_y]
        mov si, word ptr _g_vmds[si+vmds_work_y+2*1]
        sub ax, bp
        cwd
        xor ax, dx
        sub ax, dx
        je poly_vertical
        mov di, ax
        mov ax, cx
        sub ax, si
        cwd
        xor ax, dx
        sub ax, dx
        je poly_horizontal
        cmp di, ax
        jl poly_steep
        jg poly_shallow
        call _poly_edge_diagonal
        jmp short poly_edge_next
poly_vertical:
        call _poly_edge_vertical
        jmp short poly_edge_next
poly_horizontal:
        cmp bx, bp
        jl poly_horizontal_store
        xchg bx, bp
poly_horizontal_store:
        mov di, cx
        shl di, 1
        shl di, 1
        mov ax, word ptr DGROUP:poly_chain
        add di, ax
        or ax, ax
        mov ax, bx
        je poly_horizontal_x
        mov ax, bp
poly_horizontal_x:
        stosw
        jmp short poly_edge_next
poly_shallow:
        cmp word ptr DGROUP:poly_chain, 0
        jne poly_shallow_right
        call _poly_edge_shallow_left
        jmp short poly_edge_next
poly_shallow_right:
        call _poly_edge_shallow_right
        jmp short poly_edge_next
poly_steep:
        call _poly_edge_steep
        jmp short poly_edge_next
poly_chain_done:
        mov ax, word ptr DGROUP:poly_chain
        or ax, ax
        je poly_spans
        add word ptr DGROUP:poly_at, 2
        sub ax, ax
        mov word ptr DGROUP:poly_chain, ax
        mov ax, word ptr DGROUP:poly_left_count
        jmp poly_edge
poly_spans:
        mov ax, word ptr DGROUP:poly_top_at
        mov bx, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov dx, ax
        shl ax, 1
        shl ax, 1
        mov si, ax
        mov word ptr DGROUP:poly_span_seg, es
        mov ax, word ptr DGROUP:poly_bottom_at
        mov bx, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, dx
        inc ax
        mov cx, es
        dec cx
        mov es, cx
        add si, 0ch
        mov word ptr es:[si], dx
        mov word ptr es:[si+2], ax
        call dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_FILL_SPANS
        mov al, byte ptr DGROUP:_g_vmds+vmds_second_colour
        cmp al, byte ptr DGROUP:_g_vmds+vmds_fill_colour
        je poly_second_pass_check
        mov ax, word ptr DGROUP:poly_outline_count
        mov bp, ax
        mov si, offset DGROUP:_g_vmds+vmds_closed_x
        mov di, offset DGROUP:_g_vmds+vmds_closed_y
        call _poly_outline
poly_second_pass_check:
        mov al, byte ptr DGROUP:poly_second_pass
        or al, al
        je poly_return
        mov byte ptr DGROUP:poly_second_pass, 0
        mov ax, ds
        mov es, ax
        mov cx, word ptr DGROUP:poly_second_count
        mov dx, cx
        mov si, offset DGROUP:_g_vmds+vmds_closed_x
        mov di, offset DGROUP:_g_vmds+vmds_work_x
        shr cx, 1
        rep movsw
        mov cx, dx
        mov si, offset DGROUP:_g_vmds+vmds_closed_y
        mov di, offset DGROUP:_g_vmds+vmds_work_y
        shr cx, 1
        rep movsw
        mov cx, dx
        jmp poly_reverse
poly_return:
        mov ax, word ptr DGROUP:poly_span_seg
        pop es
        pop di
        pop si
        pop bp
        retf
_draw_polygon endp

; 0x20ea3
_poly_outline proc near
        cmp byte ptr DGROUP:_g_vmds+vmds_screen+vm_screen_mode_kind, 0
        jne outline_halved
outline_line:
        push word ptr [di]
        lodsw
        push ax
        add di, 2
        push word ptr [di]
        push word ptr [si]
        call FAR PTR _clip_and_draw_line
        add sp, 8
        dec bp
        jne outline_line
        ret
outline_halved:
        shr word ptr DGROUP:_g_vmds+vmds_clip_top, 1
        shr word ptr DGROUP:_g_vmds+vmds_clip_bottom, 1
outline_halved_line:
        mov ax, word ptr [di]
        add di, 2
        sar ax, 1
        push ax
        lodsw
        push ax
        mov ax, word ptr [di]
        sar ax, 1
        push ax
        mov ax, word ptr [si]
        push ax
        call FAR PTR _clip_and_draw_line
        add sp, 8
        dec bp
        jne outline_halved_line
        shl word ptr DGROUP:_g_vmds+vmds_clip_top, 1
        shl word ptr DGROUP:_g_vmds+vmds_clip_bottom, 1
        ret
_poly_outline endp

; 0x20eef
_poly_edge_vertical proc near
        cmp cx, si
        jg vertical_ordered
        xchg cx, si
vertical_ordered:
        mov di, si
        sub cx, si
        inc cx
        mov byte ptr DGROUP:poly_span_step, 2
        mov ax, bp
        sub bx, bx
        mov si, bx
        mov bp, bx
        jmp _poly_walk
        ret
_poly_edge_vertical endp

; 0x20f0b
_poly_edge_steep proc near
        cmp bx, bp
        jl steep_ordered
        xchg bx, bp
        xchg cx, si
steep_ordered:
        mov di, cx
        shl di, 1
        shl di, 1
        mov ax, word ptr DGROUP:poly_chain
        add di, ax
        mov ax, cx
        sub ax, si
        cwd
        xor ax, dx
        sub ax, dx
        mov cx, ax
        mov ax, bp
        sub ax, bx
        mov si, ax
        shl si, 1
        mov bp, ax
        mov ax, bx
        mov bx, si
        sub bx, cx
        sub bp, cx
        shl bp, 1
        xor bp, si
        inc cx
        or dx, dx
        je steep_rows_up
steep_rows_down:
        stosw
        inc di
        inc di
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_down_done
        stosw
        inc di
        inc di
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_down_done
        stosw
        inc di
        inc di
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_down_done
        stosw
        inc di
        inc di
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_down_done
        stosw
        inc di
        inc di
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_down_done
        stosw
        inc di
        inc di
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_down_done
        jmp short steep_rows_down
steep_down_done:
        ret
steep_rows_up:
        stosw
        add di, -6
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_up_done
        stosw
        add di, -6
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_up_done
        stosw
        add di, -6
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_up_done
        stosw
        add di, -6
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_up_done
        stosw
        add di, -6
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_up_done
        stosw
        add di, -6
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_up_done
        stosw
        add di, -6
        cmp bh, 80h
        sbb dx, dx
        sub ax, dx
        and dx, bp
        xor dx, si
        add bx, dx
        dec cx
        je steep_up_done
        jmp steep_rows_up
steep_up_done:
        ret
_poly_edge_steep endp

; 0x21049
_poly_edge_diagonal proc near
        cmp cx, si
        jl diagonal_ordered
        xchg bx, bp
        xchg cx, si
diagonal_ordered:
        mov di, cx
        sub cx, si
        neg cx
        inc cx
        mov si, 1
        cmp bx, bp
        jl diagonal_walk
        neg si
diagonal_walk:
        mov ax, bx
        sub bx, bx
        mov bp, bx
        mov byte ptr DGROUP:poly_span_step, 2
        jmp _poly_walk
        ret
_poly_edge_diagonal endp

; 0x21070
_poly_edge_shallow_right proc near
        cmp bx, bp
        jg sright_ordered
        xchg bx, bp
        xchg cx, si
sright_ordered:
        mov di, cx
        shl di, 1
        inc di
        shl di, 1
        mov dx, 2
        sub si, cx
        jge sright_dir_set
        neg si
        mov dx, 0fffah
sright_dir_set:
        mov cx, si
        sub bp, bx
        jle sright_dx_negated
        neg bp
sright_dx_negated:
        mov ax, bp
        add bp, si
        shl bp, 1
        shl si, 1
        add ax, si
        xchg bx, ax
        stosw
        add di, dx
        dec ax
        or bx, bx
        jl sright_run
        jmp short sright_row
sright_run:
        dec ax
        add bx, si
        jge sright_row
        dec ax
        add bx, si
        jge sright_row
        dec ax
        add bx, si
        jge sright_row
        dec ax
        add bx, si
        jge sright_row
        dec ax
        add bx, si
        jge sright_row
        dec ax
        add bx, si
        jge sright_row
        dec ax
        add bx, si
        jge sright_row
        dec ax
        add bx, si
        jge sright_row
        jmp short sright_run
sright_row_loop:
        add bx, bp
        jl sright_run
sright_row:
        stosw
        add di, dx
        dec ax
        dec cx
        je sright_done
        add bx, bp
        jl sright_run
        stosw
        add di, dx
        dec ax
        dec cx
        je sright_done
        add bx, bp
        jl sright_run
        stosw
        add di, dx
        dec ax
        dec cx
        je sright_done
        add bx, bp
        jl sright_run
        stosw
        add di, dx
        dec ax
        dec cx
        je sright_done
        add bx, bp
        jl sright_run
        stosw
        add di, dx
        dec ax
        dec cx
        je sright_done
        add bx, bp
        jl sright_run
        stosw
        add di, dx
        dec ax
        dec cx
        je sright_done
        add bx, bp
        jl sright_run
        stosw
        add di, dx
        dec ax
        dec cx
        je sright_done
        add bx, bp
        jl sright_run
        stosw
        add di, dx
        dec ax
        dec cx
        jne sright_row_loop
sright_done:
        ret
_poly_edge_shallow_right endp

; 0x2112b
_poly_edge_shallow_left proc near
        cmp bx, bp
        jl sleft_ordered
        xchg bx, bp
        xchg cx, si
sleft_ordered:
        mov di, cx
        shl di, 1
        shl di, 1
        mov dx, 2
        sub si, cx
        jge sleft_dir_set
        neg si
        mov dx, 0fffah
sleft_dir_set:
        mov cx, si
        sub bp, bx
        jle sleft_dx_negated
        neg bp
sleft_dx_negated:
        mov ax, bp
        add bp, si
        shl bp, 1
        shl si, 1
        add ax, si
        xchg bx, ax
        stosw
        add di, dx
        inc ax
        or bx, bx
        jl sleft_run
        jmp short sleft_row
sleft_run:
        inc ax
        add bx, si
        jge sleft_row
        inc ax
        add bx, si
        jge sleft_row
        inc ax
        add bx, si
        jge sleft_row
        inc ax
        add bx, si
        jge sleft_row
        inc ax
        add bx, si
        jge sleft_row
        inc ax
        add bx, si
        jge sleft_row
        inc ax
        add bx, si
        jge sleft_row
        inc ax
        add bx, si
        jge sleft_row
        jmp short sleft_run
sleft_row_loop:
        add bx, bp
        jl sleft_run
sleft_row:
        stosw
        add di, dx
        inc ax
        dec cx
        je sleft_done
        add bx, bp
        jl sleft_run
        stosw
        add di, dx
        inc ax
        dec cx
        je sleft_done
        add bx, bp
        jl sleft_run
        stosw
        add di, dx
        inc ax
        dec cx
        je sleft_done
        add bx, bp
        jl sleft_run
        stosw
        add di, dx
        inc ax
        dec cx
        je sleft_done
        add bx, bp
        jl sleft_run
        stosw
        add di, dx
        inc ax
        dec cx
        je sleft_done
        add bx, bp
        jl sleft_run
        stosw
        add di, dx
        inc ax
        dec cx
        je sleft_done
        add bx, bp
        jl sleft_run
        stosw
        add di, dx
        inc ax
        dec cx
        je sleft_done
        add bx, bp
        jl sleft_run
        stosw
        add di, dx
        inc ax
        dec cx
        jne sleft_row_loop
sleft_done:
        ret
        stosw
        add di, cx
        add bp, bx
        adc ax, si
_poly_edge_shallow_left endp

; 0x211ec
_poly_walk proc near
        push ax
        shl di, 1
        shl di, 1
        mov ax, word ptr DGROUP:poly_chain
        add di, ax
        mov dx, cx
        xor ax, ax
        add ax, dx
        shl dx, 1
        add ax, dx
        shl dx, 1
        add ax, dx
        mov dx, offset poly_walk_end
        sub dx, ax
        mov al, byte ptr DGROUP:poly_span_step
        cbw
        mov cx, ax
        pop ax
        jmp dx
        rept 400
        stosw
        add di, cx
        add bp, bx
        adc ax, si
        endm
poly_walk_end:
        ret
_poly_walk endp

; 0x21d03
_fill_rect proc far
        push bp
        mov bp, sp
        sub sp, 6
        push di
        push si
        mov ax, word ptr [bp+0ah]
        add ax, word ptr [bp+6]
        dec ax
        mov word ptr [bp-4], ax
        mov ax, word ptr [bp+0ch]
        add ax, word ptr [bp+8]
        dec ax
        mov word ptr [bp-6], ax
        cmp byte ptr DGROUP:_g_vmds+vmds_fill_enabled, 0
        jne rect_fill
        jmp rect_outline
rect_fill:
        push word ptr [bp+6]
        push word ptr [bp+8]
        cmp byte ptr DGROUP:_g_vmds+vmds_clip_enabled, 0
        je rect_spans
        mov ax, word ptr [bp+6]
        sub ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr [bp-2], ax
        or ax, ax
        jge rect_clip_top
        sub word ptr [bp+6], ax
        add word ptr [bp+0ah], ax
rect_clip_top:
        mov ax, word ptr [bp+8]
        sub ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr [bp-2], ax
        or ax, ax
        jge rect_clip_right
        sub word ptr [bp+8], ax
        add word ptr [bp+0ch], ax
rect_clip_right:
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        sub ax, word ptr [bp-4]
        mov word ptr [bp-2], ax
        or ax, ax
        jge rect_clip_bottom
        add word ptr [bp+0ah], ax
rect_clip_bottom:
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        sub ax, word ptr [bp-6]
        mov word ptr [bp-2], ax
        or ax, ax
        jge rect_spans
        add word ptr [bp+0ch], ax
rect_spans:
        cmp word ptr [bp+0ah], 0
        jle rect_filled
        cmp word ptr [bp+0ch], 0
        jle rect_filled
        mov es, word ptr DGROUP:_g_vm_driver
        xor di, di
        mov ax, word ptr [bp+8]
        stosw
        mov ax, word ptr [bp+0ch]
        mov cx, ax
        stosw
        mov bx, word ptr [bp+6]
        mov dx, bx
        add dx, word ptr [bp+0ah]
        dec dx
rect_span:
        mov ax, bx
        stosw
        mov ax, dx
        stosw
        loop rect_span
        xor si, si
        push bp
        call dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_FILL_SPANS
        pop bp
rect_filled:
        pop word ptr [bp+8]
        pop word ptr [bp+6]
rect_outline:
        cmp byte ptr DGROUP:_g_vmds+vmds_fill_enabled, 0
        je rect_outline_draw
        mov al, byte ptr DGROUP:_g_vmds+vmds_second_colour
        cmp byte ptr DGROUP:_g_vmds+vmds_fill_colour, al
        je rect_return
rect_outline_draw:
        mov si, word ptr [bp+6]
        mov di, word ptr [bp+8]
        mov ax, word ptr [bp-6]
        push ax
        mov ax, word ptr [bp-4]
        push ax
        push di
        push ax
        call FAR PTR _clip_and_draw_line
        add sp, 4
        mov ax, word ptr [bp-6]
        push ax
        push si
        call FAR PTR _clip_and_draw_line
        add sp, 6
        push si
        push di
        push si
        call FAR PTR _clip_and_draw_line
        add sp, 8
        push di
        mov ax, word ptr [bp-4]
        push ax
        push di
        push si
        call FAR PTR _clip_and_draw_line
        add sp, 8
rect_return:
        pop si
        pop di
        mov sp, bp
        pop bp
        retf
        nop
_fill_rect endp

; 0x21e0f
_draw_compressed_bitmap proc near
        jmp dword ptr DGROUP:_g_compressed_body_vector
_draw_compressed_bitmap endp
POLYGON_TEXT ends
end
