/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Saved rectangles**: the pool of records that remember what a drawn
 * object covered on each page, filing a rectangle, restoring and discarding
 * the lists, and copying them between pages.
 *
 * One module of the original's **code segment 0000** (`_TEXT`), image
 * 0x0a05f..0x0a78e. From 0x0a05f to 0x0a77d its routines reach each other
 * with bare `push cs / call`; the cursor code after it reaches 0x0a42a and
 * 0x0a6d7 through TLINK's `nop / push cs / call`, so it is another file.
 * **Its end is ours**, put before the routines that use the cursor's data.
 * **Built with Borland C++ 2.0**, like fstring.c before it: BC++ 3.0 does
 * not give `build_rect_pool`'s jumps, and 2.0 gives every routine. Its
 * `_DATA` is 0x2d06..0x2d32 - the two words at 0x2d06 are its own - and its
 * `_BSS` 0x56b6..0x56e6.
 *
 * JUDGE: compiler bc2.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* DGROUP 0x2d06..0x2d0a, after the part templates: two words. */
struct dg_2d06 {
    /* **Nothing in the port touches either word**, and the image holds no
       instruction that names 0x2d06 or 0x2d08 - so whatever reads them, if
       anything does, reaches them through a pointer. The image's own bytes are
       the initialisers below and `check_image_data` holds them to that. */
    uint16_t  _pad_2d06;       /* +0x00 */
    uint16_t  _pad_2d08;           /* +0x02 */
} PACKED;
struct dg_2d06 DG2D06 DGROUP_AT(0x2d06) = { 0x0001, 0xffff };

/*
 * **Which page pointers the saved-rect lists are restored between**, at
 * DGROUP 0x2d0a: pairs of addresses of the driver's page words at
 * 0x38a0..0x38a4 (and of the word at 0x2d08), walked by
 * `restore_saved_rect_lists` from pair 0 - or from pair 1 alone - until
 * the next pair's second word is 0. Nine pairs and the terminating pair
 * fill the run to 0x2d32.
 *
 * DGROUP 0x2d0a..0x2d32, 0x28 bytes.
 */
struct page_pair {
    dg_near_t src;                 /* +0x00  the address of a page word */
    dg_near_t dst;                 /* +0x02 */
} PACKED;

struct machine_page_pairs {
    struct page_pair pair[10];    /* +0x00 [0x28] */
} PACKED;

struct machine_page_pairs MACHINE_PAGE_PAIRS DGROUP_AT(0x2d0a) = {
    {
        { NEAR_ADDR(DG2D06._pad_2d08, 0x2d08), NEAR_ADDR(VMDS.page_front_ptr, 0x38a4) },
        { NEAR_ADDR(VMDS.page_back_ptr, 0x38a2), NEAR_ADDR(VMDS.page_front_ptr, 0x38a4) },
        { NEAR_ADDR(DG2D06._pad_2d08, 0x2d08), NEAR_ADDR(VMDS.page_back_ptr, 0x38a2) },
        { NEAR_ADDR(VMDS.rect_page, 0x38a0), NEAR_ADDR(VMDS.page_back_ptr, 0x38a2) },
        { NEAR_ADDR(DG2D06._pad_2d08, 0x2d08), NEAR_ADDR(VMDS.rect_page, 0x38a0) },
        { NEAR_ADDR(VMDS.page_front_ptr, 0x38a4), NEAR_ADDR(VMDS.rect_page, 0x38a0) },
        { NEAR_ADDR(VMDS.page_front_ptr, 0x38a4), NEAR_ADDR(VMDS.page_back_ptr, 0x38a2) },
        { NEAR_ADDR(VMDS.page_back_ptr, 0x38a2), NEAR_ADDR(VMDS.rect_page, 0x38a0) },
        { NEAR_ADDR(VMDS.rect_page, 0x38a0), NEAR_ADDR(VMDS.page_front_ptr, 0x38a4) },
    }
};

