/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Filled polygons and rectangles**: `draw_polygon`, the edge walkers it
 * builds its span list with, `fill_rect`, and the thunk into
 * `draw_compressed_body`.
 *
 * The original's **code segment 1c25**, image 0x1eded..0x20189, split out of
 * engine.c on 2026-09-27. All of it is hand-written - the edge walkers pass
 * everything in registers and answer in them, `fill_rect` pushes its own
 * arguments and pops them back - so **no C compiler judges this file**; it
 * is the host's transcription, and the byte-exact source is TASM's to make
 * (not written yet). The polygon's data is DGROUP 0x44d0..0x44ea.
 *
 * **Possibly more than one module.** Its start is the palette module's end,
 * which is C. `fill_rect` ends in a padding `nop` at 0x20184, which looks like
 * the end of a module, but nothing proves it, and nothing says which side
 * of that the thunk at 0x20185 is on. Its end is the C body the thunk jumps to.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * **The polygon walker's two chains**, DGROUP 0x44d0..0x44de, 0x0e bytes.
 *
 * `draw_polygon` finds the topmost and the bottommost vertex, and the ring
 * between them is two chains - right from one to the other and left back
 * again. `top_at` and `bottom_at` are those two vertices as byte offsets into
 * the working arrays; `right_count` and `left_count` are how many points each
 * chain got; `at` is the cursor stepping through them an edge at a time and
 * `remaining` the count parked while an edge is drawn, because the original
 * needs the register.
 */
struct engine_polygon_chains {
    uint16_t  top_at;          /* +0x00 [2] */
    uint16_t  bottom_at;          /* +0x02 [2] */
    uint16_t  right_count;          /* +0x04 [2] */
    uint16_t  left_count;          /* +0x06 [2] */
    uint16_t  remaining;          /* +0x08 [2] */
    uint16_t  at;          /* +0x0a [2] */
    uint16_t  chain;              /* +0x0c [2]  0 is the left chain and 2 the right; a computed jmp on it */
} PACKED;

struct engine_polygon_chains ENGINE_POLYGON_CHAINS DGROUP_AT(0x44d0);

/*
 * **The polygon walker's own state**, DGROUP 0x44de..0x44ea, 0x0c bytes.
 *
 * `prev_x`/`prev_y` are the vertex before this one, which is how a repeated
 * point is dropped; `span_seg` is the span buffer's segment; `span_step` is
 * what `poly_walk` adds to its cursor beside the two, so one chain's spans
 * land in the buffer's left column and the other's in the right.
 *
 * The outline is a second pass: when its colour differs from the fill,
 * `outline_count` points of the ring are kept in `closed_x`/`closed_y` to be
 * drawn after; `second_pass` and `second_count` are the other copy, the one
 * kept when the two orderings came out exactly equal.
 */
struct engine_polygon_state {
    int16_t   prev_x;          /* +0x00 [2] */
    int16_t   prev_y;          /* +0x02 [2] */
    uint16_t  span_seg;          /* +0x04 [2] */
    uint16_t  outline_count;          /* +0x06 [2] */
    uint16_t  second_count;          /* +0x08 [2] */
    uint8_t   span_step;          /* +0x0a [1] */
    uint8_t   second_pass;          /* +0x0b [1] */
} PACKED;

struct engine_polygon_state ENGINE_POLYGON_STATE DGROUP_AT(0x44de);

/*
 * 172c:2b9d, image 0x1eded
 *
 * Fill a polygon, and outline it if the two colours differ.
 *
 * Six DGROUP arrays do the work: 0x393c and 0x3964 hold the points as given,
 * 0x398c and 0x39b4 the ones actually used, and 0x39dc and 0x3a04 a closed copy
 * kept for the outline pass - the fill destroys the working pair.
 *
 * With fewer than three points, or with filling off at DGROUP 0x389c, there is
 * nothing to fill and it draws the outline and stops.
 *
 * The fill itself is the classic one. The points are walked backwards, dropping
 * any that repeat the last, and the topmost and bottommost are found on the way
 * - by y, and by x when two share a y, which is what makes the choice
 * unambiguous. If those two turn out to have the same y the whole thing is one
 * horizontal line and goes straight to `clip_and_draw_line`.
 *
 * Otherwise the two edges leaving the top vertex are compared to see which way
 * round the polygon is wound - by slope, and the comparison is done with two
 * divisions rather than a cross product so it cannot overflow - and the arrays
 * are reversed if it is the wrong way. Then the outline is split into a left
 * chain and a right chain, each walked edge by edge into a buffer of span ends,
 * and the whole buffer handed to the driver's span filler in one call.
 *
 * This routine is hand-written assembly - BP is a general register throughout,
 * and most of its 3,674 bytes are an unrolled loop entered by computed jump -
 * so the port follows its registers rather than pretending it was compiled.
 */
