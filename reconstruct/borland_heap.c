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
uint16_t heap_sbrk(uint16_t lo, uint16_t hi)
{
    uint32_t sum = (uint32_t)DGU16(0x9c) + lo + ((uint32_t)hi << 16);
    uint16_t cx = (uint16_t)sum;
    uint16_t old;

    if ((sum >> 16) != 0)
        goto fail;
    if ((uint16_t)(cx + 0x200) < cx)
        goto fail;
    if ((uint16_t)(cx + 0x200) >= guest_sp)
        goto fail;

    old = DGU16(0x9c);
    DG16(0x9c) = (int16_t)cx;
    return old;

fail:
    DG16(0x94) = 8;
    return 0xffff;
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
uint16_t heap_init(uint16_t size)
{
    uint16_t bx, got;

    if ((heap_sbrk(0, 0) & 1) != 0)
        heap_sbrk(1, 0);

    got = heap_sbrk(size, 0);
    if (got == 0xffff)
        return 0;

    bx = got;
    DG16(0x4e34) = (int16_t)bx;
    DG16(0x4e36) = (int16_t)bx;
    DG16(bx) = (int16_t)(size + 1);
    return (uint16_t)(bx + 4);
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
uint16_t heap_grow(uint16_t size)
{
    uint16_t bx = heap_sbrk(size, 0);

    if (bx == 0xffff)
        return 0;

    DG16(bx + 2) = DG16(0x4e36);
    DG16(0x4e36) = (int16_t)bx;
    DG16(bx) = (int16_t)(size + 1);
    return (uint16_t)(bx + 4);
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
uint16_t heap_split(uint16_t bx, uint16_t size)
{
    uint16_t si, di;

    DG16(bx) = (int16_t)(DGU16(bx) - size);
    si = (uint16_t)(bx + DGU16(bx));
    di = (uint16_t)(si + size);

    DG16(si) = (int16_t)(size + 1);
    DG16(si + 2) = (int16_t)bx;
    DG16(di + 2) = (int16_t)si;
    return (uint16_t)(si + 4);
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
uint16_t heap_malloc(uint16_t want)
{
    uint16_t size, bx, start;

    if (want == 0)
        return 0;
    if ((uint16_t)(want + 5) < want)
        return 0;

    size = (uint16_t)((want + 5) & 0xfffe);
    if (size < 8)
        size = 8;

    if (DGU16(0x4e34) == 0)
        return heap_init(size);

    bx = DGU16(0x4e38);
    if (bx == 0)
        return heap_grow(size);

    start = bx;
    for (;;) {
        if (DGU16(bx) >= size)
            break;
        bx = DGU16(bx + 6);
        if (bx == start)
            return heap_grow(size);
    }

    if (DGU16(bx) >= (uint16_t)(size + 8))
        return heap_split(bx, size);

    heap_ring_unlink(bx);
    DG16(bx)++;
    return (uint16_t)(bx + 4);
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
    return (uint32_t)(a * b);
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
    uint16_t bx = DGU16(0x4e34);
    uint16_t si;
    uint16_t free_by_chain = 0;      /* CX */
    uint16_t free_by_ring = 0;       /* DX */

    if (bx == 0)
        return 1;                    /* nothing allocated yet */

    si = (uint16_t)(bx + (DGU16(bx) & 0xfffe));

    for (;;) {
        if ((DG8(bx) & 1) == 0) {
            free_by_chain = (uint16_t)(free_by_chain + DGU16(bx));
            if (bx == DGU16(0x4e36))
                break;
            if ((DG8(si) & 1) == 0)
                return -1;
        } else if (bx == DGU16(0x4e36)) {
            break;
        }

        if (si <= bx)
            return -1;
        if (DGU16(bx) < 8)
            return -1;
        if (si <= DGU16(0x4e34))
            return -1;
        if (si > DGU16(0x4e36))
            return -1;
        if (DGU16((uint16_t)(si + 2)) != bx)
            return -1;

        bx = si;
        si = (uint16_t)(bx + (DGU16(bx) & 0xfffe));
    }

    bx = DGU16(0x4e38);
    if (bx == 0)
        goto totals;

    for (;;) {
        uint16_t ax = DGU16(bx);

        if ((ax & 1) != 0)
            return -1;

        free_by_ring = (uint16_t)(free_by_ring + ax);

        if (bx < DGU16(0x4e34))
            return -1;
        if (bx >= DGU16(0x4e36))
            return -1;

        si = DGU16((uint16_t)(bx + 6));
        if (si == DGU16(0x4e38))
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
uint16_t near_memset(uint16_t dst, uint16_t count, uint16_t value)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        DG8((uint16_t)(dst + i)) = (uint8_t)value;

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
uint16_t heap_calloc(uint16_t count, uint16_t size)
{
    uint32_t n = long_multiply(count, size);
    uint16_t p;

    if (n > 0xffff)
        return 0;

    p = heap_malloc((uint16_t)n);
    if (p != 0)
        near_memset(p, (uint16_t)n, 0);

    return p;
}

/*
 * 0x0bb75
 *
 * The far-callable face of `calloc`: it takes the two words off the stack and
 * hands them straight on. Four instructions and a `retf`.
 */
uint16_t heap_calloc_far(uint16_t count, uint16_t size)
{
    return heap_calloc(count, size);
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
void far_move(uint16_t src_off, uint16_t src_seg, uint16_t dst_off,
              uint16_t dst_seg, uint16_t count)
{
    uint16_t i;

    for (i = 0; i < count; i++)
        *FAR_PTR(dst_seg, (uint16_t)(dst_off + i)) =
            *FAR_PTR(src_seg, (uint16_t)(src_off + i));
}

/*
 * 0x0bb1e
 *
 * The far-callable face of `malloc`: one argument off the stack and straight
 * on to `heap_malloc`.
 */
uint16_t heap_malloc_far(uint16_t bytes)
{
    return heap_malloc(bytes);
}

/*
 * 0x0bb2d
 *
 * The far-callable face of `free`: one argument off the stack and straight on
 * to `heap_free`. The `inc sp` twice that cleans it is two bytes shorter than
 * an `add sp,2` and does the same.
 */
void heap_free_far(uint16_t p)
{
    heap_free(p);
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
uint16_t long_to_string(uint16_t letters, uint16_t is_signed, uint16_t radix,
                        uint16_t buf, uint16_t lo, uint16_t hi)
{
    uint8_t digits[0x22];
    int16_t n = 0;
    uint32_t v;
    uint16_t out = buf;

    if (radix > 0x24 || (radix & 0xff) < 2) {
        DG8(out) = 0;
        return buf;
    }

    v = ((uint32_t)hi << 16) | lo;

    if ((int16_t)hi < 0 && (is_signed & 0xff) != 0) {
        DG8(out) = '-';
        out++;
        v = (uint32_t)(-(int32_t)v);
    }

    do {
        digits[n++] = (uint8_t)(v % radix);
        v /= radix;
    } while (v != 0);

    while (n-- > 0) {
        uint8_t d = digits[n];

        DG8(out) = (uint8_t)(d >= 10 ? (d - 10) + letters : d + '0');
        out++;
    }

    DG8(out) = 0;
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
uint16_t int_to_string(int16_t value, uint16_t buf, uint16_t radix)
{
    uint32_t v = (radix == 10) ? (uint32_t)(int32_t)value
                               : (uint32_t)(uint16_t)value;

    return long_to_string(0x61, 1, radix, buf, (uint16_t)v,
                          (uint16_t)(v >> 16));
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
uint16_t long_int_to_string(uint16_t lo, uint16_t hi, uint16_t buf,
                            uint16_t radix)
{
    return long_to_string(0x61, (uint16_t)(radix == 10), radix, buf, lo, hi);
}
