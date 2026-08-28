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
 * 0x27a86
 *
 * Flush up to two pending volume changes to the driver, round-robin over the
 * sixteen channels, and remember where to resume.
 *
 * The array at the module's own `cs:0x1c8` holds one byte per channel, with
 * 0xff meaning nothing pending. A channel with anything else is marked 0xff
 * again and its value sent to the driver as MIDI controller 7 - volume - on
 * that channel.
 *
 * **At most two per call.** The scan then stops wherever it is, and `cs:0x206`
 * carries that position into the next call, so sixteen channels are serviced
 * over eight calls rather than all at once. This runs from the timer tick, and
 * sending sixteen controller changes inside one interrupt would be the thing
 * it is avoiding.
 *
 * The scan is also bounded by returning to where it started, so a pass with
 * nothing pending walks the ring once and stops rather than spinning.
 *
 * The original reaches the driver by a far call through `cs:[0x1e7]` with the
 * function number in BP; 7 selects `sx_controller`, which is what the port
 * calls directly. The number is fixed for this driver, not looked up.
 *
 * This is hand-written assembly - no frame, no arguments, a near `ret`.
 */
void flush_pending_volumes(void)
{
    uint16_t si = SND8(0x206);
    int16_t sent = 0;

    for (;;) {
        uint8_t pending = SND8(0x1c8 + si);

        if (pending != 0xff) {
            SND8(0x1c8 + si) = 0xff;
            sx_controller(si, (uint16_t)((7 << 8) | pending));
            sent++;
            if (sent == 2)
                break;
        }

        si++;
        if (si == 0x10)
            si = 0;
        if (si == SND8(0x206))
            break;
    }

    SND8(0x206) = (uint8_t)si;
}

/*
 * 0x282cb
 *
 * Scale one byte by another and halve the range: `((cl+1) * (dl+1)) >> 8`,
 * doubled, then reduced by one unless it is already zero.
 *
 * A **near** routine that takes and answers CL, preserving AX around the
 * multiply with a push and a pop. `mul dl` is the 8-bit form, so the product
 * lands in AX and `shl ah,1` doubles its high byte - the >>8 and the doubling
 * are one step, not two.
 */
uint8_t scale_byte_pair(uint8_t cl, uint8_t dl)
{
    uint16_t product = (uint16_t)((uint8_t)(cl + 1) * (uint8_t)(dl + 1));
    uint8_t out = (uint8_t)(((product >> 8) & 0xFF) << 1);

    if (out != 0)
        out--;
    return out;
}
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

/*
 * 0x2907b
 *
 * Follow a chain of **far** pointers - offset at +0x172, segment at +0x174 -
 * for at most `count` links, stopping early on a null pointer.
 *
 * The original walks by overwriting its own stack arguments, and tests the
 * pointer for null by OR-ing the two halves together, which is how a far
 * pointer is compared with zero without two compares. It answers the pointer
 * it stopped on, in DX:AX.
 */
uint32_t follow_far_chain(uint16_t off, uint16_t seg, int16_t count)
{
    for (;;) {
        if ((uint16_t)(off | seg) == 0)
            break;
        if (count == 0)
            break;
        {
            uint16_t next_seg = FARU16(seg, off + 0x174);
            uint16_t next_off = FARU16(seg, off + 0x172);
            seg = next_seg;
            off = next_off;
        }
        count--;
    }
    return ((uint32_t)seg << 16) | off;
}

