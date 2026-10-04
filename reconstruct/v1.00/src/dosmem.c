/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **DOS memory**: allocate and free far blocks with INT 21h, and two thunks
 * into the video driver before them. Hand-written assembly.
 *
 * One module of the original's **code segment 1c25**, image 0x21ab5..0x21b44,
 * with no data. **Its end is proven, its front is ours**: `dos_alloc_bytes`
 * clears what it allocated through `far_memset` with TLINK's `nop / push cs /
 * call`, so `far_memset` (lowlevel.c) was in another file; where between the
 * two the file changed is not measured, and this takes the first boundary
 * the routines' own groups offer.
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
 * The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
 * transcription is the `#else`. See glue.c for how the block reaches the
 * assembler.
 */
asm {
extrn _far_memset:far
extrn _g_vm_driver:byte
DOSMEM_TEXT segment byte public 'CODE'
assume cs:DOSMEM_TEXT, ds:DGROUP
public _save_rect_thunk, _buffer_size_thunk, _dos_alloc_bytes, _dos_free_far

/* 0x21ab5 */
_save_rect_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+18h
_save_rect_thunk endp

/* 0x21ab9 */
_buffer_size_thunk proc near
        jmp dword ptr DGROUP:_g_vm_driver+1ch
_buffer_size_thunk endp

/* 0x21abd */
_dos_alloc_bytes proc far
        push bp
        mov bp, sp
        mov ax, word ptr [bp+8]
        mov bx, word ptr [bp+6]
        cmp ax, bx
        jne alloc_to_paras
        cmp ax, 0ffffh
        je alloc_ask_free
alloc_to_paras:
        mov dx, bx
        shr ax, 1
        rcr bx, 1
        shr ax, 1
        rcr bx, 1
        shr ax, 1
        rcr bx, 1
        shr ax, 1
        rcr bx, 1
        and dx, 0fh
        je alloc_rounded
        inc bx
alloc_rounded:
        mov ah, 48h
        int 21h
        jae alloc_got
        xor ax, ax
        xor dx, dx
        jmp short alloc_return
alloc_got:
        mov dx, ax
        mov ax, word ptr [bp+0ch]
        and ax, 1
        je alloc_return
        push dx
        mov ax, word ptr [bp+8]
        push ax
        mov ax, word ptr [bp+6]
        push ax
        xor ax, ax
        push ax
        push dx
        push ax
        call FAR PTR _far_memset
        add sp, 0ah
        pop dx
        xor ax, ax
        jmp short alloc_return
alloc_ask_free:
        mov ah, 48h
        int 21h
        mov ax, bx
        xor bx, bx
        shl ax, 1
        rcl bx, 1
        shl ax, 1
        rcl bx, 1
        shl ax, 1
        rcl bx, 1
        shl ax, 1
        rcl bx, 1
        mov dx, bx
alloc_return:
        pop bp
        retf
_dos_alloc_bytes endp

/* 0x21b34 */
_dos_free_far proc far
        push bp
        mov bp, sp
        push es
        mov ax, word ptr [bp+8]
        mov es, ax
        mov ah, 49h
        int 21h
        pop es
        pop bp
        retf
_dos_free_far endp
DOSMEM_TEXT ends
}
#else


/*
 * 0x21ab5
 *
 * A thunk into the video driver: `ljmp [0x435a]`, which is `vm_save_rect`.
 * Same arrangement as 0x2149a.
 */
void save_rect_thunk(uint8_t far * buf, int16_t x, int16_t y,
                     int16_t w, int16_t h)
{
    vm_save_rect(buf, x, y, w, h);
}

/*
 * 0x21ab9
 *
 * A thunk into the video driver: `ljmp [0x435e]`, which is `vm_buffer_size`.
 * Same arrangement as 0x2149a.
 */
uint16_t buffer_size_thunk(uint16_t w, uint16_t h)
{
    return (uint16_t)vm_buffer_size(w, h);
}

/*
 * 0x21abd
 *
 * Allocate memory from DOS, given a **32-bit byte count**, and answer a far
 * pointer to it in DX:AX - always at offset 0, since DOS hands out whole
 * paragraphs.
 *
 * The size is turned into paragraphs by shifting the pair right four times
 * with `shr`/`rcr`, and rounded **up** if any of the low four bits were set -
 * the remainder is tested from a copy taken before the shifting.
 *
 * A size of 0xffffffff is not a request but a question: it calls DOS with
 * 0xffff paragraphs, which always fails, and converts the largest-free figure
 * DOS reports back into bytes. So one routine both allocates and asks how much
 * there is, told apart by its argument.
 *
 * Bit 0 of the flags' high word asks for the block to be zeroed, which it
 * does through `far_memset` at 0x22300. The flags are one `long` whose high
 * word, [bp+0xc], is the only part read. Two callers ask for zeroing -
 * `game_startup`'s 0x18-byte block and `load_archive_map`'s entry lists.
 *
 * The count goes back through the same answer as the pointer does; see
 * `DOS_ALLOC_BYTES`. The DOS call itself is IO - `io_dos_alloc`, in hostio.c.
 */
uint8_t far *dos_alloc_bytes(uint32_t size, uint32_t flags)
{
    uint16_t paras, remainder, largest;
    dg_seg_t seg;
    int32_t failed;

    /* **One Borland `long`**, low word at [bp+6]. 0x21ad1 shifts the pair
       right four with `shr ax,1 / rcr bx,1` four times over, which is a
       32-bit shift and not two 16-bit ones; the test above it is
       `cmp ax,bx / jne / cmp ax,0xffff`, the pair against 0xffffffff. */
    if (size == 0xFFFFFFFFu) {
        /* The "how much is free" question: the bytes in DX:AX. */
        io_dos_alloc(0xFFFF, &largest, &failed);
        return (uint8_t far *)(uintptr_t)((uint32_t)largest << 4);
    }

    /* Bytes to paragraphs, rounded up. The high half of the shifted pair
       is dropped - DOS takes the count in BX alone, and AH is loaded with
       0x48 over what was in AX - so a request above a megabyte would
       truncate here exactly as it does in the original. */
    remainder = (uint16_t)(size & 0x0F);
    paras = (uint16_t)(size >> 4);
    if (remainder != 0)
        paras = (uint16_t)(paras + 1);

    seg = io_dos_alloc(paras, &largest, &failed);
    if (failed)
        return NULL;

    if ((flags >> 16) & 1)
        far_memset(MK_FP(seg, 0), 0, size);

    return MK_FP(seg, 0);
}

/*
 * 0x21b34
 *
 * Hand a block back to DOS - INT 21h with AH=0x49 and the block's segment in
 * ES.
 *
 * The argument is a **far pointer**, and only its segment half is used: the
 * routine reads [bp+8], the second word, and never looks at the offset at
 * [bp+6]. DOS hands out whole paragraphs at offset zero, so the offset carries
 * no information to begin with - and a pointer to a block's start answers its
 * segment through `FP_SEG`.
 *
 * Nothing checks the result. DOS reports failure in CF with an error code in
 * AX, and the routine returns whatever DOS left there without looking, so a
 * double free or a corrupted arena passes silently.
 *
 * The DOS call is IO - `io_dos_free`, in hostio.c.
 */
void dos_free_far(void far *block)
{
    io_dos_free(FP_SEG(block));
}
#endif