/*
 * **This module's `_BSS`, 0x56b6..0x56e4**, defined in the order that
 * reverses to the image's: Borland lays `_BSS` out in reverse order of first
 * mention. `MACHINE_RECT_FREE`'s type is in dgroup.h, for the cursor code
 * that reads it, and a type is not a mention of the object.
 */

/*
 * **The saved-rect free list, and where the cursor is to be drawn**, DGROUP
 * 0x56e0..0x56e6, 0x06 bytes. The pair is the pointer less the bitmap's hot
 * spot, worked out before the redraw and compared with the slot's own so an
 * unmoved cursor is not drawn again.
 */

struct machine_rect_free MACHINE_RECT_FREE DGROUP_BSS(0x56e0);

/*
 * **The twenty saved-rectangle slots**, DGROUP 0x56b8..0x56e0, 0x28 bytes. Each is a near
 * pointer to the head of a chain of records, or zero for an empty slot;
 * `find_saved_rect_slot` walks all twenty and `restore_saved_rect_lists`
 * counts down every record on every chain. Twenty words end at 0x56e0, where
 * the free list is.
 *
 * A slot is handed around as a pointer to its word - `find_saved_rect_slot`
 * answers one, or NULL - and the records on a chain are `struct
 * rect_list_entry`.
 */
struct machine_rect_slots {
    dg_near_t slot[0x14];         /* +0x00 [0x28] */
} PACKED;

struct machine_rect_slots MACHINE_RECT_SLOTS DGROUP_BSS(0x56b8);

/*
 * **How many saved-rectangle records the pool has been given**, DGROUP
 * 0x56b6: `build_rect_pool` adds each block's count and `rect_pool_count`
 * answers it. Nothing else names the word.
 */
struct machine_rect_count {
    uint16_t  count;              /* +0x00 */
} PACKED;

struct machine_rect_count MACHINE_RECT_COUNT DGROUP_BSS(0x56b6);

/*
 * 0x0a05f
 *
 * **Grow the saved-rect pool** by `n` records, rounded up to a multiple of
 * five, in one `calloc_far(n, 0x1a)` block threaded through `next` and
 * pushed whole on the free list. Only the block's **first** record has
 * `block_head` set - that is what `free_rect_pool` tests to hand the block
 * back in one `free_far`. The count at 0x56b6 goes up by `n`. Answers 1,
 * or 0 when the heap refuses; the `cmp di, 5` before the 1 compares and then
 * ignores the result, and is not reproduced. Called only from
 * `file_saved_rect`, which nothing calls: dead in the shipped binary.
 */
uint16_t build_rect_pool(register uint16_t n)
{
    register struct rect_list_entry *rec;
    struct rect_list_entry *base;       /* [bp-2] */
    int16_t k;                          /* [bp-4] */

    n = ((int16_t)n + 4) / 5 * 5;
    base = (struct rect_list_entry *)(void *)calloc_far(n, sizeof(struct rect_list_entry));
    if (base == NULL)
        return 0;
    rec = base;
    rec->block_head = 1;
    for (k = 1; k < (int16_t)n; k++) {
        rec->next_ptr = dg_near(dgroup, rec + 1);
        rec++;
    }
    rec->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
    MACHINE_RECT_FREE.rect_free_ptr = dg_near(dgroup, base);
    MACHINE_RECT_COUNT.count += n;
    /* a test with nothing behind it: `cmp di,5 / jne` onto the next
       instruction, as the image has it */
    if (n == 5) {
    }
    return 1;
}

