/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The cursor and the pointer**: the timer's cursor redraw, setting and
 * drawing the cursor, the objects drawn over the backdrop and put back, the
 * page slots the cursor is staged in, the saved draw state, the mouse
 * buttons and the buffers.
 *
 * One module of the original's **code segment 0000** (`_TEXT`), image
 * 0x0a78e..0x0b6b7. From 0x0aa76 to 0x0b3fd its routines reach each other
 * with bare `push cs / call`, and the rest read its data: `_DATA`
 * 0x2d32..0x2d4a and `_BSS` 0x56e6..0x5788. **Both ends are ours**: rects.c
 * before it is another file by TLINK's calls, and the DOS helpers after it
 * begin 0x2d4a's data.
 *
 * JUDGE: compiler bc3.00
 * JUDGE: built-with -mm -zC_TEXT
 */
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

extern struct machine_rect_free MACHINE_RECT_FREE;

   /* rects.c's */

/*
 * **The cursor, the fade, and the palette waiting to load**, DGROUP 0x2d32..0x2d48, 0x16 bytes.
 */
struct machine_cursor_state {
    uint16_t  page;               /* +0x00 [2]  the page the middle call passes */
    uint16_t  screen_disturbed;   /* +0x02 [2]  the saved rectangles are put back when this says so */
    uint16_t  fade_first;          /* +0x04 [2] */
    uint16_t  fade_count;          /* +0x06 [2] */
    uint8_t far *pending_pal;   /* +0x08 [4]  a palette waiting to be loaded */
    uint16_t  cursor_off;         /* +0x0c [2]  clear turns the whole cursor off - nothing is drawn */
    int16_t   delay_reload;       /* +0x0e [2]  the delay counts down and is reloaded from here */
    uint16_t  read_driver;        /* +0x10 [2]  take the position from the driver rather than the last known */
    /* **May the timer redraw the cursor?** `timer_callback` is the only
       reader - it redraws only while this is set and the guard is clear - and
       every routine that is about to draw clears it and puts it back, so the
       cursor is not lifted and dropped underneath a half-drawn frame. */
    uint16_t  timer_draws_cursor; /* +0x12 [2] */
    int16_t   slots_unset;          /* +0x14 [2] */
} PACKED;

struct machine_cursor_state MACHINE_CURSOR_STATE = {
    0x0001, 0, 0, 0x0100, 0, 0x0001, 0x000c, 0x0001, 0x0001, 0x0001
};

/*
 * **This module's `_BSS`, 0x56e6..0x5788.** Borland lays `_BSS` out in
 * reverse order of first mention. dgroup.h mentions POINTER, then
 * `rect_buffer`, then `size_word`, `frame_flag` and `redraw_guard` -
 * the five highest - and the rest are defined here from the highest down.
 */
struct pointer POINTER;

uint8_t far *rect_buffer[4];   /* DGROUP 0x5758 */
int16_t size_word;                                  /* DGROUP 0x5756 */
volatile int16_t frame_flag;                        /* DGROUP 0x5754 */
uint16_t redraw_guard;                              /* DGROUP 0x5752 */

/*
 * **The two buttons' state machines**, at DGROUP 0x5742 - eight bytes each,
 * and `reset_input_state` clears both as two blocks of four words. Sixteen
 * bytes end at 0x5752, which is the guard the clear holds across itself.
 *
 * `button_state` is the whole of what reads them; the field names are its
 * comment.
 */
struct button {
    int16_t   state;              /* +0x00  0 up, 2 pressed, 4 clicked, 8 held */
    int16_t   was_down;           /* +0x02  what the driver said last time */
    int16_t   presses;            /* +0x04  what tells a click from a double one */
    int16_t   delay;              /* +0x06  reloaded from 0x2d40 on every change */
} PACKED;

/* DGROUP 0x5742..0x5752, 0x10 bytes. */
struct machine_buttons {
    struct button button[2];      /* +0x00 [0x10] */
} PACKED;

struct machine_buttons MACHINE_BUTTONS;

/*
 * **The palette request and the fade**, DGROUP 0x5738..0x5742, 0x0a bytes.
 */
struct machine_palette_fade {
    uint8_t far *request;       /* +0x00 [4]  cleared when taken, so one
                                            request loads once */
    uint16_t  fade_mark;          /* +0x04 [2]  reset to zero by a load, which forces the fade to run; */
    /* **A colour that walks 0 to 15**, stepped and plotted when a cursor slot
       has no bitmap - one pixel, in the next colour each time. Nothing else
       reads it. */
    int16_t   plot_colour;        /* +0x06 [2] */
    int16_t   busy;               /* +0x08 [2]  non-zero suppresses the slot release, and everything waits on it */
} PACKED;

struct machine_palette_fade MACHINE_PALETTE_FADE;

/*
 * **The four object buffers `claim_buffer_slot` hands out**: a taken flag
 * apiece at 0x5734; the buffers themselves are `rect_buffer`, the far
 * pointers at 0x5758 up to `POINTER`. Four is the routine's own bound.
 *
 * DGROUP 0x5734..0x5738, 0x04 bytes.
 */
struct machine_buffer_used {
    uint8_t   used[4];            /* +0x00 [4] */
} PACKED;

struct machine_buffer_used MACHINE_BUFFER_USED;

/*
 * **The drawing state saved across an interrupt**, DGROUP 0x5726..0x5734, 0x0e bytes.
 */
struct machine_saved_draw_state {
    uint16_t  saved_a;            /* +0x00 [2]  seven values - the clip box and the two page segments. */
    int16_t   saved_b;            /* +0x02 [2]  0x5726's high half is always zero: it is restored as a byte */
    int16_t   saved_c;            /* +0x04 [2] */
    int16_t   saved_d;            /* +0x06 [2] */
    int16_t   saved_e;            /* +0x08 [2] */
    uint16_t  saved_f;            /* +0x0a [2] */
    uint16_t  saved_g;            /* +0x0c [2] */
} PACKED;

struct machine_saved_draw_state MACHINE_SAVED_DRAW_STATE;

