/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **Sound**: the driver, the sequencer that steps it, and the
 * volume ramps.
 *
 * This file corresponds to the original's **code segment 2619**, image
 * 0x26190..0x2a040. Functions are in address order and each carries the image
 * offset it was read from.
 */
#include <stdio.h>
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
 * 0x265f2
 *
 * Plant the driver and ask it what it is.
 *
 * The far pointer arrives in `ES:AX` and is written straight into the module's
 * own code segment at `cs:0x1e7` - the cell every other routine here far-calls
 * through. Nothing else installs it; this is where the sound module and the
 * loaded `SX.OVL` are joined.
 *
 * Then function 0 - `sx_describe_0` - which answers two constants. `CL` and
 * `CH` are kept at `cs:0x1ff` and `cs:0x1fc`, and `AH >> 4` at `cs:0x200`, with
 * bit 0 forced on when DGROUP 0x4aaa is set. For the speaker driver those come
 * out as 1, 0x12 and 0 - but they are read from the driver, not assumed, so a
 * different `SX.OVL` describes itself differently.
 *
 * Hand-written assembly: register arguments, no frame, a far `ret`. `BP` is
 * saved around the call because it carries the function number.
 *
 * `AX` is left holding `sx_describe_0`'s answer and the routine returns it -
 * not by writing it anywhere, just by not disturbing it, which is a return
 * value in assembly and is why the port declares one.
 */
uint16_t install_driver(uint16_t ax, uint16_t es)
{
    uint16_t cx;
    uint8_t dl;

    SND16(0x1e7) = (int16_t)ax;
    SND16(0x1e9) = (int16_t)es;

    sx_describe_0(&ax, &cx);

    SND8(0x1ff) = (uint8_t)cx;
    SND8(0x1fc) = (uint8_t)(cx >> 8);

    dl = (uint8_t)((ax >> 8) >> 4);
    if (DG16(0x4aaa) != 0)
        dl |= 1;
    SND8(0x200) = dl;

    return ax;
}

/*
 * 0x26629
 *
 * Ask the driver its *second* description and set one parameter from it.
 *
 * Function 1 - `sx_describe_1` - answers another pair of constants, kept at
 * `cs:0x1fa` and `cs:0x1fb`. Then function 11 - `sx_param_349` - is called with
 * `CL` zero.
 *
 * `AX` and `CX` are pushed around that second call and popped back, so the
 * caller sees `sx_describe_1`'s answer and not `sx_param_349`'s - and that is
 * the routine's return value: 0x28580 tests it against 0xffff.
 *
 * Hand-written assembly, as above.
 */
uint16_t configure_driver(void)
{
    uint16_t ax, cx;

    sx_describe_1(&ax, &cx);

    SND8(0x1fa) = (uint8_t)cx;
    SND8(0x1fb) = (uint8_t)(cx >> 8);

    sx_param_349(0);

    return ax;
}

/*
 * 0x2664e
 *
 * Shut the driver up. Function 12 - `sx_param_345` - with `CL` 0xf, then
 * function 2 - `sx_stop_all`, which forwards to the speaker-off.
 *
 * Every register it touches is pushed and popped, `CX` included, so the two
 * calls are invisible to the caller. Hand-written assembly, as above.
 */
void silence_driver(void)
{
    sx_param_345(0xf);
    sx_stop_all();
}

/*
 * 0x26721
 *
 * Set the driver's master level. `CL` is clamped to 0..0xf and handed to
 * function 12 - `sx_param_345` - except that 0xff passes through unclamped,
 * so it is a value the driver reads as something other than a level.
 *
 * Hand-written assembly: the argument is a register and there is no frame.
 */
void set_master_level(uint8_t cl)
{
    if (cl != 0xff && cl > 0xf)
        cl = 0xf;
    sx_param_345(cl);
}

/*
 * 0x26783
 *
 * Start a sequence: stop it if it is already playing, reset every channel it
 * has, read its header, and put it in the playing table in priority order.
 * Hand-written assembly with the record in `es:ax` and a flag in `cx`.
 *
 * It runs with interrupts disabled from end to end, because the table it edits
 * is the one the timer walks.
 *
 * The reset loop covers channels 0 to 14 in full and then does channel 15
 * **partially**: 15 gets +0x8c, +0x9c, +0xac and the words at +0xc, +0x2c,
 * +0x4c, but not +0x6c, +0xbc or any of the six per-channel bytes the others
 * get. That is the original's shape, not a transcription slip - the loop test
 * is `si != 0xf`, and the tail after it writes only some of what the body did.
 *
 * The header walk follows an offset table: `ds:[bp]` is a displacement from the
 * table's own base, and a zero entry ends the walk. An entry of 0xfe is a gap -
 * with `cs:0x200` clear the channel is marked 0xfe and skipped, and with it set
 * the walk stops and records how far it got in +0x165.
 *
 * The channel a header entry configures is **not** the loop index: the loop
 * index picks the entry, and the low nibble of the entry's own first byte picks
 * the channel. The three per-channel defaults are only written where the field
 * is still 0xff, so an earlier entry wins over a later one.
 *
 * Placement is an insertion sort, descending by +0x15c: the first entry whose
 * key is less than or equal to the new one is where it goes, and everything
 * from there is shifted up one slot. A full table drops the sequence silently.
 * With `cs:0x209` set the sequence is placed but its counters are left alone
 * and the tick is not run.
 */
void start_sequence(uint16_t es, uint16_t ax, uint16_t cx)
{
    uint8_t *rec;
    uint16_t di, si, bp, base;
    uint8_t dl, dh, key;

    for (di = 0; di < 0x40; di += 4) {
        if ((uint16_t)SND16(8 + di) == ax && (uint16_t)SND16(0xa + di) == es) {
            remove_sequence(es, ax);
            sequencer_tick();
            break;
        }
    }

    rec = FAR_PTR(es, ax);
    rec[0x159] = 1;
    if (cx != 0)
        rec[0x159]++;

    init_sequence_params(es, ax);

    for (si = 0; si < 0xf; si++) {
        *(uint16_t *)(rec + 2 * si + 0xc) = 0xd;
        *(uint16_t *)(rec + 2 * si + 0x2c) = 3;
        *(uint16_t *)(rec + 2 * si + 0x4c) = 0;
        *(uint16_t *)(rec + 2 * si + 0x6c) = 0;
        *(uint16_t *)(rec + 2 * si + 0xbc) = 0x2000;
        rec[si + 0x8c] = 0xff;
        rec[si + 0x9c] = 0;
        rec[si + 0xac] = 0;
        rec[si + 0xda] = 0xff;
        rec[si + 0xe9] = 0;
        rec[si + 0x116] = 0xff;
        rec[si + 0x107] = 0xff;
        rec[si + 0xf8] = 0xff;
        rec[si + 0x125] = 0xff;
        rec[si + 0x134] = 0;
        rec[si + 0x143] = 0;
    }

    rec[0xf + 0x8c] = 0xff;
    rec[0xf + 0x9c] = 0;
    rec[0xf + 0xac] = 0;
    rec[0x165] = 0;
    rec[0x15a] = 0;
    rec[0x15f] = 0x7f;
    *(uint16_t *)(rec + 2 * 0xf + 0xc) = 0xd;
    *(uint16_t *)(rec + 2 * 0xf + 0x2c) = 3;
    *(uint16_t *)(rec + 2 * 0xf + 0x4c) = 0;
    *(uint16_t *)(rec + 0x156) = 0;

    {
        uint16_t cur_seg, cur_off, tbl_seg, tbl_off;
        const uint8_t *tbl;

        cur_off = *(uint16_t *)(rec + 8);
        cur_seg = *(uint16_t *)(rec + 0xa);
        {
            const uint8_t *via = FAR_PTR(cur_seg, cur_off);

            tbl_off = *(uint16_t *)via;
            tbl_seg = *(uint16_t *)(via + 2);
        }
        tbl = FAR_PTR(tbl_seg, tbl_off);

        if (tbl[0x20] != 0xff && rec[0x15b] == 0)
            rec[0x15c] = tbl[0x20];

        base = 0;
        si = 0;
        bp = 0;

        for (;;) {
            uint16_t entry = *(uint16_t *)(tbl + bp);
            const uint8_t *e;

            if (entry == 0)
                break;

            e = tbl + base + entry;
            dl = e[0];

            if (dl == 0xfe) {
                if (SND8(0x200) != 0) {
                    rec[0x165] = (uint8_t)(si + 1);
                    break;
                }
                *(uint16_t *)(rec + 2 * si + 0xc) = 0;
                *(uint16_t *)(rec + 2 * si + 0x2c) = 0;
                rec[si + 0x8c] = 0xfe;
            } else {
                uint16_t ch;

                rec[si + 0x8c] = dl;
                rec[si + 0x9c] = (uint8_t)(dl | 0xb0);

                dl = e[0xc];
                dh = 0;
                if (dl == 0xf8) {
                    dl = 0xf0;
                    dh = 0x80;
                }
                *(uint16_t *)(rec + 2 * si + 0x4c) =
                    (uint16_t)(((uint16_t)dh << 8) | dl);

                dl = rec[si + 0x8c];
                rec[si + 0x8c] &= 0xf;
                ch = (uint16_t)(dl & 0xf);

                if ((dl & 0x10) != 0) {
                    *(uint16_t *)(rec + 2 * si + 0xc) = 3;
                    *(uint16_t *)(rec + 2 * si + 0x4c) = 0;
                    rec[ch + 0x134] |= 2;
                } else {
                    int16_t do_f8 = 1;

                    if ((dl & 0x20) != 0)
                        rec[ch + 0x134] |= 1;
                    if ((dl & 0x40) != 0)
                        rec[ch + 0x143] = 1;

                    if (ch == 0xf) {
                        if (rec[0x15f] == 0x7f) {
                            rec[0x15f] = e[8];
                            do_f8 = 0;
                        }
                    } else {
                        if (rec[ch + 0xda] == 0xff)
                            rec[ch + 0xda] = e[1];
                        if (rec[ch + 0x116] == 0xff)
                            rec[ch + 0x116] = e[4];
                        if (rec[ch + 0x107] == 0xff)
                            rec[ch + 0x107] = e[8];
                    }

                    if (do_f8 && rec[ch + 0xf8] == 0xff)
                        rec[ch + 0xf8] = e[0xb];
                }
            }

            si++;
            bp = (uint16_t)(2 * si);
            if (si == 0x10)
                break;
        }
    }

    if (rec[0x159] == 2) {
        for (di = 0xe; (int16_t)di >= 0; di--)
            rec[di + 0x134] |= 1;
    }

    key = rec[0x15c];

    for (di = 0; di < 0x40; di += 4) {
        const uint8_t *other;

        if (SND16(0xa + di) == 0)
            break;
        other = FAR_PTR((uint16_t)SND16(0xa + di), (uint16_t)SND16(8 + di));
        if (other[0x15c] <= key) {
            /*
             * **The comparison is 16 bits and has to wrap.** The original
             * computes it in BX - `mov bx,si / add bx,4 / cmp bx,di` at
             * 0x269ee - so when `di` is 0 the walk runs down to 0xfffc, BX
             * comes out 0, and it stops. Written as `si + 4` in C the addition
             * promotes to `int`, 0xfffc + 4 is 0x10000 rather than 0, and the
             * loop never ends: it ran off the bottom of the table writing
             * pairs of words over guest memory until the frame rate collapsed
             * from thirty a second to one every two seconds.
             *
             * Reached by a sound played from a part's step - `play_sound(12)`
             * out of `part_step_1e5c` - with the new sequence's key at or
             * below the first entry's, which is what makes `di` zero.
             */
            for (si = 0x38; (uint16_t)(si + 4) != di; si -= 4) {
                SND16(si + 0xc) = SND16(si + 8);
                SND16(si + 0xe) = SND16(si + 0xa);
            }
            break;
        }
    }
    if (di >= 0x40)
        return;

    SND16(di + 8) = (int16_t)ax;
    SND16(di + 0xa) = (int16_t)es;

    if (SND8(0x209) != 0)
        return;

    *(uint16_t *)(rec + 0x152) = 0;
    *(uint16_t *)(rec + 0x154) = 0;
    rec[0x158] = 0;
    rec[0x160] = 0;
    rec[0x161] = 0;
    rec[0x162] = 0;
    rec[0x163] = 0;
    rec[0x164] = 0;

    sequencer_tick();
}

/*
 * 0x26a57
 *
 * Retire whatever has finished and run the sequencer once, with interrupts
 * masked across both. Six instructions: `pushf`, `cli`, the two near calls,
 * `popf`, `retf`.
 *
 * The mask is the point of the routine - `remove_sequence` unlinks records that
 * `sequencer_tick` is about to walk, and the timer interrupt calls
 * `sound_service` which walks the same list. Doing it with the flag saved and
 * restored rather than a bare `sti` means a caller that already had interrupts
 * off keeps them off.
 *
 * `ES:AX` is not touched here and is not this routine's own: it arrives in the
 * registers and `remove_sequence` reads it, so the port passes it through.
 *
 * Hand-written assembly, no frame, a far `ret`.
 */
void retire_and_tick(uint16_t es, uint16_t ax)
{
    io_lock();                  /* `pushf`, `cli` */
    remove_sequence(es, ax);
    sequencer_tick();
    io_unlock();                /* `popf` - which is why the lock is recursive */
}

/*
 * 0x26e7b
 *
 * Take a sequence out of the playing table and stop it. Hand-written assembly
 * with the record in `es:ax`.
 *
 * The table is the sixteen far pointers at the module's `cs:8`, the ones
 * `sequencer_tick` walks. The matching entry is cleared and every entry above
 * it moved down one, so the table stays packed with no holes for the tick to
 * skip over - and the last slot is cleared afterwards, because the shift leaves
 * a duplicate there. A record that is not in the table at all is simply
 * ignored.
 *
 * Then the record's own +0x158 is set to 0xff and +0x159 to zero.
 *
 * The rest only happens when +0x165 is non-zero **and at least 0x80**, and what
 * it computes is thrown away: two far pointers are followed and an index taken
 * from the low nibble of +0x165 is used to read an offset and add it to BP -
 * after which BP and DS are both restored by the epilogue and AX is
 * overwritten with zero. The call that follows takes 5 and 0, whatever that
 * arithmetic produced. Dead as written, and transcribed as the condition it
 * still is: the callback happens only for +0x165 >= 0x80.
 */
