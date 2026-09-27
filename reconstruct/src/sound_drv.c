/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The sound module's driver interface**: install the loaded driver and
 * ask what it is, configure it, silence it, hold it, set its levels, and
 * seek a sequence - every routine here but the seek is a call into the
 * driver through the far pointer at `cs:1e7h`. Hand-written assembly.
 *
 * One module of the original's **code segment 2619**, image
 * 0x265f2..0x26783. **Its ends are the image's, read the way TASM 3.0
 * writes a far call**: the entry table before it far-calls its routines as
 * TLINK's `nop / push cs / call`, which only a call to another file comes
 * out as, and so does its own seek's call to `start_sequence`, the
 * sequencer's first routine. The code-segment data it names is
 * sound_api.c's.
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
extrn _start_sequence:far
extrn _DG4A82:byte
SOUND_TEXT segment byte public 'CODE'
assume cs:SOUND_TEXT, ds:DGROUP
extrn _sequencer_tick:near
extrn _step_sequence:near
extrn c_26198:byte
extrn c_26368:byte
extrn c_26377:byte
extrn c_26379:byte
extrn c_26389:byte
extrn c_2638a:byte
extrn c_2638b:byte
extrn c_2638c:byte
extrn c_2638f:byte
extrn c_26390:byte
extrn c_26392:byte
extrn c_26399:byte
public _install_driver, _configure_driver, _silence_driver, _sound_hold
public _driver_fn13, _seek_sequence, _set_master_level, _set_sequence_level

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
SOUND_TEXT ends
}
#else
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
#endif
