/*
 * Borland's near-heap allocator, from the C runtime at the top of segment 0000.
 *
 * **This is not the game, and it is not part of what the port is reconstructing.**
 * The runtime is a deliberate non-goal - see STATUS.md. It is here because
 * routines the *game* wrote call `free` and `malloc`, and the whole-memory
 * comparison cannot pass them unless the port moves the same heap bytes.
 *
 * It lives in a file of its own so that it stays separable from the game, and
 * it is **kept rather than deleted**: this is Borland's allocator, not this
 * game's, so it is worth having transcribed and checked against a real binary
 * for any other Turbo C or Borland C++ DOS program someone takes apart later.
 * Whether this port links it is a separate question from whether it exists.
 *
 * The layout was read from the disassembly and matches the published
 * description of the Turbo C near heap. A block header is four bytes and sits
 * **below** the pointer the caller gets:
 *
 *     +0  size, always even; **bit 0 is the in-use flag**
 *     +2  the previous block by address, for coalescing
 *
 * A free block reuses the first four bytes of its own payload as a doubly
 * linked ring:
 *
 *     +4  forward
 *     +6  backward
 *
 * which is why the smallest block is eight bytes and why `malloc` rounds up to
 * eight. Three words in DGROUP hold the rest: 0x4e34 the first block, 0x4e36
 * the topmost, 0x4e38 the ring cursor. `__brklvl` is 0x9c and `errno` 0x94.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * 0x0c7c4
 *
 * Move the break - the boundary between the heap and unused data segment.
 *
 * It refuses to come within 0x200 bytes of the stack pointer, answering -1 and
 * setting `errno` to 8 rather than letting the two collide. The port compares
 * against its own `guest_sp`, which stands in for the guest's SP; that is exact
 * only while the caller's frame matches the original's, and the routine is
 * called far enough below the stack that the test passes either way.
 */
int16_t brk_set(uint16_t addr)
{
    if (addr >= (uint16_t)(guest_sp - 0x200)) {
        DG16(0x94) = 8;
        return -1;
    }
    DG16(0x9c) = (int16_t)addr;
    return 0;
}

/*
 * 0x0c95a
 *
 * Take a block out of the free ring. A block that is its own forward link is
 * the only one left, and the cursor is cleared rather than pointed at a block
 * that is no longer free.
 */
void heap_ring_unlink(uint16_t bx)
{
    uint16_t di = DGU16(bx + 6);
    uint16_t si;

    if (bx == di) {
        DG16(0x4e38) = 0;
        return;
    }
    DG16(0x4e38) = (int16_t)di;
    si = DGU16(bx + 4);
    DG16(di + 4) = (int16_t)si;
    DG16(si + 6) = (int16_t)di;
}

/*
 * 0x0c976
 *
 * Put a block into the free ring, before whatever the cursor points at. An
 * empty ring makes the block point at itself both ways.
 */
void heap_ring_insert(uint16_t bx)
{
    uint16_t si = DGU16(0x4e38);
    uint16_t di;

    if (si == 0) {
        DG16(0x4e38) = (int16_t)bx;
        DG16(bx + 4) = (int16_t)bx;
        DG16(bx + 6) = (int16_t)bx;
        return;
    }

    di = DGU16(si + 6);
    DG16(si + 6) = (int16_t)bx;
    DG16(di + 4) = (int16_t)bx;
    DG16(bx + 6) = (int16_t)di;
    DG16(bx + 4) = (int16_t)si;
}

/*
 * 0x0c921
 *
 * Release a block that is not the topmost one, coalescing both ways.
 *
 * The in-use flag is cleared by **decrementing the size**, which works only
 * because every size is even - so the low bit is the flag and nothing else.
 *
 * Backward first: if the block below is also free, the two become one and this
 * block's header stops existing, so the block *above* has its back-pointer
 * fixed to the merged header. Only when that does not happen is the block put
 * into the ring, because a merged block is already in it.
 *
 * Forward second: if the block above is free it is absorbed and then unlinked,
 * which is why the routine falls straight into `heap_ring_unlink` rather than
 * calling it.
 */
void heap_free_middle(uint16_t bx)
{
    uint16_t si, di, ax;

    DG16(bx)--;

    if (bx != DGU16(0x4e34)) {
        si = DGU16(bx + 2);
        ax = DGU16(si);
        if ((ax & 1) == 0) {
            ax = (uint16_t)(ax + DGU16(bx));
            DG16(si) = (int16_t)ax;
            di = (uint16_t)(bx + DGU16(bx));
            DG16(di + 2) = (int16_t)si;
            bx = si;
            goto forward;
        }
    }
    heap_ring_insert(bx);

forward:
    di = (uint16_t)(bx + DGU16(bx));
    ax = DGU16(di);
    if ((ax & 1) != 0)
        return;

    DG16(bx) = (int16_t)(DGU16(bx) + ax);
    si = (uint16_t)(di + ax);
    DG16(si + 2) = (int16_t)bx;
    heap_ring_unlink(di);
}

/*
 * 0x0c8e7
 *
 * Release the topmost block, which is the only case that can give memory back
 * to DOS: the break moves down to wherever the heap now ends.
 *
 * Freeing the topmost block when the block below is also free merges the two
 * and drops the break past both. Freeing the only block resets all three
 * globals to zero, so the next allocation starts the heap again from nothing.
 */
void heap_free_top(uint16_t bx)
{
    uint16_t si;

    if (DGU16(0x4e34) == bx)
        goto reset;

    si = DGU16(bx + 2);
    if ((DGU16(si) & 1) != 0) {
        DG16(0x4e36) = (int16_t)si;
        brk_set(bx);
        return;
    }

    if (si == DGU16(0x4e34)) {
        bx = si;
        goto reset;
    }

    bx = si;
    heap_ring_unlink(bx);
    DG16(0x4e36) = DG16(bx + 2);
    brk_set(bx);
    return;

reset:
    DG16(0x4e34) = 0;
    DG16(0x4e36) = 0;
    DG16(0x4e38) = 0;
    brk_set(bx);
}

/*
 * 0x0c8ca
 *
 * `free`. The header is the four bytes below the pointer, and a pointer below 4
 * is rejected by the borrow out of that subtraction rather than by a comparison
 * - so `free(0)` is safe, and so is any pointer in the first four bytes of the
 * segment.
 *
 * The topmost block is released differently from every other, because only it
 * can move the break.
 */
void heap_free(uint16_t p)
{
    uint16_t bx;

    if (p < 4)
        return;
    bx = (uint16_t)(p - 4);

    if (bx == DGU16(0x4e36))
        heap_free_top(bx);
    else
        heap_free_middle(bx);
}