void remove_sequence(uint16_t es, uint16_t ax)
{
    uint8_t *rec;
    int16_t si;

    for (si = 0; si < 0x40; si += 4)
        if ((uint16_t)SND16(8 + si) == ax && (uint16_t)SND16(0xa + si) == es)
            break;
    if (si >= 0x40)
        return;

    SND16(8 + si) = 0;
    SND16(0xa + si) = 0;

    if (si != 0x3c) {
        for (; si != 0x3c; si += 4) {
            SND16(8 + si) = SND16(0xc + si);
            SND16(0xa + si) = SND16(0xe + si);
        }
        SND16(8 + si) = 0;
        SND16(0xa + si) = 0;
    }

    rec = FAR_PTR(es, ax);
    rec[0x158] = 0xff;
    rec[0x159] = 0;

    if (rec[0x165] == 0)
        return;
    if (rec[0x165] < 0x80)
        return;

    sound_callback(0);
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
 * 0x278e9
 *
 * Advance a sequence's volume fade by one tick. Hand-written assembly with the
 * record in `es:bx` and the sequence's slot in `si`.
 *
 * +0x162 counts ticks down to the next step and is reloaded from +0x161, so a
 * fade moves once every +0x161 ticks rather than every tick. +0x160 holds the
 * target with a flag in its top bit, +0x163 the largest step allowed, and
 * +0x15e the volume now.
 *
 * Each step moves toward the target by at most +0x163, and lands exactly on it
 * when what remains is no more than a step - so a fade always finishes on the
 * target rather than oscillating around it.
 *
 * Arriving sets +0x158 to 0xfe and clears the step, and **if the top bit of
 * +0x160 is set the sequence is then removed altogether**. That is how a fade
 * to silence stops a sequence: the flag rides along in the spare bit of the
 * target, which is why every read of the target masks it off.
 */
void advance_volume_ramp(uint16_t es, uint16_t bx, uint16_t seq_slot)
{
    uint8_t *rec = FAR_PTR(es, bx);
    uint8_t target, now, distance;

    if (rec[0x162] != 0) {
        rec[0x162]--;
        return;
    }
    rec[0x162] = rec[0x161];

    target = (uint8_t)(rec[0x160] & 0x7f);
    now = rec[0x15e];

    if (target != now) {
        if (target > now) {
            distance = (uint8_t)(target - now);
            if (distance > rec[0x163]) {
                set_sequence_volume(es, bx, (uint8_t)(now + rec[0x163]), 1,
                                    seq_slot);
                return;
            }
        } else {
            distance = (uint8_t)(now - target);
            if (distance > rec[0x163]) {
                set_sequence_volume(es, bx, (uint8_t)(now - rec[0x163]), 1,
                                    seq_slot);
                return;
            }
        }
        set_sequence_volume(es, bx, target, 1, seq_slot);
    }

    rec[0x158] = 0xfe;
    rec[0x163] = 0;

    if ((rec[0x160] & 0x80) != 0) {
        remove_sequence(es, bx);
        SND8(0x204) = 1;
    }
}

/*
 * 0x279a9
 *
 * Set a sequence's volume and push it out to every voice the sequence owns.
 *
 * `defer` goes to `cs:0x205` and chooses how: set, the new value is left in the
 * pending array at `cs:0x1c8` for `flush_pending_volumes` to send two at a time
 * from the timer tick; clear, the driver is told immediately and the pending
 * entry is marked 0xff so the flush skips it. A fade always defers, which is
 * what stops a slow fade flooding the interrupt with controller changes.
 *
 * Nothing happens at all if the volume is already what is asked for, and
 * nothing is sent if the sequence has no slot - `0xff` - although the volume is
 * still stored, so a sequence that is not playing still remembers it.
 *
 * Two passes, and they are not the same. The first walks the sixteen voices and
 * takes those whose owner's high nibble matches this sequence, using the voice
 * number as the driver's channel. The second walks the sequence's own channel
 * map at +0x8c and takes only channels with bit 1 at +0x134 that hold **no**
 * voice, using the channel number as the driver's channel instead. The second
 * pass stops at the first 0xff in the map rather than skipping it.
 *
 * Each voice's volume is its own +0x107 scaled by the sequence's, through
 * `scale_byte_pair`.
 */
void set_sequence_volume(uint16_t es, uint16_t bx, uint8_t volume,
                         uint8_t defer, uint16_t seq_slot)
{
    uint8_t *rec = FAR_PTR(es, bx);
    uint16_t si, di;
    uint8_t want, level;

    SND8(0x205) = defer;

    if (volume == rec[0x15e])
        return;
    rec[0x15e] = volume;

    if (seq_slot == 0xff)
        return;

    want = (uint8_t)(seq_slot << 2);

    for (si = 0; si < 0x10; si++) {
        uint8_t held = SND8(0x128 + si);

        if (held == 0xff || (uint8_t)(held & 0xf0) != want)
            continue;

        di = (uint16_t)(held & 0xf);
        level = scale_byte_pair(rec[di + 0x107], rec[0x15e]);

        if (SND8(0x205) != 0) {
            SND8(0x1c8 + si) = level;
        } else {
            SND8(0x1c8 + si) = 0xff;
            sx_controller(si, (uint16_t)((7 << 8) | level));
        }
    }

    for (si = 0; si < 0x10; si++) {
        di = rec[si + 0x8c];
        if (di == 0xff)
            return;
        if ((rec[di + 0x134] & 2) == 0)
            continue;
        if (SND8(0x128 + di) != 0xff)
            continue;

        level = scale_byte_pair(rec[di + 0x107], rec[0x15e]);

        if (SND8(0x205) != 0) {
            SND8(0x1c8 + di) = level;
        } else {
            SND8(0x1c8 + di) = 0xff;
            sx_controller(di, (uint16_t)((7 << 8) | level));
        }
    }
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
 * 0x27ace
 *
 * The sound module's service routine - what the timer calls. Runs every
 * playing sequence forward one tick, then polls, flushes and tells the host.
 *
 * The first thing it does is **refuse to run** while `cs:0x1f9` is non-zero.
 * That is the depth count `sequencer_tick` maintains, so a tick already in
 * progress cannot be re-entered by the interrupt that fires during it. It is
 * the only place that guard is tested.
 *
 * Interrupts are then disabled for the whole body, because the playing table is
 * the one `start_sequence` and `remove_sequence` edit.
 *
 * `cs:0x204` set means something changed which voice plays what, so the
 * allocator is run before anything else.
 *
 * The walk keeps **two** indices. `si` is the position in the table now, and
 * `di` the position a sequence started at. They advance together until a
 * sequence is removed - `+0x158` reading 0xff - and then only `di` advances,
 * because removing an entry shifted everything below it down and the next
 * sequence is now at the same `si`. The original writes that as `sub si,4`
 * followed by the shared `add si,4`, and as a jump past the `add`; both mean
 * the same thing.
 *
 * The two indices are not interchangeable: the fade gets `si` and the stepper
 * gets `di`.
 *
 * A sequence with +0x164 set is skipped entirely. One with a fade step at
 * +0x163 has its fade advanced first. Then either it is dropped for not being
 * on the poll table, if +0x165 says it should be polled, or it is stepped.
 *
 * Afterwards `poll_sequences` and `flush_pending_volumes` run once, and the
 * host callback is asked question 3.
 */
void sound_service(void)
{
    uint16_t si, di;

    if (SND8(0x1f9) != 0)
        return;

    if (SND8(0x204) != 0)
        sequencer_tick();

    si = 0;
    di = 0;

    while (si != 0x40) {
        uint16_t bx = (uint16_t)SND16(8 + si);
        uint16_t es = (uint16_t)SND16(0xa + si);
        const uint8_t *rec;

        if (es == 0 && bx == 0)
            break;

        rec = FAR_PTR(es, bx);

        if (rec[0x164] != 0) {
            si += 4;
            di += 4;
            continue;
        }

        if (rec[0x163] != 0) {
            advance_volume_ramp(es, bx, si);
            if (rec[0x158] == 0xff) {
                di += 4;
                continue;
            }
        }

        if (rec[0x165] != 0)
            drop_unless_polled(es, bx);
        else
            step_sequence(es, bx, di);

        if (rec[0x158] != 0xff)
            si += 4;
        di += 4;
    }

    poll_sequences();
    flush_pending_volumes();

    /*
     * A **driver** call, not the host callback: this goes through `cs:[0x1e7]`
     * with the function number in BP, and 3 is one of the seven table entries
     * pointing at the do-nothing stub. Reading it as the host callback at
     * 0x292a1 - which is also reached with a 3 - leaves that routine's result
     * slot at `cs:0x30fa` holding 3 where the original leaves 0, which is
     * exactly how the mistake showed up.
     */
    sx_nop();
}

/*
 * 0x27b52
 *
 * Remove a sequence unless it is on the poll table at `cs:0x48`.
 *
 * A sequence that has asked to be polled is left alone - `poll_sequences` owns
 * it and will decide when it ends. Anything else is taken out of the playing
 * table and `cs:0x204` is set to say the table changed.
 *
 * The search compares both halves of the far pointer, so a record at the same
 * offset in a different segment does not count as a match.
 */
void drop_unless_polled(uint16_t es, uint16_t bx)
{
    int16_t si;

    for (si = 0; si < 0x40; si += 4)
        if ((uint16_t)SND16(0x48 + si) == bx
            && (uint16_t)SND16(0x4a + si) == es)
            return;

    remove_sequence(es, bx);
    SND8(0x204) = 1;
}

/*
 * 0x27b7e
 *
 * Poll every sequence that has asked to be polled, and let the host callback
 * decide whether it carries on.
 *
 * The table walked here is at the module's `cs:0x48` and is **not** the playing
 * table at `cs:8` - it is the second one, the entries `sequencer_tick` parks
 * there for sequences whose +0x165 marks them as needing attention. A null
 * entry ends the whole walk, not just that iteration, so the table is expected
 * to be packed.
 *
 * Each sequence's counter at +0x154 is bumped, and then the byte at +0x165
 * chooses between two calls. At 0x10 or below the sequence is marked with bit
 * 0x80 and the callback is asked question 3; above 0x10 it is asked question 4,
 * and the answer decides: a non-zero high byte resets the counter, a non-zero
 * low byte clears +0x165, removes the sequence from the playing table and sets
 * `cs:0x204`.
 *
 * Both calls build a small block of arguments **on the stack** and pass its
 * address. The port does not build them: `sound_callback` reads its stack
 * arguments only on the path that reaches an installed callback, which is not
 * transcribed, and the stack itself is not compared. The pointer arithmetic
 * feeding those blocks - two far pointers followed and an index taken from the
 * low nibble of +0x165 - is read-only for the same reason.
 *
 * With no callback installed, `sound_callback` answers whatever it was passed,
 * so question 4 comes back as 4: high byte zero, low byte non-zero. Every
 * sequence on this table is therefore removed.
 */
void poll_sequences(void)
{
    int16_t si;

    for (si = 0; si < 0x40; si += 4) {
        uint16_t bx = (uint16_t)SND16(0x48 + si);
        uint16_t es = (uint16_t)SND16(0x4a + si);
        uint8_t *rec;
        uint16_t answer;

        if (es == 0 && bx == 0)
            return;

        rec = FAR_PTR(es, bx);
        (*(uint16_t *)(rec + 0x154))++;

        if (rec[0x165] <= 0x10) {
            rec[0x165] |= 0x80;
            sound_callback(3);
            continue;
        }

        answer = sound_callback(4);

        if ((uint8_t)(answer >> 8) != 0)
            *(uint16_t *)(rec + 0x154) = 0;

        if ((uint8_t)answer != 0) {
            rec[0x165] = 0;
            remove_sequence(es, bx);
            SND8(0x204) = 1;
        }
    }
}

/*
 * 0x27c4e
 *
 * Step one sequence forward by one tick: for each of its channels, run down
 * the delay and, when it reaches zero, read and dispatch as many events as the
 * stream says happen at this instant.
 *
 * `di` is the sequence's slot, and `di * 4` is parked in `cs:0x201` as the high
 * nibble every request byte carries. `es:bx` is the record; the event data is
 * reached through two far pointers from +8, and the base offset is kept in
 * `cs:0x1f7` because BP is used as the cursor.
 *
 * Each channel's entry in the map at +0x8c ends the walk at 0xff and is skipped
 * at 0xfe. Before anything is read, the channel's voice is worked out into
 * `cs:0x1fd`: a channel with bit 1 at +0x134 is its own voice and `cs:0x1fe` is
 * set to say so, otherwise the sixteen voices are searched for one whose owner
 * matches. Not finding one leaves 0xff, and the handlers take that as "do not
 * tell the driver".
 *
 * The delay at +0x4c counts down each tick. **0x8000 is not zero**: reaching it
 * means the delay was a long one and the next byte of the stream extends it, so
 * a delay can be longer than a byte can hold. A delay of exactly 0xf8 is stored
 * as 0xf0 with the same top bit, which is how the two are told apart.
 *
 * With the delay expired, a byte is read. 0x80 and above is a status byte and
 * is remembered at +0x9c; below that it is **running status** - the byte is
 * pushed back, the counter undone, and the remembered status used instead,
 * which is how MIDI avoids repeating a status that has not changed.
 *
 * 0xfc ends the channel outright. A low nibble of 0xf is a meta event. Anything
 * else dispatches on the high nibble to the eight handlers - note off, note on,
 * aftertouch, controller, program, pressure, bend, system - and an unknown high
 * nibble also ends the channel.
 *
 * After each event another byte is read as the delay to the next. **Zero means
 * no delay**, so the loop goes straight back and reads another event at the
 * same instant; that is how chords are written. Anything else is stored one
 * less than it was read, because the tick that stores it has already happened.
 *
 * When every channel before the first 0xff has run out, the sequence has
 * finished. With both +0x15a and +0x15d zero it is removed; otherwise it loops
 * - every channel's position, delay and running status restored from the
 * shadows a checkpoint saved, and +0x154 from +0x156.
 */
void step_sequence(uint16_t es, uint16_t bx, uint16_t di)
{
    uint8_t *rec = FAR_PTR(es, bx);
    uint16_t ds, bp, base;
    uint16_t si;
    int16_t t;

    SND8(0x201) = (uint8_t)(di * 4);
    (*(uint16_t *)(rec + 0x154))++;

    {
        /*
         * Two loads, not three. The first reads the far pointer stored at the
         * record's +8; the second reads the far pointer *that* points at, and
         * the result is the cursor's base. Measured on the first call:
         * +8 holds 7594:016a, and 7594:016a holds 77ab:0002, so the base is 2
         * in segment 77ab. Following it once more lands in the event data and
         * reads a note as if it were a pointer.
         */
        uint16_t o = *(uint16_t *)(rec + 8), s = *(uint16_t *)(rec + 0xa);
        const uint8_t *via = FAR_PTR(s, o);

        base = *(uint16_t *)via;
        ds = *(uint16_t *)(via + 2);
    }
    bp = base;
    SND16(0x1f7) = (int16_t)base;

    for (si = 0; si < 0x10; si++) {
        uint8_t al = rec[si + 0x8c];
        uint16_t *pos = (uint16_t *)(rec + 2 * si + 0xc);
        uint16_t *delay = (uint16_t *)(rec + 2 * si + 0x4c);
        uint8_t status;

        if (al == 0xff)
            goto finished;
        if (al == 0xfe)
            continue;

        SND8(0x1fd) = 0xff;
        SND8(0x1fe) = 0;

        if ((rec[al + 0x134] & 2) != 0) {
            SND8(0x1fd) = al;
            SND8(0x1fe) = 1;
        } else {
            uint8_t want = (uint8_t)((al & 0xf) | SND8(0x201));
            uint16_t j;

            for (j = 0; j < 0x10; j++) {
                if (SND8(0x128 + j) == want) {
                    SND8(0x1fd) = (uint8_t)j;
                    break;
                }
            }
        }

        bp = (uint16_t)(base + *(const uint16_t *)(FAR_PTR(ds, 0) + base
                                                   + 2 * si) + *pos);
        if (*pos == 0)
            continue;

        if (*delay != 0) {
            (*delay)--;
            if (*delay == 0x8000) {
                uint8_t d = *FAR_PTR(ds, bp);
                uint8_t hi = 0;

                bp++;
                (*pos)++;
                if (d == 0xf8) {
                    d = 0xf0;
                    hi = 0x80;
                }
                *delay = (uint16_t)(((uint16_t)hi << 8) | d);
            }
            continue;
        }

        for (;;) {
            uint8_t b = *FAR_PTR(ds, bp);
            uint8_t hi_nibble, lo_nibble;

            bp++;
            (*pos)++;

            if (b >= 0x80) {
                rec[si + 0x9c] = b;
            } else {
                b = rec[si + 0x9c];
                bp--;
                (*pos)--;
            }

            status = b;
            hi_nibble = (uint8_t)(b & 0xf0);
            lo_nibble = (uint8_t)(b & 0xf);

            if (status == 0xfc) {
                *pos = 0;
                break;
            }

            if (lo_nibble == 0xf) {
                bp = midi_meta_event(ds, bp, es, bx, si,
                                     (uint16_t)((hi_nibble << 8) | 0xf));
                if (*pos == 0)
                    break;
            } else {
                uint16_t ax = (uint16_t)(((uint16_t)hi_nibble << 8)
                                         | SND8(0x1fd));

                switch (hi_nibble) {
                case 0x80: bp = midi_note_off_event(ds, bp, es, bx, si, ax); break;
                case 0x90: bp = midi_note_event(ds, bp, es, bx, si, ax); break;
                case 0xa0: bp = midi_event_6(ds, bp, es, bx, si, ax); break;
                case 0xb0: bp = midi_controller_event(ds, bp, es, bx, si, ax); break;
                case 0xc0: bp = midi_program_event(ds, bp, es, bx, si, ax); break;
                case 0xd0: bp = midi_event_9(ds, bp, es, bx, si, ax); break;
                case 0xe0: bp = midi_bend_event(ds, bp, es, bx, si, ax); break;
                case 0xf0: bp = midi_skip_event(ds, bp, es, bx, si, ax); break;
                default:
                    *pos = 0;
                    goto next_channel;
                }
            }

            {
                uint8_t d = *FAR_PTR(ds, bp);

                bp++;
                (*pos)++;
                if (d == 0)
                    continue;
                if (d == 0xf8)
                    *delay = 0x80ef;
                else
                    *delay = (uint16_t)(d - 1);
                break;
            }
        }
next_channel:
        ;
    }

finished:
    for (si = 0; si < 0x10; si++) {
        if (rec[si + 0x8c] == 0xff)
            break;
        if (*(uint16_t *)(rec + 2 * si + 0xc) != 0)
            return;
    }

    if (rec[0x15a] == 0 && rec[0x15d] == 0) {
        remove_sequence(es, bx);
        SND8(0x204) = 1;
        return;
    }

    *(uint16_t *)(rec + 0x154) = *(uint16_t *)(rec + 0x156);
    for (t = 0; t < 0x10; t++) {
        *(uint16_t *)(rec + 2 * t + 0xc) = *(uint16_t *)(rec + 2 * t + 0x2c);
        *(uint16_t *)(rec + 2 * t + 0x4c) = *(uint16_t *)(rec + 2 * t + 0x6c);
        rec[t + 0x9c] = rec[t + 0xac];
    }
}

/*
 * 0x27e92
 *
 * Handle an explicit note-off event, and answer the stream cursor advanced past
 * it. The same register convention and byte accounting as `midi_note_event` at
 * 0x27ee1: two bytes consumed, the per-byte counter at `+0xc + 2 * si` bumped
 * twice with the raw channel, the mapped channel from `+0x8c` used after.
 *
 * MIDI has two ways to end a note - this message, and a note-on with zero
 * velocity - and the game's sequences use both, which is why there are two
 * routines. This one reads the second byte and never looks at it: the note is
 * the first byte and the release velocity is discarded.
 *
 * As with the note-on, `+0x125` is cleared only when the note recorded there is
 * the one being released, so a channel already given a different note is left
 * alone; and only the driver call is skipped for an unplayed channel or a set
 * `cs:0x209`.
 */
uint16_t midi_note_off_event(uint16_t ds, uint16_t bp, uint16_t es,
                             uint16_t bx, uint16_t si, uint16_t ax)
{
    uint8_t note, channel;
    uint16_t *counter = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * si + 0xc));

    note = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    bp++;
    (*counter)++;

    channel = (uint8_t)(*FAR_PTR(es, (uint16_t)(bx + si + 0x8c)) & 0xf);

    if (*FAR_PTR(es, (uint16_t)(bx + channel + 0x125)) == note)
        *FAR_PTR(es, (uint16_t)(bx + channel + 0x125)) = 0xff;

    if ((uint8_t)ax != 0xff && SND8(0x209) == 0)
        sx_stop_note((uint16_t)(note << 8));

    return bp;
}

