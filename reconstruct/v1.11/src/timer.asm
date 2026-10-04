; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `timer.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `timer.c` is the host's transcription of it.
;
; The module as TASM assembled it; the host's transcription is `timer.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs vm_driver=vmdrv VM_SLOT_*
; JUDGE: assembler bc3.00

_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
_DATA segment word public 'DATA'
public _g_timer
_g_timer label byte
timer_installed label byte
        db 0h
timer_frame_budget label byte
        db 0h, 0h
timer_divisor label byte
        db 0ffh, 0ffh
timer_divider_reload label byte
        db 0h, 0h
timer_divider label byte
        db 0h, 0h
timer_slot_mask label byte
        db 0h, 0h
timer_callback_off label byte
        db 0h, 0h
timer_callback_seg label byte
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
timer_left label byte
        db 0h, 0h
timer_period label byte
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
_DATA ends

extrn _detect_pcjr:far
extrn _g_vm_driver:byte

TIMER_TEXT segment byte public 'CODE'
assume cs:TIMER_TEXT, ds:DGROUP
public _timer_add_callback, _timer_drop_callback, _timer_install, _timer_remove
public _timer_tick, _blit_rows_thunk, _blit_rows_alt_thunk

; 0x222de
_timer_add_callback proc far
        push bp
        mov bp, sp
        sub ax, ax
        cmp byte ptr DGROUP:timer_installed, al
        je add_return
        mov ax, word ptr DGROUP:timer_slot_mask
        inc ax
        je add_return
        dec ax
        sub bx, bx
        mov cx, 1
add_find_slot:
        shr ax, 1
        jae add_fill
        shl cx, 1
        add bl, 4
        jmp short add_find_slot
add_fill:
        mov ax, word ptr [bp+0ah]
        mov word ptr timer_period[bx], ax
        mov word ptr timer_left[bx], ax
        mov ax, word ptr [bp+6]
        mov word ptr timer_callback_off[bx], ax
        mov ax, word ptr [bp+8]
        mov word ptr timer_callback_seg[bx], ax
        cli
        or word ptr DGROUP:timer_slot_mask, cx
        sti
        mov ax, bx
        shr ax, 1
        shr ax, 1
        inc ax
add_return:
        pop bp
        retf
_timer_add_callback endp

; 0x22328
_timer_drop_callback proc far
        push bp
        mov bp, sp
        sub ax, ax
        mov cx, word ptr [bp+6]
        dec cx
        test cl, 0f0h
        jne drop_return
        stc
        mov ax, 0fffeh
        rcl ax, cl
        cli
        and word ptr DGROUP:timer_slot_mask, ax
        sti
        mov ax, 1
drop_return:
        pop bp
        retf
old_int8_off db 0h, 0h
old_int8_seg db 0h, 0h
_timer_drop_callback endp

; 0x2234b
_timer_install proc far
        push bp
        mov bp, sp
        sub ax, ax
        cmp byte ptr DGROUP:timer_installed, al
        jne install_return
        mov word ptr DGROUP:timer_slot_mask, ax
        call FAR PTR _detect_pcjr
        mov ax, 3508h
        int 21h
        mov word ptr cs:old_int8_off, bx
        mov word ptr cs:old_int8_seg, es
        sub ax, ax
        mov dx, ax
        mov bx, word ptr [bp+6]
        cmp bx, 0ffh
        jg install_return
        or bx, bx
        je install_return
        dec ax
        mov word ptr DGROUP:timer_divider_reload, bx
        mov word ptr DGROUP:timer_divider, bx
        div bx
        mov bx, ax
        mov word ptr DGROUP:timer_divisor, ax
        cli
        mov al, 36h
        out 43h, al
        mov al, bl
        out 40h, al
        mov al, bh
        out 40h, al
        in al, 21h
        and al, 0fch
        out 21h, al
        push ds
        mov ax, cs
        mov ds, ax
        mov dx, offset _timer_tick
        mov ax, 2508h
        int 21h
        pop ds
        sti
        mov ax, 1
        mov byte ptr DGROUP:timer_installed, al
