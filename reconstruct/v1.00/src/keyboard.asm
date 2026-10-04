; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `keyboard.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `keyboard.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `keyboard.c`.
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
kbd_installed label byte
        db 0h
kbd_hold_caps_lock label byte
        db 0h
kbd_last_event label byte
        db 0h, 0h
kbd_held label byte
        db 0h, 0h, 1h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
kbd_ascii label byte
        db 0h, 1bh, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 30h, 2dh, 3dh, 8h, 9h
        db 71h, 77h, 65h, 72h, 74h, 79h, 75h, 69h, 6fh, 70h, 5bh, 5dh, 0dh, 84h, 61h, 73h
        db 64h, 66h, 67h, 68h, 6ah, 6bh, 6ch, 3bh, 27h, 60h, 82h, 5ch, 7ah, 78h, 63h, 76h
        db 62h, 6eh, 6dh, 2ch, 2eh, 2fh, 81h, 2ah, 88h, 20h, 0c0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
kbd_shifted label byte
        db 0h, 1bh, 21h, 40h, 23h, 24h, 25h, 5eh, 26h, 2ah, 28h, 29h, 5fh, 2bh, 8h, 0h
        db 51h, 57h, 45h, 52h, 54h, 59h, 55h, 49h, 4fh, 50h, 7bh, 7dh, 0dh, 84h, 41h, 53h
        db 44h, 46h, 47h, 48h, 4ah, 4bh, 4ch, 3ah, 22h, 7eh, 82h, 7ch, 5ah, 58h, 43h, 56h
        db 42h, 4eh, 4dh, 3ch, 3eh, 3fh, 81h, 0h, 88h, 20h, 0c0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 37h, 38h, 39h, 2dh, 34h, 35h, 36h, 2bh, 31h
        db 32h, 33h, 30h, 2eh, 0h, 0h, 0h, 0h, 0h
kbd_state label byte
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 6eh, 7eh, 8eh, 6h, 6h, 6h, 6h, 6h, 6h, 6h, 2h, 2h, 0h, 0h, 5eh, 9eh
        db 1eh, 6h, 6h, 6h, 6h, 6h, 6h, 0h, 0h, 0h, 0h, 2h, 4eh, 3eh, 2eh, 6h
        db 6h, 6h, 6h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
        db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 60h, 70h, 80h, 0h, 50h, 90h, 10h, 0h, 40h
        db 30h, 20h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
kjoy_dx label byte
        db 0h, 0h, 0h, 0h
kjoy_dy label byte
        db 0h, 0h, 0h, 0h
kjoy_enabled label byte
        db 1h, 0h, 1h, 0h, 3h, 0h
kjoy_dirs label byte
        db 80h, 0a0h, 40h, 0e0h, 0c0h, 0f0h, 60h, 0b0h, 0h, 8h, 0ah, 2h, 6h, 4h, 5h, 1h
        db 9h, 0h
kbd_pcjr_from label byte
        db 59h, 5ah, 55h, 54h, 29h, 58h, 2bh, 4ah, 4eh, 56h, 57h, 57h, 58h, 52h, 46h, 48h
        db 47h, 4bh, 50h, 4dh, 53h, 1ch
kbd_pcjr label byte
        db 0h
pad_471c label byte
        db 0h, 0h
_DATA ends

extrn _detect_pcjr:far
extrn _game_teardown:far
extrn _g_vm_driver:byte
extrn _g_vmds:byte
KEYBOARD_TEXT segment byte public 'CODE'
assume cs:KEYBOARD_TEXT, ds:DGROUP
public _copy_rect_thunk, _install_keyboard, _remove_keyboard, _keyboard_isr
public _bios_read_key, _key_is_down, _show_page_thunk

; 0x21088
_copy_rect_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_COPY_RECT
old_int9_off db 0h, 0h
old_int9_seg db 0h, 0h
old_int1c_off db 0h, 0h
old_int1c_seg db 0h, 0h
_copy_rect_thunk endp

; 0x21094
_install_keyboard proc far
        push bp
        mov bp, sp
        push di
        push si
        sub ax, ax
        cmp byte ptr DGROUP:kbd_installed, al
        je install_new
        jmp install_caps
