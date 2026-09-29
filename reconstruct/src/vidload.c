/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The scaled blit, and loading the video driver out of VM.OVL.**
 *
 * A module of the original's **code segment 1c25**, image 0x22790..0x2307d,
 * split out of engine.c on 2026-09-27. Its `_DATA` is DGROUP
 * 0x48f8..0x495c, after `vm_init`'s and before the font loader's, and its
 * `_BSS` is the scale step at 0x628e, just before files.c's. No call and no
 * data reference separates the scaled blit from the driver loader, so they
 * are one file. Functions are in address order and each carries the image
 * offset it was read from.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -G -O
 * JUDGE: data 0x48f8..0x495c
 * JUDGE: via-assembler
 * JUDGE: assembler bc3.00
 *
 * **The module went through the assembler**: `blit_scaled_a` has four
 * inline `asm` blocks - its nibble decoder and its three calls into the
 * driver, which take their arguments in registers - and the image has
 * TASM's `jmp` / `nop` wherever the compiler could not size a forward jump.
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **This module's `_DATA`**, DGROUP 0x48f8..0x495c.
 *
 * The block the adapter's driver is read into, 0x48f8: a `huge` pointer,
 * which is why the tests against it call the runtime's `F_PCMP@`.
 */
uint8_t huge *VIDEO_DRIVER = 0;

/* "BAD:", 0x48fc, which the sixth tag points back at. */
char BAD_ADAPTER_TAG[] = "BAD:";

/*
 * **The adapter tags**, 0x4901..0x4919: twelve near pointers, indexed from 1
 * - the compiler folds the first index into the base, `[si*2 + 0x48ff]`. The
 * tags themselves are the literal pool at 0x4923.
 */
char *ADAPTER_TAG[12] = {
    "CGA:", "EGA:", "TAN:", "HER:", "MCG:", BAD_ADAPTER_TAG,
    "EVA:", "VGA:", "EVG:", "HVG:", "HEG:", "NEW:",
};

/*
 * **The overlay chunk's name**, 0x4919: "OVL:" and room for the tag, which
 * `load_video_driver` copies in at +4 before it searches.
 */
char OVL_TAG[] = "OVL:     ";

/*
 * **The base the two indexes are taken from**, DGROUP 0x628e..0x6292, 0x04 bytes.
 */
struct engine_scale_step {
    uint16_t  base;               /* +0x00 [2]  one `n` further on, less the one this indexes */
    /* Written once, at the top of `blit_scaled_a`, with the scale table's
       first entry, and never read - by this routine or any other in the port.
       Named by address because a lone store says nothing more. */
    uint16_t  word_6290;          /* +0x02 [2] */
} PACKED;

struct engine_scale_step ENGINE_SCALE_STEP;

#ifdef __TURBOC__
/*
 * **This module's `MK_FP` is Turbo C 2.0's**, as files.c's is: the `cwd`
 * before the bitmap's pointer is built is the segment widened to a long.
 */
#undef MK_FP
#define MK_FP(seg, ofs) ((void far *)(((uint32_t)(seg) << 16) | (uint16_t)(ofs)))
#endif

/*
 * 0x22790
 *
 * The distance between two entries of the scaling table at DGROUP 0x5956,
 * both indexed from the base at 0x628e: the one `n` further on, less the one
 * at the base.
 *
 * A **** routine - `ret`, not `retf` - so its argument is at bp+4 and not
 * bp+6. The scaled blitter calls it seven times.
 */
int16_t near scale_table_delta(int16_t n)
{
    return ENGINE_SCALE_TABLE.entry[ENGINE_SCALE_STEP.base + n]
           - ENGINE_SCALE_TABLE.entry[ENGINE_SCALE_STEP.base];
}

