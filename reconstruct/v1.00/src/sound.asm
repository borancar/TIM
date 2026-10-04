; The Incredible Machine - reconstruction
;
; Transcribed from the binary `TIM.EXE` of The Incredible Machine
; (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
; License, version 2 - see LICENSE.
;
; The TASM source of the module `sound.c` describes, which the judge
; assembles (`JUDGE: tasm`): the image's bytes come from this file, and
; `sound.c` is the host's transcription of it.
;
; The module as TASM assembled it, drafted by tools/asm2tasm.py (`--nosmart`);
; the host's transcription is `sound.c`. The judge hands the source to TASM
; 3.0 (`JUDGE: tasm`).
;
; JUDGE: built-with -mm
; JUDGE: tasm
; JUDGE: structs sequence_channels=chan sequence=seq sound_bank=bank
; JUDGE: assembler bc3.00

_DATA segment word public 'DATA'
_DATA ends
_BSS segment word public 'BSS'
_BSS ends
DGROUP group _DATA,_BSS
INCLUDE STRUCTS.ASH
extrn _sound_callback:far
extrn _g_sound_bank:byte
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
seek_saved_bits label byte
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

; 0x2639d
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
	je api_arg_word
	cmp cl, 3
	je api_arg_word
	cmp cl, 4
	je api_arg_word
	cmp cl, 7
	jne api_call
api_arg_word:
	mov dx, [bp+8]
	mov word ptr cs:api_arg0, dx
api_call:
	call _sound_api_dispatch
	pop si
	pop di
	pop ds
	pop es
	pop bp
	retf
_sound_api endp

; 0x263ff
_sound_api_dispatch proc near
	cmp cl, 0
	jne api_1
	call FAR PTR _install_driver
	xor ah, ah
	cmp al, 0ffh
	jne api_0_answer
	mov ah, al
api_0_answer:
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
api_1:
	cmp cl, 1
	jne api_2
	mov bx, word ptr cs:api_arg0
	call FAR PTR _configure_driver
	ret
api_2:
	cmp cl, 2
	jne api_3
	call FAR PTR _silence_driver
	ret
api_3:
	cmp cl, 3
	jne api_4
	mov cx, word ptr cs:api_arg0
	call FAR PTR _sound_hold
	ret
api_4:
	cmp cl, 4
	jne api_5
	mov cx, word ptr cs:api_arg0
	call FAR PTR _driver_fn13
	ret
api_5:
	cmp cl, 5
	jne api_6
	call FAR PTR _seek_sequence
	ret
api_6:
	cmp cl, 6
	jne api_7
	mov cx, word ptr cs:api_arg0
	call FAR PTR _set_master_level
	ret
api_7:
	cmp cl, 7
	jne api_8
	mov cx, word ptr cs:api_arg0
	call FAR PTR _set_sequence_level
	ret
api_8:
	cmp cl, 8
	jne api_9
	mov cx, word ptr cs:api_arg0
	call FAR PTR _start_sequence
	ret
api_9:
	cmp cl, 9
	jne api_0a
	call FAR PTR _retire_and_tick
	ret
api_0a:
	cmp cl, 0ah
	jne api_0b
	mov cl, byte ptr cs:api_arg0
	call FAR PTR sound_fn0a
	ret
api_0b:
	cmp cl, 0bh
	jne api_0c
	mov dl, byte ptr cs:api_arg0
	mov cl, byte ptr cs:api_arg1
	mov ch, byte ptr cs:api_arg2
	call FAR PTR sound_fn0b
	ret
api_0c:
	cmp cl, 0ch
	jne api_0d
	mov cl, byte ptr cs:api_arg0
	call FAR PTR sound_fn0c
	ret
api_0d:
	cmp cl, 0dh
	jne api_0e
	mov cx, word ptr cs:api_arg0
	call FAR PTR sound_fn0d
	ret
api_0e:
	cmp cl, 0eh
	jne api_0f
	mov cx, word ptr cs:api_arg0
	call FAR PTR sound_fn0e
	ret
api_0f:
	cmp cl, 0fh
	jne api_11
	mov cx, word ptr cs:api_arg0
	call FAR PTR sound_fn0f
api_11:
	cmp cl, 11h
	jne api_10
	call FAR PTR sound_fn11
	mov ax, cx
api_10:
	cmp cl, 10h
	jne api_19
	call FAR PTR sound_fn10
	mov al, cl
	xor ah, ah
	ret
api_19:
	cmp cl, 19h
	jne api_12
	call FAR PTR sound_fn19
	mov al, cl
	xor ah, ah
	ret
api_12:
	cmp cl, 12h
	jne api_13
	call FAR PTR sound_fn12
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
api_13:
	cmp cl, 13h
	jne api_14
	mov dl, byte ptr cs:api_arg0
	mov ch, byte ptr cs:api_arg1
	mov cl, byte ptr cs:api_arg2
	call FAR PTR sound_fn13
	ret
api_14:
	cmp cl, 14h
	jne api_15
	mov dl, byte ptr cs:api_arg0
	mov ch, byte ptr cs:api_arg1
	mov cl, byte ptr cs:api_arg2
	call FAR PTR sound_fn14
	ret
api_15:
	cmp cl, 15h
	jne api_16
	mov dl, byte ptr cs:api_arg0
	mov ch, byte ptr cs:api_arg1
	mov cl, byte ptr cs:api_arg2
	call FAR PTR sound_fn15
	ret
api_16:
	cmp cl, 16h
	jne api_17
	mov dl, byte ptr cs:api_arg0
	mov cl, byte ptr cs:api_arg1
	call FAR PTR sound_fn16
	ret
api_17:
	cmp cl, 17h
	jne api_none
	mov dl, byte ptr cs:api_arg0
	mov cx, word ptr cs:api_arg1
	call FAR PTR sound_fn17
	ret
api_none:
	ret
_sound_api_dispatch endp

; 0x265f2
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
	cmp word ptr DGROUP:_g_sound_bank+bank_module_live, 0
	je install_store_ah
	or dl, 1
install_store_ah:
	mov byte ptr cs:ah_high, dl
	retf
_install_driver endp

; 0x26629
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

; 0x2664e
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

; 0x2666d
_sound_hold proc far
	cmp cx, 0
	jne hold_release
	inc byte ptr cs:busy
	retf
hold_release:
	cmp byte ptr cs:busy, 0
	je hold_return
	dec byte ptr cs:busy
hold_return:
	retf
_sound_hold endp

; 0x26686
_driver_fn13 proc far
	push bp
	mov bp, 0dh
	call dword ptr cs:driver_off
	pop bp
	retf
_driver_fn13 endp

; 0x26691
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
	mov cl, es:[bx+seq_mode]
	dec cl
	mov si, 0eh
seek_save_channel:
	mov dl, es:[bx+si+seq_ch+chan_no_voice]
	and dl, 0f0h
	mov byte ptr cs:seek_saved_bits[si], dl
	dec si
	jns seek_save_channel
	mov byte ptr cs:muted, 1
	call FAR PTR _start_sequence
	mov cx, es:[bx+seq_ticks]
	mov word ptr es:[bx+seq_ticks], 0
	mov al, es:[bx+seq_looping]
	mov byte ptr es:[bx+seq_looping], 1
	cmp cx, 0
	je seek_done
seek_step:
	mov dx, es:[bx+seq_ticks]
	call _step_sequence
	cmp dx, es:[bx+seq_ticks]
	jb seek_compare
	je seek_done
	sub dx, es:[bx+seq_ticks]
	sub cx, dx
seek_compare:
	cmp cx, es:[bx+seq_ticks]
	jne seek_step
seek_done:
	mov es:[bx+seq_looping], al
	mov byte ptr cs:muted, 0
	mov si, 0eh
seek_restore_channel:
	mov dl, byte ptr cs:seek_saved_bits[si]
	or es:[bx+si+seq_ch+chan_no_voice], dl
	dec si
	jns seek_restore_channel
	call _sequencer_tick
	pop dx
	pop cx
	pop bx
	pop ax
	pop si
	popf
	retf
_seek_sequence endp

; 0x26721
_set_master_level proc far
	cmp cl, 0ffh
	je master_level_set
	cmp cl, 0fh
	jbe master_level_set
	mov cl, 0fh
master_level_set:
	push bp
	mov bp, 0ch
	call dword ptr cs:driver_off
	pop bp
	retf
_set_master_level endp

; 0x26738
_set_sequence_level proc far
	cmp cl, 0ffh
	jne seq_level_set
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
	retf
seq_level_set:
	cmp cl, 0ah
	jbe seq_level_store
	mov al, byte ptr cs:param_default
	xor ah, ah
	retf
seq_level_store:
	mov al, byte ptr cs:param_default
	push es
	push ax
	push bx
	push dx
	mov byte ptr cs:param_default, cl
	les bx, dword ptr cs:playing_0_off
	mov dx, es
	or dx, bx
	je seq_level_return
	cmp byte ptr es:[bx+seq_device_value], 7fh
	jne seq_level_return
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
seq_level_return:
	pop dx
	pop bx
	pop ax
	pop es
	retf
_set_sequence_level endp

; 0x26783
_start_sequence proc far
	pushf
	cli
	push cx
	push cx
	mov cx, es
	xor di, di
start_find_playing:
	cmp word ptr cs:playing_0_off[di], ax
	jne start_find_next
	cmp word ptr cs:playing_0_seg[di], cx
	jne start_find_next
	call _remove_sequence
	call _sequencer_tick
	jmp short start_reset
start_find_next:
	add di, 4
	cmp di, 40h
	jne start_find_playing
start_reset:
	pop cx
	mov bx, ax
	mov byte ptr es:[bx+seq_mode], 1
	cmp cx, 0
	je start_params
	inc byte ptr es:[bx+seq_mode]
start_params:
	call _init_sequence_params
	mov bx, ax
	xor si, si
	xor cx, cx
	dec ch
start_channel_defaults:
	shl si, 1
	mov word ptr es:[bx+si+seq_position], 0dh
	mov word ptr es:[bx+si+seq_position_saved], 3
	mov word ptr es:[bx+si+seq_delay], 0
	mov word ptr es:[bx+si+seq_delay_saved], 0
	mov word ptr es:[bx+si+seq_ch+chan_bend], 2000h
	shr si, 1
	mov es:[bx+si+seq_track_channel], ch
	mov es:[bx+si+seq_status], cl
	mov es:[bx+si+seq_status_saved], cl
	mov es:[bx+si+seq_ch+chan_voice_budget], ch
	mov es:[bx+si+seq_ch+chan_modulation], cl
	mov es:[bx+si+seq_ch+chan_program], ch
	mov es:[bx+si+seq_ch+chan_volume], ch
	mov es:[bx+si+seq_ch+chan_pan], ch
	mov es:[bx+si+seq_ch+chan_note], ch
	mov es:[bx+si+seq_ch+chan_channel_flags], cl
	mov es:[bx+si+seq_ch+chan_no_voice], cl
	inc si
	cmp si, 0fh
	jne start_channel_defaults
	mov es:[bx+si+seq_track_channel], ch
	mov es:[bx+si+seq_status], cl
	mov es:[bx+si+seq_status_saved], cl
	mov es:[bx+seq_poll], cl
	mov es:[bx+seq_rewind_mark], cl
	mov byte ptr es:[bx+seq_device_value], 7fh
	shl si, 1
	mov word ptr es:[bx+si+seq_position], 0dh
	mov word ptr es:[bx+si+seq_position_saved], 3
	mov word ptr es:[bx+si+seq_delay], 0
	mov word ptr es:[bx+seq_ticks_saved], 0
	push ax
	push bp
	push ds
	xor si, si
	lds bp, dword ptr es:[bx+seq_cursor_at]
	lds bp, dword ptr ds:[bp]
	cmp byte ptr ds:[bp+20h], 0ffh
	je start_tracks
	cmp byte ptr es:[bx+seq_keep_priority], 0
	jne start_tracks
	mov cl, ds:[bp+20h]
	mov es:[bx+seq_priority], cl
start_tracks:
	mov cx, bp
start_track:
	mov dx, ds:[bp]
	cmp dx, 0
	je start_tracks_end
	mov bp, cx
	add bp, dx
	mov dl, ds:[bp]
	cmp dl, 0feh
	jne start_track_channel
	cmp byte ptr cs:ah_high, 0
	jne start_track_end_flag
	shl si, 1
	mov word ptr es:[bx+si+seq_position], 0
	mov word ptr es:[bx+si+seq_position_saved], 0
	shr si, 1
	mov byte ptr es:[bx+si+seq_track_channel], 0feh
	jmp start_track_next
start_track_end_flag:
	mov dx, si
	inc dx
	mov es:[bx+seq_poll], dl
start_tracks_end:
	jmp start_tracks_done
start_track_channel:
	mov es:[bx+si+seq_track_channel], dl
	mov dh, dl
	or dh, 0b0h
	mov es:[bx+si+seq_status], dh
	mov dl, ds:[bp+0ch]
	xor dh, dh
	cmp dl, 0f8h
	jne start_track_header
	mov dl, 0f0h
	mov dh, 80h
start_track_header:
	shl si, 1
	mov es:[bx+si+seq_delay], dx
	shr si, 1
	push si
	mov dl, es:[bx+si+seq_track_channel]
	and byte ptr es:[bx+si+seq_track_channel], 0fh
	test dl, 10h
	je start_track_flags
	shl si, 1
	mov word ptr es:[bx+si+seq_position], 3
	mov word ptr es:[bx+si+seq_delay], 0
	xor dh, dh
	mov si, dx
	and si, 0fh
	or byte ptr es:[bx+si+seq_ch+chan_channel_flags], 2
	jmp short start_track_next_pop
start_track_flags:
	xor dh, dh
	mov si, dx
	and si, 0fh
	test dl, 20h
	je start_track_not_own
	or byte ptr es:[bx+si+seq_ch+chan_channel_flags], 1
start_track_not_own:
	test dl, 40h
	je start_track_defaults
	mov byte ptr es:[bx+si+seq_ch+chan_no_voice], 1
start_track_defaults:
	cmp si, 0fh
	jne start_default_da
	cmp byte ptr es:[bx+seq_device_value], 7fh
	jne start_default_pan
	mov al, ds:[bp+8]
	mov es:[bx+seq_device_value], al
	jmp short start_track_next_pop
start_default_da:
	cmp byte ptr es:[bx+si+seq_ch+chan_voice_budget], 0ffh
	jne start_default_116
	mov al, ds:[bp+1]
	mov es:[bx+si+seq_ch+chan_voice_budget], al
start_default_116:
	cmp byte ptr es:[bx+si+seq_ch+chan_program], 0ffh
	jne start_default_volume
	mov al, ds:[bp+4]
	mov es:[bx+si+seq_ch+chan_program], al
start_default_volume:
	cmp byte ptr es:[bx+si+seq_ch+chan_volume], 0ffh
	jne start_default_pan
	mov al, ds:[bp+8]
	mov es:[bx+si+seq_ch+chan_volume], al
start_default_pan:
	cmp byte ptr es:[bx+si+seq_ch+chan_pan], 0ffh
	jne start_track_next_pop
	mov al, ds:[bp+0bh]
	mov es:[bx+si+seq_ch+chan_pan], al
start_track_next_pop:
	pop si
start_track_next:
	inc si
	shl si, 1
	mov bp, cx
	add bp, si
	shr si, 1
	cmp si, 10h
	je start_tracks_done
	jmp start_track
start_tracks_done:
	pop ds
	pop bp
	pop ax
	cmp byte ptr es:[bx+seq_mode], 2
	jne start_insert
	mov di, 0eh
start_all_own:
	or byte ptr es:[bx+di+seq_ch+chan_channel_flags], 1
	dec di
	jns start_all_own
start_insert:
	mov ax, bx
	mov dl, es:[bx+seq_priority]
	push es
	xor di, di
start_find_place:
	cmp word ptr cs:playing_0_seg[di], 0
	je start_place
	les bx, dword ptr cs:playing_0_off[di]
	cmp es:[bx+seq_priority], dl
	jbe start_make_room
	add di, 4
	cmp di, 40h
	jne start_find_place
	pop es
	jmp short start_return
start_make_room:
	mov si, 38h
start_shift:
	mov bx, si
	add bx, 4
	cmp bx, di
	je start_place
	mov bx, word ptr cs:playing_0_off[si]
	mov word ptr cs:playing_1_off[si], bx
	mov bx, word ptr cs:playing_0_seg[si]
	mov word ptr cs:playing_1_seg[si], bx
	sub si, 4
	jmp short start_shift
start_place:
	pop es
	mov bx, ax
	mov ax, es
	mov word ptr cs:playing_0_off[di], bx
	mov word ptr cs:playing_0_seg[di], ax
	cmp byte ptr cs:muted, 0
	jne start_return
	xor cx, cx
	mov es:[bx+seq_loop_count], cx
	mov es:[bx+seq_ticks], cx
	mov es:[bx+seq_state], cl
	mov es:[bx+seq_fade_target], cl
	mov es:[bx+seq_fade_period], cl
	mov es:[bx+seq_fade_countdown], cl
	mov es:[bx+seq_fade_step], cl
	mov es:[bx+seq_skip], cl
	call _sequencer_tick
start_return:
	pop cx
	popf
	retf
_start_sequence endp

; 0x26a57
_retire_and_tick proc far
	pushf
	cli
	call _remove_sequence
	call _sequencer_tick
	popf
	retf
sound_fn0a:
	pushf
	cli
	push ax
	push bx
	push si
	mov bx, es
	or bx, ax
	jne fn0a_one
	xor si, si
fn0a_scan:
	les bx, dword ptr cs:playing_0_off[si]
	mov ax, es
	or ax, bx
	jne fn0a_each
	cmp si, 0
	jne fn0a_tick
	mov si, 4
	jmp short fn0a_scan
fn0a_each:
	mov al, es:[bx+seq_skip]
	cmp cl, 0
	je fn0a_each_down
	inc al
	jmp short fn0a_each_store
fn0a_each_down:
	cmp al, 0
	je fn0a_each_store
	dec al
fn0a_each_store:
	mov es:[bx+seq_skip], al
	add si, 4
	cmp si, 40h
	jne fn0a_scan
	jmp short fn0a_tick
fn0a_one:
	mov bx, ax
	mov al, es:[bx+seq_skip]
	cmp cl, 0
	je fn0a_one_down
	inc al
	jmp short fn0a_one_store
fn0a_one_down:
	cmp al, 0
	je fn0a_one_store
	dec al
fn0a_one_store:
	mov es:[bx+seq_skip], al
fn0a_tick:
	call _sequencer_tick
	pop si
	pop bx
	pop ax
	popf
	retf
sound_fn0b:
	pushf
	cli
	push bx
	mov bx, ax
	cmp es:[bx+seq_volume], dl
	je fn0b_return
	mov es:[bx+seq_fade_target], dl
	mov es:[bx+seq_fade_period], cl
	mov es:[bx+seq_fade_step], ch
	mov byte ptr es:[bx+seq_fade_countdown], 0
fn0b_return:
	pop bx
	popf
	retf
sound_fn0c:
	push bx
	mov bx, ax
	mov es:[bx+seq_rewind_mark], cl
	pop bx
	retf
sound_fn0d:
	push bx
	push dx
	push si
	inc byte ptr cs:busy
	mov bx, ax
	mov si, 0eh
fn0d_channel:
	mov dl, es:[bx+si+seq_ch+chan_no_voice]
	cmp cx, 0
	jne fn0d_up
	cmp dl, 0fh
	jbe fn0d_store
	sub dl, 10h
	jmp short fn0d_store
fn0d_up:
	cmp dl, 0f0h
	jae fn0d_store
	add dl, 10h
fn0d_store:
	mov es:[bx+si+seq_ch+chan_no_voice], dl
	dec si
	jns fn0d_channel
	call _sequencer_tick
	dec byte ptr cs:busy
	pop si
	pop dx
	pop bx
	retf
sound_fn0e:
	pushf
	cli
	push bx
	mov bx, ax
	call find_playing_slot
	xor ch, ch
	call _set_sequence_volume
	pop bx
	popf
	retf
sound_fn0f:
	pushf
	cli
	push bx
	push cx
	push dx
	push si
	push di
	mov bx, ax
	cmp es:[bx+seq_priority], cl
	jne fn0f_changed
	jmp fn0f_return
fn0f_changed:
	mov es:[bx+seq_priority], cl
	call find_playing_slot
	cmp si, 0ffh
	jne fn0f_playing
	jmp fn0f_return
fn0f_playing:
	mov word ptr cs:playing_0_off[si], 0
	mov word ptr cs:playing_0_seg[si], 0
	cmp si, 3ch
	je fn0f_reinsert
fn0f_close_gap:
	mov cx, word ptr cs:playing_1_off[si]
	mov word ptr cs:playing_0_off[si], cx
	mov cx, word ptr cs:playing_1_seg[si]
	mov word ptr cs:playing_0_seg[si], cx
	add si, 4
	cmp si, 3ch
	jne fn0f_close_gap
	mov word ptr cs:playing_0_off[si], 0
	mov word ptr cs:playing_0_seg[si], 0
fn0f_reinsert:
	mov dl, es:[bx+seq_priority]
	push es
	xor di, di
fn0f_find_place:
	cmp word ptr cs:playing_0_seg[di], 0
	je fn0f_place
	les bx, dword ptr cs:playing_0_off[di]
	mov es:[bx+seq_priority], dl
	jbe fn0f_make_room
	add di, 4
	jmp short fn0f_find_place
fn0f_make_room:
	mov si, 38h
fn0f_shift:
	mov bx, si
	add bx, 4
	cmp bx, di
	je fn0f_place
	mov bx, word ptr cs:playing_0_off[si]
	mov word ptr cs:playing_1_off[si], bx
	mov bx, word ptr cs:playing_0_seg[si]
	mov word ptr cs:playing_1_seg[si], bx
	sub si, 4
	jmp short fn0f_shift
fn0f_place:
	pop es
	mov word ptr cs:playing_0_off[di], ax
	mov cx, es
	mov word ptr cs:playing_0_seg[di], cx
	call _sequencer_tick
fn0f_return:
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	popf
	retf
sound_fn10:
	push bx
	mov bx, ax
	mov cl, es:[bx+seq_state]
	pop bx
	retf
sound_fn19:
	push bx
	mov bx, ax
	xor cl, cl
	xchg es:[bx+seq_state], cl
	pop bx
	retf
sound_fn11:
	push bx
	mov bx, ax
	mov cx, es:[bx+seq_loop_count]
	pop bx
	retf
sound_fn12:
	push ax
	push bx
	mov bx, ax
	mov ax, es:[bx+seq_ticks]
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
sound_fn13:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:busy
	mov bx, ax
	call find_playing_slot
	cmp si, 0ffh
	je fn13_return
	push si
	xor dh, dh
	mov si, dx
	mov byte ptr es:[bx+si+seq_ch+chan_note], 0ffh
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
	jmp short fn13_voice
fn13_pad_90 label byte
	db 90h
fn13_next_voice:
	inc si
	cmp si, 10h
	je fn13_return
fn13_voice:
	cmp byte ptr cs:voice_held[si], dl
	jne fn13_next_voice
	mov ax, si
	push bp
	mov bp, 4
	call dword ptr cs:driver_off
	pop bp
fn13_return:
	dec byte ptr cs:busy
	pop dx
	pop bx
	pop ax
	pop si
	retf
sound_fn14:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:busy
	mov bx, ax
	call find_playing_slot
	cmp si, 0ffh
	je fn14_return
	push si
	xor dh, dh
	mov si, dx
	mov es:[bx+si+seq_ch+chan_note], ch
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
	jmp short fn14_voice
fn14_pad_90 label byte
	db 90h
fn14_next_voice:
	inc si
	cmp si, 10h
	je fn14_return
fn14_voice:
	cmp byte ptr cs:voice_held[si], dl
	jne fn14_next_voice
	mov ax, si
	push bp
	mov bp, 5
	call dword ptr cs:driver_off
	pop bp
fn14_return:
	dec byte ptr cs:busy
	pop dx
	pop bx
	pop ax
	pop si
	retf
sound_fn15:
	pushf
	cli
	push ax
	push bx
	push cx
	push dx
	push si
	mov bx, ax
	call find_playing_slot
	cmp si, 0ffh
	jne fn15_found
	jmp fn15_return
fn15_found:
	push si
	xor dh, dh
	mov si, dx
	cmp ch, 7
	jne fn15_ctl_0a
	mov es:[bx+si+seq_ch+chan_volume], cl
	mov dl, es:[bx+seq_volume]
	call _scale_byte_pair
	jmp short fn15_voices
fn15_ctl_0a:
	cmp ch, 0ah
	jne fn15_ctl_01
	mov es:[bx+si+seq_ch+chan_pan], cl
	jmp short fn15_voices
fn15_ctl_01:
	cmp ch, 1
	jne fn15_ctl_40
	mov es:[bx+si+seq_ch+chan_modulation], cl
	jmp short fn15_voices
fn15_ctl_40:
	cmp ch, 40h
	jne fn15_ctl_4e
	shl si, 1
	mov ax, es:[bx+si+seq_ch+chan_bend]
	and ah, 7fh
	cmp cl, 0
	je fn15_sustain_set
	or ah, 80h
fn15_sustain_set:
	mov es:[bx+si+seq_ch+chan_bend], ax
	shr si, 1
	jmp short fn15_voices
fn15_ctl_4e:
	cmp ch, 4eh
	jne fn15_ctl_7f
	push dx
	mov dl, es:[bx+si+seq_ch+chan_no_voice]
	cmp cl, 0
	jne fn15_4e_up
	cmp dl, 0fh
	jbe fn15_4e_done
	sub dl, 10h
	jmp short fn15_4e_store
fn15_4e_up:
	cmp dl, 0f0h
	jae fn15_4e_done
	add dl, 10h
fn15_4e_store:
	mov es:[bx+si+seq_ch+chan_no_voice], dl
	call _sequencer_tick
fn15_4e_done:
	pop dx
	pop si
	jmp short fn15_return
fn15_ctl_7f:
	cmp ch, 7fh
	jne fn15_voices
	mov es:[bx+si+seq_ch+chan_program], cl
fn15_voices:
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
fn15_find_voice:
	cmp byte ptr cs:voice_held[si], dl
	je fn15_voice
	inc si
	cmp si, 10h
	jne fn15_find_voice
	jmp short fn15_return
fn15_voice:
	mov ax, si
	cmp ch, 7fh
	jne fn15_controller
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
	jmp short fn15_return
fn15_controller:
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
fn15_return:
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	popf
	retf
sound_fn16:
	push ax
	push bx
	push dx
	push si
	inc byte ptr cs:busy
	mov bx, ax
	call find_playing_slot
	cmp si, 0ffh
	je fn16_return
	push si
	xor dh, dh
	mov si, dx
	mov es:[bx+si+seq_ch+chan_program], cl
	mov ax, si
	pop si
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, al
	xor si, si
	jmp short fn16_voice
fn16_pad_90 label byte
	db 90h
fn16_next_voice:
	inc si
	cmp si, 10h
	je fn16_return
fn16_voice:
	cmp byte ptr cs:voice_held[si], dl
	jne fn16_next_voice
	mov ax, si
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
fn16_return:
	dec byte ptr cs:busy
	pop dx
	pop bx
	pop ax
	pop si
	retf
sound_fn17:
	push ax
	push bx
	push cx
	push dx
	push si
	mov bx, ax
	call find_playing_slot
	cmp si, 0ffh
	je fn17_return
	push si
	xor dh, dh
	mov si, dx
	shl si, 1
	mov ax, cx
	cmp byte ptr es:[bx+si+seq_ch+chan_bend+1], 80h
	jb fn17_store
	or ah, 80h
fn17_store:
	mov es:[bx+si+seq_ch+chan_bend], ax
	shr si, 1
	mov dx, si
	pop si
	shl si, 1
	shl si, 1
	or dx, si
	xor si, si
fn17_find_voice:
	cmp byte ptr cs:voice_held[si], dl
	je fn17_voice
	inc si
	cmp si, 10h
	jne fn17_find_voice
	jmp short fn17_return
fn17_voice:
	shl ch, 1
	cmp cl, 80h
	jb fn17_bend_split
	or ch, 1
fn17_bend_split:
	and cl, 7fh
	xchg ch, cl
	mov ax, si
	push bp
	mov bp, 0ah
	call dword ptr cs:driver_off
	pop bp
fn17_return:
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	retf
_retire_and_tick endp

; 0x26e7b
_remove_sequence proc near
	push si
	push es
	push ax
	push bx
	push ds
	push bp
	xor si, si
	mov cx, es
remove_find:
	cmp ax, word ptr cs:playing_0_off[si]
	jne remove_find_next
	cmp cx, word ptr cs:playing_0_seg[si]
	je remove_found
remove_find_next:
	add si, 4
	cmp si, 40h
	jne remove_find
	jmp remove_return
remove_found:
	mov word ptr cs:playing_0_off[si], 0
	mov word ptr cs:playing_0_seg[si], 0
	cmp si, 3ch
	je remove_retire
remove_close_gap:
	mov cx, word ptr cs:playing_1_off[si]
	mov word ptr cs:playing_0_off[si], cx
	mov cx, word ptr cs:playing_1_seg[si]
	mov word ptr cs:playing_0_seg[si], cx
	add si, 4
	cmp si, 3ch
	jne remove_close_gap
	mov word ptr cs:playing_0_off[si], 0
	mov word ptr cs:playing_0_seg[si], 0
remove_retire:
	mov bx, ax
	mov byte ptr es:[bx+seq_state], 0ffh
	mov byte ptr es:[bx+seq_mode], 0
	cmp byte ptr es:[bx+seq_poll], 0
	je remove_return
	lds bp, dword ptr es:[bx+seq_cursor_at]
	lds bp, dword ptr ds:[bp]
	mov al, es:[bx+seq_poll]
	cmp al, 80h
	jb remove_return
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
remove_return:
	pop bp
	pop ds
	pop bx
	pop ax
	pop es
	pop si
	ret
_remove_sequence endp

; 0x26f2a
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
	jne tick_level
	mov dx, 0ffffh
	mov word ptr cs:voice_held, dx
	mov word ptr cs:voice_held_2, dx
	mov word ptr cs:voice_held_4, dx
	mov word ptr cs:voice_held_6, dx
	mov word ptr cs:voice_held_8, dx
	mov word ptr cs:voice_held_a, dx
	mov word ptr cs:voice_held_c, dx
	mov word ptr cs:voice_held_e, dx
	jmp tick_release
tick_level:
	mov cl, es:[bx+seq_device_value]
	cmp cl, 7fh
	jne tick_level_set
	mov cl, byte ptr cs:param_default
tick_level_set:
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
	xor bp, bp
	xor si, si
	mov al, byte ptr cs:want_cl
tick_sequence:
	les bx, dword ptr cs:playing_0_off[si]
	mov dx, es
	or dx, bx
	jne tick_sequence_live
	jmp tick_place
tick_sequence_live:
	les bx, dword ptr cs:playing_0_off[si]
	cmp byte ptr es:[bx+seq_skip], 0
	je tick_not_held
	jmp tick_sequence_next
tick_not_held:
	cmp byte ptr es:[bx+seq_poll], 0
	je tick_snapshot
	cmp word ptr cs:polled_0_off, 0
	je tick_poll_seg
	jmp tick_sequence_next
tick_poll_seg:
	cmp word ptr cs:polled_0_seg, 0
	je tick_park_poll
	jmp tick_sequence_next
tick_park_poll:
	mov word ptr cs:polled_0_off, bx
	mov bx, es
	mov word ptr cs:polled_0_seg, bx
	jmp tick_sequence_next
tick_snapshot:
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
tick_channel:
	mov cl, es:[bx+di+seq_track_channel]
	cmp cl, 0ffh
	jne tick_channel_not_ff
	jmp tick_channel_next
tick_channel_not_ff:
	cmp cl, 0feh
	jne tick_channel_not_fe
	jmp tick_channel_next
tick_channel_not_fe:
	cmp cl, 0fh
	jne tick_channel_live
	jmp tick_channel_next
tick_channel_live:
	push di
	mov di, cx
	and di, 0ffh
	test byte ptr es:[bx+di+seq_ch+chan_channel_flags], 2
	je tick_channel_not_own
	pop di
	jmp tick_channel_next
tick_channel_not_own:
	test byte ptr es:[bx+di+seq_ch+chan_no_voice], 0ffh
	pop di
	je tick_channel_request
	jmp tick_channel_next
tick_channel_request:
	mov dx, si
	shl dl, 1
	shl dl, 1
	or dl, cl
	push di
	mov di, cx
	and di, 0ffh
	mov ah, es:[bx+di+seq_ch+chan_voice_budget]
	and ah, 0fh
	mov ch, es:[bx+di+seq_ch+chan_voice_budget]
	pop di
	shr ch, 1
	shr ch, 1
	shr ch, 1
	shr ch, 1
	je tick_cost_set
	push dx
	mov dx, 10h
	sub dl, ch
	add dx, bp
	mov ch, dl
	pop dx
tick_cost_set:
	push di
	mov di, cx
	and di, 0ffh
	test byte ptr es:[bx+di+seq_ch+chan_channel_flags], 1
	je tick_find_voice
	cmp byte ptr cs:voice_request[di], 0ffh
	jne tick_find_voice
	pop di
	mov dh, cl
	jmp tick_budget
tick_find_voice:
	pop di
	mov dh, 0ffh
	push bx
	xor bx, bx
tick_scan_voice:
	cmp byte ptr cs:voice_request[bx], 0ffh
	je tick_voice_free
	cmp byte ptr cs:voice_request[bx], dl
	jne tick_scan_next
	pop bx
	jmp tick_channel_next
tick_voice_free:
	cmp bl, byte ptr cs:voice_lo
	jb tick_scan_next
	cmp bl, byte ptr cs:voice_hi
	ja tick_scan_next
	mov dh, bl
tick_scan_next:
	inc bl
	cmp bl, 10h
	jne tick_scan_voice
	pop bx
	cmp dh, 0ffh
	jne tick_budget
	cmp ch, 0
	je tick_steal
	jmp tick_sequence_next
tick_steal:
	push di
	push cx
	push ax
	mov dh, 0ffh
	xor ax, ax
	xor di, di
tick_steal_scan:
	cmp al, byte ptr cs:voice_cost[di]
	jae tick_steal_next
	mov al, byte ptr cs:voice_cost[di]
	mov cx, di
	mov dh, cl
tick_steal_next:
	inc di
	cmp di, 10h
	jne tick_steal_scan
	pop ax
	cmp dh, 0ffh
	je tick_steal_done
	xor cx, cx
	mov cl, dh
	mov di, cx
	add al, byte ptr cs:voice_gives_back[di]
	mov byte ptr cs:voice_request[di], 0ffh
	mov byte ptr cs:voice_gives_back[di], 0
	mov byte ptr cs:voice_cost[di], 0
	mov byte ptr cs:voice_keep_own[di], 0
tick_steal_done:
	pop cx
	pop di
	cmp dh, 0ffh
	jne tick_budget
	jmp tick_restore_snapshot
tick_budget:
	cmp ah, al
	jbe tick_assign
	cmp ch, 0
	je tick_free_more
	jmp tick_channel_next
tick_free_more:
	push di
	push cx
	push ax
	mov dh, 0ffh
	xor ax, ax
	xor di, di
tick_free_scan:
	cmp al, byte ptr cs:voice_cost[di]
	jae tick_free_next
	mov al, byte ptr cs:voice_cost[di]
	mov cx, di
	mov dh, cl
tick_free_next:
	inc di
	cmp di, 10h
	jne tick_free_scan
	pop ax
	cmp dh, 0ffh
	je tick_free_done
	xor cx, cx
	mov cl, dh
	mov di, cx
	add al, byte ptr cs:voice_gives_back[di]
	mov byte ptr cs:voice_request[di], 0ffh
	mov byte ptr cs:voice_gives_back[di], 0
	mov byte ptr cs:voice_cost[di], 0
	mov byte ptr cs:voice_keep_own[di], 0
tick_free_done:
	pop cx
	pop di
	cmp dh, 0ffh
	jne tick_free_enough
	jmp tick_restore_snapshot
tick_free_enough:
	cmp ah, al
	ja tick_free_more
tick_assign:
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
	test byte ptr es:[bx+di+seq_ch+chan_channel_flags], 1
	pop di
	jne tick_keep_own
	mov byte ptr cs:voice_keep_own[di], 0
	pop di
	jmp tick_channel_next
tick_keep_own:
	mov byte ptr cs:voice_keep_own[di], 1
	cmp dh, cl
	jne tick_own_slot
	pop di
	jmp tick_channel_next
tick_own_slot:
	push si
	mov si, cx
	and si, 0ffh
	cmp byte ptr cs:voice_keep_own[si], 0
	jne tick_own_taken
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
	jmp short tick_channel_next
tick_own_taken:
	cmp ch, 0
	je tick_own_taker_free
	mov byte ptr cs:voice_request[di], 0ffh
	mov byte ptr cs:voice_cost[di], 0
	mov byte ptr cs:voice_gives_back[di], 0
	mov byte ptr cs:voice_keep_own[di], 0
	add al, ah
	pop si
	pop di
	jmp short tick_channel_next
tick_own_taker_free:
	cmp byte ptr cs:voice_cost[si], 0
	jne tick_own_move
	pop si
	pop di
	jmp short tick_restore_snapshot
tick_own_move:
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
tick_channel_next:
	inc di
	cmp di, 10h
	jne tick_channel_loop
	jmp tick_sequence_next
tick_channel_loop:
	jmp tick_channel
tick_restore_snapshot:
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
tick_sequence_next:
	add bp, 10h
	add si, 4
	cmp si, 40h
	je tick_place
	jmp tick_sequence
tick_place:
	xor si, si
tick_place_voice:
	cmp byte ptr cs:voice_request[si], 0ffh
	jne tick_place_requested
	jmp tick_place_next
tick_place_requested:
	cmp byte ptr cs:voice_keep_own[si], 0
	jne tick_place_own
	jmp tick_place_other
tick_place_own:
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
	jne tick_program_own
	mov di, si
	shl di, 1
	shl di, 1
	cmp word ptr cs:voice_sequence_0_off[di], bx
	jne tick_program_own
	mov cx, es
	cmp word ptr cs:voice_sequence_0_seg[di], cx
	jne tick_program_own
	jmp tick_place_next
tick_program_own:
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
	mov cl, es:[bx+si+seq_ch+chan_voice_budget]
	and cl, 0fh
	mov ch, 4bh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+seq_ch+chan_program]
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
	push si
	mov si, ax
	mov byte ptr cs:pending_volume[si], 0ffh
	pop si
	mov cl, es:[bx+si+seq_ch+chan_volume]
	mov dl, es:[bx+seq_volume]
	call _scale_byte_pair
	mov ch, 7
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov ch, 0ah
	mov cl, es:[bx+si+seq_ch+chan_pan]
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov ch, 1
	mov cl, es:[bx+si+seq_ch+chan_modulation]
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	shl si, 1
	mov cx, 4000h
	cmp byte ptr es:[bx+si+seq_ch+chan_bend+1], 80h
	jb tick_own_sustain_set
	mov cl, 7fh
