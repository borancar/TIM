/*
 * The General MIDI sound driver, `SX.OVL`'s `GMD:` chunk, as it is loaded.
 *
 * Provenance is `SX.OVL GMD:0xNNNN` - offsets within the loaded driver, not
 * image addresses, because the loader chooses the segment. `out/res/SX_GMD.mem`
 * is the dump every offset here was read from: 2592 bytes at segment 0x418f,
 * banner `dude%General MIDI for Roland MPU interface`.
 *
 * It has the same shape as the speaker driver - `jmp 0x5aa` at offset 0 onto a
 * dispatcher that indexes eighteen near offsets at `cs:0x586` with the function
 * number in BP - and the same eighteen function numbers mean the same things,
 * which is what makes one driver substitutable for another at all.
 *
 * Its state lives in its own code segment and is reached with `SX8`/`SX16`,
 * exactly as the speaker's is; most of it is a 0x481-byte configuration blob
 * that `gmd_init` copies in from a patch bank.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * The MPU-401's two ports. The driver holds neither in a variable - they are
 * immediates in every routine that touches them - so they are constants here.
 */
#define MPU_DATA    0x330
#define MPU_STATUS  0x331

/*
 * SX.OVL GMD:0x09f4
 *
 * The settling delay: 0x7500 reads of the status port, the answer thrown away.
 * Only initialisation uses it, around the commands that reset the interface.
 */
void gmd_delay(void)
{
    uint16_t di;

    for (di = 0x7500; di != 0; di--)
        (void)io_in8(MPU_STATUS);
}

/*
 * SX.OVL GMD:0x0868
 *
 * Write one **data** byte. Bit 6 of the status port clear means there is room;
 * while it is set the driver spends up to 0xff turns draining anything the
 * interface has to say - bit 7 clear means a byte is waiting, and it is read
 * and dropped - and then writes regardless.
 *
 * The drain is why this cannot be simplified to a spin: an MPU that has filled
 * its input queue will not accept output until the queue is read.
 */
void gmd_write_data(uint8_t value)
{
    uint16_t cx = 0xff;
    uint8_t al;

    for (;;) {
        al = io_in8(MPU_STATUS);
        if ((al & 0x40) == 0) {
            io_out8(MPU_DATA, value);
            return;
        }

        cx--;
        if ((al & 0x80) == 0)
            (void)io_in8(MPU_DATA);

        if ((int16_t)cx < 1)
            return;
    }
}

/*
 * SX.OVL GMD:0x0832
 *
 * Write one **command** byte and wait for the acknowledgement. Up to 0xffff
 * turns for room, then the byte, then up to 0xffff turns for bit 7 to say a
 * byte is waiting, then the read - which should be 0xFE and is not checked
 * against anything, the comparison at 0x85f falling through either way.
 */
void gmd_write_command(uint8_t value)
{
    uint16_t cx;
    uint8_t al;

    for (cx = 0xffff; cx != 0; cx--) {
        if ((io_in8(MPU_STATUS) & 0x40) == 0)
            break;
    }

    if (cx == 0)
        return;

    io_out8(MPU_STATUS, value);

    for (cx = 0xffff; cx != 0; cx--) {
        al = io_in8(MPU_STATUS);
        if ((al & 0x80) == 0)
            break;
    }

    (void)io_in8(MPU_DATA);
}

/*
 * SX.OVL GMD:0x0807
 *
 * Send one MIDI message: the status byte is the channel in AL or'd with the
 * kind in AH, and it is remembered at `cs:0x584`. Then the data bytes, CH
 * first and CL second - **except** for 0xC0 and 0xD0, which take one, as MIDI
 * requires.
 *
 * The status byte goes out every time; this driver does not use running
 * status, though the sequence its initialisation replays can.
 */
void gmd_send(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax, ah = (uint8_t)(ax >> 8);
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    uint8_t dl = (uint8_t)(al | ah);

    SX8(0x584) = dl;
    gmd_write_data(dl);

    if (ah != 0xc0 && ah != 0xd0)
        gmd_write_data(ch);

    gmd_write_data(cl);
}

/*
 * SX.OVL GMD:0x0910
 *
 * Reset the interface: command 0xFF.
 */
void gmd_reset(void)
{
    gmd_write_command(0xff);
}

