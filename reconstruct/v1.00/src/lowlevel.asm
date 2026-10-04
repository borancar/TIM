; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `lowlevel.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `lowlevel.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `lowlevel.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs vmds=vmds vm_driver=vmdrv timer=tmr VM_SLOT_*
; JUDGE: assembler bc3.00

_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
_DATA segment word public 'DATA'
joy_a_present label byte
        db 0h
joy_b_present label byte
        db 0h
joy_axis_bits label byte
        db 0h
joy_a_x_centre label byte
        db 0h, 0h
joy_a_y_centre label byte
        db 0h, 0h
joy_b_x_centre label byte
        db 0h, 0h
joy_b_y_centre label byte
        db 0h, 0h
joy_a_x_scale label byte
        db 0h, 0h
joy_a_y_scale label byte
        db 0h, 0h
joy_b_x_scale label byte
        db 0h, 0h
joy_b_y_scale label byte
        db 0h, 0h
joy_read_x label byte
        db 0h, 0h
joy_delay label byte
        db 0h, 0h
joy_dir_x label byte
        db 0h, 0h
joy_dir_y label byte
        db 0h, 0h, 0h
mouse_x label byte
        db 0h, 0h
mouse_y label byte
        db 0h, 0h
mouse_handler_off label byte
        db 0h, 0h
mouse_handler_seg label byte
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h
saved_gc_0_1 label byte
        db 0h, 0h
saved_gc_4 label byte
        db 0h, 0h
saved_gc_8 label byte
        db 0h, 0h
saved_seq_map_mask label byte
        db 0h, 0h
saved_gc_3 label byte
        db 0h, 0h, 0h, 0h
gc_mode_fill label byte
        db 2h
gc_mode_fill_256 label byte
        db 40h
gc_mode_copy label byte
        db 1h
gc_mode_copy_256 label byte
        db 41h
mouse_taken label byte
        db 0h
mouse_buttons label byte
        db 0h
divide_hooked label byte
        db 0h
old_divide_off label byte
        db 0h, 0h
old_divide_seg label byte
        db 0h, 0h, 0h
_DATA ends

extrn _g_vm_driver:byte
extrn _g_timer:byte
extrn _g_vmds:byte
LOWLEVEL_TEXT segment byte public 'CODE'
assume cs:LOWLEVEL_TEXT, ds:DGROUP
public _joy_time_axes, _joy_scale_axis, _joy_init, _joy_read
public _joy_direction, _joy_button, _joy_axis, _clip_and_draw_line
public _mouse_init, _mouse_set_ranges, _mouse_set_user_handler, _mouse_event
public _mouse_save_vga, _mouse_restore_vga, _remove_mouse, _read_mouse_pointer
public _mouse_move_to, _read_mouse_button, _normalise_pointer, _huge_add_positive
public _huge_move, _far_memcpy, _far_memset, _far_ptr_compare
public _normalise_pointer_far, _install_divide_trap, _divide_error_handler, _restore_int0_vector
public _read_pixel_clipped, _plot_pixel_clipped, _restore_rect_thunk

; 0x21b44
_joy_time_axes proc near
        mov dx, 201h
        pushf
        push bp
        cli
        mov bp, word ptr DGROUP:joy_delay
        mov cx, 190h
time_wait_ready:
        mov ax, cx
        mov cx, bp
time_wait_delay:
        loop time_wait_delay
        mov cx, ax
        in al, dx
        test al, bl
        loopne time_wait_ready
        jcxz time_fire
time_fire:
        mov cx, 190h
        xor al, al
        out dx, al
        xor si, si
        mov di, si
        test bl, 3
        je time_count_b
time_count_a:
        mov ax, cx
        mov cx, bp
time_delay_a:
        loop time_delay_a
        mov cx, ax
        in al, dx
        and al, bl
        je time_done
        shr al, 1
        adc si, 0
        shr al, 1
        adc di, 0
        loop time_count_a
time_done:
        pop bp
        popf
        ret
time_count_b:
        mov ax, cx
        mov cx, bp
time_delay_b:
        loop time_delay_b
        mov cx, ax
        in al, dx
        and al, bl
        je time_done
        shr al, 1
        shr al, 1
        shr al, 1
        adc si, 0
        shr al, 1
        adc di, 0
        loop time_count_b
        pop bp
        popf
        ret
_joy_time_axes endp

; 0x21bab
_joy_scale_axis proc near
        push di
        sub ax, bx
        or ax, ax
        jge scale_positive
        mov di, 1
        neg ax
        jmp short scale_sign_set
scale_positive:
        mov di, 0
scale_sign_set:
        or cx, cx
        je scale_scaled
        xor dx, dx
        mul cx
        mov al, ah
        mov ah, dl
scale_scaled:
        cmp ax, 7fh
        jbe scale_clamped
        mov ax, 7fh
scale_clamped:
        cmp al, 8
        jae scale_dead_zone_done
        xor al, al
scale_dead_zone_done:
        xor ah, ah
        test di, 1
        je scale_return
        neg ax
scale_return:
        pop di
        ret
_joy_scale_axis endp

; 0x21be2
_joy_init proc far
        push si
        push di
        cli
        mov al, 36h
        out 43h, al
        xor al, al
        out 40h, al
        out 40h, al
        mov dx, 201h
        mov cx, 3e8h
        out 43h, al
        in al, 40h
        mov ah, al
        in al, 40h
        xchg al, ah
        mov si, ax
init_calibrate:
        mov ax, ax
        mov ax, ax
        nop
        nop
        nop
        nop
        mov ax, ax
        in al, dx
        test al, al
        loop init_calibrate
        mov al, 6
        out 43h, al
        in al, 40h
        mov ah, al
        in al, 40h
        xchg al, ah
        mov di, ax
        mov bx, word ptr DGROUP:_g_timer+tmr_divisor
        mov al, 36h
        out 43h, al
        mov al, bl
        out 40h, al
        mov al, bh
        out 40h, al
        sti
        sub si, di
        xor dx, dx
        mov ax, 6fcch
        div si
        cmp ax, 0
        jne init_delay_set
        mov ax, 1
init_delay_set:
        mov word ptr DGROUP:joy_delay, ax
        mov bx, 3
        call _joy_time_axes
        mov ax, si
        mov word ptr DGROUP:joy_a_x_centre, ax
        cmp ax, 190h
        rcr byte ptr DGROUP:joy_axis_bits, 1
        mov ax, di
        mov word ptr DGROUP:joy_a_y_centre, ax
        cmp ax, 190h
        rcr byte ptr DGROUP:joy_axis_bits, 1
        mov al, byte ptr DGROUP:joy_axis_bits
        and al, 0c0h
        cmp al, 0c0h
        jne init_a_absent
        mov byte ptr DGROUP:joy_a_present, 1
        jmp short init_a_x_scale
init_a_absent:
        mov byte ptr DGROUP:joy_a_present, 0
init_a_x_scale:
        or si, si
        je init_a_y_scale
        mov cx, si
        mov ax, 7f00h
        xor dx, dx
        div cx
        mov word ptr DGROUP:joy_a_x_scale, ax
init_a_y_scale:
        or di, di
        je init_b
        mov cx, di
        mov ax, 7f00h
        xor dx, dx
        div cx
        mov word ptr DGROUP:joy_a_y_scale, ax
init_b:
        mov bx, 0ch
        call _joy_time_axes
        mov ax, si
        cmp ax, 190h
        mov word ptr DGROUP:joy_b_x_centre, ax
        rcr byte ptr DGROUP:joy_axis_bits, 1
        mov ax, di
        mov word ptr DGROUP:joy_b_y_centre, ax
        cmp ax, 190h
        rcr byte ptr DGROUP:joy_axis_bits, 1
        mov al, byte ptr DGROUP:joy_axis_bits
        and al, 0c0h
        cmp al, 0c0h
        jne init_b_absent
        mov byte ptr DGROUP:joy_b_present, 1
        jmp short init_b_x_scale
init_b_absent:
        mov byte ptr DGROUP:joy_b_present, 0
init_b_x_scale:
        or si, si
        je init_b_y_scale
        mov cx, si
        mov ax, 7f00h
        xor dx, dx
        div cx
        mov word ptr DGROUP:joy_b_x_scale, ax
init_b_y_scale:
        or di, di
        je init_check_ports
        mov cx, di
        mov ax, 7f00h
        xor dx, dx
        div cx
        mov word ptr DGROUP:joy_b_y_scale, ax
init_check_ports:
        mov dx, 201h
        out dx, al
        mov cx, 14h
init_settle:
        loop init_settle
        in al, dx
        test al, 3
        jne init_check_b
        mov byte ptr DGROUP:joy_a_present, 0
init_check_b:
        test al, 0ch
        jne init_answer
        mov byte ptr DGROUP:joy_b_present, 0
init_answer:
        mov cl, 4
        shr byte ptr DGROUP:joy_axis_bits, cl
        xor ax, ax
        mov al, byte ptr DGROUP:joy_b_present
        shl al, 1
        or al, byte ptr DGROUP:joy_a_present
        pop di
        pop si
        retf
_joy_init endp

; 0x21d19
_joy_read proc far
        push bp
        mov bp, sp
        push si
        push di
        mov bx, word ptr [bp+6]
        cmp bx, 0
        jne read_b
        xor ah, ah
        mov al, byte ptr DGROUP:joy_a_present
        or al, al
        je read_return
        mov bx, 3
        call _joy_time_axes
        mov bx, word ptr DGROUP:joy_a_x_centre
        mov cx, word ptr DGROUP:joy_a_x_scale
        mov ax, si
        call _joy_scale_axis
        mov word ptr DGROUP:joy_read_x, ax
        mov bx, word ptr DGROUP:joy_a_y_centre
        mov cx, word ptr DGROUP:joy_a_y_scale
        mov ax, di
        call _joy_scale_axis
        jmp short read_store
read_b:
        xor ah, ah
        mov al, byte ptr DGROUP:joy_b_present
        or al, al
        je read_return
        mov bx, 0ch
        call _joy_time_axes
        mov bx, word ptr DGROUP:joy_b_x_centre
        mov cx, word ptr DGROUP:joy_b_x_scale
        mov ax, si
        call _joy_scale_axis
        mov word ptr DGROUP:joy_read_x, ax
        mov bx, word ptr DGROUP:joy_b_y_centre
        mov cx, word ptr DGROUP:joy_b_y_scale
        mov ax, di
        call _joy_scale_axis
read_store:
        mov si, word ptr [bp+0ah]
        mov di, word ptr [bp+8]
        mov word ptr [si], ax
        mov ax, word ptr DGROUP:joy_read_x
        mov word ptr [di], ax
read_return:
        pop di
        pop si
        pop bp
        retf
_joy_read endp

; 0x21d91
_joy_direction proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+6]
        or ax, ax
        je dir_a
        mov al, byte ptr DGROUP:joy_b_present
        jmp short dir_present
dir_a:
        mov al, byte ptr DGROUP:joy_a_present
dir_present:
        or al, al
        je dir_return
        mov ax, 473dh
        push ax
        mov ax, 473bh
        push ax
        mov ax, word ptr [bp+6]
        push ax
        push cs
        call near ptr _joy_read
        add sp, 6
        xor ax, ax
        mov bx, word ptr DGROUP:joy_dir_x
        cmp bx, -1eh
        jge dir_x_not_low
        or ax, 4
        jmp short dir_y
dir_x_not_low:
        cmp bx, 1eh
        jl dir_y
        or ax, 8
dir_y:
        mov bx, word ptr DGROUP:joy_dir_y
        cmp bx, -1eh
        jge dir_y_not_low
        or ax, 1
        jmp short dir_return
dir_y_not_low:
        cmp bx, 1eh
        jl dir_return
        or ax, 2
dir_return:
        pop bp
        retf
_joy_direction endp

; 0x21dea
_joy_button proc far
        push bp
        mov bp, sp
        mov cx, word ptr [bp+6]
        and cx, 3
        mov dx, 201h
        in al, dx
        add cl, 4
        shr al, cl
        and al, 1
        xor ah, ah
        xor al, 1
        pop bp
        retf
_joy_button endp

; 0x21e04
_joy_axis proc far
        push bp
        mov bp, sp
        push si
        push di
        mov bx, 1
        mov cx, word ptr [bp+6]
        shl bx, cl
        call _joy_time_axes
        mov cx, word ptr [bp+6]
        test cl, 1
        je axis_scale
        mov si, di
axis_scale:
        mov di, cx
        shl di, 1
        mov bx, word ptr joy_a_x_centre[di]
        mov cx, word ptr joy_a_x_scale[di]
        mov ax, si
        call _joy_scale_axis
        sti
        pop di
        pop si
        pop bp
        retf
_joy_axis endp

; 0x21e34
_clip_and_draw_line proc far
        push bp
        mov bp, sp
        push si
        push di
        push es
        mov ax, word ptr DGROUP:_g_vmds+vmds_page_dst
        mov es, ax
        mov bx, word ptr [bp+6]
        mov cx, word ptr [bp+8]
        mov si, word ptr [bp+0ah]
        mov di, word ptr [bp+0ch]
        mov al, byte ptr DGROUP:_g_vmds+vmds_clip_enabled
        or al, al
        jne line_top
        jmp line_draw
line_top:
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        cmp cx, ax
        jl line_top_first_out
        cmp di, ax
        jge line_left
        xchg bx, si
        xchg cx, di
        jmp short line_top_cut
line_top_first_out:
        cmp di, ax
        jl line_reject
line_top_cut:
        mov bp, di
        sub bp, cx
        sub ax, cx
        mov cx, ax
        mov ax, si
        sub ax, bx
        imul cx
        idiv bp
        add bx, ax
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov cx, ax
line_left:
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        cmp bx, ax
        jl line_left_first_out
        cmp si, ax
        jge line_bottom
        xchg bx, si
        xchg cx, di
        jmp short line_left_cut
line_left_first_out:
        cmp si, ax
        jge line_left_cut
line_reject:
        jmp line_return
line_left_cut:
        mov bp, si
        sub bp, bx
        sub ax, bx
        mov bx, ax
        mov ax, di
        sub ax, cx
        imul bx
        idiv bp
        add cx, ax
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov bx, ax
line_bottom:
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        cmp cx, ax
        ja line_bottom_first_out
        cmp di, ax
        jbe line_right
        xchg bx, si
        xchg cx, di
        jmp short line_bottom_cut
line_bottom_first_out:
        cmp di, ax
        ja line_reject
line_bottom_cut:
        mov bp, di
        sub bp, cx
        sub ax, cx
        mov cx, ax
        mov ax, si
        sub ax, bx
        imul cx
        idiv bp
        add bx, ax
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov cx, ax
line_right:
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        cmp bx, ax
        ja line_right_first_out
        cmp si, ax
        jbe line_draw
        xchg bx, si
        xchg cx, di
        jmp short line_right_cut
line_right_first_out:
        cmp si, ax
        ja line_reject
line_right_cut:
        mov bp, si
        sub bp, bx
        sub ax, bx
        mov bx, ax
        mov ax, di
        sub ax, cx
        imul bx
        idiv bp
        add cx, ax
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov bx, ax
line_draw:
        mov dx, si
        mov si, di
        cmp bx, dx
        jbe line_ordered
        xchg bx, dx
        xchg cx, si
line_ordered:
        call dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_DRAW_LINE
line_return:
        pop es
        pop di
        pop si
        pop bp
        retf
_clip_and_draw_line endp

; 0x21f1d
_mouse_init proc far
        sub ax, ax
        cmp byte ptr DGROUP:mouse_taken, al
        jne mouse_init_return
        int 33h
        neg ax
        mov byte ptr DGROUP:mouse_taken, al
        jae mouse_init_return
        mov ax, 4
        mov cx, 7fffh
        mov dx, cx
        int 33h
        mov ax, 1
        int 33h
        mov ax, 2
        int 33h
        mov ax, 0fh
        mov cx, 8
        mov dx, 8
        int 33h
        mov ax, 4
        xor cx, cx
        mov dx, cx
        int 33h
        push word ptr DGROUP:_g_vmds+vmds_screen+vm_screen_screen_height
        push word ptr DGROUP:_g_vmds+vmds_screen+vm_screen_screen_width
        xor ax, ax
        push ax
        push ax
        push cs
        call near ptr _mouse_set_ranges
        add sp, 8
        mov ax, 0ch
        mov cx, 1fh
        push cs
        pop es
        mov dx, 5d7fh
        int 33h
        mov al, byte ptr DGROUP:_g_vmds+vmds_pixel_shift
        cmp al, 8
        jne mouse_init_modes_set
        mov al, byte ptr DGROUP:gc_mode_fill_256
        mov byte ptr DGROUP:gc_mode_fill, al
        mov al, byte ptr DGROUP:gc_mode_copy_256
        mov byte ptr DGROUP:gc_mode_copy, al
mouse_init_modes_set:
        mov ax, 1
mouse_init_return:
        retf
_mouse_init endp

; 0x21f8d
_mouse_set_ranges proc far
        push bp
        mov bp, sp
        mov ax, 7
        mov cx, word ptr [bp+6]
        mov dx, cx
        add dx, word ptr [bp+0ah]
        shl cx, 1
        shl cx, 1
        dec dx
        shl dx, 1
        shl dx, 1
        int 33h
        mov ax, 8
        mov cx, word ptr [bp+8]
        mov dx, cx
        add dx, word ptr [bp+0ch]
        shl cx, 1
        shl cx, 1
        dec dx
        shl dx, 1
        shl dx, 1
        int 33h
        pop bp
        retf
_mouse_set_ranges endp

; 0x21fbe
_mouse_set_user_handler proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+6]
        mov word ptr DGROUP:mouse_handler_off, ax
        mov ax, word ptr [bp+8]
        mov word ptr DGROUP:mouse_handler_seg, ax
        pop bp
        retf
_mouse_set_user_handler endp

; 0x21fcf
_mouse_event proc far
        push ds
        push es
        push bp
        push si
        mov bp, sp
        mov si, ss
        mov ax, DGROUP
        cli
        mov sp, 48d8h
        mov ss, ax
        sti
        mov ds, ax
        mov byte ptr DGROUP:mouse_buttons, bl
        mov word ptr DGROUP:mouse_x, cx
        mov word ptr DGROUP:mouse_y, dx
        mov ax, word ptr DGROUP:mouse_handler_off
        or ax, word ptr DGROUP:mouse_handler_seg
        je mouse_event_unstack
        push cs
        call near ptr _mouse_save_vga
        call dword ptr DGROUP:mouse_handler_off
        push cs
        call near ptr _mouse_restore_vga
mouse_event_unstack:
        cli
        mov ss, si
        mov sp, bp
        sti
        pop si
        pop bp
        pop es
        pop ds
        retf
_mouse_event endp

; 0x2200f
_mouse_save_vga proc far
        mov dx, 3ceh
        in al, dx
        mov ah, al
        xor al, al
        out dx, al
        inc dx
        in al, dx
        mov word ptr DGROUP:saved_gc_0_1, ax
        dec dx
        mov al, 1
        out dx, al
        inc dx
        in al, dx
        mov ah, al
        xor al, al
        out dx, al
        dec dx
        mov al, 4
        out dx, al
        inc dx
        in al, dx
        mov word ptr DGROUP:saved_gc_4, ax
        dec dx
        mov al, 5
        out dx, al
        inc dx
        in al, dx
        mov ah, al
        mov al, byte ptr DGROUP:gc_mode_copy
        out dx, al
        mov di, 0ffffh
        mov bx, 0a000h
        mov es, bx
        stosb
        mov al, byte ptr DGROUP:gc_mode_fill
        out dx, al
        dec dx
        mov al, 8
        out dx, al
        inc dx
        in al, dx
        mov word ptr DGROUP:saved_gc_8, ax
        mov al, 0ffh
        out dx, al
        dec dx
        mov al, 3
        out dx, al
        inc dx
        in al, dx
        mov byte ptr DGROUP:saved_gc_3, al
        xor al, al
        out dx, al
        mov dx, 3c4h
        in al, dx
        mov ah, al
        mov al, 2
        out dx, al
        inc dx
        in al, dx
        mov word ptr DGROUP:saved_seq_map_mask, ax
        mov al, 0fh
        out dx, al
        retf
_mouse_save_vga endp

; 0x22074
_mouse_restore_vga proc far
        mov dx, 3c4h
        mov al, 2
        out dx, al
        mov ax, word ptr DGROUP:saved_seq_map_mask
        inc dx
        out dx, al
        dec dx
        mov al, ah
        out dx, al
        mov dx, 3ceh
        mov al, 3
        out dx, al
        mov al, byte ptr DGROUP:saved_gc_3
        inc dx
        out dx, al
        dec dx
        mov al, 8
        out dx, al
        mov ax, word ptr DGROUP:saved_gc_8
        inc dx
        out dx, al
        dec dx
        mov al, 5
        out dx, al
        inc dx
        mov al, byte ptr DGROUP:gc_mode_copy
        out dx, al
        mov di, 0ffffh
        mov bx, 0a000h
        mov es, bx
        mov al, byte ptr es:[di]
        mov al, ah
        out dx, al
        dec dx
        mov al, 4
        out dx, al
        mov ax, word ptr DGROUP:saved_gc_4
        inc dx
        out dx, al
        dec dx
        mov al, 1
        out dx, al
        mov al, ah
        inc dx
        out dx, al
        dec dx
        xor al, al
        out dx, al
        mov ax, word ptr DGROUP:saved_gc_0_1
        inc dx
        out dx, al
        dec dx
        mov al, ah
        out dx, al
        retf
_mouse_restore_vga endp

; 0x220cd
_remove_mouse proc far
        xor ax, ax
        cmp byte ptr DGROUP:mouse_taken, al
        je remove_mouse_return
        mov byte ptr DGROUP:mouse_taken, al
        int 33h
        mov ax, 0ch
        xor cx, cx
        mov dx, cx
        mov es, cx
        int 33h
        mov ax, 1
remove_mouse_return:
        retf
_remove_mouse endp

; 0x220e9
_read_mouse_pointer proc far
        push bp
        mov bp, sp
        sub cx, cx
        mov dx, cx
        mov ax, cx
        mov al, byte ptr DGROUP:mouse_taken
        neg ax
        jae read_pointer_return
        mov bx, word ptr [bp+6]
        mov ax, word ptr DGROUP:mouse_x
        shr ax, 1
        shr ax, 1
        mov word ptr [bx], ax
        mov bx, word ptr [bp+8]
        mov ax, word ptr DGROUP:mouse_y
        shr ax, 1
        shr ax, 1
        mov word ptr [bx], ax
read_pointer_return:
        pop bp
        retf
_read_mouse_pointer endp

; 0x22113
_mouse_move_to proc far
        push bp
        mov bp, sp
        mov al, byte ptr DGROUP:mouse_taken
        neg al
        jae move_to_return
        mov ax, 4
        mov cx, word ptr [bp+6]
        shl cx, 1
        shl cx, 1
        mov word ptr DGROUP:mouse_x, cx
        mov dx, word ptr [bp+8]
        shl dx, 1
        shl dx, 1
        mov word ptr DGROUP:mouse_y, dx
        int 33h
        mov al, 1
move_to_return:
        mov ah, 0
        pop bp
        retf
_mouse_move_to endp

; 0x2213e
_read_mouse_button proc far
        push bp
        mov bp, sp
        sub bx, bx
        mov bl, byte ptr DGROUP:mouse_taken
        neg bx
        jae read_button_answer
        mov bl, byte ptr DGROUP:mouse_buttons
        xor bh, bh
        mov ax, word ptr [bp+6]
        neg ax
        jae read_button_answer
        shr bx, 1
read_button_answer:
        mov ax, bx
        and ax, 1
        pop bp
        retf
_read_mouse_button endp

; 0x22161
_normalise_pointer proc near
        push cx
        mov cx, ax
        and ax, 0fh
        shr cx, 1
        shr cx, 1
        shr cx, 1
        shr cx, 1
        add dx, cx
        pop cx
        ret
normalise_back db 0e8h, 0ebh, 0ffh
normalise_back_3 db 0f6h, 0c6h, 0f0h
normalise_back_6 db 74h, 8h
normalise_back_8 db 5h, 0f0h, 0ffh
normalise_back_b db 81h, 0eah, 0ffh, 0fh
normalise_back_f db 0c3h
normalise_back_low:
        shl dx, 1
        shl dx, 1
        shl dx, 1
        shl dx, 1
        add ax, dx
        sub dx, dx
        ret
_normalise_pointer endp

; 0x22190
_huge_add_positive proc near
        add ax, bx
        sbb bx, bx
        and bx, 1000h
        add dx, bx
        mov bx, cx
        mov cl, 5
        clc
        rcr bx, cl
        add dx, bx
        ret
unreached_huge_sub db 0f7h, 0dbh
unreached_huge_sub_2 db 3h, 0c3h
unreached_huge_sub_4 db 1bh, 0dbh
unreached_huge_sub_6 db 0f7h, 0d3h
unreached_huge_sub_8 db 81h, 0e3h, 0h, 10h
unreached_huge_sub_c db 2bh, 0d3h
unreached_huge_sub_e db 0e8h, 0ach, 0ffh
unreached_huge_sub_11 db 8bh, 0d9h
unreached_huge_sub_13 db 0b1h, 5h
unreached_huge_sub_15 db 0f8h
unreached_huge_sub_16 db 0d3h, 0dbh
unreached_huge_sub_18 db 2bh, 0d3h
unreached_huge_sub_1a db 0c3h
copy_back db 0e3h, 12h
copy_back_2 db 0f7h, 0c7h, 1h, 0h
copy_back_6 db 75h, 2h
copy_back_8 db 0a4h
copy_back_9 db 49h
copy_back_words:
        dec si
        dec di
        shr cx, 1
        rep movsw
        rcl cx, 1
        inc si
        inc di
copy_back_bytes:
        rep movsb
        ret
copy_fwd db 0e3h, 10h
copy_fwd_2 db 0f7h, 0c7h, 1h, 0h
copy_fwd_6 db 74h, 2h
copy_fwd_8 db 0a4h
copy_fwd_9 db 49h
copy_fwd_words:
        shr cx, 1
        rep movsw
        rcl cx, 1
        rep movsb
copy_fwd_done:
        ret
move_step_fn db 0h, 0h
move_copy_fn db 0h, 0h
_huge_add_positive endp

; 0x221ed
_huge_move proc far
        push bp
        mov bp, sp
        sub sp, 4
        push si
        push di
        push ds
        mov word ptr cs:move_step_fn, 5f11h
        mov word ptr cs:move_copy_fn, 5f86h
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        mov word ptr [bp-4], ax
        mov word ptr [bp-2], dx
        call _normalise_pointer
        mov word ptr [bp+6], ax
        mov word ptr [bp+8], dx
        mov ax, word ptr [bp+0ah]
        mov dx, word ptr [bp+0ch]
        call _normalise_pointer
        cmp dx, word ptr [bp+8]
        ja move_set_source
        jb move_backward
        cmp ax, word ptr [bp+6]
        ja move_set_source
        jb move_backward
        jmp move_return
move_backward:
        std
        mov word ptr cs:move_step_fn, 5f23h
        mov word ptr cs:move_copy_fn, 5f6fh
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        mov bx, word ptr [bp+0eh]
        mov cx, word ptr [bp+10h]
        sub bx, 1
        sbb cx, 0
        js move_return
        push cx
        push bx
        call _huge_add_positive
        mov word ptr [bp+6], ax
        mov word ptr [bp+8], dx
        mov ax, word ptr [bp+0ah]
        mov dx, word ptr [bp+0ch]
        pop bx
        pop cx
        call _huge_add_positive
move_set_source:
        mov ds, dx
        mov si, ax
        mov es, word ptr [bp+8]
        mov di, word ptr [bp+6]
move_chunk:
        sub cx, cx
        mov bx, 7d00h
        cmp word ptr [bp+10h], cx
        jne move_chunk_sized
        mov ax, word ptr [bp+0eh]
        cmp ax, cx
        je move_return
        cmp ax, bx
        jg move_chunk_sized
        mov bx, ax
move_chunk_sized:
        mov cx, bx
        mov ax, si
        mov dx, ds
        call word ptr cs:move_step_fn
        mov si, ax
        mov ds, dx
        mov ax, di
        mov dx, es
        call word ptr cs:move_step_fn
        mov di, ax
        mov es, dx
        call word ptr cs:move_copy_fn
        sub ax, ax
        sub word ptr [bp+0eh], bx
        sbb word ptr [bp+10h], ax
        jmp short move_chunk
move_return:
        mov dx, word ptr [bp-2]
        mov ax, word ptr [bp-4]
        cld
        pop ds
        pop di
        pop si
        mov sp, bp
        pop bp
        retf
_huge_move endp

; 0x222c6
_far_memcpy proc far
        push bp
        mov bp, sp
        mov cx, word ptr [bp+0eh]
        jcxz memcpy_return
        push si
        push di
        push ds
        mov ax, word ptr [bp+0ah]
        mov dx, word ptr [bp+0ch]
        call _normalise_pointer
        mov ds, dx
        mov si, ax
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        call _normalise_pointer
        mov es, dx
        mov di, ax
        test di, 1
        jae memcpy_even
        movsb
        dec cx
memcpy_even:
        shr cx, 1
        rep movsw
        rcl cx, 1
        rep movsb
        pop ds
        pop di
        pop si
memcpy_return:
        pop bp
        retf
_far_memcpy endp

; 0x22300
_far_memset proc far
        push bp
        mov bp, sp
        push si
        push di
        cld
        mov di, word ptr [bp+6]
        mov ax, word ptr [bp+8]
        mov es, ax
        mov ax, word ptr [bp+0ah]
        mov ah, al
        mov si, ax
memset_chunk:
        sub bx, bx
        mov cx, 7d00h
        cmp word ptr [bp+0eh], bx
        jne memset_chunk_sized
        mov ax, word ptr [bp+0ch]
        cmp ax, bx
        je memset_return
        cmp ax, cx
        jg memset_chunk_sized
        mov cx, ax
memset_chunk_sized:
        mov bx, cx
        mov ax, di
        mov dx, es
        call _normalise_pointer
        mov di, ax
        mov es, dx
        mov ax, si
        cmp cx, 0ah
        jl memset_bytes
        or di, di
        jp memset_aligned
        stosb
        dec cx
memset_aligned:
        shr cx, 1
        rep stosw
        rcl cl, 1
memset_bytes:
        rep stosb
        sub word ptr [bp+0ch], bx
        sbb word ptr [bp+0eh], cx
        jmp short memset_chunk
memset_return:
        pop di
        pop si
        pop bp
        retf
_far_memset endp

; 0x2235a
_far_ptr_compare proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        call _normalise_pointer
        mov bx, ax
        mov cx, dx
        mov ax, word ptr [bp+0ah]
        mov dx, word ptr [bp+0ch]
        call _normalise_pointer
        cmp cx, dx
        jb compare_not_above
        ja compare_above
        cmp bx, ax
        ja compare_above
compare_not_above:
        sbb ax, ax
        pop bp
        retf
compare_above:
        mov ax, 1
        pop bp
        retf
_far_ptr_compare endp

; 0x22386
_normalise_pointer_far proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+6]
        mov dx, word ptr [bp+8]
        call _normalise_pointer
        pop bp
        retf
_normalise_pointer_far endp

; 0x22394
_install_divide_trap proc far
        push ax
        push es
        mov byte ptr DGROUP:divide_hooked, 1
        sub ax, ax
        mov es, ax
        mov ax, word ptr es:[0]
        mov word ptr DGROUP:old_divide_seg, ax
        mov ax, word ptr es:[2]
        mov word ptr DGROUP:old_divide_off, ax
        cli
        mov word ptr es:[0], offset _divide_error_handler
        mov word ptr es:[2], cs
        sti
        pop es
        pop ax
        retf
_install_divide_trap endp

; 0x223be
_divide_error_handler proc near
        push bp
        mov bp, sp
        push bx
        push es
        push ax
        mov ax, word ptr [bp+2]
        mov bx, ax
        mov ax, word ptr [bp+4]
        mov es, ax
        mov ax, word ptr es:[bx]
        and ax, 0feh
        cmp ax, 0f6h
        je divide_have_opcode
        sub bx, 2
        mov ax, bx
        mov word ptr [bp+2], ax
divide_have_opcode:
        mov ax, word ptr es:[bx]
        test ax, 1
        je divide_byte
        pop ax
        sar dx, 1
        rcr ax, 1
        jmp short divide_return
divide_byte:
        pop ax
        sar ax, 1
divide_return:
        pop es
        pop bx
        pop bp
        iret
_divide_error_handler endp

; 0x223f7
_restore_int0_vector proc far
        push ax
        push es
        xor ax, ax
        cmp byte ptr DGROUP:divide_hooked, al
        je restore_int0_return
        mov byte ptr DGROUP:divide_hooked, al
        sub ax, ax
        mov es, ax
        cli
        mov ax, word ptr DGROUP:old_divide_seg
        mov ax, word ptr es:[0]
        mov ax, word ptr DGROUP:old_divide_off
        mov ax, word ptr es:[2]
        sti
restore_int0_return:
        pop es
        pop ax
        retf
_restore_int0_vector endp

; 0x2241b
_read_pixel_clipped proc far
        push bp
        mov bp, sp
        cmp byte ptr DGROUP:_g_vmds+vmds_clip_enabled, 0
        je read_pixel_inside
        mov ax, word ptr [bp+6]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        jl read_pixel_outside
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        jg read_pixel_outside
        mov ax, word ptr [bp+8]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        jl read_pixel_outside
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        jg read_pixel_outside
read_pixel_inside:
        pop bp
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_READ_PIXEL
read_pixel_outside:
        pop bp
        mov ax, 0ffffh
        retf
_read_pixel_clipped endp

; 0x2244d
_plot_pixel_clipped proc far
        push bp
        mov bp, sp
        cmp byte ptr DGROUP:_g_vmds+vmds_clip_enabled, 0
        je plot_pixel_inside
        mov ax, word ptr [bp+6]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        jl plot_pixel_outside
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        jg plot_pixel_outside
        mov ax, word ptr [bp+8]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        jl plot_pixel_outside
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        jg plot_pixel_outside
plot_pixel_inside:
        pop bp
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_PLOT_PIXEL
plot_pixel_outside:
        pop bp
        mov ax, 0ffffh
        retf
_plot_pixel_clipped endp

; 0x2247f
_restore_rect_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_RESTORE_RECT
_restore_rect_thunk endp
LOWLEVEL_TEXT ends
end
