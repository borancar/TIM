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
 * DGROUP.
 */
uint16_t guest_sp = 0xFF9E;

/*
 * The DGROUP structs dgroup.h declares, each at its address. Their
 * initialisers are the image's bytes - see DGROUP_AT in dgroup.h.
 */
struct dg_4e34 DG4E34 DGROUP_AT(0x4e34) = { .cr = { 0x0d }, .realcvt = 0xc884 };
struct dos_startup DOS_STARTUP DGROUP_AT(0x0074) = { .argc = 0 };
struct dos_program_top DOS_PROGRAM_TOP DGROUP_AT(0x00a0) = { .top_a = 0 };
/* `brklvl` starts at the end of `_BSS`, 0x64ca, which is an arena
   address the host only knows at startup: `io_start_program` sets it. */
struct dg_0094 DG0094 DGROUP_AT(0x0094) = { .pad_009a = { 0xca, 0x64 } };


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
