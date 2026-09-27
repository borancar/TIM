/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The CRTC's split-screen registers**: Line Compare and the vertical
 * display end, each a ten-bit value spread over three registers.
 *
 * The original's **code segment 0000** (`_TEXT`), image 0x08f27..0x08fc3.
 * **Hand-written assembly**, and the TASM source is the `#ifdef __TURBOC__`
 * block below. Each routine saves SI and DI though it uses neither, saves
 * AX, BX and DX, and ends `mov sp,bp` with no locals to drop: BC++ 3.0 with
 * and without `-k`, and Turbo C++ 1.0x through TASM, were all tried with the
 * body as inline `asm` and with register variables and `_SI`/`_DI` to force
 * the saves, and none gives that prologue and that epilogue together.
 * **Nothing proves the module boundary on either side**.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm
 * JUDGE: via-assembler
 * JUDGE: assembler bc2.00
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

#ifdef __TURBOC__
/*
 * The module as TASM assembled it; the host's transcription is the
 * `#else`. See glue.c for how the block reaches the assembler.
 */
asm {
_TEXT segment byte public 'CODE'
assume cs:_TEXT, ds:DGROUP
public _vm_set_line_compare, _vm_set_display_lines

/* 0x08f27 */
_vm_set_line_compare proc far
        push bp
        mov bp, sp
        push si
        push di
        push ax
        push bx
        push dx
        mov bx, [bp+6]
        mov dx, 3d4h
        mov al, 18h
        out dx, al
        inc dx
        mov al, bl
        out dx, al
        dec dx
        mov al, 7
        out dx, al
        inc dx
        in al, dx
        and al, 0efh
        mov bl, bh
        and bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        or al, bl
        out dx, al
        dec dx
        mov al, 9
        out dx, al
        inc dx
        in al, dx
        and al, 0bfh
        mov bl, bh
        and bl, 2
        shl bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        or al, bl
        out dx, al
        pop dx
        pop bx
        pop ax
        pop di
        pop si
        mov sp, bp
        pop bp
        retf
_vm_set_line_compare endp

/* 0x08f77 */
_vm_set_display_lines proc far
        push bp
        mov bp, sp
        push si
        push di
        push ax
        push bx
        push dx
        mov bx, [bp+6]
        mov dx, 3d4h
        mov al, 15h
        out dx, al
        inc dx
        mov al, bl
        out dx, al
        dec dx
        mov al, 7
        out dx, al
        inc dx
        in al, dx
        and al, 0f7h
        mov bl, bh
        and bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        or al, bl
        out dx, al
        dec dx
        mov al, 9
        out dx, al
        inc dx
        in al, dx
        and al, 0dfh
        mov bl, bh
        and bl, 2
        shl bl, 1
        shl bl, 1
        shl bl, 1
        shl bl, 1
        or al, bl
        out dx, al
        pop dx
        pop bx
        pop ax
        pop di
        pop si
        mov sp, bp
        pop bp
        retf
_vm_set_display_lines endp
_TEXT ends
}
#else

/*
 * 0x08f27
 *
 * Program the CRTC's **Line Compare**, the split-screen line: from the scan
 * line it names down, the card stops following the start address and fetches
 * from offset 0 instead.
 *
 * Ten bits again, spread the way the hardware spreads them - the low eight in
 * Line Compare itself at index 0x18, bit 8 in Overflow bit 4, bit 9 in Maximum
 * Scan Line bit 6 - and the two high registers are read back and merged rather
 * than written whole, so the timing bits sharing them survive. Exactly the
 * shape of `vm_set_display_lines`, which does the same for the blanking line.
 *
 * The four `shl bl,1` in each half are a shift by four, and by five in the
 * second, written out because an 8086 has no shift by an immediate count.
 *
 * **This is why the game's screens are 368 rows.** It is called with 0x16f -
 * 367 - as the game screen is set up, alongside `vm_set_display_lines(0x1bf)`.
 * So the card is told to show 448 lines and to restart at address 0 after 368
 * of them: the picture is the first 368 rows and the last 80 are the *split
 * screen*, showing memory from the start of the plane. That band is not
 * leftover garbage in a page, and it is not the other page bleeding through -
 * it is a hardware feature this program uses, and anything composing a frame
 * has to honour it or the bottom eighty rows are wrong.
 */
void vm_set_line_compare(uint16_t line)
{
    uint8_t v;

    io_out8(PORT_CRTC_INDEX, 0x18);
    io_out8(PORT_CRTC_DATA, (uint8_t)(line & 0xFF));

    io_out8(PORT_CRTC_INDEX, 0x07);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xEF) | (((line >> 8) & 1) << 4));
    io_out8(PORT_CRTC_DATA, v);

    io_out8(PORT_CRTC_INDEX, 0x09);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xBF) | (((line >> 8) & 2) << 5));
    io_out8(PORT_CRTC_DATA, v);
}

/*
 * 0x08f77
 *
 * Program the CRTC to blank after `lines` scan lines. The count is ten bits
 * and the hardware spreads it over three registers: the low eight in Start
 * Vertical Blank, bit 8 in Overflow bit 3, bit 9 in Maximum Scan Line bit 5.
 * Overflow and Maximum Scan Line are read back first so the other timing bits
 * the BIOS put there survive.
 *
 * Called with 0x1d6 (470) for the Sierra logo and 0x18f (399) for the game's
 * own 640x400 screens. Vertical Display End is never touched, so the CRTC goes
 * on scanning 480 lines and simply blanks the tail - which is what lets two
 * 640x400 pages fit in one 64 KB plane.
 */
void vm_set_display_lines(uint16_t lines)
{
    uint8_t v;

    io_out8(PORT_CRTC_INDEX, 0x15);
    io_out8(PORT_CRTC_DATA, (uint8_t)(lines & 0xFF));

    io_out8(PORT_CRTC_INDEX, 0x07);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xF7) | (((lines >> 8) & 1) << 3));
    io_out8(PORT_CRTC_DATA, v);

    io_out8(PORT_CRTC_INDEX, 0x09);
    v = io_in8(PORT_CRTC_DATA);
    v = (uint8_t)((v & 0xDF) | (((lines >> 8) & 2) << 4));
    io_out8(PORT_CRTC_DATA, v);
}
#endif
