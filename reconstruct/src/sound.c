/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The sound module, which was written in assembly**: its state, a far
 * entry and its dispatcher, the interface to the loaded driver, the
 * sequencer, and the far entry points the C calls. This file corresponds to
 * the first module of the original's code segment 2619, image
 * 0x26198..0x28580. The module keeps its state in its own code segment
 * (`SNDS`, placed with `SEGMENT_AT`) and reaches the loaded driver through a
 * far pointer in that segment, with the function number in BP. Its routines
 * take their arguments in registers and save what they use, and its far
 * entry points, 0x2841f..0x28580, are the only ones that build a C frame.
 *
 * **So it is TASM source**, the `#ifdef __TURBOC__` block below, with the
 * host's transcription in the `#else`. It is TASM 3.0's, assembled
 * **NOSMART**: twelve `and si, 0fh` and `and cx, 0fh` are in the image with
 * a word immediate, `81`, which TASM writes only with SMART off, and none of
 * its logical ops has the short form. (Turbo C++ 1.x and 2.01 and Borland
 * C++ 2.0 write `81` too, so the encoding alone does not rule out C with
 * inline `asm`; what does is that 26 of its 42 routines use SI or DI
 * without the save every Borland compiler wraps around a function whose
 * `asm` names them - docs/lessons.md.) NOSMART also leaves a far call to a
 * routine of the same file far, and TLINK turns it into `nop / push cs /
 * call` - so here, unlike in a SMART module, that form says nothing about
 * where a file ends; the dispatcher's calls and the seek's are this file's
 * own. Where the image has a far call as a bare `push cs / call` the source
 * wrote both. Too long for Borland C++'s front end to pass through (it
 * holds some 64K of a file's `asm`), it goes to TASM directly (`JUDGE:
 * tasm`). The C that follows it in the segment is in sound_load.c,
 * sound_device.c, sound_bank.c, sound_stop.c and sound_file.c.
 *
 * Functions are in address order and each carries the image offset it was
 * read from.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler bc3.00
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it, drafted by tools/asm2tasm.py (`--nosmart`);
 * the host's transcription is the `#else`. The judge hands the block to
 * TASM 3.0 (`JUDGE: tasm`).
 */
