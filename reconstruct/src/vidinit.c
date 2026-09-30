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
 * `g_vm_start`, and it starts a byte after the divide trap's: the pad at
 * 0x48f1 is the word alignment of a new module's data.
 *
 * **C with inline `asm`**, Borland C++ 3.0 `-mm -k-` through TASM: each
 * routine is a C function whose body is `asm` statements, and what the
 * compiler writes itself is the image's - the frame of a function with
 * parameters, the SI and DI it saves for an `asm` that names them, each
 * final return. `restore_video_mode` and `bios_video_kind` take nothing and
 * still have a frame, so they sit in `#pragma option -k`; `vm_init` saves
 * an SI it never uses, a `register` variable nothing reads. The routines
 * the `asm` reaches with `call` are functions of their own, since a call to
 * a C label is not something the compiler resolves: `crtc_present`, which
 * answers in the carry flag, and `set_colour_text_mode`, the two a host
 * `detect_adapter` does inline. The functions are in address order and
 * each carries the image offset it was read from.
 *
 * Where the module begins is not settled. 0x2241b..0x22483, the two clipped
 * pixel routines and the `vm_restore_rect` thunk, is assembly too and has no
 * data of its own, so nothing says whether it ends the divide trap's module or
 * starts this one; it stays in lowlevel.c.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -k- -O -Z
 * JUDGE: via-assembler
 * JUDGE: data 0x48f2..0x48f8
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

/*
 * **This module's `_DATA`**, DGROUP 0x48f2..0x48f8: nothing recorded yet,
 * nothing forced, and no driver.
 */
struct vm_start g_vm_start = { 0xff, 0xff };
/* 0x2410d */
uint16_t vm_init(uint16_t adapter, uint16_t unused, FILE *file)
{
    register int held;

    asm mov al, byte ptr [bp+6]
    asm mov byte ptr g_vm_start+1, al
    asm xor ax, ax
    asm mov byte ptr g_vmds+6e8h, al
    asm mov byte ptr g_vmds+1fh, al
    asm mov word ptr g_vmds+6eah, 140h
    asm mov word ptr g_vmds+6ech, 0c8h
    asm mov ax, word ptr g_vmds+19eh
    asm mov dx, word ptr g_vmds+1a0h
    asm mov bx, ax
    asm or bx, dx
    asm je L224c1
    asm push dx
    asm push ax
    asm call far ptr dos_free_far
    asm add sp, 4
    asm xor ax, ax
    asm mov word ptr g_vmds+19eh, ax
    asm mov word ptr g_vmds+1a0h, ax
L224c1:
    asm call near ptr bios_video_kind
    asm mov byte ptr g_vm_start, al
    asm call near ptr detect_adapter
    asm mov byte ptr g_vmds+1dh, al
    asm or ax, ax
    asm je L2251c
    asm push word ptr [bp+0ah]
    asm push ax
    asm call far ptr load_video_driver
    asm add sp, 4
    asm or dx, dx
    asm je L2251c
    asm mov word ptr g_vm_start+2, ax
    asm mov word ptr g_vm_start+4, dx
    asm push ds
    asm mov ax, 4412h
    asm push ax
    asm mov ax, 3890h
    asm push ax
    asm call dword ptr g_vm_start+2
    asm add sp, 6
    asm mov di, offset g_vm_driver+4h
    asm push ds
    asm mov ax, ds
    asm mov ds, dx
    asm mov es, ax
    asm mov ax, 32h
    asm mov cx, ax
    asm shl cx, 1
    asm rep movsw
    asm pop ds
    asm mov di, offset g_vm_driver+4h
    asm mov ax, dx
    asm mov cx, 32h
L22514:
    asm add di, 2
    asm stosw
    asm loop L22514
    asm jmp short L22521
L2251c:
    asm mov byte ptr g_vmds+1dh, 0
L22521:
    asm xor ax, ax
    asm mov es, ax
    asm mov ax, ds
    asm mov word ptr es:[4f0h], ax
    asm mov ax, word ptr g_vmds+14h
    asm mov word ptr g_vmds+16h, ax
    asm mov ax, word ptr g_vmds+12h
    asm mov word ptr g_vmds+18h, ax
    asm mov al, byte ptr g_vmds+1dh
    asm xor ah, ah
    asm push ax
    asm or ax, ax
    asm je L225a0
    asm mov ax, word ptr g_vm_driver
    asm or ax, ax
    asm je L22555
    asm xor bx, bx
    asm dec ax
    asm push ax
    asm push bx
    asm call far ptr dos_free_far
    asm add sp, 4
L22555:
    asm mov ax, word ptr g_vmds+6ech
    asm shl ax, 1
    asm shl ax, 1
    asm add ax, 20h
    asm xor bx, bx
    asm push bx
    asm push bx
    asm push bx
    asm push ax
    asm call far ptr dos_alloc_bytes
    asm add sp, 8
    asm or dx, dx
    asm je L225a0
    asm inc dx
    asm mov word ptr g_vm_driver, dx
    asm mov ax, 1130h
    asm mov bh, 3
    asm int 10h
    asm mov bx, offset g_font_bodies
    asm mov word ptr [bx], bp
    asm mov word ptr [bx+2], es
    asm mov word ptr [bx+4], bp
    asm mov word ptr [bx+6], es
    asm mov ax, 808h
    asm mov word ptr g_vmds+48h, ax
    asm mov word ptr g_vmds+34h, ax
    asm mov ax, 0
    asm mov word ptr g_vmds+5ch, ax
    asm mov ax, 0ffffh
    asm mov word ptr g_vmds+70h, ax
L225a0:
    asm pop ax
}