/*
 * **The two page slots**, DGROUP 0x56e6..0x5726, 0x40 bytes.
 *
 * `claim_page_slot` walks two of them at a stride of 0x20, which is
 * `sizeof(struct page_slot)`, and matches on the top bits of the record's
 * first field - the page it belongs to. It answers the slot's own offset, so
 * the callers keep taking a `PAGESLOT`.
 */
struct machine_page_slots {
    struct page_slot slots[2];   /* +0x00 [0x40] */
} PACKED;

struct machine_page_slots MACHINE_PAGE_SLOTS;

/*
 * 0x0a78e
 *
 * Let the cursor follow the mouse again, and redraw it where the mouse now is.
 * The pair to `cursor_redraw_off` three instructions below: DGROUP 0x2d44 is what
 * `redraw_cursor` tests before it asks the driver for the position, so clearing
 * it pins the cursor and setting it releases it.
 */
void cursor_redraw_on(void)
{
    MACHINE_CURSOR_STATE.timer_draws_cursor = 1;
    redraw_cursor(VMDS.page_front);
}

/*
 * 0x0a7a3
 *
 * Set the word at DGROUP 0x2d44 to zero, and nothing else.
 *
 * What the flag governs is **not established**. Its counterpart at 0x0a78e
 * sets it to 1 and then redraws through 0xacc3, passing the word at 0x38a4 -
 * which is inside the video driver's data block at 0x3890 - so the pair reads
 * like suspending and resuming something on screen. That is inference from the
 * shape of the two routines, not something measured, and the name says only
 * what the code does.
 */
void cursor_redraw_off(void)
{
    MACHINE_CURSOR_STATE.timer_draws_cursor = 0;
}

/*
 * 0x0a7ae
 *
 * What the timer calls, four ticks in five: read the keyboard and the mouse,
 * move the pointer, and release the frame.
 *
 * It refuses to run at all when DGROUP 0x5752 is above 1 or 0x5740 is already
 * set - the first is the cursor's nesting guard and the second is this routine
 * being in progress - so a redraw cannot be interrupted by the tick that would
 * start another.
 *
 * The eight scan codes it reads are the keypad's: 0x47 0x48 0x49 across the
 * top, 0x4b 0x4d either side, 0x4f 0x50 0x51 across the bottom. Any of the top
 * three moves up, any of the bottom three down, and the corners count for both
 * of their directions - which is what makes the diagonals work. Two pixels a
 * tick, clamped to the screen, and the clamp is against the *hot spot* rather
 * than the pointer's own position.
 *
 * Then Enter, Space, keypad 5 and Insert are all the same button - `si` ends up
 * 1 if any of them is down - and are ORed with the real one. `button_state`
 * turns each into a state, and the accumulators at 0x5768 and 0x576a keep it
 * until the next frame reads them.
 *
 * The last two lines are the ones everything waits on: 0x5740 is cleared and
 * **0x5754 is set**, which is the flag `wait_and_latch_frame` spins on.
 */
void timer_callback(void)
{
    int16_t moved;
    int16_t k_end, k_down, k_pgdn, k_left, k_right, k_home, k_up, k_pgup;
    int16_t si, di;

    if (((int16_t)redraw_guard) > 1 || MACHINE_PALETTE_FADE.busy != 0)
        return;

    MACHINE_PALETTE_FADE.busy = 1;
    moved = 0;

    k_end   = key_is_down(SC_END);
    k_down  = key_is_down(SC_DOWN);
    k_pgdn  = key_is_down(SC_PGDN);
    k_left  = key_is_down(SC_LEFT);
    k_right = key_is_down(SC_RIGHT);
    k_home  = key_is_down(SC_HOME);
    k_up    = key_is_down(SC_UP);
    k_pgup  = key_is_down(SC_PGUP);

    if (k_home != 0 || k_up != 0 || k_pgup != 0) {
        moved = 1;
        POINTER.cursor_y -= 2;
        if (POINTER.cursor_y - POINTER.hot_y < 0)
            POINTER.cursor_y = 0;
    }

    if (k_end != 0 || k_down != 0 || k_pgdn != 0) {
        moved = 1;
        POINTER.cursor_y += 2;
        if (POINTER.cursor_y - POINTER.hot_y > (int16_t)(VMDS.screen.screen_height - 1))
            POINTER.cursor_y = (int16_t)(VMDS.screen.screen_height - 1);
    }

    if (k_end != 0 || k_left != 0 || k_home != 0) {
        moved = 1;
        POINTER.cursor_x -= 2;
        if (POINTER.cursor_x - POINTER.hot_x < 0)
            POINTER.cursor_x = 0;
    }

    if (k_pgdn != 0 || k_right != 0 || k_pgup != 0) {
        moved = 1;
        POINTER.cursor_x += 2;
        if (POINTER.cursor_x - POINTER.hot_x > (int16_t)(VMDS.screen.screen_width - 1))
            POINTER.cursor_x = (int16_t)(VMDS.screen.screen_width - 1);
    }

    if (moved != 0)
        mouse_move_to(((uint16_t)POINTER.cursor_x), ((uint16_t)POINTER.cursor_y));

    if (MACHINE_CURSOR_STATE.timer_draws_cursor != 0 && redraw_guard == 0) {
        isr_stack_switch(1);
        redraw_cursor(VMDS.page_front);
        isr_stack_switch(0);
    }

    di = read_mouse_button(0);

    si = (key_is_down(SC_SPACE) != 0 || key_is_down(SC_ENTER) != 0
          || key_is_down(SC_KP5) != 0 || key_is_down(SC_INS) != 0) ? 1 : 0;

    di |= (si != 0) ? 1 : 0;

    si = button_state(0, di);
    if (si <= 1)
        si = POINTER.button_accum_b;
    POINTER.button_accum_b = (int16_t)(di | (si & 0xfffe));

    di = (MACHINE_CURSOR_STATE.read_driver != 0 && read_mouse_button(1) != 0) ? 1 : 0;
    si = key_is_down(1);
    di |= si;

    si = button_state(1, di);
    if (si <= 1)
        si = POINTER.button_accum_a;
    POINTER.button_accum_a = (int16_t)(di | (si & 0xfffe));

    MACHINE_PALETTE_FADE.busy = 0;
    frame_flag = 1;
}

