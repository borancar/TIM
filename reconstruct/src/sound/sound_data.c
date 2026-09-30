/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The sound library's shared data**: the sound bank record, the two words
 * after it, the seven voices and the five-tick wait.
 *
 * A module with no code, so no segment says where it is. Its place is the
 * data's: `g_sound_bank` follows setup_sound_device.c's `_DATA` at DGROUP
 * 0x4680 and ends where load_resource_block.c's literal begins, and the
 * wait and the voices follow next_matching_record.c's `_BSS` at 0x5ff2 -
 * so the module links after setup_sound_device.c. That it is one module
 * and not two of the code modules' is a reading; the addresses are not.
 *
 * JUDGE: built-with -mm -O2
 * JUDGE: data 0x4680..0x46b2
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* The seven voices, DGROUP 0x6414; the record is described in dgroup.h. */
struct sequence far *g_sound_voice[7];

/*
 * **The sound bank, its driver and its module**, DGROUP 0x4a82..0x4ab0: this
 * module's `_DATA`, or sound_stop.c's - the two are adjacent and their bytes
 * would be the same either way; this module is the one that starts and ends
 * sound. The record is described in dgroup.h. Three fields start non-zero:
 * `voice_word` at -4, `bank_choice` at 1 and `device` at -2.
 */
struct sound_bank g_sound_bank = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 0, 0, 0, 0, 1, -2,
};

/*
 * DGROUP 0x4ab0..0x4ab4 - **two words nothing in the image names**: -2, and
 * 0x2b11, 11025, a sample rate. They are the last of the game's data - the
 * run-time library's begins at 0x4ab4 - and the objects linked after this
 * one, trig.c and atan2.c, have no `_DATA`; and this module's has no string
 * literals, which Borland would have put after them. So they are this
 * module's, defined after the record. What they were for is not known.
 */
struct dg_4ab0 g_dg4ab0 = { 0xfffe, 0x2b11 };

/*
 * **The five-tick wait**, DGROUP 0x5ff2; described in dgroup.h.
 */
volatile int16_t g_sound_ticks_left;
