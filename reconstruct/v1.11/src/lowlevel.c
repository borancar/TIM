/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). Licensed under the GNU General Public
 * License, version 2 - see LICENSE.
 *
 * **The machine, in assembly**: DOS memory, the mouse, huge-pointer
 * arithmetic, the divide-error trap, clipped single pixels, and three thunks
 * into the video driver.
 *
 * This file corresponds to the original's **code segment 1c25**, image
 * 0x21b44..0x22483, split out of engine.c on 2026-09-27; dosmem.c holds
 * 0x21ab5..0x21b44, the module `far_memset` is not in. All of it is
 * hand-written: register arguments and answers, `iret`, near helpers that
 * answer in the carry, frames no compiler builds. **So it is TASM source**,
 * `lowlevel.asm`, with the host's transcription here. The functions are in
 * address order and each carries the image offset it was read from.
 *
 * **It is probably several modules, and only one boundary is proven.**
 * Measured 2026-09-27 over every call into, out of and inside the range, and
 * every DGROUP word each routine names:
 *
 *   - `dos_alloc_bytes` (0x21abd) calls `far_memset` (0x22300) through
 *     TLINK's `nop / push cs / call`, so the two are in different modules.
 *     That is the only call between the groups below.
 *   - Inside a group the calls are bare `push cs / call` (TASM's own, so the
 *     same module) or near: the joystick (0x21b44..0x21e34) calls 0x21d19,
 *     0x21b44 and 0x21bab; `mouse_init` calls `mouse_set_ranges`;
 *     `mouse_event` calls the two VGA save routines; and the huge-pointer
 *     helpers (0x22161..0x22394) near-call 0x22161 and 0x22190.
 *   - The data follows the code: the joystick's block at 0x4724..0x4740,
 *     starting on the word after the text module's; the mouse's
 *     0x4740..0x48ec, one run (0x4748..0x48d8 is the stack `mouse_event`
 *     switches to, `mov sp,0x48d8`); the divide trap's 0x48ec..0x48f1; then
 *     `vm_init`'s module at 0x48f2. No word is padded between the joystick,
 *     the mouse and the divide trap, and the thunks, the DOS memory routines,
 *     the line clipper, the huge-pointer helpers and the clipped pixels have
 *     no data at all.
 *   - No routine is padded to an alignment: each starts on the byte after
 *     the previous one's return.
 *
 * So the groups are plain - thunks and DOS memory, joystick, line clipper,
 * mouse, huge pointers, divide trap, clipped pixels and a thunk - but
 * nothing yet says which neighbours shared a file. One file stands for the
 * range until something does.
 *
 * **0x21b44..0x21e34 is not transcribed**: a joystick driver - port 0x201,
 * timed with `loop` - that nothing on the paths the port runs calls.
 */
#include <string.h>

#include "hostlib.h"
#include "tim.h"
#include "hostio.h"
#include "dgroup.h"

/*
 * **The mouse's position and the game's handler**, DGROUP 0x4740..0x4748,
 * 0x08 bytes. The position is kept at four times the pixel, which is what the
 * driver's own units are.
 */
struct engine_mouse {
    uint16_t  mouse_x;            /* +0x00 [2]  four times the pixel x: `mouse_move_to` stores `x << 2`
                                               and `read_mouse_pointer` answers `>> 2` */
    uint16_t  mouse_y;            /* +0x02 [2]  the same for y */
    void (far *mouse_handler_fn)(void); /* +0x04 [4]  the game's own handler, called by `mouse_event`;
                                               nothing in the image sets it */
} PACKED;

struct engine_mouse g_engine_mouse;

/*
 * **The cursor code's, the mouse's and the divide trap's data**, DGROUP
 * 0x48da..0x48f2, in address order.
 *
 * The graphics controller's registers the cursor code saves - 0 and 1, 4,
 * then 8 - the sequencer's map mask, and register 3.
 */
int16_t g_saved_gc_0_1;                  /* DGROUP 0x48da */
int16_t g_saved_gc_4;                    /* DGROUP 0x48dc */
int16_t g_saved_gc_8;                    /* DGROUP 0x48de */
int16_t g_saved_seq_map_mask;            /* DGROUP 0x48e0 */
uint8_t g_saved_gc_3;                    /* DGROUP 0x48e2 */

/*
 * **Two graphics-controller mode bytes**, both written to GC register 5 by the
 * cursor code: VGA write mode 2 - a byte selects a colour - and write mode 1,
 * the latch copy the routine parks with. The `_256` pair is the same two with
 * bit 6 set, the 256-colour shift; `mouse_init` copies them over the first
 * pair when the driver reports eight bits a pixel.
 */
