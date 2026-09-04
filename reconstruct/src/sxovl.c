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
#include <stdio.h>
#include <string.h>

#include "dgroup.h"
#include "io.h"
#include "tim.h"

enum { DRIVER_NONE, DRIVER_SPKR, DRIVER_GMD, DRIVER_ADL };

/*
 * OURS: which driver is loaded, read from the driver's own banner.
 *
 * Every chunk in `SX.OVL` carries a banner at offset 0x0a, and its shape is a
 * short name, one **length** byte, and the description that many characters
 * long: `stddrv` 0x25 "IBM PC or Compatible Internal Speaker", and `dude` 0x25
 * "General MIDI for Roland MPU interface" - both descriptions being 37
 * characters, which is what 0x25 is.
 *
 * **The name does not identify the driver.** `SBP:` is also called `dude`, so
 * matching "dude%" told the two apart only by their descriptions happening to
 * be the same length, which is a coincidence and not a fact. The description
 * is what says which device this is, so that is what is matched - and it is
 * searched for rather than indexed, because the name in front of it is not a
 * fixed width either.
 *
 * Reading it from the driver still beats remembering what `RESOURCE.CFG` asked
 * for: `setup_sound_device` can fall back to a chunk other than the one named.
 */
static const char *driver_banner(void)
{
    return SX_SEG == 0 ? 0 : (const char *)FAR_PTR(SX_SEG, 0x0a);
}

/* OURS: does the banner contain this text, anywhere in its first 48 bytes? */
static int32_t banner_says(const char *banner, const char *what)
{
    int32_t i, n = (int32_t)strlen(what);

    for (i = 0; i + n <= 48; i++) {
        if (memcmp(banner + i, what, (size_t)n) == 0)
            return 1;
    }
    return 0;
}

/*
 * OURS: which of the nine devices is loaded, by what its banner says. The
 * comment above `driver_banner` has why the description and not the name.
 */
static int32_t driver_kind(void)
{
    const char *banner = driver_banner();

    if (banner == 0)
        return DRIVER_NONE;

    if (banner_says(banner, "IBM PC"))
        return DRIVER_SPKR;
    if (banner_says(banner, "General MIDI"))
        return DRIVER_GMD;
    if (banner_says(banner, "AdLib"))
        return DRIVER_ADL;

    /*
     * A driver is loaded and it is not one the port has a body for - `SBP:`,
     * `ADL:`, `M32:`, `PRO:`, `PS1:` or `NLD:`. **Answering zero here would be
     * a stub returning quietly**, which is the one thing a stub must not do:
     * the game would take the answer for a description of the device and carry
     * on with it. So it stops, and quotes what the driver calls itself.
     */
    {
        static char what[128];
        char desc[52];
        int32_t i;

        for (i = 0; i < 48; i++) {
            char c = banner[i];

            desc[i] = (c >= 0x20 && c < 0x7f) ? c : '.';
        }
        desc[48] = 0;

        snprintf(what, sizeof what,
                 "the sound driver \"%s\", loaded and not transcribed", desc);
        not_transcribed(what);
    }

    return DRIVER_NONE;
}

/* OURS: function 0, as every routine in this file is. */
void driver_describe_0(uint16_t *ax, uint16_t *cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_describe_0(ax, cx); return;
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
    case DRIVER_ADL:  adl_init(off, seg, ax, cx); return;
    case DRIVER_GMD:  gmd_init(off, seg, ax, cx); return;
    case DRIVER_SPKR: sx_describe_1(ax, cx); return;
    default:          *ax = 0xffff; *cx = 0; return;
    }
}

/* OURS: function 2. */
void driver_stop_all(void)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_stop_all(); return;
    case DRIVER_GMD:  gmd_stop_all(); return;
    case DRIVER_SPKR: sx_stop_all(); return;
    default:          return;
    }
}

/* OURS: function 4. AL is the channel, CH the note, CL the velocity. */
void driver_stop_note(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_stop_note(ax, cx); return;
    case DRIVER_GMD:  gmd_stop_note(ax, cx); return;
    case DRIVER_SPKR: sx_stop_note(cx); return;
    default:          return;
    }
}

/* OURS: function 5. */
void driver_start_note(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_start_note(ax, cx); return;
    case DRIVER_GMD:  gmd_start_note(ax, cx); return;
    case DRIVER_SPKR: sx_start_note(ax, cx); return;
    default:          return;
    }
}

/* OURS: function 6, which both drivers answer with nothing. */
void driver_nop(void)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_nop(); return;
    case DRIVER_GMD:  gmd_nop(); return;
    case DRIVER_SPKR: sx_nop(); return;
    default:          return;
    }
}

/* OURS: function 7. CH is the controller, CL its value. */
void driver_controller(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_controller(ax, cx); return;
    case DRIVER_GMD:  gmd_controller(ax, cx); return;
    case DRIVER_SPKR: sx_controller(ax, cx); return;
    default:          return;
    }
}

/* OURS: function 10. */
void driver_pitch_bend(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_pitch_bend(ax, cx); return;
    case DRIVER_GMD:  gmd_pitch_bend(ax, cx); return;
    case DRIVER_SPKR: sx_pitch_bend(ax, cx); return;
    default:          return;
    }
}

/* OURS: function 11. */
uint16_t driver_param_349(uint16_t cl)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  return adl_param_349(cl);
    case DRIVER_GMD:  return gmd_param_349((uint8_t)cl);
    case DRIVER_SPKR: return sx_param_349(cl);
    default:          return 0;
    }
}

/* OURS: function 12, the master level. */
uint16_t driver_param_345(uint16_t cl)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  return adl_param_345(cl);
    case DRIVER_GMD:  return gmd_param_345((uint8_t)cl);
    case DRIVER_SPKR: return sx_param_345(cl);
    default:          return 0;
    }
}
