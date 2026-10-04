/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Detecting a PCjr, and clipping a polygon's outline to the clip box.**
 *
 * The original's **code segment 1c25**, image 0x20be0..0x21088, split out
 * of engine.c on 2026-09-27. Both routines are hand-written - no frames,
 * register answers - so it is TASM source, the `#ifdef __TURBOC__` block
 * below, with the host's transcription in the `#else`. Neither has data of
 * its own.
 *
 * **Possibly two modules.** Its end is proven: `install_keyboard` calls
 * `detect_pcjr` through TLINK's `nop / push cs / call`, so the keyboard
 * driver after it is another module. Its start is the first byte after
 * `blit_scaled_b`, a C routine. Nothing calls between the two routines here,
 * so nothing says whether they shared a file.
 *
 * JUDGE: built-with -mm
 * JUDGE: tasm
 * JUDGE: structs vmds=vmds
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
INCLUDE STRUCTS.ASH
extrn _g_vmds:byte
POLYCLIP_TEXT segment byte public 'CODE'
assume cs:POLYCLIP_TEXT, ds:DGROUP
public _detect_pcjr, _clip_polygon

/* 0x2286a */
_detect_pcjr proc far
        push es
        push bx
        mov bx, 0f000h
        mov es, bx
        mov bx, 0fffeh
        mov al, byte ptr es:[bx]
        cmp al, 0ffh
        jne pcjr_answer
        mov bx, 0c000h
        mov al, byte ptr es:[bx]
        cmp al, 21h
        jne pcjr_answer
        mov byte ptr DGROUP:_g_vmds+vmds_is_pcjr, 1
pcjr_answer:
        mov al, byte ptr DGROUP:_g_vmds+vmds_is_pcjr
        cbw
        pop bx
        pop es
        retf
_detect_pcjr endp

/* 0x22891 */
_clip_polygon proc far
        xor di, di
        mov ax, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        cmp ax, 1
        jg x_begin
        jmp clip_return
x_begin:
        dec ax
        mov bx, ax
        shl bx, 1
        xor cl, cl
        mov ax, word ptr _g_vmds[bx+vmds_poly_x]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        jge x_prev_coded
        or cl, 1
x_prev_coded:
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        jle x_start
        or cl, 2
x_start:
        xor si, si
x_edge:
        shl si, 1
        xor ch, ch
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        jge x_cur_coded
        or ch, 1
x_cur_coded:
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        jle x_classify
        or ch, 2
x_classify:
        mov al, cl
        or al, ch
        jne x_crossing
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        mov word ptr _g_vmds[di+vmds_work_x], ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp x_next
x_crossing:
        mov al, cl
        and al, ch
        je x_prev_in
        jmp x_next
