/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Bringing the video up and handing the machine back**: `vm_init`, which
 * picks the adapter and loads and starts its driver, the adapter probe it
 * calls, and the routines that put the keyboard, mouse, timer and video mode
 * back on the way out.
 *
 * A module of the original's **code segment 1c25**, image 0x22483..0x22790,
 * split out of engine.c on 2026-09-27. Its `_DATA` is DGROUP 0x48f2..0x48f8,
 * `VM_START`, and it starts a byte after the divide trap's: the pad at
 * 0x48f1 is the word alignment of a new module's data.
 *
 * **So it is TASM source**, the `#ifdef __TURBOC__` block below, with the
 * host's transcription in the `#else`. Every routine is hand-written:
 * `detect_adapter` answers in AL and calls two near helpers, one answering
 * in the carry flag; `vm_init` copies the driver's table with `rep movsw`
 * across a borrowed DS; `restore_video_mode` pushes its argument and pops it
 * back. The functions are in address order and each carries the image offset it was
 * read from.
 *
 * Where the module begins is not settled. 0x2241b..0x22483, the two clipped
 * pixel routines and the `vm_restore_rect` thunk, is assembly too and has no
 * data of its own, so nothing says whether it ends the divide trap's module or
 * starts this one; it stays in engine.c.
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
_DATA segment word public 'DATA'
d_48f2 label byte
        db 0ffh
d_48f3 label byte
        db 0ffh
d_48f4 label byte
        db 0h, 0h
d_48f6 label byte
        db 0h, 0h
_DATA ends

extrn _detect_pcjr:far
extrn _dos_alloc_bytes:far
extrn _dos_free_far:far
extrn _load_video_driver:far
extrn _remove_keyboard:far
extrn _remove_mouse:far
extrn _restore_int0_vector:far
extrn _timer_remove:far
extrn _DG4342:byte
extrn _ENGINE_FONT_BODIES:byte
extrn _VMDS:byte
VIDINIT_TEXT segment byte public 'CODE'
assume cs:VIDINIT_TEXT, ds:DGROUP
public _vm_init, _shutdown_input, _restore_video_mode, _detect_adapter
public _set_bios_video_mode, _bios_video_kind

/* 0x22483 */
_vm_init proc far
        push bp
        mov bp, sp
        push si
        push di
        mov al, byte ptr [bp+6]
        mov byte ptr DGROUP:d_48f3, al
        xor ax, ax
        mov byte ptr DGROUP:_VMDS+6e8h, al
        mov byte ptr DGROUP:_VMDS+1fh, al
        mov word ptr DGROUP:_VMDS+6eah, 140h
        mov word ptr DGROUP:_VMDS+6ech, 0c8h
        mov ax, word ptr DGROUP:_VMDS+19eh
        mov dx, word ptr DGROUP:_VMDS+1a0h
        mov bx, ax
        or bx, dx
        je L224c1
        push dx
        push ax
        call FAR PTR _dos_free_far
        add sp, 4
        xor ax, ax
        mov word ptr DGROUP:_VMDS+19eh, ax
        mov word ptr DGROUP:_VMDS+1a0h, ax
L224c1:
        call _bios_video_kind
        mov byte ptr DGROUP:d_48f2, al
        call _detect_adapter
        mov byte ptr DGROUP:_VMDS+1dh, al
        or ax, ax
        je L2251c
        push word ptr [bp+0ah]
        push ax
        call FAR PTR _load_video_driver
        add sp, 4
        or dx, dx
        je L2251c
        mov word ptr DGROUP:d_48f4, ax
        mov word ptr DGROUP:d_48f6, dx
        push ds
        mov ax, 4412h
        push ax
        mov ax, 3890h
        push ax
        call dword ptr DGROUP:d_48f4
        add sp, 6
        mov di, offset DGROUP:_DG4342+4h
        push ds
        mov ax, ds
        mov ds, dx
        mov es, ax
        mov ax, 32h
        mov cx, ax
        shl cx, 1
        rep movsw
        pop ds
        mov di, offset DGROUP:_DG4342+4h
        mov ax, dx
        mov cx, 32h
L22514:
        add di, 2
        stosw
        loop L22514
        jmp short L22521
L2251c:
        mov byte ptr DGROUP:_VMDS+1dh, 0
