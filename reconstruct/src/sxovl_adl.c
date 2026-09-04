/*
 * `ADL:`, the AdLib driver out of `SX.OVL`, as it is actually loaded.
 *
 * Provenance is `SX.OVL ADL:0xNNNN` - offsets within the loaded driver, not
 * image addresses, because the loader chooses the segment. **And it is not the
 * segment the other two use**: the speaker and General Midi drivers land at
 * 0x418f on these runs and this one at 0x502a, so it is read from
 * `SNDCS:0x1e7` rather than assumed. `out/res/SX_ADL.mem` is the dump every
 * offset here came from - 9,296 bytes, of which 5.4 KB is the zeroed state
 * below and 2.9 KB is code.
 *
 * This is the sound a Sound Blaster owner actually heard: the OPL2 is the
 * chip on an AdLib card and on every Sound Blaster, and this driver is what
 * programs it. The chip itself is not transcribed and cannot be - see
 * `src/opl.h` - so the line falls where it always does: which registers get
 * written and when is the driver's decision and is here; turning register
 * writes into sound is hardware and is ymfm's.
 *
 * The driver's own state lives in its code segment and is reached with `SX8`
 * and `SX16`, as the speaker's and General Midi's are.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * SX.OVL ADL:0x208e
 *
 * Write one of the chip's registers. The index goes to the address port and
 * the value to the data port, and between them the driver reads the address
 * port five times and afterwards thirty-three - the YM3812's 3.3 and 23
 * microseconds of settling, on a bus that took about a microsecond a read.
 *
 * **The reads are transcribed, not skipped.** They are what gives the chip
 * time to advance between writes, and `src/opl.h` records what happens
 * without them: every write of a driver tick lands at one instant, envelopes
 * never get their attack, and the music comes out hollow and half as loud.
 * `io.c` turns them into port reads and `opl_write` advances the chip; both
 * halves are needed and neither is decoration.
 *
 * The three port numbers are variables at `cs:0x37`, `cs:0x39` and `cs:0x3b`,
 * which is why searching this driver's bytes for 0x0388 finds them in what
 * looks like a table.
 */
void adl_write(uint16_t reg, uint16_t val)
{
    uint16_t dx;
    uint16_t cx;

    dx = (uint16_t)SX16(0x37);
    io_out8(dx, (uint8_t)reg);
    (void)io_in8(dx);
    (void)io_in8(dx);
    (void)io_in8(dx);
    (void)io_in8(dx);
    (void)io_in8(dx);

    dx = (uint16_t)SX16(0x3b);
    io_out8(dx, (uint8_t)val);

    dx = (uint16_t)SX16(0x39);
    for (cx = 0x21; cx != 0; cx--)
        (void)io_in8(dx);
}

/*
 * SX.OVL ADL:0x216b
 *
 * Register 0xBD: the depth bits and the rhythm mode. `cs:0x1889` and
 * `cs:0x188a` are the two depths and `cs:0x188c` the rhythm bits, which this
 * game leaves clear - it uses the nine melodic voices and no percussion mode.
 */
void adl_write_bd(void)
{
    uint16_t cx = 0;

    if (SX8(0x1889) != 0)
        cx |= 0x80;
    if (SX8(0x188a) != 0)
        cx |= 0x40;
    cx |= SX8(0x188c);

    adl_write(0xbd, cx);
}

/*
 * SX.OVL ADL:0x2194
 *
 * Register 8, which on an OPL2 is CSM and the note-select bit. Only the
 * latter is used, from `cs:0x1888`.
 */
void adl_write_nts(void)
{
    adl_write(8, (uint16_t)(SX8(0x1888) != 0 ? 0x40 : 0));
}

/*
 * OURS: one operator's fourteen bytes of unpacked patch, at `cs:0x274`.
 *
 * The driver indexes it as `slot * 14` in five different routines, each
 * building the multiply out of shifts and adds because an 8086 has no
 * immediate `imul`. The offsets within an entry are the driver's own.
 */
#define ADL_OP(slot)        (uint16_t)(0x274 + (slot) * 14)
#define ADL_OP_KSL          0            /* +0x274 */
#define ADL_OP_MULT         1            /* +0x275 */
#define ADL_OP_FEEDBACK     2            /* +0x276 */
#define ADL_OP_ATTACK       3            /* +0x277 */
#define ADL_OP_SUSTAIN      4            /* +0x278 */
#define ADL_OP_EGTYPE       5            /* +0x279 */
#define ADL_OP_DECAY        6            /* +0x27a */
#define ADL_OP_RELEASE      7            /* +0x27b */
#define ADL_OP_LEVEL        8            /* +0x27c */
#define ADL_OP_AM           9            /* +0x27d */
#define ADL_OP_VIB          10           /* +0x27e */
#define ADL_OP_KSR          11           /* +0x27f */
#define ADL_OP_CONNECT      12           /* +0x280 */
#define ADL_OP_WAVE         13           /* +0x281 */

/*
 * SX.OVL ADL:0x21ac
 *
 * Register 0x40 plus the operator's own offset: the key-scale level in the
 * top two bits and the total level in the low six. `cs:0x222` maps a slot to
 * the register offset, which is the OPL2's 0,1,2,8,9,0xa,0x10... layout and
 * not a straight index.
 */
