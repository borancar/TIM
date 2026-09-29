/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The screenshot writer**: the screen saved as an IFF ILBM, 640 by 450 in
 * eight bitplanes with the DAC's 256 colours.
 *
 * The forty-fourth module of the original's **code segment 172c**, image
 * 0x1bd14..0x1c087, after the part kinds. Functions are in address order and
 * each carries the image offset it was read from.
 *
 * **Nothing in the game calls it.** No far call and no stored far pointer in
 * the image names `save_screenshot`, and the routines under it are reached
 * only from each other. It is a developer's tool left in the link, and it is
 * transcribed because its bytes are in the image. None of the names here is
 * the original's.
 *
 * The module ends where the assembly begins: `iff_write_cmap` calls
 * `vga_get_dac` and `iff_write_body` calls `chunky_to_planar` with TLINK's
 * `nop / push cs / call`, and what follows 0x1c087 borrows BP as a data
 * register, which no compiler does (vgadac.c).
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm
 * JUDGE: data 0x355a..0x3576
 *
 * **Borland C++ 2.0, not the 3.0 the part kinds were built with.** The one
 * byte that says so is `iff_write_be`'s `p + 2`: 2.0 adds 2 to a pointer as
 * `inc ax / inc ax` and 3.0 as `add ax,2`, and no spelling moves 3.0. TC++
 * 3.0 agrees with 3.0 and TC++ 1.01 with none of the rest. A file written
 * earlier and not rebuilt is the likely story.
 */
#include <stdlib.h>
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The IFF chunk names**, DGROUP 0x355a..0x3576, and the mode the file is
 * written with: this module's `_DATA`.
 */
struct iff_chunk_names IFF_CHUNK_NAMES = {
    "FORM", /* form */
    "ILBM", /* ilbm */
    "BMHD", /* bmhd */
    "CMAP", /* cmap */
    "BODY", /* body */
    "wb", /* mode_wb */
};

/*
 * 0x1bd14
 *
 * Write `count` values of `size` bytes each, **big-endian**, as IFF wants
 * them. A long is two words, high one first; a word is two bytes, high one
 * first; a byte is itself. Any other size writes nothing but still steps.
 */
void iff_write_be(uint8_t *p, int16_t count, int16_t size, FILE *f)
{
    while (count--) {
        if (size == 4) {
            iff_write_be(p + 2, 1, 2, f);
            iff_write_be(p, 1, 2, f);
        } else if (size == 2) {
            fwrite(p + 1, 1, 1, f);
            fwrite(p, 1, 1, f);
        } else if (size == 1) {
            fwrite(p, 1, 1, f);
        }
        p += size;
    }
}

/*
 * 0x1bdaa
 *
 * The `CMAP` chunk: all 256 colours read back out of the DAC, each six-bit
 * component shifted up to eight.
 */
void iff_write_cmap(FILE *f)
{
    int32_t len;
    uint8_t pal[0x300];
    int16_t i;

    fwrite((const uint8_t *)IFF_CHUNK_NAMES.cmap, 1, 4, f);
    vga_get_dac(pal, 0, 0x100);
    len = 0x300;
    iff_write_be((uint8_t *)&len, 1, 4, f);
    for (i = 0; i < 0x300; i++)
        pal[i] <<= 2;
    fwrite(pal, 0x300, 1, f);
}

/*
 * 0x1be2c
 *
 * The `BODY` chunk: 450 rows of 640 pixels, uncompressed. Each row is read a
 * pixel at a time through the driver into the first half of a 0x500-byte
 * buffer, turned into eight bitplanes of 0x50 bytes in the second half, and
 * written.
 */
void iff_write_body(FILE *f)
{
    int16_t x;
    int16_t y;
    int32_t len;
    uint8_t *buf;
    uint8_t *p;

    fwrite((const uint8_t *)IFF_CHUNK_NAMES.body, 4, 1, f);
    len = 0x46500L;
    iff_write_be((uint8_t *)&len, 1, 4, f);
    buf = p = malloc(0x500);
    for (y = 0; y < 0x1c2; y++) {
        p = buf;
        for (x = 0; x < 0x280; x++)
            *p++ = (uint8_t)read_pixel_clipped(x, y);
        chunky_to_planar(buf, buf + 0x280);
        fwrite(buf + 0x280, 0x280, 1, f);
    }
    free(buf);
}

/*
 * 0x1bee1
 *
 * Write the file: `FORM` and its length, `ILBM`, then the `BMHD` - 640 by
 * 450 at 0,0, eight planes, no mask, no compression, transparent colour 0,
 * aspect 1:1, a 640 by 450 page - then the colours and the pixels. Nothing
 * is written if the file cannot be opened.
 */
void iff_save(char *name)
{
    int32_t len;
    int16_t w;
    FILE *f;

    if ((f = fopen(name, IFF_CHUNK_NAMES.mode_wb)) == 0)
        return;
    fwrite((const uint8_t *)IFF_CHUNK_NAMES.form, 4, 1, f);
    len = 0x46830L;
    iff_write_be((uint8_t *)&len, 1, 4, f);
    fwrite((const uint8_t *)IFF_CHUNK_NAMES.ilbm, 4, 1, f);
    fwrite((const uint8_t *)IFF_CHUNK_NAMES.bmhd, 4, 1, f);
    len = 0x14;
    iff_write_be((uint8_t *)&len, 1, 4, f);
    w = 0x280;
    iff_write_be((uint8_t *)&w, 1, 2, f);
    w = 0x1c2;
    iff_write_be((uint8_t *)&w, 1, 2, f);
    len = 0;
    iff_write_be((uint8_t *)&len, 1, 4, f);
    len = 0x08000000L;
    iff_write_be((uint8_t *)&len, 1, 4, f);
    len = 0x101;
    iff_write_be((uint8_t *)&len, 1, 4, f);
    w = 0x280;
    iff_write_be((uint8_t *)&w, 1, 2, f);
    w = 0x1c2;
    iff_write_be((uint8_t *)&w, 1, 2, f);
    iff_write_cmap(f);
    iff_write_body(f);
    fclose(f);
}

/*
 * 0x1c050
 *
 * Save the screen to `name`: read from the page a copy would take as its
 * source, with the clip off so every pixel answers, and put both back after.
 */
void save_screenshot(char *name)
{
    dg_seg_t dst;
    int16_t clip;

    dst = VMDS.page_dst;
    clip = VMDS.clip_enabled;
    VMDS.clip_enabled = 0;
    VMDS.page_dst = VMDS.page_src;
    iff_save(name);
    VMDS.clip_enabled = (uint8_t)clip;
    VMDS.page_dst = dst;
}
