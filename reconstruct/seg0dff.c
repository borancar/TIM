/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 0dff**, image
 * 0x0dff0..0x14de0. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x0e34a
 *
 * NOT TRANSCRIBED YET. Called from the frame-presentation routine at 0x081cc
 * when DGROUP 0x52fa is set. It is a large routine - it reserves 0x122 bytes
 * of locals - and reading it is a job of its own.
 */
void sub_0e34a(uint16_t arg)
{
    (void)arg;
    not_transcribed("0x0e34a");
}
/*
 * 0x11d44
 *
 * Look a word up in the table that the **far** pointer at DGROUP 0x546c points
 * at, or answer 0 for the index -1. The table is outside DGROUP - it is in a
 * block DOS handed the program - which is why the port models the guest's
 * whole address space rather than only its data segment.
 */
int16_t lookup_table_546c(int16_t index)
{
    if (index == -1)
        return 0;
    return FAR16(DG_FAR_SEG(0x546C),
                 (uint16_t)(DG_FAR_OFF(0x546C) + (uint16_t)(index * 2)));
}


/*
 * 0x11dd1
 *
 * A far-callable two-byte read: `game_fread(buf, 2, 1, file)`, with the
 * arguments the other way round from `fread`'s own - the file first and the
 * buffer second.
 */
void game_fread_far(uint16_t file, uint16_t buf)
{
    game_fread(buf, 2, 1, file);
}

/*
 * 0x12ba7
 *
 * Read `TIM.CFG`: two words, into DGROUP 0x4eb7 and 0x4ec1. Answers 1 if the
 * file was there and 0 if it was not.
 *
 * The name is the string at DGROUP 0x28bb and the mode the one at 0x28c3. Both
 * reads go through `game_fread_far`, which takes its file first and buffer
 * second, and the file is closed on the success path only - a failed open has
 * nothing to close.
 */
uint16_t read_tim_cfg(void)
{
    uint16_t file = game_fopen(0x28bb, 0x28c3);

    if (file == 0)
        return 0;

    game_fread_far(file, 0x4eb7);
    game_fread_far(file, 0x4ec1);
    game_fclose(file);

    return 1;
}
