/*
 * The original's DGROUP: one 64 KB data segment.
 *
 * It is modelled as a **byte array**, not as a set of C globals, because the
 * game uses *near pointers* - a word in DGROUP holding an offset into DGROUP,
 * dereferenced as `[bx + 0x22]`. Named globals cannot express that; an array
 * can, and it is what the original actually has.
 *
 * Named variables are macros over the array rather than storage of their own,
 * so a name and a pointer dereference reach the same byte. Where a name is a
 * guess it says so; the offsets are read from the disassembly and are not.
 *
 * DGROUP is at image 0x2d3c0, so a DGROUP offset plus that is an image offset.
 *
 * This arrangement also makes verification stronger: tools/verify.py seeds the
 * **whole** segment before a call and compares the whole of it afterwards, so
 * a routine that touches state nobody declared is caught rather than missed.
 */
#ifndef DGROUP_H
#define DGROUP_H

#include <stdint.h>

#define DGROUP_BYTES 0x10000
extern uint8_t dgroup[DGROUP_BYTES];

#define DG8(off)    (*(uint8_t  *)(dgroup + (off)))
#define DGS8(off)   (*(int8_t   *)(dgroup + (off)))
#define DG16(off)   (*(int16_t  *)(dgroup + (off)))
#define DGU16(off)  (*(uint16_t *)(dgroup + (off)))

/*
 * Set to 1 by the game's INT 08h handler by way of the code at image 0x0aa08,
 * cleared at 0x0ab17. The main loop at 0x0aaca spins until it is set, so it
 * paces the frame. The name is a guess from that behaviour.
 */
#define frame_flag        DG16(0x5754)

/* Read by the frame-presentation routine at 0x081cc to choose between three
 * paths. Names are guesses from that use. */
#define present_hook_a    DG16(0x52fa)
#define present_hook_b    DG16(0x52f2)

/*
 * A counter stepped by 0x0144e and wrapped from 0x2a00 back to 0x1c00. What it
 * counts is not established; the name says only where it lives.
 */
#define word_4e87         DG16(0x4e87)

/*
 * ---------------------------------------------------------------------------
 * The video driver's data block, which lives **inside DGROUP** at offset
 * 0x3890.
 *
 * The driver loads its own data segment from `cs:[0x13a]` and keeps the byte
 * distance from DGROUP to it in `cs:[0x13c]` - and that distance is 0x3890.
 * So `driverDS:0x12` and `DGROUP:0x38a2` are the same word, which is why the
 * game's start-up at image 0x0e183 can write the driver's page segments
 * directly:
 *
 *     0e183  mov word ptr [0x38a4], 0xa000
 *     0e189  mov word ptr [0x38a2], 0xa820
 *
 * The clip box and the colours the rectangle routine at 0x20079 reads are the
 * same block seen from the game's side: DGROUP 0x3894 is driver 0x04. There is
 * one shared structure here, not two.
 * ---------------------------------------------------------------------------
 */
#define VMDS 0x3890

#define clip_enabled       DG8(VMDS + 0x03)
#define clip_left          DG16(VMDS + 0x04)
#define clip_right         DG16(VMDS + 0x06)
#define clip_top           DG16(VMDS + 0x08)
#define clip_bottom        DG16(VMDS + 0x0a)
#define fill_enabled       DG8(VMDS + 0x0c)
#define vga_fill_colour    DG8(VMDS + 0x0d)
#define vga_second_colour  DG8(VMDS + 0x0e)

#define vga_page_back      DGU16(VMDS + 0x12)   /* being drawn into */
#define vga_page_front     DGU16(VMDS + 0x14)   /* on screen */
#define vga_page_src       DGU16(VMDS + 0x16)   /* a copy's source */
#define vga_page_dst       DGU16(VMDS + 0x18)   /* what drawing goes into */
#define vga_screen_height  DGU16(VMDS + 0x6ec)  /* the mode's height, 480 */

/* The byte offset of each scan line, indexed by y. Measured: [y] == y*80. */
#define vga_row_offset(y)  DGU16(VMDS + 0x6f2 + 2 * (y))

/*
 * Measured over 2,108 calls to the rectangle routine while the intro screens
 * run: fill is always enabled, clipping always on, and the two colour bytes
 * always equal - which is the condition that skips the outline.
 */

/* Cleared, six words, by the routine at 0x166d6. Purpose not established. */
#define word_array_50bf(i) DG16(0x50bf + 2 * (i))

/* A byte array indexed by the routine at 0x2147d, which returns its bit 0. */
#define byte_array_468c(i) DG8(0x468c + (i))

/*
 * A near pointer at DGROUP 0x5400 to a structure, and three words beside it,
 * all used by the routine at 0x002be. What the structure is has not been
 * established; only the offsets it touches are known.
 */
#define ptr_5400          DGU16(0x5400)
#define word_5402         DG16(0x5402)
#define word_5414         DG16(0x5414)
#define word_541c         DG16(0x541c)
#define word_5420         DG16(0x5420)

/*
 * DGROUP 0x4342 holds the *segment* of the buffer the game builds span lists
 * in - a separate allocation, not part of DGROUP. The port has no segments, so
 * it is a buffer of its own.
 */
#define SPAN_BUFFER_BYTES 4096
extern uint8_t span_buffer[SPAN_BUFFER_BYTES];

#endif /* DGROUP_H */
