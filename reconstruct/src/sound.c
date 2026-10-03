/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The sound module, which was written in assembly**: its far entry
 * points, its state, a far entry and its dispatcher, the interface to the
 * loaded driver, the sequencer. In 1.11 it is **code segment 2893** on its
 * own, image 0x28936..0x2ad2c, word-aligned (the zero at 0x28935 is the
 * alignment), and ending in a zero byte after its last `ret`, which the
 * link needs and nothing reads: twenty far entry points first, 0x28936..0x28aea, the only
 * routines that build a C frame - nine of them 1.00's and eleven more, one
 * for each of the API's functions 0x0a..0x17 the dispatcher reaches, which
 * nothing in the game calls - then its state, `g_snds`, 0x28aea..0x28cef,
 * then the code. It reaches the loaded driver through a far pointer in that
 * segment, with the function number in BP. Its routines take their
 * arguments in registers and save what they use.
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
 * tasm`). The sound library's C, a module per segment in 1.11, is in
 * sound_load.c, sound_device.c, sound_bank.c, sound_stop.c and
 * sound_file.c.
 *
 * Functions are in address order and each carries the image offset it was
 * read from.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler bc3.00
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it, drafted by tools/asm2tasm.py (`--nosmart`);
 * the host's transcription is the `#else`. The judge hands the block to
 * TASM 3.0 (`JUDGE: tasm`).
 */