/*
 * 0x227ac
 *
 * **Draw a compressed bitmap scaled.** Every part of the machine reaches the
 * screen through this: 1873 bytes, entered 57 times to paint the level-one
 * briefing alone.
 *
 * *The size arguments are also the mirrors.* A zero width or height draws
 * nothing. A **negative** one is a flip: the value is made positive with the
 * branchless `cwd`/`xor`/`sub`, the origin moved back by the new size, and the
 * mode xored with the matching mirror bit. A caller asks for a mirrored part
 * by passing a negative size.
 *
 * *Two column tables, built once.* `compute_step` divides the source width
 * into the destination width and the accumulator walks across it, filling
 * DGROUP 0x5956 with the destination x of each source column and 0x5e56 with
 * the source column of each destination x. Every row after that is a lookup,
 * and 0x628e indexes the first of them.
 *
 * *The clip is decided once per row, not per pixel.* The flag at 0x3893 is
 * copied on entry and **cleared** when the whole rectangle is inside the box,
 * so a bitmap that cannot be clipped pays no test.
 *
 * *The compression*, a byte at a time, the top two bits choosing - and this is
 * the same encoding `draw_compressed_bitmap` at 0x20185 decodes unscaled, which
 * is what settled it:
 *
 *   11  a literal run of `n` source pixels, each a **nibble**. Which half is
 *       taken is chosen without a branch on parity - the source column less
 *       the run's first is shifted right by one and the *carry* picks it - and
 *       the palette base from the header's first byte is added before the
 *       pixel reaches the row buffer. The stream advances by `(n + 1) / 2`.
 *   10  a solid run: one more byte is the colour, again plus the base.
 *   01  a move along the row; **a count of zero ends the whole bitmap**.
 *   00  the end of a row, followed by an optional second move of its low six
 *       bits shifted up by six - peeked at and only consumed if both top bits
 *       are clear.
 *
 * *The row buffer* is 320 bytes at the bottom of a 0x172-byte frame: a
 * literal run is decoded into it and handed to the driver whole, and a solid
 * run never touches it.
 *
 * *A run is clipped by trimming it*, not by testing pixels: the overhang past
 * either edge is subtracted from the length and added to the buffer pointer,
 * and a run trimmed to nothing is skipped.
 *
 * **And the two mirrored trims are not written the same way.** Trimming a
 * mirrored *literal* run at the right edge computes its cut as
 * `x + VMDS.clip_right` at 0x22ab4 - `03 06 96 38`, an `add` - where the mirrored
 * *solid* run at 0x22bf3 computes `x - VMDS.clip_right`, `2b 06 96 38`, a `sub`.
 * The bytes were checked rather than the listing read twice. Only the second
 * is an overhang; the first is the sum of two coordinates and can only be a
 * mistake in the original. It is transcribed as the `add` it is - the rule
 * here is to transcribe, and a port that quietly corrected it would draw a
 * mirrored literal run differently from the game when one overhangs the right
 * edge of the clip box.
 *
 * *The vertical mirror is a step and the horizontal an origin.* Bit 0 makes
 * the row step -1 and moves y to the far edge; bit 1 leaves the decode alone
 * and changes where the finished row goes, and is what selects the driver's
 * mirrored entry - `stc` rather than `clc`.
 *
 * **The tag encoding was recorded inverted once and is corrected above.** The
 * first reading had 00 as the literal run and 11 as a skip, from following the
 * `jne` at 0x22988 the wrong way: it jumps when bit 7 is *set*, so the fall
 * through to 0x22c8f is the bit-7-clear case. Comparing with 0x20185, which
 * decodes the same format without scaling, is what caught it.
 */
