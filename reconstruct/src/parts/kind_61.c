/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Kind 61's handlers**, new in 1.11 - a kind that has no name here yet;
 * its icon will say what it is. Its record in `g_part_kinds` names these.
 *
 * In 1.11, image 0x1da61.. in the part kinds' code segment. **Both ends are
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
 * 0x1da61
 *
 * **Kind 61's hit handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
uint16_t part_hit_kind_61(struct part *part)
{
    not_transcribed("0x1da61, part_hit_kind_61");
    return 0;
}

/*
 * 0x1dc13
 *
 * **Kind 61's setup handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
void part_setup_kind_61(struct part *part)
{
    not_transcribed("0x1dc13, part_setup_kind_61");
}

/*
 * 0x1dc63
 *
 * **Kind 61's step handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
void part_step_kind_61(struct part *part)
{
    not_transcribed("0x1dc63, part_step_kind_61");
}

/*
 * 0x1dd85
 *
 * **Kind 61's flip handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
void part_flip_kind_61(struct part *part)
{
    not_transcribed("0x1dd85, part_flip_kind_61");
}