uint8_t g_gc_mode_fill = 0x02;           /* DGROUP 0x48e6 */
uint8_t g_gc_mode_fill_256 = 0x40;       /* DGROUP 0x48e7 */
uint8_t g_gc_mode_copy = 0x01;           /* DGROUP 0x48e8 */
uint8_t g_gc_mode_copy_256 = 0x41;       /* DGROUP 0x48e9 */

/* Whether the mouse driver was taken - `neg al` branches on it - and the
   button byte `timer_callback` samples on the page flip. */
uint8_t g_mouse_taken;                   /* DGROUP 0x48ea */
uint8_t g_mouse_buttons;                 /* DGROUP 0x48eb */

/* Whether `install_divide_trap` has run, and the vector 0 it replaced: from
   0:0 and 0:2, a load and not a store, and stored **segment first** at an odd
   offset. */
uint8_t g_divide_hooked;                 /* DGROUP 0x48ec */
void interrupt (far *g_old_divide_vector)();   /* DGROUP 0x48ed */

/*
 * 0x237ce
 *
 * **Time the joystick's axes**: fire the one-shots and count until the chosen bits fall, into SI and DI. A near routine with register arguments. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source in `lowlevel.asm` is the original's.
 */
void joy_time_axes(void)
{
    not_transcribed("0x21b44, the joystick driver");
}

/*
 * 0x23835
 *
 * **Scale one axis** against its calibration: AX less BX, times CX, clamped to 127 with a dead zone of 8, the sign put back. A near routine with register arguments. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source in `lowlevel.asm` is the original's.
 */
int16_t joy_scale_axis(void)
{
    not_transcribed("0x21bab, the joystick driver");
    return 0;
}

/*
 * 0x2386c
 *
 * **Find and calibrate the joysticks**: time the 8253 against the port to size the delay loop, then take each stick's centre. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source in `lowlevel.asm` is the original's.
 */
int16_t joy_init(void)
{
    not_transcribed("0x21be2, the joystick driver");
    return 0;
}

/*
 * 0x239a3
 *
 * **Read a stick's two axes**, scaled. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source in `lowlevel.asm` is the original's.
 */
void joy_read(int16_t stick, int16_t *x, int16_t *y)
{
    not_transcribed("0x21d19, the joystick driver");
    (void)stick; (void)x; (void)y;
}

/*
 * 0x23a1b
 *
 * **A stick as four direction bits**: left 4, right 8, up 1, down 2, past thirty either way. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source in `lowlevel.asm` is the original's.
 */
uint16_t joy_direction(int16_t stick)
{
    not_transcribed("0x21d91, the joystick driver");
    (void)stick;
    return 0;
}

/*
 * 0x23a74
 *
 * **Is button `n` down?** Bit 4+n of port 0x201, inverted. Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source in `lowlevel.asm` is the original's.
 */
uint16_t joy_button(uint16_t n)
{
    not_transcribed("0x21dea, the joystick driver");
    (void)n;
    return 0;
}

/*
 * 0x23a8e
 *
 * **One axis, scaled.** Nothing in the game calls it. NOT TRANSCRIBED YET for the host: the
 * port has no joystick, and the routine is timing loops on port 0x201 and
 * the 8253. A stub, which aborts; the TASM source in `lowlevel.asm` is the original's.
 */
int16_t joy_axis(uint16_t n)
{
    not_transcribed("0x21e04, the joystick driver");
    (void)n;
    return 0;
}

/*
 * 0x23abe
 *
 * Clip a line to the clip box and hand what is left to the driver's line
 * drawer through the vector at DGROUP 0x434e.
 *
 * Four stages - top, left, bottom, right - each the same shape: if both ends
 * are outside the edge the line is dropped entirely; if both are inside the
 * stage is skipped; otherwise the two ends are **swapped** so the outside one
 * is first, and it is moved onto the edge by interpolation.
 *
 * The interpolation is a 32-bit intermediate: `imul` makes a 32-bit product in
 * DX:AX and `idiv` divides it, so a long multiply is essential here and doing
 * it in 16 bits would overflow on a long line.
 *
 * **The first two stages compare signed and the last two unsigned** - `jl`/
 * `jge` against top and left, `ja`/`jbe` against bottom and right. That is not
 * a slip to tidy: once a line has been clipped to the top and left edges its
 * coordinates cannot be negative, so unsigned compares are safe and shorter.
 * Transcribed with the same signedness.
 *
 * BP is used as a scratch register for the divisor, which destroys the frame
 * pointer - safe only because every argument has already been loaded into a
 * register by then.
 *
 * Finally the ends are ordered by x, swapping both coordinates together, so
 * the drawer always receives them left to right.
 */