tick_own_sustain_set:
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cx, es:[bx+si+seq_ch+chan_bend]
	shr si, 1
	xchg cl, ch
	shl cl, 1
	cmp ch, 80h
	jb tick_own_bend_split
	or cl, 1
tick_own_bend_split:
	and cx, 7f7fh
	push bp
	mov bp, 0ah
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+seq_ch+chan_note]
	mov ch, 4eh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	pop si
	pop dx
	pop ax
	jmp short tick_place_next
tick_place_other:
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
tick_find_same:
	cmp word ptr cs:voice_sequence_0_off[di], bx
	jne tick_find_same_next
	mov cx, es
	cmp word ptr cs:voice_sequence_0_seg[di], cx
	jne tick_find_same_next
	shr di, 1
	shr di, 1
	cmp byte ptr cs:voice_channel[di], al
	je tick_same_found
	shl di, 1
	shl di, 1
tick_find_same_next:
	add di, 4
	mov cx, di
	shr cl, 1
	shr cl, 1
	dec cl
	cmp byte ptr cs:voice_hi, cl
	jne tick_find_same
	jmp short tick_place_next
tick_same_found:
	cmp byte ptr cs:voice_keep_own[di], 0
	jne tick_place_next
	mov cl, byte ptr cs:voice_request[si]
	mov byte ptr cs:voice_held[di], cl
	mov byte ptr cs:voice_request[si], 0ffh