/*
 * 0x0a0d7
 *
 * **File a saved rectangle on its slot** - the one routine that creates a
 * `rect_list_entry`, and **dead in the shipped binary**: its one caller is
 * `copy_saved_rects`, which nothing calls. Ten words of arguments, and they
 * are the record's fields in order, which is how the record was typed.
 *
 * A mode-4 rect (restored from `buf`) is filed under source page -1. A mode-1
 * rect (a plain copy) is first cut to the clip box when `VMDS.clip_enabled`
 * says so, or to the screen otherwise - a rect wholly outside either is
 * dropped - and then `x` and `w` become eight-pixel columns, `w` widened by
 * whatever `x` lost to the rounding. A rect with nothing left is dropped.
 *
 * The record comes off the free list, five more being built when it is
 * empty. Then, for mode 1 only, **it is merged with any rect already on the
 * chain whose union costs little**: for each, the union box and its area are
 * worked out, and if the two areas plus 0x14 cover the union, the new rect
 * becomes the union, the old one is unlinked and returned to the free list,
 * and the walk restarts from the head, stopping where it had got to - the
 * `stop` and `from` slots below. Finally the record goes on the head of the
 * chain. The walk is written with the original's own jumps, because its
 * restart has no tidier spelling that is provably the same.
 */
void file_saved_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t mode, dg_seg_t page_src, dg_seg_t page_dst,
                     uint16_t refcount, uint8_t far * buf)
{
    register dg_near_t rec;
    register dg_near_t other;
    dg_near_t *slot;                    /* [bp-2] */
    dg_near_t stop;                      /* [bp-4] */
    dg_near_t prev;                      /* [bp-6] */
    dg_near_t after;                     /* [bp-8] */
    dg_near_t from;                      /* [bp-0xa] */
    int16_t area;                       /* [bp-0xc] */
    int16_t sum;                        /* [bp-0xe] */
    int16_t ux0;                        /* [bp-0x10] */
    int16_t ux1;                        /* [bp-0x12] */
    int16_t uy0;                        /* [bp-0x14] */
    int16_t uy1;                        /* [bp-0x16] */

    if (mode == 4)
        page_src = 0xffff;
    slot = find_saved_rect_slot(page_src, page_dst, refcount);
    if (slot == NULL)
        return;

    if (mode == 1) {
        if (VMDS.clip_enabled != 0) {
            /* a bitwise or of the four, each tested for its value */
            if ((x > VMDS.clip_right) | ((int16_t)(x + w) < VMDS.clip_left)
                | (y > VMDS.clip_bottom) | ((int16_t)(y + h) < VMDS.clip_top))
                return;
            if (x < VMDS.clip_left) {
                w -= VMDS.clip_left - x;
                x = VMDS.clip_left;
            }
            if (y < VMDS.clip_top) {
                h -= VMDS.clip_top - y;
                y = VMDS.clip_top;
            }
            if ((int16_t)(x + w - 1) > VMDS.clip_right)
                w = VMDS.clip_right - x + 1;
            if ((int16_t)(y + h - 1) > VMDS.clip_bottom)
                h = VMDS.clip_bottom - y + 1;
        } else {
            if (VMDS.screen.screen_width - 1 < x || (int16_t)(x + w) < 0
                || VMDS.screen.screen_height - 1 < y || (int16_t)(y + h) < 0)
                return;
            if (x < 0) {
                w -= 0 - x;
                x = 0;
            }
            if (y < 0) {
                h -= 0 - y;
                y = 0;
            }
            if ((int16_t)(x + w - 1) > VMDS.screen.screen_width - 1)
                w = VMDS.screen.screen_width - 1 - x + 1;
            if ((int16_t)(y + h - 1) > VMDS.screen.screen_height - 1)
                h = VMDS.screen.screen_height - 1 - y + 1;
        }
        w = (w + x % 8 + 7) / 8;
        x = x / 8;
    }

    if (w == 0 || h == 0)
        return;
    if (MACHINE_RECT_FREE.rect_free_ptr == 0 && !build_rect_pool(5))
        return;

    rec = MACHINE_RECT_FREE.rect_free_ptr;
    MACHINE_RECT_FREE.rect_free_ptr = RECTENT_PTR(rec)->next_ptr;
    RECTENT_PTR(rec)->next_ptr = 0;
    RECTENT_PTR(rec)->x = x;
    RECTENT_PTR(rec)->y = y;
    RECTENT_PTR(rec)->w = w;
    RECTENT_PTR(rec)->h = h;
    RECTENT_PTR(rec)->mode = mode;
    RECTENT_PTR(rec)->page_src = page_src;
    RECTENT_PTR(rec)->page_dst = page_dst;
    RECTENT_PTR(rec)->refcount = refcount;
    RECTENT_PTR(rec)->buf = buf;
    RECTENT_PTR(rec)->area = w * h;

    if (mode == 1) {
        other = *slot;
        from = stop = 0;
        prev = 0;
        while (other != stop) {
            stop = from;
            after = RECTENT_PTR(other)->next_ptr;
            sum = RECTENT_PTR(other)->area + RECTENT_PTR(rec)->area;
            ux0 = RECTENT_PTR(other)->x < RECTENT_PTR(rec)->x
                  ? RECTENT_PTR(other)->x : RECTENT_PTR(rec)->x;
            ux1 = (int16_t)(RECTENT_PTR(other)->x + RECTENT_PTR(other)->w)
                      > (int16_t)(RECTENT_PTR(rec)->x + RECTENT_PTR(rec)->w)
                  ? RECTENT_PTR(other)->x + RECTENT_PTR(other)->w
                  : RECTENT_PTR(rec)->x + RECTENT_PTR(rec)->w;
            uy0 = RECTENT_PTR(other)->y < RECTENT_PTR(rec)->y
                  ? RECTENT_PTR(other)->y : RECTENT_PTR(rec)->y;
            uy1 = (int16_t)(RECTENT_PTR(other)->y + RECTENT_PTR(other)->h)
                      > (int16_t)(RECTENT_PTR(rec)->y + RECTENT_PTR(rec)->h)
                  ? RECTENT_PTR(other)->y + RECTENT_PTR(other)->h
                  : RECTENT_PTR(rec)->y + RECTENT_PTR(rec)->h;
            area = (ux1 - ux0) * (uy1 - uy0);
            if ((int16_t)(sum + 0x14) >= area) {
                RECTENT_PTR(rec)->x = ux0;
                RECTENT_PTR(rec)->y = uy0;
                RECTENT_PTR(rec)->w = ux1 - ux0;
                RECTENT_PTR(rec)->h = uy1 - uy0;
                RECTENT_PTR(rec)->area = area;
                if (prev != 0)
                    RECTENT_PTR(prev)->next_ptr = after;
                else
                    *slot = after;
                RECTENT_PTR(other)->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
                MACHINE_RECT_FREE.rect_free_ptr = other;
                from = after;
                other = stop = prev;
            }
            prev = other;
            other = after;
            if (other == 0 && stop != 0) {
                other = *slot;
                prev = 0;
            }
        }
    }
    RECTENT_PTR(rec)->next_ptr = *slot;
    *slot = rec;
}