void draw_polygon(int16_t n, const int16_t *xs, const int16_t *ys)
{
    uint16_t seg;
    /* The span buffer's first byte. The edge routines step a 16-bit offset
       inside it, exactly as the original steps `di` against a segment, so the
       base and the offset stay apart; `seg` itself is still filed at
       0x44e2 below. */
    uint8_t far * span;
    int16_t ax, bx, cx, dx, si, di, bp;
    int16_t i;

    ENGINE_POLYGON_STATE.span_seg = 0;
    ENGINE_POLYGON_STATE.second_pass = 0;

    if (n >= 0) {
        VMDS.palettes.clip_count = (uint16_t)n;
        for (i = 0; i < n; i++) {
            VMDS.poly_x[i] = xs[i];
            VMDS.poly_y[i] = ys[i];
        }
    }

    if (n < 2)
        goto out;

    if (n == 2) {
        poly_outline(VMDS.poly_x, VMDS.poly_y, 1);
        goto out;
    }

    if (VMDS.fill_enabled == 0) {
        /* Filling is off: close the ring and draw it as lines. */
        n = (int16_t)VMDS.palettes.clip_count;
        VMDS.poly_x[n] = ((uint16_t)VMDS.poly_x[0]);
        VMDS.poly_y[n] = ((uint16_t)VMDS.poly_y[0]);
        poly_outline(VMDS.poly_x, VMDS.poly_y, n);
        goto out;
    }

    if (VMDS.second_colour != VMDS.fill_colour) {
        n = (int16_t)VMDS.palettes.clip_count;
        ENGINE_POLYGON_STATE.outline_count = (uint16_t)n;

        for (i = 0; i < n; i++) {
            VMDS.closed_x[i] = ((uint16_t)VMDS.poly_x[i]);
            VMDS.closed_y[i] = ((uint16_t)VMDS.poly_y[i]);
        }
        VMDS.closed_x[n] = ((uint16_t)VMDS.poly_x[0]);
        VMDS.closed_y[n] = ((uint16_t)VMDS.poly_y[0]);
    }

    if (VMDS.clip_enabled != 0)
        clip_polygon();

    n = (int16_t)VMDS.palettes.clip_count;
    if (n < 2)
        goto out;
    if (n == 2) {
        poly_outline(VMDS.poly_x, VMDS.poly_y, 1);
        goto out;
    }

    si = (int16_t)((n - 1) * 2);
    ENGINE_POLYGON_STATE.prev_y = ((uint16_t)VMDS.poly_y[0]);
    dx = 0x7fff;
    bx = (int16_t)0x8001;
    ENGINE_POLYGON_STATE.prev_x = ((uint16_t)VMDS.poly_x[0]);
    bp = dx;
    cx = bx;
    di = 0;
    ENGINE_POLYGON_CHAINS.top_at = 0;
    ENGINE_POLYGON_CHAINS.bottom_at = 0;

    for (; si >= 0; si -= 2) {
        ax = VMDS.poly_y[si >> 1];

        if (ax == ENGINE_POLYGON_STATE.prev_y
            && VMDS.poly_x[si >> 1] == ENGINE_POLYGON_STATE.prev_x)
            continue;

        ENGINE_POLYGON_STATE.prev_y = ax;
        VMDS.work_y[di >> 1] = ax;

        /*
         * The tie-breaks go opposite ways, and which way is not a matter of
         * taste: the topmost keeps the point *further* right of two on the
         * same row and the bottommost the one further left, so the two chains
         * leave the vertices from opposite corners. Reading either the other
         * way round picks a different vertex to start from, and the fill can
         * still come out right - it did, pixel for pixel - while the arrays
         * the routine leaves behind do not match, which is how it was caught.
         */
        if (ax < dx
            || (ax == dx && VMDS.poly_x[si >> 1] > cx)) {
            ENGINE_POLYGON_CHAINS.top_at = (uint16_t)di;
            dx = ax;
            cx = VMDS.poly_x[si >> 1];
        }

        if (ax > bx
            || (ax == bx && VMDS.poly_x[si >> 1] <= bp)) {
            ENGINE_POLYGON_CHAINS.bottom_at = (uint16_t)di;
            bx = ax;
            bp = VMDS.poly_x[si >> 1];
        }

        ax = VMDS.poly_x[si >> 1];
        ENGINE_POLYGON_STATE.prev_x = ax;
        VMDS.work_x[di >> 1] = ax;
        di += 2;
    }

    if (dx == bx) {
        /* Every point on one row: one line, and nothing to fill. */
        if (VMDS.screen.mode_kind == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top >> 1);
            VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top << 1);
            VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom << 1);
        }
        goto out;
    }

    ax = (int16_t)((uint16_t)di >> 1);
    if (ax < 2)
        goto out;

    if (ax == 2) {
        if (VMDS.screen.mode_kind == 0) {
            clip_and_draw_line(bp, bx, cx, dx);
        } else {
            VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top >> 1);
            VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom >> 1);
            clip_and_draw_line(bp, (int16_t)(bx >> 1), cx,
                               (int16_t)(dx >> 1));
            VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top << 1);
            VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom << 1);
        }
        goto out;
    }

    cx = di;
    VMDS.palettes.clip_count = (uint16_t)ax;

    /*
     * Which way round is it wound? Compare the slopes of the two edges leaving
     * the top vertex. A zero rise is turned into one with a huge run so the
     * comparison still means something, and the two slopes are compared as
     * quotient-then-remainder rather than by cross-multiplying, because the
     * product would not fit.
     */
    si = (int16_t)ENGINE_POLYGON_CHAINS.top_at;
    di = (int16_t)(si + 2);
    if (di >= cx)
        di = 0;

    dx = (int16_t)(VMDS.work_x[di >> 1] - VMDS.work_x[si >> 1]);
    bp = (int16_t)(VMDS.work_y[di >> 1] - VMDS.work_y[si >> 1]);
    if (bp == 0) {
        bp = 1;
        dx = (dx >= 0) ? 0x7fff : (int16_t)-0x7fff;
    }

    di = (int16_t)(si - 2);
    if (di < 0)
        di = (int16_t)(di + cx);

    ax = (int16_t)(VMDS.work_x[di >> 1] - VMDS.work_x[si >> 1]);
    bx = (int16_t)(VMDS.work_y[di >> 1] - VMDS.work_y[si >> 1]);
    if (bx == 0) {
        bx = 1;
        if (ax < 0) {
            ax = (int16_t)0x8001;
            goto ax_negative;
        }
        ax = (int16_t)-(int16_t)0x8001;
    }

    if (ax < 0)
        goto ax_negative;

    if (dx <= 0)
        goto reverse;
    goto compare;