install_new:
        push ds
        mov ax, 3509h
        int 21h
        mov word ptr cs:old_int9_off, bx
        mov word ptr cs:old_int9_seg, es
        mov ax, 351ch
        int 21h
        mov word ptr cs:old_int1c_off, bx
        mov word ptr cs:old_int1c_seg, es
        mov dx, offset _keyboard_isr
        mov ax, seg _keyboard_isr
        mov ds, ax
        mov ax, 2509h
        int 21h
        mov ax, word ptr [bp+6]
        neg ax
        jae install_no_tick
        mov dx, offset kjoy_tick
        mov ax, seg kjoy_tick
        mov ds, ax
        mov ax, 251ch
        int 21h
install_no_tick:
        pop ds
        mov byte ptr DGROUP:kbd_pcjr, 0
        call FAR PTR _detect_pcjr
        neg ax
        jae install_done
        int 15h
        jae install_not_pcjr
        mov ax, 40h
        mov es, ax
        mov si, 96h
        cmp byte ptr es:[si], 10h
        jne install_not_pcjr
        mov byte ptr DGROUP:kbd_pcjr, 1
        jmp short install_done
install_not_pcjr:
        mov byte ptr cs:caps_lock_and, 0feh
        mov byte ptr cs:num_lock_and, 0feh
        mov bx, offset DGROUP:kbd_state
        mov al, byte ptr [bx+48h]
        mov byte ptr [bx+29h], al
        mov al, byte ptr [bx+4bh]
        mov byte ptr [bx+2bh], al
        mov al, byte ptr [bx+4dh]
        mov byte ptr [bx+4eh], al
        mov al, byte ptr [bx+50h]
        mov byte ptr [bx+4ah], al
install_done:
        mov ax, 1
        mov byte ptr DGROUP:kbd_installed, al
install_caps:
        mov ax, 40h
        mov es, ax
        and byte ptr es:[17h], 0dfh
        test byte ptr DGROUP:kbd_hold_caps_lock, 0ffh
        je install_caps_set
        or byte ptr es:[17h], 40h
install_caps_set:
        mov al, byte ptr DGROUP:kbd_installed
        pop si
        pop di
        pop bp
        retf
_install_keyboard endp

; 0x21158
_remove_keyboard proc far
        sub ax, ax
        cmp byte ptr DGROUP:kbd_installed, al
        je remove_done
        mov byte ptr DGROUP:kbd_installed, al
        mov ax, 40h
        mov es, ax
        mov ax, word ptr es:[1ch]
        mov word ptr es:[1ah], ax
        push ds
        mov dx, word ptr cs:old_int9_off
        mov ax, word ptr cs:old_int9_seg
        mov ds, ax
        mov ax, 2509h
        int 21h
        mov dx, word ptr cs:old_int1c_off
        mov ax, word ptr cs:old_int1c_seg
        mov ds, ax
        mov ax, 251ch
        int 21h
        pop ds
        mov ax, 1
remove_done:
        retf
_remove_keyboard endp

; 0x21196
_keyboard_isr proc near
        push ax
        push bx
        push cx
        push dx
        push ds
        push es
        push di
        mov ax, 40h
        mov es, ax
        mov ax, DGROUP
        mov ds, ax
        xor ax, ax
        in al, 60h
        mov dx, 61h
        mov bx, ax
        in al, dx
        mov ah, al
        or al, 80h
        out dx, al
        mov al, ah
        out dx, al
        mov ax, bx
        and al, 7fh
        and bl, 80h
        cmp byte ptr DGROUP:_g_vmds+vmds_is_pcjr, 1
        jne isr_mapped
        cmp byte ptr DGROUP:kbd_pcjr, 1
        je isr_pcjr
        mov di, offset DGROUP:kbd_pcjr_from
        dec di
        mov cx, 0bh
isr_remap_find:
        inc di
        cmp al, byte ptr [di]
        loopne isr_remap_find
        jne isr_mapped
        add di, 0bh
        mov al, byte ptr [di]
        jmp short isr_mapped
isr_pcjr:
        cmp al, 29h
        jne isr_pcjr_2b
        mov al, 48h
isr_pcjr_2b:
        cmp al, 2bh
        jne isr_mapped
        mov al, 4bh
isr_mapped:
        or al, bl
        mov dh, 0ffh
        mov dl, al
        shl dx, 1
        shr dl, 1
        cmp dl, 59h
        jl isr_in_range
        jmp isr_eoi