/*
 * 0x0a42a
 *
 * Put back the saved rectangles for **a list of page-and-size pairs**, and
 * then, on one of the two paths, take one off every remaining record's +0xe.
 *
 * The argument picks which table to walk: non-zero takes the one at DGROUP
 * 0x2d0e and **stops after a single entry**, zero takes the one at 0x2d0a and
 * walks it until an entry whose second word is null. The two share the loop,
 * and the test at the bottom is what makes one of them a loop and the other a
 * single pass.
 *
 * Each entry is two near pointers, four bytes apart, to the driver's page
 * words at 0x38a0..0x38a4; the segments read *through* them are handed to
 * `restore_saved_rects` as the source and destination page, with a refcount
 * of zero.
 *
 * The copy's source and destination pages, 0x38a6 and 0x38a8, are saved on the
 * way in and put back at the end, because `restore_saved_rects` sets them from
 * the first record it finds and would otherwise leave them wherever the last
 * list went.
 *
 * The count pass only runs on the zero path, over all twenty slots at 0x56b8
 * and every record on each chain. It decrements each record's `refcount` -
 * the word at +0xe, which `find_saved_rect_slot` matches for equality and
 * the creator at 0x0a0d7 files from its eighth argument. An earlier reading
 * here had the two disagreeing about what the word was; the creator settles
 * it.
 */
