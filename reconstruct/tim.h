/*
 * The Incredible Machine - reconstruction
 *
 * Reconstructed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993), recovered from its LZEXE packing. This
 * file carries no licence: it is derived from someone else's executable and
 * that is not ours to license. See the repository README for the position on
 * both works.
 *
 * Addresses in comments are **image offsets** into the recovered image
 * (out/TIM.img), which is what tools/disasm.py lists.
 */
#ifndef TIM_H
#define TIM_H

#include <stdint.h>

/*
 * Widths are transcribed, not chosen. The original is 16-bit code where every
 * value has a width it depended on, so `int16_t` says "this truncation is the
 * instruction's" in a way `short` cannot. See CLAUDE.md.
 */

/* ---------------------------------------------------------------- segment 0000
 * Image 0x00000..0x0dff0. Contains the Borland startup at the entry point
 * 0000:0000 and game code; which parts are the C runtime is not yet
 * established - see STATUS.md.
 */

/* Set the number of scan lines the CRTC displays before blanking. */
void vm_set_display_lines(uint16_t lines);          /* 0x08f77 */

/* Non-zero while the frame flag has not yet been set by the timer handler. */
int16_t frame_pending(void);                        /* 0x0b4e2 */

/* ------------------------------------------------------- VM.OVL, VGA driver
 * A separate module: the game's video driver, loaded from the resource
 * archive. Addresses are offsets within the loaded driver, not image offsets.
 */

/* Show the page just drawn and swap the buffers. */
void vm_show_page(uint16_t wait_retrace);           /* VM.OVL VGA:0x150f */

#endif /* TIM_H */
