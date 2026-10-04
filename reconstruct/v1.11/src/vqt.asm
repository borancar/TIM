; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `vqt.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `vqt.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
; transcription is `vqt.c`.
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs vmds=vmds vm_driver=vmdrv bitmaps_state=bm vqt_reader=rd VM_SLOT_*
; JUDGE: assembler bc3.00

_DATA segment word public 'DATA'
_DATA ends
_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
extrn _redraw_cursor:far
extrn _g_bitmaps:byte
extrn _g_vm_driver:byte
extrn _g_vmds:byte
VQT_TEXT segment byte public 'CODE'
assume cs:VQT_TEXT, ds:DGROUP
public _vqt_read_bits, _vqt_screen_node, _fill_screen_quadrant, _far_copy
public _vqt_node, _fill_quadrant

; 0x275dd
_vqt_read_bits proc near
        push bp
        mov bp, sp
        mov bx, word ptr [bp+4]
        mov bp, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp+rd_pos]
        mov dx, word ptr [bp+rd_pos+2]
        add word ptr [bp+rd_pos], cx
        adc word ptr [bp+rd_pos+2], 0
        mov cx, word ptr [bp+rd_data]
        mov es, word ptr [bp+rd_data+2]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        ret
screen_node_none:
        pop di
        pop si
        add sp, 2
        pop bp
        ret
_vqt_read_bits endp

; 0x2762b
_vqt_screen_node proc near
        push bp
        mov bp, sp
        sub sp, 2
        push si
        push di
        mov si, word ptr [bp+8]
        mov di, word ptr [bp+0ah]
        mov ax, si
        or ax, di
        je screen_node_none
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 4
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        and ax, 0fh
        mov byte ptr [bp-2], al
        mov ax, di
        shr ax, 1
        push ax
        mov ax, si
        shr ax, 1
        push ax
        push word ptr [bp+6]
        push word ptr [bp+4]
        test byte ptr [bp-2], 8
        je screen_q0_fill
        call _vqt_screen_node
        jmp short screen_q1
screen_q0_fill:
        call _fill_screen_quadrant
        mov ax, word ptr DGROUP:_g_vmds+vmds_page_front
        push ax
        call FAR PTR _redraw_cursor
        add sp, 2
screen_q1:
        add sp, 8
        mov ax, di
        shr ax, 1
        push ax
        mov ax, si
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        push word ptr [bp+6]
        shr bx, 1
        add bx, word ptr [bp+4]
        push bx
        test byte ptr [bp-2], 4
        je screen_q1_fill
        call _vqt_screen_node
        jmp short screen_q2
screen_q1_fill:
        call _fill_screen_quadrant
screen_q2:
        add sp, 8
        mov ax, di
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        mov ax, si
        shr ax, 1
        push ax
        shr bx, 1
        add bx, word ptr [bp+6]
        push bx
        push word ptr [bp+4]
        test byte ptr [bp-2], 2
        je screen_q2_fill
        call _vqt_screen_node
        jmp short screen_q3
screen_q2_fill:
        call _fill_screen_quadrant
screen_q3:
        add sp, 8
        mov ax, di
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        mov ax, si
        mov cx, ax
        inc ax
        shr ax, 1
        push ax
        shr bx, 1
        add bx, word ptr [bp+6]
        push bx
        shr cx, 1
        add cx, word ptr [bp+4]
        push cx
        test byte ptr [bp-2], 1
        je screen_q3_fill
        call _vqt_screen_node
        jmp short screen_node_done
screen_q3_fill:
        call _fill_screen_quadrant
screen_node_done:
        add sp, 8
        pop di
        pop si
        add sp, 2
        pop bp
        ret
screen_fill_none:
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
_vqt_screen_node endp

; 0x27734
_fill_screen_quadrant proc near
        push bp
        mov bp, sp
        sub sp, 10ah
        push si
        push di
        mov ax, word ptr [bp+0ah]
        or ax, ax
        je screen_fill_none
        mov di, ax
        mov ax, word ptr [bp+8]
        or ax, ax
        je screen_fill_none
        mov si, ax
        cmp si, 1
        jne screen_fill_area
        cmp di, 1
        jne screen_fill_area
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov ch, al
        mov bx, word ptr [bp+6]
        shl bx, 1
        mov bx, word ptr DGROUP:_g_vmds[bx+vmds_row_offset]
        mov ax, word ptr [bp+4]
        mov cl, al
        shr ax, 1
        shr ax, 1
        add bx, ax
        and cl, 3
        mov dx, 3c4h
        mov ax, 102h
        shl ah, cl
        out dx, ax
        mov ax, word ptr DGROUP:_g_vmds+vmds_page_dst
        mov es, ax
        mov byte ptr es:[bx], ch
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
screen_fill_area:
        mov ax, si
        mov bx, di
        mul bl
        mov word ptr [bp-6], ax
        mov cx, 8
        or ah, ah
        jne screen_fill_read_colours
        xor cx, cx
        dec al
