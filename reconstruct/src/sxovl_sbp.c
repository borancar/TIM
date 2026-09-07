/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **`SBP:`, the Sound Blaster Pro driver**, one of the eleven chunks in
 * `SX.OVL`. `RESOURCE.CFG`'s device byte selects it as **device 4** - the
 * table at DGROUP 0x4a1c maps 0 to `STD:`, 2 to `ADL:` and 4 to here - and
 * INSTALL.COM writes that byte when the player picks Sound Blaster.
 *
 * Its banner names it `dude` with the description **"Sound Blaster Pro"**,
 * version 2.24, which is what `driver_kind` has to match on: `SBP:` and `GMD:`
 * are both named `dude`, so the name alone cannot tell them apart.
 *
 * **The hardware is a Sound Blaster Pro 1.0 - two OPL2 chips, not an OPL3.**
 * The driver keeps its ports in its own words rather than as constants,
 * because the base is configurable, and at run time they read:
 *
 *     cs:0x2c, cs:0x2e   0x388     the OPL index, AdLib-compatible
 *     cs:0x30            0x389     the OPL data
 *     cs:0x32, cs:0x34   0x220     the card's base - the LEFT OPL2
 *     cs:0x36            0x221
 *     cs:0x38, cs:0x3a   0x222     the RIGHT OPL2
 *     cs:0x3c            0x223
 *     cs:0x3e, cs:0x40   0x224     the mixer index and data
 *
 * A separate FM address at 0x222 is what makes this dual OPL2 rather than
 * OPL3: an SB Pro 2.0 or a 16 has one OPL3 at 0x220 and nothing at 0x222.
 *
 * Reached through the same dispatcher shape as the other drivers - the entry
 * at offset 0 jumps to 0x194a, which takes the function number in BP and calls
 * through the eighteen near offsets at `cs:0x1926`.
 *
 * The driver is read out of memory rather than the file, the way the others
 * are: `tools/dump_overlay.py --seg <seg>` after `SNDCS:0x1e7` says where the
 * loader put it.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * SX.OVL SBP:0x2185
 *
 * One OPL register: the index to `cs:0x2c`, five reads of that same port, the
 * value to `cs:0x30`, and then thirty-three reads of `cs:0x2e`.
 *
 * The reads are the YM3812's settling time and are not decoration - see
 * `src/opl.h`, where writing a driver tick's registers at one instant instead
 * of spread out came out hollow and half as loud. `ADL:0x208e` is the same
 * routine against constant ports; this one goes through the driver's own
 * words because a Sound Blaster Pro's base is configurable.
 *
 * The index arrives in BX and the value in CX, and AX, DX and CX are saved.
 */
void sbp_write(uint16_t reg, uint16_t val)
{
    uint16_t i;

    io_out8((uint16_t)SXSBP.word_002c, (uint8_t)reg);
    for (i = 0; i < 5; i++)
        (void)io_in8((uint16_t)SXSBP.word_002c);

    io_out8((uint16_t)SXSBP.word_0030, (uint8_t)val);
    for (i = 0; i < 0x21; i++)
        (void)io_in8((uint16_t)SXSBP.word_002e);
}

/*
 * SX.OVL SBP:0x21ac
 *
 * The same again for the **mixer**, whose index and data are `cs:0x3e` and
 * `cs:0x40` - 0x224 and 0x225 on a card at the usual base. The settling reads
 * are still counted against `cs:0x2e`, the OPL index, exactly as written.
 *
 * The mixer is the part of a Sound Blaster Pro an AdLib does not have, and it
 * is how this driver places a voice left or right.
 */
void sbp_mixer_write(uint16_t reg, uint16_t val)
{
    uint16_t i;

    io_out8((uint16_t)SXSBP.word_003e, (uint8_t)reg);
    for (i = 0; i < 5; i++)
        (void)io_in8((uint16_t)SXSBP.word_003e);

    io_out8((uint16_t)SXSBP.word_0040, (uint8_t)val);
    for (i = 0; i < 0x21; i++)
        (void)io_in8((uint16_t)SXSBP.word_002e);
}

/*
 * SX.OVL SBP:0x1a20
 *
 * **Function 8**, the program change: AL is the channel and CL the patch, and
 * the driver only files it. The per-channel patch numbers live at `cs:0x125`.
 *
 * The index is `AX & 0xff`, so the whole of AL and not a masked-to-16 channel.
 */
void sbp_program_change(uint16_t ax, uint16_t cx)
{
    SX8((uint16_t)((ax & 0xff) + 0x125)) = (uint8_t)cx;
}

/*
 * SX.OVL SBP:0x1abc
 *
 * **Function 11.** A byte at `cs:0x122` that this driver stores and nothing
 * here reads: 0xff asks for it, anything else sets it, and either way the
 * previous value comes back in AL. `ADL:` has the same get-or-set shape at the
 * same function number, which is why the port calls it `param_349` after the
 * *caller's* variable rather than after any meaning found in the driver.
 */
uint16_t sbp_param_349(uint16_t cl)
{
    uint8_t prev = SXSBP.byte_0122;

    if ((cl & 0xff) == 0xff)
        return prev;

    SXSBP.byte_0122 = (uint8_t)cl;
    return prev;
}

/*
 * SX.OVL SBP:0x1a92
 *
 * **Function 12, the master level** - and on a Sound Blaster Pro that is not an
 * OPL register at all but the **mixer's FM volume, register 0x26**, which is
 * the one place this driver's answer differs in kind from `ADL:`'s. An AdLib
 * has no mixer, so `ADL:` scales every voice's level instead.
 *
 * The byte written carries the level in **both nibbles**, left and right: the
 * value is shifted up four one bit at a time and then or-ed with itself back
 * out of `cs:0x124`, which the line above has just set. A level wider than a
 * nibble therefore loses its top bits in the shift and keeps them in the or -
 * transcribed as written rather than masked, because that is the byte the
 * hardware sees.
 *
 * 0xff asks for the current level without setting it. The previous value is
 * returned either way, and CX survives.
 */
uint16_t sbp_param_345(uint16_t cl)
{
    uint8_t prev = SXSBP.byte_0124;
    uint8_t both;

    if ((cl & 0xff) == 0xff)
        return prev;

    SXSBP.byte_0124 = (uint8_t)cl;

    both = (uint8_t)((uint8_t)cl << 4);
    both = (uint8_t)(both | SXSBP.byte_0124);
    sbp_mixer_write(0x26, both);

    return prev;
}

/*
 * SX.OVL SBP:0x1a6d
 *
 * **Function 13**, the FM on/off switch, kept at `cs:0x123`. 0xff asks for it.
 *
 * Switching **off** does not forget the level: it calls function 12 with zero,
 * which writes silence to the mixer *and* returns what the level had been, and
 * that returned value goes straight back into `cs:0x124`. Switching **on**
 * calls function 12 with the stored level to put it back. So the mixer follows
 * the switch while `cs:0x124` always holds the real setting.
 *
 * AX and CX are both restored around the call, so what comes back is the
 * previous state of the switch and not function 12's answer.
 */