void restore_saved_rect_lists(int16_t which)
{
    register const struct page_pair *si;
    register dg_near_t di;
    dg_near_t *slot;                    /* [bp-2] */
    int16_t left;                       /* [bp-4] */
    dg_seg_t saved_src;                 /* [bp-6] */
    dg_seg_t saved_dst;                 /* [bp-8] */

    saved_src = VMDS.page_src_ptr;
    saved_dst = VMDS.page_dst_ptr;
    si = which != 0 ? &MACHINE_PAGE_PAIRS.pair[1] : &MACHINE_PAGE_PAIRS.pair[0];
    while (si->dst != 0) {
        restore_saved_rects(*(const dg_seg_t *)dg_near_ptr(si->src),
                            *(const dg_seg_t *)dg_near_ptr(si->dst), 0);
        si++;
        if (which != 0)
            break;
    }
    VMDS.page_src_ptr = saved_src;
    VMDS.page_dst_ptr = saved_dst;
    if (which != 0)
        return;
    for (slot = &MACHINE_RECT_SLOTS.slot[0], left = 0x14; left != 0;
         slot++, left--)
        if ((di = *slot) != 0)
            for (; di != 0; di = RECTENT_PTR(di)->next_ptr)
                RECTENT_PTR(di)->refcount--;
}

/*
 * 0x0a4bf
 *
 * **Discard every saved rect**: each of the twenty slots' chains is walked to
 * its last record, which is pointed at the free list, and the whole chain is
 * then the free list and the slot is empty. Reached only from
 * `free_rect_pool`, which nothing calls: dead in the shipped binary.
 */
void discard_saved_rects(void)
{
    register dg_near_t *slot;
    register dg_near_t rec;
    int16_t left;

    for (slot = &MACHINE_RECT_SLOTS.slot[0], left = 0x14; left != 0; slot++, left--)
        if ((rec = *slot) != 0) {
            while (RECTENT_PTR(rec)->next_ptr != 0)
                rec = RECTENT_PTR(rec)->next_ptr;
            RECTENT_PTR(rec)->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
            MACHINE_RECT_FREE.rect_free_ptr = *slot;
            *slot = 0;
        }
}

/*
 * 0x0a4f9
 *
 * **Is a box covered by a saved mode-1 rect** on the slot whose head carries
 * this destination page and refcount? The box's `x` and `w` are turned into
 * eight-pixel columns the way `file_saved_rect` turns them, then every slot's
 * head is tested and the matching chain walked; the first mode-1 rect that
 * overlaps answers its mode, which is 1, and nothing answers 0. Nothing in
 * the image calls it: dead in the shipped binary.
 */
uint16_t saved_rect_covers(register int16_t x, int16_t y, register int16_t w,
                           int16_t h, dg_seg_t page_dst, uint16_t refcount)
{
    register dg_near_t rec;
    dg_near_t *slot;                    /* [bp-2] */
    int16_t left;                       /* [bp-4] */

    /* the width in bytes, in the width's own register */
    w = (w + x % 8 + 7) / 8;
    x = x / 8;
    for (slot = &MACHINE_RECT_SLOTS.slot[0], left = 0x14; left != 0; slot++, left--)
        if ((rec = *slot) != 0 && RECTENT_PTR(rec)->page_dst == page_dst
            && RECTENT_PTR(rec)->refcount == refcount)
            for (; rec != 0; rec = RECTENT_PTR(rec)->next_ptr)
                if (RECTENT_PTR(rec)->mode == 1
                    && RECTENT_PTR(rec)->x < (int16_t)(x + w)
                    && (int16_t)(RECTENT_PTR(rec)->x + RECTENT_PTR(rec)->w) > x
                    && RECTENT_PTR(rec)->y < (int16_t)(y + h)
                    && (int16_t)(RECTENT_PTR(rec)->y + RECTENT_PTR(rec)->h) > y)
                    return RECTENT_PTR(rec)->mode;
    return 0;
}