tick_place_next:
	inc si
	cmp si, 10h
	je tick_fill
	jmp tick_place_voice
tick_fill:
	mov al, byte ptr cs:voice_hi
	inc al
	xor ah, ah
	mov di, ax
	xor al, al
	xor si, si
tick_fill_voice:
	cmp byte ptr cs:voice_request[si], 0ffh
	jne tick_fill_find
	jmp tick_fill_next
tick_fill_find:
	mov bx, di
tick_fill_find_free:
	dec bx
	cmp byte ptr cs:voice_held[bx], 0ffh
	jne tick_fill_find_free
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
	mov cl, es:[bx+si+seq_ch+chan_voice_budget]
	and cl, 0fh
	mov ch, 4bh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+seq_ch+chan_program]
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
	push si
	mov si, ax
	mov byte ptr cs:pending_volume[si], 0ffh
	pop si
	mov cl, es:[bx+si+seq_ch+chan_volume]
	mov dl, es:[bx+seq_volume]
	call _scale_byte_pair
	mov ch, 7
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov ch, 0ah
	mov cl, es:[bx+si+seq_ch+chan_pan]
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov ch, 1
	mov cl, es:[bx+si+seq_ch+chan_modulation]
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	shl si, 1
	mov cx, 4000h
	cmp byte ptr es:[bx+si+seq_ch+chan_bend+1], 80h
	jb tick_fill_sustain_set
	mov cl, 7fh
