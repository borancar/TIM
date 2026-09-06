/* The OPL3 behind opl.h, over ymfm. See opl.h for why this is not
 * transcribed, and ../vendor/README.md for what ymfm is and its licence.
 *
 * This is the ONLY C++ in the reconstruction, and it is here only because
 * ymfm is C++. It holds no game logic and no decisions - every line is
 * plumbing between opl.h's C and ymfm's classes.
 */
#include "opl.h"

#include "../vendor/ymfm/ymfm_opl.h"

namespace {

/* ymfm calls back for timers, IRQs and external memory. An AdLib card has no
 * IRQ line at all - which is exactly why adlib.dat hooks no vector and must
 * be called by the game (docs/sound.md) - and a YMF262 has no external
 * memory, so every default here is the right answer. */
struct adlib_interface : public ymfm::ymfm_interface
{
};

adlib_interface g_intf;

/*
 * ONE YMF262 - an **OPL3**, which is what DOSBox emulates for a Sound Blaster
 * 16 and therefore what the reference captures were made on.
 *
 * An OPL3 is an OPL2 until something sets the NEW bit in register 0x105, and
 * nothing in this game ever does: `ADL:` is an AdLib driver and does not know
 * the register exists. ymfm models that faithfully - with NEW clear,
 * `op_waveform` masks to two bits, so four waveforms and not eight, and
 * `ch_output_0` and `ch_output_1` both return 1 unconditionally, so a driver
 * that never writes the C0 stereo bits is still heard from both channels
 * rather than from neither. The register stream the game produces is
 * therefore unchanged, and so is what it means.
 *
 * **The two addresses are one chip's two banks, not two chips.** 0x220/0x221
 * and 0x388/0x389 are bank 0; 0x222/0x223 is bank 1, which on an OPL3 is
 * channels 9..17 - nine *more* voices, not a second copy of the first nine.
 *
 * That is a real difference from a Sound Blaster Pro 1.0, which has two
 * YM3812s and answers 0x222 with a second chip carrying the same nine voices
 * at different levels. `SX.OVL`'s `SBP:` driver is written for that card - see
 * sxovl_sbp.c's `sbp_write_level`, which pans by level difference - so on an
 * OPL3 its right-hand level writes land on channels that are not sounding and
 * its panning does nothing. **That is what the driver would do on real OPL3
 * hardware too**, and it is not a defect here; the port is now an SB16 rather
 * than a Pro 1.0. It costs nothing in practice because the game cannot select
 * `SBP:` at all - `load_sound_bank` has no case for device 4.
 */
ymfm::ymf262    g_chip(g_intf);
uint32_t        g_writes;
opl_trace_fn    g_trace;

/* THE OUTPUT CAPACITOR.
 *
 * An OPL2's three added waveforms are RECTIFIED - half-sine, absolute sine
 * and quarter-pulse - so every voice using one carries a positive offset
 * proportional to its envelope. The offsets of nine channels swell and decay
 * together and the sum wanders at a few hertz.
 *
 * A real AdLib never emits that: the YM3014B feeds an op-amp through a
 * coupling capacitor, and the card's output is AC. ymfm models the CHIP, so
 * it hands back the raw sum, offset and all - correctly.
 *
 * Measured against DOSBox playing the same level: 43.3% of the port's total
 * power was in 0-4 Hz where DOSBox had 0.0%, and the music was left riding up
 * and down on it at a quarter of DOSBox's RMS. Inaudible on its own, and it
 * eats the headroom the music needed.
 *
 * One pole, about 5 Hz, which is where a coupling capacitor of that era sits.
 * It is NOT a tone control and must not become one: everything above 20 Hz is
 * untouched to within a hundredth of a decibel. */
const double DC_R = 1.0 - 6.2831853 * 5.0 / (double)OPL_SAMPLE_RATE;
double g_dc_x1[2], g_dc_y1[2];   /* one pole per output channel */

/* THE SETTLING TIME, and the samples it produces.
 *
 * A register write costs the chip OPL_WRITE_SETTLE_US of running - see
 * opl.h - and those are real samples, not a pause. They are generated here,
 * when the write happens, and handed out by opl_render before anything new:
 * discarding them would throw away a sixth of the audio at a busy tick. */
const uint32_t SETTLE_MAX = 8192;
int16_t  g_settle[2][SETTLE_MAX];
uint32_t g_settle_head, g_settle_tail;
double   g_settle_owed;

/*
 * One queue index for the pair of channels: the chip generates both at once,
 * so they cannot drift, and the settling time is the driver waiting out a
 * write whichever bank it addressed.
 */
inline uint32_t settle_count()
{
    return (g_settle_tail - g_settle_head) & (SETTLE_MAX - 1);
}

}  /* namespace */

extern "C" void opl_reset(void)
{
    g_chip.reset();
    g_writes = 0;
    g_dc_x1[0] = g_dc_y1[0] = 0.0;
    g_dc_x1[1] = g_dc_y1[1] = 0.0;
    g_settle_head = g_settle_tail = 0;
    g_settle_owed = 0.0;
}