asm {
extrn _sound_callback:far
extrn _DG4A82:byte
nosmart
SOUND_TEXT segment byte public 'CODE'
assume cs:SOUND_TEXT, ds:DGROUP
public _sound_api, _sound_api_dispatch, _install_driver, _configure_driver
public _silence_driver, _sound_hold, _driver_fn13, _seek_sequence
public _set_master_level, _set_sequence_level, _start_sequence, _retire_and_tick
public _remove_sequence, _sequencer_tick, _advance_volume_ramp, _set_sequence_volume
public _flush_pending_volumes, _sound_service, _drop_unless_polled, _poll_sequences
public _step_sequence, _midi_note_off_event, _midi_note_event, _midi_event_6
public _midi_controller_event, _midi_program_event, _midi_event_9, _midi_bend_event
public _midi_skip_event, _midi_meta_event, _skip_unknown_event, _scale_byte_pair
public _init_sequence_params, _seek_sequence_far, _set_master_level_far, _install_driver_far
public _configure_driver_far, _start_sequence_far, _driver_fn13_far, _retire_and_tick_far
public _set_sequence_level_far, _silence_driver_far
c_26198 label byte
	db 0h, 0h
c_2619a label byte
	db 0h, 0h
c_2619c label byte
	db 0h, 0h
c_2619e label byte
	db 58 dup (0h)
c_261d8 label byte
	db 0h, 0h
c_261da label byte
	db 62 dup (0h)
c_26218 label byte
	db 0h, 0h
c_2621a label byte
	db 126 dup (0h)
c_26298 label byte
	db 32 dup (0h)
c_262b8 label byte
	db 0ffh, 0ffh
c_262ba label byte
	db 0ffh, 0ffh
c_262bc label byte
	db 0ffh, 0ffh
c_262be label byte
	db 0ffh, 0ffh
c_262c0 label byte
	db 0ffh, 0ffh
c_262c2 label byte
	db 0ffh, 0ffh
c_262c4 label byte
	db 0ffh, 0ffh
c_262c6 label byte
	db 0ffh, 0ffh
c_262c8 label byte
	db 0h, 0h
c_262ca label byte
	db 0h, 0h
c_262cc label byte
	db 0h, 0h
c_262ce label byte
	db 0h, 0h
c_262d0 label byte
	db 0h, 0h
c_262d2 label byte
	db 0h, 0h
c_262d4 label byte
	db 0h, 0h
c_262d6 label byte
	db 0h, 0h
c_262d8 label byte
	db 0h, 0h
c_262da label byte
	db 0h, 0h
c_262dc label byte
	db 0h, 0h
c_262de label byte
	db 0h, 0h
c_262e0 label byte
	db 0h, 0h
c_262e2 label byte
	db 0h, 0h
c_262e4 label byte
	db 0h, 0h
c_262e6 label byte
	db 0h, 0h
c_262e8 label byte
	db 0h, 0h
c_262ea label byte
	db 0h, 0h
c_262ec label byte
	db 0h, 0h
c_262ee label byte
	db 0h, 0h
c_262f0 label byte
	db 0h, 0h
c_262f2 label byte
	db 0h, 0h
c_262f4 label byte
	db 0h, 0h
c_262f6 label byte
	db 0h, 0h
c_262f8 label byte
	db 0ffh, 0ffh
c_262fa label byte
	db 0ffh, 0ffh
c_262fc label byte
	db 0ffh, 0ffh
c_262fe label byte
	db 0ffh, 0ffh
c_26300 label byte
	db 0ffh, 0ffh
c_26302 label byte
	db 0ffh, 0ffh
c_26304 label byte
	db 0ffh, 0ffh
c_26306 label byte
	db 0ffh, 0ffh
c_26308 label byte
	db 0h, 0h
c_2630a label byte
	db 0h, 0h
c_2630c label byte
	db 0h, 0h
c_2630e label byte
	db 0h, 0h
c_26310 label byte
	db 0h, 0h
c_26312 label byte
	db 0h, 0h
c_26314 label byte
	db 0h, 0h
c_26316 label byte
	db 0h, 0h
c_26318 label byte
	db 0h, 0h
c_2631a label byte
	db 0h, 0h
c_2631c label byte
	db 0h, 0h
c_2631e label byte
	db 0h, 0h
c_26320 label byte
	db 0h, 0h
c_26322 label byte
	db 0h, 0h
c_26324 label byte
	db 0h, 0h
c_26326 label byte
	db 0h, 0h
c_26328 label byte
	db 0h, 0h
c_2632a label byte
	db 0h, 0h
c_2632c label byte
	db 0h, 0h
c_2632e label byte
	db 0h, 0h
c_26330 label byte
	db 0h, 0h
c_26332 label byte
	db 0h, 0h
c_26334 label byte
	db 0h, 0h
c_26336 label byte
	db 0h, 0h
c_26338 label byte
	db 0ffh, 0ffh
c_2633a label byte
	db 0ffh, 0ffh
c_2633c label byte
	db 0ffh, 0ffh
c_2633e label byte
	db 0ffh, 0ffh
c_26340 label byte
	db 0ffh, 0ffh
c_26342 label byte
	db 0ffh, 0ffh
c_26344 label byte
	db 0ffh, 0ffh
c_26346 label byte
	db 0ffh, 0ffh
c_26348 label byte
	db 0fh, 0fh
c_2634a label byte
	db 0fh, 0fh
c_2634c label byte
	db 0fh, 0fh
c_2634e label byte
	db 0fh, 0fh
c_26350 label byte
	db 0fh, 0fh
c_26352 label byte
	db 0fh, 0fh
c_26354 label byte
	db 0fh, 0fh
c_26356 label byte
	db 0fh, 0fh
c_26358 label byte
	db 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh
c_26368 label byte
	db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
c_26377 label byte
	db 0h, 0h
c_26379 label byte
	db 0h, 0h
c_2637b label byte
	db 0h, 0h
c_2637d label byte
	db 0h, 0h
c_2637f label byte
	db 0h, 0h
c_26381 label byte
	db 0h, 0h
c_26383 label byte
	db 0h, 0h
c_26385 label byte
	db 0h, 0h
c_26387 label byte
	db 0h, 0h
c_26389 label byte
	db 0h
c_2638a label byte
	db 0h
c_2638b label byte
	db 0fh
c_2638c label byte
	db 0h
c_2638d label byte
	db 0ffh
c_2638e label byte
	db 0h
c_2638f label byte
	db 0h
c_26390 label byte
	db 0h
c_26391 label byte
	db 0h
c_26392 label byte
	db 0h
c_26393 label byte
	db 0h
c_26394 label byte
	db 0h
c_26395 label byte
	db 0h
c_26396 label byte
	db 0h, 0h, 0h
c_26399 label byte
	db 0h, 0h, 0h
c_2639c label byte
	db 0h

/* 0x2639d */
_sound_api proc far
	push bp
	mov bp, sp
	push es
	push ds
	push di
	push si
	les ax, dword ptr [bp+8]
	mov cx, [bp+0ch]
	mov word ptr cs:c_2637b, cx
	mov cx, [bp+0eh]
	mov word ptr cs:c_2637d, cx
	mov cx, [bp+10h]
	mov word ptr cs:c_2637f, cx
	mov cx, [bp+12h]
	mov word ptr cs:c_26381, cx
	mov cx, [bp+14h]
	mov word ptr cs:c_26383, cx
	mov cx, [bp+16h]
	mov word ptr cs:c_26385, cx
	mov cx, [bp+6]
	cmp cl, 6
	je L263ee
	cmp cl, 3
	je L263ee
	cmp cl, 4
	je L263ee
	cmp cl, 7
	jne L263f6
L263ee:
	mov dx, [bp+8]
	mov word ptr cs:c_2637b, dx
L263f6:
	call _sound_api_dispatch
	pop si
	pop di
	pop ds
	pop es
	pop bp
	retf
_sound_api endp

/* 0x263ff */
_sound_api_dispatch proc near
	cmp cl, 0
	jne L2643b
	call FAR PTR _install_driver
	xor ah, ah
	cmp al, 0ffh
	jne L26411
	mov ah, al
L26411:
	xor cx, cx
	mov cl, byte ptr cs:c_2638f
	les bx, dword ptr cs:c_2637b
	mov es:[bx], cx
	mov cl, byte ptr cs:c_26390
	les bx, dword ptr cs:c_2637f
	mov es:[bx], cx
	mov cl, byte ptr cs:c_2638c
	les bx, dword ptr cs:c_26383
	mov es:[bx], cx
	ret
L2643b:
	cmp cl, 1
	jne L2644b
	mov bx, word ptr cs:c_2637b
	call FAR PTR _configure_driver
	ret
L2644b:
	cmp cl, 2
	jne L26456
	call FAR PTR _silence_driver
	ret
L26456:
	cmp cl, 3
	jne L26466
	mov cx, word ptr cs:c_2637b
	call FAR PTR _sound_hold
	ret
L26466:
	cmp cl, 4
	jne L26476
	mov cx, word ptr cs:c_2637b
	call FAR PTR _driver_fn13
	ret
L26476:
	cmp cl, 5
	jne L26481
	call FAR PTR _seek_sequence
	ret
L26481:
	cmp cl, 6
	jne L26491
	mov cx, word ptr cs:c_2637b
	call FAR PTR _set_master_level
	ret
L26491:
	cmp cl, 7
	jne L264a1
	mov cx, word ptr cs:c_2637b
	call FAR PTR _set_sequence_level
	ret
L264a1:
	cmp cl, 8
	jne L264b1
	mov cx, word ptr cs:c_2637b
	call FAR PTR _start_sequence
	ret
L264b1:
	cmp cl, 9
	jne L264bc
	call FAR PTR _retire_and_tick
	ret
L264bc:
	cmp cl, 0ah
	jne L264cc
	mov cl, byte ptr cs:c_2637b
	call FAR PTR L26a61
	ret
L264cc:
	cmp cl, 0bh
	jne L264e6
	mov dl, byte ptr cs:c_2637b
	mov cl, byte ptr cs:c_2637d
	mov ch, byte ptr cs:c_2637f
	call FAR PTR L26ac9
	ret
L264e6:
	cmp cl, 0ch
	jne L264f6
	mov cl, byte ptr cs:c_2637b
	call FAR PTR L26aed
	ret
L264f6:
	cmp cl, 0dh
	jne L26506
	mov cx, word ptr cs:c_2637b
	call FAR PTR L26af7
	ret
L26506:
	cmp cl, 0eh
	jne L26516
	mov cx, word ptr cs:c_2637b
	call FAR PTR L26b34
	ret
L26516:
	cmp cl, 0fh
	jne L26525
	mov cx, word ptr cs:c_2637b
	call FAR PTR L26b44
L26525:
	cmp cl, 11h
	jne L26531
	call FAR PTR L26c18
	mov ax, cx
L26531:
	cmp cl, 10h
	jne L26540
	call FAR PTR L26c02
	mov al, cl
	xor ah, ah
	ret
L26540:
	cmp cl, 19h
	jne L2654f
	call FAR PTR L26c0c
	mov al, cl
	xor ah, ah
	ret
L2654f:
	cmp cl, 12h
	jne L26579
	call FAR PTR L26c22
	xor ah, ah
	les bx, dword ptr cs:c_2637b
	mov al, dl
	mov es:[bx], ax
	les bx, dword ptr cs:c_2637f
	mov al, ch
	mov es:[bx], ax
	les bx, dword ptr cs:c_26383
	mov al, cl
	mov es:[bx], ax
L26579:
	cmp cl, 13h
	jne L26593
	mov dl, byte ptr cs:c_2637b
	mov ch, byte ptr cs:c_2637d
	mov cl, byte ptr cs:c_2637f
	call FAR PTR L26c43
	ret
L26593:
	cmp cl, 14h
	jne L265ad
	mov dl, byte ptr cs:c_2637b
	mov ch, byte ptr cs:c_2637d
	mov cl, byte ptr cs:c_2637f
	call FAR PTR L26c95
	ret
L265ad:
	cmp cl, 15h
	jne L265c7
	mov dl, byte ptr cs:c_2637b
	mov ch, byte ptr cs:c_2637d
	mov cl, byte ptr cs:c_2637f
	call FAR PTR L26ce6
	ret
L265c7:
	cmp cl, 16h
	jne L265dc
	mov dl, byte ptr cs:c_2637b
	mov cl, byte ptr cs:c_2637d
	call FAR PTR L26dc4
	ret
L265dc:
	cmp cl, 17h
	jne L265f1
	mov dl, byte ptr cs:c_2637b
	mov cx, word ptr cs:c_2637d
	call FAR PTR L26e15
	ret
L265f1:
	ret
_sound_api_dispatch endp

/* 0x265f2 */
_install_driver proc far
	mov word ptr cs:c_26377, ax
	mov word ptr cs:c_26379, es
	push bp
	mov bp, 0
	call dword ptr cs:c_26377
	pop bp
	mov byte ptr cs:c_2638f, cl
	mov byte ptr cs:c_2638c, ch
	mov dl, ah
	shr dl, 1
	shr dl, 1
	shr dl, 1
	shr dl, 1
	cmp word ptr DGROUP:_DG4A82+28h, 0
	je L26623
	or dl, 1
L26623:
	mov byte ptr cs:c_26390, dl
	retf
_install_driver endp

/* 0x26629 */
_configure_driver proc far
	push cx
	push bp
	mov bp, 1
	call dword ptr cs:c_26377
	pop bp
	mov byte ptr cs:c_2638a, cl
	mov byte ptr cs:c_2638b, ch
	push ax
	xor cl, cl
	push bp
	mov bp, 0bh
	call dword ptr cs:c_26377
	pop bp
	pop ax
	pop cx
	retf
_configure_driver endp

/* 0x2664e */
_silence_driver proc far
	push bx
	push cx
	push es
	push si
	mov cl, 0fh
	push bp
	mov bp, 0ch
	call dword ptr cs:c_26377
	pop bp
	push bp
	mov bp, 2
	call dword ptr cs:c_26377
	pop bp
	pop si
	pop es
	pop cx
	pop bx
	retf
_silence_driver endp

/* 0x2666d */
_sound_hold proc far
	cmp cx, 0
	jne L26678
	inc byte ptr cs:c_26389
	retf
L26678:
	cmp byte ptr cs:c_26389, 0
	je L26685
	dec byte ptr cs:c_26389
L26685:
	retf
_sound_hold endp

/* 0x26686 */
_driver_fn13 proc far
	push bp
	mov bp, 0dh
	call dword ptr cs:c_26377
	pop bp
	retf
_driver_fn13 endp

/* 0x26691 */
_seek_sequence proc far
	pushf
	cli
	push si
	push ax
	push bx
	push cx
	push dx
	mov bx, ax
	xor ch, ch
	mov cl, es:[bx+159h]
	dec cl
	mov si, 0eh
L266a6:
	mov dl, es:[bx+si+143h]
	and dl, 0f0h
	mov byte ptr cs:c_26368[si], dl
	dec si
	jns L266a6
	mov byte ptr cs:c_26399, 1
	call FAR PTR _start_sequence
	mov cx, es:[bx+154h]
	mov word ptr es:[bx+154h], 0
	mov al, es:[bx+15dh]
	mov byte ptr es:[bx+15dh], 1
	cmp cx, 0
	je L266fc
L266dd:
	mov dx, es:[bx+154h]
	call _step_sequence
	cmp dx, es:[bx+154h]
	jb L266f5
	je L266fc
	sub dx, es:[bx+154h]
	sub cx, dx
L266f5:
	cmp cx, es:[bx+154h]
	jne L266dd
L266fc:
	mov es:[bx+15dh], al
	mov byte ptr cs:c_26399, 0
	mov si, 0eh
L2670a:
	mov dl, byte ptr cs:c_26368[si]
	or es:[bx+si+143h], dl
	dec si
	jns L2670a
	call _sequencer_tick
	pop dx
	pop cx
	pop bx
	pop ax
	pop si
	popf
	retf
_seek_sequence endp

/* 0x26721 */
_set_master_level proc far
	cmp cl, 0ffh
	je L2672d
	cmp cl, 0fh
	jbe L2672d
	mov cl, 0fh
L2672d:
	push bp
	mov bp, 0ch
	call dword ptr cs:c_26377
	pop bp
	retf
_set_master_level endp

/* 0x26738 */
_set_sequence_level proc far
	cmp cl, 0ffh
	jne L26748
	push bp
	mov bp, 0bh
	call dword ptr cs:c_26377
	pop bp
	retf
L26748:
	cmp cl, 0ah
	jbe L26754
	mov al, byte ptr cs:c_26392
	xor ah, ah
	retf
L26754:
	mov al, byte ptr cs:c_26392
	push es
	push ax
	push bx
	push dx
	mov byte ptr cs:c_26392, cl
	les bx, dword ptr cs:c_26198
	mov dx, es
	or dx, bx
	je L2677e
	cmp byte ptr es:[bx+15fh], 7fh
	jne L2677e
	push bp
	mov bp, 0bh
	call dword ptr cs:c_26377
	pop bp
L2677e:
	pop dx
	pop bx
	pop ax
	pop es
	retf
_set_sequence_level endp

/* 0x26783 */
_start_sequence proc far
	pushf
	cli
	push cx
	push cx
	mov cx, es
	xor di, di
L2678b:
	cmp word ptr cs:c_26198[di], ax
	jne L267a1
	cmp word ptr cs:c_2619a[di], cx
	jne L267a1
	call _remove_sequence
	call _sequencer_tick
	jmp short L267a9
L267a1:
	add di, 4
	cmp di, 40h
	jne L2678b
L267a9:
	pop cx
	mov bx, ax
	mov byte ptr es:[bx+159h], 1
	cmp cx, 0
	je L267bc
	inc byte ptr es:[bx+159h]
L267bc:
	call _init_sequence_params
	mov bx, ax
	xor si, si
	xor cx, cx
	dec ch
L267c7:
	shl si, 1
	mov word ptr es:[bx+si+0ch], 0dh
	mov word ptr es:[bx+si+2ch], 3
	mov word ptr es:[bx+si+4ch], 0
	mov word ptr es:[bx+si+6ch], 0
	mov word ptr es:[bx+si+0bch], 2000h
	shr si, 1
	mov es:[bx+si+8ch], ch
	mov es:[bx+si+9ch], cl
	mov es:[bx+si+0ach], cl
	mov es:[bx+si+0dah], ch
	mov es:[bx+si+0e9h], cl
	mov es:[bx+si+116h], ch
	mov es:[bx+si+107h], ch
	mov es:[bx+si+0f8h], ch
	mov es:[bx+si+125h], ch
	mov es:[bx+si+134h], cl
	mov es:[bx+si+143h], cl
	inc si
	cmp si, 0fh
	jne L267c7
	mov es:[bx+si+8ch], ch
	mov es:[bx+si+9ch], cl
	mov es:[bx+si+0ach], cl
	mov es:[bx+165h], cl
	mov es:[bx+15ah], cl
	mov byte ptr es:[bx+15fh], 7fh
	shl si, 1
	mov word ptr es:[bx+si+0ch], 0dh
	mov word ptr es:[bx+si+2ch], 3
	mov word ptr es:[bx+si+4ch], 0
	mov word ptr es:[bx+156h], 0
	push ax
	push bp
	push ds
	xor si, si
	lds bp, dword ptr es:[bx+8]
	lds bp, dword ptr ds:[bp]
	cmp byte ptr ds:[bp+20h], 0ffh
	je L26886
	cmp byte ptr es:[bx+15bh], 0
	jne L26886
	mov cl, ds:[bp+20h]
	mov es:[bx+15ch], cl
L26886:
	mov cx, bp
L26888:
	mov dx, ds:[bp]
	cmp dx, 0
	je L268c7
	mov bp, cx
	add bp, dx
	mov dl, ds:[bp]
	cmp dl, 0feh
	jne L268ca
	cmp byte ptr cs:c_26390, 0
	jne L268bf
	shl si, 1
	mov word ptr es:[bx+si+0ch], 0
	mov word ptr es:[bx+si+2ch], 0
	shr si, 1
	mov byte ptr es:[bx+si+8ch], 0feh
	jmp L2699a
L268bf:
	mov dx, si
	inc dx
	mov es:[bx+165h], dl
L268c7:
	jmp L269ab
L268ca:
	mov es:[bx+si+8ch], dl
	mov dh, dl
	or dh, 0b0h
	mov es:[bx+si+9ch], dh
	mov dl, ds:[bp+0ch]
	xor dh, dh
	cmp dl, 0f8h
	jne L268e8
	mov dl, 0f0h
	mov dh, 80h
L268e8:
	shl si, 1
	mov es:[bx+si+4ch], dx
	shr si, 1
	push si
	mov dl, es:[bx+si+8ch]
	and byte ptr es:[bx+si+8ch], 0fh
	test dl, 10h
	je L2691f
	shl si, 1
	mov word ptr es:[bx+si+0ch], 3
	mov word ptr es:[bx+si+4ch], 0
	xor dh, dh
	mov si, dx
	and si, 0fh
	or byte ptr es:[bx+si+134h], 2
	jmp short L26999
L2691f:
	xor dh, dh
	mov si, dx
	and si, 0fh
	test dl, 20h
	je L26932
	or byte ptr es:[bx+si+134h], 1
L26932:
	test dl, 40h
	je L2693d
	mov byte ptr es:[bx+si+143h], 1
L2693d:
	cmp si, 0fh
	jne L26955
	cmp byte ptr es:[bx+15fh], 7fh
	jne L26988
	mov al, ds:[bp+8]
	mov es:[bx+15fh], al
	jmp short L26999
L26955:
	cmp byte ptr es:[bx+si+0dah], 0ffh
	jne L26966
	mov al, ds:[bp+1]
	mov es:[bx+si+0dah], al
L26966:
	cmp byte ptr es:[bx+si+116h], 0ffh
	jne L26977
	mov al, ds:[bp+4]
	mov es:[bx+si+116h], al
L26977:
	cmp byte ptr es:[bx+si+107h], 0ffh
	jne L26988
	mov al, ds:[bp+8]
	mov es:[bx+si+107h], al
L26988:
	cmp byte ptr es:[bx+si+0f8h], 0ffh
	jne L26999
	mov al, ds:[bp+0bh]
	mov es:[bx+si+0f8h], al
L26999:
	pop si
L2699a:
	inc si
	shl si, 1
	mov bp, cx
	add bp, si
	shr si, 1
	cmp si, 10h
	je L269ab
	jmp L26888
L269ab:
	pop ds
	pop bp
	pop ax
	cmp byte ptr es:[bx+159h], 2
	jne L269c2
	mov di, 0eh
L269b9:
	or byte ptr es:[bx+di+134h], 1
	dec di
	jns L269b9
L269c2:
	mov ax, bx
	mov dl, es:[bx+15ch]
	push es
	xor di, di
L269cc:
	cmp word ptr cs:c_2619a[di], 0
	je L26a10
	les bx, dword ptr cs:c_26198[di]
	cmp es:[bx+15ch], dl
	jbe L269eb
	add di, 4
	cmp di, 40h
	jne L269cc
	pop es
	jmp short L26a54
L269eb:
	mov si, 38h
L269ee:
	mov bx, si
	add bx, 4
	cmp bx, di
	je L26a10
	mov bx, word ptr cs:c_26198[si]
	mov word ptr cs:c_2619c[si], bx
	mov bx, word ptr cs:c_2619a[si]
	mov word ptr cs:c_2619e[si], bx
	sub si, 4
	jmp short L269ee
L26a10:
	pop es
	mov bx, ax
	mov ax, es
	mov word ptr cs:c_26198[di], bx
	mov word ptr cs:c_2619a[di], ax
	cmp byte ptr cs:c_26399, 0
	jne L26a54
	xor cx, cx
	mov es:[bx+152h], cx
	mov es:[bx+154h], cx
	mov es:[bx+158h], cl
	mov es:[bx+160h], cl
	mov es:[bx+161h], cl
	mov es:[bx+162h], cl
	mov es:[bx+163h], cl
	mov es:[bx+164h], cl
	call _sequencer_tick
L26a54:
	pop cx
	popf
	retf
_start_sequence endp

/* 0x26a57 */
_retire_and_tick proc far
	pushf
	cli
	call _remove_sequence
	call _sequencer_tick
	popf
	retf
L26a61:
	pushf
	cli
	push ax
	push bx
	push si
	mov bx, es
	or bx, ax
	jne L26aa6
	xor si, si
L26a6e:
	les bx, dword ptr cs:c_26198[si]
	mov ax, es
	or ax, bx
	jne L26a83
	cmp si, 0
	jne L26ac1
	mov si, 4
	jmp short L26a6e
L26a83:
	mov al, es:[bx+164h]
	cmp cl, 0
	je L26a91
	inc al
	jmp short L26a97
L26a91:
	cmp al, 0
	je L26a97
	dec al
L26a97:
	mov es:[bx+164h], al
	add si, 4
	cmp si, 40h
	jne L26a6e
	jmp short L26ac1
L26aa6:
	mov bx, ax
	mov al, es:[bx+164h]
	cmp cl, 0
	je L26ab6
	inc al
	jmp short L26abc
L26ab6:
	cmp al, 0
	je L26abc
	dec al
L26abc:
	mov es:[bx+164h], al
L26ac1:
	call _sequencer_tick
	pop si
	pop bx
	pop ax
	popf
	retf
L26ac9:
	pushf
	cli
	push bx
	mov bx, ax
	cmp es:[bx+15eh], dl
	je L26aea
	mov es:[bx+160h], dl
	mov es:[bx+161h], cl
	mov es:[bx+163h], ch
	mov byte ptr es:[bx+162h], 0
L26aea:
	pop bx
	popf
	retf
L26aed:
	push bx
	mov bx, ax
	mov es:[bx+15ah], cl
	pop bx
	retf
L26af7:
	push bx
	push dx
	push si
	inc byte ptr cs:c_26389
	mov bx, ax
	mov si, 0eh
L26b04:
	mov dl, es:[bx+si+143h]
	cmp cx, 0
	jne L26b18
	cmp dl, 0fh
	jbe L26b20
	sub dl, 10h
	jmp short L26b20
L26b18:
	cmp dl, 0f0h
	jae L26b20
	add dl, 10h
L26b20:
	mov es:[bx+si+143h], dl
	dec si
	jns L26b04
	call _sequencer_tick
	dec byte ptr cs:c_26389
	pop si
	pop dx
	pop bx
	retf
L26b34:
	pushf
	cli
	push bx
	mov bx, ax
	call L282e3
	xor ch, ch
	call _set_sequence_volume
	pop bx
	popf
	retf
L26b44:
	pushf
	cli
	push bx
	push cx
	push dx
	push si
	push di
	mov bx, ax
	cmp es:[bx+15ch], cl
	jne L26b57
	jmp L26bfb
L26b57:
	mov es:[bx+15ch], cl
	call L282e3
	cmp si, 0ffh
	jne L26b68
	jmp L26bfb
L26b68:
	mov word ptr cs:c_26198[si], 0
	mov word ptr cs:c_2619a[si], 0
	cmp si, 3ch
	je L26ba5
L26b7b:
	mov cx, word ptr cs:c_2619c[si]
	mov word ptr cs:c_26198[si], cx
	mov cx, word ptr cs:c_2619e[si]
	mov word ptr cs:c_2619a[si], cx
	add si, 4
	cmp si, 3ch
	jne L26b7b
	mov word ptr cs:c_26198[si], 0
	mov word ptr cs:c_2619a[si], 0
L26ba5:
	mov dl, es:[bx+15ch]
	push es
	xor di, di
L26bad:
	cmp word ptr cs:c_2619a[di], 0
	je L26beb
	les bx, dword ptr cs:c_26198[di]
	mov es:[bx+15ch], dl
	jbe L26bc6
	add di, 4
	jmp short L26bad
L26bc6:
	mov si, 38h
L26bc9:
	mov bx, si
	add bx, 4
	cmp bx, di
	je L26beb
	mov bx, word ptr cs:c_26198[si]
	mov word ptr cs:c_2619c[si], bx
	mov bx, word ptr cs:c_2619a[si]
	mov word ptr cs:c_2619e[si], bx
	sub si, 4
	jmp short L26bc9
L26beb:
	pop es
	mov word ptr cs:c_26198[di], ax
	mov cx, es
	mov word ptr cs:c_2619a[di], cx
	call _sequencer_tick
L26bfb:
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	popf
	retf
L26c02:
	push bx
	mov bx, ax
	mov cl, es:[bx+158h]
	pop bx
	retf
L26c0c:
	push bx
	mov bx, ax
	xor cl, cl
	xchg es:[bx+158h], cl
	pop bx
	retf
L26c18:
	push bx
	mov bx, ax
	mov cx, es:[bx+152h]
	pop bx
	retf
L26c22:
	push ax
	push bx
	mov bx, ax
	mov ax, es:[bx+154h]
	xor dx, dx
	mov cx, 0e10h
	div cx
	push ax
	mov ax, dx
	mov cl, 3ch
	div cl
	mov cl, ah
	shr cl, 1
	mov ch, al
	pop dx
	pop bx
	pop ax
	retf
L26c43:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:c_26389
	mov bx, ax
	call L282e3
	cmp si, 0ffh
	je L26c8b
	push si
	xor dh, dh
	mov si, dx
	mov byte ptr es:[bx+si+125h], 0ffh
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
	jmp short L26c78
c_26c71 label byte
	db 90h
L26c72:
	inc si
	cmp si, 10h
	je L26c8b
L26c78:
	cmp byte ptr cs:c_262b8[si], dl
	jne L26c72
	mov ax, si
	push bp
	mov bp, 4
	call dword ptr cs:c_26377
	pop bp
L26c8b:
	dec byte ptr cs:c_26389
	pop dx
	pop bx
	pop ax
	pop si
	retf
L26c95:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:c_26389
	mov bx, ax
	call L282e3
	cmp si, 0ffh
	je L26cdc
	push si
	xor dh, dh
	mov si, dx
	mov es:[bx+si+125h], ch
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
	jmp short L26cc9
c_26cc2 label byte
	db 90h
L26cc3:
	inc si
	cmp si, 10h
	je L26cdc
L26cc9:
	cmp byte ptr cs:c_262b8[si], dl
	jne L26cc3
	mov ax, si
	push bp
	mov bp, 5
	call dword ptr cs:c_26377
	pop bp
L26cdc:
	dec byte ptr cs:c_26389
	pop dx
	pop bx
	pop ax
	pop si
	retf
L26ce6:
	pushf
	cli
	push ax
	push bx
	push cx
	push dx
	push si
	mov bx, ax
	call L282e3
	cmp si, 0ffh
	jne L26cfb
	jmp L26dbd
L26cfb:
	push si
	xor dh, dh
	mov si, dx
	cmp ch, 7
	jne L26d14
	mov es:[bx+si+107h], cl
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	jmp short L26d84
L26d14:
	cmp ch, 0ah
	jne L26d20
	mov es:[bx+si+0f8h], cl
	jmp short L26d84
L26d20:
	cmp ch, 1
	jne L26d2c
	mov es:[bx+si+0e9h], cl
	jmp short L26d84
L26d2c:
	cmp ch, 40h
	jne L26d4c
	shl si, 1
	mov ax, es:[bx+si+0bch]
	and ah, 7fh
	cmp cl, 0
	je L26d43
	or ah, 80h
L26d43:
	mov es:[bx+si+0bch], ax
	shr si, 1
	jmp short L26d84
L26d4c:
	cmp ch, 4eh
	jne L26d7a
	push dx
	mov dl, es:[bx+si+143h]
	cmp cl, 0
	jne L26d66
	cmp dl, 0fh
	jbe L26d76
	sub dl, 10h
	jmp short L26d6e
L26d66:
	cmp dl, 0f0h
	jae L26d76
	add dl, 10h
L26d6e:
	mov es:[bx+si+143h], dl
	call _sequencer_tick
L26d76:
	pop dx
	pop si
	jmp short L26dbd
L26d7a:
	cmp ch, 7fh
	jne L26d84
	mov es:[bx+si+116h], cl
L26d84:
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
L26d91:
	cmp byte ptr cs:c_262b8[si], dl
	je L26da0
	inc si
	cmp si, 10h
	jne L26d91
	jmp short L26dbd
L26da0:
	mov ax, si
	cmp ch, 7fh
	jne L26db3
	push bp
	mov bp, 8
	call dword ptr cs:c_26377
	pop bp
	jmp short L26dbd
L26db3:
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
L26dbd:
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	popf
	retf
L26dc4:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:c_26389
	mov bx, ax
	call L282e3
	cmp si, 0ffh
	je L26e0b
	push si
	xor dh, dh
	mov si, dx
	mov es:[bx+si+116h], cl
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
	jmp short L26df8
c_26df1 label byte
	db 90h
L26df2:
	inc si
	cmp si, 10h
	je L26e0b
L26df8:
	cmp byte ptr cs:c_262b8[si], dl
	jne L26df2
	mov ax, si
	push bp
	mov bp, 8
	call dword ptr cs:c_26377
	pop bp
L26e0b:
	dec byte ptr cs:c_26389
	pop dx
	pop bx
	pop ax
	pop si
	retf
L26e15:
	push ax
	push bx
	push cx
	push dx
	push si
	mov bx, ax
	call L282e3
	cmp si, 0ffh
	je L26e75
	push si
	xor dh, dh
	mov si, dx
	shl si, 1
	mov ax, cx
	cmp byte ptr es:[bx+si+0bdh], 80h
	jb L26e39
	or ah, 80h
L26e39:
	mov es:[bx+si+0bch], ax
	shr si, 1
	mov dx, si
	pop si
	shl si, 1
	shl si, 1
	or dx, si
	xor si, si
L26e4b:
	cmp byte ptr cs:c_262b8[si], dl
	je L26e5a
	inc si
	cmp si, 10h
	jne L26e4b
	jmp short L26e75
L26e5a:
	shl ch, 1
	cmp cl, 80h
	jb L26e64
	or ch, 1
L26e64:
	and cl, 7fh
	xchg ch, cl
	mov ax, si
	push bp
	mov bp, 0ah
	call dword ptr cs:c_26377
	pop bp
L26e75:
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	retf
_retire_and_tick endp

/* 0x26e7b */
_remove_sequence proc near
	push si
	push es
	push ax
	push bx
	push ds
	push bp
	xor si, si
	mov cx, es
L26e85:
	cmp ax, word ptr cs:c_26198[si]
	jne L26e93
	cmp cx, word ptr cs:c_2619a[si]
	je L26e9e
L26e93:
	add si, 4
	cmp si, 40h
	jne L26e85
	jmp L26f23
L26e9e:
	mov word ptr cs:c_26198[si], 0
	mov word ptr cs:c_2619a[si], 0
	cmp si, 3ch
	je L26edb
L26eb1:
	mov cx, word ptr cs:c_2619c[si]
	mov word ptr cs:c_26198[si], cx
	mov cx, word ptr cs:c_2619e[si]
	mov word ptr cs:c_2619a[si], cx
	add si, 4
	cmp si, 3ch
	jne L26eb1
	mov word ptr cs:c_26198[si], 0
	mov word ptr cs:c_2619a[si], 0
L26edb:
	mov bx, ax
	mov byte ptr es:[bx+158h], 0ffh
	mov byte ptr es:[bx+159h], 0
	cmp byte ptr es:[bx+165h], 0
	je L26f23
	lds bp, dword ptr es:[bx+8]
	lds bp, dword ptr ds:[bp]
	mov al, es:[bx+165h]
	cmp al, 80h
	jb L26f23
	and ax, 0fh
	dec ax
	shl ax, 1
	push bp
	add bp, ax
	mov ax, ds:[bp]
	pop bp
	add bp, ax
	mov ax, bp
	xor ax, ax
	push ax
	mov ax, 5
	push ax
	call FAR PTR _sound_callback
	add sp, 4
L26f23:
	pop bp
	pop ds
	pop bx
	pop ax
	pop es
	pop si
	ret
_remove_sequence endp

/* 0x26f2a */
_sequencer_tick proc near
	push ax
	push bx
	push cx
	push dx
	push di
	push si
	push bp
	push es
	inc byte ptr cs:c_26389
	mov byte ptr cs:c_26394, 0
	xor ax, ax
	mov bx, 0ffffh
	mov word ptr cs:c_262b8, bx
	mov word ptr cs:c_262ba, bx
	mov word ptr cs:c_262bc, bx
	mov word ptr cs:c_262be, bx
	mov word ptr cs:c_262c0, bx
	mov word ptr cs:c_262c2, bx
	mov word ptr cs:c_262c4, bx
	mov word ptr cs:c_262c6, bx
	mov word ptr cs:c_262e8, ax
	mov word ptr cs:c_262ea, ax
	mov word ptr cs:c_262ec, ax
	mov word ptr cs:c_262ee, ax
	mov word ptr cs:c_262f0, ax
	mov word ptr cs:c_262f2, ax
	mov word ptr cs:c_262f4, ax
	mov word ptr cs:c_262f6, ax
	mov word ptr cs:c_262c8, ax
	mov word ptr cs:c_262ca, ax
	mov word ptr cs:c_262cc, ax
	mov word ptr cs:c_262ce, ax
	mov word ptr cs:c_262d0, ax
	mov word ptr cs:c_262d2, ax
	mov word ptr cs:c_262d4, ax
	mov word ptr cs:c_262d6, ax
	mov word ptr cs:c_262d8, ax
	mov word ptr cs:c_262da, ax
	mov word ptr cs:c_262dc, ax
	mov word ptr cs:c_262de, ax
	mov word ptr cs:c_262e0, ax
	mov word ptr cs:c_262e2, ax
	mov word ptr cs:c_262e4, ax
	mov word ptr cs:c_262e6, ax
	mov word ptr cs:c_262f8, bx
	mov word ptr cs:c_262fa, bx
	mov word ptr cs:c_262fc, bx
	mov word ptr cs:c_262fe, bx
	mov word ptr cs:c_26300, bx
	mov word ptr cs:c_26302, bx
	mov word ptr cs:c_26304, bx
	mov word ptr cs:c_26306, bx
	mov word ptr cs:c_261d8, ax
	mov word ptr cs:c_261da, ax
	les bx, dword ptr cs:c_26198
	mov dx, es
	or dx, bx
	jne L27033
	mov dx, 0ffffh
	mov word ptr cs:c_262b8, dx
	mov word ptr cs:c_262ba, dx
	mov word ptr cs:c_262bc, dx
	mov word ptr cs:c_262be, dx
	mov word ptr cs:c_262c0, dx
	mov word ptr cs:c_262c2, dx
	mov word ptr cs:c_262c4, dx
	mov word ptr cs:c_262c6, dx
	jmp L27801
L27033:
	mov cl, es:[bx+15fh]
	cmp cl, 7fh
	jne L27042
	mov cl, byte ptr cs:c_26392
L27042:
	push bp
	mov bp, 0bh
	call dword ptr cs:c_26377
	pop bp
	xor bp, bp
	xor si, si
	mov al, byte ptr cs:c_2638f
L27054:
	les bx, dword ptr cs:c_26198[si]
	mov dx, es
	or dx, bx
	jne L27062
	jmp L2754c
L27062:
	les bx, dword ptr cs:c_26198[si]
	cmp byte ptr es:[bx+164h], 0
	je L27072
	jmp L2753e
L27072:
	cmp byte ptr es:[bx+165h], 0
	je L2709f
	cmp word ptr cs:c_261d8, 0
	je L27085
	jmp L2753e
L27085:
	cmp word ptr cs:c_261da, 0
	je L27090
	jmp L2753e
L27090:
	mov word ptr cs:c_261d8, bx
	mov bx, es
	mov word ptr cs:c_261da, bx
	jmp L2753e
L2709f:
	push ax
	mov ax, word ptr cs:c_262f8
	mov word ptr cs:c_26338, ax
	mov ax, word ptr cs:c_262fa
	mov word ptr cs:c_2633a, ax
	mov ax, word ptr cs:c_262fc
	mov word ptr cs:c_2633c, ax
	mov ax, word ptr cs:c_262fe
	mov word ptr cs:c_2633e, ax
	mov ax, word ptr cs:c_26300
	mov word ptr cs:c_26340, ax
	mov ax, word ptr cs:c_26302
	mov word ptr cs:c_26342, ax
	mov ax, word ptr cs:c_26304
	mov word ptr cs:c_26344, ax
	mov ax, word ptr cs:c_26306
	mov word ptr cs:c_26346, ax
	mov ax, word ptr cs:c_262d8
	mov word ptr cs:c_26318, ax
	mov ax, word ptr cs:c_262da
	mov word ptr cs:c_2631a, ax
	mov ax, word ptr cs:c_262dc
	mov word ptr cs:c_2631c, ax
	mov ax, word ptr cs:c_262de
	mov word ptr cs:c_2631e, ax
	mov ax, word ptr cs:c_262e0
	mov word ptr cs:c_26320, ax
	mov ax, word ptr cs:c_262e2
	mov word ptr cs:c_26322, ax
	mov ax, word ptr cs:c_262e4
	mov word ptr cs:c_26324, ax
	mov ax, word ptr cs:c_262e6
	mov word ptr cs:c_26326, ax
	mov ax, word ptr cs:c_262e8
	mov word ptr cs:c_26328, ax
	mov ax, word ptr cs:c_262ea
	mov word ptr cs:c_2632a, ax
	mov ax, word ptr cs:c_262ec
	mov word ptr cs:c_2632c, ax
	mov ax, word ptr cs:c_262ee
	mov word ptr cs:c_2632e, ax
	mov ax, word ptr cs:c_262f0
	mov word ptr cs:c_26330, ax
	mov ax, word ptr cs:c_262f2
	mov word ptr cs:c_26332, ax
	mov ax, word ptr cs:c_262f4
	mov word ptr cs:c_26334, ax
	mov ax, word ptr cs:c_262f6
	mov word ptr cs:c_26336, ax
	mov ax, word ptr cs:c_262c8
	mov word ptr cs:c_26308, ax
	mov ax, word ptr cs:c_262ca
	mov word ptr cs:c_2630a, ax
	mov ax, word ptr cs:c_262cc
	mov word ptr cs:c_2630c, ax
	mov ax, word ptr cs:c_262ce
	mov word ptr cs:c_2630e, ax
	mov ax, word ptr cs:c_262d0
	mov word ptr cs:c_26310, ax
	mov ax, word ptr cs:c_262d2
	mov word ptr cs:c_26312, ax
	mov ax, word ptr cs:c_262d4
	mov word ptr cs:c_26314, ax
	mov ax, word ptr cs:c_262d6
	mov word ptr cs:c_26316, ax
	pop ax
	mov byte ptr cs:c_26393, al
	xor di, di
L271a7:
	mov cl, es:[bx+di+8ch]
	cmp cl, 0ffh
	jne L271b4
	jmp L2742c
L271b4:
	cmp cl, 0feh
	jne L271bc
	jmp L2742c
L271bc:
	cmp cl, 0fh
	jne L271c4
	jmp L2742c
L271c4:
	push di
	mov di, cx
	and di, 0ffh
	test byte ptr es:[bx+di+134h], 2
	je L271d7
	pop di
	jmp L2742c
L271d7:
	test byte ptr es:[bx+di+143h], 0ffh
	pop di
	je L271e3
	jmp L2742c
L271e3:
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, cl
	push di
	mov di, cx
	and di, 0ffh
	mov ah, es:[bx+di+0dah]
	and ah, 0fh
	mov ch, es:[bx+di+0dah]
	pop di
	shr ch, 1
	shr ch, 1
	shr ch, 1
	shr ch, 1
	je L27215
	push dx
	mov dx, 10h
	sub dl, ch
	add dx, bp
	mov ch, dl
	pop dx
L27215:
	push di
	mov di, cx
	and di, 0ffh
	test byte ptr es:[bx+di+134h], 1
	je L27232
	cmp byte ptr cs:c_262f8[di], 0ffh
	jne L27232
	pop di
	mov dh, cl
	jmp L272c2
L27232:
	pop di
	mov dh, 0ffh
	push bx
	xor bx, bx
L27238:
	cmp byte ptr cs:c_262f8[bx], 0ffh
	je L2724b
	cmp byte ptr cs:c_262f8[bx], dl
	jne L2725b
	pop bx
	jmp L2742c
L2724b:
	cmp bl, byte ptr cs:c_2638a
	jb L2725b
	cmp bl, byte ptr cs:c_2638b
	ja L2725b
	mov dh, bl
L2725b:
	inc bl
	cmp bl, 10h
	jne L27238
	pop bx
	cmp dh, 0ffh
	jne L272c2
	cmp ch, 0
	je L27270
	jmp L2753e
L27270:
	push di
	push cx
	push ax
	mov dh, 0ffh
	xor ax, ax
	xor di, di
L27279:
	cmp al, byte ptr cs:c_262d8[di]
	jae L27289
	mov al, byte ptr cs:c_262d8[di]
	mov cx, di
	mov dh, cl
L27289:
	inc di
	cmp di, 10h
	jne L27279
	pop ax
	cmp dh, 0ffh
	je L272b8
	xor cx, cx
	mov cl, dh
	mov di, cx
	add al, byte ptr cs:c_262e8[di]
	mov byte ptr cs:c_262f8[di], 0ffh
	mov byte ptr cs:c_262e8[di], 0
	mov byte ptr cs:c_262d8[di], 0
	mov byte ptr cs:c_262c8[di], 0
L272b8:
	pop cx
	pop di
	cmp dh, 0ffh
	jne L272c2
	jmp L27438
L272c2:
	cmp ah, al
	jbe L27324
	cmp ch, 0
	je L272ce
	jmp L2742c
L272ce:
	push di
	push cx
	push ax
	mov dh, 0ffh
	xor ax, ax
	xor di, di
L272d7:
	cmp al, byte ptr cs:c_262d8[di]
	jae L272e7
	mov al, byte ptr cs:c_262d8[di]
	mov cx, di
	mov dh, cl
L272e7:
	inc di
	cmp di, 10h
	jne L272d7
	pop ax
	cmp dh, 0ffh
	je L27316
	xor cx, cx
	mov cl, dh
	mov di, cx
	add al, byte ptr cs:c_262e8[di]
	mov byte ptr cs:c_262f8[di], 0ffh
	mov byte ptr cs:c_262e8[di], 0
	mov byte ptr cs:c_262d8[di], 0
	mov byte ptr cs:c_262c8[di], 0
L27316:
	pop cx
	pop di
	cmp dh, 0ffh
	jne L27320
	jmp L27438
L27320:
	cmp ah, al
	ja L272ce
L27324:
	push di
	xchg dl, dh
	mov di, dx
	xchg dl, dh
	and di, 0ffh
	mov byte ptr cs:c_262f8[di], dl
	mov byte ptr cs:c_262e8[di], ah
	sub al, ah
	mov byte ptr cs:c_262d8[di], ch
	push di
	mov di, cx
	and di, 0ffh
	test byte ptr es:[bx+di+134h], 1
	pop di
	jne L2735a
	mov byte ptr cs:c_262c8[di], 0
	pop di
	jmp L2742c
L2735a:
	mov byte ptr cs:c_262c8[di], 1
	cmp dh, cl
	jne L27368
	pop di
	jmp L2742c
L27368:
	push si
	mov si, cx
	and si, 0ffh
	cmp byte ptr cs:c_262c8[si], 0
	jne L273cd
	push ax
	mov al, byte ptr cs:c_262f8[di]
	mov ah, byte ptr cs:c_262f8[si]
	mov byte ptr cs:c_262f8[di], ah
	mov byte ptr cs:c_262f8[si], al
	mov al, byte ptr cs:c_262d8[di]
	mov ah, byte ptr cs:c_262d8[si]
	mov byte ptr cs:c_262d8[di], ah
	mov byte ptr cs:c_262d8[si], al
	mov al, byte ptr cs:c_262e8[di]
	mov ah, byte ptr cs:c_262e8[si]
	mov byte ptr cs:c_262e8[di], ah
	mov byte ptr cs:c_262e8[si], al
	mov al, byte ptr cs:c_262c8[di]
	mov ah, byte ptr cs:c_262c8[si]
	mov byte ptr cs:c_262c8[di], ah
	mov byte ptr cs:c_262c8[si], al
	pop ax
	pop si
	pop di
	jmp short L2742c
L273cd:
	cmp ch, 0
	je L273f0
	mov byte ptr cs:c_262f8[di], 0ffh
	mov byte ptr cs:c_262d8[di], 0
	mov byte ptr cs:c_262e8[di], 0
	mov byte ptr cs:c_262c8[di], 0
	add al, ah
	pop si
	pop di
	jmp short L2742c
L273f0:
	cmp byte ptr cs:c_262d8[si], 0
	jne L273fc
	pop si
	pop di
	jmp short L27438
L273fc:
	add al, byte ptr cs:c_262e8[si]
	mov byte ptr cs:c_262f8[di], 0ffh
	mov byte ptr cs:c_262e8[di], 0
	mov byte ptr cs:c_262d8[di], 0
	mov byte ptr cs:c_262c8[di], 0
	mov byte ptr cs:c_262f8[si], dl
	mov byte ptr cs:c_262d8[si], ch
	mov byte ptr cs:c_262e8[si], ah
	sub al, ah
	pop si
	pop di
L2742c:
	inc di
	cmp di, 10h
	jne L27435
	jmp L2753e
L27435:
	jmp L271a7
L27438:
	push ax
	mov ax, word ptr cs:c_26338
	mov word ptr cs:c_262f8, ax
	mov ax, word ptr cs:c_2633a
	mov word ptr cs:c_262fa, ax
	mov ax, word ptr cs:c_2633c
	mov word ptr cs:c_262fc, ax
	mov ax, word ptr cs:c_2633e
	mov word ptr cs:c_262fe, ax
	mov ax, word ptr cs:c_26340
	mov word ptr cs:c_26300, ax
	mov ax, word ptr cs:c_26342
	mov word ptr cs:c_26302, ax
	mov ax, word ptr cs:c_26344
	mov word ptr cs:c_26304, ax
	mov ax, word ptr cs:c_26346
	mov word ptr cs:c_26306, ax
	mov ax, word ptr cs:c_26318
	mov word ptr cs:c_262d8, ax
	mov ax, word ptr cs:c_2631a
	mov word ptr cs:c_262da, ax
	mov ax, word ptr cs:c_2631c
	mov word ptr cs:c_262dc, ax
	mov ax, word ptr cs:c_2631e
	mov word ptr cs:c_262de, ax
	mov ax, word ptr cs:c_26320
	mov word ptr cs:c_262e0, ax
	mov ax, word ptr cs:c_26322
	mov word ptr cs:c_262e2, ax
	mov ax, word ptr cs:c_26324
	mov word ptr cs:c_262e4, ax
	mov ax, word ptr cs:c_26326
	mov word ptr cs:c_262e6, ax
	mov ax, word ptr cs:c_26328
	mov word ptr cs:c_262e8, ax
	mov ax, word ptr cs:c_2632a
	mov word ptr cs:c_262ea, ax
	mov ax, word ptr cs:c_2632c
	mov word ptr cs:c_262ec, ax
	mov ax, word ptr cs:c_2632e
	mov word ptr cs:c_262ee, ax
	mov ax, word ptr cs:c_26330
	mov word ptr cs:c_262f0, ax
	mov ax, word ptr cs:c_26332
	mov word ptr cs:c_262f2, ax
	mov ax, word ptr cs:c_26334
	mov word ptr cs:c_262f4, ax
	mov ax, word ptr cs:c_26336
	mov word ptr cs:c_262f6, ax
	mov ax, word ptr cs:c_26308
	mov word ptr cs:c_262c8, ax
	mov ax, word ptr cs:c_2630a
	mov word ptr cs:c_262ca, ax
	mov ax, word ptr cs:c_2630c
	mov word ptr cs:c_262cc, ax
	mov ax, word ptr cs:c_2630e
	mov word ptr cs:c_262ce, ax
	mov ax, word ptr cs:c_26310
	mov word ptr cs:c_262d0, ax
	mov ax, word ptr cs:c_26312
	mov word ptr cs:c_262d2, ax
	mov ax, word ptr cs:c_26314
	mov word ptr cs:c_262d4, ax
	mov ax, word ptr cs:c_26316
	mov word ptr cs:c_262d6, ax
	pop ax
	mov al, byte ptr cs:c_26393
L2753e:
	add bp, 10h
	add si, 4
	cmp si, 40h
	je L2754c
	jmp L27054
L2754c:
	xor si, si
L2754e:
	cmp byte ptr cs:c_262f8[si], 0ffh
	jne L27559
	jmp L276e1
L27559:
	cmp byte ptr cs:c_262c8[si], 0
	jne L27564
	jmp L27672
L27564:
	xor ax, ax
	mov al, byte ptr cs:c_262f8[si]
	mov byte ptr cs:c_262f8[si], 0ffh
	mov byte ptr cs:c_262b8[si], al
	mov di, ax
	and di, 0f0h
	shr di, 1
	shr di, 1
	les bx, dword ptr cs:c_26198[di]
	and al, 0fh
	cmp byte ptr cs:c_26348[si], al
	jne L275a7
	mov di, si
	shl di, 1
	shl di, 1
	cmp word ptr cs:c_26218[di], bx
	jne L275a7
	mov cx, es
	cmp word ptr cs:c_2621a[di], cx
	jne L275a7
	jmp L276e1
L275a7:
	push ax
	push dx
	push si
	xor ah, ah
	xchg si, ax
	mov cx, 7b00h
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov cl, es:[bx+si+0dah]
	and cl, 0fh
	mov ch, 4bh
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov cl, es:[bx+si+116h]
	push bp
	mov bp, 8
	call dword ptr cs:c_26377
	pop bp
	push si
	mov si, ax
	mov byte ptr cs:c_26358[si], 0ffh
	pop si
	mov cl, es:[bx+si+107h]
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	mov ch, 7
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov ch, 0ah
	mov cl, es:[bx+si+0f8h]
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov ch, 1
	mov cl, es:[bx+si+0e9h]
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	shl si, 1
	mov cx, 4000h
	cmp byte ptr es:[bx+si+0bdh], 80h
	jb L27631
	mov cl, 7fh
L27631:
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov cx, es:[bx+si+0bch]
	shr si, 1
	xchg cl, ch
	shl cl, 1
	cmp ch, 80h
	jb L2764e
	or cl, 1
L2764e:
	and cx, 7f7fh
	push bp
	mov bp, 0ah
	call dword ptr cs:c_26377
	pop bp
	mov cl, es:[bx+si+125h]
	mov ch, 4eh
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	pop si
	pop dx
	pop ax
	jmp short L276e1
L27672:
	mov al, byte ptr cs:c_262f8[si]
	mov bl, al
	and ax, 0fh
	and bx, 0f0h
	shr bl, 1
	shr bl, 1
	les bx, dword ptr cs:c_26198[bx]
	mov cl, byte ptr cs:c_2638a
	xor ch, ch
	mov di, cx
	shl di, 1
	shl di, 1
L27696:
	cmp word ptr cs:c_26218[di], bx
	jne L276b5
	mov cx, es
	cmp word ptr cs:c_2621a[di], cx
	jne L276b5
	shr di, 1
	shr di, 1
	cmp byte ptr cs:c_26348[di], al
	je L276c9
	shl di, 1
	shl di, 1
L276b5:
	add di, 4
	mov cx, di
	shr cl, 1
	shr cl, 1
	dec cl
	cmp byte ptr cs:c_2638b, cl
	jne L27696
	jmp short L276e1
L276c9:
	cmp byte ptr cs:c_262c8[di], 0
	jne L276e1
	mov cl, byte ptr cs:c_262f8[si]
	mov byte ptr cs:c_262b8[di], cl
	mov byte ptr cs:c_262f8[si], 0ffh
L276e1:
	inc si
	cmp si, 10h
	je L276ea
	jmp L2754e
L276ea:
	mov al, byte ptr cs:c_2638b
	inc al
	xor ah, ah
	mov di, ax
	xor al, al
	xor si, si
L276f8:
	cmp byte ptr cs:c_262f8[si], 0ffh
	jne L27703
	jmp L277f8
L27703:
	mov bx, di
L27705:
	dec bx
	cmp byte ptr cs:c_262b8[bx], 0ffh
	jne L27705
	mov di, bx
	mov al, byte ptr cs:c_262f8[si]
	mov byte ptr cs:c_262b8[di], al
	mov bl, al
	and al, 0fh
	and bx, 0f0h
	shr bx, 1
	shr bx, 1
	les bx, dword ptr cs:c_26198[bx]
	push si
	mov si, di
	push ax
	push dx
	push si
	xor ah, ah
	xchg si, ax
	mov cx, 7b00h
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov cl, es:[bx+si+0dah]
	and cl, 0fh
	mov ch, 4bh
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov cl, es:[bx+si+116h]
	push bp
	mov bp, 8
	call dword ptr cs:c_26377
	pop bp
	push si
	mov si, ax
	mov byte ptr cs:c_26358[si], 0ffh
	pop si
	mov cl, es:[bx+si+107h]
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	mov ch, 7
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov ch, 0ah
	mov cl, es:[bx+si+0f8h]
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov ch, 1
	mov cl, es:[bx+si+0e9h]
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	shl si, 1
	mov cx, 4000h
	cmp byte ptr es:[bx+si+0bdh], 80h
	jb L277b8
	mov cl, 7fh
L277b8:
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov cx, es:[bx+si+0bch]
	shr si, 1
	xchg cl, ch
	shl cl, 1
	cmp ch, 80h
	jb L277d5
	or cl, 1
L277d5:
	and cx, 7f7fh
	push bp
	mov bp, 0ah
	call dword ptr cs:c_26377
	pop bp
	mov cl, es:[bx+si+125h]
	mov ch, 4eh
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	pop si
	pop dx
	pop ax
	pop si
L277f8:
	inc si
	cmp si, 10h
	je L27801
	jmp L276f8
L27801:
	mov si, 0fh
L27804:
	cmp byte ptr cs:c_26348[si], 0fh
	je L2783d
	cmp byte ptr cs:c_262b8[si], 0ffh
	jne L2783d
	mov ax, si
	mov cx, 4000h
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov cx, 7b00h
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	mov cx, 4b00h
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
L2783d:
	dec si
	jns L27804
	mov ax, word ptr cs:c_262b8
	and ax, 0f0fh
	mov word ptr cs:c_26348, ax
	mov ax, word ptr cs:c_262ba
	and ax, 0f0fh
	mov word ptr cs:c_2634a, ax
	mov ax, word ptr cs:c_262bc
	and ax, 0f0fh
	mov word ptr cs:c_2634c, ax
	mov ax, word ptr cs:c_262be
	and ax, 0f0fh
	mov word ptr cs:c_2634e, ax
	mov ax, word ptr cs:c_262c0
	and ax, 0f0fh
	mov word ptr cs:c_26350, ax
	mov ax, word ptr cs:c_262c2
	and ax, 0f0fh
	mov word ptr cs:c_26352, ax
	mov ax, word ptr cs:c_262c4
	and ax, 0f0fh
	mov word ptr cs:c_26354, ax
	mov ax, word ptr cs:c_262c6
	and ax, 0f0fh
	mov word ptr cs:c_26356, ax
	xor si, si
	xor di, di
L2789c:
	mov bl, byte ptr cs:c_262b8[si]
	cmp bl, 0ffh
	jne L278b6
	mov word ptr cs:c_26218[di], 0
	mov word ptr cs:c_2621a[di], 0
	jmp short L278d2
L278b6:
	and bx, 0f0h
	shr bx, 1
	shr bx, 1
	mov ax, word ptr cs:c_26198[bx]
	mov word ptr cs:c_26218[di], ax
	mov ax, word ptr cs:c_2619a[bx]
	mov word ptr cs:c_2621a[di], ax
L278d2:
	add di, 4
	inc si
	cmp si, 10h
	jne L2789c
	dec byte ptr cs:c_26389
	pop es
	pop bp
	pop si
	pop di
	pop dx
	pop cx
	pop bx
	pop ax
	ret
_sequencer_tick endp

/* 0x278e9 */
_advance_volume_ramp proc near
	push bx
	push cx
	cmp byte ptr es:[bx+162h], 0
	je L278fb
	dec byte ptr es:[bx+162h]
	jmp L279a6
L278fb:
	mov cl, es:[bx+161h]
	mov es:[bx+162h], cl
	mov cl, es:[bx+160h]
	and cl, 7fh
	cmp cl, es:[bx+15eh]
	je L27982
	ja L2794c
	mov cl, es:[bx+15eh]
	mov ch, es:[bx+160h]
	and ch, 7fh
	sub cl, ch
	cmp cl, es:[bx+163h]
	ja L2793b
	mov cl, es:[bx+160h]
	and cl, 7fh
	mov ch, 1
	call _set_sequence_volume
	jmp short L27982
L2793b:
	mov cl, es:[bx+15eh]
	sub cl, es:[bx+163h]
	mov ch, 1
	call _set_sequence_volume
	jmp short L279a6
L2794c:
	mov cl, es:[bx+160h]
	and cl, 7fh
	mov ch, es:[bx+15eh]
	sub cl, ch
	cmp cl, es:[bx+163h]
	ja L27971
	mov cl, es:[bx+160h]
	and cl, 7fh
	mov ch, 1
	call _set_sequence_volume
	jmp short L27982
L27971:
	mov cl, es:[bx+15eh]
	add cl, es:[bx+163h]
	mov ch, 1
	call _set_sequence_volume
	jmp short L279a6
L27982:
	mov byte ptr es:[bx+158h], 0feh
	mov byte ptr es:[bx+163h], 0
	mov cl, es:[bx+160h]
	and cl, 80h
	cmp cl, 0
	je L279a6
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:c_26394, 1
L279a6:
	pop cx
	pop bx
	ret
_advance_volume_ramp endp

/* 0x279a9 */
_set_sequence_volume proc near
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	mov byte ptr cs:c_26395, ch
	cmp cl, es:[bx+15eh]
	jne L279be
	jmp L27a7f
L279be:
	mov es:[bx+15eh], cl
	cmp si, 0ffh
	jne L279cc
	jmp L27a7f
L279cc:
	mov dx, si
	shl dl, 1
	shl dl, 1
	xor si, si
L279d4:
	mov cl, byte ptr cs:c_262b8[si]
	cmp cl, 0ffh
	je L27a21
	mov ch, cl
	and cl, 0f0h
	cmp cl, dl
	jne L27a21
	mov cl, ch
	and cx, 0fh
	mov di, cx
	mov cl, es:[bx+di+107h]
	push dx
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	cmp byte ptr cs:c_26395, 0
	je L27a0c
	mov byte ptr cs:c_26358[si], cl
	jmp short L27a20
L27a0c:
	mov ch, 7
	mov ax, si
	mov byte ptr cs:c_26358[si], 0ffh
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
L27a20:
	pop dx
L27a21:
	inc si
	cmp si, 10h
	jne L279d4
	xor ch, ch
	xor si, si
L27a2b:
	mov cl, es:[bx+si+8ch]
	cmp cl, 0ffh
	je L27a7f
	mov di, cx
	test byte ptr es:[bx+di+134h], 2
	je L27a79
	cmp byte ptr cs:c_262b8[di], 0ffh
	jne L27a79
	mov al, cl
	mov cl, es:[bx+di+107h]
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	cmp byte ptr cs:c_26395, 0
	je L27a65
	mov byte ptr cs:c_26358[di], cl
	jmp short L27a79
L27a65:
	mov ch, 7
	mov ax, di
	mov byte ptr cs:c_26358[di], 0ffh
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
L27a79:
	inc si
	cmp si, 10h
	jne L27a2b
L27a7f:
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret
_set_sequence_volume endp

/* 0x27a86 */
_flush_pending_volumes proc near
	xor bl, bl
	mov al, byte ptr cs:c_26396
	xor ah, ah
	mov si, ax
L27a90:
	mov cl, byte ptr cs:c_26358[si]
	cmp cl, 0ffh
	je L27ab5
	mov byte ptr cs:c_26358[si], 0ffh
	mov ch, 7
	mov ax, si
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
	inc bl
	cmp bl, 2
	je L27ac7
L27ab5:
	inc si
	cmp si, 10h
	jne L27abd
	xor si, si
L27abd:
	mov al, byte ptr cs:c_26396
	xor ah, ah
	cmp si, ax
	jne L27a90
L27ac7:
	mov ax, si
	mov byte ptr cs:c_26396, al
	ret
_flush_pending_volumes endp

/* 0x27ace */
_sound_service proc far
	cmp byte ptr cs:c_26389, 0
	je L27ad7
	retf
L27ad7:
	pushf
	cli
	push si
	push di
	push es
	push ds
	push bp
	cmp byte ptr cs:c_26394, 0
	je L27ae9
	call _sequencer_tick
L27ae9:
	xor si, si
	xor di, di
L27aed:
	les bx, dword ptr cs:c_26198[si]
	mov ax, es
	or ax, bx
	je L27b3b
	cmp byte ptr es:[bx+164h], 0
	jne L27b30
	cmp byte ptr es:[bx+163h], 0
	je L27b18
	call _advance_volume_ramp
	cmp byte ptr es:[bx+158h], 0ffh
	jne L27b18
	sub si, 4
	jmp short L27b30
L27b18:
	cmp byte ptr es:[bx+165h], 0
	je L27b25
	call _drop_unless_polled
	jmp short L27b28
L27b25:
	call _step_sequence
L27b28:
	cmp byte ptr es:[bx+158h], 0ffh
	je L27b33
L27b30:
	add si, 4
L27b33:
	add di, 4
	cmp si, 40h
	jne L27aed
L27b3b:
	call _poll_sequences
	call _flush_pending_volumes
	push bp
	mov bp, 3
	call dword ptr cs:c_26377
	pop bp
	pop bp
	pop ds
	pop es
	pop di
	pop si
	popf
	retf
_sound_service endp

/* 0x27b52 */
_drop_unless_polled proc near
	push cx
	push si
	push ax
	mov cx, es
	xor si, si
L27b59:
	cmp word ptr cs:c_261d8[si], bx
	jne L27b67
	cmp word ptr cs:c_261da[si], cx
	je L27b7a
L27b67:
	add si, 4
	cmp si, 40h
	jne L27b59
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:c_26394, 1
L27b7a:
	pop ax
	pop si
	pop cx
	ret
_drop_unless_polled endp

/* 0x27b7e */
_poll_sequences proc near
	push ds
	xor si, si
L27b81:
	les bx, dword ptr cs:c_261d8[si]
	mov cx, es
	cmp cx, 0
	jne L27b95
	cmp bx, 0
	jne L27b95
	jmp L27c4c
L27b95:
	inc word ptr es:[bx+154h]
	lds bp, dword ptr es:[bx+8]
	lds bp, dword ptr ds:[bp]
	mov cl, es:[bx+165h]
	and cl, 0fh
	dec cl
	shl cl, 1
	xor ch, ch
	push bp
	add bp, cx
	mov cx, ds:[bp]
	pop bp
	add bp, cx
	mov ax, bp
	cmp byte ptr es:[bx+165h], 10h
	ja L27c06
	or byte ptr es:[bx+165h], 80h
	mov cx, bx
	push bx
	mov bx, ax
	inc bx
	cmp byte ptr [bx], 0feh
	jne L27bd6
	inc bx
L27bd6:
	inc bx
	mov ax, [bx+2]
	push ax
	push ds
	mov ax, bx
	add ax, 8
	push ax
	mov ax, [bx]
	push ax
	push bx
	mov bx, cx
	mov cl, es:[bx+15eh]
	mov ch, es:[bx+15dh]
	pop bx
	push cx
	mov ax, sp
	push ax
	mov ax, 3
	push ax
	call FAR PTR _sound_callback
	add sp, 0eh
	pop bx
	jmp short L27c41
L27c06:
	mov ch, es:[bx+15dh]
	mov cl, es:[bx+15eh]
	push cx
	mov ax, sp
	push ax
	mov ax, 4
	push ax
	call FAR PTR _sound_callback
	add sp, 6
	cmp ah, 0
	je L27c2c
	mov word ptr es:[bx+154h], 0
L27c2c:
	cmp al, 0
	je L27c41
	mov byte ptr es:[bx+165h], 0
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:c_26394, 1
L27c41:
	add si, 4
	cmp si, 40h
	je L27c4c
	jmp L27b81
L27c4c:
	pop ds
	ret
_poll_sequences endp

/* 0x27c4e */
_step_sequence proc near
	push si
	push di
	push bp
	push ax
	push bx
	push cx
	push dx
	push ds
	shl di, 1
	shl di, 1
	mov cx, di
	mov byte ptr cs:c_26391, cl
	inc word ptr es:[bx+154h]
	lds bp, dword ptr es:[bx+8]
	lds bp, dword ptr ds:[bp]
	mov word ptr cs:c_26387, bp
	xor si, si
L27c75:
	mov al, es:[bx+si+8ch]
	cmp al, 0ffh
	jne L27c81
	jmp L27e1f
L27c81:
	cmp al, 0feh
	jne L27c88
	jmp L27e16
L27c88:
	mov byte ptr cs:c_2638d, 0ffh
	mov byte ptr cs:c_2638e, 0
	push si
	mov si, ax
	and si, 0ffh
	test byte ptr es:[bx+si+134h], 2
	pop si
	je L27cb0
	mov byte ptr cs:c_2638d, al
	mov byte ptr cs:c_2638e, 1
	jmp short L27cd1
L27cb0:
	and al, 0fh
	mov cl, al
	or cl, byte ptr cs:c_26391
	xor di, di
L27cbb:
	cmp byte ptr cs:c_262b8[di], cl
	je L27cca
	inc di
	cmp di, 10h
	jne L27cbb
	jmp short L27cd1
L27cca:
	mov dx, di
	mov byte ptr cs:c_2638d, dl
L27cd1:
	mov bp, word ptr cs:c_26387
	shl si, 1
	mov dx, ds:[bp+si]
	add bp, dx
	add bp, es:[bx+si+0ch]
	cmp word ptr es:[bx+si+0ch], 0
	jne L27ced
	shr si, 1
	jmp L27e16
L27ced:
	shr si, 1
	shl si, 1
	cmp word ptr es:[bx+si+4ch], 0
	je L27d29
	dec word ptr es:[bx+si+4ch]
	cmp word ptr es:[bx+si+4ch], 8000h
	jne L27d24
	xor dh, dh
	shr si, 1
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	shl si, 1
	cmp dl, 0f8h
	jne L27d20
	mov dl, 0f0h
	mov dh, 80h
L27d20:
	mov es:[bx+si+4ch], dx
L27d24:
	shr si, 1
	jmp L27e16
L27d29:
	shr si, 1
L27d2b:
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp dl, 80h
	jb L27d44
	mov es:[bx+si+9ch], dl
	jmp short L27d52
L27d44:
	mov dl, es:[bx+si+9ch]
	dec bp
	shl si, 1
	dec word ptr es:[bx+si+0ch]
	shr si, 1
L27d52:
	mov al, dl
	mov ah, al
	and ah, 0f0h
	and al, 0fh
	cmp dl, 0fch
	jne L27d6d
	shl si, 1
	mov word ptr es:[bx+si+0ch], 0
	shr si, 1
	jmp L27e16
L27d6d:
	cmp al, 0fh
	jne L27d84
	call _midi_meta_event
	shl si, 1
	mov dx, es:[bx+si+0ch]
	shr si, 1
	cmp dx, 0
	jne L27de4
	jmp L27e16
L27d84:
	mov al, byte ptr cs:c_2638d
	cmp ah, 80h
	jne L27d92
	call _midi_note_off_event
	jmp short L27de4
L27d92:
	cmp ah, 90h
	jne L27d9c
	call _midi_note_event
	jmp short L27de4
L27d9c:
	cmp ah, 0a0h
	jne L27da6
	call _midi_event_6
	jmp short L27de4
L27da6:
	cmp ah, 0b0h
	jne L27db0
	call _midi_controller_event
	jmp short L27de4
L27db0:
	cmp ah, 0c0h
	jne L27dba
	call _midi_program_event
	jmp short L27de4
L27dba:
	cmp ah, 0d0h
	jne L27dc4
	call _midi_event_9
	jmp short L27de4
L27dc4:
	cmp ah, 0e0h
	jne L27dce
	call _midi_bend_event
	jmp short L27de4
L27dce:
	cmp ah, 0f0h
	jne L27dd8
	call _midi_skip_event
	jmp short L27de4
L27dd8:
	shl si, 1
	mov word ptr es:[bx+si+0ch], 0
	shr si, 1
	jmp short L27e16
L27de4:
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp dl, 0
	jne L27df9
	jmp L27d2b
L27df9:
	shl si, 1
	cmp dl, 0f8h
	jne L27e0c
	mov dl, 0efh
	mov dh, 80h
	mov es:[bx+si+4ch], dx
	shr si, 1
	jmp short L27e16
L27e0c:
	xor dh, dh
	dec dl
	mov es:[bx+si+4ch], dx
	shr si, 1
L27e16:
	inc si
	cmp si, 10h
	je L27e1f
	jmp L27c75
L27e1f:
	xor si, si
L27e21:
	cmp byte ptr es:[bx+si+8ch], 0ffh
	je L27e3a
	shl si, 1
	cmp word ptr es:[bx+si+0ch], 0
	jne L27e89
	shr si, 1
	inc si
	cmp si, 10h
	jne L27e21
L27e3a:
	cmp byte ptr es:[bx+15ah], 0
	jne L27e57
	cmp byte ptr es:[bx+15dh], 0
	jne L27e57
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:c_26394, 1
	jmp short L27e89
L27e57:
	mov dx, es:[bx+156h]
	mov es:[bx+154h], dx
	xor si, si
L27e63:
	mov dx, es:[bx+si+2ch]
	mov es:[bx+si+0ch], dx
	mov dx, es:[bx+si+6ch]
	mov es:[bx+si+4ch], dx
	shr si, 1
	mov dl, es:[bx+si+0ach]
	mov es:[bx+si+9ch], dl
	shl si, 1
	add si, 2
	cmp si, 20h
	jne L27e63
L27e89:
	pop ds
	pop dx
	pop cx
	pop bx
	pop ax
	pop bp
	pop di
	pop si
	ret
_step_sequence endp

/* 0x27e92 */
_midi_note_off_event proc near
	push si
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	push ax
	mov al, es:[bx+si+8ch]
	mov si, ax
	and si, 0fh
	pop ax
	cmp es:[bx+si+125h], ch
	jne L27ec7
	mov byte ptr es:[bx+si+125h], 0ffh
L27ec7:
	cmp al, 0ffh
	je L27edf
	cmp byte ptr cs:c_26399, 0
	jne L27edf
	and al, 0fh
	push bp
	mov bp, 4
	call dword ptr cs:c_26377
	pop bp
L27edf:
	pop si
	ret
_midi_note_off_event endp

/* 0x27ee1 */
_midi_note_event proc near
	push si
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	push ax
	mov al, es:[bx+si+8ch]
	mov si, ax
	and si, 0fh
	pop ax
	cmp cl, 0
	je L27f2d
	mov es:[bx+si+125h], ch
	cmp al, 0ffh
	je L27f52
	cmp byte ptr cs:c_26399, 0
	jne L27f52
	and al, 0fh
	push bp
	mov bp, 5
	call dword ptr cs:c_26377
	pop bp
	jmp short L27f52
L27f2d:
	cmp es:[bx+si+125h], ch
	jne L27f3a
	mov byte ptr es:[bx+si+125h], 0ffh
L27f3a:
	cmp al, 0ffh
	je L27f52
	cmp byte ptr cs:c_26399, 0
	jne L27f52
	and al, 0fh
	push bp
	mov bp, 4
	call dword ptr cs:c_26377
	pop bp
L27f52:
	pop si
	ret
_midi_note_event endp

/* 0x27f54 */
_midi_event_6 proc near
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp al, 0ffh
	je L27f84
	cmp byte ptr cs:c_26399, 0
	jne L27f84
	push bp
	mov bp, 6
	call dword ptr cs:c_26377
	pop bp
L27f84:
	ret
_midi_event_6 endp

/* 0x27f85 */
_midi_controller_event proc near
	push si
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	test byte ptr cs:c_2638e, 0ffh
	je L27fbb
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:c_262b8[si], 0ffh
	pop si
	je L27fbb
	jmp L28084
L27fbb:
	push ax
	mov al, es:[bx+si+8ch]
	mov si, ax
	and si, 0fh
	pop ax
	cmp ch, 7
	jne L27ff1
	mov es:[bx+si+107h], cl
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	cmp al, 20h
	jb L27fe1
	jmp L28084
L27fe1:
	push si
	mov si, ax
	and si, 0ffh
	mov byte ptr cs:c_26358[si], 0ffh
	pop si
	jmp short L2806c
L27ff1:
	cmp ch, 0ah
	jne L27ffd
	mov es:[bx+si+0f8h], cl
	jmp short L2806c
L27ffd:
	cmp ch, 1
	jne L28009
	mov es:[bx+si+0e9h], cl
	jmp short L2806c
L28009:
	cmp ch, 40h
	jne L2802b
	push dx
	shl si, 1
	mov dx, es:[bx+si+0bch]
	or dh, 80h
	cmp cl, 0
	jne L28021
	and dh, 7fh
L28021:
	mov es:[bx+si+0bch], dx
	shr si, 1
	pop dx
	jmp short L2806c
L2802b:
	cmp ch, 4bh
	jne L28049
	push cx
	mov ch, es:[bx+si+0dah]
	and ch, 0f0h
	or ch, cl
	mov es:[bx+si+0dah], ch
	pop cx
	mov byte ptr cs:c_26394, 1
	jmp short L2806c
L28049:
	cmp ch, 4eh
	jne L2806c
	push cx
	mov ch, es:[bx+si+143h]
	and ch, 0f0h
	test cl, 0ffh
	je L2805e
	mov cl, 1
L2805e:
	or ch, cl
	mov es:[bx+si+143h], ch
	pop cx
	mov byte ptr cs:c_26394, 1
L2806c:
	cmp al, 0ffh
	jae L28084
	cmp byte ptr cs:c_26399, 0
	jne L28084
	and al, 0fh
	push bp
	mov bp, 7
	call dword ptr cs:c_26377
	pop bp
L28084:
	pop si
	ret
_midi_controller_event endp

/* 0x28086 */
_midi_program_event proc near
	push si
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	test byte ptr cs:c_2638e, 0ffh
	je L280ae
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:c_262b8[si], 0ffh
	pop si
	je L280ae
	jmp short L280d8
L280ae:
	push ax
	mov al, es:[bx+si+8ch]
	mov si, ax
	and si, 0fh
	pop ax
	mov es:[bx+si+116h], cl
	cmp al, 0ffh
	jae L280d8
	cmp byte ptr cs:c_26399, 0
	jne L280d8
	and al, 0fh
	push bp
	mov bp, 8
	call dword ptr cs:c_26377
	pop bp
L280d8:
	pop si
	ret
_midi_program_event endp

/* 0x280da */
_midi_event_9 proc near
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp al, 0ffh
	jae L280fd
	cmp byte ptr cs:c_26399, 0
	jne L280fd
	push bp
	mov bp, 9
	call dword ptr cs:c_26377
	pop bp
L280fd:
	ret
_midi_event_9 endp

/* 0x280fe */
_midi_bend_event proc near
	push si
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	test byte ptr cs:c_2638e, 0ffh
	je L28133
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:c_262b8[si], 0ffh
	pop si
	je L28133
	jmp short L28178
L28133:
	push ax
	mov al, es:[bx+si+8ch]
	mov si, ax
	and si, 0fh
	pop ax
	push cx
	xchg ch, cl
	shr ch, 1
	jae L2814a
	or cl, 80h
L2814a:
	shl si, 1
	cmp word ptr es:[bx+si+0bch], 8000h
	jb L28158
	or ch, 80h
L28158:
	mov es:[bx+si+0bch], cx
	shr si, 1
	pop cx
	cmp al, 0ffh
	jae L28178
	cmp byte ptr cs:c_26399, 0
	jne L28178
	and al, 0fh
	push bp
	mov bp, 0ah
	call dword ptr cs:c_26377
	pop bp
L28178:
	pop si
	ret
_midi_bend_event endp

/* 0x2817a */
_midi_skip_event proc near
	call _skip_unknown_event
	ret
_midi_skip_event endp

/* 0x2817e */
_midi_meta_event proc near
	cmp ah, 0c0h
	je L28191
	cmp ah, 0b0h
	jne L2818b
	jmp L2821f
L2818b:
	call _skip_unknown_event
	jmp L2828d
L28191:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp ch, 7fh
	jne L28210
	push dx
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	xor dh, dh
	cmp dl, 0f8h
	jne L281bc
	mov dh, 80h
	mov dl, 0f0h
L281bc:
	shl si, 1
	mov es:[bx+si+4ch], dx
	shr si, 1
	pop dx
	mov byte ptr es:[bx+si+9ch], 0cfh
	push si
	push dx
	xor si, si
L281cf:
	shl si, 1
	mov dx, es:[bx+si+0ch]
	mov es:[bx+si+2ch], dx
	mov dx, es:[bx+si+4ch]
	mov es:[bx+si+6ch], dx
	shr si, 1
	mov dl, es:[bx+si+9ch]
	mov es:[bx+si+0ach], dl
	inc si
	cmp si, 10h
	jne L281cf
	mov dx, es:[bx+154h]
	mov es:[bx+156h], dx
	pop dx
	pop si
	shl si, 1
	dec word ptr es:[bx+si+0ch]
	dec bp
	mov word ptr es:[bx+si+4ch], 0
	shr si, 1
	jmp short L2828d
L28210:
	cmp byte ptr cs:c_26399, 0
	jne L2828d
	mov es:[bx+158h], ch
	jmp short L2828d
L2821f:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp ch, 50h
	jne L2825b
	cmp cl, 7fh
	jne L28248
	mov cl, byte ptr cs:c_26392
L28248:
	mov es:[bx+15fh], cl
	push ax
	push bp
	mov bp, 0bh
	call dword ptr cs:c_26377
	pop bp
	pop ax
	jmp short L2828d
L2825b:
	cmp ch, 60h
	jne L2826f
	cmp byte ptr cs:c_26399, 0
	jne L2828d
	inc word ptr es:[bx+152h]
	jmp short L2828d
L2826f:
	cmp ch, 52h
	jne L2828d
	cmp es:[bx+15ah], cl
	jne L2828d
	push si
	xor si, si
L2827e:
	mov word ptr es:[bx+si+0ch], 0
	add si, 2
	cmp si, 20h
	jne L2827e
	pop si
L2828d:
	ret
_midi_meta_event endp

/* 0x2828e */
_skip_unknown_event proc near
	cmp ah, 0f0h
	jne L282a6
L28293:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp ch, 0f7h
	jne L28293
	ret
L282a6:
	cmp ah, 0c0h
	je L282bd
	cmp ah, 0d0h
	je L282bd
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
L282bd:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	ret
_skip_unknown_event endp

/* 0x282cb */
_scale_byte_pair proc near
	push ax
	mov al, cl
	xor ah, ah
	inc al
	inc dl
	mul dl
	shl ah, 1
	mov cl, ah
	cmp cl, 0
	je L282e1
	dec cl
L282e1:
	pop ax
	ret
L282e3:
	push cx
	mov cx, es
	xor si, si
L282e8:
	cmp word ptr cs:c_26198[si], bx
	jne L282f8
	cmp word ptr cs:c_2619a[si], cx
	jne L282f8
	jmp short L28303
L282f8:
	add si, 4
	cmp si, 40h
	jne L282e8
	mov si, 0ffh
L28303:
	pop cx
	ret
_scale_byte_pair endp

/* 0x28305 */
_init_sequence_params proc near
	push bx
	push cx
	push dx
	push bp
	push ds
	mov bx, ax
	cmp word ptr es:[bx+8], -1
	jne L2831d
	cmp word ptr es:[bx+0ah], -1
	jne L2831d
	jmp L283d9
L2831d:
	lds bp, dword ptr es:[bx+8]
	lds bp, dword ptr ds:[bp]
	cmp byte ptr ds:[bp+23h], 0feh
	jne L2833d
	cmp byte ptr ds:[bp+22h], 0fdh
	jne L2833d
	cmp byte ptr ds:[bp+21h], 0fch
	jne L2833d
	jmp L283d9
L2833d:
	push bp
	mov si, 20h
L28341:
	sub si, 2
	mov word ptr cs:c_26298[si], 0
	cmp si, 0
	jne L28341
	mov byte ptr cs:c_2639c, 0ffh
	cmp byte ptr ds:[bp], 0f0h
	jne L28369
	mov cl, ds:[bp+1]
	mov byte ptr cs:c_2639c, cl
	add bp, 8
L28369:
	mov cl, ds:[bp]
	cmp cl, byte ptr cs:c_2638c
	je L2838b
	cmp cl, 0ffh
	je L283a8
	inc bp
L2837a:
	mov cl, ds:[bp]
	inc bp
	cmp cl, 0ffh
	jne L28386
	jmp short L28369
L28386:
	add bp, 5
	jmp short L2837a
L2838b:
	inc bp
L2838c:
	mov cl, ds:[bp]
	inc bp
	cmp cl, 0ffh
	je L283a8
	inc bp
	mov cx, ds:[bp]
	add bp, 4
	mov word ptr cs:c_26298[si], cx
	add si, 2
	jmp short L2838c
L283a8:
	pop bp
	push bp
	xor si, si
L283ac:
	mov cx, word ptr cs:c_26298[si]
	mov ds:[bp], cx
	add si, 2
	add bp, 2
	cmp si, 20h
	jne L283ac
	mov cl, byte ptr cs:c_2639c
	mov ds:[bp], cl
	pop bp
	mov byte ptr ds:[bp+21h], 0fch
	mov byte ptr ds:[bp+22h], 0fdh
	mov byte ptr ds:[bp+23h], 0feh
L283d9:
	pop ds
	pop bp
	pop dx
	pop cx
	pop bx
	ret
c_283df label byte
	db 0h, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 0eh, 0e8h, 2bh, 0e8h, 8bh, 0c1h
	db 5eh, 5fh, 1fh, 5dh, 0cbh, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 4eh, 0ah
	db 0eh, 0e8h, 40h, 0e7h, 5eh, 5fh, 1fh, 5dh, 0cbh, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h
	db 0eh, 0e8h, 0ch, 0e8h, 32h, 0f6h, 8bh, 0c1h, 5eh, 5fh, 1fh, 5dh, 0cbh
_init_sequence_params endp

/* 0x2841f */
_seek_sequence_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	push cs
	call near ptr _seek_sequence
	pop si
	pop di
	pop ds
	pop bp
	retf
_seek_sequence_far endp

/* 0x28431 */
_set_master_level_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	mov cx, [bp+6]
	push cs
	call near ptr _set_master_level
	pop si
	pop di
	pop ds
	pop bp
	retf
c_28443 label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 4eh, 0ah, 0eh, 0e8h, 0eh, 0e6h
	db 5eh, 5fh, 1fh, 5dh, 0cbh
_set_master_level_far endp

/* 0x28458 */
_install_driver_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	push cs
	call near ptr _install_driver
	pop si
	pop di
	pop ds
	pop bp
	retf
_install_driver_far endp

/* 0x2846a */
_configure_driver_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	push bx
	xor bx, bx
	les ax, dword ptr [bp+6]
	push cs
	call near ptr _configure_driver
	pop bx
	pop si
	pop di
	pop ds
	pop bp
	retf
_configure_driver_far endp

/* 0x28480 */
_start_sequence_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov cx, [bp+0ah]
	push cs
	call near ptr _start_sequence
	pop si
	pop di
	pop ds
	pop bp
	retf
c_28495 label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 56h, 0ah, 8ah, 4eh, 0ch, 8ah, 6eh, 0eh
	db 0eh, 0e8h, 1eh, 0e6h, 5eh, 5fh, 1fh, 5dh, 0cbh
_start_sequence_far endp

/* 0x284b0 */
_driver_fn13_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	mov cx, [bp+6]
	push cs
	call near ptr _driver_fn13
	pop si
	pop di
	pop ds
	pop bp
	retf
c_284c2 label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 4eh, 0ah, 0eh, 0e8h, 62h, 0e6h
	db 5eh, 5fh, 1fh, 5dh, 0cbh, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8bh, 4eh, 0ah
	db 8ah, 56h, 0ch, 0eh, 0e8h, 2bh, 0e9h, 5eh, 5fh, 1fh, 5dh, 0cbh
_driver_fn13_far endp

/* 0x284ef */
_retire_and_tick_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	push cs
	call near ptr _retire_and_tick
	pop si
	pop di
	pop ds
	pop bp
	retf
c_28501 label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 4eh, 0ah, 0eh, 0e8h, 0dch, 0e5h
	db 5eh, 5fh, 1fh, 5dh, 0cbh, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 0eh, 0e8h, 0dfh, 0e6h
	db 5eh, 5fh, 1fh, 5dh, 32h, 0e4h, 8ah, 0c1h, 0cbh
_retire_and_tick_far endp

/* 0x2852c */
_set_sequence_level_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	mov cl, [bp+6]
	push cs
	call near ptr _set_sequence_level
	pop si
	pop di
	pop ds
	pop bp
	retf
c_2853e label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 6eh, 0ah, 8ah, 4eh, 0ch, 8ah, 56h, 0eh
	db 0eh, 0e8h, 92h, 0e7h, 5eh, 5fh, 1fh, 5dh, 0cbh
_set_sequence_level_far endp

/* 0x28559 */
_silence_driver_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	push cs
	call near ptr _silence_driver
	pop si
	pop di
	pop ds
	pop bp
	retf
c_2856b label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8bh, 4eh, 0ah, 0eh, 0e8h, 7ch, 0e5h
	db 5eh, 5fh, 1fh, 5dh, 0cbh
_silence_driver_far endp
SOUND_TEXT ends
}
#else
/*
 * 0x2639d
 *
 * **The sound module's one far entry** (a name that is a guess): its arguments filed into the code segment at `cs:1ebh` on, then `sound_api_dispatch` on the function number. Nothing in the game calls it; the game calls the routines' own far faces. NOT TRANSCRIBED YET for the host: nothing the port runs reaches it. A
 * stub, which aborts; the TASM source above is the original's.
 */
