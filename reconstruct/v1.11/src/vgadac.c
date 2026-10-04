/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The screenshot writer's assembly**: writing and reading the DAC, and
 * turning a row of pixels into bitplanes. This file corresponds to the
 * forty-fifth and last module of the original's code segment 172c, image
 * 0x1c087..0x1c251, which ends a byte into segment 1c25's first paragraph.
 *
 * **C with inline `asm`**, Borland C++ 3.0 `-mm -k-` through TASM: each
 * routine is a C function whose body is `asm` statements, and what the
 * compiler writes itself - the frame, the SI and DI it saves because the
 * `asm` names them, and each final return - is the image's, byte for byte.
 * `chunky_to_planar` borrows BP as a data register and keeps its loop count
 * on the stack inside that `asm`. The functions are in address order and
 * each carries the image offset it was read from, as everywhere else.
 *
 * Only screenshot.c calls into it, and nothing calls `vga_set_dac` at all.
 * The names are ours.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -k- -O -Z
 * JUDGE: via-assembler
 * JUDGE: assembler bc3.00
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * C with inline `asm`, compiled through TASM (`JUDGE: via-assembler`):
 * the frame, the SI/DI save and each final return are the compiler's.
 * Drafted by tools/asm2c.py.
 */
/* 0x1ec86 */
void vga_set_dac(const uint8_t *rgb, int16_t first, int16_t count)
{
    asm mov si, word ptr [bp+6]
    asm mov cx, word ptr [bp+0ah]
    asm mov ax, cx
    asm shl cx, 1
    asm add cx, ax
    asm mov bx, word ptr [bp+8]
    asm mov dx, 3dah
L1c09d:
    asm in al, dx
    asm and al, 8
    asm je L1c09d
    asm mov dx, 3c7h
    asm in al, dx
    asm and al, 3
    asm cmp al, 3
    asm je L1c0b0
    asm mov dx, 3c9h
    asm out dx, al
L1c0b0:
    asm mov dx, 3c8h
    asm mov ax, bx
    asm out dx, al
    asm mov dx, 3c9h
    asm cli
L1c0ba:
    asm lodsb
    asm out dx, al
    asm loop L1c0ba
    asm sti
}

/* 0x1ecc1 */
void vga_get_dac(uint8_t *rgb, int16_t first, int16_t count)
{
    asm push ds
    asm pop es
    asm mov di, word ptr [bp+6]
    asm mov cx, word ptr [bp+0ah]
    asm mov ax, cx
    asm shl cx, 1
    asm add cx, ax
    asm mov bx, word ptr [bp+8]
    asm mov dx, 3dah
L1c0da:
    asm in al, dx
    asm and al, 8
    asm je L1c0da
    asm mov dx, 3c7h
    asm in al, dx
    asm and al, 3
    asm or al, al
    asm je L1c0f0
    asm mov dx, 3c9h
    asm in al, dx
    asm mov dx, 3c7h
L1c0f0:
    asm mov ax, bx
    asm out dx, al
    asm mov dx, 3c9h
    asm cli
L1c0f7:
    asm in al, dx
    asm stosb
    asm loop L1c0f7
    asm sti
}

