/*
 * OURS: which loaded driver a driver call goes to.
 *
 * The original needs nothing like this file. Every call into `SX.OVL` is an
 * `lcall cs:[0x1e7]` with the function number in BP, so the call site names a
 * *function*, never a driver, and whichever chunk the loader put there answers.
 * The port has C functions with names, so a call site has to pick one, and
 * that is all this file does: one routine per function number, dispatching on
 * the driver that is actually loaded.
 *
 * The register conventions are the original's and are not softened here: AL is
 * the channel, CH and CL the two data bytes, and a parameter of 0xff means
 * "read without writing". A driver that ignores a register still gets it.
 *
 * **Adding a driver is adding a case.** `docs/sound-driver.md` lists the nine
 * the game knows and the two the port has bodies for.
 */
#include <string.h>

#include "dgroup.h"
#include "io.h"
#include "tim.h"

enum { DRIVER_NONE, DRIVER_SPKR, DRIVER_GMD };

/*
 * OURS: which driver is loaded, read from the driver's own banner.
 *
 * Every chunk in `SX.OVL` carries its name at offset 0x0a, up to a `%`:
 * `stddrv%IBM PC or Compatible Internal Speaker` and `dude%General MIDI for
 * Roland MPU interface`. That is the driver saying what it is, which beats
 * the port remembering what it asked for - `setup_sound_device` can fall back
 * to a driver other than the one `RESOURCE.CFG` named.
 */
static int32_t driver_kind(void)
{
    const char *banner;

    if (SX_SEG == 0)
        return DRIVER_NONE;

    banner = (const char *)FAR_PTR(SX_SEG, 0x0a);

    if (memcmp(banner, "stddrv%", 7) == 0)
        return DRIVER_SPKR;
    if (memcmp(banner, "dude%", 5) == 0)
        return DRIVER_GMD;

    return DRIVER_NONE;
}

/* OURS: function 0, as every routine in this file is. */
void driver_describe_0(uint16_t *ax, uint16_t *cx)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  gmd_describe_0(ax, cx); return;
    case DRIVER_SPKR: sx_describe_0(ax, cx); return;
    default:          *ax = 0; *cx = 0; return;
    }
}

/*
 * OURS: function 1, which is the one place the two drivers disagree about their
 * arguments: the speaker answers two constants and the General MIDI driver
 * initialises itself from the patch bank at ES:AX. Both get the pointer.
 */
void driver_describe_1(uint16_t off, uint16_t seg, uint16_t *ax, uint16_t *cx)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  gmd_init(off, seg, ax, cx); return;
    case DRIVER_SPKR: sx_describe_1(ax, cx); return;
    default:          *ax = 0xffff; *cx = 0; return;
    }
}

/* OURS: function 2. */
void driver_stop_all(void)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  gmd_stop_all(); return;
    case DRIVER_SPKR: sx_stop_all(); return;
    default:          return;
    }
}

/* OURS: function 4. AL is the channel, CH the note, CL the velocity. */
void driver_stop_note(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  gmd_stop_note(ax, cx); return;
    case DRIVER_SPKR: sx_stop_note(cx); return;
    default:          return;
    }
}

/* OURS: function 5. */
void driver_start_note(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  gmd_start_note(ax, cx); return;
    case DRIVER_SPKR: sx_start_note(ax, cx); return;
    default:          return;
    }
}

/* OURS: function 6, which both drivers answer with nothing. */
void driver_nop(void)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  gmd_nop(); return;
    case DRIVER_SPKR: sx_nop(); return;
    default:          return;
    }
}

/* OURS: function 7. CH is the controller, CL its value. */
void driver_controller(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  gmd_controller(ax, cx); return;
    case DRIVER_SPKR: sx_controller(ax, cx); return;
    default:          return;
    }
}

/* OURS: function 10. */
void driver_pitch_bend(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  gmd_pitch_bend(ax, cx); return;
    case DRIVER_SPKR: sx_pitch_bend(ax, cx); return;
    default:          return;
    }
}

/* OURS: function 11. */
uint16_t driver_param_349(uint16_t cl)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  return gmd_param_349((uint8_t)cl);
    case DRIVER_SPKR: return sx_param_349(cl);
    default:          return 0;
    }
}

/* OURS: function 12, the master level. */
uint16_t driver_param_345(uint16_t cl)
{
    switch (driver_kind()) {
    case DRIVER_GMD:  return gmd_param_345((uint8_t)cl);
    case DRIVER_SPKR: return sx_param_345(cl);
    default:          return 0;
    }
}