asm {
extrn _sound_callback:far
extrn _g_sound_bank:byte
nosmart
SOUND_TEXT segment word public 'CODE'
assume cs:SOUND_TEXT, ds:DGROUP
public _sound_fn11_far, _sound_fn0f_far, _sound_fn12_far, _seek_sequence_far
public _set_master_level_far, _sound_fn0a_far, _install_driver_far, _configure_driver_far
public _start_sequence_far, _sound_fn0b_far, _driver_fn13_far, _sound_fn0e_far
public _sound_fn17_far, _retire_and_tick_far, _sound_fn0c_far, _sound_fn10_far
public _set_sequence_level_far, _sound_fn15_far, _silence_driver_far, _sound_fn0d_far
public _sound_api, _sound_api_dispatch, _install_driver, _configure_driver
public _silence_driver, _sound_hold, _driver_fn13, _seek_sequence
public _set_master_level, _set_sequence_level, _start_sequence, _retire_and_tick
public _remove_sequence, _sequencer_tick, _advance_volume_ramp, _set_sequence_volume
public _flush_pending_volumes, _sound_service, _drop_unless_polled, _poll_sequences
public _step_sequence, _midi_note_off_event, _midi_note_event, _midi_event_6
public _midi_controller_event, _midi_program_event, _midi_event_9, _midi_bend_event
public _midi_skip_event, _midi_meta_event, _skip_unknown_event, _scale_byte_pair
public _init_sequence_params

/* 0x28936 */
_sound_fn11_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	call FAR PTR L2956a
	mov ax, cx
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn11_far endp

/* 0x2894b */
_sound_fn0f_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov cl, [bp+0ah]
	call FAR PTR L29496
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn0f_far endp

/* 0x28961 */
_sound_fn12_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	call FAR PTR L29574
	xor dh, dh
	mov ax, cx
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn12_far endp

/* 0x28978 */
_seek_sequence_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	call FAR PTR _seek_sequence
	pop si
	pop di
	pop ds
	pop bp
	retf
_seek_sequence_far endp

/* 0x2898b */
_set_master_level_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	mov cx, [bp+6]
	call FAR PTR _set_master_level
	pop si
	pop di
	pop ds
	pop bp
	retf
_set_master_level_far endp

/* 0x2899e */
_sound_fn0a_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov cl, [bp+0ah]
	call FAR PTR L293b3
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn0a_far endp

/* 0x289b4 */
_install_driver_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	call FAR PTR _install_driver
	pop si
	pop di
	pop ds
	pop bp
	retf
_install_driver_far endp

/* 0x289c7 */
_configure_driver_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	push bx
	xor bx, bx
	les ax, dword ptr [bp+6]
	call FAR PTR _configure_driver
	pop bx
	pop si
	pop di
	pop ds
	pop bp
	retf
_configure_driver_far endp

/* 0x289de */
_start_sequence_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov cx, [bp+0ah]
	call FAR PTR _start_sequence
	pop si
	pop di
	pop ds
	pop bp
	retf
_start_sequence_far endp

/* 0x289f4 */
_sound_fn0b_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov dl, [bp+0ah]
	mov cl, [bp+0ch]
	mov ch, [bp+0eh]
	call FAR PTR L2941b
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn0b_far endp

/* 0x28a10 */
_driver_fn13_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	mov cx, [bp+6]
	call FAR PTR _driver_fn13
	pop si
	pop di
	pop ds
	pop bp
	retf
_driver_fn13_far endp

/* 0x28a23 */
_sound_fn0e_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov cl, [bp+0ah]
	call FAR PTR L29486
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn0e_far endp

/* 0x28a39 */
_sound_fn17_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov cx, [bp+0ah]
	mov dl, [bp+0ch]
	call FAR PTR L29767
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn17_far endp

/* 0x28a52 */
_retire_and_tick_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	call FAR PTR _retire_and_tick
	pop si
	pop di
	pop ds
	pop bp
	retf
_retire_and_tick_far endp

/* 0x28a65 */
_sound_fn0c_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov cl, [bp+0ah]
	call FAR PTR L2943f
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn0c_far endp

/* 0x28a7b */
_sound_fn10_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	call FAR PTR L29554
	pop si
	pop di
	pop ds
	pop bp
	xor ah, ah
	mov al, cl
	retf
_sound_fn10_far endp

/* 0x28a92 */
_set_sequence_level_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	mov cl, [bp+6]
	call FAR PTR _set_sequence_level
	pop si
	pop di
	pop ds
	pop bp
	retf
_set_sequence_level_far endp

/* 0x28aa5 */
_sound_fn15_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov ch, [bp+0ah]
	mov cl, [bp+0ch]
	mov dl, [bp+0eh]
	call FAR PTR L29638
	pop si
	pop di
	pop ds
	pop bp
	retf
_sound_fn15_far endp

/* 0x28ac1 */
_silence_driver_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	call FAR PTR _silence_driver
	pop si
	pop di
	pop ds
	pop bp
	retf
_silence_driver_far endp

/* 0x28ad4 */
_sound_fn0d_far proc far
	push bp
	mov bp, sp
	push ds
	push di
	push si
	les ax, dword ptr [bp+6]
	mov cx, [bp+0ah]
	call FAR PTR L29449
	pop si
	pop di
	pop ds
	pop bp
	retf
playing_0_off label byte
	db 0h, 0h
playing_0_seg label byte
	db 0h, 0h
playing_1_off label byte
	db 0h, 0h
playing_1_seg label byte
	db 58 dup (0h)
polled_0_off label byte
	db 0h, 0h
polled_0_seg label byte
	db 62 dup (0h)
voice_sequence_0_off label byte
	db 0h, 0h
voice_sequence_0_seg label byte
	db 126 dup (0h)
scratch label byte
	db 32 dup (0h)
voice_held label byte
	db 0ffh, 0ffh
voice_held_2 label byte
	db 0ffh, 0ffh
voice_held_4 label byte
	db 0ffh, 0ffh
voice_held_6 label byte
	db 0ffh, 0ffh
voice_held_8 label byte
	db 0ffh, 0ffh
voice_held_a label byte
	db 0ffh, 0ffh
voice_held_c label byte
	db 0ffh, 0ffh
voice_held_e label byte
	db 0ffh, 0ffh
voice_keep_own label byte
	db 0h, 0h
voice_keep_own_2 label byte
	db 0h, 0h
voice_keep_own_4 label byte
	db 0h, 0h
voice_keep_own_6 label byte
	db 0h, 0h
voice_keep_own_8 label byte
	db 0h, 0h
voice_keep_own_a label byte
	db 0h, 0h
voice_keep_own_c label byte
	db 0h, 0h
voice_keep_own_e label byte
	db 0h, 0h
voice_cost label byte
	db 0h, 0h
voice_cost_2 label byte
	db 0h, 0h
voice_cost_4 label byte
	db 0h, 0h
voice_cost_6 label byte
	db 0h, 0h
voice_cost_8 label byte
	db 0h, 0h
voice_cost_a label byte
	db 0h, 0h
voice_cost_c label byte
	db 0h, 0h
voice_cost_e label byte
	db 0h, 0h
voice_gives_back label byte
	db 0h, 0h
voice_gives_back_2 label byte
	db 0h, 0h
voice_gives_back_4 label byte
	db 0h, 0h
voice_gives_back_6 label byte
	db 0h, 0h
voice_gives_back_8 label byte
	db 0h, 0h
voice_gives_back_a label byte
	db 0h, 0h
voice_gives_back_c label byte
	db 0h, 0h
voice_gives_back_e label byte
	db 0h, 0h
voice_request label byte
	db 0ffh, 0ffh
voice_request_2 label byte
	db 0ffh, 0ffh
voice_request_4 label byte
	db 0ffh, 0ffh
voice_request_6 label byte
	db 0ffh, 0ffh
voice_request_8 label byte
	db 0ffh, 0ffh
voice_request_a label byte
	db 0ffh, 0ffh
voice_request_c label byte
	db 0ffh, 0ffh
voice_request_e label byte
	db 0ffh, 0ffh
saved_keep_own label byte
	db 0h, 0h
saved_keep_own_2 label byte
	db 0h, 0h
saved_keep_own_4 label byte
	db 0h, 0h
saved_keep_own_6 label byte
	db 0h, 0h
saved_keep_own_8 label byte
	db 0h, 0h
saved_keep_own_a label byte
	db 0h, 0h
saved_keep_own_c label byte
	db 0h, 0h
saved_keep_own_e label byte
	db 0h, 0h
saved_cost label byte
	db 0h, 0h
saved_cost_2 label byte
	db 0h, 0h
saved_cost_4 label byte
	db 0h, 0h
saved_cost_6 label byte
	db 0h, 0h
saved_cost_8 label byte
	db 0h, 0h
saved_cost_a label byte
	db 0h, 0h
saved_cost_c label byte
	db 0h, 0h
saved_cost_e label byte
	db 0h, 0h
saved_gives_back label byte
	db 0h, 0h
saved_gives_back_2 label byte
	db 0h, 0h
saved_gives_back_4 label byte
	db 0h, 0h
saved_gives_back_6 label byte
	db 0h, 0h
saved_gives_back_8 label byte
	db 0h, 0h
saved_gives_back_a label byte
	db 0h, 0h
saved_gives_back_c label byte
	db 0h, 0h
saved_gives_back_e label byte
	db 0h, 0h
saved_request label byte
	db 0ffh, 0ffh
saved_request_2 label byte
	db 0ffh, 0ffh
saved_request_4 label byte
	db 0ffh, 0ffh
saved_request_6 label byte
	db 0ffh, 0ffh
saved_request_8 label byte
	db 0ffh, 0ffh
saved_request_a label byte
	db 0ffh, 0ffh
saved_request_c label byte
	db 0ffh, 0ffh
saved_request_e label byte
	db 0ffh, 0ffh
voice_channel label byte
	db 0fh, 0fh
voice_channel_2 label byte
	db 0fh, 0fh
voice_channel_4 label byte
	db 0fh, 0fh
voice_channel_6 label byte
	db 0fh, 0fh
voice_channel_8 label byte
	db 0fh, 0fh
voice_channel_a label byte
	db 0fh, 0fh
voice_channel_c label byte
	db 0fh, 0fh
voice_channel_e label byte
	db 0fh, 0fh
pending_volume label byte
	db 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh
snd_unknown_1d8 label byte
	db 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h, 0h
driver_off label byte
	db 0h, 0h
driver_seg label byte
	db 0h, 0h
api_arg0 label byte
	db 0h, 0h
api_arg1 label byte
	db 0h, 0h
api_arg2 label byte
	db 0h, 0h
api_arg3 label byte
	db 0h, 0h
api_arg4 label byte
	db 0h, 0h
api_arg5 label byte
	db 0h, 0h
cursor_park label byte
	db 0h, 0h
busy label byte
	db 0h
voice_lo label byte
	db 0h
voice_hi label byte
	db 0fh
want_ch label byte
	db 0h
own_voice label byte
	db 0ffh
bend_gate label byte
	db 0h
want_cl label byte
	db 0h
ah_high label byte
	db 0h
slot_high label byte
	db 0h
param_default label byte
	db 0h
saved_total label byte
	db 0h
voices_changed label byte
	db 0h
defer label byte
	db 0h
scan_stopped label byte
	db 0h, 0h, 0h
muted label byte
	db 0h, 0h, 0h
scratch_mark label byte
	db 0h
_sound_fn0d_far endp

/* 0x28cef */
_sound_api proc far
	push bp
	mov bp, sp
	push es
	push ds
	push di
	push si
	les ax, dword ptr [bp+8]
	mov cx, [bp+0ch]
	mov word ptr cs:api_arg0, cx
	mov cx, [bp+0eh]
	mov word ptr cs:api_arg1, cx
	mov cx, [bp+10h]
	mov word ptr cs:api_arg2, cx
	mov cx, [bp+12h]
	mov word ptr cs:api_arg3, cx
	mov cx, [bp+14h]
	mov word ptr cs:api_arg4, cx
	mov cx, [bp+16h]
	mov word ptr cs:api_arg5, cx
	mov cx, [bp+6]
	cmp cl, 6
	je L28d40
	cmp cl, 3
	je L28d40
	cmp cl, 4
	je L28d40
	cmp cl, 7
	jne L28d48
L28d40:
	mov dx, [bp+8]
	mov word ptr cs:api_arg0, dx
L28d48:
	call _sound_api_dispatch
	pop si
	pop di
	pop ds
	pop es
	pop bp
	retf
_sound_api endp

/* 0x28d51 */
_sound_api_dispatch proc near
	cmp cl, 0
	jne L28d8d
	call FAR PTR _install_driver
	xor ah, ah
	cmp al, 0ffh
	jne L28d63
	mov ah, al
L28d63:
	xor cx, cx
	mov cl, byte ptr cs:want_cl
	les bx, dword ptr cs:api_arg0
	mov es:[bx], cx
	mov cl, byte ptr cs:ah_high
	les bx, dword ptr cs:api_arg2
	mov es:[bx], cx
	mov cl, byte ptr cs:want_ch
	les bx, dword ptr cs:api_arg4
	mov es:[bx], cx
	ret
L28d8d:
	cmp cl, 1
	jne L28d9d
	mov bx, word ptr cs:api_arg0
	call FAR PTR _configure_driver
	ret
L28d9d:
	cmp cl, 2
	jne L28da8
	call FAR PTR _silence_driver
	ret
L28da8:
	cmp cl, 3
	jne L28db8
	mov cx, word ptr cs:api_arg0
	call FAR PTR _sound_hold
	ret
L28db8:
	cmp cl, 4
	jne L28dc8
	mov cx, word ptr cs:api_arg0
	call FAR PTR _driver_fn13
	ret
L28dc8:
	cmp cl, 5
	jne L28dd3
	call FAR PTR _seek_sequence
	ret
L28dd3:
	cmp cl, 6
	jne L28de3
	mov cx, word ptr cs:api_arg0
	call FAR PTR _set_master_level
	ret
L28de3:
	cmp cl, 7
	jne L28df3
	mov cx, word ptr cs:api_arg0
	call FAR PTR _set_sequence_level
	ret
L28df3:
	cmp cl, 8
	jne L28e03
	mov cx, word ptr cs:api_arg0
	call FAR PTR _start_sequence
	ret
L28e03:
	cmp cl, 9
	jne L28e0e
	call FAR PTR _retire_and_tick
	ret
L28e0e:
	cmp cl, 0ah
	jne L28e1e
	mov cl, byte ptr cs:api_arg0
	call FAR PTR L293b3
	ret
L28e1e:
	cmp cl, 0bh
	jne L28e38
	mov dl, byte ptr cs:api_arg0
	mov cl, byte ptr cs:api_arg1
	mov ch, byte ptr cs:api_arg2
	call FAR PTR L2941b
	ret
L28e38:
	cmp cl, 0ch
	jne L28e48
	mov cl, byte ptr cs:api_arg0
	call FAR PTR L2943f
	ret
L28e48:
	cmp cl, 0dh
	jne L28e58
	mov cx, word ptr cs:api_arg0
	call FAR PTR L29449
	ret
L28e58:
	cmp cl, 0eh
	jne L28e68
	mov cx, word ptr cs:api_arg0
	call FAR PTR L29486
	ret
L28e68:
	cmp cl, 0fh
	jne L28e77
	mov cx, word ptr cs:api_arg0
	call FAR PTR L29496
L28e77:
	cmp cl, 11h
	jne L28e83
	call FAR PTR L2956a
	mov ax, cx
L28e83:
	cmp cl, 10h
	jne L28e92
	call FAR PTR L29554
	mov al, cl
	xor ah, ah
	ret
L28e92:
	cmp cl, 19h
	jne L28ea1
	call FAR PTR L2955e
	mov al, cl
	xor ah, ah
	ret
L28ea1:
	cmp cl, 12h
	jne L28ecb
	call FAR PTR L29574
	xor ah, ah
	les bx, dword ptr cs:api_arg0
	mov al, dl
	mov es:[bx], ax
	les bx, dword ptr cs:api_arg2
	mov al, ch
	mov es:[bx], ax
	les bx, dword ptr cs:api_arg4
	mov al, cl
	mov es:[bx], ax
L28ecb:
	cmp cl, 13h
	jne L28ee5
	mov dl, byte ptr cs:api_arg0
	mov ch, byte ptr cs:api_arg1
	mov cl, byte ptr cs:api_arg2
	call FAR PTR L29595
	ret
L28ee5:
	cmp cl, 14h
	jne L28eff
	mov dl, byte ptr cs:api_arg0
	mov ch, byte ptr cs:api_arg1
	mov cl, byte ptr cs:api_arg2
	call FAR PTR L295e7
	ret
L28eff:
	cmp cl, 15h
	jne L28f19
	mov dl, byte ptr cs:api_arg0
	mov ch, byte ptr cs:api_arg1
	mov cl, byte ptr cs:api_arg2
	call FAR PTR L29638
	ret
L28f19:
	cmp cl, 16h
	jne L28f2e
	mov dl, byte ptr cs:api_arg0
	mov cl, byte ptr cs:api_arg1
	call FAR PTR L29716
	ret
L28f2e:
	cmp cl, 17h
	jne L28f43
	mov dl, byte ptr cs:api_arg0
	mov cx, word ptr cs:api_arg1
	call FAR PTR L29767
	ret
L28f43:
	ret
_sound_api_dispatch endp

/* 0x28f44 */
_install_driver proc far
	mov word ptr cs:driver_off, ax
	mov word ptr cs:driver_seg, es
	push bp
	mov bp, 0
	call dword ptr cs:driver_off
	pop bp
	mov byte ptr cs:want_cl, cl
	mov byte ptr cs:want_ch, ch
	mov dl, ah
	shr dl, 1
	shr dl, 1
	shr dl, 1
	shr dl, 1
	cmp word ptr DGROUP:_g_sound_bank+28h, 0
	je L28f75
	or dl, 1
L28f75:
	mov byte ptr cs:ah_high, dl
	retf
_install_driver endp

/* 0x28f7b */
_configure_driver proc far
	push cx
	push bp
	mov bp, 1
	call dword ptr cs:driver_off
	pop bp
	mov byte ptr cs:voice_lo, cl
	mov byte ptr cs:voice_hi, ch
	push ax
	xor cl, cl
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
	pop ax
	pop cx
	retf
_configure_driver endp

/* 0x28fa0 */
_silence_driver proc far
	push bx
	push cx
	push es
	push si
	mov cl, 0fh
	push bp
	mov bp, 0ch
	call dword ptr cs:driver_off
	pop bp
	push bp
	mov bp, 2
	call dword ptr cs:driver_off
	pop bp
	pop si
	pop es
	pop cx
	pop bx
	retf
_silence_driver endp

/* 0x28fbf */
_sound_hold proc far
	cmp cx, 0
	jne L28fca
	inc byte ptr cs:busy
	retf
L28fca:
	cmp byte ptr cs:busy, 0
	je L28fd7
	dec byte ptr cs:busy
L28fd7:
	retf
_sound_hold endp

/* 0x28fd8 */
_driver_fn13 proc far
	push bp
	mov bp, 0dh
	call dword ptr cs:driver_off
	pop bp
	retf
_driver_fn13 endp

/* 0x28fe3 */
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
L28ff8:
	mov dl, es:[bx+si+143h]
	and dl, 0f0h
	mov byte ptr cs:snd_unknown_1d8[si], dl
	dec si
	jns L28ff8
	mov byte ptr cs:muted, 1
	call FAR PTR _start_sequence
	mov cx, es:[bx+154h]
	mov word ptr es:[bx+154h], 0
	mov al, es:[bx+15dh]
	mov byte ptr es:[bx+15dh], 1
	cmp cx, 0
	je L2904e
L2902f:
	mov dx, es:[bx+154h]
	call _step_sequence
	cmp dx, es:[bx+154h]
	jb L29047
	je L2904e
	sub dx, es:[bx+154h]
	sub cx, dx
L29047:
	cmp cx, es:[bx+154h]
	jne L2902f
L2904e:
	mov es:[bx+15dh], al
	mov byte ptr cs:muted, 0
	mov si, 0eh
L2905c:
	mov dl, byte ptr cs:snd_unknown_1d8[si]
	or es:[bx+si+143h], dl
	dec si
	jns L2905c
	call _sequencer_tick
	pop dx
	pop cx
	pop bx
	pop ax
	pop si
	popf
	retf
_seek_sequence endp

/* 0x29073 */
_set_master_level proc far
	cmp cl, 0ffh
	je L2907f
	cmp cl, 0fh
	jbe L2907f
	mov cl, 0fh
L2907f:
	push bp
	mov bp, 0ch
	call dword ptr cs:driver_off
	pop bp
	retf
_set_master_level endp

/* 0x2908a */
_set_sequence_level proc far
	cmp cl, 0ffh
	jne L2909a
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
	retf
L2909a:
	cmp cl, 0ah
	jbe L290a6
	mov al, byte ptr cs:param_default
	xor ah, ah
	retf
L290a6:
	mov al, byte ptr cs:param_default
	push es
	push ax
	push bx
	push dx
	mov byte ptr cs:param_default, cl
	les bx, dword ptr cs:playing_0_off
	mov dx, es
	or dx, bx
	je L290d0
	cmp byte ptr es:[bx+15fh], 7fh
	jne L290d0
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
L290d0:
	pop dx
	pop bx
	pop ax
	pop es
	retf
_set_sequence_level endp

/* 0x290d5 */
_start_sequence proc far
	pushf
	cli
	push cx
	push cx
	mov cx, es
	xor di, di
L290dd:
	cmp word ptr cs:playing_0_off[di], ax
	jne L290f3
	cmp word ptr cs:playing_0_seg[di], cx
	jne L290f3
	call _remove_sequence
	call _sequencer_tick
	jmp short L290fb
L290f3:
	add di, 4
	cmp di, 40h
	jne L290dd
L290fb:
	pop cx
	mov bx, ax
	mov byte ptr es:[bx+159h], 1
	cmp cx, 0
	je L2910e
	inc byte ptr es:[bx+159h]
L2910e:
	call _init_sequence_params
	mov bx, ax
	xor si, si
	xor cx, cx
	dec ch
L29119:
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
	jne L29119
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
	je L291d8
	cmp byte ptr es:[bx+15bh], 0
	jne L291d8
	mov cl, ds:[bp+20h]
	mov es:[bx+15ch], cl
L291d8:
	mov cx, bp
L291da:
	mov dx, ds:[bp]
	cmp dx, 0
	je L29219
	mov bp, cx
	add bp, dx
	mov dl, ds:[bp]
	cmp dl, 0feh
	jne L2921c
	cmp byte ptr cs:ah_high, 0
	jne L29211
	shl si, 1
	mov word ptr es:[bx+si+0ch], 0
	mov word ptr es:[bx+si+2ch], 0
	shr si, 1
	mov byte ptr es:[bx+si+8ch], 0feh
	jmp L292ec
L29211:
	mov dx, si
	inc dx
	mov es:[bx+165h], dl
L29219:
	jmp L292fd
L2921c:
	mov es:[bx+si+8ch], dl
	mov dh, dl
	or dh, 0b0h
	mov es:[bx+si+9ch], dh
	mov dl, ds:[bp+0ch]
	xor dh, dh
	cmp dl, 0f8h
	jne L2923a
	mov dl, 0f0h
	mov dh, 80h
L2923a:
	shl si, 1
	mov es:[bx+si+4ch], dx
	shr si, 1
	push si
	mov dl, es:[bx+si+8ch]
	and byte ptr es:[bx+si+8ch], 0fh
	test dl, 10h
	je L29271
	shl si, 1
	mov word ptr es:[bx+si+0ch], 3
	mov word ptr es:[bx+si+4ch], 0
	xor dh, dh
	mov si, dx
	and si, 0fh
	or byte ptr es:[bx+si+134h], 2
	jmp short L292eb
L29271:
	xor dh, dh
	mov si, dx
	and si, 0fh
	test dl, 20h
	je L29284
	or byte ptr es:[bx+si+134h], 1
L29284:
	test dl, 40h
	je L2928f
	mov byte ptr es:[bx+si+143h], 1
L2928f:
	cmp si, 0fh
	jne L292a7
	cmp byte ptr es:[bx+15fh], 7fh
	jne L292da
	mov al, ds:[bp+8]
	mov es:[bx+15fh], al
	jmp short L292eb
L292a7:
	cmp byte ptr es:[bx+si+0dah], 0ffh
	jne L292b8
	mov al, ds:[bp+1]
	mov es:[bx+si+0dah], al
L292b8:
	cmp byte ptr es:[bx+si+116h], 0ffh
	jne L292c9
	mov al, ds:[bp+4]
	mov es:[bx+si+116h], al
L292c9:
	cmp byte ptr es:[bx+si+107h], 0ffh
	jne L292da
	mov al, ds:[bp+8]
	mov es:[bx+si+107h], al
L292da:
	cmp byte ptr es:[bx+si+0f8h], 0ffh
	jne L292eb
	mov al, ds:[bp+0bh]
	mov es:[bx+si+0f8h], al
L292eb:
	pop si
L292ec:
	inc si
	shl si, 1
	mov bp, cx
	add bp, si
	shr si, 1
	cmp si, 10h
	je L292fd
	jmp L291da
L292fd:
	pop ds
	pop bp
	pop ax
	cmp byte ptr es:[bx+159h], 2
	jne L29314
	mov di, 0eh
L2930b:
	or byte ptr es:[bx+di+134h], 1
	dec di
	jns L2930b
L29314:
	mov ax, bx
	mov dl, es:[bx+15ch]
	push es
	xor di, di
L2931e:
	cmp word ptr cs:playing_0_seg[di], 0
	je L29362
	les bx, dword ptr cs:playing_0_off[di]
	cmp es:[bx+15ch], dl
	jbe L2933d
	add di, 4
	cmp di, 40h
	jne L2931e
	pop es
	jmp short L293a6
L2933d:
	mov si, 38h
L29340:
	mov bx, si
	add bx, 4
	cmp bx, di
	je L29362
	mov bx, word ptr cs:playing_0_off[si]
	mov word ptr cs:playing_1_off[si], bx
	mov bx, word ptr cs:playing_0_seg[si]
	mov word ptr cs:playing_1_seg[si], bx
	sub si, 4
	jmp short L29340
L29362:
	pop es
	mov bx, ax
	mov ax, es
	mov word ptr cs:playing_0_off[di], bx
	mov word ptr cs:playing_0_seg[di], ax
	cmp byte ptr cs:muted, 0
	jne L293a6
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
L293a6:
	pop cx
	popf
	retf
_start_sequence endp

/* 0x293a9 */
_retire_and_tick proc far
	pushf
	cli
	call _remove_sequence
	call _sequencer_tick
	popf
	retf
L293b3:
	pushf
	cli
	push ax
	push bx
	push si
	mov bx, es
	or bx, ax
	jne L293f8
	xor si, si
L293c0:
	les bx, dword ptr cs:playing_0_off[si]
	mov ax, es
	or ax, bx
	jne L293d5
	cmp si, 0
	jne L29413
	mov si, 4
	jmp short L293c0
L293d5:
	mov al, es:[bx+164h]
	cmp cl, 0
	je L293e3
	inc al
	jmp short L293e9
L293e3:
	cmp al, 0
	je L293e9
	dec al
L293e9:
	mov es:[bx+164h], al
	add si, 4
	cmp si, 40h
	jne L293c0
	jmp short L29413
L293f8:
	mov bx, ax
	mov al, es:[bx+164h]
	cmp cl, 0
	je L29408
	inc al
	jmp short L2940e
L29408:
	cmp al, 0
	je L2940e
	dec al
L2940e:
	mov es:[bx+164h], al
L29413:
	call _sequencer_tick
	pop si
	pop bx
	pop ax
	popf
	retf
L2941b:
	pushf
	cli
	push bx
	mov bx, ax
	cmp es:[bx+15eh], dl
	je L2943c
	mov es:[bx+160h], dl
	mov es:[bx+161h], cl
	mov es:[bx+163h], ch
	mov byte ptr es:[bx+162h], 0
L2943c:
	pop bx
	popf
	retf
L2943f:
	push bx
	mov bx, ax
	mov es:[bx+15ah], cl
	pop bx
	retf
L29449:
	push bx
	push dx
	push si
	inc byte ptr cs:busy
	mov bx, ax
	mov si, 0eh
L29456:
	mov dl, es:[bx+si+143h]
	cmp cx, 0
	jne L2946a
	cmp dl, 0fh
	jbe L29472
	sub dl, 10h
	jmp short L29472
L2946a:
	cmp dl, 0f0h
	jae L29472
	add dl, 10h
L29472:
	mov es:[bx+si+143h], dl
	dec si
	jns L29456
	call _sequencer_tick
	dec byte ptr cs:busy
	pop si
	pop dx
	pop bx
	retf
L29486:
	pushf
	cli
	push bx
	mov bx, ax
	call L2ac2f
	xor ch, ch
	call _set_sequence_volume
	pop bx
	popf
	retf
L29496:
	pushf
	cli
	push bx
	push cx
	push dx
	push si
	push di
	mov bx, ax
	cmp es:[bx+15ch], cl
	jne L294a9
	jmp L2954d
L294a9:
	mov es:[bx+15ch], cl
	call L2ac2f
	cmp si, 0ffh
	jne L294ba
	jmp L2954d
L294ba:
	mov word ptr cs:playing_0_off[si], 0
	mov word ptr cs:playing_0_seg[si], 0
	cmp si, 3ch
	je L294f7
L294cd:
	mov cx, word ptr cs:playing_1_off[si]
	mov word ptr cs:playing_0_off[si], cx
	mov cx, word ptr cs:playing_1_seg[si]
	mov word ptr cs:playing_0_seg[si], cx
	add si, 4
	cmp si, 3ch
	jne L294cd
	mov word ptr cs:playing_0_off[si], 0
	mov word ptr cs:playing_0_seg[si], 0
L294f7:
	mov dl, es:[bx+15ch]
	push es
	xor di, di
L294ff:
	cmp word ptr cs:playing_0_seg[di], 0
	je L2953d
	les bx, dword ptr cs:playing_0_off[di]
	mov es:[bx+15ch], dl
	jbe L29518
	add di, 4
	jmp short L294ff
L29518:
	mov si, 38h
L2951b:
	mov bx, si
	add bx, 4
	cmp bx, di
	je L2953d
	mov bx, word ptr cs:playing_0_off[si]
	mov word ptr cs:playing_1_off[si], bx
	mov bx, word ptr cs:playing_0_seg[si]
	mov word ptr cs:playing_1_seg[si], bx
	sub si, 4
	jmp short L2951b
L2953d:
	pop es
	mov word ptr cs:playing_0_off[di], ax
	mov cx, es
	mov word ptr cs:playing_0_seg[di], cx
	call _sequencer_tick
L2954d:
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	popf
	retf
L29554:
	push bx
	mov bx, ax
	mov cl, es:[bx+158h]
	pop bx
	retf
L2955e:
	push bx
	mov bx, ax
	xor cl, cl
	xchg es:[bx+158h], cl
	pop bx
	retf
L2956a:
	push bx
	mov bx, ax
	mov cx, es:[bx+152h]
	pop bx
	retf
L29574:
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
L29595:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:busy
	mov bx, ax
	call L2ac2f
	cmp si, 0ffh
	je L295dd
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
	jmp short L295ca
c_295c3 label byte
	db 90h
L295c4:
	inc si
	cmp si, 10h
	je L295dd
L295ca:
	cmp byte ptr cs:voice_held[si], dl
	jne L295c4
	mov ax, si
	push bp
	mov bp, 4
	call dword ptr cs:driver_off
	pop bp
L295dd:
	dec byte ptr cs:busy
	pop dx
	pop bx
	pop ax
	pop si
	retf
L295e7:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:busy
	mov bx, ax
	call L2ac2f
	cmp si, 0ffh
	je L2962e
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
	jmp short L2961b
c_29614 label byte
	db 90h
L29615:
	inc si
	cmp si, 10h
	je L2962e
L2961b:
	cmp byte ptr cs:voice_held[si], dl
	jne L29615
	mov ax, si
	push bp
	mov bp, 5
	call dword ptr cs:driver_off
	pop bp
L2962e:
	dec byte ptr cs:busy
	pop dx
	pop bx
	pop ax
	pop si
	retf
L29638:
	pushf
	cli
	push ax
	push bx
	push cx
	push dx
	push si
	mov bx, ax
	call L2ac2f
	cmp si, 0ffh
	jne L2964d
	jmp L2970f
L2964d:
	push si
	xor dh, dh
	mov si, dx
	cmp ch, 7
	jne L29666
	mov es:[bx+si+107h], cl
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	jmp short L296d6
L29666:
	cmp ch, 0ah
	jne L29672
	mov es:[bx+si+0f8h], cl
	jmp short L296d6
L29672:
	cmp ch, 1
	jne L2967e
	mov es:[bx+si+0e9h], cl
	jmp short L296d6
L2967e:
	cmp ch, 40h
	jne L2969e
	shl si, 1
	mov ax, es:[bx+si+0bch]
	and ah, 7fh
	cmp cl, 0
	je L29695
	or ah, 80h
L29695:
	mov es:[bx+si+0bch], ax
	shr si, 1
	jmp short L296d6
L2969e:
	cmp ch, 4eh
	jne L296cc
	push dx
	mov dl, es:[bx+si+143h]
	cmp cl, 0
	jne L296b8
	cmp dl, 0fh
	jbe L296c8
	sub dl, 10h
	jmp short L296c0
L296b8:
	cmp dl, 0f0h
	jae L296c8
	add dl, 10h
L296c0:
	mov es:[bx+si+143h], dl
	call _sequencer_tick
L296c8:
	pop dx
	pop si
	jmp short L2970f
L296cc:
	cmp ch, 7fh
	jne L296d6
	mov es:[bx+si+116h], cl
L296d6:
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
L296e3:
	cmp byte ptr cs:voice_held[si], dl
	je L296f2
	inc si
	cmp si, 10h
	jne L296e3
	jmp short L2970f
L296f2:
	mov ax, si
	cmp ch, 7fh
	jne L29705
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
	jmp short L2970f
L29705:
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
L2970f:
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	popf
	retf
L29716:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:busy
	mov bx, ax
	call L2ac2f
	cmp si, 0ffh
	je L2975d
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
	jmp short L2974a
c_29743 label byte
	db 90h
L29744:
	inc si
	cmp si, 10h
	je L2975d
L2974a:
	cmp byte ptr cs:voice_held[si], dl
	jne L29744
	mov ax, si
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
L2975d:
	dec byte ptr cs:busy
	pop dx
	pop bx
	pop ax
	pop si
	retf
L29767:
	push ax
	push bx
	push cx
	push dx
	push si
	mov bx, ax
	call L2ac2f
	cmp si, 0ffh
	je L297c7
	push si
	xor dh, dh
	mov si, dx
	shl si, 1
	mov ax, cx
	cmp byte ptr es:[bx+si+0bdh], 80h
	jb L2978b
	or ah, 80h
L2978b:
	mov es:[bx+si+0bch], ax
	shr si, 1
	mov dx, si
	pop si
	shl si, 1
	shl si, 1
	or dx, si
	xor si, si
L2979d:
	cmp byte ptr cs:voice_held[si], dl
	je L297ac
	inc si
	cmp si, 10h
	jne L2979d
	jmp short L297c7
L297ac:
	shl ch, 1
	cmp cl, 80h
	jb L297b6
	or ch, 1
L297b6:
	and cl, 7fh
	xchg ch, cl
	mov ax, si
	push bp
	mov bp, 0ah
	call dword ptr cs:driver_off
	pop bp
L297c7:
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	retf
_retire_and_tick endp

/* 0x297cd */
_remove_sequence proc near
	push si
	push es
	push ax
	push bx
	push ds
	push bp
	xor si, si
	mov cx, es
L297d7:
	cmp ax, word ptr cs:playing_0_off[si]
	jne L297e5
	cmp cx, word ptr cs:playing_0_seg[si]
	je L297f0
L297e5:
	add si, 4
	cmp si, 40h
	jne L297d7
	jmp L29875
L297f0:
	mov word ptr cs:playing_0_off[si], 0
	mov word ptr cs:playing_0_seg[si], 0
	cmp si, 3ch
	je L2982d
L29803:
	mov cx, word ptr cs:playing_1_off[si]
	mov word ptr cs:playing_0_off[si], cx
	mov cx, word ptr cs:playing_1_seg[si]
	mov word ptr cs:playing_0_seg[si], cx
	add si, 4
	cmp si, 3ch
	jne L29803
	mov word ptr cs:playing_0_off[si], 0
	mov word ptr cs:playing_0_seg[si], 0
L2982d:
	mov bx, ax
	mov byte ptr es:[bx+158h], 0ffh
	mov byte ptr es:[bx+159h], 0
	cmp byte ptr es:[bx+165h], 0
	je L29875
	lds bp, dword ptr es:[bx+8]
	lds bp, dword ptr ds:[bp]
	mov al, es:[bx+165h]
	cmp al, 80h
	jb L29875
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
L29875:
	pop bp
	pop ds
	pop bx
	pop ax
	pop es
	pop si
	ret
_remove_sequence endp

/* 0x2987c */
_sequencer_tick proc near
	push ax
	push bx
	push cx
	push dx
	push di
	push si
	push bp
	push es
	inc byte ptr cs:busy
	mov byte ptr cs:voices_changed, 0
	xor ax, ax
	mov bx, 0ffffh
	mov word ptr cs:voice_held, bx
	mov word ptr cs:voice_held_2, bx
	mov word ptr cs:voice_held_4, bx
	mov word ptr cs:voice_held_6, bx
	mov word ptr cs:voice_held_8, bx
	mov word ptr cs:voice_held_a, bx
	mov word ptr cs:voice_held_c, bx
	mov word ptr cs:voice_held_e, bx
	mov word ptr cs:voice_gives_back, ax
	mov word ptr cs:voice_gives_back_2, ax
	mov word ptr cs:voice_gives_back_4, ax
	mov word ptr cs:voice_gives_back_6, ax
	mov word ptr cs:voice_gives_back_8, ax
	mov word ptr cs:voice_gives_back_a, ax
	mov word ptr cs:voice_gives_back_c, ax
	mov word ptr cs:voice_gives_back_e, ax
	mov word ptr cs:voice_keep_own, ax
	mov word ptr cs:voice_keep_own_2, ax
	mov word ptr cs:voice_keep_own_4, ax
	mov word ptr cs:voice_keep_own_6, ax
	mov word ptr cs:voice_keep_own_8, ax
	mov word ptr cs:voice_keep_own_a, ax
	mov word ptr cs:voice_keep_own_c, ax
	mov word ptr cs:voice_keep_own_e, ax
	mov word ptr cs:voice_cost, ax
	mov word ptr cs:voice_cost_2, ax
	mov word ptr cs:voice_cost_4, ax
	mov word ptr cs:voice_cost_6, ax
	mov word ptr cs:voice_cost_8, ax
	mov word ptr cs:voice_cost_a, ax
	mov word ptr cs:voice_cost_c, ax
	mov word ptr cs:voice_cost_e, ax
	mov word ptr cs:voice_request, bx
	mov word ptr cs:voice_request_2, bx
	mov word ptr cs:voice_request_4, bx
	mov word ptr cs:voice_request_6, bx
	mov word ptr cs:voice_request_8, bx
	mov word ptr cs:voice_request_a, bx
	mov word ptr cs:voice_request_c, bx
	mov word ptr cs:voice_request_e, bx
	mov word ptr cs:polled_0_off, ax
	mov word ptr cs:polled_0_seg, ax
	les bx, dword ptr cs:playing_0_off
	mov dx, es
	or dx, bx
	jne L29985
	mov dx, 0ffffh
	mov word ptr cs:voice_held, dx
	mov word ptr cs:voice_held_2, dx
	mov word ptr cs:voice_held_4, dx
	mov word ptr cs:voice_held_6, dx
	mov word ptr cs:voice_held_8, dx
	mov word ptr cs:voice_held_a, dx
	mov word ptr cs:voice_held_c, dx
	mov word ptr cs:voice_held_e, dx
	jmp L2a153
L29985:
	mov cl, es:[bx+15fh]
	cmp cl, 7fh
	jne L29994
	mov cl, byte ptr cs:param_default
L29994:
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
	xor bp, bp
	xor si, si
	mov al, byte ptr cs:want_cl
L299a6:
	les bx, dword ptr cs:playing_0_off[si]
	mov dx, es
	or dx, bx
	jne L299b4
	jmp L29e9e
L299b4:
	les bx, dword ptr cs:playing_0_off[si]
	cmp byte ptr es:[bx+164h], 0
	je L299c4
	jmp L29e90
L299c4:
	cmp byte ptr es:[bx+165h], 0
	je L299f1
	cmp word ptr cs:polled_0_off, 0
	je L299d7
	jmp L29e90
L299d7:
	cmp word ptr cs:polled_0_seg, 0
	je L299e2
	jmp L29e90
L299e2:
	mov word ptr cs:polled_0_off, bx
	mov bx, es
	mov word ptr cs:polled_0_seg, bx
	jmp L29e90
L299f1:
	push ax
	mov ax, word ptr cs:voice_request
	mov word ptr cs:saved_request, ax
	mov ax, word ptr cs:voice_request_2
	mov word ptr cs:saved_request_2, ax
	mov ax, word ptr cs:voice_request_4
	mov word ptr cs:saved_request_4, ax
	mov ax, word ptr cs:voice_request_6
	mov word ptr cs:saved_request_6, ax
	mov ax, word ptr cs:voice_request_8
	mov word ptr cs:saved_request_8, ax
	mov ax, word ptr cs:voice_request_a
	mov word ptr cs:saved_request_a, ax
	mov ax, word ptr cs:voice_request_c
	mov word ptr cs:saved_request_c, ax
	mov ax, word ptr cs:voice_request_e
	mov word ptr cs:saved_request_e, ax
	mov ax, word ptr cs:voice_cost
	mov word ptr cs:saved_cost, ax
	mov ax, word ptr cs:voice_cost_2
	mov word ptr cs:saved_cost_2, ax
	mov ax, word ptr cs:voice_cost_4
	mov word ptr cs:saved_cost_4, ax
	mov ax, word ptr cs:voice_cost_6
	mov word ptr cs:saved_cost_6, ax
	mov ax, word ptr cs:voice_cost_8
	mov word ptr cs:saved_cost_8, ax
	mov ax, word ptr cs:voice_cost_a
	mov word ptr cs:saved_cost_a, ax
	mov ax, word ptr cs:voice_cost_c
	mov word ptr cs:saved_cost_c, ax
	mov ax, word ptr cs:voice_cost_e
	mov word ptr cs:saved_cost_e, ax
	mov ax, word ptr cs:voice_gives_back
	mov word ptr cs:saved_gives_back, ax
	mov ax, word ptr cs:voice_gives_back_2
	mov word ptr cs:saved_gives_back_2, ax
	mov ax, word ptr cs:voice_gives_back_4
	mov word ptr cs:saved_gives_back_4, ax
	mov ax, word ptr cs:voice_gives_back_6
	mov word ptr cs:saved_gives_back_6, ax
	mov ax, word ptr cs:voice_gives_back_8
	mov word ptr cs:saved_gives_back_8, ax
	mov ax, word ptr cs:voice_gives_back_a
	mov word ptr cs:saved_gives_back_a, ax
	mov ax, word ptr cs:voice_gives_back_c
	mov word ptr cs:saved_gives_back_c, ax
	mov ax, word ptr cs:voice_gives_back_e
	mov word ptr cs:saved_gives_back_e, ax
	mov ax, word ptr cs:voice_keep_own
	mov word ptr cs:saved_keep_own, ax
	mov ax, word ptr cs:voice_keep_own_2
	mov word ptr cs:saved_keep_own_2, ax
	mov ax, word ptr cs:voice_keep_own_4
	mov word ptr cs:saved_keep_own_4, ax
	mov ax, word ptr cs:voice_keep_own_6
	mov word ptr cs:saved_keep_own_6, ax
	mov ax, word ptr cs:voice_keep_own_8
	mov word ptr cs:saved_keep_own_8, ax
	mov ax, word ptr cs:voice_keep_own_a
	mov word ptr cs:saved_keep_own_a, ax
	mov ax, word ptr cs:voice_keep_own_c
	mov word ptr cs:saved_keep_own_c, ax
	mov ax, word ptr cs:voice_keep_own_e
	mov word ptr cs:saved_keep_own_e, ax
	pop ax
	mov byte ptr cs:saved_total, al
	xor di, di
L29af9:
	mov cl, es:[bx+di+8ch]
	cmp cl, 0ffh
	jne L29b06
	jmp L29d7e
L29b06:
	cmp cl, 0feh
	jne L29b0e
	jmp L29d7e
L29b0e:
	cmp cl, 0fh
	jne L29b16
	jmp L29d7e
L29b16:
	push di
	mov di, cx
	and di, 0ffh
	test byte ptr es:[bx+di+134h], 2
	je L29b29
	pop di
	jmp L29d7e
L29b29:
	test byte ptr es:[bx+di+143h], 0ffh
	pop di
	je L29b35
	jmp L29d7e
L29b35:
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
	je L29b67
	push dx
	mov dx, 10h
	sub dl, ch
	add dx, bp
	mov ch, dl
	pop dx
L29b67:
	push di
	mov di, cx
	and di, 0ffh
	test byte ptr es:[bx+di+134h], 1
	je L29b84
	cmp byte ptr cs:voice_request[di], 0ffh
	jne L29b84
	pop di
	mov dh, cl
	jmp L29c14
L29b84:
	pop di
	mov dh, 0ffh
	push bx
	xor bx, bx
L29b8a:
	cmp byte ptr cs:voice_request[bx], 0ffh
	je L29b9d
	cmp byte ptr cs:voice_request[bx], dl
	jne L29bad
	pop bx
	jmp L29d7e
L29b9d:
	cmp bl, byte ptr cs:voice_lo
	jb L29bad
	cmp bl, byte ptr cs:voice_hi
	ja L29bad
	mov dh, bl
L29bad:
	inc bl
	cmp bl, 10h
	jne L29b8a
	pop bx
	cmp dh, 0ffh
	jne L29c14
	cmp ch, 0
	je L29bc2
	jmp L29e90
L29bc2:
	push di
	push cx
	push ax
	mov dh, 0ffh
	xor ax, ax
	xor di, di
L29bcb:
	cmp al, byte ptr cs:voice_cost[di]
	jae L29bdb
	mov al, byte ptr cs:voice_cost[di]
	mov cx, di
	mov dh, cl
L29bdb:
	inc di
	cmp di, 10h
	jne L29bcb
	pop ax
	cmp dh, 0ffh
	je L29c0a
	xor cx, cx
	mov cl, dh
	mov di, cx
	add al, byte ptr cs:voice_gives_back[di]
	mov byte ptr cs:voice_request[di], 0ffh
	mov byte ptr cs:voice_gives_back[di], 0
	mov byte ptr cs:voice_cost[di], 0
	mov byte ptr cs:voice_keep_own[di], 0
L29c0a:
	pop cx
	pop di
	cmp dh, 0ffh
	jne L29c14
	jmp L29d8a
L29c14:
	cmp ah, al
	jbe L29c76
	cmp ch, 0
	je L29c20
	jmp L29d7e
L29c20:
	push di
	push cx
	push ax
	mov dh, 0ffh
	xor ax, ax
	xor di, di
L29c29:
	cmp al, byte ptr cs:voice_cost[di]
	jae L29c39
	mov al, byte ptr cs:voice_cost[di]
	mov cx, di
	mov dh, cl
L29c39:
	inc di
	cmp di, 10h
	jne L29c29
	pop ax
	cmp dh, 0ffh
	je L29c68
	xor cx, cx
	mov cl, dh
	mov di, cx
	add al, byte ptr cs:voice_gives_back[di]
	mov byte ptr cs:voice_request[di], 0ffh
	mov byte ptr cs:voice_gives_back[di], 0
	mov byte ptr cs:voice_cost[di], 0
	mov byte ptr cs:voice_keep_own[di], 0
L29c68:
	pop cx
	pop di
	cmp dh, 0ffh
	jne L29c72
	jmp L29d8a
L29c72:
	cmp ah, al
	ja L29c20
L29c76:
	push di
	xchg dl, dh
	mov di, dx
	xchg dl, dh
	and di, 0ffh
	mov byte ptr cs:voice_request[di], dl
	mov byte ptr cs:voice_gives_back[di], ah
	sub al, ah
	mov byte ptr cs:voice_cost[di], ch
	push di
	mov di, cx
	and di, 0ffh
	test byte ptr es:[bx+di+134h], 1
	pop di
	jne L29cac
	mov byte ptr cs:voice_keep_own[di], 0
	pop di
	jmp L29d7e
L29cac:
	mov byte ptr cs:voice_keep_own[di], 1
	cmp dh, cl
	jne L29cba
	pop di
	jmp L29d7e
L29cba:
	push si
	mov si, cx
	and si, 0ffh
	cmp byte ptr cs:voice_keep_own[si], 0
	jne L29d1f
	push ax
	mov al, byte ptr cs:voice_request[di]
	mov ah, byte ptr cs:voice_request[si]
	mov byte ptr cs:voice_request[di], ah
	mov byte ptr cs:voice_request[si], al
	mov al, byte ptr cs:voice_cost[di]
	mov ah, byte ptr cs:voice_cost[si]
	mov byte ptr cs:voice_cost[di], ah
	mov byte ptr cs:voice_cost[si], al
	mov al, byte ptr cs:voice_gives_back[di]
	mov ah, byte ptr cs:voice_gives_back[si]
	mov byte ptr cs:voice_gives_back[di], ah
	mov byte ptr cs:voice_gives_back[si], al
	mov al, byte ptr cs:voice_keep_own[di]
	mov ah, byte ptr cs:voice_keep_own[si]
	mov byte ptr cs:voice_keep_own[di], ah
	mov byte ptr cs:voice_keep_own[si], al
	pop ax
	pop si
	pop di
	jmp short L29d7e
L29d1f:
	cmp ch, 0
	je L29d42
	mov byte ptr cs:voice_request[di], 0ffh
	mov byte ptr cs:voice_cost[di], 0
	mov byte ptr cs:voice_gives_back[di], 0
	mov byte ptr cs:voice_keep_own[di], 0
	add al, ah
	pop si
	pop di
	jmp short L29d7e
L29d42:
	cmp byte ptr cs:voice_cost[si], 0
	jne L29d4e
	pop si
	pop di
	jmp short L29d8a
L29d4e:
	add al, byte ptr cs:voice_gives_back[si]
	mov byte ptr cs:voice_request[di], 0ffh
	mov byte ptr cs:voice_gives_back[di], 0
	mov byte ptr cs:voice_cost[di], 0
	mov byte ptr cs:voice_keep_own[di], 0
	mov byte ptr cs:voice_request[si], dl
	mov byte ptr cs:voice_cost[si], ch
	mov byte ptr cs:voice_gives_back[si], ah
	sub al, ah
	pop si
	pop di
L29d7e:
	inc di
	cmp di, 10h
	jne L29d87
	jmp L29e90
L29d87:
	jmp L29af9
L29d8a:
	push ax
	mov ax, word ptr cs:saved_request
	mov word ptr cs:voice_request, ax
	mov ax, word ptr cs:saved_request_2
	mov word ptr cs:voice_request_2, ax
	mov ax, word ptr cs:saved_request_4
	mov word ptr cs:voice_request_4, ax
	mov ax, word ptr cs:saved_request_6
	mov word ptr cs:voice_request_6, ax
	mov ax, word ptr cs:saved_request_8
	mov word ptr cs:voice_request_8, ax
	mov ax, word ptr cs:saved_request_a
	mov word ptr cs:voice_request_a, ax
	mov ax, word ptr cs:saved_request_c
	mov word ptr cs:voice_request_c, ax
	mov ax, word ptr cs:saved_request_e
	mov word ptr cs:voice_request_e, ax
	mov ax, word ptr cs:saved_cost
	mov word ptr cs:voice_cost, ax
	mov ax, word ptr cs:saved_cost_2
	mov word ptr cs:voice_cost_2, ax
	mov ax, word ptr cs:saved_cost_4
	mov word ptr cs:voice_cost_4, ax
	mov ax, word ptr cs:saved_cost_6
	mov word ptr cs:voice_cost_6, ax
	mov ax, word ptr cs:saved_cost_8
	mov word ptr cs:voice_cost_8, ax
	mov ax, word ptr cs:saved_cost_a
	mov word ptr cs:voice_cost_a, ax
	mov ax, word ptr cs:saved_cost_c
	mov word ptr cs:voice_cost_c, ax
	mov ax, word ptr cs:saved_cost_e
	mov word ptr cs:voice_cost_e, ax
	mov ax, word ptr cs:saved_gives_back
	mov word ptr cs:voice_gives_back, ax
	mov ax, word ptr cs:saved_gives_back_2
	mov word ptr cs:voice_gives_back_2, ax
	mov ax, word ptr cs:saved_gives_back_4
	mov word ptr cs:voice_gives_back_4, ax
	mov ax, word ptr cs:saved_gives_back_6
	mov word ptr cs:voice_gives_back_6, ax
	mov ax, word ptr cs:saved_gives_back_8
	mov word ptr cs:voice_gives_back_8, ax
	mov ax, word ptr cs:saved_gives_back_a
	mov word ptr cs:voice_gives_back_a, ax
	mov ax, word ptr cs:saved_gives_back_c
	mov word ptr cs:voice_gives_back_c, ax
	mov ax, word ptr cs:saved_gives_back_e
	mov word ptr cs:voice_gives_back_e, ax
	mov ax, word ptr cs:saved_keep_own
	mov word ptr cs:voice_keep_own, ax
	mov ax, word ptr cs:saved_keep_own_2
	mov word ptr cs:voice_keep_own_2, ax
	mov ax, word ptr cs:saved_keep_own_4
	mov word ptr cs:voice_keep_own_4, ax
	mov ax, word ptr cs:saved_keep_own_6
	mov word ptr cs:voice_keep_own_6, ax
	mov ax, word ptr cs:saved_keep_own_8
	mov word ptr cs:voice_keep_own_8, ax
	mov ax, word ptr cs:saved_keep_own_a
	mov word ptr cs:voice_keep_own_a, ax
	mov ax, word ptr cs:saved_keep_own_c
	mov word ptr cs:voice_keep_own_c, ax
	mov ax, word ptr cs:saved_keep_own_e
	mov word ptr cs:voice_keep_own_e, ax
	pop ax
	mov al, byte ptr cs:saved_total
L29e90:
	add bp, 10h
	add si, 4
	cmp si, 40h
	je L29e9e
	jmp L299a6
L29e9e:
	xor si, si
L29ea0:
	cmp byte ptr cs:voice_request[si], 0ffh
	jne L29eab
	jmp L2a033
L29eab:
	cmp byte ptr cs:voice_keep_own[si], 0
	jne L29eb6
	jmp L29fc4
L29eb6:
	xor ax, ax
	mov al, byte ptr cs:voice_request[si]
	mov byte ptr cs:voice_request[si], 0ffh
	mov byte ptr cs:voice_held[si], al
	mov di, ax
	and di, 0f0h
	shr di, 1
	shr di, 1
	les bx, dword ptr cs:playing_0_off[di]
	and al, 0fh
	cmp byte ptr cs:voice_channel[si], al
	jne L29ef9
	mov di, si
	shl di, 1
	shl di, 1
	cmp word ptr cs:voice_sequence_0_off[di], bx
	jne L29ef9
	mov cx, es
	cmp word ptr cs:voice_sequence_0_seg[di], cx
	jne L29ef9
	jmp L2a033
L29ef9:
	push ax
	push dx
	push si
	xor ah, ah
	xchg si, ax
	mov cx, 7b00h
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+0dah]
	and cl, 0fh
	mov ch, 4bh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+116h]
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
	push si
	mov si, ax
	mov byte ptr cs:pending_volume[si], 0ffh
	pop si
	mov cl, es:[bx+si+107h]
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	mov ch, 7
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov ch, 0ah
	mov cl, es:[bx+si+0f8h]
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov ch, 1
	mov cl, es:[bx+si+0e9h]
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	shl si, 1
	mov cx, 4000h
	cmp byte ptr es:[bx+si+0bdh], 80h
	jb L29f83
	mov cl, 7fh