tick_fill_sustain_set:
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	mov cx, es:[bx+si+seq_ch+chan_bend]
	shr si, 1
	xchg cl, ch
	shl cl, 1
	cmp ch, 80h
	jb tick_fill_bend_split
	or cl, 1
tick_fill_bend_split:
	and cx, 7f7fh
	push bp
	mov bp, 0ah
	call dword ptr cs:driver_off
	pop bp
	mov cl, es:[bx+si+seq_ch+chan_note]
	mov ch, 4eh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	pop si
	pop dx
	pop ax
	pop si
tick_fill_next:
	inc si
	cmp si, 10h
	je tick_release
	jmp tick_fill_voice
tick_release:
	mov si, 0fh
tick_release_voice:
	cmp byte ptr cs:voice_channel[si], 0fh
	je tick_release_next
	cmp byte ptr cs:voice_held[si], 0ffh
	jne tick_release_next
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
tick_release_next:
	dec si
	jns tick_release_voice
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
tick_map_voice:
	mov bl, byte ptr cs:voice_held[si]
	cmp bl, 0ffh
	jne tick_map_held
	mov word ptr cs:voice_sequence_0_off[di], 0
	mov word ptr cs:voice_sequence_0_seg[di], 0
	jmp short tick_map_next
tick_map_held:
	and bx, 0f0h
	shr bx, 1
	shr bx, 1
	mov ax, word ptr cs:playing_0_off[bx]
	mov word ptr cs:voice_sequence_0_off[di], ax
	mov ax, word ptr cs:playing_0_seg[bx]
	mov word ptr cs:voice_sequence_0_seg[di], ax
