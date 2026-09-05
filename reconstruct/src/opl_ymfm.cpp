/* The OPL2 behind opl.h, over ymfm. See opl.h for why this is not
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
 * be called by the game (docs/sound.md) - and a YM3812 has no external
 * memory, so every default here is the right answer. */
struct adlib_interface : public ymfm::ymfm_interface
{
};

adlib_interface g_intf;

/*
 * TWO CHIPS, because a Sound Blaster Pro 1.0 has two YM3812s - one per
 * speaker - and `SX.OVL`'s `SBP:` driver pans by writing them *different
 * levels* rather than by any stereo bit. See sxovl_sbp.c's `sbp_write_level`.
 *
 * An AdLib has one. It needs no mode here: `ADL:` reaches the port only
 * through 0x388, which writes both chips, so the two produce identical output
 * and the stereo pair is the mono signal. The second chip costs the time it
 * takes to render and decides nothing.
 */
ymfm::ym3812    g_chip0(g_intf);
ymfm::ym3812    g_chip1(g_intf);
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
double g_dc_x1[2], g_dc_y1[2];

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
 * ONE QUEUE INDEX FOR BOTH CHIPS, and that is the point rather than an economy.
 * The settling time is the *driver* waiting out a write, so both chips run
 * through it whichever one was written. Advancing only the chip that was
 * addressed would let the two drift apart, and a stereo pair whose halves are
 * a different age is not a stereo pair.
 */
inline uint32_t settle_count()
{
    return (g_settle_tail - g_settle_head) & (SETTLE_MAX - 1);
}

}  /* namespace */

extern "C" void opl_reset(void)
{
    g_chip0.reset();
    g_chip1.reset();
    g_writes = 0;
    g_dc_x1[0] = g_dc_y1[0] = 0.0;
    g_dc_x1[1] = g_dc_y1[1] = 0.0;
    g_settle_head = g_settle_tail = 0;
    g_settle_owed = 0.0;
}

namespace {
/* One sample from one chip, through its own coupling capacitor. */
int16_t one_sample(uint32_t c)
{
    ymfm::ym3812::output_data frame;
    (c == 0 ? g_chip0 : g_chip1).generate(&frame, 1);
    double x = (double)frame.data[0];
    double y = x - g_dc_x1[c] + DC_R * g_dc_y1[c];
    g_dc_x1[c] = x;
    g_dc_y1[c] = y;
    int32_t v = (int32_t)y;
    if (v >  32767) v =  32767;
    if (v < -32768) v = -32768;
    return (int16_t)v;
}

/* Both chips run for as long as the driver waited out the write. */
void settle_after_write()
{
    g_settle_owed += OPL_WRITE_SETTLE_US * OPL_SAMPLE_RATE / 1000000.0;
    while (g_settle_owed >= 1.0) {
        g_settle_owed -= 1.0;
        uint32_t next = (g_settle_tail + 1) & (SETTLE_MAX - 1);
        if (next == g_settle_head) break;     /* full: the render is behind */
        g_settle[0][g_settle_tail] = one_sample(0);
        g_settle[1][g_settle_tail] = one_sample(1);
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
    return g_chip0.read_status();
}

/*
 * A write both chips see - port 0x388, which on a Sound Blaster Pro 1.0 is
 * wired to both YM3812s so that an AdLib-only program is heard from both
 * speakers. `SBP:` sends everything but the levels this way.
 */
extern "C" void opl_write(uint8_t reg, uint8_t val)
{
    if (g_trace) g_trace(0, reg, val);
    /* offset 0 is the address port (0x388), offset 1 the data port (0x389) */
    g_chip0.write(0, reg);
    g_chip0.write(1, val);
    g_chip1.write(0, reg);
    g_chip1.write(1, val);
    g_writes++;

    /* AND THE CHIP RUNS WHILE THE DRIVER WAITS OUT THE WRITE - see opl.h.
     * Without this every register write of a tick lands at one instant and
     * the music comes out hollow and half as loud. */
    settle_after_write();
}

/* A write one chip sees - 0x220 for the left, 0x222 for the right. */
extern "C" void opl_write_chip(uint8_t chip, uint8_t reg, uint8_t val)
{
    ymfm::ym3812 &c = (chip == 0) ? g_chip0 : g_chip1;

    if (g_trace) g_trace(chip, reg, val);
    c.write(0, reg);
    c.write(1, val);
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
    for (; i < frames; i++) {
        out[i * 2]     = one_sample(0);
        out[i * 2 + 1] = one_sample(1);
    }
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
    for (; i < frames; i++)
        out[i] = (int16_t)((one_sample(0) + one_sample(1)) / 2);
}

extern "C" uint32_t opl_writes(void)
{
    return g_writes;
}
