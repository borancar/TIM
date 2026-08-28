/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * This file corresponds to the original's **code segment 2619**, image
 * 0x26190..0x2a040. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include "tim.h"
#include "io.h"
#include "dgroup.h"

/*
 * NOT a transcription of a routine of its own: the block of driver calls that
 * 0x26f2a contains **twice**, at 0x275a7 and again at 0x2772e, byte for byte.
 * Factored out so the difference between the two paths that use it - which is
 * only how they arrive - stays visible.
 *
 * `voice` is the driver's idea of a channel and `channel` the sequence's, and
 * the two are not the same number. Everything read out of the sequence is
 * indexed by `channel`; everything told to the driver is addressed to `voice`.
 */
static void tick_program_voice(uint16_t es, uint16_t bx, uint16_t voice,
                               uint16_t channel)
{
    uint8_t cl, ch;
    uint16_t bend;

    sx_controller(voice, 0x7b00);                    /* all notes off */

    cl = (uint8_t)(*FAR_PTR(es, (uint16_t)(bx + channel + 0xda)) & 0xf);
    sx_controller(voice, (uint16_t)((0x4b << 8) | cl));

    /* Entry 8 of the driver's table is the do-nothing stub. */
    sx_nop();

    SND8(0x1c8 + voice) = 0xff;

    cl = scale_byte_pair(*FAR_PTR(es, (uint16_t)(bx + channel + 0x107)),
                         *FAR_PTR(es, (uint16_t)(bx + 0x15e)));
    sx_controller(voice, (uint16_t)((7 << 8) | cl));

    cl = *FAR_PTR(es, (uint16_t)(bx + channel + 0xf8));
    sx_controller(voice, (uint16_t)((0xa << 8) | cl));

    cl = *FAR_PTR(es, (uint16_t)(bx + channel + 0xe9));
    sx_controller(voice, (uint16_t)((1 << 8) | cl));

    cl = 0;
    if (*FAR_PTR(es, (uint16_t)(bx + 2 * channel + 0xbd)) >= 0x80)
        cl = 0x7f;
    sx_controller(voice, (uint16_t)((0x40 << 8) | cl));

    bend = *(uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * channel + 0xbc));
    ch = (uint8_t)bend;
    cl = (uint8_t)((bend >> 8) << 1);
    if (ch >= 0x80)
        cl |= 1;
    sx_pitch_bend(voice, (uint16_t)((((uint16_t)ch << 8) | cl) & 0x7f7f));

    cl = *FAR_PTR(es, (uint16_t)(bx + channel + 0x125));
    sx_controller(voice, (uint16_t)((0x4e << 8) | cl));
}

/*
 * NOT a transcription of its own routine either: the four sixteen-byte arrays
 * that 0x26f2a snapshots before it tries to place a sequence's channels, and
 * puts back if the placement fails. The original writes both copies out
 * unrolled, eight words at a time; they are loops here.
 */
static void tick_save_state(void)
{
    int16_t i;

    for (i = 0; i < 0x10; i++) {
        SND8(0x1a8 + i) = SND8(0x168 + i);
        SND8(0x188 + i) = SND8(0x148 + i);
        SND8(0x198 + i) = SND8(0x158 + i);
        SND8(0x178 + i) = SND8(0x138 + i);
    }
}

/*
 * NOT a transcription either: the other half of the pair above, putting back
 * what a failed placement changed.
 */
static void tick_restore_state(void)
{
    int16_t i;

    for (i = 0; i < 0x10; i++) {
        SND8(0x168 + i) = SND8(0x1a8 + i);
        SND8(0x148 + i) = SND8(0x188 + i);
        SND8(0x158 + i) = SND8(0x198 + i);
        SND8(0x138 + i) = SND8(0x178 + i);
    }
}