/*
 * 0x0aa14
 *
 * Choose the mouse cursor: which bitmap, and where its hot spot is. The three
 * are kept at DGROUP 0x5770, 0x5780 and 0x577e, and a call that names what is
 * already showing does nothing at all - not even the redraw.
 *
 * A cursor of 0 means none, and then both hot-spot words are zeroed rather than
 * taking the arguments, so turning the cursor off cannot leave a stale offset
 * behind for the next one.
 *
 * DGROUP 0x5752 is raised over the redraw and put back afterwards. It is not a
 * simple flag: the value it had is *saved*, so a redraw inside a redraw leaves
 * the outer one's state alone when it finishes.
 */
void set_cursor(struct bitmap *bitmap, int16_t hot_x, int16_t hot_y)
{
    uint16_t saved;

    if (POINTER.cursor_bitmap == bitmap && POINTER.hot_x == hot_x
        && POINTER.hot_y == hot_y)
        return;

    saved = redraw_guard;
    redraw_guard = 1;

    POINTER.cursor_bitmap = bitmap;

    if (bitmap == NULL) {
        POINTER.hot_x = POINTER.hot_y = 0;
    } else {
        POINTER.hot_x = hot_x;
        POINTER.hot_y = hot_y;
    }

    redraw_cursor(VMDS.page_front);

    redraw_guard = saved;
}

/*
 * 0x0aa76
 *
 * Put the pointer somewhere, clamped to the screen, and tell the driver.
 *
 * Each coordinate is pinned to 0 below and to the screen size less one above -
 * the width at DGROUP 0x3f7a and the height at 0x3f7c - so a caller may ask
 * for anything and the pointer stays on the screen. Both bounds are the
 * *game's* idea of the screen, which is why this follows a mode change: the
 * copy-protection screen sets 0x3f7c to 0x18f before it does any of this.
 *
 * The result is written to **two pairs**: 0x5784/0x5782, which is where the
 * game reads the pointer, and 0x576e/0x576c, which is where it remembers it.
 * Then `mouse_move_to` moves the driver's own cursor to match, so the three
 * agree.
 */
void move_pointer_to(int16_t x, int16_t y)
{
    if (x < 0)
        x = 0;
    else if ((int16_t)(VMDS.screen.screen_width - 1) < x)
        x = (int16_t)(VMDS.screen.screen_width - 1);

    if (y < 0)
        y = 0;
    else if ((int16_t)(VMDS.screen.screen_height - 1) < y)
        y = (int16_t)(VMDS.screen.screen_height - 1);

    POINTER.cursor_x = POINTER.pointer_x = x;
    POINTER.cursor_y = POINTER.pointer_y = y;

    mouse_move_to((uint16_t)x, (uint16_t)y);
}

/*
 * 0x0aaca
 *
 * Wait for the frame, then latch the input state for the frame about to be
 * drawn and clear the accumulators.
 *
 * The wait is the spin on `frame_pending` that the INT 08h handler releases -
 * and it is guarded, so when DGROUP 0x44ee is clear the routine does not wait
 * at all. That spin measured at 64% of all basic block executions under an
 * emulator paced on the host clock; see STATUS.md.
 *
 * The pair at 0x5782/0x5784 is filled either from `read_mouse_pointer` or from the
 * two words at 0x576c/0x576e, and the pair at 0x5768/0x576a is moved into
 * 0x5772/0x5774 and zeroed - accumulated since the last frame, then handed
 * over and reset, which is what a frame boundary looks like.
 */
void wait_and_latch_frame(void)
{
    if (TIMER.installed != 0) {
        while (frame_pending())
            ;
    }

    if (((int16_t)MACHINE_CURSOR_STATE.read_driver) != 0) {
        read_mouse_pointer(&POINTER.pointer_x, &POINTER.pointer_y);
    } else {
        POINTER.pointer_x = POINTER.cursor_x;
        POINTER.pointer_y = POINTER.cursor_y;
    }

    POINTER.button_left = POINTER.button_accum_b;
    POINTER.button_right = POINTER.button_accum_a;
    POINTER.button_accum_b = POINTER.button_accum_a = 0;
    frame_flag = 0;
}

/*
 * 0x0ab1f
 *
 * Draw the cursor on a page: put back what was under the last one, save what is
 * under the new one, draw it, and remember where.
 *
 * The page's slot holds both states at once - the *previous* one at +0x14 and
 * the current at +8 - and bits 0 and 1 of the byte at +0x1f say which of them
 * is live. That is what lets the erase happen after the save rather than before
 * it, so the two rectangles can overlap without the erase undoing the save.
 *
 * Clipping is set wide open first: DGROUP 0x3894 and 0x3898 to zero, 0x3896 and
 * 0x389a to the screen's size less one, and both page pointers at 0x38a6 and
 * 0x38a8 to the slot's own page - so the cursor is drawn on that page whichever
 * one is being shown.
 *
 * A slot buffer of zero means "nothing was saved", and then the erase is a
 * single `plot_pixel_clipped` of the byte at +0x1e instead of a rectangle - the
 * one-pixel case, which a saved rectangle would be wasteful for.
 *
 * DGROUP 0x2d3e turns the whole cursor off: with it clear nothing is drawn and
 * bit 1 of +0x13 is cleared instead, which is what `redraw_cursor` tests.
 */
