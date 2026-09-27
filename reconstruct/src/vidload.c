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
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **This module's `_DATA`**, DGROUP 0x48f8..0x495c.
 *
 * The block the adapter's driver is read into, 0x48f8: a `huge` pointer,
 * which is why the tests against it call the runtime's `F_PCMP@`.
 */
uint8_t huge *VIDEO_DRIVER DGROUP_WAS(0x48f8) = 0;

/* "BAD:", 0x48fc, which the sixth tag points back at. */
char BAD_ADAPTER_TAG[] DGROUP_AT(0x48fc) = "BAD:";

/*
 * **The adapter tags**, 0x4901..0x4919: twelve near pointers, indexed from 1
 * - the compiler folds the first index into the base, `[si*2 + 0x48ff]`. The
 * tags themselves are the literal pool at 0x4923.
 */
char *ADAPTER_TAG[12] DGROUP_WAS(0x4901) = {
    "CGA:", "EGA:", "TAN:", "HER:", "MCG:", BAD_ADAPTER_TAG,
    "EVA:", "VGA:", "EVG:", "HVG:", "HEG:", "NEW:",
};

/*
 * **The overlay chunk's name**, 0x4919: "OVL:" and room for the tag, which
 * `load_video_driver` copies in at +4 before it searches.
 */
char OVL_TAG[] DGROUP_AT(0x4919) = "OVL:     ";

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