/*
 * 0x26f2a
 *
 * The sequencer's tick: decide which of sixteen hardware voices plays each
 * channel of each playing sequence, and tell the driver about everything that
 * changed. 2494 bytes of hand-written assembly, the largest routine in the
 * game, called from the timer interrupt.
 *
 * It works on five sixteen-byte arrays in the module's own code segment:
 *
 *   0x128  the voice assignment now in force, 0xff for a free voice
 *   0x168  the assignment being *requested* this tick, 0xff for none
 *   0x148  what the request costs
 *   0x158  what it contributes back if it is dropped
 *   0x138  a flag saying the request must keep its own voice number
 *
 * A request is a byte packing the sequence in the high nibble and the channel
 * in the low, so the sixteen sequences are reached as `cs:[8 + 4 * sequence]`
 * and a request byte can be turned back into both halves.
 *
 * Placement runs per sequence, over that sequence's sixteen channels. Channels
 * marked 0xff, 0xfe or 0x0f in the map at `+0x8c` are skipped, and so are those
 * whose flags at `+0x134` have bit 1 or whose byte at `+0x143` is set. What is
 * left needs a voice within the range `cs:0x1fa`..`cs:0x1fb`.
 *
 * When no voice is free, the loudest already-placed request is dropped and its
 * contribution added to a running total, repeatedly, until the total covers
 * what this channel needs - so quiet requests are given up before loud ones,
 * and only as many as are actually needed. If that still is not enough the
 * whole sequence is abandoned and `tick_restore_state` puts back everything the
 * attempt changed, which is why the snapshot is taken per sequence rather than
 * once.
 *
 * A channel whose `+0x134` has bit 0 must have the voice matching its own
 * number. If that voice went to another channel the two are swapped outright;
 * if the swap is not possible the request is dropped instead.
 *
 * Then every voice whose request differs from what it is playing is
 * reprogrammed - `tick_program_voice` - and voices that were playing and are
 * not wanted are silenced. The two remaining passes hand out any voice still
 * unclaimed and then rebuild the per-voice sequence pointers at `cs:0x88`.
 *
 * `cs:0x1f9` is incremented on the way in and decremented on the way out. It is
 * a depth count, not a lock: nothing here tests it, and it is `0x27ace` - the
 * entry the interrupt actually calls - that refuses to run when it is set.
 */