void draw_cursor(uint16_t page)
{
    struct page_slot *slot;
    uint16_t saved;

    if ((slot = claim_page_slot(page)) == NULL)
        return;

    saved = redraw_guard;
    redraw_guard = 1;

    restage_object_rect(page);
    save_or_restore_draw_state(1);

    VMDS.page_dst = VMDS.page_src = slot->page;
    VMDS.clip_enabled = 1;
    VMDS.clip_left = VMDS.clip_top = 0;
    VMDS.clip_bottom = VMDS.screen.screen_height - 1;
    VMDS.clip_right = VMDS.screen.screen_width - 1;

    /* Put back what the last cursor covered. */
    if (slot->cursor.flags & 2) {
        if (slot->cursor.buf != 0) {
            if (slot->cursor.w > 0 && slot->cursor.h > 0)
                restore_rect_thunk(rect_buffer[slot->cursor.buf - 1],
                                   slot->cursor.x, slot->cursor.y,
                                   slot->cursor.w, slot->cursor.h);
        } else {
            plot_pixel_clipped(slot->cursor.x, slot->cursor.y,
                               slot->cursor.pixel);
        }
        slot->cursor.flags &= 0xfd;
    }

    /* Save what the new one will cover. */
    if (MACHINE_CURSOR_STATE.cursor_off != 0) {
        if (slot->obj.buf != 0 && slot->bitmap != 0) {
            if (slot->obj.w > 0 && slot->obj.h > 0)
                save_rect_thunk(rect_buffer[slot->obj.buf - 1],
                                slot->obj.x, slot->obj.y,
                                slot->obj.w, slot->obj.h);
        } else {
            slot->obj.pixel = read_pixel_clipped(slot->obj.x, slot->obj.y);
        }
    }

    /* And draw it. */
    if (MACHINE_CURSOR_STATE.cursor_off != 0) {
        if (slot->bitmap != 0 && slot->obj.buf != 0) {
            /*
             * On adapter 8 a negative y is nudged one further up before the
             * blit.
             */
            if ((uint8_t)VMDS.pixel_shift == 8 && slot->y < 0)
                draw_bitmap(slot->bitmap, slot->x, slot->y - 1, 0);
            else
                draw_bitmap(slot->bitmap, slot->x, slot->y, 0);
        } else {
            MACHINE_PALETTE_FADE.plot_colour = (MACHINE_PALETTE_FADE.plot_colour + 1) & 0x0f;
            plot_pixel_clipped(slot->x, slot->y,
                               MACHINE_PALETTE_FADE.plot_colour);
        }
        slot->obj.flags |= 2;
    } else {
        slot->obj.flags &= 0xfd;
    }

    save_or_restore_draw_state(0);

    /* Give back the buffer the erase used, if nothing else wants it. */
    if ((slot->cursor.flags & 1) && slot->cursor.buf != 0
        && MACHINE_PALETTE_FADE.busy == 0) {
        release_buffer(slot->cursor.buf);
        slot->cursor.buf = 0;
        slot->cursor.flags &= 0xfe;
    }

    redraw_guard = saved;
}

/*
 * 0x0acc3
 *
 * Redraw the cursor on a page, if anything about it has changed.
 *
 * The page's slot comes from `claim_page_slot`; a page with no slot is not
 * drawn on at all. Then the mouse's position is read into DGROUP 0x576e and
 * 0x576c - but only when DGROUP 0x2d42 says to, so a caller that has already
 * decided where the cursor goes can suppress it - and the hot spot is
 * subtracted to give the top-left corner at 0x56e2 and 0x56e4.
 *
 * The redraw is then skipped when all four of the slot's remembered values
 * still agree with what was just worked out **and** bit 1 of the slot's byte at
 * +0x13 is set. A cursor of 0 skips the comparison and always redraws, which is
 * how it gets erased.
 *
 * 0x5752 is raised across the whole thing and restored, the same nesting guard
 * `set_cursor` uses.
 */
void redraw_cursor(uint16_t page)
{
    struct page_slot *slot;
    uint16_t saved;

    if ((slot = claim_page_slot(page)) == NULL)
        return;

    saved = redraw_guard;
    redraw_guard = 1;

    if (MACHINE_CURSOR_STATE.read_driver != 0)
        read_mouse_pointer(&POINTER.cursor_x, &POINTER.cursor_y);

    MACHINE_RECT_FREE.draw_x = POINTER.cursor_x - POINTER.hot_x;
    MACHINE_RECT_FREE.draw_y = POINTER.cursor_y - POINTER.hot_y;

    if (POINTER.cursor_bitmap == 0
        || slot->x != MACHINE_RECT_FREE.draw_x
        || slot->y != MACHINE_RECT_FREE.draw_y
        || slot->bitmap != POINTER.cursor_bitmap
        || !(slot->obj.flags & 2))
        draw_cursor(page);

    redraw_guard = saved;
}

/*
 * 0x0ad51
 *
 * Put back whatever an object was covering, and mark it no longer drawn.
 *
 * Bit 1 of the byte at +0x13 says the object is currently on screen; with it
 * clear this does nothing but the bookkeeping around it. With it set there are
 * two ways back. If the object has a buffer slot at +0x10 and a rectangle with
 * both extents positive, the saved pixels are put back with `vm_restore_rect`,
 * the buffer being the far pointer at DGROUP 0x5754 + 4 * slot - one-based, as
 * `claim_buffer_slot` hands them out. Otherwise a single pixel is replaced from
 * the colour byte at +0x12, which is what `restage_object_rect` left there.
 *
 * The rectangle used is the **clipped** one at +8, not the unclipped position
 * at +4, so an object partly off-screen restores only the part that was drawn.
 *
 * The two words at 0x38a6 and 0x38a8 are both set from the record's first word
 * before any of that. They are inside the driver's data block, and the same
 * value goes to both.
 *
 * `save_or_restore_draw_state` brackets the whole thing, and the global at
 * 0x5752 is forced to 1 for the duration and put back at the end - the same
 * pattern `restage_object_rect` uses.
 */
void erase_object(uint16_t handle)
{
    struct page_slot *rec;
    int16_t saved;

    if ((rec = claim_page_slot(handle)) == NULL)
        return;

    saved = redraw_guard;
    redraw_guard = 1;

    save_or_restore_draw_state(1);

    VMDS.page_dst = VMDS.page_src = rec->page;

    if (rec->obj.flags & 2) {
        if (rec->obj.buf != 0 && rec->obj.w > 0 && rec->obj.h > 0)
            vm_restore_rect(rect_buffer[rec->obj.buf - 1],
                            rec->obj.x, rec->obj.y,
                            rec->obj.w, rec->obj.h);
        else
            plot_pixel_clipped(rec->obj.x, rec->obj.y, rec->obj.pixel);
        rec->obj.flags &= 0xfd;
    }

    save_or_restore_draw_state(0);
    redraw_guard = saved;
}

/*
 * 0x0adf1
 *
 * Put back what an object covered on one page, and make the other page the
 * one being drawn to.
 *
 * The slot's +0x10 says where the backdrop was kept: non-zero and it is a far
 * pointer in the pair of tables at DGROUP 0x5754 and 0x5756, four bytes apart
 * per slot, and the rectangle goes back whole. Zero and there was only ever one
 * pixel, whose colour is the byte at +0x12.
 *
 * The draw state is saved across it and DGROUP 0x5752 pinned, which is what
 * stops the cursor being redrawn in the middle.
 */