uint16_t sound_api(uint16_t fn)
{
    (void)fn;
    not_transcribed("0x2639d");
    return 0;
}
/*
 * 0x263ff
 *
 * **The dispatcher**: CL chooses one of the driver interface's routines, each far-called, with the arguments `sound_api` filed. NOT TRANSCRIBED YET for the host: nothing the port runs reaches it. A
 * stub, which aborts; the TASM source above is the original's.
 */
void sound_api_dispatch(void)
{
    not_transcribed("0x263ff");
}

/*
 * 0x265f2
 *
 * Plant the driver and ask it what it is.
 *
 * The far pointer arrives in `ES:AX` and is written straight into the module's
 * own code segment at `cs:0x1e7` - the cell every other routine here far-calls
 * through. Nothing else installs it; this is where the sound module and the
 * loaded `SX.OVL` are joined.
 *
 * Then function 0 - `sx_describe_0` - which answers two constants. `CL` and
 * `CH` are kept at `cs:0x1ff` and `cs:0x1fc`, and `AH >> 4` at `cs:0x200`, with
 * bit 0 forced on when DGROUP 0x4aaa is set. For the speaker driver those come
 * out as 1, 0x12 and 0 - but they are read from the driver, not assumed, so a
 * different `SX.OVL` describes itself differently.
 *
 * Hand-written assembly: register arguments, no frame, a far `ret`. `BP` is
 * saved around the call because it carries the function number.
 *
 * `AX` is left holding `sx_describe_0`'s answer and the routine returns it -
 * not by writing it anywhere, just by not disturbing it, which is a return
 * value in assembly and is why the port declares one.
 */
