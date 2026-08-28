/*
 * The original's DGROUP variables.
 *
 * **A boundary the port chose, not one the binary proves.** In the original,
 * each module's statics sit together inside DGROUP in link order, so these
 * belong distributed across the segment files. They are collected here until
 * that attribution is established - see STATUS.md - and each carries the
 * DGROUP offset it was read at, which is the fact that can be checked.
 *
 * DGROUP is at image 0x2d3c0; a DGROUP offset plus that is an image offset.
 */
#ifndef DGROUP_H
#define DGROUP_H

#include <stdint.h>

/*
 * DGROUP 0x5754. Set to 1 by the game's INT 08h handler by way of the code at
 * image 0x0aa08, cleared at 0x0ab17. The main loop at 0x0aaca spins until it
 * is set, so it paces the frame. The *name* is a guess from that behaviour;
 * the offset is not.
 */
extern int16_t frame_flag;

#endif /* DGROUP_H */