void restore_object_backdrop(uint16_t from_page, uint16_t to_page)
{
    int16_t saved;
    struct page_slot *si;

    if ((si = claim_page_slot(from_page)) == NULL)
        return;

    saved = redraw_guard;
    redraw_guard = 1;

    save_or_restore_draw_state(1);

    VMDS.page_dst = VMDS.page_src = to_page;

    if (si->obj.flags & 2) {
        if (si->obj.buf != 0 && si->obj.w > 0 && si->obj.h > 0)
            restore_rect_thunk(rect_buffer[si->obj.buf - 1],
                               si->obj.x, si->obj.y, si->obj.w, si->obj.h);
        else
            plot_pixel_clipped(si->obj.x, si->obj.y, si->obj.pixel);
    }

    save_or_restore_draw_state(0);
    redraw_guard = saved;
}

/*
 * 0x0ae8e
 *
 * **Swap the object lists of two pages.** Each page's slot holds the head of a
 * list of saved rectangles; this exchanges the two heads, so everything drawn
 * over one page is now attributed to the other.
 *
 * Nothing happens unless both pages have slots: `claim_page_slot` is asked for
 * each, and either answering zero leaves the two lists alone.
 *
 * The swap is done with the re-entry guard at DGROUP 0x5752 raised and put
 * back afterwards, because for the two instructions between the two stores
 * neither list is whole - one head is in a local and the other is in both
 * slots - and a cursor redraw arriving there would walk it.
 */
void swap_page_objects(uint16_t page_a, uint16_t page_b)
{
    struct page_slot *slot_a;
    struct page_slot *slot_b;
    uint16_t was;
    uint16_t head;

    if ((slot_a = claim_page_slot(page_b)) == NULL
        || (slot_b = claim_page_slot(page_a)) == NULL)
        return;

    was = redraw_guard;
    redraw_guard = 1;

    head = slot_a->page;
    slot_a->page = slot_b->page;
    slot_b->page = head;

    redraw_guard = was;
}

/*
 * 0x0aedc
 *
 * Say a page's object no longer covers anything: clear bit 1 of the slot's
 * +0x13. A page with no slot is left alone.
 */
void clear_object_covered(uint16_t page)
{
    struct page_slot *si;

    if ((si = claim_page_slot(page)) != NULL)
        si->obj.flags &= 0xfd;
}

/*
 * 0x0aef6
 *
 * Age an object's on-screen rectangle by one frame: copy where it is now into
 * where it was, then work out where it is now from the current globals and clip
 * that to the screen.
 *
 * The record has two parallel blocks. The current one runs from +8 - x, y, w, h
 * at +8/+0xa/+0xc/+0xe, a buffer slot at +0x10, and two bytes at +0x12/+0x13 -
 * and the previous one from +0x14 with the same shape, its bytes at
 * +0x1e/+0x1f. Copying one onto the other is the whole of the first half.
 *
 * Before that copy it may hand back a buffer slot, and **only the
 * `release_buffer` call survives**: the two stores beside it, zeroing +0x1c
 * and clearing bit 0 of +0x1f, are both overwritten a few instructions later by
 * the copy. They are dead as written, and transcribed anyway.
 *
 * If the object's parent at +2 no longer matches the global at 0x5770 it is
 * re-parented, which means asking the driver how big the new parent's image is
 * and claiming a scratch buffer for it. `claim_buffer_slot` ignores the size it
 * is handed, so that measurement goes nowhere - see 0x0b5ed. A null parent
 * gives slot zero and a 1 by 1 rectangle.
 *
 * The unclipped position goes to +4/+6 and is kept; the clipped copy goes to
 * +8. Clipping is one-sided in the usual way: a negative coordinate is pulled
 * to zero and taken out of the extent, and an extent running past 0x3f7a or
 * 0x3f7c - the screen width and height - is cut back to the edge. Nothing stops
 * an extent going negative if the rectangle is entirely off-screen.
 *
 * The global at 0x5752 is set to 1 for the duration and put back at the end,
 * and 0x5740 being non-zero suppresses both the slot release and the
 * re-parenting.
 */
void restage_object_rect(uint16_t handle)
{
    struct page_slot *rec;
    int16_t x, y, w, h, saved, size;

    if ((rec = claim_page_slot(handle)) == NULL)
        return;

    saved = redraw_guard;
    redraw_guard = 1;

    if ((rec->cursor.flags & 1) && rec->cursor.buf != 0
        && MACHINE_PALETTE_FADE.busy == 0) {
        release_buffer(rec->cursor.buf);
        rec->cursor.buf = 0;
        rec->cursor.flags &= 0xfe;
    }

    rec->cursor.x = rec->obj.x;
    rec->cursor.y = rec->obj.y;
    rec->cursor.w = rec->obj.w;
    rec->cursor.h = rec->obj.h;
    rec->cursor.buf = rec->obj.buf;
    rec->cursor.flags = rec->obj.flags;
    rec->cursor.pixel = rec->obj.pixel;

    if (rec->bitmap != POINTER.cursor_bitmap && MACHINE_PALETTE_FADE.busy == 0) {
        rec->cursor.flags |= 1;
        if ((rec->bitmap = POINTER.cursor_bitmap) != 0) {
            size = vm_buffer_size(POINTER.cursor_bitmap->width,
                                  POINTER.cursor_bitmap->height);
            rec->obj.buf = claim_buffer_slot(size, 0L);
        } else {
            rec->obj.buf = 0;
        }
    }

    if (MACHINE_CURSOR_STATE.read_driver != 0)
        read_mouse_pointer(&POINTER.cursor_x, &POINTER.cursor_y);

    x = POINTER.cursor_x - POINTER.hot_x;
    y = POINTER.cursor_y - POINTER.hot_y;

    if (POINTER.cursor_bitmap != 0) {
        w = POINTER.cursor_bitmap->width;
        h = POINTER.cursor_bitmap->height;
    } else {
        w = h = 1;
    }

    rec->x = x;
    rec->y = y;

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w >= VMDS.screen.screen_width)
        w = VMDS.screen.screen_width - x;
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (y + h >= VMDS.screen.screen_height)
        h = VMDS.screen.screen_height - y;

    rec->obj.x = x;
    rec->obj.y = y;
    rec->obj.w = w;
    rec->obj.h = h;

    redraw_guard = saved;
}