uint16_t sbp_set_enable(uint16_t cl)
{
    uint8_t prev = SXSBP.byte_0123;
    uint16_t arg;

    if ((cl & 0xff) == 0xff)
        return prev;

    SXSBP.byte_0123 = (uint8_t)cl;

    arg = cl & 0xff;
    if (arg != 0)
        arg = SXSBP.byte_0124;

    SXSBP.byte_0124 = (uint8_t)sbp_param_345(arg);

    return prev;
}

/*
 * SX.OVL SBP:0x21d3
 *
 * One register on the **left** OPL bank - index `cs:0x32`, data `cs:0x36`, the
 * thirty-three settling reads against `cs:0x34` - which on a card at the usual
 * base is 0x220/0x221. That is the same chip `sbp_write` reaches through
 * 0x388: a Sound Blaster decodes its OPL at both addresses.
 *
 * **The panning is dead code as the driver ships.** When `cs:0x202` is set and
 * the register is one of 0xc0..0xc8 - the feedback and connection registers -
 * bit 0x20 is or-ed into the value, which is the OPL3's "enable left". But
 * `cs:0x202` is **0** in the image and no instruction in the driver's 16 KB
 * writes it: searched for all three direct encodings and found none, and the
 * driver overrides `cs:` on every other access, so a computed write would be
 * out of character. It is a build-time constant saying "this is not an OPL3",
 * and the two banks therefore receive the same values with no stereo bits.
 *
 * Transcribed whole regardless. The branch is what the original executes -
 * it tests the flag on every level write - and a transcription that dropped it
 * because the flag is zero would be a judgement, not a reading.
 */
void sbp_write_left(uint16_t reg, uint16_t val)
{
    uint16_t i;

    if (SXSBP.byte_0202 != 0 && (reg & 0xff) >= 0xc0 && (reg & 0xff) <= 0xc8)
        val |= 0x20;

    io_out8((uint16_t)SXSBP.word_0032, (uint8_t)reg);
    for (i = 0; i < 5; i++)
        (void)io_in8((uint16_t)SXSBP.word_0032);

    io_out8((uint16_t)SXSBP.word_0036, (uint8_t)val);
    for (i = 0; i < 0x21; i++)
        (void)io_in8((uint16_t)SXSBP.word_0034);
}

/*
 * SX.OVL SBP:0x220f
 *
 * The **right** bank - `cs:0x38`, `cs:0x3c`, `cs:0x3a`, so 0x222/0x223 - and
 * the same shape as the left with **0x10** for "enable right" instead of 0x20.
 *
 * On a Sound Blaster Pro 1.0 this is a second, physically separate YM3812. On
 * an OPL3 at 0x220 the very same ports are that chip's second register bank.
 * The driver's code cannot tell them apart and does not try; `cs:0x202` is the
 * only thing that would have, and it is zero.
 */
void sbp_write_right(uint16_t reg, uint16_t val)
{
    uint16_t i;

    if (SXSBP.byte_0202 != 0 && (reg & 0xff) >= 0xc0 && (reg & 0xff) <= 0xc8)
        val |= 0x10;

    io_out8((uint16_t)SXSBP.word_0038, (uint8_t)reg);
    for (i = 0; i < 5; i++)
        (void)io_in8((uint16_t)SXSBP.word_0038);

    io_out8((uint16_t)SXSBP.word_003c, (uint8_t)val);
    for (i = 0; i < 0x21; i++)
        (void)io_in8((uint16_t)SXSBP.word_003a);
}

/*
 * SX.OVL SBP:0x1dfe
 *
 * Move voice BL to the **back of the priority list** at `cs:0x1cc`, which is
 * nine bytes holding the voice numbers in the order the allocator will consider
 * them. The voice is found, everything after it shifts down one, and the last
 * slot - `cs:0x1d4`, which is `0x1cc + 8` - is set to the voice just freed.
 *
 * So the list is least-recently-freed first, and `sbp_alloc_voice` walking it
 * from the front takes the voice that has been silent longest.
 *
 * The shift loop leaves SI at 8 and falls into the outer loop's increment,
 * which takes it to 9 and ends the search - so a voice is moved once even
 * though there is no explicit break.
 */
void sbp_touch_voice(uint16_t voice)
{
    uint16_t si;

    for (si = 0; si < 9; si++) {
        if (SX8((uint16_t)(si + 0x1cc)) == (uint8_t)voice) {
            for (; si < 8; si++)
                SX8((uint16_t)(si + 0x1cc)) = SX8((uint16_t)(si + 0x1cd));
            break;
        }
    }

    SXSBP.byte_01d4 = (uint8_t)voice;
}

/*
 * SX.OVL SBP:0x1ed7
 *
 * Apply the channel's pitch bend to a note position. CX arrives in **quarter
 * semitones** and comes back the same way, BX is the voice.
 *
 * The bend word at `cs:0x175` is centred on 0x2000. The distance from centre
 * divides by **0xab** - 171, so a full 0x2000 of bend is 48 quarter-semitones,
 * two semitones either way - and moves CX up or down.
 *
 * The clamp at the end is `jbe`, **unsigned**, so a bend that takes CX below
 * zero does not clamp to zero: it wraps to something near 0xffff, fails the
 * comparison and comes out as 0x1fc, the top of the range. Transcribed as
 * written; the note would have to be within two semitones of the bottom of the
 * scale for it to happen.
 *
 * This is also why `sbp_note`'s test for -1 never fires - every path through
 * here returns at most 0x1fc.
 */
uint16_t sbp_apply_bend(uint16_t voice, uint16_t cx)
{
    uint16_t ch = SX8((uint16_t)(voice + 0x195));
    uint16_t di = (uint16_t)SX16((uint16_t)(ch * 2 + 0x175));
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

    ax = (uint16_t)(uint8_t)(ax / 0xab);

    if (dl == 1)
        cx = (uint16_t)(cx + ax);
    else
        cx = (uint16_t)(cx - ax);

    if (cx > 0x1fc)
        cx = 0x1fc;

    return cx;
}

