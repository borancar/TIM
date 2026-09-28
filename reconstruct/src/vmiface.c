/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The video driver's interface**: four jumps through the driver's vectors,
 * `restore_write_mode` and a one-`retf` routine in code, and in data
 * `VMDS`, the drawing state the driver and the game share, `DG4342`, the
 * driver's vector table, and `DG440E`, the game's services it is handed -
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
 * transcription in the `#else`. `VMDS` is the image's bytes - the host's
 * `struct vmds` names its fields.
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
 * The module as TASM assembled it; the host's transcription is the `#else`.
 */
asm {
extrn _dos_alloc_bytes:far
extrn _dos_free_far:far
extrn _borland_read_far:far
extrn _game_fseek:far
extrn _game_ftell:far
extrn _game_fwrite:far
extrn _game_fputc:far
extrn _game_rewind:far
extrn _close_handle_far:far
extrn _game_fopen:far
extrn _game_fread:far
extrn _game_fclose:far
extrn _heap_malloc_far:far
extrn _heap_calloc_far:far
extrn _game_fgetc:far
extrn _heap_free_far:far
extrn _string_concat_far:far
extrn _string_copy_far:far
extrn _string_chr_far:far
_DATA segment para public 'DATA'
public _VMDS, _DG4342, _DG440E
_VMDS label byte
        db 6 dup (0)
        db 03fh, 001h
        db 2 dup (0)
        db 0c7h
        db 0
        db 001h
        db 1757 dup (0)
        db 040h, 001h, 0c8h
        db 965 dup (0)
_DG4342 label byte
        dw 0
        dw 1
        dd 50 dup (_vm_null_hook)
_DG440E label byte
        dd _vm_null_hook
        dd _dos_alloc_bytes
        dd _dos_free_far
        dd _borland_read_far
        dd _game_fseek
        dd _game_ftell
        dd _game_fwrite
        dd _game_fputc
        dd _game_rewind
        dd _close_handle_far
        dd _game_fopen
        dd _game_fread
        dd _game_fclose
        dd _heap_malloc_far
        dd _heap_calloc_far
        dd _game_fgetc
        dd _heap_free_far
        dd _string_concat_far
        dd _string_copy_far
        dd _string_chr_far
_DATA ends

VMIFACE_TEXT segment byte public 'CODE'
assume cs:VMIFACE_TEXT, ds:DGROUP
public _vm_call_4_thunk, _blit_bitmap_thunk, _blit_scaled_thunk, _vm_call_38_thunk
public _restore_write_mode, _vm_null_hook

/* 0x1e93c */
_vm_call_4_thunk proc near
        jmp dword ptr DGROUP:_DG4342+10h
_vm_call_4_thunk endp

/* 0x1e940 */
_blit_bitmap_thunk proc near
        jmp dword ptr DGROUP:_DG4342+78h
_blit_bitmap_thunk endp

/* 0x1e944 */
_blit_scaled_thunk proc near
        jmp dword ptr DGROUP:_DG4342+88h
_blit_scaled_thunk endp

/* 0x1e948 */
_vm_call_38_thunk proc near
        jmp dword ptr DGROUP:_DG4342+98h
_vm_call_38_thunk endp

/* 0x1e94c */
_restore_write_mode proc far
        cmp byte ptr DGROUP:_VMDS+21h, 10h
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

/* 0x1e966 */
_vm_null_hook proc far
        retf
_vm_null_hook endp
VMIFACE_TEXT ends
}
#else

/*
 * DGROUP 0x3890..0x445e - **this module's `_DATA`**, at their addresses; the
 * pointer tables as the guest's pairs, since the driver reads them.
 */
struct vmds VMDS DGROUP_AT(0x3890) = {
    .clip_right = 0x013f,
    .clip_bottom = 0x00c7,
    .fill_enabled = 0x01,
    .screen = { .screen_width = 0x0140, .screen_height = 0x00c8 },
};

struct dg_4342 DG4342 DGROUP_AT(0x4342) = {
    .detect_allowed = 0x0001,
    .font = {
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
    },
};

struct dg_440e DG440E DGROUP_AT(0x440e) = {
    .ptr_440e = { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
    .driver_table = {
        { .off = 0x586d, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x58e4, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0xba6a, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x92dc, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x93a2, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x94fb, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x9571, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x93e0, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xba5b, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x8fcd, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x91ef, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x917f, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb1e, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb75, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x93f6, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb2d, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb3c, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb4f, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb62, .seg = LOAD_SEG + 0x0000 },
    },
};

/*
 * 0x1e93c
 *
 * A jump through the video driver's vector 4, DGROUP 0x4352. Nothing calls it. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void vm_call_4_thunk(void)
{
    not_transcribed("0x1e93c");
}

/*
 * 0x1e940
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
 * 0x1e944
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
 * 0x1e948
 *
 * A jump through the video driver's vector 38, DGROUP 0x43da. Nothing calls it. NOT TRANSCRIBED YET for the host: nothing the port runs reaches
 * it. A stub, which aborts; the TASM source above is the original's.
 */
void vm_call_38_thunk(void)
{
    not_transcribed("0x1e948");
}

/*
 * 0x1e94c
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
    if (VMDS.adapter != 0x10)
        return;

    io_out16(PORT_GC_INDEX, 0x0205);            /* write mode 2 */
    io_out16(PORT_GC_INDEX, 0xff08);            /* bit mask: every bit */
    io_out16(PORT_SEQ_INDEX, 0x0f02);           /* map mask: every plane */
}

/*
 * 0x1e966
 *
 * **A far routine that does nothing**, one `retf` - the entry every one of
 * `DG4342.font`'s fifty slots and `DG440E.ptr_440e` hold until the driver
 * fills them. The name is ours. NOT TRANSCRIBED YET for the host: the host
 * keeps those slots as the guest's pairs and never calls through them. A
 * stub, which aborts.
 */
void vm_null_hook(void)
{
    not_transcribed("0x1e966");
}
#endif
