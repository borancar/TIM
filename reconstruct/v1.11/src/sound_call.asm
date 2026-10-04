; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `sound_call.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `sound_call.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `sound_call.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs sound_bank=bank
; JUDGE: assembler bc3.00

_DATA segment word public 'DATA'
_DATA ends
_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
extrn _g_sound_bank:byte
SOUND_CALL_TEXT segment byte public 'CODE'
assume cs:SOUND_CALL_TEXT, ds:DGROUP
public _set_sound_callback, _sound_callback, _sound_callback_quiet
callback_off label byte
        db 0h, 0h
callback_seg label byte
        db 0h, 0h
callback_answer label byte
        db 0h, 0h

; 0x2bb2c
_set_sound_callback proc far
        push bp
        mov bp, sp
        push ax
        mov ax, word ptr [bp+6]
        mov word ptr cs:callback_off, ax
        mov ax, word ptr [bp+8]
        mov word ptr cs:callback_seg, ax
        pop ax
        pop bp
        retf
_set_sound_callback endp

; 0x2bb41
_sound_callback proc far
        push bp
        mov bp, sp
        push ds
        push es
        pushf
        push ax
        push cx
        push dx
        push bx
        push bp
        push si
        push di
        mov ax, DGROUP
        mov ds, ax
        cmp word ptr DGROUP:_g_sound_bank+bank_module_live, 0
        je callback_none
        mov si, word ptr [bp+8]
        mov ax, word ptr [bp+6]
        call dword ptr cs:callback_off
callback_none:
        mov word ptr cs:callback_answer, ax
        pop di
        pop si
        pop bp
        pop bx
        pop dx
        pop cx
        pop ax
        popf
        mov ax, word ptr cs:callback_answer
        pop es
        pop ds
        pop bp
        retf
_sound_callback endp

; 0x2bb79
_sound_callback_quiet proc far
        push bp
        mov bp, sp
        push si
        push di
        cmp word ptr DGROUP:_g_sound_bank+bank_module_live, 0
        je quiet_none
        mov si, word ptr [bp+8]
        mov ax, word ptr [bp+6]
        call dword ptr cs:callback_off
quiet_none:
        pop di
        pop si
        pop bp
        retf
_sound_callback_quiet endp
SOUND_CALL_TEXT ends
end