L29f83:
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cx, es:[bx+si+0bch]
	shr si, 1
	xchg cl, ch
	shl cl, 1
	cmp ch, 80h
	jb L29fa0
	or cl, 1
L29fa0:
	and cx, 7f7fh
	push bp
	mov bp, 0ah
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+125h]
	mov ch, 4eh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	pop si
	pop dx
	pop ax
	jmp short L2a033
L29fc4:
	mov al, byte ptr cs:voice_request[si]
	mov bl, al
	and ax, 0fh
	and bx, 0f0h
	shr bl, 1
	shr bl, 1
	les bx, dword ptr cs:playing_0_off[bx]
	mov cl, byte ptr cs:voice_lo
	xor ch, ch
	mov di, cx
	shl di, 1
	shl di, 1
L29fe8:
	cmp word ptr cs:voice_sequence_0_off[di], bx
	jne L2a007
	mov cx, es
	cmp word ptr cs:voice_sequence_0_seg[di], cx
	jne L2a007
	shr di, 1
	shr di, 1
	cmp byte ptr cs:voice_channel[di], al
	je L2a01b
	shl di, 1
	shl di, 1
L2a007:
	add di, 4
	mov cx, di
	shr cl, 1
	shr cl, 1
	dec cl
	cmp byte ptr cs:voice_hi, cl
	jne L29fe8
	jmp short L2a033
