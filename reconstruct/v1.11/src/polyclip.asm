; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `polyclip.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `polyclip.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `polyclip.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs vmds=vmds
; JUDGE: assembler bc3.00

_DATA segment word public 'DATA'
_DATA ends
_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
extrn _g_vmds:byte
POLYCLIP_TEXT segment byte public 'CODE'
assume cs:POLYCLIP_TEXT, ds:DGROUP
public _detect_pcjr, _clip_polygon

; 0x2286a
_detect_pcjr proc far
        push es
        push bx
        mov bx, 0f000h
        mov es, bx
        mov bx, 0fffeh
        mov al, byte ptr es:[bx]
        cmp al, 0ffh
        jne pcjr_answer
        mov bx, 0c000h
        mov al, byte ptr es:[bx]
        cmp al, 21h
        jne pcjr_answer
        mov byte ptr DGROUP:_g_vmds+vmds_is_pcjr, 1
pcjr_answer:
        mov al, byte ptr DGROUP:_g_vmds+vmds_is_pcjr
        cbw
        pop bx
        pop es
        retf
_detect_pcjr endp

; 0x22891
_clip_polygon proc far
        xor di, di
        mov ax, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        cmp ax, 1
        jg x_begin
        jmp clip_return
x_begin:
        dec ax
        mov bx, ax
        shl bx, 1
        xor cl, cl
        mov ax, word ptr _g_vmds[bx+vmds_poly_x]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        jge x_prev_coded
        or cl, 1
x_prev_coded:
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        jle x_start
        or cl, 2
x_start:
        xor si, si
x_edge:
        shl si, 1
        xor ch, ch
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        jge x_cur_coded
        or ch, 1
x_cur_coded:
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        jle x_classify
        or ch, 2
x_classify:
        mov al, cl
        or al, ch
        jne x_crossing
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        mov word ptr _g_vmds[di+vmds_work_x], ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp x_next
x_crossing:
        mov al, cl
        and al, ch
        je x_prev_in
        jmp x_next
x_prev_in:
        or cl, cl
        jne x_prev_out
        test ch, 1
        je x_leave_right
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[si+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, word ptr _g_vmds[si+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_poly_x]
        sub bp, word ptr _g_vmds[si+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp short x_left
x_leave_right:
        test ch, 2
        je x_left
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[si+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, word ptr _g_vmds[si+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_poly_x]
        sub bp, word ptr _g_vmds[si+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
x_left:
        jmp x_next
x_prev_out:
        or ch, ch
        je x_enter
        jmp short x_both_out
x_enter:
        test cl, 1
        je x_enter_right
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[bx+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        sub ax, word ptr _g_vmds[bx+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
        sub bp, word ptr _g_vmds[bx+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp short x_enter_keep
x_enter_right:
        test cl, 2
        je x_enter_keep
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[bx+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        sub ax, word ptr _g_vmds[bx+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
        sub bp, word ptr _g_vmds[bx+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
x_enter_keep:
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        mov word ptr _g_vmds[di+vmds_work_x], ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp x_next
x_both_out:
        test cl, 1
        je x_cross_from_right
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[bx+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        sub ax, word ptr _g_vmds[bx+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
        sub bp, word ptr _g_vmds[bx+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp short x_cross_to
x_cross_from_right:
        test cl, 2
        je x_cross_to
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[bx+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        sub ax, word ptr _g_vmds[bx+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
        sub bp, word ptr _g_vmds[bx+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
x_cross_to:
        test ch, 1
        je x_cross_to_right
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[si+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, word ptr _g_vmds[si+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_poly_x]
        sub bp, word ptr _g_vmds[si+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp short x_next
x_cross_to_right:
        test ch, 2
        je x_next
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[si+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, word ptr _g_vmds[si+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_poly_x]
        sub bp, word ptr _g_vmds[si+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
x_next:
        mov bx, si
        mov cl, ch
        shr si, 1
        inc si
        cmp si, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        je x_done
        jmp x_edge
x_done:
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count, ax
        cmp ax, 1
        jg y_begin
        jmp clip_copy_back
y_begin:
        dec ax
        mov bx, ax
        shl bx, 1
        xor cl, cl
        mov ax, word ptr _g_vmds[bx+vmds_work_y]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        jle y_prev_coded
        or cl, 4
y_prev_coded:
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        jge y_start
        or cl, 8
y_start:
        xor di, di
        mov si, di
y_edge:
        shl si, 1
        xor ch, ch
        mov ax, word ptr _g_vmds[si+vmds_work_y]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        jle y_cur_coded
        or ch, 4
y_cur_coded:
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        jge y_classify
        or ch, 8
y_classify:
        mov al, cl
        or al, ch
        jne y_crossing
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        mov ax, word ptr _g_vmds[si+vmds_work_y]
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        add di, 2
        jmp y_next
y_crossing:
        mov al, cl
        and al, ch
        je y_prev_in
        jmp y_next
y_prev_in:
        or cl, cl
        jne y_prev_out
        test ch, 8
        je y_leave_bottom
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[si+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
        jmp short y_left
y_leave_bottom:
        test ch, 4
        je y_left
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[si+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
y_left:
        jmp y_next
y_prev_out:
        or ch, ch
        je y_enter
        jmp short y_both_out
y_enter:
        test cl, 8
        je y_enter_bottom
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[bx+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        sub ax, word ptr _g_vmds[bx+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_work_y]
        sub bp, word ptr _g_vmds[bx+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
        jmp short y_enter_keep
y_enter_bottom:
        test cl, 4
        je y_enter_keep
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[bx+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        sub ax, word ptr _g_vmds[bx+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_work_y]
        sub bp, word ptr _g_vmds[bx+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
y_enter_keep:
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        mov ax, word ptr _g_vmds[si+vmds_work_y]
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        add di, 2
        jmp y_next
y_both_out:
        test cl, 8
        je y_cross_from_bottom
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[bx+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        sub ax, word ptr _g_vmds[bx+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_work_y]
        sub bp, word ptr _g_vmds[bx+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
        jmp short y_cross_to
y_cross_from_bottom:
        test cl, 4
        je y_cross_to
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[bx+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        sub ax, word ptr _g_vmds[bx+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_work_y]
        sub bp, word ptr _g_vmds[bx+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
y_cross_to:
        test ch, 8
        je y_cross_to_bottom
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[si+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
        jmp short y_next
y_cross_to_bottom:
        test ch, 4
        je y_next
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[si+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
y_next:
        mov bx, si
        mov cl, ch
        shr si, 1
        inc si
        cmp si, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        je y_done
        jmp y_edge
y_done:
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count, ax
        jmp short clip_return
clip_copy_back:
        mov ax, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        mov cx, ax
        mov si, offset DGROUP:_g_vmds+vmds_work_x
        mov di, offset DGROUP:_g_vmds+vmds_poly_x
        rep movsw
        mov cx, ax
        mov si, offset DGROUP:_g_vmds+vmds_work_y
        mov di, offset DGROUP:_g_vmds+vmds_poly_y
        rep movsw
clip_return:
        retf
_clip_polygon endp
POLYCLIP_TEXT ends
end
