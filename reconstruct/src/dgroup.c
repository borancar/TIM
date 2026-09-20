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

/* The draw step `draw_part` fills in for a part whose kind has no table. */
struct draw_step DG0124 DGROUP_AT(0x0124) = { .frame = { [1] = 0xff } };

/*
 * DGROUP 0x0133..0x0ea6 - **the kinds' drawing tables**, which `PART_KINDS`
 * points into and nothing else does. A kind that draws in steps has a run of
 * `draw_step`s, then three tables indexed by form: the first step
 * (`bitmaps2_ptr`), the size (`sizes_ptr`) and the hot spot (`hotspots_ptr`); a kind
 * that draws one bitmap per form has at most the last two. The boundaries are
 * where the kind records point, and they tile the range with nothing left
 * over. The `next` links and the table entries are near pointers, as the
 * image has them.
 */
struct draw_step JACK_IN_THE_BOX_DRAW_STEPS[19] DGROUP_AT(0x0133) = {
    [0] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x02, 0xff },
        .offset = { { .y = 0x03 }, { 0 }, { .x = 0x09, .y = 0x09 } },
    },
    [1] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x03, 0xff },
        .offset = { { .y = 0x03 }, { 0 }, { .x = 0x09, .y = 0x09 } },
    },
    [2] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x04, 0xff },
        .offset = { { .y = 0x03 }, { 0 }, { .x = 0x09, .y = 0x09 } },
    },
    [3] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x05, 0xff },
        .offset = { { .y = 0x03 }, { 0 }, { .x = 0x09, .y = 0x09 } },
    },
    [4] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x06, 0xff },
        .offset = { { .y = 0x03 }, { 0 }, { .x = 0x09, .y = 0x09 } },
    },
    [5] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x07, 0xff },
        .offset = { { .y = 0x03 }, { 0 }, { .x = 0x08, .y = 0x09 } },
    },
    [6] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x08, 0xff },
        .offset = { { .y = 0x03 }, { 0 }, { .x = 0x09, .y = 0x09 } },
    },
    [7] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x09, 0xff },
        .offset = { { .y = 0x03 }, { 0 }, { .x = 0x09, .y = 0x08 } },
    },
    [8] = {
        .level = 0x03,
        .frame = { 0x00, 0x0a, 0x02, 0xff },
        .offset = { { .y = 0x03 }, { .y = 0xeb }, { .x = 0x09, .y = 0x09 } },
    },
    [9] = {
        .level = 0x03,
        .frame = { 0x00, 0x0b, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0x01, .y = 0xde }, { .x = 0x09, .y = 0x09 },
        },
    },
    [10] = {
        .level = 0x03,
        .frame = { 0x00, 0x0c, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfa, .y = 0xc5 }, { .x = 0x09, .y = 0x09 },
        },
    },
    [11] = {
        .level = 0x03,
        .frame = { 0x00, 0x0d, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfc, .y = 0xe7 }, { .x = 0x09, .y = 0x09 },
        },
    },
    [12] = {
        .level = 0x03,
        .frame = { 0x00, 0x0e, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfa, .y = 0xe2 }, { .x = 0x09, .y = 0x09 },
        },
    },
    [13] = {
        .level = 0x03,
        .frame = { 0x00, 0x0f, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfc, .y = 0xe7 }, { .x = 0x09, .y = 0x09 },
        },
    },
    [14] = {
        .level = 0x03,
        .frame = { 0x00, 0x10, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfc, .y = 0xe7 }, { .x = 0x09, .y = 0x09 },
        },
    },
    [15] = {
        .level = 0x03,
        .frame = { 0x00, 0x11, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfc, .y = 0xe7 }, { .x = 0x09, .y = 0x09 },
        },
    },
    [16] = {
        .level = 0x03,
        .frame = { 0x00, 0x12, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfc, .y = 0xe7 }, { .x = 0x09, .y = 0x09 },
        },
    },
    [17] = {
        .level = 0x03,
        .frame = { 0x00, 0x13, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfc, .y = 0xe7 }, { .x = 0x09, .y = 0x09 },
        },
    },
    [18] = {
        .level = 0x03,
        .frame = { 0x00, 0x14, 0x02, 0xff },
        .offset = {
            { .y = 0x03 }, { .x = 0xfc, .y = 0xe7 }, { .x = 0x09, .y = 0x09 },
        },
    },
};
dg_near_t JACK_IN_THE_BOX_FORM_STEPS[19] DGROUP_AT(0x0250) = {
    0x0133, 0x0142, 0x0151, 0x0160, 0x016f, 0x017e, 0x018d, 0x019c, 0x01ab,
    0x01ba, 0x01c9, 0x01d8, 0x01e7, 0x01f6, 0x0205, 0x0214, 0x0223, 0x0232,
    0x0241,
};
struct point16 JACK_IN_THE_BOX_FORM_SIZES[19] DGROUP_AT(0x0276) = {
    { .x = 0x0020, .y = 0x0020 }, { .x = 0x0020, .y = 0x0020 },
    { .x = 0x0020, .y = 0x0020 }, { .x = 0x0020, .y = 0x0020 },
    { .x = 0x0020, .y = 0x0020 }, { .x = 0x0020, .y = 0x0020 },
    { .x = 0x0020, .y = 0x0020 }, { .x = 0x0020, .y = 0x0020 },
    { .x = 0x0023, .y = 0x0035 }, { .x = 0x0023, .y = 0x0042 },
    { .x = 0x0027, .y = 0x005b }, { .x = 0x0024, .y = 0x0039 },
    { .x = 0x0026, .y = 0x003e }, { .x = 0x0025, .y = 0x0039 },
    { .x = 0x0029, .y = 0x0039 }, { .x = 0x0028, .y = 0x0039 },
    { .x = 0x0024, .y = 0x0039 }, { .x = 0x0024, .y = 0x0039 },
    { .x = 0x0024, .y = 0x0039 },
};
struct point8 JACK_IN_THE_BOX_HOT_SPOTS[19] DGROUP_AT(0x02c2) = {
    [8] = { .y = 0xeb },
    [9] = { .y = 0xde },
    [10] = { .x = 0xfa, .y = 0xc4 },
    [11] = { .x = 0xfc, .y = 0xe7 },
    [12] = { .x = 0xfa, .y = 0xe2 },
    [13] = { .x = 0xfc, .y = 0xe7 },
    [14] = { .x = 0xfc, .y = 0xe7 },
    [15] = { .x = 0xfc, .y = 0xe7 },
    [16] = { .x = 0xfc, .y = 0xe7 },
    [17] = { .x = 0xfc, .y = 0xe7 },
    [18] = { .x = 0xfc, .y = 0xe7 },
};
struct draw_step BOB_THE_FISH_DRAW_STEPS[23] DGROUP_AT(0x02e8) = {
    [0] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x0a, .y = 0x0b }, { .x = 0x0a, .y = 0x1b } },
    },
    [1] = {
        .level = 0x03,
        .frame = { 0x00, 0x02, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x0f, .y = 0x09 }, { .x = 0x0a, .y = 0x1b } },
    },
    [2] = {
        .level = 0x03,
        .frame = { 0x00, 0x03, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x1a, .y = 0x10 }, { .x = 0x0a, .y = 0x1b } },
    },
    [3] = {
        .level = 0x03,
        .frame = { 0x00, 0x04, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x1f, .y = 0x11 }, { .x = 0x0a, .y = 0x1b } },
    },
    [4] = {
        .level = 0x03,
        .frame = { 0x00, 0x05, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x1a, .y = 0x10 }, { .x = 0x0a, .y = 0x1b } },
    },
    [5] = {
        .level = 0x03,
        .frame = { 0x00, 0x06, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x0a, .y = 0x0b }, { .x = 0x0a, .y = 0x1b } },
    },
    [6] = {
        .level = 0x03,
        .frame = { 0x00, 0x07, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x07, .y = 0x0b }, { .x = 0x0a, .y = 0x1b } },
    },
    [7] = {
        .level = 0x03,
        .frame = { 0x00, 0x08, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x06, .y = 0x0a }, { .x = 0x0a, .y = 0x1b } },
    },
    [8] = {
        .level = 0x03,
        .frame = { 0x00, 0x09, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x07, .y = 0x10 }, { .x = 0x0a, .y = 0x1b } },
    },
    [9] = {
        .level = 0x03,
        .frame = { 0x00, 0x0a, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x08, .y = 0x10 }, { .x = 0x0a, .y = 0x1b } },
    },
    [10] = {
        .level = 0x03,
        .frame = { 0x00, 0x0b, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x06, .y = 0x0a }, { .x = 0x0a, .y = 0x1b } },
    },
    [11] = {
        .level = 0x03,
        .frame = { 0x0d, 0xff, 0xff, 0xff },
        .offset = { { .x = 0xf0, .y = 0x08 } },
    },
    [12] = {
        .level = 0x03,
        .frame = { 0x0e, 0xff, 0xff, 0xff },
        .offset = { { .x = 0xed, .y = 0x0f } },
    },
    [13] = {
        .level = 0x03,
        .frame = { 0x0f, 0xff, 0xff, 0xff },
        .offset = { { .x = 0xe7, .y = 0x13 } },
    },
    [14] = {
        .level = 0x03,
        .frame = { 0x10, 0x11, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0x05, .y = 0x27 } },
    },
    [15] = {
        .level = 0x03,
        .frame = { 0x10, 0x12, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0x05, .y = 0x1f } },
    },
    [16] = {
        .level = 0x03,
        .frame = { 0x10, 0x13, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0x03, .y = 0x1d } },
    },
    [17] = {
        .level = 0x03,
        .frame = { 0x10, 0x14, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0x05, .y = 0x1d } },
    },
    [18] = {
        .level = 0x03,
        .frame = { 0x10, 0x15, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0xfb, .y = 0x1b } },
    },
    [19] = {
        .level = 0x03,
        .frame = { 0x10, 0x16, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0xfb, .y = 0x16 } },
    },
    [20] = {
        .level = 0x03,
        .frame = { 0x10, 0x17, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0x01, .y = 0x1f } },
    },
    [21] = {
        .level = 0x03,
        .frame = { 0x10, 0x18, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0x05, .y = 0x24 } },
    },
    [22] = {
        .level = 0x03,
        .frame = { 0x10, 0x19, 0xff, 0xff },
        .offset = { { .x = 0xe4, .y = 0x19 }, { .x = 0x05, .y = 0x26 } },
    },
};
dg_near_t BOB_THE_FISH_FORM_STEPS[23] DGROUP_AT(0x0441) = {
    0x02e8, 0x02f7, 0x0306, 0x0315, 0x0324, 0x0333, 0x0342, 0x0351, 0x0360,
    0x036f, 0x037e, 0x038d, 0x039c, 0x03ab, 0x03ba, 0x03c9, 0x03d8, 0x03e7,
    0x03f6, 0x0405, 0x0414, 0x0423, 0x0432,
};
struct point16 BOB_THE_FISH_FORM_SIZES[23] DGROUP_AT(0x046f) = {
    { .x = 0x0030, .y = 0x0030 }, { .x = 0x0030, .y = 0x0030 },
    { .x = 0x0030, .y = 0x0030 }, { .x = 0x0030, .y = 0x0030 },
    { .x = 0x0030, .y = 0x0030 }, { .x = 0x0030, .y = 0x0030 },
    { .x = 0x0030, .y = 0x0030 }, { .x = 0x0030, .y = 0x0030 },
    { .x = 0x0030, .y = 0x0030 }, { .x = 0x0030, .y = 0x0030 },
    { .x = 0x0030, .y = 0x0030 }, { .x = 0x0058, .y = 0x002b },
    { .x = 0x0058, .y = 0x0026 }, { .x = 0x0060, .y = 0x0025 },
    { .x = 0x0068, .y = 0x0020 }, { .x = 0x0068, .y = 0x0020 },
    { .x = 0x0068, .y = 0x0020 }, { .x = 0x0068, .y = 0x0020 },
    { .x = 0x0068, .y = 0x0020 }, { .x = 0x0068, .y = 0x0023 },
    { .x = 0x0068, .y = 0x0020 }, { .x = 0x0068, .y = 0x0020 },
    { .x = 0x0068, .y = 0x0020 },
};
struct point8 BOB_THE_FISH_HOT_SPOTS[23] DGROUP_AT(0x04cb) = {
    [11] = { .x = 0xf0, .y = 0x08 },
    [12] = { .x = 0xed, .y = 0x0f },
    [13] = { .x = 0xe7, .y = 0x13 },
    [14] = { .x = 0xe4, .y = 0x19 },
    [15] = { .x = 0xe4, .y = 0x19 },
    [16] = { .x = 0xe4, .y = 0x19 },
    [17] = { .x = 0xe4, .y = 0x19 },
    [18] = { .x = 0xe4, .y = 0x19 },
    [19] = { .x = 0xe4, .y = 0x16 },
    [20] = { .x = 0xe4, .y = 0x19 },
    [21] = { .x = 0xe4, .y = 0x19 },
    [22] = { .x = 0xe4, .y = 0x19 },
};
struct draw_step CANNON_DRAW_STEPS[15] DGROUP_AT(0x04f9) = {
    [0] = {
        .level = 0x04,
        .frame = { 0x00, 0x09, 0xff, 0xff },
        .offset = { [1] = { .x = 0x09, .y = 0x0d } },
    },
    [1] = {
        .level = 0x04,
        .frame = { 0x00, 0x09, 0x0a, 0xff },
        .offset = { { 0 }, { .x = 0x09, .y = 0x0d }, { .x = 0xf7, .y = 0xfa } },
    },
    [2] = {
        .level = 0x04,
        .frame = { 0x00, 0x09, 0x0b, 0xff },
        .offset = { { 0 }, { .x = 0x09, .y = 0x0d }, { .x = 0xf7, .y = 0xfa } },
    },
    [3] = {
        .level = 0x04,
        .frame = { 0x00, 0x09, 0x0c, 0xff },
        .offset = { { 0 }, { .x = 0x09, .y = 0x0d }, { .x = 0xfa, .y = 0xfb } },
    },
    [4] = {
        .level = 0x04,
        .frame = { 0x00, 0x09, 0x0d, 0xff },
        .offset = { { 0 }, { .x = 0x09, .y = 0x0d }, { .x = 0xf9, .y = 0xfd } },
    },
    [5] = {
        .level = 0x04,
        .frame = { 0x00, 0x09, 0x0e, 0xff },
        .offset = { { 0 }, { .x = 0x09, .y = 0x0d }, { .x = 0xfa, .y = 0xfe } },
    },
    [6] = {
        .level = 0x04,
        .frame = { 0x01, 0x09, 0xff, 0xff },
        .offset = { { .x = 0xf9, .y = 0xf8 }, { .x = 0x09, .y = 0x0d } },
    },
    [7] = {
        .level = 0x04,
        .frame = { 0x02, 0x09, 0xff, 0xff },
        .offset = { { .x = 0xf8, .y = 0xf5 }, { .x = 0x09, .y = 0x0d } },
    },
    [8] = {
        .frame = { 0x04, 0xff, 0xff, 0xff },
        .offset = { { .x = 0x53, .y = 0xdf } },
    },
    [9] = {
        .next = 0x0571,
        .level = 0x04,
        .frame = { 0x03, 0x09, 0xff, 0xff },
        .offset = { { .x = 0xfe, .y = 0xfd }, { .x = 0x09, .y = 0x0d } },
    },
    [10] = {
        .frame = { 0x06, 0xff, 0xff, 0xff },
        .offset = { { .x = 0x65, .y = 0xd2 } },
    },
    [11] = {
        .next = 0x058f,
        .level = 0x04,
        .frame = { 0x05, 0x09, 0xff, 0xff },
        .offset = { { .x = 0xfe, .y = 0xf9 }, { .x = 0x09, .y = 0x0d } },
    },
    [12] = {
        .frame = { 0x08, 0xff, 0xff, 0xff },
        .offset = { { .x = 0x7f, .y = 0xdf } },
    },
    [13] = {
        .next = 0x05ad,
        .level = 0x04,
        .frame = { 0x07, 0x09, 0xff, 0xff },
        .offset = { { .x = 0xfd, .y = 0xfd }, { .x = 0x09, .y = 0x0d } },
    },
    [14] = {
        .level = 0x04,
        .frame = { 0x00, 0x09, 0x0f, 0xff },
        .offset = { { 0 }, { .x = 0x09, .y = 0x0d }, { .y = 0x02 } },
    },
};
dg_near_t CANNON_FORM_STEPS[12] DGROUP_AT(0x05da) = {
    0x04f9, 0x0508, 0x0517, 0x0526, 0x0535, 0x0544, 0x0553, 0x0562, 0x0580,
    0x059e, 0x05bc, 0x05cb,
};
struct point16 CANNON_FORM_SIZES[12] DGROUP_AT(0x05f2) = {
    { .x = 0x0040, .y = 0x0034 }, { .x = 0x0049, .y = 0x003a },
    { .x = 0x0049, .y = 0x003a }, { .x = 0x0046, .y = 0x0039 },
    { .x = 0x0047, .y = 0x0037 }, { .x = 0x0046, .y = 0x0036 },
    { .x = 0x0035, .y = 0x003c }, { .x = 0x003d, .y = 0x003f },
    { .x = 0x00c2, .y = 0x0054 }, { .x = 0x00d2, .y = 0x005e },
    { .x = 0x00c7, .y = 0x0054 }, { .x = 0x0040, .y = 0x0034 },
};
struct point8 CANNON_HOT_SPOTS[12] DGROUP_AT(0x0622) = {
    { 0 }, { .x = 0xf7, .y = 0xfa }, { .x = 0xf7, .y = 0xfa },
    { .x = 0xfa, .y = 0xfb }, { .x = 0xf9, .y = 0xfd },
    { .x = 0xfa, .y = 0xfe }, { .x = 0xf9, .y = 0xf8 },
    { .x = 0xf8, .y = 0xf5 }, { .x = 0xfe, .y = 0xdf },
    { .x = 0xfe, .y = 0xd2 }, { .x = 0xfd, .y = 0xdf },
};
struct draw_step DYNAMITE_DRAW_STEPS[6] DGROUP_AT(0x063a) = {
    [0] = { .level = 0x03, .frame = { 0x00, 0xff, 0xff, 0xff } },
    [1] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0xff, 0xff },
        .offset = { [1] = { .x = 0x28, .y = 0x09 } },
    },
    [2] = {
        .level = 0x03,
        .frame = { 0x00, 0x02, 0xff, 0xff },
        .offset = { [1] = { .x = 0x27, .y = 0x0a } },
    },
    [3] = {
        .level = 0x03,
        .frame = { 0x00, 0x03, 0xff, 0xff },
        .offset = { [1] = { .x = 0x27, .y = 0x09 } },
    },
    [4] = {
        .level = 0x03,
        .frame = { 0x00, 0x04, 0xff, 0xff },
        .offset = { [1] = { .x = 0x25, .y = 0x0d } },
    },
    [5] = {
        .level = 0x03,
        .frame = { 0x00, 0x05, 0xff, 0xff },
        .offset = { [1] = { .x = 0x26, .y = 0x0c } },
    },
};
dg_near_t DYNAMITE_FORM_STEPS[6] DGROUP_AT(0x0694) = { 0x063a, 0x0649, 0x0658, 0x0667, 0x0676, 0x0685 };
struct point16 DYNAMITE_FORM_SIZES[6] DGROUP_AT(0x06a0) = {
    { .x = 0x0030, .y = 0x001c }, { .x = 0x0038, .y = 0x001c },
    { .x = 0x0038, .y = 0x001c }, { .x = 0x0038, .y = 0x001c },
    { .x = 0x0038, .y = 0x001c }, { .x = 0x0038, .y = 0x001c },
};
struct point8 DYNAMITE_HOT_SPOTS[6] DGROUP_AT(0x06b8);
struct draw_step ELECTRIC_PLUG_DRAW_STEPS[8] DGROUP_AT(0x06c4) = {
    [0] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0xff, 0xff },
        .offset = { [1] = { .x = 0x08, .y = 0x08 } },
    },
    [1] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0x03, 0xff },
        .offset = { { 0 }, { .x = 0x08, .y = 0x08 }, { .x = 0x1d, .y = 0x04 } },
    },
    [2] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0x03, 0xff },
        .offset = { { 0 }, { .x = 0x08, .y = 0x08 }, { .x = 0x1d, .y = 0x12 } },
    },
    [3] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0x03, 0x03 },
        .offset = {
            { 0 }, { .x = 0x08, .y = 0x08 }, { .x = 0x1d, .y = 0x04 },
            { .x = 0x1d, .y = 0x12 },
        },
    },
    [4] = {
        .level = 0x05,
        .frame = { 0x00, 0x02, 0xff, 0xff },
        .offset = { [1] = { .x = 0x08, .y = 0x08 } },
    },
    [5] = {
        .level = 0x05,
        .frame = { 0x00, 0x02, 0x03, 0xff },
        .offset = { { 0 }, { .x = 0x08, .y = 0x08 }, { .x = 0x1d, .y = 0x04 } },
    },
    [6] = {
        .level = 0x05,
        .frame = { 0x00, 0x02, 0x03, 0xff },
        .offset = { { 0 }, { .x = 0x08, .y = 0x08 }, { .x = 0x1d, .y = 0x12 } },
    },
    [7] = {
        .level = 0x05,
        .frame = { 0x00, 0x02, 0x03, 0x03 },
        .offset = {
            { 0 }, { .x = 0x08, .y = 0x08 }, { .x = 0x1d, .y = 0x04 },
            { .x = 0x1d, .y = 0x12 },
        },
    },
};
dg_near_t ELECTRIC_PLUG_FORM_STEPS[8] DGROUP_AT(0x073c) = { 0x06c4, 0x06d3, 0x06e2, 0x06f1, 0x0700, 0x070f, 0x071e, 0x072d };
struct point16 ELECTRIC_PLUG_FORM_SIZES[8] DGROUP_AT(0x074c) = {
    { .x = 0x0030, .y = 0x0020 }, { .x = 0x0030, .y = 0x0020 },
    { .x = 0x0030, .y = 0x0020 }, { .x = 0x0030, .y = 0x0020 },
    { .x = 0x0030, .y = 0x0020 }, { .x = 0x0030, .y = 0x0020 },
    { .x = 0x0030, .y = 0x0020 }, { .x = 0x0030, .y = 0x0020 },
};
struct draw_step DYNAMITE_PLUNGER_DRAW_STEPS[3] DGROUP_AT(0x076c) = {
    [0] = {
        .level = 0x04,
        .frame = { 0x00, 0x02, 0x01, 0xff },
        .offset = { { .y = 0x13 }, { .x = 0x67 }, { .x = 0x28, .y = 0x10 } },
    },
    [1] = {
        .level = 0x04,
        .frame = { 0x00, 0x02, 0x01, 0xff },
        .offset = {
            { .y = 0x13 }, { .x = 0x67, .y = 0x05 }, { .x = 0x28, .y = 0x10 },
        },
    },
    [2] = {
        .level = 0x04,
        .frame = { 0x02, 0x01, 0xff, 0xff },
        .offset = { { .x = 0x67, .y = 0x0a }, { .x = 0x28, .y = 0x10 } },
    },
};
dg_near_t DYNAMITE_PLUNGER_FORM_STEPS[3] DGROUP_AT(0x0799) = { 0x076c, 0x077b, 0x078a };
struct point16 DYNAMITE_PLUNGER_FORM_SIZES[3] DGROUP_AT(0x079f) = {
    { .x = 0x0087, .y = 0x0030 }, { .x = 0x0087, .y = 0x002e },
    { .x = 0x0087, .y = 0x0029 },
};
struct point8 DYNAMITE_PLUNGER_HOT_SPOTS[3] DGROUP_AT(0x07ab);
struct draw_step FAN_DRAW_STEPS[4] DGROUP_AT(0x07b1) = {
    [0] = {
        .level = 0x04,
        .frame = { 0x00, 0x01, 0xff, 0xff },
        .offset = { { .y = 0x08 }, { .x = 0x10 } },
    },
    [1] = {
        .level = 0x04,
        .frame = { 0x00, 0x02, 0xff, 0xff },
        .offset = { { .y = 0x08 }, { .x = 0x10 } },
    },
    [2] = {
        .level = 0x04,
        .frame = { 0x00, 0x03, 0xff, 0xff },
        .offset = { { .y = 0x08 }, { .x = 0x10 } },
    },
    [3] = {
        .level = 0x04,
        .frame = { 0x00, 0x04, 0xff, 0xff },
        .offset = { { .y = 0x08 }, { .x = 0x10 } },
    },
};
dg_near_t FAN_FORM_STEPS[4] DGROUP_AT(0x07ed) = { 0x07b1, 0x07c0, 0x07cf, 0x07de };
struct point16 FAN_FORM_SIZES[4] DGROUP_AT(0x07f5) = {
    { .x = 0x0020, .y = 0x0020 }, { .x = 0x0020, .y = 0x0020 },
    { .x = 0x0020, .y = 0x0020 }, { .x = 0x0020, .y = 0x0020 },
};
struct draw_step GENERATOR_DRAW_STEPS[16] DGROUP_AT(0x0805) = {
    [0] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0xff, 0xff },
        .offset = { [1] = { .x = 0x15 } },
    },
    [1] = {
        .level = 0x05,
        .frame = { 0x00, 0x02, 0xff, 0xff },
        .offset = { [1] = { .x = 0x15, .y = 0xfa } },
    },
    [2] = {
        .level = 0x05,
        .frame = { 0x00, 0x03, 0xff, 0xff },
        .offset = { [1] = { .x = 0x15, .y = 0x01 } },
    },
    [3] = {
        .level = 0x05,
        .frame = { 0x00, 0x04, 0xff, 0xff },
        .offset = { [1] = { .x = 0x15, .y = 0xfb } },
    },
    [4] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0x05, 0xff },
        .offset = { { 0 }, { .x = 0x15 }, { .x = 0x05, .y = 0x04 } },
    },
    [5] = {
        .level = 0x05,
        .frame = { 0x00, 0x02, 0x05, 0xff },
        .offset = { { 0 }, { .x = 0x15, .y = 0xfa }, { .x = 0x05, .y = 0x04 } },
    },
    [6] = {
        .level = 0x05,
        .frame = { 0x00, 0x03, 0x05, 0xff },
        .offset = { { 0 }, { .x = 0x15, .y = 0x01 }, { .x = 0x05, .y = 0x04 } },
    },
    [7] = {
        .level = 0x05,
        .frame = { 0x00, 0x04, 0x05, 0xff },
        .offset = { { 0 }, { .x = 0x15, .y = 0xfb }, { .x = 0x05, .y = 0x04 } },
    },
    [8] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0x05, 0xff },
        .offset = { { 0 }, { .x = 0x15 }, { .x = 0x05, .y = 0x12 } },
    },
    [9] = {
        .level = 0x05,
        .frame = { 0x00, 0x02, 0x05, 0xff },
        .offset = { { 0 }, { .x = 0x15, .y = 0xfa }, { .x = 0x05, .y = 0x12 } },
    },
    [10] = {
        .level = 0x05,
        .frame = { 0x00, 0x03, 0x05, 0xff },
        .offset = { { 0 }, { .x = 0x15, .y = 0x01 }, { .x = 0x05, .y = 0x12 } },
    },
    [11] = {
        .level = 0x05,
        .frame = { 0x00, 0x04, 0x05, 0xff },
        .offset = { { 0 }, { .x = 0x15, .y = 0xfb }, { .x = 0x05, .y = 0x12 } },
    },
    [12] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0x05, 0x05 },
        .offset = {
            { 0 }, { .x = 0x15 }, { .x = 0x05, .y = 0x04 },
            { .x = 0x05, .y = 0x12 },
        },
    },
    [13] = {
        .level = 0x05,
        .frame = { 0x00, 0x02, 0x05, 0x05 },
        .offset = {
            { 0 }, { .x = 0x15, .y = 0xfa }, { .x = 0x05, .y = 0x04 },
            { .x = 0x05, .y = 0x12 },
        },
    },
    [14] = {
        .level = 0x05,
        .frame = { 0x00, 0x03, 0x05, 0x05 },
        .offset = {
            { 0 }, { .x = 0x15, .y = 0x01 }, { .x = 0x05, .y = 0x04 },
            { .x = 0x05, .y = 0x12 },
        },
    },
    [15] = {
        .level = 0x05,
        .frame = { 0x00, 0x04, 0x05, 0x05 },
        .offset = {
            { 0 }, { .x = 0x15, .y = 0xfb }, { .x = 0x05, .y = 0x04 },
            { .x = 0x05, .y = 0x12 },
        },
    },
};
dg_near_t GENERATOR_FORM_STEPS[16] DGROUP_AT(0x08f5) = {
    0x0805, 0x0814, 0x0823, 0x0832, 0x0841, 0x0850, 0x085f, 0x086e, 0x087d,
    0x088c, 0x089b, 0x08aa, 0x08b9, 0x08c8, 0x08d7, 0x08e6,
};
struct point16 GENERATOR_FORM_SIZES[16] DGROUP_AT(0x0915) = {
    { .x = 0x0050, .y = 0x0020 }, { .x = 0x0050, .y = 0x0026 },
    { .x = 0x0050, .y = 0x0020 }, { .x = 0x0050, .y = 0x0025 },
    { .x = 0x0050, .y = 0x0020 }, { .x = 0x0050, .y = 0x0026 },
    { .x = 0x0050, .y = 0x0020 }, { .x = 0x0050, .y = 0x0025 },
    { .x = 0x0050, .y = 0x0020 }, { .x = 0x0050, .y = 0x0026 },
    { .x = 0x0050, .y = 0x0020 }, { .x = 0x0050, .y = 0x0025 },
    { .x = 0x0050, .y = 0x0020 }, { .x = 0x0050, .y = 0x0026 },
    { .x = 0x0050, .y = 0x0020 }, { .x = 0x0050, .y = 0x0025 },
};
struct point8 GENERATOR_HOT_SPOTS[16] DGROUP_AT(0x0955) = {
    [1] = { .y = 0xfa },
    [3] = { .y = 0xfb },
    [5] = { .y = 0xfa },
    [7] = { .y = 0xfb },
    [9] = { .y = 0xfa },
    [11] = { .y = 0xfb },
    [13] = { .y = 0xfa },
    [15] = { .y = 0xfb },
};
struct draw_step GUN_DRAW_STEPS[7] DGROUP_AT(0x0975) = {
    [0] = { .level = 0x04, .frame = { 0x00, 0xff, 0xff, 0xff } },
    [1] = {
        .level = 0x04,
        .frame = { 0x01, 0xff, 0xff, 0xff },
        .offset = { { .x = 0xff, .y = 0xfb } },
    },
    [2] = {
        .level = 0x04,
        .frame = { 0x02, 0xff, 0xff, 0xff },
        .offset = { { .x = 0xfe, .y = 0xfd } },
    },
    [3] = {
        .level = 0x04,
        .frame = { 0x03, 0xff, 0xff, 0xff },
        .offset = { { .x = 0xf1, .y = 0xfa } },
    },
    [4] = {
        .level = 0x04,
        .frame = { 0x04, 0xff, 0xff, 0xff },
        .offset = { { .x = 0xf5, .y = 0xfd } },
    },
    [5] = {
        .level = 0x04,
        .frame = { 0x00, 0x05, 0xff, 0xff },
        .offset = { { .x = 0xfe }, { .x = 0x40, .y = 0xf4 } },
    },
    [6] = { .level = 0x04, .frame = { 0x00, 0xff, 0xff, 0xff } },
};
dg_near_t GUN_FORM_STEPS[7] DGROUP_AT(0x09de) = { 0x0975, 0x0984, 0x0993, 0x09a2, 0x09b1, 0x09c0, 0x09cf };
struct point16 GUN_FORM_SIZES[7] DGROUP_AT(0x09ec) = {
    { .x = 0x0040, .y = 0x001f }, { .x = 0x0038, .y = 0x0024 },
    { .x = 0x0080, .y = 0x0025 }, { .x = 0x0070, .y = 0x0022 },
    { .x = 0x0080, .y = 0x0022 }, { .x = 0x0080, .y = 0x002b },
    { .x = 0x0040, .y = 0x001f },
};
struct point8 GUN_HOT_SPOTS[7] DGROUP_AT(0x0a08) = {
    { 0 }, { .x = 0xff, .y = 0xfb }, { .x = 0xfe, .y = 0xfd },
    { .x = 0xf1, .y = 0xfa }, { .x = 0xf5, .y = 0xfd },
    { .x = 0xfe, .y = 0xf4 },
};
struct draw_step LIGHT_DRAW_STEPS[4] DGROUP_AT(0x0a16) = {
    [0] = {
        .level = 0x02,
        .frame = { 0x00, 0x04, 0xff, 0xff },
        .offset = { [1] = { .x = 0x14, .y = 0x1c } },
    },
    [1] = {
        .level = 0x02,
        .frame = { 0x01, 0x05, 0xff, 0xff },
        .offset = { { .x = 0xf8, .y = 0xee }, { .x = 0x14, .y = 0x1c } },
    },
    [2] = {
        .level = 0x02,
        .frame = { 0x02, 0x04, 0xff, 0xff },
        .offset = { [1] = { .x = 0x13, .y = 0x02 } },
    },
    [3] = {
        .level = 0x02,
        .frame = { 0x03, 0x05, 0xff, 0xff },
        .offset = { { .x = 0xf8 }, { .x = 0x13, .y = 0x02 } },
    },
};
dg_near_t LIGHT_FORM_STEPS[4] DGROUP_AT(0x0a52) = { 0x0a16, 0x0a25, 0x0a34, 0x0a43 };
struct point16 LIGHT_FORM_SIZES[4] DGROUP_AT(0x0a5a) = {
    { .x = 0x0020, .y = 0x0036 }, { .x = 0x002f, .y = 0x0048 },
    { .x = 0x0020, .y = 0x0026 }, { .x = 0x002f, .y = 0x0032 },
};
struct point8 LIGHT_HOT_SPOTS[4] DGROUP_AT(0x0a6a) = { [1] = { .x = 0xf8, .y = 0xee }, [3] = { .x = 0xf8 } };
struct draw_step MONKEY_DRAW_STEPS[13] DGROUP_AT(0x0a72) = {
    [0] = {
        .level = 0x04,
        .frame = { 0x00, 0x04, 0xff, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28 } },
    },
    [1] = {
        .level = 0x04,
        .frame = { 0x00, 0x05, 0xff, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28, .y = 0x06 } },
    },
    [2] = {
        .level = 0x04,
        .frame = { 0x01, 0x05, 0xff, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28, .y = 0x06 } },
    },
    [3] = {
        .level = 0x04,
        .frame = { 0x02, 0x05, 0xff, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28, .y = 0x06 } },
    },
    [4] = {
        .level = 0x04,
        .frame = { 0x03, 0x05, 0xff, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28, .y = 0x06 } },
    },
    [5] = {
        .level = 0x04,
        .frame = { 0x00, 0x04, 0x06, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28 }, { .x = 0x0e, .y = 0x01 } },
    },
    [6] = {
        .level = 0x04,
        .frame = { 0x00, 0x04, 0x07, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28 }, { .x = 0x10, .y = 0x01 } },
    },
    [7] = {
        .level = 0x04,
        .frame = { 0x00, 0x04, 0x08, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28 }, { .x = 0x10, .y = 0x01 } },
    },
    [8] = {
        .level = 0x04,
        .frame = { 0x00, 0x04, 0x09, 0xff },
        .offset = { { .y = 0x0c }, { .x = 0x28 }, { .x = 0x0f, .y = 0x01 } },
    },
    [9] = {
        .level = 0x04,
        .frame = { 0x00, 0x05, 0x06, 0xff },
        .offset = {
            { .y = 0x0c }, { .x = 0x28, .y = 0x06 }, { .x = 0x0e, .y = 0x01 },
        },
    },
    [10] = {
        .level = 0x04,
        .frame = { 0x00, 0x05, 0x07, 0xff },
        .offset = {
            { .y = 0x0c }, { .x = 0x28, .y = 0x06 }, { .x = 0x10, .y = 0x01 },
        },
    },
    [11] = {
        .level = 0x04,
        .frame = { 0x00, 0x05, 0x08, 0xff },
        .offset = {
            { .y = 0x0c }, { .x = 0x28, .y = 0x06 }, { .x = 0x10, .y = 0x01 },
        },
    },
    [12] = {
        .level = 0x04,
        .frame = { 0x00, 0x05, 0x09, 0xff },
        .offset = {
            { .y = 0x0c }, { .x = 0x28, .y = 0x06 }, { .x = 0x0e, .y = 0x01 },
        },
    },
};
dg_near_t MONKEY_FORM_STEPS[13] DGROUP_AT(0x0b35) = {
    0x0a72, 0x0a81, 0x0a90, 0x0a9f, 0x0aae, 0x0abd, 0x0acc, 0x0adb, 0x0aea,
    0x0af9, 0x0b08, 0x0b17, 0x0b26,
};
struct point16 MONKEY_FORM_SIZES[13] DGROUP_AT(0x0b4f) = {
    { .x = 0x005c, .y = 0x004f }, { .x = 0x005c, .y = 0x004f },
    { .x = 0x005c, .y = 0x004f }, { .x = 0x005c, .y = 0x004f },
    { .x = 0x005c, .y = 0x004f }, { .x = 0x005c, .y = 0x004f },
    { .x = 0x005c, .y = 0x004f }, { .x = 0x005c, .y = 0x004f },
    { .x = 0x005c, .y = 0x004f }, { .x = 0x005c, .y = 0x004f },
    { .x = 0x005c, .y = 0x004f }, { .x = 0x005c, .y = 0x004f },
    { .x = 0x005c, .y = 0x004f },
};
struct draw_step ROCKET_DRAW_STEPS[10] DGROUP_AT(0x0b83) = {
    [0] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0xff, 0xff },
        .offset = { [1] = { .x = 0x03, .y = 0x2d } },
    },
    [1] = {
        .level = 0x03,
        .frame = { 0x00, 0x02, 0xff, 0xff },
        .offset = { [1] = { .x = 0x02, .y = 0x2d } },
    },
    [2] = {
        .level = 0x03,
        .frame = { 0x00, 0x03, 0xff, 0xff },
        .offset = { [1] = { .x = 0x02, .y = 0x2d } },
    },
    [3] = {
        .level = 0x03,
        .frame = { 0x00, 0x04, 0xff, 0xff },
        .offset = { [1] = { .y = 0x2d } },
    },
    [4] = {
        .level = 0x03,
        .frame = { 0x00, 0x05, 0xff, 0xff },
        .offset = { [1] = { .x = 0xfe, .y = 0x2d } },
    },
    [5] = {
        .level = 0x03,
        .frame = { 0x00, 0x06, 0xff, 0xff },
        .offset = { [1] = { .x = 0xfe, .y = 0x2d } },
    },
    [6] = {
        .level = 0x03,
        .frame = { 0x00, 0x07, 0xff, 0xff },
        .offset = { [1] = { .y = 0x2c } },
    },
    [7] = {
        .level = 0x03,
        .frame = { 0x00, 0x08, 0xff, 0xff },
        .offset = { [1] = { .x = 0x01, .y = 0x2e } },
    },
    [8] = {
        .level = 0x03,
        .frame = { 0x00, 0x09, 0xff, 0xff },
        .offset = { [1] = { .y = 0x2e } },
    },
    [9] = {
        .level = 0x03,
        .frame = { 0x00, 0x0a, 0xff, 0xff },
        .offset = { [1] = { .y = 0x2e } },
    },
};
dg_near_t ROCKET_FORM_STEPS[10] DGROUP_AT(0x0c19) = {
    0x0b83, 0x0b92, 0x0ba1, 0x0bb0, 0x0bbf, 0x0bce, 0x0bdd, 0x0bec, 0x0bfb,
    0x0c0a,
};
struct point16 ROCKET_FORM_SIZES[10] DGROUP_AT(0x0c2d) = {
    { .x = 0x0010, .y = 0x0042 }, { .x = 0x0010, .y = 0x004a },
    { .x = 0x0010, .y = 0x0046 }, { .x = 0x0010, .y = 0x0041 },
    { .x = 0x0010, .y = 0x003b }, { .x = 0x0010, .y = 0x0038 },
    { .x = 0x0010, .y = 0x0042 }, { .x = 0x0010, .y = 0x0051 },
    { .x = 0x0010, .y = 0x0053 }, { .x = 0x0010, .y = 0x0052 },
};
struct point8 ROCKET_HOT_SPOTS[10] DGROUP_AT(0x0c55) = { [4] = { .x = 0xfe }, [5] = { .x = 0xfe } };
struct draw_step SCISSORS_DRAW_STEPS[3] DGROUP_AT(0x0c69) = {
    [0] = {
        .level = 0x04,
        .frame = { 0x01, 0xff, 0xff, 0xff },
        .offset = { { .x = 0x15, .y = 0x11 } },
    },
    [1] = { .next = 0x0c69, .frame = { 0x00, 0xff, 0xff, 0xff } },
    [2] = {
        .level = 0x04,
        .frame = { 0x02, 0xff, 0xff, 0xff },
        .offset = { { .x = 0xfe, .y = 0x04 } },
    },
};
dg_near_t SCISSORS_FORM_STEPS[2] DGROUP_AT(0x0c96) = { 0x0c78, 0x0c87 };
struct point16 SCISSORS_FORM_SIZES[2] DGROUP_AT(0x0c9a) = { { .x = 0x0028, .y = 0x0022 }, { .x = 0x0030, .y = 0x0018 } };
struct point8 SCISSORS_HOT_SPOTS[2] DGROUP_AT(0x0ca2) = { [1] = { .x = 0xfe } };
struct draw_step SOLAR_PANEL_DRAW_STEPS[4] DGROUP_AT(0x0ca6) = {
    [0] = { .level = 0x05, .frame = { 0x00, 0xff, 0xff, 0xff } },
    [1] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0xff, 0xff },
        .offset = { [1] = { .x = 0x34, .y = 0x04 } },
    },
    [2] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0xff, 0xff },
        .offset = { [1] = { .x = 0x34, .y = 0x12 } },
    },
    [3] = {
        .level = 0x05,
        .frame = { 0x00, 0x01, 0x01, 0xff },
        .offset = { { 0 }, { .x = 0x34, .y = 0x04 }, { .x = 0x34, .y = 0x12 } },
    },
};
dg_near_t SOLAR_PANEL_FORM_STEPS[4] DGROUP_AT(0x0ce2) = { 0x0ca6, 0x0cb5, 0x0cc4, 0x0cd3 };
struct point16 SOLAR_PANEL_FORM_SIZES[4] DGROUP_AT(0x0cea) = {
    { .x = 0x0048, .y = 0x0020 }, { .x = 0x0048, .y = 0x0020 },
    { .x = 0x0048, .y = 0x0020 }, { .x = 0x0048, .y = 0x0020 },
};
struct draw_step TRAMPOLINE_DRAW_STEPS[10] DGROUP_AT(0x0cfa) = {
    [0] = {
        .frame = { 0x01, 0xff, 0xff, 0xff },
        .offset = { { .y = 0x09 } },
    },
    [1] = {
        .next = 0x0cfa,
        .level = 0x04,
        .frame = { 0x00, 0xff, 0xff, 0xff },
    },
    [2] = {
        .frame = { 0x03, 0xff, 0xff, 0xff },
        .offset = { { .y = 0x09 } },
    },
    [3] = {
        .next = 0x0d18,
        .level = 0x04,
        .frame = { 0x02, 0xff, 0xff, 0xff },
    },
    [4] = {
        .frame = { 0x05, 0xff, 0xff, 0xff },
        .offset = { { .y = 0x09 } },
    },
    [5] = {
        .next = 0x0d36,
        .level = 0x04,
        .frame = { 0x04, 0xff, 0xff, 0xff },
    },
    [6] = {
        .frame = { 0x01, 0xff, 0xff, 0xff },
        .offset = { { .y = 0x09 } },
    },
    [7] = {
        .next = 0x0d54,
        .level = 0x04,
        .frame = { 0x06, 0xff, 0xff, 0xff },
    },
    [8] = {
        .frame = { 0x01, 0xff, 0xff, 0xff },
        .offset = { { .y = 0x09 } },
    },
    [9] = {
        .next = 0x0d72,
        .level = 0x04,
        .frame = { 0x07, 0xff, 0xff, 0xff },
        .offset = { { .y = 0xfd } },
    },
};
dg_near_t TRAMPOLINE_FORM_STEPS[5] DGROUP_AT(0x0d90) = { 0x0d09, 0x0d27, 0x0d45, 0x0d63, 0x0d81 };
struct point16 TRAMPOLINE_FORM_SIZES[5] DGROUP_AT(0x0d9a) = {
    { .x = 0x0030, .y = 0x001c }, { .x = 0x0030, .y = 0x001c },
    { .x = 0x0030, .y = 0x001c }, { .x = 0x0030, .y = 0x001c },
    { .x = 0x0030, .y = 0x001f },
};
struct point8 TRAMPOLINE_HOT_SPOTS[5] DGROUP_AT(0x0dae) = { [4] = { .y = 0xfd } };
struct draw_step CANDLE_DRAW_STEPS[6] DGROUP_AT(0x0db8) = {
    [0] = { .level = 0x03, .frame = { 0x00, 0xff, 0xff, 0xff } },
    [1] = {
        .level = 0x03,
        .frame = { 0x00, 0x01, 0xff, 0xff },
        .offset = { [1] = { .x = 0x0b, .y = 0xfc } },
    },
    [2] = {
        .level = 0x03,
        .frame = { 0x00, 0x02, 0xff, 0xff },
        .offset = { [1] = { .x = 0x0b, .y = 0xfc } },
    },
    [3] = {
        .level = 0x03,
        .frame = { 0x00, 0x03, 0xff, 0xff },
        .offset = { [1] = { .x = 0x0b, .y = 0xfc } },
    },
    [4] = {
        .level = 0x03,
        .frame = { 0x00, 0x04, 0xff, 0xff },
        .offset = { [1] = { .x = 0x0b, .y = 0xfc } },
    },
    [5] = {
        .level = 0x03,
        .frame = { 0x00, 0x05, 0xff, 0xff },
        .offset = { [1] = { .x = 0x0b, .y = 0xfc } },
    },
};
dg_near_t CANDLE_FORM_STEPS[6] DGROUP_AT(0x0e12) = { 0x0db8, 0x0dc7, 0x0dd6, 0x0de5, 0x0df4, 0x0e03 };
struct point16 CANDLE_FORM_SIZES[6] DGROUP_AT(0x0e1e) = {
    { .x = 0x0022, .y = 0x0020 }, { .x = 0x0022, .y = 0x0024 },
    { .x = 0x0022, .y = 0x0024 }, { .x = 0x0022, .y = 0x0024 },
    { .x = 0x0022, .y = 0x0024 }, { .x = 0x0022, .y = 0x0024 },
};
struct point8 CANDLE_HOT_SPOTS[6] DGROUP_AT(0x0e36) = {
    { 0 }, { .y = 0xfc }, { .y = 0xfc }, { .y = 0xfc }, { .y = 0xfc },
    { .y = 0xfc },
};
struct point8 SEESAW_HOT_SPOTS[3] DGROUP_AT(0x0e42) = { [1] = { .y = 0x0c } };
struct point8 BALLOON_HOT_SPOTS[7] DGROUP_AT(0x0e48) = {
    { 0 }, { .x = 0xf1, .y = 0xf7 }, { .x = 0xec, .y = 0xfb },
    { .x = 0xe4, .y = 0x08 }, { .x = 0xe2, .y = 0x19 },
    { .x = 0xe6, .y = 0x29 }, { .x = 0xe6, .y = 0x38 },
};
struct point8 POKEY_HOT_SPOTS[10] DGROUP_AT(0x0e56) = {
    { 0 }, { .x = 0xfa, .y = 0xf0 }, { .x = 0x13, .y = 0xff }, { .x = 0x0e },
    { .x = 0x0a }, { .x = 0x08 }, { .x = 0x06, .y = 0xfe },
    { .x = 0xff, .y = 0xfe }, { .x = 0xfb, .y = 0xfe },
    { .x = 0xf7, .y = 0xfd },
};
struct point8 BELLOW_HOT_SPOTS[3] DGROUP_AT(0x0e6a) = { { 0 }, { .x = 0xf8, .y = 0x08 }, { .x = 0xf5, .y = 0x0c } };
struct point8 BULLET_HOT_SPOTS[3] DGROUP_AT(0x0e70) = { { 0 }, { .x = 0x04, .y = 0xfd }, { .x = 0x01, .y = 0xf4 } };
struct point8 FLASHLIGHT_HOT_SPOTS[2] DGROUP_AT(0x0e76) = { [1] = { .y = 0xf6 } };
struct point8 BOXING_GLOVE_HOT_SPOTS[10] DGROUP_AT(0x0e7a) = {
    { 0 }, { .x = 0x07, .y = 0xf4 }, { .x = 0xe3, .y = 0xfd },
    { .x = 0xac, .y = 0xfa }, { .x = 0xf1, .y = 0xfd },
    { .x = 0xe3, .y = 0xfd }, { .x = 0xe2, .y = 0x05 },
    { .x = 0xeb, .y = 0x05 }, { .x = 0xe7, .y = 0x06 },
    { .x = 0xeb, .y = 0x06 },
};
struct point8 WINDMILL_HOT_SPOTS[4] DGROUP_AT(0x0e8e) = {
    { 0 }, { .x = 0xfd, .y = 0xfd }, { .x = 0xfc, .y = 0xfc },
    { .x = 0xfd, .y = 0xfd },
};
struct point8 BLAST_HOT_SPOTS[6] DGROUP_AT(0x0e96) = {
    { 0 }, { .x = 0xfc, .y = 0xf3 }, { .y = 0xfa }, { .x = 0x0d, .y = 0x09 },
    { .x = 0x14, .y = 0x13 }, { .x = 0x14, .y = 0x14 },
};
struct point8 MORT_THE_MOUSE_HOT_SPOTS[2] DGROUP_AT(0x0ea2) = { [1] = { .y = 0x01 } };


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
        .density = 0x0b10, .weight = 0x00c8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .priority = 0x0000,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x0001, LOAD_SEG + 0x172c },    /* part_setup_0001 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BRICK_PLATFORM] = {    /* DGROUP 0x0ee0 */
        .density = 0x1039, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0018,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0010, .min_h = 0x0010,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .priority = 0x0028,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x48ab, LOAD_SEG + 0x172c },    /* part_setup_48ab */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x48f7, LOAD_SEG + 0x172c },    /* part_settle_48f7 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_RAMP] = {    /* DGROUP 0x0f1a */
        .density = 0x05e6, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0040, .max_h = 0x0000, .min_w = 0x0010, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .priority = 0x002c,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x2728, LOAD_SEG + 0x172c },    /* part_setup_ramp */
        .flip   = { 0x27b6, LOAD_SEG + 0x172c },    /* part_flip_ramp */
        .settle = { 0x2789, LOAD_SEG + 0x172c },    /* part_settle_ramp */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_SEESAW] = {    /* DGROUP 0x0f54 */
        .density = 0x0760, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e42, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0008, .priority = 0x0006,
        .hit    = { 0x3fe8, LOAD_SEG + 0x172c },    /* part_hit_seesaw */
        .step   = { 0x420f, LOAD_SEG + 0x172c },    /* part_step_seesaw */
        .setup  = { 0x40f0, LOAD_SEG + 0x172c },    /* part_setup_seesaw */
        .flip   = { 0x41bb, LOAD_SEG + 0x172c },    /* part_flip_seesaw */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x44fe, LOAD_SEG + 0x172c },    /* part_drive_44fe */
    },
    [KIND_BALLOON] = {    /* DGROUP 0x0f8e */
        .density = 0x0009, .weight = 0x0001, .bounce = 0x0040, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e48, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .priority = 0x0005,
        .hit    = { 0x016e, LOAD_SEG + 0x172c },    /* part_hit_balloon */
        .step   = { 0x018e, LOAD_SEG + 0x172c },    /* part_step_balloon */
        .setup  = { 0x012d, LOAD_SEG + 0x172c },    /* part_setup_balloon */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02cd, LOAD_SEG + 0x172c },    /* part_drive_02cd */
    },
    [KIND_CONVEYOR] = {    /* DGROUP 0x0fc8 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0060, .max_h = 0x0000, .min_w = 0x0020, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .priority = 0x000c,
        .hit    = { 0x2514, LOAD_SEG + 0x172c },    /* part_hit_conveyor */
        .step   = { 0x2592, LOAD_SEG + 0x172c },    /* part_step_conveyor */
        .setup  = { 0x24d0, LOAD_SEG + 0x172c },    /* part_setup_conveyor */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x261d, LOAD_SEG + 0x172c },    /* part_settle_conveyor */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_MOUSE_CAGE] = {    /* DGROUP 0x1002 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .priority = 0x0025,
        .hit    = { 0x2f25, LOAD_SEG + 0x172c },    /* part_hit_mouse_cage */
        .step   = { 0x2f3e, LOAD_SEG + 0x172c },    /* part_step_mouse_cage */
        .setup  = { 0x2ee1, LOAD_SEG + 0x172c },    /* part_setup_mouse_cage */
        .flip   = { 0x2fba, LOAD_SEG + 0x172c },    /* part_flip_mouse_cage */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_PULLEY] = {    /* DGROUP 0x103c */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0000, .grip = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0000, .priority = 0x0011,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BELT] = {    /* DGROUP 0x1076 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0000, .grip = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x01, 0xff }, .point_count = 0x0000, .priority = 0x000a,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BASKETBALL] = {    /* DGROUP 0x10b0 */
        .density = 0x052a, .weight = 0x0014, .bounce = 0x00c0, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .priority = 0x0001,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x0001, LOAD_SEG + 0x172c },    /* part_setup_0001 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_ROPE] = {    /* DGROUP 0x10ea */
        .density = 0x0640, .weight = 0x03e8, .bounce = 0x0000, .grip = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x01, 0xff }, .point_count = 0x0000, .priority = 0x000f,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BIRD_CAGE] = {    /* DGROUP 0x1124 */
        .density = 0x1d80, .weight = 0x0096, .bounce = 0x0020, .grip = 0x0040,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x000c, .priority = 0x0022,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x0f70, LOAD_SEG + 0x172c },    /* part_setup_bird_cage */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x0ffc, LOAD_SEG + 0x172c },    /* part_drive_0ffc */
    },
    [KIND_POKEY] = {    /* DGROUP 0x115e */
        .density = 0x07d0, .weight = 0x0078, .bounce = 0x0000, .grip = 0x0040,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e56, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0005, .priority = 0x0023,
        .hit    = { 0x0c6c, LOAD_SEG + 0x172c },    /* part_hit_pokey */
        .step   = { 0x0ca3, LOAD_SEG + 0x172c },    /* part_step_pokey */
        .setup  = { 0x0c1c, LOAD_SEG + 0x172c },    /* part_setup_pokey */
        .flip   = { 0x0f3d, LOAD_SEG + 0x172c },    /* part_flip_pokey */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_JACK_IN_THE_BOX] = {    /* DGROUP 0x1198 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0250, .hotspots_ptr = 0x02c2, .sizes_ptr = 0x0276,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0004, .priority = 0x000d,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x27e2, LOAD_SEG + 0x172c },    /* part_step_jack_in_the_box */
        .setup  = { 0x295d, LOAD_SEG + 0x172c },    /* part_setup_jack_in_the_box */
        .flip   = { 0x2999, LOAD_SEG + 0x172c },    /* part_flip_jack_in_the_box */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_GEAR] = {    /* DGROUP 0x11d2 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0030,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0008, .priority = 0x000b,
        .hit    = { 0x1f78, LOAD_SEG + 0x172c },    /* part_hit_gear */
        .step   = { 0x20fc, LOAD_SEG + 0x172c },    /* part_step_gear */
        .setup  = { 0x2068, LOAD_SEG + 0x172c },    /* part_setup_gear */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BOB_THE_FISH] = {    /* DGROUP 0x120c */
        .density = 0x07d0, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0441, .hotspots_ptr = 0x04cb, .sizes_ptr = 0x046f,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .priority = 0x0026,
        .hit    = { 0x1c39, LOAD_SEG + 0x172c },    /* part_hit_bob_the_fish */
        .step   = { 0x1c5f, LOAD_SEG + 0x172c },    /* part_step_bob_the_fish */
        .setup  = { 0x1be9, LOAD_SEG + 0x172c },    /* part_setup_bob_the_fish */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BELLOW] = {    /* DGROUP 0x1246 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e6a, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0006, .priority = 0x0007,
        .hit    = { 0x0332, LOAD_SEG + 0x172c },    /* part_hit_bellow */
        .step   = { 0x0405, LOAD_SEG + 0x172c },    /* part_step_bellow */
        .setup  = { 0x0371, LOAD_SEG + 0x172c },    /* part_setup_bellow */
        .flip   = { 0x03d2, LOAD_SEG + 0x172c },    /* part_flip_bellow */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BUCKET] = {    /* DGROUP 0x1280 */
        .density = 0x1d80, .weight = 0x0064, .bounce = 0x0020, .grip = 0x0030,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0006, .priority = 0x0021,
        .hit    = { 0x0763, LOAD_SEG + 0x172c },    /* part_hit_bucket */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x07b2, LOAD_SEG + 0x172c },    /* part_setup_bucket */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x0802, LOAD_SEG + 0x172c },    /* part_drive_0802 */
    },
    [KIND_CANNON] = {    /* DGROUP 0x12ba */
        .density = 0x3986, .weight = 0x03e8, .bounce = 0x00c0, .grip = 0x000c,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x05da, .hotspots_ptr = 0x0622, .sizes_ptr = 0x05f2,
        .refile_level = { 0x04, 0x00 }, .point_count = 0x0008, .priority = 0x001c,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x0a5d, LOAD_SEG + 0x172c },    /* part_step_cannon */
        .setup  = { 0x0b88, LOAD_SEG + 0x172c },    /* part_setup_cannon */
        .flip   = { 0x0be9, LOAD_SEG + 0x172c },    /* part_flip_cannon */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_DYNAMITE] = {    /* DGROUP 0x12f4 */
        .density = 0x046c, .weight = 0x005a, .bounce = 0x0040, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0694, .hotspots_ptr = 0x06b8, .sizes_ptr = 0x06a0,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0005, .priority = 0x001d,
        .hit    = { 0x1237, LOAD_SEG + 0x172c },    /* part_hit_dynamite */
        .step   = { 0x12c2, LOAD_SEG + 0x172c },    /* part_step_dynamite */
        .setup  = { 0x1261, LOAD_SEG + 0x172c },    /* part_setup_dynamite */
        .flip   = { 0x12fc, LOAD_SEG + 0x172c },    /* part_flip_dynamite */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BULLET] = {    /* DGROUP 0x132e */
        .density = 0x0000, .weight = 0x4e20, .bounce = 0x0000, .grip = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e70, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0004, .priority = 0x0032,
        .hit    = { 0x0867, LOAD_SEG + 0x172c },    /* part_hit_0867 */
        .step   = { 0x08f1, LOAD_SEG + 0x172c },    /* part_step_08f1 */
        .setup  = { 0x08a1, LOAD_SEG + 0x172c },    /* part_setup_08a1 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_ELECTRIC_PLUG] = {    /* DGROUP 0x1368 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x073c, .hotspots_ptr = 0x0000, .sizes_ptr = 0x074c,
        .refile_level = { 0x05, 0xff }, .point_count = 0x0004, .priority = 0x0014,
        .hit    = { 0x14d3, LOAD_SEG + 0x172c },    /* part_hit_electric_plug */
        .step   = { 0x15ce, LOAD_SEG + 0x172c },    /* part_step_electric_plug */
        .setup  = { 0x1556, LOAD_SEG + 0x172c },    /* part_setup_electric_plug */
        .flip   = { 0x15fc, LOAD_SEG + 0x172c },    /* part_flip_electric_plug */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_DYNAMITE_PLUNGER] = {    /* DGROUP 0x13a2 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0799, .hotspots_ptr = 0x07ab, .sizes_ptr = 0x079f,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .priority = 0x0020,
        .hit    = { 0x323f, LOAD_SEG + 0x172c },    /* part_hit_dynamite_plunger */
        .step   = { 0x332a, LOAD_SEG + 0x172c },    /* part_step_dynamite_plunger */
        .setup  = { 0x3294, LOAD_SEG + 0x172c },    /* part_setup_dynamite_plunger */
        .flip   = { 0x33e5, LOAD_SEG + 0x172c },    /* part_flip_dynamite_plunger */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x341d, LOAD_SEG + 0x172c },    /* part_drive_341d */
    },
    [KIND_HOOK] = {    /* DGROUP 0x13dc */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0000, .priority = 0x0010,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x19db, LOAD_SEG + 0x172c },    /* part_setup_hook */
        .flip   = { 0x19fa, LOAD_SEG + 0x172c },    /* part_flip_hook */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_FAN] = {    /* DGROUP 0x1416 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x07ed, .hotspots_ptr = 0x0000, .sizes_ptr = 0x07f5,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0005, .priority = 0x0017,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x1a82, LOAD_SEG + 0x172c },    /* part_step_fan */
        .setup  = { 0x1a32, LOAD_SEG + 0x172c },    /* part_setup_fan */
        .flip   = { 0x1bbd, LOAD_SEG + 0x172c },    /* part_flip_fan */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_FLASHLIGHT] = {    /* DGROUP 0x1450 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e76, .sizes_ptr = 0x0000,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0006, .priority = 0x001a,
        .hit    = { 0x1d07, LOAD_SEG + 0x172c },    /* part_hit_flashlight */
        .step   = { 0x1d78, LOAD_SEG + 0x172c },    /* part_step_flashlight */
        .setup  = { 0x1d28, LOAD_SEG + 0x172c },    /* part_setup_flashlight */
        .flip   = { 0x1da8, LOAD_SEG + 0x172c },    /* part_flip_flashlight */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_GENERATOR] = {    /* DGROUP 0x148a */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x08f5, .hotspots_ptr = 0x0955, .sizes_ptr = 0x0915,
        .refile_level = { 0x05, 0xff }, .point_count = 0x0004, .priority = 0x0015,
        .hit    = { 0x1de0, LOAD_SEG + 0x172c },    /* part_hit_generator */
        .step   = { 0x1e5c, LOAD_SEG + 0x172c },    /* part_step_generator */
        .setup  = { 0x1dfb, LOAD_SEG + 0x172c },    /* part_setup_generator */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_GUN] = {    /* DGROUP 0x14c4 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x00c0, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x09de, .hotspots_ptr = 0x0a08, .sizes_ptr = 0x09ec,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0007, .priority = 0x0012,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x22ae, LOAD_SEG + 0x172c },    /* part_step_gun */
        .setup  = { 0x23b1, LOAD_SEG + 0x172c },    /* part_setup_gun */
        .flip   = { 0x2412, LOAD_SEG + 0x172c },    /* part_flip_gun */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x2451, LOAD_SEG + 0x172c },    /* part_drive_2451 */
    },
    [KIND_BASEBALL] = {    /* DGROUP 0x14fe */
        .density = 0x07d0, .weight = 0x0009, .bounce = 0x0040, .grip = 0x0018,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .priority = 0x0003,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x00c9, LOAD_SEG + 0x172c },    /* part_setup_00c9 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_LIGHT] = {    /* DGROUP 0x1538 */
        .density = 0x0514, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0a52, .hotspots_ptr = 0x0a6a, .sizes_ptr = 0x0a5a,
        .refile_level = { 0x02, 0xff }, .point_count = 0x0000, .priority = 0x001b,
        .hit    = { 0x2b7e, LOAD_SEG + 0x172c },    /* part_hit_light */
        .step   = { 0x2b99, LOAD_SEG + 0x172c },    /* part_step_light */
        .setup  = { 0x2b58, LOAD_SEG + 0x172c },    /* part_setup_light */
        .flip   = { 0x2bc5, LOAD_SEG + 0x172c },    /* part_flip_light */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x2c19, LOAD_SEG + 0x172c },    /* part_drive_2c19 */
    },
    [KIND_MAGNIFYING_GLASS] = {    /* DGROUP 0x1572 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0000, .priority = 0x0019,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x3035, LOAD_SEG + 0x172c },    /* part_step_magnifying_glass */
        .setup  = { 0x3030, LOAD_SEG + 0x172c },    /* part_setup_magnifying_glass */
        .flip   = { 0x31af, LOAD_SEG + 0x172c },    /* part_flip_magnifying_glass */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_MONKEY] = {    /* DGROUP 0x15ac */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0b35, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0b4f,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0009, .priority = 0x0027,
        .hit    = { 0x2c83, LOAD_SEG + 0x172c },    /* part_hit_monkey */
        .step   = { 0x2d40, LOAD_SEG + 0x172c },    /* part_step_monkey */
        .setup  = { 0x2cce, LOAD_SEG + 0x172c },    /* part_setup_monkey */
        .flip   = { 0x2e0c, LOAD_SEG + 0x172c },    /* part_flip_monkey */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x2e4b, LOAD_SEG + 0x172c },    /* part_drive_2e4b */
    },
    [KIND_PUMPKIN] = {    /* DGROUP 0x15e6 */
        .density = 0x0960, .weight = 0x0064, .bounce = 0x0040, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x35f4, LOAD_SEG + 0x172c },    /* part_setup_pumpkin */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_HEART_BALLOON] = {    /* DGROUP 0x1620 */
        .density = 0x000b, .weight = 0x0004, .bounce = 0x0080, .grip = 0x0008,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0007, .priority = 0x0033,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x2682, LOAD_SEG + 0x172c },    /* part_setup_heart_balloon */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x26c3, LOAD_SEG + 0x172c },    /* part_drive_26c3 */
    },
    [KIND_CHRISTMAS_TREE] = {    /* DGROUP 0x165a */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0007, .priority = 0x0034,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x1075, LOAD_SEG + 0x172c },    /* part_setup_christmas_tree */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BOXING_GLOVE] = {    /* DGROUP 0x1694 */
        .density = 0x0ec0, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e7a, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0006, .priority = 0x0008,
        .hit    = { 0x0552, LOAD_SEG + 0x172c },    /* part_hit_boxing_glove */
        .step   = { 0x057e, LOAD_SEG + 0x172c },    /* part_step_boxing_glove */
        .setup  = { 0x065b, LOAD_SEG + 0x172c },    /* part_setup_boxing_glove */
        .flip   = { 0x06c6, LOAD_SEG + 0x172c },    /* part_flip_boxing_glove */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_ROCKET] = {    /* DGROUP 0x16ce */
        .density = 0x4650, .weight = 0x0708, .bounce = 0x0080, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0c19, .hotspots_ptr = 0x0c55, .sizes_ptr = 0x0c2d,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0004, .priority = 0x001e,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x3635, LOAD_SEG + 0x172c },    /* part_step_rocket */
        .setup  = { 0x3737, LOAD_SEG + 0x172c },    /* part_setup_rocket */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_SCISSORS] = {    /* DGROUP 0x1708 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0c96, .hotspots_ptr = 0x0ca2, .sizes_ptr = 0x0c9a,
        .refile_level = { 0x04, 0x00 }, .point_count = 0x0008, .priority = 0x0013,
        .hit    = { 0x3824, LOAD_SEG + 0x172c },    /* part_hit_scissors */
        .step   = { 0x38fc, LOAD_SEG + 0x172c },    /* part_step_scissors */
        .setup  = { 0x389b, LOAD_SEG + 0x172c },    /* part_setup_scissors */
        .flip   = { 0x3944, LOAD_SEG + 0x172c },    /* part_flip_scissors */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_SOLAR_PANEL] = {    /* DGROUP 0x1742 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0ce2, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0cea,
        .refile_level = { 0x05, 0xff }, .point_count = 0x0000, .priority = 0x0016,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x3e08, LOAD_SEG + 0x172c },    /* part_step_solar_panel */
        .setup  = { 0x3de5, LOAD_SEG + 0x172c },    /* part_setup_solar_panel */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_TRAMPOLINE] = {    /* DGROUP 0x177c */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0d90, .hotspots_ptr = 0x0dae, .sizes_ptr = 0x0d9a,
        .refile_level = { 0x04, 0x00 }, .point_count = 0x0004, .priority = 0x0009,
        .hit    = { 0x3ebf, LOAD_SEG + 0x172c },    /* part_hit_trampoline */
        .step   = { 0x3fae, LOAD_SEG + 0x172c },    /* part_step_trampoline */
        .setup  = { 0x3f72, LOAD_SEG + 0x172c },    /* part_setup_trampoline */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_WINDMILL] = {    /* DGROUP 0x17b6 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0080, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e8e, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0003, .priority = 0x000e,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x49a1, LOAD_SEG + 0x172c },    /* part_step_windmill */
        .setup  = { 0x496f, LOAD_SEG + 0x172c },    /* part_setup_windmill */
        .flip   = { 0x4a22, LOAD_SEG + 0x172c },    /* part_flip_windmill */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_BLAST] = {    /* DGROUP 0x17f0 */
        .density = 0x0000, .weight = 0x0001, .bounce = 0x0100, .grip = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0e96, .sizes_ptr = 0x0000,
        .refile_level = { 0x00, 0xff }, .point_count = 0x0000, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x1649, LOAD_SEG + 0x172c },    /* part_step_1649 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_MORT_THE_MOUSE] = {    /* DGROUP 0x182a */
        .density = 0x07d0, .weight = 0x0001, .bounce = 0x0000, .grip = 0x0100,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0ea2, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0005, .priority = 0x0024,
        .hit    = { 0x34b5, LOAD_SEG + 0x172c },    /* part_hit_mort_the_mouse */
        .step   = { 0x34d0, LOAD_SEG + 0x172c },    /* part_step_mort_the_mouse */
        .setup  = { 0x346f, LOAD_SEG + 0x172c },    /* part_setup_mort_the_mouse */
        .flip   = { 0x35c7, LOAD_SEG + 0x172c },    /* part_flip_mort_the_mouse */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_CANNON_BALL] = {    /* DGROUP 0x1864 */
        .density = 0x53b4, .weight = 0x6d60, .bounce = 0x0020, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .priority = 0x0002,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x0065, LOAD_SEG + 0x172c },    /* part_setup_cannon_ball */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_TENNIS_BALL] = {    /* DGROUP 0x189e */
        .density = 0x052a, .weight = 0x0005, .bounce = 0x00c0, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0008, .priority = 0x0004,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x00c9, LOAD_SEG + 0x172c },    /* part_setup_00c9 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_CANDLE] = {    /* DGROUP 0x18d8 */
        .density = 0x07d0, .weight = 0x000c, .bounce = 0x0080, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0e12, .hotspots_ptr = 0x0e36, .sizes_ptr = 0x0e1e,
        .refile_level = { 0x03, 0xff }, .point_count = 0x0003, .priority = 0x001f,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x098a, LOAD_SEG + 0x172c },    /* part_step_candle */
        .setup  = { 0x0950, LOAD_SEG + 0x172c },    /* part_setup_candle */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_PIPE] = {    /* DGROUP 0x1912 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .priority = 0x0029,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x48ab, LOAD_SEG + 0x172c },    /* part_setup_48ab */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x48f7, LOAD_SEG + 0x172c },    /* part_settle_48f7 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_CORNER_PIPE] = {    /* DGROUP 0x194c */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0008, .priority = 0x002a,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x377b, LOAD_SEG + 0x172c },    /* part_setup_corner_pipe */
        .flip   = { 0x37e5, LOAD_SEG + 0x172c },    /* part_flip_corner_pipe */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_WOODEN_PLATFORM] = {    /* DGROUP 0x1986 */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x000c,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .priority = 0x002b,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x48ab, LOAD_SEG + 0x172c },    /* part_setup_48ab */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x48f7, LOAD_SEG + 0x172c },    /* part_settle_48f7 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_ANCHOR] = {    /* DGROUP 0x19c0 */
        .density = 0x0640, .weight = 0x03e8, .bounce = 0x0000, .grip = 0x0000,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_MOTOR] = {    /* DGROUP 0x19fa */
        .density = 0x1d80, .weight = 0x03e8, .bounce = 0x0100, .grip = 0x0010,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0005, .priority = 0x0018,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x13c9, LOAD_SEG + 0x172c },    /* part_step_motor */
        .setup  = { 0x1435, LOAD_SEG + 0x172c },    /* part_setup_motor */
        .flip   = { 0x149b, LOAD_SEG + 0x172c },    /* part_flip_motor */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [51] = {    /* DGROUP 0x1a34 */
        .density = 0x0064, .weight = 0x008c, .bounce = 0x0000, .grip = 0x0002,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [52] = {    /* DGROUP 0x1a6e */
        .density = 0x0064, .weight = 0x008c, .bounce = 0x0000, .grip = 0x0002,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [53] = {    /* DGROUP 0x1aa8 */
        .density = 0x0064, .weight = 0x008c, .bounce = 0x0000, .grip = 0x0002,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [54] = {    /* DGROUP 0x1ae2 */
        .density = 0x0064, .weight = 0x008c, .bounce = 0x0000, .grip = 0x0002,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x00f0, .max_h = 0x00f0, .min_w = 0x0020, .min_h = 0x0020,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0000, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x02a6, LOAD_SEG + 0x0000 },    /* part_hook_none_2a6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_55] = {    /* DGROUP 0x1b1c */
        .density = 0x0064, .weight = 0x008c, .bounce = 0x0040, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x04, 0xff }, .point_count = 0x0004, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x1105, LOAD_SEG + 0x172c },    /* part_setup_1105 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_56] = {    /* DGROUP 0x1b56 */
        .density = 0x0064, .weight = 0x008c, .bounce = 0x0080, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x05, 0xff }, .point_count = 0x0007, .priority = 0x0032,
        .hit    = { 0x0297, LOAD_SEG + 0x0000 },    /* part_hook_yes */
        .step   = { 0x02a1, LOAD_SEG + 0x0000 },    /* part_hook_none_2a1 */
        .setup  = { 0x10b6, LOAD_SEG + 0x172c },    /* part_setup_10b6 */
        .flip   = { 0x02ab, LOAD_SEG + 0x0000 },    /* part_hook_none_2ab */
        .settle = { 0x02b0, LOAD_SEG + 0x0000 },    /* part_hook_none_2b0 */
        .drive  = { 0x02b5, LOAD_SEG + 0x0000 },    /* part_hook_no */
    },
    [KIND_57] = {    /* DGROUP 0x1b90 */
        .density = 0x0064, .weight = 0x008c, .bounce = 0x0080, .grip = 0x0020,
        .gravity = 0x0000, .max_speed = 0x0000,
        .max_w = 0x0000, .max_h = 0x0000, .min_w = 0x00f0, .min_h = 0x00f0,
        .bitmaps_ptr = 0x0000, .bitmaps2_ptr = 0x0000, .hotspots_ptr = 0x0000, .sizes_ptr = 0x0000,
        .refile_level = { 0x00, 0xff }, .point_count = 0x0004, .priority = 0x0032,
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
struct timer TIMER DGROUP_AT(0x44ee) = { .divisor = -1 };
struct game_text_lines GAME_TEXT_LINES DGROUP_BSS(0x56a6);
struct dg_5752 DG5752 DGROUP_BSS(0x5752);
struct dg_5456 DG5456 DGROUP_BSS(0x5456);
struct dg_4e34 DG4E34 DGROUP_AT(0x4e34) = { .cr = { 0x0d }, .realcvt_ptr = 0xc884 };
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
    .gc_mode_fill = 0x02,
    .quarter_a = 0x40,
    .gc_mode_copy = 0x01,
    .quarter_b = 0x41,
    .mode_found = 0xff,
    .mode_forced = 0xff,
};
struct dg_3576 DG3576 DGROUP_AT(0x3576);
struct dg_521b DG521B DGROUP_BSS(0x521b);
struct dos_startup DOS_STARTUP DGROUP_AT(0x0074) = { .argc = 0 };
struct dos_program_top DOS_PROGRAM_TOP DGROUP_AT(0x00a0) = { .top_a = 0 };
struct dg_0094 DG0094 DGROUP_AT(0x0094) = { .pad_009a = { 0xca, 0x64 }, .brklvl_ptr = 0x64ca };
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
    .detect_allowed = 0x0001,
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
    .cut_line = {
        { 0x0016, 0x000f, 0x0027, 0x000f },
        { 0x0000, 0x000f, 0x0010, 0x000f },
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
        { .x = 0x0024, .y = 0x0015 }, { .y = 0x0008 },
    },
    .shaft_line = {
        { 0x0000, 0x0020, 0x004f, 0x0003 },
        { 0x0000, 0x0011, 0x004f, 0x0011 },
        { 0x0000, 0x0003, 0x004f, 0x0020 },
    },
};
struct iff_chunk_names IFF_CHUNK_NAMES DGROUP_AT(0x355a) = {
    .form = "FORM",
    .ilbm = "ILBM",
    .bmhd = "BMHD",
    .cmap = "CMAP",
    .body = "BODY",
    .mode_wb = "wb",
};

