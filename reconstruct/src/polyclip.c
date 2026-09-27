/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
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
 * The module as TASM assembled it, drafted by tools/asm2tasm.py; the host's
 * transcription is the `#else`. See glue.c for how the block reaches the
 * assembler.
 */
asm {
extrn _VMDS:byte
POLYCLIP_TEXT segment byte public 'CODE'
assume cs:POLYCLIP_TEXT, ds:DGROUP
public _detect_pcjr, _clip_polygon

/* 0x20be0 */
_detect_pcjr proc far
        push es
        push bx
        mov bx, 0f000h
        mov es, bx
        mov bx, 0fffeh
        mov al, byte ptr es:[bx]
        cmp al, 0ffh
        jne L20c00
        mov bx, 0c000h
        mov al, byte ptr es:[bx]
        cmp al, 21h
        jne L20c00
        mov byte ptr DGROUP:_VMDS+1ch, 1
L20c00:
        mov al, byte ptr DGROUP:_VMDS+1ch
        cbw
        pop bx
        pop es
        retf
_detect_pcjr endp

/* 0x20c07 */
_clip_polygon proc far
        xor di, di
        mov ax, word ptr DGROUP:_VMDS+19ch
        cmp ax, 1
        jg L20c14
        jmp L21087
L20c14:
        dec ax
        mov bx, ax
        shl bx, 1
        xor cl, cl
        mov ax, word ptr [bx+393ch]
        cmp ax, word ptr DGROUP:_VMDS+4h
        jge L20c28
        or cl, 1
L20c28:
        cmp ax, word ptr DGROUP:_VMDS+6h
        jle L20c31
        or cl, 2
L20c31:
        xor si, si
L20c33:
        shl si, 1
        xor ch, ch
        mov ax, word ptr [si+393ch]
        cmp ax, word ptr DGROUP:_VMDS+4h
        jge L20c44
        or ch, 1
L20c44:
        cmp ax, word ptr DGROUP:_VMDS+6h
        jle L20c4d
        or ch, 2
L20c4d:
        mov al, cl
        or al, ch
        jne L20c69
        mov ax, word ptr [si+393ch]
        mov word ptr [di+398ch], ax
        mov ax, word ptr [si+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
        jmp L20e25
L20c69:
        mov al, cl
        and al, ch
        je L20c72
        jmp L20e25
L20c72:
        or cl, cl
        jne L20cdd
        test ch, 1
        je L20ca9
        mov ax, word ptr DGROUP:_VMDS+4h
        mov word ptr [di+398ch], ax
        sub ax, word ptr [si+393ch]
        mov bp, ax
        mov ax, word ptr [bx+3964h]
        sub ax, word ptr [si+3964h]
        imul bp
        mov bp, word ptr [bx+393ch]
        sub bp, word ptr [si+393ch]
        idiv bp
        add ax, word ptr [si+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
        jmp short L20cda
L20ca9:
        test ch, 2
        je L20cda
        mov ax, word ptr DGROUP:_VMDS+6h
        mov word ptr [di+398ch], ax
        sub ax, word ptr [si+393ch]
        mov bp, ax
        mov ax, word ptr [bx+3964h]
        sub ax, word ptr [si+3964h]
        imul bp
        mov bp, word ptr [bx+393ch]
        sub bp, word ptr [si+393ch]
        idiv bp
        add ax, word ptr [si+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
L20cda:
        jmp L20e25
L20cdd:
        or ch, ch
        je L20ce3
        jmp short L20d5d
L20ce3:
        test cl, 1
        je L20d16
        mov ax, word ptr DGROUP:_VMDS+4h
        mov word ptr [di+398ch], ax
        sub ax, word ptr [bx+393ch]
        mov bp, ax
        mov ax, word ptr [si+3964h]
        sub ax, word ptr [bx+3964h]
        imul bp
        mov bp, word ptr [si+393ch]
        sub bp, word ptr [bx+393ch]
        idiv bp
        add ax, word ptr [bx+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
        jmp short L20d47
L20d16:
        test cl, 2
        je L20d47
        mov ax, word ptr DGROUP:_VMDS+6h
        mov word ptr [di+398ch], ax
        sub ax, word ptr [bx+393ch]
        mov bp, ax
        mov ax, word ptr [si+3964h]
        sub ax, word ptr [bx+3964h]
        imul bp
        mov bp, word ptr [si+393ch]
        sub bp, word ptr [bx+393ch]
        idiv bp
        add ax, word ptr [bx+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
L20d47:
        mov ax, word ptr [si+393ch]
        mov word ptr [di+398ch], ax
        mov ax, word ptr [si+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
        jmp L20e25
L20d5d:
        test cl, 1
        je L20d90
        mov ax, word ptr DGROUP:_VMDS+4h
        mov word ptr [di+398ch], ax
        sub ax, word ptr [bx+393ch]
        mov bp, ax
        mov ax, word ptr [si+3964h]
        sub ax, word ptr [bx+3964h]
        imul bp
        mov bp, word ptr [si+393ch]
        sub bp, word ptr [bx+393ch]
        idiv bp
        add ax, word ptr [bx+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
        jmp short L20dc1
L20d90:
        test cl, 2
        je L20dc1
        mov ax, word ptr DGROUP:_VMDS+6h
        mov word ptr [di+398ch], ax
        sub ax, word ptr [bx+393ch]
        mov bp, ax
        mov ax, word ptr [si+3964h]
        sub ax, word ptr [bx+3964h]
        imul bp
        mov bp, word ptr [si+393ch]
        sub bp, word ptr [bx+393ch]
        idiv bp
        add ax, word ptr [bx+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
L20dc1:
        test ch, 1
        je L20df4
        mov ax, word ptr DGROUP:_VMDS+4h
        mov word ptr [di+398ch], ax
        sub ax, word ptr [si+393ch]
        mov bp, ax
        mov ax, word ptr [bx+3964h]
        sub ax, word ptr [si+3964h]
        imul bp
        mov bp, word ptr [bx+393ch]
        sub bp, word ptr [si+393ch]
        idiv bp
        add ax, word ptr [si+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
        jmp short L20e25
L20df4:
        test ch, 2
        je L20e25
        mov ax, word ptr DGROUP:_VMDS+6h
        mov word ptr [di+398ch], ax
        sub ax, word ptr [si+393ch]
        mov bp, ax
        mov ax, word ptr [bx+3964h]
        sub ax, word ptr [si+3964h]
        imul bp
        mov bp, word ptr [bx+393ch]
        sub bp, word ptr [si+393ch]
        idiv bp
        add ax, word ptr [si+3964h]
        mov word ptr [di+39b4h], ax
        add di, 2
L20e25:
        mov bx, si
        mov cl, ch
        shr si, 1
        inc si
        cmp si, word ptr DGROUP:_VMDS+19ch
        je L20e35
        jmp L20c33
L20e35:
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_VMDS+19ch, ax
        cmp ax, 1
        jg L20e44
        jmp L21070
L20e44:
        dec ax
        mov bx, ax
        shl bx, 1
        xor cl, cl
        mov ax, word ptr [bx+39b4h]
        cmp ax, word ptr DGROUP:_VMDS+0ah
        jle L20e58
        or cl, 4
L20e58:
        cmp ax, word ptr DGROUP:_VMDS+8h
        jge L20e61
        or cl, 8
L20e61:
        xor di, di
        mov si, di
L20e65:
        shl si, 1
        xor ch, ch
        mov ax, word ptr [si+39b4h]
        cmp ax, word ptr DGROUP:_VMDS+0ah
        jle L20e76
        or ch, 4
L20e76:
        cmp ax, word ptr DGROUP:_VMDS+8h
        jge L20e7f
        or ch, 8
L20e7f:
        mov al, cl
        or al, ch
        jne L20e9b
        mov ax, word ptr [si+398ch]
        mov word ptr [di+393ch], ax
        mov ax, word ptr [si+39b4h]
        mov word ptr [di+3964h], ax
        add di, 2
        jmp L21057
L20e9b:
        mov al, cl
        and al, ch
        je L20ea4
        jmp L21057
L20ea4:
        or cl, cl
        jne L20f0f
        test ch, 8
        je L20edb
        mov ax, word ptr DGROUP:_VMDS+8h
        mov word ptr [di+3964h], ax
        sub ax, word ptr [si+39b4h]
        mov bp, ax
        mov ax, word ptr [bx+398ch]
        sub ax, word ptr [si+398ch]
        imul bp
        mov bp, word ptr [bx+39b4h]
        sub bp, word ptr [si+39b4h]
        idiv bp
        add ax, word ptr [si+398ch]
        mov word ptr [di+393ch], ax
        add di, 2
        jmp short L20f0c
L20edb:
        test ch, 4
        je L20f0c
        mov ax, word ptr DGROUP:_VMDS+0ah
        mov word ptr [di+3964h], ax
        sub ax, word ptr [si+39b4h]
        mov bp, ax
        mov ax, word ptr [bx+398ch]
        sub ax, word ptr [si+398ch]
        imul bp
        mov bp, word ptr [bx+39b4h]
        sub bp, word ptr [si+39b4h]
        idiv bp
        add ax, word ptr [si+398ch]
        mov word ptr [di+393ch], ax
        add di, 2
L20f0c:
        jmp L21057
L20f0f:
        or ch, ch
        je L20f15
        jmp short L20f8f
L20f15:
        test cl, 8
        je L20f48
        mov ax, word ptr DGROUP:_VMDS+8h
        mov word ptr [di+3964h], ax
        sub ax, word ptr [bx+39b4h]
        mov bp, ax
        mov ax, word ptr [si+398ch]
        sub ax, word ptr [bx+398ch]
        imul bp
        mov bp, word ptr [si+39b4h]
        sub bp, word ptr [bx+39b4h]
        idiv bp
        add ax, word ptr [bx+398ch]
        mov word ptr [di+393ch], ax
        add di, 2
        jmp short L20f79
L20f48:
        test cl, 4
        je L20f79
        mov ax, word ptr DGROUP:_VMDS+0ah
        mov word ptr [di+3964h], ax
        sub ax, word ptr [bx+39b4h]
        mov bp, ax
        mov ax, word ptr [si+398ch]
        sub ax, word ptr [bx+398ch]
        imul bp
        mov bp, word ptr [si+39b4h]
        sub bp, word ptr [bx+39b4h]
        idiv bp
        add ax, word ptr [bx+398ch]
        mov word ptr [di+393ch], ax
        add di, 2
L20f79:
        mov ax, word ptr [si+398ch]
        mov word ptr [di+393ch], ax
        mov ax, word ptr [si+39b4h]
        mov word ptr [di+3964h], ax
        add di, 2
        jmp L21057
L20f8f:
        test cl, 8
        je L20fc2
        mov ax, word ptr DGROUP:_VMDS+8h
        mov word ptr [di+3964h], ax
        sub ax, word ptr [bx+39b4h]
        mov bp, ax
        mov ax, word ptr [si+398ch]
        sub ax, word ptr [bx+398ch]
        imul bp
        mov bp, word ptr [si+39b4h]
        sub bp, word ptr [bx+39b4h]
        idiv bp
        add ax, word ptr [bx+398ch]
        mov word ptr [di+393ch], ax
        add di, 2
        jmp short L20ff3
L20fc2:
        test cl, 4
        je L20ff3
        mov ax, word ptr DGROUP:_VMDS+0ah
        mov word ptr [di+3964h], ax
        sub ax, word ptr [bx+39b4h]
        mov bp, ax
        mov ax, word ptr [si+398ch]
        sub ax, word ptr [bx+398ch]
        imul bp
        mov bp, word ptr [si+39b4h]
        sub bp, word ptr [bx+39b4h]
        idiv bp
        add ax, word ptr [bx+398ch]
        mov word ptr [di+393ch], ax
        add di, 2
L20ff3:
        test ch, 8
        je L21026
        mov ax, word ptr DGROUP:_VMDS+8h
        mov word ptr [di+3964h], ax
        sub ax, word ptr [si+39b4h]
        mov bp, ax
        mov ax, word ptr [bx+398ch]
        sub ax, word ptr [si+398ch]
        imul bp
        mov bp, word ptr [bx+39b4h]
        sub bp, word ptr [si+39b4h]
        idiv bp
        add ax, word ptr [si+398ch]
        mov word ptr [di+393ch], ax
        add di, 2
        jmp short L21057
L21026:
        test ch, 4
        je L21057
        mov ax, word ptr DGROUP:_VMDS+0ah
        mov word ptr [di+3964h], ax
        sub ax, word ptr [si+39b4h]
        mov bp, ax
        mov ax, word ptr [bx+398ch]
        sub ax, word ptr [si+398ch]
        imul bp
        mov bp, word ptr [bx+39b4h]
        sub bp, word ptr [si+39b4h]
        idiv bp
        add ax, word ptr [si+398ch]
        mov word ptr [di+393ch], ax
        add di, 2
L21057:
        mov bx, si
        mov cl, ch
        shr si, 1
        inc si
        cmp si, word ptr DGROUP:_VMDS+19ch
        je L21067
        jmp L20e65
L21067:
        shr di, 1
        mov ax, di
        mov word ptr DGROUP:_VMDS+19ch, ax
        jmp short L21087
L21070:
        mov ax, word ptr DGROUP:_VMDS+19ch
        mov cx, ax
        mov si, offset DGROUP:_VMDS+0fch
        mov di, offset DGROUP:_VMDS+0ach
        rep movsw
        mov cx, ax
        mov si, offset DGROUP:_VMDS+124h
        mov di, offset DGROUP:_VMDS+0d4h
        rep movsw
L21087:
        retf
_clip_polygon endp
POLYCLIP_TEXT ends
}
#else

/*
 * 0x20be0
 *
 * Ask whether this is a PCjr, and remember the answer at DGROUP 0x38ac.
 *
 * The test is the ROM: the model byte at F000:FFFE being 0xff and the byte at
 * F000:C000 being 0x21. Answers the flag, sign-extended - and it is only ever
 * **set**, never cleared, so asking twice cannot unset it.
 *
 * Both addresses are ordinary memory as far as the port is concerned: the
 * verifier seeds all of it, ROM included.
 */
int16_t detect_pcjr(void)
{
    if (*MK_FP(0xf000, 0xfffe) == 0xff
        && *MK_FP(0xf000, 0xc000) == 0x21)
        VMDS.is_pcjr = 1;

    return (int16_t)(int8_t)VMDS.is_pcjr;
}

/*
 * 172c:39b7, image 0x20c07
 *
 * Clip the polygon against the window, in two passes: left and right into the
 * working arrays at 0x398c and dg_near(dgroup, VMDS.work_y), then top and bottom back into 0x393c and
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
    n = (int16_t)VMDS.palettes.clip_count;
    if (n <= 1)
        return;

    bx = (int16_t)((n - 1) * 2);

    cl = 0;
    if (VMDS.poly_x[bx >> 1] < VMDS.clip_left)
        cl |= 1;
    if (VMDS.poly_x[bx >> 1] > VMDS.clip_right)
        cl |= 2;

    for (si = 0; ; ) {
        ch = 0;
        if (VMDS.poly_x[si >> 1] < VMDS.clip_left)
            ch |= 1;
        if (VMDS.poly_x[si >> 1] > VMDS.clip_right)
            ch |= 2;

        if ((cl | ch) == 0) {
            VMDS.work_x[di >> 1] = VMDS.poly_x[si >> 1];
            VMDS.work_y[di >> 1] = VMDS.poly_y[si >> 1];
            di += 2;
        } else if ((cl & ch) != 0) {
            /* Both outside the same edge: nothing survives. */
        } else if (cl == 0) {
            /* Leaving: the crossing only. */
            int16_t edge = (ch & 1) ? VMDS.clip_left
                         : (ch & 2) ? VMDS.clip_right : 0;

            if (ch & 3) {
                VMDS.work_x[di >> 1] = edge;
                VMDS.work_y[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.poly_y[bx >> 1]
                              - VMDS.poly_y[si >> 1])
                    * (int32_t)(int16_t)(edge - VMDS.poly_x[si >> 1])
                    / (int32_t)(int16_t)(VMDS.poly_x[bx >> 1]
                                         - VMDS.poly_x[si >> 1])
                    + VMDS.poly_y[si >> 1]);
                di += 2;
            }
        } else if (ch == 0) {
            /* Arriving: the crossing, and then the point itself. */
            int16_t edge = (cl & 1) ? VMDS.clip_left
                         : (cl & 2) ? VMDS.clip_right : 0;

            if (cl & 3) {
                VMDS.work_x[di >> 1] = edge;
                VMDS.work_y[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.poly_y[si >> 1]
                              - VMDS.poly_y[bx >> 1])
                    * (int32_t)(int16_t)(edge - VMDS.poly_x[bx >> 1])
                    / (int32_t)(int16_t)(VMDS.poly_x[si >> 1]
                                         - VMDS.poly_x[bx >> 1])
                    + VMDS.poly_y[bx >> 1]);
                di += 2;
            }

            VMDS.work_x[di >> 1] = VMDS.poly_x[si >> 1];
            VMDS.work_y[di >> 1] = VMDS.poly_y[si >> 1];
            di += 2;
        } else {
            /* Out one side and in the other: both crossings, no vertex. */
            int16_t e1 = (cl & 1) ? VMDS.clip_left
                       : (cl & 2) ? VMDS.clip_right : 0;
            int16_t e2 = (ch & 1) ? VMDS.clip_left
                       : (ch & 2) ? VMDS.clip_right : 0;

            if (cl & 3) {
                VMDS.work_x[di >> 1] = e1;
                VMDS.work_y[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.poly_y[si >> 1]
                              - VMDS.poly_y[bx >> 1])
                    * (int32_t)(int16_t)(e1 - VMDS.poly_x[bx >> 1])
                    / (int32_t)(int16_t)(VMDS.poly_x[si >> 1]
                                         - VMDS.poly_x[bx >> 1])
                    + VMDS.poly_y[bx >> 1]);
                di += 2;
            }

            if (ch & 3) {
                VMDS.work_x[di >> 1] = e2;
                VMDS.work_y[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.poly_y[bx >> 1]
                              - VMDS.poly_y[si >> 1])
                    * (int32_t)(int16_t)(e2 - VMDS.poly_x[si >> 1])
                    / (int32_t)(int16_t)(VMDS.poly_x[bx >> 1]
                                         - VMDS.poly_x[si >> 1])
                    + VMDS.poly_y[si >> 1]);
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)VMDS.palettes.clip_count)
            break;
        si = (int16_t)(si * 2);
    }

    n = (int16_t)((uint16_t)di >> 1);
    VMDS.palettes.clip_count = (uint16_t)n;

    if (n <= 1) {
        int16_t i;

        for (i = 0; i < n; i++) {
            VMDS.poly_x[i] = ((uint16_t)VMDS.work_x[i]);
            VMDS.poly_y[i] = ((uint16_t)VMDS.work_y[i]);
        }
        return;
    }

    bx = (int16_t)((n - 1) * 2);
    di = 0;

    cl = 0;
    if (VMDS.work_y[bx >> 1] > VMDS.clip_bottom)
        cl |= 4;
    if (VMDS.work_y[bx >> 1] < VMDS.clip_top)
        cl |= 8;

    for (si = 0; ; ) {
        ch = 0;
        if (VMDS.work_y[si >> 1] > VMDS.clip_bottom)
            ch |= 4;
        if (VMDS.work_y[si >> 1] < VMDS.clip_top)
            ch |= 8;

        if ((cl | ch) == 0) {
            VMDS.poly_x[di >> 1] = VMDS.work_x[si >> 1];
            VMDS.poly_y[di >> 1] = VMDS.work_y[si >> 1];
            di += 2;
        } else if ((cl & ch) != 0) {
            /* nothing */
        } else if (cl == 0) {
            int16_t edge = (ch & 4) ? VMDS.clip_bottom
                         : (ch & 8) ? VMDS.clip_top : 0;

            if (ch & 12) {
                VMDS.poly_y[di >> 1] = edge;
                VMDS.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.work_x[bx >> 1]
                              - VMDS.work_x[si >> 1])
                    * (int32_t)(int16_t)(edge - VMDS.work_y[si >> 1])
                    / (int32_t)(int16_t)(VMDS.work_y[bx >> 1]
                                         - VMDS.work_y[si >> 1])
                    + VMDS.work_x[si >> 1]);
                di += 2;
            }
        } else if (ch == 0) {
            int16_t edge = (cl & 4) ? VMDS.clip_bottom
                         : (cl & 8) ? VMDS.clip_top : 0;

            if (cl & 12) {
                VMDS.poly_y[di >> 1] = edge;
                VMDS.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.work_x[si >> 1]
                              - VMDS.work_x[bx >> 1])
                    * (int32_t)(int16_t)(edge - VMDS.work_y[bx >> 1])
                    / (int32_t)(int16_t)(VMDS.work_y[si >> 1]
                                         - VMDS.work_y[bx >> 1])
                    + VMDS.work_x[bx >> 1]);
                di += 2;
            }

            VMDS.poly_x[di >> 1] = VMDS.work_x[si >> 1];
            VMDS.poly_y[di >> 1] = VMDS.work_y[si >> 1];
            di += 2;
        } else {
            int16_t e1 = (cl & 4) ? VMDS.clip_bottom
                       : (cl & 8) ? VMDS.clip_top : 0;
            int16_t e2 = (ch & 4) ? VMDS.clip_bottom
                       : (ch & 8) ? VMDS.clip_top : 0;

            if (cl & 12) {
                VMDS.poly_y[di >> 1] = e1;
                VMDS.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.work_x[si >> 1]
                              - VMDS.work_x[bx >> 1])
                    * (int32_t)(int16_t)(e1 - VMDS.work_y[bx >> 1])
                    / (int32_t)(int16_t)(VMDS.work_y[si >> 1]
                                         - VMDS.work_y[bx >> 1])
                    + VMDS.work_x[bx >> 1]);
                di += 2;
            }

            if (ch & 12) {
                VMDS.poly_y[di >> 1] = e2;
                VMDS.poly_x[di >> 1] = (int16_t)(
                    (int32_t)(VMDS.work_x[bx >> 1]
                              - VMDS.work_x[si >> 1])
                    * (int32_t)(int16_t)(e2 - VMDS.work_y[si >> 1])
                    / (int32_t)(int16_t)(VMDS.work_y[bx >> 1]
                                         - VMDS.work_y[si >> 1])
                    + VMDS.work_x[si >> 1]);
                di += 2;
            }
        }

        bx = si;
        cl = ch;
        si = (int16_t)(((uint16_t)si >> 1) + 1);
        if (si == (int16_t)VMDS.palettes.clip_count)
            break;
        si = (int16_t)(si * 2);
    }

    VMDS.palettes.clip_count = (uint16_t)((uint16_t)di >> 1);
}
#endif