/* 0x2422f */
void shutdown_input(void)
{
    asm call far ptr remove_keyboard
    asm call far ptr remove_mouse
    asm call far ptr timer_remove
    asm call far ptr restore_int0_vector
}

#pragma option -k
/* 0x24244 */
void restore_video_mode(void)
{
    asm xor ax, ax
    asm mov al, byte ptr g_vm_start
    asm cmp al, 0ffh
    asm je L225d0
    asm push ax
    asm call near ptr set_bios_video_mode
    asm pop ax
    asm mov byte ptr g_vm_start, 0ffh
L225d0:
    ;
}
#pragma option -k-

/* 0x2425c */
uint16_t near detect_adapter(void)
{
    asm mov al, byte ptr g_vm_start+1
    asm cmp word ptr g_vm_driver+2h, 0
    asm jne L225df
    asm xor ah, ah
    asm ret
L225df:
    asm or al, al
    asm je L2261a
    asm cmp al, 9
    asm je L22612
    asm cmp al, 0ah
    asm je L2261a
    asm cmp al, 8
    asm je L2261a
    asm cmp al, 0dh
    asm je L2261a
    asm cmp al, 0ch
    asm je L2261a
    asm cmp al, 0eh
    asm je L2261a
    asm cmp al, 0fh
    asm je L2261a
    asm cmp al, 5
    asm je L2264b
    asm cmp al, 2
    asm je L22682
    asm cmp al, 7
    asm je L22682
    asm cmp al, 0bh
    asm je L22682
    asm jmp L226ab
L22612:
    asm call near ptr set_colour_text_mode
    asm mov al, 9
    asm jmp L22724
L2261a:
    asm cmp byte ptr g_vm_driver+2h, 0
    asm mov ax, 1a00h
    asm int 10h
    asm cmp bl, 7
    asm je L2263d
    asm cmp bl, 8
    asm je L2263d
    asm cmp bh, 7
    asm je L2263a
    asm cmp bh, 8
    asm je L2263a
    asm jne L2264b
L2263a:
    asm call near ptr set_colour_text_mode
L2263d:
    asm mov al, byte ptr g_vm_start+1
    asm xor ah, ah
    asm or ax, ax
    asm jne L22648
    asm mov al, 8
L22648:
    asm jmp L22724
L2264b:
    asm mov ax, 1a00h
    asm int 10h
    asm cmp bl, 7
    asm je L2267d
    asm cmp bl, 8
    asm je L2267d
    asm cmp bh, 7
    asm je L2267a
    asm cmp bh, 8
    asm je L2267a
    asm cmp bl, 0bh
    asm je L2267d
    asm cmp bl, 0ch
    asm je L2267d
    asm cmp bh, 0bh
    asm je L2267a
    asm cmp bh, 0ch
    asm je L2267a
    asm jne L22682
L2267a:
    asm call near ptr set_colour_text_mode
L2267d:
    asm mov al, 5
    asm jmp L22724
L22682:
    asm mov ax, 40h
    asm mov es, ax
    asm mov ah, 12h
    asm mov bx, 10h
    asm int 10h
    asm cmp bx, 10h
    asm je L226ab
    asm mov bx, 87h
    asm mov al, byte ptr es:[bx]
    asm and al, 8
    asm jne L226a3
    asm mov al, byte ptr g_vm_start+1
    asm jmp L22724
L226a3:
    asm call near ptr set_colour_text_mode
    asm mov al, byte ptr g_vm_start+1
    asm jmp short L22724
L226ab:
    asm mov al, byte ptr g_vm_start+1
    asm or al, al
    asm je L226ba
    asm cmp al, 1
    asm je L226ba
    asm cmp al, 3
    asm jne L226f7
L226ba:
    asm mov dx, 3d4h
    asm mov al, 0fh
    asm out dx, al
    asm inc dx
    asm mov ah, al
    asm mov al, 66h
    asm out dx, al
    asm mov cx, 64h
L226c9:
    asm nop
    asm loop L226c9
    asm in al, dx
    asm xchg ah, al
    asm out dx, al
    asm cmp ah, 66h
    asm jne L226f7
    asm call near ptr set_colour_text_mode
    asm call far ptr detect_pcjr
    asm cmp byte ptr g_vm_start+1, 1
    asm je L226ec
    asm or al, al
    asm je L226ec
    asm mov al, 3
    asm jmp short L22724
L226ec:
    asm cmp byte ptr g_vm_start+1, 3
    asm je L22722
    asm mov al, 1
    asm jmp short L22724
L226f7:
    asm mov al, byte ptr g_vm_start+1
    asm or al, al
    asm je L22702
    asm cmp al, 4
    asm jne L22722
L22702:
    asm mov dx, 3b4h
    asm call near ptr crtc_present
    asm jb L22722
    asm mov dl, 0bah
    asm in al, dx
    asm and al, 80h
    asm mov ah, al
    asm mov cx, 8000h
L22714:
    asm in al, dx
    asm and al, 80h
    asm cmp ah, al
    asm loope L22714
    asm je L22722
    asm mov ax, 4
    asm jmp short L22724
L22722:
    asm xor al, al
L22724:
    asm xor ah, ah
}