void adl_write_level(uint16_t slot)
{
    uint16_t at = ADL_OP(slot);
    uint16_t cx;

    cx = (uint16_t)((uint8_t)(SX8((uint16_t)(at + ADL_OP_KSL)) << 6));
    cx |= (uint16_t)(SX8((uint16_t)(at + ADL_OP_LEVEL)) & 0x3f);

    adl_write((uint16_t)(0x40 + SX8((uint16_t)(0x222 + slot))), cx);
}

/*
 * SX.OVL ADL:0x2244
 *
 * Register 0x60: attack in the high nibble, decay in the low.
 */
void adl_write_attack_decay(uint16_t slot)
{
    uint16_t at = ADL_OP(slot);
    uint16_t cx;

    cx = (uint16_t)((uint8_t)(SX8((uint16_t)(at + ADL_OP_ATTACK)) << 4));
    cx |= (uint16_t)(SX8((uint16_t)(at + ADL_OP_DECAY)) & 0x0f);

    adl_write((uint16_t)(0x60 + SX8((uint16_t)(0x222 + slot))), cx);
}

/*
 * SX.OVL ADL:0x228a
 *
 * Register 0x80: sustain level in the high nibble, release in the low.
 */
void adl_write_sustain_release(uint16_t slot)
{
    uint16_t at = ADL_OP(slot);
    uint16_t cx;

    cx = (uint16_t)((uint8_t)(SX8((uint16_t)(at + ADL_OP_SUSTAIN)) << 4));
    cx |= (uint16_t)(SX8((uint16_t)(at + ADL_OP_RELEASE)) & 0x0f);

    adl_write((uint16_t)(0x80 + SX8((uint16_t)(0x222 + slot))), cx);
}

/*
 * SX.OVL ADL:0x21f4
 *
 * Register 0xC0 - feedback and the connection bit - and it is per *channel*,
 * not per operator, so it uses the channel table at `cs:0x234` and is skipped
 * for a slot the table at `cs:0x210` marks as not owning one.
 *
 * The connection bit is set when `cs:[+0x280]` is **zero**, which reads
 * backwards until you notice the byte is the patch's "additive" flag inverted.
 */
void adl_write_feedback(uint16_t slot)
{
    uint16_t at, cx;

    if (SX8((uint16_t)(0x210 + slot)) != 0)
        return;

    at = ADL_OP(slot);
    cx = (uint16_t)(SX8((uint16_t)(at + ADL_OP_FEEDBACK)) << 1);
    if (SX8((uint16_t)(at + ADL_OP_CONNECT)) == 0)
        cx++;

    adl_write((uint16_t)(0xc0 + SX8((uint16_t)(0x234 + slot))),
              (uint16_t)(cx & 0x0f));
}

/*
 * SX.OVL ADL:0x22d0
 *
 * Register 0x20: tremolo, vibrato, the sustaining envelope, key-scale rate
 * and the frequency multiplier - five fields the patch keeps as five separate
 * bytes and the chip wants in one.
 */
void adl_write_mult(uint16_t slot)
{
    uint16_t at = ADL_OP(slot);
    uint16_t cx = 0;

    if (SX8((uint16_t)(at + ADL_OP_AM)) != 0)
        cx |= 0x80;
    if (SX8((uint16_t)(at + ADL_OP_VIB)) != 0)
        cx |= 0x40;
    if (SX8((uint16_t)(at + ADL_OP_EGTYPE)) != 0)
        cx |= 0x20;
    if (SX8((uint16_t)(at + ADL_OP_KSR)) != 0)
        cx |= 0x10;
    cx |= (uint16_t)(SX8((uint16_t)(at + ADL_OP_MULT)) & 0x0f);

    adl_write((uint16_t)(0x20 + SX8((uint16_t)(0x222 + slot))), cx);
}

/*
 * SX.OVL ADL:0x233e
 *
 * Register 0xE0, the waveform - and **only when wave select is enabled** at
 * `cs:0x188d`, because an OPL2 with that bit clear has one waveform and
 * writing 0xE0 at all is meaningless. `adl_reset` is what sets it.
 */
void adl_write_wave(uint16_t slot)
{
    uint16_t at;

    if (SX16(0x188d) == 0)
        return;

    at = ADL_OP(slot);
    adl_write((uint16_t)(0xe0 + SX8((uint16_t)(0x222 + slot))),
              SX8((uint16_t)(at + ADL_OP_WAVE)));
}

/*
 * SX.OVL ADL:0x1eee
 *
 * Bend a note, given in quarter-tones, by the channel's pitch wheel.
 *
 * The wheel is a word per channel at `cs:0x170` with 0x2000 as centre, and
 * the distance from centre is divided by 0xab - 171 - which puts a full bend
 * at 48 quarter-tones, one octave. The answer is clamped to 0x1fc, which is
 * note 127 in quarter-tones and the top of the table.
 */