L2a01b:
	cmp byte ptr cs:voice_keep_own[di], 0
	jne L2a033
	mov cl, byte ptr cs:voice_request[si]
	mov byte ptr cs:voice_held[di], cl
	mov byte ptr cs:voice_request[si], 0ffh
L2a033:
	inc si
	cmp si, 10h
	je L2a03c
	jmp L29ea0
L2a03c:
	mov al, byte ptr cs:voice_hi
	inc al
	xor ah, ah
	mov di, ax
	xor al, al
	xor si, si
L2a04a:
	cmp byte ptr cs:voice_request[si], 0ffh
	jne L2a055
	jmp L2a14a
L2a055:
	mov bx, di
L2a057:
	dec bx
	cmp byte ptr cs:voice_held[bx], 0ffh
	jne L2a057
	mov di, bx
	mov al, byte ptr cs:voice_request[si]
	mov byte ptr cs:voice_held[di], al
	mov bl, al
	and al, 0fh
	and bx, 0f0h
	shr bx, 1
	shr bx, 1
	les bx, dword ptr cs:playing_0_off[bx]
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
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+0dah]
	and cl, 0fh
	mov ch, 4bh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+116h]
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
	push si
	mov si, ax
	mov byte ptr cs:pending_volume[si], 0ffh
	pop si
	mov cl, es:[bx+si+107h]
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	mov ch, 7
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov ch, 0ah
	mov cl, es:[bx+si+0f8h]
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov ch, 1
	mov cl, es:[bx+si+0e9h]
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	shl si, 1
	mov cx, 4000h
	cmp byte ptr es:[bx+si+0bdh], 80h
	jb L2a10a
	mov cl, 7fh
