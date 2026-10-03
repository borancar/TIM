/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Filled polygons and rectangles**: `draw_polygon`, the edge walkers it
 * builds its span list with, `fill_rect`, and the thunk into
 * `draw_compressed_body`.
 *
 * The original's **code segment 1c25**, image 0x1eded..0x20189, split out of
 * engine.c on 2026-09-27. All of it is hand-written - the edge walkers pass
 * everything in registers and answer in them, `fill_rect` pushes its own
 * arguments and pops them back - so it is TASM source, the `#ifdef
 * __TURBOC__` block below, with the host's transcription in the `#else`.
 * The polygon's data is DGROUP 0x44d0..0x44ea.
 *
 * **Possibly more than one module.** Its start is the palette module's end,
 * which is C. `fill_rect` ends in a padding `nop` at 0x20184, which looks like
 * the end of a module, but nothing proves it, and nothing says which side
 * of that the thunk at 0x20185 is on. Its end is the C body the thunk jumps to.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: structs vmds=vmds vm_driver=vmdrv
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

/* 0x20a77 */
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
        call dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*27
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

/* 0x20ea3 */
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

/* 0x20eef */
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
c_1f280 db 0c3h
_poly_edge_vertical endp

/* 0x20f0b */
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

/* 0x21049 */
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
c_1f3e5 db 0c3h
_poly_edge_diagonal endp

/* 0x21070 */
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

/* 0x2112b */
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
c_1f55b db 0abh
c_1f55c db 3h, 0f9h
c_1f55e db 3h, 0ebh
c_1f560 db 13h, 0c6h
_poly_edge_shallow_left endp

/* 0x211ec */
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
        mov dx, 2eb2h
        sub dx, ax
        mov al, byte ptr DGROUP:poly_span_step
        cbw
        mov cx, ax
        pop ax
        jmp dx