tick_map_next:
	add di, 4
	inc si
	cmp si, 10h
	jne tick_map_voice
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

; 0x278e9
_advance_volume_ramp proc near
	push bx
	push cx
	cmp byte ptr es:[bx+seq_fade_countdown], 0
	je ramp_step
	dec byte ptr es:[bx+seq_fade_countdown]
	jmp ramp_return
ramp_step:
	mov cl, es:[bx+seq_fade_period]
	mov es:[bx+seq_fade_countdown], cl
	mov cl, es:[bx+seq_fade_target]
	and cl, 7fh
	cmp cl, es:[bx+seq_volume]
	je ramp_reached
	ja ramp_up
	mov cl, es:[bx+seq_volume]
	mov ch, es:[bx+seq_fade_target]
	and ch, 7fh
	sub cl, ch
	cmp cl, es:[bx+seq_fade_step]
	ja ramp_down_step
	mov cl, es:[bx+seq_fade_target]
	and cl, 7fh
	mov ch, 1
	call _set_sequence_volume
	jmp short ramp_reached
ramp_down_step:
	mov cl, es:[bx+seq_volume]
	sub cl, es:[bx+seq_fade_step]
	mov ch, 1
	call _set_sequence_volume
	jmp short ramp_return
ramp_up:
	mov cl, es:[bx+seq_fade_target]
	and cl, 7fh
	mov ch, es:[bx+seq_volume]
	sub cl, ch
	cmp cl, es:[bx+seq_fade_step]
	ja ramp_up_step
	mov cl, es:[bx+seq_fade_target]
	and cl, 7fh
	mov ch, 1
	call _set_sequence_volume
	jmp short ramp_reached
ramp_up_step:
	mov cl, es:[bx+seq_volume]
	add cl, es:[bx+seq_fade_step]
	mov ch, 1
	call _set_sequence_volume
	jmp short ramp_return
ramp_reached:
	mov byte ptr es:[bx+seq_state], 0feh
	mov byte ptr es:[bx+seq_fade_step], 0
	mov cl, es:[bx+seq_fade_target]
	and cl, 80h
	cmp cl, 0
	je ramp_return
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:voices_changed, 1
ramp_return:
	pop cx
	pop bx
	ret
_advance_volume_ramp endp

; 0x279a9
_set_sequence_volume proc near
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	mov byte ptr cs:defer, ch
	cmp cl, es:[bx+seq_volume]
	jne volume_changed
	jmp volume_return
volume_changed:
	mov es:[bx+seq_volume], cl
	cmp si, 0ffh
	jne volume_voices
	jmp volume_return
volume_voices:
	mov dx, si
	shl dl, 1
	shl dl, 1
	xor si, si