/*
 * 0x0a5a1
 *
 * **Free the saved-rect pool.** Everything is discarded onto the free list
 * first, and then the list is walked for a block head - the first record of
 * each `calloc_far` block, marked by `build_rect_pool`. On finding one
 * the mark is cleared, **the routine calls itself** - which walks on past
 * the cleared mark and frees every later block first - and then this block
 * is freed in one `free_far` and the list head zeroed. That recursion
 * is the original's, at 0x0a5bc, and is kept. Nothing in the image calls
 * this: dead in the shipped binary.
 */
void free_rect_pool(void)
{
    dg_near_t rec;

    discard_saved_rects();

    for (rec = MACHINE_RECT_FREE.rect_free_ptr; rec != 0; rec = RECTENT_PTR(rec)->next_ptr) {
        if ((RECTENT_PTR(rec)->block_head & 1) != 0) {
            RECTENT_PTR(rec)->block_head = 0;
            free_rect_pool();
            free_far(dg_near_ptr(rec));
            break;
        }
    }

    MACHINE_RECT_FREE.rect_free_ptr = 0;
}

/*
 * 0x0a5d8
 *
 * How many records the pool holds. Nothing in the image calls it.
 */
uint16_t rect_pool_count(void)
{
    return MACHINE_RECT_COUNT.count;
}

/*
 * 0x0a5e2
 *
 * Find the slot in the table of **twenty saved-rectangle objects** at DGROUP
 * 0x56b8 that already holds a given page, width and height - or, failing that,
 * the first empty slot.
 *
 * Each slot is a word: a near pointer to a record, or zero. A record matches
 * when its page at +0xe, its width at +8 and its height at +0xa are all the
 * ones asked for. The answer is **the slot**, not the record, so a caller can
 * put a new record into it.
 *
 * The first empty slot is remembered as the walk goes past it - `or dx,dx`
 * keeps the *first* one rather than the last - and is what comes back when
 * nothing matched. A full table with no match answers 0, which is also what an
 * empty slot's own contents look like, so the two are told apart by the caller
 * looking at what the slot holds rather than by the answer.
 */
dg_near_t *find_saved_rect_slot(dg_seg_t page_src, dg_seg_t page_dst,
                                        uint16_t refcount)
{
    register dg_near_t *slot;
    register dg_near_t rec;
    dg_near_t *empty;
    int16_t left;

    for (slot = &MACHINE_RECT_SLOTS.slot[0], empty = NULL, left = 0x14; left != 0;
         slot++, left--)
        if ((rec = *slot) != 0) {
            if (RECTENT_PTR(rec)->refcount == refcount
                && RECTENT_PTR(rec)->page_src == page_src
                && RECTENT_PTR(rec)->page_dst == page_dst)
                return slot;
        } else if (empty == NULL)
            empty = slot;
    return empty;
}

/*
 * 0x0a62c
 *
 * **Put back everything saved for one page and size**, then give the records
 * away.
 *
 * The slot comes from `find_saved_rect_slot`, and an empty slot or an empty
 * list does nothing at all - not even the page switch below. The first record
 * sets the copy's source and destination pages, at DGROUP 0x38a6 and 0x38a8,
 * from its own +8 and +0xa; every record after it is restored between the same
 * two pages, so a list is only ever built for one pair.
 *
 * Each record is restored one of two ways, by the kind at +0xc:
 *
 *   1  a page-to-page copy of the rectangle, through `copy_rect_thunk`
 *   4  a restore from a saved buffer, through `restore_rect_thunk`, the far
 *      pointer at +0x14
 *
 * and any other kind is skipped in silence, which is how a record can be
 * parked in the list without being drawn.
 *
 * **x and width are in bytes and y and height in pixels.** The two `<< 3`s
 * turn +0 and +4 into pixels for the copy; +2 and +6 are passed through as
 * they stand. That asymmetry is the planar layout showing through - a byte is
 * eight pixels across and one pixel down.
 *
 * The whole chain then goes onto the free list at 0x56e0 in one splice, using
 * the last record the walk saw rather than walking it again.
 */