c_1f588 db 0abh
c_1f589 db 3h, 0f9h
c_1f58b db 3h, 0ebh
c_1f58d db 13h, 0c6h
c_1f58f db 0abh
c_1f590 db 3h, 0f9h
c_1f592 db 3h, 0ebh
c_1f594 db 13h, 0c6h
c_1f596 db 0abh
c_1f597 db 3h, 0f9h
c_1f599 db 3h, 0ebh
c_1f59b db 13h, 0c6h
c_1f59d db 0abh
c_1f59e db 3h, 0f9h
c_1f5a0 db 3h, 0ebh
c_1f5a2 db 13h, 0c6h
c_1f5a4 db 0abh
c_1f5a5 db 3h, 0f9h
c_1f5a7 db 3h, 0ebh
c_1f5a9 db 13h, 0c6h
c_1f5ab db 0abh
c_1f5ac db 3h, 0f9h
c_1f5ae db 3h, 0ebh
c_1f5b0 db 13h, 0c6h
c_1f5b2 db 0abh
c_1f5b3 db 3h, 0f9h
c_1f5b5 db 3h, 0ebh
c_1f5b7 db 13h, 0c6h
c_1f5b9 db 0abh
c_1f5ba db 3h, 0f9h
c_1f5bc db 3h, 0ebh
c_1f5be db 13h, 0c6h
c_1f5c0 db 0abh
c_1f5c1 db 3h, 0f9h
c_1f5c3 db 3h, 0ebh
c_1f5c5 db 13h, 0c6h
c_1f5c7 db 0abh
c_1f5c8 db 3h, 0f9h
c_1f5ca db 3h, 0ebh
c_1f5cc db 13h, 0c6h
c_1f5ce db 0abh
c_1f5cf db 3h, 0f9h
c_1f5d1 db 3h, 0ebh
c_1f5d3 db 13h, 0c6h
c_1f5d5 db 0abh
c_1f5d6 db 3h, 0f9h
c_1f5d8 db 3h, 0ebh
c_1f5da db 13h, 0c6h
c_1f5dc db 0abh
c_1f5dd db 3h, 0f9h
c_1f5df db 3h, 0ebh
c_1f5e1 db 13h, 0c6h
c_1f5e3 db 0abh
c_1f5e4 db 3h, 0f9h
c_1f5e6 db 3h, 0ebh
c_1f5e8 db 13h, 0c6h
c_1f5ea db 0abh
c_1f5eb db 3h, 0f9h
c_1f5ed db 3h, 0ebh
c_1f5ef db 13h, 0c6h
c_1f5f1 db 0abh
c_1f5f2 db 3h, 0f9h
c_1f5f4 db 3h, 0ebh
c_1f5f6 db 13h, 0c6h
c_1f5f8 db 0abh
c_1f5f9 db 3h, 0f9h
c_1f5fb db 3h, 0ebh
c_1f5fd db 13h, 0c6h
c_1f5ff db 0abh
c_1f600 db 3h, 0f9h
c_1f602 db 3h, 0ebh
c_1f604 db 13h, 0c6h
c_1f606 db 0abh
c_1f607 db 3h, 0f9h
c_1f609 db 3h, 0ebh
c_1f60b db 13h, 0c6h
c_1f60d db 0abh
c_1f60e db 3h, 0f9h
c_1f610 db 3h, 0ebh
c_1f612 db 13h, 0c6h
c_1f614 db 0abh
c_1f615 db 3h, 0f9h
c_1f617 db 3h, 0ebh
c_1f619 db 13h, 0c6h
c_1f61b db 0abh
c_1f61c db 3h, 0f9h
c_1f61e db 3h, 0ebh
c_1f620 db 13h, 0c6h
c_1f622 db 0abh
c_1f623 db 3h, 0f9h
c_1f625 db 3h, 0ebh
c_1f627 db 13h, 0c6h
c_1f629 db 0abh
c_1f62a db 3h, 0f9h
c_1f62c db 3h, 0ebh
c_1f62e db 13h, 0c6h
c_1f630 db 0abh
c_1f631 db 3h, 0f9h
c_1f633 db 3h, 0ebh
c_1f635 db 13h, 0c6h
c_1f637 db 0abh
c_1f638 db 3h, 0f9h
c_1f63a db 3h, 0ebh
c_1f63c db 13h, 0c6h
c_1f63e db 0abh
c_1f63f db 3h, 0f9h
c_1f641 db 3h, 0ebh
c_1f643 db 13h, 0c6h
c_1f645 db 0abh
c_1f646 db 3h, 0f9h
c_1f648 db 3h, 0ebh
c_1f64a db 13h, 0c6h
c_1f64c db 0abh
c_1f64d db 3h, 0f9h
c_1f64f db 3h, 0ebh
c_1f651 db 13h, 0c6h
c_1f653 db 0abh
c_1f654 db 3h, 0f9h
c_1f656 db 3h, 0ebh
c_1f658 db 13h, 0c6h
c_1f65a db 0abh
c_1f65b db 3h, 0f9h
c_1f65d db 3h, 0ebh
c_1f65f db 13h, 0c6h
c_1f661 db 0abh
c_1f662 db 3h, 0f9h
c_1f664 db 3h, 0ebh
c_1f666 db 13h, 0c6h
c_1f668 db 0abh
c_1f669 db 3h, 0f9h
c_1f66b db 3h, 0ebh
c_1f66d db 13h, 0c6h
c_1f66f db 0abh
c_1f670 db 3h, 0f9h
c_1f672 db 3h, 0ebh
c_1f674 db 13h, 0c6h
c_1f676 db 0abh
c_1f677 db 3h, 0f9h
c_1f679 db 3h, 0ebh
c_1f67b db 13h, 0c6h
c_1f67d db 0abh
c_1f67e db 3h, 0f9h
c_1f680 db 3h, 0ebh
c_1f682 db 13h, 0c6h
c_1f684 db 0abh
c_1f685 db 3h, 0f9h
c_1f687 db 3h, 0ebh
c_1f689 db 13h, 0c6h
c_1f68b db 0abh
c_1f68c db 3h, 0f9h
c_1f68e db 3h, 0ebh
c_1f690 db 13h, 0c6h
c_1f692 db 0abh
c_1f693 db 3h, 0f9h
c_1f695 db 3h, 0ebh
c_1f697 db 13h, 0c6h
c_1f699 db 0abh
c_1f69a db 3h, 0f9h
c_1f69c db 3h, 0ebh
c_1f69e db 13h, 0c6h
c_1f6a0 db 0abh
c_1f6a1 db 3h, 0f9h
c_1f6a3 db 3h, 0ebh
c_1f6a5 db 13h, 0c6h
c_1f6a7 db 0abh
c_1f6a8 db 3h, 0f9h
c_1f6aa db 3h, 0ebh
c_1f6ac db 13h, 0c6h
c_1f6ae db 0abh
c_1f6af db 3h, 0f9h
c_1f6b1 db 3h, 0ebh
c_1f6b3 db 13h, 0c6h
c_1f6b5 db 0abh
c_1f6b6 db 3h, 0f9h
c_1f6b8 db 3h, 0ebh
c_1f6ba db 13h, 0c6h
c_1f6bc db 0abh
c_1f6bd db 3h, 0f9h
c_1f6bf db 3h, 0ebh
c_1f6c1 db 13h, 0c6h
c_1f6c3 db 0abh
c_1f6c4 db 3h, 0f9h
c_1f6c6 db 3h, 0ebh
c_1f6c8 db 13h, 0c6h
c_1f6ca db 0abh
c_1f6cb db 3h, 0f9h
c_1f6cd db 3h, 0ebh
c_1f6cf db 13h, 0c6h
c_1f6d1 db 0abh
c_1f6d2 db 3h, 0f9h
c_1f6d4 db 3h, 0ebh
c_1f6d6 db 13h, 0c6h
c_1f6d8 db 0abh
c_1f6d9 db 3h, 0f9h
c_1f6db db 3h, 0ebh
c_1f6dd db 13h, 0c6h
c_1f6df db 0abh
c_1f6e0 db 3h, 0f9h
c_1f6e2 db 3h, 0ebh
c_1f6e4 db 13h, 0c6h
c_1f6e6 db 0abh
c_1f6e7 db 3h, 0f9h
c_1f6e9 db 3h, 0ebh
c_1f6eb db 13h, 0c6h
c_1f6ed db 0abh
c_1f6ee db 3h, 0f9h
c_1f6f0 db 3h, 0ebh
c_1f6f2 db 13h, 0c6h
c_1f6f4 db 0abh
c_1f6f5 db 3h, 0f9h
c_1f6f7 db 3h, 0ebh
c_1f6f9 db 13h, 0c6h
c_1f6fb db 0abh
c_1f6fc db 3h, 0f9h
c_1f6fe db 3h, 0ebh
c_1f700 db 13h, 0c6h
c_1f702 db 0abh
c_1f703 db 3h, 0f9h
c_1f705 db 3h, 0ebh
c_1f707 db 13h, 0c6h
c_1f709 db 0abh
c_1f70a db 3h, 0f9h
c_1f70c db 3h, 0ebh
c_1f70e db 13h, 0c6h
c_1f710 db 0abh
c_1f711 db 3h, 0f9h
c_1f713 db 3h, 0ebh
c_1f715 db 13h, 0c6h
c_1f717 db 0abh
c_1f718 db 3h, 0f9h
c_1f71a db 3h, 0ebh
c_1f71c db 13h, 0c6h
c_1f71e db 0abh
c_1f71f db 3h, 0f9h
c_1f721 db 3h, 0ebh
c_1f723 db 13h, 0c6h
c_1f725 db 0abh
c_1f726 db 3h, 0f9h
c_1f728 db 3h, 0ebh
c_1f72a db 13h, 0c6h
c_1f72c db 0abh
c_1f72d db 3h, 0f9h
c_1f72f db 3h, 0ebh
c_1f731 db 13h, 0c6h
c_1f733 db 0abh
c_1f734 db 3h, 0f9h
c_1f736 db 3h, 0ebh
c_1f738 db 13h, 0c6h
c_1f73a db 0abh
c_1f73b db 3h, 0f9h
c_1f73d db 3h, 0ebh
c_1f73f db 13h, 0c6h
c_1f741 db 0abh
c_1f742 db 3h, 0f9h
c_1f744 db 3h, 0ebh
c_1f746 db 13h, 0c6h
c_1f748 db 0abh
c_1f749 db 3h, 0f9h
c_1f74b db 3h, 0ebh
c_1f74d db 13h, 0c6h
c_1f74f db 0abh
c_1f750 db 3h, 0f9h
c_1f752 db 3h, 0ebh
c_1f754 db 13h, 0c6h
c_1f756 db 0abh
c_1f757 db 3h, 0f9h
c_1f759 db 3h, 0ebh
c_1f75b db 13h, 0c6h
c_1f75d db 0abh
c_1f75e db 3h, 0f9h
c_1f760 db 3h, 0ebh
c_1f762 db 13h, 0c6h
c_1f764 db 0abh
c_1f765 db 3h, 0f9h
c_1f767 db 3h, 0ebh
c_1f769 db 13h, 0c6h
c_1f76b db 0abh
c_1f76c db 3h, 0f9h
c_1f76e db 3h, 0ebh
c_1f770 db 13h, 0c6h
c_1f772 db 0abh
c_1f773 db 3h, 0f9h
c_1f775 db 3h, 0ebh
c_1f777 db 13h, 0c6h
c_1f779 db 0abh
c_1f77a db 3h, 0f9h
c_1f77c db 3h, 0ebh
c_1f77e db 13h, 0c6h
c_1f780 db 0abh
c_1f781 db 3h, 0f9h
c_1f783 db 3h, 0ebh
c_1f785 db 13h, 0c6h
c_1f787 db 0abh
c_1f788 db 3h, 0f9h
c_1f78a db 3h, 0ebh
c_1f78c db 13h, 0c6h
c_1f78e db 0abh
c_1f78f db 3h, 0f9h
c_1f791 db 3h, 0ebh
c_1f793 db 13h, 0c6h
c_1f795 db 0abh
c_1f796 db 3h, 0f9h
c_1f798 db 3h, 0ebh
c_1f79a db 13h, 0c6h
c_1f79c db 0abh
c_1f79d db 3h, 0f9h
c_1f79f db 3h, 0ebh
c_1f7a1 db 13h, 0c6h
c_1f7a3 db 0abh
c_1f7a4 db 3h, 0f9h
c_1f7a6 db 3h, 0ebh
c_1f7a8 db 13h, 0c6h
c_1f7aa db 0abh
c_1f7ab db 3h, 0f9h
c_1f7ad db 3h, 0ebh
c_1f7af db 13h, 0c6h
c_1f7b1 db 0abh
c_1f7b2 db 3h, 0f9h
c_1f7b4 db 3h, 0ebh
c_1f7b6 db 13h, 0c6h
c_1f7b8 db 0abh
c_1f7b9 db 3h, 0f9h
c_1f7bb db 3h, 0ebh
c_1f7bd db 13h, 0c6h
c_1f7bf db 0abh
c_1f7c0 db 3h, 0f9h
c_1f7c2 db 3h, 0ebh
c_1f7c4 db 13h, 0c6h
c_1f7c6 db 0abh
c_1f7c7 db 3h, 0f9h
c_1f7c9 db 3h, 0ebh
c_1f7cb db 13h, 0c6h
c_1f7cd db 0abh
c_1f7ce db 3h, 0f9h
c_1f7d0 db 3h, 0ebh
c_1f7d2 db 13h, 0c6h
c_1f7d4 db 0abh
c_1f7d5 db 3h, 0f9h
c_1f7d7 db 3h, 0ebh
c_1f7d9 db 13h, 0c6h
c_1f7db db 0abh
c_1f7dc db 3h, 0f9h
c_1f7de db 3h, 0ebh
c_1f7e0 db 13h, 0c6h
c_1f7e2 db 0abh
c_1f7e3 db 3h, 0f9h
c_1f7e5 db 3h, 0ebh
c_1f7e7 db 13h, 0c6h
c_1f7e9 db 0abh
c_1f7ea db 3h, 0f9h
c_1f7ec db 3h, 0ebh
c_1f7ee db 13h, 0c6h
c_1f7f0 db 0abh
c_1f7f1 db 3h, 0f9h
c_1f7f3 db 3h, 0ebh
c_1f7f5 db 13h, 0c6h
c_1f7f7 db 0abh
c_1f7f8 db 3h, 0f9h
c_1f7fa db 3h, 0ebh
c_1f7fc db 13h, 0c6h
c_1f7fe db 0abh
c_1f7ff db 3h, 0f9h
c_1f801 db 3h, 0ebh
c_1f803 db 13h, 0c6h
c_1f805 db 0abh
c_1f806 db 3h, 0f9h
c_1f808 db 3h, 0ebh
c_1f80a db 13h, 0c6h
c_1f80c db 0abh
c_1f80d db 3h, 0f9h
c_1f80f db 3h, 0ebh
c_1f811 db 13h, 0c6h
c_1f813 db 0abh
c_1f814 db 3h, 0f9h
c_1f816 db 3h, 0ebh
c_1f818 db 13h, 0c6h
c_1f81a db 0abh
c_1f81b db 3h, 0f9h
c_1f81d db 3h, 0ebh
c_1f81f db 13h, 0c6h
c_1f821 db 0abh
c_1f822 db 3h, 0f9h
c_1f824 db 3h, 0ebh
c_1f826 db 13h, 0c6h
c_1f828 db 0abh
c_1f829 db 3h, 0f9h
c_1f82b db 3h, 0ebh
c_1f82d db 13h, 0c6h
c_1f82f db 0abh
c_1f830 db 3h, 0f9h
c_1f832 db 3h, 0ebh
c_1f834 db 13h, 0c6h
c_1f836 db 0abh
c_1f837 db 3h, 0f9h
c_1f839 db 3h, 0ebh
c_1f83b db 13h, 0c6h
c_1f83d db 0abh
c_1f83e db 3h, 0f9h
c_1f840 db 3h, 0ebh
c_1f842 db 13h, 0c6h
c_1f844 db 0abh
c_1f845 db 3h, 0f9h
c_1f847 db 3h, 0ebh
c_1f849 db 13h, 0c6h
c_1f84b db 0abh
c_1f84c db 3h, 0f9h
c_1f84e db 3h, 0ebh
c_1f850 db 13h, 0c6h
c_1f852 db 0abh
c_1f853 db 3h, 0f9h
c_1f855 db 3h, 0ebh
c_1f857 db 13h, 0c6h
c_1f859 db 0abh
c_1f85a db 3h, 0f9h
c_1f85c db 3h, 0ebh
c_1f85e db 13h, 0c6h
c_1f860 db 0abh
c_1f861 db 3h, 0f9h
c_1f863 db 3h, 0ebh
c_1f865 db 13h, 0c6h
c_1f867 db 0abh
c_1f868 db 3h, 0f9h
c_1f86a db 3h, 0ebh
c_1f86c db 13h, 0c6h
c_1f86e db 0abh
c_1f86f db 3h, 0f9h
c_1f871 db 3h, 0ebh
c_1f873 db 13h, 0c6h
c_1f875 db 0abh
c_1f876 db 3h, 0f9h
c_1f878 db 3h, 0ebh
c_1f87a db 13h, 0c6h
c_1f87c db 0abh
c_1f87d db 3h, 0f9h
c_1f87f db 3h, 0ebh
c_1f881 db 13h, 0c6h
c_1f883 db 0abh
c_1f884 db 3h, 0f9h
c_1f886 db 3h, 0ebh
c_1f888 db 13h, 0c6h
c_1f88a db 0abh
c_1f88b db 3h, 0f9h
c_1f88d db 3h, 0ebh
c_1f88f db 13h, 0c6h
c_1f891 db 0abh
c_1f892 db 3h, 0f9h
c_1f894 db 3h, 0ebh
c_1f896 db 13h, 0c6h
c_1f898 db 0abh
c_1f899 db 3h, 0f9h
c_1f89b db 3h, 0ebh
c_1f89d db 13h, 0c6h
c_1f89f db 0abh
c_1f8a0 db 3h, 0f9h
c_1f8a2 db 3h, 0ebh
c_1f8a4 db 13h, 0c6h
c_1f8a6 db 0abh
c_1f8a7 db 3h, 0f9h
c_1f8a9 db 3h, 0ebh
c_1f8ab db 13h, 0c6h
c_1f8ad db 0abh
c_1f8ae db 3h, 0f9h
c_1f8b0 db 3h, 0ebh
c_1f8b2 db 13h, 0c6h
c_1f8b4 db 0abh
c_1f8b5 db 3h, 0f9h
c_1f8b7 db 3h, 0ebh
c_1f8b9 db 13h, 0c6h
c_1f8bb db 0abh
c_1f8bc db 3h, 0f9h
c_1f8be db 3h, 0ebh
c_1f8c0 db 13h, 0c6h
c_1f8c2 db 0abh
c_1f8c3 db 3h, 0f9h
c_1f8c5 db 3h, 0ebh
c_1f8c7 db 13h, 0c6h
c_1f8c9 db 0abh
c_1f8ca db 3h, 0f9h
c_1f8cc db 3h, 0ebh
c_1f8ce db 13h, 0c6h
c_1f8d0 db 0abh
c_1f8d1 db 3h, 0f9h
c_1f8d3 db 3h, 0ebh
c_1f8d5 db 13h, 0c6h
c_1f8d7 db 0abh
c_1f8d8 db 3h, 0f9h
c_1f8da db 3h, 0ebh
c_1f8dc db 13h, 0c6h
c_1f8de db 0abh
c_1f8df db 3h, 0f9h
c_1f8e1 db 3h, 0ebh
c_1f8e3 db 13h, 0c6h
c_1f8e5 db 0abh
c_1f8e6 db 3h, 0f9h
c_1f8e8 db 3h, 0ebh
c_1f8ea db 13h, 0c6h
c_1f8ec db 0abh
c_1f8ed db 3h, 0f9h
c_1f8ef db 3h, 0ebh
c_1f8f1 db 13h, 0c6h
c_1f8f3 db 0abh
c_1f8f4 db 3h, 0f9h
c_1f8f6 db 3h, 0ebh
c_1f8f8 db 13h, 0c6h
c_1f8fa db 0abh
c_1f8fb db 3h, 0f9h
c_1f8fd db 3h, 0ebh
c_1f8ff db 13h, 0c6h
c_1f901 db 0abh
c_1f902 db 3h, 0f9h
c_1f904 db 3h, 0ebh
c_1f906 db 13h, 0c6h
c_1f908 db 0abh
c_1f909 db 3h, 0f9h
c_1f90b db 3h, 0ebh
c_1f90d db 13h, 0c6h
c_1f90f db 0abh
c_1f910 db 3h, 0f9h
c_1f912 db 3h, 0ebh
c_1f914 db 13h, 0c6h
c_1f916 db 0abh
c_1f917 db 3h, 0f9h
c_1f919 db 3h, 0ebh
c_1f91b db 13h, 0c6h
c_1f91d db 0abh
c_1f91e db 3h, 0f9h
c_1f920 db 3h, 0ebh
c_1f922 db 13h, 0c6h
c_1f924 db 0abh
c_1f925 db 3h, 0f9h
c_1f927 db 3h, 0ebh
c_1f929 db 13h, 0c6h
c_1f92b db 0abh
c_1f92c db 3h, 0f9h
c_1f92e db 3h, 0ebh
c_1f930 db 13h, 0c6h
c_1f932 db 0abh
c_1f933 db 3h, 0f9h
c_1f935 db 3h, 0ebh
c_1f937 db 13h, 0c6h
c_1f939 db 0abh
c_1f93a db 3h, 0f9h
c_1f93c db 3h, 0ebh
c_1f93e db 13h, 0c6h
c_1f940 db 0abh
c_1f941 db 3h, 0f9h
c_1f943 db 3h, 0ebh
c_1f945 db 13h, 0c6h
c_1f947 db 0abh
c_1f948 db 3h, 0f9h
c_1f94a db 3h, 0ebh
c_1f94c db 13h, 0c6h
c_1f94e db 0abh
c_1f94f db 3h, 0f9h
c_1f951 db 3h, 0ebh
c_1f953 db 13h, 0c6h
c_1f955 db 0abh
c_1f956 db 3h, 0f9h
c_1f958 db 3h, 0ebh
c_1f95a db 13h, 0c6h
c_1f95c db 0abh
c_1f95d db 3h, 0f9h
c_1f95f db 3h, 0ebh
c_1f961 db 13h, 0c6h
c_1f963 db 0abh
c_1f964 db 3h, 0f9h
c_1f966 db 3h, 0ebh
c_1f968 db 13h, 0c6h
c_1f96a db 0abh
c_1f96b db 3h, 0f9h
c_1f96d db 3h, 0ebh
c_1f96f db 13h, 0c6h
c_1f971 db 0abh
c_1f972 db 3h, 0f9h
c_1f974 db 3h, 0ebh
c_1f976 db 13h, 0c6h
c_1f978 db 0abh
c_1f979 db 3h, 0f9h
c_1f97b db 3h, 0ebh
c_1f97d db 13h, 0c6h
c_1f97f db 0abh
c_1f980 db 3h, 0f9h
c_1f982 db 3h, 0ebh
c_1f984 db 13h, 0c6h
c_1f986 db 0abh
c_1f987 db 3h, 0f9h
c_1f989 db 3h, 0ebh
c_1f98b db 13h, 0c6h
c_1f98d db 0abh
c_1f98e db 3h, 0f9h
c_1f990 db 3h, 0ebh
c_1f992 db 13h, 0c6h
c_1f994 db 0abh
c_1f995 db 3h, 0f9h
c_1f997 db 3h, 0ebh
c_1f999 db 13h, 0c6h
c_1f99b db 0abh
c_1f99c db 3h, 0f9h
c_1f99e db 3h, 0ebh
c_1f9a0 db 13h, 0c6h
c_1f9a2 db 0abh
c_1f9a3 db 3h, 0f9h
c_1f9a5 db 3h, 0ebh
c_1f9a7 db 13h, 0c6h
c_1f9a9 db 0abh
c_1f9aa db 3h, 0f9h
c_1f9ac db 3h, 0ebh
c_1f9ae db 13h, 0c6h
c_1f9b0 db 0abh
c_1f9b1 db 3h, 0f9h
c_1f9b3 db 3h, 0ebh
c_1f9b5 db 13h, 0c6h
c_1f9b7 db 0abh
c_1f9b8 db 3h, 0f9h
c_1f9ba db 3h, 0ebh
c_1f9bc db 13h, 0c6h
c_1f9be db 0abh
c_1f9bf db 3h, 0f9h
c_1f9c1 db 3h, 0ebh
c_1f9c3 db 13h, 0c6h
c_1f9c5 db 0abh
c_1f9c6 db 3h, 0f9h
c_1f9c8 db 3h, 0ebh
c_1f9ca db 13h, 0c6h
c_1f9cc db 0abh
c_1f9cd db 3h, 0f9h
c_1f9cf db 3h, 0ebh
c_1f9d1 db 13h, 0c6h
c_1f9d3 db 0abh
c_1f9d4 db 3h, 0f9h
c_1f9d6 db 3h, 0ebh
c_1f9d8 db 13h, 0c6h
c_1f9da db 0abh
c_1f9db db 3h, 0f9h
c_1f9dd db 3h, 0ebh
c_1f9df db 13h, 0c6h
c_1f9e1 db 0abh
c_1f9e2 db 3h, 0f9h
c_1f9e4 db 3h, 0ebh
c_1f9e6 db 13h, 0c6h
c_1f9e8 db 0abh
c_1f9e9 db 3h, 0f9h
c_1f9eb db 3h, 0ebh
c_1f9ed db 13h, 0c6h
c_1f9ef db 0abh
c_1f9f0 db 3h, 0f9h
c_1f9f2 db 3h, 0ebh
c_1f9f4 db 13h, 0c6h
c_1f9f6 db 0abh
c_1f9f7 db 3h, 0f9h
c_1f9f9 db 3h, 0ebh
c_1f9fb db 13h, 0c6h
c_1f9fd db 0abh
c_1f9fe db 3h, 0f9h
c_1fa00 db 3h, 0ebh
c_1fa02 db 13h, 0c6h
c_1fa04 db 0abh
c_1fa05 db 3h, 0f9h
c_1fa07 db 3h, 0ebh
c_1fa09 db 13h, 0c6h
c_1fa0b db 0abh
c_1fa0c db 3h, 0f9h
c_1fa0e db 3h, 0ebh
c_1fa10 db 13h, 0c6h
c_1fa12 db 0abh
c_1fa13 db 3h, 0f9h
c_1fa15 db 3h, 0ebh
c_1fa17 db 13h, 0c6h
c_1fa19 db 0abh
c_1fa1a db 3h, 0f9h
c_1fa1c db 3h, 0ebh
c_1fa1e db 13h, 0c6h
c_1fa20 db 0abh
c_1fa21 db 3h, 0f9h
c_1fa23 db 3h, 0ebh
c_1fa25 db 13h, 0c6h
c_1fa27 db 0abh
c_1fa28 db 3h, 0f9h
c_1fa2a db 3h, 0ebh
c_1fa2c db 13h, 0c6h
c_1fa2e db 0abh
c_1fa2f db 3h, 0f9h
c_1fa31 db 3h, 0ebh
c_1fa33 db 13h, 0c6h
c_1fa35 db 0abh
c_1fa36 db 3h, 0f9h
c_1fa38 db 3h, 0ebh
c_1fa3a db 13h, 0c6h
c_1fa3c db 0abh
c_1fa3d db 3h, 0f9h
c_1fa3f db 3h, 0ebh
c_1fa41 db 13h, 0c6h
c_1fa43 db 0abh
c_1fa44 db 3h, 0f9h
c_1fa46 db 3h, 0ebh
c_1fa48 db 13h, 0c6h
c_1fa4a db 0abh
c_1fa4b db 3h, 0f9h
c_1fa4d db 3h, 0ebh
c_1fa4f db 13h, 0c6h
c_1fa51 db 0abh
c_1fa52 db 3h, 0f9h
c_1fa54 db 3h, 0ebh
c_1fa56 db 13h, 0c6h
c_1fa58 db 0abh
c_1fa59 db 3h, 0f9h
c_1fa5b db 3h, 0ebh
c_1fa5d db 13h, 0c6h
c_1fa5f db 0abh
c_1fa60 db 3h, 0f9h
c_1fa62 db 3h, 0ebh
c_1fa64 db 13h, 0c6h
c_1fa66 db 0abh
c_1fa67 db 3h, 0f9h
c_1fa69 db 3h, 0ebh
c_1fa6b db 13h, 0c6h
c_1fa6d db 0abh
c_1fa6e db 3h, 0f9h
c_1fa70 db 3h, 0ebh
c_1fa72 db 13h, 0c6h
c_1fa74 db 0abh
c_1fa75 db 3h, 0f9h
c_1fa77 db 3h, 0ebh
c_1fa79 db 13h, 0c6h
c_1fa7b db 0abh
c_1fa7c db 3h, 0f9h
c_1fa7e db 3h, 0ebh
c_1fa80 db 13h, 0c6h
c_1fa82 db 0abh
c_1fa83 db 3h, 0f9h
c_1fa85 db 3h, 0ebh
c_1fa87 db 13h, 0c6h
c_1fa89 db 0abh
c_1fa8a db 3h, 0f9h
c_1fa8c db 3h, 0ebh
c_1fa8e db 13h, 0c6h
c_1fa90 db 0abh
c_1fa91 db 3h, 0f9h
c_1fa93 db 3h, 0ebh
c_1fa95 db 13h, 0c6h
c_1fa97 db 0abh
c_1fa98 db 3h, 0f9h
c_1fa9a db 3h, 0ebh
c_1fa9c db 13h, 0c6h
c_1fa9e db 0abh
c_1fa9f db 3h, 0f9h
c_1faa1 db 3h, 0ebh
c_1faa3 db 13h, 0c6h
c_1faa5 db 0abh
c_1faa6 db 3h, 0f9h
c_1faa8 db 3h, 0ebh
c_1faaa db 13h, 0c6h
c_1faac db 0abh
c_1faad db 3h, 0f9h
c_1faaf db 3h, 0ebh
c_1fab1 db 13h, 0c6h
c_1fab3 db 0abh
c_1fab4 db 3h, 0f9h
c_1fab6 db 3h, 0ebh
c_1fab8 db 13h, 0c6h
c_1faba db 0abh
c_1fabb db 3h, 0f9h
c_1fabd db 3h, 0ebh
c_1fabf db 13h, 0c6h
c_1fac1 db 0abh
c_1fac2 db 3h, 0f9h
c_1fac4 db 3h, 0ebh
c_1fac6 db 13h, 0c6h
c_1fac8 db 0abh
c_1fac9 db 3h, 0f9h
c_1facb db 3h, 0ebh
c_1facd db 13h, 0c6h
c_1facf db 0abh
c_1fad0 db 3h, 0f9h
c_1fad2 db 3h, 0ebh
c_1fad4 db 13h, 0c6h
c_1fad6 db 0abh
c_1fad7 db 3h, 0f9h
c_1fad9 db 3h, 0ebh
c_1fadb db 13h, 0c6h
c_1fadd db 0abh
c_1fade db 3h, 0f9h
c_1fae0 db 3h, 0ebh
c_1fae2 db 13h, 0c6h
c_1fae4 db 0abh
c_1fae5 db 3h, 0f9h
c_1fae7 db 3h, 0ebh
c_1fae9 db 13h, 0c6h
c_1faeb db 0abh
c_1faec db 3h, 0f9h
c_1faee db 3h, 0ebh
c_1faf0 db 13h, 0c6h
c_1faf2 db 0abh
c_1faf3 db 3h, 0f9h
c_1faf5 db 3h, 0ebh
c_1faf7 db 13h, 0c6h
c_1faf9 db 0abh
c_1fafa db 3h, 0f9h
c_1fafc db 3h, 0ebh
c_1fafe db 13h, 0c6h
c_1fb00 db 0abh
c_1fb01 db 3h, 0f9h
c_1fb03 db 3h, 0ebh
c_1fb05 db 13h, 0c6h
c_1fb07 db 0abh
c_1fb08 db 3h, 0f9h
c_1fb0a db 3h, 0ebh
c_1fb0c db 13h, 0c6h
c_1fb0e db 0abh
c_1fb0f db 3h, 0f9h
c_1fb11 db 3h, 0ebh
c_1fb13 db 13h, 0c6h
c_1fb15 db 0abh
c_1fb16 db 3h, 0f9h
c_1fb18 db 3h, 0ebh
c_1fb1a db 13h, 0c6h
c_1fb1c db 0abh
c_1fb1d db 3h, 0f9h
c_1fb1f db 3h, 0ebh
c_1fb21 db 13h, 0c6h
c_1fb23 db 0abh
c_1fb24 db 3h, 0f9h
c_1fb26 db 3h, 0ebh
c_1fb28 db 13h, 0c6h
c_1fb2a db 0abh
c_1fb2b db 3h, 0f9h
c_1fb2d db 3h, 0ebh
c_1fb2f db 13h, 0c6h
c_1fb31 db 0abh
c_1fb32 db 3h, 0f9h
c_1fb34 db 3h, 0ebh
c_1fb36 db 13h, 0c6h
c_1fb38 db 0abh
c_1fb39 db 3h, 0f9h
c_1fb3b db 3h, 0ebh
c_1fb3d db 13h, 0c6h
c_1fb3f db 0abh
c_1fb40 db 3h, 0f9h
c_1fb42 db 3h, 0ebh
c_1fb44 db 13h, 0c6h
c_1fb46 db 0abh
c_1fb47 db 3h, 0f9h
c_1fb49 db 3h, 0ebh
c_1fb4b db 13h, 0c6h
c_1fb4d db 0abh
c_1fb4e db 3h, 0f9h
c_1fb50 db 3h, 0ebh
c_1fb52 db 13h, 0c6h
c_1fb54 db 0abh
c_1fb55 db 3h, 0f9h
c_1fb57 db 3h, 0ebh
c_1fb59 db 13h, 0c6h
c_1fb5b db 0abh
c_1fb5c db 3h, 0f9h
c_1fb5e db 3h, 0ebh
c_1fb60 db 13h, 0c6h
c_1fb62 db 0abh
c_1fb63 db 3h, 0f9h
c_1fb65 db 3h, 0ebh
c_1fb67 db 13h, 0c6h
c_1fb69 db 0abh
c_1fb6a db 3h, 0f9h
c_1fb6c db 3h, 0ebh
c_1fb6e db 13h, 0c6h
c_1fb70 db 0abh
c_1fb71 db 3h, 0f9h
c_1fb73 db 3h, 0ebh
c_1fb75 db 13h, 0c6h
c_1fb77 db 0abh
c_1fb78 db 3h, 0f9h
c_1fb7a db 3h, 0ebh
c_1fb7c db 13h, 0c6h
c_1fb7e db 0abh
c_1fb7f db 3h, 0f9h
c_1fb81 db 3h, 0ebh
c_1fb83 db 13h, 0c6h
c_1fb85 db 0abh
c_1fb86 db 3h, 0f9h
c_1fb88 db 3h, 0ebh
c_1fb8a db 13h, 0c6h
c_1fb8c db 0abh
c_1fb8d db 3h, 0f9h
c_1fb8f db 3h, 0ebh
c_1fb91 db 13h, 0c6h
c_1fb93 db 0abh
c_1fb94 db 3h, 0f9h
c_1fb96 db 3h, 0ebh
c_1fb98 db 13h, 0c6h
c_1fb9a db 0abh
c_1fb9b db 3h, 0f9h
c_1fb9d db 3h, 0ebh
c_1fb9f db 13h, 0c6h
c_1fba1 db 0abh
c_1fba2 db 3h, 0f9h
c_1fba4 db 3h, 0ebh
c_1fba6 db 13h, 0c6h
c_1fba8 db 0abh
c_1fba9 db 3h, 0f9h
c_1fbab db 3h, 0ebh
c_1fbad db 13h, 0c6h
c_1fbaf db 0abh
c_1fbb0 db 3h, 0f9h
c_1fbb2 db 3h, 0ebh
c_1fbb4 db 13h, 0c6h
c_1fbb6 db 0abh
c_1fbb7 db 3h, 0f9h
c_1fbb9 db 3h, 0ebh
c_1fbbb db 13h, 0c6h
c_1fbbd db 0abh
c_1fbbe db 3h, 0f9h
c_1fbc0 db 3h, 0ebh
c_1fbc2 db 13h, 0c6h
c_1fbc4 db 0abh
c_1fbc5 db 3h, 0f9h
c_1fbc7 db 3h, 0ebh
c_1fbc9 db 13h, 0c6h
c_1fbcb db 0abh
c_1fbcc db 3h, 0f9h
c_1fbce db 3h, 0ebh
c_1fbd0 db 13h, 0c6h
c_1fbd2 db 0abh
c_1fbd3 db 3h, 0f9h
c_1fbd5 db 3h, 0ebh
c_1fbd7 db 13h, 0c6h
c_1fbd9 db 0abh
c_1fbda db 3h, 0f9h
c_1fbdc db 3h, 0ebh
c_1fbde db 13h, 0c6h
c_1fbe0 db 0abh
c_1fbe1 db 3h, 0f9h
c_1fbe3 db 3h, 0ebh
c_1fbe5 db 13h, 0c6h
c_1fbe7 db 0abh
c_1fbe8 db 3h, 0f9h
c_1fbea db 3h, 0ebh
c_1fbec db 13h, 0c6h
c_1fbee db 0abh
c_1fbef db 3h, 0f9h
c_1fbf1 db 3h, 0ebh
c_1fbf3 db 13h, 0c6h
c_1fbf5 db 0abh
c_1fbf6 db 3h, 0f9h
c_1fbf8 db 3h, 0ebh
c_1fbfa db 13h, 0c6h
c_1fbfc db 0abh
c_1fbfd db 3h, 0f9h
c_1fbff db 3h, 0ebh
c_1fc01 db 13h, 0c6h
c_1fc03 db 0abh
c_1fc04 db 3h, 0f9h
c_1fc06 db 3h, 0ebh
c_1fc08 db 13h, 0c6h
c_1fc0a db 0abh
c_1fc0b db 3h, 0f9h
c_1fc0d db 3h, 0ebh
c_1fc0f db 13h, 0c6h
c_1fc11 db 0abh
c_1fc12 db 3h, 0f9h
c_1fc14 db 3h, 0ebh
c_1fc16 db 13h, 0c6h
c_1fc18 db 0abh
c_1fc19 db 3h, 0f9h
c_1fc1b db 3h, 0ebh
c_1fc1d db 13h, 0c6h
c_1fc1f db 0abh
c_1fc20 db 3h, 0f9h
c_1fc22 db 3h, 0ebh
c_1fc24 db 13h, 0c6h
c_1fc26 db 0abh
c_1fc27 db 3h, 0f9h
c_1fc29 db 3h, 0ebh
c_1fc2b db 13h, 0c6h
c_1fc2d db 0abh
c_1fc2e db 3h, 0f9h
c_1fc30 db 3h, 0ebh
c_1fc32 db 13h, 0c6h
c_1fc34 db 0abh
c_1fc35 db 3h, 0f9h
c_1fc37 db 3h, 0ebh
c_1fc39 db 13h, 0c6h
c_1fc3b db 0abh
c_1fc3c db 3h, 0f9h
c_1fc3e db 3h, 0ebh
c_1fc40 db 13h, 0c6h
c_1fc42 db 0abh
c_1fc43 db 3h, 0f9h
c_1fc45 db 3h, 0ebh
c_1fc47 db 13h, 0c6h
c_1fc49 db 0abh
c_1fc4a db 3h, 0f9h
c_1fc4c db 3h, 0ebh
c_1fc4e db 13h, 0c6h
c_1fc50 db 0abh
c_1fc51 db 3h, 0f9h
c_1fc53 db 3h, 0ebh
c_1fc55 db 13h, 0c6h
c_1fc57 db 0abh
c_1fc58 db 3h, 0f9h
c_1fc5a db 3h, 0ebh
c_1fc5c db 13h, 0c6h
c_1fc5e db 0abh
c_1fc5f db 3h, 0f9h
c_1fc61 db 3h, 0ebh
c_1fc63 db 13h, 0c6h
c_1fc65 db 0abh
c_1fc66 db 3h, 0f9h
c_1fc68 db 3h, 0ebh
c_1fc6a db 13h, 0c6h
c_1fc6c db 0abh
c_1fc6d db 3h, 0f9h
c_1fc6f db 3h, 0ebh
c_1fc71 db 13h, 0c6h
c_1fc73 db 0abh
c_1fc74 db 3h, 0f9h
c_1fc76 db 3h, 0ebh
c_1fc78 db 13h, 0c6h
c_1fc7a db 0abh
c_1fc7b db 3h, 0f9h
c_1fc7d db 3h, 0ebh
c_1fc7f db 13h, 0c6h
c_1fc81 db 0abh
c_1fc82 db 3h, 0f9h
c_1fc84 db 3h, 0ebh
c_1fc86 db 13h, 0c6h
c_1fc88 db 0abh
c_1fc89 db 3h, 0f9h
c_1fc8b db 3h, 0ebh
c_1fc8d db 13h, 0c6h
c_1fc8f db 0abh
c_1fc90 db 3h, 0f9h
c_1fc92 db 3h, 0ebh
c_1fc94 db 13h, 0c6h
c_1fc96 db 0abh
c_1fc97 db 3h, 0f9h
c_1fc99 db 3h, 0ebh
c_1fc9b db 13h, 0c6h
c_1fc9d db 0abh
c_1fc9e db 3h, 0f9h
c_1fca0 db 3h, 0ebh
c_1fca2 db 13h, 0c6h
c_1fca4 db 0abh
c_1fca5 db 3h, 0f9h
c_1fca7 db 3h, 0ebh
c_1fca9 db 13h, 0c6h
c_1fcab db 0abh
c_1fcac db 3h, 0f9h
c_1fcae db 3h, 0ebh
c_1fcb0 db 13h, 0c6h
c_1fcb2 db 0abh
c_1fcb3 db 3h, 0f9h
c_1fcb5 db 3h, 0ebh
c_1fcb7 db 13h, 0c6h
c_1fcb9 db 0abh
c_1fcba db 3h, 0f9h
c_1fcbc db 3h, 0ebh
c_1fcbe db 13h, 0c6h
c_1fcc0 db 0abh
c_1fcc1 db 3h, 0f9h
c_1fcc3 db 3h, 0ebh
c_1fcc5 db 13h, 0c6h
c_1fcc7 db 0abh
c_1fcc8 db 3h, 0f9h
c_1fcca db 3h, 0ebh
c_1fccc db 13h, 0c6h
c_1fcce db 0abh
c_1fccf db 3h, 0f9h
c_1fcd1 db 3h, 0ebh
c_1fcd3 db 13h, 0c6h
c_1fcd5 db 0abh
c_1fcd6 db 3h, 0f9h
c_1fcd8 db 3h, 0ebh
c_1fcda db 13h, 0c6h
c_1fcdc db 0abh
c_1fcdd db 3h, 0f9h
c_1fcdf db 3h, 0ebh
c_1fce1 db 13h, 0c6h
c_1fce3 db 0abh
c_1fce4 db 3h, 0f9h
c_1fce6 db 3h, 0ebh
c_1fce8 db 13h, 0c6h
c_1fcea db 0abh
c_1fceb db 3h, 0f9h
c_1fced db 3h, 0ebh
c_1fcef db 13h, 0c6h
c_1fcf1 db 0abh
c_1fcf2 db 3h, 0f9h
c_1fcf4 db 3h, 0ebh
c_1fcf6 db 13h, 0c6h
c_1fcf8 db 0abh
c_1fcf9 db 3h, 0f9h
c_1fcfb db 3h, 0ebh
c_1fcfd db 13h, 0c6h
c_1fcff db 0abh
c_1fd00 db 3h, 0f9h
c_1fd02 db 3h, 0ebh
c_1fd04 db 13h, 0c6h
c_1fd06 db 0abh
c_1fd07 db 3h, 0f9h
c_1fd09 db 3h, 0ebh
c_1fd0b db 13h, 0c6h
c_1fd0d db 0abh
c_1fd0e db 3h, 0f9h
c_1fd10 db 3h, 0ebh
c_1fd12 db 13h, 0c6h
c_1fd14 db 0abh
c_1fd15 db 3h, 0f9h
c_1fd17 db 3h, 0ebh
c_1fd19 db 13h, 0c6h
c_1fd1b db 0abh
c_1fd1c db 3h, 0f9h
c_1fd1e db 3h, 0ebh
c_1fd20 db 13h, 0c6h
c_1fd22 db 0abh
c_1fd23 db 3h, 0f9h
c_1fd25 db 3h, 0ebh
c_1fd27 db 13h, 0c6h
c_1fd29 db 0abh
c_1fd2a db 3h, 0f9h
c_1fd2c db 3h, 0ebh
c_1fd2e db 13h, 0c6h
c_1fd30 db 0abh
c_1fd31 db 3h, 0f9h
c_1fd33 db 3h, 0ebh
c_1fd35 db 13h, 0c6h
c_1fd37 db 0abh
c_1fd38 db 3h, 0f9h
c_1fd3a db 3h, 0ebh
c_1fd3c db 13h, 0c6h
c_1fd3e db 0abh
c_1fd3f db 3h, 0f9h
c_1fd41 db 3h, 0ebh
c_1fd43 db 13h, 0c6h
c_1fd45 db 0abh
c_1fd46 db 3h, 0f9h
c_1fd48 db 3h, 0ebh
c_1fd4a db 13h, 0c6h
c_1fd4c db 0abh
c_1fd4d db 3h, 0f9h
c_1fd4f db 3h, 0ebh
c_1fd51 db 13h, 0c6h
c_1fd53 db 0abh
c_1fd54 db 3h, 0f9h
c_1fd56 db 3h, 0ebh
c_1fd58 db 13h, 0c6h
c_1fd5a db 0abh
c_1fd5b db 3h, 0f9h
c_1fd5d db 3h, 0ebh
c_1fd5f db 13h, 0c6h
c_1fd61 db 0abh
c_1fd62 db 3h, 0f9h
c_1fd64 db 3h, 0ebh
c_1fd66 db 13h, 0c6h
c_1fd68 db 0abh
c_1fd69 db 3h, 0f9h
c_1fd6b db 3h, 0ebh
c_1fd6d db 13h, 0c6h
c_1fd6f db 0abh
c_1fd70 db 3h, 0f9h
c_1fd72 db 3h, 0ebh
c_1fd74 db 13h, 0c6h
c_1fd76 db 0abh
c_1fd77 db 3h, 0f9h
c_1fd79 db 3h, 0ebh
c_1fd7b db 13h, 0c6h
c_1fd7d db 0abh
c_1fd7e db 3h, 0f9h
c_1fd80 db 3h, 0ebh
c_1fd82 db 13h, 0c6h
c_1fd84 db 0abh
c_1fd85 db 3h, 0f9h
c_1fd87 db 3h, 0ebh
c_1fd89 db 13h, 0c6h
c_1fd8b db 0abh
c_1fd8c db 3h, 0f9h
c_1fd8e db 3h, 0ebh
c_1fd90 db 13h, 0c6h
c_1fd92 db 0abh
c_1fd93 db 3h, 0f9h
c_1fd95 db 3h, 0ebh
c_1fd97 db 13h, 0c6h
c_1fd99 db 0abh
c_1fd9a db 3h, 0f9h
c_1fd9c db 3h, 0ebh
c_1fd9e db 13h, 0c6h
c_1fda0 db 0abh
c_1fda1 db 3h, 0f9h
c_1fda3 db 3h, 0ebh
c_1fda5 db 13h, 0c6h
c_1fda7 db 0abh
c_1fda8 db 3h, 0f9h
c_1fdaa db 3h, 0ebh
c_1fdac db 13h, 0c6h
c_1fdae db 0abh
c_1fdaf db 3h, 0f9h
c_1fdb1 db 3h, 0ebh
c_1fdb3 db 13h, 0c6h
c_1fdb5 db 0abh
c_1fdb6 db 3h, 0f9h
c_1fdb8 db 3h, 0ebh
c_1fdba db 13h, 0c6h
c_1fdbc db 0abh
c_1fdbd db 3h, 0f9h
c_1fdbf db 3h, 0ebh
c_1fdc1 db 13h, 0c6h
c_1fdc3 db 0abh
c_1fdc4 db 3h, 0f9h
c_1fdc6 db 3h, 0ebh
c_1fdc8 db 13h, 0c6h
c_1fdca db 0abh
c_1fdcb db 3h, 0f9h
c_1fdcd db 3h, 0ebh
c_1fdcf db 13h, 0c6h
c_1fdd1 db 0abh
c_1fdd2 db 3h, 0f9h
c_1fdd4 db 3h, 0ebh
c_1fdd6 db 13h, 0c6h
c_1fdd8 db 0abh
c_1fdd9 db 3h, 0f9h
c_1fddb db 3h, 0ebh
c_1fddd db 13h, 0c6h
c_1fddf db 0abh
c_1fde0 db 3h, 0f9h
c_1fde2 db 3h, 0ebh
c_1fde4 db 13h, 0c6h
c_1fde6 db 0abh
c_1fde7 db 3h, 0f9h
c_1fde9 db 3h, 0ebh
c_1fdeb db 13h, 0c6h
c_1fded db 0abh
c_1fdee db 3h, 0f9h
c_1fdf0 db 3h, 0ebh
c_1fdf2 db 13h, 0c6h
c_1fdf4 db 0abh
c_1fdf5 db 3h, 0f9h
c_1fdf7 db 3h, 0ebh
c_1fdf9 db 13h, 0c6h
c_1fdfb db 0abh
c_1fdfc db 3h, 0f9h
c_1fdfe db 3h, 0ebh
c_1fe00 db 13h, 0c6h
c_1fe02 db 0abh
c_1fe03 db 3h, 0f9h
c_1fe05 db 3h, 0ebh
c_1fe07 db 13h, 0c6h
c_1fe09 db 0abh
c_1fe0a db 3h, 0f9h
c_1fe0c db 3h, 0ebh
c_1fe0e db 13h, 0c6h
c_1fe10 db 0abh
c_1fe11 db 3h, 0f9h
c_1fe13 db 3h, 0ebh
c_1fe15 db 13h, 0c6h
c_1fe17 db 0abh
c_1fe18 db 3h, 0f9h
c_1fe1a db 3h, 0ebh
c_1fe1c db 13h, 0c6h
c_1fe1e db 0abh
c_1fe1f db 3h, 0f9h
c_1fe21 db 3h, 0ebh
c_1fe23 db 13h, 0c6h
c_1fe25 db 0abh
c_1fe26 db 3h, 0f9h
c_1fe28 db 3h, 0ebh
c_1fe2a db 13h, 0c6h
c_1fe2c db 0abh
c_1fe2d db 3h, 0f9h
c_1fe2f db 3h, 0ebh
c_1fe31 db 13h, 0c6h
c_1fe33 db 0abh
c_1fe34 db 3h, 0f9h
c_1fe36 db 3h, 0ebh
c_1fe38 db 13h, 0c6h
c_1fe3a db 0abh
c_1fe3b db 3h, 0f9h
c_1fe3d db 3h, 0ebh
c_1fe3f db 13h, 0c6h
c_1fe41 db 0abh
c_1fe42 db 3h, 0f9h
c_1fe44 db 3h, 0ebh
c_1fe46 db 13h, 0c6h
c_1fe48 db 0abh
c_1fe49 db 3h, 0f9h
c_1fe4b db 3h, 0ebh
c_1fe4d db 13h, 0c6h
c_1fe4f db 0abh
c_1fe50 db 3h, 0f9h
c_1fe52 db 3h, 0ebh
c_1fe54 db 13h, 0c6h
c_1fe56 db 0abh
c_1fe57 db 3h, 0f9h
c_1fe59 db 3h, 0ebh
c_1fe5b db 13h, 0c6h
c_1fe5d db 0abh
c_1fe5e db 3h, 0f9h
c_1fe60 db 3h, 0ebh
c_1fe62 db 13h, 0c6h
c_1fe64 db 0abh
c_1fe65 db 3h, 0f9h
c_1fe67 db 3h, 0ebh
c_1fe69 db 13h, 0c6h
c_1fe6b db 0abh
c_1fe6c db 3h, 0f9h
c_1fe6e db 3h, 0ebh
c_1fe70 db 13h, 0c6h
c_1fe72 db 0abh
c_1fe73 db 3h, 0f9h
c_1fe75 db 3h, 0ebh
c_1fe77 db 13h, 0c6h
c_1fe79 db 0abh
c_1fe7a db 3h, 0f9h
c_1fe7c db 3h, 0ebh
c_1fe7e db 13h, 0c6h
c_1fe80 db 0abh
c_1fe81 db 3h, 0f9h
c_1fe83 db 3h, 0ebh
c_1fe85 db 13h, 0c6h
c_1fe87 db 0abh
c_1fe88 db 3h, 0f9h
c_1fe8a db 3h, 0ebh
c_1fe8c db 13h, 0c6h
c_1fe8e db 0abh
c_1fe8f db 3h, 0f9h
c_1fe91 db 3h, 0ebh
c_1fe93 db 13h, 0c6h
c_1fe95 db 0abh
c_1fe96 db 3h, 0f9h
c_1fe98 db 3h, 0ebh
c_1fe9a db 13h, 0c6h
c_1fe9c db 0abh
c_1fe9d db 3h, 0f9h
c_1fe9f db 3h, 0ebh
c_1fea1 db 13h, 0c6h
c_1fea3 db 0abh
c_1fea4 db 3h, 0f9h
c_1fea6 db 3h, 0ebh
c_1fea8 db 13h, 0c6h
c_1feaa db 0abh
c_1feab db 3h, 0f9h
c_1fead db 3h, 0ebh
c_1feaf db 13h, 0c6h
c_1feb1 db 0abh
c_1feb2 db 3h, 0f9h
c_1feb4 db 3h, 0ebh
c_1feb6 db 13h, 0c6h
c_1feb8 db 0abh
c_1feb9 db 3h, 0f9h
c_1febb db 3h, 0ebh
c_1febd db 13h, 0c6h
c_1febf db 0abh
c_1fec0 db 3h, 0f9h
c_1fec2 db 3h, 0ebh
c_1fec4 db 13h, 0c6h
c_1fec6 db 0abh
c_1fec7 db 3h, 0f9h
c_1fec9 db 3h, 0ebh
c_1fecb db 13h, 0c6h
c_1fecd db 0abh
c_1fece db 3h, 0f9h
c_1fed0 db 3h, 0ebh
c_1fed2 db 13h, 0c6h
c_1fed4 db 0abh
c_1fed5 db 3h, 0f9h
c_1fed7 db 3h, 0ebh
c_1fed9 db 13h, 0c6h
c_1fedb db 0abh
c_1fedc db 3h, 0f9h
c_1fede db 3h, 0ebh
c_1fee0 db 13h, 0c6h
c_1fee2 db 0abh
c_1fee3 db 3h, 0f9h
c_1fee5 db 3h, 0ebh
c_1fee7 db 13h, 0c6h
c_1fee9 db 0abh
c_1feea db 3h, 0f9h
c_1feec db 3h, 0ebh
c_1feee db 13h, 0c6h
c_1fef0 db 0abh
c_1fef1 db 3h, 0f9h
c_1fef3 db 3h, 0ebh
c_1fef5 db 13h, 0c6h
c_1fef7 db 0abh
c_1fef8 db 3h, 0f9h
c_1fefa db 3h, 0ebh
c_1fefc db 13h, 0c6h
c_1fefe db 0abh
c_1feff db 3h, 0f9h
c_1ff01 db 3h, 0ebh
c_1ff03 db 13h, 0c6h
c_1ff05 db 0abh
c_1ff06 db 3h, 0f9h
c_1ff08 db 3h, 0ebh
c_1ff0a db 13h, 0c6h
c_1ff0c db 0abh
c_1ff0d db 3h, 0f9h
c_1ff0f db 3h, 0ebh
c_1ff11 db 13h, 0c6h
c_1ff13 db 0abh
c_1ff14 db 3h, 0f9h
c_1ff16 db 3h, 0ebh
c_1ff18 db 13h, 0c6h
c_1ff1a db 0abh
c_1ff1b db 3h, 0f9h
c_1ff1d db 3h, 0ebh
c_1ff1f db 13h, 0c6h
c_1ff21 db 0abh
c_1ff22 db 3h, 0f9h
c_1ff24 db 3h, 0ebh
c_1ff26 db 13h, 0c6h
c_1ff28 db 0abh
c_1ff29 db 3h, 0f9h
c_1ff2b db 3h, 0ebh
c_1ff2d db 13h, 0c6h
c_1ff2f db 0abh
c_1ff30 db 3h, 0f9h
c_1ff32 db 3h, 0ebh
c_1ff34 db 13h, 0c6h
c_1ff36 db 0abh
c_1ff37 db 3h, 0f9h
c_1ff39 db 3h, 0ebh
c_1ff3b db 13h, 0c6h
c_1ff3d db 0abh
c_1ff3e db 3h, 0f9h
c_1ff40 db 3h, 0ebh
c_1ff42 db 13h, 0c6h
c_1ff44 db 0abh
c_1ff45 db 3h, 0f9h
c_1ff47 db 3h, 0ebh
c_1ff49 db 13h, 0c6h
c_1ff4b db 0abh
c_1ff4c db 3h, 0f9h
c_1ff4e db 3h, 0ebh
c_1ff50 db 13h, 0c6h
c_1ff52 db 0abh
c_1ff53 db 3h, 0f9h
c_1ff55 db 3h, 0ebh
c_1ff57 db 13h, 0c6h
c_1ff59 db 0abh
c_1ff5a db 3h, 0f9h
c_1ff5c db 3h, 0ebh
c_1ff5e db 13h, 0c6h
c_1ff60 db 0abh
c_1ff61 db 3h, 0f9h
c_1ff63 db 3h, 0ebh
c_1ff65 db 13h, 0c6h
c_1ff67 db 0abh
c_1ff68 db 3h, 0f9h
c_1ff6a db 3h, 0ebh
c_1ff6c db 13h, 0c6h
c_1ff6e db 0abh
c_1ff6f db 3h, 0f9h
c_1ff71 db 3h, 0ebh
c_1ff73 db 13h, 0c6h
c_1ff75 db 0abh
c_1ff76 db 3h, 0f9h
c_1ff78 db 3h, 0ebh
c_1ff7a db 13h, 0c6h
c_1ff7c db 0abh
c_1ff7d db 3h, 0f9h
c_1ff7f db 3h, 0ebh
c_1ff81 db 13h, 0c6h
c_1ff83 db 0abh
c_1ff84 db 3h, 0f9h
c_1ff86 db 3h, 0ebh
c_1ff88 db 13h, 0c6h
c_1ff8a db 0abh
c_1ff8b db 3h, 0f9h
c_1ff8d db 3h, 0ebh
c_1ff8f db 13h, 0c6h
c_1ff91 db 0abh
c_1ff92 db 3h, 0f9h
c_1ff94 db 3h, 0ebh
c_1ff96 db 13h, 0c6h
c_1ff98 db 0abh
c_1ff99 db 3h, 0f9h
c_1ff9b db 3h, 0ebh
c_1ff9d db 13h, 0c6h
c_1ff9f db 0abh
c_1ffa0 db 3h, 0f9h
c_1ffa2 db 3h, 0ebh
c_1ffa4 db 13h, 0c6h
c_1ffa6 db 0abh
c_1ffa7 db 3h, 0f9h
c_1ffa9 db 3h, 0ebh
c_1ffab db 13h, 0c6h
c_1ffad db 0abh
c_1ffae db 3h, 0f9h
c_1ffb0 db 3h, 0ebh
c_1ffb2 db 13h, 0c6h
c_1ffb4 db 0abh
c_1ffb5 db 3h, 0f9h
c_1ffb7 db 3h, 0ebh
c_1ffb9 db 13h, 0c6h
c_1ffbb db 0abh
c_1ffbc db 3h, 0f9h
c_1ffbe db 3h, 0ebh
c_1ffc0 db 13h, 0c6h
c_1ffc2 db 0abh
c_1ffc3 db 3h, 0f9h
c_1ffc5 db 3h, 0ebh
c_1ffc7 db 13h, 0c6h
c_1ffc9 db 0abh
c_1ffca db 3h, 0f9h
c_1ffcc db 3h, 0ebh
c_1ffce db 13h, 0c6h
c_1ffd0 db 0abh
c_1ffd1 db 3h, 0f9h
c_1ffd3 db 3h, 0ebh
c_1ffd5 db 13h, 0c6h
c_1ffd7 db 0abh
c_1ffd8 db 3h, 0f9h
c_1ffda db 3h, 0ebh
c_1ffdc db 13h, 0c6h
c_1ffde db 0abh
c_1ffdf db 3h, 0f9h
c_1ffe1 db 3h, 0ebh
c_1ffe3 db 13h, 0c6h
c_1ffe5 db 0abh
c_1ffe6 db 3h, 0f9h
c_1ffe8 db 3h, 0ebh
c_1ffea db 13h, 0c6h
c_1ffec db 0abh
c_1ffed db 3h, 0f9h
c_1ffef db 3h, 0ebh
c_1fff1 db 13h, 0c6h
c_1fff3 db 0abh
c_1fff4 db 3h, 0f9h
c_1fff6 db 3h, 0ebh
c_1fff8 db 13h, 0c6h
c_1fffa db 0abh
c_1fffb db 3h, 0f9h
c_1fffd db 3h, 0ebh
c_1ffff db 13h, 0c6h
c_20001 db 0abh
c_20002 db 3h, 0f9h
c_20004 db 3h, 0ebh
c_20006 db 13h, 0c6h
c_20008 db 0abh
c_20009 db 3h, 0f9h
c_2000b db 3h, 0ebh
c_2000d db 13h, 0c6h
c_2000f db 0abh
c_20010 db 3h, 0f9h
c_20012 db 3h, 0ebh
c_20014 db 13h, 0c6h
c_20016 db 0abh
c_20017 db 3h, 0f9h
c_20019 db 3h, 0ebh
c_2001b db 13h, 0c6h
c_2001d db 0abh
c_2001e db 3h, 0f9h
c_20020 db 3h, 0ebh
c_20022 db 13h, 0c6h
c_20024 db 0abh
c_20025 db 3h, 0f9h
c_20027 db 3h, 0ebh
c_20029 db 13h, 0c6h
c_2002b db 0abh
c_2002c db 3h, 0f9h
c_2002e db 3h, 0ebh
c_20030 db 13h, 0c6h
c_20032 db 0abh
c_20033 db 3h, 0f9h
c_20035 db 3h, 0ebh
c_20037 db 13h, 0c6h
c_20039 db 0abh
c_2003a db 3h, 0f9h
c_2003c db 3h, 0ebh
c_2003e db 13h, 0c6h
c_20040 db 0abh
c_20041 db 3h, 0f9h
c_20043 db 3h, 0ebh
c_20045 db 13h, 0c6h
c_20047 db 0abh
c_20048 db 3h, 0f9h
c_2004a db 3h, 0ebh
c_2004c db 13h, 0c6h
c_2004e db 0abh
c_2004f db 3h, 0f9h
c_20051 db 3h, 0ebh
c_20053 db 13h, 0c6h
c_20055 db 0abh
c_20056 db 3h, 0f9h
c_20058 db 3h, 0ebh
c_2005a db 13h, 0c6h
c_2005c db 0abh
c_2005d db 3h, 0f9h
c_2005f db 3h, 0ebh
c_20061 db 13h, 0c6h
c_20063 db 0abh
c_20064 db 3h, 0f9h
c_20066 db 3h, 0ebh
c_20068 db 13h, 0c6h
c_2006a db 0abh
c_2006b db 3h, 0f9h
c_2006d db 3h, 0ebh
c_2006f db 13h, 0c6h
c_20071 db 0abh
c_20072 db 3h, 0f9h
c_20074 db 3h, 0ebh
c_20076 db 13h, 0c6h
c_20078 db 0c3h
_poly_walk endp