L2a10a:
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cx, es:[bx+si+0bch]
	shr si, 1
	xchg cl, ch
	shl cl, 1
	cmp ch, 80h
	jb L2a127
	or cl, 1
L2a127:
	and cx, 7f7fh
	push bp
	mov bp, 0ah
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+125h]
	mov ch, 4eh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	pop si
	pop dx
	pop ax
	pop si
L2a14a:
	inc si
	cmp si, 10h
	je L2a153
	jmp L2a04a
L2a153:
	mov si, 0fh
L2a156:
	cmp byte ptr cs:voice_channel[si], 0fh
	je L2a18f
	cmp byte ptr cs:voice_held[si], 0ffh
	jne L2a18f
	mov ax, si
	mov cx, 4000h
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cx, 7b00h
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cx, 4b00h
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
L2a18f:
	dec si
	jns L2a156
	mov ax, word ptr cs:voice_held
	and ax, 0f0fh
	mov word ptr cs:voice_channel, ax
	mov ax, word ptr cs:voice_held_2
	and ax, 0f0fh
	mov word ptr cs:voice_channel_2, ax
	mov ax, word ptr cs:voice_held_4
	and ax, 0f0fh
	mov word ptr cs:voice_channel_4, ax
	mov ax, word ptr cs:voice_held_6
	and ax, 0f0fh
	mov word ptr cs:voice_channel_6, ax
	mov ax, word ptr cs:voice_held_8
	and ax, 0f0fh
	mov word ptr cs:voice_channel_8, ax
	mov ax, word ptr cs:voice_held_a
	and ax, 0f0fh
	mov word ptr cs:voice_channel_a, ax
	mov ax, word ptr cs:voice_held_c
	and ax, 0f0fh
	mov word ptr cs:voice_channel_c, ax
	mov ax, word ptr cs:voice_held_e
	and ax, 0f0fh
	mov word ptr cs:voice_channel_e, ax
	xor si, si
	xor di, di
L2a1ee:
	mov bl, byte ptr cs:voice_held[si]
	cmp bl, 0ffh
	jne L2a208
	mov word ptr cs:voice_sequence_0_off[di], 0
	mov word ptr cs:voice_sequence_0_seg[di], 0
	jmp short L2a224
L2a208:
	and bx, 0f0h
	shr bx, 1
	shr bx, 1
	mov ax, word ptr cs:playing_0_off[bx]
	mov word ptr cs:voice_sequence_0_off[di], ax
	mov ax, word ptr cs:playing_0_seg[bx]
	mov word ptr cs:voice_sequence_0_seg[di], ax
L2a224:
	add di, 4
	inc si
	cmp si, 10h
	jne L2a1ee
	dec byte ptr cs:busy
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

/* 0x2a23b */
_advance_volume_ramp proc near
	push bx
	push cx
	cmp byte ptr es:[bx+162h], 0
	je L2a24d
	dec byte ptr es:[bx+162h]
	jmp L2a2f8
L2a24d:
	mov cl, es:[bx+161h]
	mov es:[bx+162h], cl
	mov cl, es:[bx+160h]
	and cl, 7fh
	cmp cl, es:[bx+15eh]
	je L2a2d4
	ja L2a29e
	mov cl, es:[bx+15eh]
	mov ch, es:[bx+160h]
	and ch, 7fh
	sub cl, ch
	cmp cl, es:[bx+163h]
	ja L2a28d
	mov cl, es:[bx+160h]
	and cl, 7fh
	mov ch, 1
	call _set_sequence_volume
	jmp short L2a2d4
L2a28d:
	mov cl, es:[bx+15eh]
	sub cl, es:[bx+163h]
	mov ch, 1
	call _set_sequence_volume
	jmp short L2a2f8
L2a29e:
	mov cl, es:[bx+160h]
	and cl, 7fh
	mov ch, es:[bx+15eh]
	sub cl, ch
	cmp cl, es:[bx+163h]
	ja L2a2c3
	mov cl, es:[bx+160h]
	and cl, 7fh
	mov ch, 1
	call _set_sequence_volume
	jmp short L2a2d4
L2a2c3:
	mov cl, es:[bx+15eh]
	add cl, es:[bx+163h]
	mov ch, 1
	call _set_sequence_volume
	jmp short L2a2f8
L2a2d4:
	mov byte ptr es:[bx+158h], 0feh
	mov byte ptr es:[bx+163h], 0
	mov cl, es:[bx+160h]
	and cl, 80h
	cmp cl, 0
	je L2a2f8
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:voices_changed, 1
L2a2f8:
	pop cx
	pop bx
	ret
_advance_volume_ramp endp

/* 0x2a2fb */
_set_sequence_volume proc near
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	mov byte ptr cs:defer, ch
	cmp cl, es:[bx+15eh]
	jne L2a310
	jmp L2a3d1
L2a310:
	mov es:[bx+15eh], cl
	cmp si, 0ffh
	jne L2a31e
	jmp L2a3d1
L2a31e:
	mov dx, si
	shl dl, 1
	shl dl, 1
	xor si, si
L2a326:
	mov cl, byte ptr cs:voice_held[si]
	cmp cl, 0ffh
	je L2a373
	mov ch, cl
	and cl, 0f0h
	cmp cl, dl
	jne L2a373
	mov cl, ch
	and cx, 0fh
	mov di, cx
	mov cl, es:[bx+di+107h]
	push dx
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	cmp byte ptr cs:defer, 0
	je L2a35e
	mov byte ptr cs:pending_volume[si], cl
	jmp short L2a372
L2a35e:
	mov ch, 7
	mov ax, si
	mov byte ptr cs:pending_volume[si], 0ffh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
L2a372:
	pop dx
L2a373:
	inc si
	cmp si, 10h
	jne L2a326
	xor ch, ch
	xor si, si
L2a37d:
	mov cl, es:[bx+si+8ch]
	cmp cl, 0ffh
	je L2a3d1
	mov di, cx
	test byte ptr es:[bx+di+134h], 2
	je L2a3cb
	cmp byte ptr cs:voice_held[di], 0ffh
	jne L2a3cb
	mov al, cl
	mov cl, es:[bx+di+107h]
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	cmp byte ptr cs:defer, 0
	je L2a3b7
	mov byte ptr cs:pending_volume[di], cl
	jmp short L2a3cb
L2a3b7:
	mov ch, 7
	mov ax, di
	mov byte ptr cs:pending_volume[di], 0ffh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
