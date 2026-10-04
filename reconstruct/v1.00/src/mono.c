/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The mouse's speed, the monochrome debugging screen, a huge `fread` and
 * the scaled bitmap**: code segment 0000 (`_TEXT`), image 0x0b859..0x0ba32.
 * The printer at 0x0b907 reaches 0x0b89d with a bare `push cs / call`, so
 * those two are one module; nothing else here has data or a backward call,
 * so **every other boundary in the range is unknown**, and this file is one
 * for the span rather than a claim that it was one source. BC++ 2.0, like
 * fstring.c and rects.c: BC++ 3.0 cross-jumps `mono_puts`' first
 * `huge_add_to` call into the loop's last, where the image keeps both.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include <stdarg.h>

#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* The monochrome adapter's text screen, B000:0000; `g_mono_screen` on the
   host. Ours, as a spelling. */
#ifdef __TURBOC__
#  define MONO_SCREEN MK_FP(0xb000, 0)
#else
#  define MONO_SCREEN ((void *)g_mono_screen)
#endif

/*
 * 0x0b859
 *
 * Set the mouse's mickeys-per-pixel, INT 33h AX=0x0f, the same value for both
 * axes: the argument goes into CX and DX alike. The start-up asks for 3.
 *
 * Nothing of it is in guest memory, so the port sends it to the IO boundary and
 * there is nothing here for the two artefacts to disagree about.
 */
void mouse_set_speed(uint16_t mickeys)
{
#ifdef __TURBOC__
    _CX = mickeys;
    _DX = _CX;
    _AX = 0x0f;
    geninterrupt(0x33);
#else
    io_mouse_set_speed(mickeys, mickeys);
#endif
}

/*
 * 0x0b868
 *
 * **Clear the monochrome adapter's screen**, 0x780 words at B000:0000 -
 * eighty by twenty-four - each the word 7. A debugging aid: nothing calls it.
 */
void mono_clear(void)
{
    uint16_t huge *p;
    int16_t n;

    p = (uint16_t huge *)MONO_SCREEN;
    for (n = 0x780; n != 0; n--)
        *p++ = 7;
}

/*
 * 0x0b89d
 *
 * **Write a string to the monochrome screen** at column `x`, row `y`, each
 * character with the attribute 0x18. The position is stepped as a huge
 * pointer, through `huge_add_to`. Only `mono_printf` calls it.
 */
void mono_puts(const char *s, int16_t x, int16_t y)
{
    uint8_t huge *p;

    p = MONO_SCREEN;
    x += y * 80;
    p += x * 2;
    while (*s) {
        *p++ = *s++;
        *p++ = 0x18;
    }
}

/*
 * 0x0b907
 *
 * **`printf` to the monochrome screen**: format into 256 bytes of stack and
 * write them at `x`, `y`. Nothing calls it.
 */
void mono_printf(int16_t x, int16_t y, const char *fmt, ...)
{
    va_list ap;
    char buf[256];

    va_start(ap, fmt);
#ifdef __TURBOC__
    vsprintf(buf, fmt, (const uint8_t *)ap);
#else
    /* Ours: the host's `va_list` is not a pointer into the stack, which is
       what `vsprintf` walks, so the host's own formats it - the
       builtin, because <stdio.h> has a `FILE` of its own. */
    __builtin_vsnprintf(buf, sizeof buf, fmt, ap);
#endif
    mono_puts(buf, x, y);
}

/*
 * 0x0b93d
 *
 * `fread` into a **huge** pointer, one byte at a time, answering how many whole
 * items came in.
 *
 * A byte at a time because the destination may cross a segment: each one is
 * stored through the far pointer and then `huge_add_to` steps and renormalises
 * it - reached here by its near door at 0x0be7f.
 *
 * The count is `size * count` as a 32-bit product, and the answer is the bytes
 * actually read divided by the size, which is why a partial last item does not
 * count. The loop stops on the count running out or on `game_fgetc` answering
 * -1, and the test is made **before** the decrement, so a count of zero reads
 * nothing.
 */
uint32_t fread_huge(uint8_t far * dst, uint32_t size, uint32_t count,
                    FILE *file)
{
    uint32_t total;
    uint8_t huge *p;
    uint32_t got;
    int16_t c;

    total = size * count;
    p = dst;
    got = 0;
    while (total-- != 0) {
        c = game_fgetc(file);
        if (c == -1)
            break;
        *p++ = c;
        got++;
    }
    return got / size;
}

/*
 * 0x0b9c9
 *
 * Draw one bitmap scaled - the same choice `draw_bitmap` makes, from the same
 * marker in field 4, and the same normalisation of the header's far pointer
 * written back into it.
 *
 * Two of the four forms have no scaled blitter: 0xfffd, the already-scaled
 * one, and 0xffff, the offset-table one, both simply do nothing here. That is
 * the original's own silence and not a gap - a bitmap in either form is never
 * asked to be drawn scaled.
 */
void draw_bitmap_scaled(struct bitmap *hdr, int16_t x, int16_t y,
                        int16_t w, int16_t h, uint16_t mode)
{
    hdr->data_seg = hdr->data_seg + (hdr->data_off >> 4);
    hdr->data_off &= 0xf;

    switch (hdr->mask_off) {
    case 0xffff:
        break;
    case 0xfffe:
        blit_scaled_a(hdr, x, y, mode, w, h);
        break;
    case 0xfffd:
        break;
    default:
        blit_scaled_b(hdr, x, y, mode, w, h);
        break;
    }
}
