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

enum { DRIVER_NONE, DRIVER_SPKR, DRIVER_ADL, DRIVER_SBP };

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
    if (banner_says(banner, "AdLib"))
        return DRIVER_ADL;
    if (banner_says(banner, "Sound Blaster Pro"))
        return DRIVER_SBP;

    /*
     * A driver is loaded and it is not one the port has a body for -
     * `M32:`, `PRO:`, `PS1:` or `NLD:`. **Answering zero here would be
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
    case DRIVER_SBP:  sbp_describe_0(ax, cx); return;
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
    case DRIVER_SBP:  sbp_init(off, seg, ax, cx); return;
    case DRIVER_SPKR: sx_describe_1(ax, cx); return;
    default:          *ax = 0xffff; *cx = 0; return;
    }
}

/*
 * OURS: function 2. It carries CX because `SBP:` reads CL - see the note on
 * `sbp_stop_all`, which or-s it into the mixer's FM volume. `ADL:` and the
 * speaker ignore it, as does the original's function 2 for those two drivers.
 */
void driver_stop_all(uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_stop_all(); return;
    case DRIVER_SBP:  sbp_stop_all(cx); return;
    case DRIVER_SPKR: sx_stop_all(); return;
    default:          return;
    }
}

/* OURS: function 4. AL is the channel, CH the note, CL the velocity. */
void driver_stop_note(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_stop_note(ax, cx); return;
    case DRIVER_SBP:  sbp_stop_note(ax, cx); return;
    case DRIVER_SPKR: sx_stop_note(cx); return;
    default:          return;
    }
}

/* OURS: function 5. */
void driver_start_note(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_start_note(ax, cx); return;
    case DRIVER_SBP:  sbp_start_note(ax, cx); return;
    case DRIVER_SPKR: sx_start_note(ax, cx); return;
    default:          return;
    }
}

/* OURS: function 6, which both drivers answer with nothing. */
void driver_nop(void)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_nop(); return;
    case DRIVER_SBP:  sbp_nop(); return;
    case DRIVER_SPKR: sx_nop(); return;
    default:          return;
    }
}

/* OURS: function 7. CH is the controller, CL its value. */
void driver_controller(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_controller(ax, cx); return;
    case DRIVER_SBP:  sbp_controller(ax, cx); return;
    case DRIVER_SPKR: sx_controller(ax, cx); return;
    default:          return;
    }
}

/*
 * OURS: function 8, the program change. AL is the channel and CL the patch.
 *
 * **This was `driver_nop` until 2026-09-06, and that was a real bug in the
 * music.** Entry 8 *is* the do-nothing stub in `SPKR:` - a speaker has no
 * patches, so a program change is genuinely nothing there - and that fact,
 * true of the driver transcribed first, was written into `sound.c`, which is
 * device-independent. It is not true of the others: `ADL:` sends entry 8 to
 * 0x1a1b and `SBP:` to 0x1a20, both of which file the patch per channel. The
 * stub in those two is 0x1951 and 0x1956, which entries 3, 6, 9 and 14 to 16
 * point at.
 *
 * What it cost: every channel kept whatever patch it started with, so the
 * sequencer's program changes were parsed, filed at the sequence's +0x116, and
 * then dropped. Measured against a DOSBox capture over a window of 43
 * key-ons aligned by content, the port loaded **8 patches where the original
 * loads 20**, and used 2 timbres where it uses 8 - with the note writes
 * identical, 35 and 79, because only the instruments were wrong.
 */
void driver_program_change(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_program(ax, cx); return;
    case DRIVER_SBP:  sbp_program_change(ax, cx); return;
    case DRIVER_SPKR: sx_nop(); return;
    default:          return;
    }
}

/* OURS: function 10. */
void driver_pitch_bend(uint16_t ax, uint16_t cx)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  adl_pitch_bend(ax, cx); return;
    case DRIVER_SBP:  sbp_pitch_bend(ax, cx); return;
    case DRIVER_SPKR: sx_pitch_bend(ax, cx); return;
    default:          return;
    }
}

/* OURS: function 11. */
uint16_t driver_param_349(uint16_t cl)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  return adl_param_349(cl);
    case DRIVER_SBP:  return sbp_param_349(cl);
    case DRIVER_SPKR: return sx_param_349(cl);
    default:          return 0;
    }
}

