/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Choosing and starting the sound device, and the voice records.**
 *
 * The third module of the original's **code segment 2619**, image
 * 0x28655..0x28935 - the second of its modules in C; sound_device.c says how the
 * segment's boundaries are known. Functions are in address order and each
 * carries the image offset it was read from.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -G -Z
 * JUDGE: data 0x4a12..0x4a7e
 *
 * **How segment 2619's boundaries are known.** The segment was one file,
 * sound.c, until 2026-09-27. It is seven modules: two hand written and five
 * in C.
 *
 * - **The assembly** is told apart by its code. The first module,
 *   0x26198..0x28580, keeps its state in its own code segment and takes its
 *   arguments in registers, and only its far entry points build a C frame.
 *   Its end is the first compiler's prologue after them, `load_sound_module`.
 *   The fifth, 0x2928c..0x292f4 (sound_call.c), saves AX round stores, or
 *   every register and the flags, and keeps its answer in the code segment.
 * - **Between the C modules the calls decide**, as in segment 172c
 *   (parts/ball.c). A backward call with TLINK's `nop / push cs / call`
 *   proves a boundary between callee and caller, and one without it rules a
 *   boundary out. Those calls force cuts at 0x28655 (`load_sound_module` is
 *   called from `setup_sound_device`) and 0x28935 (`advance_record` from
 *   `create_sequence`), and one in 0x294ff..0x296b4. The files take its
 *   latest place, 0x296b4, which gives the fewest cuts that satisfy every
 *   call.
 * - **The literal pool agrees.** DGROUP 0x4a08.. holds `load_sound_module`'s
 *   template, then this module's name buffer, tag tables and tags, then the
 *   two `"r"`s of `load_sound_bank` and `load_resource_block` - one module,
 *   not built with `-d`.
 *
 * Two C files with no backward call across the line between them would look
 * like one. The judge would say so only if the data or the code disagreed.
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

/* The seven voices, DGROUP 0x6414; the record is described in dgroup.h. */
struct sequence far *g_sound_voice[7];

/*
 * 0x28655
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

/*
 * 0x287ad
 *
 * Which of the seven voices is playing a given sequence. The argument is the
 * sequence's far pointer; the answer is the voice's record, also as a far
 * pointer in DX:AX, or null.
 *
 * The seven voices are a table of far pointers at DGROUP 0x6414, four bytes
 * apart. A voice matches when the pointer it keeps at its own +0x166 equals the
 * one asked for **and** the byte at +0x158 is not 0xff - the second test is
 * what excludes a voice that still remembers a sequence it has stopped
 * playing.
 *
 * The original re-loads the table entry twice more after the `les`, and keeps
 * ES from the first load while doing so. That is only a compiler making the
 * same address three times, not three different pointers.
 */
struct sequence far *voice_playing(const uint8_t far * source)
{
    int16_t i;

    for (i = 0; i < 7; i++) {
        if (g_sound_voice[i]->source == source
            && g_sound_voice[i]->state != 0xff)
            return g_sound_voice[i];
    }

    return NULL;
}

/*
 * 0x28800
 *
 * Allocate the seven voice records - 0x17a bytes each, kind 2 - and put them in
 * the table at DGROUP 0x6414.
 *
 * A table that is **already** filled is refused with 0, not accepted as work
 * already done: the test is on the first entry only, and the answer is the
 * failure code. So this is called once.
 *
 * Each record is marked free with 0xff at +0x158 and given a far pointer at +8
 * to its own +0x16a. If any allocation fails the whole table is handed back
 * through `free_voice_records` - including the entries this loop has not
 * reached, which are whatever they were before.
 */
uint16_t alloc_voice_records(void)
{
    int16_t i;
    struct sequence far *voice;

    if (g_sound_voice[0] == NULL) {
        for (i = 0; i < 7; i++) {
            if ((g_sound_voice[i] = (struct sequence far *)
                     alloc_for_kind(sizeof(struct sequence), 2))
                == NULL) {
                free_voice_records();
                return 0;
            }

            /* `cursor_at` is where the record's own `cursor` is. */
            voice = g_sound_voice[i];
            voice->state = 0xff;
            voice->cursor_at = &voice->cursor;
        }
        return 1;
    }

    return 0;
}

/*
 * 0x28886
 *
 * Load a named chunk out of a file, and answer it as a far pointer or null.
 *
 * The file may arrive as a handle or as a name: `file_record_valid` decides
 * which, and a name is opened here - the flag in the first local remembering
 * that this routine owns it and has to close it again.
 *
 * `seek_named_chunk` positions the file at the chunk and is refused on -1:-1.
 * `file_record_size` then answers the size, and it is handed straight to
 * `load_resource_block` as the size to open with - and the two values pushed
 * before it, a kind of 1 and a null out-pointer, are left on the stack across
 * that call, which is why only two bytes are cleaned after the size.
 *
 * A file this routine opened is closed on every path, including the ones that
 * give up; one it was handed is left alone.
 */
uint8_t far *load_named_chunk(char *name, const char * path,
                              uint16_t index)
{
    int16_t opened = 0;
    uint8_t far *r = NULL;
    FILE *si;

    /* A handle, or a name to open. */
    if (file_record_valid((FILE *)name) == 0) {
        opened = 1;
        si = open_file_record(name);
    } else {
        si = (FILE *)name;
    }

    if (si != 0 && seek_named_chunk(si, path, (int16_t)index) != -1L)
        r = load_resource_block(si, file_record_size(si), NULL, 1);

    if (opened != 0)
        close_file_record(si);

    return r;
}

/*
 * 0x2891a
 *
 * Step a far pointer past one record: the record's length is the byte at
 * offset 1, and there is a two-byte header, so the next record is
 * `off + rec[1] + 2`.
 *
 * The original takes and returns a far pointer in DX:AX and leaves DX - the
 * segment - untouched, so only the offset moves. The port answers the pointer,
 * and a caller that files the result files the segment it already holds with
 * the difference added to its offset.
 */
const uint8_t far *advance_record(const uint8_t far * rec)
{
    return rec + rec[1] + 2;
}