/*
 * SX.OVL GMD:0x05b6
 *
 * The do-nothing entry, which is what BP 3, 6, 9, 14, 15 and 16 all point at.
 */
void gmd_nop(void)
{
}

/*
 * SX.OVL GMD:0x05b7  - function 11
 *
 * Read and set the byte at `cs:0x585`. 0xff reads without writing, which is
 * the convention this driver family uses for every parameter.
 */
uint16_t gmd_param_349(uint8_t cl)
{
    uint16_t ax = SX8(0x585);

    if (cl == 0xff)
        return ax;

    SX8(0x585) = cl;
    return ax;
}

/*
 * OURS: the note-range fold the two note routines share.
 *
 * Both add the channel's transpose at `cs:[si+0x544]` and then fold the result
 * back under 0x80 by repeatedly adding 0xf4 - twelve semitones down - or 0x0c
 * if the transpose was itself negative. The two copies in the driver are
 * instruction for instruction the same; this is that code once.
 */
static uint8_t gmd_transpose(uint16_t si, uint8_t note)
{
    uint8_t step = 0xf4;

    note = (uint8_t)(note + SX8((uint16_t)(si + 0x544)));

    if (SX8((uint16_t)(si + 0x544)) >= 0x80)
        step = 0x0c;

    while (note >= 0x80)
        note = (uint8_t)(note + step);

    return note;
}

/*
 * SX.OVL GMD:0x05c9  - function 4
 *
 * Stop a note: a 0x90 status with a velocity of zero, which is MIDI's other
 * way of saying note-off and the only one this driver uses.
 *
 * Channel 9 is percussion and goes through the map at `cs:0x1c1`, which turns
 * the game's note into the drum the bank assigns it; 0xff there means the
 * note is not mapped and nothing is sent. Every other channel is skipped
 * entirely when `cs:[si+0x574]` is 0xff, which is how a bank disables one.
 */
void gmd_stop_note(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax;
    uint8_t ch = (uint8_t)(cx >> 8);
    uint16_t si;

    if (al == 9) {
        si = (uint16_t)(ch & 0x7f);
        ch = SX8((uint16_t)(si + 0x1c1));
        if (ch == 0xff)
            return;
    } else {
        si = (uint16_t)(ax & 0xf);
        if (SX8((uint16_t)(si + 0x574)) == 0xff)
            return;
        ch = gmd_transpose(si, ch);
    }

    gmd_send((uint16_t)((0x90 << 8) | al), (uint16_t)(ch << 8));
}

/*
 * SX.OVL GMD:0x0617  - function 5
 *
 * Start a note. The same channel handling as the stop, and then the velocity
 * is run through a curve: `cs:0x2c2` is a table of 0x80-byte rows, the row
 * chosen by `cs:[si+0x564]`, indexed by the velocity the game asked for. So a
 * bank can shape how hard a voice speaks without the game knowing.
 *
 * `cs:[si+0x534]` is raised to say the channel is sounding, which is what the
 * all-notes-off controller tests before bothering to send anything.
 */
void gmd_start_note(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax;
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    uint16_t si, di;

    if (al == 9) {
        si = (uint16_t)(ch & 0x7f);
        ch = SX8((uint16_t)(si + 0x1c1));
        if (ch == 0xff)
            return;
        si = (uint16_t)(ax & 0xf);
    } else {
        si = (uint16_t)(ax & 0xf);
        if (SX8((uint16_t)(si + 0x574)) == 0xff)
            return;
        ch = gmd_transpose(si, ch);
    }

    di = (uint16_t)((cl & 0x7f)
                    + 0x80 * (uint16_t)SX8((uint16_t)(si + 0x564)));
    cl = SX8((uint16_t)(di + 0x2c2));

    SX8((uint16_t)(si + 0x534)) = 1;

    gmd_send((uint16_t)((0x90 << 8) | al),
             (uint16_t)((ch << 8) | cl));
}

/*
 * SX.OVL GMD:0x0685  - function 7
 *
 * A controller. Four are understood and the rest are dropped:
 *
 * - **7, volume.** Kept per channel at `cs:[si+0x504]`, and then - only if the
 *   master volume is enabled at `cs:0x4c3` - offset by `cs:[si+0x554]`,
 *   clamped, and scaled by the master at `cs:0x4c2` over 15. A scaled value
 *   that lands on zero when the input was not zero is nudged back up to 1, so
 *   a quiet voice never disappears entirely.
 * - **10, pan**, **0x40, sustain**: sent only when the value has changed.
 * - **0x7b, all notes off**: sent only when the channel is sounding, and
 *   clears that flag.
 *
 * The clamp is worth reading twice: 0x7f is the ceiling, but if the *offset*
 * was negative the overflow means the sum went below zero, and the answer is
 * 1 rather than 0x7f.
 */
