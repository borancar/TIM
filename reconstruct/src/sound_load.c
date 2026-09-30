/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Loading the sound driver out of SX.OVL.**
 *
 * The second module of the original's **code segment 2619**, image
 * 0x28580..0x28655 - the first of its modules in C; sound_device.c says how the
 * segment's boundaries are known. Functions are in address order and each
 * carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -G -Z
 * JUDGE: data 0x4a08..0x4a11
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The sound module's name template**, DGROUP 0x4a08..0x4a11: this module's
 * `_DATA`. Not only a constant: `load_sound_module` builds the name in
 * place, writing the three digits at +4, +5 and +6 over "000".
 */
char g_sound_module_name[] = "SSM:000:";

/*
 * 0x28580 (1.00's; not yet placed in 1.11)
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
uint16_t load_sound_module(FILE *handle, const int16_t *number, uint16_t index)
{
    int16_t ok = 1;

    if (*number != 0xff) {
        char *name = g_sound_module_name;

        g_sound_module_name[4] = (char)(*number / 100 + '0');
        g_sound_module_name[5] = (char)(*number / 10 % 10 + '0');
        g_sound_module_name[6] = (char)(*number % 10 + '0');

        if (g_sound_bank.config != NULL)
            free_for_kind(g_sound_bank.config, 1);

        if ((g_sound_bank.config = load_named_chunk((char *)handle, name, index))
            == NULL)
            ok = 0;
    }

    /* With no module named, `config` is still null here, and the original
       reads the driver's configuration out of the vector table. */
    if (ok != 0
        && configure_driver_far(advance_record(ZERO_PAGE(g_sound_bank.config)))
           == 0xffff)
        ok = 0;

    if (g_sound_bank.config != NULL) {
        free_for_kind(g_sound_bank.config, 1);
        g_sound_bank.config = 0;
    }

    return (uint16_t)ok;
}