/* 0x21d03 */
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
        call dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*27
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
c_20184 db 90h
_fill_rect endp

/* 0x21e0f */
_draw_compressed_bitmap proc near
        jmp dword ptr DGROUP:_g_compressed_body_vector
_draw_compressed_bitmap endp
POLYGON_TEXT ends
}
#else

/*
 * **The polygon walker's two chains**, DGROUP 0x44d0..0x44de, 0x0e bytes.
 *
 * `draw_polygon` finds the topmost and the bottommost vertex, and the ring
 * between them is two chains - right from one to the other and left back
 * again. `top_at` and `bottom_at` are those two vertices as byte offsets into
 * the working arrays; `right_count` and `left_count` are how many points each
 * chain got; `at` is the cursor stepping through them an edge at a time and
 * `remaining` the count parked while an edge is drawn, because the original
 * needs the register.
 */
struct engine_polygon_chains {
    uint16_t  top_at;          /* +0x00 [2] */
    uint16_t  bottom_at;          /* +0x02 [2] */
    uint16_t  right_count;          /* +0x04 [2] */
    uint16_t  left_count;          /* +0x06 [2] */
    uint16_t  remaining;          /* +0x08 [2] */
    uint16_t  at;          /* +0x0a [2] */
    uint16_t  chain;              /* +0x0c [2]  0 is the left chain and 2 the right; a computed jmp on it */
} PACKED;

