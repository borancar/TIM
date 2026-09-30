/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Kind 51's handlers**, new in 1.11 - a kind that has no name here yet;
 * its icon will say what it is. Its record in `g_part_kinds` names these.
 *
 * In 1.11, image 0x1e5af.. in the part kinds' code segment. **Both ends are
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
 * 0x1e5af
 *
 * **Kind 51's hit handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
uint16_t part_hit_kind_51(struct part *part)
{
    not_transcribed("0x1e5af, part_hit_kind_51");
    return 0;
}

/*
 * 0x1e5f3
 *
 * **Kind 51's setup handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
void part_setup_kind_51(struct part *part)
{
    not_transcribed("0x1e5f3, part_setup_kind_51");
}

/*
 * 0x1e643
 *
 * **Kind 51's step handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
void part_step_kind_51(struct part *part)
{
    not_transcribed("0x1e643, part_step_kind_51");
}

/*
 * 0x1e822
 *
 * **Kind 51's flip handler**, new in 1.11. NOT TRANSCRIBED YET: a stub, which aborts.
 */
void part_flip_kind_51(struct part *part)
{
    not_transcribed("0x1e822, part_flip_kind_51");
}