ax_negative:
    if (dx >= 0)
        goto keep;

    dx = (int16_t)-dx;
    ax = (int16_t)-ax;
    {
        int16_t t = dx;

        dx = ax;
        ax = t;
        t = bx;
        bx = bp;
        bp = t;
    }

compare:
    /*
     * Each edge is a rise over a run and each keeps its own pair: `dx` goes
     * with `bp`, `ax` with `bx`, and the `xchg` above swaps the two pairs
     * whole rather than breaking them up. Dividing one edge's rise by the
     * other's run compares nothing, and the winding then comes out backwards
     * on the polygons where the two slopes happen to straddle - which is a
     * fill built from the wrong chains.
     */
    {
        uint16_t q2 = (uint16_t)ax / (uint16_t)bx;
        uint16_t r2 = (uint16_t)ax % (uint16_t)bx;
        uint16_t q1 = (uint16_t)dx / (uint16_t)bp;
        uint16_t r1 = (uint16_t)dx % (uint16_t)bp;

        if (q1 > q2)
            goto keep;
        if (q1 < q2)
            goto reverse;

        /*
         * Equal whole parts, so the remainders decide - each shifted up a
         * word and divided by its own run again, which is the original's way
         * of getting another sixteen bits of the quotient without a 32-bit
         * divide it has no instruction for.
         */
        {
            uint16_t f1 = (uint16_t)(((uint32_t)r1 << 16) / (uint16_t)bp);
            uint16_t f2 = (uint16_t)(((uint32_t)r2 << 16) / (uint16_t)bx);

            if (f1 < f2)
                goto reverse;
            if (f1 > f2)
                goto keep;
        }
    }

    /* Exactly equal: keep a copy for a second pass and go on. */
    ENGINE_POLYGON_STATE.second_pass = 1;
    ENGINE_POLYGON_STATE.second_count = (uint16_t)cx;
    for (i = 0; i < cx; i += 2) {
        VMDS.closed_x[i >> 1] = VMDS.work_x[i >> 1];
        VMDS.closed_y[i >> 1] = VMDS.work_y[i >> 1];
    }