struct engine_polygon_chains g_engine_polygon_chains;

/*
 * **The polygon walker's own state**, DGROUP 0x44de..0x44ea, 0x0c bytes.
 *
 * `prev_x`/`prev_y` are the vertex before this one, which is how a repeated
 * point is dropped; `span_seg` is the span buffer's segment; `span_step` is
 * what `poly_walk` adds to its cursor beside the two, so one chain's spans
 * land in the buffer's left column and the other's in the right.
 *
 * The outline is a second pass: when its colour differs from the fill,
 * `outline_count` points of the ring are kept in `closed_x`/`closed_y` to be
 * drawn after; `second_pass` and `second_count` are the other copy, the one
 * kept when the two orderings came out exactly equal.
 */
struct engine_polygon_state {
    int16_t   prev_x;          /* +0x00 [2] */
    int16_t   prev_y;          /* +0x02 [2] */
    dg_seg_t  span_seg;          /* +0x04 [2] */
    uint16_t  outline_count;          /* +0x06 [2] */
    uint16_t  second_count;          /* +0x08 [2] */
    uint8_t   span_step;          /* +0x0a [1] */
    uint8_t   second_pass;          /* +0x0b [1] */
} PACKED;

struct engine_polygon_state g_engine_polygon_state;

/*
 * 1ee5:1c27, image 0x20a77
 *
 * Fill a polygon, and outline it if the two colours differ.
 *
 * Six DGROUP arrays do the work: 0x393c and 0x3964 hold the points as given,
 * 0x398c and 0x39b4 the ones actually used, and 0x39dc and 0x3a04 a closed copy
 * kept for the outline pass - the fill destroys the working pair.
 *
 * With fewer than three points, or with filling off at DGROUP 0x389c, there is
 * nothing to fill and it draws the outline and stops.
 *
 * The fill itself is the classic one. The points are walked backwards, dropping
 * any that repeat the last, and the topmost and bottommost are found on the way
 * - by y, and by x when two share a y, which is what makes the choice
 * unambiguous. If those two turn out to have the same y the whole thing is one
 * horizontal line and goes straight to `clip_and_draw_line`.
 *
 * Otherwise the two edges leaving the top vertex are compared to see which way
 * round the polygon is wound - by slope, and the comparison is done with two
 * divisions rather than a cross product so it cannot overflow - and the arrays
 * are reversed if it is the wrong way. Then the outline is split into a left
 * chain and a right chain, each walked edge by edge into a buffer of span ends,
 * and the whole buffer handed to the driver's span filler in one call.
 *
 * This routine is hand-written assembly - BP is a general register throughout,
 * and most of its 3,674 bytes are an unrolled loop entered by computed jump -
 * so the port follows its registers rather than pretending it was compiled.
 */
