/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Drawing a compressed bitmap unscaled.**
 *
 * A module of the original's **code segment 1c25**, image 0x20189..0x20654,
 * split out of engine.c on 2026-09-27. The thunk at 0x20185 that jumps to it
 * through DGROUP 0x44ea, the assembly `fill_rect` before that and the timer
 * after it are all assembly and stay outside. Whether the vector at 0x44ea is
 * this module's `_DATA` is not settled: nothing here reads it, so the judge
 * cannot place it.
 *
 * Its `asm` is BC++ 2.0's own - `xor ah,ah` is `30 e4`, the built-in
 * assembler's encoding, where TASM writes `32 e4` - and it has no TASM
 * padding, so it was compiled directly.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -G -O
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

#ifdef __TURBOC__
/* This module's `MK_FP` is Turbo C 2.0's, as files.c's is. */
#undef MK_FP
#define MK_FP(seg, ofs) ((void far *)(((uint32_t)(seg) << 16) | (uint16_t)(ofs)))

/* The driver's page hook (slot 28, DGROUP 0x43b6), run blit (slot 38,
   0x43de) and span fill (slot 10, 0x436e), all taking their arguments in
   registers. Called from C between the `asm` lines that load them, because
   the built-in assembler writes a `call` through `DG4342` plus a
   displacement with the displacement negated (docs/lessons.md). */
typedef void (far *vm_hook_fn)(void);
typedef void (far *vm_run_fn)(void);
typedef void (far *vm_span_fn)(void);
#endif

/*
 * 0x20189
 *
 * Draw a bitmap in the compressed form `compress_bitmap_list` writes.
 *
 * The body behind the thunk at 0x20185 (engine.c): 0x44ea, which the thunk
 * jumps through, holds this routine's address.
 *
 * **The stream.** A byte of state first, the colour base every pixel is
 * measured from, and then a tag byte per run:
 *
 *   0xc0 | n   n **nibbles**, packed two to a byte, each added to the base.
 *              They are unpacked into a scratch buffer on the stack and blitted
 *              in one call.
 *   0x80 | n   n pixels of one colour, the next byte plus the base.
 *   0x40 | n   move n along the row; n of zero ends the bitmap.
 *   0x00 | n   end of row: step down, and move n back along it. The byte that
 *              follows is peeked at, and if its top two bits are both clear and
 *              its low six are not zero it is consumed as a *further* move of
 *              n << 6 - which is how a skip longer than 63 is written.
 *
 * **Mirroring.** Bit 0 of the mode draws the rows bottom to top, bit 1 draws
 * each row right to left - and then every "move along" is the other way round,
 * including the one at the end of a row.
 *
 * **Clipping.** The byte at 0x3893 turns it on, but the whole bitmap is tested
 * against the window first and the flag turned back off when it fits, so a
 * bitmap wholly on screen pays nothing per run. With it on, each row is tested
 * once - `row_ok` - and each run is trimmed against 0x3894 and 0x3896. A trim
 * of more than 0x3f means the run is entirely outside, because no run is
 * longer than that.
 *
 * The row's base address comes from the table at DGROUP 0x3f82, two bytes per
 * scan line, and is only reloaded when the row changes.
 */
