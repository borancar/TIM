/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The timer driver**: reprogramming the 8253, its interrupt handler, and
 * the table of periodic callbacks it runs - plus the two row-blit thunks
 * into the video driver after it.
 *
 * A module of the original's **code segment 1c25**, image 0x20654..0x20840,
 * split out of engine.c on 2026-09-27. Its `_DATA` is `g_timer`, DGROUP
 * 0x44ee..0x4579, padded to the word before scale.c's.
 *
 * **Hand-written assembly**: the handler is an interrupt routine and every
 * entry is written by hand. The TASM source is `timer.asm`, drafted by
 * tools/asm2tasm.py; the old INT 08h vector is kept in the code segment, in
 * the four bytes after `timer_drop_callback`. Its start is C's end:
 * `draw_compressed_body` (compbmp.c) returns at 0x20653. Whether the two
 * thunks at 0x20838 (0x224c2 in 1.11) are its last routines or a module of
 * their own is not settled.
 */
#include "hostlib.h"
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/* **This module's `_DATA`**: the timer's state and its callback table. */
struct timer g_timer = { .divisor = -1 };   /* DGROUP 0x40ee */

/* Ours: the callback table the timer keeps at DGROUP 0x44f9, as the host's
   own code pointers - see `struct timer`. */
static void (far *g_timer_callbacks[16])(void);

/*
 * 0x222de
 *
 * Take a slot in the timer's callback table and fill it in. Answers the slot
 * number plus one - so 1..8, with 0 meaning it could not.
 *
 * Two things stop it: a **zero** byte at DGROUP 0x44ee, which is the flag
 * saying the timer handler is not installed - 0x206c1 installs only while it is
 * zero and sets it, and this registers only once it is set - and a full mask at
 * 0x44f7, tested as `mask + 1 == 0` rather than against 0xffff, which is the
 * same thing in one instruction.
 *
 * The free slot is found by shifting the mask right until a zero bit falls out,
 * counting `BX` up in fours and `CX` along as the bit. The four parallel tables
 * are therefore indexed by `slot * 4`: the far pointer at 0x44f9 and 0x44fb,
 * and the reload count at 0x4539 with its running copy at 0x453b - both set to
 * the same value here, so the first tick is a whole period away.
 *
 * The mask is set with interrupts off, because the handler reads it.
 *
 * Hand-written assembly: no locals, and `AX` is the answer throughout.
 */
uint16_t timer_add_callback(void (far *cb)(void), uint16_t period)
{
    uint16_t mask, bx, cx;

    if (g_timer.installed == 0)
        return 0;

    mask = g_timer.slot_mask;
    if ((uint16_t)(mask + 1) == 0)
        return 0;

    bx = 0;
    cx = 1;
    while ((mask & 1) != 0) {
        mask >>= 1;
        cx = (uint16_t)(cx << 1);
        bx = (uint16_t)(bx + 4);
    }

    /* `bx` is the original's 4 * slot, which is how it addressed the two
       tables; the slot is `bx >> 2`, which is also what it answers. */
    g_timer.tick[bx >> 2].period = (int16_t)period;
    g_timer.tick[bx >> 2].left = (int16_t)period;
    g_timer_callbacks[bx >> 2] = cb;

    /* `cli` / `sti`, around this one instruction and nothing else. */
    io_lock();
    g_timer.slot_mask = (int16_t)(g_timer.slot_mask | cx);
    io_unlock();

    return (uint16_t)((bx >> 2) + 1);
}

/*
 * 0x22328
 *
 * Give a timer slot back: clear its bit in the mask at DGROUP 0x44f7. Answers 1
 * if it did, 0 if the handle was out of range.
 *
 * The handle is the slot plus one, and the range test is `(handle - 1) & 0xf0`
 * - so it admits 1..16 while only eight slots exist. Clearing a bit above the
 * eighth is harmless, since nothing reads it.
 *
 * The mask of everything-but-one bit is built rather than looked up: `stc`,
 * then 0xfffe rotated **left through carry** by the slot number, which walks
 * the single zero up and feeds ones in behind it.
 *
 * Hand-written assembly, no locals.
 */
uint16_t timer_drop_callback(uint16_t handle)
{
    uint16_t cl = (uint16_t)((handle - 1) & 0xff);
    uint16_t v;
    int16_t i;
    uint16_t carry;

    if ((cl & 0xf0) != 0)
        return 0;

    v = 0xfffe;
    carry = 1;
    for (i = 0; i < (int16_t)cl; i++) {
        uint16_t out = (uint16_t)(v >> 15);

        v = (uint16_t)((v << 1) | carry);
        carry = out;
    }

    g_timer.slot_mask = (int16_t)(g_timer.slot_mask & v);

    return 1;
}

/*
 * 0x2234b
 *
 * Take over the timer. Answers 1, or 0 if it was already taken or the rate is
 * out of range.
 *
 * The old INT 08h vector is kept **inside this code segment**, at cs:0x446d,
 * not in DGROUP - which is why the port needs `S1C16` to reach it.
 *
 * The divisor is `0xffff / rate`, not the usual 0x1234dc / rate, so the rate is
 * a divisor of the top of a 16-bit counter rather than a frequency in hertz.
 * A rate above 0xff or of zero is refused, and the answer there is 0 - which is
 * `AX` left as the zero it was set to before the range test, not a value
 * written for the purpose.
 *
 * Then the 8253 is programmed - mode 3, low byte then high - the two lowest
 * interrupts unmasked at the PIC, and the handler at cs:0x4517 installed with
 * interrupts off throughout. DGROUP 0x44ee is the flag that says all this has
 * happened.
 */