uint16_t adl_bend(uint16_t voice, uint16_t cx)
{
    uint16_t si = (uint16_t)(SX8((uint16_t)(voice + 0x190)) * 2);
    uint16_t di = (uint16_t)SX16((uint16_t)(si + 0x170));
    uint16_t ax;
    uint8_t  dl;

    if (di == 0x2000) {
        ax = 0;
        dl = 0;
    } else if (di > 0x2000) {
        ax = (uint16_t)(di - 0x2000);
        dl = 1;
    } else {
        ax = (uint16_t)(0x2000 - di);
        dl = 0xff;
    }

    ax = (uint16_t)((uint8_t)(ax / 0xab));

    if (dl == 1)
        cx = (uint16_t)(cx + ax);
    else
        cx = (uint16_t)(cx - ax);

    if (cx > 0x1fc)
        cx = 0x1fc;

    return cx;
}

/*
 * SX.OVL ADL:0x1f45
 *
 * Write a voice's level, scaled three ways.
 *
 * The argument is a level 0..0x3f, which first goes through the curve at
 * `cs:0xdd`, and then through the *patch's* own total level - `cs:0x18a8` for
 * the carrier - as `level * patch / 0x3f`, subtracted from the patch's own
 * figure. The key-scale bits from `cs:0x1892` go in the top two bits.
 *
 * The modulator gets the same treatment, but only when `cs:0x1900` says the
 * patch is additive: in a two-operator FM patch the modulator's level is a
 * timbre control and turning it down with the volume would change the
 * instrument rather than its loudness.
 */
void adl_write_voice_level(uint16_t voice, uint16_t level)
{
    uint16_t di = level;
    uint8_t  curve = SX8((uint16_t)(di + 0xdd));
    uint16_t at = (uint16_t)(voice * 2);
    uint16_t cx, ax;

    cx = (uint16_t)SX16((uint16_t)(at + 0x18a8));
    ax = (uint16_t)((uint8_t)((curve * (uint8_t)cx) / 0x3f));
    cx = (uint16_t)((uint8_t)((uint8_t)cx - (uint8_t)ax));
    ax = (uint16_t)SX16((uint16_t)(at + 0x1892));
    cx = (uint16_t)(((uint8_t)cx) | (uint8_t)((uint8_t)ax << 6));

    adl_write((uint16_t)(0x40 + SX8((uint16_t)(SX8((uint16_t)(voice + 0x1fe))
                                               + 0x222))),
              (uint16_t)(cx & 0xff));

    if (SX8((uint16_t)(voice + 0x1900)) == 0)
        return;

    cx = (uint16_t)SX16((uint16_t)(at + 0x18ea));
    ax = (uint16_t)((uint8_t)((curve * (uint8_t)cx) / 0x3f));
    cx = (uint16_t)((uint8_t)((uint8_t)cx - (uint8_t)ax));
    ax = (uint16_t)SX16((uint16_t)(at + 0x18d4));
    cx = (uint16_t)(((uint8_t)cx) | (uint8_t)((uint8_t)ax << 6));

    adl_write((uint16_t)(0x40 + SX8((uint16_t)(SX8((uint16_t)(voice + 0x207))
                                               + 0x222))),
              (uint16_t)(cx & 0xff));
}

/*
 * SX.OVL ADL:0x1e23
 *
 * **The note itself.** `dx` non-zero keys it on, zero keys it off, and the
 * two paths are the same code because the key bit is one bit of register
 * 0xB0 and everything else about a note has to be written either way.
 *
 * A program at or above 0x80 is percussion: the note is clamped to 0x1b..0x58
 * and mapped through `cs:0x183c`, which is what turns a drum's MIDI note into
 * the pitch the patch was designed at.
 *
 * Then the note is taken to **quarter-tones** - `cx <<= 2` - bent, and split
 * by 0x30: forty-eight quarter-tones to the octave, so the quotient is the
 * block and the remainder indexes the F-number table at `cs:0x3d`. Register
 * 0xA0 takes the low byte of that word and 0xB0 the key bit, the block and
 * the two high bits.
 *
 * The loudness is three multiplications - the channel's volume at `cs:0x130`,
 * the velocity through the curve at `cs:0x9d`, and the master at `cs:0x11f` -
 * and it is zero outright when parameter 346 at `cs:0x11e` is clear, which is
 * how the driver is muted without forgetting anything.
 */
void adl_note(uint16_t voice, uint16_t cx, uint16_t dx)
{
    uint16_t si, di, ax, bx;
    uint8_t  cl = (uint8_t)cx, ch, al, block;

    SX8((uint16_t)(voice + 0x19b)) = cl;
    si = SX8((uint16_t)(voice + 0x190));

    if (SX8((uint16_t)(voice + 0x1bc)) >= 0x80) {
        if (cl < 0x1b)
            cl = 0x1b;
        else if (cl > 0x58)
            cl = 0x58;
        cl = SX8((uint16_t)((cl - 0x1b) + 0x183c));
    }

    cx = (uint16_t)(cl << 2);
    cx = adl_bend(voice, cx);
    if (cx == 0xffff)
        return;

    block = (uint8_t)(cx / 0x30);
    di = (uint16_t)((cx % 0x30) * 2);
    cx = (uint16_t)SX16((uint16_t)(di + 0x3d));
    ch = (uint8_t)(cx >> 8);

    adl_write((uint16_t)(0xa0 + voice), (uint16_t)(cx & 0xff));

    al = block;
    if (al > 0)
        al--;
    al = (uint8_t)(al << 2);
    al |= ch;
    if (dx != 0)
        al |= 0x20;

    /* The level, before the key bit: an OPL2 that is keyed on before its
     * level is written speaks at whatever the last note left there. */
    bx = voice;
    ax = (uint16_t)(SX8((uint16_t)(si + 0x130)) + 1);
    di = SX8((uint16_t)(bx + 0x1a6));
    ax = (uint16_t)(ax * (uint16_t)(SX8((uint16_t)(di + 0x9d)) + 1));
    ax >>= 6;
    ax = (uint16_t)(ax * (uint16_t)(SX8(0x11f) + 1));
    ax >>= 4;
    if ((uint8_t)ax != 0)
        ax--;
    if (SX8(0x11e) == 0)
        ax = 0;

    adl_write_voice_level(bx, (uint16_t)(ax & 0xff));

    adl_write((uint16_t)(0xb0 + voice), al);
}