L2a3cb:
	inc si
	cmp si, 10h
	jne L2a37d
L2a3d1:
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret
_set_sequence_volume endp

/* 0x2a3d8 */
_flush_pending_volumes proc near
	xor bl, bl
	mov al, byte ptr cs:scan_stopped
	xor ah, ah
	mov si, ax
L2a3e2:
	mov cl, byte ptr cs:pending_volume[si]
	cmp cl, 0ffh
	je L2a407
	mov byte ptr cs:pending_volume[si], 0ffh
	mov ch, 7
	mov ax, si
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	inc bl
	cmp bl, 2
	je L2a419
L2a407:
	inc si
	cmp si, 10h
	jne L2a40f
	xor si, si
L2a40f:
	mov al, byte ptr cs:scan_stopped
	xor ah, ah
	cmp si, ax
	jne L2a3e2
L2a419:
	mov ax, si
	mov byte ptr cs:scan_stopped, al
	ret
_flush_pending_volumes endp

/* 0x2a420 */
_sound_service proc far
	cmp byte ptr cs:busy, 0
	je L2a429
	retf
L2a429:
	pushf
	cli
	push si
	push di
	push es
	push ds
	push bp
	cmp byte ptr cs:voices_changed, 0
	je L2a43b
	call _sequencer_tick
L2a43b:
	xor si, si
	xor di, di
L2a43f:
	les bx, dword ptr cs:playing_0_off[si]
	mov ax, es
	or ax, bx
	je L2a48d
	cmp byte ptr es:[bx+164h], 0
	jne L2a482
	cmp byte ptr es:[bx+163h], 0
	je L2a46a
	call _advance_volume_ramp
	cmp byte ptr es:[bx+158h], 0ffh
	jne L2a46a
	sub si, 4
	jmp short L2a482
L2a46a:
	cmp byte ptr es:[bx+165h], 0
	je L2a477
	call _drop_unless_polled
	jmp short L2a47a
L2a477:
	call _step_sequence
L2a47a:
	cmp byte ptr es:[bx+158h], 0ffh
	je L2a485
L2a482:
	add si, 4
L2a485:
	add di, 4
	cmp si, 40h
	jne L2a43f
L2a48d:
	call _poll_sequences
	call _flush_pending_volumes
	push bp
	mov bp, 3
	call dword ptr cs:driver_off
	pop bp
	pop bp
	pop ds
	pop es
	pop di
	pop si
	popf
	retf
_sound_service endp

/* 0x2a4a4 */
_drop_unless_polled proc near
	push cx
	push si
	push ax
	mov cx, es
	xor si, si
L2a4ab:
	cmp word ptr cs:polled_0_off[si], bx
	jne L2a4b9
	cmp word ptr cs:polled_0_seg[si], cx
	je L2a4cc
L2a4b9:
	add si, 4
	cmp si, 40h
	jne L2a4ab
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:voices_changed, 1
L2a4cc:
	pop ax
	pop si
	pop cx
	ret
_drop_unless_polled endp

/* 0x2a4d0 */
_poll_sequences proc near
	push ds
	xor si, si
L2a4d3:
	les bx, dword ptr cs:polled_0_off[si]
	mov cx, es
	cmp cx, 0
	jne L2a4e7
	cmp bx, 0
	jne L2a4e7
	jmp L2a598
L2a4e7:
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
	ja L2a552
	or byte ptr es:[bx+165h], 80h
	push bx
	mov bx, ax
	inc bx
	cmp byte ptr [bx], 0feh
	jne L2a526
	inc bx
L2a526:
	inc bx
	mov ax, [bx+2]
	push ax
	push ds
	mov ax, bx
	add ax, 8
	push ax
	mov ax, [bx]
	push ax
	mov cl, es:[bx+15eh]
	mov ch, es:[bx+15dh]
	push cx
	mov ax, sp
	push ax
	mov ax, 3
	push ax
	call FAR PTR _sound_callback
	add sp, 0eh
	pop bx
	jmp short L2a58d
L2a552:
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
	je L2a578
	mov word ptr es:[bx+154h], 0
L2a578:
	cmp al, 0
	je L2a58d
	mov byte ptr es:[bx+165h], 0
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:voices_changed, 1
L2a58d:
	add si, 4
	cmp si, 40h
	je L2a598
	jmp L2a4d3
L2a598:
	pop ds
	ret
_poll_sequences endp

/* 0x2a59a */
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
	mov byte ptr cs:slot_high, cl
	inc word ptr es:[bx+154h]
	lds bp, dword ptr es:[bx+8]
	lds bp, dword ptr ds:[bp]
	mov word ptr cs:cursor_park, bp
	xor si, si
L2a5c1:
	mov al, es:[bx+si+8ch]
	cmp al, 0ffh
	jne L2a5cd
	jmp L2a76b
L2a5cd:
	cmp al, 0feh
	jne L2a5d4
	jmp L2a762
L2a5d4:
	mov byte ptr cs:own_voice, 0ffh
	mov byte ptr cs:bend_gate, 0
	push si
	mov si, ax
	and si, 0ffh
	test byte ptr es:[bx+si+134h], 2
	pop si
	je L2a5fc
	mov byte ptr cs:own_voice, al
	mov byte ptr cs:bend_gate, 1
	jmp short L2a61d
L2a5fc:
	and al, 0fh
	mov cl, al
	or cl, byte ptr cs:slot_high
	xor di, di
L2a607:
	cmp byte ptr cs:voice_held[di], cl
	je L2a616
	inc di
	cmp di, 10h
	jne L2a607
	jmp short L2a61d
L2a616:
	mov dx, di
	mov byte ptr cs:own_voice, dl
L2a61d:
	mov bp, word ptr cs:cursor_park
	shl si, 1
	mov dx, ds:[bp+si]
	add bp, dx
	add bp, es:[bx+si+0ch]
	cmp word ptr es:[bx+si+0ch], 0
	jne L2a639
	shr si, 1
	jmp L2a762
L2a639:
	shr si, 1
	shl si, 1
	cmp word ptr es:[bx+si+4ch], 0
	je L2a675
	dec word ptr es:[bx+si+4ch]
	cmp word ptr es:[bx+si+4ch], 8000h
	jne L2a670
	xor dh, dh
	shr si, 1
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	shl si, 1
	cmp dl, 0f8h
	jne L2a66c
	mov dl, 0f0h
	mov dh, 80h
L2a66c:
	mov es:[bx+si+4ch], dx
L2a670:
	shr si, 1
	jmp L2a762
L2a675:
	shr si, 1
L2a677:
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp dl, 80h
	jb L2a690
	mov es:[bx+si+9ch], dl
	jmp short L2a69e
L2a690:
	mov dl, es:[bx+si+9ch]
	dec bp
	shl si, 1
	dec word ptr es:[bx+si+0ch]
	shr si, 1
L2a69e:
	mov al, dl
	mov ah, al
	and ah, 0f0h
	and al, 0fh
	cmp dl, 0fch
	jne L2a6b9
	shl si, 1
	mov word ptr es:[bx+si+0ch], 0
	shr si, 1
	jmp L2a762
L2a6b9:
	cmp al, 0fh
	jne L2a6d0
	call _midi_meta_event
	shl si, 1
	mov dx, es:[bx+si+0ch]
	shr si, 1
	cmp dx, 0
	jne L2a730
	jmp L2a762
L2a6d0:
	mov al, byte ptr cs:own_voice
	cmp ah, 80h
	jne L2a6de
	call _midi_note_off_event
	jmp short L2a730
L2a6de:
	cmp ah, 90h
	jne L2a6e8
	call _midi_note_event
	jmp short L2a730
L2a6e8:
	cmp ah, 0a0h
	jne L2a6f2
	call _midi_event_6
	jmp short L2a730
L2a6f2:
	cmp ah, 0b0h
	jne L2a6fc
	call _midi_controller_event
	jmp short L2a730
L2a6fc:
	cmp ah, 0c0h
	jne L2a706
	call _midi_program_event
	jmp short L2a730
L2a706:
	cmp ah, 0d0h
	jne L2a710
	call _midi_event_9
	jmp short L2a730
L2a710:
	cmp ah, 0e0h
	jne L2a71a
	call _midi_bend_event
	jmp short L2a730
L2a71a:
	cmp ah, 0f0h
	jne L2a724
	call _midi_skip_event
	jmp short L2a730
L2a724:
	shl si, 1
	mov word ptr es:[bx+si+0ch], 0
	shr si, 1
	jmp short L2a762
L2a730:
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp dl, 0
	jne L2a745
	jmp L2a677
L2a745:
	shl si, 1
	cmp dl, 0f8h
	jne L2a758
	mov dl, 0efh
	mov dh, 80h
	mov es:[bx+si+4ch], dx
	shr si, 1
	jmp short L2a762
L2a758:
	xor dh, dh
	dec dl
	mov es:[bx+si+4ch], dx
	shr si, 1
L2a762:
	inc si
	cmp si, 10h
	je L2a76b
	jmp L2a5c1
L2a76b:
	xor si, si
L2a76d:
	cmp byte ptr es:[bx+si+8ch], 0ffh
	je L2a786
	shl si, 1
	cmp word ptr es:[bx+si+0ch], 0
	jne L2a7d5
	shr si, 1
	inc si
	cmp si, 10h
	jne L2a76d
L2a786:
	cmp byte ptr es:[bx+15ah], 0
	jne L2a7a3
	cmp byte ptr es:[bx+15dh], 0
	jne L2a7a3
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:voices_changed, 1
	jmp short L2a7d5
L2a7a3:
	mov dx, es:[bx+156h]
	mov es:[bx+154h], dx
	xor si, si
L2a7af:
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
	jne L2a7af
L2a7d5:
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

/* 0x2a7de */
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
	jne L2a813
	mov byte ptr es:[bx+si+125h], 0ffh
L2a813:
	cmp al, 0ffh
	je L2a82b
	cmp byte ptr cs:muted, 0
	jne L2a82b
	and al, 0fh
	push bp
	mov bp, 4
	call dword ptr cs:driver_off
	pop bp
L2a82b:
	pop si
	ret
_midi_note_off_event endp

/* 0x2a82d */
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
	je L2a879
	mov es:[bx+si+125h], ch
	cmp al, 0ffh
	je L2a89e
	cmp byte ptr cs:muted, 0
	jne L2a89e
	and al, 0fh
	push bp
	mov bp, 5
	call dword ptr cs:driver_off
	pop bp
	jmp short L2a89e
L2a879:
	cmp es:[bx+si+125h], ch
	jne L2a886
	mov byte ptr es:[bx+si+125h], 0ffh
L2a886:
	cmp al, 0ffh
	je L2a89e
	cmp byte ptr cs:muted, 0
	jne L2a89e
	and al, 0fh
	push bp
	mov bp, 4
	call dword ptr cs:driver_off
	pop bp
L2a89e:
	pop si
	ret
_midi_note_event endp

/* 0x2a8a0 */
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
	je L2a8d0
	cmp byte ptr cs:muted, 0
	jne L2a8d0
	push bp
	mov bp, 6
	call dword ptr cs:driver_off
	pop bp
L2a8d0:
	ret
_midi_event_6 endp

/* 0x2a8d1 */
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
	test byte ptr cs:bend_gate, 0ffh
	je L2a907
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:voice_held[si], 0ffh
	pop si
	je L2a907
	jmp L2a9d0
L2a907:
	push ax
	mov al, es:[bx+si+8ch]
	mov si, ax
	and si, 0fh
	pop ax
	cmp ch, 7
	jne L2a93d
	mov es:[bx+si+107h], cl
	mov dl, es:[bx+15eh]
	call _scale_byte_pair
	cmp al, 20h
	jb L2a92d
	jmp L2a9d0
L2a92d:
	push si
	mov si, ax
	and si, 0ffh
	mov byte ptr cs:pending_volume[si], 0ffh
	pop si
	jmp short L2a9b8
L2a93d:
	cmp ch, 0ah
	jne L2a949
	mov es:[bx+si+0f8h], cl
	jmp short L2a9b8
L2a949:
	cmp ch, 1
	jne L2a955
	mov es:[bx+si+0e9h], cl
	jmp short L2a9b8
L2a955:
	cmp ch, 40h
	jne L2a977
	push dx
	shl si, 1
	mov dx, es:[bx+si+0bch]
	or dh, 80h
	cmp cl, 0
	jne L2a96d
	and dh, 7fh
L2a96d:
	mov es:[bx+si+0bch], dx
	shr si, 1
	pop dx
	jmp short L2a9b8
L2a977:
	cmp ch, 4bh
	jne L2a995
	push cx
	mov ch, es:[bx+si+0dah]
	and ch, 0f0h
	or ch, cl
	mov es:[bx+si+0dah], ch
	pop cx
	mov byte ptr cs:voices_changed, 1
	jmp short L2a9b8
L2a995:
	cmp ch, 4eh
	jne L2a9b8
	push cx
	mov ch, es:[bx+si+143h]
	and ch, 0f0h
	test cl, 0ffh
	je L2a9aa
	mov cl, 1
L2a9aa:
	or ch, cl
	mov es:[bx+si+143h], ch
	pop cx
	mov byte ptr cs:voices_changed, 1
L2a9b8:
	cmp al, 0ffh
	jae L2a9d0
	cmp byte ptr cs:muted, 0
	jne L2a9d0
	and al, 0fh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
L2a9d0:
	pop si
	ret
_midi_controller_event endp

/* 0x2a9d2 */
_midi_program_event proc near
	push si
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	test byte ptr cs:bend_gate, 0ffh
	je L2a9fa
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:voice_held[si], 0ffh
	pop si
	je L2a9fa
	jmp short L2aa24
