/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The sound module's data and its one far entry**: the module's state,
 * kept in its own code segment and named from the other two files, then
 * `sound_api`, which files its arguments there and dispatches on a function
 * number to the driver interface. Nothing in the game calls the entry.
 *
 * One module of the original's **code segment 2619**, image
 * 0x26198..0x265f2: the dispatcher far-calls every routine it names as
 * TLINK's `nop / push cs / call`, so they were in other files. The host
 * keeps the state as `SNDS` in dgroup.c.
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
 * The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
 * transcription is the `#else`. The judge hands the blocks to TASM 3.0
 * (`JUDGE: tasm`).
 */
asm {
extrn _configure_driver:far
extrn _driver_fn13:far
extrn _install_driver:far
extrn _retire_and_tick:far
extrn _seek_sequence:far
extrn _set_master_level:far
extrn _set_sequence_level:far
extrn _silence_driver:far
extrn _sound_hold:far
extrn _start_sequence:far
SOUND_TEXT segment byte public 'CODE'
assume cs:SOUND_TEXT, ds:DGROUP
public c_26198, c_2619a, c_2619c, c_2619e, c_261d8, c_261da
public c_26218, c_2621a, c_26298, c_262b8, c_262ba, c_262bc
public c_262be, c_262c0, c_262c2, c_262c4, c_262c6, c_262c8
public c_262ca, c_262cc, c_262ce, c_262d0, c_262d2, c_262d4
public c_262d6, c_262d8, c_262da, c_262dc, c_262de, c_262e0
public c_262e2, c_262e4, c_262e6, c_262e8, c_262ea, c_262ec
public c_262ee, c_262f0, c_262f2, c_262f4, c_262f6, c_262f8
public c_262fa, c_262fc, c_262fe, c_26300, c_26302, c_26304
public c_26306, c_26308, c_2630a, c_2630c, c_2630e, c_26310
public c_26312, c_26314, c_26316, c_26318, c_2631a, c_2631c
public c_2631e, c_26320, c_26322, c_26324, c_26326, c_26328
public c_2632a, c_2632c, c_2632e, c_26330, c_26332, c_26334
public c_26336, c_26338, c_2633a, c_2633c, c_2633e, c_26340
public c_26342, c_26344, c_26346, c_26348, c_2634a, c_2634c
public c_2634e, c_26350, c_26352, c_26354, c_26356, c_26358
public c_26368, c_26377, c_26379, c_2637b, c_2637d, c_2637f
public c_26381, c_26383, c_26385, c_26387, c_26389, c_2638a
public c_2638b, c_2638c, c_2638d, c_2638e, c_2638f, c_26390
public c_26391, c_26392, c_26393, c_26394, c_26395, c_26396
public c_26399, c_2639c
public _sound_api, _sound_api_dispatch
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
	nop
	push cs
	db 0e8h, 96h, 5h  /* call 0x26a61 outside */
	ret
L264cc:
	cmp cl, 0bh
	jne L264e6
	mov dl, byte ptr cs:c_2637b
	mov cl, byte ptr cs:c_2637d
	mov ch, byte ptr cs:c_2637f
	nop
	push cs
	db 0e8h, 0e4h, 5h  /* call 0x26ac9 outside */
	ret
L264e6:
	cmp cl, 0ch
	jne L264f6
	mov cl, byte ptr cs:c_2637b
	nop
	push cs
	db 0e8h, 0f8h, 5h  /* call 0x26aed outside */
	ret
L264f6:
	cmp cl, 0dh
	jne L26506
	mov cx, word ptr cs:c_2637b
	nop
	push cs
	db 0e8h, 0f2h, 5h  /* call 0x26af7 outside */
	ret
L26506:
	cmp cl, 0eh
	jne L26516
	mov cx, word ptr cs:c_2637b
	nop
	push cs
	db 0e8h, 1fh, 6h  /* call 0x26b34 outside */
	ret
L26516:
	cmp cl, 0fh
	jne L26525
	mov cx, word ptr cs:c_2637b
	nop
	push cs
	db 0e8h, 1fh, 6h  /* call 0x26b44 outside */
L26525:
	cmp cl, 11h
	jne L26531
	nop
	push cs
	db 0e8h, 0e9h, 6h  /* call 0x26c18 outside */
	mov ax, cx
L26531:
	cmp cl, 10h
	jne L26540
	nop
	push cs
	db 0e8h, 0c7h, 6h  /* call 0x26c02 outside */
	mov al, cl
	xor ah, ah
	ret
L26540:
	cmp cl, 19h
	jne L2654f
	nop
	push cs
	db 0e8h, 0c2h, 6h  /* call 0x26c0c outside */
	mov al, cl
	xor ah, ah
	ret
L2654f:
	cmp cl, 12h
	jne L26579
	nop
	push cs
	db 0e8h, 0c9h, 6h  /* call 0x26c22 outside */
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
	nop
	push cs
	db 0e8h, 0b1h, 6h  /* call 0x26c43 outside */
	ret
L26593:
	cmp cl, 14h
	jne L265ad
	mov dl, byte ptr cs:c_2637b
	mov ch, byte ptr cs:c_2637d
	mov cl, byte ptr cs:c_2637f
	nop
	push cs
	db 0e8h, 0e9h, 6h  /* call 0x26c95 outside */
	ret
L265ad:
	cmp cl, 15h
	jne L265c7
	mov dl, byte ptr cs:c_2637b
	mov ch, byte ptr cs:c_2637d
	mov cl, byte ptr cs:c_2637f
	nop
	push cs
	db 0e8h, 20h, 7h  /* call 0x26ce6 outside */
	ret
L265c7:
	cmp cl, 16h
	jne L265dc
	mov dl, byte ptr cs:c_2637b
	mov cl, byte ptr cs:c_2637d
	nop
	push cs
	db 0e8h, 0e9h, 7h  /* call 0x26dc4 outside */
	ret
L265dc:
	cmp cl, 17h
	jne L265f1
	mov dl, byte ptr cs:c_2637b
	mov cx, word ptr cs:c_2637d
	nop
	push cs
	db 0e8h, 25h, 8h  /* call 0x26e15 outside */
	ret
L265f1:
	ret
_sound_api_dispatch endp
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
#endif
