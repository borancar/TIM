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

/*
 * The DGROUP structs dgroup.h declares, each at its address. Their
 * initialisers are the image's bytes - see DGROUP_AT in dgroup.h.
 */
struct vmds VMDS DGROUP_AT(0x3890) = {
    .clip_right = 0x013f,
    .clip_bottom = 0x00c7,
    .fill_enabled = 0x01,
    .screen = { .screen_width = 0x0140, .screen_height = 0x00c8 },
};
struct dg_50bf DG50BF DGROUP_BSS(0x50bf);
struct dg_4e67 DG4E67 DGROUP_BSS(0x4e67);
struct dg_5768 DG5768 DGROUP_BSS(0x5768);
struct dg_53fc DG53FC DGROUP_BSS(0x53fc);
struct dg_4a82 DG4A82 DGROUP_AT(0x4a82) = { .voice_word = 0xfffc, .bank_choice = 0x0001, .device = 0xfffe };
struct dg_52bd DG52BD DGROUP_BSS(0x52bd);
struct dg_52ed DG52ED DGROUP_BSS(0x52ed);
struct dg_52fe DG52FE DGROUP_BSS(0x52fe);
struct dg_4e4e DG4E4E DGROUP_BSS(0x4e4e);
struct dg_50af DG50AF DGROUP_BSS(0x50af);
struct timer TIMER DGROUP_AT(0x44ee) = { .word_44f1 = -1 };
struct game_text_lines GAME_TEXT_LINES DGROUP_BSS(0x56a6);
struct dg_5752 DG5752 DGROUP_BSS(0x5752);
struct dg_5456 DG5456 DGROUP_BSS(0x5456);
struct dg_4e34 DG4E34 DGROUP_AT(0x4e34) = { .pad_4e3a = { 0x0d }, .realcvt_ptr = 0xc884 };
struct chunk_names CHUNK DGROUP_AT(0x4966) = {
    .bmp_inf = "BMP:INF:",
    .bmp_bin = "BMP:BIN:",
    .mode_r_a = "r",
    .bmp_vga = "BMP:VGA:",
    .bmp_amg = "BMP:AMG:",
    .mode_r_b = "r",
    .scr_dim = "SCR:DIM:",
    .scr_bin = "SCR:BIN:",
    .mode_r_c = "r",
    .scr_vga = "SCR:VGA:",
    .scr_amg = "SCR:AMG:",
    .mode_r_d = "r",
    .mode_rb = "rb",
};
struct chunk_names2 CHUNK2 DGROUP_AT(0x49c6) = {
    .bmp_scn = "BMP:SCN:",
    .bmp_off = "BMP:OFF:",
    .bmp_vqt = "BMP:VQT:",
    .bmp_off_b = "BMP:OFF:",
    .bmp_rle = "BMP:RLE:",
    .bmp_scl = "BMP:SCL:",
    .scr_vqt = "SCR:VQT:",
    .ssm_000 = "SSM:000:",
    .ssm_tag = "SSM:     ",
};
struct pal_chunk_names PALCHUNK DGROUP_AT(0x4486) = {
    .pal_vga = "PAL:VGA:",
    .pal_ega = "PAL:EGA:",
    .pal_cga = "PAL:CGA:",
    .by_adapter = {
        0x44a1, 0x4498, 0x448f, 0x448f, 0x4498, 0x4486, 0x44a1, 0x4486,
        0x4486, 0x4486, 0x4486, 0x448f, 0x4486, 0x4486, 0x4486, 0x4486,
    },
    .pal_amg = "PAL:AMG:",
};
struct ovl_chunk_names OVLCHUNK DGROUP_AT(0x4919) = {
    .ovl_tag = "OVL:     ",
    .adapter_tag = {
        { 0x43, 0x47, 0x41, 0x3a }, { 0x45, 0x47, 0x41, 0x3a },
        { 0x54, 0x41, 0x4e, 0x3a }, { 0x48, 0x45, 0x52, 0x3a },
        { 0x4d, 0x43, 0x47, 0x3a }, { 0x45, 0x56, 0x41, 0x3a },
        { 0x56, 0x47, 0x41, 0x3a }, { 0x45, 0x56, 0x47, 0x3a },
        { 0x48, 0x56, 0x47, 0x3a }, { 0x48, 0x45, 0x47, 0x3a },
        { 0x4e, 0x45, 0x57, 0x3a },
    },
};
struct dg_50d3 DG50D3 DGROUP_BSS(0x50d3);
struct dg_5179 DG5179 DGROUP_BSS(0x5179);
struct dg_546c DG546C DGROUP_BSS(0x546c);
struct dg_48da DG48DA DGROUP_AT(0x48da) = {
    .word_48e6 = 0x02,
    .quarter_a = 0x40,
    .word_48e8 = 0x01,
    .quarter_b = 0x41,
    .mode_found = 0xff,
    .mode_forced = 0xff,
};
struct dg_3576 DG3576 DGROUP_AT(0x3576);
struct dg_521b DG521B DGROUP_BSS(0x521b);
struct dg_0094 DG0094 DGROUP_AT(0x0094) = { .pad_0096 = { [4] = 0xca, [5] = 0x64 }, .brklvl = 0x64ca };
struct dg_1bcc DG1BCC DGROUP_AT(0x1bcc) = {
    .not_enough_free_memory = "\012\012NOT ENOUGH FREE MEMORY\012",
    .you_need_at_least = "\012You need at least 550k of free memory to run 'The Incredible Machine'.\012\012",
    .unable_to_initialize_vm = "Unable to initialize vm.",
    .thanks_for_playing = "\012\012Thanks for playing 'The Incredible Machine'.\012The last password given to you was:  ",
    .please_select_in_order = "Please select, in order, the three parts listed on page ",
    .of_the_users_manual = " of the user's manual.",
    .version_number = "VERSION NUMBER",
    .this_is_version = "This is version 1.00 of 'The Incredible Machine.'",
    .memory_low = "MEMORY LOW",
    .memory_is_getting_low = "Memory is getting low.  You can only place a few more parts.",
    .out_of_memory = "OUT OF MEMORY",
    .you_cant_place_any = "You can't place any more parts.",
    .quit_game = "QUIT GAME",
    .quit_body = "Are you sure you want to quit the game?",
    .restart_level = "RESTART LEVEL",
    .restart_body = "Are you sure you want to clear all parts and restart this level?",
    .freeform_mode = "FREEFORM MODE",
    .freeform_body = "Are you sure you want to enter freeform mode?",
    .leave_freeform_mode = "LEAVE FREEFORM MODE",
    .leave_freeform_body = "Are you sure you want to leave freeform mode?",
    .cant_change_gravity = "CAN'T CHANGE GRAVITY",
    .gravity_body = "You are only allowed to change the gravitational force in freeform mode.",
    .cant_change_air_pressure = "CAN'T CHANGE AIR PRESSURE",
    .air_pressure_body = "You are only allowed to change the air pressure in freeform mode.",
    .overwrite_file = "OVERWRITE FILE",
    .overwrite_body = "File already exists.  Do you want to overwrite it?",
    .file_error = "FILE ERROR",
    .cant_open_for_saving = "Unable to open that file for saving.",
    .cant_open_for_loading = "Unable to open that file for loading.",
    .disk_write_protected = "Disk is write protected or there is not enough memory on that disk to save this machine.",
    .path_error = "PATH ERROR",
    .path_error_body = "Unable to choose that path.",
    .wrong_format = "WRONG FORMAT",
    .wrong_format_body = "That file has not been saved in 'The Incredible Machine' format.",
    .need_password = "NEED PASSWORD",
    .need_password_body = "You need to enter the correct password in order to try this puzzle.",
    .bad_password = "BAD PASSWORD",
    .bad_password_body = "That is not a valid password.",
    .score_code_invalid = "SCORE CODE INVALID",
    .score_code_body = "That score code is invalid.  Your score will be set to zero.",
    .parent_dir = "<PARENT DIR>",
    .load_machine = "LOAD MACHINE",
    .save_machine = "SAVE MACHINE",
    .load = "LOAD",
    .save = "SAVE",
    .cancel = "CANCEL",
    .file_name = "File Name:",
    .freeform_mode_title = "FREEFORM MODE",
    .puzzle_prefix = "PUZZLE ",
    .completed = " COMPLETED!",
    .total_bonus_points = "Total bonus points: ",
    .new_password = "New Password",
    .click_button_to_continue = "(click button to continue)",
    .replay_solution = "REPLAY SOLUTION",
    .replay_body = "Do you want to advance to the next puzzle or replay your solution to this puzzle?",
    .select_puzzle = "SELECT PUZZLE",
    .password = "PASSWORD",
    .solved_all_puzzles = "SOLVED ALL PUZZLES",
    .freeform_hint = "You can create any type of machine that you wish to in freeform mode.",
    .solved_all_body = "Wow!!  INCREDIBLE Job!!!  You have solved all of the puzzles!!  Advance will take you to freeform mode.",
    .path_sep = "\\",
};
struct dg_254a DG254A DGROUP_AT(0x254a) = {
    .sierra_bmp = "sierra.bmp",
    .sierra_scr = "sierra.scr",
    .corners_bmp = "corners.bmp",
    .title_gkc = "title.gkc",
    .credits_gkc = "credits.gkc",
    .icons_bmp = "icons.bmp",
};
struct dg_2630 DG2630 DGROUP_AT(0x2630) = {
    .goal_test = {
        { .off = 0x151b, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1476, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x15fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1cc4, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1cea, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1d1d, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1d5e, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x15fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1d8c, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1846, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1dbb, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1df1, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1e1e, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1907, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1907, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1a49, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1e59, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1eb9, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1b89, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x14ad, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x14cc, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x14ee, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x16a6, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1935, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x17db, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x16fb, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x15fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2065, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2010, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x15fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1fa6, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1846, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x15fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x18d9, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x242c, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x20fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2260, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x19ac, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1753, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x23ef, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1819, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1f25, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x172d, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x19e0, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x15fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1888, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1ab0, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1b89, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x23a4, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x15fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1d5e, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1b63, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x17ad, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x17ad, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x197e, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1a49, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1f77, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1d1d, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2351, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2231, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x203f, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x15fa, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x17ad, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1907, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1d5e, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1fe3, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x17ad, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x22d8, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1b2f, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1af7, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1a0c, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2322, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1907, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1f77, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1f25, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1ee6, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x21a6, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1552, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1630, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1a77, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1c0a, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1bd9, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x1a49, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2292, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x21fd, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2172, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x17ad, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2467, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2470, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2479, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2482, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x248b, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x2494, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x249d, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24a6, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24af, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24b4, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24b9, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24be, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24c3, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24c8, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24cd, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24d2, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24d7, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24dc, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24e1, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24e6, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24eb, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24f0, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x24f5, .seg = LOAD_SEG + 0x0000 },
    },
};
struct dg_4342 DG4342 DGROUP_AT(0x4342) = {
    .word_4344 = 0x0001,
    .font = {
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
    },
};
struct dg_5677 DG5677 DGROUP_BSS(0x5677);
struct dg_49ba DG49BA DGROUP_AT(0x49ba) = {
    .min_run = 0x0006,
    .fill_fn = { .off = 0x3e29, .seg = LOAD_SEG + 0x1c25 },
    .plot_fn = { .off = 0x61fd, .seg = LOAD_SEG + 0x1c25 },
    .read_fn = 0x1063,
};
bitmaps_t BITMAPS DGROUP_BSS(0x6400);
struct part_shapes PARTSHAPES DGROUP_AT(0x3182) = {
    .s_3182 = {
        { .y = 0x0a }, { .x = 0x0c }, { .x = 0x16 }, { .x = 0x1f, .y = 0x0a },
        { .x = 0x1f, .y = 0x1c }, { .x = 0x13, .y = 0x2b },
        { .x = 0x0b, .y = 0x2b }, { .y = 0x1d },
    },
    .s_3192 = {
        { 0 }, { .x = 0x2c, .y = 0x12 }, { .x = 0x3f, .y = 0x14 },
        { .x = 0x3f, .y = 0x1b }, { .x = 0x2c, .y = 0x1d }, { .y = 0x2f },
    },
    .s_319e = {
        { .y = 0x0a }, { .x = 0x2c, .y = 0x12 }, { .x = 0x3f, .y = 0x14 },
        { .x = 0x3f, .y = 0x1b }, { .x = 0x2c, .y = 0x1d }, { .y = 0x25 },
    },
    .s_31aa = {
        { .y = 0x0f }, { .x = 0x2c, .y = 0x12 }, { .x = 0x3f, .y = 0x14 },
        { .x = 0x3f, .y = 0x1b }, { .x = 0x2c, .y = 0x1d }, { .y = 0x20 },
    },
    .o_31b6 = { 0x3192, 0x319e, 0x31aa },
    .s_31bc = {
        { .y = 0x14 }, { .x = 0x13, .y = 0x12 }, { .x = 0x3f },
        { .x = 0x3f, .y = 0x2f }, { .x = 0x13, .y = 0x1d }, { .y = 0x1b },
    },
    .s_31c8 = {
        { .y = 0x14 }, { .x = 0x13, .y = 0x12 }, { .x = 0x47, .y = 0x0a },
        { .x = 0x47, .y = 0x25 }, { .x = 0x13, .y = 0x1d }, { .y = 0x1b },
    },
    .s_31d4 = {
        { .y = 0x14 }, { .x = 0x13, .y = 0x12 }, { .x = 0x47, .y = 0x0f },
        { .x = 0x47, .y = 0x20 }, { .x = 0x13, .y = 0x1d }, { .y = 0x1b },
    },
    .o_31e0 = { 0x31bc, 0x31c8, 0x31d4 },
    .glove_reach = { -32, -82, 0x0000, 0x0050, 0x0082 },
    .s_31f2 = {
        { .y = 0x0c }, { .x = 0x10 }, { .x = 0x2f, .y = 0x05 },
        { .x = 0x2f, .y = 0x14 }, { .x = 0x1a, .y = 0x15 },
        { .x = 0x09, .y = 0x1b },
    },
    .s_31fe = {
        { .x = 0x05, .y = 0x15 }, { .x = 0x11, .y = 0x0a },
        { .x = 0x2f, .y = 0x05 }, { .x = 0x2f, .y = 0x14 },
        { .x = 0x1a, .y = 0x15 }, { .x = 0x09, .y = 0x1b },
    },
    .s_320a = {
        { .x = 0x26, .y = 0x1b }, { .x = 0x15, .y = 0x15 }, { .y = 0x14 },
        { .y = 0x05 }, { .x = 0x1f }, { .x = 0x2f, .y = 0x0c },
    },
    .s_3216 = {
        { .x = 0x26, .y = 0x1b }, { .x = 0x15, .y = 0x15 }, { .y = 0x14 },
        { .y = 0x05 }, { .x = 0x1e, .y = 0x0a }, { .x = 0x2b, .y = 0x15 },
    },
    .s_3222 = {
        { .x = 0x1c }, { .x = 0x27, .y = 0x01 }, { .x = 0x27, .y = 0x05 },
        { .x = 0x1c, .y = 0x06 },
    },
    .s_322a = {
        { 0 }, { .x = 0x0b, .y = 0x01 }, { .x = 0x0b, .y = 0x05 },
        { .y = 0x06 },
    },
    .s_3232 = {
        { .x = 0x09, .y = 0x25 }, { .x = 0x12, .y = 0x06 }, { .x = 0x3c },
        { .x = 0x3f, .y = 0x14 }, { .x = 0x2f, .y = 0x1e },
        { .x = 0x2f, .y = 0x27 }, { .x = 0x22, .y = 0x33 },
        { .x = 0x16, .y = 0x33 },
    },
    .s_3242 = {
        { .y = 0x14 }, { .x = 0x03 }, { .x = 0x2d, .y = 0x06 },
        { .x = 0x36, .y = 0x25 }, { .x = 0x29, .y = 0x33 },
        { .x = 0x1d, .y = 0x33 }, { .x = 0x10, .y = 0x27 },
        { .x = 0x10, .y = 0x1e },
    },
    .s_3252 = {
        { .y = 0x07 }, { .x = 0x0a }, { .x = 0x24, .y = 0x1a },
        { .x = 0x24, .y = 0x25 }, { .x = 0x0a, .y = 0x28 },
    },
    .s_325c = {
        { .x = 0x03, .y = 0x1a }, { .x = 0x1d }, { .x = 0x27, .y = 0x0a },
        { .x = 0x1d, .y = 0x28 }, { .x = 0x03, .y = 0x25 },
    },
    .s_3266 = {
        { .y = 0x35 }, { .x = 0x14 }, { .x = 0x27, .y = 0x37 },
        { .x = 0x19, .y = 0x3d }, { .x = 0x19, .y = 0x48 },
        { .x = 0x10, .y = 0x48 }, { .x = 0x10, .y = 0x3d },
    },
    .s_3274 = {
        { .x = 0x19 }, { .x = 0x19, .y = 0x3c }, { .x = 0x72, .y = 0x3c },
        { .x = 0x72 }, { .x = 0xf8 }, { .x = 0xf8, .y = 0xb6 }, { .y = 0xb6 },
    },
    .s_3282 = {
        { .x = 0xf0 }, { .x = 0xf8 }, { .x = 0xf8, .y = 0x10 },
        { .x = 0xf6, .y = 0x14 }, { .x = 0xf4, .y = 0x14 },
        { .x = 0xf2, .y = 0x10 }, { .x = 0xf0, .y = 0x10 },
    },
    .s_3290 = {
        { .y = 0x0e }, { .x = 0x05 }, { .x = 0x25, .y = 0x12 },
        { .x = 0x1b, .y = 0x1b }, { .x = 0x14, .y = 0x1b },
    },
    .s_329a = {
        { .x = 0x0a, .y = 0x12 }, { .x = 0x2a }, { .x = 0x2f, .y = 0x0e },
        { .x = 0x1b, .y = 0x1b }, { .x = 0x14, .y = 0x1b },
    },
    .s_32a4 = {
        { .y = 0x13 }, { .x = 0x1a }, { .x = 0x35, .y = 0x18 },
        { .x = 0x30, .y = 0x2e }, { .x = 0x06, .y = 0x2e },
    },
    .s_32ae = {
        { .y = 0x18 }, { .x = 0x1b }, { .x = 0x35, .y = 0x13 },
        { .x = 0x2f, .y = 0x2e }, { .x = 0x05, .y = 0x2e },
    },
    .s_32b8 = {
        { .x = 0x08, .y = 0x08 }, { .x = 0x0f, .y = 0x08 },
        { .x = 0x0f, .y = 0x09 }, { .x = 0x08, .y = 0x09 },
    },
    .s_32c0 = {
        { .x = 0x0f, .y = 0x18 }, { .x = 0x08, .y = 0x18 },
        { .x = 0x08, .y = 0x17 }, { .x = 0x0f, .y = 0x17 },
    },
    .s_32c8 = {
        { .y = 0x0b }, { .x = 0x16 }, { .x = 0x1f, .y = 0x0e },
        { .x = 0x17, .y = 0x1f }, { .x = 0x03, .y = 0x1f },
    },
    .s_32d2 = {
        { .y = 0x0e }, { .x = 0x09 }, { .x = 0x1f, .y = 0x0b },
        { .x = 0x1c, .y = 0x1f }, { .x = 0x08, .y = 0x1f },
    },
    .p_32dc = {
        { .y = 0x0012 }, { .x = 0x000b }, { .x = 0x0025 },
        { .x = 0x002f, .y = 0x0012 }, { .x = 0x002f, .y = 0x0023 },
        { .x = 0x0027, .y = 0x002f }, { .x = 0x0008, .y = 0x002f },
        { .y = 0x0022 },
    },
    .s_32fc = {
        { .y = 0x04 }, { .x = 0x17, .y = 0x04 }, { .x = 0x1f },
        { .x = 0x1f, .y = 0x10 }, { .x = 0x17, .y = 0x0c }, { .y = 0x0c },
    },
    .s_3308 = {
        { .x = 0x08, .y = 0x04 }, { .x = 0x1f, .y = 0x04 },
        { .x = 0x1f, .y = 0x0c }, { .x = 0x08, .y = 0x0c }, { .y = 0x10 },
    },
    .s_3314 = {
        { .y = 0x1e }, { .x = 0x07, .y = 0x0a }, { .x = 0x12, .y = 0x01 },
        { .x = 0x3f, .y = 0x03 }, { .x = 0x3f, .y = 0x09 },
        { .x = 0x1b, .y = 0x10 }, { .x = 0x0c, .y = 0x1e },
    },
    .s_3322 = {
        { .y = 0x03 }, { .x = 0x2d, .y = 0x01 }, { .x = 0x38, .y = 0x0a },
        { .x = 0x3f, .y = 0x1e }, { .x = 0x33, .y = 0x1e },
        { .x = 0x24, .y = 0x10 }, { .y = 0x09 },
    },
    .conveyor_grab_x = { 0x09, 0x17, 0x26, 0x2c, 0x3b },
    .s_3336 = {
        { .y = 0x08 }, { .x = 0x06 }, { .x = 0x1e }, { .x = 0x24, .y = 0x07 },
        { .x = 0x24, .y = 0x10 }, { .x = 0x11, .y = 0x23 }, { .y = 0x10 },
    },
    .s_3344 = {
        { 0 }, { .x = 0x0f, .y = 0x0f }, { .x = 0x0f, .y = 0x1f },
        { .y = 0x10 },
    },
    .s_334c = {
        { 0 }, { .x = 0x1f, .y = 0x0f }, { .x = 0x1f, .y = 0x1f },
        { .y = 0x10 },
    },
    .s_3354 = {
        { 0 }, { .x = 0x2f, .y = 0x0f }, { .x = 0x2f, .y = 0x1f },
        { .y = 0x10 },
    },
    .s_335c = {
        { 0 }, { .x = 0x3f, .y = 0x0f }, { .x = 0x3f, .y = 0x1f },
        { .y = 0x10 },
    },
    .o_3364 = { 0x3344, 0x334c, 0x3354, 0x335c },
    .s_336c = {
        { .y = 0x0f }, { .x = 0x0f }, { .x = 0x0f, .y = 0x10 }, { .y = 0x1f },
    },
    .s_3374 = {
        { .y = 0x0f }, { .x = 0x1f }, { .x = 0x1f, .y = 0x10 }, { .y = 0x1f },
    },
    .s_337c = {
        { .y = 0x0f }, { .x = 0x2f }, { .x = 0x2f, .y = 0x10 }, { .y = 0x1f },
    },
    .s_3384 = {
        { .y = 0x0f }, { .x = 0x3f }, { .x = 0x3f, .y = 0x10 }, { .y = 0x1f },
    },
    .o_338c = { 0x336c, 0x3374, 0x337c, 0x3384 },
    .jack_reach = { -21, -34, -59 },
    .p_339a = {
        { .x = 0x0015, .y = 0x0033 }, { .x = 0x001d, .y = 0x004f },
        { .x = 0x0014, .y = 0x0019 }, { .x = 0x001c, .y = 0x0023 },
    },
    .s_33aa = {
        { .x = 0x11, .y = 0x2a }, { .x = 0x21, .y = 0x0c },
        { .x = 0x28, .y = 0x0d }, { .x = 0x2a, .y = 0x25 },
        { .x = 0x42, .y = 0x3c }, { .x = 0x42, .y = 0x44 },
        { .x = 0x38, .y = 0x4e }, { .x = 0x17, .y = 0x43 }, { .y = 0x4c },
    },
    .s_33bc = {
        { .x = 0x31, .y = 0x25 }, { .x = 0x33, .y = 0x0d },
        { .x = 0x3a, .y = 0x0c }, { .x = 0x4a, .y = 0x2a },
        { .x = 0x5b, .y = 0x4c }, { .x = 0x44, .y = 0x43 },
        { .x = 0x23, .y = 0x4e }, { .x = 0x19, .y = 0x44 },
        { .x = 0x19, .y = 0x3c },
    },
    .s_33ce = {
        { .x = 0x67 }, { .x = 0x86 }, { .x = 0x7f, .y = 0x2f },
        { .x = 0x6f, .y = 0x2f },
    },
    .s_33d6 = {
        { .x = 0x67, .y = 0x05 }, { .x = 0x86, .y = 0x05 },
        { .x = 0x7f, .y = 0x2f }, { .x = 0x6f, .y = 0x2f },
    },
    .s_33de = {
        { .x = 0x67, .y = 0x0a }, { .x = 0x86, .y = 0x0a },
        { .x = 0x7f, .y = 0x2f }, { .x = 0x6f, .y = 0x2f },
    },
    .o_33e6 = { 0x33ce, 0x33d6, 0x33de },
    .s_33ec = {
        { 0 }, { .x = 0x1f }, { .x = 0x18, .y = 0x2f },
        { .x = 0x08, .y = 0x2f },
    },
    .s_33f4 = {
        { .y = 0x05 }, { .x = 0x1f, .y = 0x05 }, { .x = 0x18, .y = 0x2f },
        { .x = 0x08, .y = 0x2f },
    },
    .s_33fc = {
        { .y = 0x0a }, { .x = 0x1f, .y = 0x0a }, { .x = 0x18, .y = 0x2f },
        { .x = 0x08, .y = 0x2f },
    },
    .o_3404 = { 0x33ec, 0x33f4, 0x33fc },
    .p_340a = {
        { .x = 0x0072 }, { .x = 0x0072, .y = 0x0005 },
        { .x = 0x0072, .y = 0x000a },
    },
    .p_3416 = {
        { .x = 0x000b }, { .x = 0x000b, .y = 0x0005 },
        { .x = 0x000b, .y = 0x000a },
    },
    .s_3422 = {
        { .y = 0x0f }, { .x = 0x09, .y = 0x09 }, { .x = 0x1d, .y = 0x09 },
        { .x = 0x26, .y = 0x12 }, { .x = 0x26, .y = 0x16 },
        { .x = 0x1b, .y = 0x20 }, { .x = 0x0b, .y = 0x20 }, { .y = 0x16 },
    },
    .s_3432 = {
        { 0 }, { .x = 0x13, .y = 0x01 }, { .x = 0x1e, .y = 0x0c },
        { .x = 0x1f, .y = 0x1f }, { .x = 0x10, .y = 0x1f },
        { .x = 0x10, .y = 0x14 }, { .x = 0x0b, .y = 0x0f }, { .y = 0x0f },
    },
    .s_3442 = {
        { .y = 0x1f }, { .x = 0x01, .y = 0x0c }, { .x = 0x0c, .y = 0x01 },
        { .x = 0x1f }, { .x = 0x1f, .y = 0x0f }, { .x = 0x14, .y = 0x0f },
        { .x = 0x0f, .y = 0x14 }, { .x = 0x0f, .y = 0x1f },
    },
    .s_3452 = {
        { .y = 0x10 }, { .x = 0x0b, .y = 0x10 }, { .x = 0x10, .y = 0x0b },
        { .x = 0x10 }, { .x = 0x1f }, { .x = 0x1e, .y = 0x13 },
        { .x = 0x13, .y = 0x1e }, { .y = 0x1f },
    },
    .s_3462 = {
        { 0 }, { .x = 0x0f }, { .x = 0x0f, .y = 0x0b },
        { .x = 0x14, .y = 0x10 }, { .x = 0x1f, .y = 0x10 },
        { .x = 0x1f, .y = 0x1f }, { .x = 0x0c, .y = 0x1e },
        { .x = 0x01, .y = 0x13 },
    },
    .s_3472 = {
        { .y = 0x04 }, { .x = 0x0a }, { .x = 0x11, .y = 0x0b },
        { .x = 0x27, .y = 0x07 }, { .x = 0x27, .y = 0x1a },
        { .x = 0x11, .y = 0x14 }, { .x = 0x0b, .y = 0x21 }, { .y = 0x1f },
    },
    .s_3482 = {
        { .y = 0x08 }, { .x = 0x09, .y = 0x04 }, { .x = 0x10, .y = 0x0e },
        { .x = 0x27, .y = 0x10 }, { .x = 0x27, .y = 0x12 },
        { .x = 0x10, .y = 0x14 }, { .x = 0x09, .y = 0x1b }, { .y = 0x18 },
    },
    .o_3492 = { 0x3472, 0x3482 },
    .s_3496 = {
        { .y = 0x07 }, { .x = 0x16, .y = 0x0b }, { .x = 0x1d },
        { .x = 0x27, .y = 0x04 }, { .x = 0x27, .y = 0x1f },
        { .x = 0x1c, .y = 0x21 }, { .x = 0x16, .y = 0x14 }, { .y = 0x1a },
    },
    .s_34a6 = {
        { .y = 0x10 }, { .x = 0x17, .y = 0x0e }, { .x = 0x1e, .y = 0x04 },
        { .x = 0x27, .y = 0x08 }, { .x = 0x27, .y = 0x18 },
        { .x = 0x1e, .y = 0x1b }, { .x = 0x17, .y = 0x1e }, { .y = 0x12 },
    },
    .o_34b6 = { 0x3496, 0x34a6 },
    .unread_34ba = {
        [0] = 0x16,
        [2] = 0x0f,
        [4] = 0x27,
        [6] = 0x0f,
        [10] = 0x0f,
        [12] = 0x10,
        [14] = 0x0f,
    },
    .p_34ca = {
        { .x = 0x0005, .y = 0x001b }, { .x = 0x0004, .y = 0x0002 },
        { .x = 0x0006, .y = 0x0003 },
    },
    .p_34d6 = {
        { .x = 0x0049, .y = 0x0003 }, { .x = 0x004b, .y = 0x0002 },
        { .x = 0x004a, .y = 0x001b },
    },
    .p_34e2 = {
        { .y = 0x0020 }, { .x = 0x004f, .y = 0x0003 },
        { .x = 0x004f, .y = 0x0008 }, { .x = 0x002c, .y = 0x0015 },
        { .x = 0x002c, .y = 0x0022 }, { .x = 0x0024, .y = 0x0022 },
        { .x = 0x0024, .y = 0x0018 }, { .y = 0x0024 },
    },
    .p_3502 = {
        { .y = 0x0011 }, { .x = 0x004f, .y = 0x0011 },
        { .x = 0x004f, .y = 0x0015 }, { .x = 0x002c, .y = 0x0015 },
        { .x = 0x002c, .y = 0x0022 }, { .x = 0x0024, .y = 0x0022 },
        { .x = 0x0024, .y = 0x0015 }, { .y = 0x0015 },
    },
    .p_3522 = {
        { .y = 0x0003 }, { .x = 0x004f, .y = 0x0020 },
        { .x = 0x004f, .y = 0x0024 }, { .x = 0x002c, .y = 0x0018 },
        { .x = 0x002c, .y = 0x0022 }, { .x = 0x0024, .y = 0x0022 },
        { .x = 0x0024, .y = 0x0015 }, { .y = 0x0008 }, { .y = 0x0020 },
        { .x = 0x004f, .y = 0x0003 }, { .y = 0x0011 },
        { .x = 0x004f, .y = 0x0011 }, { .y = 0x0003 },
        { .x = 0x004f, .y = 0x0020 }, { .x = 0x4f46, .y = 0x4d52 },
        { .x = 0x4900, .y = 0x424c }, { .x = 0x004d, .y = 0x4d42 },
        { .x = 0x4448, .y = 0x4300 }, { .x = 0x414d, .y = 0x0050 },
        { .x = 0x4f42, .y = 0x5944 }, { .x = 0x7700, .y = 0x0062 },
    },
};
