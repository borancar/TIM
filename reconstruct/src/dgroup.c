/*
 * Storage for the original's DGROUP and for the span-list buffer. See
 * dgroup.h for why DGROUP is an array rather than a set of named globals.
 */
#include "dgroup.h"

/* `guest_mem` itself is the linker script's - see DGROUP_AT in dgroup.h. */

/*
 * Where DGROUP sits in that megabyte. The original's loader decides it - 0x110
 * paragraphs for the program, so DGROUP lands at 0x2e4c0 - and tools/verify.py
 * sets it from the run it captured, so the segment values held in the game's
 * own data mean the same thing on both sides.
 */
uint32_t dgroup_base = 0x2E4C0;

/*
 * The port's own stack pointer - see dgroup.h. The default is the top of
 * DGROUP; tools/verify.py replaces it with the original's SP for each call it
 * compares, which is where the frames of the routine under test then land.
 * Nothing depends on that: the stack is not part of what the two artefacts
 * compare.
 */
uint16_t guest_sp = 0xFF9E;

/*
 * NOT a transcription: the port's own frame reservation. The original has no
 * such routine - it says `push bp / mov bp,sp / sub sp,0x40` - so there is no
 * address to point at. See dgroup.h for why the port needs a stack at all.
 *
 * **All this exists for is a near-addressable scratch area.** A handful of
 * routines build something and hand its *DGROUP offset* to another routine,
 * and a C local has no such offset; these bytes do. That is the whole
 * requirement - somewhere inside the guest's memory, not in use by anything
 * else, for the life of the call. It is a convenience stack for those
 * routines and nothing more.
 *
 * It reserves **two bytes more** than asked for, because the saved BP sits
 * between the caller's SP and the locals: with the frame `sub sp,N` builds,
 * the local at `bp-N` is at entry-SP minus N minus 2.
 *
 * That looks like it is reproducing the original's stack layout for its own
 * sake, and the comment here used to say as much - which made it look
 * removable, since nothing compares the stack. **Removing it was tried on
 * 2026-09-10 and measured.** `read_sound_records` and `seek_to_sound_record`
 * went from verified to DIFFERS over 20 calls each, by one byte: DGROUP 0x5894
 * read 04 where the original had 02. That word is the decompression cursor,
 * and `read_resource` files the frame's address into it - so those two frames
 * are exactly the ones whose *address escapes into compared memory*, which is
 * the same reason they cannot be C arrays. Where such a frame sits is not free
 * after all, and the two bytes are what put it where the original's was.
 *
 * Pass the **whole** frame below BP - the locals `sub sp` reserves and any
 * registers the prologue pushes after it - not just the locals. A routine whose
 * own locals nothing looks at still has to reserve, if it calls one whose
 * locals are looked at: otherwise the callee's frame lands inside the caller's
 * and every address it hands out is wrong. That much is still required, and it
 * is about not overlapping rather than about matching an address.
 */
uint16_t dg_alloca(uint16_t bytes)
{
    guest_sp = (uint16_t)(guest_sp - bytes - 2);
    return guest_sp;
}

/*
 * NOT a transcription either, and the counterpart of the `mov sp,bp / pop bp`
 * the original's epilogue does. It gives back what dg_alloca took, the saved
 * BP's two bytes included.
 */
void dg_free(uint16_t bytes)
{
    guest_sp = (uint16_t)(guest_sp + bytes + 2);
}

/*
 * DGROUP 0x0ea6 - the part kinds, 58 records of 0x3a bytes, as the image holds
 * them. See `PART_KINDS` in dgroup.h for how the game reaches a record.
 *
 * **The hooks are far pointers the loader relocates** - segment 0000 for the
 * do-nothing routines at 0x0297..0x02b5, segment 172c for the rest - and they
 * are exactly the relocation table's entries in this range, six a record and
 * nothing else. So every `seg` is `LOAD_SEG +` the image's word, and no other
 * field is. The routine each names is the port's transcription at that address.
 *
 * `bitmaps_ptr` is 0 for every kind here: `load_part_bitmap` fills it at run
 * time. The field names and the kinds' names are ours - see `struct part_kind`.
 * The `word_*` fields are words nothing has been read for yet, transcribed as
 * they are.
 */
