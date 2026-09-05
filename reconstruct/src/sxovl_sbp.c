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

    io_out8((uint16_t)SX16(0x2c), (uint8_t)reg);
    for (i = 0; i < 5; i++)
        (void)io_in8((uint16_t)SX16(0x2c));

    io_out8((uint16_t)SX16(0x30), (uint8_t)val);
    for (i = 0; i < 0x21; i++)
        (void)io_in8((uint16_t)SX16(0x2e));
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

    io_out8((uint16_t)SX16(0x3e), (uint8_t)reg);
    for (i = 0; i < 5; i++)
        (void)io_in8((uint16_t)SX16(0x3e));

    io_out8((uint16_t)SX16(0x40), (uint8_t)val);
    for (i = 0; i < 0x21; i++)
        (void)io_in8((uint16_t)SX16(0x2e));
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
    uint8_t prev = SX8(0x122);

    if ((cl & 0xff) == 0xff)
        return prev;

    SX8(0x122) = (uint8_t)cl;
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
    uint8_t prev = SX8(0x124);
    uint8_t both;

    if ((cl & 0xff) == 0xff)
        return prev;

    SX8(0x124) = (uint8_t)cl;

    both = (uint8_t)((uint8_t)cl << 4);
    both = (uint8_t)(both | SX8(0x124));
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
    uint8_t prev = SX8(0x123);
    uint16_t arg;

    if ((cl & 0xff) == 0xff)
        return prev;

    SX8(0x123) = (uint8_t)cl;

    arg = cl & 0xff;
    if (arg != 0)
        arg = SX8(0x124);

    SX8(0x124) = (uint8_t)sbp_param_345(arg);

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

    if (SX8(0x202) != 0 && (reg & 0xff) >= 0xc0 && (reg & 0xff) <= 0xc8)
        val |= 0x20;

    io_out8((uint16_t)SX16(0x32), (uint8_t)reg);
    for (i = 0; i < 5; i++)
        (void)io_in8((uint16_t)SX16(0x32));

    io_out8((uint16_t)SX16(0x36), (uint8_t)val);
    for (i = 0; i < 0x21; i++)
        (void)io_in8((uint16_t)SX16(0x34));
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

    if (SX8(0x202) != 0 && (reg & 0xff) >= 0xc0 && (reg & 0xff) <= 0xc8)
        val |= 0x10;

    io_out8((uint16_t)SX16(0x38), (uint8_t)reg);
    for (i = 0; i < 5; i++)
        (void)io_in8((uint16_t)SX16(0x38));

    io_out8((uint16_t)SX16(0x3c), (uint8_t)val);
    for (i = 0; i < 0x21; i++)
        (void)io_in8((uint16_t)SX16(0x3a));
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

    SX8(0x1d4) = (uint8_t)voice;
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