volume_voice:
	mov cl, byte ptr cs:voice_held[si]
	cmp cl, 0ffh
	je volume_voice_next
	mov ch, cl
	and cl, 0f0h
	cmp cl, dl
	jne volume_voice_next
	mov cl, ch
	and cx, 0fh
	mov di, cx
	mov cl, es:[bx+di+seq_ch+chan_volume]
	push dx
	mov dl, es:[bx+seq_volume]
	call _scale_byte_pair
	cmp byte ptr cs:defer, 0
	je volume_now
	mov byte ptr cs:pending_volume[si], cl
	jmp short volume_voice_done
volume_now:
	mov ch, 7
	mov ax, si
	mov byte ptr cs:pending_volume[si], 0ffh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
volume_voice_done:
	pop dx
volume_voice_next:
	inc si
	cmp si, 10h
	jne volume_voice
	xor ch, ch
	xor si, si
volume_own_channel:
	mov cl, es:[bx+si+seq_track_channel]
	cmp cl, 0ffh
	je volume_return
	mov di, cx
	test byte ptr es:[bx+di+seq_ch+chan_channel_flags], 2
	je volume_own_next
	cmp byte ptr cs:voice_held[di], 0ffh
	jne volume_own_next
	mov al, cl
	mov cl, es:[bx+di+seq_ch+chan_volume]
	mov dl, es:[bx+seq_volume]
	call _scale_byte_pair
	cmp byte ptr cs:defer, 0
	je volume_own_now
	mov byte ptr cs:pending_volume[di], cl
	jmp short volume_own_next
volume_own_now:
	mov ch, 7
	mov ax, di
	mov byte ptr cs:pending_volume[di], 0ffh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
volume_own_next:
	inc si
	cmp si, 10h
	jne volume_own_channel
volume_return:
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret
_set_sequence_volume endp

; 0x27a86
_flush_pending_volumes proc near
	xor bl, bl
	mov al, byte ptr cs:scan_stopped
	xor ah, ah
	mov si, ax
flush_voice:
	mov cl, byte ptr cs:pending_volume[si]
	cmp cl, 0ffh
	je flush_next
	mov byte ptr cs:pending_volume[si], 0ffh
	mov ch, 7
	mov ax, si
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
	inc bl
	cmp bl, 2
	je flush_stop
flush_next:
	inc si
	cmp si, 10h
	jne flush_wrapped
	xor si, si
flush_wrapped:
	mov al, byte ptr cs:scan_stopped
	xor ah, ah
	cmp si, ax
	jne flush_voice
flush_stop:
	mov ax, si
	mov byte ptr cs:scan_stopped, al
	ret
_flush_pending_volumes endp

; 0x27ace
_sound_service proc far
	cmp byte ptr cs:busy, 0
	je service_go
	retf
service_go:
	pushf
	cli
	push si
	push di
	push es
	push ds
	push bp
	cmp byte ptr cs:voices_changed, 0
	je service_sequences
	call _sequencer_tick
service_sequences:
	xor si, si
	xor di, di
service_sequence:
	les bx, dword ptr cs:playing_0_off[si]
	mov ax, es
	or ax, bx
	je service_done
	cmp byte ptr es:[bx+seq_skip], 0
	jne service_next_slot
	cmp byte ptr es:[bx+seq_fade_step], 0
	je service_step
	call _advance_volume_ramp
	cmp byte ptr es:[bx+seq_state], 0ffh
	jne service_step
	sub si, 4
	jmp short service_next_slot
service_step:
	cmp byte ptr es:[bx+seq_poll], 0
	je service_step_sequence
	call _drop_unless_polled
	jmp short service_check_retired
service_step_sequence:
	call _step_sequence
service_check_retired:
	cmp byte ptr es:[bx+seq_state], 0ffh
	je service_next
service_next_slot:
	add si, 4
service_next:
	add di, 4
	cmp si, 40h
	jne service_sequence
service_done:
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

; 0x27b52
_drop_unless_polled proc near
	push cx
	push si
	push ax
	mov cx, es
	xor si, si
drop_find:
	cmp word ptr cs:polled_0_off[si], bx
	jne drop_find_next
	cmp word ptr cs:polled_0_seg[si], cx
	je drop_return
drop_find_next:
	add si, 4
	cmp si, 40h
	jne drop_find
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:voices_changed, 1
drop_return:
	pop ax
	pop si
	pop cx
	ret
_drop_unless_polled endp

; 0x27b7e
_poll_sequences proc near
	push ds
	xor si, si
poll_slot:
	les bx, dword ptr cs:polled_0_off[si]
	mov cx, es
	cmp cx, 0
	jne poll_sequence
	cmp bx, 0
	jne poll_sequence
	jmp poll_done
poll_sequence:
	inc word ptr es:[bx+seq_ticks]
	lds bp, dword ptr es:[bx+seq_cursor_at]
	lds bp, dword ptr ds:[bp]
	mov cl, es:[bx+seq_poll]
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
	cmp byte ptr es:[bx+seq_poll], 10h
	ja poll_ask
	or byte ptr es:[bx+seq_poll], 80h
	mov cx, bx
	push bx
	mov bx, ax
	inc bx
	cmp byte ptr [bx], 0feh
	jne poll_first_args
	inc bx
poll_first_args:
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
	mov cl, es:[bx+seq_volume]
	mov ch, es:[bx+seq_looping]
	pop bx
	push cx
	mov ax, sp
	push ax
	mov ax, 3
	push ax
	call FAR PTR _sound_callback
	add sp, 0eh
	pop bx
	jmp short poll_next
poll_ask:
	mov ch, es:[bx+seq_looping]
	mov cl, es:[bx+seq_volume]
	push cx
	mov ax, sp
	push ax
	mov ax, 4
	push ax
	call FAR PTR _sound_callback
	add sp, 6
	cmp ah, 0
	je poll_keep
	mov word ptr es:[bx+seq_ticks], 0
poll_keep:
	cmp al, 0
	je poll_next
	mov byte ptr es:[bx+seq_poll], 0
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:voices_changed, 1
poll_next:
	add si, 4
	cmp si, 40h
	je poll_done
	jmp poll_slot
poll_done:
	pop ds
	ret
_poll_sequences endp

; 0x27c4e
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
	inc word ptr es:[bx+seq_ticks]
	lds bp, dword ptr es:[bx+seq_cursor_at]
	lds bp, dword ptr ds:[bp]
	mov word ptr cs:cursor_park, bp
	xor si, si
step_channel:
	mov al, es:[bx+si+seq_track_channel]
	cmp al, 0ffh
	jne step_not_ended
	jmp step_check_end
step_not_ended:
	cmp al, 0feh
	jne step_live
	jmp step_channel_next
step_live:
	mov byte ptr cs:own_voice, 0ffh
	mov byte ptr cs:bend_gate, 0
	push si
	mov si, ax
	and si, 0ffh
	test byte ptr es:[bx+si+seq_ch+chan_channel_flags], 2
	pop si
	je step_find_voice
	mov byte ptr cs:own_voice, al
	mov byte ptr cs:bend_gate, 1
	jmp short step_position
step_find_voice:
	and al, 0fh
	mov cl, al
	or cl, byte ptr cs:slot_high
	xor di, di
step_scan_voice:
	cmp byte ptr cs:voice_held[di], cl
	je step_voice_found
	inc di
	cmp di, 10h
	jne step_scan_voice
	jmp short step_position
step_voice_found:
	mov dx, di
	mov byte ptr cs:own_voice, dl
step_position:
	mov bp, word ptr cs:cursor_park
	shl si, 1
	mov dx, ds:[bp+si]
	add bp, dx
	add bp, es:[bx+si+seq_position]
	cmp word ptr es:[bx+si+seq_position], 0
	jne step_track_live
	shr si, 1
	jmp step_channel_next
step_track_live:
	shr si, 1
	shl si, 1
	cmp word ptr es:[bx+si+seq_delay], 0
	je step_due
	dec word ptr es:[bx+si+seq_delay]
	cmp word ptr es:[bx+si+seq_delay], 8000h
	jne step_waiting
	xor dh, dh
	shr si, 1
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	shl si, 1
	cmp dl, 0f8h
	jne step_delay_store
	mov dl, 0f0h
	mov dh, 80h
step_delay_store:
	mov es:[bx+si+seq_delay], dx
step_waiting:
	shr si, 1
	jmp step_channel_next
step_due:
	shr si, 1
step_event:
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	cmp dl, 80h
	jb step_running_status
	mov es:[bx+si+seq_status], dl
	jmp short step_status
step_running_status:
	mov dl, es:[bx+si+seq_status]
	dec bp
	shl si, 1
	dec word ptr es:[bx+si+seq_position]
	shr si, 1
step_status:
	mov al, dl
	mov ah, al
	and ah, 0f0h
	and al, 0fh
	cmp dl, 0fch
	jne step_not_track_end
	shl si, 1
	mov word ptr es:[bx+si+seq_position], 0
	shr si, 1
	jmp step_channel_next
step_not_track_end:
	cmp al, 0fh
	jne step_channel_event
	call _midi_meta_event
	shl si, 1
	mov dx, es:[bx+si+seq_position]
	shr si, 1
	cmp dx, 0
	jne step_delta
	jmp step_channel_next
step_channel_event:
	mov al, byte ptr cs:own_voice
	cmp ah, 80h
	jne step_not_note_off
	call _midi_note_off_event
	jmp short step_delta