/*
 * SX.OVL SBP:0x1f2e
 *
 * Write a voice's level to both OPL banks. BX is the voice, AL the level.
 *
 * **This is where the card's stereo actually happens.** With `cs:0x202` clear
 * there are no OPL3 panning bits to write, so the driver pans by sending the
 * two chips *different levels*: the left bank is attenuated by the channel's
 * pan byte at `cs:0x145` and the right by its complement, `0x7f - pan`. A
 * centred channel lands at 0x40 on both; a hard-left one silences the right.
 * That is why the port needs two OPL2s mixed to two speakers and not one chip
 * played twice - the values genuinely differ.
 *
 * Each bank writes the **carrier** always and the **modulator** only when
 * `cs:0x1905` marks the voice as additive, since on a two-operator FM voice in
 * frequency-modulation mode the modulator's level is timbre, not loudness.
 * That is four operator writes, and the original inlines the same fifteen
 * instructions for each rather than calling a helper. **Transcribed the same
 * way, inlined four times**, so this reads line for line against the listing:
 * the four differ in which scale, key-scale, operator and bank they use, and a
 * shared helper here would be the place a difference could hide.
 *
 * Two dead branches, transcribed as no-ops with the reason rather than as
 * code, because writing them in C would need casts that imply the opposite of
 * what the original does:
 *
 *   - after `sub al, dl` both halves test **`cmp dl, 0x3f`** and clamp *DL*,
 *     which is never read again. The clamp was meant for AL.
 *   - the right half then guards with `cmp al, 0` / `jae`, and `jae` is
 *     unsigned, so it is taken for every possible AL. The `xor al, al` behind
 *     it cannot execute.
 *
 * So neither half clamps the subtraction, and a pan attenuation larger than
 * the level wraps AL past 0xff into a curve-table read beyond `cs:0xe2`'s
 * range. Left as the original leaves it.
 *
 * The names of the four per-voice tables - scale at `cs:0x18ad`/`cs:0x18ef`
 * and key-scale at `cs:0x1897`/`cs:0x18d9` - are **guesses** from how the
 * values are used: one multiplies the level and one lands in the top two bits
 * of OPL register 0x40, which is where key-scale level lives. They are read as
 * words, as written, though only the low byte of each is ever used.
 */
void sbp_write_level(uint16_t voice, uint16_t level)
{
    uint16_t si;
    uint16_t bx;
    uint8_t  al, dl, cl, ksl;

    al = (uint8_t)level;
    if (al > 0x3f)
        al = 0x3f;

    si = SX8((uint16_t)(voice + 0x195));        /* the channel */

    /* ---- the left bank: attenuated by the pan ---- */
    dl = (uint8_t)(SX8((uint16_t)(si + 0x145)) >> 1);
    if (dl > 0x3f)
        dl = 0x3f;
    dl = (uint8_t)(0x3f - SX8((uint16_t)(dl + 0xe2)));
    dl = (uint8_t)(((uint16_t)al * dl) / 0x3f);
    al = (uint8_t)(al - dl);
    al = SX8((uint16_t)(al + 0xe2));

    cl = (uint8_t)SX16((uint16_t)(voice * 2 + 0x18ad));
    cl = (uint8_t)(0x3f - (uint8_t)(((uint16_t)al * cl) / 0x3f));
    ksl = (uint8_t)SX16((uint16_t)(voice * 2 + 0x1897));
    cl = (uint8_t)(cl | (uint8_t)(ksl << 6));
    bx = SX8((uint16_t)(SX8((uint16_t)(voice + 0x203)) + 0x227));
    sbp_write_left((uint16_t)(bx + 0x40), cl);

    if (SX8((uint16_t)(voice + 0x1905)) != 0) {
        cl = (uint8_t)SX16((uint16_t)(voice * 2 + 0x18ef));
        cl = (uint8_t)(0x3f - (uint8_t)(((uint16_t)al * cl) / 0x3f));
        ksl = (uint8_t)SX16((uint16_t)(voice * 2 + 0x18d9));
        cl = (uint8_t)(cl | (uint8_t)(ksl << 6));
        bx = SX8((uint16_t)(SX8((uint16_t)(voice + 0x20c)) + 0x227));
        sbp_write_left((uint16_t)(bx + 0x40), cl);
    }

    /* ---- the right bank: attenuated by the pan's complement ---- */
    al = (uint8_t)level;
    if (al > 0x3f)
        al = 0x3f;

    dl = (uint8_t)((uint8_t)(0x7f - SX8((uint16_t)(si + 0x145))) >> 1);
    if (dl > 0x3f)
        dl = 0x3f;
    dl = (uint8_t)(0x3f - SX8((uint16_t)(dl + 0xe2)));
    dl = (uint8_t)(((uint16_t)al * dl) / 0x3f);
    al = (uint8_t)(al - dl);
    al = SX8((uint16_t)(al + 0xe2));

    cl = (uint8_t)SX16((uint16_t)(voice * 2 + 0x18ad));
    cl = (uint8_t)(0x3f - (uint8_t)(((uint16_t)al * cl) / 0x3f));
    ksl = (uint8_t)SX16((uint16_t)(voice * 2 + 0x1897));
    cl = (uint8_t)(cl | (uint8_t)(ksl << 6));
    bx = SX8((uint16_t)(SX8((uint16_t)(voice + 0x203)) + 0x227));
    sbp_write_right((uint16_t)(bx + 0x40), cl);

    if (SX8((uint16_t)(voice + 0x1905)) != 0) {
        cl = (uint8_t)SX16((uint16_t)(voice * 2 + 0x18ef));
        cl = (uint8_t)(0x3f - (uint8_t)(((uint16_t)al * cl) / 0x3f));
        ksl = (uint8_t)SX16((uint16_t)(voice * 2 + 0x18d9));
        cl = (uint8_t)(cl | (uint8_t)(ksl << 6));
        bx = SX8((uint16_t)(SX8((uint16_t)(voice + 0x20c)) + 0x227));
        sbp_write_right((uint16_t)(bx + 0x40), cl);
    }
}

/*
 * SX.OVL SBP:0x1e2d
 *
 * Sound a note on a voice: BX the voice, CL the note, DL non-zero to key on.
 * Keying *off* is the same routine with DL zero, which is why `sbp_key_off`
 * calls it rather than writing 0xb0 itself.
 *
 * A voice whose `cs:0x1c1` byte is 0x80 or more is a **percussion** voice: the
 * note is clamped into 0x1b..0x58, and what is played is not that note but
 * `cs:0x1841[note - 0x1b]`, a drum map. Melodic voices skip all of it.
 *
 * The note then goes to quarter semitones, takes the channel's bend, and
 * splits by 0x30 - forty-eight quarter semitones to the octave - into a block
 * in AL and a position within the octave in AH. The position indexes the
 * F-number table at `cs:0x42`, whose low byte is register 0xa0 and whose top
 * two bits join the block in 0xb0.
 *
 * **The block is decremented before use**, unless it is already zero. So the
 * driver's octave numbering is one above the chip's.
 *
 * The level is the channel volume at `cs:0x135` times the voice's stored
 * velocity mapped through `cs:0xa2`, both taken one higher so that a zero of
 * either is not a silent product, and the 12-bit result shifted down by six.
 *
 * The test for -1 after the bend is transcribed and **cannot fire**: every path
 * out of `sbp_apply_bend` returns 0x1fc or less. Kept because the original
 * executes the comparison.
 */
