/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Choose and start the sound device.**
 *
 * A module of the sound library, in 1.11 **code segment 2b2d** on its own,
 * image 0x2b2d0..0x2b418. 1.00 linked the library's C into one segment,
 * 2619, where its module boundaries had to be inferred; 1.11 gives each
 * module a segment, and the far calls into them say where each begins.
 *
 * JUDGE: built-with -mm -O2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The sound device's chunk name**, DGROUP 0x4a12..0x4a1c. **Not a
 * constant: a buffer.** The image holds "SSM:" and *five* spaces, which is
 * nine characters and would fail the multiple-of-four check.
 * `setup_sound_device` writes a four-character tag **and its NUL** over the
 * spaces at +4 first, so the path is eight when it is walked and the fifth
 * space is the room that NUL needs.
 */
char g_sound_chunk_name[] = "SSM:     ";

/*
 * **The device tags**, DGROUP 0x4a1c..0x4a2e, indexed by the device byte of
 * RESOURCE.CFG. The tags themselves are the module's literal pool, from
 * 0x4a38 to 0x4a7e, after the two tables.
 */
char *g_sound_device_tags[9] = {
    "STD:", "TAN:", "ADL:", "M32:", "SBP:", "PS1:", "PRO:", "GMD:", "NLD:",
};

/*
 * **The sound module tags**, DGROUP 0x4a2e..0x4a38, indexed by the module
 * byte of RESOURCE.CFG.
 */
char *g_sound_module_tags[5] = {
    "ASB:", "APS:", "ATD:", "APA:", "ADS:",
};

/*
 * 0x2b2d0
 *
 * Set up the sound device: load its **module** and then its **driver**, and
 * answer 0 if both worked and 1 if either did not.
 *
 * Two names are built the same way - `strcpy_far` puts one of the strings
 * named by the tables at DGROUP 0x4a2e and 0x4a1c into the buffer at 0x4a16,
 * which the template at 0x4a12 is the head of - and `load_named_chunk` reads
 * the chunk of that name.
 *
 * The module goes to DGROUP 0x4a98 and becomes **loaded code**: 0x4aaa marks it
 * present and `set_sound_callback` points the module's own dispatcher at it,
 * after which calls through it are calls into a block that is not part of this
 * binary at all - for `ASB:` the port has that block, in
 * reconstruct/src/sxovl_asb.c, and `call_sound_module` reaches it.
 *
 * **A module does not replace the driver.** Both halves run: the module is
 * loaded and installed, and then the device's driver is loaded too. So a
 * digitised module and a music device are a pair rather than alternatives, and
 * nothing in the game ties a particular module to a particular device - the two
 * bytes of RESOURCE.CFG are independent indices into the tables at 0x4a2e and
 * 0x4a1c.
 *
 * The driver goes to 0x4a94 and is installed with `install_driver_far`, whose
 * answer is kept at 0x4a82 as the number `load_sound_module` then looks up.
 *
 * A device of 8 is recorded as 3 at DGROUP 0x4aae, which is the number
 * `load_sound_bank` later switches on. An argument of -2 skips a load
 * entirely, and a failed load rewrites the argument to -2 so the second half
 * skips too.
 *
 * The answer is the sense of the failure flag turned round by
 * `neg`/`sbb`/`inc` - a compiler writing `!di` without a branch.
 */
uint16_t setup_sound_device(int16_t device, int16_t module_index,
                            uint16_t callback, FILE *handle)
{
    int16_t failed = 0;

    if (module_index != -2) {
        strcpy_far(g_sound_chunk_name + 4,
                        g_sound_module_tags[module_index]);

        if ((g_sound_bank.module = load_named_chunk((char *)handle, g_sound_chunk_name, 0))
            != NULL) {
            g_sound_bank.module_live = 1;
            set_sound_callback(g_sound_bank.module);

            /*
             * **And then on to the driver, whatever this answers.** A
             * non-zero answer goes straight to the driver half keeping the
             * module, and a zero one takes the module down again - 0x4aaa
             * cleared, 0x0bbc6 told to stop, `free_for_kind`, the pointer
             * zeroed - and *then* goes to the driver half. Either way the
             * driver is loaded: a module and a device are a pair, which is
             * why the two bytes of RESOURCE.CFG are independent indices into
             * two tables.
             */
            if (sound_module_install(callback, 1) == 0) {
                g_sound_bank.module_live = 0;
                stop_loaded_module();
                free_for_kind(g_sound_bank.module, 1);
                g_sound_bank.module = 0;
                module_index = -2;
                failed = 1;
            }
        } else {
            module_index = -2;
            failed = 1;
        }
    }

    if (device != -2) {
        strcpy_far(g_sound_chunk_name + 4,
                        g_sound_device_tags[device]);

        if ((g_sound_bank.driver = load_named_chunk((char *)handle, g_sound_chunk_name, 0))
            != NULL) {
            g_sound_bank.driver_number = (uint8_t)install_driver_far(g_sound_bank.driver);

            if (load_sound_module(handle, &g_sound_bank.driver_number, 0) == 0) {
                free_for_kind(g_sound_bank.driver, 1);
                g_sound_bank.driver = 0;
                failed = 1;
            }
        } else {
            failed = 1;
        }

        device = device == 8 ? 3 : device;
    }

    g_sound_bank.device = device;
    return !failed;
}
