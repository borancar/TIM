/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **Blocks by kind**: the allocator the sound library and the game share,
 * and its free - where a block comes from, and whether it is zeroed, decided
 * by a `kind` argument.
 *
 * In 1.11, a module of its own at the end of the code segment that starts
 * at 0x0ecc0, image 0x16beb..0x16ca7, after game.c. 1.00 had the two in the
 * sound library's segment, at the end of what is now sound_file.c.
 *
 * JUDGE: built-with -mm
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * 0x16beb
 *
 * Allocate a block for the sound module, choosing where from by a `kind`
 * argument, and zero it for some kinds but not others.
 *
 * Kinds 6 and 8 come from the C runtime's own heap - a near pointer, with the
 * data segment supplied as the segment half - and everything else from DOS
 * through `dos_alloc_bytes`. The two are not interchangeable: only the DOS path
 * can hand back more than a segment, and only the heap path gives a pointer the
 * runtime can later free.
 *
 * Kinds 2, 3, 4 and 7 are then zeroed with `far_memset`. Note that 6 and 8 are
 * not among them, so a heap block comes back holding whatever was there - and
 * the zeroing is skipped entirely when the allocation failed, which is the only
 * thing the null check guards.
 *
 * `malloc` is not transcribed. The runtime's heap is a deliberate non-goal, and
 * the port refuses rather than inventing a pointer it could not also give a
 * block header to; see `io_malloc`. Kinds 6 and 8 are not reached on the
 * screens checked, so the rest of this verifies.
 */
uint8_t far *alloc_for_kind(uint32_t size, uint16_t kind)
{
    uint8_t far *blk;
    uint8_t *p;

    if (kind == 6 || kind == 8) {
        /* The near heap takes a word, and its answer is widened with DS - so
           a refusal is DGROUP:0000, not the far null. Nothing in the game
           asks for these two kinds. */
        p = malloc_far((uint16_t)size);
        blk = (uint8_t far *)NEAR_ZERO(p);
    } else {
        blk = dos_alloc_bytes(size, 0);
    }

    if (blk != NULL
        && (kind == 2 || kind == 3 || kind == 4 || kind == 7))
        far_memset(blk, 0, size);

    if (blk == NULL)
        g_sound_bank.load_error = 1;

    return blk;
}

/*
 * 0x16c79
 *
 * Release a block the sound module allocated, and the exact counterpart of
 * `alloc_for_kind`: the same `kind` argument picks the same two
 * places, kinds 6 and 8 going back to the C runtime's heap and everything else
 * to DOS.
 *
 * The kind is not stored with the block, so it is the caller's job to release
 * one with the same kind it asked for. Passing the wrong one hands a heap
 * pointer to DOS or a DOS segment to `free`, and nothing here would notice.
 *
 * `free` is not transcribed, for the reason `io_malloc` gives; the DOS path is
 * the one these screens take.
 */
void free_for_kind(uint8_t far * blk, uint16_t kind)
{
    if (kind == 6 || kind == 8)
        free_far((uint8_t *)blk);   /* the near heap takes only the offset */
    else
        dos_free_far(blk);
    /* The image's: a `return` whose jump to the epilogue is the next
       instruction. */
    return;
}