/* 0x22727 */
void near crtc_present(void)
{
    asm mov al, 0fh
    asm out dx, al
    asm inc dx
    asm in al, dx
    asm mov ah, al
    asm mov al, 66h
    asm out dx, al
    asm mov cx, 100h
L22734:
    asm loop L22734
    asm in al, dx
    asm xchg ah, al
    asm out dx, al
    asm cmp ah, 66h
    asm je L22740
    asm stc
L22740:
    ;
}

/* 0x243cb */
void near set_bios_video_mode(uint16_t bits)
{
    asm mov ax, 40h
    asm mov es, ax
    asm mov ax, word ptr [bp+4]
    asm mov cl, 4
    asm shl ax, cl
    asm mov bx, 10h
    asm and byte ptr es:[bx], 0cfh
    asm or byte ptr es:[bx], al
    asm mov ax, 3
    asm mov bx, 3
    asm int 10h
}

#pragma option -k
/* 0x243ee */
uint16_t near bios_video_kind(void)
{
    asm mov ax, 40h
    asm mov es, ax
    asm mov bx, 10h
    asm mov al, byte ptr es:[bx]
    asm and al, 30h
    asm mov cl, 4
    asm shr al, cl
    asm xor ah, ah
}
#pragma option -k-

/* 0x2277c */
void near set_colour_text_mode(void)
{
    asm mov bx, 10h
    asm and byte ptr es:[bx], 0cfh
    asm or byte ptr es:[bx], 20h
    asm mov ax, 3
    asm mov bx, 3
    asm int 10h
}
#else

/*
 * **This module's `_DATA`**, DGROUP 0x48f2..0x48f8: nothing recorded yet,
 * nothing forced, and no driver.
 */
struct vm_start g_vm_start = {
    .mode_found = 0xff,
    .mode_forced = 0xff,
};

