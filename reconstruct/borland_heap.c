/*
 * Borland's near-heap allocator, from the C runtime at the top of segment 0000.
 *
 * **Segment 0000, the library's stretch of it from 0x0bbfe.** Segment 0000
 * is `_TEXT`, which the startup, some of the game's units and every library
 * module share; TLINK lays it out startup, game, library, and the split
 * between the game's part and the library's is ours, so the C library stays
 * separable from the game. The game's last object before the library - the
 * far thunks into this heap and the sound module's interface,
 * 0x0bb1e..0x0bbfd - is `src/glue.c`, whose header says how the segment came
 * to be shared. Within the library the three `borland_*.c` files divide it by
 * module rather than by address: this one is the heap, the long arithmetic
 * and the number formatting, `borland_file.c` the streams, the DOS calls, the
 * strings and the `printf` engine, `borland_huge.c` the huge-pointer
 * arithmetic - and their addresses interleave, because the library's link
 * order does. Functions are in address order within this file.
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
 * eight. That header is `struct heap_block` in dgroup.h, reached through
 * `HEAPBLK_PTR`. Three words in DGROUP hold the rest: 0x4e34 the first block,
 * 0x4e36 the topmost, 0x4e38 the ring cursor. `__brklvl` is 0x9c and `errno`
 * 0x94.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * 0x0bd93
 *
 * A **signed** 32-bit divide, answering the quotient: the fourth door into the
 * body at 0x0bdad that `ulong_divide` describes. It is the only one reached by
 * a far call - it sets CX and jumps straight in, where the others first turn a
 * near return address into a far one - and CX bit 0 is what the body tests to
 * decide whether to take the signs off first, so a zero there is the signed
 * one.
 */
int32_t long_divide(int32_t a, int32_t b)
{
    if (b == 0)
        return 0;

    return a / b;
}

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
int16_t brk_set(const uint8_t *addr)
{
    if (addr >= dg_near_ptr((uint16_t)(guest_sp - 0x200))) {
        DG0094.err_no = 8;
        return -1;
    }
    DG0094.brklvl_ptr = dg_near(dgroup, addr);
    return 0;
}

/*
 * 0x0c7e6
 *
 * Move the break by a signed 32-bit amount and answer where it **was** - the
 * Unix convention, and what makes the caller's new block start at the returned
 * address.
 *
 * A high word that is not zero fails outright, so the near heap can never be
 * asked to grow past a segment. The rest is the same 0x200 of stack headroom
 * `brk_set` keeps, tested here both for the carry out of the addition and
 * against SP itself.
 */
uint8_t *heap_sbrk(uint16_t lo, uint16_t hi)
{
    uint32_t sum = (uint32_t)DG0094.brklvl_ptr + lo + ((uint32_t)hi << 16);
    uint16_t cx = (uint16_t)sum;
    uint8_t *old;

    if ((sum >> 16) != 0)
        goto fail;
    if ((uint16_t)(cx + 0x200) < cx)
        goto fail;
    if ((uint16_t)(cx + 0x200) >= guest_sp)
        goto fail;

    old = dg_near_ptr(DG0094.brklvl_ptr);
    DG0094.brklvl_ptr = cx;
    return old;

fail:
    DG0094.err_no = 8;
    return HEAP_SBRK_FAIL;
}

/*
 * OURS: the block arithmetic the original writes on BX, spelled on block
 * pointers. A block's successor by address is its own address plus its size
 * (with the in-use bit masked off where the original masks it), a block's
 * payload - what `malloc` answers and `free` is handed - is four bytes past its
 * header, and a break or payload is turned back into the header it belongs to.
 */
static inline struct heap_block *heap_above(struct heap_block *b, uint16_t size)
{
    return (struct heap_block *)(void *)((uint8_t *)b + size);
}

/* OURS: a block's payload, four bytes past its header. */
static inline uint8_t *heap_payload(struct heap_block *b)
{
    return (uint8_t *)b + 4;
}

/* OURS: the header a payload belongs to. */
static inline struct heap_block *heap_block_of(uint8_t *payload)
{
    return (struct heap_block *)(void *)(payload - 4);
}