step_not_note_off:
	cmp ah, 90h
	jne step_not_note
	call _midi_note_event
	jmp short step_delta
step_not_note:
	cmp ah, 0a0h
	jne step_not_a0
	call _midi_event_6
	jmp short step_delta
step_not_a0:
	cmp ah, 0b0h
	jne step_not_controller
	call _midi_controller_event
	jmp short step_delta
step_not_controller:
	cmp ah, 0c0h
	jne step_not_program
	call _midi_program_event
	jmp short step_delta
step_not_program:
	cmp ah, 0d0h
	jne step_not_d0
	call _midi_event_9
	jmp short step_delta
step_not_d0:
	cmp ah, 0e0h
	jne step_not_bend
	call _midi_bend_event
	jmp short step_delta
step_not_bend:
	cmp ah, 0f0h
	jne step_bad_status
	call _midi_skip_event
	jmp short step_delta
step_bad_status:
	shl si, 1
	mov word ptr es:[bx+si+seq_position], 0
	shr si, 1
	jmp short step_channel_next
step_delta:
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	cmp dl, 0
	jne step_delta_set
	jmp step_event
step_delta_set:
	shl si, 1
	cmp dl, 0f8h
	jne step_delta_short
	mov dl, 0efh
	mov dh, 80h
	mov es:[bx+si+seq_delay], dx
	shr si, 1
	jmp short step_channel_next
step_delta_short:
	xor dh, dh
	dec dl
	mov es:[bx+si+seq_delay], dx
	shr si, 1
step_channel_next:
	inc si
	cmp si, 10h
	je step_check_end
	jmp step_channel
step_check_end:
	xor si, si
step_check_track:
	cmp byte ptr es:[bx+si+seq_track_channel], 0ffh
	je step_all_ended
	shl si, 1
	cmp word ptr es:[bx+si+seq_position], 0
	jne step_return
	shr si, 1
	inc si
	cmp si, 10h
	jne step_check_track
step_all_ended:
	cmp byte ptr es:[bx+seq_rewind_mark], 0
	jne step_loop
	cmp byte ptr es:[bx+seq_looping], 0
	jne step_loop
	mov ax, bx
	call _remove_sequence
	mov byte ptr cs:voices_changed, 1
	jmp short step_return
step_loop:
	mov dx, es:[bx+seq_ticks_saved]
	mov es:[bx+seq_ticks], dx
	xor si, si
step_loop_track:
	mov dx, es:[bx+si+seq_position_saved]
	mov es:[bx+si+seq_position], dx
	mov dx, es:[bx+si+seq_delay_saved]
	mov es:[bx+si+seq_delay], dx
	shr si, 1
	mov dl, es:[bx+si+seq_status_saved]
	mov es:[bx+si+seq_status], dl
	shl si, 1
	add si, 2
	cmp si, 20h
	jne step_loop_track
step_return:
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

; 0x27e92
_midi_note_off_event proc near
	push si
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	push ax
	mov al, es:[bx+si+seq_track_channel]
	mov si, ax
	and si, 0fh
	pop ax
	cmp es:[bx+si+seq_ch+chan_note], ch
	jne note_off_send
	mov byte ptr es:[bx+si+seq_ch+chan_note], 0ffh
note_off_send:
	cmp al, 0ffh
	je note_off_return
	cmp byte ptr cs:muted, 0
	jne note_off_return
	and al, 0fh
	push bp
	mov bp, 4
	call dword ptr cs:driver_off
	pop bp
note_off_return:
	pop si
	ret
_midi_note_off_event endp

; 0x27ee1
_midi_note_event proc near
	push si
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	push ax
	mov al, es:[bx+si+seq_track_channel]
	mov si, ax
	and si, 0fh
	pop ax
	cmp cl, 0
	je note_zero_velocity
	mov es:[bx+si+seq_ch+chan_note], ch
	cmp al, 0ffh
	je note_return
	cmp byte ptr cs:muted, 0
	jne note_return
	and al, 0fh
	push bp
	mov bp, 5
	call dword ptr cs:driver_off
	pop bp
	jmp short note_return
note_zero_velocity:
	cmp es:[bx+si+seq_ch+chan_note], ch
	jne note_release_send
	mov byte ptr es:[bx+si+seq_ch+chan_note], 0ffh
note_release_send:
	cmp al, 0ffh
	je note_return
	cmp byte ptr cs:muted, 0
	jne note_return
	and al, 0fh
	push bp
	mov bp, 4
	call dword ptr cs:driver_off
	pop bp
note_return:
	pop si
	ret
_midi_note_event endp

; 0x27f54
_midi_event_6 proc near
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	cmp al, 0ffh
	je event_6_return
	cmp byte ptr cs:muted, 0
	jne event_6_return
	push bp
	mov bp, 6
	call dword ptr cs:driver_off
	pop bp
event_6_return:
	ret
_midi_event_6 endp

; 0x27f85
_midi_controller_event proc near
	push si
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	test byte ptr cs:bend_gate, 0ffh
	je controller_go
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:voice_held[si], 0ffh
	pop si
	je controller_go
	jmp controller_return
controller_go:
	push ax
	mov al, es:[bx+si+seq_track_channel]
	mov si, ax
	and si, 0fh
	pop ax
	cmp ch, 7
	jne controller_not_volume
	mov es:[bx+si+seq_ch+chan_volume], cl
	mov dl, es:[bx+seq_volume]
	call _scale_byte_pair
	cmp al, 20h
	jb controller_volume_now
	jmp controller_return
controller_volume_now:
	push si
	mov si, ax
	and si, 0ffh
	mov byte ptr cs:pending_volume[si], 0ffh
	pop si
	jmp short controller_send
controller_not_volume:
	cmp ch, 0ah
	jne controller_not_pan
	mov es:[bx+si+seq_ch+chan_pan], cl
	jmp short controller_send
controller_not_pan:
	cmp ch, 1
	jne controller_not_modulation
	mov es:[bx+si+seq_ch+chan_modulation], cl
	jmp short controller_send
controller_not_modulation:
	cmp ch, 40h
	jne controller_not_sustain
	push dx
	shl si, 1
	mov dx, es:[bx+si+seq_ch+chan_bend]
	or dh, 80h
	cmp cl, 0
	jne controller_sustain_store
	and dh, 7fh
controller_sustain_store:
	mov es:[bx+si+seq_ch+chan_bend], dx
	shr si, 1
	pop dx
	jmp short controller_send
controller_not_sustain:
	cmp ch, 4bh
	jne controller_not_4b
	push cx
	mov ch, es:[bx+si+seq_ch+chan_voice_budget]
	and ch, 0f0h
	or ch, cl
	mov es:[bx+si+seq_ch+chan_voice_budget], ch
	pop cx
	mov byte ptr cs:voices_changed, 1
	jmp short controller_send
controller_not_4b:
	cmp ch, 4eh
	jne controller_send
	push cx
	mov ch, es:[bx+si+seq_ch+chan_no_voice]
	and ch, 0f0h
	test cl, 0ffh
	je controller_4e_flag
	mov cl, 1
controller_4e_flag:
	or ch, cl
	mov es:[bx+si+seq_ch+chan_no_voice], ch
	pop cx
	mov byte ptr cs:voices_changed, 1
controller_send:
	cmp al, 0ffh
	jae controller_return
	cmp byte ptr cs:muted, 0
	jne controller_return
	and al, 0fh
	push bp
	mov bp, 7
	call dword ptr cs:driver_off
	pop bp
controller_return:
	pop si
	ret
_midi_controller_event endp

; 0x28086
_midi_program_event proc near
	push si
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	test byte ptr cs:bend_gate, 0ffh
	je program_go
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:voice_held[si], 0ffh
	pop si
	je program_go
	jmp short program_return
program_go:
	push ax
	mov al, es:[bx+si+seq_track_channel]
	mov si, ax
	and si, 0fh
	pop ax
	mov es:[bx+si+seq_ch+chan_program], cl
	cmp al, 0ffh
	jae program_return
	cmp byte ptr cs:muted, 0
	jne program_return
	and al, 0fh
	push bp
	mov bp, 8
	call dword ptr cs:driver_off
	pop bp
program_return:
	pop si
	ret
_midi_program_event endp

; 0x280da
_midi_event_9 proc near
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	cmp al, 0ffh
	jae event_9_return
	cmp byte ptr cs:muted, 0
	jne event_9_return
	push bp
	mov bp, 9
	call dword ptr cs:driver_off
	pop bp
event_9_return:
	ret
_midi_event_9 endp

; 0x280fe
_midi_bend_event proc near
	push si
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	test byte ptr cs:bend_gate, 0ffh
	je bend_go
	push si
	mov si, ax
	and si, 0fh
	cmp byte ptr cs:voice_held[si], 0ffh
	pop si
	je bend_go
	jmp short bend_return
bend_go:
	push ax
	mov al, es:[bx+si+seq_track_channel]
	mov si, ax
	and si, 0fh
	pop ax
	push cx
	xchg ch, cl
	shr ch, 1
	jae bend_low_bit
	or cl, 80h
bend_low_bit:
	shl si, 1
	cmp word ptr es:[bx+si+seq_ch+chan_bend], 8000h
	jb bend_store
	or ch, 80h