/*
 * 0x27f54
 *
 * Handle a two-byte event whose driver function is number 6 - which is one of
 * the seven entries pointing at the do-nothing stub, so on this driver the
 * event costs two bytes of stream and changes nothing.
 *
 * Both bytes are read and counted the usual way, and neither is stored
 * anywhere: this routine's whole effect on the sequence is the cursor and the
 * two counter increments. Whatever it means, a PC speaker has no way to do it.
 */
uint16_t midi_event_6(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                      uint16_t si, uint16_t ax)
{
    uint16_t *counter = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * si + 0xc));

    (void)*FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    (void)*FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    if ((uint8_t)ax != 0xff && SND8(0x209) == 0)
        sx_nop();

    return bp;
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
 * 0x27f85
 *
 * Handle a controller change - the busiest of the event handlers, and the one
 * that keeps most of a channel's state.
 *
 * Two bytes are read and counted as usual, the controller then its value, and
 * the same `cs:0x1fe` gate `midi_bend_event` has applies after they are
 * consumed. Six controllers are recognised and everything else falls through to
 * the driver unchanged:
 *
 *   0x07  volume. Stored at +0x107, and then **the value handed to the driver
 *         is replaced**: `scale_byte_pair` scales it by the sequence's own
 *         volume at +0x15e, so a channel's volume is always relative. The
 *         pending entry at `cs:0x1c8` is cleared first, so a deferred volume
 *         already queued for this channel does not later overwrite this one.
 *   0x0a  pan, stored at +0xf8.
 *   0x01  modulation, stored at +0xe9.
 *   0x40  sustain. This is the flag that lives in **bit 15 of the pitch bend
 *         word** at +0xbc - set for any non-zero value, cleared for zero -
 *         which is why `midi_bend_event` carries that bit across every write.
 *   0x4b  replaces the low nibble of +0xda, and sets `cs:0x204`.
 *   0x4e  sets the low nibble of +0x143 to 1 or 0, and sets `cs:0x204`.
 *
 * Only 0x07 changes what the driver is told; the rest pass their own value
 * through. The two that set `cs:0x204` are the two that change how voices are
 * allocated, so the tick is told the table needs redoing.
 */
uint16_t midi_controller_event(uint16_t ds, uint16_t bp, uint16_t es,
                               uint16_t bx, uint16_t si, uint16_t ax)
{
    uint16_t *counter = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * si + 0xc));
    uint8_t ctrl, value, channel;

    ctrl = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    value = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    if (SND8(0x1fe) != 0 && SND8(0x128 + (ax & 0xf)) != 0xff)
        return bp;

    channel = (uint8_t)(*FAR_PTR(es, (uint16_t)(bx + si + 0x8c)) & 0xf);

    if (ctrl == 7) {
        *FAR_PTR(es, (uint16_t)(bx + channel + 0x107)) = value;
        value = scale_byte_pair(value,
                                *FAR_PTR(es, (uint16_t)(bx + 0x15e)));
        if ((uint8_t)ax >= 0x20)
            return bp;
        SND8(0x1c8 + (uint8_t)ax) = 0xff;
    } else if (ctrl == 0xa) {
        *FAR_PTR(es, (uint16_t)(bx + channel + 0xf8)) = value;
    } else if (ctrl == 1) {
        *FAR_PTR(es, (uint16_t)(bx + channel + 0xe9)) = value;
    } else if (ctrl == 0x40) {
        uint16_t *bend =
            (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * channel + 0xbc));

        if (value != 0)
            *bend |= 0x8000;
        else
            *bend &= 0x7fff;
    } else if (ctrl == 0x4b) {
        uint8_t *p = FAR_PTR(es, (uint16_t)(bx + channel + 0xda));

        *p = (uint8_t)((*p & 0xf0) | value);
        SND8(0x204) = 1;
    } else if (ctrl == 0x4e) {
        uint8_t *p = FAR_PTR(es, (uint16_t)(bx + channel + 0x143));

        *p = (uint8_t)((*p & 0xf0) | (value != 0 ? 1 : 0));
        SND8(0x204) = 1;
    }

    if ((uint8_t)ax != 0xff && SND8(0x209) == 0)
        sx_controller((uint16_t)(ax & 0xf),
                      (uint16_t)(((uint16_t)ctrl << 8) | value));

    return bp;
}

/*
 * 0x28086
 *
 * Handle a program change: one byte, stored as the channel's instrument at
 * +0x116, then driver function 8 - another of the stub entries, so the speaker
 * driver is told and does nothing with it.
 *
 * The value is stored even when the driver call is skipped, so a muted or
 * unassigned channel still remembers its instrument for whenever it is heard.
 *
 * It carries the same extra gate `midi_bend_event` has: with `cs:0x1fe`
 * non-zero, a channel whose byte at `cs:0x128` is not 0xff is dropped - but
 * only after the byte has been consumed and counted, so the stream stays in
 * step.
 */
uint16_t midi_program_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                            uint16_t si, uint16_t ax)
{
    uint16_t *counter = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * si + 0xc));
    uint8_t program, channel;

    program = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    if (SND8(0x1fe) != 0 && SND8(0x128 + (ax & 0xf)) != 0xff)
        return bp;

    channel = (uint8_t)(*FAR_PTR(es, (uint16_t)(bx + si + 0x8c)) & 0xf);
    *FAR_PTR(es, (uint16_t)(bx + channel + 0x116)) = program;

    if ((uint8_t)ax != 0xff && SND8(0x209) == 0)
        sx_nop();

    return bp;
}

/*
 * 0x280da
 *
 * Handle a one-byte event whose driver function is number 9 - a stub entry
 * again. The byte is read and counted and nothing keeps it.
 *
 * Unlike its neighbours this one does not save SI: it never changes it, so
 * there is nothing to put back.
 */
uint16_t midi_event_9(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                      uint16_t si, uint16_t ax)
{
    uint16_t *counter = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * si + 0xc));

    (void)*FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    if ((uint8_t)ax != 0xff && SND8(0x209) == 0)
        sx_nop();

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
 * 0x2817e
 *
 * Handle the two status bytes that carry the sequencer's own meta events, and
 * hand anything else to `skip_unknown_event`.
 *
 * **0xc0** is either a plain value or a checkpoint. Any first byte but 0x7f is
 * stored at +0x158 - unless `cs:0x209` is set, in which case it is read and
 * dropped, so a muted sequence still consumes the same bytes.
 *
 * A first byte of 0x7f is a checkpoint: the second byte is read, 0xf8 being
 * rewritten as 0xf0 with 0x80 in the high half, and parked at `+0x4c`. Every
 * channel's running position is then copied into its shadow - `+0xc` to `+0x2c`,
 * `+0x4c` to `+0x6c`, `+0x9c` to `+0xac` - and +0x154 to +0x156. The parked
 * value exists only to ride into `+0x6c` on that copy: as soon as it has, the
 * byte read is **undone** - the counter decremented, the cursor stepped back,
 * and `+0x4c` cleared - so the second byte is left in the stream to be read
 * again.
 *
 * **0xb0** carries three of its own controllers, read as a pair:
 *
 *   0x50  the sequence's device value at +0x15f, with 0x7f meaning "use the
 *         default at `cs:0x202`", then passed to the driver as function 11.
 *   0x60  bumps the loop counter at +0x152, and does nothing while `cs:0x209`
 *         is set.
 *   0x52  resets all sixteen channel positions at +0xc to zero, but **only if
 *         the value matches +0x15a** - so a sequence ignores a rewind aimed at
 *         a different one.
 */
uint16_t midi_meta_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                         uint16_t si, uint16_t ax)
{
    uint8_t *rec = FAR_PTR(es, bx);
    uint16_t *counter = (uint16_t *)(rec + 2 * si + 0xc);
    uint8_t status = (uint8_t)(ax >> 8);
    uint8_t first, second;
    int16_t t;

    if (status != 0xc0 && status != 0xb0)
        return skip_unknown_event(ds, bp, es, bx, si, ax);

    if (status == 0xc0) {
        first = *FAR_PTR(ds, bp);
        bp++;
        (*counter)++;

        if (first != 0x7f) {
            if (SND8(0x209) == 0)
                rec[0x158] = first;
            return bp;
        }

        second = *FAR_PTR(ds, bp);
        bp++;
        (*counter)++;

        {
            uint8_t hi = 0;

            if (second == 0xf8) {
                hi = 0x80;
                second = 0xf0;
            }
            *(uint16_t *)(rec + 2 * si + 0x4c) =
                (uint16_t)(((uint16_t)hi << 8) | second);
        }
        rec[si + 0x9c] = 0xcf;

        for (t = 0; t < 0x10; t++) {
            *(uint16_t *)(rec + 2 * t + 0x2c) = *(uint16_t *)(rec + 2 * t + 0xc);
            *(uint16_t *)(rec + 2 * t + 0x6c) = *(uint16_t *)(rec + 2 * t + 0x4c);
            rec[t + 0xac] = rec[t + 0x9c];
        }
        *(uint16_t *)(rec + 0x156) = *(uint16_t *)(rec + 0x154);

        (*counter)--;
        bp--;
        *(uint16_t *)(rec + 2 * si + 0x4c) = 0;
        return bp;
    }

    first = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    second = *FAR_PTR(ds, bp);
    bp++;
    (*counter)++;

    if (first == 0x50) {
        if (second == 0x7f)
            second = SND8(0x202);
        rec[0x15f] = second;
        sx_param_349(second);
        return bp;
    }

    if (first == 0x60) {
        if (SND8(0x209) == 0)
            (*(uint16_t *)(rec + 0x152))++;
        return bp;
    }

    if (first == 0x52 && rec[0x15a] == second) {
        for (t = 0; t < 0x20; t += 2)
            *(uint16_t *)(rec + t + 0xc) = 0;
    }

    return bp;
}

/*
 * 0x2817a
 *
 * A one-instruction forwarder to `skip_unknown_event`. It exists so that the
 * dispatch that reaches it has an entry of its own rather than sharing one.
 */
uint16_t midi_skip_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                         uint16_t si, uint16_t ax)
{
    return skip_unknown_event(ds, bp, es, bx, si, ax);
}