int16_t timer_install(uint16_t rate)
{
    uint16_t divisor;

    if (g_timer.installed != 0)
        return 0;

    g_timer.slot_mask = 0;
    detect_pcjr();

    g_s1c_timer.old_int8 = getvect(8);

    if (rate > 0xff || rate == 0)
        return 0;

    g_timer.divider_reload = (int16_t)rate;
    g_timer.divider = (int16_t)rate;

    divisor = (uint16_t)(0xffffu / rate);
    g_timer.divisor = (int16_t)divisor;

    /*
     * `cli` from here to just before the flag is set: the 8253 is half
     * programmed and the vector half installed in between, and a tick landing
     * inside that would run through whichever half was in place.
     */
    io_lock();

    io_out8(0x43, 0x36);
    io_out8(0x40, (uint8_t)divisor);
    io_out8(0x40, (uint8_t)(divisor >> 8));
    io_out8(0x21, (uint8_t)(io_in8(0x21) & 0xfc));

    setvect(8, (void interrupt (far *)())timer_tick);

    io_unlock();                                        /* `sti` */

    g_timer.installed = 1;
    return 1;
}

/*
 * 0x223b8
 *
 * Give the timer back. Answers 1 if it had it, 0 if it did not.
 *
 * The 8253 is put back to a divisor of **zero**, which the chip reads as
 * 0x10000 - the slowest it goes, and the rate DOS expects - and the vector
 * saved at cs:0x446d restored. The two lowest interrupts are unmasked again,
 * which is what `timer_install` did too, so neither routine ever masks them.
 *
 * The answer of 1 is set before the flag at DGROUP 0x44ee is cleared, and the
 * answer of 0 is the `AX` the routine started with rather than one written for
 * the purpose.
 */
int16_t timer_remove(void)
{
    if (g_timer.installed == 0)
        return 0;

    io_out8(0x43, 0x36);
    io_out8(0x40, 0);
    io_out8(0x40, 0);
    io_out8(0x21, (uint8_t)(io_in8(0x21) & 0xfc));

    setvect(8, g_s1c_timer.old_int8);

    g_timer.installed = 0;
    return 1;
}

/*
 * 0x223f1
 *
 * The game's timer interrupt: what everything paced is paced by.
 *
 * It does three things. **DGROUP 0x44ef counts down** - `dec ax / cwd / xor
 * ax,dx`, which is a decrement clamped at zero rather than a wrap, because at
 * -1 the `cwd` makes 0xffff and the `xor` turns -1 into 0. That counter is the
 * intro's frame budget and half the game's timing.
 *
 * Then **sixteen callback slots**: a bitmask at 0x44f7 says which are in use, a
 * counter at 0x4539 and a reload at 0x453b, and a far handler at 0x44f9. A slot
 * whose counter reaches zero calls its handler and reloads. The original writes
 * the sixteen out in full rather than looping - `shr di,1` walks the mask and
 * `ja`/`jae` tell "unused, more to come" from "unused, and that was the last" -
 * and the port folds them into the loop they are.
 *
 * And it **divides itself down**: 0x44f5 counts from 0x44f3, and only when it
 * reaches zero does the old BIOS handler get its tick. So the 8253 is running
 * far faster than 18.2 Hz and the BIOS still sees 18.2.
 *
 * The `mov ax,0x2d3c` that loads DS is a relocation, not a constant.
 */
void timer_tick(void)
{
    uint16_t mask = g_timer.slot_mask;
    int32_t slot;
    int16_t n;

    n = (int16_t)(g_timer.frame_budget - 1);
    if (n < 0)
        n = 0;
    g_timer.frame_budget = n;

    for (slot = 0; slot < 16; slot++) {
        uint16_t used = (uint16_t)(mask & 1);

        mask = (uint16_t)(mask >> 1);

        if (used == 0) {
            if (mask == 0)
                break;
            continue;
        }

        {
            int16_t left = (int16_t)(g_timer.tick[slot].left - 1);

            if (left == 0) {
                g_timer_callbacks[slot]();
                left = g_timer.tick[slot].period;
            }
            g_timer.tick[slot].left = left;
        }
    }

    if (--g_timer.divider != 0) {
        io_out8(0x20, 0x20);            /* end of interrupt */
        return;
    }

    g_timer.divider = g_timer.divider_reload;

    /*
     * And chain to the vector `timer_install` displaced, at ((int16_t)g_s1c_timer.old_int8.off). That
     * is the BIOS's own handler, which keeps 0040:006c ticking. The port has no
     * BIOS handler to chain to and does not pretend otherwise - nothing here
     * reads the BIOS tick count.
     */
}

/*
 * 0x224c2
 *
 * A thunk into the video driver: `ljmp [0x438a]`, which is `vm_blit_rows`.
 */
void blit_rows_thunk(const uint8_t far * src, int16_t x, int16_t y,
                     int16_t w, int16_t h)
{
    vm_blit_rows(src, x, y, w, h);
}

/*
 * 0x224c6
 *
 * A thunk into the video driver: `ljmp [0x438e]`, which on this adapter is
 * VGA:0x0252 - the entry that does nothing at all.
 */
void blit_rows_alt_thunk(const uint8_t far * src, int16_t x, int16_t y,
                         int16_t w, int16_t h)
{
    /* The same five arguments as `blit_rows_thunk`, which the driver's
       entry never reads. */
    (void)src; (void)x; (void)y; (void)w; (void)h;
    vm_nothing();
}
