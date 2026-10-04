; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `crtc.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `crtc.c` is the host's transcription of it.
;
; The module as TASM assembled it; the host's transcription is `crtc.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: assembler bc3.00

_DATA segment word public 'DATA'
_DATA ends
_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
_TEXT segment byte public 'CODE'
assume cs:_TEXT, ds:DGROUP
public _vm_set_line_compare, _vm_set_display_lines

; 0x08f27
_vm_set_line_compare proc far
        push bp
        mov bp, sp
        push si
        push di
        push ax
        push bx
        push dx
        mov bx, [bp+6]
        mov dx, 3d4h
        mov al, 18h
        out dx, al
        inc dx
        mov al, bl
        out dx, al
        dec dx
        mov al, 7
        out dx, al
        inc dx
        in al, dx
        and al, 0efh
        mov bl, bh
        and bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        or al, bl
        out dx, al
        dec dx
        mov al, 9
        out dx, al
        inc dx
        in al, dx
        and al, 0bfh
        mov bl, bh
        and bl, 2
        shl bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        or al, bl
        out dx, al
        pop dx
        pop bx
        pop ax
        pop di
        pop si
        mov sp, bp
        pop bp
        retf
_vm_set_line_compare endp

; 0x08f77
_vm_set_display_lines proc far
        push bp
        mov bp, sp
        push si
        push di
        push ax
        push bx
        push dx
        mov bx, [bp+6]
        mov dx, 3d4h
        mov al, 15h
        out dx, al
        inc dx
        mov al, bl
        out dx, al
        dec dx
        mov al, 7
        out dx, al
        inc dx
        in al, dx
        and al, 0f7h
        mov bl, bh
        and bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        or al, bl
        out dx, al
        dec dx
        mov al, 9
        out dx, al
        inc dx
        in al, dx
        and al, 0dfh
        mov bl, bh
        and bl, 2
        shl bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        or al, bl
        out dx, al
        pop dx
        pop bx
        pop ax
        pop di
        pop si
        mov sp, bp
        pop bp
        retf
_vm_set_display_lines endp
_TEXT ends
end