/*
 * 0x2828e
 *
 * Step the cursor past an event this module does not handle, using MIDI's own
 * rule for how long a message is - which is why it only needs the status byte
 * in AH and never looks at the data.
 *
 * A status of 0xf0 is system exclusive and has no fixed length: bytes are
 * consumed until 0xf7, and **the terminator is counted too**, so the cursor
 * ends past it rather than on it. A malformed stream with no 0xf7 runs off the
 * end; nothing bounds this loop.
 *
 * 0xc0 and 0xd0 - program change and channel pressure - carry one data byte.
 * Everything else carries two. That is the standard rule and the reason the two
 * cases share their second read: the two-byte path falls through into the
 * one-byte path rather than repeating it.
 *
 * Every byte consumed bumps the per-byte counter at `+0xc + 2 * si`, so an
 * unhandled event still costs the channel exactly what it read.
 */
uint16_t skip_unknown_event(uint16_t ds, uint16_t bp, uint16_t es, uint16_t bx,
                            uint16_t si, uint16_t ax)
{
    uint16_t *counter = (uint16_t *)FAR_PTR(es, (uint16_t)(bx + 2 * si + 0xc));
    uint8_t status = (uint8_t)(ax >> 8);
    uint8_t b;

    if (status == 0xf0) {
        do {
            b = *FAR_PTR(ds, bp);
            bp++;
            (*counter)++;
        } while (b != 0xf7);
        return bp;
    }

    if (status != 0xc0 && status != 0xd0) {
        bp++;
        (*counter)++;
    }

    bp++;
    (*counter)++;

    return bp;
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
 * 0x28559
 *
 * The ordinary-call face of `silence_driver`. It loads `ES:AX` from the stack
 * argument, and `silence_driver` reads neither - the same dead argument as in
 * 0x2846a, and kept here for the same reason.
 */
void silence_driver_far(uint16_t off, uint16_t seg)
{
    (void)off;
    (void)seg;
    silence_driver();
}

/*
 * 0x28580
 *
 * Load one numbered sound module. Answers 1 if it is there, 0 if not.
 *
 * The name is built in place: the template `SSM:000:` at DGROUP 0x4a08 with its
 * three digits overwritten from the number at the caller's pointer. Each digit
 * comes from its own division - hundreds, then tens by dividing twice by ten,
 * then units - so the three are worked out independently rather than by one
 * loop.
 *
 * A number of 0xff means there is nothing to load and the answer is 1
 * regardless, which is how the caller's list is terminated.
 *
 * Whatever was loaded before is freed first, as kind 1, and the new block kept
 * at DGROUP 0x4a84. It is then handed to `configure_driver_far` past its first
 * record - `advance_record` steps over the header - and an answer of 0xffff
 * from that is a failure. The block is freed again on the way out either way:
 * this loads a module to configure the driver with, not to keep.
 */
uint16_t load_sound_module(uint16_t handle, uint16_t number, uint16_t index)
{
    int16_t di = 1;
    int16_t n;

    if (DGU16(number) == 0xff)
        goto out;

    n = DG16(number);
    DG8(0x4a0c) = (uint8_t)((n / 100) + 0x30);
    DG8(0x4a0d) = (uint8_t)(((n / 10) % 10) + 0x30);
    DG8(0x4a0e) = (uint8_t)((n % 10) + 0x30);

    if (DGU16(0x4a84) != 0 || DGU16(0x4a86) != 0)
        free_for_kind(DGU16(0x4a84), DGU16(0x4a86), 1);

    {
        uint32_t p = load_named_chunk(handle, 0x4a08, index);

        DG16(0x4a86) = (int16_t)(p >> 16);
        DG16(0x4a84) = (int16_t)p;
        if (p == 0)
            di = 0;
    }

out:
    if (di != 0) {
        uint16_t off = DGU16(0x4a84);
        uint16_t seg = DGU16(0x4a86);
        uint16_t next = advance_record(FAR_PTR(seg, off), off);

        if (configure_driver_far(next, seg) == 0xffff)
            di = 0;
    }

    if (DGU16(0x4a84) != 0 || DGU16(0x4a86) != 0) {
        free_for_kind(DGU16(0x4a84), DGU16(0x4a86), 1);
        DG16(0x4a86) = 0;
        DG16(0x4a84) = 0;
    }

    return (uint16_t)di;
}

/*
 * 0x28655
 *
 * Set up the sound device: load its **module** and then its **driver**, and
 * answer 0 if both worked and 1 if either did not.
 *
 * Two names are built the same way - `string_copy_far` puts one of the strings
 * named by the tables at DGROUP 0x4a2e and 0x4a1c into the buffer at 0x4a16,
 * which the template at 0x4a12 is the head of - and `load_named_chunk` reads
 * the chunk of that name.
 *
 * The module goes to DGROUP 0x4a98 and becomes **loaded code**: 0x4aaa marks it
 * present and `set_sound_callback` points the module's own dispatcher at it,
 * after which calls through it are calls into a block that is not part of this
 * binary at all - for `ASB:` the port has that block, in
 * reconstruct/src/sxovl_asb.c, and `call_sound_module` reaches it.
 *
 * **A module does not replace the driver.** Both halves run: the module is
 * loaded and installed, and then the device's driver is loaded too. So a
 * digitised module and a music device are a pair rather than alternatives, and
 * nothing in the game ties a particular module to a particular device - the two
 * bytes of RESOURCE.CFG are independent indices into the tables at 0x4a2e and
 * 0x4a1c.
 *
 * The driver goes to 0x4a94 and is installed with `install_driver_far`, whose
 * answer is kept at 0x4a82 as the number `load_sound_module` then looks up.
 *
 * A device of 8 is recorded as 3 at DGROUP 0x4aae, which is the number
 * `load_sound_bank` later switches on. An argument of -2 skips a load
 * entirely, and a failed load rewrites the argument to -2 so the second half
 * skips too.
 *
 * The answer is the sense of the failure flag turned round by
 * `neg`/`sbb`/`inc` - a compiler writing `!di` without a branch.
 */
uint16_t setup_sound_device(int16_t device, int16_t module_index,
                            uint16_t callback, uint16_t handle)
{
    int16_t di = 0;

    if (module_index != -2) {
        uint32_t p;

        string_copy_far(0x4a16, DGU16((uint16_t)(0x4a2e + 2 * module_index)));

        p = load_named_chunk(handle, 0x4a12, 0);
        DG16(0x4a9a) = (int16_t)(p >> 16);
        DG16(0x4a98) = (int16_t)p;

        if (p == 0) {
            module_index = -2;
            di = 1;
        } else {
            DG16(0x4aaa) = 1;
            set_sound_callback(DGU16(0x4a98), DGU16(0x4a9a));

            /*
             * **And then on to the driver, whatever this answers.** The call
             * at 0x286b7 is `sub_0bb98(callback, 1)`; a non-zero answer jumps
             * straight to the driver half keeping the module, and a zero one
             * takes the module down again - 0x4aaa cleared, 0x0bbc6 told to
             * stop, `free_for_kind`, the pointers zeroed - and *then* goes to
             * the driver half. Either way the driver is loaded.
             *
             * The port used to `return 1` here, which said a module supersedes
             * the driver. It does not: they are a pair, and that is why the two
             * bytes of RESOURCE.CFG are independent indices into two tables.
             * The mistake was invisible because the stub below aborts before
             * reaching it, and it had been written into the comment above and
             * into docs/sound-driver.md as though it were a finding.
             */
            if (sound_module_install(callback, 1) == 0) {
                DG16(0x4aaa) = 0;
                stop_loaded_module();
                free_for_kind(DGU16(0x4a98), DGU16(0x4a9a), 1);
                DG16(0x4a9a) = 0;
                DG16(0x4a98) = 0;
                module_index = -2;
                di = 1;
            }
        }
    }

    if (device != -2) {
        uint32_t p;

        string_copy_far(0x4a16, DGU16((uint16_t)(0x4a1c + 2 * device)));

        p = load_named_chunk(handle, 0x4a12, 0);
        DG16(0x4a96) = (int16_t)(p >> 16);
        DG16(0x4a94) = (int16_t)p;

        if (p == 0) {
            di = 1;
        } else {
            DG16(0x4a82) = (int16_t)(install_driver_far(DGU16(0x4a94),
                                                        DGU16(0x4a96)) & 0xff);

            if (load_sound_module(handle, 0x4a82, 0) == 0) {
                free_for_kind(DGU16(0x4a94), DGU16(0x4a96), 1);
                DG16(0x4a96) = 0;
                DG16(0x4a94) = 0;
                di = 1;
            }
        }

        if (device == 8)
            device = 3;
    }

    DG16(0x4aae) = device;
    return (uint16_t)(di == 0 ? 1 : 0);
}

/*
 * 0x287ad
 *
 * Which of the seven voices is playing a given sequence. The argument is the
 * sequence's far pointer; the answer is the voice's record, also as a far
 * pointer in DX:AX, or null.
 *
 * The seven voices are a table of far pointers at DGROUP 0x6414, four bytes
 * apart. A voice matches when the pointer it keeps at its own +0x166 equals the
 * one asked for **and** the byte at +0x158 is not 0xff - the second test is
 * what excludes a voice that still remembers a sequence it has stopped
 * playing.
 *
 * The original re-loads the table entry twice more after the `les`, and keeps
 * ES from the first load while doing so. That is only a compiler making the
 * same address three times, not three different pointers.
 */
uint32_t voice_playing(uint16_t off, uint16_t seg)
{
    int16_t i;

    for (i = 0; i < 7; i++) {
        uint16_t voff = DGU16(0x6414 + 4 * i);
        uint16_t vseg = DGU16(0x6416 + 4 * i);
        const uint8_t *rec = FAR_PTR(vseg, voff);

        if (*(uint16_t *)(rec + 0x168) != seg
            || *(uint16_t *)(rec + 0x166) != off)
            continue;
        if (*FAR_PTR(vseg, (uint16_t)(voff + 0x158)) == 0xff)
            continue;
        return ((uint32_t)vseg << 16) | voff;
    }

    return 0;
}

/*
 * 0x28800
 *
 * Allocate the seven voice records - 0x17a bytes each, kind 2 - and put them in
 * the table at DGROUP 0x6414.
 *
 * A table that is **already** filled is refused with 0, not accepted as work
 * already done: the test is on the first entry only, and the answer is the
 * failure code. So this is called once.
 *
 * Each record is marked free with 0xff at +0x158 and given a far pointer at +8
 * to its own +0x16a. If any allocation fails the whole table is handed back
 * through `free_voice_records` - including the entries this loop has not
 * reached, which are whatever they were before.
 */
uint16_t alloc_voice_records(void)
{
    int16_t i;

    if (DGU16(0x6414) != 0 || DGU16(0x6416) != 0)
        return 0;

    for (i = 0; i < 7; i++) {
        uint32_t p = alloc_for_kind(0x17a, 0, 2);
        uint16_t voff, vseg;
        uint8_t *voice;

        DG16(0x6416 + 4 * i) = (int16_t)(p >> 16);
        DG16(0x6414 + 4 * i) = (int16_t)p;

        if (p == 0) {
            free_voice_records();
            return 0;
        }

        voff = DGU16(0x6414 + 4 * i);
        vseg = DGU16(0x6416 + 4 * i);
        voice = FAR_PTR(vseg, voff);

        voice[0x158] = 0xff;
        *(uint16_t *)(voice + 0xa) = vseg;
        *(uint16_t *)(voice + 8) = (uint16_t)(voff + 0x16a);
    }

    return 1;
}

/*
 * 0x28886
 *
 * Load a named chunk out of a file, and answer it as a far pointer or null.
 *
 * The file may arrive as a handle or as a name: `file_record_valid` decides
 * which, and a name is opened here - the flag in the first local remembering
 * that this routine owns it and has to close it again.
 *
 * `seek_named_chunk` positions the file at the chunk and is refused on -1:-1.
 * `file_record_size` then answers the size, and it is handed straight to
 * `load_resource_block` as the size to open with - and the two values pushed
 * before it, a kind of 1 and a null out-pointer, are left on the stack across
 * that call, which is why only two bytes are cleaned after the size.
 *
 * A file this routine opened is closed on every path, including the ones that
 * give up; one it was handed is left alone.
 */
uint32_t load_named_chunk(uint16_t handle, uint16_t path, uint16_t index)
{
    uint16_t opened = 0;
    uint16_t si;
    uint32_t r = 0;

    if (file_record_valid(handle) == 0) {
        opened = 1;
        si = open_file_record(handle);
    } else {
        si = handle;
    }

    if (si != 0) {
        uint32_t p = seek_named_chunk(si, path, (int16_t)index);

        if (p != 0xffffffffu) {
            uint32_t size = file_record_size(si);

            r = load_resource_block(si, (uint16_t)size,
                                    (uint16_t)(size >> 16), 0, 1);
        }
    }

    if (opened != 0)
        close_file_record(si);

    return r;
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
 * 0x28431
 *
 * The ordinary-call face of `set_master_level`. The level arrives as a word on
 * the stack and goes into `CX`; only `CL` is read. DS, DI and SI are saved
 * around the call, as in every wrapper in this band.
 */
void set_master_level_far(uint16_t level)
{
    set_master_level((uint8_t)level);
}

/*
 * 0x28458
 *
 * The ordinary-call face of `install_driver`. The driver's far pointer arrives
 * on the stack and is loaded into `ES:AX` with one `les`, and `AX` comes back
 * out untouched, so this returns what `install_driver` did.
 */
uint16_t install_driver_far(uint16_t off, uint16_t seg)
{
    return install_driver(off, seg);
}

/*
 * 0x2846a
 *
 * The ordinary-call face of `configure_driver`. It loads `ES:AX` from the
 * stack argument and zeroes `BX` before the call, but `configure_driver` reads
 * neither - so the argument is dead and the port takes it only to keep the
 * signature the callers use. `AX` passes back out, and 0x28580 reads it.
 */
uint16_t configure_driver_far(uint16_t off, uint16_t seg)
{
    (void)off;
    (void)seg;
    return configure_driver();
}

/*
 * 0x284ef
 *
 * The ordinary-call face of `retire_and_tick`, which reads its record from
 * `ES:AX` - loaded here from the stack argument with one `les`.
 */
void retire_and_tick_far(uint16_t off, uint16_t seg)
{
    retire_and_tick(seg, off);
}

/*
 * 0x28480
 *
 * The ordinary-call face of `start_sequence`. That routine takes its record in
 * `es:ax` and its flag in `cx`, which no C caller can arrange, so this takes
 * them on the stack and puts them in registers.
 *
 * It also saves DS, DI and SI around the call. `start_sequence` restores what
 * it changes, but the ones this pushes are the ones a C caller expects to keep,
 * and the hand-written routine makes no such promise.
 */
void start_sequence_far(uint16_t off, uint16_t seg, uint16_t flag)
{
    start_sequence(seg, off, flag);
}

/*
 * 0x28935
 *
 * Build a sequence record around a block of note data, and answer it as a far
 * pointer - or a null one if there was no room.
 *
 * The record is 0x17a bytes of kind 2, so `alloc_for_kind` zeroes it; every
 * field not written below is therefore known to be zero rather than merely
 * assumed so.
 *
 * The source pointer is kept at +0x166, and +0x16a gets the same pointer
 * stepped past the first record by `advance_record` - the segment half is
 * carried across unchanged, because that routine only moves the offset.
 *
 * +8 is then made to point at **+0x16a of the record itself**, so the cursor
 * the sequencer follows lives inside the record and starts at the second entry.
 * That is why the record's own segment is stored beside it at +0xa.
 *
 * +0x15e is set to 0x7f, which `sequencer_tick` reads as "use the default"
 * where it feeds `scale_byte_pair`, and the two words at +0x172 are cleared
 * again although the allocation already did it.
 */
uint32_t create_sequence(uint16_t src_off, uint16_t src_seg)
{
    uint32_t p = alloc_for_kind(0x17a, 0, 2);
    uint16_t off = (uint16_t)p, seg = (uint16_t)(p >> 16);
    uint8_t *rec;
    uint16_t stepped;

    if ((off | seg) == 0)
        return 0;

    rec = FAR_PTR(seg, off);

    *(uint16_t *)(rec + 0x168) = src_seg;
    *(uint16_t *)(rec + 0x166) = src_off;

    stepped = advance_record(FAR_PTR(src_seg, src_off), src_off);
    *(uint16_t *)(rec + 0x16c) = src_seg;
    *(uint16_t *)(rec + 0x16a) = stepped;

    *(uint16_t *)(rec + 0xa) = seg;
    *(uint16_t *)(rec + 8) = (uint16_t)(off + 0x16a);

    rec[0x15e] = 0x7f;
    *(uint16_t *)(rec + 0x174) = 0;
    *(uint16_t *)(rec + 0x172) = 0;

    return p;
}
/*
 * 0x289e8
 *
 * Load the sound bank for whatever device is configured, and answer it as a far
 * pointer, or null.
 *
 * The device at DGROUP 0x4aae picks the identifier to look for. Four of the
 * cases go through a jump table in this code segment at `cs:0x2a17`, which is
 * how a Borland `switch` over a small dense range is compiled, and the rest are
 * compares - 5 and 6 landing on the same answers as 1 and 2, 7 on its own, and
 * 0x7e taking the identifier from DGROUP 0x4a9e instead of a constant. Anything
 * else answers null without opening the resource at all.
 *
 * Then the resource is opened under the name at DGROUP 0x4a7e, walked to that
 * record, and its items read into a node list.
 *
 * The block to hold them is sized by walking that list: six bytes of directory
 * per node plus five, rounded up to even and to at least 0x26, and then the
 * items' own lengths on top - and one more byte, which is the `add dx,1` before
 * the allocation. That rounded directory size is also where the items start, so
 * it is handed to `build_sound_index` as the same number.
 *
 * DGROUP 0x4a9c is set to 2 on the two failures that mean the resource was
 * there but the bank was not - a missing record, or a list that came back
 * empty. An allocation that fails does not set it.
 *
 * The node list is freed on every path, and the resource closed on every path
 * that opened it.
 */
uint32_t load_sound_bank(uint16_t file, uint16_t size_lo, uint16_t size_hi,
                         uint16_t out)
{
    uint16_t want;
    int16_t handle;
    uint16_t list_off = 0, list_seg = 0;
    uint16_t blk_off = 0, blk_seg = 0;
    uint32_t r = 0;

    /*
     * None of this routine's locals has its address taken, but the ones it
     * calls do, and their frames have to land **below** this one. So the frame
     * is reserved anyway: 0x12 bytes of locals and the two saved registers.
     */
    dg_enter(0x16);

    switch (DGU16(0x4aae)) {
    case 0:    want = 0x12; break;
    case 1:    want = 0x13; break;
    case 2:    want = 0;    break;
    case 3:    want = 0xc;  break;
    case 5:    want = 0x13; break;
    case 6:    want = 0;    break;
    case 7:    want = 7;    break;
    case 0x7e: want = DG8(0x4a9e); break;
    default:   goto out;
    }

    handle = open_resource(0, file, 0x4a7e, size_lo, size_hi);
    if (handle < 0)
        goto out;

    dg_call(8);                           /* two arguments and a far return */
    if (seek_to_sound_record(handle, want) == 0) {
        dg_uncall(8);
        DG16(0x4a9c) = 2;
        close_resource(handle);
        goto out;
    }
    dg_uncall(8);

    {
        uint32_t p;

        dg_call(6);                       /* one argument and a far return */
        p = read_sound_records(handle);
        dg_uncall(6);

        list_off = (uint16_t)p;
        list_seg = (uint16_t)(p >> 16);
        if (p == 0) {
            DG16(0x4a9c) = 2;
            close_resource(handle);
            goto out;
        }
    }

    {
        uint16_t off = list_off, seg = list_seg;
        uint16_t len_lo = 0, len_hi = 0;
        uint16_t si = 5;

        while (off != 0 || seg != 0) {
            const uint8_t *node = FAR_PTR(seg, off);
            uint16_t n = *(uint16_t *)(node + 2);

            len_lo = (uint16_t)(len_lo + n);
            if (len_lo < n)
                len_hi = (uint16_t)(len_hi + 1);

            si = (uint16_t)(si + 6);
            seg = *(uint16_t *)(node + 6);
            off = *(uint16_t *)(node + 4);
        }

        if ((si & 1) != 0)
            si++;
        if (si < 0x26)
            si = 0x26;

        len_lo = (uint16_t)(len_lo + si);
        if (len_lo < si)
            len_hi = (uint16_t)(len_hi + 1);

        {
            uint32_t p = alloc_for_kind((uint16_t)(len_lo + 1),
                                        (uint16_t)(len_hi
                                                   + (len_lo + 1 > 0xffff
                                                      ? 1 : 0)),
                                        4);

            blk_off = (uint16_t)p;
            blk_seg = (uint16_t)(p >> 16);
            if (p == 0) {
                close_resource(handle);
                free_node_list(list_off, list_seg);
                goto out;
            }
        }

        if (build_sound_index(handle, list_off, list_seg, blk_off, blk_seg,
                              si, want) == 0) {
            close_resource(handle);
            free_node_list(list_off, list_seg);
            goto out;
        }

        free_node_list(list_off, list_seg);

        if (out != 0) {
            DG16(out + 2) = (int16_t)len_hi;
            DG16(out) = (int16_t)len_lo;
        }
    }

    close_resource(handle);
    r = ((uint32_t)blk_seg << 16) | blk_off;

out:
    dg_leave(0x16);
    return r;
}

/*
 * 0x28baf
 *
 * Free a whole chain of nodes, each linked to the next by the far pointer at
 * its +4, and all of them kind 9.
 *
 * The next pointer is read out **before** the node is freed, into the routine's
 * own locals; reading it afterwards would be following a pointer into a block
 * that has just been given back. The original does this by overwriting its own
 * two argument words with the next pointer and freeing the copy it kept, so the
 * argument is also the loop variable.
 *
 * A null chain is not a special case - the test is at the top.
 */
void free_node_list(uint16_t off, uint16_t seg)
{
    while (off != 0 || seg != 0) {
        uint16_t cur_off = off, cur_seg = seg;
        const uint8_t *node = FAR_PTR(seg, off);

        off = *(uint16_t *)(node + 4);
        seg = *(uint16_t *)(node + 6);

        free_for_kind(cur_off, cur_seg, 9);
    }
}

/*
 * 0x29106
 *
 * Give the seven voice records back to the allocator, as kind 2.
 *
 * The whole table is skipped when its **first** entry is null, and the answer
 * is 0 rather than 1 - so an uninitialised table is reported as a failure
 * rather than as nothing to do. Each entry is then tested again inside the
 * loop, which is what makes a hole in the middle harmless.
 *
 * The table itself is not cleared. What clears it is not this routine.
 */
uint16_t free_voice_records(void)
{
    int16_t i;

    if (DGU16(0x6414) == 0 && DGU16(0x6416) == 0)
        return 0;

    for (i = 0; i < 7; i++) {
        uint16_t voff = DGU16(0x6414 + 4 * i);
        uint16_t vseg = DGU16(0x6416 + 4 * i);

        if (voff == 0 && vseg == 0)
            continue;
        free_for_kind(voff, vseg, 2);
    }

    return 1;
}

/*
 * 0x29152
 *
 * Give a sequence to the first free voice and start it.
 *
 * Free means 0xff at +0x158 - the same mark `stop_all_voices` writes. The voice
 * then remembers the sequence twice: the pointer it was given, at +0x166, and
 * the record **after** it, at +0x16a, which is where playing begins.
 *
 * Three bytes of per-voice state are set from one of two places. When the table
 * at DGROUP 0x4a92 exists, +0x15d and +0x15c come out of it as a pair - two
 * bytes per index - and +0x15e is 0x7f. When it does not, the three come from
 * the caller instead: +0x15d from the third argument, +0x15c is 1, and +0x15e
 * is the index itself. So the table, when present, overrides what the caller
 * asked for.
 *
 * Answers the voice as a far pointer, or 0 if the sequence was null or every
 * voice was busy.
 */
uint32_t start_on_free_voice(uint16_t off, uint16_t seg, uint16_t index,
                             uint16_t byte_arg)
{
    int16_t i;

    if (off == 0 && seg == 0)
        return 0;

    for (i = 0; i < 7; i++) {
        uint16_t voff = DGU16(0x6414 + 4 * i);
        uint16_t vseg = DGU16(0x6416 + 4 * i);
        uint8_t *voice = FAR_PTR(vseg, voff);
        uint16_t next;

        if (voice[0x158] != 0xff)
            continue;

        *(uint16_t *)(voice + 0x168) = seg;
        *(uint16_t *)(voice + 0x166) = off;

        next = advance_record(FAR_PTR(seg, off), off);
        *(uint16_t *)(voice + 0x16c) = seg;
        *(uint16_t *)(voice + 0x16a) = next;

        if (DGU16(0x4a92) != 0) {
            uint16_t p = (uint16_t)(DGU16(0x4a92) + 2 * index);

            voice[0x15d] = DG8(p);
            voice[0x15c] = DG8(p + 1);
            voice[0x15e] = 0x7f;
        } else {
            voice[0x15d] = (uint8_t)byte_arg;
            voice[0x15c] = 1;
            voice[0x15e] = (uint8_t)index;
        }

        start_sequence_far(voff, vseg, 0);
        return ((uint32_t)vseg << 16) | voff;
    }

    return 0;
}

/*
 * 0x2923d
 *
 * Retire every voice that is still marked as playing. The seven-entry table at
 * DGROUP 0x6414 again, the 0xff at +0x158 as the mark, and
 * `retire_and_tick_far` as the retirement - the same three pieces as
 * `stop_voice_playing`, over all of them rather than one.
 */
void stop_all_voices(void)
{
    int16_t i;

    for (i = 0; i < 7; i++) {
        uint16_t voff = DGU16(0x6414 + 4 * i);
        uint16_t vseg = DGU16(0x6416 + 4 * i);

        if (*FAR_PTR(vseg, (uint16_t)(voff + 0x158)) == 0xff)
            continue;

        retire_and_tick_far(voff, vseg);
        *FAR_PTR(vseg, (uint16_t)(voff + 0x158)) = 0xff;
    }
}

/*
 * 0x2928c
 *
 * Install the host callback: a far pointer written into this module's own code
 * segment at `cs:0x30f6`, which is the cell `sound_callback` calls through.
 *
 * `AX` is pushed and popped around the two stores, so the caller's `AX`
 * survives - the routine has no return value of its own.
 */
void set_sound_callback(uint16_t off, uint16_t seg)
{
    SND16(0x30f6) = (int16_t)off;
    SND16(0x30f8) = (int16_t)seg;
}

/*
 * 0x292a1
 *
 * Call the host's sound callback, if one is installed, and answer what it
 * returned.
 *
 * The vector is the far pointer at the module's `cs:0x30f6` and it is only
 * called when the word at DGROUP 0x4aaa says a callback exists. With none
 * installed the routine still answers - AX is untouched from entry, so the
 * caller gets back whatever it passed in.
 *
 * The answer is parked at `cs:0x30fa` before the registers are popped and read
 * back afterwards, because the pops would otherwise destroy it. That is why a
 * routine that appears to return AX has a global in the middle of it.
 *
 * Everything is saved, flags included, because a callback is arbitrary code.
 * The port takes only the register input: the two stack arguments are read
 * solely on the path that calls the callback, and calling an arbitrary guest
 * function pointer is not something the port can do.
 */
uint16_t sound_callback(uint16_t ax)
{
    if (DG16(0x4aaa) != 0)
        not_transcribed("the sound module's installed callback, cs:0x30f6");

    SND16(0x30fa) = (int16_t)ax;
    return (uint16_t)SND16(0x30fa);
}

/*
 * 0x289ba
 *
 * Follow a chain to its end and, if anything is there, retire and tick.
 *
 * The original writes `follow_far_chain`'s answer back over its own stack
 * arguments before testing it, which is a compiler reusing the incoming slots
 * as a local and not a second meaning for them.
 */
void follow_then_tick(uint16_t off, uint16_t seg, int16_t count)
{
    uint32_t p = follow_far_chain(off, seg, count);

    if (p != 0)
        retire_and_tick_far((uint16_t)p, (uint16_t)(p >> 16));
}

/*
 * 0x28bf2
 *
 * Walk a resource's record list looking for one with a given identifier.
 * Answers 1 if it stopped on it, 0 for anything else.
 *
 * The resource opens with 0x84 and one more byte; anything else and this gives
 * up at once. After that it is a list of records, each an identifier byte
 * followed by items terminated by 0xff, and each item five bytes long - which
 * are stepped over with `resource_seek` rather than read, since only the
 * identifiers matter here.
 *
 * An identifier of 0xff ends the list and is the failure. Every read that does
 * not answer 1 is also a failure, so a truncated resource stops rather than
 * running on.
 *
 * The three bytes it reads into are locals, and their addresses are handed to
 * `read_resource` as `SS:offset` - which in this program is a DGROUP address,
 * so the port puts them on the guest stack. See `dg_enter` in dgroup.h.
 *
 * The name is a guess from the shape; what the records are is not established
 * here.
 */
uint16_t seek_to_sound_record(int16_t handle, uint16_t want)
{
    uint16_t fp = dg_enter(6);            /* four bytes of locals, and SI */
    uint16_t bp = (uint16_t)(fp + 6);
    uint16_t b3 = (uint16_t)(bp - 3);
    uint16_t b2 = (uint16_t)(bp - 2);
    uint16_t b1 = (uint16_t)(bp - 1);
    uint16_t r = 0;

    if (read_resource(handle, b3, DGROUP_SEG, 1) != 1)
        goto out;
    if (DG8(b3) != 0x84)
        goto out;
    if (read_resource(handle, b3, DGROUP_SEG, 1) != 1)
        goto out;
    if (read_resource(handle, b1, DGROUP_SEG, 1) != 1)
        goto out;

    for (;;) {
        if (DG8(b1) == (uint8_t)want) {
            r = 1;
            goto out;
        }

        if (DG8(b1) == 0xff)
            goto out;
        if (read_resource(handle, b2, DGROUP_SEG, 1) != 1)
            goto out;

        while (DG8(b2) != 0xff) {
            resource_seek(handle, 5, 0, 1);
            if (read_resource(handle, b2, DGROUP_SEG, 1) != 1)
                goto out;
        }

        if (read_resource(handle, b1, DGROUP_SEG, 1) != 1)
            goto out;
    }

out:
    dg_leave(6);
    return r;
}

/*
 * 0x28cf7
 *
 * Read a run of four-byte items out of a resource into an ordered list of
 * eight-byte nodes, and answer the head.
 *
 * Each item is preceded by a byte that is read *ahead* - once before the loop
 * and once at the end of each turn - so the terminating 0xff is seen before a
 * node is allocated for it. One byte is skipped before each item, which is what
 * `resource_seek` with a whence of 1 is doing.
 *
 * A node is 8 bytes of kind 9: four read from the resource and a link at +4
 * that is cleared first, which is the layout `insert_by_key` expects - it
 * orders on the word at +0 and links at +4.
 *
 * The first node becomes the head outright; every later one goes through
 * `insert_by_key`, which can move the head.
 *
 * If an allocation fails part-way the whole list is freed and null answered.
 * That is the only path on which the byte read ahead is not 0xff, which is what
 * the second test distinguishes.
 */
uint32_t read_sound_records(int16_t handle)
{
    uint16_t fp = dg_enter(0xc);          /* ten bytes of locals, and SI */
    uint16_t b = (uint16_t)(fp + 0xc - 1);
    uint16_t head_off = 0, head_seg = 0;
    uint16_t node_off = 0, node_seg = 0;

    read_resource(handle, b, DGROUP_SEG, 1);

    for (;;) {
        uint32_t p;

        if (DG8(b) == 0xff)
            break;

        p = alloc_for_kind(8, 0, 9);
        node_off = (uint16_t)p;
        node_seg = (uint16_t)(p >> 16);
        if (p == 0)
            break;

        *(uint16_t *)FAR_PTR(node_seg, (uint16_t)(node_off + 6)) = 0;
        *(uint16_t *)FAR_PTR(node_seg, (uint16_t)(node_off + 4)) = 0;

        resource_seek(handle, 1, 0, 1);
        read_resource(handle, node_off, node_seg, 4);
        read_resource(handle, b, DGROUP_SEG, 1);

        if (head_off == 0 && head_seg == 0) {
            head_off = node_off;
            head_seg = node_seg;
        } else {
            uint32_t h = insert_by_key(head_off, head_seg,
                                       node_off, node_seg);

            head_seg = (uint16_t)(h >> 16);
            head_off = (uint16_t)h;
        }
    }

    if (DG8(b) != 0xff)
        free_node_list(head_off, head_seg);

    dg_leave(0xc);
    return ((uint32_t)head_seg << 16) | head_off;
}

/*
 * 0x28ddb
 *
 * Insert a node into a list kept in ascending order of the word at its +0. The
 * link is at +4, as a far pointer, and the answer is the head - which changes
 * only when the new node goes in front of it.
 *
 * An empty list is answered unchanged: the routine has nowhere to put the node
 * and does not make it the head. Whether that is deliberate or an oversight is
 * not established; it is transcribed as it stands.
 *
 * The walk keeps `prev` and `cur` a step apart and stops at the first node
 * whose key is not below the new one, so equal keys go **after** the ones
 * already there.
 */
uint32_t insert_by_key(uint16_t head_off, uint16_t head_seg,
                       uint16_t node_off, uint16_t node_seg)
{
    uint16_t cur_off, cur_seg, prev_off, prev_seg;
    uint16_t key = *(uint16_t *)FAR_PTR(node_seg, node_off);

    if (head_off == 0 && head_seg == 0)
        return ((uint32_t)head_seg << 16) | head_off;

    if (*(uint16_t *)FAR_PTR(head_seg, head_off) >= key) {
        uint8_t *node = FAR_PTR(node_seg, node_off);

        *(uint16_t *)(node + 6) = head_seg;
        *(uint16_t *)(node + 4) = head_off;
        return ((uint32_t)node_seg << 16) | node_off;
    }

    cur_off = prev_off = head_off;
    cur_seg = prev_seg = head_seg;

    for (;;) {
        const uint8_t *cur;

        prev_off = cur_off;
        prev_seg = cur_seg;

        cur = FAR_PTR(cur_seg, cur_off);
        cur_seg = *(uint16_t *)(cur + 6);
        cur_off = *(uint16_t *)(cur + 4);

        if (cur_off == 0 && cur_seg == 0)
            break;
        if (*(uint16_t *)FAR_PTR(cur_seg, cur_off) >= key)
            break;
    }

    {
        uint8_t *node = FAR_PTR(node_seg, node_off);
        uint8_t *prev = FAR_PTR(prev_seg, prev_off);

        *(uint16_t *)(node + 6) = cur_seg;
        *(uint16_t *)(node + 4) = cur_off;
        *(uint16_t *)(prev + 6) = node_seg;
        *(uint16_t *)(prev + 4) = node_off;
    }

    return ((uint32_t)head_seg << 16) | head_off;
}

/*
 * 0x28e87
 *
 * Gather the items a node list names into one block: a small directory at the
 * front and the items themselves behind it.
 *
 * The block opens the way `seek_to_sound_record` expects to find it - 0x84, a
 * zero, and a tag byte from the caller - and then carries six bytes per node:
 * two zeros, the item's offset within the block **less two**, and its length.
 * A 0xffff ends the directory.
 *
 * The items go to a second cursor that starts the caller's given distance into
 * the block, so the directory and the data grow towards each other from known
 * ends rather than being sized first.
 *
 * Each item is read by seeking the resource to the node's own offset plus two -
 * from the start, not from where the last read left off - and reading its
 * length. A short read abandons the whole thing and answers 0.
 */
uint16_t build_sound_index(int16_t handle, uint16_t list_off,
                           uint16_t list_seg, uint16_t dst_off,
                           uint16_t dst_seg, uint16_t data_at, uint16_t tag)
{
    uint16_t dir = dst_off;
    uint16_t data = (uint16_t)(dst_off + data_at);

    *FAR_PTR(dst_seg, dir++) = 0x84;
    *FAR_PTR(dst_seg, dir++) = 0;
    *FAR_PTR(dst_seg, dir++) = (uint8_t)tag;

    while (list_off != 0 || list_seg != 0) {
        const uint8_t *node = FAR_PTR(list_seg, list_off);
        uint16_t len = *(uint16_t *)(node + 2);
        uint8_t *e = FAR_PTR(dst_seg, dir);

        e[0] = 0;
        e[1] = 0;
        *(uint16_t *)(e + 2) = (uint16_t)(data - dst_off - 2);
        *(uint16_t *)(e + 4) = len;

        resource_seek(handle, (uint16_t)(*(uint16_t *)node + 2), 0, 0);

        if ((uint16_t)read_resource(handle, data, dst_seg, len) != len)
            return 0;

        data = (uint16_t)(data + len);
        node = FAR_PTR(list_seg, list_off);
        list_seg = *(uint16_t *)(node + 6);
        list_off = *(uint16_t *)(node + 4);
        dir = (uint16_t)(dir + 6);
    }

    *(uint16_t *)FAR_PTR(dst_seg, dir) = 0xffff;
    return 1;
}

/*
 * 0x28f74
 *
 * Load a whole resource into a fresh block and answer it as a far pointer, or
 * null.
 *
 * The resource is opened under the name at DGROUP 0x4a80, its size asked for,
 * a block of exactly that size allocated of the caller's kind, and the whole
 * thing read in. A short read - or any size at all in the high half - frees the
 * block and answers null, so a partial resource is never handed back.
 *
 * The resource is closed on every path that opened it, including the failures.
 *
 * The optional pointer in the fourth argument is filled with the size, but only
 * when there is a block to go with it.
 */
uint32_t load_resource_block(uint16_t file, uint16_t size_lo,
                             uint16_t size_hi, uint16_t out, uint16_t kind)
{
    uint16_t buf_off = 0, buf_seg = 0;
    uint16_t len_lo = 0, len_hi = 0;
    int16_t handle;

    handle = open_resource(0, file, 0x4a80, size_lo, size_hi);

    if (handle >= 0) {
        uint32_t sz = resource_size(handle);
        uint32_t p;

        len_lo = (uint16_t)sz;
        len_hi = (uint16_t)(sz >> 16);

        p = alloc_for_kind(len_lo, len_hi, kind);
        buf_off = (uint16_t)p;
        buf_seg = (uint16_t)(p >> 16);

        if (p != 0) {
            uint16_t got = (uint16_t)read_resource(handle, buf_off,
                                                   buf_seg, len_lo);

            if (len_hi != 0 || got != len_lo) {
                free_for_kind(buf_off, buf_seg, kind);
                buf_off = 0;
                buf_seg = 0;
            }
        }

        close_resource(handle);
    }

    if (out != 0 && (buf_off != 0 || buf_seg != 0)) {
        DG16(out + 2) = (int16_t)len_hi;
        DG16(out) = (int16_t)len_lo;
    }

    return ((uint32_t)buf_seg << 16) | buf_off;
}

/*
 * 0x29034
 *
 * Load a sequence and start it: follow the chain of far pointers to the record,
 * set its default volume, and hand it to `start_sequence`.
 *
 * The far pointer that comes back is written **into the caller's own first two
 * argument words** before anything else uses it, so those arguments are both
 * input and output - the caller sees the located record even though the value
 * is also returned in DX:AX.
 *
 * A null result is answered as a null far pointer without touching anything
 * else. Otherwise +0x15e takes the fourth argument, which is the byte
 * `sequencer_tick` feeds to `scale_byte_pair` as the sequence's own volume, and
 * the sequence is started with the flag set - so `start_sequence` will write 2
 * to +0x159 and mark every channel as needing its own voice.
 */
uint32_t load_and_start_sequence(uint16_t off, uint16_t seg, int16_t count,
                                 uint16_t volume)
{
    uint32_t p = follow_far_chain(off, seg, count);
    uint16_t r_off = (uint16_t)p, r_seg = (uint16_t)(p >> 16);

    if ((r_off | r_seg) == 0)
        return 0;

    *FAR_PTR(r_seg, (uint16_t)(r_off + 0x15e)) = (uint8_t)volume;

    start_sequence_far(r_off, r_seg, 1);

    return ((uint32_t)r_seg << 16) | r_off;
}

/*
 * 0x290ab
 *
 * Stop whichever voice is playing a given sequence. The same seven-entry table
 * at DGROUP 0x6414 that `voice_playing` searches, and the same match on the far
 * pointer at +0x166 - but this one does not test +0x158 first, so a voice
 * already marked stopped is retired and marked again.
 *
 * It returns after the first match: nothing here handles a second voice on the
 * same sequence, which is the assumption that a sequence has one.
 */
void stop_voice_playing(uint16_t off, uint16_t seg)
{
    int16_t i;

    for (i = 0; i < 7; i++) {
        uint16_t voff = DGU16(0x6414 + 4 * i);
        uint16_t vseg = DGU16(0x6416 + 4 * i);
        const uint8_t *rec = FAR_PTR(vseg, voff);

        if (*(uint16_t *)(rec + 0x168) != seg
            || *(uint16_t *)(rec + 0x166) != off)
            continue;

        retire_and_tick_far(voff, vseg);
        *FAR_PTR(vseg, (uint16_t)(voff + 0x158)) = 0xff;
        return;
    }
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
 * 0x292f4
 *
 * Shut the sound down: silence the driver, let whatever is playing finish, and
 * give both blocks back.
 *
 * How it waits depends on whether the sequencer's timer callback is
 * registered - DGROUP 0x4a8e. With it registered the tick is running and
 * `delay_five_ticks` is enough; without it nothing is driving the sequencer, so
 * `sound_service` is called twice by hand instead.
 *
 * `silence_driver_far` is called with **no arguments at all**, which is safe
 * only because it reads none - the same dead argument 0x2846a has.
 *
 * The loaded module is told to stop through its own dispatcher at 0x0bbc6, a
 * call into a block that is not part of this binary. Not reached here, and left
 * as a stub.
 */
void stop_sound(void)
{
    if (DGU16(0x4a94) != 0 || DGU16(0x4a96) != 0) {
        silence_driver_far(0, 0);

        if (DG16(0x4a8e) == 0) {
            sound_service();
            sound_service();
        } else {
            delay_five_ticks();
        }
    }

    if (DGU16(0x4a98) != 0 || DGU16(0x4a9a) != 0) {
        stop_loaded_module();
    }

    if (DGU16(0x4a94) != 0 || DGU16(0x4a96) != 0) {
        free_for_kind(DGU16(0x4a94), DGU16(0x4a96), 1);
        DG16(0x4a96) = 0;
        DG16(0x4a94) = 0;
    }

    if (DGU16(0x4a98) != 0 || DGU16(0x4a9a) != 0) {
        free_for_kind(DGU16(0x4a98), DGU16(0x4a9a), 1);
        DG16(0x4a9a) = 0;
        DG16(0x4a98) = 0;
    }
}

/*
 * 0x2937f
 *
 * Wait five timer ticks. A counter at DGROUP 0x6430 is set to five, a callback
 * registered at four ticks a time, and the routine **spins** until the callback
 * has counted it down; then the slot is given back.
 *
 * The far pointer it registers is this module's own `cs:0x3228`, which is
 * `tick_delay` below. The segment is a relocated constant in the image, so the
 * port works it out from where the module actually is rather than using the
 * 0x2619 the bytes read.
 *
 * The spin only ends because the timer interrupt runs the callback, so in the
 * port it ends only when something drives the timer - the same standing as
 * `wait_and_latch_frame`. Nothing reaches it on these screens.
 */
void delay_five_ticks(void)
{
    uint16_t handle;

    DG16(0x6430) = 5;

    handle = timer_add_callback(0x3228, (uint16_t)(SNDCS >> 4), 4);

    while (DG16(0x6430) > 0)
        ;

    timer_drop_callback(handle);
}

/*
 * 0x293b8
 *
 * The callback `delay_five_ticks` registers: one instruction of work, counting
 * DGROUP 0x6430 down by one each tick.
 */
void tick_delay(void)
{
    DG16(0x6430) = (int16_t)(DGU16(0x6430) - 1);
}

/*
 * 0x293c1
 *
 * Unlink records from the list at DGROUP 0x4a88 and give them back. The
 * selector is the one `next_matching_record` uses - 0 for every record, -1 and
 * -2 for the two families, anything else an identifier - but the matching is
 * open-coded here rather than borrowed, because this walk has to hold on to the
 * *previous* node and the shared iterator cannot.
 *
 * `stop_all_voices` runs first, but only for 0 and -2. An identifier or -1 does
 * not silence anything up front; each record is stopped individually by
 * `stop_sequences` as it is reached.
 *
 * The previous link starts as a **local**: `mov [bp-2],ss` puts the stack
 * segment beside the offset of a two-word cell at `bp-0x1c`, so the first
 * unlink writes into that scratch rather than into a real node, and reading it
 * back gives the next record. Since SS is DGROUP the cell is an ordinary
 * DGROUP address, which is why the port needs a guest stack of its own - see
 * `dg_enter` in dgroup.h.
 *
 * The list head is fixed up separately, by comparing against it rather than by
 * treating it as another link.
 *
 * Each record costs three frees: its +4 pointer as kind 4 or kind 7 depending
 * on bit 0 of +0x12, and the record itself as kind 3.
 *
 * A positive selector stops after the first match. The two families and 0 walk
 * to the end.
 */
uint16_t remove_and_free_records(int16_t selector)
{
    uint16_t fp = dg_enter(0x1c);
    uint16_t link_off = fp;
    uint16_t link_seg = DGROUP_SEG;
    uint16_t cur_off = DGU16(0x4a88);
    uint16_t cur_seg = DGU16(0x4a8a);
    int16_t found = 0;

    if (selector == 0 || selector == -2)
        stop_all_voices();

    while (cur_off != 0 || cur_seg != 0) {
        uint8_t *cur = FAR_PTR(cur_seg, cur_off);
        int16_t match;

        if (selector == 0)
            match = 1;
        else if (*(int16_t *)(cur + 0xa) == selector)
            match = 1;
        else if (selector == -1)
            match = (*(uint16_t *)(cur + 0x12) & 1) != 0;
        else if (selector == -2)
            match = (*(uint16_t *)(cur + 0x12) & 1) == 0;
        else
            match = 0;

        if (match) {
            uint8_t *link;

            found = 1;
            stop_sequences(*(int16_t *)(cur + 0xa));

            cur = FAR_PTR(cur_seg, cur_off);
            if (cur_seg == DGU16(0x4a8a) && cur_off == DGU16(0x4a88)) {
                DG16(0x4a8a) = *(int16_t *)(cur + 2);
                DG16(0x4a88) = *(int16_t *)cur;
            }

            link = FAR_PTR(link_seg, link_off);
            *(uint16_t *)(link + 2) = *(uint16_t *)(cur + 2);
            *(uint16_t *)link = *(uint16_t *)cur;

            if ((*(uint16_t *)(cur + 0x12) & 1) != 0)
                free_for_kind(*(uint16_t *)(cur + 4),
                              *(uint16_t *)(cur + 6), 4);
            else
                free_for_kind(*(uint16_t *)(cur + 4),
                              *(uint16_t *)(cur + 6), 7);

            free_for_kind(cur_off, cur_seg, 3);

            if (selector > 0)
                break;
        } else {
            link_off = cur_off;
            link_seg = cur_seg;
        }

        {
            const uint8_t *link = FAR_PTR(link_seg, link_off);

            cur_seg = *(uint16_t *)(link + 2);
            cur_off = *(uint16_t *)link;
        }
    }

    dg_leave(0x1c);
    return (uint16_t)found;
}

/*
 * 0x294ff
 *
 * Stop sequences. Which ones is the selector, and it is the same vocabulary
 * `next_matching_record` uses: -1 for the ones with bit 0 of +0x12 set, -2 for
 * the ones without, 0 for both, and anything else names one by its identifier.
 *
 * Stopping a record of the first family means letting its voice finish -
 * `follow_then_tick`, then a **busy wait** on that voice's +0x158 until it
 * reads 0xff - and then giving the voice back as kind 2 and clearing +0xe. The
 * wait is not a spin against an interrupt: `follow_then_tick` runs the
 * sequencer itself, and it is the sequencer that writes the 0xff.
 *
 * That branch then abandons the walk by clearing the cursor - one record of
 * that family is stopped per call, not all of them - while a record with
 * nothing at +0xe simply has its 0x10 bit cleared and the walk continues.
 *
 * The second family is only ever cleared of 0x10; what actually silences it is
 * the `stop_all_voices` at the end. A selector of 0 does the first family and
 * then falls into the second, which is why the `-1` case returns early and the
 * `0` case does not.
 *
 * An identifier selects one record and takes whichever of the two paths its
 * bit 0 says, and answers 0 when there is no such record - the only path here
 * that reports failure.
 */
uint16_t stop_sequences(int16_t selector)
{
    uint16_t off, seg;
    uint32_t p;

    if (selector == -1 || selector == 0) {
        p = next_matching_record(-1);
        for (;;) {
            uint8_t *rec;

            off = (uint16_t)p;
            seg = (uint16_t)(p >> 16);
            if (off == 0 && seg == 0)
                break;

            rec = FAR_PTR(seg, off);
            *(uint16_t *)(rec + 0x12) &= 0xffef;

            if (*(uint16_t *)(rec + 0xe) != 0
                || *(uint16_t *)(rec + 0x10) != 0) {
                uint16_t voff = *(uint16_t *)(rec + 0xe);
                uint16_t vseg = *(uint16_t *)(rec + 0x10);

                follow_then_tick(voff, vseg, 0);

                do {
                    rec = FAR_PTR(seg, off);
                    voff = *(uint16_t *)(rec + 0xe);
                    vseg = *(uint16_t *)(rec + 0x10);
                } while (*FAR_PTR(vseg, (uint16_t)(voff + 0x158)) != 0xff);

                free_for_kind(voff, vseg, 2);
                rec = FAR_PTR(seg, off);
                *(uint16_t *)(rec + 0x10) = 0;
                *(uint16_t *)(rec + 0xe) = 0;
                p = 0;
            } else {
                p = next_matching_record(-3);
            }
        }

        if (selector == -1)
            return 1;
    }

    if (selector == -1 || selector == 0 || selector == -2) {
        p = next_matching_record(-2);
        for (;;) {
            uint8_t *rec;

            off = (uint16_t)p;
            seg = (uint16_t)(p >> 16);
            if (off == 0 && seg == 0)
                break;

            rec = FAR_PTR(seg, off);
            *(uint16_t *)(rec + 0x12) &= 0xffef;
            p = next_matching_record(-3);
        }

        stop_all_voices();
        return 1;
    }

    p = next_matching_record(selector);
    off = (uint16_t)p;
    seg = (uint16_t)(p >> 16);
    if (p == 0)
        return 0;

    {
        uint8_t *rec = FAR_PTR(seg, off);

        *(uint16_t *)(rec + 0x12) &= 0xffef;

        if ((*(uint16_t *)(rec + 0x12) & 1) == 0) {
            stop_voice_playing(*(uint16_t *)(rec + 4),
                               *(uint16_t *)(rec + 6));
            return 1;
        }

        if (*(uint16_t *)(rec + 0xe) != 0
            || *(uint16_t *)(rec + 0x10) != 0) {
            uint16_t voff = *(uint16_t *)(rec + 0xe);
            uint16_t vseg = *(uint16_t *)(rec + 0x10);

            follow_then_tick(voff, vseg, 0);

            do {
                rec = FAR_PTR(seg, off);
                voff = *(uint16_t *)(rec + 0xe);
                vseg = *(uint16_t *)(rec + 0x10);
            } while (*FAR_PTR(vseg, (uint16_t)(voff + 0x158)) != 0xff);

            free_for_kind(voff, vseg, 2);
            rec = FAR_PTR(seg, off);
            *(uint16_t *)(rec + 0x10) = 0;
            *(uint16_t *)(rec + 0xe) = 0;
        }
    }

    return 1;
}

/*
 * 0x296b4
 *
 * Open a sound file and load either one record out of it or all of them.
 * Answers the handle it used, or 0.
 *
 * The first argument is a handle **or** a name: `file_record_valid` decides
 * which, and a name is opened here and remembered as ours to close - which is
 * what DGROUP 0x4aa8 records, beside the handle at 0x4aa6.
 *
 * Asking again for the same handle with a positive identifier short-circuits
 * to the search, so a second record out of an already-open file costs nothing.
 *
 * The directory is at offset 0xc of the file: a 32-bit size, then a block four
 * bytes longer read whole, whose first word must be 2. Its +6 is the number of
 * six-byte entries and +8 a byte handed to `read_record`; each entry is an
 * identifier and a 32-bit offset.
 *
 * A positive identifier loads one record, a non-positive one loads them all in
 * order. **The one-record search leaves its offset uninitialised when nothing
 * matches**, and the test that follows reads whatever the stack held; the port
 * keeps those two words on the guest stack for that reason rather than in C
 * locals.
 *
 * Every failure runs the same cleanup: close the file if this routine opened
 * it, free the directory, and throw away every record read so far.
 */
uint16_t open_sound_file(uint16_t handle, int16_t id)
{
    uint16_t fp = dg_enter(0x10);
    uint16_t bp = (uint16_t)(fp + 0x10);
    uint16_t found = (uint16_t)(bp - 4);    /* [bp-4]:[bp-2] */
    uint16_t size = (uint16_t)(bp - 8);     /* [bp-8]:[bp-6] */
    uint16_t cur = (uint16_t)(bp - 0xc);    /* [bp-0xc]:[bp-0xa] */
    int16_t si;
    uint16_t r = 0;

    if (id != 0 && handle == DGU16(0x4aa6) && DGU16(0x4aa6) != 0)
        goto search;

    if (DGU16(0x4aa6) != handle && DGU16(0x4aa8) != 0)
        close_file_record(DGU16(0x4aa6));

    DG16(0x4aa6) = 0;
    DG16(0x4aa8) = 0;

    if (file_record_valid(handle) != 0) {
        DG16(0x4aa6) = (int16_t)handle;
    } else {
        DG16(0x4aa6) = (int16_t)open_file_record(handle);
        if (DGU16(0x4aa6) == 0)
            goto fail;
        DG16(0x4aa8) = 1;
    }

    dg_call(6);                           /* one argument and a far return */
    remove_and_free_records(0);
    dg_uncall(6);

    game_fseek(DGU16(0x4aa6), 0xc, 0, 0);

    if (game_fread(size, 4, 1, DGU16(0x4aa6)) != 1)
        goto fail;

    if (DGU16(0x4aa2) != 0 || DGU16(0x4aa4) != 0)
        free_for_kind(DGU16(0x4aa2), DGU16(0x4aa4), 0xa);

    {
        uint16_t lo = (uint16_t)(DGU16(size) + 4);
        uint32_t p = alloc_for_kind(lo,
                                    (uint16_t)(DGU16(size + 2)
                                               + (lo < 4 ? 1 : 0)),
                                    0xa);

        DG16(0x4aa4) = (int16_t)(p >> 16);
        DG16(0x4aa2) = (int16_t)p;
        if (p == 0)
            goto fail;
    }

    if (fread_huge((uint16_t)(DGU16(0x4aa2) + 4), DGU16(0x4aa4),
                   DGU16(size), DGU16(size + 2), 1, 0,
                   DGU16(0x4aa6)) != 1)
        goto fail;

    if (*(uint16_t *)FAR_PTR(DGU16(0x4aa4),
                             (uint16_t)(DGU16(0x4aa2) + 4)) != 2)
        goto fail;

    {
        uint8_t *hdr = FAR_PTR(DGU16(0x4aa4), DGU16(0x4aa2));

        *(uint16_t *)(hdr + 2) = DGU16(0x4aa4);
        *(uint16_t *)hdr = (uint16_t)(DGU16(0x4aa2) + 9);
    }

search:
    if (id > 0 && next_matching_record(id) != 0) {
        r = DGU16(0x4aa6);
        goto out;
    }

    {
        const uint8_t *hdr = FAR_PTR(DGU16(0x4aa4), DGU16(0x4aa2));

        DG16(cur + 2) = (int16_t)*(uint16_t *)(hdr + 2);
        DG16(cur) = (int16_t)*(uint16_t *)hdr;
    }

    if (id > 0) {
        for (si = 0; ; si++) {
            const uint8_t *hdr = FAR_PTR(DGU16(0x4aa4), DGU16(0x4aa2));
            const uint8_t *e;

            if (*(int16_t *)(hdr + 6) <= si)
                break;

            e = FAR_PTR(DGU16(cur + 2), DGU16(cur));
            if (*(int16_t *)e == id) {
                DG16(found + 2) = (int16_t)*(uint16_t *)(e + 4);
                DG16(found) = (int16_t)*(uint16_t *)(e + 2);
                break;
            }
            DG16(cur) = (int16_t)(DGU16(cur) + 6);
        }

        {
            uint16_t lo = (uint16_t)(DGU16(found) + 4);

            if (game_fseek(DGU16(0x4aa6), lo,
                           (uint16_t)(DGU16(found + 2) + (lo < 4 ? 1 : 0)),
                           0) != 0)
                goto fail;
        }

        if (DGU16(found) == 0 && DGU16(found + 2) == 0)
            goto fail;

        {
            uint16_t ok;

            dg_call(8);                   /* two arguments and a far return */
            ok = read_record(DGU16(0x4aa6),
                             *FAR_PTR(DGU16(0x4aa4),
                                      (uint16_t)(DGU16(0x4aa2) + 8)));
            dg_uncall(8);
            if (ok == 0)
                goto out;
        }

        r = DGU16(0x4aa6);
        goto out;
    }

    for (si = 0; ; si++) {
        const uint8_t *hdr = FAR_PTR(DGU16(0x4aa4), DGU16(0x4aa2));
        const uint8_t *e;
        uint16_t lo;

        if (*(int16_t *)(hdr + 6) <= si)
            break;

        e = FAR_PTR(DGU16(cur + 2), DGU16(cur));
        lo = (uint16_t)(*(uint16_t *)(e + 2) + 4);

        if (game_fseek(DGU16(0x4aa6), lo,
                       (uint16_t)(*(uint16_t *)(e + 4) + (lo < 4 ? 1 : 0)),
                       0) != 0)
            goto fail;

        {
            uint16_t ok;

            dg_call(8);                   /* two arguments and a far return */
            ok = read_record(DGU16(0x4aa6),
                             *FAR_PTR(DGU16(0x4aa4),
                                      (uint16_t)(DGU16(0x4aa2) + 8)));
            dg_uncall(8);
            if (ok == 0)
                goto fail;
        }

        DG16(cur) = (int16_t)(DGU16(cur) + 6);
    }

    r = DGU16(0x4aa6);
    goto out;

fail:
    if (DGU16(0x4aa6) != 0 && DGU16(0x4aa8) != 0)
        close_file_record(DGU16(0x4aa6));

    if (DGU16(0x4aa2) != 0 || DGU16(0x4aa4) != 0)
        free_for_kind(DGU16(0x4aa2), DGU16(0x4aa4), 0xa);

    dg_call(6);                           /* one argument and a far return */
    remove_and_free_records(0);
    dg_uncall(6);

    DG16(0x4aa6) = 0;
    DG16(0x4aa4) = 0;
    DG16(0x4aa2) = 0;
    r = 0;

out:
    dg_leave(0x10);
    return r;
}

/*
 * 0x296a1
 *
 * Set the master level and answer 1. The 1 is unconditional - nothing below
 * reports failure, so this cannot either.
 */
uint16_t set_master_level_ok(uint16_t level)
{
    set_master_level_far(level);
    return 1;
}

/*
 * 0x29a49
 *
 * Start the sequence with a given identifier, loading it if it is not loaded
 * yet. Answers 1 for "it is playing or there is nothing to do", 0 for a
 * failure to load.
 *
 * The list at DGROUP 0x4a88 is walked by hand rather than through
 * `next_matching_record`, because that iterator has one shared cursor and this
 * routine walks the list a second time inside itself.
 *
 * Three conditions each mean there is nothing to do, and all three answer 1:
 * the 0x10 bit already set at +0x12, no source at +4, or something already
 * loaded at +0xe.
 *
 * Then the two families part. A record with bit 0 set - music - first stops
 * **every other** loaded record of the same family, so only one plays at a
 * time. If DGROUP 0x4aa0 is 0 or -1 it stops there and marks 0x10 without
 * loading anything, which is how a disabled device still leaves the game
 * believing the music started. Otherwise `create_sequence` builds it from the
 * source at +4, two bytes are copied into the built sequence at +0x15c and
 * +0x15d - the second from +0xc, the first from bit 1 of +0x12 - and
 * `load_and_start_sequence` starts it at level 0x7f.
 *
 * A record without bit 0 - an effect - asks `voice_playing` whether its source
 * is already on a voice, and answers 1 if it is. If not, 0x4aa0 being 0 or -2
 * is again the disabled case, and otherwise `start_on_free_voice` places it,
 * again at 0x7f, with bit 1 of +0x12 as the byte argument.
 */
uint16_t start_sequence_by_id(int16_t id)
{
    uint16_t off = DGU16(0x4a88);
    uint16_t seg = DGU16(0x4a8a);
    uint8_t *rec;

    while (off != 0 || seg != 0) {
        rec = FAR_PTR(seg, off);
        if (*(int16_t *)(rec + 0xa) == id)
            break;
        seg = *(uint16_t *)(rec + 2);
        off = *(uint16_t *)rec;
    }

    if (off == 0 && seg == 0)
        return 0;

    rec = FAR_PTR(seg, off);

    if ((*(uint16_t *)(rec + 0x12) & 0x10) != 0)
        return 1;
    if (*(uint16_t *)(rec + 4) == 0 && *(uint16_t *)(rec + 6) == 0)
        return 1;
    if (*(uint16_t *)(rec + 0xe) != 0 || *(uint16_t *)(rec + 0x10) != 0)
        return 1;

    if ((*(uint16_t *)(rec + 0x12) & 1) != 0) {
        uint16_t other_off = DGU16(0x4a88);
        uint16_t other_seg = DGU16(0x4a8a);

        while (other_off != 0 || other_seg != 0) {
            uint8_t *other = FAR_PTR(other_seg, other_off);

            if ((*(uint16_t *)(other + 0x12) & 1) != 0
                && (*(uint16_t *)(other + 0xe) != 0
                    || *(uint16_t *)(other + 0x10) != 0)
                && *(int16_t *)(other + 0xa) != id)
                stop_sequences(*(int16_t *)(other + 0xa));

            other_seg = *(uint16_t *)(other + 2);
            other_off = *(uint16_t *)other;
        }

        rec = FAR_PTR(seg, off);

        if (DG16(0x4aa0) == 0 || DG16(0x4aa0) == -1) {
            *(uint16_t *)(rec + 0x12) |= 0x10;
            return 1;
        }

        {
            uint32_t built = create_sequence(*(uint16_t *)(rec + 4),
                                             *(uint16_t *)(rec + 6));
            uint16_t boff, bseg;
            uint8_t *seq;

            *(uint16_t *)(rec + 0x10) = (uint16_t)(built >> 16);
            *(uint16_t *)(rec + 0xe) = (uint16_t)built;
            if (built == 0)
                return 0;

            boff = *(uint16_t *)(rec + 0xe);
            bseg = *(uint16_t *)(rec + 0x10);
            seq = FAR_PTR(bseg, boff);

            seq[0x15d] = (uint8_t)((*(uint16_t *)(rec + 0x12) & 2) ? 1 : 0);
            seq[0x15c] = rec[0xc];

            if (load_and_start_sequence(boff, bseg, 0, 0x7f) == 0)
                return 0;
            return 1;
        }
    }

    if (voice_playing(*(uint16_t *)(rec + 4), *(uint16_t *)(rec + 6)) != 0)
        return 1;

    if (DG16(0x4aa0) == 0 || DG16(0x4aa0) == -2) {
        if ((*(uint16_t *)(rec + 0x12) & 2) != 0)
            *(uint16_t *)(rec + 0x12) |= 0x10;
        return 1;
    }

    start_on_free_voice(*(uint16_t *)(rec + 4), *(uint16_t *)(rec + 6),
                        0x7f,
                        (uint16_t)((*(uint16_t *)(rec + 0x12) & 2) ? 1 : 0));
    return 1;
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

/*
 * 0x29c3b
 *
 * Start the sound system. Answers 1 if it came up, 0 if it did not - and 1
 * again, immediately, if either the driver at DGROUP 0x4a94 or the module at
 * 0x4a98 is already loaded, so this cannot run twice.
 *
 * A device of -1 means "no sound": the device becomes 2 and the flag that
 * drives everything after is cleared, so `setup_sound_device` still runs but
 * nothing is installed on the back of it.
 *
 * With sound wanted, three things follow. The timer is taken over at rate 0xd
 * unless something already has it - DGROUP 0x44ee - and 0x4a8c records that.
 * The sequencer's own tick is registered as a callback at rate 4, keeping its
 * slot at 0x4a8e; **the segment it registers is a relocation**, reading 0x2619
 * in the image, so the port works it out from where the module is. And a third
 * callback goes to the loaded module's own dispatcher, at 0x0bba6 in segment 0,
 * but only if that module loaded - which it does not here.
 *
 * `alloc_voice_records` is last, and its answer is not looked at.
 */
uint16_t start_sound(int16_t device, int16_t module_index, uint16_t callback,
                     uint16_t handle)
{
    int16_t si = 1;

    if (DGU16(0x4a94) != 0 || DGU16(0x4a96) != 0
        || DGU16(0x4a98) != 0 || DGU16(0x4a9a) != 0)
        return 1;

    if (device == -1) {
        device = 2;
        si = 0;
    }

    if (setup_sound_device(device, module_index, callback, handle) == 0)
        return 0;

    if (si != 0 && (int16_t)(int8_t)DG8(0x44ee) == 0) {
        timer_install(0xd);
        DG16(0x4a8c) = 1;
    }

    if (si != 0) {
        DG16(0x4a8e) = (int16_t)timer_add_callback(0x193e,
                                                   (uint16_t)(SNDCS >> 4), 4);
        if (DGU16(0x4a8e) == 0 && si != 0)
            return 0;
    } else if (si != 0) {
        return 0;
    }

    if (si != 0 && (DGU16(0x4a98) != 0 || DGU16(0x4a9a) != 0))
        DG16(0x4a90) = (int16_t)timer_add_callback(0xbba6,
                                                   (uint16_t)(IMAGE_BASE >> 4),
                                                   2);

    alloc_voice_records();
    return 1;
}

/*
 * 0x29cf6
 *
 * Take the whole sound system down, in the reverse order `start_sound` built
 * it up. Does nothing at all if neither the driver nor the module is loaded.
 *
 * Everything is released and its slot zeroed as it goes: the records and their
 * payloads, the directory at DGROUP 0x4aa2, the file at 0x4aa6 if this module
 * opened it, the two timer callbacks at 0x4a8e and 0x4a90, and the timer itself
 * if 0x4a8c says it was taken. Then the voice records, and `stop_sound` last.
 *
 * `free_voice_records` answers whether it found a table to free, and that
 * answer is ignored - so a system that never allocated one comes down just as
 * quietly.
 */
void shutdown_sound(void)
{
    if (DGU16(0x4a94) == 0 && DGU16(0x4a96) == 0
        && DGU16(0x4a98) == 0 && DGU16(0x4a9a) == 0)
        return;

    remove_and_free_records(0);

    if (DGU16(0x4aa2) != 0 || DGU16(0x4aa4) != 0)
        free_for_kind(DGU16(0x4aa2), DGU16(0x4aa4), 0xa);

    if (DGU16(0x4aa6) != 0 && DGU16(0x4aa8) != 0)
        close_file_record(DGU16(0x4aa6));

    if (DG16(0x4a8e) != 0) {
        timer_drop_callback(DGU16(0x4a8e));
        DG16(0x4a8e) = 0;
    }

    if (DG16(0x4a90) != 0) {
        timer_drop_callback(DGU16(0x4a90));
        DG16(0x4a90) = 0;
    }

    if (DG16(0x4a8c) != 0) {
        timer_remove();
        DG16(0x4a8c) = 0;
    }

    free_voice_records();
    stop_sound();
}

/*
 * 0x29da0
 *
 * Read one record's header out of a file, load whatever it points at, and put
 * the record on the front of the list at DGROUP 0x4a88. Answers 1, or 0 if
 * anything failed.
 *
 * The header is four fields read one after another: a 32-bit length, then an
 * identifier word into +0xa, then two bytes into +0xc and +0x12. Bit 0 of that
 * last one is what makes the payload kind 4 rather than kind 7 - the same two
 * kinds `remove_and_free_records` frees by.
 *
 * The length then has **four taken off it**, because the identifier and the two
 * bytes were part of it.
 *
 * Where the payload comes from is three cases. A second argument of 0x63 means
 * it is raw: a block of its own and `fread_huge` straight into it. Otherwise
 * DGROUP 0x4aac chooses between `load_sound_bank`, which selects a record for
 * the configured device, and `load_resource_block`, which takes the resource
 * whole.
 *
 * Any failure frees the record as kind 3 and answers 0; the payload's own
 * pointer is left where it was written, which is null on every path that gets
 * there.
 */
uint16_t read_record(uint16_t file, uint16_t mode)
{
    uint16_t fp = dg_enter(0x12);         /* 0xe of locals, and SI and DI */
    uint16_t bp = (uint16_t)(fp + 0x12);
    uint16_t len = (uint16_t)(bp - 4);    /* the 32-bit length */
    uint16_t out = (uint16_t)(bp - 8);
    uint16_t scratch = (uint16_t)(bp - 0xe);
    uint16_t rec_off, rec_seg, kind;
    uint32_t p;
    uint16_t r = 0;

    game_fread(len, 4, 1, file);
    game_fread(scratch, 2, 1, file);

    p = alloc_for_kind(0x14, 0, 3);
    rec_off = (uint16_t)p;
    rec_seg = (uint16_t)(p >> 16);
    if (p == 0)
        goto out_;

    *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 0xa)) = DGU16(scratch);

    game_fread(scratch, 1, 1, file);
    *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 0xc)) = DG8(scratch);

    game_fread(scratch, 1, 1, file);
    *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 0x12)) = DG8(scratch);

    kind = (*(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 0x12)) & 1)
           ? 4 : 7;

    if (DGU16(len) < 4)
        DG16(len + 2) = (int16_t)(DGU16(len + 2) - 1);
    DG16(len) = (int16_t)(DGU16(len) - 4);

    *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 6)) = 0;
    *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 4)) = 0;

    if ((uint8_t)mode == 0x63) {
        p = alloc_for_kind(DGU16(len), DGU16(len + 2), kind);
        *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 6)) =
            (uint16_t)(p >> 16);
        *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 4)) = (uint16_t)p;

        if (p == 0)
            goto fail;

        if (fread_huge((uint16_t)p, (uint16_t)(p >> 16),
                       DGU16(len), DGU16(len + 2), 1, 0, file) != 1)
            goto fail;
    } else if (DG16(0x4aac) != 0) {
        dg_call(0xe);                     /* five arguments and a far return */
        p = load_sound_bank(file, DGU16(len), DGU16(len + 2), out);
        dg_uncall(0xe);

        *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 6)) =
            (uint16_t)(p >> 16);
        *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 4)) = (uint16_t)p;
        if (p == 0)
            goto fail;
    } else {
        dg_call(0xe);                     /* five arguments and a far return */
        p = load_resource_block(file, DGU16(len), DGU16(len + 2), out, kind);
        dg_uncall(0xe);

        *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 6)) =
            (uint16_t)(p >> 16);
        *(uint16_t *)FAR_PTR(rec_seg, (uint16_t)(rec_off + 4)) = (uint16_t)p;
        if (p == 0)
            goto fail;
    }

    {
        uint8_t *rec = FAR_PTR(rec_seg, rec_off);

        *(uint16_t *)(rec + 2) = DGU16(0x4a8a);
        *(uint16_t *)rec = DGU16(0x4a88);
        *(uint16_t *)(rec + 8) = DGU16(out);
    }

    DG16(0x4a8a) = (int16_t)rec_seg;
    DG16(0x4a88) = (int16_t)rec_off;
    r = 1;
    goto out_;

