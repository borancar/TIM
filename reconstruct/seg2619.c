/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 2619**, image
 * 0x26190..0x2a040. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x2891a
 *
 * Step a far pointer past one record: the record's length is the byte at
 * offset 1, and there is a two-byte header, so the next record is
 * `off + rec[1] + 2`.
 *
 * The original takes and returns a far pointer in DX:AX and leaves DX - the
 * segment - untouched, so only the offset moves. The port has no segments, so
 * the target and the offset are passed separately: `rec` is what the pointer
 * points at, `off` is the offset half that the arithmetic is done on.
 */
uint16_t advance_record(const uint8_t *rec, uint16_t off)
{
    return (uint16_t)(off + rec[1] + 2);
}