struct part_kind PART_KINDS[PART_KIND_COUNT] DGROUP_AT(0x0ea6) = {
    [KIND_BOWLING_BALL] = {    /* DGROUP 0x0ea6 */
        .word_00 = 0x0b10, .weight = 0x00c8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .word_20 = 0x0000,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x0001, LOAD_SEG + 0x172c },    /* part_setup_0001 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BRICK_PLATFORM] = {    /* DGROUP 0x0ee0 */
        .word_00 = 0x1039, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0018,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0010, .min_h = 0x0010,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .word_20 = 0x0028,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x48ab, LOAD_SEG + 0x172c },    /* part_setup_48ab */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x48f7, LOAD_SEG + 0x172c },    /* part_settle_48f7 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_RAMP] = {    /* DGROUP 0x0f1a */
        .word_00 = 0x05e6, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0040, .max_h = 0x0000, .min_w = 0x0010, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .word_20 = 0x002c,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x2728, LOAD_SEG + 0x172c },    /* part_setup_ramp */
        .flip   = { 0x27b6, LOAD_SEG + 0x172c },    /* part_flip_ramp */
        .settle = { 0x2789, LOAD_SEG + 0x172c },    /* part_settle_ramp */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_SEESAW] = {    /* DGROUP 0x0f54 */
        .word_00 = 0x0760, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e42, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0008, .word_20 = 0x0006,
        .hit    = { 0x3fe8, LOAD_SEG + 0x172c },    /* part_hit_seesaw */
        .step   = { 0x420f, LOAD_SEG + 0x172c },    /* part_step_seesaw */
        .setup  = { 0x40f0, LOAD_SEG + 0x172c },    /* part_setup_seesaw */
        .flip   = { 0x41bb, LOAD_SEG + 0x172c },    /* part_flip_seesaw */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x44fe, LOAD_SEG + 0x172c },    /* part_drive_44fe */
    },
    [KIND_BALLOON] = {    /* DGROUP 0x0f8e */
        .word_00 = 0x0009, .weight = 0x0001, .word_04 = 0x0040, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e48, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .word_20 = 0x0005,
        .hit    = { 0x016e, LOAD_SEG + 0x172c },    /* part_hit_balloon */
        .step   = { 0x018e, LOAD_SEG + 0x172c },    /* part_step_balloon */
        .setup  = { 0x012d, LOAD_SEG + 0x172c },    /* part_setup_balloon */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02cd, LOAD_SEG + 0x172c },    /* part_drive_02cd */
    },
    [KIND_CONVEYOR] = {    /* DGROUP 0x0fc8 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0060, .max_h = 0x0000, .min_w = 0x0020, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .word_20 = 0x000c,
        .hit    = { 0x2514, LOAD_SEG + 0x172c },    /* part_hit_conveyor */
        .step   = { 0x2592, LOAD_SEG + 0x172c },    /* part_step_conveyor */
        .setup  = { 0x24d0, LOAD_SEG + 0x172c },    /* part_setup_conveyor */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x261d, LOAD_SEG + 0x172c },    /* part_settle_conveyor */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_MOUSE_CAGE] = {    /* DGROUP 0x1002 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .word_20 = 0x0025,
        .hit    = { 0x2f25, LOAD_SEG + 0x172c },    /* part_hit_mouse_cage */
        .step   = { 0x2f3e, LOAD_SEG + 0x172c },    /* part_step_mouse_cage */
        .setup  = { 0x2ee1, LOAD_SEG + 0x172c },    /* part_setup_mouse_cage */
        .flip   = { 0x2fba, LOAD_SEG + 0x172c },    /* part_flip_mouse_cage */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_PULLEY] = {    /* DGROUP 0x103c */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0000, .word_06 = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0000, .word_20 = 0x0011,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BELT] = {    /* DGROUP 0x1076 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0000, .word_06 = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x01, 0xff }, .point_count = 0x0000, .word_20 = 0x000a,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BASKETBALL] = {    /* DGROUP 0x10b0 */
        .word_00 = 0x052a, .weight = 0x0014, .word_04 = 0x00c0, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .word_20 = 0x0001,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x0001, LOAD_SEG + 0x172c },    /* part_setup_0001 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_ROPE] = {    /* DGROUP 0x10ea */
        .word_00 = 0x0640, .weight = 0x03e8, .word_04 = 0x0000, .word_06 = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x01, 0xff }, .point_count = 0x0000, .word_20 = 0x000f,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BIRD_CAGE] = {    /* DGROUP 0x1124 */
        .word_00 = 0x1d80, .weight = 0x0096, .word_04 = 0x0020, .word_06 = 0x0040,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x000c, .word_20 = 0x0022,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x0f70, LOAD_SEG + 0x172c },    /* part_setup_bird_cage */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x0ffc, LOAD_SEG + 0x172c },    /* part_drive_0ffc */
    },
    [KIND_POKEY] = {    /* DGROUP 0x115e */
        .word_00 = 0x07d0, .weight = 0x0078, .word_04 = 0x0000, .word_06 = 0x0040,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e56, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0005, .word_20 = 0x0023,
        .hit    = { 0x0c6c, LOAD_SEG + 0x172c },    /* part_hit_pokey */
        .step   = { 0x0ca3, LOAD_SEG + 0x172c },    /* part_step_pokey */
        .setup  = { 0x0c1c, LOAD_SEG + 0x172c },    /* part_setup_pokey */
        .flip   = { 0x0f3d, LOAD_SEG + 0x172c },    /* part_flip_pokey */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_JACK_IN_THE_BOX] = {    /* DGROUP 0x1198 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0250, .word_18 = 0x02c2, .word_1a = 0x0276,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0004, .word_20 = 0x000d,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x27e2, LOAD_SEG + 0x172c },    /* part_step_jack_in_the_box */
        .setup  = { 0x295d, LOAD_SEG + 0x172c },    /* part_setup_jack_in_the_box */
        .flip   = { 0x2999, LOAD_SEG + 0x172c },    /* part_flip_jack_in_the_box */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_GEAR] = {    /* DGROUP 0x11d2 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0030,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0008, .word_20 = 0x000b,
        .hit    = { 0x1f78, LOAD_SEG + 0x172c },    /* part_hit_gear */
        .step   = { 0x20fc, LOAD_SEG + 0x172c },    /* part_step_gear */
        .setup  = { 0x2068, LOAD_SEG + 0x172c },    /* part_setup_gear */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BOB_THE_FISH] = {    /* DGROUP 0x120c */
        .word_00 = 0x07d0, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0441, .word_18 = 0x04cb, .word_1a = 0x046f,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .word_20 = 0x0026,
        .hit    = { 0x1c39, LOAD_SEG + 0x172c },    /* part_hit_bob_the_fish */
        .step   = { 0x1c5f, LOAD_SEG + 0x172c },    /* part_step_bob_the_fish */
        .setup  = { 0x1be9, LOAD_SEG + 0x172c },    /* part_setup_bob_the_fish */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BELLOW] = {    /* DGROUP 0x1246 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e6a, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0006, .word_20 = 0x0007,
        .hit    = { 0x0332, LOAD_SEG + 0x172c },    /* part_hit_bellow */
        .step   = { 0x0405, LOAD_SEG + 0x172c },    /* part_step_bellow */
        .setup  = { 0x0371, LOAD_SEG + 0x172c },    /* part_setup_bellow */
        .flip   = { 0x03d2, LOAD_SEG + 0x172c },    /* part_flip_bellow */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BUCKET] = {    /* DGROUP 0x1280 */
        .word_00 = 0x1d80, .weight = 0x0064, .word_04 = 0x0020, .word_06 = 0x0030,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0006, .word_20 = 0x0021,
        .hit    = { 0x0763, LOAD_SEG + 0x172c },    /* part_hit_bucket */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x07b2, LOAD_SEG + 0x172c },    /* part_setup_bucket */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x0802, LOAD_SEG + 0x172c },    /* part_drive_0802 */
    },
    [KIND_CANNON] = {    /* DGROUP 0x12ba */
        .word_00 = 0x3986, .weight = 0x03e8, .word_04 = 0x00c0, .word_06 = 0x000c,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x05da, .word_18 = 0x0622, .word_1a = 0x05f2,
        .refile_level = { 0x04, 0x00 }, .point_count = 0x0008, .word_20 = 0x001c,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x0a5d, LOAD_SEG + 0x172c },    /* part_step_cannon */
        .setup  = { 0x0b88, LOAD_SEG + 0x172c },    /* part_setup_cannon */
        .flip   = { 0x0be9, LOAD_SEG + 0x172c },    /* part_flip_cannon */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_DYNAMITE] = {    /* DGROUP 0x12f4 */
        .word_00 = 0x046c, .weight = 0x005a, .word_04 = 0x0040, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0694, .word_18 = 0x06b8, .word_1a = 0x06a0,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0005, .word_20 = 0x001d,
        .hit    = { 0x1237, LOAD_SEG + 0x172c },    /* part_hit_dynamite */
        .step   = { 0x12c2, LOAD_SEG + 0x172c },    /* part_step_dynamite */
        .setup  = { 0x1261, LOAD_SEG + 0x172c },    /* part_setup_dynamite */
        .flip   = { 0x12fc, LOAD_SEG + 0x172c },    /* part_flip_dynamite */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BULLET] = {    /* DGROUP 0x132e */
        .word_00 = 0x0000, .weight = 0x4e20, .word_04 = 0x0000, .word_06 = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e70, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0004, .word_20 = 0x0032,
        .hit    = { 0x0867, LOAD_SEG + 0x172c },    /* part_hit_0867 */
        .step   = { 0x08f1, LOAD_SEG + 0x172c },    /* part_step_08f1 */
        .setup  = { 0x08a1, LOAD_SEG + 0x172c },    /* part_setup_08a1 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_ELECTRIC_PLUG] = {    /* DGROUP 0x1368 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x073c, .word_18 = 0x0000, .word_1a = 0x074c,
        .refile_level = { 0x05, 0xff }, .point_count = 0x0004, .word_20 = 0x0014,
        .hit    = { 0x14d3, LOAD_SEG + 0x172c },    /* part_hit_electric_plug */
        .step   = { 0x15ce, LOAD_SEG + 0x172c },    /* part_step_electric_plug */
        .setup  = { 0x1556, LOAD_SEG + 0x172c },    /* part_setup_electric_plug */
        .flip   = { 0x15fc, LOAD_SEG + 0x172c },    /* part_flip_electric_plug */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_DYNAMITE_PLUNGER] = {    /* DGROUP 0x13a2 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0799, .word_18 = 0x07ab, .word_1a = 0x079f,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .word_20 = 0x0020,
        .hit    = { 0x323f, LOAD_SEG + 0x172c },    /* part_hit_dynamite_plunger */
        .step   = { 0x332a, LOAD_SEG + 0x172c },    /* part_step_dynamite_plunger */
        .setup  = { 0x3294, LOAD_SEG + 0x172c },    /* part_setup_dynamite_plunger */
        .flip   = { 0x33e5, LOAD_SEG + 0x172c },    /* part_flip_dynamite_plunger */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x341d, LOAD_SEG + 0x172c },    /* part_drive_341d */
    },
    [KIND_HOOK] = {    /* DGROUP 0x13dc */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0000, .word_20 = 0x0010,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x19db, LOAD_SEG + 0x172c },    /* part_setup_hook */
        .flip   = { 0x19fa, LOAD_SEG + 0x172c },    /* part_flip_hook */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_FAN] = {    /* DGROUP 0x1416 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x07ed, .word_18 = 0x0000, .word_1a = 0x07f5,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0005, .word_20 = 0x0017,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x1a82, LOAD_SEG + 0x172c },    /* part_step_fan */
        .setup  = { 0x1a32, LOAD_SEG + 0x172c },    /* part_setup_fan */
        .flip   = { 0x1bbd, LOAD_SEG + 0x172c },    /* part_flip_fan */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_FLASHLIGHT] = {    /* DGROUP 0x1450 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e76, .word_1a = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0006, .word_20 = 0x001a,
        .hit    = { 0x1d07, LOAD_SEG + 0x172c },    /* part_hit_flashlight */
        .step   = { 0x1d78, LOAD_SEG + 0x172c },    /* part_step_flashlight */
        .setup  = { 0x1d28, LOAD_SEG + 0x172c },    /* part_setup_flashlight */
        .flip   = { 0x1da8, LOAD_SEG + 0x172c },    /* part_flip_flashlight */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_GENERATOR] = {    /* DGROUP 0x148a */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x08f5, .word_18 = 0x0955, .word_1a = 0x0915,
        .refile_level = { 0x05, 0xff }, .point_count = 0x0004, .word_20 = 0x0015,
        .hit    = { 0x1de0, LOAD_SEG + 0x172c },    /* part_hit_generator */
        .step   = { 0x1e5c, LOAD_SEG + 0x172c },    /* part_step_generator */
        .setup  = { 0x1dfb, LOAD_SEG + 0x172c },    /* part_setup_generator */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_GUN] = {    /* DGROUP 0x14c4 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x00c0, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x09de, .word_18 = 0x0a08, .word_1a = 0x09ec,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0007, .word_20 = 0x0012,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x22ae, LOAD_SEG + 0x172c },    /* part_step_gun */
        .setup  = { 0x23b1, LOAD_SEG + 0x172c },    /* part_setup_gun */
        .flip   = { 0x2412, LOAD_SEG + 0x172c },    /* part_flip_gun */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x2451, LOAD_SEG + 0x172c },    /* part_drive_2451 */
    },
    [KIND_BASEBALL] = {    /* DGROUP 0x14fe */
        .word_00 = 0x07d0, .weight = 0x0009, .word_04 = 0x0040, .word_06 = 0x0018,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .word_20 = 0x0003,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x00c9, LOAD_SEG + 0x172c },    /* part_setup_00c9 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_LIGHT] = {    /* DGROUP 0x1538 */
        .word_00 = 0x0514, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0a52, .word_18 = 0x0a6a, .word_1a = 0x0a5a,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0000, .word_20 = 0x001b,
        .hit    = { 0x2b7e, LOAD_SEG + 0x172c },    /* part_hit_light */
        .step   = { 0x2b99, LOAD_SEG + 0x172c },    /* part_step_light */
        .setup  = { 0x2b58, LOAD_SEG + 0x172c },    /* part_setup_light */
        .flip   = { 0x2bc5, LOAD_SEG + 0x172c },    /* part_flip_light */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x2c19, LOAD_SEG + 0x172c },    /* part_drive_2c19 */
    },
    [KIND_MAGNIFYING_GLASS] = {    /* DGROUP 0x1572 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0000, .word_20 = 0x0019,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x3035, LOAD_SEG + 0x172c },    /* part_step_magnifying_glass */
        .setup  = { 0x3030, LOAD_SEG + 0x172c },    /* part_setup_magnifying_glass */
        .flip   = { 0x31af, LOAD_SEG + 0x172c },    /* part_flip_magnifying_glass */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_MONKEY] = {    /* DGROUP 0x15ac */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0b35, .word_18 = 0x0000, .word_1a = 0x0b4f,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0009, .word_20 = 0x0027,
        .hit    = { 0x2c83, LOAD_SEG + 0x172c },    /* part_hit_monkey */
        .step   = { 0x2d40, LOAD_SEG + 0x172c },    /* part_step_monkey */
        .setup  = { 0x2cce, LOAD_SEG + 0x172c },    /* part_setup_monkey */
        .flip   = { 0x2e0c, LOAD_SEG + 0x172c },    /* part_flip_monkey */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x2e4b, LOAD_SEG + 0x172c },    /* part_drive_2e4b */
    },
    [KIND_PUMPKIN] = {    /* DGROUP 0x15e6 */
        .word_00 = 0x0960, .weight = 0x0064, .word_04 = 0x0040, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x35f4, LOAD_SEG + 0x172c },    /* part_setup_pumpkin */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_HEART_BALLOON] = {    /* DGROUP 0x1620 */
        .word_00 = 0x000b, .weight = 0x0004, .word_04 = 0x0080, .word_06 = 0x0008,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0007, .word_20 = 0x0033,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x2682, LOAD_SEG + 0x172c },    /* part_setup_heart_balloon */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x26c3, LOAD_SEG + 0x172c },    /* part_drive_26c3 */
    },
    [KIND_CHRISTMAS_TREE] = {    /* DGROUP 0x165a */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0007, .word_20 = 0x0034,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x1075, LOAD_SEG + 0x172c },    /* part_setup_christmas_tree */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BOXING_GLOVE] = {    /* DGROUP 0x1694 */
        .word_00 = 0x0ec0, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e7a, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0006, .word_20 = 0x0008,
        .hit    = { 0x0552, LOAD_SEG + 0x172c },    /* part_hit_boxing_glove */
        .step   = { 0x057e, LOAD_SEG + 0x172c },    /* part_step_boxing_glove */
        .setup  = { 0x065b, LOAD_SEG + 0x172c },    /* part_setup_boxing_glove */
        .flip   = { 0x06c6, LOAD_SEG + 0x172c },    /* part_flip_boxing_glove */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_ROCKET] = {    /* DGROUP 0x16ce */
        .word_00 = 0x4650, .weight = 0x0708, .word_04 = 0x0080, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0c19, .word_18 = 0x0c55, .word_1a = 0x0c2d,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0004, .word_20 = 0x001e,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x3635, LOAD_SEG + 0x172c },    /* part_step_rocket */
        .setup  = { 0x3737, LOAD_SEG + 0x172c },    /* part_setup_rocket */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_SCISSORS] = {    /* DGROUP 0x1708 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0c96, .word_18 = 0x0ca2, .word_1a = 0x0c9a,
        .refile_level = { 0x04, 0x00 }, .point_count = 0x0008, .word_20 = 0x0013,
        .hit    = { 0x3824, LOAD_SEG + 0x172c },    /* part_hit_scissors */
        .step   = { 0x38fc, LOAD_SEG + 0x172c },    /* part_step_scissors */
        .setup  = { 0x389b, LOAD_SEG + 0x172c },    /* part_setup_scissors */
        .flip   = { 0x3944, LOAD_SEG + 0x172c },    /* part_flip_scissors */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_SOLAR_PANEL] = {    /* DGROUP 0x1742 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0ce2, .word_18 = 0x0000, .word_1a = 0x0cea,
        .refile_level = { 0x05, 0xff }, .point_count = 0x0000, .word_20 = 0x0016,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x3e08, LOAD_SEG + 0x172c },    /* part_step_solar_panel */
        .setup  = { 0x3de5, LOAD_SEG + 0x172c },    /* part_setup_solar_panel */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_TRAMPOLINE] = {    /* DGROUP 0x177c */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0d90, .word_18 = 0x0dae, .word_1a = 0x0d9a,
        .refile_level = { 0x04, 0x00 }, .point_count = 0x0004, .word_20 = 0x0009,
        .hit    = { 0x3ebf, LOAD_SEG + 0x172c },    /* part_hit_trampoline */
        .step   = { 0x3fae, LOAD_SEG + 0x172c },    /* part_step_trampoline */
        .setup  = { 0x3f72, LOAD_SEG + 0x172c },    /* part_setup_trampoline */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_WINDMILL] = {    /* DGROUP 0x17b6 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0080, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e8e, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0003, .word_20 = 0x000e,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x49a1, LOAD_SEG + 0x172c },    /* part_step_windmill */
        .setup  = { 0x496f, LOAD_SEG + 0x172c },    /* part_setup_windmill */
        .flip   = { 0x4a22, LOAD_SEG + 0x172c },    /* part_flip_windmill */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BLAST] = {    /* DGROUP 0x17f0 */
        .word_00 = 0x0000, .weight = 0x0001, .word_04 = 0x0100, .word_06 = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0e96, .word_1a = 0x0000,
        .refile_level = { 0x00, 0xff }, .point_count = 0x0000, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x1649, LOAD_SEG + 0x172c },    /* part_step_1649 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_MORT_THE_MOUSE] = {    /* DGROUP 0x182a */
        .word_00 = 0x07d0, .weight = 0x0001, .word_04 = 0x0000, .word_06 = 0x0100,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0ea2, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0005, .word_20 = 0x0024,
        .hit    = { 0x34b5, LOAD_SEG + 0x172c },    /* part_hit_mort_the_mouse */
        .step   = { 0x34d0, LOAD_SEG + 0x172c },    /* part_step_mort_the_mouse */
        .setup  = { 0x346f, LOAD_SEG + 0x172c },    /* part_setup_mort_the_mouse */
        .flip   = { 0x35c7, LOAD_SEG + 0x172c },    /* part_flip_mort_the_mouse */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_CANNON_BALL] = {    /* DGROUP 0x1864 */
        .word_00 = 0x53b4, .weight = 0x6d60, .word_04 = 0x0020, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .word_20 = 0x0002,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x0065, LOAD_SEG + 0x172c },    /* part_setup_cannon_ball */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_TENNIS_BALL] = {    /* DGROUP 0x189e */
        .word_00 = 0x052a, .weight = 0x0005, .word_04 = 0x00c0, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .word_20 = 0x0004,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x00c9, LOAD_SEG + 0x172c },    /* part_setup_00c9 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_CANDLE] = {    /* DGROUP 0x18d8 */
        .word_00 = 0x07d0, .weight = 0x000c, .word_04 = 0x0080, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0e12, .word_18 = 0x0e36, .word_1a = 0x0e1e,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0003, .word_20 = 0x001f,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x098a, LOAD_SEG + 0x172c },    /* part_step_candle */
        .setup  = { 0x0950, LOAD_SEG + 0x172c },    /* part_setup_candle */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_PIPE] = {    /* DGROUP 0x1912 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .word_20 = 0x0029,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x48ab, LOAD_SEG + 0x172c },    /* part_setup_48ab */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x48f7, LOAD_SEG + 0x172c },    /* part_settle_48f7 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_CORNER_PIPE] = {    /* DGROUP 0x194c */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0008, .word_20 = 0x002a,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x377b, LOAD_SEG + 0x172c },    /* part_setup_corner_pipe */
        .flip   = { 0x37e5, LOAD_SEG + 0x172c },    /* part_flip_corner_pipe */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_WOODEN_PLATFORM] = {    /* DGROUP 0x1986 */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x000c,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .word_20 = 0x002b,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x48ab, LOAD_SEG + 0x172c },    /* part_setup_48ab */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x48f7, LOAD_SEG + 0x172c },    /* part_settle_48f7 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_ANCHOR] = {    /* DGROUP 0x19c0 */
        .word_00 = 0x0640, .weight = 0x03e8, .word_04 = 0x0000, .word_06 = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_MOTOR] = {    /* DGROUP 0x19fa */
        .word_00 = 0x1d80, .weight = 0x03e8, .word_04 = 0x0100, .word_06 = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0005, .word_20 = 0x0018,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x13c9, LOAD_SEG + 0x172c },    /* part_step_motor */
        .setup  = { 0x1435, LOAD_SEG + 0x172c },    /* part_setup_motor */
        .flip   = { 0x149b, LOAD_SEG + 0x172c },    /* part_flip_motor */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [51] = {    /* DGROUP 0x1a34 */
        .word_00 = 0x0064, .weight = 0x008c, .word_04 = 0x0000, .word_06 = 0x0002,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [52] = {    /* DGROUP 0x1a6e */
        .word_00 = 0x0064, .weight = 0x008c, .word_04 = 0x0000, .word_06 = 0x0002,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [53] = {    /* DGROUP 0x1aa8 */
        .word_00 = 0x0064, .weight = 0x008c, .word_04 = 0x0000, .word_06 = 0x0002,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [54] = {    /* DGROUP 0x1ae2 */
        .word_00 = 0x0064, .weight = 0x008c, .word_04 = 0x0000, .word_06 = 0x0002,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_55] = {    /* DGROUP 0x1b1c */
        .word_00 = 0x0064, .weight = 0x008c, .word_04 = 0x0040, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x1105, LOAD_SEG + 0x172c },    /* part_setup_1105 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_56] = {    /* DGROUP 0x1b56 */
        .word_00 = 0x0064, .weight = 0x008c, .word_04 = 0x0080, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x05, 0xff }, .point_count = 0x0007, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x10b6, LOAD_SEG + 0x172c },    /* part_setup_10b6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_57] = {    /* DGROUP 0x1b90 */
        .word_00 = 0x0064, .weight = 0x008c, .word_04 = 0x0080, .word_06 = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .word_18 = 0x0000, .word_1a = 0x0000,
        .refile_level = { 0x00, 0xff }, .point_count = 0x0004, .word_20 = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x11a6, LOAD_SEG + 0x172c },    /* part_step_11a6 */
        .setup  = { 0x1105, LOAD_SEG + 0x172c },    /* part_setup_1105 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x11d2, LOAD_SEG + 0x172c },    /* part_drive_11d2 */
    },
};