void clip_and_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    int16_t edge, t;

    if (g_vmds.clip_enabled != 0) {
        /* top */
        edge = g_vmds.clip_top;
        if (y1 < edge) {
            if (y2 < edge)
                return;
        } else if (y2 >= edge) {
            goto left;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        x1 = (int16_t)(x1 + (int16_t)(((int32_t)(x2 - x1) * (edge - y1))
                                      / (y2 - y1)));
        y1 = edge;

left:
        edge = g_vmds.clip_left;
        if (x1 < edge) {
            if (x2 < edge)
                return;
        } else if (x2 >= edge) {
            goto bottom;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        y1 = (int16_t)(y1 + (int16_t)(((int32_t)(y2 - y1) * (edge - x1))
                                      / (x2 - x1)));
        x1 = edge;

bottom:
        edge = g_vmds.clip_bottom;
        if ((uint16_t)y1 > (uint16_t)edge) {
            if ((uint16_t)y2 > (uint16_t)edge)
                return;
        } else if ((uint16_t)y2 <= (uint16_t)edge) {
            goto right;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        x1 = (int16_t)(x1 + (int16_t)(((int32_t)(x2 - x1) * (edge - y1))
                                      / (y2 - y1)));
        y1 = edge;

right:
        edge = g_vmds.clip_right;
        if ((uint16_t)x1 > (uint16_t)edge) {
            if ((uint16_t)x2 > (uint16_t)edge)
                return;
        } else if ((uint16_t)x2 <= (uint16_t)edge) {
            goto draw;
        } else {
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        y1 = (int16_t)(y1 + (int16_t)(((int32_t)(y2 - y1) * (edge - x1))
                                      / (x2 - x1)));
        x1 = edge;
    }

draw:
    if ((uint16_t)x1 > (uint16_t)x2) {
        t = x1; x1 = x2; x2 = t;
        t = y1; y1 = y2; y2 = t;
    }
    vm_draw_line(x1, y1, x2, y2);
}

/*
 * 0x23ba7
 *
 * Start the mouse, once. The flag at DGROUP 0x48ea says whether it has already
 * been done, and a second call answers 0 without touching anything.
 *
 * INT 33h AX=0 resets the driver and answers 0xffff if one is installed. The
 * original turns that into the flag with `neg ax`, which makes 1 from 0xffff
 * and 0 from 0, and sets carry for any non-zero answer - so the `jae` that
 * follows is "no mouse, give up", and the flag is written either way.
 *
 * With a driver there it sets the cursor far off-screen, shows and immediately
 * hides it, sets the mickeys-per-pixel to 8 by 8, puts the cursor at the
 * origin, limits it to the screen the game recorded at DGROUP 0x3f7a and
 * 0x3f7c, and installs the handler at 0x21fcf for the 0x1f events. None of
 * that is in guest memory.
 *
 * The two bytes it copies at the end are: on adapter 8 - the byte at DGROUP
 * 0x38ad, the same one that chooses the palette length - the cursor's hot spot
 * is taken from a second pair at 0x48e7 and 0x48e9.
 */
uint16_t mouse_init(void)
{
    uint16_t present;

    if (g_mouse_taken != 0)
        return 0;

    present = io_mouse_reset();
    g_mouse_taken = (uint8_t)(-(int16_t)present);

    if (present == 0)
        return 0;

    io_mouse_move_to(0x7fff, 0x7fff);
    io_mouse_show();
    io_mouse_hide();
    io_mouse_set_speed(8, 8);
    io_mouse_move_to(0, 0);

    mouse_set_ranges(0, 0, ((uint16_t)g_vmds.screen.screen_width), ((uint16_t)g_vmds.screen.screen_height));

    io_mouse_set_handler(0x1f, (void (far *)(void))mouse_event);

    if (((uint8_t)g_vmds.pixel_shift) == 8) {
        g_gc_mode_fill = g_gc_mode_fill_256;
        g_gc_mode_copy = g_gc_mode_copy_256;
    }

    return 1;
}

/*
 * 0x23c17
 *
 * Set how far the cursor may travel, from an origin and a size in cells: INT
 * 33h AX=7 for the horizontal range and AX=8 for the vertical. Both ends are
 * multiplied by four - the same cell-to-pixel scale `mouse_move_to` uses - and
 * the far end has one subtracted before scaling, so a size of `n` cells ends at
 * the last pixel of the `n`th rather than the first pixel of the next.
 *
 * It writes nothing to memory: the ranges live in the mouse driver.
 */
void mouse_set_ranges(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    io_mouse_set_x_range((uint16_t)(x << 2), (uint16_t)((x + w - 1) << 2));
    io_mouse_set_y_range((uint16_t)(y << 2), (uint16_t)((y + h - 1) << 2));
}

/*
 * 0x23c48
 *
 * Remember the game's own mouse handler, as a far pointer in DGROUP 0x4744 and
 * 0x4746. `mouse_event` below calls it after it has recorded the event, and
 * calls nothing when both words are zero.
 *
 * **Nothing in the image calls this**, and the only other references to either
 * word are the two inside `mouse_event`. Searched for both, as a far call and
 * as a near one, and as any instruction with those displacements: three sites
 * for 0x4744 and two for 0x4746, all of them here. So the pointer is never
 * set, the branch in `mouse_event` is never taken, and the two routines that
 * save and restore the VGA around it never run. Transcribed because the code
 * is there and the branch has to be right if it is ever reached; marked
 * unreachable because it is.
 */
void mouse_set_user_handler(void (far *h)(void))
{
    g_engine_mouse.mouse_handler_fn = h;
}

/*
 * 0x23c59
 *
 * **The mouse driver's callback.** INT 33h AX=0x0c installs this, and the
 * driver calls it on every event the mask asked for, with the button state in
 * `BL` and the position in `CX` and `DX`.
 *
 * It records all three in DGROUP - the buttons at 0x48eb, which is the byte
 * `read_mouse_button` answers from and therefore the *only* way a click reaches
 * the game - and then, if a handler is installed, saves the VGA, calls it, and
 * puts the VGA back.
 *
 * The first thing it does is switch to a stack of its own inside DGROUP, at
 * 0x48d8, with interrupts off across the two writes that change `ss` and `sp`
 * together. That is machine, not program: an interrupt arriving between them
 * would run on half a stack. The port has one stack and no interrupts to
 * arrive, so the switch is **not transcribed** - it is the one part of this
 * routine with nothing to correspond to. Everything it does to memory is here.
 */
void mouse_event(uint16_t buttons, uint16_t x, uint16_t y)
{
    g_mouse_buttons = (uint8_t)buttons;
    g_engine_mouse.mouse_x = x;
    g_engine_mouse.mouse_y = y;

    if (g_engine_mouse.mouse_handler_fn == NULL)
        return;

    mouse_save_vga();
    /* `lcall [0x4744]`. Nothing sets the pointer - see
       `mouse_set_user_handler` - so this is never reached. */
    g_engine_mouse.mouse_handler_fn();
    mouse_restore_vga();
}

/*
 * 0x23c99
 *
 * NEVER REACHED: only `mouse_event` calls this, on the branch that needs a
 * user handler, and nothing installs one. See `mouse_set_user_handler`.
 *
 * Save the VGA state the game's own mouse handler is about to disturb, into
 * DGROUP: graphics controller 0 and 1 at 0x48da, 4 at 0x48dc, 8 at 0x48de,
 * 3 at 0x48e2, and the sequencer's map mask at 0x48e0.
 *
 * Every one is read the same way - index the register, read the data port,
 * keep the old index in `ah` so it can go back - and then set to the value a
 * plain write needs: the set/reset registers to 0, the bit mask to 0xff, the
 * function select to 0, and the map mask to 0xf.
 *
 * The **read-modify-write latch** is the odd part. Between writing register 5
 * and restoring it the routine does `stosb` to `a000:ffff`, and on the way back
 * out reads the same byte. That is not a pixel: writing a byte through the VGA
 * loads the four latches from it, and reading one does the same, so this is how
 * the handler's own read-modify-write state is parked and picked up again. The
 * byte it uses is the very last in the aperture, which no mode this game runs
 * displays.
 *
 * Ours only in that the port has no latch to park: `vga_write`/`vga_read` in
 * io.c model the latches, so the same two accesses do the same thing here.
 */
void mouse_save_vga(void)
{
    uint16_t v;

    io_out8(PORT_GC_INDEX, 0);
    v = (uint16_t)(io_in8(PORT_GC_INDEX) << 8);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    g_saved_gc_0_1 = (int16_t)v;
    io_out8(PORT_GC_INDEX, 0);
    io_out8(PORT_GC_DATA, 0);

    io_out8(PORT_GC_INDEX, 1);
    v = (uint16_t)(io_in8(PORT_GC_DATA) << 8);
    io_out8(PORT_GC_DATA, 0);

    io_out8(PORT_GC_INDEX, 4);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    g_saved_gc_4 = (int16_t)v;

    io_out8(PORT_GC_INDEX, 5);
    v = (uint16_t)(io_in8(PORT_GC_DATA) << 8);
    io_out8(PORT_GC_DATA, g_gc_mode_copy);
    vga_write(0xffff, g_gc_mode_copy);          /* park the latches */
    io_out8(PORT_GC_DATA, g_gc_mode_fill);

    io_out8(PORT_GC_INDEX, 8);
    v = (uint16_t)(v | io_in8(PORT_GC_DATA));
    g_saved_gc_8 = (int16_t)v;
    io_out8(PORT_GC_DATA, 0xff);

    io_out8(PORT_GC_INDEX, 3);
    g_saved_gc_3 = io_in8(PORT_GC_DATA);
    io_out8(PORT_GC_DATA, 0);

    v = (uint16_t)(io_in8(PORT_SEQ_INDEX) << 8);
    io_out8(PORT_SEQ_INDEX, 2);
    v = (uint16_t)(v | io_in8(PORT_SEQ_DATA));
    g_saved_seq_map_mask = (int16_t)v;
    io_out8(PORT_SEQ_DATA, 0x0f);
}

/*
 * 0x23cfe
 *
 * NEVER REACHED, for the same reason as `mouse_save_vga`.
 *
 * Put back what `mouse_save_vga` took, in the reverse order, index register
 * last so the card is left selecting whatever it was selecting before. The
 * latch byte at `a000:ffff` is read rather than written this time, which
 * reloads the latches from it.
 */
void mouse_restore_vga(void)
{
    uint16_t v;

    io_out8(PORT_SEQ_INDEX, 2);
    v = (uint16_t)g_saved_seq_map_mask;
    io_out8(PORT_SEQ_DATA, (uint8_t)v);
    io_out8(PORT_SEQ_INDEX, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 3);
    io_out8(PORT_GC_DATA, g_saved_gc_3);

    io_out8(PORT_GC_INDEX, 8);
    v = (uint16_t)g_saved_gc_8;
    io_out8(PORT_GC_DATA, (uint8_t)v);

    io_out8(PORT_GC_INDEX, 5);
    io_out8(PORT_GC_DATA, g_gc_mode_copy);
    (void)vga_read(0xffff);                  /* pick the latches back up */
    io_out8(PORT_GC_DATA, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 4);
    v = (uint16_t)g_saved_gc_4;
    io_out8(PORT_GC_DATA, (uint8_t)v);

    io_out8(PORT_GC_INDEX, 1);
    io_out8(PORT_GC_DATA, (uint8_t)(v >> 8));

    io_out8(PORT_GC_INDEX, 0);
    v = (uint16_t)g_saved_gc_0_1;
    io_out8(PORT_GC_DATA, (uint8_t)v);
    io_out8(PORT_GC_INDEX, (uint8_t)(v >> 8));
}

/*
 * 0x23d57
 *
 * **Let the mouse go.** The flag at DGROUP 0x48ea says the driver was taken
 * over; clearing it, INT 33h AX=0 resets the driver and AX=0x0C with ES:DX
 * zero takes the event handler off it. Answers 1 if it did the work.
 */
int16_t remove_mouse(void)
{
    if (g_mouse_taken == 0)
        return 0;

    g_mouse_taken = 0;

    io_mouse_reset();
    io_mouse_set_handler(0, NULL);

    return 1;
}

/*
 * 0x23d73
 *
 * If the flag byte at DGROUP 0x48ea is set, store a quarter of each of the two
 * words at DGROUP 0x4740 and 0x4742 through the two near pointers passed in.
 * If it is clear, both are left alone - the routine writes nothing at all,
 * which a caller that did not initialise them would notice.
 *
 * The `neg`/`jae` pair again: carry is set exactly when the byte was non-zero.
 */
void read_mouse_pointer(int16_t *x, int16_t *y)
{
    if (g_mouse_taken == 0)
        return;
    *x = (int16_t)(g_engine_mouse.mouse_x >> 2);
    *y = (int16_t)(g_engine_mouse.mouse_y >> 2);
}

/*
 * 0x23d9d
 *
 * Put the mouse cursor at a given cell: INT 33h AX=4, with the position
 * multiplied by four into DGROUP 0x4740 and 0x4742.
 *
 * The two DGROUP words are written whether or not the driver is there, but only
 * when the flag at 0x48ea says it is - `neg al` sets carry for any non-zero
 * byte, and `jae` skips everything on a zero one. Answers 1 when it moved the
 * cursor and 0 when there was no mouse.
 *
 * The interrupt itself is not reproduced: the port has no mouse driver, and the
 * call leaves nothing in guest memory to compare. What it writes to DGROUP is
 * what anything else can see.
 */
uint16_t mouse_move_to(uint16_t x, uint16_t y)
{
    if (g_mouse_taken == 0)
        return 0;

    g_engine_mouse.mouse_x = (int16_t)(x << 2);
    g_engine_mouse.mouse_y = (int16_t)(y << 2);

    /*
     * `mov ax,4 / int 0x33` - the driver's "set cursor position", with the
     * quartered coordinates already in CX and DX. The port had the two DGROUP
     * words and not the call, so the pointer was never actually warped.
     */
    io_mouse_move_to((uint16_t)(x << 2), (uint16_t)(y << 2));

    return 1;
}

/*
 * 0x23dc8
 *
 * Answer bit 0 of one of two flag bytes, or 0 if the first of them is clear.
 *
 * The original tests with `neg` and `jae`: `neg` leaves the carry flag set
 * exactly when its operand was non-zero, which is how this reads a byte and
 * branches on it without a compare. When DGROUP 0x48ea is zero the negation
 * leaves zero and the final AND answers 0; otherwise the byte at 0x48eb is
 * taken, shifted right once if the argument is non-zero, and its bit 0
 * returned.
 */
int16_t read_mouse_button(uint16_t which)
{
    uint16_t v = g_mouse_taken;

    if (v == 0)
        return 0;

    v = g_mouse_buttons;
    if (which != 0)
        v >>= 1;
    return (int16_t)(v & 1);
}

/*
 * 0x23deb
 *
 * **Normalise a far pointer**: carry the paragraphs out of the offset into the
 * segment, leaving an offset of at most 15. AX holds the offset and DX the
 * segment; `AX >> 4` paragraphs move into DX and the offset is masked to four
 * bits.
 *
 * *This was first transcribed under the name `fixed_normalise`, as though AX
 * held a fixed-point fraction.* The behaviour is the same either way - it is
 * the same six instructions - but the name was wrong, and 0x222c6 is what
 * settled it: that routine calls this on the offset/segment halves of two far
 * pointers and then copies between them with `movsb`, which only makes sense
 * for pointer normalisation. A wrong name survives longer than a wrong line,
 * so it is corrected here and the correction recorded.
 *
 * A near routine taking and answering registers - AX and DX, the offset and
 * the segment of one far pointer - so the port passes that pointer by
 * reference: the segment plus the offset's paragraphs, and the offset's low
 * four bits.
 */
void normalise_pointer(uint8_t far **p)
{
    /* Carry the offset's paragraphs into the segment. A host pointer is one
       address, and normalising changes nothing about it. */
    (void)p;
}

/*
 * 0x23e1a
 *
 * Add a signed 32-bit byte count to a far pointer, the offset in `AX` and
 * segment in `DX`, the count in `CX:BX`. This is the **positive** door; the
 * negative one is at 0x221a4 and normalises afterwards, which this does not.
 *
 * The carry out of the offset add becomes 0x1000 paragraphs on the segment,
 * built without a branch: `sbb bx,bx` makes -1 or 0 and `and bx,0x1000` picks
 * the bit.
 *
 * The high half is folded in with **one `rcr bx,5`** after a `clc`, which is a
 * seventeen-bit rotate: the result is `(cx >> 5) | ((cx & 0xf) << 12)`. The
 * second term is the `cx * 0x1000` the arithmetic wants; the first is a
 * leftover that is zero only while `cx` is under 32, which for a count under
 * two megabytes it is. Transcribed as the rotate it is rather than as the
 * multiply it stands for.
 */
uint8_t far *huge_add_positive(uint8_t far *p, uint32_t delta)
{
    uint16_t lo = (uint16_t)delta;              /* BX */
    uint16_t hi = (uint16_t)(delta >> 16);      /* CX */
    uint32_t sum = (uint32_t)FP_OFF(p) + lo;
    dg_seg_t seg = FP_SEG(p);

    if (sum > 0xffff)
        seg = seg + 0x1000;

    /* The rotate takes the high word alone, which is why it is split out of
       the count here rather than shifted as a whole. */
    seg = seg + (uint16_t)((hi >> 5) | ((hi & 0xf) << 12));

    return MK_FP(seg, (uint16_t)sum);
}

/*
 * 0x23e77
 *
 * Copy `count` bytes between two far pointers, with a **32-bit** count, safe
 * when the two overlap. Answers the destination it was given, unnormalised.
 *
 * The original is this written for an 8086: it normalises both pointers,
 * compares them, and dispatches through a pair of function pointers it stores
 * at `cs:[0x5f99]` and `cs:[0x5f9b]` - the normaliser (0x22161 going up,
 * 0x22173 going down) and the copy loop (0x221d6 up, 0x221bf down, the latter
 * under `std`). It works in chunks of at most 0x7d00 bytes, renormalising at
 * the top of each so an offset can never carry past 64K, and each loop copies
 * one byte first where that aligns the destination and then moves words.
 *
 * **That machinery is not reconstructed.** All of it - the indirect calls, the
 * word moves, the chunking, the alignment step - is there to make the copy fast
 * on a 16-bit machine. The port has a flat address space and none of those
 * costs, and what the two artefacts have to agree on is which bytes end up
 * where, which is exactly `memmove`'s contract.
 *
 * So the port takes two pointers and a count. The original's own `seg:off`
 * arithmetic was the *only* thing that forced its source to be a guest
 * address - it never read the source, it indexed memory by the linear address
 * it computed - and with that gone `load_palette` hands it an ordinary C
 * array.
 *
 * One thing does not survive the change. The routine answered the destination
 * pair it was given, unnormalised, in DX:AX; a pointer cannot spell an
 * unnormalised pair, so it answers the destination pointer and `routines.def`
 * says RET_NONE. Nothing reads the answer: `load_palette` is the only caller
 * and drops it, and `syms.c` does not dispatch this routine.
 *
 * The two dispatch words *do* survive, and getting them wrong was the first
 * thing the verifier said - `0x5f11/0x5f86` against `0x5f23/0x5f6f`, two bytes
 * of a 578-byte write. They are written from the same comparison the original
 * makes. They are *data* that happens to sit in a code segment, the same as
 * the saved timer vector further up this module. Nothing reads them back.
 */
uint8_t far * huge_move(uint8_t far * dst, const uint8_t far * src, uint32_t count)
{
    /* The original's dispatch offsets - `normalise_pointer` at 0x22161 and the
       forward copy at 0x221d6 - stored for the comparison's sake only: this
       body calls neither through them. Overwritten below if the copy has to
       go down. */
    g_s1c_huge_move.normalise_off = 0x5f11;
    g_s1c_huge_move.copy_off = 0x5f86;

    /* The original compares the two linear addresses; the host's pointers
       are them. */
    if (src < (const uint8_t far *)dst) {
        /* The normalise that steps back a paragraph, and the backward copy. */
        g_s1c_huge_move.normalise_off = 0x5f23;
        g_s1c_huge_move.copy_off = 0x5f6f;
    }

    memmove((void *)(uintptr_t)dst, (const void *)(uintptr_t)src, count);

    return dst;
}

/*
 * 0x23f50
 *
 * Copy `count` bytes between two far pointers, normalising both first so that
 * each offset is under 16 and the segment carries the paragraphs.
 *
 * **The alignment step is dead code, in the original.** It reads
 *
 *     test di, 1
 *     jae  skip
 *     movsb
 *     dec  cx
 *
 * and `test` always clears the carry flag, so `jae` is always taken: the byte
 * that would have aligned the destination is never copied. It was presumably
 * meant to be `jz`. Transcribed as it behaves, not as it was meant, with the
 * dead branch recorded here rather than silently reinstated.
 *
 * The tail is `shr cx,1 / rep movsw / rcl cx,1 / rep movsb`: the shift puts the
 * odd bit into carry, the words are copied, and the rotate brings that bit back
 * into a count of 0 or 1 for the trailing byte. No compare anywhere.
 */
void far_memcpy(uint8_t far * dst, const uint8_t far * src, uint16_t count)
{
    uint16_t words;

    if (count == 0)
        return;

    /*
     * The original normalises both pairs and then steps the offsets 16-bit,
     * which is `rep movsw` on a machine with segments. A host pointer is that
     * normalised address already, and the wrap goes with the pair - measured
     * across the intro, the briefing, the picker and all twenty-eight level
     * snapshots, neither end ever reached `off + count > 0x10000`.
     *
     * The words-then-a-byte shape is kept: it decides which byte of an odd
     * count is copied last, and the word is read at an odd address the
     * way the guest's unaligned `movsw` does.
     */
    words = (uint16_t)(count >> 1);
    while (words--) {
        *(int16_t *)(dst) = *(int16_t *)(src);
        src += 2;
        dst += 2;
    }
    if (count & 1)
        *dst = *src;
}

/*
 * 0x23f8a
 *
 * Set `count` bytes to the low byte of `value`, starting at a far pointer.
 *
 * **The machinery is not reconstructed**, on the same reasoning as `huge_move`
 * above: the original works in chunks of at most 0x7d00 bytes, renormalising
 * the pointer at the top of each so an offset can never carry past 64K, writes
 * one byte first where that makes the destination even, and then moves words
 * with both halves of the value. Every part of that is there to make the fill
 * fast on a 16-bit machine, and none of it changes which bytes end up holding
 * what - which is `memset`'s contract.
 *
 * The offset stepping was the only thing that made the destination a guest
 * address: `off = (uint16_t)(off + 2)` is a 16-bit add, and the renormalisation
 * is what keeps it from wrapping. Neither survives as a pointer and neither
 * needs to.
 */
void far_memset(uint8_t far * dst, uint16_t value, uint32_t count)
{
    memset((void *)(uintptr_t)dst, (int)(value & 0xff), count);
}

/*
 * 0x23fe4
 *
 * **Compare two far pointers**, each normalised first: -1 below, 0 equal,
 * 1 above. Nothing calls it.
 */
int16_t far_ptr_compare(const uint8_t far *a, const uint8_t far *b)
{
    if (a < b)
        return -1;
    return a == b ? 0 : 1;
}

/*
 * 0x24010
 *
 * The far-callable face of `normalise_pointer` at 0x22161: load the pointer
 * into AX and DX, call the near routine, and let its registers be the result.
 * So this answers a normalised far pointer in DX:AX, like any other far
 * routine returning a long.
 */
uint8_t far *normalise_pointer_far(uint8_t far *p)
{
    normalise_pointer(&p);
    return p;
}

/*
 * 0x2401e
 *
 * Take over INT 0, the divide-by-zero trap. The old vector is kept at DGROUP
 * 0x48ed and the new one points at 0x616e in this code segment - the handler
 * that begins immediately after this routine. DGROUP 0x48ec records that it
 * has been done.
 *
 * The original writes the vector table directly, at absolute 0; the port
 * hands the handler to `setvect`, whose table stands for it.
 *
 * The two halves are stored the other way round from how they are read: the
 * offset from 0:0 goes to 0x48ef and the segment from 0:2 to 0x48ed, so the
 * saved pair is segment-first.
 */
void install_divide_trap(void)
{
    g_divide_hooked = 1;

    g_old_divide_vector = getvect(0);

    setvect(0, (void interrupt (far *)())divide_error_handler);
}

/*
 * 0x24048
 *
 * **The divide-error handler** `install_divide_trap` puts at INT 0: it steps
 * the saved IP over the faulting `div` and returns. An interrupt handler;
 * the host takes no INT 0, so it has no body to run. A stub, which aborts.
 */
void divide_error_handler(void)
{
    not_transcribed("0x223be, the divide-error handler");
}

/*
 * 0x24081
 *
 * **A restore that restores nothing**, and it is the original's and not a
 * transcription slip.
 *
 * The flag at DGROUP 0x48ec is cleared, and then two pairs of instructions
 * that look like they put vector 0 back - `mov ax,[0x48ef]` then
 * `mov ax,es:[0]`, and the same for [0x48ed] and es:[2] - are **both loads**.
 * The bytes are `a1 ef 48 26 a1 00 00`; a store would be `26 a3`. So the saved
 * words are read into AX and thrown away, and the vector at 0000:0000 is read
 * and thrown away too. Nothing in memory changes.
 *
 * Written out as the flag clear it is, with the loads left off because a load
 * into a register nothing reads is not something C can express and not
 * something anything can observe.
 */
void restore_int0_vector(void)
{
    if (g_divide_hooked == 0)
        return;

    g_divide_hooked = 0;
}

/*
 * 0x240a5
 *
 * Read a pixel if it is inside the driver's clip window, and answer -1 if it
 * was not.
 *
 * The same guard as `plot_pixel_clipped` at 0x2244d, on the same four inclusive
 * bounds and the same on/off byte at 0x3893, differing only in taking two
 * arguments instead of three and jumping through DGROUP 0x439a. The two sit
 * next to each other in the image and are plainly one pair.
 *
 * -1 for "outside" is not distinguishable from a legitimately read colour only
 * because colours here are 0..15; any answer above that is the clip.
 */
int16_t read_pixel_clipped(int16_t x, int16_t y)
{
    if (g_vmds.clip_enabled != 0) {
        if (x < g_vmds.clip_left)
            return -1;
        if (x > g_vmds.clip_right)
            return -1;
        if (y < g_vmds.clip_top)
            return -1;
        if (y > g_vmds.clip_bottom)
            return -1;
    }

    return (int16_t)vm_read_pixel(x, y);
}

/*
 * 0x240d7
 *
 * Plot a pixel if it is inside the driver's clip window, and answer -1 if it
 * was not.
 *
 * The window is four words in the driver's data block - 0x3894 and 0x3896 for
 * x, 0x3898 and 0x389a for y - and both bounds are **inclusive**, tested with
 * `jl` and `jg`. The byte at 0x3893 switches the whole test off, and with it
 * zero any coordinate is passed straight through.
 *
 * The call into the driver is an `ljmp` through DGROUP 0x439e, not a call, so
 * the driver runs on this frame, sees the same three arguments, and returns
 * directly to whoever called here. The third argument is the colour and is
 * never looked at on the way past.
 *
 * That also means the answer on the drawn path is not chosen: it is whatever
 * the driver left in AX, which is 0xff08. Only the clipped path returns a
 * deliberate value.
 */
int16_t plot_pixel_clipped(int16_t x, int16_t y, int16_t colour)
{
    if (g_vmds.clip_enabled != 0) {
        if (x < g_vmds.clip_left)
            return -1;
        if (x > g_vmds.clip_right)
            return -1;
        if (y < g_vmds.clip_top)
            return -1;
        if (y > g_vmds.clip_bottom)
            return -1;
    }

    return (int16_t)vm_plot_pixel(x, y, (uint8_t)colour);
}

/*
 * 0x24109
 *
 * A thunk into the video driver: `ljmp [0x4362]`, which is `vm_restore_rect`.
 * Same arrangement as 0x2149a.
 */
void restore_rect_thunk(const uint8_t far * buf, int16_t x,
                        int16_t y, int16_t w, int16_t h)
{
    vm_restore_rect(buf, x, y, w, h);
}