uint16_t install_driver(const uint8_t far * drv)
{
    uint16_t ax, cx;
    uint8_t dl;

    SNDS.driver = far_of(drv);

    driver_describe_0(&ax, &cx);

    SNDS.cl = (uint8_t)cx;
    SNDS.ch = (uint8_t)(cx >> 8);

    dl = (uint8_t)((ax >> 8) >> 4);
    if (((int16_t)DG4A82.module_live) != 0)
        dl |= 1;
    SNDS.ah_high = dl;

    return ax;
}

/*
 * 0x26629
 *
 * Ask the driver its *second* description and set one parameter from it.
 *
 * Function 1 - `sx_describe_1` - answers another pair of constants, kept at
 * `cs:0x1fa` and `cs:0x1fb`. Then function 11 - `sx_param_349` - is called with
 * `CL` zero.
 *
 * `AX` and `CX` are pushed around that second call and popped back, so the
 * caller sees `sx_describe_1`'s answer and not `sx_param_349`'s - and that is
 * the routine's return value: 0x28580 tests it against 0xffff.
 *
 * Hand-written assembly, as above.
 */
uint16_t configure_driver(const uint8_t far * drv)
{
    uint16_t ax, cx;

    driver_describe_1(drv, &ax, &cx);

    SNDS.voice_lo = (uint8_t)cx;
    SNDS.voice_hi = (uint8_t)(cx >> 8);

    driver_param_349(0);

    return ax;
}