/*
 * 0x2410d
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
     * looks like it - see `io_bios_font`.
     */
    uint8_t *font;
    uint16_t al;
    uint16_t r;

    (void)unused;

    g_vm_start.mode_forced = (uint8_t)adapter;
    g_vmds.screen.mode_kind = 0;
    g_vmds.vga_chunks = 0;
    g_vmds.screen.screen_width = 0x140;
    g_vmds.screen.screen_height = 0xc8;

    if (g_vmds.palettes.blocks[0] != NULL) {
        dos_free_far(g_vmds.palettes.blocks[0]);
        g_vmds.palettes.blocks[0] = NULL;
    }

    g_vm_start.mode_found = (uint8_t)bios_video_kind();

    al = detect_adapter() & 0xff;
    g_vmds.pixel_shift = (uint8_t)al;

    if (al != 0) {
        uint8_t *p = load_video_driver((int16_t)al, (char *)file);

        /* Only DX is tested. */
        if (FP_SEG(p) == 0) {
            g_vmds.pixel_shift = 0;
        } else {
            int16_t i;

            g_vm_start.driver = p;

            vm_driver_init(&g_vmds, g_vm_hooks.driver_table, DGROUP_SEG);

            /* The driver's fifty entry points: the original copies the
               offsets from the driver's table at its 0x13e and puts the
               driver's segment beside each. The port runs its own routine
               for each driver entry, so a slot is that routine. */
            for (i = 0; i < 0x32; i++)
                g_vm_driver.entry[i] = vm_vector_host(i);
        }
    } else {
        g_vmds.pixel_shift = 0;
    }

    g_bios.intra_app[0] = DGROUP_SEG;

    g_vmds.page_src = ((int16_t)g_vmds.page_front);
    g_vmds.page_dst = ((int16_t)g_vmds.page_back);

    r = ((uint8_t)g_vmds.pixel_shift);
    if (r == 0)
        goto out;

    if (g_vm_driver.span_buffer_seg != 0)
        dos_free_far(MK_FP(g_vm_driver.span_buffer_seg - 1, 0));

    {
        uint8_t *p = dos_alloc_bytes((uint16_t)(((uint16_t)g_vmds.screen.screen_height) * 4 + 0x20), 0);

        /* Only the segment is kept, and tested: `or dx,dx`. */
        if (FP_SEG(p) == 0)
            goto out;

        g_vm_driver.span_buffer_seg = FP_SEG(p) + 1;
    }

    /*
     * `mov ax,0x1130 / mov bh,3 / int 0x10`, and the answer is in **ES:BP** -
     * a far pointer, filed twice. The emulator does not implement the call,
     * which is why it comes back null; see `io_bios_font`.
     */
    font = io_bios_font(3);

    g_font_bodies.body[0] = font;
    g_font_bodies.body[1] = font;

    *(int16_t *)(&g_vmds.font_cell_height[0]) = 0x808;
    *(int16_t *)(&g_vmds.font_cell_width[0]) = 0x808;
    *(int16_t *)(&g_vmds.font_first_char[0]) = 0;
    *(int16_t *)(&g_vmds.font_char_count[0]) = (int16_t)0xffff;

out:
    return r;
}

/*
 * 0x2422f
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
 * 0x24244
 *
 * Put the adapter back in the mode the program found it in. DGROUP 0x48f2 is
 * that mode, and 0xff means it was never recorded - which is why the store of
 * 0xff afterwards is inside the test and not after it: having restored the
 * mode, the record is spent.
 */
void restore_video_mode(void)
{
    uint16_t mode = g_vm_start.mode_found;

    if (mode != 0xff) {
        set_bios_video_mode(mode);
        g_vm_start.mode_found = 0xff;
    }
}

/*
 * 0x2425c
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
    uint8_t al = g_vm_start.mode_forced;

    if (g_vm_driver.detect_allowed == 0)
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

    al = g_vm_start.mode_forced;
    if (al == 0)
        al = 8;

    return al;
}

/*
 * 0x243cb
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
    uint8_t eq = g_bios.equipment;

    g_bios.equipment = (uint8_t)((eq & 0xcf)
                                 | (uint8_t)((equipment_bits << 4) & 0x30));

    io_bios_set_mode(3);
}

/*
 * 0x243ee
 *
 * The BIOS video mode the machine booted in, as bits 4 and 5 of the equipment
 * word at 0040:0010 shifted down - so 0 to 3, of which 3 is monochrome.
 *
 * The port reads it out of `g_bios`, the BIOS data area.
 */
uint16_t bios_video_kind(void)
{
    return (uint16_t)((g_bios.equipment & 0x30) >> 4);
}
#endif