/*
 * SX.OVL ADL:0x1df4
 *
 * Move a voice to the end of the rotation order at `cs:0x1c7`, so the next
 * allocation takes the one used longest ago. Nine entries, and the routine
 * finds the voice, shifts every later entry down and writes it at `cs:0x1cf`.
 */
void adl_touch_voice(uint16_t voice)
{
    uint16_t si;

    for (si = 0; si < 9; si++) {
        if (SX8((uint16_t)(si + 0x1c7)) != (uint8_t)voice)
            continue;

        for (; si < 8; si++)
            SX8((uint16_t)(si + 0x1c7)) = SX8((uint16_t)(si + 0x1c8));
        break;
    }

    SX8(0x1cf) = (uint8_t)voice;
}

/*
 * SX.OVL ADL:0x1dc4
 *
 * Key a voice off and give it back: the sustained flag cleared, the note
 * written once more with `dx` zero so the release actually starts, the note
 * marked free with 0xff, the voice moved to the end of the rotation, and the
 * channel's count of voices in use decremented.
 */
void adl_key_off(uint16_t voice)
{
    uint16_t si;

    SX8((uint16_t)(voice + 0x1b1)) = 0;
    adl_note(voice, SX8((uint16_t)(voice + 0x19b)), 0);
    SX8((uint16_t)(voice + 0x19b)) = 0xff;
    adl_touch_voice(voice);

    si = SX8((uint16_t)(voice + 0x190));
    SX8((uint16_t)(si + 0x1ed))--;
}

/*
 * SX.OVL ADL:0x1fe1
 *
 * Unpack one 28-byte patch into the tables the level and register writers
 * read, and send its two operators to the chip.
 *
 * The byte at +0xc chooses the arrangement. Zero is the ordinary two-operator
 * FM patch, where only the carrier's level follows the volume; non-zero is
 * additive, where both do - which is what `cs:0x1900` records.
 *
 * `cs:0x190b` is the level divided by fifteen, kept for a scaling this game's
 * music does not reach.
 */
void adl_load_patch(uint16_t voice, uint16_t at)
{
    uint16_t bx = voice, di = at, ax;

    SX8((uint16_t)(bx + 0x1900)) = 1;

    if (SX8((uint16_t)(di + 0x0c)) != 0) {
        SX8((uint16_t)(bx + 0x1900)) = 0;
        bx = (uint16_t)(bx * 2);
    } else {
        bx = (uint16_t)(bx * 2);
        ax = SX8(di);
        SX16((uint16_t)(bx + 0x18d4)) = (int16_t)ax;
        ax = (uint16_t)(0x3f - SX8((uint16_t)(di + 8)));
        SX16((uint16_t)(bx + 0x18ea)) = (int16_t)ax;
        SX16((uint16_t)(bx + 0x190b)) = (int16_t)(ax / 0x0f);
    }

    ax = SX8((uint16_t)(di + 0x0d));
    SX16((uint16_t)(bx + 0x1892)) = (int16_t)ax;
    ax = (uint16_t)(0x3f - SX8((uint16_t)(di + 0x15)));
    SX16((uint16_t)(bx + 0x18a8)) = (int16_t)ax;
    SX16((uint16_t)(bx + 0x18be)) = (int16_t)(ax / 0x0f);

    /*
     * And the two operators. The patch's own thirteen bytes for each go to
     * `adl_write_operator`, which stores them and emits the six registers;
     * the fourteenth byte is the connection, and it comes from +0x1a of the
     * patch rather than from the operator's own run.
     */
    {
        uint16_t src = (uint16_t)(at + 0x1a);
        uint8_t  conn = SX8(src);
        uint8_t  second = SX8((uint16_t)(src + 1));
        uint16_t idx = (uint16_t)(voice * 2);

        adl_write_operator(SX8((uint16_t)(idx + 0x246)), at, conn);
        adl_write_operator(SX8((uint16_t)(idx + 1 + 0x246)),
                           (uint16_t)(at + 0x0d), second);
    }
}

/*
 * SX.OVL ADL:0x2109
 *
 * Store one operator's thirteen patch bytes at `cs:0x274 + slot * 14`, put
 * the connection in the fourteenth, and then write every register that
 * depends on them.
 */