void draw_polygon(int16_t n, const int16_t *xs, const int16_t *ys)
{
    dg_seg_t seg;
    /* The span buffer's first byte. The edge routines step a 16-bit offset
       inside it, exactly as the original steps `di` against a segment, so the
       base and the offset stay apart; `seg` itself is still filed at
       0x44e2 below. */
    uint8_t far * span;
    int16_t ax, bx, cx, dx, si, di, bp;
    int16_t i;

    g_engine_polygon_state.span_seg = 0;
    g_engine_polygon_state.second_pass = 0;

    if (n >= 0) {
        g_vmds.palettes.clip_count = (uint16_t)n;
        for (i = 0; i < n; i++) {
            g_vmds.poly_x[i] = xs[i];
            g_vmds.poly_y[i] = ys[i];
        }
    }

    if (n < 2)
        goto out;

    if (n == 2) {
        poly_outline(g_vmds.poly_x, g_vmds.poly_y, 1);
        goto out;
    }

    if (g_vmds.fill_enabled == 0) {
        /* Filling is off: close the ring and draw it as lines. */
        n = (int16_t)g_vmds.palettes.clip_count;
        g_vmds.poly_x[n] = ((uint16_t)g_vmds.poly_x[0]);
        g_vmds.poly_y[n] = ((uint16_t)g_vmds.poly_y[0]);
        poly_outline(g_vmds.poly_x, g_vmds.poly_y, n);
        goto out;
    }

    if (g_vmds.second_colour != g_vmds.fill_colour) {
        n = (int16_t)g_vmds.palettes.clip_count;
        g_engine_polygon_state.outline_count = (uint16_t)n;

        for (i = 0; i < n; i++) {
            g_vmds.closed_x[i] = ((uint16_t)g_vmds.poly_x[i]);
            g_vmds.closed_y[i] = ((uint16_t)g_vmds.poly_y[i]);
        }
        g_vmds.closed_x[n] = ((uint16_t)g_vmds.poly_x[0]);
        g_vmds.closed_y[n] = ((uint16_t)g_vmds.poly_y[0]);
    }

    if (g_vmds.clip_enabled != 0)
        clip_polygon();

    n = (int16_t)g_vmds.palettes.clip_count;
    if (n < 2)
        goto out;
    if (n == 2) {
        poly_outline(g_vmds.poly_x, g_vmds.poly_y, 1);
        goto out;
    }

    si = (int16_t)((n - 1) * 2);
    g_engine_polygon_state.prev_y = ((uint16_t)g_vmds.poly_y[0]);
    dx = 0x7fff;
    bx = (int16_t)0x8001;
    g_engine_polygon_state.prev_x = ((uint16_t)g_vmds.poly_x[0]);
    bp = dx;
    cx = bx;
    di = 0;
    g_engine_polygon_chains.top_at = 0;
    g_engine_polygon_chains.bottom_at = 0;

    for (; si >= 0; si -= 2) {
        ax = g_vmds.poly_y[si >> 1];

        if (ax == g_engine_polygon_state.prev_y
            && g_vmds.poly_x[si >> 1] == g_engine_polygon_state.prev_x)
            continue;

        g_engine_polygon_state.prev_y = ax;
        g_vmds.work_y[di >> 1] = ax;

        /*
         * The tie-breaks go opposite ways, and which way is not a matter of
         * taste: the topmost keeps the point *further* right of two on the
         * same row and the bottommost the one further left, so the two chains
         * leave the vertices from opposite corners. Reading either the other
         * way round picks a different vertex to start from, and the fill can
         * still come out right - it did, pixel for pixel - while the arrays
         * the routine leaves behind do not match, which is how it was caught.
         */
        if (ax < dx
            || (ax == dx && g_vmds.poly_x[si >> 1] > cx)) {
            g_engine_polygon_chains.top_at = (uint16_t)di;
            dx = ax;
            cx = g_vmds.poly_x[si >> 1];
        }

        if (ax > bx
            || (ax == bx && g_vmds.poly_x[si >> 1] <= bp)) {
            g_engine_polygon_chains.bottom_at = (uint16_t)di;
            bx = ax;
            bp = g_vmds.poly_x[si >> 1];
        }

        ax = g_vmds.poly_x[si >> 1];
        g_engine_polygon_state.prev_x = ax;
        g_vmds.work_x[di >> 1] = ax;
        di += 2;
    }

    if (dx == bx) {
        /* Every point on one row: one line, and nothing to fill. */
        if (g_vmds.screen.mode_kind == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            g_vmds.clip_top = (int16_t)((uint16_t)g_vmds.clip_top >> 1);
            g_vmds.clip_bottom = (int16_t)((uint16_t)g_vmds.clip_bottom >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            g_vmds.clip_top = (int16_t)((uint16_t)g_vmds.clip_top << 1);
            g_vmds.clip_bottom = (int16_t)((uint16_t)g_vmds.clip_bottom << 1);
        }
        goto out;
    }

    ax = (int16_t)((uint16_t)di >> 1);
    if (ax < 2)
        goto out;

    if (ax == 2) {
        if (g_vmds.screen.mode_kind == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            g_vmds.clip_top = (int16_t)((uint16_t)g_vmds.clip_top >> 1);
            g_vmds.clip_bottom = (int16_t)((uint16_t)g_vmds.clip_bottom >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            g_vmds.clip_top = (int16_t)((uint16_t)g_vmds.clip_top << 1);
            g_vmds.clip_bottom = (int16_t)((uint16_t)g_vmds.clip_bottom << 1);
        }
        goto out;
    }

    cx = di;
    g_vmds.palettes.clip_count = (uint16_t)ax;

    /*
     * Which way round is it wound? Compare the slopes of the two edges leaving
     * the top vertex. A zero rise is turned into one with a huge run so the
     * comparison still means something, and the two slopes are compared as
     * quotient-then-remainder rather than by cross-multiplying, because the
     * product would not fit.
     */
    si = (int16_t)g_engine_polygon_chains.top_at;
    di = (int16_t)(si + 2);
    if (di >= cx)
        di = 0;

    dx = (int16_t)(g_vmds.work_x[di >> 1] - g_vmds.work_x[si >> 1]);
    bp = (int16_t)(g_vmds.work_y[di >> 1] - g_vmds.work_y[si >> 1]);
    if (bp == 0) {
        bp = 1;
        dx = (dx >= 0) ? 0x7fff : (int16_t)-0x7fff;
    }

    di = (int16_t)(si - 2);
    if (di < 0)
        di = (int16_t)(di + cx);

    ax = (int16_t)(g_vmds.work_x[di >> 1] - g_vmds.work_x[si >> 1]);
    bx = (int16_t)(g_vmds.work_y[di >> 1] - g_vmds.work_y[si >> 1]);
    if (bx == 0) {
        bx = 1;
        if (ax < 0) {
            ax = (int16_t)0x8001;
            goto ax_negative;
        }
        ax = (int16_t)-(int16_t)0x8001;
    }

    if (ax < 0)
        goto ax_negative;

    if (dx <= 0)
        goto reverse;
    goto compare;

ax_negative:
    if (dx >= 0)
        goto keep;

    dx = (int16_t)-dx;
    ax = (int16_t)-ax;
    {
        int16_t t = dx;

        dx = ax;
        ax = t;
        t = bx;
        bx = bp;
        bp = t;
    }

compare:
    /*
     * Each edge is a rise over a run and each keeps its own pair: `dx` goes
     * with `bp`, `ax` with `bx`, and the `xchg` above swaps the two pairs
     * whole rather than breaking them up. Dividing one edge's rise by the
     * other's run compares nothing, and the winding then comes out backwards
     * on the polygons where the two slopes happen to straddle - which is a
     * fill built from the wrong chains.
     */
    {
        uint16_t q2 = (uint16_t)ax / (uint16_t)bx;
        uint16_t r2 = (uint16_t)ax % (uint16_t)bx;
        uint16_t q1 = (uint16_t)dx / (uint16_t)bp;
        uint16_t r1 = (uint16_t)dx % (uint16_t)bp;

        if (q1 > q2)
            goto keep;
        if (q1 < q2)
            goto reverse;

        /*
         * Equal whole parts, so the remainders decide - each shifted up a
         * word and divided by its own run again, which is the original's way
         * of getting another sixteen bits of the quotient without a 32-bit
         * divide it has no instruction for.
         */
        {
            uint16_t f1 = (uint16_t)(((uint32_t)r1 << 16) / (uint16_t)bp);
            uint16_t f2 = (uint16_t)(((uint32_t)r2 << 16) / (uint16_t)bx);

            if (f1 < f2)
                goto reverse;
            if (f1 > f2)
                goto keep;
        }
    }

    /* Exactly equal: keep a copy for a second pass and go on. */
    g_engine_polygon_state.second_pass = 1;
    g_engine_polygon_state.second_count = (uint16_t)cx;
    for (i = 0; i < cx; i += 2) {
        g_vmds.closed_x[i >> 1] = g_vmds.work_x[i >> 1];
        g_vmds.closed_y[i >> 1] = g_vmds.work_y[i >> 1];
    }

keep:
    for (i = 0; i < cx; i += 2) {
        g_vmds.poly_x[i >> 1] = g_vmds.work_x[i >> 1];
        g_vmds.poly_y[i >> 1] = g_vmds.work_y[i >> 1];
    }
    goto chains;

reverse:
    for (i = 0; i < cx; i += 2) {
        g_vmds.poly_x[(cx - 2 - i) >> 1] = g_vmds.work_x[i >> 1];
        g_vmds.poly_y[(cx - 2 - i) >> 1] = g_vmds.work_y[i >> 1];
    }
    g_engine_polygon_chains.top_at = (uint16_t)(cx - 2 - (int16_t)g_engine_polygon_chains.top_at);
    g_engine_polygon_chains.bottom_at = (uint16_t)(cx - 2 - (int16_t)g_engine_polygon_chains.bottom_at);

chains:
    /* The right chain: from the bottom vertex up to the top. */
    dx = g_vmds.poly_y[g_engine_polygon_chains.bottom_at >> 1];
    si = (int16_t)g_engine_polygon_chains.top_at;
    di = 0;
    for (;;) {
        g_vmds.work_x[di >> 1] = g_vmds.poly_x[si >> 1];
        ax = g_vmds.poly_y[si >> 1];
        g_vmds.work_y[di >> 1] = ax;
        di += 2;
        if (ax >= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    g_engine_polygon_chains.right_count = (uint16_t)((uint16_t)di >> 1);

    /* The left chain: from the top vertex down to the bottom. */
    dx = g_vmds.poly_y[g_engine_polygon_chains.top_at >> 1];
    si = (int16_t)g_engine_polygon_chains.bottom_at;
    for (;;) {
        g_vmds.work_x[di >> 1] = g_vmds.poly_x[si >> 1];
        ax = g_vmds.poly_y[si >> 1];
        g_vmds.work_y[di >> 1] = ax;
        di += 2;
        if (ax <= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    g_engine_polygon_chains.left_count = (uint16_t)(((uint16_t)di >> 1) - g_engine_polygon_chains.right_count);

    seg = g_vm_driver.span_buffer_seg;
    span = MK_FP(seg, 0);

    g_engine_polygon_chains.chain = 2;
    g_engine_polygon_chains.at = 0;
    ax = (int16_t)g_engine_polygon_chains.right_count;

    for (;;) {
        ax--;
        if (ax == 0) {
            if (g_engine_polygon_chains.chain != 0) {
                g_engine_polygon_chains.at += 2;
                g_engine_polygon_chains.chain = 0;
                ax = (int16_t)g_engine_polygon_chains.left_count;
                continue;
            }
            break;
        }

        g_engine_polygon_chains.remaining = (uint16_t)ax;

        si = (int16_t)g_engine_polygon_chains.at;
        g_engine_polygon_chains.at = (uint16_t)(si + 2);

        {
            int16_t x1 = g_vmds.work_x[si >> 1];
            int16_t x2 = g_vmds.work_x[(si >> 1) + 1];
            int16_t y1 = g_vmds.work_y[si >> 1];
            int16_t y2 = g_vmds.work_y[(si >> 1) + 1];
            int16_t adx = (int16_t)(x1 - x2);
            int16_t ady;

            if (adx < 0)
                adx = (int16_t)-adx;

            if (adx == 0) {
                poly_edge_vertical(span, x1, y1, y2);
            } else {
                ady = (int16_t)(y1 - y2);
                if (ady < 0)
                    ady = (int16_t)-ady;

                if (ady == 0) {
                    /* One row: write whichever end the side wants. */
                    int16_t lo = (x1 < x2) ? x1 : x2;
                    int16_t hi = (x1 < x2) ? x2 : x1;
                    uint16_t at = (uint16_t)((y1 << 2) + g_engine_polygon_chains.chain);

                    *(int16_t *)(void *)(span + at) =
                        (g_engine_polygon_chains.chain == 0) ? lo : hi;
                } else if (adx < ady) {
                    poly_edge_steep(span, x1, x2, y1, y2);
                } else if (adx > ady) {
                    /* `cmp [0x44dc],0; jne 0x21070; je 0x1f4a1`. */
                    if (g_engine_polygon_chains.chain != 0)
                        poly_edge_shallow_right(span, x1, x2, y1, y2);
                    else
                        poly_edge_shallow_left(span, x1, x2, y1, y2);
                } else {
                    poly_edge_diagonal(span, x1, x2, y1, y2);
                }
            }
        }

        ax = (int16_t)g_engine_polygon_chains.remaining;
    }

    /* Hand the whole buffer to the driver's span filler in one call. */
    {
        int16_t top = g_vmds.poly_y[g_engine_polygon_chains.top_at >> 1];
        int16_t bottom = g_vmds.poly_y[g_engine_polygon_chains.bottom_at >> 1];
        /* The list starts four words before the first row's pair: the
           first row and the row count, in the segment below `seg`. */
        /* The paragraph below the buffer - `seg - 1` - is where the list's
           own header lives. */
        uint8_t *spans = span - 0x10 + (uint16_t)((top << 2) + 0x0c);
        int16_t rows = (int16_t)(bottom - top + 1);

        g_engine_polygon_state.span_seg = seg;

        spans[0] = (uint8_t)top;
        spans[1] = (uint8_t)((uint16_t)top >> 8);
        spans[2] = (uint8_t)rows;
        spans[3] = (uint8_t)((uint16_t)rows >> 8);

        vm_fill_spans(spans);
    }

    if (g_vmds.second_colour != g_vmds.fill_colour)
        poly_outline(g_vmds.closed_x, g_vmds.closed_y, (int16_t)g_engine_polygon_state.outline_count);

out:
    if (g_engine_polygon_state.second_pass != 0) {
        /* The second pass, for a polygon whose two top edges had one slope. */
        g_engine_polygon_state.second_pass = 0;
        cx = (int16_t)g_engine_polygon_state.second_count;
        for (i = 0; i < cx; i += 2) {
            g_vmds.work_x[i >> 1] = ((uint16_t)g_vmds.closed_x[i >> 1]);
            g_vmds.work_y[i >> 1] = ((uint16_t)g_vmds.closed_y[i >> 1]);
        }
        goto reverse;
    }
}

/*
 * 1ee5:2053, image 0x20ea3
 *
 * Draw the outline: one `clip_and_draw_line` per side, from two arrays of
 * points. The second half of the routine is the same again with the vertical
 * window and every y halved, which is the mode where a row is two scan lines -
 * the byte at DGROUP 0x3f78 says which.
 */
void poly_outline(int16_t *xs, int16_t *ys, int16_t n)
{
    if (g_vmds.screen.mode_kind == 0) {
        while (n-- > 0) {
            clip_and_draw_line(xs[0], ys[0],
                               xs[1],
                               ys[1]);
            xs++;
            ys++;
        }
        return;
    }

    g_vmds.clip_top = (int16_t)((uint16_t)g_vmds.clip_top >> 1);
    g_vmds.clip_bottom = (int16_t)((uint16_t)g_vmds.clip_bottom >> 1);

    while (n-- > 0) {
        clip_and_draw_line(xs[0], (int16_t)(ys[0] >> 1),
                           xs[1],
                           (int16_t)(ys[1] >> 1));
        xs++;
        ys++;
    }

    g_vmds.clip_top = (int16_t)((uint16_t)g_vmds.clip_top << 1);
    g_vmds.clip_bottom = (int16_t)((uint16_t)g_vmds.clip_bottom << 1);
}

/*
 * 1ee5:209f, image 0x20eef - an edge with no run at all.
 *
 * Both ends have the same x, so every row gets it: no fractional part and no
 * step. The two ends are put in top-to-bottom order first.
 */
void poly_edge_vertical(uint8_t far * span, int16_t x,
                        int16_t y1, int16_t y2)
{
    if (y2 <= y1) {
        int16_t t = y1;

        y1 = y2;
        y2 = t;
    }

    g_engine_polygon_state.span_step = 2;
    poly_walk(span, x, 0, 0, 0, (int16_t)(y2 - y1 + 1), (uint16_t)y1);
}

/*
 * 1ee5:20bb, image 0x20f0b - an edge steeper than 45 degrees.
 *
 * Bresenham's, written out: the error starts at `2 * dx - dy`, a step that
 * takes the error non-negative moves x by one and adds `2 * (dx - dy)`, and
 * anything else adds `2 * dx`.
 *
 * The original unrolls the body six times and picks the direction by which way
 * the rows run - `di` four bytes forward or four back - which is the same two
 * loops the port writes as one with a signed step.
 */
void poly_edge_steep(uint8_t far * span, int16_t x1, int16_t x2,
                     int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e1, e2, x, count, sign;
    uint16_t di;

    if (x1 >= x2) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)((y1 << 2) + g_engine_polygon_chains.chain);

    dy = (int16_t)(y1 - y2);
    sign = (dy >= 0) ? 0 : -1;
    if (dy < 0)
        dy = (int16_t)-dy;

    dx = (int16_t)(x2 - x1);
    e2 = (int16_t)(dx * 2);
    x = x1;
    err = (int16_t)(e2 - dy);
    e1 = (int16_t)((int16_t)((dx - dy) * 2) ^ e2);
    count = (int16_t)(dy + 1);

    /*
     * `sign` is the sign the absolute value above threw away: zero means the
     * rows run backwards through the buffer, which the original reaches by a
     * second copy of the whole unrolled body.
     */
    while (count-- > 0) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + (sign == 0 ? -4 : 4));

        /*
         * `cmp bh,0x80 / sbb dx,dx`: all ones when the error is not negative.
         * It steps x, and it picks the increment out of `e1`, which holds
         * the two increments xor'd together: `mask & e1 ^ e2` is
         * `2 * (dx - dy)` on a step and `2 * dx` otherwise. Adding `e1`
         * itself - which this did until 2026-09-27 - put S15's machine 21
         * flips off the original's.
         */
        {
            int16_t mask = (int16_t)(err >= 0 ? -1 : 0);

            x = (int16_t)(x - mask);
            err = (int16_t)(err + ((mask & e1) ^ e2));
        }
    }
}

/*
 * 1ee5:21f9, image 0x21049 - an edge at exactly 45 degrees.
 *
 * One across for every one down, so again no fractional part: the step is 1 or
 * -1 by which way the x runs.
 */
void poly_edge_diagonal(uint8_t far * span, int16_t x1, int16_t x2,
                        int16_t y1, int16_t y2)
{
    /*
     * The ends are put top-first, so the swap is the one that happens when the
     * first end is *below* the second. Testing it the other way round leaves
     * the ends bottom-first, and the count below - `y2 - y1 + 1` - then comes
     * out zero or negative and the edge is not written at all. A polygon whose
     * side is at exactly 45 degrees loses that side, which is why the fault
     * hid: 45 is a special case of its own, and everything shallower or
     * steeper goes elsewhere.
     */
    if (y1 >= y2) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    g_engine_polygon_state.span_step = 2;
    poly_walk(span, x1, 0, (x1 < x2) ? 1 : -1, 0,
              (int16_t)(-(int16_t)(y1 - y2) + 1), (uint16_t)y1);
}

/*
 * 1ee5:2220, image 0x21070 - an edge shallower than 45 degrees,
 * the right chain's.
 *
 * Walked *along* rather than down: one row covers several columns, so the loop
 * runs over x and only writes when the error says the row has changed. Unrolled
 * eight times in the original, with the "catch up the error" chain unrolled
 * eight times inside that; a speed device with no observable difference, so the
 * port writes the loop.
 *
 * This is the one the **right** chain takes, where DGROUP 0x44dc is 2: it
 * writes the second slot of each row, `y * 4 + 2`, and counts x **down**.
 *
 * This and `poly_edge_shallow_left` are **not** one routine with a flag: they
 * differ in three
 * places - which way the ends are put in order, which of the row's two slots
 * is written, and which way x counts - and the original has two of them, each
 * its own entry reached by a computed `jmp` on DGROUP 0x44dc. Written as one
 * function with a flag they could not be told apart by the verifier, and the
 * coverage tool counted neither.
 */
void poly_edge_shallow_right(uint8_t far * span, int16_t x1, int16_t x2,
                             int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e, x, count, di_step;
    uint16_t di;

    if ((x1 <= x2)) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)(((y1 << 1) + 1) << 1);   /* the row's second slot */
    di_step = 2;

    dy = (int16_t)(y2 - y1);
    if (dy < 0) {
        dy = (int16_t)-dy;
        di_step = -6;
    }

    count = dy;

    dx = (int16_t)(x2 - x1);
    if (dx > 0)
        dx = (int16_t)-dx;

    err = dx;
    e = (int16_t)((dx + dy) * 2);
    dy = (int16_t)(dy * 2);
    err = (int16_t)(err + dy);

    x = x1;

    /*
     * The first end is written outside the loop, and the *only* thing between
     * it and the second is one step of x and a catch-up: the error is not
     * advanced by `e` yet. Folding that first write into the loop adds an
     * `err += e` that the original does not do there, and on a shallow edge
     * `e` is negative, so the catch-up steps x an extra column or two and
     * every end after it is wrong by that much.
     *
     * `stosw` advances DI by two of its own accord and the original adds its
     * 2 or -6 on top, so a row costs four bytes either way - the same slot of
     * the next row, or of the one before. Adding only the 2 or -6 lands on the
     * *other* slot of the row just written, which leaves the right ends of a
     * whole run of rows unset and the driver fills those to the clip's right
     * edge: a stripe from wherever the polygon was to x=639.
     */
    *(int16_t *)(void *)(span + di) = x;
    di = (uint16_t)(di + 2 + di_step);
    x = (int16_t)(x - 1);

    while (err < 0) {
        x = (int16_t)(x - 1);
        err = (int16_t)(err + dy);
    }

    for (;;) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);
        x = (int16_t)(x - 1);

        if (--count == 0)
            break;

        err = (int16_t)(err + e);
        while (err < 0) {
            x = (int16_t)(x - 1);
            err = (int16_t)(err + dy);
        }
    }
}