void sbp_note(uint16_t voice, uint16_t note, uint16_t keyon)
{
    uint16_t si = SX8((uint16_t)(voice + 0x195));   /* the channel */
    uint16_t cx = note & 0xff;
    uint16_t di;
    uint8_t  al, ah;

    SX8((uint16_t)(voice + 0x1a0)) = (uint8_t)cx;

    if (SX8((uint16_t)(voice + 0x1c1)) >= 0x80) {
        if (cx < 0x1b)
            cx = 0x1b;
        else if (cx > 0x58)
            cx = 0x58;
        cx = SX8((uint16_t)((cx - 0x1b) + 0x1841));
    }

    cx = (uint16_t)(cx << 2);
    cx = sbp_apply_bend(voice, cx);
    if (cx == 0xffff)
        return;

    al = (uint8_t)(cx / 0x30);
    ah = (uint8_t)(cx % 0x30);

    di = (uint16_t)(ah * 2);
    cx = (uint16_t)SX16((uint16_t)(di + 0x42));

    sbp_write((uint16_t)(0xa0 + voice), (uint8_t)cx);

    if (al != 0)
        al--;
    al = (uint8_t)(al << 2);
    al = (uint8_t)(al | (uint8_t)(cx >> 8));
    if ((keyon & 0xff) != 0)
        al |= 0x20;

    sbp_write((uint16_t)(0xb0 + voice), al);

    {
        uint8_t vol = (uint8_t)(SX8((uint16_t)(si + 0x135)) + 1);
        uint8_t vel = SX8((uint16_t)(SX8((uint16_t)(voice + 0x1ab)) + 0xa2));

        vel = (uint8_t)(vel + 1);
        sbp_write_level(voice, (uint16_t)(((uint16_t)vol * vel) >> 6));
    }
}

/*
 * SX.OVL SBP:0x1dce
 *
 * Release a voice: sound its note with the key-on bit clear, mark it free with
 * 0xff, put it at the back of the priority list and drop its channel's count of
 * voices in use at `cs:0x1f2`.
 *
 * `cs:0x1b6` - the voice's held flag, set by `sbp_start_note` when a note it
 * wanted was already sounding - is cleared first.
 */
void sbp_key_off(uint16_t voice)
{
    uint16_t ch;

    SX8((uint16_t)(voice + 0x1b6)) = 0;

    sbp_note(voice, SX8((uint16_t)(voice + 0x1a0)), 0);

    SX8((uint16_t)(voice + 0x1a0)) = 0xff;
    sbp_touch_voice(voice);

    ch = SX8((uint16_t)(voice + 0x195));
    SX8((uint16_t)(ch + 0x1f2))--;
}

/*
 * SX.OVL SBP:0x1ade
 *
 * Find a voice for channel AL, returning it in BX or 0xffff if there is none.
 *
 * A free voice - note 0xff - is taken first, in priority order, which after
 * `sbp_touch_voice` means the one silent longest. Failing that the driver
 * **steals**, and it does not steal from the channel that asked unless it has
 * to: it walks all sixteen channels looking for the one furthest *over* its
 * quota at `cs:0x1e2`, and only when no channel is over quota does it fall
 * back to taking one of the requesting channel's own voices.
 *
 * The victim is keyed off properly rather than just reassigned.
 *
 * The quotas are all zero in the image, so on the face of it every channel is
 * always over quota by exactly its voice count and the worst offender wins.
 * Whether the caller fills them in has not been checked.
 */
uint16_t sbp_alloc_voice(uint16_t channel)
{
    uint16_t si, bx, dx = 0;
    uint8_t  cl, bl = 0;

    for (si = 0; si < 9; si++) {
        bx = SX8((uint16_t)(si + 0x1cc));
        if (SX8((uint16_t)(bx + 0x1a0)) == 0xff) {
            SX8((uint16_t)(bx + 0x195)) = (uint8_t)channel;
            return bx;
        }
    }

    for (si = 0; si < 0x10; si++) {
        cl = SX8((uint16_t)(si + 0x1f2));
        if (cl > SX8((uint16_t)(si + 0x1e2))) {
            cl = (uint8_t)(cl - SX8((uint16_t)(si + 0x1e2)));
            if (bl < cl) {
                bl = cl;
                dx = si;
            }
        }
    }

    cl = (uint8_t)channel;
    if (bl > 0)
        cl = (uint8_t)dx;

    for (si = 0; si < 9; si++) {
        bx = SX8((uint16_t)(si + 0x1cc));
        if (SX8((uint16_t)(bx + 0x195)) == cl) {
            sbp_key_off(bx);
            SX8((uint16_t)(bx + 0x195)) = (uint8_t)channel;
            return bx;
        }
    }

    return 0xffff;
}

/*
 * SX.OVL SBP:0x1a2e
 *
 * **Function 10**, the pitch bend. AL is the channel; **CL is the bend's most
 * significant seven bits and CH the least**, which is the same way round as the
 * note functions take note in CH and velocity in CL.
 *
 * The swap and shift assemble MIDI's two halves into one 14-bit number centred
 * on 0x2000, which is the centre `sbp_apply_bend` expects, and it is filed per
 * channel at `cs:0x175`. Every sounding voice on that channel is then re-sounded
 * so the new bend takes effect immediately - with the key-on bit set, since DX
 * is 1, which retriggers rather than merely retuning.
 */
void sbp_pitch_bend(uint16_t ax, uint16_t cx)
{
    uint16_t bend = (uint16_t)((((cx & 0xff) >> 1) << 8)
                               | ((cx >> 8) & 0xff)
                               | (((cx & 1) != 0) ? 0x80 : 0));
    uint16_t bx;

    SX16((uint16_t)((ax & 0xff) * 2 + 0x175)) = (int16_t)bend;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x195)) == (uint8_t)ax
            && SX8((uint16_t)(bx + 0x1a0)) != 0xff)
            sbp_note(bx, SX8((uint16_t)(bx + 0x1a0)), 1);
    }
}

/*
 * The driver keeps a **fourteen-byte shadow per operator** at `cs:0x279` -
 * eighteen operators, so 252 bytes, ending just below the patch bank at
 * `cs:0x379`. Each of the eight routines below builds one OPL register out of
 * it. The field offsets are read from which register bit each byte lands in:
 *
 *    +0  key scale level      +7  release
 *    +1  frequency multiple   +8  total level
 *    +2  feedback             +9  tremolo
 *    +3  attack               +10 vibrato
 *    +4  sustain              +11 key scale rate
 *    +5  sustaining envelope  +12 connection
 *    +6  decay                +13 waveform
 *
 * `cs:0x227` turns an operator into its OPL slot, `cs:0x239` into its voice,
 * and `cs:0x215` says which are carriers. Those three are transcribed tables
 * and their contents confirm the layout: 0x239 reads 0,1,2,0,1,2,3,4,5,... so
 * operators 0-2 are the modulators of voices 0-2 and 3-5 their carriers, and
 * 0x215 is 0 for exactly the first three of each six.
 */

