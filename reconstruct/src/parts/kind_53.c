/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Kind 53's handlers**, new in 1.11 - a kind that has no name here yet;
 * its icon will say what it is. Its record in `g_part_kinds` names these.
 *
 * In 1.11, image 0x1e2ce.. in the part kinds' code segment. **Both ends are
 * ours**: each kind in 1.00 is a module of its own, and so is this one until
 * the far calls between its routines and its neighbours' say otherwise.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -O -Z
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x1e2ce
 *
 * **Kind 53's hit handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
uint16_t part_hit_kind_53(struct part *part)
{
    not_transcribed("0x1e2ce, part_hit_kind_53");
    return 0;
}

/*
 * 0x1e2fc
 *
 * **Kind 53's setup handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
void part_setup_kind_53(struct part *part)
{
    not_transcribed("0x1e2fc, part_setup_kind_53");
}
