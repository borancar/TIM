/*
 * Storage for the original's DGROUP and for the span-list buffer. See
 * dgroup.h for why DGROUP is an array rather than a set of named globals.
 */
#include "tim.h"
#include "dgroup.h"

/* **`part_hook_no` as a drive hook**: it takes the part and answers 0, and
   the drive's caller pushes all six arguments, which the C calling convention
   lets it ignore. C needs the types to agree, and `void (*)(void)` is the one
   GCC lets a cast go through without complaint. Ours. */
#define NO_DRIVE ((uint16_t (far *)(struct part *, struct part *, uint16_t, \
                                    uint16_t, uint16_t, int32_t)) \
                  (void (far *)(void))part_hook_no)

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
 * The DGROUP structs dgroup.h declares, each at its address. Their
 * initialisers are the image's bytes - see DGROUP_AT in dgroup.h.
 */
struct dg_4e34 DG4E34 DGROUP_AT(0x4e34) = { .cr = { 0x0d }, .realcvt_ptr = 0xc884 };
struct dos_startup DOS_STARTUP DGROUP_AT(0x0074) = { .argc = 0 };
struct dos_program_top DOS_PROGRAM_TOP DGROUP_AT(0x00a0) = { .top_a = 0 };
struct dg_0094 DG0094 DGROUP_AT(0x0094) = { .pad_009a = { 0xca, 0x64 }, .brklvl_ptr = 0x64ca };


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

/* Segment 14de references these; their owner module is not converted
   yet. */