x_prev_in:
        or cl, cl
        jne x_prev_out
        test ch, 1
        je x_leave_right
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[si+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, word ptr _g_vmds[si+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_poly_x]
        sub bp, word ptr _g_vmds[si+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp short x_left
x_leave_right:
        test ch, 2
        je x_left
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[si+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, word ptr _g_vmds[si+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_poly_x]
        sub bp, word ptr _g_vmds[si+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
x_left:
        jmp x_next
x_prev_out:
        or ch, ch
        je x_enter
        jmp short x_both_out
x_enter:
        test cl, 1
        je x_enter_right
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[bx+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        sub ax, word ptr _g_vmds[bx+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
        sub bp, word ptr _g_vmds[bx+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp short x_enter_keep
x_enter_right:
        test cl, 2
        je x_enter_keep
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[bx+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        sub ax, word ptr _g_vmds[bx+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
        sub bp, word ptr _g_vmds[bx+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
x_enter_keep:
        mov ax, word ptr _g_vmds[si+vmds_poly_x]
        mov word ptr _g_vmds[di+vmds_work_x], ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp x_next
x_both_out:
        test cl, 1
        je x_cross_from_right
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[bx+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        sub ax, word ptr _g_vmds[bx+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
        sub bp, word ptr _g_vmds[bx+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp short x_cross_to
x_cross_from_right:
        test cl, 2
        je x_cross_to
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[bx+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_poly_y]
        sub ax, word ptr _g_vmds[bx+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_poly_x]
        sub bp, word ptr _g_vmds[bx+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
x_cross_to:
        test ch, 1
        je x_cross_to_right
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_left
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[si+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, word ptr _g_vmds[si+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_poly_x]
        sub bp, word ptr _g_vmds[si+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
        jmp short x_next
x_cross_to_right:
        test ch, 2
        je x_next
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_right
        mov word ptr _g_vmds[di+vmds_work_x], ax
        sub ax, word ptr _g_vmds[si+vmds_poly_x]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_poly_y]
        sub ax, word ptr _g_vmds[si+vmds_poly_y]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_poly_x]
        sub bp, word ptr _g_vmds[si+vmds_poly_x]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_poly_y]
        mov word ptr _g_vmds[di+vmds_work_y], ax
        add di, 2
x_next:
        mov bx, si
        mov cl, ch
        shr si, 1
        inc si
        cmp si, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        je x_done
        jmp x_edge
x_done:
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count, ax
        cmp ax, 1
        jg y_begin
        jmp clip_copy_back
y_begin:
        dec ax
        mov bx, ax
        shl bx, 1
        xor cl, cl
        mov ax, word ptr _g_vmds[bx+vmds_work_y]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        jle y_prev_coded
        or cl, 4
y_prev_coded:
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        jge y_start
        or cl, 8
y_start:
        xor di, di
        mov si, di
y_edge:
        shl si, 1
        xor ch, ch
        mov ax, word ptr _g_vmds[si+vmds_work_y]
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        jle y_cur_coded
        or ch, 4
y_cur_coded:
        cmp ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        jge y_classify
        or ch, 8
y_classify:
        mov al, cl
        or al, ch
        jne y_crossing
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        mov ax, word ptr _g_vmds[si+vmds_work_y]
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        add di, 2
        jmp y_next
y_crossing:
        mov al, cl
        and al, ch
        je y_prev_in
        jmp y_next
y_prev_in:
        or cl, cl
        jne y_prev_out
        test ch, 8
        je y_leave_bottom
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[si+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
        jmp short y_left
y_leave_bottom:
        test ch, 4
        je y_left
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[si+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
y_left:
        jmp y_next
y_prev_out:
        or ch, ch
        je y_enter
        jmp short y_both_out
y_enter:
        test cl, 8
        je y_enter_bottom
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[bx+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        sub ax, word ptr _g_vmds[bx+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_work_y]
        sub bp, word ptr _g_vmds[bx+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
        jmp short y_enter_keep
y_enter_bottom:
        test cl, 4
        je y_enter_keep
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[bx+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        sub ax, word ptr _g_vmds[bx+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_work_y]
        sub bp, word ptr _g_vmds[bx+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
y_enter_keep:
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        mov ax, word ptr _g_vmds[si+vmds_work_y]
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        add di, 2
        jmp y_next
y_both_out:
        test cl, 8
        je y_cross_from_bottom
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[bx+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        sub ax, word ptr _g_vmds[bx+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_work_y]
        sub bp, word ptr _g_vmds[bx+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
        jmp short y_cross_to
y_cross_from_bottom:
        test cl, 4
        je y_cross_to
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[bx+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[si+vmds_work_x]
        sub ax, word ptr _g_vmds[bx+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[si+vmds_work_y]
        sub bp, word ptr _g_vmds[bx+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[bx+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
y_cross_to:
        test ch, 8
        je y_cross_to_bottom
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_top
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[si+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
        jmp short y_next
y_cross_to_bottom:
        test ch, 4
        je y_next
        mov ax, word ptr DGROUP:_g_vmds+vmds_clip_bottom
        mov word ptr _g_vmds[di+vmds_poly_y], ax
        sub ax, word ptr _g_vmds[si+vmds_work_y]
        mov bp, ax
        mov ax, word ptr _g_vmds[bx+vmds_work_x]
        sub ax, word ptr _g_vmds[si+vmds_work_x]
        imul bp
        mov bp, word ptr _g_vmds[bx+vmds_work_y]
        sub bp, word ptr _g_vmds[si+vmds_work_y]
        idiv bp
        add ax, word ptr _g_vmds[si+vmds_work_x]
        mov word ptr _g_vmds[di+vmds_poly_x], ax
        add di, 2
y_next:
        mov bx, si
        mov cl, ch
        shr si, 1
        inc si
        cmp si, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        je y_done
        jmp y_edge
y_done:
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count, ax
        jmp short clip_return
clip_copy_back:
        mov ax, word ptr DGROUP:_g_vmds+vmds_palettes+vm_palettes_clip_count
        mov cx, ax
        mov si, offset DGROUP:_g_vmds+vmds_work_x
        mov di, offset DGROUP:_g_vmds+vmds_poly_x
        rep movsw
        mov cx, ax
        mov si, offset DGROUP:_g_vmds+vmds_work_y
        mov di, offset DGROUP:_g_vmds+vmds_poly_y
        rep movsw
clip_return:
        retf
_clip_polygon endp
POLYCLIP_TEXT ends
}
#else

/*
 * 0x2286a
 *
 * Ask whether this is a PCjr, and remember the answer at DGROUP 0x38ac.
 *
 * The test is the ROM: the model byte at F000:FFFE being 0xff and the byte at
 * F000:C000 being 0x21. Answers the flag, sign-extended - and it is only ever
 * **set**, never cleared, so asking twice cannot unset it.
 *
 * The port's ROM is `g_rom_model` and `g_rom_c000`, and it is an AT's.
 */
int16_t detect_pcjr(void)
{
    if (g_rom_model == 0xff && g_rom_c000 == 0x21)
        g_vmds.is_pcjr = 1;

    return (int16_t)(int8_t)g_vmds.is_pcjr;
}

/*
 * 1ee5:3a41, image 0x22891
 *
 * Clip the polygon against the window, in two passes: left and right into the
 * working arrays at 0x398c and g_vmds.work_y, then top and bottom back into 0x393c and
 * 0x3964. Sutherland and Hodgman's, and the count at 0x3a2c is rewritten after
 * each pass.
 *
 * Each pass walks the edges with an outcode for the previous point and one for
 * this one, and there are four cases: both inside emits this point, both
 * outside on the *same* side emits nothing, and the two crossing cases emit the
 * intersection - and, when this point is the one inside, the point after it.
 * An edge that leaves through one side and comes back through the other emits
 * both intersections and no vertex, which is how a polygon wider than the
 * window keeps its shape.
 *
 * The intersection is `y0 + (y1 - y0) * (edge - x0) / (x1 - x0)`, computed with
 * `imul` and `idiv` so the product is 32 bits before the divide - the
 * coordinates are large enough that a 16-bit product would wrap.
 *
 * A polygon left with one point or none is not clipped a second time: the first
 * pass's answer is copied back and that is that.
 */
void clip_polygon(void)
{
    int16_t si, di, bx;
    uint8_t cl, ch;
    int16_t n;

    di = 0;
    n = (int16_t)g_vmds.palettes.clip_count;
    if (n <= 1)
        return;

    bx = (int16_t)((n - 1) * 2);

    cl = 0;
    if (g_vmds.poly_x[bx >> 1] < g_vmds.clip_left)
        cl |= 1;
    if (g_vmds.poly_x[bx >> 1] > g_vmds.clip_right)
        cl |= 2;

    for (si = 0; ; ) {
        ch = 0;
        if (g_vmds.poly_x[si >> 1] < g_vmds.clip_left)
            ch |= 1;
        if (g_vmds.poly_x[si >> 1] > g_vmds.clip_right)
            ch |= 2;

        if ((cl | ch) == 0) {
            g_vmds.work_x[di >> 1] = g_vmds.poly_x[si >> 1];
            g_vmds.work_y[di >> 1] = g_vmds.poly_y[si >> 1];
            di += 2;
        } else if ((cl & ch) != 0) {
            /* Both outside the same edge: nothing survives. */
        } else if (cl == 0) {
            /* Leaving: the crossing only. */
            int16_t edge = (ch & 1) ? g_vmds.clip_left
                         : (ch & 2) ? g_vmds.clip_right : 0;

            if (ch & 3) {
                g_vmds.work_x[di >> 1] = edge;
                g_vmds.work_y[di >> 1] = (int16_t)(
                    (int32_t)(g_vmds.poly_y[bx >> 1]
                              - g_vmds.poly_y[si >> 1])
                    * (int32_t)(int16_t)(edge - g_vmds.poly_x[si >> 1])
                    / (int32_t)(int16_t)(g_vmds.poly_x[bx >> 1]
                                         - g_vmds.poly_x[si >> 1])
                    + g_vmds.poly_y[si >> 1]);
                di += 2;
            }
        } else if (ch == 0) {
            /* Arriving: the crossing, and then the point itself. */
            int16_t edge = (cl & 1) ? g_vmds.clip_left
                         : (cl & 2) ? g_vmds.clip_right : 0;

            if (cl & 3) {
                g_vmds.work_x[di >> 1] = edge;
                g_vmds.work_y[di >> 1] = (int16_t)(
                    (int32_t)(g_vmds.poly_y[si >> 1]
                              - g_vmds.poly_y[bx >> 1])
                    * (int32_t)(int16_t)(edge - g_vmds.poly_x[bx >> 1])
                    / (int32_t)(int16_t)(g_vmds.poly_x[si >> 1]
                                         - g_vmds.poly_x[bx >> 1])
                    + g_vmds.poly_y[bx >> 1]);
                di += 2;
            }

            g_vmds.work_x[di >> 1] = g_vmds.poly_x[si >> 1];
            g_vmds.work_y[di >> 1] = g_vmds.poly_y[si >> 1];
            di += 2;
        } else {
            /* Out one side and in the other: both crossings, no vertex. */
            int16_t e1 = (cl & 1) ? g_vmds.clip_left
                       : (cl & 2) ? g_vmds.clip_right : 0;
            int16_t e2 = (ch & 1) ? g_vmds.clip_left
                       : (ch & 2) ? g_vmds.clip_right : 0;

            if (cl & 3) {
                g_vmds.work_x[di >> 1] = e1;
                g_vmds.work_y[di >> 1] = (int16_t)(
                    (int32_t)(g_vmds.poly_y[si >> 1]
                              - g_vmds.poly_y[bx >> 1])
                    * (int32_t)(int16_t)(e1 - g_vmds.poly_x[bx >> 1])
                    / (int32_t)(int16_t)(g_vmds.poly_x[si >> 1]
                                         - g_vmds.poly_x[bx >> 1])
                    + g_vmds.poly_y[bx >> 1]);
                di += 2;
            }

            if (ch & 3) {
                g_vmds.work_x[di >> 1] = e2;
                g_vmds.work_y[di >> 1] = (int16_t)(
                    (int32_t)(g_vmds.poly_y[bx >> 1]
                              - g_vmds.poly_y[si >> 1])
                    * (int32_t)(int16_t)(e2 - g_vmds.poly_x[si >> 1])
                    / (int32_t)(int16_t)(g_vmds.poly_x[bx >> 1]
                                         - g_vmds.poly_x[si >> 1])
                    + g_vmds.poly_y[si >> 1]);
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)g_vmds.palettes.clip_count)
            break;
        si = (int16_t)(si * 2);
    }

    n = (int16_t)((uint16_t)di >> 1);
    g_vmds.palettes.clip_count = (uint16_t)n;

    if (n <= 1) {
        int16_t i;

        for (i = 0; i < n; i++) {
            g_vmds.poly_x[i] = ((uint16_t)g_vmds.work_x[i]);
            g_vmds.poly_y[i] = ((uint16_t)g_vmds.work_y[i]);
        }
        return;
    }

    bx = (int16_t)((n - 1) * 2);
    di = 0;

    cl = 0;
    if (g_vmds.work_y[bx >> 1] > g_vmds.clip_bottom)
        cl |= 4;
    if (g_vmds.work_y[bx >> 1] < g_vmds.clip_top)
        cl |= 8;

    for (si = 0; ; ) {
        ch = 0;
        if (g_vmds.work_y[si >> 1] > g_vmds.clip_bottom)
            ch |= 4;
        if (g_vmds.work_y[si >> 1] < g_vmds.clip_top)
            ch |= 8;

        if ((cl | ch) == 0) {
            g_vmds.poly_x[di >> 1] = g_vmds.work_x[si >> 1];
            g_vmds.poly_y[di >> 1] = g_vmds.work_y[si >> 1];
            di += 2;
        } else if ((cl & ch) != 0) {
            /* nothing */
        } else if (cl == 0) {
            int16_t edge = (ch & 4) ? g_vmds.clip_bottom
                         : (ch & 8) ? g_vmds.clip_top : 0;

            if (ch & 12) {
                g_vmds.poly_y[di >> 1] = edge;
                g_vmds.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(g_vmds.work_x[bx >> 1]
                              - g_vmds.work_x[si >> 1])
                    * (int32_t)(int16_t)(edge - g_vmds.work_y[si >> 1])
                    / (int32_t)(int16_t)(g_vmds.work_y[bx >> 1]
                                         - g_vmds.work_y[si >> 1])
                    + g_vmds.work_x[si >> 1]);
                di += 2;
            }
        } else if (ch == 0) {
            int16_t edge = (cl & 4) ? g_vmds.clip_bottom
                         : (cl & 8) ? g_vmds.clip_top : 0;

            if (cl & 12) {
                g_vmds.poly_y[di >> 1] = edge;
                g_vmds.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(g_vmds.work_x[si >> 1]
                              - g_vmds.work_x[bx >> 1])
                    * (int32_t)(int16_t)(edge - g_vmds.work_y[bx >> 1])
                    / (int32_t)(int16_t)(g_vmds.work_y[si >> 1]
                                         - g_vmds.work_y[bx >> 1])
                    + g_vmds.work_x[bx >> 1]);
                di += 2;
            }

            g_vmds.poly_x[di >> 1] = g_vmds.work_x[si >> 1];
            g_vmds.poly_y[di >> 1] = g_vmds.work_y[si >> 1];
            di += 2;
        } else {
            int16_t e1 = (cl & 4) ? g_vmds.clip_bottom
                       : (cl & 8) ? g_vmds.clip_top : 0;
            int16_t e2 = (ch & 4) ? g_vmds.clip_bottom
                       : (ch & 8) ? g_vmds.clip_top : 0;

            if (cl & 12) {
                g_vmds.poly_y[di >> 1] = e1;
                g_vmds.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(g_vmds.work_x[si >> 1]
                              - g_vmds.work_x[bx >> 1])
                    * (int32_t)(int16_t)(e1 - g_vmds.work_y[bx >> 1])
                    / (int32_t)(int16_t)(g_vmds.work_y[si >> 1]
                                         - g_vmds.work_y[bx >> 1])
                    + g_vmds.work_x[bx >> 1]);
                di += 2;
            }

            if (ch & 12) {
                g_vmds.poly_y[di >> 1] = e2;
                g_vmds.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(g_vmds.work_x[bx >> 1]
                              - g_vmds.work_x[si >> 1])
                    * (int32_t)(int16_t)(e2 - g_vmds.work_y[si >> 1])
                    / (int32_t)(int16_t)(g_vmds.work_y[bx >> 1]
                                         - g_vmds.work_y[si >> 1])
                    + g_vmds.work_x[si >> 1]);
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)g_vmds.palettes.clip_count)
            break;
        si = (int16_t)(si * 2);
    }

    g_vmds.palettes.clip_count = (uint16_t)((uint16_t)di >> 1);
}
#endif