/* SX.OVL SBP:0x232a - register 0x40, key scale level and total level. */
void sbp_write_op_level(uint16_t op)
{
    uint16_t di = (uint16_t)(op * 14);
    uint8_t  cl;

    cl = (uint8_t)(SX8((uint16_t)(di + 0x279)) << 6);
    cl = (uint8_t)(cl | (uint8_t)(SX8((uint16_t)(di + 0x281)) & 0x3f));

    sbp_write((uint16_t)(SX8((uint16_t)(op + 0x227)) + 0x40), cl);
}

/*
 * SX.OVL SBP:0x238a - register 0xc0, feedback and connection.
 *
 * This one is **per voice, not per operator**, so the carriers skip it - that
 * is what `cs:0x215` is for - and the register index comes from `cs:0x239`,
 * the operator's voice, rather than from its slot.
 *
 * The connection bit is **inverted**: the shadow's byte is non-zero for
 * frequency modulation and the chip's bit means additive, so a zero byte adds
 * one. The shift here is a 16-bit `shl ax, 1` where the neighbouring routines
 * shift AL alone; it makes no difference after the mask to four bits, but it
 * is what is written.
 */
void sbp_write_op_feedback(uint16_t op)
{
    uint16_t di;
    uint8_t  cl;

    if (SX8((uint16_t)(op + 0x215)) != 0)
        return;

    di = (uint16_t)(op * 14);

    cl = (uint8_t)(SX8((uint16_t)(di + 0x27b)) << 1);
    if (SX8((uint16_t)(di + 0x285)) == 0)
        cl++;
    cl &= 0x0f;

    sbp_write((uint16_t)(SX8((uint16_t)(op + 0x239)) + 0xc0), cl);
}

/* SX.OVL SBP:0x23da - register 0x60, attack and decay. */
void sbp_write_op_attack_decay(uint16_t op)
{
    uint16_t di = (uint16_t)(op * 14);
    uint8_t  cl;

    cl = (uint8_t)(SX8((uint16_t)(di + 0x27c)) << 4);
    cl = (uint8_t)(cl | (uint8_t)(SX8((uint16_t)(di + 0x27f)) & 0x0f));

    sbp_write((uint16_t)(SX8((uint16_t)(op + 0x227)) + 0x60), cl);
}

/* SX.OVL SBP:0x2420 - register 0x80, sustain and release. */
void sbp_write_op_sustain_release(uint16_t op)
{
    uint16_t di = (uint16_t)(op * 14);
    uint8_t  cl;

    cl = (uint8_t)(SX8((uint16_t)(di + 0x27d)) << 4);
    cl = (uint8_t)(cl | (uint8_t)(SX8((uint16_t)(di + 0x280)) & 0x0f));

    sbp_write((uint16_t)(SX8((uint16_t)(op + 0x227)) + 0x80), cl);
}

/*
 * SX.OVL SBP:0x2466 - register 0x20: tremolo, vibrato, sustaining envelope,
 * key scale rate and frequency multiple. Each of the four flags is tested
 * against zero rather than masked, so any non-zero byte sets its bit.
 */
void sbp_write_op_mult(uint16_t op)
{
    uint16_t di = (uint16_t)(op * 14);
    uint8_t  cl = 0;

    if (SX8((uint16_t)(di + 0x282)) != 0)
        cl |= 0x80;
    if (SX8((uint16_t)(di + 0x283)) != 0)
        cl |= 0x40;
    if (SX8((uint16_t)(di + 0x27e)) != 0)
        cl |= 0x20;
    if (SX8((uint16_t)(di + 0x284)) != 0)
        cl |= 0x10;
    cl = (uint8_t)(cl | (uint8_t)(SX8((uint16_t)(di + 0x27a)) & 0x0f));

    sbp_write((uint16_t)(SX8((uint16_t)(op + 0x227)) + 0x20), cl);
}

/*
 * SX.OVL SBP:0x24d4 - register 0xe0, the waveform, and only when the word at
 * `cs:0x1892` is non-zero. That word is the value the driver writes to
 * register 1 to enable wave select, so it doubles as "this chip has waveforms".
 */
void sbp_write_op_wave(uint16_t op)
{
    uint16_t di;

    if (SXSBP.word_1892 == 0)
        return;

    di = (uint16_t)(op * 14);

    sbp_write((uint16_t)(SX8((uint16_t)(op + 0x227)) + 0xe0),
              SX8((uint16_t)(di + 0x286)));
}

/*
 * SX.OVL SBP:0x2278 - register 0xbd, which is global rather than per operator:
 * tremolo depth, vibrato depth and the rhythm bits, from three separate bytes.
 */
void sbp_write_rhythm(void)
{
    uint8_t cl = 0;

    if (SXSBP.byte_188e != 0)
        cl |= 0x80;
    if (SXSBP.byte_188f != 0)
        cl |= 0x40;
    cl = (uint8_t)(cl | SXSBP.byte_1891);

    sbp_write(0xbd, cl);
}

/* SX.OVL SBP:0x2372 - register 8, the note-select bit, also global. */
void sbp_write_nts(void)
{
    sbp_write(8, (uint16_t)(SXSBP.byte_188d != 0 ? 0x40 : 0));
}

/*
 * SX.OVL SBP:0x2311
 *
 * Push one operator's whole shadow to the chip - the two global registers
 * first, then the six that belong to the operator. In this order.
 */
void sbp_write_operator(uint16_t op)
{
    sbp_write_rhythm();
    sbp_write_nts();
    sbp_write_op_level(op);
    sbp_write_op_feedback(op);
    sbp_write_op_attack_decay(op);
    sbp_write_op_sustain_release(op);
    sbp_write_op_mult(op);
    sbp_write_op_wave(op);
}

/*
 * SX.OVL SBP:0x22c8
 *
 * Copy a thirteen-byte operator block from `cs:CX` into the operator's shadow
 * and write it out. The fourteenth byte - the waveform - is not part of the
 * block: it comes from DL and is masked to two bits, since the OPL2 has four
 * waveforms.
 */
void sbp_load_operator(uint16_t op, uint16_t src, uint16_t dl)
{
    uint16_t si = (uint16_t)(op * 14);
    uint16_t i;

    for (i = 0; i != 0xd; i++) {
        SX8((uint16_t)(si + 0x279)) = SX8(src);
        si++;
        src++;
    }

    SX8((uint16_t)(si + 0x279)) = (uint8_t)(dl & 3);

    sbp_write_operator(op);
}

/*
 * SX.OVL SBP:0x22a1
 *
 * The same, by way of the scratch block at `cs:0x187f`. Why the copy exists is
 * not obvious - `sbp_load_operator` would take the caller's pointer directly -
 * and the only caller passes one of the two constant defaults, so nothing here
 * depends on it. Transcribed rather than folded away.
 */
void sbp_load_operator_scratch(uint16_t op, uint16_t src, uint16_t dl)
{
    uint16_t i;

    for (i = 0; i != 0xd; i++)
        SX8((uint16_t)(i + 0x187f)) = SX8((uint16_t)(src + i));

    sbp_load_operator(op, 0x187f, dl);
}