isr_in_range:
        xor bh, bh
        mov bl, dl
        mov dl, byte ptr kbd_state[bx]
        and dh, dl
        xor dh, 1
        mov byte ptr kbd_state[bx], dh
        cmp byte ptr DGROUP:kbd_pcjr, 1
        jne isr_toggled
        cmp bl, 3ah
        jne isr_num_lock
        mov al, bl
        and dl, 0ffh
caps_lock_and equ byte ptr $-1
        jmp short isr_toggled
isr_num_lock:
        cmp bl, 45h
        jne isr_toggled
        mov al, bl
        and dl, 0ffh
num_lock_and equ byte ptr $-1
isr_toggled:
        mov bx, offset DGROUP:kbd_ascii
        test al, 80h
        je isr_press
        test dl, 0f8h
        je isr_release_char
        mov cx, dx
        mov ch, 0
        shr cx, 1
        shr cx, 1
        shr cx, 1
        mov di, cx
        shr cx, 1
        and di, 1
        mov ch, byte ptr kbd_held[di]
        cmp ch, cl
        jne isr_release_char
        mov byte ptr kbd_held[di], 0
isr_release_char:
        mov word ptr DGROUP:kbd_last_event, 0
        and al, 7fh
        xlatb
        test al, 80h
        je isr_release_done
        test al, 70h
        jne isr_release_done
        xor al, 7fh
        and byte ptr es:[17h], al
isr_release_done:
        jmp isr_eoi
isr_press:
        test dl, 0f8h
        je isr_press_char
        mov cx, dx
        mov ch, 0
        shr cx, 1
        shr cx, 1
        shr cx, 1
        mov di, cx
        shr cx, 1
        and di, 1
        mov byte ptr kbd_held[di], cl
isr_press_char:
        mov cl, al
        xor ah, ah
        mov di, ax
        xlatb
        test al, 80h
        je isr_char
        and al, 7fh
        test al, 70h
        jne isr_lock_key
        or byte ptr es:[17h], al
        jmp isr_eoi
isr_lock_key:
        test al, 40h
        je isr_lock_toggle
        test byte ptr DGROUP:kbd_hold_caps_lock, 0ffh
        jne isr_lock_done
isr_lock_toggle:
        shr dl, 1
        jb isr_lock_done
        xor byte ptr es:[17h], al
isr_lock_done:
        jmp isr_eoi
isr_char:
        test byte ptr es:[17h], 4
        je isr_no_ctrl
        or al, 80h
        test dl, 4
        je isr_have_char
        sub al, 20h
        jmp short isr_have_char
isr_no_ctrl:
        test byte ptr es:[17h], 40h
        je isr_no_caps
        test dl, 4
        je isr_no_caps
        sub al, 20h
        jmp short isr_have_char
isr_no_caps:
        test byte ptr es:[17h], 3
        je isr_have_char
        mov al, byte ptr kbd_shifted[di]
isr_have_char:
        mov ah, cl
        mov word ptr DGROUP:kbd_last_event, ax
        mov cx, word ptr es:[1ah]
        mov di, word ptr es:[1ch]
        cmp cx, 3ch
        je isr_ring_wrap
        inc cx
        inc cx
        cmp cx, di
        je isr_ring_full
        jmp short isr_ring_put
isr_ring_wrap:
        cmp di, 1eh
        je isr_ring_full
isr_ring_put:
        mov word ptr es:[di], ax
        cmp di, 3ch
        jne isr_ring_advance
        mov di, 1ch
isr_ring_advance:
        add di, 2
        mov word ptr es:[1ch], di
isr_ring_full:
        cmp ah, 20h
        jne isr_check_quit
        test byte ptr es:[17h], 4
        je isr_check_quit
        mov al, 20h
        out 20h, al
        jmp short isr_return
isr_check_quit:
        xor bx, bx
        cmp ax, 19bh
        je isr_quit
        inc bx
        cmp ax, 5380h
        jne isr_eoi
        test byte ptr es:[17h], 8
        je isr_eoi
isr_quit:
        test byte ptr es:[17h], 4
        je isr_eoi
        sub di, 2
        cmp di, 1ch
        jne isr_unput
        mov di, 3ch
isr_unput:
        mov word ptr es:[di], 0
        mov ax, word ptr es:[1ch]
        mov word ptr es:[1ah], ax
        mov al, 20h
        out 20h, al
        sti
        mov ax, bx
        push ax
        call FAR PTR _game_teardown
        pop ax
        jmp short isr_return