bend_store:
	mov es:[bx+si+seq_ch+chan_bend], cx
	shr si, 1
	pop cx
	cmp al, 0ffh
	jae bend_return
	cmp byte ptr cs:muted, 0
	jne bend_return
	and al, 0fh
	push bp
	mov bp, 0ah
	call dword ptr cs:driver_off
	pop bp
bend_return:
	pop si
	ret
_midi_bend_event endp

; 0x2817a
_midi_skip_event proc near
	call _skip_unknown_event
	ret
_midi_skip_event endp

; 0x2817e
_midi_meta_event proc near
	cmp ah, 0c0h
	je meta_c0
	cmp ah, 0b0h
	jne meta_skip
	jmp meta_b0
meta_skip:
	call _skip_unknown_event
	jmp meta_return
meta_c0:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	cmp ch, 7fh
	jne meta_cue
	push dx
	mov dl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	xor dh, dh
	cmp dl, 0f8h
	jne meta_mark_delay
	mov dh, 80h
	mov dl, 0f0h
meta_mark_delay:
	shl si, 1
	mov es:[bx+si+seq_delay], dx
	shr si, 1
	pop dx
	mov byte ptr es:[bx+si+seq_status], 0cfh
	push si
	push dx
	xor si, si
meta_mark_track:
	shl si, 1
	mov dx, es:[bx+si+seq_position]
	mov es:[bx+si+seq_position_saved], dx
	mov dx, es:[bx+si+seq_delay]
	mov es:[bx+si+seq_delay_saved], dx
	shr si, 1
	mov dl, es:[bx+si+seq_status]
	mov es:[bx+si+seq_status_saved], dl
	inc si
	cmp si, 10h
	jne meta_mark_track
	mov dx, es:[bx+seq_ticks]
	mov es:[bx+seq_ticks_saved], dx
	pop dx
	pop si
	shl si, 1
	dec word ptr es:[bx+si+seq_position]
	dec bp
	mov word ptr es:[bx+si+seq_delay], 0
	shr si, 1
	jmp short meta_return
meta_cue:
	cmp byte ptr cs:muted, 0
	jne meta_return
	mov es:[bx+seq_state], ch
	jmp short meta_return
meta_b0:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	mov cl, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	cmp ch, 50h
	jne meta_not_50
	cmp cl, 7fh
	jne meta_level_store
	mov cl, byte ptr cs:param_default
meta_level_store:
	mov es:[bx+seq_device_value], cl
	push ax
	push bp
	mov bp, 0bh
	call dword ptr cs:driver_off
	pop bp
	pop ax
	jmp short meta_return
meta_not_50:
	cmp ch, 60h
	jne meta_not_60
	cmp byte ptr cs:muted, 0
	jne meta_return
	inc word ptr es:[bx+seq_loop_count]
	jmp short meta_return
meta_not_60:
	cmp ch, 52h
	jne meta_return
	cmp es:[bx+seq_rewind_mark], cl
	jne meta_return
	push si
	xor si, si
meta_end_tracks:
	mov word ptr es:[bx+si+seq_position], 0
	add si, 2
	cmp si, 20h
	jne meta_end_tracks
	pop si
meta_return:
	ret
_midi_meta_event endp

; 0x2828e
_skip_unknown_event proc near
	cmp ah, 0f0h
	jne skip_not_sysex
skip_sysex:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	cmp ch, 0f7h
	jne skip_sysex
	ret
skip_not_sysex:
	cmp ah, 0c0h
	je skip_last_byte
	cmp ah, 0d0h
	je skip_last_byte
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
skip_last_byte:
	mov ch, ds:[bp]
	inc bp
	shl si, 1
	inc word ptr es:[bx+si+seq_position]
	shr si, 1
	ret
_skip_unknown_event endp

; 0x282cb
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
	je scale_floor_done
	dec cl
scale_floor_done:
	pop ax
	ret
find_playing_slot:
	push cx
	mov cx, es
	xor si, si
find_slot_loop:
	cmp word ptr cs:playing_0_off[si], bx
	jne find_slot_next
	cmp word ptr cs:playing_0_seg[si], cx
	jne find_slot_next
	jmp short find_slot_done
find_slot_next:
	add si, 4
	cmp si, 40h
	jne find_slot_loop
	mov si, 0ffh
find_slot_done:
	pop cx
	ret
_scale_byte_pair endp

; 0x28305
_init_sequence_params proc near
	push bx
	push cx
	push dx
	push bp
	push ds
	mov bx, ax
	cmp word ptr es:[bx+seq_cursor_at], -1
	jne init_has_params
	cmp word ptr es:[bx+seq_cursor_at+2], -1
	jne init_has_params
	jmp init_return
init_has_params:
	lds bp, dword ptr es:[bx+seq_cursor_at]
	lds bp, dword ptr ds:[bp]
	cmp byte ptr ds:[bp+23h], 0feh
	jne init_build
	cmp byte ptr ds:[bp+22h], 0fdh
	jne init_build
	cmp byte ptr ds:[bp+21h], 0fch
	jne init_build
	jmp init_return
init_build:
	push bp
	mov si, 20h
init_clear:
	sub si, 2
	mov word ptr cs:scratch[si], 0
	cmp si, 0
	jne init_clear
	mov byte ptr cs:scratch_mark, 0ffh
	cmp byte ptr ds:[bp], 0f0h
	jne init_find_device
	mov cl, ds:[bp+1]
	mov byte ptr cs:scratch_mark, cl
	add bp, 8
init_find_device:
	mov cl, ds:[bp]
	cmp cl, byte ptr cs:want_ch
	je init_device_found
	cmp cl, 0ffh
	je init_write_back
	inc bp
init_skip_entries:
	mov cl, ds:[bp]
	inc bp
	cmp cl, 0ffh
	jne init_skip_entry
	jmp short init_find_device
init_skip_entry:
	add bp, 5
	jmp short init_skip_entries
init_device_found:
	inc bp
init_copy_entry:
	mov cl, ds:[bp]
	inc bp
	cmp cl, 0ffh
	je init_write_back
	inc bp
	mov cx, ds:[bp]
	add bp, 4
	mov word ptr cs:scratch[si], cx
	add si, 2
	jmp short init_copy_entry
init_write_back:
	pop bp
	push bp
	xor si, si
init_write_word:
	mov cx, word ptr cs:scratch[si]
	mov ds:[bp], cx
	add si, 2
	add bp, 2
	cmp si, 20h
	jne init_write_word
	mov cl, byte ptr cs:scratch_mark
	mov ds:[bp], cl
	pop bp
	mov byte ptr ds:[bp+21h], 0fch
	mov byte ptr ds:[bp+22h], 0fdh
	mov byte ptr ds:[bp+23h], 0feh
init_return:
	pop ds
	pop bp
	pop dx
	pop cx
	pop bx
	ret
far_fn11_fn0f label byte
	db 0h, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 0eh, 0e8h, 2bh, 0e8h, 8bh, 0c1h
	db 5eh, 5fh, 1fh, 5dh, 0cbh, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 4eh, 0ah
	db 0eh, 0e8h, 40h, 0e7h, 5eh, 5fh, 1fh, 5dh, 0cbh, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h
	db 0eh, 0e8h, 0ch, 0e8h, 32h, 0f6h, 8bh, 0c1h, 5eh, 5fh, 1fh, 5dh, 0cbh
_init_sequence_params endp

; 0x2841f
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

; 0x28431
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
far_fn0a_install label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 4eh, 0ah, 0eh, 0e8h, 0eh, 0e6h
	db 5eh, 5fh, 1fh, 5dh, 0cbh
_set_master_level_far endp

; 0x28458
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

; 0x2846a
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

; 0x28480
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
far_fn0b_fn13 label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 56h, 0ah, 8ah, 4eh, 0ch, 8ah, 6eh, 0eh
	db 0eh, 0e8h, 1eh, 0e6h, 5eh, 5fh, 1fh, 5dh, 0cbh
_start_sequence_far endp

; 0x284b0
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
far_fn0e_fn17 label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 4eh, 0ah, 0eh, 0e8h, 62h, 0e6h
	db 5eh, 5fh, 1fh, 5dh, 0cbh, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8bh, 4eh, 0ah
	db 8ah, 56h, 0ch, 0eh, 0e8h, 2bh, 0e9h, 5eh, 5fh, 1fh, 5dh, 0cbh
_driver_fn13_far endp

; 0x284ef
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
far_fn0c_fn10 label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 4eh, 0ah, 0eh, 0e8h, 0dch, 0e5h
	db 5eh, 5fh, 1fh, 5dh, 0cbh, 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 0eh, 0e8h, 0dfh, 0e6h
	db 5eh, 5fh, 1fh, 5dh, 32h, 0e4h, 8ah, 0c1h, 0cbh
_retire_and_tick_far endp

; 0x2852c
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
far_fn15_silence label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8ah, 6eh, 0ah, 8ah, 4eh, 0ch, 8ah, 56h, 0eh
	db 0eh, 0e8h, 92h, 0e7h, 5eh, 5fh, 1fh, 5dh, 0cbh
_set_sequence_level_far endp

; 0x28559
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
far_fn0d label byte
	db 55h, 8bh, 0ech, 1eh, 57h, 56h, 0c4h, 46h, 6h, 8bh, 4eh, 0ah, 0eh, 0e8h, 7ch, 0e5h
	db 5eh, 5fh, 1fh, 5dh, 0cbh
_silence_driver_far endp
SOUND_TEXT ends
end