void sequencer_tick(void)
{
    int16_t i, seq, voice, ch_i;
    uint16_t es, bx, bp_;
    uint8_t al, ah, cl, chh, dl, dh;

    SND8(0x1f9)++;
    SND8(0x204) = 0;

    for (i = 0; i < 0x10; i++) {
        SND8(0x128 + i) = 0xff;
        SND8(0x158 + i) = 0;
        SND8(0x138 + i) = 0;
        SND8(0x148 + i) = 0;
        SND8(0x168 + i) = 0xff;
    }
    SND16(0x48) = 0;
    SND16(0x4a) = 0;

    bx = (uint16_t)SND16(8);
    es = (uint16_t)SND16(0xa);

    if (es == 0 && bx == 0) {
        for (i = 0; i < 0x10; i++)
            SND8(0x128 + i) = 0xff;
        goto silence_unused;
    }

    cl = *FAR_PTR(es, (uint16_t)(bx + 0x15f));
    if (cl == 0x7f)
        cl = SND8(0x202);
    sx_param_349(cl);

    al = SND8(0x1ff);

    bp_ = 0;
    for (seq = 0; seq < 0x40; seq += 4) {
        bx = (uint16_t)SND16(8 + seq);
        es = (uint16_t)SND16(0xa + seq);
        if (es == 0 && bx == 0)
            break;

        if (*FAR_PTR(es, (uint16_t)(bx + 0x164)) != 0)
            goto next_sequence;

        if (*FAR_PTR(es, (uint16_t)(bx + 0x165)) != 0) {
            if (SND16(0x48) != 0 || SND16(0x4a) != 0)
                goto next_sequence;
            SND16(0x48) = (int16_t)bx;
            SND16(0x4a) = (int16_t)es;
            goto next_sequence;
        }

        tick_save_state();
        /*
         * The running total carried across sequences is parked here, not
         * zeroed: the abandon path below reads it back so a sequence that
         * fails leaves the total exactly as it found it.
         */
        SND8(0x203) = al;

        for (ch_i = 0; ch_i < 0x10; ch_i++) {
            cl = *FAR_PTR(es, (uint16_t)(bx + ch_i + 0x8c));
            if (cl == 0xff || cl == 0xfe || cl == 0x0f)
                continue;
            if ((*FAR_PTR(es, (uint16_t)(bx + cl + 0x134)) & 2) != 0)
                continue;
            if (*FAR_PTR(es, (uint16_t)(bx + cl + 0x143)) != 0)
                continue;

            dl = (uint8_t)((seq * 4) | cl);

            ah = (uint8_t)(*FAR_PTR(es, (uint16_t)(bx + cl + 0xda)) & 0xf);
            chh = (uint8_t)(*FAR_PTR(es, (uint16_t)(bx + cl + 0xda)) >> 4);
            if (chh != 0)
                chh = (uint8_t)(0x10 - chh + bp_);

            if ((*FAR_PTR(es, (uint16_t)(bx + cl + 0x134)) & 1) != 0
                && SND8(0x168 + cl) == 0xff) {
                dh = cl;
                goto have_voice;
            }

            dh = 0xff;
            {
                int16_t bl;

                for (bl = 0; bl < 0x10; bl++) {
                    if (SND8(0x168 + bl) == 0xff) {
                        if (bl >= (int16_t)SND8(0x1fa)
                            && bl <= (int16_t)SND8(0x1fb))
                            dh = (uint8_t)bl;
                    } else if (SND8(0x168 + bl) == dl) {
                        goto next_channel;
                    }
                }
            }
            if (dh != 0xff)
                goto have_voice;

            if (chh != 0)
                goto next_sequence;

            for (;;) {
                int16_t best = -1;
                uint8_t most = 0;

                for (i = 0; i < 0x10; i++) {
                    if (most < SND8(0x148 + i)) {
                        most = SND8(0x148 + i);
                        best = i;
                    }
                }
                if (best >= 0) {
                    al = (uint8_t)(al + SND8(0x158 + best));
                    SND8(0x168 + best) = 0xff;
                    SND8(0x158 + best) = 0;
                    SND8(0x148 + best) = 0;
                    SND8(0x138 + best) = 0;
                } else {
                    goto abandon_sequence;
                }
                if (ah <= al)
                    break;
            }

have_voice:
            SND8(0x168 + dh) = dl;
            SND8(0x158 + dh) = ah;
            al = (uint8_t)(al - ah);
            SND8(0x148 + dh) = chh;

            if ((*FAR_PTR(es, (uint16_t)(bx + cl + 0x134)) & 1) == 0) {
                SND8(0x138 + dh) = 0;
                continue;
            }

            SND8(0x138 + dh) = 1;
            if (dh == cl)
                continue;

            if (SND8(0x138 + cl) == 0) {
                uint8_t t;

                t = SND8(0x168 + dh);
                SND8(0x168 + dh) = SND8(0x168 + cl);
                SND8(0x168 + cl) = t;
                t = SND8(0x148 + dh);
                SND8(0x148 + dh) = SND8(0x148 + cl);
                SND8(0x148 + cl) = t;
                t = SND8(0x158 + dh);
                SND8(0x158 + dh) = SND8(0x158 + cl);
                SND8(0x158 + cl) = t;
                t = SND8(0x138 + dh);
                SND8(0x138 + dh) = SND8(0x138 + cl);
                SND8(0x138 + cl) = t;
                continue;
            }

            if (chh != 0) {
                SND8(0x168 + dh) = 0xff;
                SND8(0x148 + dh) = 0;
                SND8(0x158 + dh) = 0;
                SND8(0x138 + dh) = 0;
                al = (uint8_t)(al + ah);
                continue;
            }

            if (SND8(0x148 + cl) != 0)
                goto abandon_sequence;

            al = (uint8_t)(al + SND8(0x158 + cl));
            SND8(0x168 + dh) = 0xff;
            SND8(0x158 + dh) = 0;
            SND8(0x148 + dh) = 0;
            SND8(0x138 + dh) = 0;
            SND8(0x168 + cl) = dl;
            SND8(0x148 + cl) = chh;
            SND8(0x158 + cl) = ah;
            al = (uint8_t)(al - ah);

next_channel:
            ;
        }
        goto next_sequence;

abandon_sequence:
        tick_restore_state();
        al = SND8(0x203);

next_sequence:
        bp_ = (uint16_t)(bp_ + 0x10);
    }

    /* Apply: reprogram every voice whose request differs from what it plays. */
    for (voice = 0; voice < 0x10; voice++) {
        if (SND8(0x168 + voice) == 0xff)
            continue;

        if (SND8(0x138 + voice) == 0) {
            uint8_t want = SND8(0x168 + voice);
            uint16_t sbx, ses;
            int16_t d;

            al = (uint8_t)(want & 0xf);
            sbx = (uint16_t)SND16(8 + ((want & 0xf0) >> 2));
            ses = (uint16_t)SND16(0xa + ((want & 0xf0) >> 2));

            d = SND8(0x1fa);
            for (;;) {
                if ((uint16_t)SND16(0x88 + 4 * d) == sbx
                    && (uint16_t)SND16(0x8a + 4 * d) == ses
                    && SND8(0x1b8 + d) == al) {
                    if (SND8(0x138 + d) == 0) {
                        SND8(0x128 + d) = SND8(0x168 + voice);
                        SND8(0x168 + voice) = 0xff;
                    }
                    break;
                }
                d++;
                if ((int16_t)SND8(0x1fb) < d - 1)
                    break;
            }
            continue;
        }

        {
            uint8_t want = SND8(0x168 + voice);
            uint16_t sbx, ses;

            SND8(0x168 + voice) = 0xff;
            SND8(0x128 + voice) = want;

            sbx = (uint16_t)SND16(8 + ((want & 0xf0) >> 2));
            ses = (uint16_t)SND16(0xa + ((want & 0xf0) >> 2));
            al = (uint8_t)(want & 0xf);

            if (SND8(0x1b8 + voice) == al
                && (uint16_t)SND16(0x88 + 4 * voice) == sbx
                && (uint16_t)SND16(0x8a + 4 * voice) == ses)
                continue;

            tick_program_voice(ses, sbx, (uint16_t)voice, al);
        }
    }

    /* Hand out anything still requested to a voice that is still free. */
    {
        int16_t free_from = (int16_t)(uint8_t)(SND8(0x1fb) + 1);

        for (voice = 0; voice < 0x10; voice++) {
            uint8_t want = SND8(0x168 + voice);
            uint16_t sbx, ses;
            int16_t d;

            if (want == 0xff)
                continue;

            d = free_from;
            do {
                d--;
            } while (SND8(0x128 + d) != 0xff);
            free_from = d;

            SND8(0x128 + d) = want;
            al = (uint8_t)(want & 0xf);
            sbx = (uint16_t)SND16(8 + ((want & 0xf0) >> 2));
            ses = (uint16_t)SND16(0xa + ((want & 0xf0) >> 2));

            tick_program_voice(ses, sbx, (uint16_t)d, al);
        }
    }

silence_unused:
    for (voice = 0xf; voice >= 0; voice--) {
        if (SND8(0x1b8 + voice) == 0xf)
            continue;
        if (SND8(0x128 + voice) != 0xff)
            continue;
        sx_controller((uint16_t)voice, 0x4000);
        sx_controller((uint16_t)voice, 0x7b00);
        sx_controller((uint16_t)voice, 0x4b00);
    }

    for (i = 0; i < 0x10; i += 2)
        SND16(0x1b8 + i) = (int16_t)(SND16(0x128 + i) & 0x0f0f);

    for (voice = 0; voice < 0x10; voice++) {
        uint8_t held = SND8(0x128 + voice);

        if (held == 0xff) {
            SND16(0x88 + 4 * voice) = 0;
            SND16(0x8a + 4 * voice) = 0;
        } else {
            SND16(0x88 + 4 * voice) = SND16(8 + ((held & 0xf0) >> 2));
            SND16(0x8a + 4 * voice) = SND16(0xa + ((held & 0xf0) >> 2));
        }
    }

    SND8(0x1f9)--;
}

