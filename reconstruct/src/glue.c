/*
 * The Incredible Machine - reconstruction
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file: it is
 * derived from someone else's executable.
 *
 * **The interface to the loaded sound module**, in `_TEXT`: code segment
 * 0000, image range 0x0bb98..0x0bbfe. Nine wrappers that load a function
 * number and fall into the trampoline at 0x0bbd4, which far-calls the module
 * through DGROUP 0x4a98. Hand-written assembly - a near `call` to a shared
 * tail and no frame of their own - so this is the host's transcription and
 * is not judged. It starts on a word boundary: 0x0bb97 is the pad byte after
 * thunks.c, the far faces of the runtime's routines.
 *
 * **This boundary is ours, and so is the one on either side of it.** Segment
 * 0000 is `_TEXT`, which three parties share: Borland's startup owns the entry
 * point at its front, the library modules all put their code there too, and
 * some of the game's units did as well. TLINK combines a segment's parts in
 * the order it meets them - the startup first, then the program's objects in
 * link order, then the library modules it pulls in at the end - so the segment
 * reads startup, game, library. These routines are the last of the game's
 * objects before the library begins at 0x0bbfe with its `int 21h` wrappers.
 * `machine.c` holds the game's other `_TEXT` code and `borland_heap.c` and
 * `borland_file.c` the library's; this file is what sits between, and it is a
 * file of its own so that the library units stay the library and nothing else.
 * Which compiler option put these units into `_TEXT` cannot be read off the
 * image and is not claimed here.
 *
 * Functions are in address order and each carries the image offset it was
 * read from.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: assembler tasm1.01
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it. Borland C++ passes a file-level `asm`
 * block through to the assembler whole (`-S`, then TASM), so this is the
 * source the image's bytes come from; the host's transcription is the
 * `#else`. A C comment is the only kind the block may carry.
 */
asm {
_TEXT segment word public 'CODE'
assume cs:_TEXT, ds:DGROUP
public _sound_module_install, _sound_module_set_rate, _sound_module_service
public _sound_module_9, _sound_module_10, _sound_module_11
public _stop_loaded_module, _sound_module_shutdown
public _call_sound_module, _sound_module_position
extrn _DG4A82:byte

/* 0x0bb98 */
_sound_module_install proc far
    mov ax, 0
    call _call_sound_module
    retf
_sound_module_install endp

/* 0x0bb9f */
_sound_module_set_rate proc far
    mov ax, 6
    call _call_sound_module
    retf
_sound_module_set_rate endp

/* 0x0bba6: the EOI to the master PIC, then the service call. */
_sound_module_service proc far
    mov al, 20h
    out 20h, al
    mov ax, 1
    call _call_sound_module
    retf
_sound_module_service endp

/* 0x0bbb1 */
_sound_module_9 proc far
    mov ax, 9
    call _call_sound_module
    retf
_sound_module_9 endp

/* 0x0bbb8 */
_sound_module_10 proc far
    mov ax, 0ah
    call _call_sound_module
    retf
_sound_module_10 endp

/* 0x0bbbf */
_sound_module_11 proc far
    mov ax, 0bh
    call _call_sound_module
    retf
_sound_module_11 endp

/* 0x0bbc6 */
_stop_loaded_module proc far
    mov ax, 2
    call _call_sound_module
    retf
_stop_loaded_module endp

/* 0x0bbcd */
_sound_module_shutdown proc far
    mov ax, 0ch
    call _call_sound_module
    retf
_sound_module_shutdown endp

/* 0x0bbd4: SI at the wrapper's caller's arguments, past BP and two returns. */
_call_sound_module proc near
    push bp
    mov bp, sp
    push di
    push si
    mov si, bp
    add si, 8
    call dword ptr DGROUP:_DG4A82+16h
    pop si
    pop di
    pop bp
    ret
_call_sound_module endp

/* 0x0bbe6: six bytes of stack for the three words the module writes back. */
_sound_module_position proc far
    mov ax, 0dh
    push bp
    mov bp, sp
    push di
    push si
    sub sp, 6
    mov si, sp
    call dword ptr DGROUP:_DG4A82+16h
    pop ax
    pop ax
    pop dx
    pop si
    pop di
    pop bp
    retf
_sound_module_position endp
_TEXT ends
}
#else

