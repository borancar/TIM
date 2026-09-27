/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The screenshot writer's assembly**: writing and reading the DAC, and
 * turning a row of pixels into bitplanes. This file corresponds to the
 * forty-fifth and last module of the original's code segment 172c, image
 * 0x1c087..0x1c251, which ends a byte into segment 1c25's first paragraph.
 *
 * `chunky_to_planar` borrows BP as a
 * data register and keeps its loop count on the stack, which no compiler
 * does. **So it is TASM source**, the `#ifdef __TURBOC__` block below, with the
 * host's transcription in the `#else`. The functions are in address order and each carries the image offset it was
 * read from, as everywhere else.
 *
 * Only screenshot.c calls into it, and nothing calls `vga_set_dac` at all.
 * The names are ours.
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
 * The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
 * transcription is the `#else`. See glue.c for how the block reaches the
 * assembler.
 */
asm {
VGADAC_TEXT segment byte public 'CODE'
assume cs:VGADAC_TEXT, ds:DGROUP
public _vga_set_dac, _vga_get_dac, _chunky_to_planar

/* 0x1c087 */
_vga_set_dac proc far
        push bp
        mov bp, sp
        push si
        mov si, word ptr [bp+6]
        mov cx, word ptr [bp+0ah]
        mov ax, cx
        shl cx, 1
        add cx, ax
        mov bx, word ptr [bp+8]
        mov dx, 3dah
L1c09d:
        in al, dx
        and al, 8
        je L1c09d
        mov dx, 3c7h
        in al, dx
        and al, 3
        cmp al, 3
        je L1c0b0
        mov dx, 3c9h
        out dx, al
L1c0b0:
        mov dx, 3c8h
        mov ax, bx
        out dx, al
        mov dx, 3c9h
        cli
L1c0ba:
        lodsb
        out dx, al
        loop L1c0ba
        sti
        pop si
        pop bp
        retf
_vga_set_dac endp

/* 0x1c0c2 */
_vga_get_dac proc far
        push bp
        mov bp, sp
        push di
        push ds
        pop es
        mov di, word ptr [bp+6]
        mov cx, word ptr [bp+0ah]
        mov ax, cx
        shl cx, 1
        add cx, ax
        mov bx, word ptr [bp+8]
        mov dx, 3dah
L1c0da:
        in al, dx
        and al, 8
        je L1c0da
        mov dx, 3c7h
        in al, dx
        and al, 3
        or al, al
        je L1c0f0
        mov dx, 3c9h
        in al, dx
        mov dx, 3c7h
L1c0f0:
        mov ax, bx
        out dx, al
        mov dx, 3c9h
        cli
L1c0f7:
        in al, dx
        stosb
        loop L1c0f7
        sti
        pop di
        pop bp
        retf
_vga_get_dac endp

/* 0x1c0ff */
_chunky_to_planar proc far
        push bp
        mov bp, sp
        push si
        push di
        mov si, word ptr [bp+6]
        mov di, word ptr [bp+8]
        mov ax, 50h
        push ax
L1c10e:
        mov bp, word ptr [si]
        shr bp, 1
        rcl ah, 1
        shr bp, 1
        rcl al, 1
        shr bp, 1
        rcl bh, 1
        shr bp, 1
        rcl bl, 1
        shr bp, 1
        rcl ch, 1
        shr bp, 1
        rcl cl, 1
        shr bp, 1
        rcl dh, 1
        shr bp, 1
        rcl dl, 1
        shr bp, 1
        rcl ah, 1
        shr bp, 1
        rcl al, 1
        shr bp, 1
        rcl bh, 1
        shr bp, 1
        rcl bl, 1
        shr bp, 1
        rcl ch, 1
        shr bp, 1
        rcl cl, 1
        shr bp, 1
        rcl dh, 1
        shr bp, 1
        rcl dl, 1
        mov bp, word ptr [si+2]
        shr bp, 1
        rcl ah, 1
        shr bp, 1
        rcl al, 1
        shr bp, 1
        rcl bh, 1
        shr bp, 1
        rcl bl, 1
        shr bp, 1
        rcl ch, 1
        shr bp, 1
        rcl cl, 1
        shr bp, 1
        rcl dh, 1
        shr bp, 1
        rcl dl, 1
        shr bp, 1
        rcl ah, 1
        shr bp, 1
        rcl al, 1
        shr bp, 1
        rcl bh, 1
        shr bp, 1
        rcl bl, 1
        shr bp, 1
        rcl ch, 1
        shr bp, 1
        rcl cl, 1
        shr bp, 1
        rcl dh, 1
        shr bp, 1
        rcl dl, 1
        mov bp, word ptr [si+4]
        shr bp, 1
        rcl ah, 1
        shr bp, 1
        rcl al, 1
        shr bp, 1
        rcl bh, 1
        shr bp, 1
        rcl bl, 1
        shr bp, 1
        rcl ch, 1
        shr bp, 1
        rcl cl, 1
        shr bp, 1
        rcl dh, 1
        shr bp, 1
        rcl dl, 1
        shr bp, 1
        rcl ah, 1
        shr bp, 1
        rcl al, 1
        shr bp, 1
        rcl bh, 1
        shr bp, 1
        rcl bl, 1
        shr bp, 1
        rcl ch, 1
        shr bp, 1
        rcl cl, 1
        shr bp, 1
        rcl dh, 1
        shr bp, 1
        rcl dl, 1
        mov bp, word ptr [si+6]
        shr bp, 1
        rcl ah, 1
        shr bp, 1
        rcl al, 1
        shr bp, 1
        rcl bh, 1
        shr bp, 1
        rcl bl, 1
        shr bp, 1
        rcl ch, 1
        shr bp, 1
        rcl cl, 1
        shr bp, 1
        rcl dh, 1
        shr bp, 1
        rcl dl, 1
        shr bp, 1
        rcl ah, 1
        shr bp, 1
        rcl al, 1
        shr bp, 1
        rcl bh, 1
        shr bp, 1
        rcl bl, 1
        shr bp, 1
        rcl ch, 1
        shr bp, 1
        rcl cl, 1
        shr bp, 1
        rcl dh, 1
        shr bp, 1
        rcl dl, 1
        push di
        add si, 8
        mov byte ptr [di], ah
        add di, 50h
        mov byte ptr [di], al
        add di, 50h
        mov byte ptr [di], bh
        add di, 50h
        mov byte ptr [di], bl
        add di, 50h
        mov byte ptr [di], ch
        add di, 50h
        mov byte ptr [di], cl
        add di, 50h
        mov byte ptr [di], dh
        add di, 50h
        mov byte ptr [di], dl
        pop di
        inc di
        pop ax
        dec ax
        push ax
        je L1c24c
        jmp L1c10e
L1c24c:
        pop ax
        pop di
        pop si
        pop bp
        retf
_chunky_to_planar endp
VGADAC_TEXT ends
}
#else