screen_fill_area_bits:
        inc cx
        shr al, 1
        jne screen_fill_area_bits
screen_fill_read_colours:
        push bp
        mov bx, cx
        mov bp, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp+rd_pos]
        mov dx, word ptr [bp+rd_pos+2]
        add word ptr [bp+rd_pos], cx
        adc word ptr [bp+rd_pos+2], 0
        mov cx, word ptr [bp+rd_data]
        mov es, word ptr [bp+rd_data+2]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        xor cx, cx
        mov word ptr [bp-4], ax
        or al, al
        je screen_fill_choose
screen_fill_index_bits:
        inc cx
        shr al, 1
        jne screen_fill_index_bits
screen_fill_choose:
        mov ax, word ptr [bp+4]
        mov di, ax
        add ax, word ptr [bp+8]
        mov word ptr [bp-8], ax
        mov ax, word ptr [bp+6]
        mov si, ax
        add ax, word ptr [bp+0ah]
        mov word ptr [bp-0ah], ax
        mov word ptr [bp-2], cx
        inc byte ptr [bp-4]
        mov ax, word ptr [bp-6]
        mov bx, ax
        shl bx, 1
        shl bx, 1
        shl bx, 1
        mul word ptr [bp-2]
        mov cx, word ptr [bp-4]
        shl cx, 1
        shl cx, 1
        shl cx, 1
        add ax, cx
        cmp bx, ax
        ja screen_fill_coded
screen_fill_raw:
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov ch, al
        mov bx, si
        shl bx, 1
        mov bx, word ptr DGROUP:_g_vmds[bx+vmds_row_offset]
        mov ax, di
        mov cl, al
        shr ax, 1
        shr ax, 1
        add bx, ax
        and cl, 3
        mov dx, 3c4h
        mov ax, 102h
        shl ah, cl
        out dx, ax
        mov ax, word ptr DGROUP:_g_vmds+vmds_page_dst
        mov es, ax
        mov byte ptr es:[bx], ch
        inc si
        cmp si, word ptr [bp-0ah]
        jl screen_fill_raw
        mov si, word ptr [bp+6]
        inc di
        cmp di, word ptr [bp-8]
        jl screen_fill_raw
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
screen_fill_coded:
        cmp byte ptr [bp-4], 1
        jne screen_fill_palette
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov bx, word ptr [bp+6]
        shl bx, 1
        mov di, word ptr DGROUP:_g_vmds[bx+vmds_row_offset]
        mov bx, word ptr DGROUP:_g_vmds+vmds_page_dst
        mov es, bx
        mov si, word ptr [bp+0ah]
screen_fill_solid_row:
        mov cx, word ptr [bp+8]
        mov bx, word ptr [bp+4]
        push di
        mov ah, al
        call dword ptr DGROUP:_g_vm_driver+vmdrv_entry+4*VM_SLOT_SPAN
        pop di
        add di, 50h
        dec si
        jne screen_fill_solid_row
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
screen_fill_palette:
        lea di, [bp-10ah]
screen_fill_palette_read:
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov bx, ds
        mov es, bx
        stosb
        dec byte ptr [bp-4]
        jne screen_fill_palette_read
        mov di, word ptr [bp+4]
screen_fill_indexed:
        push bp
        mov bx, word ptr [bp-2]
        mov bp, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp+rd_pos]
        mov dx, word ptr [bp+rd_pos+2]
        add word ptr [bp+rd_pos], cx
        adc word ptr [bp+rd_pos+2], 0
        mov cx, word ptr [bp+rd_data]
        mov es, word ptr [bp+rd_data+2]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        lea bx, [bp-10ah]
        add bx, ax
        mov al, byte ptr [bx]
        mov ch, al
        mov bx, si
        shl bx, 1
        mov bx, word ptr DGROUP:_g_vmds[bx+vmds_row_offset]
        mov ax, di
        mov cl, al
        shr ax, 1
        shr ax, 1
        add bx, ax
        and cl, 3
        mov dx, 3c4h
        mov ax, 102h
        shl ah, cl
        out dx, ax
        mov ax, word ptr DGROUP:_g_vmds+vmds_page_dst
        mov es, ax
        mov byte ptr es:[bx], ch
        inc si
        cmp si, word ptr [bp-0ah]
        jl screen_fill_indexed
        mov si, word ptr [bp+6]
        inc di
        cmp di, word ptr [bp-8]
        jae screen_fill_done
        jmp screen_fill_indexed