/*
 * 1ee5:22db, image 0x2112b - an edge shallower than 45 degrees,
 * the left chain's.
 *
 * Walked *along* rather than down: one row covers several columns, so the loop
 * runs over x and only writes when the error says the row has changed. Unrolled
 * eight times in the original, with the "catch up the error" chain unrolled
 * eight times inside that; a speed device with no observable difference, so the
 * port writes the loop.
 *
 * This is the one the **left** chain takes, where DGROUP 0x44dc is 0: it
 * writes the first slot of each row, `y * 4`, and counts x **up**.
 *
 * This and `poly_edge_shallow_right` are **not** one routine with a flag: they
 * differ in three
 * places - which way the ends are put in order, which of the row's two slots
 * is written, and which way x counts - and the original has two of them, each
 * its own entry reached by a computed `jmp` on DGROUP 0x44dc. Written as one
 * function with a flag they could not be told apart by the verifier, and the
 * coverage tool counted neither.
 */
void poly_edge_shallow_left(uint8_t far * span, int16_t x1, int16_t x2,
                            int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e, x, count, di_step;
    uint16_t di;

    if ((x1 >= x2)) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)(y1 << 2);                /* the row's first slot */
    di_step = 2;

    dy = (int16_t)(y2 - y1);
    if (dy < 0) {
        dy = (int16_t)-dy;
        di_step = -6;
    }

    count = dy;

    dx = (int16_t)(x2 - x1);
    if (dx > 0)
        dx = (int16_t)-dx;

    err = dx;
    e = (int16_t)((dx + dy) * 2);
    dy = (int16_t)(dy * 2);
    err = (int16_t)(err + dy);

    x = x1;

    /*
     * The first end is written outside the loop, and the *only* thing between
     * it and the second is one step of x and a catch-up: the error is not
     * advanced by `e` yet. Folding that first write into the loop adds an
     * `err += e` that the original does not do there, and on a shallow edge
     * `e` is negative, so the catch-up steps x an extra column or two and
     * every end after it is wrong by that much.
     *
     * `stosw` advances DI by two of its own accord and the original adds its
     * 2 or -6 on top, so a row costs four bytes either way - the same slot of
     * the next row, or of the one before. Adding only the 2 or -6 lands on the
     * *other* slot of the row just written, which leaves the right ends of a
     * whole run of rows unset and the driver fills those to the clip's right
     * edge: a stripe from wherever the polygon was to x=639.
     */
    *(int16_t *)(void *)(span + di) = x;
    di = (uint16_t)(di + 2 + di_step);
    x = (int16_t)(x + 1);

    while (err < 0) {
        x = (int16_t)(x + 1);
        err = (int16_t)(err + dy);
    }

    for (;;) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);
        x = (int16_t)(x + 1);

        if (--count == 0)
            break;

        err = (int16_t)(err + e);
        while (err < 0) {
            x = (int16_t)(x + 1);
            err = (int16_t)(err + dy);
        }
    }
}