/* 0x1ecfe */
void chunky_to_planar(const uint8_t *src, uint8_t *dst)
{
    asm mov si, word ptr [bp+6]
    asm mov di, word ptr [bp+8]
    asm mov ax, 50h
    asm push ax
L1c10e:
    asm mov bp, word ptr [si]
    asm shr bp, 1
    asm rcl ah, 1
    asm shr bp, 1
    asm rcl al, 1
    asm shr bp, 1
    asm rcl bh, 1
    asm shr bp, 1
    asm rcl bl, 1
    asm shr bp, 1
    asm rcl ch, 1
    asm shr bp, 1
    asm rcl cl, 1
    asm shr bp, 1
    asm rcl dh, 1
    asm shr bp, 1
    asm rcl dl, 1
    asm shr bp, 1
    asm rcl ah, 1
    asm shr bp, 1
    asm rcl al, 1
    asm shr bp, 1
    asm rcl bh, 1
    asm shr bp, 1
    asm rcl bl, 1
    asm shr bp, 1
    asm rcl ch, 1
    asm shr bp, 1
    asm rcl cl, 1
    asm shr bp, 1
    asm rcl dh, 1
    asm shr bp, 1
    asm rcl dl, 1
    asm mov bp, word ptr [si+2]
    asm shr bp, 1
    asm rcl ah, 1
    asm shr bp, 1
    asm rcl al, 1
    asm shr bp, 1
    asm rcl bh, 1
    asm shr bp, 1
    asm rcl bl, 1
    asm shr bp, 1
    asm rcl ch, 1
    asm shr bp, 1
    asm rcl cl, 1
    asm shr bp, 1
    asm rcl dh, 1
    asm shr bp, 1
    asm rcl dl, 1
    asm shr bp, 1
    asm rcl ah, 1
    asm shr bp, 1
    asm rcl al, 1
    asm shr bp, 1
    asm rcl bh, 1
    asm shr bp, 1
    asm rcl bl, 1
    asm shr bp, 1
    asm rcl ch, 1
    asm shr bp, 1
    asm rcl cl, 1
    asm shr bp, 1
    asm rcl dh, 1
    asm shr bp, 1
    asm rcl dl, 1
    asm mov bp, word ptr [si+4]
    asm shr bp, 1
    asm rcl ah, 1
    asm shr bp, 1
    asm rcl al, 1
    asm shr bp, 1
    asm rcl bh, 1
    asm shr bp, 1
    asm rcl bl, 1
    asm shr bp, 1
    asm rcl ch, 1
    asm shr bp, 1
    asm rcl cl, 1
    asm shr bp, 1
    asm rcl dh, 1
    asm shr bp, 1
    asm rcl dl, 1
    asm shr bp, 1
    asm rcl ah, 1
    asm shr bp, 1
    asm rcl al, 1
    asm shr bp, 1
    asm rcl bh, 1
    asm shr bp, 1
    asm rcl bl, 1
    asm shr bp, 1
    asm rcl ch, 1
    asm shr bp, 1
    asm rcl cl, 1
    asm shr bp, 1
    asm rcl dh, 1
    asm shr bp, 1
    asm rcl dl, 1
    asm mov bp, word ptr [si+6]
    asm shr bp, 1
    asm rcl ah, 1
    asm shr bp, 1
    asm rcl al, 1
    asm shr bp, 1
    asm rcl bh, 1
    asm shr bp, 1
    asm rcl bl, 1
    asm shr bp, 1
    asm rcl ch, 1
    asm shr bp, 1
    asm rcl cl, 1
    asm shr bp, 1
    asm rcl dh, 1
    asm shr bp, 1
    asm rcl dl, 1
    asm shr bp, 1
    asm rcl ah, 1
    asm shr bp, 1
    asm rcl al, 1
    asm shr bp, 1
    asm rcl bh, 1
    asm shr bp, 1
    asm rcl bl, 1
    asm shr bp, 1
    asm rcl ch, 1
    asm shr bp, 1
    asm rcl cl, 1
    asm shr bp, 1
    asm rcl dh, 1
    asm shr bp, 1
    asm rcl dl, 1
    asm push di
    asm add si, 8
    asm mov byte ptr [di], ah
    asm add di, 50h
    asm mov byte ptr [di], al
    asm add di, 50h
    asm mov byte ptr [di], bh
    asm add di, 50h
    asm mov byte ptr [di], bl
    asm add di, 50h
    asm mov byte ptr [di], ch
    asm add di, 50h
    asm mov byte ptr [di], cl
    asm add di, 50h
    asm mov byte ptr [di], dh
    asm add di, 50h
    asm mov byte ptr [di], dl
    asm pop di
    asm inc di
    asm pop ax
    asm dec ax
    asm push ax
    asm je L1c24c
    asm jmp L1c10e
L1c24c:
    asm pop ax
}
#else

/*
 * 0x1ec86
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
 * 0x1ecc1
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
 * 0x1ecfe
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