void blit_scaled_a(struct bitmap *bmp, int16_t x, int16_t y,
                   uint16_t mode, int16_t w, int16_t h)
{
    int16_t op;                         /* [bp-2] */
    int16_t n;                          /* [bp-4] */
    /*
     * [bp-6], and it has to be its own slot. The skipped-row loop at 0x22d94
     * keeps its scaled delta here - `mov [bp-6], ax` at 0x22db0 - while the
     * count of rows still to skip sits in `cut`. Writing the delta through
     * `cut` overwrote the counter with a pixel distance: the loop then
     * skipped as many source rows as the sprite was wide and the decode
     * walked off into the next rows' tags. Every scaled part on the briefing
     * screen came out as a smear.
     */
    int16_t delta;                      /* [bp-6] */
    uint8_t far *src;                   /* [bp-0xa] */
    uint8_t far *srcrow;                /* [bp-0xe], the row's start */
    uint16_t col;                       /* [bp-0x10] */
    int16_t ydir;                       /* [bp-0x12] */
    int16_t x2;                         /* [bp-0x14] */
    /* The overhang a run is trimmed by, and in the skipped-row loop the
       number of rows still to skip: the original's one slot for both. */
    int16_t cut;                        /* [bp-0x16] */
    uint8_t *p;                         /* [bp-0x18], a cursor into `buf` */
    uint8_t rowok;                      /* [bp-0x19] */
    int8_t clip;                        /* [bp-0x1a] */
#ifdef __TURBOC__
    int16_t row;                        /* [bp-0x1c] */
#else
    /* Ours: only read once `rowok` says it was set, which gcc cannot follow. */
    int16_t row = 0;
#endif
#ifdef __TURBOC__
    uint16_t page;                      /* [bp-0x1e], the page's segment */
#else
    uint8_t *page;
#endif
    int16_t at;                         /* [bp-0x20] */
    uint8_t base;                       /* [bp-0x21] */
    uint8_t colour;                     /* [bp-0x22] */
    union scale_step rec;               /* [bp-0x2a] */
    int16_t rowacc;                     /* [bp-0x2c] */
    int16_t xrow;                       /* [bp-0x2e] */
    int16_t x0;                         /* [bp-0x30] */
    int16_t colrow;                     /* [bp-0x32] */
    uint8_t buf[320];                   /* [bp-0x172] */

    if (w == 0)
        return;
    if (h == 0)
        return;

    if (w < 0) {
        x -= w = abs(w);
        mode ^= 2;
    }
    if (h < 0) {
        y -= h = abs(h);
        mode ^= 1;
    }

    /*
     * The same do-nothing vector `draw_compressed_bitmap` calls, kept for the
     * same reason: a build whose 0x3f72 is clear must not be silently
     * different from one whose is set. The page goes in and comes back in
     * `ax`.
     */
#ifdef __TURBOC__
    _AX = VMDS.page_dst;
    if (VMDS.page_hook != 0) {
        asm push ax
        asm call dword ptr VM_DRIVER+074h
        asm add sp, 2
    }
    page = _AX;
#else
    page = vga_window_at(VMDS.page_dst, 0);
    if (VMDS.page_hook != 0)
        vm_nothing();
#endif

    if ((clip = VMDS.clip_enabled) != 0
        && x >= VMDS.clip_left && x + w <= VMDS.clip_right
        && y >= VMDS.clip_top && y + h <= VMDS.clip_bottom)
        clip = 0;

    if (mode & 2)
        x += w - 1;

    /*
     * The two column tables. `compute_step` puts the destination-per-source
     * step in the accumulator's high half, and walking it across the source
     * width fills 0x5956 with where each source column lands and 0x5e56 with
     * which source column each destination pixel came from.
     *
     * **The record's words are +2 and +6, not +0 and +2.** `compute_step`
     * clears +0 and +4 itself and takes the span from `+6 - +2`, so the caller
     * writes the two ends there; 0x2284c and 0x22854 are `[bp-0x28]` and
     * `[bp-0x24]` against a record at `[bp-0x2a]`. Writing +0 and +2 instead
     * left +6 holding whatever was there, and the step came out large enough
     * that the first source column already mapped past the end of the
     * destination - which filled 0x5e56 with -1 and made the row buffer
     * overrun. The row step below has the same two slots.
     */
    rec.w[1] = 0;
    rec.w[3] = w;
    compute_step(&rec, bmp->width);

    for (col = x2 = 0; bmp->width >= x2; x2++) {
        ENGINE_SCALE_TABLE.entry[x2] = at = rec.w[1] < w ? rec.w[1] : w;
        rec.l[0] += rec.l[1];
        while (col < (uint16_t)at) {
            ENGINE_ROW_OFFSETS.row[col] = x2 - 1;
            col++;
        }
    }

    rowacc = 0;

    if ((uint8_t)mode & 1) {
        ydir = -1;
        y += h - 1;
    } else {
        ydir = 1;
    }

    if (clip == 0
        || (rowok = y <= VMDS.clip_bottom && y >= VMDS.clip_top) != 0)
        row = VMDS.row_offset[y];

    src = MK_FP((dg_sseg_t)bmp->data_seg, bmp->data_off);

    base = *src;
    src++;

    x0 = x;
    xrow = x;
    colrow = ENGINE_SCALE_STEP.base = 0;
    ENGINE_SCALE_STEP.word_6290 = ENGINE_SCALE_TABLE.entry[ENGINE_SCALE_STEP.base];

    srcrow = src;

    rec.w[1] = 0;
    rec.w[3] = bmp->height - 1;
    compute_step(&rec, h - 1);

    for (;;) {
        op = *src++;

        if (op & 0x80) {
            if (op & 0x40) {
                /* 0x22997 - a run of nibbles, decoded into the row buffer. */
                op &= 0x3f;
                n = op;
                n = scale_table_delta(n);

                if (op != 0) {
                    at = ENGINE_SCALE_TABLE.entry[ENGINE_SCALE_STEP.base];
                    col = ENGINE_ROW_OFFSETS.row[at];
                    /*
                     * `shr` puts bit 0 of the column's distance from the
                     * run's first in the carry and `jae` takes the even
                     * column, so an even column is the *high* nibble.
                     */
#ifdef __TURBOC__
                    asm push di
                    asm push si
                    asm lea di, buf
                    asm mov bx, at
                    asm shl bx, 1
                    asm mov cx, n
                    asm or cx, cx
                    asm jle decoded
                    asm mov dl, base
decode:
                    asm les si, src
                    asm mov ax, word ptr ENGINE_ROW_OFFSETS[bx]
                    asm sub ax, col
                    asm shr ax, 1
                    asm jae high
                    asm add si, ax
                    asm mov al, es:[si]
                    asm mov si, ds
                    asm mov es, si
                    asm and al, 0fh
                    asm add al, dl
                    asm stosb
                    asm add bx, 2
                    asm loop decode
                    asm jmp short decoded
high:
                    asm add si, ax
                    asm mov al, es:[si]
                    asm mov si, ds
                    asm mov es, si
                    asm shr al, 1
                    asm shr al, 1
                    asm shr al, 1
                    asm shr al, 1
                    asm add al, dl
                    asm stosb
                    asm add bx, 2
                    asm loop decode
decoded:
                    asm pop si
                    asm pop di
#else
                    {
                        uint8_t *out = buf;
                        uint16_t *e = &ENGINE_ROW_OFFSETS.row[at];
                        int16_t k;

                        for (k = n; k > 0; k--, e++) {
                            uint16_t rel = (uint16_t)(*e - col);
                            uint8_t b = src[rel >> 1];

                            *out++ = (uint8_t)(((rel & 1) ? (b & 0x0f)
                                                          : (b >> 4)) + base);
                        }
                    }
#endif
                    src += (op + 1) >> 1;
                }

                ENGINE_SCALE_STEP.base += op;
                if (n == 0)
                    continue;

                p = buf;

                if ((uint8_t)mode & 2) {
                    x2 = x - n;
                    if (!clip)
                        goto draw_mirrored;
                    if (rowok == 0)
                        goto next_run;
                    if (x2 < VMDS.clip_left || x >= VMDS.clip_right)
                        goto trim_mirrored;
draw_mirrored:
#ifdef __TURBOC__
                    asm push si
                    asm push di
                    asm mov cl, byte ptr n
                    asm xor ah, ah
                    asm mov ch, ah
                    asm mov si, p
                    asm mov di, row
                    asm mov bx, x
                    asm mov es, page
                    asm stc
                    asm mov dx, y
                    asm call dword ptr VM_DRIVER+09ch
                    asm pop di
                    asm pop si
#else
                    vm_blit_run((uint16_t)x, (uint8_t)n, p,
                                page + (uint16_t)row, 1);
#endif
                    goto next_run;
trim_mirrored:
                    if (x2 < VMDS.clip_left) {
                        cut = VMDS.clip_left - x2;
                        if ((n -= cut) > 0)
                            goto draw_mirrored;
                        goto next_run;
                    }
                    /* The `add` at 0x22ab4, as written. */
                    cut = x + VMDS.clip_right;
                    if ((n -= cut) > 0) {
                        p += cut;
                        x = VMDS.clip_right;
                        goto draw_mirrored;
                    }
                } else {
                    x2 = x + n;
                    if (!clip)
                        goto draw;
                    if (rowok == 0)
                        goto next_run;
                    if (x < VMDS.clip_left || x2 > VMDS.clip_right)
                        goto trim;
draw:
#ifdef __TURBOC__
                    asm push si
                    asm push di
                    asm mov cl, byte ptr n
                    asm xor ah, ah
                    asm mov ch, ah
                    asm mov si, p
                    asm mov di, row
                    asm mov bx, x
                    asm mov es, page
                    asm clc
                    asm mov dx, y
                    asm call dword ptr VM_DRIVER+09ch
                    asm pop di
                    asm pop si
#else
                    vm_blit_run((uint16_t)x, (uint8_t)n, p,
                                page + (uint16_t)row, 0);
#endif
                    goto next_run;
trim:
                    if (x < VMDS.clip_left) {
                        cut = VMDS.clip_left - x;
                        if ((n -= cut) > 0) {
                            p += cut;
                            x = VMDS.clip_left;
                            goto draw;
                        }
                        goto next_run;
                    }
                    cut = x2 - VMDS.clip_right - 1;
                    if ((n -= cut) <= 0)
                        goto next_run;
                    goto draw;
                }
next_run:
                x = x2;
                continue;
            }

            /* 0x22b5b - a solid run: one colour byte, plus the base. */
            op &= 0x3f;
            n = scale_table_delta(op);
            ENGINE_SCALE_STEP.base += op;
            colour = *src++;

            if ((uint8_t)mode & 2) {
                x2 = x - n;
                if (!clip)
                    goto fill_mirrored;
                if (rowok == 0)
                    goto next_solid;
                if (x2 < VMDS.clip_left || x >= VMDS.clip_right)
                    goto trim_solid_mirrored;
fill_mirrored:
#ifdef __TURBOC__
                asm push di
                asm mov al, base
                asm add al, colour
                asm xor ah, ah
                asm mov ch, ah
                asm mov bx, x
                asm mov cl, byte ptr n
                asm sub bx, cx
                asm inc bx
                asm mov di, row
                asm mov es, page
                asm mov dx, y
                asm call dword ptr VM_DRIVER+02ch
                asm pop di
#else
                vm_span((uint8_t)(base + colour),
                        (uint16_t)(x - (uint8_t)n + 1), (uint8_t)n,
                        page + (uint16_t)row);
#endif
                goto next_solid;
trim_solid_mirrored:
                if (x2 < VMDS.clip_left) {
                    cut = VMDS.clip_left - x2;
                    if ((n -= cut) > 0)
                        goto fill_mirrored;
                    goto next_solid;
                }
                cut = x - VMDS.clip_right;
                if ((n -= cut) <= 0)
                    goto next_solid;
                x = VMDS.clip_right;
                goto fill_mirrored;
            } else {
                x2 = x + n;
                if (!clip)
                    goto fill;
                if (rowok == 0)
                    goto next_solid;
                if (x < VMDS.clip_left || x2 > VMDS.clip_right)
                    goto trim_solid;
fill:
#ifdef __TURBOC__
                asm push di
                asm mov al, colour
                asm add al, base
                asm xor ah, ah
                asm mov ch, ah
                asm mov bx, x
                asm mov cl, byte ptr n
                asm mov di, row
                asm mov es, page
                asm mov dx, y
                asm call dword ptr VM_DRIVER+02ch
                asm pop di
#else
                vm_span((uint8_t)(colour + base), (uint16_t)x, (uint8_t)n,
                        page + (uint16_t)row);
#endif
                goto next_solid;
trim_solid:
                if (x < VMDS.clip_left) {
                    cut = VMDS.clip_left - x;
                    if ((n -= cut) > 0) {
                        x += cut;
                        goto fill;
                    }
                    goto next_solid;
                }
                cut = x2 - VMDS.clip_right - 1;
                if ((n -= cut) > 0)
                    goto fill;
            }
next_solid:
            x = x2;
            continue;
        }

        if (op & 0x40) {
            /* 0x22c96 - a move along the row; a count of zero ends it all. */
            if ((op &= 0x3f) == 0)
                return;
            n = scale_table_delta(op);
            ENGINE_SCALE_STEP.base += op;
            if ((uint8_t)mode & 2)
                x -= n;
            else
                x += n;
            continue;
        }

        /* 0x22cc9 - the end of a row. */
        op &= 0x3f;
        n = abs(scale_table_delta(-op));
        ENGINE_SCALE_STEP.base -= op;
        if ((uint8_t)mode & 2)
            x += n;
        else
            x -= n;

        /*
         * Peek at the next tag without consuming it: only one with both top
         * bits clear is taken here, as a second move of its low six bits
         * shifted up by six.
         */
        if (((op = *src) & 0xc0) == 0 && (col = op & 0x3f) != 0) {
            src++;
            col <<= 6;
            n = scale_table_delta(col);
            ENGINE_SCALE_STEP.base = ENGINE_SCALE_STEP.base - col;
            if ((uint8_t)mode & 2)
                x += n;
            else
                x -= n;
        }

        /* 0x22d45 - step the row accumulator and see how many rows it covers. */
        rec.l[0] += rec.l[1];
        at = (int16_t)(rec.l[0] >> 16);

        if (rowacc == at) {
            /*
             * The scaled row lands on the same destination row as the last
             * one, so this source row is not drawn at all: the source pointer,
             * x and the column index all go back to where the row began.
             */
            src = srcrow;
            x = xrow;
            ENGINE_SCALE_STEP.base = colrow;
        } else if ((cut = abs(at - rowacc) - 1) != 0) {
            /*
             * 0x22d94 - a destination row covering more than one source row
             * still has to have those rows' tags stepped over, and their moves
             * applied, without drawing any of them.
             *
             * **Only the end-of-row tag counts.** Every branch of the body
             * jumps to the test at 0x22e6a, and just one of them - the tag
             * with both top bits clear, which is what ends a row - falls
             * through the `dec [bp-0x16]` at 0x22e67 on the way. So the
             * counter is a count of source *rows*, and the runs and moves
             * inside a row are consumed without touching it. Decrementing on
             * every tag skipped a row after one tag rather than after a row,
             * and the decode walked into the middle of the next row.
             */
            while (cut != 0) {
                op = *src++;
                n = op & 0x3f;
                delta = scale_table_delta(n);
                if ((uint8_t)mode & 2)
                    delta = -delta;

                if (op & 0x80) {
                    ENGINE_SCALE_STEP.base += n;
                    if (op & 0x40) {
                        x += delta;
                        src += (n + 1) >> 1;
                    } else {
                        x += delta;
                        src++;
                    }
                } else if (op & 0x40) {
                    if (n == 0)
                        return;
                    ENGINE_SCALE_STEP.base += n;
                    x += delta;
                } else {
                    ENGINE_SCALE_STEP.base -= n;
                    x -= delta;
                    if (((op = *src) & 0xc0) == 0 && (col = op & 0x3f) != 0) {
                        src++;
                        col <<= 6;
                        n = scale_table_delta(col);
                        ENGINE_SCALE_STEP.base = ENGINE_SCALE_STEP.base - col;
                        if ((uint8_t)mode & 2)
                            x += n;
                        else
                            x -= n;
                    }
                    cut--;
                }
            }
        }

        /* 0x22e73 - the row is finished; remember where the next one begins. */
        srcrow = src;
        rowacc = at;
        xrow = x;
        colrow = ENGINE_SCALE_STEP.base;

        if (--h == 0)
            return;

        x = x0 + (((uint8_t)mode & 2) ? -ENGINE_SCALE_TABLE.entry[ENGINE_SCALE_STEP.base]
                             : ENGINE_SCALE_TABLE.entry[ENGINE_SCALE_STEP.base]);
        y += ydir;

        if (clip == 0
            || (rowok = y <= VMDS.clip_bottom && y >= VMDS.clip_top) != 0)
            row = VMDS.row_offset[y];
    }
}