void adl_write_operator(uint16_t slot, uint16_t src, uint8_t connect)
{
    uint16_t at = ADL_OP(slot);
    uint16_t i;

    for (i = 0; i < 0x0d; i++)
        SX8((uint16_t)(at + i)) = SX8((uint16_t)(src + i));

    SX8((uint16_t)(at + 0x0d)) = (uint8_t)(connect & 3);

    adl_write_bd();
    adl_write_nts();
    adl_write_level(slot);
    adl_write_feedback(slot);
    adl_write_attack_decay(slot);
    adl_write_sustain_release(slot);
    adl_write_mult(slot);
    adl_write_wave(slot);
}

/*
 * SX.OVL ADL:0x237d
 *
 * Silence the chip: every register from 0 to 0xf5 written zero, then register
 * 1 set to 0x20 - wave select enable, which an OPL2 needs before the four
 * waveforms are anything but sine - and the eighteen operators put back to
 * their defaults.
 */
void adl_reset(void)
{
    uint16_t bx;

    for (bx = 0; bx < 0xf6; bx++)
        adl_write(bx, 0);

    SX16(0x188d) = 0x20;
    adl_write(1, (uint16_t)SX16(0x188d));
    adl_default_operators();
}

/*
 * SX.OVL ADL:0x20b5
 *
 * The eighteen operators' defaults, from one of two thirteen-byte blocks:
 * `cs:0x266` for a slot the table at `cs:0x210` marks, `cs:0x258` otherwise.
 */
void adl_default_operators(void)
{
    uint16_t di;

    for (di = 0; di < 0x12; di++)
        adl_default_operator(di, (uint16_t)(SX8((uint16_t)(di + 0x210)) != 0
                                            ? 0x266 : 0x258));
}

/*
 * SX.OVL ADL:0x20e2
 *
 * One operator's default: thirteen bytes copied to the scratch at `cs:0x187a`
 * and written from there, so the patch store is not disturbed.
 */
void adl_default_operator(uint16_t slot, uint16_t src)
{
    uint16_t di;

    for (di = 0; di < 0x0d; di++)
        SX8((uint16_t)(di + 0x187a)) = SX8((uint16_t)(src + di));

    adl_write_operator(slot, 0x187a, 0);
}

/*
 * SX.OVL ADL:0x1ad4
 *
 * Find a voice for a note on channel `al`, and answer 0xffff if there is
 * none.
 *
 * A free voice first, walking the rotation order so the one used longest ago
 * is taken. Failing that it **steals**: every channel is compared against its
 * allowance - `cs:0x1ed` voices in use against `cs:0x1dd` allowed - and the
 * one furthest over loses a voice. A channel within its allowance is never
 * robbed, which is what the allowance is for.
 */
uint16_t adl_alloc_voice(uint16_t ax)
{
    uint16_t si, bx, dx = 0;
    uint8_t  cl, worst = 0;

    for (si = 0; si < 9; si++) {
        bx = SX8((uint16_t)(si + 0x1c7));
        if (SX8((uint16_t)(bx + 0x19b)) == 0xff) {
            SX8((uint16_t)(bx + 0x190)) = (uint8_t)ax;
            return bx;
        }
    }

    for (si = 0; si < 0x10; si++) {
        cl = SX8((uint16_t)(si + 0x1ed));
        if (cl <= SX8((uint16_t)(si + 0x1dd)))
            continue;
        cl = (uint8_t)(cl - SX8((uint16_t)(si + 0x1dd)));
        if (worst >= cl)
            continue;
        worst = cl;
        dx = si;
    }

    cl = (uint8_t)ax;
    if (worst > 0)
        cl = (uint8_t)dx;

    for (si = 0; si < 9; si++) {
        bx = SX8((uint16_t)(si + 0x1c7));
        if (SX8((uint16_t)(bx + 0x190)) != cl)
            continue;
        adl_key_off(bx);
        SX8((uint16_t)(bx + 0x190)) = (uint8_t)ax;
        return bx;
    }

    return 0xffff;
}

/*
 * SX.OVL ADL:0x1d45
 *
 * Key a voice on. The channel's program is loaded into the voice if it is
 * holding another - and only when parameter 346 at `cs:0x11e` allows it, so a
 * muted driver does not spend the chip's time on patches nobody will hear.
 *
 * Channel 9 is percussion and its note is floored at 0x1b, which is where the
 * map at `cs:0x183c` begins.
 */
void adl_key_on(uint16_t voice, uint16_t cx)
{
    uint16_t si = SX8((uint16_t)(voice + 0x190));
    uint8_t  dl = SX8((uint16_t)(si + 0x120));
    uint8_t  ch = (uint8_t)(cx >> 8);

    SX8((uint16_t)(si + 0x1ed))++;
    adl_touch_voice(voice);

    /*
     * **Channel 9's program comes from the note**, not from the channel: the
     * note is clamped to 0x1b..0x58 and 0x65 added, which lands it in
     * 0x80..0xbd - and that is exactly the range `adl_note` treats as
     * percussion. One drum per note, each with its own patch, which is what
     * a General Midi drum channel means and what the map at `cs:0x183c` is
     * the other half of.
     */
    if (si == 9) {
        dl = ch;
        if (dl < 0x1b)
            dl = 0x1b;
        else if (dl > 0x58)
            dl = 0x58;
        dl = (uint8_t)(dl + 0x65);
    }

    if (dl != SX8((uint16_t)(voice + 0x1bc)) && SX8(0x11e) != 0) {
        SX8((uint16_t)(voice + 0x1bc)) = dl;
        adl_load_patch(voice, (uint16_t)(0x374 + dl * 28));
    }

    SX8((uint16_t)(voice + 0x1a6)) = (uint8_t)cx;
    adl_note(voice, ch, 1);
}