screen_fill_done:
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
_fill_screen_quadrant endp

; 0x27a20
_far_copy proc near
        push bp
        mov bp, sp
        push ds
        push si
        push di
        les di, dword ptr [bp+4]
        lds si, dword ptr [bp+8]
        mov cx, word ptr [bp+0ch]
        shr cx, 1
        rep movsw
        jae far_copy_even
        movsb
far_copy_even:
        pop di
        pop si
        pop ds
        pop bp
        ret
node_none:
        pop di
        pop si
        add sp, 2
        pop bp
        ret
_far_copy endp

; 0x27a42
_vqt_node proc near
        push bp
        mov bp, sp
        sub sp, 2
        push si
        push di
        mov si, word ptr [bp+8]
        mov di, word ptr [bp+0ah]
        mov ax, si
        or ax, di
        je node_none
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 4
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        and ax, 0fh
        mov byte ptr [bp-2], al
        mov ax, di
        shr ax, 1
        push ax
        mov ax, si
        shr ax, 1
        push ax
        push word ptr [bp+6]
        push word ptr [bp+4]
        test byte ptr [bp-2], 8
        je q0_fill
        call _vqt_node
        jmp short q1
q0_fill:
        call _fill_quadrant
q1:
        add sp, 8
        mov ax, di
        shr ax, 1
        push ax
        mov ax, si
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        push word ptr [bp+6]
        shr bx, 1
        add bx, word ptr [bp+4]
        push bx
        test byte ptr [bp-2], 4
        je q1_fill
        call _vqt_node
        jmp short q2
q1_fill:
        call _fill_quadrant
q2:
        add sp, 8
        mov ax, di
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        mov ax, si
        shr ax, 1
        push ax
        shr bx, 1
        add bx, word ptr [bp+6]
        push bx
        push word ptr [bp+4]
        test byte ptr [bp-2], 2
        je q2_fill
        call _vqt_node
        jmp short q3
q2_fill:
        call _fill_quadrant
q3:
        add sp, 8
        mov ax, di
        mov bx, ax
        inc ax
        shr ax, 1
        push ax
        mov ax, si
        mov cx, ax
        inc ax
        shr ax, 1
        push ax
        shr bx, 1
        add bx, word ptr [bp+6]
        push bx
        shr cx, 1
        add cx, word ptr [bp+4]
        push cx
        test byte ptr [bp-2], 1
        je q3_fill
        call _vqt_node
        jmp short node_done
q3_fill:
        call _fill_quadrant
node_done:
        add sp, 8
        pop di
        pop si
        add sp, 2
        pop bp
        ret
fill_none:
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
_vqt_node endp

; 0x27b3f
_fill_quadrant proc near
        push bp
        mov bp, sp
        sub sp, 10ah
        push si
        push di
        mov ax, word ptr [bp+0ah]
        or ax, ax
        je fill_none
        mov di, ax
        mov ax, word ptr [bp+8]
        or ax, ax
        je fill_none
        mov si, ax
        cmp si, 1
        jne fill_area
        cmp di, 1
        jne fill_area
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov di, word ptr [bp+4]
        mov dl, al
        mov cx, di
        mov di, word ptr DGROUP:_g_bitmaps+bm_walk
        lea bx, [di+rd_row]
        mov ax, word ptr [bp+6]
        shl ax, 1
        add bx, ax
        mov bx, word ptr [bx]
        mov ax, cx
        add bx, ax
        les di, dword ptr [di+rd_plane]
        add di, bx
        mov al, dl
        stosb
        mov di, cx
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
fill_area:
        mov ax, si
        mov bx, di
        mul bl
        mov word ptr [bp-6], ax
        mov cx, 8
        or ah, ah
        jne fill_read_colours
        xor cx, cx
        dec al
fill_area_bits:
        inc cx
        shr al, 1
        jne fill_area_bits
fill_read_colours:
        push bp
        mov bx, cx
        mov bp, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp+rd_pos]
        mov dx, word ptr [bp+rd_pos+2]
        add word ptr [bp+rd_pos], cx
        adc word ptr [bp+rd_pos+2], 0
        mov cx, word ptr [bp+rd_data]
        mov es, word ptr [bp+rd_data+2]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        xor cx, cx
        mov word ptr [bp-4], ax
        or al, al
        je fill_choose
fill_index_bits:
        inc cx
        shr al, 1
        jne fill_index_bits