keep:
    for (i = 0; i < cx; i += 2) {
        VMDS.poly_x[i >> 1] = VMDS.work_x[i >> 1];
        VMDS.poly_y[i >> 1] = VMDS.work_y[i >> 1];
    }
    goto chains;

reverse:
    for (i = 0; i < cx; i += 2) {
        VMDS.poly_x[(cx - 2 - i) >> 1] = VMDS.work_x[i >> 1];
        VMDS.poly_y[(cx - 2 - i) >> 1] = VMDS.work_y[i >> 1];
    }
    ENGINE_POLYGON_CHAINS.top_at = (uint16_t)(cx - 2 - (int16_t)ENGINE_POLYGON_CHAINS.top_at);
    ENGINE_POLYGON_CHAINS.bottom_at = (uint16_t)(cx - 2 - (int16_t)ENGINE_POLYGON_CHAINS.bottom_at);

chains:
    /* The right chain: from the bottom vertex up to the top. */
    dx = VMDS.poly_y[ENGINE_POLYGON_CHAINS.bottom_at >> 1];
    si = (int16_t)ENGINE_POLYGON_CHAINS.top_at;
    di = 0;
    for (;;) {
        VMDS.work_x[di >> 1] = VMDS.poly_x[si >> 1];
        ax = VMDS.poly_y[si >> 1];
        VMDS.work_y[di >> 1] = ax;
        di += 2;
        if (ax >= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    ENGINE_POLYGON_CHAINS.right_count = (uint16_t)((uint16_t)di >> 1);

    /* The left chain: from the top vertex down to the bottom. */
    dx = VMDS.poly_y[ENGINE_POLYGON_CHAINS.top_at >> 1];
    si = (int16_t)ENGINE_POLYGON_CHAINS.bottom_at;
    for (;;) {
        VMDS.work_x[di >> 1] = VMDS.poly_x[si >> 1];
        ax = VMDS.poly_y[si >> 1];
        VMDS.work_y[di >> 1] = ax;
        di += 2;
        if (ax <= dx)
            break;
        si += 2;
        if (si >= cx)
            si = 0;
    }
    ENGINE_POLYGON_CHAINS.left_count = (uint16_t)(((uint16_t)di >> 1) - ENGINE_POLYGON_CHAINS.right_count);

    seg = DG4342.span_buffer_seg;
    span = MK_FP(seg, 0);

    ENGINE_POLYGON_CHAINS.chain = 2;
    ENGINE_POLYGON_CHAINS.at = 0;
    ax = (int16_t)ENGINE_POLYGON_CHAINS.right_count;

    for (;;) {
        ax--;
        if (ax == 0) {
            if (ENGINE_POLYGON_CHAINS.chain != 0) {
                ENGINE_POLYGON_CHAINS.at += 2;
                ENGINE_POLYGON_CHAINS.chain = 0;
                ax = (int16_t)ENGINE_POLYGON_CHAINS.left_count;
                continue;
            }
            break;
        }

        ENGINE_POLYGON_CHAINS.remaining = (uint16_t)ax;

        si = (int16_t)ENGINE_POLYGON_CHAINS.at;
        ENGINE_POLYGON_CHAINS.at = (uint16_t)(si + 2);

        {
            int16_t x1 = VMDS.work_x[si >> 1];
            int16_t x2 = VMDS.work_x[(si >> 1) + 1];
            int16_t y1 = VMDS.work_y[si >> 1];
            int16_t y2 = VMDS.work_y[(si >> 1) + 1];
            int16_t adx = (int16_t)(x1 - x2);
            int16_t ady;

            if (adx < 0)
                adx = (int16_t)-adx;

            if (adx == 0) {
                poly_edge_vertical(span, x1, y1, y2);
            } else {
                ady = (int16_t)(y1 - y2);
                if (ady < 0)
                    ady = (int16_t)-ady;

                if (ady == 0) {
                    /* One row: write whichever end the side wants. */
                    int16_t lo = (x1 < x2) ? x1 : x2;
                    int16_t hi = (x1 < x2) ? x2 : x1;
                    uint16_t at = (uint16_t)((y1 << 2) + ENGINE_POLYGON_CHAINS.chain);

                    *(int16_t *)(void *)(span + at) =
                        (ENGINE_POLYGON_CHAINS.chain == 0) ? lo : hi;
                } else if (adx < ady) {
                    poly_edge_steep(span, x1, x2, y1, y2);
                } else if (adx > ady) {
                    /* `cmp [0x44dc],0; jne 0x1f3e6; je 0x1f4a1`. */
                    if (ENGINE_POLYGON_CHAINS.chain != 0)
                        poly_edge_shallow_right(span, x1, x2, y1, y2);
                    else
                        poly_edge_shallow_left(span, x1, x2, y1, y2);
                } else {
                    poly_edge_diagonal(span, x1, x2, y1, y2);
                }
            }
        }

        ax = (int16_t)ENGINE_POLYGON_CHAINS.remaining;
    }

    /* Hand the whole buffer to the driver's span filler in one call. */
    {
        int16_t top = VMDS.poly_y[ENGINE_POLYGON_CHAINS.top_at >> 1];
        int16_t bottom = VMDS.poly_y[ENGINE_POLYGON_CHAINS.bottom_at >> 1];
        /* The list starts four words before the first row's pair: the
           first row and the row count, in the segment below `seg`. */
        /* The paragraph below the buffer - `seg - 1` - is where the list's
           own header lives. */
        uint8_t *spans = span - 0x10 + (uint16_t)((top << 2) + 0x0c);
        int16_t rows = (int16_t)(bottom - top + 1);

        ENGINE_POLYGON_STATE.span_seg = seg;

        spans[0] = (uint8_t)top;
        spans[1] = (uint8_t)((uint16_t)top >> 8);
        spans[2] = (uint8_t)rows;
        spans[3] = (uint8_t)((uint16_t)rows >> 8);

        vm_fill_spans(spans);
    }

    if (VMDS.second_colour != VMDS.fill_colour)
        poly_outline(VMDS.closed_x, VMDS.closed_y, (int16_t)ENGINE_POLYGON_STATE.outline_count);

out:
    if (ENGINE_POLYGON_STATE.second_pass != 0) {
        /* The second pass, for a polygon whose two top edges had one slope. */
        ENGINE_POLYGON_STATE.second_pass = 0;
        cx = (int16_t)ENGINE_POLYGON_STATE.second_count;
        for (i = 0; i < cx; i += 2) {
            VMDS.work_x[i >> 1] = ((uint16_t)VMDS.closed_x[i >> 1]);
            VMDS.work_y[i >> 1] = ((uint16_t)VMDS.closed_y[i >> 1]);
        }
        goto reverse;
    }
}

/*
 * 172c:2f69, image 0x1f219
 *
 * Draw the outline: one `clip_and_draw_line` per side, from two arrays of
 * points. The second half of the routine is the same again with the vertical
 * window and every y halved, which is the mode where a row is two scan lines -
 * the byte at DGROUP 0x3f78 says which.
 */
void poly_outline(int16_t *xs, int16_t *ys, int16_t n)
{
    if (VMDS.screen.mode_kind == 0) {
        while (n-- > 0) {
            clip_and_draw_line(xs[0], ys[0],
                               xs[1],
                               ys[1]);
            xs++;
            ys++;
        }
        return;
    }

    VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top >> 1);
    VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom >> 1);

    while (n-- > 0) {
        clip_and_draw_line(xs[0], (int16_t)(ys[0] >> 1),
                           xs[1],
                           (int16_t)(ys[1] >> 1));
        xs++;
        ys++;
    }

    VMDS.clip_top = (int16_t)((uint16_t)VMDS.clip_top << 1);
    VMDS.clip_bottom = (int16_t)((uint16_t)VMDS.clip_bottom << 1);
}