/*
 * 0x2664e
 *
 * Shut the driver up. Function 12 - `sx_param_345` - with `CL` 0xf, then
 * function 2 - `sx_stop_all`, which forwards to the speaker-off.
 *
 * Every register it touches is pushed and popped, `CX` included, so the two
 * calls are invisible to the caller. Hand-written assembly, as above.
 *
 * **`CL` is set once, to 0xf, and both calls see it** - function 12 preserves
 * CX. That matters because `SBP:`'s function 2 reads CL where the other
 * drivers ignore it, so the 0xf is passed on rather than dropped.
 */
void silence_driver(void)
{
    driver_param_345(0xf);
    driver_stop_all(0xf);
}

/*
 * 0x2666d
 *
 * **Hold or release the driver** (a name that is a guess): with CX zero the count at `cs:1f9h` goes up, otherwise it comes down to zero. Called only through `sound_api`'s table. NOT TRANSCRIBED YET for the host: nothing the port runs reaches it. A
 * stub, which aborts; the TASM source above is the original's.
 */
void sound_hold(uint16_t cx)
{
    (void)cx;
    not_transcribed("0x2666d");
}

/*
 * 0x26686
 *
 * **The driver's function 13**, and nothing else (the name says only that). Called only through `sound_api`'s table. NOT TRANSCRIBED YET for the host: nothing the port runs reaches it. A
 * stub, which aborts; the TASM source above is the original's.
 */
void driver_fn13(void)
{
    not_transcribed("0x26686");
}

/*
 * 0x26691
 *
 * **Seek a sequence** (a name that is a guess): its channels muted, `start_sequence` to restart it, `step_sequence` until its position reaches the one it had, the channels put back, and a tick. Called only through `sound_api`'s table. NOT TRANSCRIBED YET for the host: nothing the port runs reaches it. A
 * stub, which aborts; the TASM source above is the original's.
 */
void seek_sequence(struct sequence far *seq)
{
    (void)seq;
    not_transcribed("0x26691");
}

/*
 * 0x26721
 *
 * Set the driver's master level. `CL` is clamped to 0..0xf and handed to
 * function 12 - `sx_param_345` - except that 0xff passes through unclamped,
 * so it is a value the driver reads as something other than a level.
 *
 * Hand-written assembly: the argument is a register and there is no frame.
 */
void set_master_level(uint8_t cl)
{
    if (cl != 0xff && cl > 0xf)
        cl = 0xf;
    driver_param_345(cl);
}

/*
 * 0x26738
 *
 * **Set the level the playing sequence is heard at** (a name that is a guess): 0xff asks the driver, above 10 answers the current one at `cs:202h`, and otherwise it is stored and, if a sequence is playing, handed on. Called only through `sound_api`'s table. NOT TRANSCRIBED YET for the host: nothing the port runs reaches it. A
 * stub, which aborts; the TASM source above is the original's.
 */
uint16_t set_sequence_level(uint8_t cl)
{
    (void)cl;
    not_transcribed("0x26738");
    return 0;
}

/*
 * NOT a transcription of a routine of its own: the block of driver calls that
 * 0x26f2a contains **twice**, at 0x275a7 and again at 0x2772e, byte for byte.
 * Factored out so the difference between the two paths that use it - which is
 * only how they arrive - stays visible.
 *
 * `voice` is the driver's idea of a channel and `channel` the sequence's, and
 * the two are not the same number. Everything read out of the sequence is
 * indexed by `channel`; everything told to the driver is addressed to `voice`.
 */
static void tick_program_voice(struct sequence far * seq, uint16_t voice,
                               uint16_t channel)
{
    uint8_t cl, ch;
    uint16_t bend;

    driver_controller(voice, 0x7b00);                    /* all notes off */

    cl = (uint8_t)(seq->ch.voice_budget[channel] & 0xf);
    driver_controller(voice, (uint16_t)((0x4b << 8) | cl));

    cl = seq->ch.program[channel];
    driver_program_change(voice, cl);

    SNDS.pending_volume[voice] = 0xff;

    cl = scale_byte_pair(seq->ch.volume[channel], seq->volume);
    driver_controller(voice, (uint16_t)((7 << 8) | cl));

    cl = seq->ch.pan[channel];
    driver_controller(voice, (uint16_t)((0xa << 8) | cl));

    cl = seq->ch.modulation[channel];
    driver_controller(voice, (uint16_t)((1 << 8) | cl));

    cl = 0;
    if ((uint8_t)(seq->ch.bend[channel] >> 8) >= 0x80)
        cl = 0x7f;
    driver_controller(voice, (uint16_t)((0x40 << 8) | cl));

    bend = seq->ch.bend[channel];
    ch = (uint8_t)bend;
    cl = (uint8_t)((bend >> 8) << 1);
    if (ch >= 0x80)
        cl |= 1;
    driver_pitch_bend(voice, (uint16_t)((((uint16_t)ch << 8) | cl) & 0x7f7f));

    cl = seq->ch.note[channel];
    driver_controller(voice, (uint16_t)((0x4e << 8) | cl));
}

/*
 * NOT a transcription of its own routine either: the four sixteen-byte arrays
 * that 0x26f2a snapshots before it tries to place a sequence's channels, and
 * puts back if the placement fails. The original writes both copies out
 * unrolled, eight words at a time; they are loops here.
 */
static void tick_save_state(void)
{
    int16_t i;

    for (i = 0; i < 0x10; i++) {
        SNDS.saved_request[i] = SNDS.voice_request[i];
        SNDS.saved_cost[i] = SNDS.voice_cost[i];
        SNDS.saved_gives_back[i] = SNDS.voice_gives_back[i];
        SNDS.saved_keep_own[i] = SNDS.voice_keep_own[i];
    }
}

/*
 * NOT a transcription either: the other half of the pair above, putting back
 * what a failed placement changed.
 */
static void tick_restore_state(void)
{
    int16_t i;

    for (i = 0; i < 0x10; i++) {
        SNDS.voice_request[i] = SNDS.saved_request[i];
        SNDS.voice_cost[i] = SNDS.saved_cost[i];
        SNDS.voice_gives_back[i] = SNDS.saved_gives_back[i];
        SNDS.voice_keep_own[i] = SNDS.saved_keep_own[i];
    }
}

/*
 * 0x26783
 *
 * Start a sequence: stop it if it is already playing, reset every channel it
 * has, read its header, and put it in the playing table in priority order.
 * Hand-written assembly with the record in `es:ax` and a flag in `cx`.
 *
 * It runs with interrupts disabled from end to end, because the table it edits
 * is the one the timer walks.
 *
 * The reset loop covers channels 0 to 14 in full and then does channel 15
 * **partially**: 15 gets +0x8c, +0x9c, +0xac and the words at +0xc, +0x2c,
 * +0x4c, but not +0x6c, +0xbc or any of the six per-channel bytes the others
 * get. That is the original's shape, not a transcription slip - the loop test
 * is `si != 0xf`, and the tail after it writes only some of what the body did.
 *
 * The header walk follows an offset table: `ds:[bp]` is a displacement from the
 * table's own base, and a zero entry ends the walk. An entry of 0xfe is a gap -
 * with `cs:0x200` clear the channel is marked 0xfe and skipped, and with it set
 * the walk stops and records how far it got in +0x165.
 *
 * The channel a header entry configures is **not** the loop index: the loop
 * index picks the entry, and the low nibble of the entry's own first byte picks
 * the channel. The three per-channel defaults are only written where the field
 * is still 0xff, so an earlier entry wins over a later one.
 *
 * Placement is an insertion sort, descending by +0x15c: the first entry whose
 * key is less than or equal to the new one is where it goes, and everything
 * from there is shifted up one slot. A full table drops the sequence silently.
 * With `cs:0x209` set the sequence is placed but its counters are left alone
 * and the tick is not run.
 */