L2a9fa:
	push ax
	mov al, es:[bx+si+8ch]
	mov si, ax
	and si, 0fh
	pop ax
	mov es:[bx+si+116h], cl
	cmp al, 0ffh
	jae L2aa24
	cmp byte ptr cs:muted, 0
	jne L2aa24
	and al, 0fh
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
L2aa24:
	pop si
	ret
_midi_program_event endp

/* 0x2aa26 */
_midi_event_9 proc near
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp al, 0ffh
	jae L2aa49
	cmp byte ptr cs:muted, 0
	jne L2aa49
	push bp
	mov bp, 9
	call dword ptr cs:driver_off
	pop bp
L2aa49:
	ret
_midi_event_9 endp

/* 0x2aa4a */
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
	test byte ptr cs:bend_gate, 0ffh
	je L2aa7f
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:voice_held[si], 0ffh
	pop si
	je L2aa7f
	jmp short L2aac4
L2aa7f:
	push ax
	mov al, es:[bx+si+8ch]
	mov si, ax
	and si, 0fh
	pop ax
	push cx
	xchg ch, cl
	shr ch, 1
	jae L2aa96
	or cl, 80h
L2aa96:
	shl si, 1
	cmp word ptr es:[bx+si+0bch], 8000h
	jb L2aaa4
	or ch, 80h
L2aaa4:
	mov es:[bx+si+0bch], cx
	shr si, 1
	pop cx
	cmp al, 0ffh
	jae L2aac4
	cmp byte ptr cs:muted, 0
	jne L2aac4
	and al, 0fh
	push bp
	mov bp, 0ah
	call dword ptr cs:driver_off
	pop bp
L2aac4:
	pop si
	ret
_midi_bend_event endp

/* 0x2aac6 */
_midi_skip_event proc near
	call _skip_unknown_event
	ret
_midi_skip_event endp

/* 0x2aaca */
_midi_meta_event proc near
	cmp ah, 0c0h
	je L2aadd
	cmp ah, 0b0h
	jne L2aad7
	jmp L2ab6b
L2aad7:
	call _skip_unknown_event
	jmp L2abd9
L2aadd:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp ch, 7fh
	jne L2ab5c
	push dx
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	xor dh, dh
	cmp dl, 0f8h
	jne L2ab08
	mov dh, 80h
	mov dl, 0f0h
L2ab08:
	shl si, 1
	mov es:[bx+si+4ch], dx
	shr si, 1
	pop dx
	mov byte ptr es:[bx+si+9ch], 0cfh
	push si
	push dx
	xor si, si
L2ab1b:
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
	jne L2ab1b
	mov dx, es:[bx+154h]
	mov es:[bx+156h], dx
	pop dx
	pop si
	shl si, 1
	dec word ptr es:[bx+si+0ch]
	dec bp
	mov word ptr es:[bx+si+4ch], 0
	shr si, 1
	jmp short L2abd9
L2ab5c:
	cmp byte ptr cs:muted, 0
	jne L2abd9
	mov es:[bx+158h], ch
	jmp short L2abd9
L2ab6b:
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
	jne L2aba7
	cmp cl, 7fh
	jne L2ab94
	mov cl, byte ptr cs:param_default
L2ab94:
	mov es:[bx+15fh], cl
	push ax
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
	pop ax
	jmp short L2abd9
L2aba7:
	cmp ch, 60h
	jne L2abbb
	cmp byte ptr cs:muted, 0
	jne L2abd9
	inc word ptr es:[bx+152h]
	jmp short L2abd9
L2abbb:
	cmp ch, 52h
	jne L2abd9
	cmp es:[bx+15ah], cl
	jne L2abd9
	push si
	xor si, si
L2abca:
	mov word ptr es:[bx+si+0ch], 0
	add si, 2
	cmp si, 20h
	jne L2abca
	pop si
L2abd9:
	ret
_midi_meta_event endp

/* 0x2abda */
_skip_unknown_event proc near
	cmp ah, 0f0h
	jne L2abf2
L2abdf:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	cmp ch, 0f7h
	jne L2abdf
	ret
L2abf2:
	cmp ah, 0c0h
	je L2ac09
	cmp ah, 0d0h
	je L2ac09
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
L2ac09:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+0ch]
	shr si, 1
	ret
_skip_unknown_event endp

/* 0x2ac17 */
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
	je L2ac2d
	dec cl
L2ac2d:
	pop ax
	ret
L2ac2f:
	push cx
	mov cx, es
	xor si, si
L2ac34:
	cmp word ptr cs:playing_0_off[si], bx
	jne L2ac44
	cmp word ptr cs:playing_0_seg[si], cx
	jne L2ac44
	jmp short L2ac4f
L2ac44:
	add si, 4
	cmp si, 40h
	jne L2ac34
	mov si, 0ffh
L2ac4f:
	pop cx
	ret
_scale_byte_pair endp

/* 0x2ac51 */
_init_sequence_params proc near
	push bx
	push cx
	push dx
	push bp
	push ds
	mov bx, ax
	cmp word ptr es:[bx+8], -1
	jne L2ac69
	cmp word ptr es:[bx+0ah], -1
	jne L2ac69
	jmp L2ad25
L2ac69:
	lds bp, dword ptr es:[bx+8]
	lds bp, dword ptr ds:[bp]
	cmp byte ptr ds:[bp+23h], 0feh
	jne L2ac89
	cmp byte ptr ds:[bp+22h], 0fdh
	jne L2ac89
	cmp byte ptr ds:[bp+21h], 0fch
	jne L2ac89
	jmp L2ad25
L2ac89:
	push bp
	mov si, 20h
L2ac8d:
	sub si, 2
	mov word ptr cs:scratch[si], 0
	cmp si, 0
	jne L2ac8d
	mov byte ptr cs:scratch_mark, 0ffh
	cmp byte ptr ds:[bp], 0f0h
	jne L2acb5
	mov cl, ds:[bp+1]
	mov byte ptr cs:scratch_mark, cl
	add bp, 8
L2acb5:
	mov cl, ds:[bp]
	cmp cl, byte ptr cs:want_ch
	je L2acd7
	cmp cl, 0ffh
	je L2acf4
	inc bp
L2acc6:
	mov cl, ds:[bp]
	inc bp
	cmp cl, 0ffh
	jne L2acd2
	jmp short L2acb5
L2acd2:
	add bp, 5
	jmp short L2acc6
L2acd7:
	inc bp
L2acd8:
	mov cl, ds:[bp]
	inc bp
	cmp cl, 0ffh
	je L2acf4
	inc bp
	mov cx, ds:[bp]
	add bp, 4
	mov word ptr cs:scratch[si], cx
	add si, 2
	jmp short L2acd8
L2acf4:
	pop bp
	push bp
	xor si, si
L2acf8:
	mov cx, word ptr cs:scratch[si]
	mov ds:[bp], cx
	add si, 2
	add bp, 2
	cmp si, 20h
	jne L2acf8
	mov cl, byte ptr cs:scratch_mark
	mov ds:[bp], cl
	pop bp
	mov byte ptr ds:[bp+21h], 0fch
	mov byte ptr ds:[bp+22h], 0fdh
	mov byte ptr ds:[bp+23h], 0feh
L2ad25:
	pop ds
	pop bp
	pop dx
	pop cx
	pop bx
	ret
_init_sequence_params endp
	db 0
SOUND_TEXT ends
}
#else
/*
 * 0x28cef
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
 * 0x28d51
 *
 * **The dispatcher**: CL chooses one of the driver interface's routines, each far-called, with the arguments `sound_api` filed. NOT TRANSCRIBED YET for the host: nothing the port runs reaches it. A
 * stub, which aborts; the TASM source above is the original's.
 */
void sound_api_dispatch(void)
{
    not_transcribed("0x263ff");
}

/*
 * 0x28f44
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

    g_snds.driver = drv;

    driver_describe_0(&ax, &cx);

    g_snds.cl = (uint8_t)cx;
    g_snds.ch = (uint8_t)(cx >> 8);

    dl = (uint8_t)((ax >> 8) >> 4);
    if (((int16_t)g_sound_bank.module_live) != 0)
        dl |= 1;
    g_snds.ah_high = dl;

    return ax;
}

/*
 * 0x28f7b
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

    g_snds.voice_lo = (uint8_t)cx;
    g_snds.voice_hi = (uint8_t)(cx >> 8);

    driver_param_349(0);

    return ax;
}

/*
 * 0x28fa0
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
 * 0x28fbf
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
 * 0x28fd8
 *
 * **The driver's function 13**, and nothing else (the name says only that). Called only through `sound_api`'s table. NOT TRANSCRIBED YET for the host: nothing the port runs reaches it. A
 * stub, which aborts; the TASM source above is the original's.
 */
void driver_fn13(void)
{
    not_transcribed("0x26686");
}

/*
 * 0x28fe3
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
 * 0x29073
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
 * 0x2908a
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

    g_snds.pending_volume[voice] = 0xff;

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
        g_snds.saved_request[i] = g_snds.voice_request[i];
        g_snds.saved_cost[i] = g_snds.voice_cost[i];
        g_snds.saved_gives_back[i] = g_snds.voice_gives_back[i];
        g_snds.saved_keep_own[i] = g_snds.voice_keep_own[i];
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
        g_snds.voice_request[i] = g_snds.saved_request[i];
        g_snds.voice_cost[i] = g_snds.saved_cost[i];
        g_snds.voice_gives_back[i] = g_snds.saved_gives_back[i];
        g_snds.voice_keep_own[i] = g_snds.saved_keep_own[i];
    }
}

/*
 * 0x290d5
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
        if (g_snds.playing[di / 4] == seq) {
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
                if (g_snds.ah_high != 0) {
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
        if (FP_SEG(g_snds.playing[di / 4]) == 0)
            break;
        if (g_snds.playing[di / 4]->priority <= key) {
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
                g_snds.playing[si / 4 + 1] = g_snds.playing[si / 4];
            }
            break;
        }
    }
    if (di >= 0x40)
        return;

    g_snds.playing[di / 4] = seq;

    if (g_snds.muted != 0)
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
 * 0x293a9
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
 * 0x297cd
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
        if (g_snds.playing[i] == seq)
            break;
    if (i >= 0x10)
        return;

    g_snds.playing[i] = NULL;

    if (i != 0xf) {
        for (; i != 0xf; i++)
            g_snds.playing[i] = g_snds.playing[i + 1];
        g_snds.playing[i] = NULL;
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
 * 0x2987c
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

    g_snds.busy++;
    g_snds.voices_changed = 0;

    for (i = 0; i < 0x10; i++) {
        g_snds.voice_held[i] = 0xff;
        g_snds.voice_gives_back[i] = 0;
        g_snds.voice_keep_own[i] = 0;
        g_snds.voice_cost[i] = 0;
        g_snds.voice_request[i] = 0xff;
    }
    g_snds.polled[0] = NULL;

    rec = g_snds.playing[0];

    if (rec == NULL) {
        for (i = 0; i < 0x10; i++)
            g_snds.voice_held[i] = 0xff;
        goto silence_unused;
    }

    cl = rec->device_value;
    if (cl == 0x7f)
        cl = g_snds.param_default;
    driver_param_349(cl);

    al = g_snds.cl;

    bp_ = 0;
    for (seq = 0; seq < 0x40; seq += 4) {
        rec = g_snds.playing[seq / 4];
        if (rec == NULL)
            break;

        if (rec->skip != 0)
            goto next_sequence;

        if (rec->poll != 0) {
            if (g_snds.polled[0] != NULL)
                goto next_sequence;
            g_snds.polled[0] = g_snds.playing[seq / 4];
            goto next_sequence;
        }

        tick_save_state();
        /*
         * The running total carried across sequences is parked here, not
         * zeroed: the abandon path below reads it back so a sequence that
         * fails leaves the total exactly as it found it.
         */
        g_snds.saved_total = al;

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
                && g_snds.voice_request[cl] == 0xff) {
                dh = cl;
                goto check_budget;
            }

            dh = 0xff;
            {
                int16_t bl;

                for (bl = 0; bl < 0x10; bl++) {
                    if (g_snds.voice_request[bl] == 0xff) {
                        if (bl >= (int16_t)g_snds.voice_lo
                            && bl <= (int16_t)g_snds.voice_hi)
                            dh = (uint8_t)bl;
                    } else if (g_snds.voice_request[bl] == dl) {
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
                    if (most < g_snds.voice_cost[i]) {
                        most = g_snds.voice_cost[i];
                        dh = (uint8_t)i;
                    }
                }
                if (dh == 0xff)
                    goto abandon_sequence;

                al = (uint8_t)(al + g_snds.voice_gives_back[dh]);
                g_snds.voice_request[dh] = 0xff;
                g_snds.voice_gives_back[dh] = 0;
                g_snds.voice_cost[dh] = 0;
                g_snds.voice_keep_own[dh] = 0;
            }
            if (ah > al)
                goto drop_loudest;

have_voice:
            g_snds.voice_request[dh] = dl;
            g_snds.voice_gives_back[dh] = ah;
            al = (uint8_t)(al - ah);
            g_snds.voice_cost[dh] = chh;

            if ((rec->ch.channel_flags[cl] & 1) == 0) {
                g_snds.voice_keep_own[dh] = 0;
                continue;
            }

            g_snds.voice_keep_own[dh] = 1;
            if (dh == cl)
                continue;

            if (g_snds.voice_keep_own[cl] == 0) {
                uint8_t t;

                t = g_snds.voice_request[dh];
                g_snds.voice_request[dh] = g_snds.voice_request[cl];
                g_snds.voice_request[cl] = t;
                t = g_snds.voice_cost[dh];
                g_snds.voice_cost[dh] = g_snds.voice_cost[cl];
                g_snds.voice_cost[cl] = t;
                t = g_snds.voice_gives_back[dh];
                g_snds.voice_gives_back[dh] = g_snds.voice_gives_back[cl];
                g_snds.voice_gives_back[cl] = t;
                t = g_snds.voice_keep_own[dh];
                g_snds.voice_keep_own[dh] = g_snds.voice_keep_own[cl];
                g_snds.voice_keep_own[cl] = t;
                continue;
            }

            if (chh != 0) {
                g_snds.voice_request[dh] = 0xff;
                g_snds.voice_cost[dh] = 0;
                g_snds.voice_gives_back[dh] = 0;
                g_snds.voice_keep_own[dh] = 0;
                al = (uint8_t)(al + ah);
                continue;
            }

            if (g_snds.voice_cost[cl] != 0)
                goto abandon_sequence;

            al = (uint8_t)(al + g_snds.voice_gives_back[cl]);
            g_snds.voice_request[dh] = 0xff;
            g_snds.voice_gives_back[dh] = 0;
            g_snds.voice_cost[dh] = 0;
            g_snds.voice_keep_own[dh] = 0;
            g_snds.voice_request[cl] = dl;
            g_snds.voice_cost[cl] = chh;
            g_snds.voice_gives_back[cl] = ah;
            al = (uint8_t)(al - ah);

next_channel:
            ;
        }
        goto next_sequence;

