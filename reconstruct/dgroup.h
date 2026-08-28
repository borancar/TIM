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

/*
 * DGROUP is a **window into the guest's address space**, not storage of its
 * own. The game holds far pointers - `les bx, [0x546c]` - into blocks DOS gave
 * it, which are outside DGROUP entirely, so a DGROUP-only array cannot express
 * them. Real mode is a flat megabyte with segments as sixteen-byte units, and
 * that is what this models.
 */
#define GUEST_MEM_BYTES 0x100000
#define DGROUP_BYTES    0x10000

extern uint8_t  guest_mem[GUEST_MEM_BYTES];
extern uint32_t dgroup_base;        /* linear address of DGROUP */

#define dgroup      (guest_mem + dgroup_base)

#define DG8(off)    (*(uint8_t  *)(dgroup + (off)))
#define DGS8(off)   (*(int8_t   *)(dgroup + (off)))
#define DG16(off)   (*(int16_t  *)(dgroup + (off)))
#define DG32(off)   (*(int32_t  *)(dgroup + (off)))
#define DGU16(off)  (*(uint16_t *)(dgroup + (off)))

/* A far pointer: segment and offset, as the hardware forms an address. */
#define FAR_PTR(seg, off) \
    (guest_mem + (((uint32_t)(uint16_t)(seg)) << 4) + (uint16_t)(off))
#define FAR8(seg, off)    (*(uint8_t  *)FAR_PTR(seg, off))
#define FAR16(seg, off)   (*(int16_t  *)FAR_PTR(seg, off))
#define FARU16(seg, off)  (*(uint16_t *)FAR_PTR(seg, off))

/* A far pointer *stored* in DGROUP: offset first, then segment. */
#define DG_FAR_OFF(o)     DGU16(o)
#define DG_FAR_SEG(o)     DGU16((o) + 2)

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

/*
 * The line drawer's own scratch, all inside the same block: the colour it is
 * drawing with, its current bit mask, and the four words its fixed-point DDA
 * keeps between rows. The original stores these rather than holding them in
 * registers, and the port has to as well or the memory comparison sees the
 * difference - which is how they were found.
 */
#define vga_line_colour    DGU16(VMDS + 0x22)
#define vga_dda_whole      DGU16(VMDS + 0x6bc)
#define vga_dda_frac       DGU16(VMDS + 0x6be)
#define vga_dda_saved      DG16(VMDS + 0x6c0)
#define vga_dda_acc        DGU16(VMDS + 0x6c2)
#define vga_line_mask      DG8(VMDS + 0x6c4)

/* Where VGA:0x0f15 keeps the copy of the current palette. */
#define vga_pal_copy_off   DGU16(VMDS + 0x19e)
#define vga_pal_copy_seg   DGU16(VMDS + 0x1a0)

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
 * DGROUP 0x4342 holds the *segment* of the block the game builds span lists in
 * - a separate allocation, not part of DGROUP. It is reached through
 * `FAR_PTR(span_buffer_seg, 0)`, in the guest's address space, exactly where
 * the original puts it.
 *
 * An earlier version gave the port an array of its own for this. It passed
 * every check until the verifier began comparing all of conventional memory
 * rather than only DGROUP, and then `fill_rect` and `vm_fill_spans` both
 * failed at once: the original's span list was being written somewhere the
 * port never touched.
 */
#define span_buffer_seg   DGU16(0x4342)

/*
 * The sound module keeps its state in **its own code segment**, segment 0x2619,
 * the same way the video driver keeps its data inside DGROUP. `SND8`/`SND16`
 * reach it. The image base is derived from `dgroup_base` because that is the
 * one thing tools/verify.py sets from the run it captured.
 *
 * The sound driver is a separate loaded block, and its address is not a
 * constant: the game holds a far pointer to it at the sound module's own
 * `cs:[0x1e7]`. `SX_SEG` reads that rather than hard-coding the 0x418f seen in
 * these runs, so the port follows the loader wherever it puts the driver.
 */
#define IMAGE_BASE  (dgroup_base - 0x2D3C0)
#define SNDCS       (IMAGE_BASE + 0x26190)

#define SND8(off)   (*(uint8_t  *)(guest_mem + SNDCS + (off)))
#define SND16(off)  (*(int16_t  *)(guest_mem + SNDCS + (off)))

#define SX_SEG      (*(uint16_t *)(guest_mem + SNDCS + 0x1e9))
#define SX8(off)    (*(uint8_t  *)FAR_PTR(SX_SEG, (off)))
#define SX16(off)   (*(int16_t  *)FAR_PTR(SX_SEG, (off)))

/*
 * NOT a transcription: a stand-in for the guest's own stack frame.
 *
 * In the large model SS and DS are the same segment, so a local whose address
 * is taken - `lea ax,[bp-0x34]` - hands out an ordinary DGROUP offset, and a
 * routine that receives one cannot tell it from a pointer to a global. Several
 * transcribed routines build a structure on the stack and pass its offset to
 * another transcribed routine, and a C local cannot serve: it is not in
 * `guest_mem` and has no DGROUP offset at all.
 *
 * So the port carries a stack pointer of its own. `dg_enter` reserves bytes
 * below it and answers the offset of the low end; `dg_leave` gives them back.
 * tools/verify.py sets `guest_sp` to whatever the original's SP was at the
 * routine's entry, so the port's frame lands inside the range the verifier
 * already excludes from comparison - the bytes the call used as its stack.
 * Nothing is read back from a frame after `dg_leave`.
 */
extern uint16_t guest_sp;

uint16_t dg_enter(uint16_t bytes);
void     dg_leave(uint16_t bytes);

#endif /* DGROUP_H */