/*
 * 0x27a86
 *
 * Flush up to two pending volume changes to the driver, round-robin over the
 * sixteen channels, and remember where to resume.
 *
 * The array at the module's own `cs:0x1c8` holds one byte per channel, with
 * 0xff meaning nothing pending. A channel with anything else is marked 0xff
 * again and its value sent to the driver as MIDI controller 7 - volume - on
 * that channel.
 *
 * **At most two per call.** The scan then stops wherever it is, and `cs:0x206`
 * carries that position into the next call, so sixteen channels are serviced
 * over eight calls rather than all at once. This runs from the timer tick, and
 * sending sixteen controller changes inside one interrupt would be the thing
 * it is avoiding.
 *
 * The scan is also bounded by returning to where it started, so a pass with
 * nothing pending walks the ring once and stops rather than spinning.
 *
 * The original reaches the driver by a far call through `cs:[0x1e7]` with the
 * function number in BP; 7 selects `sx_controller`, which is what the port
 * calls directly. The number is fixed for this driver, not looked up.
 *
 * This is hand-written assembly - no frame, no arguments, a near `ret`.
 */
void flush_pending_volumes(void)
{
    uint16_t si = SND8(0x206);
    int16_t sent = 0;

    for (;;) {
        uint8_t pending = SND8(0x1c8 + si);

        if (pending != 0xff) {
            SND8(0x1c8 + si) = 0xff;
            sx_controller(si, (uint16_t)((7 << 8) | pending));
            sent++;
            if (sent == 2)
                break;
        }

        si++;
        if (si == 0x10)
            si = 0;
        if (si == SND8(0x206))
            break;
    }

    SND8(0x206) = (uint8_t)si;
}