isr_eoi:
        mov al, 20h
        out 20h, al
isr_return:
        pop di
        pop es
        pop ds
        pop dx
        pop cx
        pop bx
        pop ax
        iret
kjoy_tick label byte
        push ax
        push bx
        push cx
        push dx
        push di
        push si
        push bp
        push ds
        push es
        mov ax, DGROUP
        mov ds, ax
        mov ax, word ptr ds:[46f1h]
        mov es, ax
        mov bp, 7fh
        mov si, 2
        mov bh, 0
kjoy_stick:
        sub ax, ax
        shr si, 1
        mov bl, byte ptr kbd_held[si]
        shl si, 1
        dec bl
        jns kjoy_key
        cmp word ptr kjoy_enabled[si], ax
        je kjoy_next
kjoy_stop:
        mov word ptr kjoy_dx[si], ax
        mov word ptr kjoy_dy[si], ax
        jmp short kjoy_next
kjoy_key:
        cmp bl, 8
        je kjoy_stop
        mov bl, byte ptr kjoy_dirs[bx]
        shl bl, 1
        jae kjoy_y
        mov ax, word ptr kjoy_dx[si]
        cwd
        xor ax, dx
        sub ax, dx
        mov cx, ax
        mov di, dx
        mov ah, bl
        shl bl, 1
        cwd
        xor dx, di
        mov ax, es
        xor ax, dx
        sub ax, dx
        add ax, cx
        cmp ax, bp
        jle kjoy_x_clamped
        mov ax, bp
kjoy_x_clamped:
        xor ax, di
        sub ax, di
        mov word ptr kjoy_dx[si], ax
kjoy_y:
        shl bl, 1
        jae kjoy_next
        mov ax, word ptr kjoy_dy[si]
        cwd
        xor ax, dx
        sub ax, dx
        mov cx, ax
        mov di, dx
        mov ah, bl
        cwd
        xor dx, di
        mov ax, es
        xor ax, dx
        sub ax, dx
        add ax, cx
        cmp ax, bp
        jle kjoy_y_clamped
        mov ax, bp
kjoy_y_clamped:
        xor ax, di
        sub ax, di
        mov word ptr kjoy_dy[si], ax
kjoy_next:
        sub si, 2
        js kjoy_done
        jmp kjoy_stick
kjoy_done:
        pop es
        pop ds
        pop bp
        pop si
        pop di
        pop dx
        pop cx
        pop bx
        pop ax
        iret
_keyboard_isr endp

; 0x21434
_bios_read_key proc far
        pushf
        cli
        push es
        mov ax, 40h
        mov es, ax
        xor ax, ax
        mov bx, word ptr es:[1ah]
        cmp bx, word ptr es:[1ch]
        je bios_key_none
        mov ax, word ptr es:[bx]
        inc bx
        inc bx
        cmp bx, word ptr es:[82h]
        jne bios_key_head
        mov bx, word ptr es:[80h]
bios_key_head:
        mov word ptr es:[1ah], bx
bios_key_none:
        pop es
        popf
        retf
bios_read_key_dead db 8bh, 0d5h
bios_read_key_dead_2 db 8bh, 0ech
bios_read_key_dead_4 db 2bh, 0dbh
bios_read_key_dead_6 db 8bh, 46h, 4h
bios_read_key_dead_9 db 0f7h, 0d8h
bios_read_key_dead_b db 0d1h, 0d3h
bios_read_key_dead_d db 8ah, 9fh, 90h, 45h
bios_read_key_dead_11 db 8ah, 0e7h
bios_read_key_dead_13 db 8ah, 87h, 0fbh, 46h
bios_read_key_dead_17 db 8bh, 0eah
bios_read_key_dead_19 db 0cbh
_bios_read_key endp

; 0x2147d
_key_is_down proc far
        cli
        mov dx, bp
        mov bp, sp
        mov bx, word ptr [bp+4]
        xor ax, ax
        mov al, byte ptr kbd_state[bx]
        and al, 1
        mov bp, dx
        sti
        retf
key_is_down_dead db 0cbh
border_colour_thunk:
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_BORDER_COLOUR
slot_26_thunk:
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_26
_key_is_down endp

; 0x2149a
_show_page_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_SHOW_PAGE
_show_page_thunk endp
KEYBOARD_TEXT ends
end