void start_sequence(struct sequence far * seq, uint16_t cx)
{
    uint16_t di, si, bp, base;
    uint8_t dl, dh, key;

    for (di = 0; di < 0x40; di += 4) {
        if (SEQUENCE_PTR(SNDS.playing[di / 4]) == seq) {
            remove_sequence(seq);
            sequencer_tick();
            break;
        }
    }

    seq->mode = 1;
    if (cx != 0)
        seq->mode++;

    init_sequence_params(seq);

    for (si = 0; si < 0xf; si++) {
        seq->position[si] = 0xd;
        seq->position_saved[si] = 3;
        seq->delay[si] = 0;
        seq->delay_saved[si] = 0;
        seq->ch.bend[si] = 0x2000;
        seq->track_channel[si] = 0xff;
        seq->status[si] = 0;
        seq->status_saved[si] = 0;
        seq->ch.voice_budget[si] = 0xff;
        seq->ch.modulation[si] = 0;
        seq->ch.program[si] = 0xff;
        seq->ch.volume[si] = 0xff;
        seq->ch.pan[si] = 0xff;
        seq->ch.note[si] = 0xff;
        seq->ch.channel_flags[si] = 0;
        seq->ch.no_voice[si] = 0;
    }

    seq->track_channel[0xf] = 0xff;
    seq->status[0xf] = 0;
    seq->status_saved[0xf] = 0;
    seq->poll = 0;
    seq->rewind_mark = 0;
    seq->device_value = 0x7f;
    seq->position[0xf] = 0xd;
    seq->position_saved[0xf] = 3;
    seq->delay[0xf] = 0;
    seq->ticks_saved = 0;

    {
        /* Both are far pointers stored in records: the sequence's own at
           +8, and the table that one points at, each followed once. */
        const uint8_t far *tbl = *seq->cursor_at;

        if (tbl[0x20] != 0xff && seq->keep_priority == 0)
            seq->priority = tbl[0x20];

        base = 0;
        si = 0;
        bp = 0;

        for (;;) {
            uint16_t entry = *(const uint16_t *)(tbl + bp);
            const uint8_t far *e;

            if (entry == 0)
                break;

            e = tbl + base + entry;
            dl = e[0];

            if (dl == 0xfe) {
                if (SNDS.ah_high != 0) {
                    seq->poll = (uint8_t)(si + 1);
                    break;
                }
                seq->position[si] = 0;
                seq->position_saved[si] = 0;
                seq->track_channel[si] = 0xfe;
            } else {
                uint16_t channel;

                seq->track_channel[si] = dl;
                seq->status[si] = (uint8_t)(dl | 0xb0);

                dl = e[0xc];
                dh = 0;
                if (dl == 0xf8) {
                    dl = 0xf0;
                    dh = 0x80;
                }
                seq->delay[si] = (uint16_t)(((uint16_t)dh << 8) | dl);

                dl = seq->track_channel[si];
                seq->track_channel[si] &= 0xf;
                channel = (uint16_t)(dl & 0xf);

                if ((dl & 0x10) != 0) {
                    seq->position[si] = 3;
                    seq->delay[si] = 0;
                    seq->ch.channel_flags[channel] |= 2;
                } else {
                    int16_t do_f8 = 1;

                    if ((dl & 0x20) != 0)
                        seq->ch.channel_flags[channel] |= 1;
                    if ((dl & 0x40) != 0)
                        seq->ch.no_voice[channel] = 1;

                    if (channel == 0xf) {
                        if (seq->device_value == 0x7f) {
                            seq->device_value = e[8];
                            do_f8 = 0;
                        }
                    } else {
                        if (seq->ch.voice_budget[channel] == 0xff)
                            seq->ch.voice_budget[channel] = e[1];
                        if (seq->ch.program[channel] == 0xff)
                            seq->ch.program[channel] = e[4];
                        if (seq->ch.volume[channel] == 0xff)
                            seq->ch.volume[channel] = e[8];
                    }

                    if (do_f8 && seq->ch.pan[channel] == 0xff)
                        seq->ch.pan[channel] = e[0xb];
                }
            }

            si++;
            bp = (uint16_t)(2 * si);
            if (si == 0x10)
                break;
        }
    }

    if (seq->mode == 2) {
        for (di = 0xe; (int16_t)di >= 0; di--)
            seq->ch.channel_flags[di] |= 1;
    }

    key = seq->priority;

    for (di = 0; di < 0x40; di += 4) {
        if (SNDS.playing[di / 4].seg == 0)
            break;
        if (SEQUENCE_PTR(SNDS.playing[di / 4])->priority <= key) {
            /*
             * **The comparison is 16 bits and has to wrap.** The original
             * computes it in BX - `mov bx,si / add bx,4 / cmp bx,di` at
             * 0x269ee - so when `di` is 0 the walk runs down to 0xfffc, BX
             * comes out 0, and it stops. Written as `si + 4` in C the addition
             * promotes to `int`, 0xfffc + 4 is 0x10000 rather than 0, and the
             * loop never ends: it ran off the bottom of the table writing
             * pairs of words over guest memory until the frame rate collapsed
             * from thirty a second to one every two seconds.
             *
             * Reached by a sound played from a part's step - `play_sound(12)`
             * out of `part_step_generator` - with the new sequence's key at or
             * below the first entry's, which is what makes `di` zero.
             */
            for (si = 0x38; (uint16_t)(si + 4) != di; si -= 4) {
                SNDS.playing[si / 4 + 1] = SNDS.playing[si / 4];
            }
            break;
        }
    }
    if (di >= 0x40)
        return;

    SNDS.playing[di / 4] = far_of((uint8_t *)seq);

    if (SNDS.muted != 0)
        return;

    seq->loop_count = 0;
    seq->ticks = 0;
    seq->state = 0;
    seq->fade_target = 0;
    seq->fade_period = 0;
    seq->fade_countdown = 0;
    seq->fade_step = 0;
    seq->skip = 0;

    sequencer_tick();
}

/*
 * 0x26a57
 *
 * Retire whatever has finished and run the sequencer once, with interrupts
 * masked across both. Six instructions: `pushf`, `cli`, the two near calls,
 * `popf`, `retf`.
 *
 * The mask is the point of the routine - `remove_sequence` unlinks records that
 * `sequencer_tick` is about to walk, and the timer interrupt calls
 * `sound_service` which walks the same list. Doing it with the flag saved and
 * restored rather than a bare `sti` means a caller that already had interrupts
 * off keeps them off.
 *
 * `ES:AX` is not touched here and is not this routine's own: it arrives in the
 * registers and `remove_sequence` reads it, so the port passes it through.
 *
 * Hand-written assembly, no frame, a far `ret`.
 */
void retire_and_tick(struct sequence far * seq)
{
    io_lock();                  /* `pushf`, `cli` */
    remove_sequence(seq);
    sequencer_tick();
    io_unlock();                /* `popf` - which is why the lock is recursive */
}

/*
 * 0x26e7b
 *
 * Take a sequence out of the playing table and stop it. Hand-written assembly
 * with the record in `es:ax`.
 *
 * The table is the sixteen far pointers at the module's `cs:8`, the ones
 * `sequencer_tick` walks. The matching entry is cleared and every entry above
 * it moved down one, so the table stays packed with no holes for the tick to
 * skip over - and the last slot is cleared afterwards, because the shift leaves
 * a duplicate there. A record that is not in the table at all is simply
 * ignored.
 *
 * Then the record's own +0x158 is set to 0xff and +0x159 to zero.
 *
 * The rest only happens when +0x165 is non-zero **and at least 0x80**, and what
 * it computes is thrown away: two far pointers are followed and an index taken
 * from the low nibble of +0x165 is used to read an offset and add it to BP -
 * after which BP and DS are both restored by the epilogue and AX is
 * overwritten with zero. The call that follows takes 5 and 0, whatever that
 * arithmetic produced. Dead as written, and transcribed as the condition it
 * still is: the callback happens only for +0x165 >= 0x80.
 */
void remove_sequence(struct sequence far * seq)
{
    int16_t i;

    /* The table's pairs are filed from pointers to DOS blocks, so comparing
       the pointer is comparing the pair `es:ax` was matched against. */
    for (i = 0; i < 0x10; i++)
        if (SEQUENCE_PTR(SNDS.playing[i]) == seq)
            break;
    if (i >= 0x10)
        return;

    SNDS.playing[i] = FAR_NULL;

    if (i != 0xf) {
        for (; i != 0xf; i++)
            SNDS.playing[i] = SNDS.playing[i + 1];
        SNDS.playing[i] = FAR_NULL;
    }

    seq->state = 0xff;
    seq->mode = 0;

    if (seq->poll == 0)
        return;
    if (seq->poll < 0x80)
        return;

    /*
     * `xor ax,ax; push ax; mov ax,5; push ax` - so the **function number is
     * 5** and the argument pointer is null. Read in source order the two come
     * out swapped, which is what this used to say.
     */
    sound_callback(5, NULL);
}

/*
 * 0x26f2a
 *
 * The sequencer's tick: decide which of sixteen hardware voices plays each
 * channel of each playing sequence, and tell the driver about everything that
 * changed. 2494 bytes of hand-written assembly, the largest routine in the
 * game, called from the timer interrupt.
 *
 * It works on five sixteen-byte arrays in the module's own code segment:
 *
 *   0x128  the voice assignment now in force, 0xff for a free voice
 *   0x168  the assignment being *requested* this tick, 0xff for none
 *   0x148  what the request costs
 *   0x158  what it contributes back if it is dropped
 *   0x138  a flag saying the request must keep its own voice number
 *
 * A request is a byte packing the sequence in the high nibble and the channel
 * in the low, so the sixteen sequences are reached as `cs:[8 + 4 * sequence]`
 * and a request byte can be turned back into both halves.
 *
 * Placement runs per sequence, over that sequence's sixteen channels. Channels
 * marked 0xff, 0xfe or 0x0f in the map at `+0x8c` are skipped, and so are those
 * whose flags at `+0x134` have bit 1 or whose byte at `+0x143` is set. What is
 * left needs a voice within the range `cs:0x1fa`..`cs:0x1fb`.
 *
 * A voice in hand still has to be paid for. When no voice is free, or the
 * running total does not cover what the channel costs, the loudest
 * already-placed request is dropped - its voice becoming this channel's - and
 * its contribution added to the total, repeatedly, until the total covers what
 * this channel needs - so quiet requests are given up before loud ones, and
 * only as many as are actually needed. If that still is not enough the
 * whole sequence is abandoned and `tick_restore_state` puts back everything the
 * attempt changed, which is why the snapshot is taken per sequence rather than
 * once.
 *
 * A channel whose `+0x134` has bit 0 must have the voice matching its own
 * number. If that voice went to another channel the two are swapped outright;
 * if the swap is not possible the request is dropped instead.
 *
 * Then every voice whose request differs from what it is playing is
 * reprogrammed - `tick_program_voice` - and voices that were playing and are
 * not wanted are silenced. The two remaining passes hand out any voice still
 * unclaimed and then rebuild the per-voice sequence pointers at `cs:0x88`.
 *
 * `cs:0x1f9` is incremented on the way in and decremented on the way out. It is
 * a depth count, not a lock: nothing here tests it, and it is `0x27ace` - the
 * entry the interrupt actually calls - that refuses to run when it is set.
 */
void sequencer_tick(void)
{
    int16_t i, seq, voice, ch_i;
    struct sequence far *rec;
    uint16_t bp_;
    uint8_t al, ah, cl, chh, dl, dh;

    SNDS.busy++;
    SNDS.voices_changed = 0;

    for (i = 0; i < 0x10; i++) {
        SNDS.voice_held[i] = 0xff;
        SNDS.voice_gives_back[i] = 0;
        SNDS.voice_keep_own[i] = 0;
        SNDS.voice_cost[i] = 0;
        SNDS.voice_request[i] = 0xff;
    }
    SNDS.polled[0] = FAR_NULL;

    rec = SEQUENCE_PTR(SNDS.playing[0]);

    if (rec == SEQUENCE_NONE) {
        for (i = 0; i < 0x10; i++)
            SNDS.voice_held[i] = 0xff;
        goto silence_unused;
    }

    cl = rec->device_value;
    if (cl == 0x7f)
        cl = SNDS.param_default;
    driver_param_349(cl);

    al = SNDS.cl;

    bp_ = 0;
    for (seq = 0; seq < 0x40; seq += 4) {
        rec = SEQUENCE_PTR(SNDS.playing[seq / 4]);
        if (rec == SEQUENCE_NONE)
            break;

        if (rec->skip != 0)
            goto next_sequence;

        if (rec->poll != 0) {
            if (dg_far_ptr(SNDS.polled[0]) != FAR_NULL_PTR)
                goto next_sequence;
            SNDS.polled[0] = SNDS.playing[seq / 4];
            goto next_sequence;
        }

        tick_save_state();
        /*
         * The running total carried across sequences is parked here, not
         * zeroed: the abandon path below reads it back so a sequence that
         * fails leaves the total exactly as it found it.
         */
        SNDS.saved_total = al;

        for (ch_i = 0; ch_i < 0x10; ch_i++) {
            cl = rec->track_channel[ch_i];
            if (cl == 0xff || cl == 0xfe || cl == 0x0f)
                continue;
            if ((rec->ch.channel_flags[cl] & 2) != 0)
                continue;
            if (rec->ch.no_voice[cl] != 0)
                continue;

            dl = (uint8_t)((seq * 4) | cl);

            ah = (uint8_t)(rec->ch.voice_budget[cl] & 0xf);
            chh = (uint8_t)(rec->ch.voice_budget[cl] >> 4);
            if (chh != 0)
                chh = (uint8_t)(0x10 - chh + bp_);

            if ((rec->ch.channel_flags[cl] & 1) != 0
                && SNDS.voice_request[cl] == 0xff) {
                dh = cl;
                goto check_budget;
            }

            dh = 0xff;
            {
                int16_t bl;

                for (bl = 0; bl < 0x10; bl++) {
                    if (SNDS.voice_request[bl] == 0xff) {
                        if (bl >= (int16_t)SNDS.voice_lo
                            && bl <= (int16_t)SNDS.voice_hi)
                            dh = (uint8_t)bl;
                    } else if (SNDS.voice_request[bl] == dl) {
                        goto next_channel;
                    }
                }
            }
            if (dh != 0xff)
                goto check_budget;

            if (chh != 0)
                goto next_sequence;
            goto drop_loudest;

            /*
             * 0x272c2: a voice is in hand, but only if what is left covers the
             * cost. A request that cannot drop anything gives up on this
             * channel; one that can drops the loudest placed request - and
             * **takes its voice**, because the scan leaves it in DH, which is
             * where the request is then put. The port kept the scan's answer
             * in a local and put the request at DH's 0xff, past the table.
             */
check_budget:
            if (ah <= al)
                goto have_voice;
            if (chh != 0)
                goto next_channel;

            /* 0x27270 and 0x272ce, the same scan written out twice. */
drop_loudest:
            {
                uint8_t most = 0;

                dh = 0xff;
                for (i = 0; i < 0x10; i++) {
                    if (most < SNDS.voice_cost[i]) {
                        most = SNDS.voice_cost[i];
                        dh = (uint8_t)i;
                    }
                }
                if (dh == 0xff)
                    goto abandon_sequence;

                al = (uint8_t)(al + SNDS.voice_gives_back[dh]);
                SNDS.voice_request[dh] = 0xff;
                SNDS.voice_gives_back[dh] = 0;
                SNDS.voice_cost[dh] = 0;
                SNDS.voice_keep_own[dh] = 0;
            }
            if (ah > al)
                goto drop_loudest;

have_voice:
            SNDS.voice_request[dh] = dl;
            SNDS.voice_gives_back[dh] = ah;
            al = (uint8_t)(al - ah);
            SNDS.voice_cost[dh] = chh;

            if ((rec->ch.channel_flags[cl] & 1) == 0) {
                SNDS.voice_keep_own[dh] = 0;
                continue;
            }

            SNDS.voice_keep_own[dh] = 1;
            if (dh == cl)
                continue;

            if (SNDS.voice_keep_own[cl] == 0) {
                uint8_t t;

                t = SNDS.voice_request[dh];
                SNDS.voice_request[dh] = SNDS.voice_request[cl];
                SNDS.voice_request[cl] = t;
                t = SNDS.voice_cost[dh];
                SNDS.voice_cost[dh] = SNDS.voice_cost[cl];
                SNDS.voice_cost[cl] = t;
                t = SNDS.voice_gives_back[dh];
                SNDS.voice_gives_back[dh] = SNDS.voice_gives_back[cl];
                SNDS.voice_gives_back[cl] = t;
                t = SNDS.voice_keep_own[dh];
                SNDS.voice_keep_own[dh] = SNDS.voice_keep_own[cl];
                SNDS.voice_keep_own[cl] = t;
                continue;
            }

            if (chh != 0) {
                SNDS.voice_request[dh] = 0xff;
                SNDS.voice_cost[dh] = 0;
                SNDS.voice_gives_back[dh] = 0;
                SNDS.voice_keep_own[dh] = 0;
                al = (uint8_t)(al + ah);
                continue;
            }

            if (SNDS.voice_cost[cl] != 0)
                goto abandon_sequence;

            al = (uint8_t)(al + SNDS.voice_gives_back[cl]);
            SNDS.voice_request[dh] = 0xff;
            SNDS.voice_gives_back[dh] = 0;
            SNDS.voice_cost[dh] = 0;
            SNDS.voice_keep_own[dh] = 0;
            SNDS.voice_request[cl] = dl;
            SNDS.voice_cost[cl] = chh;
            SNDS.voice_gives_back[cl] = ah;
            al = (uint8_t)(al - ah);

next_channel:
            ;
        }
        goto next_sequence;

abandon_sequence:
        tick_restore_state();
        al = SNDS.saved_total;

next_sequence:
        bp_ = (uint16_t)(bp_ + 0x10);
    }

    /* Apply: reprogram every voice whose request differs from what it plays. */
    for (voice = 0; voice < 0x10; voice++) {
        if (SNDS.voice_request[voice] == 0xff)
            continue;

        if (SNDS.voice_keep_own[voice] == 0) {
            uint8_t want = SNDS.voice_request[voice];
            int16_t d;

            al = (uint8_t)(want & 0xf);

            d = SNDS.voice_lo;
            for (;;) {
                if (dg_far_ptr(SNDS.voice_sequence[d])
                        == dg_far_ptr(SNDS.playing[want >> 4])
                    && SNDS.voice_channel[d] == al) {
                    if (SNDS.voice_keep_own[d] == 0) {
                        SNDS.voice_held[d] = SNDS.voice_request[voice];
                        SNDS.voice_request[voice] = 0xff;
                    }
                    break;
                }
                d++;
                if ((int16_t)SNDS.voice_hi < d - 1)
                    break;
            }
            continue;
        }

        {
            uint8_t want = SNDS.voice_request[voice];

            SNDS.voice_request[voice] = 0xff;
            SNDS.voice_held[voice] = want;

            al = (uint8_t)(want & 0xf);

            if (SNDS.voice_channel[voice] == al
                && dg_far_ptr(SNDS.voice_sequence[voice])
                       == dg_far_ptr(SNDS.playing[want >> 4]))
                continue;

            tick_program_voice(SEQUENCE_PTR(SNDS.playing[want >> 4]),
                               (uint16_t)voice, al);
        }
    }

    /* Hand out anything still requested to a voice that is still free. */
    {
        int16_t free_from = (int16_t)(uint8_t)(SNDS.voice_hi + 1);

        for (voice = 0; voice < 0x10; voice++) {
            uint8_t want = SNDS.voice_request[voice];
            int16_t d;

            if (want == 0xff)
                continue;

            d = free_from;
            do {
                d--;
            } while (SNDS.voice_held[d] != 0xff);
            free_from = d;

            SNDS.voice_held[d] = want;
            al = (uint8_t)(want & 0xf);

            tick_program_voice(SEQUENCE_PTR(SNDS.playing[want >> 4]),
                               (uint16_t)d, al);
        }
    }

silence_unused:
    for (voice = 0xf; voice >= 0; voice--) {
        if (SNDS.voice_channel[voice] == 0xf)
            continue;
        if (SNDS.voice_held[voice] != 0xff)
            continue;
        driver_controller((uint16_t)voice, 0x4000);
        driver_controller((uint16_t)voice, 0x7b00);
        driver_controller((uint16_t)voice, 0x4b00);
    }

    /* Eight word moves masked with 0x0f0f in the original; the same sixteen
       bytes one at a time. */
    for (i = 0; i < 0x10; i++)
        SNDS.voice_channel[i] = (uint8_t)(SNDS.voice_held[i] & 0x0f);

    for (voice = 0; voice < 0x10; voice++) {
        uint8_t held = SNDS.voice_held[voice];

        if (held == 0xff) {
            SNDS.voice_sequence[voice] = FAR_NULL;
        } else {
            SNDS.voice_sequence[voice] = SNDS.playing[held >> 4];
        }
    }

    SNDS.busy--;
}

/*
 * 0x278e9
 *
 * Advance a sequence's volume fade by one tick. Hand-written assembly with the
 * record in `es:bx` and the sequence's slot in `si`.
 *
 * +0x162 counts ticks down to the next step and is reloaded from +0x161, so a
 * fade moves once every +0x161 ticks rather than every tick. +0x160 holds the
 * target with a flag in its top bit, +0x163 the largest step allowed, and
 * +0x15e the volume now.
 *
 * Each step moves toward the target by at most +0x163, and lands exactly on it
 * when what remains is no more than a step - so a fade always finishes on the
 * target rather than oscillating around it.
 *
 * Arriving sets +0x158 to 0xfe and clears the step, and **if the top bit of
 * +0x160 is set the sequence is then removed altogether**. That is how a fade
 * to silence stops a sequence: the flag rides along in the spare bit of the
 * target, which is why every read of the target masks it off.
 */
void advance_volume_ramp(struct sequence far * seq, uint16_t seq_slot)
{
    uint8_t target, now, distance;

    if (seq->fade_countdown != 0) {
        seq->fade_countdown--;
        return;
    }
    seq->fade_countdown = seq->fade_period;

    target = (uint8_t)(seq->fade_target & 0x7f);
    now = seq->volume;

    if (target != now) {
        if (target > now) {
            distance = (uint8_t)(target - now);
            if (distance > seq->fade_step) {
                set_sequence_volume(seq, (uint8_t)(now + seq->fade_step), 1,
                                    seq_slot);
                return;
            }
        } else {
            distance = (uint8_t)(now - target);
            if (distance > seq->fade_step) {
                set_sequence_volume(seq, (uint8_t)(now - seq->fade_step), 1,
                                    seq_slot);
                return;
            }
        }
        set_sequence_volume(seq, target, 1, seq_slot);
    }

    seq->state = 0xfe;
    seq->fade_step = 0;

    if ((seq->fade_target & 0x80) != 0) {
        remove_sequence(seq);
        SNDS.voices_changed = 1;
    }
}