/*
 * 172c:3015, image 0x1f265 - an edge with no run at all.
 *
 * Both ends have the same x, so every row gets it: no fractional part and no
 * step. The two ends are put in top-to-bottom order first.
 */
void poly_edge_vertical(uint8_t far * span, int16_t x,
                        int16_t y1, int16_t y2)
{
    if (y2 <= y1) {
        int16_t t = y1;

        y1 = y2;
        y2 = t;
    }

    ENGINE_POLYGON_STATE.span_step = 2;
    poly_walk(span, x, 0, 0, 0, (int16_t)(y2 - y1 + 1), (uint16_t)y1);
}

/*
 * 172c:3031, image 0x1f281 - an edge steeper than 45 degrees.
 *
 * Bresenham's, written out: the error starts at `2 * dx - dy`, a step that
 * takes the error non-negative moves x by one and adds `2 * (dx - dy)`, and
 * anything else adds `2 * dx`.
 *
 * The original unrolls the body six times and picks the direction by which way
 * the rows run - `di` four bytes forward or four back - which is the same two
 * loops the port writes as one with a signed step.
 */
void poly_edge_steep(uint8_t far * span, int16_t x1, int16_t x2,
                     int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e1, e2, x, count, sign;
    uint16_t di;

    if (x1 >= x2) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)((y1 << 2) + ENGINE_POLYGON_CHAINS.chain);

    dy = (int16_t)(y1 - y2);
    sign = (dy >= 0) ? 0 : -1;
    if (dy < 0)
        dy = (int16_t)-dy;

    dx = (int16_t)(x2 - x1);
    e2 = (int16_t)(dx * 2);
    x = x1;
    err = (int16_t)(e2 - dy);
    e1 = (int16_t)((int16_t)((dx - dy) * 2) ^ e2);
    count = (int16_t)(dy + 1);

    /*
     * `sign` is the sign the absolute value above threw away: zero means the
     * rows run backwards through the buffer, which the original reaches by a
     * second copy of the whole unrolled body.
     */
    while (count-- > 0) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + (sign == 0 ? -4 : 4));

        /*
         * `cmp bh,0x80 / sbb dx,dx`: all ones when the error is not negative.
         * It steps x, and it picks the increment out of `e1`, which holds
         * the two increments xor'd together: `mask & e1 ^ e2` is
         * `2 * (dx - dy)` on a step and `2 * dx` otherwise. Adding `e1`
         * itself - which this did until 2026-09-27 - put S15's machine 21
         * flips off the original's.
         */
        {
            int16_t mask = (int16_t)(err >= 0 ? -1 : 0);

            x = (int16_t)(x - mask);
            err = (int16_t)(err + ((mask & e1) ^ e2));
        }
    }
}

