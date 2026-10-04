; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `dosmem.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `dosmem.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `dosmem.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs vm_driver=vmdrv VM_SLOT_*
; JUDGE: assembler bc3.00

_DATA segment word public 'DATA'
_DATA ends
_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
extrn _far_memset:far
extrn _g_vm_driver:byte
DOSMEM_TEXT segment byte public 'CODE'
assume cs:DOSMEM_TEXT, ds:DGROUP
public _save_rect_thunk, _buffer_size_thunk, _dos_alloc_bytes, _dos_free_far

; 0x21ab5
_save_rect_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_SAVE_RECT
_save_rect_thunk endp

; 0x21ab9
_buffer_size_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_BUFFER_SIZE
_buffer_size_thunk endp

; 0x21abd
_dos_alloc_bytes proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+8]
        mov bx, word ptr [bp+6]
        cmp ax, bx
        jne alloc_to_paras
        cmp ax, 0ffffh
        je alloc_ask_free
alloc_to_paras:
        mov dx, bx
        shr ax, 1
        rcr bx, 1
        shr ax, 1
        rcr bx, 1
        shr ax, 1
        rcr bx, 1
        shr ax, 1
        rcr bx, 1
        and dx, 0fh
        je alloc_rounded
        inc bx
alloc_rounded:
        mov ah, 48h
        int 21h
        jae alloc_got
        xor ax, ax
        xor dx, dx
        jmp short alloc_return
alloc_got:
        mov dx, ax
        mov ax, word ptr [bp+0ch]
        and ax, 1
        je alloc_return
        push dx
        mov ax, word ptr [bp+8]
        push ax
        mov ax, word ptr [bp+6]
        push ax
        xor ax, ax
        push ax
        push dx
        push ax
        call FAR PTR _far_memset
        add sp, 0ah
        pop dx
        xor ax, ax
        jmp short alloc_return
alloc_ask_free:
        mov ah, 48h
        int 21h
        mov ax, bx
        xor bx, bx
        shl ax, 1
        rcl bx, 1
        shl ax, 1
        rcl bx, 1
        shl ax, 1
        rcl bx, 1
        shl ax, 1
        rcl bx, 1
        mov dx, bx
alloc_return:
        pop bp
        retf
_dos_alloc_bytes endp

; 0x21b34
_dos_free_far proc far
        push bp
        mov bp, sp
        push es
        mov ax, word ptr [bp+8]
        mov es, ax
        mov ah, 49h
        int 21h
        pop es
        pop bp
        retf
_dos_free_far endp
DOSMEM_TEXT ends
end