/*
 * 0x27ee1
 *
 * Handle one MIDI note event out of a sequence, and answer the stream cursor
 * advanced past it.
 *
 * Hand-written assembly taking everything in registers: `ds:bp` is the cursor
 * into the note stream, `es:bx` the sequence's record, `si` the raw channel and
 * `al` the channel the driver should be told about - or 0xff for a channel that
 * is not being played.
 *
 * Two bytes are consumed, the note then the velocity, and the word counter at
 * `+0xc + 2 * si` is bumped once for **each byte**, not once for the event.
 * Those two increments use the raw channel; everything after uses the mapped
 * one, read from the byte table at `+0x8c` and masked to four bits. The two
 * indices are easy to conflate and are not the same.
 *
 * A non-zero velocity is a note on: the note is recorded at `+0x125 + channel`
 * and the driver told to start it. A zero velocity is a note off - the MIDI
 * convention, rather than a separate message - and it clears `+0x125` **only if
 * the note there is the one being released**, so a channel that has already
 * been given a different note is left alone.
 *
 * The record is updated either way. Only the call to the driver is skipped when
 * the channel is 0xff or the flag at `cs:0x209` is set, so muting stops the
 * sound without letting the sequence's own state drift.
 *
 * The driver is reached through `cs:[0x1e7]` with the function number in BP: 5
 * to start, 4 to stop, which are `sx_start_note` and `sx_stop_note`.
 */
uint16_t midi_note_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                         uint16_t si, uint16_t ax)
{
    uint8_t note, velocity, channel;
    uint16_t *counter = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * si + 0xc));

    note = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    velocity = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    channel = (uint8_t)(*FAR_PTR(es, (uint16_t)(bx + si + 0x8c)) & 0xf);

    if (velocity != 0) {
        *FAR_PTR(es, (uint16_t)(bx + channel + 0x125)) = note;

        if ((uint8_t)ax != 0xff && SND8(0x209) == 0)
            sx_start_note((uint16_t)(ax & 0xf), (uint16_t)(note << 8));
    } else {
        if (*FAR_PTR(es, (uint16_t)(bx + channel + 0x125)) == note)
            *FAR_PTR(es, (uint16_t)(bx + channel + 0x125)) = 0xff;

        if ((uint8_t)ax != 0xff && SND8(0x209) == 0)
            sx_stop_note((uint16_t)(note << 8));
    }

    return bp;
}

