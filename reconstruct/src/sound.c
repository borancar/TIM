/*
 * The Incredible Machine - reconstruction
 *
 * Transcribed from the binary `TIM.EXE` of The Incredible Machine
 * (Dynamix / Sierra On-Line, 1993). No licence is asserted on this file.
 *
 * **The sequencer and the sound driver's interface, which were written in
 * assembly.** This file corresponds to the first module of the original's
 * code segment 2619, image 0x26198..0x28580. The module keeps its state in
 * its own code segment (`SNDS`, placed with `SEGMENT_AT`) and reaches the
 * loaded driver through a far pointer in that segment, with the function
 * number in BP. Its routines take their arguments in registers and save what
 * they use, and its far entry points, 0x28431..0x28580, are the only ones
 * that build a C frame.
 *
 * **So no C compiler judges this file.** It is the host's transcription of a
 * hand-written module, and the byte-exact source of these bytes is TASM's to
 * make (not written yet). The C that follows it in the segment is in
 * sound_load.c, sound_device.c, sound_bank.c, sound_stop.c and sound_file.c.
 *
 * Functions are in address order and each carries the image offset it was
 * read from.
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
static void tick_program_voice(struct sequence far * seq, uint16_t voice,
                               uint16_t channel)
{
    uint8_t cl, ch;
    uint16_t bend;

    driver_controller(voice, 0x7b00);                    /* all notes off */

    cl = (uint8_t)(seq->ch.voice_budget[channel] & 0xf);
    driver_controller(voice, (uint16_t)((0x4b << 8) | cl));

    cl = seq->ch.program[channel];
    driver_program_change(voice, cl);

    SNDS.pending_volume[voice] = 0xff;

    cl = scale_byte_pair(seq->ch.volume[channel], seq->volume);
    driver_controller(voice, (uint16_t)((7 << 8) | cl));

    cl = seq->ch.pan[channel];
    driver_controller(voice, (uint16_t)((0xa << 8) | cl));

    cl = seq->ch.modulation[channel];
    driver_controller(voice, (uint16_t)((1 << 8) | cl));

    cl = 0;
    if ((uint8_t)(seq->ch.bend[channel] >> 8) >= 0x80)
        cl = 0x7f;
    driver_controller(voice, (uint16_t)((0x40 << 8) | cl));

    bend = seq->ch.bend[channel];
    ch = (uint8_t)bend;
    cl = (uint8_t)((bend >> 8) << 1);
    if (ch >= 0x80)
        cl |= 1;
    driver_pitch_bend(voice, (uint16_t)((((uint16_t)ch << 8) | cl) & 0x7f7f));

    cl = seq->ch.note[channel];
    driver_controller(voice, (uint16_t)((0x4e << 8) | cl));
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
        SNDS.saved_request[i] = SNDS.voice_request[i];
        SNDS.saved_cost[i] = SNDS.voice_cost[i];
        SNDS.saved_gives_back[i] = SNDS.voice_gives_back[i];
        SNDS.saved_keep_own[i] = SNDS.voice_keep_own[i];
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
        SNDS.voice_request[i] = SNDS.saved_request[i];
        SNDS.voice_cost[i] = SNDS.saved_cost[i];
        SNDS.voice_gives_back[i] = SNDS.saved_gives_back[i];
        SNDS.voice_keep_own[i] = SNDS.saved_keep_own[i];
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
uint16_t install_driver(const uint8_t far * drv)
{
    uint16_t ax, cx;
    uint8_t dl;

    SNDS.driver = far_of(drv);

    driver_describe_0(&ax, &cx);

    SNDS.cl = (uint8_t)cx;
    SNDS.ch = (uint8_t)(cx >> 8);

    dl = (uint8_t)((ax >> 8) >> 4);
    if (((int16_t)DG4A82.module_live) != 0)
        dl |= 1;
    SNDS.ah_high = dl;

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
uint16_t configure_driver(const uint8_t far * drv)
{
    uint16_t ax, cx;

    driver_describe_1(drv, &ax, &cx);

    SNDS.voice_lo = (uint8_t)cx;
    SNDS.voice_hi = (uint8_t)(cx >> 8);

    driver_param_349(0);

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
 *
 * **`CL` is set once, to 0xf, and both calls see it** - function 12 preserves
 * CX. That matters because `SBP:`'s function 2 reads CL where the other
 * drivers ignore it, so the 0xf is passed on rather than dropped.
 */
void silence_driver(void)
{
    driver_param_345(0xf);
    driver_stop_all(0xf);
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
    driver_param_345(cl);
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
void start_sequence(struct sequence far * seq, uint16_t cx)
{
    uint16_t di, si, bp, base;
    uint8_t dl, dh, key;

    for (di = 0; di < 0x40; di += 4) {
        if (SEQUENCE_PTR(SNDS.playing[di / 4]) == seq) {
            remove_sequence(seq);
            sequencer_tick();
            break;
        }
    }

    seq->mode = 1;
    if (cx != 0)
        seq->mode++;

    init_sequence_params(seq);

    for (si = 0; si < 0xf; si++) {
        seq->position[si] = 0xd;
        seq->position_saved[si] = 3;
        seq->delay[si] = 0;
        seq->delay_saved[si] = 0;
        seq->ch.bend[si] = 0x2000;
        seq->track_channel[si] = 0xff;
        seq->status[si] = 0;
        seq->status_saved[si] = 0;
        seq->ch.voice_budget[si] = 0xff;
        seq->ch.modulation[si] = 0;
        seq->ch.program[si] = 0xff;
        seq->ch.volume[si] = 0xff;
        seq->ch.pan[si] = 0xff;
        seq->ch.note[si] = 0xff;
        seq->ch.channel_flags[si] = 0;
        seq->ch.no_voice[si] = 0;
    }

    seq->track_channel[0xf] = 0xff;
    seq->status[0xf] = 0;
    seq->status_saved[0xf] = 0;
    seq->poll = 0;
    seq->rewind_mark = 0;
    seq->device_value = 0x7f;
    seq->position[0xf] = 0xd;
    seq->position_saved[0xf] = 3;
    seq->delay[0xf] = 0;
    seq->ticks_saved = 0;

    {
        /* Both are far pointers stored in records: the sequence's own at
           +8, and the table that one points at, each followed once. */
        const uint8_t far *tbl = *seq->cursor_at;

        if (tbl[0x20] != 0xff && seq->keep_priority == 0)
            seq->priority = tbl[0x20];

        base = 0;
        si = 0;
        bp = 0;

        for (;;) {
            uint16_t entry = *(const uint16_t *)(tbl + bp);
            const uint8_t far *e;

            if (entry == 0)
                break;

            e = tbl + base + entry;
            dl = e[0];

            if (dl == 0xfe) {
                if (SNDS.ah_high != 0) {
                    seq->poll = (uint8_t)(si + 1);
                    break;
                }
                seq->position[si] = 0;
                seq->position_saved[si] = 0;
                seq->track_channel[si] = 0xfe;
            } else {
                uint16_t channel;

                seq->track_channel[si] = dl;
                seq->status[si] = (uint8_t)(dl | 0xb0);

                dl = e[0xc];
                dh = 0;
                if (dl == 0xf8) {
                    dl = 0xf0;
                    dh = 0x80;
                }
                seq->delay[si] = (uint16_t)(((uint16_t)dh << 8) | dl);

                dl = seq->track_channel[si];
                seq->track_channel[si] &= 0xf;
                channel = (uint16_t)(dl & 0xf);

                if ((dl & 0x10) != 0) {
                    seq->position[si] = 3;
                    seq->delay[si] = 0;
                    seq->ch.channel_flags[channel] |= 2;
                } else {
                    int16_t do_f8 = 1;

                    if ((dl & 0x20) != 0)
                        seq->ch.channel_flags[channel] |= 1;
                    if ((dl & 0x40) != 0)
                        seq->ch.no_voice[channel] = 1;

                    if (channel == 0xf) {
                        if (seq->device_value == 0x7f) {
                            seq->device_value = e[8];
                            do_f8 = 0;
                        }
                    } else {
                        if (seq->ch.voice_budget[channel] == 0xff)
                            seq->ch.voice_budget[channel] = e[1];
                        if (seq->ch.program[channel] == 0xff)
                            seq->ch.program[channel] = e[4];
                        if (seq->ch.volume[channel] == 0xff)
                            seq->ch.volume[channel] = e[8];
                    }

                    if (do_f8 && seq->ch.pan[channel] == 0xff)
                        seq->ch.pan[channel] = e[0xb];
                }
            }

            si++;
            bp = (uint16_t)(2 * si);
            if (si == 0x10)
                break;
        }
    }

    if (seq->mode == 2) {
        for (di = 0xe; (int16_t)di >= 0; di--)
            seq->ch.channel_flags[di] |= 1;
    }

    key = seq->priority;

    for (di = 0; di < 0x40; di += 4) {
        if (SNDS.playing[di / 4].seg == 0)
            break;
        if (SEQUENCE_PTR(SNDS.playing[di / 4])->priority <= key) {
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
             * out of `part_step_generator` - with the new sequence's key at or
             * below the first entry's, which is what makes `di` zero.
             */
            for (si = 0x38; (uint16_t)(si + 4) != di; si -= 4) {
                SNDS.playing[si / 4 + 1] = SNDS.playing[si / 4];
            }
            break;
        }
    }
    if (di >= 0x40)
        return;

    SNDS.playing[di / 4] = far_of((uint8_t *)seq);

    if (SNDS.muted != 0)
        return;

    seq->loop_count = 0;
    seq->ticks = 0;
    seq->state = 0;
    seq->fade_target = 0;
    seq->fade_period = 0;
    seq->fade_countdown = 0;
    seq->fade_step = 0;
    seq->skip = 0;

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
void retire_and_tick(struct sequence far * seq)
{
    io_lock();                  /* `pushf`, `cli` */
    remove_sequence(seq);
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
void remove_sequence(struct sequence far * seq)
{
    int16_t i;

    /* The table's pairs are filed from pointers to DOS blocks, so comparing
       the pointer is comparing the pair `es:ax` was matched against. */
    for (i = 0; i < 0x10; i++)
        if (SEQUENCE_PTR(SNDS.playing[i]) == seq)
            break;
    if (i >= 0x10)
        return;

    SNDS.playing[i] = FAR_NULL;

    if (i != 0xf) {
        for (; i != 0xf; i++)
            SNDS.playing[i] = SNDS.playing[i + 1];
        SNDS.playing[i] = FAR_NULL;
    }

    seq->state = 0xff;
    seq->mode = 0;

    if (seq->poll == 0)
        return;
    if (seq->poll < 0x80)
        return;

    /*
     * `xor ax,ax; push ax; mov ax,5; push ax` - so the **function number is
     * 5** and the argument pointer is null. Read in source order the two come
     * out swapped, which is what this used to say.
     */
    sound_callback(5, NULL);
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
 * A voice in hand still has to be paid for. When no voice is free, or the
 * running total does not cover what the channel costs, the loudest
 * already-placed request is dropped - its voice becoming this channel's - and
 * its contribution added to the total, repeatedly, until the total covers what
 * this channel needs - so quiet requests are given up before loud ones, and
 * only as many as are actually needed. If that still is not enough the
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
    struct sequence far *rec;
    uint16_t bp_;
    uint8_t al, ah, cl, chh, dl, dh;

    SNDS.busy++;
    SNDS.voices_changed = 0;

    for (i = 0; i < 0x10; i++) {
        SNDS.voice_held[i] = 0xff;
        SNDS.voice_gives_back[i] = 0;
        SNDS.voice_keep_own[i] = 0;
        SNDS.voice_cost[i] = 0;
        SNDS.voice_request[i] = 0xff;
    }
    SNDS.polled[0] = FAR_NULL;

    rec = SEQUENCE_PTR(SNDS.playing[0]);

    if (rec == SEQUENCE_NONE) {
        for (i = 0; i < 0x10; i++)
            SNDS.voice_held[i] = 0xff;
        goto silence_unused;
    }

    cl = rec->device_value;
    if (cl == 0x7f)
        cl = SNDS.param_default;
    driver_param_349(cl);

    al = SNDS.cl;

    bp_ = 0;
    for (seq = 0; seq < 0x40; seq += 4) {
        rec = SEQUENCE_PTR(SNDS.playing[seq / 4]);
        if (rec == SEQUENCE_NONE)
            break;

        if (rec->skip != 0)
            goto next_sequence;

        if (rec->poll != 0) {
            if (dg_far_ptr(SNDS.polled[0]) != FAR_NULL_PTR)
                goto next_sequence;
            SNDS.polled[0] = SNDS.playing[seq / 4];
            goto next_sequence;
        }

        tick_save_state();
        /*
         * The running total carried across sequences is parked here, not
         * zeroed: the abandon path below reads it back so a sequence that
         * fails leaves the total exactly as it found it.
         */
        SNDS.saved_total = al;

        for (ch_i = 0; ch_i < 0x10; ch_i++) {
            cl = rec->track_channel[ch_i];
            if (cl == 0xff || cl == 0xfe || cl == 0x0f)
                continue;
            if ((rec->ch.channel_flags[cl] & 2) != 0)
                continue;
            if (rec->ch.no_voice[cl] != 0)
                continue;

            dl = (uint8_t)((seq * 4) | cl);

            ah = (uint8_t)(rec->ch.voice_budget[cl] & 0xf);
            chh = (uint8_t)(rec->ch.voice_budget[cl] >> 4);
            if (chh != 0)
                chh = (uint8_t)(0x10 - chh + bp_);

            if ((rec->ch.channel_flags[cl] & 1) != 0
                && SNDS.voice_request[cl] == 0xff) {
                dh = cl;
                goto check_budget;
            }

            dh = 0xff;
            {
                int16_t bl;

                for (bl = 0; bl < 0x10; bl++) {
                    if (SNDS.voice_request[bl] == 0xff) {
                        if (bl >= (int16_t)SNDS.voice_lo
                            && bl <= (int16_t)SNDS.voice_hi)
                            dh = (uint8_t)bl;
                    } else if (SNDS.voice_request[bl] == dl) {
                        goto next_channel;
                    }
                }
            }
            if (dh != 0xff)
                goto check_budget;

            if (chh != 0)
                goto next_sequence;
            goto drop_loudest;

            /*
             * 0x272c2: a voice is in hand, but only if what is left covers the
             * cost. A request that cannot drop anything gives up on this
             * channel; one that can drops the loudest placed request - and
             * **takes its voice**, because the scan leaves it in DH, which is
             * where the request is then put. The port kept the scan's answer
             * in a local and put the request at DH's 0xff, past the table.
             */
check_budget:
            if (ah <= al)
                goto have_voice;
            if (chh != 0)
                goto next_channel;

            /* 0x27270 and 0x272ce, the same scan written out twice. */
drop_loudest:
            {
                uint8_t most = 0;

                dh = 0xff;
                for (i = 0; i < 0x10; i++) {
                    if (most < SNDS.voice_cost[i]) {
                        most = SNDS.voice_cost[i];
                        dh = (uint8_t)i;
                    }
                }
                if (dh == 0xff)
                    goto abandon_sequence;

                al = (uint8_t)(al + SNDS.voice_gives_back[dh]);
                SNDS.voice_request[dh] = 0xff;
                SNDS.voice_gives_back[dh] = 0;
                SNDS.voice_cost[dh] = 0;
                SNDS.voice_keep_own[dh] = 0;
            }
            if (ah > al)
                goto drop_loudest;

have_voice:
            SNDS.voice_request[dh] = dl;
            SNDS.voice_gives_back[dh] = ah;
            al = (uint8_t)(al - ah);
            SNDS.voice_cost[dh] = chh;

            if ((rec->ch.channel_flags[cl] & 1) == 0) {
                SNDS.voice_keep_own[dh] = 0;
                continue;
            }

            SNDS.voice_keep_own[dh] = 1;
            if (dh == cl)
                continue;

            if (SNDS.voice_keep_own[cl] == 0) {
                uint8_t t;

                t = SNDS.voice_request[dh];
                SNDS.voice_request[dh] = SNDS.voice_request[cl];
                SNDS.voice_request[cl] = t;
                t = SNDS.voice_cost[dh];
                SNDS.voice_cost[dh] = SNDS.voice_cost[cl];
                SNDS.voice_cost[cl] = t;
                t = SNDS.voice_gives_back[dh];
                SNDS.voice_gives_back[dh] = SNDS.voice_gives_back[cl];
                SNDS.voice_gives_back[cl] = t;
                t = SNDS.voice_keep_own[dh];
                SNDS.voice_keep_own[dh] = SNDS.voice_keep_own[cl];
                SNDS.voice_keep_own[cl] = t;
                continue;
            }

            if (chh != 0) {
                SNDS.voice_request[dh] = 0xff;
                SNDS.voice_cost[dh] = 0;
                SNDS.voice_gives_back[dh] = 0;
                SNDS.voice_keep_own[dh] = 0;
                al = (uint8_t)(al + ah);
                continue;
            }

            if (SNDS.voice_cost[cl] != 0)
                goto abandon_sequence;

            al = (uint8_t)(al + SNDS.voice_gives_back[cl]);
            SNDS.voice_request[dh] = 0xff;
            SNDS.voice_gives_back[dh] = 0;
            SNDS.voice_cost[dh] = 0;
            SNDS.voice_keep_own[dh] = 0;
            SNDS.voice_request[cl] = dl;
            SNDS.voice_cost[cl] = chh;
            SNDS.voice_gives_back[cl] = ah;
            al = (uint8_t)(al - ah);

next_channel:
            ;
        }
        goto next_sequence;

abandon_sequence:
        tick_restore_state();
        al = SNDS.saved_total;

next_sequence:
        bp_ = (uint16_t)(bp_ + 0x10);
    }

    /* Apply: reprogram every voice whose request differs from what it plays. */
    for (voice = 0; voice < 0x10; voice++) {
        if (SNDS.voice_request[voice] == 0xff)
            continue;

        if (SNDS.voice_keep_own[voice] == 0) {
            uint8_t want = SNDS.voice_request[voice];
            int16_t d;

            al = (uint8_t)(want & 0xf);

            d = SNDS.voice_lo;
            for (;;) {
                if (dg_far_ptr(SNDS.voice_sequence[d])
                        == dg_far_ptr(SNDS.playing[want >> 4])
                    && SNDS.voice_channel[d] == al) {
                    if (SNDS.voice_keep_own[d] == 0) {
                        SNDS.voice_held[d] = SNDS.voice_request[voice];
                        SNDS.voice_request[voice] = 0xff;
                    }
                    break;
                }
                d++;
                if ((int16_t)SNDS.voice_hi < d - 1)
                    break;
            }
            continue;
        }

        {
            uint8_t want = SNDS.voice_request[voice];

            SNDS.voice_request[voice] = 0xff;
            SNDS.voice_held[voice] = want;

            al = (uint8_t)(want & 0xf);

            if (SNDS.voice_channel[voice] == al
                && dg_far_ptr(SNDS.voice_sequence[voice])
                       == dg_far_ptr(SNDS.playing[want >> 4]))
                continue;

            tick_program_voice(SEQUENCE_PTR(SNDS.playing[want >> 4]),
                               (uint16_t)voice, al);
        }
    }

    /* Hand out anything still requested to a voice that is still free. */
    {
        int16_t free_from = (int16_t)(uint8_t)(SNDS.voice_hi + 1);

        for (voice = 0; voice < 0x10; voice++) {
            uint8_t want = SNDS.voice_request[voice];
            int16_t d;

            if (want == 0xff)
                continue;

            d = free_from;
            do {
                d--;
            } while (SNDS.voice_held[d] != 0xff);
            free_from = d;

            SNDS.voice_held[d] = want;
            al = (uint8_t)(want & 0xf);

            tick_program_voice(SEQUENCE_PTR(SNDS.playing[want >> 4]),
                               (uint16_t)d, al);
        }
    }

silence_unused:
    for (voice = 0xf; voice >= 0; voice--) {
        if (SNDS.voice_channel[voice] == 0xf)
            continue;
        if (SNDS.voice_held[voice] != 0xff)
            continue;
        driver_controller((uint16_t)voice, 0x4000);
        driver_controller((uint16_t)voice, 0x7b00);
        driver_controller((uint16_t)voice, 0x4b00);
    }

    /* Eight word moves masked with 0x0f0f in the original; the same sixteen
       bytes one at a time. */
    for (i = 0; i < 0x10; i++)
        SNDS.voice_channel[i] = (uint8_t)(SNDS.voice_held[i] & 0x0f);

    for (voice = 0; voice < 0x10; voice++) {
        uint8_t held = SNDS.voice_held[voice];

        if (held == 0xff) {
            SNDS.voice_sequence[voice] = FAR_NULL;
        } else {
            SNDS.voice_sequence[voice] = SNDS.playing[held >> 4];
        }
    }

    SNDS.busy--;
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
void advance_volume_ramp(struct sequence far * seq, uint16_t seq_slot)
{
    uint8_t target, now, distance;

    if (seq->fade_countdown != 0) {
        seq->fade_countdown--;
        return;
    }
    seq->fade_countdown = seq->fade_period;

    target = (uint8_t)(seq->fade_target & 0x7f);
    now = seq->volume;

    if (target != now) {
        if (target > now) {
            distance = (uint8_t)(target - now);
            if (distance > seq->fade_step) {
                set_sequence_volume(seq, (uint8_t)(now + seq->fade_step), 1,
                                    seq_slot);
                return;
            }
        } else {
            distance = (uint8_t)(now - target);
            if (distance > seq->fade_step) {
                set_sequence_volume(seq, (uint8_t)(now - seq->fade_step), 1,
                                    seq_slot);
                return;
            }
        }
        set_sequence_volume(seq, target, 1, seq_slot);
    }

    seq->state = 0xfe;
    seq->fade_step = 0;

    if ((seq->fade_target & 0x80) != 0) {
        remove_sequence(seq);
        SNDS.voices_changed = 1;
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
void set_sequence_volume(struct sequence far * seq, uint8_t volume,
                         uint8_t defer, uint16_t seq_slot)
{
    uint16_t si, di;
    uint8_t want, level;

    SNDS.defer = defer;

    if (volume == seq->volume)
        return;
    seq->volume = volume;

    if (seq_slot == 0xff)
        return;

    want = (uint8_t)(seq_slot << 2);

    for (si = 0; si < 0x10; si++) {
        uint8_t held = SNDS.voice_held[si];

        if (held == 0xff || (uint8_t)(held & 0xf0) != want)
            continue;

        di = (uint16_t)(held & 0xf);
        level = scale_byte_pair(seq->ch.volume[di], seq->volume);

        if (SNDS.defer != 0) {
            SNDS.pending_volume[si] = level;
        } else {
            SNDS.pending_volume[si] = 0xff;
            driver_controller(si, (uint16_t)((7 << 8) | level));
        }
    }

    for (si = 0; si < 0x10; si++) {
        di = seq->track_channel[si];
        if (di == 0xff)
            return;
        if ((seq->ch.channel_flags[di] & 2) == 0)
            continue;
        if (SNDS.voice_held[di] != 0xff)
            continue;

        level = scale_byte_pair(seq->ch.volume[di], seq->volume);

        if (SNDS.defer != 0) {
            SNDS.pending_volume[di] = level;
        } else {
            SNDS.pending_volume[di] = 0xff;
            driver_controller(di, (uint16_t)((7 << 8) | level));
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
    uint16_t si = SNDS.scan_stopped;
    int16_t sent = 0;

    for (;;) {
        uint8_t pending = SNDS.pending_volume[si];

        if (pending != 0xff) {
            SNDS.pending_volume[si] = 0xff;
            driver_controller(si, (uint16_t)((7 << 8) | pending));
            sent++;
            if (sent == 2)
                break;
        }

        si++;
        if (si == 0x10)
            si = 0;
        if (si == SNDS.scan_stopped)
            break;
    }

    SNDS.scan_stopped = (uint8_t)si;
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

    if (SNDS.busy != 0)
        return;

    if (SNDS.voices_changed != 0)
        sequencer_tick();

    si = 0;
    di = 0;

    while (si != 0x40) {
        struct sequence far *seq = SEQUENCE_PTR(SNDS.playing[si / 4]);

        if (seq == SEQUENCE_NONE)
            break;

        if (seq->skip != 0) {
            si += 4;
            di += 4;
            continue;
        }

        if (seq->fade_step != 0) {
            advance_volume_ramp(seq, si);
            if (seq->state == 0xff) {
                di += 4;
                continue;
            }
        }

        if (seq->poll != 0)
            drop_unless_polled(seq);
        else
            step_sequence(seq, di);

        if (seq->state != 0xff)
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
    driver_nop();
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
 * The search compares both halves of the far pointer. The port compares the
 * pointer, which is the same test: both tables hold pairs `sequencer_tick`
 * copied from the playing table, so one record is never filed two ways.
 */
void drop_unless_polled(struct sequence far * seq)
{
    int16_t si;

    for (si = 0; si < 0x40; si += 4)
        if (SEQUENCE_PTR(SNDS.polled[si / 4]) == seq)
            return;

    remove_sequence(seq);
    SNDS.voices_changed = 1;
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
 * address, and question 3's is the five words a digitised module's "play this"
 * takes: flags, rate, the sample's far pointer and its length. **This is where
 * a sampled sound is started**, and the only place in the game that starts
 * one - the nine wrappers at 0x0bb98 never ask a module to play.
 *
 * With no module installed `sound_callback` answers the DGROUP segment, whose
 * low byte is non-zero, so question 4 removes every sequence on this table -
 * which is the behaviour of a machine with no digitised sound and is why the
 * game runs happily without one.
 */
void poll_sequences(void)
{
    int16_t si;

    for (si = 0; si < 0x40; si += 4) {
        struct sequence far *rec = SEQUENCE_PTR(SNDS.polled[si / 4]);
        const uint8_t far *at;
        const uint8_t far *data;
        uint16_t answer;
        uint8_t cl;

        if (rec == SEQUENCE_NONE)
            return;

        rec->ticks++;

        /*
         * Two far pointers followed - `lds bp, es:[bx+8]` and then
         * `lds bp, ds:[bp]` - land on the sequence's own data, and the low
         * nibble of +0x165, less one and doubled, indexes a table of offsets
         * there. `data` - the original's `ds:bp` - ends up on the record the
         * callback is asked about. The second pointer, `at`, is kept: an
         * offset stepped inside its segment is what the module is handed
         * below.
         */
        at = *rec->cursor_at;
        data = at;

        cl = (uint8_t)((rec->poll & 0x0f) - 1);
        cl = (uint8_t)(cl << 1);
        data += *(const uint16_t *)(data + cl);

        if (rec->poll <= 0x10) {
            const uint8_t far *b = data + 1;

            rec->poll |= 0x80;

            /*
             * A leading 0xfe is stepped over, and then one more byte, which
             * puts `b` on the record proper: its first word is the sampling
             * rate, its second the length, and the sample itself starts eight
             * bytes in.
             */
            if (*b == 0xfe)
                b++;
            b++;

            /*
             * The five words the module reads through SI, pushed length first
             * so the last pushed - the volume and the loop - is what SI points
             * at. The sample is filed as `at`'s segment beside the offset `b`
             * has been stepped to within it.
             */
            union sound_module_args args;

            args.play.volume = rec->volume;
            args.play.loop = rec->loop;
            args.play.rate = *(const uint16_t *)b;
            args.play.sample = far_from(FP_SEG(at), b + 8);
            args.play.length = *(const uint16_t *)(b + 2);

            sound_callback(3, &args);
            continue;
        }

        {
            union sound_module_args args;

            args.poll.volume = rec->volume;
            args.poll.loop = rec->loop;
            answer = sound_callback(4, &args);
        }

        if ((uint8_t)(answer >> 8) != 0)
            rec->ticks = 0;

        if ((uint8_t)answer != 0) {
            rec->poll = 0;
            remove_sequence(rec);
            SNDS.voices_changed = 1;
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
void step_sequence(struct sequence far * seq, uint16_t di)
{
    const uint8_t far *base, *data;
    uint16_t si;
    int16_t t;

    SNDS.slot_high = (uint8_t)(di * 4);
    seq->ticks++;

    {
        /*
         * Two loads, not three. The first reads the far pointer stored at the
         * record's +8; the second reads the far pointer *that* points at, and
         * the result is the cursor's base. Measured on the first call:
         * +8 holds 7594:016a, and 7594:016a holds 77ab:0002, so the base is 2
         * in segment 77ab. Following it once more lands in the event data and
         * reads a note as if it were a pointer.
         */
        base = *seq->cursor_at;
        SNDS.cursor_park = (int16_t)FP_OFF(base);
    }
    data = base;

    for (si = 0; si < 0x10; si++) {
        uint8_t al = seq->track_channel[si];
        uint16_t *pos = &seq->position[si];
        uint16_t *delay = &seq->delay[si];
        uint8_t status;

        if (al == 0xff)
            goto finished;
        if (al == 0xfe)
            continue;

        SNDS.own_voice = 0xff;
        SNDS.bend_gate = 0;

        if ((seq->ch.channel_flags[al] & 2) != 0) {
            SNDS.own_voice = al;
            SNDS.bend_gate = 1;
        } else {
            uint8_t want = (uint8_t)((al & 0xf) | SNDS.slot_high);
            uint16_t j;

            for (j = 0; j < 0x10; j++) {
                if (SNDS.voice_held[j] == want) {
                    SNDS.own_voice = (uint8_t)j;
                    break;
                }
            }
        }

        data = base + *(const uint16_t *)(base + 2 * si) + *pos;
        if (*pos == 0)
            continue;

        if (*delay != 0) {
            (*delay)--;
            if (*delay == 0x8000) {
                uint8_t d = *data;
                uint8_t hi = 0;

                data++;
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
            uint8_t b = *data;
            uint8_t hi_nibble, lo_nibble;

            data++;
            (*pos)++;

            if (b >= 0x80) {
                seq->status[si] = b;
            } else {
                b = seq->status[si];
                data--;
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
                data = midi_meta_event(data, seq, si,
                                       (uint16_t)((hi_nibble << 8) | 0xf));
                if (*pos == 0)
                    break;
            } else {
                uint16_t ax = (uint16_t)(((uint16_t)hi_nibble << 8)
                                         | SNDS.own_voice);

                switch (hi_nibble) {
                case 0x80: data = midi_note_off_event(data, seq, si, ax); break;
                case 0x90: data = midi_note_event(data, seq, si, ax); break;
                case 0xa0: data = midi_event_6(data, seq, si, ax); break;
                case 0xb0: data = midi_controller_event(data, seq, si, ax); break;
                case 0xc0: data = midi_program_event(data, seq, si, ax); break;
                case 0xd0: data = midi_event_9(data, seq, si, ax); break;
                case 0xe0: data = midi_bend_event(data, seq, si, ax); break;
                case 0xf0: data = midi_skip_event(data, seq, si, ax); break;
                default:
                    *pos = 0;
                    goto next_channel;
                }
            }

            {
                uint8_t d = *data;

                data++;
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
        if (seq->track_channel[si] == 0xff)
            break;
        if (seq->position[si] != 0)
            return;
    }

    if (seq->rewind_mark == 0 && seq->loop == 0) {
        remove_sequence(seq);
        SNDS.voices_changed = 1;
        return;
    }

    seq->ticks = seq->ticks_saved;
    for (t = 0; t < 0x10; t++) {
        seq->position[t] = seq->position_saved[t];
        seq->delay[t] = seq->delay_saved[t];
        seq->status[t] = seq->status_saved[t];
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
const uint8_t far *midi_note_off_event(const uint8_t far * data,
                                       struct sequence far * seq, uint16_t si,
                                       uint16_t ax)
{
    uint8_t note, velocity, channel;
    uint16_t *counter = &seq->position[si];

    note = *data;
    data++;
    (*counter)++;

    /*
     * The second byte is the velocity, and it is **not** discarded: CL still
     * holds it at the call and a driver may read it. The port used to drop it
     * on the strength of the speaker driver ignoring CL, which is the shape of
     * mistake that survives every screen comparison.
     */
    velocity = *data;
    data++;
    (*counter)++;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    if (seq->ch.note[channel] == note)
        seq->ch.note[channel] = 0xff;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_stop_note((uint16_t)(ax & 0xf),
                         (uint16_t)((note << 8) | velocity));

    return data;
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
const uint8_t far *midi_event_6(const uint8_t far * data,
                                struct sequence far * seq, uint16_t si,
                                uint16_t ax)
{
    uint16_t *counter = &seq->position[si];

    data++;
    (*counter)++;

    data++;
    (*counter)++;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_nop();

    return data;
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
const uint8_t far *midi_note_event(const uint8_t far * data,
                                   struct sequence far * seq, uint16_t si,
                                   uint16_t ax)
{
    uint8_t note, velocity, channel;
    uint16_t *counter = &seq->position[si];

    note = *data;
    data++;
    (*counter)++;

    velocity = *data;
    data++;
    (*counter)++;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    if (velocity != 0) {
        seq->ch.note[channel] = note;

        if ((uint8_t)ax != 0xff && SNDS.muted == 0)
            driver_start_note((uint16_t)(ax & 0xf),
                              (uint16_t)((note << 8) | velocity));
    } else {
        if (seq->ch.note[channel] == note)
            seq->ch.note[channel] = 0xff;

        if ((uint8_t)ax != 0xff && SNDS.muted == 0)
            driver_stop_note((uint16_t)(ax & 0xf),
                             (uint16_t)((note << 8) | velocity));
    }

    return data;
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
const uint8_t far *midi_controller_event(const uint8_t far * data,
                                         struct sequence far * seq, uint16_t si,
                                         uint16_t ax)
{
    uint16_t *counter = &seq->position[si];
    uint8_t ctrl, value, channel;

    ctrl = *data;
    data++;
    (*counter)++;

    value = *data;
    data++;
    (*counter)++;

    if (SNDS.bend_gate != 0 && SNDS.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    if (ctrl == 7) {
        seq->ch.volume[channel] = value;
        value = scale_byte_pair(value, seq->volume);
        if ((uint8_t)ax >= 0x20)
            return data;
        SNDS.pending_volume[(uint8_t)ax] = 0xff;
    } else if (ctrl == 0xa) {
        seq->ch.pan[channel] = value;
    } else if (ctrl == 1) {
        seq->ch.modulation[channel] = value;
    } else if (ctrl == 0x40) {
        uint16_t *bend = &seq->ch.bend[channel];

        if (value != 0)
            *bend |= 0x8000;
        else
            *bend &= 0x7fff;
    } else if (ctrl == 0x4b) {
        uint8_t *p = &seq->ch.voice_budget[channel];

        *p = (uint8_t)((*p & 0xf0) | value);
        SNDS.voices_changed = 1;
    } else if (ctrl == 0x4e) {
        uint8_t *p = &seq->ch.no_voice[channel];

        *p = (uint8_t)((*p & 0xf0) | (value != 0 ? 1 : 0));
        SNDS.voices_changed = 1;
    }

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_controller((uint16_t)(ax & 0xf),
                      (uint16_t)(((uint16_t)ctrl << 8) | value));

    return data;
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
const uint8_t far *midi_program_event(const uint8_t far * data,
                                      struct sequence far * seq, uint16_t si,
                                      uint16_t ax)
{
    uint16_t *counter = &seq->position[si];
    uint8_t program, channel;

    program = *data;
    data++;
    (*counter)++;

    if (SNDS.bend_gate != 0 && SNDS.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);
    seq->ch.program[channel] = program;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_program_change((uint16_t)(ax & 0xf), program);

    return data;
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
const uint8_t far *midi_event_9(const uint8_t far * data,
                                struct sequence far * seq, uint16_t si,
                                uint16_t ax)
{
    uint16_t *counter = &seq->position[si];

    data++;
    (*counter)++;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_nop();

    return data;
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
const uint8_t far *midi_bend_event(const uint8_t far * data,
                                   struct sequence far * seq, uint16_t si,
                                   uint16_t ax)
{
    uint8_t lsb, msb, channel;
    uint16_t *counter = &seq->position[si];
    uint16_t value;
    uint16_t *slot;

    lsb = *data;
    data++;
    (*counter)++;

    msb = *data;
    data++;
    (*counter)++;

    if (SNDS.bend_gate != 0 && SNDS.voice_held[(ax & 0xf)] != 0xff)
        return data;

    channel = (uint8_t)(seq->track_channel[si] & 0xf);

    value = (uint16_t)((((uint16_t)msb >> 1) << 8)
                       | (uint16_t)(lsb | ((msb & 1) ? 0x80 : 0)));

    slot = &seq->ch.bend[channel];
    if (*slot >= 0x8000)
        value |= 0x8000;
    *slot = value;

    if ((uint8_t)ax != 0xff && SNDS.muted == 0)
        driver_pitch_bend((uint16_t)(ax & 0xf),
                      (uint16_t)(((uint16_t)lsb << 8) | msb));

    return data;
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
const uint8_t far *midi_meta_event(const uint8_t far * data,
                                   struct sequence far * seq, uint16_t si,
                                   uint16_t ax)
{
    uint16_t *counter = &seq->position[si];
    uint8_t status = (uint8_t)(ax >> 8);
    uint8_t first, second;
    int16_t t;

    if (status != 0xc0 && status != 0xb0)
        return skip_unknown_event(data, seq, si, ax);

    if (status == 0xc0) {
        first = *data;
        data++;
        (*counter)++;

        if (first != 0x7f) {
            if (SNDS.muted == 0)
                seq->state = first;
            return data;
        }

        second = *data;
        data++;
        (*counter)++;

        {
            uint8_t hi = 0;

            if (second == 0xf8) {
                hi = 0x80;
                second = 0xf0;
            }
            seq->delay[si] = (uint16_t)(((uint16_t)hi << 8) | second);
        }
        seq->status[si] = 0xcf;

        for (t = 0; t < 0x10; t++) {
            seq->position_saved[t] = seq->position[t];
            seq->delay_saved[t] = seq->delay[t];
            seq->status_saved[t] = seq->status[t];
        }
        seq->ticks_saved = seq->ticks;

        (*counter)--;
        data--;
        seq->delay[si] = 0;
        return data;
    }

    first = *data;
    data++;
    (*counter)++;

    second = *data;
    data++;
    (*counter)++;

    if (first == 0x50) {
        if (second == 0x7f)
            second = SNDS.param_default;
        seq->device_value = second;
        driver_param_349(second);
        return data;
    }

    if (first == 0x60) {
        if (SNDS.muted == 0)
            seq->loop_count++;
        return data;
    }

    if (first == 0x52 && seq->rewind_mark == second) {
        for (t = 0; t < 0x10; t++)
            seq->position[t] = 0;
    }

    return data;
}

/*
 * 0x2817a
 *
 * A one-instruction forwarder to `skip_unknown_event`. It exists so that the
 * dispatch that reaches it has an entry of its own rather than sharing one.
 */
const uint8_t far *midi_skip_event(const uint8_t far * data,
                                   struct sequence far * seq, uint16_t si,
                                   uint16_t ax)
{
    return skip_unknown_event(data, seq, si, ax);
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
const uint8_t far *skip_unknown_event(const uint8_t far * data,
                                      struct sequence far * seq, uint16_t si,
                                      uint16_t ax)
{
    uint16_t *counter = &seq->position[si];
    uint8_t status = (uint8_t)(ax >> 8);
    uint8_t b;

    if (status == 0xf0) {
        do {
            b = *data;
            data++;
            (*counter)++;
        } while (b != 0xf7);
        return data;
    }

    if (status != 0xc0 && status != 0xd0) {
        data++;
        (*counter)++;
    }

    data++;
    (*counter)++;

    return data;
}

/*
 * 0x282cb
 *
 * Scale one byte by another and halve the range: `((cl+1) * (dl+1)) >> 8`,
 * doubled, then reduced by one unless it is already zero.
 *
 * A **** routine that takes and answers CL, preserving AX around the
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
void init_sequence_params(struct sequence far * seq)
{
    uint8_t far *tbl;
    uint16_t si;

    if (FP_OFF(seq->cursor_at) == 0xffff && FP_SEG(seq->cursor_at) == 0xffff)
        return;

    tbl = *seq->cursor_at;

    if (tbl[0x23] == 0xfe && tbl[0x22] == 0xfd && tbl[0x21] == 0xfc)
        return;

    for (si = 0x20; si != 0;) {
        si -= 2;
        SNDS.scratch[si / 2] = 0;
    }
    SNDS.scratch_mark = 0xff;

    {
        uint16_t bp = 0;

        if (tbl[bp] == 0xf0) {
            SNDS.scratch_mark = tbl[bp + 1];
            bp += 8;
        }

        si = 0;
        for (;;) {
            uint8_t id = tbl[bp];

            if (id == SNDS.ch) {
                bp++;
                for (;;) {
                    uint8_t c = tbl[bp];

                    bp++;
                    if (c == 0xff)
                        break;
                    bp++;
                    SNDS.scratch[si / 2] = *(int16_t *)(tbl + bp);
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
            *(int16_t *)(tbl + si) = SNDS.scratch[si / 2];
        tbl[0x20] = SNDS.scratch_mark;
    }

    tbl[0x21] = 0xfc;
    tbl[0x22] = 0xfd;
    tbl[0x23] = 0xfe;
}

/*
 * 0x28559
 *
 * The ordinary-call face of `silence_driver`. It loads `ES:AX` from where a
 * stack argument would be, and `silence_driver` reads neither - the same dead
 * argument as in 0x2846a. Its one C caller, `stop_sound`, pushes nothing, so
 * the port declares none.
 */
void silence_driver_far(void)
{
    silence_driver();
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
uint16_t install_driver_far(const uint8_t far * drv)
{
    return install_driver(drv);
}

/*
 * 0x2846a
 *
 * The ordinary-call face of `configure_driver`. It loads `ES:AX` from the
 * stack argument and zeroes `BX` before the call. `AX` passes back out, and
 * 0x28580 reads it.
 *
 * **That argument is not dead**, and this comment used to say it was. The
 * speaker driver's function 1 answers two constants and reads neither
 * register, so nothing the port could run disagreed; `GMD:` function 1 copies
 * a 0x481-byte patch bank out of exactly this `ES:AX`. `configure_driver`
 * takes it now and hands it to the driver.
 */
uint16_t configure_driver_far(const uint8_t far * drv)
{
    return configure_driver(drv);
}

/*
 * 0x284ef
 *
 * The ordinary-call face of `retire_and_tick`, which reads its record from
 * `ES:AX` - loaded here from the stack argument with one `les`.
 */
void retire_and_tick_far(struct sequence far * seq)
{
    retire_and_tick(seq);
}

/*
 * 0x28480
 *
 * The ordinary-call face of `start_sequence`. The original takes its record in
 * `es:ax` and its flag in `cx`, which no C caller can arrange, so this takes
 * them on the stack and puts them in registers.
 *
 * It also saves DS, DI and SI around the call. `start_sequence` restores what
 * it changes, but the ones this pushes are the ones a C caller expects to keep,
 * and the hand-written routine makes no such promise.
 */
void start_sequence_far(struct sequence far * seq, uint16_t flag)
{
    start_sequence(seq, flag);
}