void draw_compressed_body(struct bitmap *bmp, int16_t x, int16_t y,
                          uint16_t mode)
{
    int8_t   op;                        /* [bp-1] */
    int8_t   n;                         /* [bp-2] */
    const uint8_t far *src;             /* [bp-6] */
    uint16_t skip;                      /* [bp-8] */
    int16_t  ystep;                     /* [bp-0xa] */
    int16_t  x2;                        /* [bp-0xc] */
    int16_t  cut;                       /* [bp-0xe] */
    /* A cursor into `buf`, which the driver is handed as it stands. */
    uint8_t *p;                         /* [bp-0x10] */
    uint8_t  rowok;                     /* [bp-0x11] */
    int8_t   clip;                      /* [bp-0x12] */
    int16_t  row;                       /* [bp-0x14] */
#ifdef __TURBOC__
    uint16_t page;                      /* [bp-0x16], the page's segment */
#else
    uint8_t *page;
#endif
    uint8_t  base;                      /* [bp-0x17] */
    uint8_t  b2;                        /* [bp-0x18] */
    uint8_t  buf[320];                  /* [bp-0x158] */

#ifndef __TURBOC__
    row = 0;    /* ours: read only once `rowok` says it was set */
#endif

    /*
     * The vector at DGROUP 0x43b6 is the driver's do-nothing stub, so the page
     * comes back exactly as it went in. It is called at all only when 0x3f72
     * is set, and the port keeps the guard so that a build whose 0x3f72 is
     * clear is not silently different.
     */
#ifdef __TURBOC__
    _AX = VMDS.page_dst_ptr;
    if (VMDS.page_hook != 0) {
        asm push ax
        VM_VECTOR(28, vm_hook_fn)();
        asm add sp, 2
    }
    page = _AX;
#else
    page = MK_FP(VMDS.page_dst_ptr, 0);
    if (VMDS.page_hook != 0)
        vm_nothing();
#endif

    if ((clip = VMDS.clip_enabled) != 0
        && x >= VMDS.clip_left && x + bmp->width <= VMDS.clip_right
        && y >= VMDS.clip_top && y + bmp->height <= VMDS.clip_bottom)
        clip = 0;

    if ((uint8_t)mode & 1) {
        ystep = -1;
        y += bmp->height - 1;
    } else
        ystep = 1;

    if ((uint8_t)mode & 2)
        x += bmp->width - 1;

    if (!clip || (rowok = y <= VMDS.clip_bottom && y >= VMDS.clip_top) != 0)
        row = VMDS.row_offset[y];

    src = MK_FP((int16_t)bmp->data.seg, bmp->data.off);

    base = *src;
    src++;

    for (;;) {
        op = *src;
        src++;

        if (op & 0x80) {
            if (op & 0x40) {
                /* 0x20272 - a run of nibbles, unpacked into the buffer. */
                op &= 0x3f;
                n = op;
                p = buf;
                while (op != 0) {
                    b2 = *src;
                    src++;
                    *p = (b2 >> 4) + base;
                    p++;
                    op--;
                    if (op != 0) {
                        *p = (b2 & 0x0f) + base;
                        p++;
                        op--;
                    }
                }
                p = buf;

                if ((uint8_t)mode & 2) {
                    x2 = x - n;
                    if (!clip)
                        goto run_mirrored;
                    if (rowok == 0)
                        goto next_run;
                    if (x2 < VMDS.clip_left || x >= VMDS.clip_right)
                        goto trim_run_mirrored;
run_mirrored:
#ifdef __TURBOC__
                    asm push si
                    asm push di
                    asm mov cl, n
                    asm xor ah, ah
                    asm mov ch, ah
                    asm mov si, p
                    asm mov di, row
                    asm mov bx, x
                    asm mov es, page
                    asm stc
                    asm mov dx, y
                    VM_VECTOR(38, vm_run_fn)();
                    asm pop di
                    asm pop si
#else
                    vm_blit_run((uint16_t)x, (uint8_t)n, p,
                                page + (uint16_t)row, 1);
#endif
                    goto next_run;
trim_run_mirrored:
                    /* A trim of more than 0x3f is a run wholly outside. */
                    if (x2 < VMDS.clip_left) {
                        if ((cut = VMDS.clip_left - x2) > 0x3f)
                            goto next_run;
                        if ((n -= cut) > 0)
                            goto run_mirrored;
                        goto next_run;
                    }
                    if ((cut = x - VMDS.clip_right) > 0x3f)
                        goto next_run;
                    if ((n -= cut) <= 0)
                        goto next_run;
                    p += cut;
                    x = VMDS.clip_right;
                    goto run_mirrored;
                } else {
                    x2 = x + n;
                    if (!clip)
                        goto run;
                    if (rowok == 0)
                        goto next_run;
                    if (x < VMDS.clip_left || x2 > VMDS.clip_right)
                        goto trim_run;
run:
#ifdef __TURBOC__
                    asm push si
                    asm push di
                    asm mov cl, n
                    asm xor ah, ah
                    asm mov ch, ah
                    asm mov si, p
                    asm mov di, row
                    asm mov bx, x
                    asm mov es, page
                    asm clc
                    asm mov dx, y
                    VM_VECTOR(38, vm_run_fn)();
                    asm pop di
                    asm pop si
#else
                    vm_blit_run((uint16_t)x, (uint8_t)n, p,
                                page + (uint16_t)row, 0);
#endif
                    goto next_run;
trim_run:
                    if (x < VMDS.clip_left) {
                        if ((cut = VMDS.clip_left - x) > 0x3f)
                            goto next_run;
                        if ((n -= cut) <= 0)
                            goto next_run;
                        p += cut;
                        x = VMDS.clip_left;
                        goto run;
                    }
                    if ((cut = x2 - VMDS.clip_right - 1) > 0x3f)
                        goto next_run;
                    if ((n -= cut) > 0)
                        goto run;
                }
next_run:
                x = x2;
                continue;
            }

            /* 0x20429 - a run of one colour. */
            op &= 0x3f;
            b2 = *src;
            src++;

            if ((uint8_t)mode & 2) {
                x2 = x - op;
                if (!clip)
                    goto fill_mirrored;
                if (rowok == 0)
                    goto next_fill;
                if (x2 < VMDS.clip_left || x >= VMDS.clip_right)
                    goto trim_fill_mirrored;
fill_mirrored:
#ifdef __TURBOC__
                asm push di
                asm mov al, base
                asm add al, b2
                asm xor ah, ah
                asm mov ch, ah
                asm mov bx, x
                asm mov cl, op
                asm sub bx, cx
                asm inc bx
                asm mov di, row
                asm mov es, page
                asm mov dx, y
                VM_VECTOR(10, vm_span_fn)();
                asm pop di
#else
                vm_span((uint8_t)(base + b2), (uint16_t)(x - (uint8_t)op + 1),
                        (uint8_t)op, page + (uint16_t)row);
#endif
                goto next_fill;
trim_fill_mirrored:
                if (x2 < VMDS.clip_left) {
                    if ((cut = VMDS.clip_left - x2) > 0x3f)
                        goto next_fill;
                    if ((op -= cut) > 0)
                        goto fill_mirrored;
                    goto next_fill;
                }
                if ((cut = x - VMDS.clip_right) > 0x3f)
                    goto next_fill;
                if ((op -= cut) <= 0)
                    goto next_fill;
                x = VMDS.clip_right;
                goto fill_mirrored;
            } else {
                x2 = x + op;
                if (!clip)
                    goto fill;
                if (rowok == 0)
                    goto next_fill;
                if (x < VMDS.clip_left || x2 > VMDS.clip_right)
                    goto trim_fill;
fill:
#ifdef __TURBOC__
                asm push di
                asm mov al, b2
                asm add al, base
                asm xor ah, ah
                asm mov ch, ah
                asm mov bx, x
                asm mov cl, op
                asm mov di, row
                asm mov es, page
                asm mov dx, y
                VM_VECTOR(10, vm_span_fn)();
                asm pop di
#else
                vm_span((uint8_t)(b2 + base), (uint16_t)x, (uint8_t)op,
                        page + (uint16_t)row);
#endif
                goto next_fill;
trim_fill:
                if (x < VMDS.clip_left) {
                    if ((cut = VMDS.clip_left - x) > 0x3f)
                        goto next_fill;
                    if ((op -= cut) <= 0)
                        goto next_fill;
                    x += cut;
                    goto fill;
                }
                if ((cut = x2 - VMDS.clip_right - 1) > 0x3f)
                    goto next_fill;
                if ((op -= cut) > 0)
                    goto fill;
            }
next_fill:
            x = x2;
            continue;
        }

        if (op & 0x40) {
            /* 0x2058b - a move along the row; zero ends the bitmap. */
            if ((op &= 0x3f) == 0)
                return;
            if ((uint8_t)mode & 2)
                x -= op;
            else
                x += op;
            continue;
        }

        /* 0x205b4 - the end of a row: step down, and move back along it. */
        op &= 0x3f;
        y += ystep;
        if (!clip || (rowok = y <= VMDS.clip_bottom && y >= VMDS.clip_top) != 0)
            row = VMDS.row_offset[y];
        if ((uint8_t)mode & 2)
            x += op;
        else
            x -= op;

        /*
         * Peek at the next tag without consuming it. Only a tag with both
         * top bits clear is taken here, as a move of its low six bits
         * shifted up by six; anything else is left for the loop to read
         * again.
         */
        if (!((op = *src) & 0xc0) && (skip = op & 0x3f) != 0) {
            src++;
            skip <<= 6;
            if ((uint8_t)mode & 2)
                x += skip;
            else
                x -= skip;
        }
    }
}