/*
 * 1ee5:239c, image 0x211ec
 *
 * One edge of the polygon, walked down the scanlines, writing the x it reaches
 * on each into the span buffer.
 *
 * `x` steps by `step` every row plus the carry out of a fractional accumulator
 * that `frac` is added to - a fixed-point DDA rather than Bresenham's error
 * term - and `di` walks the buffer four bytes a row, which is one pair of
 * span ends.
 *
 * **The original is a computed jump into an unrolled loop.** It works out
 * `0x3e28 - 7 * count` and jumps there, landing exactly `count` copies of the
 * seven-byte body from the end - about four hundred of them, which is most of
 * this routine's 3,674 bytes. That is a speed device with no observable
 * difference, so the port writes the loop.
 */
void poly_walk(uint8_t far * span, int16_t x, int16_t frac, int16_t step,
               int16_t acc, int16_t count, uint16_t di)
{
    int16_t di_step = (int8_t)g_engine_polygon_state.span_step;

    di = (uint16_t)((di << 2) + g_engine_polygon_chains.chain);

    while (count-- > 0) {
        uint32_t t;

        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);

        t = (uint32_t)(uint16_t)acc + (uint32_t)(uint16_t)frac;
        acc = (int16_t)t;
        x = (int16_t)(x + step + (int16_t)(t >> 16));
    }
}