struct engine_scale_step ENGINE_SCALE_STEP DGROUP_BSS(0x628e);

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
 * *The row buffer* is 0x172 bytes on the stack: a literal run is decoded into
 * it and handed to the driver whole, and a solid run never touches it.
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
    uint8_t scratch[320];                        /* [bp-0x172] */
    int32_t vstep32[2];    /* [bp-0x2a], the accumulator and the step, 16.16 */
    uint8_t *vpage;    /* [bp-0x1e], the page, as the aperture address of its row 0 */
    int16_t vrow;    /* [bp-0x1c] */
    uint8_t vclip;    /* [bp-0x1a] */
    uint8_t vrowok;    /* [bp-0x19] */
    /* A cursor into `scratch`, not storage - see the note on the same slot
       in `draw_compressed_bitmap`. The original keeps it in two frame bytes
       because it has nowhere else; nothing outside the frame reads it. */
    uint8_t * vp;                                /* [bp-0x18] */
    int16_t vcut;    /* [bp-0x16] */
    int16_t vx2;    /* [bp-0x14] */
    int16_t vydir;    /* [bp-0x12] */
    int16_t vcol;    /* [bp-0x10] */
    const uint8_t *vsrc;    /* [bp-0xa], the source; the original steps the offset alone */
    uint8_t vbase;    /* [bp-0x21] */
    uint8_t vcolour;    /* [bp-0x22] */
    int16_t vn;    /* [bp-4] */
    int16_t vop;    /* [bp-2] */
    int16_t vx0;    /* [bp-0x30] */
    int16_t vxrow;    /* [bp-0x2e] */
    int16_t vcolrow;    /* [bp-0x32] */
    int16_t vrowacc;    /* [bp-0x2c] */
    const uint8_t *vsrcrow;    /* [bp-0xe], the row's start */
    int16_t *vrepeat = &vcut;   /* the same slot as `vcut` */    /* [bp-0x16], reused */
    /*
     * [bp-6], and it has to be its own slot. The skipped-row loop at 0x22d94
     * keeps its scaled delta here - `mov [bp-6], ax` at 0x22db0 - while the
     * count of rows still to skip sits in [bp-0x16]. Writing the delta through
     * `vcut`, which *is* [bp-0x16], overwrote the counter with a pixel
     * distance: the loop then skipped as many source rows as the sprite was
     * wide and the decode walked off into the next rows' tags. Every scaled
     * part on the briefing screen came out as a smear.
     */
    int16_t vdelta;    /* [bp-6] */
    int16_t  i, j;

    if (w == 0 || h == 0) {
        return;
    }

    if (w < 0) {
        w = (int16_t)-w;
        x = (int16_t)(x - w);
        mode ^= 2;
    }
    if (h < 0) {
        h = (int16_t)-h;
        y = (int16_t)(y - h);
        mode ^= 1;
    }

    /*
     * The same do-nothing vector `draw_compressed_bitmap` calls, kept for the
     * same reason: a build whose 0x3f72 is clear must not be silently
     * different from one whose is set.
     */
    vpage = MK_FP(VMDS.page_dst_ptr, 0);
    if (VMDS.page_hook != 0)
        vm_nothing();

    vclip = VMDS.clip_enabled;
    if (vclip != 0
        && x >= VMDS.clip_left && (int16_t)(x + w) <= VMDS.clip_right
        && y >= VMDS.clip_top && (int16_t)(y + h) <= VMDS.clip_bottom)
        vclip = 0;

    if (mode & 2)
        x = (int16_t)(x + w - 1);

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
    vstep32[0] = 0;
    vstep32[1] = (int32_t)((uint32_t)(uint16_t)(w) << 16);
    compute_step(vstep32, bmp->width);

    i = 0;
    j = 0;
    while (bmp->width >= i) {
        int16_t at = (int16_t)((uint32_t)vstep32[0] >> 16);

        if (at > w)
            at = w;
        ENGINE_SCALE_TABLE.entry[i] = at;

        vstep32[0] += vstep32[1];

        while (j < at) {
            ENGINE_ROW_OFFSETS.row[j] = (uint16_t)(i - 1);
            j++;
        }
        i++;
    }

    vrowacc = 0;

    if (mode & 1) {
        vydir = -1;
        y = (int16_t)(y + h - 1);
    } else {
        vydir = 1;
    }

    if (vclip != 0) {
        vrowok = (y <= VMDS.clip_bottom && y >= VMDS.clip_top) ? 1 : 0;
        if (vrowok != 0)
            vrow = (int16_t)VMDS.row_offset[y];
    } else {
        vrow = (int16_t)VMDS.row_offset[y];
    }

    vsrc = dg_far_ptr_rev(bmp->data);

    vbase = *vsrc;
    vsrc++;

    vx0   = x;
    vxrow = x;
    ENGINE_SCALE_STEP.base = 0;
    vcolrow = 0;
    ENGINE_SCALE_STEP.word_6290 = (uint16_t)ENGINE_SCALE_TABLE.entry[0];

    vsrcrow = vsrc;

    vstep32[0] = 0;
    vstep32[1] = (int32_t)((uint32_t)(uint16_t)(bmp->height - 1) << 16);
    compute_step(vstep32, (int16_t)(h - 1));

    for (;;) {
        vop = *vsrc;
        vsrc++;

        if ((vop & 0x80) && (vop & 0x40)) {
            /* 0x22997 - a run of nibbles, decoded into the row buffer. */
            vop &= 0x3f;
            vn = scale_table_delta(vop);

            if (vop != 0) {
                int16_t  at    = ENGINE_SCALE_TABLE.entry[ENGINE_SCALE_STEP.base];
                int16_t  first = (int16_t)ENGINE_ROW_OFFSETS.row[at];
                uint8_t *  out   = scratch;
                int16_t  k     = vn;
                int16_t  col   = at;

                while (k-- > 0) {
                    int16_t rel = (int16_t)((int16_t)ENGINE_ROW_OFFSETS.row[col] - first);
                    uint16_t byte_at = (uint16_t)((uint16_t)rel >> 1);
                    uint8_t  b = vsrc[byte_at];

                    /*
                     * `shr` puts bit 0 in the carry and `jae` takes the even
                     * column, so an even column is the *high* nibble.
                     */
                    *out = (uint8_t)(((rel & 1) ? (b & 0x0f) : (b >> 4))
                                     + vbase);
                    out++;
                    col++;
                }

                vsrc += ((vop + 1) >> 1);
            }

            ENGINE_SCALE_STEP.base = (uint16_t)(ENGINE_SCALE_STEP.base + vop);
            if (vn == 0)
                continue;

            vp = scratch;

            if (mode & 2) {
                vx2 = (int16_t)(x - vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto next_run;
                    if (!(vx2 >= VMDS.clip_left && x < VMDS.clip_right)) {
                        if (vx2 < VMDS.clip_left) {
                            vcut = (int16_t)(VMDS.clip_left - vx2);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_run;
                        } else {
                            /* The `add` at 0x22ab4, as written. */
                            vcut = (int16_t)(x + VMDS.clip_right);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_run;
                            vp = vp + vcut;
                            x = VMDS.clip_right;
                        }
                    }
                }

                vm_blit_run((uint16_t)x, (uint16_t)vn,
                            vp,
                            vpage + (uint16_t)vrow, 1);
            } else {
                vx2 = (int16_t)(x + vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto next_run;
                    if (!(x >= VMDS.clip_left && vx2 <= VMDS.clip_right)) {
                        if (x < VMDS.clip_left) {
                            vcut = (int16_t)(VMDS.clip_left - x);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_run;
                            vp = vp + vcut;
                            x = VMDS.clip_left;
                        } else {
                            vcut = (int16_t)(vx2 - VMDS.clip_right - 1);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_run;
                        }
                    }
                }

                vm_blit_run((uint16_t)x, (uint16_t)vn,
                            vp,
                            vpage + (uint16_t)vrow, 0);
            }

next_run:
            x = vx2;
            continue;
        }

        if (vop & 0x80) {
            /* 0x22b5b - a solid run: one colour byte, plus the base. */
            vop &= 0x3f;
            vn = scale_table_delta(vop);
            ENGINE_SCALE_STEP.base = (uint16_t)(ENGINE_SCALE_STEP.base + vop);

            vcolour = *vsrc;
            vsrc++;

            if (mode & 2) {
                vx2 = (int16_t)(x - vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto next_solid;
                    if (!(vx2 >= VMDS.clip_left && x < VMDS.clip_right)) {
                        if (vx2 < VMDS.clip_left) {
                            vcut = (int16_t)(VMDS.clip_left - vx2);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_solid;
                        } else {
                            vcut = (int16_t)(x - VMDS.clip_right);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_solid;
                            x = VMDS.clip_right;
                        }
                    }
                }

                vm_span((uint16_t)(uint8_t)(vbase + vcolour),
                        (uint16_t)(x - vn + 1), vn,
                        vpage + (uint16_t)vrow);
            } else {
                vx2 = (int16_t)(x + vn);

                if (vclip != 0) {
                    if (vrowok == 0)
                        goto next_solid;
                    if (!(x >= VMDS.clip_left && vx2 <= VMDS.clip_right)) {
                        if (x < VMDS.clip_left) {
                            vcut = (int16_t)(VMDS.clip_left - x);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_solid;
                            x = (int16_t)(x + vcut);
                        } else {
                            vcut = (int16_t)(vx2 - VMDS.clip_right - 1);
                            vn = (int16_t)(vn - vcut);
                            if (vn <= 0)
                                goto next_solid;
                        }
                    }
                }

                vm_span((uint16_t)(uint8_t)(vcolour + vbase),
                        (uint16_t)x, vn,
                vpage + (uint16_t)vrow);
            }

next_solid:
            x = vx2;
            continue;
        }

        if (vop & 0x40) {
            /* 0x22c96 - a move along the row; a count of zero ends it all. */
            vop &= 0x3f;
            if (vop == 0)
                break;

            vn = scale_table_delta(vop);
            ENGINE_SCALE_STEP.base = (uint16_t)(ENGINE_SCALE_STEP.base + vop);

            if (mode & 2)
                x = (int16_t)(x - vn);
            else
                x = (int16_t)(x + vn);
            continue;
        }

        /* 0x22cc9 - the end of a row. */
        vop &= 0x3f;
        vn = scale_table_delta((int16_t)-vop);
        if (vn < 0)
            vn = (int16_t)-vn;
        ENGINE_SCALE_STEP.base = (uint16_t)(ENGINE_SCALE_STEP.base - vop);

        if (mode & 2)
            x = (int16_t)(x + vn);
        else
            x = (int16_t)(x - vn);

        /*
         * Peek at the next tag without consuming it: only one with both top
         * bits clear is taken here, as a second move of its low six bits
         * shifted up by six.
         */
        vop = *vsrc;
        if ((vop & 0xc0) == 0) {
            vcol = (int16_t)(vop & 0x3f);
            if (vcol != 0) {
                vsrc++;
                vcol = (int16_t)(vcol << 6);
                vn = scale_table_delta(vcol);
                ENGINE_SCALE_STEP.base = (uint16_t)(ENGINE_SCALE_STEP.base - vcol);
                if (mode & 2)
                    x = (int16_t)(x + vn);
                else
                    x = (int16_t)(x - vn);
            }
        }

        /* 0x22d45 - step the row accumulator and see how many rows it covers. */
        vstep32[0] += vstep32[1];

        vx2 = (int16_t)((uint32_t)vstep32[0] >> 16);

        if (vrowacc == vx2) {
            /*
             * The scaled row lands on the same destination row as the last
             * one, so this source row is not drawn at all: the source pointer,
             * x and the column index all go back to where the row began.
             */
            vsrc = vsrcrow;
            x = vxrow;
            ENGINE_SCALE_STEP.base = (uint16_t)vcolrow;
        } else {
            int16_t repeat = (int16_t)(vx2 - vrowacc);

            if (repeat < 0)
                repeat = (int16_t)-repeat;
            repeat--;

            vrepeat[0] = repeat;

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
            while (vrepeat[0] != 0) {
                vop = *vsrc;
                vsrc++;
                vn = (int16_t)(vop & 0x3f);
                vdelta = scale_table_delta(vn);
                if (mode & 2)
                    vdelta = (int16_t)-vdelta;

                if (vop & 0x80) {
                    ENGINE_SCALE_STEP.base = (uint16_t)(ENGINE_SCALE_STEP.base + vn);
                    x = (int16_t)(x + vdelta);
                    if (vop & 0x40)
                        vsrc += ((vn + 1) >> 1);
                    else
                        vsrc++;
                } else if (vop & 0x40) {
                    if (vn == 0)
                        goto done;
                    ENGINE_SCALE_STEP.base = (uint16_t)(ENGINE_SCALE_STEP.base + vn);
                    x = (int16_t)(x + vdelta);
                } else {
                    ENGINE_SCALE_STEP.base = (uint16_t)(ENGINE_SCALE_STEP.base - vn);
                    x = (int16_t)(x - vdelta);

                    vop = *vsrc;
                    if ((vop & 0xc0) == 0) {
                        vcol = (int16_t)(vop & 0x3f);
                        if (vcol != 0) {
                            vsrc++;
                            vcol = (int16_t)(vcol << 6);
                            vn = scale_table_delta(vcol);
                            ENGINE_SCALE_STEP.base =
                                (uint16_t)(ENGINE_SCALE_STEP.base - vcol);
                            if (mode & 2)
                                x = (int16_t)(x + vn);
                            else
                                x = (int16_t)(x - vn);
                        }
                    }
                    vrepeat[0] = (int16_t)(vrepeat[0] - 1);
                }
            }
        }

        /* 0x22e73 - the row is finished; remember where the next one begins. */
        vsrcrow = vsrc;
        vrowacc = vx2;
        vxrow   = x;
        vcolrow = (int16_t)ENGINE_SCALE_STEP.base;

        h--;
        if (h == 0)
            break;

        {
            int16_t back = ENGINE_SCALE_TABLE.entry[ENGINE_SCALE_STEP.base];

            if (mode & 2)
                back = (int16_t)-back;
            x = (int16_t)(vx0 + back);
        }

        y = (int16_t)(y + vydir);

        if (vclip != 0) {
            vrowok = (y <= VMDS.clip_bottom && y >= VMDS.clip_top) ? 1 : 0;
            if (vrowok == 0)
                continue;
        }

        vrow = (int16_t)VMDS.row_offset[y];
    }

done:
    ;
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
        return FAR_NULL_PTR;

    string_copy_far(OVL_TAG + 4, ADAPTER_TAG[adapter - 1]);

    if (seek_named_chunk(di, OVL_TAG, 0) == -1L)
        return FAR_NULL_PTR;
    if ((handle = open_resource(0xffff, di, "r", file_record_size(di))) < 0)
        return FAR_NULL_PTR;

    len = resource_size(handle);

    if (VIDEO_DRIVER != FAR_NULL_PTR)
        dos_free_far(VIDEO_DRIVER);

    if ((VIDEO_DRIVER = DOS_ALLOC_PTR(DOS_ALLOC(len, 0))) != FAR_NULL_PTR) {
        read_resource(handle, VIDEO_DRIVER, (uint16_t)len);
        close_resource(handle);

        if (opened != 0)
            close_file_record(di);

        return VIDEO_DRIVER;
    }

    return FAR_NULL_PTR;
}