/* OURS: function 12, the master level. */
uint16_t driver_param_345(uint16_t cl)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  return adl_param_345(cl);
    case DRIVER_SBP:  return sbp_param_345(cl);
    case DRIVER_SPKR: return sx_param_345(cl);
    default:          return 0;
    }
}

/*
 * OURS: function 13.
 *
 * **`SBP:` is not known to have this one and therefore stops.** `SPKR:` sends
 * it to 0x055b and `ADL:` to 0x1a68, both transcribed; the Sound Blaster Pro's
 * own do-nothing list at SBP:0x1956 covers functions 3, 6, 9, 14, 15 and 16
 * and **13 is not among them**, so it has a body somewhere that has not been
 * read. Answering 0 here would be exactly the mistake entry 8 was: a fact
 * about one driver written into code that serves all of them. So it aborts and
 * says which driver asked, and the first run that reaches it says what to go
 * and read.
 */
uint16_t driver_param_346(uint16_t cl)
{
    switch (driver_kind()) {
    case DRIVER_ADL:  return adl_param_346(cl);
    case DRIVER_SPKR: return sx_param_346(cl);
    case DRIVER_SBP:
        not_transcribed("SBP: function 13, which is not one of its stubs");
        return 0;
    default:          return 0;
    }
}

/*
 * OURS: the driver's single entry, by function number.
 *
 * The original has no such routine and does not need one: every call into
 * `SX.OVL` is
 *
 *     push bp
 *     mov  bp, <function>
 *     lcall cs:[0x1e7]
 *     pop  bp
 *
 * - fifty of them in the sound module - and the driver's own dispatcher
 * indexes its eighteen-entry table with BP. The port's callers name a C
 * function instead, so nothing in the port ever needed the number.
 *
 * **The hybrid does.** When the original's sound module runs under emulation
 * and the port stands in for the driver, what arrives is a function number in
 * a register, and this is what turns it back into a call. That is the only
 * caller: `tools/native/routines.def` binds it at the far pointer the game
 * itself stores at `((int16_t)SNDS.driver_off)`.
 *
 * Registers as the drivers read them: AL the channel, CH and CL the two data
 * bytes, ES:AX a far pointer for function 1, and the answer in AX - and in CX
 * as well for the two describes. They are passed whole rather than split
 * because a driver that ignores half of one still gets it, which is the rule
 * the rest of this file already follows.
 *
 * A number with no case **aborts**. Functions 0 to 13 are the ones the game
 * uses - counted in the image, at the fifty call sites - and 14 to 17 are
 * reached only from inside the driver, so a call here for one of those means
 * the reading of the call sites was wrong and not that a case is missing.
 */
void sx_driver_call(uint16_t fn, uint16_t *ax, uint16_t *cx, uint16_t es)
{
    uint16_t off = *ax;

    switch (fn) {
    case 0:  driver_describe_0(ax, cx);              return;
    case 1:  driver_describe_1(off, es, ax, cx);     return;
    case 2:  driver_stop_all(*cx);                   return;
    /*
     * 3 and 9 go to the same stub as 6, and that is **read off all three
     * drivers rather than carried over from one**:
     *
     *   SPKR:0x037a  entries 3, 6, 8, 9, 14, 15, 16
     *   SBP:0x1956   functions 3, 6, 9, 14, 15, 16
     *   ADL:0x1951   "what BP 3, 6, 9 and 14 to 16 all point at"
     *
     * Entry 8 is in the speaker's list and in neither of the others, which is
     * the whole of the bug this file carries a long note about - so the three
     * addresses are written out here rather than one of them being taken as
     * the rule. Function 3 is worth the care: it is the *most called* of the
     * eighteen, 211 times in nine hundred frames, so getting it wrong would be
     * silent and constant.
     */
    case 3:  driver_nop();                           return;
    case 4:  driver_stop_note(*ax, *cx);             return;
    case 5:  driver_start_note(*ax, *cx);            return;
    case 6:  driver_nop();                           return;
    case 7:  driver_controller(*ax, *cx);            return;
    case 8:  driver_program_change(*ax, *cx);        return;
    case 9:  driver_nop();                           return;
    case 10: driver_pitch_bend(*ax, *cx);            return;
    case 11: *ax = driver_param_349(*cx);            return;
    case 12: *ax = driver_param_345(*cx);            return;
    case 13: *ax = driver_param_346(*cx);            return;
    default: {
        static char what[80];

        snprintf(what, sizeof what,
                 "SX.OVL function %u, called with bp=%u", fn, fn);
        not_transcribed(what);
        return;
    }
    }
}