/*
 * 0x21d03
 *
 * Fill a rectangle, clipped, and optionally outline it.
 *
 * The fill is done by turning the rectangle into a **span list** - the first
 * row, the row count, then one `x1, x2` pair per row, all identical - and
 * handing it to the driver at VGA:0x0be6 through the vector at DGROUP 0x43b2.
 * That is a general span filler being used for the simplest possible case,
 * which is why a solid rectangle costs one entry per scan line.
 *
 * Clipping shrinks the rectangle in place against the box at DGROUP
 * 0x3894..0x389a, and the original x and y are pushed before that and popped
 * back afterwards, because the outline below wants the unclipped ones.
 *
 * The outline at 0x2013f is **not transcribed**. It draws four lines through
 * 0x21e34 with a stack-reuse trick - each call pushes only the arguments that
 * differ from the last and relies on the rest still being there - which has no
 * honest expression in C without modelling the stack. It is never reached on
 * the intro screens: over 2,108 calls the two border bytes at DGROUP
 * 0x389d/0x389e - the driver's own colour bytes, seen through DGROUP - were
 * always equal, which is the condition that skips it.
 */
void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t right = (int16_t)(x + w - 1);
    int16_t bottom = (int16_t)(y + h - 1);

    if (g_vmds.fill_enabled != 0) {
        int16_t cx = x, cy = y, cw = w, ch = h;

        if (g_vmds.clip_enabled != 0) {
            int16_t d = (int16_t)(cx - g_vmds.clip_left);
            if (d < 0) {
                cx = (int16_t)(cx - d);
                cw = (int16_t)(cw + d);
            }
            d = (int16_t)(cy - g_vmds.clip_top);
            if (d < 0) {
                cy = (int16_t)(cy - d);
                ch = (int16_t)(ch + d);
            }
            d = (int16_t)(g_vmds.clip_right - right);
            if (d < 0)
                cw = (int16_t)(cw + d);
            d = (int16_t)(g_vmds.clip_bottom - bottom);
            if (d < 0)
                ch = (int16_t)(ch + d);
        }

        if (cw > 0 && ch > 0) {
            uint8_t *p = MK_FP(g_vm_driver.span_buffer_seg, 0);
            int16_t n = ch;
            int16_t x2 = (int16_t)(cx + cw - 1);

            *p++ = (uint8_t)(cy & 0xFF);
            *p++ = (uint8_t)((uint16_t)cy >> 8);
            *p++ = (uint8_t)(ch & 0xFF);
            *p++ = (uint8_t)((uint16_t)ch >> 8);
            do {
                *p++ = (uint8_t)(cx & 0xFF);
                *p++ = (uint8_t)((uint16_t)cx >> 8);
                *p++ = (uint8_t)(x2 & 0xFF);
                *p++ = (uint8_t)((uint16_t)x2 >> 8);
            } while (--n);

            vm_fill_spans(MK_FP(g_vm_driver.span_buffer_seg, 0));
        }
    }

    if (g_vmds.fill_enabled != 0 && g_vmds.fill_colour == g_vmds.second_colour)
        return;
    not_transcribed("0x2013f, the rectangle outline");
}

/*
 * 0x21e0f
 *
 * A thunk - `ljmp [0x44ea]` - and 0x44ea was measured pointing at the
 * instruction after it, `draw_compressed_body` at 0x20189, so the vector
 * exists to be repointed and not to reach another module. The port calls
 * the body. Assembly, and which module it ends or starts is not settled: the
 * `nop` before it pads the assembly `fill_rect` ends, and the body after it
 * is C (compbmp.c).
 */
void draw_compressed_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    draw_compressed_body(bmp, x, y, mode);
}
#endif