/*
 * 0x1c087
 *
 * Load `count` colours into the DAC from index `first`. It is the driver's
 * `vm_set_palette` instruction for instruction: wait for retrace, write the
 * masked DAC state back to the data port unless it reads 3, set the index,
 * and send `count * 3` bytes with interrupts off.
 */
void vga_set_dac(const uint8_t *rgb, int16_t first, int16_t count)
{
    uint16_t bytes = (uint16_t)(count * 3);
    uint8_t state;

    while (!(io_in8(PORT_INPUT_ST1) & 0x08))
        ;
    state = (uint8_t)(io_in8(PORT_DAC_READ) & 3);
    if (state != 3)
        io_out8(PORT_DAC_DATA, state);
    io_out8(PORT_DAC_WRITE, (uint8_t)first);
    do {
        io_out8(PORT_DAC_DATA, *rgb++);
    } while (--bytes);
}

/*
 * 0x1c0c2
 *
 * Read `count` colours out of the DAC from index `first`. The mirror of the
 * one above, with the state test turned round: unless the state reads 0 it
 * throws one byte of the data port away first. The index goes to the read
 * port either way - the `mov dx,0x3c7` after the dummy read only puts back
 * the port the state was read from.
 */
void vga_get_dac(uint8_t *rgb, int16_t first, int16_t count)
{
    uint16_t bytes = (uint16_t)(count * 3);

    while (!(io_in8(PORT_INPUT_ST1) & 0x08))
        ;
    if ((io_in8(PORT_DAC_READ) & 3) != 0)
        (void)io_in8(PORT_DAC_DATA);
    io_out8(PORT_DAC_READ, (uint8_t)first);
    do {
        *rgb++ = io_in8(PORT_DAC_DATA);
    } while (--bytes);
}

/*
 * 0x1c0ff
 *
 * Turn 640 pixels of one byte each into the eight bitplanes of an ILBM row,
 * 0x50 bytes apiece, plane 0 first. Eight pixels at a time: each is shifted
 * out a bit at a time, low bit first, and bit `k` is rotated into the byte
 * for plane `k`, so the first pixel ends in the top bit. The original holds
 * the eight plane bytes in AH, AL, BH, BL, CH, CL, DH and DL, in that order.
 */
void chunky_to_planar(const uint8_t *src, uint8_t *dst)
{
    uint8_t plane[8] = { 0 };           /* shifted out whole; the registers'
                                           old values never reach a plane */
    int16_t n;
    int16_t j;
    int16_t k;

    for (n = 0x50; n != 0; n--) {
        for (j = 0; j < 8; j++)
            for (k = 0; k < 8; k++)
                plane[k] = (uint8_t)((plane[k] << 1) | ((src[j] >> k) & 1));
        src += 8;
        for (k = 0; k < 8; k++)
            dst[k * 0x50] = plane[k];
        dst++;
    }
}
#endif
