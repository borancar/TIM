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

/*
 * DGROUP 0x52fa and 0x52f2. Both are read by the frame-presentation routine at
 * image 0x081cc to choose between three paths. Names are guesses from that use
 * and nothing more; the offsets are not.
 */
extern int16_t present_hook_a;      /* DGROUP 0x52fa */
extern int16_t present_hook_b;      /* DGROUP 0x52f2 */

/*
 * The drawing state the rectangle routine at 0x20079 reads. Names are guesses
 * from how it uses them; the offsets are not. Measured over 2,108 calls while
 * the intro screens run: fill is always enabled, clipping always on, and the
 * two border bytes always equal - so the border is never drawn.
 */
extern uint8_t  fill_enabled;       /* DGROUP 0x389c */
extern uint8_t  clip_enabled;       /* DGROUP 0x3893 */
extern int16_t  clip_left;          /* DGROUP 0x3894 */
extern int16_t  clip_right;         /* DGROUP 0x3896 */
extern int16_t  clip_top;           /* DGROUP 0x3898 */
extern int16_t  clip_bottom;        /* DGROUP 0x389a */
extern uint8_t  border_colour_a;    /* DGROUP 0x389d */
extern uint8_t  border_colour_b;    /* DGROUP 0x389e */

/*
 * DGROUP 0x4342 holds the *segment* of the buffer the game builds span lists
 * in. The port has no segments, so it is a buffer here - see seg1c25.c.
 */
#define SPAN_BUFFER_BYTES 4096
extern uint8_t span_buffer[SPAN_BUFFER_BYTES];

#endif /* DGROUP_H */