/*
 * 0x0b078
 *
 * **Redraw the cursor**, and with it everything the cursor was standing on.
 *
 * This is what the whole saved-rectangle machinery exists for. The cursor is
 * drawn over the picture, so before it can move, what it covered has to go
 * back - and because the game is double buffered, on both pages, in an order
 * that never leaves either half restored.
 *
 * The re-entry guard at DGROUP 0x5752 is raised for the whole routine and the
 * *entering* value put back at the end, not a zero, so a call from inside a
 * call leaves the flag as it found it.
 *
 * The order, which is the substance of it:
 *
 *   1  a pending move at 0x577a/0x577c is taken first and cleared, so the
 *      pointer is where it is going before anything is drawn. Either word
 *      being non-zero is enough to trigger it.
 *   2  the cursor is erased from the drawing page.
 *   3  when the pages differ, `show_page_thunk` is told whether nothing else
 *      is pending - no palette waiting at 0x2d3a and the fade already where
 *      0x5786 asks - so it can wait for retrace only when it is worth it.
 *   4  a palette waiting at 0x2d3a/0x2d3c is loaded, remembered at
 *      0x5738/0x573a, and **cleared**, so one request loads once. Loading one
 *      also resets 0x573c to zero, which forces the fade below to run.
 *   5  the fade runs only when 0x5786 differs from 0x573c, and 0x573c is then
 *      caught up.
 *   6  if 0x2d34 says the screen was disturbed, the saved rectangles for both
 *      pages and for the copy pair are given back, the whole screen is copied
 *      between pages, and the objects are moved or erased. Otherwise only the
 *      drawing page's objects are erased.
 *   7  when the pages are the same, the cursor goes back on, the two pages'
 *      object lists are swapped, each page's own saved rectangle is copied
 *      back through its slot, and the backdrop is restored between them.
 *   8  and whatever page it took, the lists of lists at 0x2d0a are put back.
 *
 * The `add sp, 6` after each `free_saved_rects` is the caller cleaning three
 * arguments; the middle call passes 0x2d32 as the page, where the other two
 * pass zero.
 */
void redraw_cursor_all(void)
{
    uint16_t was;
    struct page_slot *rec;

    was = redraw_guard;
    redraw_guard = 1;

    if (POINTER.pending_move_x != 0 || POINTER.pending_move_y != 0) {
        move_pointer_to(POINTER.pending_move_x, POINTER.pending_move_y);
        POINTER.pending_move_x = POINTER.pending_move_y = 0;
    }

    draw_cursor(VMDS.page_back);

    if (MACHINE_CURSOR_STATE.page != 0)
        show_page_thunk(MACHINE_CURSOR_STATE.pending_pal == NULL
                        && POINTER.fade_weight == MACHINE_PALETTE_FADE.fade_mark ? 1 : 0);

    if (MACHINE_CURSOR_STATE.pending_pal != NULL) {
        set_palette_pointer(MACHINE_CURSOR_STATE.pending_pal);
        MACHINE_PALETTE_FADE.request = MACHINE_CURSOR_STATE.pending_pal;
        MACHINE_CURSOR_STATE.pending_pal = NULL;
        MACHINE_PALETTE_FADE.fade_mark = 0;
    }

    if (POINTER.fade_weight != MACHINE_PALETTE_FADE.fade_mark) {
        fade_palette_run(MACHINE_CURSOR_STATE.fade_first, MACHINE_CURSOR_STATE.fade_count, 0, POINTER.fade_weight);
        MACHINE_PALETTE_FADE.fade_mark = POINTER.fade_weight;
    }

    if (MACHINE_CURSOR_STATE.screen_disturbed != 0) {
        if (MACHINE_CURSOR_STATE.page != 0) {
            VMDS.page_src = VMDS.page_front;
            VMDS.page_dst = VMDS.page_back;
        } else {
            VMDS.page_src = VMDS.page_back;
            VMDS.page_dst = VMDS.page_front;
        }

        free_saved_rects(VMDS.rect_page, VMDS.page_back, 0);
        free_saved_rects(VMDS.rect_page, VMDS.page_front, MACHINE_CURSOR_STATE.page);
        free_saved_rects(VMDS.page_src, VMDS.page_dst, 0);

        copy_rect_thunk(0, 0, VMDS.screen.screen_width, VMDS.screen.screen_height);

        if (MACHINE_CURSOR_STATE.page != 0) {
            restore_object_backdrop(VMDS.page_front, VMDS.page_back);
            clear_object_covered(VMDS.page_back);
        } else {
            erase_object(VMDS.page_back);
        }

        MACHINE_CURSOR_STATE.screen_disturbed = 0;
    } else {
        erase_object(VMDS.page_back);
    }

    if (MACHINE_CURSOR_STATE.page == 0) {
        clear_object_covered(VMDS.page_front);
        draw_cursor(VMDS.page_back);
        swap_page_objects(VMDS.page_front, VMDS.page_back);

        VMDS.page_dst = VMDS.page_front;
        VMDS.page_src = VMDS.page_back;

        if ((rec = claim_page_slot(VMDS.page_front)) != NULL)
            copy_rect_thunk(rec->obj.x, rec->obj.y, rec->obj.w, rec->obj.h);

        if ((rec = claim_page_slot(VMDS.page_back)) != NULL)
            copy_rect_thunk(rec->obj.x, rec->obj.y, rec->obj.w, rec->obj.h);

        restore_object_backdrop(VMDS.page_front, VMDS.page_back);
    }

    restore_saved_rect_lists(0);

    redraw_guard = was;
}

