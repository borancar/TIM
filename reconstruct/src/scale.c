/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The 16.16 step, and the plain scaled blit.**
 *
 * A module of the original's **code segment 1c25**, image 0x20840..0x20be0,
 * split out of engine.c on 2026-09-27. Its `_DATA` is DGROUP 0x457a..0x458c:
 * the timer's record before it ends at 0x4579, padded to the word, and the
 * keyboard's begins at 0x458c. Its `_BSS` is the two scaling tables at
 * 0x5956 and 0x5e56, which vidload.c's blitter fills too. The two thunks
 * before it and `detect_pcjr` after it are assembly. Functions are in address
 * order and each carries the image offset it was read from.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -G -O
 * JUDGE: data 0x457a..0x458c
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

#ifdef __TURBOC__
/* This module's `MK_FP` is Turbo C 2.0's, as files.c's is: the `cwd` before
   the bitmap's pointer is built is the segment widened to a long. */
#undef MK_FP
#define MK_FP(seg, ofs) ((void far *)(((uint32_t)(seg) << 16) | (uint16_t)(ofs)))

/* The driver's page hook (slot 28, DGROUP 0x43b6) and scaled-row entry
   (slot 37, 0x43da), both taking everything in registers. They are called
   from C between the `asm` lines that load those registers, because the
   built-in assembler writes `call dword ptr VM_DRIVER+98h` as `VM_DRIVER-98h`
   (docs/lessons.md). The row entry is reached through SS - DS is the
   bitmap's by then - and C cannot say that: a `_ss` pointer into DGROUP
   loses its prefix, because the model takes SS to be DGROUP. So the
   prefix is a `db` in front of the call. */
typedef void (far *vm_hook_fn)(void);
typedef void (far *vm_row_fn)(void);
#endif

/*
 * **The stride shift table**, DGROUP 0x457a..0x458c, 0x12 bytes: `blit_scaled_b` shifts a
 * bitmap's width by the entry the driver's pixel shift selects - read as
 * `mov al, [bx+0x457a]` with a sign-extended byte in `bx`, so the index can
 * be negative and the table is only known to start here. Fourteen bytes,
 * then four the port never reads, up to ENGINE_KEYBOARD.
 */
struct engine_stride_shifts {
    uint8_t   stride_shift[14];   /* +0x00 [0xe]  ff 02 03 01 ff 00 ff 00 00 03 01 03 03 03 */
    uint8_t   bytes_4588[4] NONSTRING;  /* +0x0e [4] */
} PACKED;

struct engine_stride_shifts ENGINE_STRIDE_SHIFTS = {
    {
        0xff, 0x02, 0x03, 0x01, 0xff, 0x00, 0xff, 0x00, 0x00, 0x03, 0x01,
        0x03, 0x03, 0x03,
    },
    "andy",
};


/* **This module's `_BSS`**: the two tables the scaled blits build. */
struct engine_scale_table ENGINE_SCALE_TABLE;   /* DGROUP 0x5956 */
struct engine_row_offsets ENGINE_ROW_OFFSETS;   /* DGROUP 0x5e56 */

/*
 * 0x20840
 *
 * Work out a **16.16 fixed-point step**: the span at +4..+6 divided by a
 * count, left at +4 with its low word also copied to +0.
 *
 * The span is not what it looks like. `[si]` and `[si+4]` are both zeroed
 * first, and the 32-bit subtract that follows is
 * `[si+6]:[si+4] -= [si+2]:[si]` - so with two of the four words just cleared
 * it comes to `([si+6] - [si+2]) << 16`, and the low half is zero by
 * construction. The caller puts the destination size in +6 and zero in +2, so
 * the dividend is `size << 16` and the quotient is destination pixels per
 * source pixel in 16.16.
 *
 * A count of zero or less clears +0, +4 and +6 and answers 0, so asking for no
 * steps gets a zero step rather than a division by zero.
 *
 * The quotient's sign is handled by hand: made positive, its low half filed,
 * and negated back - which is why the routine keeps a flag.
 *
 * And +0 gets the low half of the step **unless the step is below 0x8000**,
 * when it gets 0x8000: the test is `and` with 0xffff8000, so a step of less
 * than half a unit starts the accumulator at half a unit. An earlier reading
 * had this as "unless the step is zero"; the judge's compile of that read
 * the bytes differently.
 */