/* OURS: the block that starts at a break `heap_sbrk` answered. */
static inline struct heap_block *heap_block_at(uint8_t *brk)
{
    return (struct heap_block *)(void *)brk;
}

/*
 * 0x0c95a
 *
 * Take a block out of the free ring. A block that is its own forward link is
 * the only one left, and the cursor is cleared rather than pointed at a block
 * that is no longer free.
 */
void heap_ring_unlink(struct heap_block *bx)
{
    struct heap_block *di = HEAPBLK_PTR(bx->back_ptr);
    struct heap_block *si;

    if (bx == di) {
        DG4E34.ring_cursor_ptr = 0;
        return;
    }
    DG4E34.ring_cursor_ptr = dg_near(dgroup, di);
    si = HEAPBLK_PTR(bx->fwd_ptr);
    di->fwd_ptr = dg_near(dgroup, si);
    si->back_ptr = dg_near(dgroup, di);
}

/*
 * 0x0c976
 *
 * Put a block into the free ring, before whatever the cursor points at. An
 * empty ring makes the block point at itself both ways.
 */
void heap_ring_insert(struct heap_block *bx)
{
    struct heap_block *si = HEAPBLK_PTR(DG4E34.ring_cursor_ptr);
    struct heap_block *di;

    if (si == HEAPBLK_NONE) {
        DG4E34.ring_cursor_ptr = dg_near(dgroup, bx);
        bx->fwd_ptr = dg_near(dgroup, bx);
        bx->back_ptr = dg_near(dgroup, bx);
        return;
    }

    di = HEAPBLK_PTR(si->back_ptr);
    si->back_ptr = dg_near(dgroup, bx);
    di->fwd_ptr = dg_near(dgroup, bx);
    bx->back_ptr = dg_near(dgroup, di);
    bx->fwd_ptr = dg_near(dgroup, si);
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
void heap_free_middle(struct heap_block *bx)
{
    struct heap_block *si, *di;
    uint16_t ax;

    bx->size--;

    if (bx != HEAPBLK_PTR(DG4E34.first_block_ptr)) {
        si = HEAPBLK_PTR(bx->prev_ptr);
        ax = si->size;
        if ((ax & 1) == 0) {
            ax = (uint16_t)(ax + bx->size);
            si->size = ax;
            di = heap_above(bx, bx->size);
            di->prev_ptr = dg_near(dgroup, si);
            bx = si;
            goto forward;
        }
    }
    heap_ring_insert(bx);

forward:
    di = heap_above(bx, bx->size);
    ax = di->size;
    if ((ax & 1) != 0)
        return;

    bx->size = (uint16_t)(bx->size + ax);
    si = heap_above(di, ax);
    si->prev_ptr = dg_near(dgroup, bx);
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
void heap_free_top(struct heap_block *bx)
{
    struct heap_block *si;

    if (HEAPBLK_PTR(DG4E34.first_block_ptr) == bx)
        goto reset;

    si = HEAPBLK_PTR(bx->prev_ptr);
    if ((si->size & 1) != 0) {
        DG4E34.top_block_ptr = dg_near(dgroup, si);
        brk_set((uint8_t *)bx);
        return;
    }

    if (si == HEAPBLK_PTR(DG4E34.first_block_ptr)) {
        bx = si;
        goto reset;
    }

    bx = si;
    heap_ring_unlink(bx);
    DG4E34.top_block_ptr = bx->prev_ptr;
    brk_set((uint8_t *)bx);
    return;

reset:
    DG4E34.first_block_ptr = 0;
    DG4E34.top_block_ptr = 0;
    DG4E34.ring_cursor_ptr = 0;
    brk_set((uint8_t *)bx);
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
void heap_free(uint8_t *p)
{
    struct heap_block *bx;

    /* An offset below 4 borrows - a null pointer included - and is refused. */
    if (p == NULL || p < dgroup + 4)
        return;
    if (!dg_is_guest(p))
        port_abort("heap_free on a pointer outside guest memory");
    bx = heap_block_of(p);

    if (bx == HEAPBLK_PTR(DG4E34.top_block_ptr))
        heap_free_top(bx);
    else
        heap_free_middle(bx);
}

/*
 * 0x0c9f9
 *
 * Start the heap: take the first block straight from `sbrk`.
 *
 * The break is asked for twice before the block is taken - once with zero, to
 * read where it is, and again with one if that came back odd. Every block
 * address has to be even, because the low bit of the size word is the in-use
 * flag and the arithmetic that clears it would otherwise be wrong.
 */
uint8_t *heap_init(uint16_t size)
{
    struct heap_block *bx;
    uint8_t *at, *got;

    /* An odd break gets one byte - and so does a failed call, `and ax,1`
       at 0x0ca03 reading its -1 as odd. */
    at = heap_sbrk(0, 0);
    if (((at - dgroup) & 1) != 0)
        heap_sbrk(1, 0);

    got = heap_sbrk(size, 0);
    if (got == HEAP_SBRK_FAIL)
        return NULL;

    bx = heap_block_at(got);
    DG4E34.first_block_ptr = dg_near(dgroup, bx);
    DG4E34.top_block_ptr = dg_near(dgroup, bx);
    bx->size = (uint16_t)(size + 1);
    return heap_payload(bx);
}

/*
 * 0x0ca39
 *
 * Grow the heap by one block when nothing in the ring will do.
 *
 * The new block becomes the topmost one and its +2 is pointed at the old top,
 * which is what keeps the chain of previous-blocks-by-address unbroken across
 * every growth.
 */
uint8_t *heap_grow(uint16_t size)
{
    uint8_t *got = heap_sbrk(size, 0);
    struct heap_block *bx;

    if (got == HEAP_SBRK_FAIL)
        return NULL;

    bx = heap_block_at(got);
    bx->prev_ptr = DG4E34.top_block_ptr;
    DG4E34.top_block_ptr = dg_near(dgroup, bx);
    bx->size = (uint16_t)(size + 1);
    return heap_payload(bx);
}

/*
 * 0x0ca62
 *
 * Split a free block that is more than big enough, and answer the piece.
 *
 * The piece taken is the **tail**, not the head - so the part left free keeps
 * its address, its header and its place in the ring, and no ring surgery is
 * needed at all. Only the block above has to be told its neighbour changed.
 */
uint8_t *heap_split(struct heap_block *bx, uint16_t size)
{
    struct heap_block *si, *di;

    bx->size = (uint16_t)(bx->size - size);
    si = heap_above(bx, bx->size);
    di = heap_above(si, size);

    si->size = (uint16_t)(size + 1);
    si->prev_ptr = dg_near(dgroup, bx);
    di->prev_ptr = dg_near(dgroup, si);
    return heap_payload(si);
}

/*
 * 0x0c999
 *
 * `malloc`. Answers a near pointer into DGROUP, or zero.
 *
 * The request grows by five and is then masked even: four for the header and
 * one to round up. Anything under eight becomes eight, because a free block
 * needs room for the ring links its payload will hold.
 *
 * The search is **first fit walking backward** from the ring cursor at 0x4e38,
 * stopping when it comes back to where it started. A block big enough to leave
 * a usable remainder - eight bytes more than asked - is split and the front
 * stays free; one that is merely big enough is taken whole and unlinked.
 *
 * An empty heap starts one, and a walk that finds nothing grows it.
 */
uint8_t *heap_malloc(uint16_t want)
{
    uint16_t size;
    struct heap_block *bx, *start;

    if (want == 0)
        return NULL;
    if ((uint16_t)(want + 5) < want)
        return NULL;

    size = (uint16_t)((want + 5) & 0xfffe);
    if (size < 8)
        size = 8;

    if (DG4E34.first_block_ptr == 0)
        return heap_init(size);

    bx = HEAPBLK_PTR(DG4E34.ring_cursor_ptr);
    if (bx == HEAPBLK_NONE)
        return heap_grow(size);

    start = bx;
    for (;;) {
        if (bx->size >= size)
            break;
        bx = HEAPBLK_PTR(bx->back_ptr);
        if (bx == start)
            return heap_grow(size);
    }

    if (bx->size >= (uint16_t)(size + 8))
        return heap_split(bx, size);

    heap_ring_unlink(bx);
    bx->size++;
    return heap_payload(bx);
}

/*
 * 0x0c16e
 *
 * A 32-bit multiply, `DX:AX` times `CX:BX`, answered in `DX:AX`. Borland's
 * `__LMUL`.
 *
 * Three `mul`s at most and two of them skipped when a high half is zero, which
 * is what the `test`/`jcxz` are for. The port writes it as the multiply it is;
 * the skipping changes nothing but the time it takes.
 */
uint32_t long_multiply(uint32_t a, uint32_t b)
{
    return (a * b);
}

/*
 * 0x0cb45
 *
 * Borland's `heapcheck`: walk the near heap and answer whether it is intact.
 * 1 for an empty heap, 2 for a good one, -1 for a broken one.
 *
 * Two walks. The first follows the block chain from DGROUP 0x4e34 to 0x4e36,
 * where a block's first word is its size with bit 0 saying whether it is in
 * use, and adds up the free ones. Every step is checked: the next block must be
 * *above* this one, a block must be at least 8 bytes, it must stay inside the
 * arena, and its back link at +2 must point at where it was reached from.
 *
 * The second walks the free *ring* from 0x4e38 through the link at +6 and adds
 * those up too. The two totals have to agree - a free block reachable one way
 * and not the other is the corruption this exists to find - and the answer is
 * 2 only if they do.
 *
 * Nothing here is reconstructed loosely: a heap check that answered 2 for a
 * broken heap would turn a real fault into a wrong picture much later.
 */
int16_t heap_check(void)
{
    struct heap_block *bx = HEAPBLK_PTR(DG4E34.first_block_ptr);
    struct heap_block *si;
    uint16_t free_by_chain = 0;      /* CX */
    uint16_t free_by_ring = 0;       /* DX */

    if (bx == HEAPBLK_NONE)
        return 1;                    /* nothing allocated yet */

    si = heap_above(bx, bx->size & 0xfffe);

    for (;;) {
        /* `test byte [bx],1` in the original: bit 0 of the size word,
           which reading the word and masking answers identically. */
        if ((bx->size & 1) == 0) {
            free_by_chain = (uint16_t)(free_by_chain + bx->size);
            if (bx == HEAPBLK_PTR(DG4E34.top_block_ptr))
                break;
            if ((si->size & 1) == 0)
                return -1;
        } else if (bx == HEAPBLK_PTR(DG4E34.top_block_ptr)) {
            break;
        }

        if (si <= bx)
            return -1;
        if (bx->size < 8)
            return -1;
        if (si <= HEAPBLK_PTR(DG4E34.first_block_ptr))
            return -1;
        if (si > HEAPBLK_PTR(DG4E34.top_block_ptr))
            return -1;
        if (HEAPBLK_PTR(si->prev_ptr) != bx)
            return -1;

        bx = si;
        si = heap_above(bx, bx->size & 0xfffe);
    }

    bx = HEAPBLK_PTR(DG4E34.ring_cursor_ptr);
    if (bx == HEAPBLK_NONE)
        goto totals;

    for (;;) {
        uint16_t ax = bx->size;

        if ((ax & 1) != 0)
            return -1;

        free_by_ring = (uint16_t)(free_by_ring + ax);

        if (bx < HEAPBLK_PTR(DG4E34.first_block_ptr))
            return -1;
        if (bx >= HEAPBLK_PTR(DG4E34.top_block_ptr))
            return -1;

        si = HEAPBLK_PTR(bx->back_ptr);
        if (si == HEAPBLK_PTR(DG4E34.ring_cursor_ptr))
            break;
        if (si == bx)
            return -1;
        bx = si;
    }

totals:
    if (free_by_ring != free_by_chain)
        return -1;

    return 2;
}

/*
 * 0x0d543
 *
 * `memset` over a near pointer. Borland's, and the shape is the usual one: a
 * leading byte when the destination is odd, then words, then a trailing byte
 * when the count was odd.
 *
 * Unlike `far_memset` at 0x22300 the alignment test here is correct - `test
 * di,1` rather than a parity flag - so this one has no bug to preserve.
 *
 * It answers the **fill byte doubled into a word**, not the destination: `mov
 * al,[bp+0xa] / mov ah,al` is setting up the `stosw` and AX is simply left
 * holding it. C's `memset` returns the pointer; this one never did, and no
 * caller reads it.
 */
uint16_t near_memset(uint8_t *dst, uint16_t count, uint16_t value)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        dst[i] = (uint8_t)value;

    return (uint16_t)((value & 0xff) * 0x0101);
}

/*
 * 0x0c833
 *
 * `calloc`. The product is worked out in 32 bits by `long_multiply` and
 * **refused if it will not fit in 16**, which is the `cmp` against -1 on the
 * high half and then the low - so a request of 0x10000 bytes or more answers
 * null rather than allocating a wrapped-round size.
 *
 * Otherwise it is `heap_malloc` and a `memset` to zero, and a failed
 * allocation skips the clear.
 */
uint8_t *heap_calloc(uint16_t count, uint16_t size)
{
    uint32_t n = long_multiply(count, size);
    uint8_t *p;

    if (n > 0xffff)
        return NULL;

    p = heap_malloc((uint16_t)n);
    if (p != NULL)
        near_memset(p, (uint16_t)n, 0);

    return p;
}

/*
 * 0x0bd97
 *
 * An **unsigned** 32-bit divide, answering the quotient. One of four near
 * doors - at 0x0bd97, 0x0bd9f and 0x0bda7 - into one body, each setting CX to
 * say which of signed/unsigned and quotient/remainder is wanted; this is the
 * unsigned quotient.
 *
 * The body is a shift-and-subtract loop over 32 bits, except that a divisor
 * whose high half is zero **and** a dividend whose high half is zero take a
 * single `div` instead. It cleans its own arguments - `retf 8`.
 *
 * A zero divisor faults on the original, through the `div`. The port does not
 * reproduce that; nothing here divides by zero.
 */
uint32_t ulong_divide(uint32_t a, uint32_t b)
{
    if (b == 0)
        return 0;

    return a / b;
}

/*
 * 0x0bd2e
 *
 * A far block move, source first and destination second - the opposite way
 * round from `far_memcpy` at 0x22300, which is the game's own. The count is in
 * **CX**, not on the stack, which is why no caller pushes it.
 *
 * Words then a trailing byte: `shr cx,1` halves the count and `adc cx,cx`
 * turns the bit that fell out back into a count of 0 or 1. It cleans its own
 * arguments - `retf 8`.
 */
void far_move(const uint8_t far * src, uint8_t far * dst, uint16_t count)
{
    uint16_t i;

    /*
     * **Both ends were `seg:off` and the offsets were stepped 16-bit**, which
     * is the original's `inc si`/`inc di` and a wrap a host pointer cannot
     * do. Instrumented on 2026-09-09 across the intro, the briefing, the
     * picker and all twenty-eight level snapshots, neither end ever reached
     * `off + count > 0x10000` - and every call in the port passes 0x43 or
     * 0x4c, so a wrap would need a record starting within that of the top of
     * its segment. The wrap is given up; nothing that runs depended on it.
     */
    for (i = 0; i < count; i++)
        dst[i] = src[i];
}


/*
 * 0x0c029
 *
 * The body of `ltoa`: a 32-bit value into a string in a given radix. Answers
 * the buffer.
 *
 * A radix above 0x24 or below 2 writes an empty string and stops, which is the
 * only error it reports.
 *
 * The digits come out **backwards** into a 0x22-byte scratch on the stack and
 * are reversed on the way to the buffer. The 32-bit division is the usual
 * two-`div` pair - the high half first, its remainder carried into the low
 * half - and it drops to a single `div` once the high half is zero.
 *
 * A digit is turned into a character with one subtraction and a branch that
 * costs nothing either way: `sub al,0xa` then either `+ 0x3a`, which is
 * `- 10 + '0'`, or `+` the letter base the caller gave.
 *
 * The sign is only looked at when the caller asks for it, and the `-` goes
 * straight to the buffer before any digit does.
 *
 * The original cleans its own arguments - `ret 0xc`.
 */
char *long_to_string(uint16_t letters, uint16_t is_signed, uint16_t radix,
                       char *buf, int32_t value)
{
    uint8_t digits[0x22];
    int16_t n = 0;
    uint32_t v;
    char *out = buf;

    if (radix > 0x24 || (radix & 0xff) < 2) {
        *out = 0;
        return buf;
    }

    v = (uint32_t)value;

    /* The sign test is on the *high word*, which is where the original's
       `or dx,dx / jns` looks - the same bit as `value < 0`, said the way the
       original says it. */
    if ((int16_t)(uint16_t)(v >> 16) < 0 && (is_signed & 0xff) != 0) {
        *out = '-';
        out++;
        v = (uint32_t)(-(int32_t)v);
    }

    do {
        digits[n++] = (uint8_t)(v % radix);
        v /= radix;
    } while (v != 0);

    while (n-- > 0) {
        uint8_t d = digits[n];

        *out = (char)(d >= 10 ? (d - 10) + letters : d + '0');
        out++;
    }

    *out = 0;
    return buf;
}

/*
 * 0x0d4bd
 *
 * `itoa`. A radix of 10 sign-extends the value into 32 bits and every other
 * radix zero-extends it, which is how the same routine prints -1 as `-1` in
 * decimal and as `ffff` in hex.
 *
 * The "signed" flag it hands to `long_to_string` is 1 regardless; it is the
 * widening above that decides, not the flag. Lower case for the digits past 9.
 */
char *int_to_string(int16_t value, char *buf, uint16_t radix)
{
    uint32_t v = (radix == 10) ? (uint32_t)value
                               : (uint32_t)(uint16_t)value;

    return long_to_string(0x61, 1, radix, buf, (int32_t)v);
}

/*
 * 0x0d4ff
 *
 * `ltoa`. The 32-bit sibling of `itoa`, and the whole difference between them
 * is one flag: here "signed" is `radix == 10`, where `itoa` passes 1 always and
 * lets the widening of its own argument decide instead. Same letter base, same
 * body.
 *
 * **Unverified.** The counters belong to the game proper; the intro screens
 * never reach them, so this is transcribed from the disassembly and has never
 * been run against the original.
 */
char *long_int_to_string(int32_t value, char *buf, uint16_t radix)
{
    return long_to_string(0x61, (uint16_t)(radix == 10), radix, buf, value);
}

/*
 * 0x0ccef
 *
 * **`heapwalk`**: step to the next heap block, filling in the caller's record.
 *
 * Borland's, and its three answers are Borland's constants - 1 `_HEAPEMPTY`,
 * 2 `_HEAPOK`, 5 `_HEAPEND` - which is what identifies the routine. The record
 * is the `heapinfo` the caller owns, a DGROUP offset:
 *
 *     +0  the block pointer, which is also the cursor
 *     +2  its size
 *     +4  whether it is in use
 *
 * A zero cursor starts the walk at 0x4e34, the first block, and an empty heap
 * answers 1 rather than 2. Otherwise the cursor is stepped back over the
 * four-byte header, checked against 0x4e36 - the topmost block, which is where
 * the walk ends with 5 - and advanced by the size in its own first word.
 *
 * **The cursor the caller sees is the payload, not the header.** `+0` is set
 * to the block and then 4 is added to it, so a caller walking the heap is
 * handed pointers it could pass to `free`, and the routine takes the 4 back
 * off at the top of the next call. The size is masked with 0xfffe and the
 * low bit kept separately, because that bit is the in-use flag living in the
 * size word.
 *
 * The prologue is `push bp / push si / mov bp,sp` - bp saved *before* si, not
 * after - so the frame is two words deep before the return address and the
 * argument is at [bp+8] rather than [bp+6]. Read from the instruction, not
 * assumed from the family.
 */
int16_t heapwalk(struct heapinfo *info)
{
    struct heap_block *si;

    if (info->block_ptr != 0) {
        si = heap_block_of(dg_near_ptr(info->block_ptr));
        if (si == HEAPBLK_PTR(DG4E34.top_block_ptr))
            return 5;
        /* `add si,[si] / and si,0xfffe`: the in-use bit in the size makes the
           sum odd, and masking the address clears it - the same as stepping
           by the size without the bit, since every block address is even. */
        si = heap_above(si, si->size & 0xfffe);
    } else {
        si = HEAPBLK_PTR(DG4E34.first_block_ptr);
        if (si == HEAPBLK_NONE)
            return 1;
    }

    info->block_ptr = dg_near(dgroup, heap_payload(si));
    info->size = (uint16_t)(si->size & 0xfffe);
    info->in_use = (uint16_t)(si->size & 1);
    return 2;
}
