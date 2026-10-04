; The Incredible Machine - reconstruction
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `glue.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `glue.c` is the host's transcription of it.
;
; The module as TASM assembled it: the judge hands this source to TASM 3.0
; (`JUDGE: tasm`), so it is the source the image's bytes come from; the host's
; transcription is `glue.c`. A C comment is the only kind the source carries.
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
_TEXT segment word public 'CODE'
assume cs:_TEXT, ds:DGROUP
public _sound_module_install, _sound_module_set_rate, _sound_module_service
public _sound_module_9, _sound_module_10, _sound_module_11
public _stop_loaded_module, _sound_module_shutdown
public _call_sound_module, _sound_module_position
extrn _g_sound_bank:byte

; 0x0bb98
_sound_module_install proc far
    mov ax, 0
    call _call_sound_module
    retf
_sound_module_install endp

; 0x0bb9f
_sound_module_set_rate proc far
    mov ax, 6
    call _call_sound_module
    retf
_sound_module_set_rate endp

; 0x0bba6: the EOI to the master PIC, then the service call.
_sound_module_service proc far
    mov al, 20h
    out 20h, al
    mov ax, 1
    call _call_sound_module
    retf
_sound_module_service endp

; 0x0bbb1
_sound_module_9 proc far
    mov ax, 9
    call _call_sound_module
    retf
_sound_module_9 endp

; 0x0bbb8
_sound_module_10 proc far
    mov ax, 0ah
    call _call_sound_module
    retf
_sound_module_10 endp

; 0x0bbbf
_sound_module_11 proc far
    mov ax, 0bh
    call _call_sound_module
    retf
_sound_module_11 endp

; 0x0bbc6
_stop_loaded_module proc far
    mov ax, 2
    call _call_sound_module
    retf
_stop_loaded_module endp

; 0x0bbcd
_sound_module_shutdown proc far
    mov ax, 0ch
    call _call_sound_module
    retf
_sound_module_shutdown endp

; 0x0bbd4: SI at the wrapper's caller's arguments, past BP and two returns.
_call_sound_module proc near
    push bp
    mov bp, sp
    push di
    push si
    mov si, bp
    add si, 8
    call dword ptr DGROUP:_g_sound_bank+bank_module
    pop si
    pop di
    pop bp
    ret
_call_sound_module endp

; 0x0bbe6: six bytes of stack for the three words the module writes back.
_sound_module_position proc far
    mov ax, 0dh
    push bp
    mov bp, sp
    push di
    push si
    sub sp, 6
    mov si, sp
    call dword ptr DGROUP:_g_sound_bank+bank_module
    pop ax
    pop ax
    pop dx
    pop si
    pop di
    pop bp
    retf
_sound_module_position endp
_TEXT ends
end