/*
 * 0x280fe
 *
 * Handle one pitch bend event out of a sequence, and answer the stream cursor
 * advanced past it. The register convention is `midi_note_event`'s at 0x27ee1,
 * and so is the byte accounting: two bytes consumed, the per-byte counter at
 * `+0xc + 2 * si` bumped twice with the **raw** channel, everything after using
 * the mapped one from `+0x8c`.
 *
 * Two bytes are read, and the pair is put together the way MIDI does it -
 * `(msb << 7) | lsb` - by rotating the low bit of the most significant byte
 * into the top of the least. The result goes to the sequence's own store at
 * `+0xbc + 2 * channel`.
 *
 * **Bit 15 of that word is sticky.** Before the new value is written, the old
 * one is tested and its top bit carried into the new; a 14-bit bend can never
 * set it, so nothing this routine writes will ever clear it once something else
 * has. It is a flag living in the spare bit of a value, not part of the bend.
 *
 * There is an extra gate this event has and the note event does not: with
 * `cs:0x1fe` non-zero, a channel whose byte at `cs:0x128` is not 0xff is
 * dropped entirely - after the two bytes have been consumed and counted, so the
 * stream stays in step either way.
 *
 * As before, only the call to the driver is skipped for an unplayed channel or
 * a set `cs:0x209`; the stored bend is updated regardless. The driver is
 * reached with function number 10, `sx_pitch_bend`, and gets the *original*
 * register pair rather than the assembled value - it does its own assembly.
 */
uint16_t midi_bend_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                         uint16_t si, uint16_t ax)
{
    uint8_t lsb, msb, channel;
    uint16_t *counter = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * si + 0xc));
    uint16_t value;
    uint16_t *slot;

    lsb = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    msb = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    if (SND8(0x1fe) != 0 && SND8(0x128 + (ax & 0xf)) != 0xff)
        return bp;

    channel = (uint8_t)(*FAR_PTR(es, (uint16_t)(bx + si + 0x8c)) & 0xf);

    value = (uint16_t)((((uint16_t)msb >> 1) << 8)
                       | (uint16_t)(lsb | ((msb & 1) ? 0x80 : 0)));

    slot = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * channel + 0xbc));
    if (*slot >= 0x8000)
        value |= 0x8000;
    *slot = value;

    if ((uint8_t)ax != 0xff && SND8(0x209) == 0)
        sx_pitch_bend((uint16_t)(ax & 0xf),
                      (uint16_t)(((uint16_t)lsb << 8) | msb));

    return bp;
}

/*
 * 0x28305
 *
 * Parse a sequence's device-specific parameter table once, and cache the result
 * in place. Hand-written assembly: `es:ax` is the record, and nothing is
 * returned.
 *
 * The table is reached by **two** far pointers - the one at the record's +8,
 * and then the one that points at. A record whose +8 is a null far pointer,
 * both halves 0xffff, is left alone.
 *
 * The cache is guarded by a three-byte signature, 0xfc 0xfd 0xfe at +0x21,
 * +0x22 and +0x23 of the table itself, and it is checked **backwards** - +0x23
 * first. Finding it means the work has already been done and the routine
 * returns. Writing it is the last thing that happens, so a parse interrupted
 * part way is redone rather than half-trusted.
 *
 * Sixteen words of scratch at the module's own `cs:0x108` are cleared, along
 * with a byte at `cs:0x20c` set to 0xff. A leading 0xf0 in the table supplies
 * that byte from the following one and skips eight bytes.
 *
 * What follows is a list of devices. Each is an identifier byte and then
 * six-byte entries ending at 0xff; the identifier is matched against
 * `cs:0x1fc`, and a device that does not match has its entries stepped over
 * six bytes at a time without being read. The matching device's entries each
 * contribute one word - the third and fourth bytes - to the scratch, in order.
 *
 * The sixteen words and the byte are then written **over the start of the
 * table**, at +0 to +0x20, and the signature after them. So the parsed form
 * replaces the source it was parsed from, which is why the signature has to be
 * checked before anything else: a second parse would read its own output.
 *
 * Nothing bounds the number of entries a device may have. Seventeen or more
 * would run the scratch index past its sixteen words and write into whatever
 * follows `cs:0x108`.
 */
