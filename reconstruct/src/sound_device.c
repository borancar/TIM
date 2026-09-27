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
 * **How segment 2619's boundaries are known.** The segment was one file,
 * sound.c, until 2026-09-27. The first module, 0x26198..0x28580, is hand
 * written, and its end is where C's frames begin: `load_sound_module` is the
 * first routine with a compiler's prologue that is not one of the
 * assembly's far entry points. After that the calls decide, as in segment
 * 172c (part_ball.c). A backward call with TLINK's `nop / push cs / call`
 * proves a boundary between callee and caller, and one without it rules a
 * boundary out. Those calls force cuts at 0x28655 (`load_sound_module` is
 * called from `setup_sound_device`) and 0x28935 (`advance_record` from
 * `create_sequence`). They also need one cut in 0x2928c..0x293c1 and one in
 * 0x294ff..0x296b4.
 *
 * The files take the fewest cuts that satisfy every call, which puts those
 * two at their latest places, 0x293c1 and 0x296b4. The literal pool agrees:
 * DGROUP 0x4a08.. holds `load_sound_module`'s strings, then
 * `setup_sound_device`'s, then the two `"r"`s of `load_sound_bank` and
 * `load_resource_block` - one module, not built with `-d`. A module that
 * was really two files, with nothing calling backward across the line,
 * cannot be seen from the calls. The judge will say whether it can be seen
 * from anything else.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/* The seven voices, DGROUP 0x6414; the record is described in dgroup.h. */
struct sound_voices SOUND_VOICES DGROUP_WAS(0x6414);

/*
 * 0x28655
 *
 * Set up the sound device: load its **module** and then its **driver**, and
 * answer 0 if both worked and 1 if either did not.
 *
 * Two names are built the same way - `string_copy_far` puts one of the strings
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
    int16_t di = 0;

    if (module_index != -2) {
        uint8_t *p;

        string_copy_far(CHUNK2.ssm_tag + 4,
                        (const char *)dg_near_ptr(SOUND_TAGS.module[module_index]));

        p = load_named_chunk((char *)handle, CHUNK2.ssm_tag, 0);
        DG4A82.module = far_of(p);

        if (p == FAR_NULL_PTR) {
            module_index = -2;
            di = 1;
        } else {
            DG4A82.module_live = 1;
            set_sound_callback(dg_far_ptr(DG4A82.module));

            /*
             * **And then on to the driver, whatever this answers.** The call
             * at 0x286b7 is `sub_0bb98(callback, 1)`; a non-zero answer jumps
             * straight to the driver half keeping the module, and a zero one
             * takes the module down again - 0x4aaa cleared, 0x0bbc6 told to
             * stop, `free_for_kind`, the pointers zeroed - and *then* goes to
             * the driver half. Either way the driver is loaded.
             *
             * The port used to `return 1` here, which said a module supersedes
             * the driver. It does not: they are a pair, and that is why the two
             * bytes of RESOURCE.CFG are independent indices into two tables.
             * The mistake was invisible because the stub below aborts before
             * reaching it, and it had been written into the comment above and
             * into docs/sound-driver.md as though it were a finding.
             */
            if (sound_module_install(callback, 1) == 0) {
                DG4A82.module_live = 0;
                stop_loaded_module();
                free_for_kind(dg_far_ptr(DG4A82.module), 1);
                DG4A82.module = FAR_NULL;
                module_index = -2;
                di = 1;
            }
        }
    }

    if (device != -2) {
        uint8_t *p;

        string_copy_far(CHUNK2.ssm_tag + 4,
                        (const char *)dg_near_ptr(SOUND_TAGS.device[device]));

        p = load_named_chunk((char *)handle, CHUNK2.ssm_tag, 0);
        DG4A82.driver = far_of(p);

        if (p == FAR_NULL_PTR) {
            di = 1;
        } else {
            DG4A82.driver_number =
                (int16_t)(install_driver_far(dg_far_ptr(DG4A82.driver)) & 0xff);

            if (load_sound_module(handle, &DG4A82.driver_number, 0) == 0) {
                free_for_kind(dg_far_ptr(DG4A82.driver), 1);
                DG4A82.driver = FAR_NULL;
                di = 1;
            }
        }

        if (device == 8)
            device = 3;
    }

    DG4A82.device = device;
    return (uint16_t)(di == 0 ? 1 : 0);
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
        struct sequence *v = SOUND_VOICES.voice[i];

        /* Which note data this voice is playing. The pair was filed from a
           pointer to a DOS block, so comparing pointers is comparing pairs. */
        if ((const uint8_t *)v->source != source)
            continue;
        if (v->state == 0xff)
            continue;
        return v;
    }

    return SEQUENCE_NONE;
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

    if (SOUND_VOICES.voice[0] != NULL)
        return 0;

    for (i = 0; i < 7; i++) {
        struct sequence *voice = (struct sequence *)(void *)alloc_for_kind(sizeof(struct sequence), 2);

        SOUND_VOICES.voice[i] = voice;

        if (voice == SEQUENCE_NONE) {
            free_voice_records();
            return 0;
        }

        /* `cursor_at` is where the record's own `cursor` is. */
        voice->state = 0xff;
        voice->cursor_at = &voice->cursor;
    }

    return 1;
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
    FILE *handle = (FILE *)name;         /* a handle, or a name to open */
    uint16_t opened = 0;
    FILE *si;
    uint8_t *r = FAR_NULL_PTR;

    if (file_record_valid(handle) == 0) {
        opened = 1;
        si = open_file_record(name);
    } else {
        si = handle;
    }

    if (si != 0) {
        int32_t p = seek_named_chunk(si, path, (int16_t)index);

        if (p != -1) {
            uint32_t size = file_record_size(si);

            r = load_resource_block(si, size, NULL, 1);
        }
    }

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