/* SX.OVL ADL:0x1951 - what BP 3, 6, 9 and 14 to 16 all point at. */
void adl_nop(void)
{
}

/*
 * SX.OVL ADL:0x1952  - function 4
 *
 * Stop a note: every voice playing it for this channel is keyed off, unless
 * the channel is holding the sustain pedal, in which case it is only marked
 * at `cs:0x1b1` and released when the pedal is.
 */
void adl_stop_note(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax, ch = (uint8_t)(cx >> 8);
    uint16_t bx, si;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x190)) != al)
            continue;
        if (SX8((uint16_t)(bx + 0x19b)) != ch)
            continue;

        si = (uint16_t)(ax & 0xff);
        if (SX8((uint16_t)(si + 0x150)) != 0)
            SX8((uint16_t)(bx + 0x1b1)) = 1;
        else
            adl_key_off(bx);
    }
}

/*
 * SX.OVL ADL:0x1988  - function 5
 *
 * Start a note. A velocity of zero is a stop, which is the convention this
 * whole game uses; notes outside 12..107 are dropped; and the velocity is
 * halved to the six bits the chip's level is.
 *
 * A voice already playing this note on this channel is keyed off and on
 * again rather than left alone, so a repeated note re-attacks.
 */
void adl_start_note(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax, cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    uint16_t bx;

    if (cl == 0) {
        adl_stop_note(ax, cx);
        return;
    }

    if (ch < 0x0c || ch > 0x6b)
        return;

    cl = (uint8_t)(cl >> 1);
    cx = (uint16_t)((ch << 8) | cl);

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x190)) != al)
            continue;
        if (SX8((uint16_t)(bx + 0x19b)) != ch)
            continue;
        adl_key_off(bx);
        adl_key_on(bx, cx);
        return;
    }

    bx = adl_alloc_voice(ax);
    if (bx != 0xffff)
        adl_key_on(bx, cx);
}

/*
 * SX.OVL ADL:0x1ca9
 *
 * Volume: halved to the chip's six bits, stored, and then re-applied to every
 * voice the channel is sounding by writing the note again - which is how a
 * volume change reaches a note already playing without restarting it.
 */
void adl_ctl_volume(uint16_t ax, uint16_t cx)
{
    uint16_t si = (uint16_t)(ax & 0xff), bx;
    uint8_t  al = (uint8_t)ax;

    SX8((uint16_t)(si + 0x130)) = (uint8_t)((uint8_t)cx >> 1);

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x190)) != al)
            continue;
        if (SX8((uint16_t)(bx + 0x19b)) == 0xff)
            continue;
        adl_note(bx, SX8((uint16_t)(bx + 0x19b)), 1);
    }
}

/*
 * SX.OVL ADL:0x1ce2
 *
 * Pan, stored and re-applied the same way. An OPL2 is mono, so nothing the
 * chip is told changes - the driver keeps it because function 17 can be
 * asked for it back.
 */
void adl_ctl_pan(uint16_t ax, uint16_t cx)
{
    uint16_t si = (uint16_t)(ax & 0xff), bx;
    uint8_t  al = (uint8_t)ax;

    SX8((uint16_t)(si + 0x140)) = (uint8_t)cx;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x190)) != al)
            continue;
        if (SX8((uint16_t)(bx + 0x19b)) == 0xff)
            continue;
        adl_note(bx, SX8((uint16_t)(bx + 0x19b)), 1);
    }
}

/*
 * SX.OVL ADL:0x1d15
 *
 * The sustain pedal. Pressing it only records the fact; releasing it keys off
 * every voice that `adl_stop_note` left held at `cs:0x1b1`.
 */
void adl_ctl_sustain(uint16_t ax, uint16_t cx)
{
    uint16_t si = (uint16_t)(ax & 0xff), bx;
    uint8_t  al = (uint8_t)ax;

    SX8((uint16_t)(si + 0x150)) = (uint8_t)cx;
    if ((uint8_t)cx != 0)
        return;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x190)) != al)
            continue;
        if (SX8((uint16_t)(bx + 0x1b1)) == 0)
            continue;
        adl_key_off(bx);
    }
}

/*
 * SX.OVL ADL:0x1bec
 *
 * Give a channel `cl` more voices, out of the ones nothing has reserved -
 * `cs:0x1d2` holds 0xff for a voice that is free to be claimed. A voice that
 * is sounding is keyed off first, because it is about to belong to somebody
 * else. What cannot be granted now is remembered at `cs:0x160` and handed
 * over by `adl_rebalance` when a voice comes back.
 */