/*
 * 172c:316f, image 0x1f3bf - an edge at exactly 45 degrees.
 *
 * One across for every one down, so again no fractional part: the step is 1 or
 * -1 by which way the x runs.
 */
void poly_edge_diagonal(uint8_t far * span, int16_t x1, int16_t x2,
                        int16_t y1, int16_t y2)
{
    /*
     * The ends are put top-first, so the swap is the one that happens when the
     * first end is *below* the second. Testing it the other way round leaves
     * the ends bottom-first, and the count below - `y2 - y1 + 1` - then comes
     * out zero or negative and the edge is not written at all. A polygon whose
     * side is at exactly 45 degrees loses that side, which is why the fault
     * hid: 45 is a special case of its own, and everything shallower or
     * steeper goes elsewhere.
     */
    if (y1 >= y2) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    ENGINE_POLYGON_STATE.span_step = 2;
    poly_walk(span, x1, 0, (x1 < x2) ? 1 : -1, 0,
              (int16_t)(-(int16_t)(y1 - y2) + 1), (uint16_t)y1);
}

/*
 * 172c:3196, image 0x1f3e6 - an edge shallower than 45 degrees,
 * the right chain's.
 *
 * Walked *along* rather than down: one row covers several columns, so the loop
 * runs over x and only writes when the error says the row has changed. Unrolled
 * eight times in the original, with the "catch up the error" chain unrolled
 * eight times inside that; a speed device with no observable difference, so the
 * port writes the loop.
 *
 * This is the one the **right** chain takes, where DGROUP 0x44dc is 2: it
 * writes the second slot of each row, `y * 4 + 2`, and counts x **down**.
 *
 * This and `poly_edge_shallow_left` are **not** one routine with a flag: they
 * differ in three
 * places - which way the ends are put in order, which of the row's two slots
 * is written, and which way x counts - and the original has two of them, each
 * its own entry reached by a computed `jmp` on DGROUP 0x44dc. Written as one
 * function with a flag they could not be told apart by the verifier, and the
 * coverage tool counted neither.
 */
void poly_edge_shallow_right(uint8_t far * span, int16_t x1, int16_t x2,
                             int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e, x, count, di_step;
    uint16_t di;

    if ((x1 <= x2)) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)(((y1 << 1) + 1) << 1);   /* the row's second slot */
    di_step = 2;

    dy = (int16_t)(y2 - y1);
    if (dy < 0) {
        dy = (int16_t)-dy;
        di_step = -6;
    }

    count = dy;

    dx = (int16_t)(x2 - x1);
    if (dx > 0)
        dx = (int16_t)-dx;

    err = dx;
    e = (int16_t)((dx + dy) * 2);
    dy = (int16_t)(dy * 2);
    err = (int16_t)(err + dy);

    x = x1;

    /*
     * The first end is written outside the loop, and the *only* thing between
     * it and the second is one step of x and a catch-up: the error is not
     * advanced by `e` yet. Folding that first write into the loop adds an
     * `err += e` that the original does not do there, and on a shallow edge
     * `e` is negative, so the catch-up steps x an extra column or two and
     * every end after it is wrong by that much.
     *
     * `stosw` advances DI by two of its own accord and the original adds its
     * 2 or -6 on top, so a row costs four bytes either way - the same slot of
     * the next row, or of the one before. Adding only the 2 or -6 lands on the
     * *other* slot of the row just written, which leaves the right ends of a
     * whole run of rows unset and the driver fills those to the clip's right
     * edge: a stripe from wherever the polygon was to x=639.
     */
    *(int16_t *)(void *)(span + di) = x;
    di = (uint16_t)(di + 2 + di_step);
    x = (int16_t)(x - 1);

    while (err < 0) {
        x = (int16_t)(x - 1);
        err = (int16_t)(err + dy);
    }

    for (;;) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);
        x = (int16_t)(x - 1);

        if (--count == 0)
            break;

        err = (int16_t)(err + e);
        while (err < 0) {
            x = (int16_t)(x - 1);
            err = (int16_t)(err + dy);
        }
    }
}

