/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Loading the sound driver out of SX.OVL.**
 *
 * The second module of the original's **code segment 2619**, image
 * 0x28580..0x28655 - the second of its modules in C; sound_device.c says how the
 * segment's boundaries are known. Functions are in address order and each
 * carries the image offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * 0x28580
 *
 * Load one numbered sound module. Answers 1 if it is there, 0 if not.
 *
 * The name is built in place: the template `SSM:000:` at DGROUP 0x4a08 with its
 * three digits overwritten from the number at the caller's pointer. Each digit
 * comes from its own division - hundreds, then tens by dividing twice by ten,
 * then units - so the three are worked out independently rather than by one
 * loop.
 *
 * A number of 0xff means there is nothing to load and the answer is 1
 * regardless, which is how the caller's list is terminated.
 *
 * Whatever was loaded before is freed first, as kind 1, and the new block kept
 * at DGROUP 0x4a84. It is then handed to `configure_driver_far` past its first
 * record - `advance_record` steps over the header - and an answer of 0xffff
 * from that is a failure. The block is freed again on the way out either way:
 * this loads a module to configure the driver with, not to keep.
 */
uint16_t load_sound_module(FILE *handle, const uint16_t *number, uint16_t index)
{
    int16_t di = 1;
    int16_t n;

    if (*number == 0xff)
        goto out;

    n = (int16_t)*number;
    CHUNK2.ssm_000[4] = (uint8_t)((n / 100) + 0x30);
    CHUNK2.ssm_000[5] = (uint8_t)(((n / 10) % 10) + 0x30);
    CHUNK2.ssm_000[6] = (uint8_t)((n % 10) + 0x30);

    if (dg_far_ptr(DG4A82.config) != FAR_NULL_PTR)
        free_for_kind(dg_far_ptr(DG4A82.config), 1);

    {
        uint8_t *p = load_named_chunk((char *)handle, CHUNK2.ssm_000, index);

        DG4A82.config = far_of(p);
        if (p == FAR_NULL_PTR)
            di = 0;
    }

out:
    if (di != 0) {
        /* With no module named, `config` is still null here, and the
           original reads the driver's configuration out of the vector table. */
        const uint8_t far *config = NULL_READ(dg_far_ptr(DG4A82.config));

        if (configure_driver_far(advance_record(config)) == 0xffff)
            di = 0;
    }

    if (dg_far_ptr(DG4A82.config) != FAR_NULL_PTR) {
        free_for_kind(dg_far_ptr(DG4A82.config), 1);
        DG4A82.config = FAR_NULL;
    }

    return (uint16_t)di;
}