fail:
    free_for_kind(rec_off, rec_seg, 3);

out_:
    dg_leave(0x12);
    return r;
}

/*
 * 0x29f89
 *
 * Allocate a block for the sound module, choosing where from by a `kind`
 * argument, and zero it for some kinds but not others.
 *
 * Kinds 6 and 8 come from the C runtime's own heap - a near pointer, with the
 * data segment supplied as the segment half - and everything else from DOS
 * through `dos_alloc_bytes`. The two are not interchangeable: only the DOS path
 * can hand back more than a segment, and only the heap path gives a pointer the
 * runtime can later free.
 *
 * Kinds 2, 3, 4 and 7 are then zeroed with `far_memset`. Note that 6 and 8 are
 * not among them, so a heap block comes back holding whatever was there - and
 * the zeroing is skipped entirely when the allocation failed, which is the only
 * thing the null check guards.
 *
 * `malloc` is not transcribed. The runtime's heap is a deliberate non-goal, and
 * the port refuses rather than inventing a pointer it could not also give a
 * block header to; see `io_malloc`. Kinds 6 and 8 are not reached on the
 * screens checked, so the rest of this verifies.
 */
uint32_t alloc_for_kind(uint16_t size_lo, uint16_t size_hi, uint16_t kind)
{
    uint16_t off, seg;
    uint32_t p;

    if (kind == 6 || kind == 8) {
        off = io_malloc(size_lo);
        seg = 0;                      /* the original supplies DS here */
        p = ((uint32_t)seg << 16) | off;
    } else {
        p = dos_alloc_bytes(size_lo, size_hi, 0, 0);
    }

    off = (uint16_t)p;
    seg = (uint16_t)(p >> 16);

    if ((off | seg) != 0
        && (kind == 2 || kind == 3 || kind == 4 || kind == 7))
        far_memset(off, seg, 0, size_lo, size_hi);

    return p;
}

/*
 * 0x2a017
 *
 * Release a block the sound module allocated, and the exact counterpart of
 * `alloc_for_kind` at 0x29f89: the same `kind` argument picks the same two
 * places, kinds 6 and 8 going back to the C runtime's heap and everything else
 * to DOS.
 *
 * The kind is not stored with the block, so it is the caller's job to release
 * one with the same kind it asked for. Passing the wrong one hands a heap
 * pointer to DOS or a DOS segment to `free`, and nothing here would notice.
 *
 * `free` is not transcribed, for the reason `io_malloc` gives; the DOS path is
 * the one these screens take.
 */
void free_for_kind(uint16_t off, uint16_t seg, uint16_t kind)
{
    if (kind == 6 || kind == 8) {
        io_free(off);
        return;
    }
    dos_free_far(off, seg);
}