int16_t compute_step(register union scale_step *v, register int16_t count)
{
    uint8_t negative = 0;

    if (count <= 0) {
        v->l[1] = 0;
        return v->w[0] = 0;
    }

    /* Clearing +0 and +4 is clearing each long's low word. */
    v->w[0] = 0;
    v->w[2] = 0;
    v->l[1] -= v->l[0];
    v->l[1] /= count;

    if (v->l[1] < 0) {
        v->l[1] = -v->l[1];
        negative = 1;
    }

    if ((v->l[1] & 0xffff8000L) == 0)
        v->w[0] = (int16_t)0x8000;
    else
        v->w[0] = v->w[2];

    if (negative)
        v->l[1] = -v->l[1];

    return 1;
}

/*
 * 0x208f3
 *
 * Draw a plain planar bitmap scaled - the sibling of 0x227ac, 641 bytes
 * against its 1873, and the port reaches it as soon as the compressed one
 * works.
 *
 * **Its prologue is not its sibling's.** A negative size here `or`s the mirror
 * bit rather than xoring it, and does **not** move the origin back; the
 * compressed one does both. It then clamps the destination to 0x280 by 0x190,
 * the whole screen, which the other never does. Two routines doing the same
 * job for two formats, and their argument handling differs - so neither can be
 * written from the other.
 *
 * **One table, not two.** 0x5956 gets the source column for each destination
 * pixel, walked with the accumulator across the source width; the mirrored
 * case starts at `width - 1` and steps back. After the loop the last entry is
 * *incremented*, which gives the run one column of overrun to read.
 *
 * **A second table for the rows**, at 0x5e56, holding each destination row's
 * byte offset into the source - accumulated by the row's stride, which is the
 * source width shifted right by the adapter's byte-per-pixel shift at
 * 0x457a[0x38ad]. The mirrored case fills it backwards from 0x5e54.
 *
 * **The clip is applied to the rectangle, not per row**: each edge is pulled
 * in, and the left edge's overhang is kept as a *column offset* into the table
 * rather than moving the source pointer, which is what makes a clipped scale
 * still sample the right columns.
 *
 * On adapter 0x10 it programs graphics-controller registers 1, 5 and 8 before
 * drawing, which no other path here does.
 *
 * The row is drawn by **VM.OVL VGA:0x03db**, the vector at DGROUP 0x43da, with
 * `bp` pointed at `0x5956 + 2 * left_cut`; `restore_write_mode` (0x1e94c) puts
 * the graphics controller back afterwards. Both are transcribed now.
 */
