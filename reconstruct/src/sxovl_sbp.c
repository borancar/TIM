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