L22521:
        xor ax, ax
        mov es, ax
        mov ax, ds
        mov word ptr es:[4f0h], ax
        mov ax, word ptr DGROUP:_VMDS+14h
        mov word ptr DGROUP:_VMDS+16h, ax
        mov ax, word ptr DGROUP:_VMDS+12h
        mov word ptr DGROUP:_VMDS+18h, ax
        mov al, byte ptr DGROUP:_VMDS+1dh
        xor ah, ah
        push ax
        or ax, ax
        je L225a0
        mov ax, word ptr DGROUP:_DG4342
        or ax, ax
        je L22555
        xor bx, bx
        dec ax
        push ax
        push bx
        call FAR PTR _dos_free_far
        add sp, 4
L22555:
        mov ax, word ptr DGROUP:_VMDS+6ech
        shl ax, 1
        shl ax, 1
        add ax, 20h
        xor bx, bx
        push bx
        push bx
        push bx
        push ax
        call FAR PTR _dos_alloc_bytes
        add sp, 8
        or dx, dx
        je L225a0
        inc dx
        mov word ptr DGROUP:_DG4342, dx
        mov ax, 1130h
        mov bh, 3
        int 10h
        mov bx, offset DGROUP:_ENGINE_FONT_BODIES
        mov word ptr [bx], bp
        mov word ptr [bx+2], es
        mov word ptr [bx+4], bp
        mov word ptr [bx+6], es
        mov ax, 808h
        mov word ptr DGROUP:_VMDS+48h, ax
        mov word ptr DGROUP:_VMDS+34h, ax
        mov ax, 0
        mov word ptr DGROUP:_VMDS+5ch, ax
        mov ax, 0ffffh
        mov word ptr DGROUP:_VMDS+70h, ax
L225a0:
        pop ax
        pop di
        pop si
        pop bp
        retf
_vm_init endp

/* 0x225a5 */
_shutdown_input proc far
        call FAR PTR _remove_keyboard
        call FAR PTR _remove_mouse
        call FAR PTR _timer_remove
        call FAR PTR _restore_int0_vector
        retf
_shutdown_input endp

/* 0x225ba */
_restore_video_mode proc far
        push bp
        mov bp, sp
        xor ax, ax
        mov al, byte ptr DGROUP:d_48f2
        cmp al, 0ffh
        je L225d0
        push ax
        call _set_bios_video_mode
        pop ax
        mov byte ptr DGROUP:d_48f2, 0ffh
L225d0:
        pop bp
        retf
_restore_video_mode endp

/* 0x225d2 */
_detect_adapter proc near
        mov al, byte ptr DGROUP:d_48f3
        cmp word ptr DGROUP:_DG4342+2h, 0
        jne L225df
        xor ah, ah
        ret
L225df:
        or al, al
        je L2261a
        cmp al, 9
        je L22612
        cmp al, 0ah
        je L2261a
        cmp al, 8
        je L2261a
        cmp al, 0dh
        je L2261a
        cmp al, 0ch
        je L2261a
        cmp al, 0eh
        je L2261a
        cmp al, 0fh
        je L2261a
        cmp al, 5
        je L2264b
        cmp al, 2
        je L22682
        cmp al, 7
        je L22682
        cmp al, 0bh
        je L22682
        jmp L226ab
L22612:
        call L2277c
        mov al, 9
        jmp L22724
L2261a:
        cmp byte ptr DGROUP:_DG4342+2h, 0
        mov ax, 1a00h
        int 10h
        cmp bl, 7
        je L2263d
        cmp bl, 8
        je L2263d
        cmp bh, 7
        je L2263a
        cmp bh, 8
        je L2263a
        jne L2264b
L2263a:
        call L2277c
L2263d:
        mov al, byte ptr DGROUP:d_48f3
        xor ah, ah
        or ax, ax
        jne L22648
        mov al, 8
L22648:
        jmp L22724
L2264b:
        mov ax, 1a00h
        int 10h
        cmp bl, 7
        je L2267d
        cmp bl, 8
        je L2267d
        cmp bh, 7
        je L2267a
        cmp bh, 8
        je L2267a
        cmp bl, 0bh
        je L2267d
        cmp bl, 0ch
        je L2267d
        cmp bh, 0bh
        je L2267a
        cmp bh, 0ch
        je L2267a
        jne L22682
L2267a:
        call L2277c
L2267d:
        mov al, 5
        jmp L22724
L22682:
        mov ax, 40h
        mov es, ax
        mov ah, 12h
        mov bx, 10h
        int 10h
        cmp bx, 10h
        je L226ab
        mov bx, 87h
        mov al, byte ptr es:[bx]
        and al, 8
        jne L226a3
        mov al, byte ptr DGROUP:d_48f3
        jmp L22724
