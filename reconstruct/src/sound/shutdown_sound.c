/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Stop the sound library and give back what it holds.**
 *
 * A module of the sound library, in 1.11 **code segment 2b1f** on its own,
 * image 0x2b1f8..0x2b29e. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: compiler bc3.10
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2b1f8
 *
 * Take the whole sound system down, in the reverse order `start_sound` built
 * it up. Does nothing at all if neither the driver nor the module is loaded.
 *
 * Everything is released and its slot zeroed as it goes: the records and their
 * payloads, the directory at DGROUP 0x4aa2, the file at 0x4aa6 if this module
 * opened it, the two timer callbacks at 0x4a8e and 0x4a90, and the timer itself
 * if 0x4a8c says it was taken. Then the voice records, and `stop_sound` last.
 *
 * `free_voice_records` answers whether it found a table to free, and that
 * answer is ignored - so a system that never allocated one comes down just as
 * quietly.
 */
void shutdown_sound(void)
{
    if (g_sound_bank.driver == NULL
        && g_sound_bank.module == NULL)
        return;

    remove_and_free_records(0);

    if (g_sound_bank.directory != 0)
        free_for_kind((uint8_t far *)g_sound_bank.directory, 0xa);

    if (g_sound_bank.file != 0 && g_sound_bank.file_kind != 0)
        close_file_record(g_sound_bank.file);

    if (((int16_t)g_sound_bank.tick_handle) != 0) {
        timer_drop_callback(g_sound_bank.tick_handle);
        g_sound_bank.tick_handle = 0;
    }

    if (((int16_t)g_sound_bank.module_handle) != 0) {
        timer_drop_callback(g_sound_bank.module_handle);
        g_sound_bank.module_handle = 0;
    }

    if (((int16_t)g_sound_bank.timer_taken) != 0) {
        timer_remove();
        g_sound_bank.timer_taken = 0;
    }

    free_voice_records();
    stop_sound();
}