/*
 * 0x0bb98
 *
 * Install the module. Its two arguments are the host callback and a flag, and
 * `asb_install` takes neither: what the original passes on the stack the
 * module reads through SI, and this one reads nothing.
 */
uint16_t sound_module_install(uint16_t callback, uint16_t flag)
{
    (void)callback;
    (void)flag;
    return call_sound_module(0, (union sound_module_args *)(void *)dg_near_ptr(guest_sp));
}


/*
 * 0x0bb9f
 */
uint16_t sound_module_set_rate(union sound_module_args * si)
{
    return call_sound_module(6, si);
}


/*
 * 0x0bba6
 *
 * The service call, and the only wrapper that touches hardware itself: it
 * sends the non-specific EOI to the master PIC before entering the module.
 * `ASB:` function 1 is a bare `xor ax,ax; ret`, so on this module the EOI is
 * the whole of it.
 */
uint16_t sound_module_service(union sound_module_args * si)
{
    io_out8(0x20, 0x20);
    return call_sound_module(1, si);
}


/* OURS: `SOUND_MODULE_TICK` on the host - see tim.h. */
void sound_module_tick(void)
{
    sound_module_service((union sound_module_args *)(void *)dg_near_ptr(guest_sp));
}


/*
 * 0x0bbb1
 *
 * Three the game calls and `ASB:` does not implement - its entries 9, 10 and
 * 11 are the bare `ret`s at 0x42c, 0x42f and 0x430. One address each, because
 * a group comment is provenance for the routine it sits above and no other.
 */
uint16_t sound_module_9(union sound_module_args * si)  { return call_sound_module(9, si); }
/* 0x0bbb8 */
uint16_t sound_module_10(union sound_module_args * si) { return call_sound_module(10, si); }
/* 0x0bbbf */
uint16_t sound_module_11(union sound_module_args * si) { return call_sound_module(11, si); }


/*
 * 0x0bbc6
 *
 * Take the module down.
 */
uint16_t stop_loaded_module(void)
{
    return call_sound_module(2, (union sound_module_args *)(void *)dg_near_ptr(guest_sp));
}


/*
 * 0x0bbcd
 */
uint16_t sound_module_shutdown(void)
{
    return call_sound_module(12, (union sound_module_args *)(void *)dg_near_ptr(guest_sp));
}


/*
 * 0x0bbd4
 *
 * The trampoline every call into the **loaded sound module** goes through:
 * push a frame, point SI at the caller's own arguments - `bp + 8`, which is
 * past the saved BP, this routine's near return and the wrapper's far one -
 * and far-call the module through DGROUP 0x4a98.
 *
 * The nine wrappers below it are one shape: load AX with a function number,
 * come here, and `retf`. Which numbers exist is a property of the *game*, not
 * of the module: these nine are the only calls in the image, so the module's
 * other seven entries are never reached however many it implements.
 *
 * The port dispatches into `asb_dispatch` rather than through the pointer,
 * because `ASB:` is the one module it transcribes. A different module in
 * RESOURCE.CFG would need its own, and `setup_sound_device` would have loaded
 * a block of code the port has no body for.
 */
uint16_t call_sound_module(uint16_t fn, union sound_module_args * si)
{
    return asb_dispatch(fn, si);
}


/*
 * 0x0bbe6
 *
 * Ask the module where it has got to. Six bytes of stack are reserved for the
 * three words it writes back and popped afterwards - the original keeps the
 * third in DX and drops the first two.
 */
uint16_t sound_module_position(uint16_t *a, uint16_t *b, uint16_t *c)
{
    /*
     * Six bytes the module fills in - `asb_position` writes three words
     * through the pointer and nothing else looks at the address. The comment
     * here used to say they had to be the guest's because the module reads
     * them through SI; the module is `asb_dispatch` in this port, its own C,
     * and SI is the shape the original had to pass a pointer in.
     */
    union sound_module_args fp;

    call_sound_module(13, &fp);

    if (a) *a = fp.position.id;
    if (b) *b = (uint16_t)fp.position.position;
    if (c) *c = (uint16_t)(fp.position.position >> 16);

    return 0;
}
#endif