fill_choose:
        mov ax, word ptr [bp+4]
        mov di, ax
        add ax, word ptr [bp+8]
        mov word ptr [bp-8], ax
        mov ax, word ptr [bp+6]
        mov si, ax
        add ax, word ptr [bp+0ah]
        mov word ptr [bp-0ah], ax
        mov word ptr [bp-2], cx
        inc byte ptr [bp-4]
        mov ax, word ptr [bp-6]
        mov bx, ax
        shl bx, 1
        shl bx, 1
        shl bx, 1
        mul word ptr [bp-2]
        mov cx, word ptr [bp-4]
        shl cx, 1
        shl cx, 1
        shl cx, 1
        add ax, cx
        cmp bx, ax
        ja fill_coded
fill_raw:
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov dl, al
        mov cx, di
        mov di, word ptr DGROUP:_g_bitmaps+bm_walk
        lea bx, [di+rd_row]
        mov ax, si
        shl ax, 1
        add bx, ax
        mov bx, word ptr [bx]
        mov ax, cx
        add bx, ax
        les di, dword ptr [di+rd_plane]
        add di, bx
        mov al, dl
        stosb
        mov di, cx
        inc si
        cmp si, word ptr [bp-0ah]
        jl fill_raw
        mov si, word ptr [bp+6]
        inc di
        cmp di, word ptr [bp-8]
        jl fill_raw
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
fill_coded:
        cmp byte ptr [bp-4], 1
        jne fill_palette
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov byte ptr [bp-2], al
        mov dx, word ptr [bp+6]
fill_solid_row:
        mov si, word ptr [bp+4]
        mov cx, word ptr [bp+8]
fill_solid_pixel:
        mov di, word ptr DGROUP:_g_bitmaps+bm_walk
        lea bx, [di+rd_row]
        mov ax, dx
        shl ax, 1
        add bx, ax
        mov bx, word ptr [bx]
        mov ax, si
        add bx, ax
        les di, dword ptr [di+rd_plane]
        add di, bx
        mov al, byte ptr [bp-2]
        stosb
        inc si
        loop fill_solid_pixel
        inc dx
        dec word ptr [bp+0ah]
        jne fill_solid_row
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
fill_palette:
        lea di, [bp-10ah]
fill_palette_read:
        mov bx, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, word ptr [bx+rd_pos]
        mov dx, word ptr [bx+rd_pos+2]
        mov cx, ax
        add cx, 8
        mov word ptr [bx+rd_pos], cx
        mov cx, dx
        adc cx, 0
        mov word ptr [bx+rd_pos+2], cx
        mov cx, word ptr [bx+rd_data]
        mov es, word ptr [bx+rd_data+2]
        mov bx, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bx, ax
        mov ax, word ptr es:[bx]
        and cl, 7
        shr ax, cl
        mov bx, ds
        mov es, bx
        stosb
        dec byte ptr [bp-4]
        jne fill_palette_read
        mov di, word ptr [bp+4]
fill_indexed:
        push bp
        mov bx, word ptr [bp-2]
        mov bp, word ptr DGROUP:_g_bitmaps+bm_walk
        mov ax, 0ff00h
        mov cx, bx
        rol ax, cl
        xor ah, ah
        mov bx, ax
        mov ax, word ptr [bp+rd_pos]
        mov dx, word ptr [bp+rd_pos+2]
        add word ptr [bp+rd_pos], cx
        adc word ptr [bp+rd_pos+2], 0
        mov cx, word ptr [bp+rd_data]
        mov es, word ptr [bp+rd_data+2]
        mov bp, cx
        mov cx, ax
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        shr dx, 1
        rcr ax, 1
        add bp, ax
        mov ax, word ptr es:[bp]
        and cl, 7
        shr ax, cl
        and ax, bx
        pop bp
        lea bx, [bp-10ah]
        add bx, ax
        mov al, byte ptr [bx]
        mov dl, al
        mov cx, di
        mov di, word ptr DGROUP:_g_bitmaps+bm_walk
        lea bx, [di+rd_row]
        mov ax, si
        shl ax, 1
        add bx, ax
        mov bx, word ptr [bx]
        mov ax, cx
        add bx, ax
        les di, dword ptr [di+rd_plane]
        add di, bx
        mov al, dl
        stosb
        mov di, cx
        inc si
        cmp si, word ptr [bp-0ah]
        jl fill_indexed
        mov si, word ptr [bp+6]
        inc di
        cmp di, word ptr [bp-8]
        jge fill_done
        jmp short fill_indexed
fill_done:
        pop di
        pop si
        add sp, 10ah
        pop bp
        ret
_fill_quadrant endp
VQT_TEXT ends
end