/*
 * SX.OVL SBP:0x224b
 *
 * Put all eighteen operators back to a default - `cs:0x26b` for the carriers,
 * `cs:0x25d` for the modulators, picked by `cs:0x215` - with waveform zero.
 */
void sbp_reset_operators(void)
{
    uint16_t di;

    for (di = 0; di != 0x12; di++) {
        if (SX8((uint16_t)(di + 0x215)) != 0)
            sbp_load_operator_scratch(di, 0x26b, 0);
        else
            sbp_load_operator_scratch(di, 0x25d, 0);
    }
}

/*
 * SX.OVL SBP:0x2513
 *
 * Silence the chip: zero **every register from 0 to 0xf5**, then set register 1
 * to 0x20 to turn wave select back on - keeping that value in `cs:0x1892`,
 * which is what gates the waveform writes - and reload the operator defaults.
 *
 * Zeroing 0xf6 registers rather than the ones that exist is what the original
 * does; the chip ignores the gaps.
 */
void sbp_silence(void)
{
    uint16_t bx;

    for (bx = 0; bx < 0xf6; bx++)
        sbp_write(bx, 0);

    SXSBP.word_1892 = 0x20;
    sbp_write(1, (uint16_t)SXSBP.word_1892);

    sbp_reset_operators();
}

/*
 * SX.OVL SBP:0x20d8
 *
 * Load a patch into a voice. BX is the voice and CX points at a **28-byte**
 * record: a thirteen-byte modulator block, a thirteen-byte carrier block, then
 * one waveform byte for each.
 *
 * Byte 0xc - the last of the modulator block - is the connection, and a
 * non-zero value means frequency modulation. In that case `cs:0x1905` is
 * cleared and **the modulator's level parameters are not cached**, because on
 * an FM voice the modulator's level is timbre and `sbp_write_level` must not
 * scale it. Additive voices cache both, which is exactly the flag that routine
 * tests.
 *
 * The cached values are the key scale level as it stands and the level
 * *inverted* - 0x3f minus it - so the level path can multiply rather than
 * subtract. The third cache, divided by fifteen, is written and never read by
 * anything in this driver.
 */
void sbp_load_patch(uint16_t voice, uint16_t patch)
{
    uint16_t w = (uint16_t)(voice * 2);
    uint16_t mod, car;

    SX8((uint16_t)(voice + 0x1905)) = 1;

    if (SX8((uint16_t)(patch + 0xc)) != 0) {
        SX8((uint16_t)(voice + 0x1905)) = 0;
    } else {
        SX16((uint16_t)(w + 0x18d9)) = SX8(patch);
        SX16((uint16_t)(w + 0x18ef)) =
            (int16_t)(uint8_t)(0x3f - SX8((uint16_t)(patch + 8)));
        SX16((uint16_t)(w + 0x1910)) =
            (int16_t)((uint16_t)SX16((uint16_t)(w + 0x18ef)) / 0xf);
    }

    SX16((uint16_t)(w + 0x1897)) = SX8((uint16_t)(patch + 0xd));
    SX16((uint16_t)(w + 0x18ad)) =
        (int16_t)(uint8_t)(0x3f - SX8((uint16_t)(patch + 0x15)));
    SX16((uint16_t)(w + 0x18c3)) =
        (int16_t)((uint16_t)SX16((uint16_t)(w + 0x18ad)) / 0xf);

    mod = SX8((uint16_t)(w + 0x24b));
    car = SX8((uint16_t)(w + 1 + 0x24b));

    sbp_load_operator(mod, patch, SX8((uint16_t)(patch + 0x1a)));
    sbp_load_operator(car, (uint16_t)(patch + 0xd),
                      SX8((uint16_t)(patch + 0x1b)));
}

/*
 * SX.OVL SBP:0x1d4f
 *
 * Start a note on a voice already allocated to a channel: BX the voice, CH the
 * note, CL the velocity.
 *
 * **Channel 9 is percussion**, and there the patch is not the channel's
 * program at all - it is the note itself, clamped to 0x1b..0x58 and offset by
 * 0x65, which puts every drum patch at 0x80 or above. That is precisely the
 * test `sbp_note` uses to decide a voice is percussion, and it is why the drum
 * map there is indexed by the same `note - 0x1b`.
 *
 * The patch is only reloaded when it differs from what the voice already holds,
 * and only when the FM switch at `cs:0x123` is on - so a driver muted through
 * function 13 stops re-programming the chip as well as silencing the mixer.
 * The patch bank is 28-byte records from `cs:0x379`, and the multiply by 28 is
 * open-coded as shifts and adds.
 */
void sbp_start_voice(uint16_t voice, uint16_t cx)
{
    uint16_t ch = SX8((uint16_t)(voice + 0x195));
    uint8_t  note = (uint8_t)(cx >> 8);
    uint8_t  patch;

    patch = SX8((uint16_t)(ch + 0x125));
    SX8((uint16_t)(ch + 0x1f2))++;
    sbp_touch_voice(voice);

    if (ch == 9) {
        patch = note;
        if (patch < 0x1b)
            patch = 0x1b;
        else if (patch > 0x58)
            patch = 0x58;
        patch = (uint8_t)(patch + 0x65);
    }

    if (patch != SX8((uint16_t)(voice + 0x1c1)) && SXSBP.byte_0123 != 0) {
        SX8((uint16_t)(voice + 0x1c1)) = patch;
        sbp_load_patch(voice, (uint16_t)(patch * 28 + 0x379));
    }

    SX8((uint16_t)(voice + 0x1ab)) = (uint8_t)cx;
    sbp_note(voice, note, 1);
}

/*
 * SX.OVL SBP:0x1cb3
 *
 * **Controller 7**, the channel volume, **halved** on the way in - MIDI's 0..127
 * into the 0..63 the level path works in - and then every sounding voice on the
 * channel is re-sounded so the change is heard at once.
 *
 * Re-sounding is `sbp_note` with the key-on bit set, which retriggers the
 * envelope rather than merely changing the level. The original does that for
 * volume, pan and bend alike.
 */
void sbp_ctrl_volume(uint16_t ax, uint16_t cx)
{
    uint16_t bx;

    SX8((uint16_t)((ax & 0xff) + 0x135)) = (uint8_t)((cx & 0xff) >> 1);

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x195)) == (uint8_t)ax
            && SX8((uint16_t)(bx + 0x1a0)) != 0xff)
            sbp_note(bx, SX8((uint16_t)(bx + 0x1a0)), 1);
    }
}

/*
 * SX.OVL SBP:0x1cec
 *
 * **Controller 10**, the pan, stored whole - not halved, unlike the volume -
 * because `sbp_write_level` halves it there and takes its complement against
 * 0x7f. Same re-sound loop.
 */