/*
 * 0x279a9
 *
 * Set a sequence's volume and push it out to every voice the sequence owns.
 *
 * `defer` goes to `cs:0x205` and chooses how: set, the new value is left in the
 * pending array at `cs:0x1c8` for `flush_pending_volumes` to send two at a time
 * from the timer tick; clear, the driver is told immediately and the pending
 * entry is marked 0xff so the flush skips it. A fade always defers, which is
 * what stops a slow fade flooding the interrupt with controller changes.
 *
 * Nothing happens at all if the volume is already what is asked for, and
 * nothing is sent if the sequence has no slot - `0xff` - although the volume is
 * still stored, so a sequence that is not playing still remembers it.
 *
 * Two passes, and they are not the same. The first walks the sixteen voices and
 * takes those whose owner's high nibble matches this sequence, using the voice
 * number as the driver's channel. The second walks the sequence's own channel
 * map at +0x8c and takes only channels with bit 1 at +0x134 that hold **no**
 * voice, using the channel number as the driver's channel instead. The second
 * pass stops at the first 0xff in the map rather than skipping it.
 *
 * Each voice's volume is its own +0x107 scaled by the sequence's, through
 * `scale_byte_pair`.
 */
void set_sequence_volume(struct sequence far * seq, uint8_t volume,
                         uint8_t defer, uint16_t seq_slot)
{
    uint16_t si, di;
    uint8_t want, level;

    SNDS.defer = defer;

    if (volume == seq->volume)
        return;
    seq->volume = volume;

    if (seq_slot == 0xff)
        return;

    want = (uint8_t)(seq_slot << 2);

    for (si = 0; si < 0x10; si++) {
        uint8_t held = SNDS.voice_held[si];

        if (held == 0xff || (uint8_t)(held & 0xf0) != want)
            continue;

        di = (uint16_t)(held & 0xf);
        level = scale_byte_pair(seq->ch.volume[di], seq->volume);

        if (SNDS.defer != 0) {
            SNDS.pending_volume[si] = level;
        } else {
            SNDS.pending_volume[si] = 0xff;
            driver_controller(si, (uint16_t)((7 << 8) | level));
        }
    }

    for (si = 0; si < 0x10; si++) {
        di = seq->track_channel[si];
        if (di == 0xff)
            return;
        if ((seq->ch.channel_flags[di] & 2) == 0)
            continue;
        if (SNDS.voice_held[di] != 0xff)
            continue;

        level = scale_byte_pair(seq->ch.volume[di], seq->volume);

        if (SNDS.defer != 0) {
            SNDS.pending_volume[di] = level;
        } else {
            SNDS.pending_volume[di] = 0xff;
            driver_controller(di, (uint16_t)((7 << 8) | level));
        }
    }
}

/*
 * 0x27a86
 *
 * Flush up to two pending volume changes to the driver, round-robin over the
 * sixteen channels, and remember where to resume.
 *
 * The array at the module's own `cs:0x1c8` holds one byte per channel, with
 * 0xff meaning nothing pending. A channel with anything else is marked 0xff
 * again and its value sent to the driver as MIDI controller 7 - volume - on
 * that channel.
 *
 * **At most two per call.** The scan then stops wherever it is, and `cs:0x206`
 * carries that position into the next call, so sixteen channels are serviced
 * over eight calls rather than all at once. This runs from the timer tick, and
 * sending sixteen controller changes inside one interrupt would be the thing
 * it is avoiding.
 *
 * The scan is also bounded by returning to where it started, so a pass with
 * nothing pending walks the ring once and stops rather than spinning.
 *
 * The original reaches the driver by a far call through `cs:[0x1e7]` with the
 * function number in BP; 7 selects `sx_controller`, which is what the port
 * calls directly. The number is fixed for this driver, not looked up.
 *
 * This is hand-written assembly - no frame, no arguments, a near `ret`.
 */
void flush_pending_volumes(void)
{
    uint16_t si = SNDS.scan_stopped;
    int16_t sent = 0;

    for (;;) {
        uint8_t pending = SNDS.pending_volume[si];

        if (pending != 0xff) {
            SNDS.pending_volume[si] = 0xff;
            driver_controller(si, (uint16_t)((7 << 8) | pending));
            sent++;
            if (sent == 2)
                break;
        }

        si++;
        if (si == 0x10)
            si = 0;
        if (si == SNDS.scan_stopped)
            break;
    }

    SNDS.scan_stopped = (uint8_t)si;
}

/*
 * 0x27ace
 *
 * The sound module's service routine - what the timer calls. Runs every
 * playing sequence forward one tick, then polls, flushes and tells the host.
 *
 * The first thing it does is **refuse to run** while `cs:0x1f9` is non-zero.
 * That is the depth count `sequencer_tick` maintains, so a tick already in
 * progress cannot be re-entered by the interrupt that fires during it. It is
 * the only place that guard is tested.
 *
 * Interrupts are then disabled for the whole body, because the playing table is
 * the one `start_sequence` and `remove_sequence` edit.
 *
 * `cs:0x204` set means something changed which voice plays what, so the
 * allocator is run before anything else.
 *
 * The walk keeps **two** indices. `si` is the position in the table now, and
 * `di` the position a sequence started at. They advance together until a
 * sequence is removed - `+0x158` reading 0xff - and then only `di` advances,
 * because removing an entry shifted everything below it down and the next
 * sequence is now at the same `si`. The original writes that as `sub si,4`
 * followed by the shared `add si,4`, and as a jump past the `add`; both mean
 * the same thing.
 *
 * The two indices are not interchangeable: the fade gets `si` and the stepper
 * gets `di`.
 *
 * A sequence with +0x164 set is skipped entirely. One with a fade step at
 * +0x163 has its fade advanced first. Then either it is dropped for not being
 * on the poll table, if +0x165 says it should be polled, or it is stepped.
 *
 * Afterwards `poll_sequences` and `flush_pending_volumes` run once, and the
 * host callback is asked question 3.
 */
void sound_service(void)
{
    uint16_t si, di;

    if (SNDS.busy != 0)
        return;

    if (SNDS.voices_changed != 0)
        sequencer_tick();

    si = 0;
    di = 0;

    while (si != 0x40) {
        struct sequence far *seq = SEQUENCE_PTR(SNDS.playing[si / 4]);

        if (seq == SEQUENCE_NONE)
            break;

        if (seq->skip != 0) {
            si += 4;
            di += 4;
            continue;
        }

        if (seq->fade_step != 0) {
            advance_volume_ramp(seq, si);
            if (seq->state == 0xff) {
                di += 4;
                continue;
            }
        }

        if (seq->poll != 0)
            drop_unless_polled(seq);
        else
            step_sequence(seq, di);

        if (seq->state != 0xff)
            si += 4;
        di += 4;
    }

    poll_sequences();
    flush_pending_volumes();

    /*
     * A **driver** call, not the host callback: this goes through `cs:[0x1e7]`
     * with the function number in BP, and 3 is one of the seven table entries
     * pointing at the do-nothing stub. Reading it as the host callback at
     * 0x292a1 - which is also reached with a 3 - leaves that routine's result
     * slot at `cs:0x30fa` holding 3 where the original leaves 0, which is
     * exactly how the mistake showed up.
     */
    driver_nop();
}

/*
 * 0x27b52
 *
 * Remove a sequence unless it is on the poll table at `cs:0x48`.
 *
 * A sequence that has asked to be polled is left alone - `poll_sequences` owns
 * it and will decide when it ends. Anything else is taken out of the playing
 * table and `cs:0x204` is set to say the table changed.
 *
 * The search compares both halves of the far pointer. The port compares the
 * pointer, which is the same test: both tables hold pairs `sequencer_tick`
 * copied from the playing table, so one record is never filed two ways.
 */
void drop_unless_polled(struct sequence far * seq)
{
    int16_t si;

    for (si = 0; si < 0x40; si += 4)
        if (SEQUENCE_PTR(SNDS.polled[si / 4]) == seq)
            return;

    remove_sequence(seq);
    SNDS.voices_changed = 1;
}

/*
 * 0x27b7e
 *
 * Poll every sequence that has asked to be polled, and let the host callback
 * decide whether it carries on.
 *
 * The table walked here is at the module's `cs:0x48` and is **not** the playing
 * table at `cs:8` - it is the second one, the entries `sequencer_tick` parks
 * there for sequences whose +0x165 marks them as needing attention. A null
 * entry ends the whole walk, not just that iteration, so the table is expected
 * to be packed.
 *
 * Each sequence's counter at +0x154 is bumped, and then the byte at +0x165
 * chooses between two calls. At 0x10 or below the sequence is marked with bit
 * 0x80 and the callback is asked question 3; above 0x10 it is asked question 4,
 * and the answer decides: a non-zero high byte resets the counter, a non-zero
 * low byte clears +0x165, removes the sequence from the playing table and sets
 * `cs:0x204`.
 *
 * Both calls build a small block of arguments **on the stack** and pass its
 * address, and question 3's is the five words a digitised module's "play this"
 * takes: flags, rate, the sample's far pointer and its length. **This is where
 * a sampled sound is started**, and the only place in the game that starts
 * one - the nine wrappers at 0x0bb98 never ask a module to play.
 *
 * With no module installed `sound_callback` answers the DGROUP segment, whose
 * low byte is non-zero, so question 4 removes every sequence on this table -
 * which is the behaviour of a machine with no digitised sound and is why the
 * game runs happily without one.
 */
void poll_sequences(void)
{
    int16_t si;

    for (si = 0; si < 0x40; si += 4) {
        struct sequence far *rec = SEQUENCE_PTR(SNDS.polled[si / 4]);
        const uint8_t far *at;
        const uint8_t far *data;
        uint16_t answer;
        uint8_t cl;

        if (rec == SEQUENCE_NONE)
            return;

        rec->ticks++;

        /*
         * Two far pointers followed - `lds bp, es:[bx+8]` and then
         * `lds bp, ds:[bp]` - land on the sequence's own data, and the low
         * nibble of +0x165, less one and doubled, indexes a table of offsets
         * there. `data` - the original's `ds:bp` - ends up on the record the
         * callback is asked about. The second pointer, `at`, is kept: an
         * offset stepped inside its segment is what the module is handed
         * below.
         */
        at = *rec->cursor_at;
        data = at;

        cl = (uint8_t)((rec->poll & 0x0f) - 1);
        cl = (uint8_t)(cl << 1);
        data += *(const uint16_t *)(data + cl);

        if (rec->poll <= 0x10) {
            const uint8_t far *b = data + 1;

            rec->poll |= 0x80;

            /*
             * A leading 0xfe is stepped over, and then one more byte, which
             * puts `b` on the record proper: its first word is the sampling
             * rate, its second the length, and the sample itself starts eight
             * bytes in.
             */
            if (*b == 0xfe)
                b++;
            b++;

            /*
             * The five words the module reads through SI, pushed length first
             * so the last pushed - the volume and the loop - is what SI points
             * at. The sample is filed as `at`'s segment beside the offset `b`
             * has been stepped to within it.
             */
            union sound_module_args args;

            args.play.volume = rec->volume;
            args.play.loop = rec->loop;
            args.play.rate = *(const uint16_t *)b;
            args.play.sample = far_from(FP_SEG(at), b + 8);
            args.play.length = *(const uint16_t *)(b + 2);

            sound_callback(3, &args);
            continue;
        }

        {
            union sound_module_args args;

            args.poll.volume = rec->volume;
            args.poll.loop = rec->loop;
            answer = sound_callback(4, &args);
        }

        if ((uint8_t)(answer >> 8) != 0)
            rec->ticks = 0;

        if ((uint8_t)answer != 0) {
            rec->poll = 0;
            remove_sequence(rec);
            SNDS.voices_changed = 1;
        }
    }
}

/*
 * 0x27c4e
 *
 * Step one sequence forward by one tick: for each of its channels, run down
 * the delay and, when it reaches zero, read and dispatch as many events as the
 * stream says happen at this instant.
 *
 * `di` is the sequence's slot, and `di * 4` is parked in `cs:0x201` as the high
 * nibble every request byte carries. `es:bx` is the record; the event data is
 * reached through two far pointers from +8, and the base offset is kept in
 * `cs:0x1f7` because BP is used as the cursor.
 *
 * Each channel's entry in the map at +0x8c ends the walk at 0xff and is skipped
 * at 0xfe. Before anything is read, the channel's voice is worked out into
 * `cs:0x1fd`: a channel with bit 1 at +0x134 is its own voice and `cs:0x1fe` is
 * set to say so, otherwise the sixteen voices are searched for one whose owner
 * matches. Not finding one leaves 0xff, and the handlers take that as "do not
 * tell the driver".
 *
 * The delay at +0x4c counts down each tick. **0x8000 is not zero**: reaching it
 * means the delay was a long one and the next byte of the stream extends it, so
 * a delay can be longer than a byte can hold. A delay of exactly 0xf8 is stored
 * as 0xf0 with the same top bit, which is how the two are told apart.
 *
 * With the delay expired, a byte is read. 0x80 and above is a status byte and
 * is remembered at +0x9c; below that it is **running status** - the byte is
 * pushed back, the counter undone, and the remembered status used instead,
 * which is how MIDI avoids repeating a status that has not changed.
 *
 * 0xfc ends the channel outright. A low nibble of 0xf is a meta event. Anything
 * else dispatches on the high nibble to the eight handlers - note off, note on,
 * aftertouch, controller, program, pressure, bend, system - and an unknown high
 * nibble also ends the channel.
 *
 * After each event another byte is read as the delay to the next. **Zero means
 * no delay**, so the loop goes straight back and reads another event at the
 * same instant; that is how chords are written. Anything else is stored one
 * less than it was read, because the tick that stores it has already happened.
 *
 * When every channel before the first 0xff has run out, the sequence has
 * finished. With both +0x15a and +0x15d zero it is removed; otherwise it loops
 * - every channel's position, delay and running status restored from the
 * shadows a checkpoint saved, and +0x154 from +0x156.
 */
void step_sequence(struct sequence far * seq, uint16_t di)
{
    const uint8_t far *base, *data;
    uint16_t si;
    int16_t t;

    SNDS.slot_high = (uint8_t)(di * 4);
    seq->ticks++;

    {
        /*
         * Two loads, not three. The first reads the far pointer stored at the
         * record's +8; the second reads the far pointer *that* points at, and
         * the result is the cursor's base. Measured on the first call:
         * +8 holds 7594:016a, and 7594:016a holds 77ab:0002, so the base is 2
         * in segment 77ab. Following it once more lands in the event data and
         * reads a note as if it were a pointer.
         */
        base = *seq->cursor_at;
        SNDS.cursor_park = (int16_t)FP_OFF(base);
    }
    data = base;

    for (si = 0; si < 0x10; si++) {
        uint8_t al = seq->track_channel[si];
        uint16_t *pos = &seq->position[si];
        uint16_t *delay = &seq->delay[si];
        uint8_t status;

        if (al == 0xff)
            goto finished;
        if (al == 0xfe)
            continue;

        SNDS.own_voice = 0xff;
        SNDS.bend_gate = 0;

        if ((seq->ch.channel_flags[al] & 2) != 0) {
            SNDS.own_voice = al;
            SNDS.bend_gate = 1;
        } else {
            uint8_t want = (uint8_t)((al & 0xf) | SNDS.slot_high);
            uint16_t j;

            for (j = 0; j < 0x10; j++) {
                if (SNDS.voice_held[j] == want) {
                    SNDS.own_voice = (uint8_t)j;
                    break;
                }
            }
        }

        data = base + *(const uint16_t *)(base + 2 * si) + *pos;
        if (*pos == 0)
            continue;

        if (*delay != 0) {
            (*delay)--;
            if (*delay == 0x8000) {
                uint8_t d = *data;
                uint8_t hi = 0;

                data++;
                (*pos)++;
                if (d == 0xf8) {
                    d = 0xf0;
                    hi = 0x80;
                }
                *delay = (uint16_t)(((uint16_t)hi << 8) | d);
            }
            continue;
        }

        for (;;) {
            uint8_t b = *data;
            uint8_t hi_nibble, lo_nibble;

            data++;
            (*pos)++;

            if (b >= 0x80) {
                seq->status[si] = b;
            } else {
                b = seq->status[si];
                data--;
                (*pos)--;
            }

            status = b;
            hi_nibble = (uint8_t)(b & 0xf0);
            lo_nibble = (uint8_t)(b & 0xf);

            if (status == 0xfc) {
                *pos = 0;
                break;
            }

            if (lo_nibble == 0xf) {
                data = midi_meta_event(data, seq, si,
                                       (uint16_t)((hi_nibble << 8) | 0xf));
                if (*pos == 0)
                    break;
            } else {
                uint16_t ax = (uint16_t)(((uint16_t)hi_nibble << 8)
                                         | SNDS.own_voice);

                switch (hi_nibble) {
                case 0x80: data = midi_note_off_event(data, seq, si, ax); break;
                case 0x90: data = midi_note_event(data, seq, si, ax); break;
                case 0xa0: data = midi_event_6(data, seq, si, ax); break;
                case 0xb0: data = midi_controller_event(data, seq, si, ax); break;
                case 0xc0: data = midi_program_event(data, seq, si, ax); break;
                case 0xd0: data = midi_event_9(data, seq, si, ax); break;
                case 0xe0: data = midi_bend_event(data, seq, si, ax); break;
                case 0xf0: data = midi_skip_event(data, seq, si, ax); break;
                default:
                    *pos = 0;
                    goto next_channel;
                }
            }

            {
                uint8_t d = *data;

                data++;
                (*pos)++;
                if (d == 0)
                    continue;
                if (d == 0xf8)
                    *delay = 0x80ef;
                else
                    *delay = (uint16_t)(d - 1);
                break;
            }
        }
next_channel:
        ;
    }

finished:
    for (si = 0; si < 0x10; si++) {
        if (seq->track_channel[si] == 0xff)
            break;
        if (seq->position[si] != 0)
            return;
    }

    if (seq->rewind_mark == 0 && seq->loop == 0) {
        remove_sequence(seq);
        SNDS.voices_changed = 1;
        return;
    }

    seq->ticks = seq->ticks_saved;
    for (t = 0; t < 0x10; t++) {
        seq->position[t] = seq->position_saved[t];
        seq->delay[t] = seq->delay_saved[t];
        seq->status[t] = seq->status_saved[t];
    }
}

/*
 * 0x27e92
 *
 * Handle an explicit note-off event, and answer the stream cursor advanced past
 * it. The same register convention and byte accounting as `midi_note_event` at
 * 0x27ee1: two bytes consumed, the per-byte counter at `+0xc + 2 * si` bumped
 * twice with the raw channel, the mapped channel from `+0x8c` used after.
 *
 * MIDI has two ways to end a note - this message, and a note-on with zero
 * velocity - and the game's sequences use both, which is why there are two
 * routines. This one reads the second byte and never looks at it: the note is
 * the first byte and the release velocity is discarded.
 *
 * As with the note-on, `+0x125` is cleared only when the note recorded there is
 * the one being released, so a channel already given a different note is left
 * alone; and only the driver call is skipped for an unplayed channel or a set
 * `cs:0x209`.
 */
const uint8_t far *midi_note_off_event(const uint8_t far * data,
                                       struct sequence far * seq, uint16_t si,
                                       uint16_t ax)
{
    uint8_t note, velocity, channel;
    uint16_t *counter = &seq->position[si];

    note = *data;
    data++;
    (*counter)++;

    /*
     * The second byte is the velocity, and it is **not** discarded: CL still
     * holds it at the call and a driver may read it. The port used to drop it
     * on the strength of the speaker driver ignoring CL, which is the shape of
     * mistake that survives every screen comparison.
     */
    velocity = *data;
    data++;
    (*counter)++;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    if (seq->ch.note[channel] == note)
        seq->ch.note[channel] = 0xff;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_stop_note((uint16_t)(ax & 0xf),
                         (uint16_t)((note << 8) | velocity));

    return data;
}

/*
 * 0x27f54
 *
 * Handle a two-byte event whose driver function is number 6 - which is one of
 * the seven entries pointing at the do-nothing stub, so on this driver the
 * event costs two bytes of stream and changes nothing.
 *
 * Both bytes are read and counted the usual way, and neither is stored
 * anywhere: this routine's whole effect on the sequence is the cursor and the
 * two counter increments. Whatever it means, a PC speaker has no way to do it.
 */
const uint8_t far *midi_event_6(const uint8_t far * data,
                                struct sequence far * seq, uint16_t si,
                                uint16_t ax)
{
    uint16_t *counter = &seq->position[si];

    data++;
    (*counter)++;

    data++;
    (*counter)++;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_nop();

    return data;
}

/*
 * 0x27ee1
 *
 * Handle one MIDI note event out of a sequence, and answer the stream cursor
 * advanced past it.
 *
 * Hand-written assembly taking everything in registers: `ds:bp` is the cursor
 * into the note stream, `es:bx` the sequence's record, `si` the raw channel and
 * `al` the channel the driver should be told about - or 0xff for a channel that
 * is not being played.
 *
 * Two bytes are consumed, the note then the velocity, and the word counter at
 * `+0xc + 2 * si` is bumped once for **each byte**, not once for the event.
 * Those two increments use the raw channel; everything after uses the mapped
 * one, read from the byte table at `+0x8c` and masked to four bits. The two
 * indices are easy to conflate and are not the same.
 *
 * A non-zero velocity is a note on: the note is recorded at `+0x125 + channel`
 * and the driver told to start it. A zero velocity is a note off - the MIDI
 * convention, rather than a separate message - and it clears `+0x125` **only if
 * the note there is the one being released**, so a channel that has already
 * been given a different note is left alone.
 *
 * The record is updated either way. Only the call to the driver is skipped when
 * the channel is 0xff or the flag at `cs:0x209` is set, so muting stops the
 * sound without letting the sequence's own state drift.
 *
 * The driver is reached through `cs:[0x1e7]` with the function number in BP: 5
 * to start, 4 to stop, which are `sx_start_note` and `sx_stop_note`.
 */