/* DGROUP 0x2966 - the part templates `make_part` copies from; see dgroup.h. */
struct part_template PART_TEMPLATES[PART_KIND_COUNT] DGROUP_AT(0x2966) = {
    [0] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0020, .height = 0x0020 },
        .init = { .off = 0x6246, .seg = LOAD_SEG + 0x0dff },
    },
    [1] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0020, .height = 0x0010 },
        .size = { .width = 0x0020, .height = 0x0010 },
        .init = { .off = 0x6277, .seg = LOAD_SEG + 0x0dff },
    },
    [2] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0020, .height = 0x0020 },
        .init = { .off = 0x62b1, .seg = LOAD_SEG + 0x0dff },
    },
    [3] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0050, .height = 0x0020 },
        .size = { .width = 0x0050, .height = 0x0020 },
        .init = { .off = 0x62f6, .seg = LOAD_SEG + 0x0dff },
    },
    [4] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0020, .height = 0x0030 },
        .size = { .width = 0x0020, .height = 0x0030 },
        .init = { .off = 0x6330, .seg = LOAD_SEG + 0x0dff },
    },
    [5] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0060, .height = 0x0010 },
        .size = { .width = 0x0060, .height = 0x0010 },
        .init = { .off = 0x6371, .seg = LOAD_SEG + 0x0dff },
    },
    [6] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0030, .height = 0x0020 },
        .size = { .width = 0x0030, .height = 0x0020 },
        .init = { .off = 0x63c3, .seg = LOAD_SEG + 0x0dff },
    },
    [7] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0010, .height = 0x0010 },
        .size = { .width = 0x0010, .height = 0x0010 },
        .init = { .off = 0x640b, .seg = LOAD_SEG + 0x0dff },
    },
    [8] = {
        .flags_06 = 0x4800,
        .init = { .off = 0x644d, .seg = LOAD_SEG + 0x0dff },
    },
    [9] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0020, .height = 0x0020 },
        .init = { .off = 0x647c, .seg = LOAD_SEG + 0x0dff },
    },
    [10] = {
        .flags_06 = 0x4800,
        .init = { .off = 0x64ad, .seg = LOAD_SEG + 0x0dff },
    },
    [11] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0030, .height = 0x0040 },
        .size = { .width = 0x0030, .height = 0x0040 },
        .init = { .off = 0x64db, .seg = LOAD_SEG + 0x0dff },
    },
    [12] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0028, .height = 0x0029 },
        .size = { .width = 0x0028, .height = 0x0027 },
        .init = { .off = 0x651c, .seg = LOAD_SEG + 0x0dff },
    },
    [13] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0020, .height = 0x0020 },
        .init = { .off = 0x6557, .seg = LOAD_SEG + 0x0dff },
    },
    [14] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0023, .height = 0x0023 },
        .init = { .off = 0x659f, .seg = LOAD_SEG + 0x0dff },
    },
    [15] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0030, .height = 0x0030 },
        .size = { .width = 0x0030, .height = 0x0030 },
        .init = { .off = 0x65e1, .seg = LOAD_SEG + 0x0dff },
    },
    [16] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0040, .height = 0x0030 },
        .size = { .width = 0x0040, .height = 0x0030 },
        .init = { .off = 0x6617, .seg = LOAD_SEG + 0x0dff },
    },
    [17] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0025, .height = 0x0030 },
        .size = { .width = 0x0028, .height = 0x0030 },
        .init = { .off = 0x664d, .seg = LOAD_SEG + 0x0dff },
    },
    [18] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0040, .height = 0x0034 },
        .size = { .width = 0x0040, .height = 0x0034 },
        .init = { .off = 0x668e, .seg = LOAD_SEG + 0x0dff },
    },
    [19] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0030, .height = 0x001c },
        .size = { .width = 0x0030, .height = 0x001c },
        .init = { .off = 0x66cd, .seg = LOAD_SEG + 0x0dff },
    },
    [20] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0028, .height = 0x0007 },
        .size = { .width = 0x0028, .height = 0x0007 },
        .init = { .off = 0x670c, .seg = LOAD_SEG + 0x0dff },
    },
    [21] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0030, .height = 0x0020 },
        .size = { .width = 0x0030, .height = 0x0020 },
        .init = { .off = 0x673d, .seg = LOAD_SEG + 0x0dff },
    },
    [22] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0087, .height = 0x002f },
        .size = { .width = 0x0087, .height = 0x002f },
        .init = { .off = 0x677c, .seg = LOAD_SEG + 0x0dff },
    },
    [23] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0010, .height = 0x0010 },
        .size = { .width = 0x0010, .height = 0x0010 },
        .init = { .off = 0x67b7, .seg = LOAD_SEG + 0x0dff },
    },
    [24] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0020, .height = 0x0020 },
        .init = { .off = 0x67d5, .seg = LOAD_SEG + 0x0dff },
    },
    [25] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0020, .height = 0x0010 },
        .size = { .width = 0x0020, .height = 0x0010 },
        .init = { .off = 0x6814, .seg = LOAD_SEG + 0x0dff },
    },
    [26] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0048, .height = 0x0020 },
        .size = { .width = 0x0048, .height = 0x0020 },
        .init = { .off = 0x684a, .seg = LOAD_SEG + 0x0dff },
    },
    [27] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0040, .height = 0x001f },
        .size = { .width = 0x0040, .height = 0x001f },
        .init = { .off = 0x6884, .seg = LOAD_SEG + 0x0dff },
    },
    [28] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x000f, .height = 0x000f },
        .size = { .width = 0x000f, .height = 0x000f },
        .init = { .off = 0x68bf, .seg = LOAD_SEG + 0x0dff },
    },
    [29] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0020, .height = 0x0020 },
        .init = { .off = 0x68f0, .seg = LOAD_SEG + 0x0dff },
    },
    [30] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0010, .height = 0x0025 },
        .size = { .width = 0x0010, .height = 0x0025 },
        .init = { .off = 0x690f, .seg = LOAD_SEG + 0x0dff },
    },
    [31] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x005c, .height = 0x004f },
        .size = { .width = 0x005c, .height = 0x004f },
        .init = { .off = 0x6929, .seg = LOAD_SEG + 0x0dff },
    },
    [32] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0027, .height = 0x0021 },
        .size = { .width = 0x0027, .height = 0x0021 },
        .init = { .off = 0x6964, .seg = LOAD_SEG + 0x0dff },
    },
    [33] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0025, .height = 0x0027 },
        .size = { .width = 0x0025, .height = 0x0027 },
        .init = { .off = 0x6995, .seg = LOAD_SEG + 0x0dff },
    },
    [34] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0029, .height = 0x0049 },
        .size = { .width = 0x0029, .height = 0x0049 },
        .init = { .off = 0x69d6, .seg = LOAD_SEG + 0x0dff },
    },
    [35] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0030, .height = 0x001f },
        .size = { .width = 0x0030, .height = 0x001f },
        .init = { .off = 0x6a07, .seg = LOAD_SEG + 0x0dff },
    },
    [36] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0010, .height = 0x0034 },
        .size = { .width = 0x0010, .height = 0x0034 },
        .init = { .off = 0x6a3d, .seg = LOAD_SEG + 0x0dff },
    },
    [37] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0028, .height = 0x0020 },
        .size = { .width = 0x0028, .height = 0x0020 },
        .init = { .off = 0x6a77, .seg = LOAD_SEG + 0x0dff },
    },
    [38] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0048, .height = 0x0020 },
        .size = { .width = 0x0048, .height = 0x0020 },
        .init = { .off = 0x6ab2, .seg = LOAD_SEG + 0x0dff },
    },
    [39] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0030, .height = 0x001c },
        .size = { .width = 0x0030, .height = 0x001c },
        .init = { .off = 0x6ac9, .seg = LOAD_SEG + 0x0dff },
    },
    [40] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0028, .height = 0x0030 },
        .size = { .width = 0x0028, .height = 0x0030 },
        .init = { .off = 0x6aff, .seg = LOAD_SEG + 0x0dff },
    },
    [41] = { .flags_0a = 0x0008 },
    [42] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0018, .height = 0x000b },
        .size = { .width = 0x0018, .height = 0x000b },
        .init = { .off = 0x6b47, .seg = LOAD_SEG + 0x0dff },
    },
    [43] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0018, .height = 0x0017 },
        .size = { .width = 0x0018, .height = 0x0017 },
        .init = { .off = 0x6b82, .seg = LOAD_SEG + 0x0dff },
    },
    [44] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x000f, .height = 0x000f },
        .size = { .width = 0x000f, .height = 0x000f },
        .init = { .off = 0x6bb3, .seg = LOAD_SEG + 0x0dff },
    },
    [45] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0020, .height = 0x0020 },
        .init = { .off = 0x6be4, .seg = LOAD_SEG + 0x0dff },
    },
    [46] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0020, .height = 0x0010 },
        .size = { .width = 0x0020, .height = 0x0010 },
        .init = { .off = 0x6277, .seg = LOAD_SEG + 0x0dff },
    },
    [47] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0020, .height = 0x0020 },
        .size = { .width = 0x0020, .height = 0x0020 },
        .init = { .off = 0x6c22, .seg = LOAD_SEG + 0x0dff },
    },
    [48] = {
        .flags_06 = 0x4800,
        .set_size = { .width = 0x0020, .height = 0x0010 },
        .size = { .width = 0x0020, .height = 0x0010 },
        .init = { .off = 0x6277, .seg = LOAD_SEG + 0x0dff },
    },
    [49] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .init = { .off = 0x6c58, .seg = LOAD_SEG + 0x0dff },
    },
    [50] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0038, .height = 0x002f },
        .size = { .width = 0x0038, .height = 0x002f },
        .init = { .off = 0x6c72, .seg = LOAD_SEG + 0x0dff },
    },
    [51] = { .flags_0a = 0x0008 },
    [52] = { .flags_0a = 0x0008 },
    [53] = { .flags_0a = 0x0008 },
    [54] = { .flags_0a = 0x0008 },
    [55] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0060, .height = 0x0010 },
        .size = { .width = 0x0060, .height = 0x0010 },
        .init = { .off = 0x6cb0, .seg = LOAD_SEG + 0x0dff },
    },
    [56] = {
        .flags_06 = 0x4800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x00d4, .height = 0x0010 },
        .size = { .width = 0x00d4, .height = 0x0010 },
        .init = { .off = 0x6ce9, .seg = LOAD_SEG + 0x0dff },
    },
    [57] = {
        .flags_06 = 0x0800,
        .flags_0a = 0x0008,
        .set_size = { .width = 0x0020, .height = 0x0010 },
        .size = { .width = 0x0020, .height = 0x0010 },
        .init = { .off = 0x6d1a, .seg = LOAD_SEG + 0x0dff },
    },
};
struct dg_2d06 DG2D06 DGROUP_AT(0x2d06) = { ._pad_2d06 = 0x0001, ._pad_2d08 = 0xffff };