void adl_grant_voices(uint16_t ax, uint8_t cl)
{
    uint8_t al = (uint8_t)ax;
    uint16_t bx, si;

    for (bx = 0; bx < 9 && cl > 0; bx++) {
        if (SX8((uint16_t)(bx + 0x1d2)) != 0xff)
            continue;
        if (SX8((uint16_t)(bx + 0x19b)) != 0xff)
            adl_key_off(bx);

        SX8((uint16_t)(bx + 0x1d2)) = al;
        si = al;
        SX8((uint16_t)(si + 0x1dd))++;
        cl--;
    }

    si = (uint16_t)(ax & 0xff);
    SX8((uint16_t)(si + 0x160)) = (uint8_t)(SX8((uint16_t)(si + 0x160)) + cl);
}

/*
 * SX.OVL ADL:0x1c30
 *
 * Take `cl` voices back from a channel. Anything still outstanding at
 * `cs:0x160` is cancelled first, because a request never granted costs
 * nothing to withdraw; then the channel's **idle** voices are released, and
 * only then its sounding ones, which have to be keyed off on the way out.
 */
void adl_release_voices(uint16_t ax, uint8_t cl)
{
    uint8_t al = (uint8_t)ax;
    uint16_t si = (uint16_t)(ax & 0xff), bx;

    if (SX8((uint16_t)(si + 0x160)) >= cl) {
        SX8((uint16_t)(si + 0x160)) =
            (uint8_t)(SX8((uint16_t)(si + 0x160)) - cl);
        return;
    }

    cl = (uint8_t)(cl - SX8((uint16_t)(si + 0x160)));
    SX8((uint16_t)(si + 0x160)) = 0;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x1d2)) != al)
            continue;
        if (SX8((uint16_t)(bx + 0x19b)) != 0xff)
            continue;
        SX8((uint16_t)(bx + 0x1d2)) = 0xff;
        SX8((uint16_t)(al + 0x1dd))--;
        if (--cl == 0)
            return;
    }

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x1d2)) != al)
            continue;
        adl_key_off(bx);
        SX8((uint16_t)(bx + 0x1d2)) = 0xff;
        SX8((uint16_t)(al + 0x1dd))--;
        if (--cl == 0)
            return;
    }
}

/*
 * SX.OVL ADL:0x1b92
 *
 * A voice has come free, so hand it to whoever asked and did not get one.
 * Channels are served in order, and a channel whose whole request can be met
 * takes it all and ends the walk.
 */
void adl_rebalance(void)
{
    uint8_t cl = 0, ch;
    uint16_t si;

    for (si = 0; si < 9; si++)
        if (SX8((uint16_t)(si + 0x1d2)) == 0xff)
            cl++;

    if (cl == 0)
        return;

    for (si = 0; si < 0x10; si++) {
        ch = SX8((uint16_t)(si + 0x160));
        if (ch == 0)
            continue;

        if (ch < cl) {
            cl = (uint8_t)(cl - ch);
            SX8((uint16_t)(si + 0x160)) = 0;
            adl_grant_voices(si, ch);
            continue;
        }

        SX8((uint16_t)(si + 0x160)) = (uint8_t)(ch - cl);
        adl_grant_voices(si, cl);
        return;
    }
}

/*
 * SX.OVL ADL:0x1b52
 *
 * Controller 0x4b: how many of the nine voices this channel wants.
 *
 * The count it has now is the voices reserved to it at `cs:0x1d2` plus
 * whatever it is still owed at `cs:0x160`. Wanting more grants them; wanting
 * fewer releases them and then offers what came free to everybody else.
 *
 * This is the controller the stub here first caught: the game **does** send
 * it, which a quiet no-op would have turned into notes going missing under
 * load with nothing to say why.
 */
void adl_ctl_reserve(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax, cl = (uint8_t)cx, dl = 0;
    uint16_t si;

    for (si = 0; si < 9; si++)
        if (SX8((uint16_t)(si + 0x1d2)) == al)
            dl++;

    si = (uint16_t)(ax & 0xff);
    dl = (uint8_t)(dl + SX8((uint16_t)(si + 0x160)));

    if (dl == cl)
        return;

    if (dl < cl) {
        adl_grant_voices(ax, (uint8_t)(cl - dl));
        return;
    }

    adl_release_voices(ax, (uint8_t)(dl - cl));
    adl_rebalance();
}

/*
 * SX.OVL ADL:0x19cf  - function 7
 */
void adl_controller(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax, ch = (uint8_t)(cx >> 8);
    uint16_t bx;

    if (ch == 7) {
        adl_ctl_volume(ax, cx);
        return;
    }
    if (ch == 0x0a) {
        adl_ctl_pan(ax, cx);
        return;
    }
    if (ch == 0x40) {
        adl_ctl_sustain(ax, cx);
        return;
    }
    if (ch == 0x4b) {
        adl_ctl_reserve(ax, cx);
        return;
    }
    if (ch != 0x7b)
        return;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x190)) != al)
            continue;
        if (SX8((uint16_t)(bx + 0x19b)) == 0xff)
            continue;
        adl_key_off(bx);
    }
}