void sbp_ctrl_pan(uint16_t ax, uint16_t cx)
{
    uint16_t bx;

    SX8((uint16_t)((ax & 0xff) + 0x145)) = (uint8_t)cx;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x195)) == (uint8_t)ax
            && SX8((uint16_t)(bx + 0x1a0)) != 0xff)
            sbp_note(bx, SX8((uint16_t)(bx + 0x1a0)), 1);
    }
}

/*
 * SX.OVL SBP:0x1d1f
 *
 * **Controller 64**, the sustain pedal. Pressing it only records the value;
 * releasing it - a zero - keys off every voice on the channel that `cs:0x1b6`
 * marks as held, which is where `sbp_stop_note` put the notes whose release
 * the pedal swallowed.
 */
void sbp_ctrl_sustain(uint16_t ax, uint16_t cx)
{
    uint16_t bx;

    SX8((uint16_t)((ax & 0xff) + 0x155)) = (uint8_t)cx;

    if ((cx & 0xff) != 0)
        return;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x195)) == (uint8_t)ax
            && SX8((uint16_t)(bx + 0x1b6)) != 0)
            sbp_key_off(bx);
    }
}

/*
 * SX.OVL SBP:0x1bf6
 *
 * Reserve CL voices for channel AL. A voice is reserved by writing the channel
 * into `cs:0x1d7`, and any note it was playing is stopped first. Each one taken
 * raises the channel's quota at `cs:0x1e2`, which is what `sbp_alloc_voice`
 * compares against when it decides whom to steal from.
 *
 * Whatever could not be satisfied is added to `cs:0x165`, the channel's
 * outstanding request, to be met later by `sbp_redistribute_voices`.
 */
void sbp_reserve_voices(uint16_t ax, uint16_t cx)
{
    uint16_t bx;
    uint8_t  cl = (uint8_t)cx;

    for (bx = 0; bx < 9 && cl > 0; bx++) {
        if (SX8((uint16_t)(bx + 0x1d7)) != 0xff)
            continue;
        if (SX8((uint16_t)(bx + 0x1a0)) != 0xff)
            sbp_key_off(bx);
        SX8((uint16_t)(bx + 0x1d7)) = (uint8_t)ax;
        SX8((uint16_t)((ax & 0xff) + 0x1e2))++;
        cl--;
    }

    SX8((uint16_t)((ax & 0xff) + 0x165)) =
        (uint8_t)(SX8((uint16_t)((ax & 0xff) + 0x165)) + cl);
}

/*
 * SX.OVL SBP:0x1c3a
 *
 * Give CL voices back from channel AL. The outstanding request at `cs:0x165`
 * is spent first, since a request never met costs nothing to withdraw. Then
 * **two passes**: silent voices are released before sounding ones, so a note
 * is only cut when there is no idle voice left to give up.
 */
void sbp_release_voices(uint16_t ax, uint16_t cx)
{
    uint16_t si = ax & 0xff;
    uint16_t bx;
    uint8_t  cl = (uint8_t)cx;

    if (SX8((uint16_t)(si + 0x165)) >= cl) {
        SX8((uint16_t)(si + 0x165)) =
            (uint8_t)(SX8((uint16_t)(si + 0x165)) - cl);
        return;
    }

    cl = (uint8_t)(cl - SX8((uint16_t)(si + 0x165)));
    SX8((uint16_t)(si + 0x165)) = 0;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x1d7)) == (uint8_t)ax
            && SX8((uint16_t)(bx + 0x1a0)) == 0xff) {
            SX8((uint16_t)(bx + 0x1d7)) = 0xff;
            SX8((uint16_t)(si + 0x1e2))--;
            if (--cl == 0)
                return;
        }
    }

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x1d7)) == (uint8_t)ax) {
            sbp_key_off(bx);
            SX8((uint16_t)(bx + 0x1d7)) = 0xff;
            SX8((uint16_t)(si + 0x1e2))--;
            if (--cl == 0)
                return;
        }
    }
}

/*
 * SX.OVL SBP:0x1b9c
 *
 * Hand newly freed voices to the channels still waiting for them. Counts the
 * unreserved voices, then walks the sixteen channels in order giving each its
 * outstanding request from `cs:0x165` until the free voices run out - the last
 * channel served keeps the remainder of its request pending.
 *
 * First come, first served by channel number, not by how long anyone waited.
 */
void sbp_redistribute_voices(void)
{
    uint16_t si;
    uint8_t  cl = 0;
    uint8_t  ch;

    for (si = 0; si < 9; si++) {
        if (SX8((uint16_t)(si + 0x1d7)) == 0xff)
            cl++;
    }

    if (cl == 0)
        return;

    for (si = 0; si < 0x10; si++) {
        ch = SX8((uint16_t)(si + 0x165));
        if (ch == 0)
            continue;

        if (ch < cl) {
            cl = (uint8_t)(cl - ch);
            SX8((uint16_t)(si + 0x165)) = 0;
            sbp_reserve_voices(si, ch);
        } else {
            SX8((uint16_t)(si + 0x165)) = (uint8_t)(ch - cl);
            sbp_reserve_voices(si, cl);
            return;
        }
    }
}

/*
 * SX.OVL SBP:0x1b5c
 *
 * **Controller 75**, the voice reservation. CL is how many voices the channel
 * wants *in total*, so the driver counts what it already has - reserved voices
 * plus its outstanding request - and asks for or gives back the difference.
 *
 * Giving back is followed by a redistribution, since the voices just released
 * may be what another channel has been waiting for. Asking for more is not:
 * anything unavailable simply joins this channel's own request.
 */
void sbp_ctrl_reserve(uint16_t ax, uint16_t cx)
{
    uint16_t si;
    uint8_t  dl = 0;
    uint8_t  cl = (uint8_t)cx;

    for (si = 0; si < 9; si++) {
        if (SX8((uint16_t)(si + 0x1d7)) == (uint8_t)ax)
            dl++;
    }

    dl = (uint8_t)(dl + SX8((uint16_t)((ax & 0xff) + 0x165)));

    if (dl == cl)
        return;

    if (dl < cl) {
        sbp_reserve_voices(ax, (uint16_t)(cl - dl));
    } else {
        sbp_release_voices(ax, (uint16_t)(dl - cl));
        sbp_redistribute_voices();
    }
}

/*
 * SX.OVL SBP:0x19d4
 *
 * **Function 7**, the controller. AL is the channel, CH the controller number
 * and CL its value. Five are handled - volume, pan, sustain, voice reservation
 * and all-notes-off - and everything else is ignored silently.
 *
 * All-notes-off is inline here rather than a routine of its own, and it keys
 * off every sounding voice on the channel **without regard to the sustain
 * pedal**, unlike `sbp_stop_note`.
 */