L226a3:
        call L2277c
        mov al, byte ptr DGROUP:d_48f3
        jmp short L22724
L226ab:
        mov al, byte ptr DGROUP:d_48f3
        or al, al
        je L226ba
        cmp al, 1
        je L226ba
        cmp al, 3
        jne L226f7
L226ba:
        mov dx, 3d4h
        mov al, 0fh
        out dx, al
        inc dx
        mov ah, al
        mov al, 66h
        out dx, al
        mov cx, 64h
L226c9:
        nop
        loop L226c9
        in al, dx
        xchg ah, al
        out dx, al
        cmp ah, 66h
        jne L226f7
        call L2277c
        call FAR PTR _detect_pcjr
        cmp byte ptr DGROUP:d_48f3, 1
        je L226ec
        or al, al
        je L226ec
        mov al, 3
        jmp short L22724
L226ec:
        cmp byte ptr DGROUP:d_48f3, 3
        je L22722
        mov al, 1
        jmp short L22724
L226f7:
        mov al, byte ptr DGROUP:d_48f3
        or al, al
        je L22702
        cmp al, 4
        jne L22722
L22702:
        mov dx, 3b4h
        call L22727
        jb L22722
        mov dl, 0bah
        in al, dx
        and al, 80h
        mov ah, al
        mov cx, 8000h
L22714:
        in al, dx
        and al, 80h
        cmp ah, al
        loope L22714
        je L22722
        mov ax, 4
        jmp short L22724
L22722:
        xor al, al
L22724:
        xor ah, ah
        ret
L22727:
        mov al, 0fh
        out dx, al
        inc dx
        in al, dx
        mov ah, al
        mov al, 66h
        out dx, al
        mov cx, 100h
L22734:
        loop L22734
        in al, dx
        xchg ah, al
        out dx, al
        cmp ah, 66h
        je L22740
        stc
L22740:
        ret
_detect_adapter endp

/* 0x22741 */
_set_bios_video_mode proc near
        push bp
        mov bp, sp
        mov ax, 40h
        mov es, ax
        mov ax, word ptr [bp+4]
        mov cl, 4
        shl ax, cl
        mov bx, 10h
        and byte ptr es:[bx], 0cfh
        or byte ptr es:[bx], al
        mov ax, 3
        mov bx, 3
        int 10h
        pop bp
        ret
_set_bios_video_mode endp

/* 0x22764 */
_bios_video_kind proc near
        push bp
        mov bp, sp
        mov ax, 40h
        mov es, ax
        mov bx, 10h
        mov al, byte ptr es:[bx]
        and al, 30h
        mov cl, 4
        shr al, cl
        xor ah, ah
        pop bp
        ret
L2277c:
        mov bx, 10h
        and byte ptr es:[bx], 0cfh
        or byte ptr es:[bx], 20h
        mov ax, 3
        mov bx, 3
        int 10h
        ret
_bios_video_kind endp
VIDINIT_TEXT ends
}
#else

/*
 * **This module's `_DATA`**, DGROUP 0x48f2..0x48f8: nothing recorded yet,
 * nothing forced, and no driver.
 */
struct vm_start VM_START DGROUP_AT(0x48f2) = {
    .mode_found = 0xff,
    .mode_forced = 0xff,
};

/*
 * 0x22483
 *
 * Bring the video up: pick the adapter, load its driver, start it, and build
 * the far vector table every drawing call goes through. Answers the adapter
 * code, or 0 if there is none or the driver would not load.
 *
 * The table at DGROUP 0x4346 is built in two passes. The driver's start-up
 * answers `DX:SI` pointing at its own table of near offsets; 0x64 words of that
 * are copied in, and then the driver's segment is written into **every second
 * word** 0x32 times. Both counts say fifty entries, which is what the table
 * actually is.
 *
 * DGROUP's own segment is planted at 0000:04f0 on the way past, where anything
 * that needs to find the program's data can read it.
 *
 * **The 8x8 font pointer is not what it looks like.** `INT 10h AX=1130 BH=3`
 * is the BIOS "get font pointer" call and it answers in **ES:BP**, so the two
 * pairs the game files here are that answer and not, as this note once said,
 * the routine's own frame pointer.
 *
 * Nothing implements it on either side. The emulator leaves the registers as
 * it found them, so it answers `0000:ffca` - which is BP, which is the frame
 * pointer, which is why the mistake was easy to make - and the port answers a
 * plain zero, because a font pointer aimed at the stack is an accident rather
 * than a behaviour to reproduce. Real fonts are a separate piece of work.
 *
 * So the two sides differ by four bytes at DGROUP 0x618a and 0x618e, on
 * purpose. `vm_init`'s spec carries `deviation` so a sweep says so, and
 * STATUS.md records it.
 */
