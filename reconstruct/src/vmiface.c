/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The video driver's interface**: four jumps through the driver's vectors,
 * `restore_write_mode` and a one-`retf` routine in code, and in data
 * `g_vmds`, the drawing state the driver and the game share, `g_vm_driver`, the
 * driver's vector table, and `g_vm_hooks`, the game's services it is handed -
 * file I/O, the heap, strings - as far pointers the loader relocates.
 *
 * One module of the original's **code segment 1c25**, image
 * 0x1e93c..0x1e967, and DGROUP 0x3890..0x445e. **It is its own module
 * because of where its data starts**: lzhuf.c's `_DATA` ends at 0x3886 and
 * this starts at 0x3890, the next paragraph, with ten zeros between - a
 * `para` data segment, which only an assembly module declares, and a
 * C module's word-aligned `_DATA` could not leave. The code before it is
 * lzhuf.c's and the data after it palette.c's. Split out of lzhuf.c on
 * 2026-09-28; the name is ours.
 *
 * TASM source, the `#ifdef __TURBOC__` block below, with the host's
 * transcription in the `#else`. `g_vmds` is the image's bytes - the host's
 * `struct vmds` names its fields.
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
 * The module as TASM assembled it; the host's transcription is the `#else`.
 */
asm {
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
public _vm_call_4_thunk, _blit_bitmap_thunk, _blit_scaled_thunk, _vm_call_38_thunk
public _restore_write_mode, _vm_null_hook

/* 0x205c6 */
_vm_call_4_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+10h
_vm_call_4_thunk endp

/* 0x205ca */
_blit_bitmap_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+78h
_blit_bitmap_thunk endp

/* 0x205ce */
_blit_scaled_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+88h
_blit_scaled_thunk endp

/* 0x205d2 */
_vm_call_38_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+98h
_vm_call_38_thunk endp

/* 0x205d6 */
_restore_write_mode proc far
        cmp byte ptr DGROUP:_g_vmds+21h, 10h
        jne L1e965
        mov ax, 205h
        mov dx, 3ceh
        out dx, ax
        mov ax, 0ff08h
        out dx, ax
        mov dx, 3c4h
        mov ax, 0f02h
        out dx, ax
L1e965:
        retf
_restore_write_mode endp

/* 0x205f0 */
_vm_null_hook proc far
        retf
_vm_null_hook endp
VMIFACE_TEXT ends
}
#else

/*
 * DGROUP 0x3890..0x445e - **this module's `_DATA`**: the driver's data, its
 * call table - every slot `vm_null_hook` until `vm_init` fills it - and the
 * table of the game's routines the driver is handed, the `dd` list above.
 */
struct vmds g_vmds = {
    .clip_right = 0x013f,
    .clip_bottom = 0x00c7,
    .fill_enabled = 0x01,
    .screen = { .screen_width = 0x0140, .screen_height = 0x00c8 },
};

struct vm_driver g_vm_driver = {   /* DGROUP 0x4342 */
    .detect_allowed = 0x0001,
    .entry = {
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
        vm_null_hook,
    },
};

struct vm_hooks g_vm_hooks = {   /* DGROUP 0x440e */
    vm_null_hook,
    {
        (void (*)(void))dos_alloc_bytes,
        (void (*)(void))dos_free_far,
        (void (*)(void))readfd_far,
        (void (*)(void))game_fseek,
        (void (*)(void))game_ftell,
        (void (*)(void))game_fwrite,
        (void (*)(void))game_fputc,
        (void (*)(void))game_rewind,
        (void (*)(void))close_far,
        (void (*)(void))game_fopen,
        (void (*)(void))game_fread,
        (void (*)(void))game_fclose,
        (void (*)(void))malloc_far,
        (void (*)(void))calloc_far,
        (void (*)(void))game_fgetc,
        (void (*)(void))free_far,
        (void (*)(void))strcat_far,
        (void (*)(void))strcpy_far,
        (void (*)(void))strchr_far,
    },
};

/*
 * 0x205c6
 *
 * A jump through the video driver's vector 4, DGROUP 0x4352. Nothing calls it. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void vm_call_4_thunk(void)
{
    not_transcribed("0x1e93c");
}

/*
 * 0x205ca
 *
 * A thunk into the video driver: `ljmp [0x43ba]`, which is `vm_blit_bitmap`.
 * It jumps rather than calls, so the driver returns to this routine's caller
 * and reads that caller's arguments off the stack unchanged.
 */
void blit_bitmap_thunk(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    vm_blit_bitmap(bmp, x, y, mode);
}

/*
 * 0x205ce
 *
 * A thunk into the video driver: `ljmp [0x43ca]`, which is VGA:0x271b. Same
 * arrangement as 0x1e940 - it takes three arguments rather than four, because
 * that is what its caller pushed.
 */
void blit_scaled_thunk(struct bitmap * bmp, int16_t x, int16_t y)
{
    vm_blit_scaled(bmp, x, y);
}

/*
 * 0x205d2
 *
 * A jump through the video driver's vector 38, DGROUP 0x43da. Nothing calls it. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void vm_call_38_thunk(void)
{
    not_transcribed("0x1e948");
}

/*
 * 0x205d6
 *
 * Put the graphics controller back the way the rest of the code expects it,
 * after a routine that changed it to draw. Write mode 2, every bit of the bit
 * mask, every plane of the map mask - the same three registers `vm_blit_bitmap`
 * restores in its epilogue, and the same values.
 *
 * On any adapter but 0x10 it does nothing at all: the whole body is behind that
 * test, and the routine is two `retf`s in a row in the image because the second
 * one is a separate one-byte routine.
 */
void restore_write_mode(void)
{
    if (g_vmds.adapter != 0x10)
        return;

    io_out16(PORT_GC_INDEX, 0x0205);            /* write mode 2 */
    io_out16(PORT_GC_INDEX, 0xff08);            /* bit mask: every bit */
    io_out16(PORT_SEQ_INDEX, 0x0f02);           /* map mask: every plane */
}

/*
 * 0x205f0
 *
 * **A far routine that does nothing**, one `retf` - the entry every one of
 * `g_vm_driver.entry`'s fifty slots and `g_vm_hooks.ptr_440e` hold until the driver
 * fills them. The name is ours. NOT TRANSCRIBED YET for the host: nothing
 * calls a slot before `vm_init` has filled it. A stub, which aborts.
 */
void vm_null_hook(void)
{
    not_transcribed("0x205f0");
}
#endif