void sbp_controller(uint16_t ax, uint16_t cx)
{
    uint8_t  ctrl = (uint8_t)(cx >> 8);
    uint16_t bx;

    if (ctrl == 7) {
        sbp_ctrl_volume(ax, cx);
    } else if (ctrl == 0xa) {
        sbp_ctrl_pan(ax, cx);
    } else if (ctrl == 0x40) {
        sbp_ctrl_sustain(ax, cx);
    } else if (ctrl == 0x4b) {
        sbp_ctrl_reserve(ax, cx);
    } else if (ctrl == 0x7b) {
        for (bx = 0; bx < 9; bx++) {
            if (SX8((uint16_t)(bx + 0x195)) == (uint8_t)ax
                && SX8((uint16_t)(bx + 0x1a0)) != 0xff)
                sbp_key_off(bx);
        }
    }
}

/*
 * SX.OVL SBP:0x1957
 *
 * **Function 4**, note off. AL is the channel and CH the note.
 *
 * A note released while the channel's sustain pedal is down is not stopped: the
 * voice is marked held at `cs:0x1b6` and `sbp_ctrl_sustain` releases it when
 * the pedal comes up.
 *
 * The loop does not stop at the first match - if the same note is sounding on
 * two voices of one channel, both are released.
 */
void sbp_stop_note(uint16_t ax, uint16_t cx)
{
    uint16_t bx;

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x195)) != (uint8_t)ax)
            continue;
        if (SX8((uint16_t)(bx + 0x1a0)) != (uint8_t)(cx >> 8))
            continue;

        if (SX8((uint16_t)((ax & 0xff) + 0x155)) != 0)
            SX8((uint16_t)(bx + 0x1b6)) = 1;
        else
            sbp_key_off(bx);
    }
}

/*
 * SX.OVL SBP:0x198d
 *
 * **Function 5**, note on. AL the channel, CH the note, CL the velocity.
 *
 * A velocity of zero is a note off, which is how running-status MIDI ends
 * notes. Notes outside 12..107 are dropped - the F-number table only covers
 * that span - and the velocity is halved into the 0..63 the level path uses.
 *
 * If the channel is **already sounding this note**, that voice is reused: keyed
 * off and started again, rather than a second voice being allocated to the same
 * pitch. Otherwise the allocator finds one, and only 0xff in its low byte -
 * not the whole 0xffff - is taken as failure.
 */
void sbp_start_note(uint16_t ax, uint16_t cx)
{
    uint8_t  note = (uint8_t)(cx >> 8);
    uint16_t bx;

    if ((cx & 0xff) == 0) {
        sbp_stop_note(ax, cx);
        return;
    }

    if (note < 0xc || note > 0x6b)
        return;

    cx = (uint16_t)((cx & 0xff00) | (uint8_t)((cx & 0xff) >> 1));

    for (bx = 0; bx < 9; bx++) {
        if (SX8((uint16_t)(bx + 0x195)) == (uint8_t)ax
            && SX8((uint16_t)(bx + 0x1a0)) == note) {
            sbp_key_off(bx);
            sbp_start_voice(bx, cx);
            return;
        }
    }

    bx = sbp_alloc_voice(ax & 0xff);
    if ((uint8_t)bx != 0xff)
        sbp_start_voice(bx, cx);
}

/*
 * SX.OVL SBP:0x1acd
 *
 * **Function 2**, stop everything: silence the chip, then write the mixer's FM
 * volume.
 *
 * **The value or-ed with 0x99 is CL, which function 2 has no argument of its
 * own.** `mov cl, 0x99` was surely the intent. It is not undefined, though:
 * `silence_driver` at 0x2664e sets `cl` to 0xf once and calls function 12 and
 * then function 2 without touching it again, and function 12 preserves CX. So
 * the mixer receives 0x0f | 0x99 = **0x9f** every time. Transcribed as an or
 * rather than folded to the constant, since the argument is what the original
 * reads.
 */
void sbp_stop_all(uint16_t cx)
{
    sbp_silence();
    sbp_mixer_write(0x26, (uint16_t)((cx & 0xff) | 0x99));
}

/* SX.OVL SBP:0x1956 - functions 3, 6, 9, 14, 15 and 16, all a bare `ret`. */
void sbp_nop(void)
{
}

/*
 * SX.OVL SBP:0x253d
 *
 * **Function 17**, read back a channel's state. AH says which - 0xe0 the pitch
 * bend, 0xc0 the program, 0xb0 a controller chosen by CH - and AL is the
 * channel. Anything unrecognised answers 0xffff.
 *
 * Controller 75 is the odd one: a CL of 0xff asks for the channel's *quota* at
 * `cs:0x1e2` rather than its outstanding request at `cs:0x165`.
 *
 * There is an unreachable `jmp` in the middle of the controller-75 arm, left
 * where it is.
 */
uint16_t sbp_query(uint16_t ax, uint16_t cx)
{
    uint16_t si = ax & 0xff;
    uint8_t  ah = (uint8_t)(ax >> 8);
    uint8_t  ch = (uint8_t)(cx >> 8);

    if (ah == 0xe0)
        return (uint16_t)SX16((uint16_t)(si * 2 + 0x175));

    if (ah == 0xc0)
        return SX8((uint16_t)(si + 0x125));

    if (ah == 0xb0) {
        if (ch == 7)
            return SX8((uint16_t)(si + 0x135));
        if (ch == 0xa)
            return SX8((uint16_t)(si + 0x145));
        if (ch == 0x40)
            return SX8((uint16_t)(si + 0x155));
        if (ch == 0x4b) {
            if ((cx & 0xff) == 0xff)
                return SX8((uint16_t)(si + 0x1e2));
            return SX8((uint16_t)(si + 0x165));
        }
    }

    return 0xffff;
}

/*
 * SX.OVL SBP:0x25aa
 *
 * **Function 1**, initialise. ES:AX points at the patch bank, whose length the
 * driver already holds at `cs:0x377`; it is copied to `cs:0x379`, the chip is
 * silenced and the master level set to 15.
 *
 * The answer is **0x25aa in AX - this routine's own offset** - and 0x0800 in
 * CX. `ADL:` does exactly the same thing with its own address, 0x2414, so the
 * convention is the driver's rather than a value either one computed.
 */
void sbp_init(uint16_t off, uint16_t seg, uint16_t *ax, uint16_t *cx)
{
    const uint8_t *src = (const uint8_t *)FAR_PTR(seg, off);
    uint16_t n = (uint16_t)SXSBP.word_0377;
    uint16_t di;

    for (di = 0; di < n; di++)
        SX8((uint16_t)(di + 0x379)) = src[di];

    sbp_silence();
    sbp_param_345(0x0f);

    *ax = 0x25aa;
    *cx = 0x0800;
}

/*
 * SX.OVL SBP:0x25dc
 *
 * **Function 0**, describe: 0x0103 in AX and **9 in CL, the voice count**. Nine
 * is one OPL2's worth of channels, which is what this driver drives - the
 * second bank carries the same nine voices at different levels to make the
 * stereo, not another nine.
 */
void sbp_describe_0(uint16_t *ax, uint16_t *cx)
{
    *ax = 0x0103;
    *cx = 0x0009;
}
