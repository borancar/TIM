/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Start the sound library: the device, the module and the callback.**
 *
 * A module of the sound library, in 1.11 **code segment 27ff** on its own,
 * image 0x27ffe..0x280b5. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x27ffe
 *
 * Start the sound system. Answers 1 if it came up, 0 if it did not - and 1
 * again, immediately, if either the driver at DGROUP 0x4a94 or the module at
 * 0x4a98 is already loaded, so this cannot run twice.
 *
 * A device of -1 means "no sound": the device becomes 2 and the flag that
 * drives everything after is cleared, so `setup_sound_device` still runs but
 * nothing is installed on the back of it.
 *
 * With sound wanted, three things follow. The timer is taken over at rate 0xd
 * unless something already has it - DGROUP 0x44ee - and 0x4a8c records that.
 * The sequencer's own tick is registered as a callback at rate 4, keeping its
 * slot at 0x4a8e. And a third
 * callback goes to the loaded module's own dispatcher, at 0x0bba6 in segment 0,
 * but only if that module loaded - which it does not here.
 *
 * `alloc_voice_records` is last, and its answer is not looked at.
 */
uint16_t start_sound(int16_t device, int16_t module_index, uint16_t callback,
                     FILE *handle)
{
    int16_t si = 1;

    if (g_sound_bank.driver != NULL || g_sound_bank.module != NULL)
        return 1;

    if (device == -1) {
        device = 2;
        si = 0;
    }

    if (setup_sound_device(device, module_index, callback, handle) != 0) {
        if (si != 0 && !(int8_t)g_timer.installed) {
            timer_install(0xd);
            g_sound_bank.timer_taken = 1;
        }

        if ((si != 0
             && (g_sound_bank.tick_handle = (int16_t)timer_add_callback(sound_service, 4)) != 0)
            || si == 0) {
            if (si != 0 && g_sound_bank.module != NULL)
                g_sound_bank.module_handle =
                    (int16_t)timer_add_callback(SOUND_MODULE_TICK, 2);

            alloc_voice_records();
            return 1;
        }
    }

    return 0;
}
