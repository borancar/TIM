/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The offset-table bitmap's draw**: segment 248f's second module, image
 * 0x24e9a..0x24f72, one routine.

 * **Segment 248f holds four modules, and this is one.** In the medium model
 * each file's code goes into a segment named for it unless it is told
 * otherwise, and these four were told the same name: three C files and the
 * assembly module in vqt.c, in that order. The boundaries are the image's
 * own evidence. Every far call from one routine of the segment to another is
 * `nop / push cs / call` - TLINK's rewrite of a `9A` - where Borland C++
 * calls a routine *defined earlier in the same file* with a bare
 * `push cs / call`, as it does 411 times elsewhere in the image. So
 * `draw_offset_bitmap` is not in `open_bit_reader`'s file, and `draw_bitmap`
 * is not in `draw_offset_bitmap`'s. The data agrees: each file's `_DATA` is
 * its own run of DGROUP.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -G -Z
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x24e9a
 *
 * Draw a bitmap stored as a quadtree - the form `load_bitmaps` gives a
 * "BMP:VQT:" file - mirrored as `mode` says, through `draw_vqt_flipped`.
 *
 * The bit reader is opened on the header's pixels, normalised; if it is busy
 * nothing is drawn. When the bitmap lies wholly inside the clip box the
 * leaves plot through the driver's own plot, the vector's slot 22, and
 * otherwise through `plot_pixel_clipped` with clipping switched on.
 *
 * The clip switch and the driver's two colour bytes - which a one-colour leaf
 * overwrites - are saved first and put back on every path, including an
 * `open_bit_reader` that refuses, which also leaves `BITMAPS.walk` 0.
 *
 * **Which far pointer reaches the reader** is Borland's `MK_FP` with its own
 * evaluation: the normalised segment and the offset's low nibble go into `p`,
 * and what the call is handed is whatever TCC made of that.
 */
void draw_offset_bitmap(struct bitmap *bmp, int16_t x, int16_t y, uint16_t mode)
{
    uint8_t far *p;
    uint8_t saved_clip;
    uint8_t saved_second;
    uint8_t saved_fill;
    int16_t seg;
    register int16_t w;
    register int16_t h;

    saved_second = VMDS.second_colour;
    saved_fill = VMDS.fill_colour;
    saved_clip = VMDS.clip_enabled;
    seg = bmp->data_seg + (bmp->data_off >> 4);
    p = FAR_OF_LONG(seg, bmp->data_off & 0xf);
    if ((BITMAPS.walk = open_bit_reader(BCC_FAR_ARG(p, seg))) != 0) {
        w = bmp->width;
        h = bmp->height;
        if (x >= VMDS.clip_left && y >= VMDS.clip_top
            && x + w <= VMDS.clip_right && y + h <= VMDS.clip_bottom)
            DG49BA.plot_fn = ((bmp_plot_fn)DG4342.font[22]);
        else {
            DG49BA.plot_fn = plot_pixel_clipped;
            VMDS.clip_enabled = 1;
        }
        BITMAPS.plot_zero = 0;
        BITMAPS.draw_flags = mode;
        draw_vqt_flipped(x, y, w, h);
        close_bit_reader();
    }
    VMDS.clip_enabled = saved_clip;
    VMDS.second_colour = saved_second;
    VMDS.fill_colour = saved_fill;
}
