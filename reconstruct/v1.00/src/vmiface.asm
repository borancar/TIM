; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `vmiface.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `vmiface.c` is the host's transcription of it.
;
; The module as TASM assembled it; the host's transcription is `vmiface.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs vmds=vmds vm_driver=vmdrv VM_SLOT_*
; JUDGE: assembler bc3.00

_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
extrn _dos_alloc_bytes:far
extrn _dos_free_far:far
extrn _readfd_far:far
extrn _game_fseek:far
extrn _game_ftell:far
extrn _game_fwrite:far
extrn _game_fputc:far
extrn _game_rewind:far
extrn _close_far:far
extrn _game_fopen:far
extrn _game_fread:far
extrn _game_fclose:far
extrn _malloc_far:far
extrn _calloc_far:far
extrn _game_fgetc:far
extrn _free_far:far
extrn _strcat_far:far
extrn _strcpy_far:far
extrn _strchr_far:far
_DATA segment para public 'DATA'
public _g_vmds, _g_vm_driver, _g_vm_hooks
_g_vmds label byte
        db 6 dup (0)
        db 03fh, 001h
        db 2 dup (0)
        db 0c7h
        db 0
        db 001h
        db 1757 dup (0)
        db 040h, 001h, 0c8h
        db 965 dup (0)
_g_vm_driver label byte
        dw 0
        dw 1
        dd 50 dup (_vm_null_hook)
_g_vm_hooks label byte
        dd _vm_null_hook
        dd _dos_alloc_bytes
        dd _dos_free_far
        dd _readfd_far
        dd _game_fseek
        dd _game_ftell
        dd _game_fwrite
        dd _game_fputc
        dd _game_rewind
        dd _close_far
        dd _game_fopen
        dd _game_fread
        dd _game_fclose
        dd _malloc_far
        dd _calloc_far
        dd _game_fgetc
        dd _free_far
        dd _strcat_far
        dd _strcpy_far
        dd _strchr_far
_DATA ends

VMIFACE_TEXT segment byte public 'CODE'
assume cs:VMIFACE_TEXT, ds:DGROUP
public _blit_bitmap_plain_thunk, _blit_bitmap_thunk, _blit_scaled_thunk, _blit_scaled_row_thunk
public _restore_write_mode, _vm_null_hook

; 0x1e93c
_blit_bitmap_plain_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_BLIT_PLAIN
_blit_bitmap_plain_thunk endp

; 0x1e940
_blit_bitmap_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_BLIT_BITMAP
_blit_bitmap_thunk endp

; 0x1e944
_blit_scaled_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_BLIT_SCALED
_blit_scaled_thunk endp

; 0x1e948
_blit_scaled_row_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_SCALED_ROW
_blit_scaled_row_thunk endp

; 0x1e94c
_restore_write_mode proc far
        cmp byte ptr DGROUP:_g_vmds+vmds_adapter, 10h
        jne restore_done
        mov ax, 205h
        mov dx, 3ceh
        out dx, ax
        mov ax, 0ff08h
        out dx, ax
        mov dx, 3c4h
        mov ax, 0f02h
        out dx, ax
restore_done:
        retf
_restore_write_mode endp

; 0x1e966
_vm_null_hook proc far
        retf
_vm_null_hook endp
VMIFACE_TEXT ends
end