/*
 * 172c:3251, image 0x1f4a1 - an edge shallower than 45 degrees,
 * the left chain's.
 *
 * Walked *along* rather than down: one row covers several columns, so the loop
 * runs over x and only writes when the error says the row has changed. Unrolled
 * eight times in the original, with the "catch up the error" chain unrolled
 * eight times inside that; a speed device with no observable difference, so the
 * port writes the loop.
 *
 * This is the one the **left** chain takes, where DGROUP 0x44dc is 0: it
 * writes the first slot of each row, `y * 4`, and counts x **up**.
 *
 * This and `poly_edge_shallow_right` are **not** one routine with a flag: they
 * differ in three
 * places - which way the ends are put in order, which of the row's two slots
 * is written, and which way x counts - and the original has two of them, each
 * its own entry reached by a computed `jmp` on DGROUP 0x44dc. Written as one
 * function with a flag they could not be told apart by the verifier, and the
 * coverage tool counted neither.
 */
void poly_edge_shallow_left(uint8_t far * span, int16_t x1, int16_t x2,
                            int16_t y1, int16_t y2)
{
    int16_t dx, dy, err, e, x, count, di_step;
    uint16_t di;

    if ((x1 >= x2)) {
        int16_t t = x1;

        x1 = x2;
        x2 = t;
        t = y1;
        y1 = y2;
        y2 = t;
    }

    di = (uint16_t)(y1 << 2);                /* the row's first slot */
    di_step = 2;

    dy = (int16_t)(y2 - y1);
    if (dy < 0) {
        dy = (int16_t)-dy;
        di_step = -6;
    }

    count = dy;

    dx = (int16_t)(x2 - x1);
    if (dx > 0)
        dx = (int16_t)-dx;

    err = dx;
    e = (int16_t)((dx + dy) * 2);
    dy = (int16_t)(dy * 2);
    err = (int16_t)(err + dy);

    x = x1;

    /*
     * The first end is written outside the loop, and the *only* thing between
     * it and the second is one step of x and a catch-up: the error is not
     * advanced by `e` yet. Folding that first write into the loop adds an
     * `err += e` that the original does not do there, and on a shallow edge
     * `e` is negative, so the catch-up steps x an extra column or two and
     * every end after it is wrong by that much.
     *
     * `stosw` advances DI by two of its own accord and the original adds its
     * 2 or -6 on top, so a row costs four bytes either way - the same slot of
     * the next row, or of the one before. Adding only the 2 or -6 lands on the
     * *other* slot of the row just written, which leaves the right ends of a
     * whole run of rows unset and the driver fills those to the clip's right
     * edge: a stripe from wherever the polygon was to x=639.
     */
    *(int16_t *)(void *)(span + di) = x;
    di = (uint16_t)(di + 2 + di_step);
    x = (int16_t)(x + 1);

    while (err < 0) {
        x = (int16_t)(x + 1);
        err = (int16_t)(err + dy);
    }

    for (;;) {
        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);
        x = (int16_t)(x + 1);

        if (--count == 0)
            break;

        err = (int16_t)(err + e);
        while (err < 0) {
            x = (int16_t)(x + 1);
            err = (int16_t)(err + dy);
        }
    }
}

/*
 * 172c:3312, image 0x1f562
 *
 * One edge of the polygon, walked down the scanlines, writing the x it reaches
 * on each into the span buffer.
 *
 * `x` steps by `step` every row plus the carry out of a fractional accumulator
 * that `frac` is added to - a fixed-point DDA rather than Bresenham's error
 * term - and `di` walks the buffer four bytes a row, which is one pair of
 * span ends.
 *
 * **The original is a computed jump into an unrolled loop.** It works out
 * `0x3e28 - 7 * count` and jumps there, landing exactly `count` copies of the
 * seven-byte body from the end - about four hundred of them, which is most of
 * this routine's 3,674 bytes. That is a speed device with no observable
 * difference, so the port writes the loop.
 */