void gmd_controller(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax;
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    uint16_t si = (uint16_t)(ax & 0xf);

    cl &= 0x7f;

    if (ch == 7) {
        uint8_t save, quot, rem;

        SX8((uint16_t)(si + 0x504)) = cl;

        if (SX8(0x4c3) == 0)
            return;

        cl = (uint8_t)(cl + SX8((uint16_t)(si + 0x554)));
        if (cl >= 0x80) {
            cl = 0x7f;
            if (SX8((uint16_t)(si + 0x554)) >= 0x80)
                cl = 1;
        }

        save = al;
        {
            uint16_t prod = (uint16_t)(SX8(0x4c2) * cl);
            quot = (uint8_t)(prod / 0xf);
            rem  = (uint8_t)(prod % 0xf);
        }
        cl = quot;
        al = save;
        ch = 7;

        if (cl == 0 && rem != 0)
            cl++;
    } else if (ch == 0x0a) {
        if (SX8((uint16_t)(si + 0x514)) == cl)
            return;
        SX8((uint16_t)(si + 0x514)) = cl;
    } else if (ch == 0x40) {
        if (SX8((uint16_t)(si + 0x524)) == cl)
            return;
        SX8((uint16_t)(si + 0x524)) = cl;
    } else if (ch == 0x7b) {
        if (SX8((uint16_t)(si + 0x534)) == 0)
            return;
        SX8((uint16_t)(si + 0x534)) = 0;
    } else {
        return;
    }

    gmd_send((uint16_t)((0xb0 << 8) | al), (uint16_t)((ch << 8) | cl));
}

/*
 * SX.OVL GMD:0x07de  - function 10
 *
 * Pitch bend, sent only when it has changed. The two bytes are kept at
 * `cs:[si*2 + 0x4d4]` and go out CH first, which is the order `gmd_send`
 * gives every two-data-byte message.
 */
void gmd_pitch_bend(uint16_t ax, uint16_t cx)
{
    uint8_t al = (uint8_t)ax;
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);
    uint16_t si = (uint16_t)((ax & 0xf) * 2);

    if (SX8((uint16_t)(si + 0x4d4)) == cl
        && SX8((uint16_t)(si + 0x4d5)) == ch)
        return;

    SX8((uint16_t)(si + 0x4d4)) = cl;
    SX8((uint16_t)(si + 0x4d5)) = ch;

    gmd_send((uint16_t)((0xe0 << 8) | al), cx);
}

/*
 * SX.OVL GMD:0x082c  - function 2
 *
 * Stop everything, by resetting the interface rather than by sending an
 * all-notes-off to each channel.
 */
void gmd_stop_all(void)
{
    gmd_reset();
}

/*
 * SX.OVL GMD:0x0896  - function 12
 *
 * Read and set the master volume at `cs:0x4c2`, answering what it was. 0xff
 * reads without writing, as ever.
 *
 * Setting it re-sends controller 7 for channels **1 to 9** with each one's
 * stored level, so the new master takes effect on voices that are already
 * playing. Channel 0 is not among them and neither is anything above 9; the
 * loop is `al` from 1 while it is not 0xa.
 */
uint16_t gmd_param_345(uint8_t cl)
{
    uint16_t ax = SX8(0x4c2);
    uint16_t al, si;

    if (cl == 0xff)
        return ax;

    SX8(0x4c2) = cl;

    if (SX8(0x4c3) == 0)
        return ax;

    for (al = 1, si = 1; al != 0x0a; al++, si++) {
        uint8_t level = SX8((uint16_t)(si + 0x504));

        if (level != 0xff)
            gmd_controller(al, (uint16_t)((7 << 8) | level));
    }

    return ax;
}

/*
 * SX.OVL GMD:0x0918  - function 17
 *
 * Read back what the driver last stored for a channel: the pitch bend for a
 * 0xE0 query, the program for 0xC0, and for 0xB0 one of the four controllers
 * it keeps. Anything else answers 0xffff.
 *
 * The pitch-bend answer is the stored pair with its halves swapped and the
 * high bit of the low byte set from what falls out of the shift, which is the
 * inverse of how `gmd_pitch_bend` stored it.
 */