/*
 * 0x0b28e
 *
 * Copy a rectangle from the page on screen to the page being drawn to, with
 * the pointer out of the way.
 *
 * Both pages are asked whether their object - the mouse pointer - overlaps the
 * rectangle, and the two answers decide what has to be taken down and put back.
 * The usual way is: erase the pointer from the page being drawn to, copy, put
 * the shown page's backdrop back if it was covered, and draw the pointer again.
 *
 * There is a second way, taken only when DGROUP 0x2d32 is clear *and* the drawn
 * page's pointer is in the way: draw the pointer on the shown page first, copy,
 * and erase it from the shown page afterwards - so the copy carries the pointer
 * across rather than working around it.
 *
 * A rectangle with no width or no height is not copied, but everything else
 * still happens. DGROUP 0x5752 is pinned throughout and put back at the end.
 */
void copy_rect_around_cursor(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t hit_draw, hit_shown;
    int16_t ox, oy, ow, oh;
    uint16_t saved;
    struct page_slot *si;

    hit_draw = 0;
    hit_shown = 0;
    saved = redraw_guard;
    redraw_guard = 1;

    if ((si = claim_page_slot(VMDS.page_src)) != NULL
        && (si->obj.flags & 2)) {
        ox = si->obj.x;
        oy = si->obj.y;
        ow = si->obj.w;
        oh = si->obj.h;
        if (x + w > ox && ox + ow > x && y + h > oy && oy + oh > y)
            hit_shown = 1;
    }

    if ((si = claim_page_slot(VMDS.page_dst)) != NULL
        && (si->obj.flags & 2)) {
        ox = si->obj.x;
        oy = si->obj.y;
        ow = si->obj.w;
        oh = si->obj.h;
        if (x + w > ox && ox + ow > x && y + h > oy && oy + oh > y)
            hit_draw = 1;
    }

    if (MACHINE_CURSOR_STATE.page != 0 || hit_draw == 0) {
        if (hit_draw != 0)
            erase_object(VMDS.page_dst);

        if (w > 0 && h > 0)
            copy_rect_thunk(x, y, w, h);

        if (hit_shown != 0) {
            restore_object_backdrop(VMDS.page_src, VMDS.page_dst);
            clear_object_covered(VMDS.page_dst);
        }

        if (hit_draw != 0)
            draw_cursor(VMDS.page_dst);
    } else {
        draw_cursor(VMDS.page_src);

        if (w > 0 && h > 0)
            copy_rect_thunk(x, y, w, h);

        erase_object(VMDS.page_src);
    }

    redraw_guard = saved;
}

/*
 * 0x0b40d
 *
 * **May the timer draw the cursor now?** - `timer_draws_cursor` set and
 * `timer_callback` not in progress. Nothing in the image calls it.
 */
int16_t timer_may_draw_cursor(void)
{
    return MACHINE_CURSOR_STATE.timer_draws_cursor != 0
        && MACHINE_PALETTE_FADE.busy == 0;
}

/*
 * 0x0b429
 *
 * Find the entry in the two-slot table at DGROUP 0x56e6 whose top bits match,
 * and claim it. The slots are 0x20 bytes apart, so the second is at 0x5706 -
 * and those are exactly the two words the initialisation below fills with the
 * driver's back and front pages, once, the first time through.
 *
 * The match is on bits **0xa800** only, not on the whole word, so a slot
 * matches a page that differs from it in the low bits. Answers the slot, or
 * NULL - the original's 0 - if neither matched.
 *
 * A `want` of 0 means "the page currently being drawn into".
 */
struct page_slot *claim_page_slot(uint16_t want)
{
    struct page_slot *p;
    int16_t i;

    if (MACHINE_CURSOR_STATE.slots_unset != 0) {
        MACHINE_PAGE_SLOTS.slots[0].page = VMDS.page_back;
        MACHINE_PAGE_SLOTS.slots[1].page = VMDS.page_front;
        MACHINE_CURSOR_STATE.slots_unset = 0;
    }

    if (want == 0)
        want = VMDS.page_back;

    for (p = MACHINE_PAGE_SLOTS.slots, i = 0; i < 2; i++, p++) {
        if ((want & 0xA800) == (p->page & 0xA800)) {
            p->page = want;
            return p;
        }
    }
    return NULL;
}

/*
 * 0x0b47f
 *
 * Save the driver's drawing state, or put it back: a non-zero argument saves,
 * zero restores. The state is the clip box, whether clipping is on, and the
 * two page segments - seven values, kept at DGROUP 0x5726..0x5732.
 *
 * `VMDS.clip_enabled` is a byte and is saved **zero-extended into a word**, then
 * restored as a byte, so the high half of 0x5726 is always zero. Transcribed
 * with the same widths rather than made symmetrical.
 */
void save_or_restore_draw_state(int16_t save)
{
    if (save != 0) {
        MACHINE_SAVED_DRAW_STATE.saved_a = VMDS.clip_enabled;
        MACHINE_SAVED_DRAW_STATE.saved_b = VMDS.clip_left;
        MACHINE_SAVED_DRAW_STATE.saved_c = VMDS.clip_right;
        MACHINE_SAVED_DRAW_STATE.saved_d = VMDS.clip_top;
        MACHINE_SAVED_DRAW_STATE.saved_e = VMDS.clip_bottom;
        MACHINE_SAVED_DRAW_STATE.saved_g = VMDS.page_dst;
        MACHINE_SAVED_DRAW_STATE.saved_f = VMDS.page_src;
    } else {
        VMDS.clip_enabled = ((uint8_t)MACHINE_SAVED_DRAW_STATE.saved_a);
        VMDS.clip_left = MACHINE_SAVED_DRAW_STATE.saved_b;
        VMDS.clip_right = MACHINE_SAVED_DRAW_STATE.saved_c;
        VMDS.clip_top = MACHINE_SAVED_DRAW_STATE.saved_d;
        VMDS.clip_bottom = MACHINE_SAVED_DRAW_STATE.saved_e;
        VMDS.page_dst = MACHINE_SAVED_DRAW_STATE.saved_g;
        VMDS.page_src = MACHINE_SAVED_DRAW_STATE.saved_f;
    }
}

/*
 * 0x0b4e2
 *
 * Non-zero while `frame_flag` is still clear. The original is
 * `neg ax / sbb ax,ax / inc ax`, which is Borland's idiom for `ax = (ax == 0)`.
 *
 * The caller at 0x0aaca spins on this waiting for the INT 08h handler to set
 * the flag; that spin was 64% of all basic block executions under an emulator
 * paced on the host clock. See STATUS.md.
 */
