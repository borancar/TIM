/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Stop the sound, and the five-tick wait it takes to.**
 *
 * A module of the sound library, in 1.11 **code segment 2b60** on its own,
 * image 0x2b608..0x2b6cd. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x2b608
 *
 * Shut the sound down: silence the driver, let whatever is playing finish, and
 * give both blocks back.
 *
 * How it waits depends on whether the sequencer's timer callback is
 * registered - DGROUP 0x4a8e. With it registered the tick is running and
 * `delay_five_ticks` is enough; without it nothing is driving the sequencer, so
 * `sound_service` is called twice by hand instead.
 *
 * `silence_driver_far` is called with **no arguments at all**, which is safe
 * only because it reads none - the same dead argument 0x2846a has.
 *
 * The loaded module is told to stop through its own dispatcher at 0x0bbc6, a
 * call into a block that is not part of this binary. Not reached here, and left
 * as a stub.
 */
void stop_sound(void)
{
    if (g_sound_bank.driver != NULL) {
        silence_driver_far();

        if (((int16_t)g_sound_bank.tick_handle) == 0) {
            sound_service();
            sound_service();
        } else {
            delay_five_ticks();
        }
    }

    if (g_sound_bank.module != NULL) {
        stop_loaded_module();
    }

    if (g_sound_bank.driver != NULL) {
        free_for_kind(g_sound_bank.driver, 1);
        g_sound_bank.driver = 0;
    }

    if (g_sound_bank.module != NULL) {
        free_for_kind(g_sound_bank.module, 1);
        g_sound_bank.module = 0;
    }
}

/*
 * 0x2b68f
 *
 * Wait five timer ticks. A counter at DGROUP 0x6430 is set to five, a callback
 * registered at four ticks a time, and the routine **spins** until the callback
 * has counted it down; then the slot is given back.
 *
 * The far pointer it registers is this module's own `cs:0x3228`, which is
 * `tick_delay` below.
 *
 * The spin only ends because the timer interrupt runs the callback, so in the
 * port it ends only when something drives the timer - the same standing as
 * `wait_and_latch_frame`. Nothing reaches it on these screens.
 */
void delay_five_ticks(void)
{
    uint16_t handle;

    g_sound_ticks_left = 5;

    handle = timer_add_callback(tick_delay, 4);

    while (g_sound_ticks_left > 0)
        ;

    timer_drop_callback(handle);
}

/*
 * 0x2b6c8
 *
 * The callback `delay_five_ticks` registers: one instruction of work, counting
 * DGROUP 0x6430 down by one each tick.
 */
void tick_delay(void)
{
    g_sound_ticks_left--;
}