uint16_t gmd_query(uint16_t ax, uint16_t cx)
{
    uint8_t ah = (uint8_t)(ax >> 8), ch = (uint8_t)(cx >> 8);
    uint16_t si = (uint16_t)(ax & 0xf);

    if (ah == 0xe0) {
        uint16_t v;

        si *= 2;
        v = (uint16_t)SX8((uint16_t)(si + 0x4d4))
            | (uint16_t)(SX8((uint16_t)(si + 0x4d5)) << 8);
        if (v == 0xffff)
            return v;

        v = (uint16_t)((v >> 8) | (v << 8));    /* xchg al, ah */
        {
            uint8_t al = (uint8_t)v, hi = (uint8_t)(v >> 8);
            uint8_t carry = (uint8_t)(hi & 1);

            hi >>= 1;
            if (carry)
                al |= 0x80;
            return (uint16_t)((hi << 8) | al);
        }
    }

    if (ah == 0xc0)
        return SX8((uint16_t)(si + 0x4c4));

    if (ah == 0xb0) {
        if (ch == 0x4b)
            return 0xffff;
        if (ch == 1)
            return SX8((uint16_t)(si + 0x4f4));
        if (ch == 7)
            return SX8((uint16_t)(si + 0x504));
        if (ch == 0x0a)
            return SX8((uint16_t)(si + 0x514));
        if (ch == 0x40)
            return SX8((uint16_t)(si + 0x524));
    }

    return 0xffff;
}

/*
 * SX.OVL GMD:0x0a08  - function 0
 *
 * Describe the driver: AX = 0x0104 and CX = 0x0720. `install_driver` keeps CL
 * and CH and the top nibble of AH, so what this says is that the driver is
 * kind 1, revision 4, with 0x20 of something and 7 of something else. The
 * speaker answers 1, 0x12 and 0 to the same question.
 */
void gmd_describe_0(uint16_t *ax, uint16_t *cx)
{
    *ax = 0x0104;
    *cx = 0x0720;
}

/*
 * SX.OVL GMD:0x0984  - function 1
 *
 * Initialise, from a **patch bank**: reset the interface, put it in UART mode
 * three times over with a settling delay after each, and then copy 0x481 bytes
 * from `ES:AX` into the driver's own segment at `cs:0x41` - which is every
 * table the note routines read, the percussion map and the velocity curves
 * included.
 *
 * After the blob comes a **stored MIDI sequence**: a 16-bit count and then
 * that many bytes, sent as data. A 0xF7 - the end of a system-exclusive - gets
 * two settling delays after it, because a synthesiser asked to swallow a SysEx
 * needs the time.
 *
 * Then the master volume is set to 12 and the answer is AX = 0x984, CX =
 * 0x0801. `configure_driver` keeps CL and CH; the AX it returns is only tested
 * against 0xffff, so what matters is that it is not that.
 *
 * **The `ES:AX` this reads is what `configure_driver_far` was handed** and
 * `configure_driver` passes on. For the speaker driver function 1 ignores it
 * and the port used to drop it on that evidence; this driver is why it cannot.
 */
void gmd_init(uint16_t off, uint16_t seg, uint16_t *ax, uint16_t *cx)
{
    const uint8_t *src = (const uint8_t *)FAR_PTR(seg, off);
    uint16_t di, n;

    gmd_reset();

    gmd_write_command(0x3f);
    gmd_delay();
    gmd_write_command(0x3f);
    gmd_delay();
    gmd_write_command(0x3f);
    gmd_delay();
    gmd_delay();

    for (di = 0; di != 0x481; di++)
        SX8((uint16_t)(di + 0x41)) = src[di];

    SX8(0x55d) = SX8(0x241);

    n = (uint16_t)(src[0x481] | (src[0x482] << 8));
    {
        uint16_t i = 0x483;

        while (n != 0) {
            uint8_t b = src[i++];

            gmd_write_data(b);
            if (b == 0xf7) {
                gmd_delay();
                gmd_delay();
            }
            n--;
        }
    }

    gmd_param_345(0x0c);

    *ax = 0x984;
    *cx = 0x0801;
}