namespace {
/*
 * One frame from the chip, each channel through its own coupling capacitor.
 *
 * ymfm gives an OPL3 four outputs; the first two are the YMF262's left and
 * right, and the other two exist only on an OPL4 and are always silent here.
 */
void one_frame(int16_t *l, int16_t *r)
{
    ymfm::ymf262::output_data frame;
    int16_t *out[2] = { l, r };
    uint32_t c;

    g_chip.generate(&frame, 1);

    for (c = 0; c < 2; c++) {
        /*
         * HALVED, because ymfm's two chips do not share a full scale.
         *
         *   ym3812: m_fm.output(..., 1, 32767, ...) then roundtrip_fp()
         *   ymf262: m_fm.output(..., 0, 32767, ...) then clamp16()
         *
         * The 1 is a right shift, so the OPL2 path is exactly half the OPL3's
         * before either DAC is modelled - the YM3014's companding round trip
         * against the YAC512's linear 16 bits. ymfm's own comments say the
         * mixing details of both "need verification", so this is not a
         * quantity to take on faith either way.
         *
         * Measured on 7,090 of the game's own register writes: the OPL3 came
         * out 1.76x the OPL2's rms and peaked at the clamp, and halving brings
         * it to 0.878 of the OPL2 - the rest being the YM3014 truncation the
         * OPL2 path applies and this one does not.
         *
         * Undoing it here rather than in `FM_GAIN` keeps that constant meaning
         * what it says it means: a judgement about FM against the digitised
         * stream, not a correction for a DAC.
         */
        double x = (double)frame.data[c] / 2.0;
        double y = x - g_dc_x1[c] + DC_R * g_dc_y1[c];
        int32_t v;

        g_dc_x1[c] = x;
        g_dc_y1[c] = y;
        v = (int32_t)y;
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        *out[c] = (int16_t)v;
    }
}

/* The chip runs for as long as the driver waited out the write. */
void settle_after_write()
{
    g_settle_owed += OPL_WRITE_SETTLE_US * OPL_SAMPLE_RATE / 1000000.0;
    while (g_settle_owed >= 1.0) {
        g_settle_owed -= 1.0;
        uint32_t next = (g_settle_tail + 1) & (SETTLE_MAX - 1);
        if (next == g_settle_head) break;     /* full: the render is behind */
        one_frame(&g_settle[0][g_settle_tail], &g_settle[1][g_settle_tail]);
        g_settle_tail = next;
    }
}
}

extern "C" void opl_set_trace(opl_trace_fn fn)
{
    g_trace = fn;
}

/* The status port. ymfm keeps the real one - the timer bits a driver's
 * detection programs and then reads back - so this asks the chip rather than
 * answering a constant. */
extern "C" uint8_t opl_status(void)
{
    return g_chip.read_status();
}

/*
 * A write through the AdLib address, 0x388 - which is bank 0, the same nine
 * channels 0x220 reaches. `ADL:` uses nothing else, so on an OPL3 the whole
 * game lives in bank 0 and the chip stays in its OPL2-compatible mode.
 */
extern "C" void opl_write(uint8_t reg, uint8_t val)
{
    if (g_trace) g_trace(0, reg, val);
    /* offset 0 is the address port (0x388), offset 1 the data port (0x389) */
    g_chip.write(0, reg);
    g_chip.write(1, val);
    g_writes++;

    /* AND THE CHIP RUNS WHILE THE DRIVER WAITS OUT THE WRITE - see opl.h.
     * Without this every register write of a tick lands at one instant and
     * the music comes out hollow and half as loud. */
    settle_after_write();
}

/*
 * A write to one register bank - 0x220 is bank 0, 0x222 bank 1. ymfm's offsets
 * are the hardware's: 0 and 1 address and data for the first bank, 2 and 3 for
 * the second.
 */
extern "C" void opl_write_bank(uint8_t bank, uint8_t reg, uint8_t val)
{
    uint32_t base = (bank == 0) ? 0 : 2;

    if (g_trace) g_trace(bank, reg, val);
    g_chip.write(base + 0, reg);
    g_chip.write(base + 1, val);
    g_writes++;

    settle_after_write();
}

extern "C" void opl_render_stereo(int16_t *out, uint32_t frames)
{
    uint32_t i = 0;

    /* whatever the writes already produced, in the order they produced it */
    while (i < frames && g_settle_head != g_settle_tail) {
        out[i * 2]     = g_settle[0][g_settle_head];
        out[i * 2 + 1] = g_settle[1][g_settle_head];
        g_settle_head = (g_settle_head + 1) & (SETTLE_MAX - 1);
        i++;
    }
    for (; i < frames; i++)
        one_frame(&out[i * 2], &out[i * 2 + 1]);
}

/* The mono mix, for the file writer, which wants one channel. */
extern "C" void opl_render(int16_t *out, uint32_t frames)
{
    uint32_t i = 0;

    while (i < frames && g_settle_head != g_settle_tail) {
        out[i++] = (int16_t)((g_settle[0][g_settle_head]
                              + g_settle[1][g_settle_head]) / 2);
        g_settle_head = (g_settle_head + 1) & (SETTLE_MAX - 1);
    }
    for (; i < frames; i++) {
        int16_t l, r;

        one_frame(&l, &r);
        out[i] = (int16_t)(((int32_t)l + r) / 2);
    }
}

extern "C" uint32_t opl_writes(void)
{
    return g_writes;
}