/*
 * 0x22efd
 *
 * Load the video driver for an adapter and answer it as a far pointer, or null.
 *
 * The adapter number picks both a **screen size** and a **driver name**, and it
 * does the first through a jump table in this code segment at `cs:0x6e15`,
 * twelve entries covering adapters 4 to 0xf. Anything outside that range takes
 * no default and simply keeps whatever DGROUP 0x3f7a and 0x3f7c already held.
 *
 * The mapping is not one-to-one: several adapters collapse onto driver 0xb or
 * 8, and adapter 4's first assignment of 1 is overwritten by 8 two instructions
 * later without ever being read - dead, and transcribed as such rather than
 * tidied away.
 *
 * The name is then built by copying one of the strings named by the table at
 * DGROUP 0x48ff into the buffer at 0x491d, which is the tail of the chunk path
 * at 0x4919. The chunk is found, its size asked for, a DOS block of that size
 * allocated - freeing whatever was there before - and the driver read into it.
 *
 * The file may arrive as a handle or a name, and one this routine opened is
 * closed again; one it was handed is left alone.
 */
uint8_t far *load_video_driver(register int16_t adapter, char *name)
{
    int32_t len;
    int16_t handle;
    int16_t opened;
    register FILE *di;

    opened = 0;

    switch (adapter) {
    case 0xc:
        adapter = 0xb;
        VMDS.screen.screen_height = 0x15e;
        break;
    case 0xd:
        adapter = 0xb;
        VMDS.screen.screen_height = 0x1e0;
        break;
    case 0xe:
        adapter = 0xb;
        VMDS.screen.screen_height = 0x190;
        break;
    case 4:
        adapter = 1;                    /* overwritten below, never read */
        VMDS.screen.screen_width = 0x280;
        /* falls through */
    case 0xf:
        adapter = 8;
        VMDS.screen.screen_height = 0x190;
        break;
    }

    /* A handle, or a name to open. */
    if (file_record_valid((FILE *)name) == 0) {
        opened = 1;
        di = open_file_record(name);
    } else {
        di = (FILE *)name;
    }

    if (di == 0)
        return NULL;

    strcpy_far(OVL_TAG + 4, ADAPTER_TAG[adapter - 1]);

    if (seek_named_chunk(di, OVL_TAG, 0) == -1L)
        return NULL;
    if ((handle = open_resource(0xffff, di, "r", file_record_size(di))) < 0)
        return NULL;

    len = resource_size(handle);

    if (VIDEO_DRIVER != NULL)
        dos_free_far(VIDEO_DRIVER);

    if ((VIDEO_DRIVER = dos_alloc_bytes(len, 0)) != NULL) {
        read_resource(handle, VIDEO_DRIVER, (uint16_t)len);
        close_resource(handle);

        if (opened != 0)
            close_file_record(di);

        return VIDEO_DRIVER;
    }

    return NULL;
}