void init_sequence_params(uint16_t es, uint16_t ax)
{
    uint8_t *slot = FAR_PTR(es, (uint16_t)(ax + 8));
    uint16_t seg1, off1, seg, off;
    uint8_t *tbl;
    uint16_t si;

    off1 = *(uint16_t *)slot;
    seg1 = *(uint16_t *)(slot + 2);
    if (off1 == 0xffff && seg1 == 0xffff)
        return;

    {
        uint8_t *via = FAR_PTR(seg1, off1);

        off = *(uint16_t *)via;
        seg = *(uint16_t *)(via + 2);
    }
    tbl = FAR_PTR(seg, off);

    if (tbl[0x23] == 0xfe && tbl[0x22] == 0xfd && tbl[0x21] == 0xfc)
        return;

    for (si = 0x20; si != 0;) {
        si -= 2;
        SND16(0x108 + si) = 0;
    }
    SND8(0x20c) = 0xff;

    {
        uint16_t bp = 0;

        if (tbl[bp] == 0xf0) {
            SND8(0x20c) = tbl[bp + 1];
            bp += 8;
        }

        si = 0;
        for (;;) {
            uint8_t id = tbl[bp];

            if (id == SND8(0x1fc)) {
                bp++;
                for (;;) {
                    uint8_t c = tbl[bp];

                    bp++;
                    if (c == 0xff)
                        break;
                    bp++;
                    SND16(0x108 + si) = *(int16_t *)(tbl + bp);
                    bp += 4;
                    si += 2;
                }
                break;
            }
            if (id == 0xff)
                break;

            bp++;
            for (;;) {
                uint8_t c = tbl[bp];

                bp++;
                if (c == 0xff)
                    break;
                bp += 5;
            }
        }

        for (si = 0; si != 0x20; si += 2)
            *(int16_t *)(tbl + si) = SND16(0x108 + si);
        tbl[0x20] = SND8(0x20c);
    }

    tbl[0x21] = 0xfc;
    tbl[0x22] = 0xfd;
    tbl[0x23] = 0xfe;
}

/*
 * 0x282cb
 *
 * Scale one byte by another and halve the range: `((cl+1) * (dl+1)) >> 8`,
 * doubled, then reduced by one unless it is already zero.
 *
 * A **near** routine that takes and answers CL, preserving AX around the
 * multiply with a push and a pop. `mul dl` is the 8-bit form, so the product
 * lands in AX and `shl ah,1` doubles its high byte - the >>8 and the doubling
 * are one step, not two.
 */
uint8_t scale_byte_pair(uint8_t cl, uint8_t dl)
{
    uint16_t product = (uint16_t)((uint8_t)(cl + 1) * (uint8_t)(dl + 1));
    uint8_t out = (uint8_t)(((product >> 8) & 0xFF) << 1);

    if (out != 0)
        out--;
    return out;
}
/*
 * 0x2891a
 *
 * Step a far pointer past one record: the record's length is the byte at
 * offset 1, and there is a two-byte header, so the next record is
 * `off + rec[1] + 2`.
 *
 * The original takes and returns a far pointer in DX:AX and leaves DX - the
 * segment - untouched, so only the offset moves. The port has no segments, so
 * the target and the offset are passed separately: `rec` is what the pointer
 * points at, `off` is the offset half that the arithmetic is done on.
 */
uint16_t advance_record(const uint8_t *rec, uint16_t off)
{
    return (uint16_t)(off + rec[1] + 2);
}

/*
 * 0x2907b
 *
 * Follow a chain of **far** pointers - offset at +0x172, segment at +0x174 -
 * for at most `count` links, stopping early on a null pointer.
 *
 * The original walks by overwriting its own stack arguments, and tests the
 * pointer for null by OR-ing the two halves together, which is how a far
 * pointer is compared with zero without two compares. It answers the pointer
 * it stopped on, in DX:AX.
 */