void blit_scaled_b(struct bitmap *bmp, int16_t x, int16_t y,
                   uint16_t mode, int16_t w, int16_t h)
{
    const uint8_t far *src;             /* [bp-4] */
    int16_t  i;                         /* [bp-6] */
    int16_t  j;                         /* [bp-8] */
    int16_t  stride;                    /* [bp-0xa] */
    int16_t  plane_size;                /* [bp-0xc] */
    int16_t  right;                     /* [bp-0xe] */
    int16_t  bottom;                    /* [bp-0x10] */
    int16_t  want;                      /* [bp-0x12] */
    /* The original's two reused slots: `row` counts source rows while the
       row table is built and is then the left edge's column cut, and `off`
       is the running byte offset and then the top edge. */
    int16_t  row;                       /* [bp-0x14] */
    int16_t  off;                       /* [bp-0x16] */
    int16_t  left;                      /* [bp-0x18] */
    union scale_step rec;               /* [bp-0x20] */
#ifndef __TURBOC__
    uint16_t page;
#endif

    /* A negative size is a mirror, and unlike 0x227ac it does not move the
     * origin back - the tables below are filled backwards instead. The
     * absolute value is taken in `asm`: the image's `xor ax,dx` and
     * `sub ax,dx` are `31 d0` and `29 d0`, the built-in assembler's
     * encodings, where the compiler's own `abs` writes `33 c2` and `2b c2`. */
    if (w < 0) {
#ifdef __TURBOC__
        _AX = w;
        asm cwd
        asm xor ax, dx
        asm sub ax, dx
        w = _AX;
#else
        w = abs(w);
#endif
        mode |= 2;
    }
    if (h < 0) {
#ifdef __TURBOC__
        _AX = h;
        asm cwd
        asm xor ax, dx
        asm sub ax, dx
        h = _AX;
#else
        h = abs(h);
#endif
        mode |= 1;
    }

    right = w < 0x280 ? w : 0x280;
    bottom = h < 0x190 ? h : 0x190;

    /*
     * The column table: for each destination pixel, the source column to take
     * it from. Mirrored, it starts at the last column and the step is negative.
     * **The step is over the whole width**, `w - 1`, and not over the part
     * of it that fits the screen: a bitmap scaled wider than 0x280 has its
     * columns spaced for its full width and is cut at the screen's edge.
     */
    if (mode & 2) {
        rec.w[1] = bmp->width - 1;
        rec.w[3] = 0;
    } else {
        rec.w[1] = 0;
        rec.w[3] = bmp->width - 1;
    }
    compute_step(&rec, w - 1);

    for (i = 0; i < right; i++) {
        ENGINE_SCALE_TABLE.entry[i] = rec.w[1];
        rec.l[0] += rec.l[1];
    }

    /* One column of overrun past the end, so the driver's run can read it. */
    ENGINE_SCALE_TABLE.entry[i]++;

    /*
     * The row table, holding each destination row's *byte offset* into the
     * source rather than its row number - accumulated a stride at a time, so
     * the driver needs no multiply. The step always runs forwards; mirroring
     * writes the entries in from the far end instead.
     */
    rec.w[1] = 0;
    rec.w[3] = bmp->height - 1;
    compute_step(&rec, h - 1);

    stride = bmp->width
             >> ENGINE_STRIDE_SHIFTS.stride_shift[(int8_t)VMDS.pixel_shift];
    plane_size = bmp->height * stride;

    for (j = row = off = 0; j < bottom; j++) {
        want = rec.w[1];
        rec.l[0] += rec.l[1];

        while (want > row) {
            row++;
            off += stride;
        }

        if (mode & 1)
            ENGINE_ROW_OFFSETS.row[bottom - j - 1] = off;   /* `[bx+0x5e54]`, one entry down */
        else
            ENGINE_ROW_OFFSETS.row[j] = off;
    }

    /* Only now does the rectangle become screen coordinates. */
    bottom += y;
    right += x;
    off = y;                            /* the top edge */
    left = x;
    row = 0;                            /* the left edge's column cut */

    /*
     * The clip pulls each edge in - and the left edge's overhang is kept as a
     * *column offset* into the table rather than by moving the source, which
     * is what makes a clipped scale still sample the columns it would have.
     */
    if (VMDS.clip_enabled != 0) {
        if (right > VMDS.clip_right)
            right -= right - VMDS.clip_right - 1;
        if (bottom > VMDS.clip_bottom)
            bottom -= bottom - VMDS.clip_bottom - 1;
        if (off < VMDS.clip_top)
            off = VMDS.clip_top;
        if (left < VMDS.clip_left) {
            row = VMDS.clip_left - left;
            left = VMDS.clip_left;
        }
    }

    src = MK_FP((int16_t)bmp->data_seg, bmp->data_off);

    if (bottom - off > 0 && right - left > 1) {
        /*
         * Set/reset off, write mode 0, and the index left on the bit mask -
         * which no other path here does, and which the driver row blit relies
         * on. `restore_write_mode` puts them back.
         */
        if (VMDS.adapter == 0x10) {
#ifdef __TURBOC__
            asm mov dx, 3ceh
            asm mov ax, 1
            asm out dx, ax
            asm mov ax, 5
            asm out dx, ax
            asm mov al, 8
            asm out dx, al
#else
            io_out16(PORT_GC_INDEX, 0x0001);
            io_out16(PORT_GC_INDEX, 0x0005);
            io_out8(PORT_GC_INDEX, 0x08);
#endif
        }

        /*
         * The rows go to the driver's scaled-row entry (slot 37, DGROUP
         * 0x43da) with everything in registers: DS the bitmap's segment, SI
         * the row's first byte, ES:DI the page's row, DX and CX the left edge
         * and the width, BX the row, AX the plane size and BP the column
         * table from the cut. The page hook is the same do-nothing vector
         * the other two blitters call.
         */
#ifdef __TURBOC__
        asm mov ax, word ptr VMDS+18h
        asm cmp word ptr VMDS+6e2h, 0
        asm je hooked
        asm push ax
        ((vm_hook_fn)VM_DRIVER.entry[28])();
        asm add sp, 2
hooked:
        asm mov es, ax
        j = off;
next_row:
        asm mov ax, ss
        asm mov ds, ax
        asm mov bx, j
        asm mov cx, bx
        asm shl bx, 1
        asm mov di, word ptr VMDS+6f2h[bx]
        asm mov ax, y
        asm shl ax, 1
        asm sub bx, ax
        asm mov ax, plane_size
        asm mov si, word ptr ENGINE_ROW_OFFSETS[bx]
        asm mov bx, cx
        asm add si, word ptr src
        asm mov ds, word ptr src+2
        asm mov dx, left
        asm mov cx, right
        asm sub cx, dx
        asm push bp
        asm mov bp, row
        asm shl bp, 1
        asm lea bp, ENGINE_SCALE_TABLE[bp]
        asm db 36h                      /* ss: */
        ((vm_row_fn)VM_DRIVER.entry[37])();
        asm pop bp
        asm mov ax, j
        asm inc ax
        asm cmp ax, bottom
        asm mov j, ax
        asm jl next_row
        asm mov ax, ss
        asm mov ds, ax
#else
        page = VMDS.page_dst;
        if (VMDS.page_hook != 0)
            vm_nothing();

        for (j = off; j < bottom; j++)
            vm_blit_scaled_row(
                (uint16_t)plane_size,
                &ENGINE_SCALE_TABLE.entry[row],
                vga_window_at(page, VMDS.row_offset[j]),
                left, (int16_t)(right - left),
                src + ENGINE_ROW_OFFSETS.row[j - y]);
#endif

        restore_write_mode();
    }
}

/*
 * 0x20b74
 *
 * **Draw a bitmap scaled by a factor, about a point.** The factor is in
 * 1024ths: the size is the bitmap's times `scale`, shifted right by ten, and
 * the rectangle is centred on (x, y) by taking half the size off each.
 * **Nothing in the image calls it** - no near or far call reaches 0x20b74 -
 * and the port never had it; it is here because the module has it.
 */
void blit_scaled_centred(register struct bitmap *bmp, int16_t x, int16_t y,
                         uint16_t mode, register int16_t scale)
{
    int16_t w;                          /* [bp-2] */
    int16_t h;                          /* [bp-4] */

    w = (int16_t)(((int32_t)bmp->width * scale) >> 10);
    h = (int16_t)(((int32_t)bmp->height * scale) >> 10);
    blit_scaled_b(bmp, x - (w >> 1), y - (h >> 1), mode, w, h);
}