struct dg_440e DG440E DGROUP_AT(0x440e) = {
    .ptr_440e = { .off = 0x2716, .seg = LOAD_SEG + 0x1c25 },
    .driver_table = {
        { .off = 0x586d, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0x58e4, .seg = LOAD_SEG + 0x1c25 },
        { .off = 0xba6a, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x92dc, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x93a2, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x94fb, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x9571, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x93e0, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xba5b, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x8fcd, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x91ef, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x917f, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb1e, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb75, .seg = LOAD_SEG + 0x0000 },
        { .off = 0x93f6, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb2d, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb3c, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb4f, .seg = LOAD_SEG + 0x0000 },
        { .off = 0xbb62, .seg = LOAD_SEG + 0x0000 },
    },
};
struct far_ptr DG44EA DGROUP_AT(0x44ea) = { .off = 0x3f39, .seg = LOAD_SEG + 0x1c25 };
struct adapter_tags ADAPTER_TAGS DGROUP_AT(0x48fc) = {
    .bad = "BAD:",
    .tag = {
        0x4923, 0x4928, 0x492d, 0x4932, 0x4937, 0x48fc, 0x493c, 0x4941,
        0x4946, 0x494b, 0x4950, 0x4955,
    },
};
struct sound_tags SOUND_TAGS DGROUP_AT(0x4a1c) = {
    .device = {
        0x4a38, 0x4a3d, 0x4a42, 0x4a47, 0x4a4c, 0x4a51, 0x4a56, 0x4a5b,
        0x4a60,
    },
    .module = { 0x4a65, 0x4a6a, 0x4a6f, 0x4a74, 0x4a79 },
    .tag = {
        "STD:", "TAN:", "ADL:", "M32:", "SBP:", "PS1:", "PRO:", "GMD:",
        "NLD:", "ASB:", "APS:", "ATD:", "APA:", "ADS:",
    },
    .mode_r_a = "r",
    .mode_r_b = "r",
};
struct dg_4ab0 DG4AB0 DGROUP_AT(0x4ab0) = { ._pad_4ab0 = 0xfffe, ._pad_4ab2 = 0x2b11 };

/* The sound module's data inside its code segment, and segment 1c25's three
   cells - see `struct snd_cs` and `struct s1c_timer` in dgroup.h. */
struct snd_cs SNDS SEGMENT_AT(0x2619, 0x0008) = {
    .voice_held = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .voice_request = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .saved_request = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .voice_channel = { 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f },
    .pending_volume = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .voice_hi = 0x0f,
    .own_voice = 0xff,
};
struct snd_cs_call SNDCALL SEGMENT_AT(0x2619, 0x30f6);
struct s1c_timer S1C_TIMER SEGMENT_AT(0x1c25, 0x446d);
struct s1c_keyboard S1C_KEYBOARD SEGMENT_AT(0x1c25, 0x4e3c);
struct s1c_huge_move S1C_HUGE_MOVE SEGMENT_AT(0x1c25, 0x5f99);