abandon_sequence:
        tick_restore_state();
        al = g_snds.saved_total;

next_sequence:
        bp_ = (uint16_t)(bp_ + 0x10);
    }

    /* Apply: reprogram every voice whose request differs from what it plays. */
    for (voice = 0; voice < 0x10; voice++) {
        if (g_snds.voice_request[voice] == 0xff)
            continue;

        if (g_snds.voice_keep_own[voice] == 0) {
            uint8_t want = g_snds.voice_request[voice];
            int16_t d;

            al = (uint8_t)(want & 0xf);

            d = g_snds.voice_lo;
            for (;;) {
                if (g_snds.voice_sequence[d]
                        == g_snds.playing[want >> 4]
                    && g_snds.voice_channel[d] == al) {
                    if (g_snds.voice_keep_own[d] == 0) {
                        g_snds.voice_held[d] = g_snds.voice_request[voice];
                        g_snds.voice_request[voice] = 0xff;
                    }
                    break;
                }
                d++;
                if ((int16_t)g_snds.voice_hi < d - 1)
                    break;
            }
            continue;
        }

        {
            uint8_t want = g_snds.voice_request[voice];

            g_snds.voice_request[voice] = 0xff;
            g_snds.voice_held[voice] = want;

            al = (uint8_t)(want & 0xf);

            if (g_snds.voice_channel[voice] == al
                && g_snds.voice_sequence[voice]
                       == g_snds.playing[want >> 4])
                continue;

            tick_program_voice(g_snds.playing[want >> 4],
                               (uint16_t)voice, al);
        }
    }

    /* Hand out anything still requested to a voice that is still free. */
    {
        int16_t free_from = (int16_t)(uint8_t)(g_snds.voice_hi + 1);

        for (voice = 0; voice < 0x10; voice++) {
            uint8_t want = g_snds.voice_request[voice];
            int16_t d;

            if (want == 0xff)
                continue;

            d = free_from;
            do {
                d--;
            } while (g_snds.voice_held[d] != 0xff);
            free_from = d;

            g_snds.voice_held[d] = want;
            al = (uint8_t)(want & 0xf);

            tick_program_voice(g_snds.playing[want >> 4],
                               (uint16_t)d, al);
        }
    }

silence_unused:
    for (voice = 0xf; voice >= 0; voice--) {
        if (g_snds.voice_channel[voice] == 0xf)
            continue;
        if (g_snds.voice_held[voice] != 0xff)
            continue;
        driver_controller((uint16_t)voice, 0x4000);
        driver_controller((uint16_t)voice, 0x7b00);
        driver_controller((uint16_t)voice, 0x4b00);
    }

    /* Eight word moves masked with 0x0f0f in the original; the same sixteen
       bytes one at a time. */
    for (i = 0; i < 0x10; i++)
        g_snds.voice_channel[i] = (uint8_t)(g_snds.voice_held[i] & 0x0f);

    for (voice = 0; voice < 0x10; voice++) {
        uint8_t held = g_snds.voice_held[voice];

        if (held == 0xff) {
            g_snds.voice_sequence[voice] = NULL;
        } else {
            g_snds.voice_sequence[voice] = g_snds.playing[held >> 4];
        }
    }

    g_snds.busy--;
}

/*
 * 0x2a23b
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
        g_snds.voices_changed = 1;
    }
}

/*
 * 0x2a2fb
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

    g_snds.defer = defer;

    if (volume == seq->volume)
        return;
    seq->volume = volume;

    if (seq_slot == 0xff)
        return;

    want = (uint8_t)(seq_slot << 2);

    for (si = 0; si < 0x10; si++) {
        uint8_t held = g_snds.voice_held[si];

        if (held == 0xff || (uint8_t)(held & 0xf0) != want)
            continue;

        di = (uint16_t)(held & 0xf);
        level = scale_byte_pair(seq->ch.volume[di], seq->volume);

        if (g_snds.defer != 0) {
            g_snds.pending_volume[si] = level;
        } else {
            g_snds.pending_volume[si] = 0xff;
            driver_controller(si, (uint16_t)((7 << 8) | level));
        }
    }

    for (si = 0; si < 0x10; si++) {
        di = seq->track_channel[si];
        if (di == 0xff)
            return;
        if ((seq->ch.channel_flags[di] & 2) == 0)
            continue;
        if (g_snds.voice_held[di] != 0xff)
            continue;

        level = scale_byte_pair(seq->ch.volume[di], seq->volume);

        if (g_snds.defer != 0) {
            g_snds.pending_volume[di] = level;
        } else {
            g_snds.pending_volume[di] = 0xff;
            driver_controller(di, (uint16_t)((7 << 8) | level));
        }
    }
}

/*
 * 0x2a3d8
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
    uint16_t si = g_snds.scan_stopped;
    int16_t sent = 0;

    for (;;) {
        uint8_t pending = g_snds.pending_volume[si];

        if (pending != 0xff) {
            g_snds.pending_volume[si] = 0xff;
            driver_controller(si, (uint16_t)((7 << 8) | pending));
            sent++;
            if (sent == 2)
                break;
        }

        si++;
        if (si == 0x10)
            si = 0;
        if (si == g_snds.scan_stopped)
            break;
    }

    g_snds.scan_stopped = (uint8_t)si;
}

/*
 * 0x2a420
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

    if (g_snds.busy != 0)
        return;

    if (g_snds.voices_changed != 0)
        sequencer_tick();

    si = 0;
    di = 0;

    while (si != 0x40) {
        struct sequence far *seq = g_snds.playing[si / 4];

        if (seq == NULL)
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
 * 0x2a4a4
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
        if (g_snds.polled[si / 4] == seq)
            return;

    remove_sequence(seq);
    g_snds.voices_changed = 1;
}

/*
 * 0x2a4d0
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
        struct sequence far *rec = g_snds.polled[si / 4];
        const uint8_t far *at;
        const uint8_t far *data;
        uint16_t answer;
        uint8_t cl;

        if (rec == NULL)
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
             * at. The sample is where `b` has been stepped to.
             */
            union sound_module_args args;

            args.play.volume = rec->volume;
            args.play.loop = rec->loop;
            args.play.rate = *(const uint16_t *)b;
            args.play.sample = b + 8;
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
            g_snds.voices_changed = 1;
        }
    }
}

/*
 * 0x2a59a
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

    g_snds.slot_high = (uint8_t)(di * 4);
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
        g_snds.cursor_park = (int16_t)FP_OFF(base);
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

        g_snds.own_voice = 0xff;
        g_snds.bend_gate = 0;

        if ((seq->ch.channel_flags[al] & 2) != 0) {
            g_snds.own_voice = al;
            g_snds.bend_gate = 1;
        } else {
            uint8_t want = (uint8_t)((al & 0xf) | g_snds.slot_high);
            uint16_t j;

            for (j = 0; j < 0x10; j++) {
                if (g_snds.voice_held[j] == want) {
                    g_snds.own_voice = (uint8_t)j;
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
                                         | g_snds.own_voice);

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
        g_snds.voices_changed = 1;
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
 * 0x2a7de
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

    if ((uint8_t)ax != 0xff && g_snds.muted == 0)
        driver_stop_note((uint16_t)(ax & 0xf),
                         (uint16_t)((note << 8) | velocity));

    return data;
}

/*
 * 0x2a8a0
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

    if ((uint8_t)ax != 0xff && g_snds.muted == 0)
        driver_nop();

    return data;
}

/*
 * 0x2a82d
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

        if ((uint8_t)ax != 0xff && g_snds.muted == 0)
            driver_start_note((uint16_t)(ax & 0xf),
                              (uint16_t)((note << 8) | velocity));
    } else {
        if (seq->ch.note[channel] == note)
            seq->ch.note[channel] = 0xff;

        if ((uint8_t)ax != 0xff && g_snds.muted == 0)
            driver_stop_note((uint16_t)(ax & 0xf),
                             (uint16_t)((note << 8) | velocity));
    }

    return data;
}

/*
 * 0x2a8d1
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

    if (g_snds.bend_gate != 0 && g_snds.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    if (ctrl == 7) {
        seq->ch.volume[channel] = value;
        value = scale_byte_pair(value, seq->volume);
        if ((uint8_t)ax >= 0x20)
            return data;
        g_snds.pending_volume[(uint8_t)ax] = 0xff;
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
        g_snds.voices_changed = 1;
    } else if (ctrl == 0x4e) {
        uint8_t *p = &seq->ch.no_voice[channel];

        *p = (uint8_t)((*p & 0xf0) | (value != 0 ? 1 : 0));
        g_snds.voices_changed = 1;
    }

    if ((uint8_t)ax != 0xff && g_snds.muted == 0)
        driver_controller((uint16_t)(ax & 0xf),
                      (uint16_t)(((uint16_t)ctrl << 8) | value));

    return data;
}

/*
 * 0x2a9d2
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

    if (g_snds.bend_gate != 0 && g_snds.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);
    seq->ch.program[channel] = program;

    if ((uint8_t)ax != 0xff && g_snds.muted == 0)
        driver_program_change((uint16_t)(ax & 0xf), program);

    return data;
}

/*
 * 0x2aa26
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

    if ((uint8_t)ax != 0xff && g_snds.muted == 0)
        driver_nop();

    return data;
}

/*
 * 0x2aa4a
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

    if (g_snds.bend_gate != 0 && g_snds.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    value = (uint16_t)((((uint16_t)msb >> 1) << 8)
                       | (uint16_t)(lsb | ((msb & 1) ? 0x80 : 0)));

    slot = &seq->ch.bend[channel];
    if (*slot >= 0x8000)
        value |= 0x8000;
    *slot = value;

    if ((uint8_t)ax != 0xff && g_snds.muted == 0)
        driver_pitch_bend((uint16_t)(ax & 0xf),
                      (uint16_t)(((uint16_t)lsb << 8) | msb));

    return data;
}

/*
 * 0x2aaca
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
            if (g_snds.muted == 0)
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
            second = g_snds.param_default;
        seq->device_value = second;
        driver_param_349(second);
        return data;
    }

    if (first == 0x60) {
        if (g_snds.muted == 0)
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
 * 0x2aac6
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
 * 0x2abda
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
 * 0x2ac17
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
 * 0x2ac51
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

    /* The original returns here when `cursor_at` is FFFF:FFFF. No host
       pointer is that pair - it is set to the record's own `cursor` and to
       nothing else - so the test has nothing to find. */

    tbl = *seq->cursor_at;

    if (tbl[0x23] == 0xfe && tbl[0x22] == 0xfd && tbl[0x21] == 0xfc)
        return;

    for (si = 0x20; si != 0;) {
        si -= 2;
        g_snds.scratch[si / 2] = 0;
    }
    g_snds.scratch_mark = 0xff;

    {
        uint16_t bp = 0;

        if (tbl[bp] == 0xf0) {
            g_snds.scratch_mark = tbl[bp + 1];
            bp += 8;
        }

        si = 0;
        for (;;) {
            uint8_t id = tbl[bp];

            if (id == g_snds.ch) {
                bp++;
                for (;;) {
                    uint8_t c = tbl[bp];

                    bp++;
                    if (c == 0xff)
                        break;
                    bp++;
                    g_snds.scratch[si / 2] = *(int16_t *)(tbl + bp);
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
            *(int16_t *)(tbl + si) = g_snds.scratch[si / 2];
        tbl[0x20] = g_snds.scratch_mark;
    }

    tbl[0x21] = 0xfc;
    tbl[0x22] = 0xfd;
    tbl[0x23] = 0xfe;
}

/*
 * 0x28ac1
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
 * 0x2898b
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
 * 0x289b4
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
 * 0x289c7
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
 * 0x28a52
 *
 * The ordinary-call face of `retire_and_tick`, which reads its record from
 * `ES:AX` - loaded here from the stack argument with one `les`.
 */
void retire_and_tick_far(struct sequence far * seq)
{
    retire_and_tick(seq);
}

/*
 * 0x289de
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
 * 0x28978
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
 * 0x28a10
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
 * 0x28a92
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