void restore_saved_rects(dg_seg_t page_src, dg_seg_t page_dst, uint16_t refcount)
{
    register struct rect_list_entry *rec;
    register dg_near_t *slot;
    struct rect_list_entry *last;       /* [bp-2] */
    int16_t x;                          /* [bp-4] */
    int16_t rw;                         /* [bp-6] */

    slot = find_saved_rect_slot(page_src, page_dst, refcount);
    if (slot == NULL)
        return;
    if ((rec = RECTENT_PTR(*slot)) != RECTENT_NONE) {
        VMDS.page_src_ptr = rec->page_src;
        VMDS.page_dst_ptr = rec->page_dst;
        while (rec != RECTENT_NONE) {
            x = rec->x << 3;
            rw = rec->w << 3;
            if (rec->mode == 1)
                copy_rect_thunk(x, rec->y, rw, rec->h);
            else if (rec->mode == 4)
                restore_rect_thunk(rec->buf, rec->x, rec->y, rec->w, rec->h);
            last = rec;
            rec = RECTENT_PTR(rec->next_ptr);
        }
        last->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
        MACHINE_RECT_FREE.rect_free_ptr = *slot;
        *slot = 0;
    }
}

/*
 * 0x0a6d7
 *
 * Give back every saved rectangle held for one page and size: find the slot,
 * walk its chain of records to the end through the links at +0x18, and put the
 * whole chain onto the free list at DGROUP 0x56e0 in one move rather than one
 * record at a time. The slot is then cleared.
 *
 * A slot that does not exist, or holds nothing, is left alone. The list is
 * pushed on the front, so the freed records come back in the reverse of the
 * order they were taken - which nothing depends on, but it is what happens.
 */
void free_saved_rects(dg_seg_t page_src, dg_seg_t page_dst, uint16_t refcount)
{
    register dg_near_t *slot;
    register dg_near_t rec;

    slot = find_saved_rect_slot(page_src, page_dst, refcount);
    if (slot != NULL && *slot != 0) {
        for (rec = *slot; RECTENT_PTR(rec)->next_ptr != 0;
             rec = RECTENT_PTR(rec)->next_ptr)
            ;
        RECTENT_PTR(rec)->next_ptr = MACHINE_RECT_FREE.rect_free_ptr;
        MACHINE_RECT_FREE.rect_free_ptr = *slot;
        *slot = 0;
    }
}

/*
 * 0x0a717
 *
 * **Copy one slot's saved rects onto another**: the two slots are found by
 * their page pairs and refcounts, and if they differ and the first has a
 * chain, each rect on it is filed again - `x` and `w` back from columns to
 * pixels - under the second's page pair and refcount, with no buffer.
 *
 * **As compiled, the loop never advances.** Its step, `mov si, [si+0x18]` at
 * 0x0a787, sits after the exit test at 0x0a783 and is reached only once `si`
 * is already 0, so a non-empty chain re-files its first rect for ever. That
 * is transcribed as it is, because nothing in the image calls this routine -
 * no near or far call, no occurrence of its address as data, and the code
 * map from the entry point never reaches it - so no run ever met the defect.
 */
void copy_saved_rects(dg_seg_t from_src, dg_seg_t from_dst, uint16_t from_ref,
                      dg_seg_t to_src, dg_seg_t to_dst, uint16_t to_ref)
{
    register dg_near_t rec;
    register dg_near_t *from_slot;

    from_slot = find_saved_rect_slot(from_src, from_dst, from_ref);
    if (find_saved_rect_slot(to_src, to_dst, to_ref) == from_slot)
        return;
    if (from_slot != NULL && *from_slot != 0) {
        rec = *from_slot;
        while (rec != 0)
            file_saved_rect(RECTENT_PTR(rec)->x << 3, RECTENT_PTR(rec)->y,
                            RECTENT_PTR(rec)->w << 3, RECTENT_PTR(rec)->h,
                            RECTENT_PTR(rec)->mode, to_src, to_dst, to_ref,
                            FAR_NULL_PTR);
        /* the step, after the loop: see above */
        rec = RECTENT_PTR(rec)->next_ptr;
    }
}