uint32_t follow_far_chain(uint16_t off, uint16_t seg, int16_t count)
{
    for (;;) {
        if ((uint16_t)(off | seg) == 0)
            break;
        if (count == 0)
            break;
        {
            uint16_t next_seg = FARU16(seg, off + 0x174);
            uint16_t next_off = FARU16(seg, off + 0x172);
            seg = next_seg;
            off = next_off;
        }
        count--;
    }
    return ((uint32_t)seg << 16) | off;
}

/*
 * 0x29966
 *
 * Walk the record list and answer the next one matching a selector, as a far
 * pointer in DX:AX. The cursor is a **static** far pointer at DGROUP 0x6432,
 * with the selector remembered beside it at 0x6436, so this is an iterator with
 * one shared position rather than a search - two overlapping walks would tread
 * on each other.
 *
 * A selector of -3 means "continue": the cursor steps on and the remembered
 * selector is reused. Anything else starts again from the list head at 0x4a88
 * and is remembered.
 *
 * Three selectors filter on the flag word at each record's +0x12, and they are
 * expressed as a mask and an expected value rather than as three tests:
 *
 *   -1  mask 1, expect 0 - records with bit 0 set
 *   -2  mask 1, expect 1 - records with bit 0 clear
 *    0  mask 0, expect 1 - every record, since `0 ^ 1` is never zero
 *
 * Any other selector matches on the identifier at +0xa instead.
 *
 * **An identifier search cannot be continued.** Reaching that branch with the
 * argument -3 clears the cursor and answers nothing - and the cursor has
 * already stepped on by then, so the record after a match is skipped as well as
 * unreported. Whether that is deliberate because identifiers are unique, or an
 * oversight, is not established; it is transcribed as it stands.
 *
 * Running off the end answers a null far pointer, and the two selector families
 * differ in whether they also *clear* the cursor: the flag walk leaves it at
 * null naturally, the identifier walk writes zeros explicitly on the paths that
 * give up early.
 */
uint32_t next_matching_record(int16_t selector)
{
    int16_t expect = 0, mask = 1;

    if (selector != -3) {
        DG16(0x6436) = selector;
        DG16(0x6434) = DG16(0x4a8a);
        DG16(0x6432) = DG16(0x4a88);
    } else if (DGU16(0x6432) != 0 || DGU16(0x6434) != 0) {
        uint8_t *rec = FAR_PTR(DGU16(0x6434), DGU16(0x6432));

        DG16(0x6434) = *(int16_t *)(rec + 2);
        DG16(0x6432) = *(int16_t *)rec;
    }

    if (DG16(0x6436) == -2) {
        expect = 1;
    } else if (DG16(0x6436) == -1) {
        /* mask 1, expect 0 - the defaults */
    } else if (DG16(0x6436) == 0) {
        mask = 0;
        expect = 1;
    } else {
        /* Match on the identifier at +0xa. */
        if ((DGU16(0x6432) == 0 && DGU16(0x6434) == 0) || selector == -3) {
            DG16(0x6434) = 0;
            DG16(0x6432) = 0;
            return 0;
        }

        for (;;) {
            uint8_t *rec;

            if (DGU16(0x6432) == 0 && DGU16(0x6434) == 0)
                break;
            rec = FAR_PTR(DGU16(0x6434), DGU16(0x6432));
            if (*(int16_t *)(rec + 0xa) == selector)
                break;
            DG16(0x6434) = *(int16_t *)(rec + 2);
            DG16(0x6432) = *(int16_t *)rec;
        }
        return ((uint32_t)DGU16(0x6434) << 16) | DGU16(0x6432);
    }

    while (DGU16(0x6432) != 0 || DGU16(0x6434) != 0) {
        uint8_t *rec = FAR_PTR(DGU16(0x6434), DGU16(0x6432));

        if (((*(int16_t *)(rec + 0x12) & mask) ^ expect) != 0)
            break;
        DG16(0x6434) = *(int16_t *)(rec + 2);
        DG16(0x6432) = *(int16_t *)rec;
    }

    return ((uint32_t)DGU16(0x6434) << 16) | DGU16(0x6432);
}