int16_t frame_pending(void)
{
    return !frame_flag;
}

/*
 * 0x0b4f1
 *
 * Clear the input state: two eight-byte blocks at DGROUP 0x5742, then the two
 * accumulators at 0x5768/0x576a and the two latched values at 0x5772/0x5774 -
 * the same four words `wait_and_latch_frame` moves and zeroes each frame.
 *
 * The word at 0x5752 is **saved, set to 2, and put back**. It sits immediately
 * after the sixteen bytes being cleared, so it is not being protected from the
 * loop; it is a guard held across the clear, which only makes sense if
 * something asynchronous - the INT 08h handler, which writes these very words -
 * reads it.
 */
void reset_input_state(void)
{
    int16_t saved;
    struct button *b;
    register int16_t n;

    saved = redraw_guard;
    redraw_guard = 2;

    for (b = MACHINE_BUTTONS.button, n = 2; n != 0; b++, n--) {
        b->state = 0;
        b->was_down = 0;
        b->presses = 0;
        b->delay = 0;
    }

    POINTER.button_accum_b = POINTER.button_accum_a = 0;
    POINTER.button_left = POINTER.button_right = 0;

    redraw_guard = saved;
}

/*
 * 0x0b542
 *
 * The button state machine, one eight-byte record per button at DGROUP 0x5742:
 * the state at +0, whether it was down last time at +2, a press count at +4,
 * and a repeat delay at +6.
 *
 * The states are 0 up, 2 pressed, 4 clicked and 8 held. A release with the
 * state at 8 goes straight back to 0; otherwise the press count goes up and the
 * state becomes 2 the first time and 4 after that - which is what tells a click
 * from a double one.
 *
 * A change also latches where the pointer was, at DGROUP 0x5776 and 0x5778 -
 * from the driver when 0x2d42 says so, and from the last known position when it
 * does not - and reloads the delay from 0x2d40. The delay then counts down on
 * every call, and while it is still running *and* something has been pressed,
 * the answer is the raw button rather than the state, which is what holds a
 * click on screen long enough to be seen.
 */
int16_t button_state(uint16_t index, int16_t down)
{
    struct button *b = &MACHINE_BUTTONS.button[index];

    if (b->was_down != down) {
        b->was_down = down;

        if (down == 0) {
            if (b->state != 8) {
                b->presses++;
                if (b->presses == 1 && b->state != 2)
                    b->state = 2;
                else
                    b->state = 4;
            } else {
                b->state = 0;
            }
        }

        if (MACHINE_CURSOR_STATE.read_driver != 0) {
            read_mouse_pointer(&POINTER.button_at_x, &POINTER.button_at_y);
        } else {
            POINTER.button_at_x = POINTER.cursor_x;
            POINTER.button_at_y = POINTER.cursor_y;
        }

        b->delay = MACHINE_CURSOR_STATE.delay_reload;
    }

    if (b->delay != 0)
        b->delay--;

    if (b->delay == 0 || b->presses > 0) {
        if (down != 0)
            b->state = 8;
        else if (b->presses == 0)
            b->state = 0;

        b->presses = 0;

        return b->state;
    } else {
        return down;
    }
}

/*
 * 0x0b5ed
 *
 * Make sure the four scratch buffers exist, then claim a free one and answer
 * its **one-based** index, or -1 if all four are taken. `release_buffer` is
 * the release.
 *
 * The four buffers are far pointers at DGROUP 0x5758, four bytes apart; the
 * four in-use bytes are at 0x5734, which is why that array is one-based - zero
 * is the "no slot" answer. Any buffer still null is allocated on the way past,
 * so the first call does all four allocations and later ones do none.
 *
 * The size is the word at 0x5756, or, if that is zero, whatever the driver
 * says a 64 by 64 planar image needs - reached through the thunk at 0x21ab9,
 * which is `vm_buffer_size` here. Only the low word of the driver's DX:AX
 * answer is kept, and it is then sign-extended by `cwd` into the 32-bit size
 * DOS is asked for, so a size at or above 0x8000 would be asked for as a
 * negative length. Nothing seen produces one.
 *
 * **The four argument words are ignored.** The routine opens by loading each
 * of the two pairs and storing them straight back where they came from, which
 * is a no-op, and then never reads them again. The caller at 0x0aef6 goes to
 * the trouble of asking the driver for a size and passing it in, and this
 * discards it in favour of 0x5756 or the 64 by 64 default. Transcribed as it
 * stands, with the parameters named and voided.
 */
int16_t claim_buffer_slot(int32_t a, int32_t b)
{
    int16_t size;
    int16_t i;

    /* Two Borland `long`s the routine does not use - its one caller passes
       the size in the first and zero in the second. The image copies each
       onto itself, which is what these two lines compile to. It sizes the
       buffer from `size_word` or `vm_buffer_size` instead. */
    a = a;
    b = b;

    size = size_word != 0 ? size_word : (int16_t)vm_buffer_size(0x40, 0x40);

    for (i = 0; i < 4; i++) {
        if (rect_buffer[i] == NULL)
            rect_buffer[i] = DOS_ALLOC_PTR(DOS_ALLOC((int32_t)size, 0));
    }

    for (i = 0; i < 4; i++) {
        if (!MACHINE_BUFFER_USED.used[i]
            && rect_buffer[i] != NULL) {
            MACHINE_BUFFER_USED.used[i] = 1;
            return i + 1;
        }
    }

    return -1;
}

/*
 * 0x0b69c
 *
 * Clear one byte of the four-entry array at DGROUP 0x5734, addressed
 * **one-based**: the argument is decremented before it is used as the index.
 *
 * Zero is rejected, and so is anything that lands at index 4 or above. Nothing
 * rejects a *negative* argument: the bound is `jge 4`, a signed test that a
 * negative index passes, so a caller passing a number below zero writes a zero
 * byte in front of the array. No caller seen does, but the guard is genuinely
 * one-sided and the port reproduces it rather than adding the missing half.
 */
void release_buffer(int16_t n)
{
    if (n-- != 0 && n < 4)
        MACHINE_BUFFER_USED.used[n] = 0;
}