uint16_t vm_init(uint16_t adapter, uint16_t unused, FILE *file)
{
    /*
     * **No frame.** The prologue at 0x22483 is `push bp / mov bp,sp / push si
     * / push di` with no `sub sp` at all: the four bytes are SI and DI, and
     * saving registers is not something the port has to model.
     *
     * The font pointer below is not this routine's BP either, however much it
     * looks like it - see `io_bios_font_ptr`.
     */
    struct bios_font_ptr font;
    uint16_t al;
    uint16_t r;

    (void)unused;

    VM_START.mode_forced = (uint8_t)adapter;
    VMDS.screen.mode_kind = 0;
    VMDS.vga_chunks = 0;
    VMDS.screen.screen_width = 0x140;
    VMDS.screen.screen_height = 0xc8;

    if (dg_far_ptr(VMDS.palettes.blocks[0]) != FAR_NULL_PTR) {
        dos_free_far(dg_far_ptr(VMDS.palettes.blocks[0]));
        VMDS.palettes.blocks[0] = FAR_NULL;
    }

    VM_START.mode_found = (uint8_t)bios_video_kind();

    al = detect_adapter() & 0xff;
    VMDS.pixel_shift = (uint8_t)al;

    if (al != 0) {
        uint8_t *p = load_video_driver((int16_t)al, (char *)file);

        /* Only DX is tested. */
        if (FP_SEG(p) == 0) {
            VMDS.pixel_shift = 0;
        } else {
            uint16_t seg;
            int16_t i;

            VM_START.driver = far_of(p);

            vm_driver_init(&VMDS, DG440E.driver_table, DGROUP_SEG);
            seg = VM_START.driver.seg;

            /* a hundred words of the driver's table at its 0x13e, word by
               word, and then the driver's segment over every second one */
            {
                const int16_t far *table = (const int16_t far *)(void *)
                    MK_FP(seg, 0x13e);

                for (i = 0; i < 0x64; i++)
                    ((int16_t *)DG4342.font)[i] = table[i];
            }

            for (i = 0; i < 0x32; i++)
                DG4342.font[i].seg = seg;
        }
    } else {
        VMDS.pixel_shift = 0;
    }

    *(uint16_t *)(guest_mem + 0x4f0) = DGROUP_SEG;

    VMDS.page_src_ptr = ((int16_t)VMDS.page_front_ptr);
    VMDS.page_dst_ptr = ((int16_t)VMDS.page_back_ptr);

    r = ((uint8_t)VMDS.pixel_shift);
    if (r == 0)
        goto out;

    if (DG4342.span_buffer_seg != 0)
        dos_free_far(MK_FP((uint16_t)(DG4342.span_buffer_seg - 1), 0));

    {
        uint8_t *p = dos_alloc_bytes((uint16_t)(((uint16_t)VMDS.screen.screen_height) * 4 + 0x20), 0, 0).ptr;

        /* Only the segment is kept, and tested: `or dx,dx`. */
        if (FP_SEG(p) == 0)
            goto out;

        DG4342.span_buffer_seg = (int16_t)(FP_SEG(p) + 1);
    }

    /*
     * `mov ax,0x1130 / mov bh,3 / int 0x10`, and the answer is in **ES:BP** -
     * so the four words are that pair, filed twice. The emulator does not
     * implement the call, which is why they come back zero; see
     * `io_bios_font_ptr`.
     */
    font = io_bios_font_ptr(3);

    ENGINE_FONT_BODIES.body[0] = MK_FP(font.es, font.bp);
    ENGINE_FONT_BODIES.body[1] = MK_FP(font.es, font.bp);

    *(int16_t *)(&VMDS.font_table_48[0]) = 0x808;
    *(int16_t *)(&VMDS.font_table_34[0]) = 0x808;
    *(int16_t *)(&VMDS.font_table_5c[0]) = 0;
    *(int16_t *)(&VMDS.font_table_70[0]) = (int16_t)0xffff;

out:
    return r;
}