void poly_walk(uint8_t far * span, int16_t x, int16_t frac, int16_t step,
               int16_t acc, int16_t count, uint16_t di)
{
    int16_t di_step = (int8_t)ENGINE_POLYGON_STATE.span_step;

    di = (uint16_t)((di << 2) + ENGINE_POLYGON_CHAINS.chain);

    while (count-- > 0) {
        uint32_t t;

        *(int16_t *)(void *)(span + di) = x;
        di = (uint16_t)(di + 2 + di_step);

        t = (uint32_t)(uint16_t)acc + (uint32_t)(uint16_t)frac;
        acc = (int16_t)t;
        x = (int16_t)(x + step + (int16_t)(t >> 16));
    }
}

/*
 * 0x20079
 *
 * Fill a rectangle, clipped, and optionally outline it.
 *
 * The fill is done by turning the rectangle into a **span list** - the first
 * row, the row count, then one `x1, x2` pair per row, all identical - and
 * handing it to the driver at VGA:0x0be6 through the vector at DGROUP 0x43b2.
 * That is a general span filler being used for the simplest possible case,
 * which is why a solid rectangle costs one entry per scan line.
 *
 * Clipping shrinks the rectangle in place against the box at DGROUP
 * 0x3894..0x389a, and the original x and y are pushed before that and popped
 * back afterwards, because the outline below wants the unclipped ones.
 *
 * The outline at 0x2013f is **not transcribed**. It draws four lines through
 * 0x21e34 with a stack-reuse trick - each call pushes only the arguments that
 * differ from the last and relies on the rest still being there - which has no
 * honest expression in C without modelling the stack. It is never reached on
 * the intro screens: over 2,108 calls the two border bytes at DGROUP
 * 0x389d/0x389e - the driver's own colour bytes, seen through DGROUP - were
 * always equal, which is the condition that skips it.
 */
void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t right = (int16_t)(x + w - 1);
    int16_t bottom = (int16_t)(y + h - 1);

    if (VMDS.fill_enabled != 0) {
        int16_t cx = x, cy = y, cw = w, ch = h;

        if (VMDS.clip_enabled != 0) {
            int16_t d = (int16_t)(cx - VMDS.clip_left);
            if (d < 0) {
                cx = (int16_t)(cx - d);
                cw = (int16_t)(cw + d);
            }
            d = (int16_t)(cy - VMDS.clip_top);
            if (d < 0) {
                cy = (int16_t)(cy - d);
                ch = (int16_t)(ch + d);
            }
            d = (int16_t)(VMDS.clip_right - right);
            if (d < 0)
                cw = (int16_t)(cw + d);
            d = (int16_t)(VMDS.clip_bottom - bottom);
            if (d < 0)
                ch = (int16_t)(ch + d);
        }

        if (cw > 0 && ch > 0) {
            uint8_t *p = MK_FP(DG4342.span_buffer_seg, 0);
            int16_t n = ch;
            int16_t x2 = (int16_t)(cx + cw - 1);

            *p++ = (uint8_t)(cy & 0xFF);
            *p++ = (uint8_t)((uint16_t)cy >> 8);
            *p++ = (uint8_t)(ch & 0xFF);
            *p++ = (uint8_t)((uint16_t)ch >> 8);
            do {
                *p++ = (uint8_t)(cx & 0xFF);
                *p++ = (uint8_t)((uint16_t)cx >> 8);
                *p++ = (uint8_t)(x2 & 0xFF);
                *p++ = (uint8_t)((uint16_t)x2 >> 8);
            } while (--n);

            vm_fill_spans(MK_FP(DG4342.span_buffer_seg, 0));
        }
    }

    if (VMDS.fill_enabled != 0 && VMDS.fill_colour == VMDS.second_colour)
        return;
    not_transcribed("0x2013f, the rectangle outline");
}

/*
 * 0x20185
 *
 * A thunk - `ljmp [0x44ea]` - and 0x44ea was measured pointing at the
 * instruction after it, `draw_compressed_body` at 0x20189, so the vector
 * exists to be repointed and not to reach another module. The port calls
 * the body. Assembly, and which module it ends or starts is not settled: the
 * `nop` before it pads the assembly `fill_rect` ends, and the body after it
 * is C (compbmp.c).
 */
void draw_compressed_bitmap(struct bitmap * bmp, int16_t x, int16_t y, uint16_t mode)
{
    draw_compressed_body(bmp, x, y, mode);
}
