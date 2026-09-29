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

/*
 * Where DGROUP sits in the machine's megabyte. The original's loader decides
 * it - 0x110 paragraphs for the program, so DGROUP lands at 0x2e4c0.
 */
uint32_t dgroup_base = 0x2E4C0;

/*
 * The port's own stack pointer - see dgroup.h. The default is the top of
 * DGROUP.
 */
uint16_t guest_sp = 0xFF9E;

/*
 * The DGROUP structs dgroup.h declares, with the address each has in the
 * image. Their initialisers are the image's bytes.
 */
struct borland_heap BORLAND_HEAP = { .cr = { 0x0d }, .realcvt = 0xc884 };   /* DGROUP 0x4e34 */
struct dos_startup DOS_STARTUP = { .argc = 0 };   /* DGROUP 0x0074 */
struct dos_program_top DOS_PROGRAM_TOP = { .top_a = 0 };   /* DGROUP 0x00a0 */
/* `brklvl` starts at the end of `_BSS`, 0x64ca, which is an arena
   address the host only knows at startup: `io_start_program` sets it. */
struct borland_globals BORLAND_GLOBALS = { .pad_009a = { 0xca, 0x64 } };   /* DGROUP 0x0094 */


/* The sound module's data inside its code segment, and segment 1c25's three
   cells - see `struct snd_cs` and `struct s1c_timer` in dgroup.h. */
struct snd_cs SNDS = {   /* 2619:0008 */
    .voice_held = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .voice_request = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .saved_request = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .voice_channel = { 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f },
    .pending_volume = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    .voice_hi = 0x0f,
    .own_voice = 0xff,
};
struct snd_cs_call SNDCALL;   /* 2619:30f6 */
struct s1c_timer S1C_TIMER;   /* 1c25:446d */
struct s1c_keyboard S1C_KEYBOARD;   /* 1c25:4e3c */
struct s1c_huge_move S1C_HUGE_MOVE;   /* 1c25:5f99 */

/* Segment 14de references these; their owner module is not converted
   yet. */