install_return:
        pop bp
        retf
_timer_install endp

; 0x223b8
_timer_remove proc far
        mov ax, 0
        cmp byte ptr DGROUP:timer_installed, al
        je remove_return
        sub bx, bx
        cli
        mov al, 36h
        out 43h, al
        mov al, bl
        out 40h, al
        mov al, bh
        out 40h, al
        in al, 21h
        and al, 0fch
        out 21h, al
        push ds
        mov dx, word ptr cs:old_int8_off
        mov ds, word ptr cs:old_int8_seg
        mov ax, 2508h
        int 21h
        pop ds
        mov ax, 1
        mov byte ptr DGROUP:timer_installed, 0
        sti
remove_return:
        retf
_timer_remove endp

; 0x223f1
_timer_tick proc near
        push ax
        push bx
        push cx
        push dx
        push bp
        push ds
        push es
        push si
        push di
        mov ax, DGROUP
        mov ds, ax
        mov ax, word ptr DGROUP:timer_frame_budget
        dec ax
        cwd
        xor ax, dx
        mov word ptr DGROUP:timer_frame_budget, ax
        sub si, si
        mov di, word ptr DGROUP:timer_slot_mask
        mov bp, offset DGROUP:timer_left
tick_slots:
        shr di, 1
        ja tick_slot_b
        jae tick_slots_done
        mov ax, word ptr ds:[bp+si]
        dec ax
        jne tick_store_a
        call dword ptr timer_callback_off[si]
        mov ax, word ptr timer_period[si]
tick_store_a:
        mov word ptr ds:[bp+si], ax
tick_slot_b:
        add si, 4
        shr di, 1
        ja tick_slot_c
        jae tick_slots_done
        mov ax, word ptr ds:[bp+si]
        dec ax
        jne tick_store_b
        call dword ptr timer_callback_off[si]
        mov ax, word ptr timer_period[si]
tick_store_b:
        mov word ptr ds:[bp+si], ax
tick_slot_c:
        add si, 4
        shr di, 1
        ja tick_slot_d
        jae tick_slots_done
        mov ax, word ptr ds:[bp+si]
        dec ax
        jne tick_store_c
        call dword ptr timer_callback_off[si]
        mov ax, word ptr timer_period[si]
tick_store_c:
        mov word ptr ds:[bp+si], ax
tick_slot_d:
        add si, 4
        shr di, 1
        ja tick_slot_e
        jae tick_slots_done
        mov ax, word ptr ds:[bp+si]
        dec ax
        jne tick_store_d
        call dword ptr timer_callback_off[si]
        mov ax, word ptr timer_period[si]
tick_store_d:
        mov word ptr ds:[bp+si], ax
tick_slot_e:
        add si, 4
        shr di, 1
        ja tick_next_five
        jae tick_slots_done
        mov ax, word ptr ds:[bp+si]
        dec ax
        jne tick_store_e
        call dword ptr timer_callback_off[si]
        mov ax, word ptr timer_period[si]
tick_store_e:
        mov word ptr ds:[bp+si], ax
tick_next_five:
        add si, 4
        jmp tick_slots
tick_slots_done:
        mov ax, word ptr DGROUP:timer_divider
        dec ax
        je tick_chain_bios
        mov word ptr DGROUP:timer_divider, ax
        pop di
        pop si
        pop es
        pop ds
        pop bp
        pop dx
        pop cx
        pop bx
        mov al, 20h
        out 20h, al
        pop ax
        iret
tick_chain_bios:
        mov ax, word ptr DGROUP:timer_divider_reload
        mov word ptr DGROUP:timer_divider, ax
        pop di
        pop si
        pop es
        pop ds
        pop bp
        pop dx
        pop cx
        pop bx
        pop ax
        jmp dword ptr cs:old_int8_off
_timer_tick endp

; 0x224c2
_blit_rows_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_BLIT_ROWS
_blit_rows_thunk endp

; 0x224c6
_blit_rows_alt_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_BLIT_ROWS_ALT
_blit_rows_alt_thunk endp
TIMER_TEXT ends
end