/* SX.OVL ADL:0x1a1b  - function 8. The program, kept per channel. */
void adl_program(uint16_t ax, uint16_t cx)
{
    SX8((uint16_t)((ax & 0xff) + 0x120)) = (uint8_t)cx;
}

/*
 * SX.OVL ADL:0x1a29  - function 10
 *
 * Pitch bend: stored as a word per channel, then every sounding voice on that
 * channel has its note written again so the new bend takes effect at once.
 */
void adl_pitch_bend(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax;
    uint8_t cl = (uint8_t)(cx >> 8), ch = (uint8_t)cx;
    uint16_t bx;

    /* `xchg cl,ch` then `shr ch,1` with the bit going into cl's top. */
    if (ch & 1)
        cl |= 0x80;
    ch = (uint8_t)(ch >> 1);

    SX16((uint16_t)((ax & 0xff) * 2 + 0x170)) = (int16_t)((ch << 8) | cl);

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x190)) != al)
            continue;
        if (SX8((uint16_t)(bx + 0x19b)) == 0xff)
            continue;
        adl_note(bx, SX8((uint16_t)(bx + 0x19b)), 1);
    }
}

/*
 * SX.OVL ADL:0x1a8d  - function 12, the master level.
 *
 * 0xff reads without writing, as every parameter in this family does. Setting
 * it writes every sounding voice's note again, which is how the master
 * reaches notes that are already playing.
 */
uint16_t adl_param_345(uint16_t cl)
{
    uint16_t ax = SX8(0x11f);
    uint16_t bx;

    if ((uint8_t)cl == 0xff)
        return ax;

    SX8(0x11f) = (uint8_t)cl;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x19b)) == 0xff)
            continue;
        adl_note(bx, SX8((uint16_t)(bx + 0x19b)), 1);
    }

    return ax;
}

/* SX.OVL ADL:0x1a68  - function 13. */
uint16_t adl_param_346(uint16_t cl)
{
    uint16_t ax = SX8(0x11e);

    if ((uint8_t)cl == 0xff)
        return ax;

    SX8(0x11e) = (uint8_t)cl;
    if ((uint8_t)cl != 0)
        cl = SX8(0x11f);

    SX8(0x11f) = (uint8_t)adl_param_345(cl);
    return ax;
}

/* SX.OVL ADL:0x1abf  - function 11. */
uint16_t adl_param_349(uint16_t cl)
{
    uint16_t ax = SX8(0x11d);

    if ((uint8_t)cl != 0xff)
        SX8(0x11d) = (uint8_t)cl;

    return ax;
}

/* SX.OVL ADL:0x1ad0  - function 2. */
void adl_stop_all(void)
{
    adl_reset();
}

/*
 * SX.OVL ADL:0x23a7  - function 17
 *
 * Read back what the driver holds. **It zero-extends**, where `GMD:`'s
 * function 17 keeps AH and answers `(kind << 8) | value`. The same function
 * number and the opposite convention, which is worth knowing before assuming
 * one from the other.
 */
uint16_t adl_query(uint16_t ax, uint16_t cx)
{
    uint16_t si = (uint16_t)(ax & 0xff);
    uint8_t  ah = (uint8_t)(ax >> 8), ch = (uint8_t)(cx >> 8);

    if (ah == 0xe0)
        return (uint16_t)SX16((uint16_t)(si * 2 + 0x170));
    if (ah == 0xc0)
        return SX8((uint16_t)(si + 0x120));

    if (ah == 0xb0) {
        if (ch == 7)
            return SX8((uint16_t)(si + 0x130));
        if (ch == 0x0a)
            return SX8((uint16_t)(si + 0x140));
        if (ch == 0x40)
            return SX8((uint16_t)(si + 0x150));
        if (ch == 0x4b)
            return (uint8_t)cx == 0xff ? SX8((uint16_t)(si + 0x1dd))
                                       : SX8((uint16_t)(si + 0x160));
    }

    return 0xffff;
}

/*
 * SX.OVL ADL:0x2414  - function 1
 *
 * Initialise from the patch bank at `ES:AX`, whose length the driver already
 * holds at `cs:0x372`. The same `ES:AX` `GMD:` reads, and the argument
 * `configure_driver` was for years said not to have.
 *
 * Then the chip is reset and the master level set to 15. The answer is
 * AX 0x2414 and CX 0x0800, of which `configure_driver` keeps CL and CH.
 */
void adl_init(uint16_t off, uint16_t seg, uint16_t *ax, uint16_t *cx)
{
    const uint8_t *src = (const uint8_t *)FAR_PTR(seg, off);
    uint16_t n = (uint16_t)SX16(0x372);
    uint16_t di;

    for (di = 0; di < n; di++)
        SX8((uint16_t)(di + 0x374)) = src[di];

    adl_reset();
    adl_param_345(0x0f);

    *ax = 0x2414;
    *cx = 0x0800;
}

/*
 * SX.OVL ADL:0x2446  - function 0
 *
 * AX 0x0103 and CX 0x0009: nine voices, which is what an OPL2 has in melodic
 * mode. `install_driver` keeps CL and CH and the top nibble of AH.
 */
void adl_describe_0(uint16_t *ax, uint16_t *cx)
{
    *ax = 0x0103;
    *cx = 0x0009;
}