const uint8_t far *midi_note_event(const uint8_t far * data,
                                   struct sequence far * seq, uint16_t si,
                                   uint16_t ax)
{
    uint8_t note, velocity, channel;
    uint16_t *counter = &seq->position[si];

    note = *data;
    data++;
    (*counter)++;

    velocity = *data;
    data++;
    (*counter)++;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    if (velocity != 0) {
        seq->ch.note[channel] = note;

        if ((uint8_t)ax != 0xff && SNDS.muted == 0)
            driver_start_note((uint16_t)(ax & 0xf),
                              (uint16_t)((note << 8) | velocity));
    } else {
        if (seq->ch.note[channel] == note)
            seq->ch.note[channel] = 0xff;

        if ((uint8_t)ax != 0xff && SNDS.muted == 0)
            driver_stop_note((uint16_t)(ax & 0xf),
                             (uint16_t)((note << 8) | velocity));
    }

    return data;
}

/*
 * 0x27f85
 *
 * Handle a controller change - the busiest of the event handlers, and the one
 * that keeps most of a channel's state.
 *
 * Two bytes are read and counted as usual, the controller then its value, and
 * the same `cs:0x1fe` gate `midi_bend_event` has applies after they are
 * consumed. Six controllers are recognised and everything else falls through to
 * the driver unchanged:
 *
 *   0x07  volume. Stored at +0x107, and then **the value handed to the driver
 *         is replaced**: `scale_byte_pair` scales it by the sequence's own
 *         volume at +0x15e, so a channel's volume is always relative. The
 *         pending entry at `cs:0x1c8` is cleared first, so a deferred volume
 *         already queued for this channel does not later overwrite this one.
 *   0x0a  pan, stored at +0xf8.
 *   0x01  modulation, stored at +0xe9.
 *   0x40  sustain. This is the flag that lives in **bit 15 of the pitch bend
 *         word** at +0xbc - set for any non-zero value, cleared for zero -
 *         which is why `midi_bend_event` carries that bit across every write.
 *   0x4b  replaces the low nibble of +0xda, and sets `cs:0x204`.
 *   0x4e  sets the low nibble of +0x143 to 1 or 0, and sets `cs:0x204`.
 *
 * Only 0x07 changes what the driver is told; the rest pass their own value
 * through. The two that set `cs:0x204` are the two that change how voices are
 * allocated, so the tick is told the table needs redoing.
 */
const uint8_t far *midi_controller_event(const uint8_t far * data,
                                         struct sequence far * seq, uint16_t si,
                                         uint16_t ax)
{
    uint16_t *counter = &seq->position[si];
    uint8_t ctrl, value, channel;

    ctrl = *data;
    data++;
    (*counter)++;

    value = *data;
    data++;
    (*counter)++;

    if (SNDS.bend_gate != 0 && SNDS.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    if (ctrl == 7) {
        seq->ch.volume[channel] = value;
        value = scale_byte_pair(value, seq->volume);
        if ((uint8_t)ax >= 0x20)
            return data;
        SNDS.pending_volume[(uint8_t)ax] = 0xff;
    } else if (ctrl == 0xa) {
        seq->ch.pan[channel] = value;
    } else if (ctrl == 1) {
        seq->ch.modulation[channel] = value;
    } else if (ctrl == 0x40) {
        uint16_t *bend = &seq->ch.bend[channel];

        if (value != 0)
            *bend |= 0x8000;
        else
            *bend &= 0x7fff;
    } else if (ctrl == 0x4b) {
        uint8_t *p = &seq->ch.voice_budget[channel];

        *p = (uint8_t)((*p & 0xf0) | value);
        SNDS.voices_changed = 1;
    } else if (ctrl == 0x4e) {
        uint8_t *p = &seq->ch.no_voice[channel];

        *p = (uint8_t)((*p & 0xf0) | (value != 0 ? 1 : 0));
        SNDS.voices_changed = 1;
    }

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_controller((uint16_t)(ax & 0xf),
                      (uint16_t)(((uint16_t)ctrl << 8) | value));

    return data;
}

/*
 * 0x28086
 *
 * Handle a program change: one byte, stored as the channel's instrument at
 * +0x116, then driver function 8 - another of the stub entries, so the speaker
 * driver is told and does nothing with it.
 *
 * The value is stored even when the driver call is skipped, so a muted or
 * unassigned channel still remembers its instrument for whenever it is heard.
 *
 * It carries the same extra gate `midi_bend_event` has: with `cs:0x1fe`
 * non-zero, a channel whose byte at `cs:0x128` is not 0xff is dropped - but
 * only after the byte has been consumed and counted, so the stream stays in
 * step.
 */
const uint8_t far *midi_program_event(const uint8_t far * data,
                                      struct sequence far * seq, uint16_t si,
                                      uint16_t ax)
{
    uint16_t *counter = &seq->position[si];
    uint8_t program, channel;

    program = *data;
    data++;
    (*counter)++;

    if (SNDS.bend_gate != 0 && SNDS.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);
    seq->ch.program[channel] = program;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_program_change((uint16_t)(ax & 0xf), program);

    return data;
}

/*
 * 0x280da
 *
 * Handle a one-byte event whose driver function is number 9 - a stub entry
 * again. The byte is read and counted and nothing keeps it.
 *
 * Unlike its neighbours this one does not save SI: it never changes it, so
 * there is nothing to put back.
 */
const uint8_t far *midi_event_9(const uint8_t far * data,
                                struct sequence far * seq, uint16_t si,
                                uint16_t ax)
{
    uint16_t *counter = &seq->position[si];

    data++;
    (*counter)++;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_nop();

    return data;
}

/*
 * 0x280fe
 *
 * Handle one pitch bend event out of a sequence, and answer the stream cursor
 * advanced past it. The register convention is `midi_note_event`'s at 0x27ee1,
 * and so is the byte accounting: two bytes consumed, the per-byte counter at
 * `+0xc + 2 * si` bumped twice with the **raw** channel, everything after using
 * the mapped one from `+0x8c`.
 *
 * Two bytes are read, and the pair is put together the way MIDI does it -
 * `(msb << 7) | lsb` - by rotating the low bit of the most significant byte
 * into the top of the least. The result goes to the sequence's own store at
 * `+0xbc + 2 * channel`.
 *
 * **Bit 15 of that word is sticky.** Before the new value is written, the old
 * one is tested and its top bit carried into the new; a 14-bit bend can never
 * set it, so nothing this routine writes will ever clear it once something else
 * has. It is a flag living in the spare bit of a value, not part of the bend.
 *
 * There is an extra gate this event has and the note event does not: with
 * `cs:0x1fe` non-zero, a channel whose byte at `cs:0x128` is not 0xff is
 * dropped entirely - after the two bytes have been consumed and counted, so the
 * stream stays in step either way.
 *
 * As before, only the call to the driver is skipped for an unplayed channel or
 * a set `cs:0x209`; the stored bend is updated regardless. The driver is
 * reached with function number 10, `sx_pitch_bend`, and gets the *original*
 * register pair rather than the assembled value - it does its own assembly.
 */
const uint8_t far *midi_bend_event(const uint8_t far * data,
                                   struct sequence far * seq, uint16_t si,
                                   uint16_t ax)
{
    uint8_t lsb, msb, channel;
    uint16_t *counter = &seq->position[si];
    uint16_t value;
    uint16_t *slot;

    lsb = *data;
    data++;
    (*counter)++;

    msb = *data;
    data++;
    (*counter)++;

    if (SNDS.bend_gate != 0 && SNDS.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    value = (uint16_t)((((uint16_t)msb >> 1) << 8)
                       | (uint16_t)(lsb | ((msb & 1) ? 0x80 : 0)));

    slot = &seq->ch.bend[channel];
    if (*slot >= 0x8000)
        value |= 0x8000;
    *slot = value;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_pitch_bend((uint16_t)(ax & 0xf),
                      (uint16_t)(((uint16_t)lsb << 8) | msb));

    return data;
}

/*
 * 0x2817e
 *
 * Handle the two status bytes that carry the sequencer's own meta events, and
 * hand anything else to `skip_unknown_event`.
 *
 * **0xc0** is either a plain value or a checkpoint. Any first byte but 0x7f is
 * stored at +0x158 - unless `cs:0x209` is set, in which case it is read and
 * dropped, so a muted sequence still consumes the same bytes.
 *
 * A first byte of 0x7f is a checkpoint: the second byte is read, 0xf8 being
 * rewritten as 0xf0 with 0x80 in the high half, and parked at `+0x4c`. Every
 * channel's running position is then copied into its shadow - `+0xc` to `+0x2c`,
 * `+0x4c` to `+0x6c`, `+0x9c` to `+0xac` - and +0x154 to +0x156. The parked
 * value exists only to ride into `+0x6c` on that copy: as soon as it has, the
 * byte read is **undone** - the counter decremented, the cursor stepped back,
 * and `+0x4c` cleared - so the second byte is left in the stream to be read
 * again.
 *
 * **0xb0** carries three of its own controllers, read as a pair:
 *
 *   0x50  the sequence's device value at +0x15f, with 0x7f meaning "use the
 *         default at `cs:0x202`", then passed to the driver as function 11.
 *   0x60  bumps the loop counter at +0x152, and does nothing while `cs:0x209`
 *         is set.
 *   0x52  resets all sixteen channel positions at +0xc to zero, but **only if
 *         the value matches +0x15a** - so a sequence ignores a rewind aimed at
 *         a different one.
 */
const uint8_t far *midi_meta_event(const uint8_t far * data,
                                   struct sequence far * seq, uint16_t si,
                                   uint16_t ax)
{
    uint16_t *counter = &seq->position[si];
    uint8_t status = (uint8_t)(ax >> 8);
    uint8_t first, second;
    int16_t t;

    if (status != 0xc0 && status != 0xb0)
        return skip_unknown_event(data, seq, si, ax);

    if (status == 0xc0) {
        first = *data;
        data++;
        (*counter)++;

        if (first != 0x7f) {
            if (SNDS.muted == 0)
                seq->state = first;
            return data;
        }

        second = *data;
        data++;
        (*counter)++;

        {
            uint8_t hi = 0;

            if (second == 0xf8) {
                hi = 0x80;
                second = 0xf0;
            }
            seq->delay[si] = (uint16_t)(((uint16_t)hi << 8) | second);
        }
        seq->status[si] = 0xcf;

        for (t = 0; t < 0x10; t++) {
            seq->position_saved[t] = seq->position[t];
            seq->delay_saved[t] = seq->delay[t];
            seq->status_saved[t] = seq->status[t];
        }
        seq->ticks_saved = seq->ticks;

        (*counter)--;
        data--;
        seq->delay[si] = 0;
        return data;
    }

    first = *data;
    data++;
    (*counter)++;

    second = *data;
    data++;
    (*counter)++;

    if (first == 0x50) {
        if (second == 0x7f)
            second = SNDS.param_default;
        seq->device_value = second;
        driver_param_349(second);
        return data;
    }

    if (first == 0x60) {
        if (SNDS.muted == 0)
            seq->loop_count++;
        return data;
    }

    if (first == 0x52 && seq->rewind_mark == second) {
        for (t = 0; t < 0x10; t++)
            seq->position[t] = 0;
    }

    return data;
}

/*
 * 0x2817a
 *
 * A one-instruction forwarder to `skip_unknown_event`. It exists so that the
 * dispatch that reaches it has an entry of its own rather than sharing one.
 */
const uint8_t far *midi_skip_event(const uint8_t far * data,
                                   struct sequence far * seq, uint16_t si,
                                   uint16_t ax)
{
    return skip_unknown_event(data, seq, si, ax);
}

/*
 * 0x2828e
 *
 * Step the cursor past an event this module does not handle, using MIDI's own
 * rule for how long a message is - which is why it only needs the status byte
 * in AH and never looks at the data.
 *
 * A status of 0xf0 is system exclusive and has no fixed length: bytes are
 * consumed until 0xf7, and **the terminator is counted too**, so the cursor
 * ends past it rather than on it. A malformed stream with no 0xf7 runs off the
 * end; nothing bounds this loop.
 *
 * 0xc0 and 0xd0 - program change and channel pressure - carry one data byte.
 * Everything else carries two. That is the standard rule and the reason the two
 * cases share their second read: the two-byte path falls through into the
 * one-byte path rather than repeating it.
 *
 * Every byte consumed bumps the per-byte counter at `+0xc + 2 * si`, so an
 * unhandled event still costs the channel exactly what it read.
 */
const uint8_t far *skip_unknown_event(const uint8_t far * data,
                                      struct sequence far * seq, uint16_t si,
                                      uint16_t ax)
{
    uint16_t *counter = &seq->position[si];
    uint8_t status = (uint8_t)(ax >> 8);
    uint8_t b;

    if (status == 0xf0) {
        do {
            b = *data;
            data++;
            (*counter)++;
        } while (b != 0xf7);
        return data;
    }

    if (status != 0xc0 && status != 0xd0) {
        data++;
        (*counter)++;
    }

    data++;
    (*counter)++;

    return data;
}

/*
 * 0x282cb
 *
 * Scale one byte by another and halve the range: `((cl+1) * (dl+1)) >> 8`,
 * doubled, then reduced by one unless it is already zero.
 *
 * A **** routine that takes and answers CL, preserving AX around the
 * multiply with a push and a pop. `mul dl` is the 8-bit form, so the product
 * lands in AX and `shl ah,1` doubles its high byte - the >>8 and the doubling
 * are one step, not two.
 */
uint8_t scale_byte_pair(uint8_t cl, uint8_t dl)
{
    uint16_t product = (uint16_t)((uint8_t)(cl + 1) * (uint8_t)(dl + 1));
    uint8_t out = (uint8_t)(((product >> 8) & 0xFF) << 1);

    if (out != 0)
        out--;
    return out;
}
/*
 * 0x28305
 *
 * Parse a sequence's device-specific parameter table once, and cache the result
 * in place. Hand-written assembly: `es:ax` is the record, and nothing is
 * returned.
 *
 * The table is reached by **two** far pointers - the one at the record's +8,
 * and then the one that points at. A record whose +8 is a null far pointer,
 * both halves 0xffff, is left alone.
 *
 * The cache is guarded by a three-byte signature, 0xfc 0xfd 0xfe at +0x21,
 * +0x22 and +0x23 of the table itself, and it is checked **backwards** - +0x23
 * first. Finding it means the work has already been done and the routine
 * returns. Writing it is the last thing that happens, so a parse interrupted
 * part way is redone rather than half-trusted.
 *
 * Sixteen words of scratch at the module's own `cs:0x108` are cleared, along
 * with a byte at `cs:0x20c` set to 0xff. A leading 0xf0 in the table supplies
 * that byte from the following one and skips eight bytes.
 *
 * What follows is a list of devices. Each is an identifier byte and then
 * six-byte entries ending at 0xff; the identifier is matched against
 * `cs:0x1fc`, and a device that does not match has its entries stepped over
 * six bytes at a time without being read. The matching device's entries each
 * contribute one word - the third and fourth bytes - to the scratch, in order.
 *
 * The sixteen words and the byte are then written **over the start of the
 * table**, at +0 to +0x20, and the signature after them. So the parsed form
 * replaces the source it was parsed from, which is why the signature has to be
 * checked before anything else: a second parse would read its own output.
 *
 * Nothing bounds the number of entries a device may have. Seventeen or more
 * would run the scratch index past its sixteen words and write into whatever
 * follows `cs:0x108`.
 */
void init_sequence_params(struct sequence far * seq)
{
    uint8_t far *tbl;
    uint16_t si;

    if (FP_OFF(seq->cursor_at) == 0xffff && FP_SEG(seq->cursor_at) == 0xffff)
        return;

    tbl = *seq->cursor_at;

    if (tbl[0x23] == 0xfe && tbl[0x22] == 0xfd && tbl[0x21] == 0xfc)
        return;

    for (si = 0x20; si != 0;) {
        si -= 2;
        SNDS.scratch[si / 2] = 0;
    }
    SNDS.scratch_mark = 0xff;

    {
        uint16_t bp = 0;

        if (tbl[bp] == 0xf0) {
            SNDS.scratch_mark = tbl[bp + 1];
            bp += 8;
        }

        si = 0;
        for (;;) {
            uint8_t id = tbl[bp];

            if (id == SNDS.ch) {
                bp++;
                for (;;) {
                    uint8_t c = tbl[bp];

                    bp++;
                    if (c == 0xff)
                        break;
                    bp++;
                    SNDS.scratch[si / 2] = *(int16_t *)(tbl + bp);
                    bp += 4;
                    si += 2;
                }
                break;
            }
            if (id == 0xff)
                break;

            bp++;
            for (;;) {
                uint8_t c = tbl[bp];

                bp++;
                if (c == 0xff)
                    break;
                bp += 5;
            }
        }

        for (si = 0; si != 0x20; si += 2)
            *(int16_t *)(tbl + si) = SNDS.scratch[si / 2];
        tbl[0x20] = SNDS.scratch_mark;
    }

    tbl[0x21] = 0xfc;
    tbl[0x22] = 0xfd;
    tbl[0x23] = 0xfe;
}

/*
 * 0x28559
 *
 * The ordinary-call face of `silence_driver`. It loads `ES:AX` from where a
 * stack argument would be, and `silence_driver` reads neither - the same dead
 * argument as in 0x2846a. Its one C caller, `stop_sound`, pushes nothing, so
 * the port declares none.
 */
void silence_driver_far(void)
{
    silence_driver();
}

/*
 * 0x28431
 *
 * The ordinary-call face of `set_master_level`. The level arrives as a word on
 * the stack and goes into `CX`; only `CL` is read. DS, DI and SI are saved
 * around the call, as in every wrapper in this band.
 */
void set_master_level_far(uint16_t level)
{
    set_master_level((uint8_t)level);
}

/*
 * 0x28458
 *
 * The ordinary-call face of `install_driver`. The driver's far pointer arrives
 * on the stack and is loaded into `ES:AX` with one `les`, and `AX` comes back
 * out untouched, so this returns what `install_driver` did.
 */
uint16_t install_driver_far(const uint8_t far * drv)
{
    return install_driver(drv);
}

/*
 * 0x2846a
 *
 * The ordinary-call face of `configure_driver`. It loads `ES:AX` from the
 * stack argument and zeroes `BX` before the call. `AX` passes back out, and
 * 0x28580 reads it.
 *
 * **That argument is not dead**, and this comment used to say it was. The
 * speaker driver's function 1 answers two constants and reads neither
 * register, so nothing the port could run disagreed; `GMD:` function 1 copies
 * a 0x481-byte patch bank out of exactly this `ES:AX`. `configure_driver`
 * takes it now and hands it to the driver.
 */
uint16_t configure_driver_far(const uint8_t far * drv)
{
    return configure_driver(drv);
}

/*
 * 0x284ef
 *
 * The ordinary-call face of `retire_and_tick`, which reads its record from
 * `ES:AX` - loaded here from the stack argument with one `les`.
 */
void retire_and_tick_far(struct sequence far * seq)
{
    retire_and_tick(seq);
}

/*
 * 0x28480
 *
 * The ordinary-call face of `start_sequence`. The original takes its record in
 * `es:ax` and its flag in `cx`, which no C caller can arrange, so this takes
 * them on the stack and puts them in registers.
 *
 * It also saves DS, DI and SI around the call. `start_sequence` restores what
 * it changes, but the ones this pushes are the ones a C caller expects to keep,
 * and the hand-written routine makes no such promise.
 */
void start_sequence_far(struct sequence far * seq, uint16_t flag)
{
    start_sequence(seq, flag);
}

/*
 * 0x2841f
 *
 * The ordinary-call face of `seek_sequence`, in sound_drv.c. Nothing calls it.
 * NOT TRANSCRIBED YET for the host; a stub, which aborts. The TASM source
 * above is the original's.
 */
void seek_sequence_far(struct sequence far * seq)
{
    (void)seq;
    not_transcribed("0x2841f");
}

/*
 * 0x284b0
 *
 * The ordinary-call face of `driver_fn13`, in sound_drv.c. Nothing calls it.
 * NOT TRANSCRIBED YET for the host; a stub, which aborts. The TASM source
 * above is the original's.
 */
void driver_fn13_far(void)
{
    not_transcribed("0x284b0");
}

/*
 * 0x2852c
 *
 * The ordinary-call face of `set_sequence_level`, in sound_drv.c. Nothing calls it.
 * NOT TRANSCRIBED YET for the host; a stub, which aborts. The TASM source
 * above is the original's.
 */
void set_sequence_level_far(uint16_t level)
{
    (void)level;
    not_transcribed("0x2852c");
}
#endif
