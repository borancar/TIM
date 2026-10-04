; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `dos.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `dos.c` is the host's transcription of it.
;
; The module as TASM assembled it; the host's transcription is `dos.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: assembler bc3.00

_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
_DATA segment word public 'DATA'
        dw 0
find_name db 13 dup (0)
        db 1fh dup (0)
find_attr db 0
find_size dw 0, 0
dos_result dw 0
        db 0ffh dup (0)
isr_stack_top label word
        db 302h dup (0)
isr_saved_ss dw 0
isr_saved_sp dw 0
_DATA ends

_TEXT segment byte public 'CODE'
assume cs:_TEXT, ds:DGROUP
public _dos_findfirst, _dos_findnext, _dos_find_to_dgroup, _dos_find_attr
public _dos_find_name, _dos_find_size, _diskette_motor_bit, _dos_chdir
public _dos_mkdir, _dos_rmdir, _dos_unlink, _dos_drive_letter
public _dos_get_cur_dir, _dos_drive_fixed, _dos_disk_reset
public _dos_set_attributes, _dos_get_attributes, _dos_setdisk
public _isr_stack_switch

; 0x0b6b7
_dos_findfirst proc far
        push bp
        mov bp, sp
        push si
        push di
        mov ax, [bp+6]
        mov cx, [bp+8]
        mov dx, ax
        mov ah, 4eh
        int 21h
        pop di
        pop si
        pop bp
        xor ah, ah
        push ax
        call _dos_find_to_dgroup
        pop ax
        retf
_dos_findfirst endp

; 0x0b6d3
_dos_findnext proc far
        push bp
        mov bp, sp
        push si
        push di
        mov ax, [bp+6]
        mov cx, [bp+8]
        mov dx, ax
        mov ah, 4fh
        int 21h
        pop di
        pop si
        pop bp
        xor ah, ah
        push ax
        call _dos_find_to_dgroup
        pop ax
        retf
_dos_findnext endp

; 0x0b6ef: the DTA's attribute, size and name into DGROUP.
_dos_find_to_dgroup proc near
        push bp
        mov bp, sp
        push bx
        push cx
        push si
        push es
        mov ah, 2fh
        int 21h
        mov cx, 0dh
        mov si, bx
        add si, 1eh
        add bx, 15h
        mov al, es:[bx]
        mov find_attr, al
        add bx, 5
        mov ax, es:[bx]
        mov find_size, ax
        mov ax, es:[bx+2]
        mov find_size+2, ax
        xor bx, bx
next_byte:
        mov al, es:[si]
        mov find_name[bx], al
        inc bx
        inc si
        loop next_byte
        pop es
        pop si
        pop cx
        pop bx
        pop bp
        ret
_dos_find_to_dgroup endp

; 0x0b72e
_dos_find_attr proc far
        mov al, find_attr
        xor ah, ah
        retf
_dos_find_attr endp

; 0x0b734
_dos_find_name proc far
        mov ax, offset DGROUP:find_name
        retf
_dos_find_name endp

; 0x0b738
_dos_find_size proc far
        mov ax, find_size
        mov dx, find_size+2
        retf
_dos_find_size endp

; 0x0b740
_diskette_motor_bit proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov es, ax
        mov bx, 43fh
        mov cl, [bp+6]
        inc ax
        shl ax, cl
        and ax, es:[bx]
        pop bp
        retf
_diskette_motor_bit endp

; 0x0b755
_dos_chdir proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov dx, [bp+6]
        mov ah, 3bh
        int 21h
        jb chdir_err
        xor ax, ax
chdir_err:
        mov dos_result, ax
        pop bp
        retf
_dos_chdir endp

; 0x0b76a
_dos_mkdir proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov dx, [bp+6]
        mov ah, 39h
        int 21h
        jb mkdir_err
        xor ax, ax
mkdir_err:
        mov dos_result, ax
        pop bp
        retf
_dos_mkdir endp

; 0x0b77f
_dos_rmdir proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov dx, [bp+6]
        mov ah, 3ah
        int 21h
        jb rmdir_err
        xor ax, ax
rmdir_err:
        mov dos_result, ax
        pop bp
        retf
_dos_rmdir endp

; 0x0b794
_dos_unlink proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov dx, [bp+6]
        mov ah, 41h
        int 21h
        jb unlink_err
        xor ax, ax
unlink_err:
        mov dos_result, ax
        pop bp
        retf
_dos_unlink endp

; 0x0b7a9
_dos_drive_letter proc far
        mov ax, 1900h
        int 21h
        add al, 41h
        xor ah, ah
        retf
_dos_drive_letter endp

; 0x0b7b3
_dos_get_cur_dir proc far
        push bp
        mov bp, sp
        push si
        xor ax, ax
        mov ah, 19h
        int 21h
        add al, 41h
        mov bx, [bp+6]
        mov [bx], al
        mov byte ptr [bx+1], 3ah
        mov byte ptr [bx+2], 5ch
        add bx, 3
        mov si, bx
        mov dl, 0
        mov ax, 4700h
        int 21h
        pop si
        pop bp
        retf
_dos_get_cur_dir endp

; 0x0b7db
_dos_drive_fixed proc far
        push bp
        mov bp, sp
        mov bx, [bp+6]
        add bx, 1
        mov ax, 4408h
        int 21h
        pop bp
        retf
_dos_drive_fixed endp

; 0x0b7eb
_dos_disk_reset proc far
        mov ax, 0d00h
        int 21h
        retf
_dos_disk_reset endp

; 0x0b7f1
_dos_set_attributes proc far
        push bp
        mov bp, sp
        mov dx, [bp+6]
        mov cx, [bp+8]
        mov ax, 4301h
        int 21h
        jb setattr_err
        xor ax, ax
setattr_err:
        pop bp
        retf
_dos_set_attributes endp

; 0x0b805
_dos_get_attributes proc far
        push bp
        mov bp, sp
        mov dx, [bp+6]
        mov ax, 4300h
        int 21h
        mov ax, cx
        jae getattr_ok
        mov ax, 0ffffh
getattr_ok:
        pop bp
        retf
_dos_get_attributes endp

; 0x0b819
_dos_setdisk proc far
        push bp
        mov bp, sp
        mov al, [bp+6]
        and al, 5fh
        sub al, 41h
        mov dl, al
        mov ax, 0e00h
        int 21h
        pop bp
        retf
_dos_setdisk endp

; 0x0b82c: the return address and the argument come off the stack, and go
; back on whichever stack SS:SP is left on.
_isr_stack_switch proc far
        pop bx
        pop cx
        pop dx
        or dx, dx
        je isr_restore
        mov ax, ss
        mov isr_saved_ss, ax
        mov ax, sp
        mov isr_saved_sp, ax
        mov ax, ds
        mov ss, ax
        mov ax, offset DGROUP:isr_stack_top
        mov sp, ax
        push dx
        push cx
        push bx
        cld
        retf
isr_restore:
        mov ax, isr_saved_ss
        mov ss, ax
        mov ax, isr_saved_sp
        mov sp, ax
        push dx
        push cx
        push bx
        retf
_isr_stack_switch endp
_TEXT ends
end
