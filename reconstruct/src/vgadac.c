/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The screenshot writer's assembly**: writing and reading the DAC, and
 * turning a row of pixels into bitplanes. This file corresponds to the
 * forty-fifth and last module of the original's code segment 172c, image
 * 0x1c087..0x1c251, which ends a byte into segment 1c25's first paragraph.
 *
 * **So no C compiler judges this file.** `chunky_to_planar` borrows BP as a
 * data register and keeps its loop count on the stack, which no compiler
 * does. It is the host's transcription of a hand-written module, and the
 * byte-exact source of these bytes is TASM's to make (not written yet). The
 * functions are in address order and each carries the image offset it was
 * read from, as everywhere else.
 *
 * Only screenshot.c calls into it, and nothing calls `vga_set_dac` at all.
 * The names are ours.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x1c087
 *
 * Load `count` colours into the DAC from index `first`. It is the driver's
 * `vm_set_palette` instruction for instruction: wait for retrace, write the
 * masked DAC state back to the data port unless it reads 3, set the index,
 * and send `count * 3` bytes with interrupts off.
 */
void vga_set_dac(const uint8_t *rgb, int16_t first, int16_t count)
{
    uint16_t bytes = (uint16_t)(count * 3);
    uint8_t state;

    while (!(io_in8(PORT_INPUT_ST1) & 0x08))
        ;
    state = (uint8_t)(io_in8(PORT_DAC_READ) & 3);
    if (state != 3)
        io_out8(PORT_DAC_DATA, state);
    io_out8(PORT_DAC_WRITE, (uint8_t)first);
    do {
        io_out8(PORT_DAC_DATA, *rgb++);
    } while (--bytes);
}

/*
 * 0x1c0c2
 *
 * Read `count` colours out of the DAC from index `first`. The mirror of the
 * one above, with the state test turned round: unless the state reads 0 it
 * throws one byte of the data port away first. The index goes to the read
 * port either way - the `mov dx,0x3c7` after the dummy read only puts back
 * the port the state was read from.
 */
void vga_get_dac(uint8_t *rgb, int16_t first, int16_t count)
{
    uint16_t bytes = (uint16_t)(count * 3);

    while (!(io_in8(PORT_INPUT_ST1) & 0x08))
        ;
    if ((io_in8(PORT_DAC_READ) & 3) != 0)
        (void)io_in8(PORT_DAC_DATA);
    io_out8(PORT_DAC_READ, (uint8_t)first);
    do {
        *rgb++ = io_in8(PORT_DAC_DATA);
    } while (--bytes);
}

/*
 * 0x1c0ff
 *
 * Turn 640 pixels of one byte each into the eight bitplanes of an ILBM row,
 * 0x50 bytes apiece, plane 0 first. Eight pixels at a time: each is shifted
 * out a bit at a time, low bit first, and bit `k` is rotated into the byte
 * for plane `k`, so the first pixel ends in the top bit. The original holds
 * the eight plane bytes in AH, AL, BH, BL, CH, CL, DH and DL, in that order.
 */
void chunky_to_planar(const uint8_t *src, uint8_t *dst)
{
    uint8_t plane[8] = { 0 };           /* shifted out whole; the registers'
                                           old values never reach a plane */
    int16_t n;
    int16_t j;
    int16_t k;

    for (n = 0x50; n != 0; n--) {
        for (j = 0; j < 8; j++)
            for (k = 0; k < 8; k++)
                plane[k] = (uint8_t)((plane[k] << 1) | ((src[j] >> k) & 1));
        src += 8;
        for (k = 0; k < 8; k++)
            dst[k * 0x50] = plane[k];
        dst++;
    }
}