/*
 * 0x225a5
 *
 * The four things that have to be handed back before the program can leave:
 * the keyboard, the mouse, the timer and vector 0. Four calls and nothing else.
 */
void shutdown_input(void)
{
    remove_keyboard();
    remove_mouse();
    timer_remove();
    restore_int0_vector();
}

/*
 * 0x225ba
 *
 * Put the adapter back in the mode the program found it in. DGROUP 0x48f2 is
 * that mode, and 0xff means it was never recorded - which is why the store of
 * 0xff afterwards is inside the test and not after it: having restored the
 * mode, the record is spent.
 */
void restore_video_mode(void)
{
    uint16_t mode = VM_START.mode_found;

    if (mode != 0xff) {
        set_bios_video_mode(mode);
        VM_START.mode_found = 0xff;
    }
}

/*
 * 0x225d2
 *
 * Decide which video adapter is there, and answer its code. Hand-written
 * assembly: no frame, the answer in `AL`.
 *
 * DGROUP 0x48f3 is a forced setting, and it is the answer on most paths - the
 * BIOS is asked only to confirm it, not to override it. A zero at DGROUP 0x4344
 * refuses to look at all and answers 0.
 *
 * Of the eight paths only one runs on these screens: 0x48f3 is 0xd, which
 * sends it to the `INT 10h AH=1Ah` display-combination call, and a BL of 7 or
 * 8 there - a monochrome or colour VGA - accepts the forced setting unchanged.
 *
 * The rest are transcribed as stubs, each measured as unreached: the EGA
 * information call at AH=12h, and two probes that write 0x66 to a CRTC register
 * and read it back to tell a real card from an absent one. Those two are the
 * only places in this routine that touch hardware directly, and reproducing
 * them would mean modelling a card that is not there.
 */
uint16_t detect_adapter(void)
{
    uint8_t al = VM_START.mode_forced;

    if (DG4342.detect_allowed == 0)
        return 0;

    if (al == 0)
        goto ask_dcc;

    switch (al) {
    case 9:
        not_transcribed("0x22612, the adapter path for a forced 9");
        return 0;
    case 0xa: case 8: case 0xd: case 0xc: case 0xe: case 0xf:
        goto ask_dcc;
    case 5:
        not_transcribed("0x2264b, the second display-combination path");
        return 0;
    case 2: case 7: case 0xb:
        not_transcribed("0x22682, the EGA information path");
        return 0;
    default:
        not_transcribed("0x226ab, the CRTC probes");
        return 0;
    }

ask_dcc:
    {
        uint16_t bx = io_bios_display_combination();

        if ((bx & 0xff) == 7 || (bx & 0xff) == 8) {
            /* accepted; fall through */
        } else if ((bx >> 8) == 7 || (bx >> 8) == 8) {
            not_transcribed("0x2263a, a second display of 7 or 8");
            return 0;
        } else {
            not_transcribed("0x2264b, reached from the first DCC call");
            return 0;
        }
    }

    al = VM_START.mode_forced;
    if (al == 0)
        al = 8;

    return al;
}

/*
 * 0x22741
 *
 * **Back to text.** The two bits at 4 and 5 of the BIOS equipment word at
 * 0040:0010 are set from the argument - that is the "initial video mode" the
 * BIOS boots with - and INT 10h AX=0x0003 puts the adapter in mode 3.
 *
 * BX is loaded with 3 as well, which mode 3 has no use for. Transcribed as the
 * mode set it is.
 *
 * Near and cdecl: `ret` with the argument at [bp+4].
 */
void set_bios_video_mode(uint16_t equipment_bits)
{
    uint8_t eq = FAR8(0x40, 0x10);

    FAR8(0x40, 0x10) = (uint8_t)((eq & 0xcf)
                                 | (uint8_t)((equipment_bits << 4) & 0x30));

    io_bios_set_mode(3);
}

/*
 * 0x22764
 *
 * The BIOS video mode the machine booted in, as bits 4 and 5 of the equipment
 * word at 0040:0010 shifted down - so 0 to 3, of which 3 is monochrome.
 *
 * The port reads that byte out of guest memory. The BIOS data area is at
 * absolute 0x400 and is part of what the verifier seeds and compares, so this
 * needs nothing invented.
 */
uint16_t bios_video_kind(void)
{
    return (uint16_t)((*MK_FP(0x40, 0x10) & 0x30) >> 4);
}
#endif
