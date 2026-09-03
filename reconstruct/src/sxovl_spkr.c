/*
 * The PC-speaker sound driver, `SX.OVL`, as it is actually loaded.
 *
 * Provenance is `SX.OVL SPKR:0xNNNN` - offsets within the loaded driver, not
 * image addresses, because the loader chooses the segment. See
 * docs/sound-driver.md for how it was found and dumped, and for the dispatch
 * table at SPKR:0x34a that every call comes through.
 *
 * The driver's own state lives in its code segment and is reached with `SX8`;
 * the port does not copy it anywhere.
 *
 * Reconstructed from `incredible-machine/TIM.EXE`.
 */
#include "dgroup.h"
#include "io.h"
#include "tim.h"

/*
 * SX.OVL SPKR:0x0042
 *
 * One PIT divisor source per **quarter semitone**, 381 of them, covering MIDI
 * notes 24 to 119 - `(119 - 24) * 4` is exactly 380, so the last index is the
 * top note and there is no spare entry.
 *
 * These are not divisors themselves: `sx_note_on` divides 1,331,000 by the
 * entry to get what it sends to the timer. Measured against equal temperament
 * that comes out about eleven cents sharp at A440 - entry 494 gives a divisor
 * of 2694 where 440 Hz wants 2712 - so the table and the dividend were derived
 * with slightly different constants. Transcribed as it is; the port is not the
 * place to retune it.
 */
static const uint16_t SX_DIVISOR[381] = {
       37,    37,    38,    38,    39,    39,    40,    41,
       41,    42,    42,    43,    44,    44,    45,    46,
       46,    47,    48,    48,    49,    50,    50,    51,
       52,    53,    53,    54,    55,    56,    57,    57,
       58,    59,    60,    61,    62,    63,    63,    64,
       65,    66,    67,    68,    69,    70,    71,    72,
       73,    74,    76,    77,    78,    79,    80,    81,
       82,    84,    85,    86,    87,    89,    90,    91,
       92,    94,    95,    97,    98,    99,   101,   102,
      104,   105,   107,   108,   110,   112,   113,   115,
      116,   118,   120,   122,   123,   125,   127,   129,
      131,   133,   135,   137,   139,   141,   143,   145,
      147,   149,   151,   153,   156,   158,   160,   162,
      165,   167,   170,   172,   175,   177,   180,   182,
      185,   188,   190,   193,   196,   199,   202,   205,
      208,   211,   214,   217,   220,   223,   226,   230,
      233,   236,   240,   243,   247,   250,   254,   258,
      262,   265,   269,   273,   277,   281,   285,   289,
      294,   298,   302,   307,   311,   316,   320,   325,
      330,   334,   339,   344,   349,   354,   359,   365,
      370,   375,   381,   386,   392,   398,   403,   409,
      415,   421,   427,   434,   440,   446,   453,   459,
      466,   473,   480,   487,   494,   501,   508,   516,
      523,   531,   539,   546,   554,   562,   571,   579,
      587,   596,   604,   613,   622,   631,   640,   650,
      659,   669,   679,   688,   698,   709,   719,   729,
      740,   751,   762,   773,   784,   795,   807,   819,
      831,   843,   855,   867,   880,   893,   906,   919,
      932,   946,   960,   974,   988,  1002,  1017,  1031,
     1046,  1062,  1077,  1093,  1109,  1125,  1141,  1158,
     1175,  1192,  1209,  1227,  1244,  1263,  1281,  1300,
     1318,  1338,  1357,  1377,  1397,  1417,  1438,  1459,
     1480,  1501,  1523,  1545,  1568,  1591,  1614,  1637,
     1661,  1685,  1710,  1735,  1760,  1786,  1812,  1838,
     1865,  1892,  1919,  1947,  1975,  2004,  2033,  2063,
     2093,  2123,  2154,  2186,  2217,  2250,  2282,  2316,
     2349,  2383,  2418,  2453,  2489,  2525,  2562,  2599,
     2637,  2675,  2714,  2754,  2794,  2834,  2876,  2917,
     2960,  3003,  3047,  3091,  3136,  3182,  3228,  3275,
     3322,  3371,  3420,  3469,  3520,  3571,  3623,  3676,
     3729,  3783,  3839,  3894,  3951,  4008,  4067,  4126,
     4186,  4247,  4309,  4371,  4435,  4499,  4565,  4631,
     4699,  4767,  4836,  4907,  4978,  5050,  5124,  5198,
     5274,  5351,  5429,  5507,  5588,  5669,  5751,  5835,
     5920,  6006,  6093,  6182,  6272,  6363,  6456,  6550,
     6645,  6741,  6840,  6939,  7040,  7142,  7246,  7352,
     7459,  7567,  7677,  7789,  7902,  8016,  8134,  8252,
     8372,  8494,  8618,  8742,  8870,
};

/*
 * SX.OVL SPKR:0x0480
 *
 * Silence the speaker: clear bits 0 and 1 of port 0x61, the timer gate and the
 * speaker data line, and forget the sounding note at SPKR:0x344.
 *
 * Does nothing at all when no note is sounding, so it is safe to call twice.
 */
void sx_speaker_off(void)
{
    if (SX8(0x344) != 0) {
        io_out8(0x61, (uint8_t)(io_in8(0x61) & 0xFC));
        SX8(0x344) = 0;
    }
}

/*
 * SX.OVL SPKR:0x04fd
 *
 * Apply the pitch bend at SPKR:0x342 to an index already in quarter semitones,
 * adding or subtracting by the direction flag at SPKR:0x343, and answer 0xffff
 * if the result leaves the playable range.
 *
 * The range test is 0x18 to 0x1dc - 24 to 476 - but the table only has entries
 * 0 to 380. **A bend large enough to push the index past 380 is accepted here
 * and then reads past the end of the table**, into the driver's own code.
 *
 * That is reachable. `sx_pitch_bend` divides a full-scale bend by 0xab, giving
 * a magnitude up to 47 quarter semitones, so any note above about 93 bent fully
 * upward lands beyond 380 and still passes this test. The bound is wrong and
 * the port reproduces it rather than tightening it.
 */
uint16_t sx_apply_bend(uint16_t index)
{
    uint16_t bend = SX8(0x342);

    if (SX8(0x343) != 0)
        index = (uint16_t)(index + bend);
    else
        index = (uint16_t)(index - bend);

    if (index < 0x18 || index > 0x1dc)
        return 0xFFFF;
    return index;
}

/*
 * SX.OVL SPKR:0x0497
 *
 * Start a note. The argument is a MIDI note number and anything outside 0x18 to
 * 0x77 - 24 to 119 - is ignored rather than clamped.
 *
 * The note is remembered at SPKR:0x344 **before** any of the enable flags are
 * looked at, so `sx_speaker_off` will still try to silence a note that was
 * never actually sounded. That is the original's order and not an accident of
 * transcription.
 *
 * Four separate bytes can veto the sound - SPKR:0x345, 0x346 and 0x347 here,
 * and the bend at 0x342 by answering out-of-range. Which is which is not
 * established.
 *
 * The timer is programmed channel 2, mode 3, low byte then high byte, which is
 * the square wave the speaker needs; then bits 0 and 1 of port 0x61 connect it.
 */
void sx_note_on(uint16_t note)
{
    uint16_t index;

    if (note < 0x18 || note > 0x77)
        return;

    SX8(0x344) = (uint8_t)note;

    index = (uint16_t)((note - 0x18) * 4);

    if (SX8(0x342) != 0) {
        index = sx_apply_bend(index);
        if (index == 0xFFFF)
            return;
    }

    if (SX8(0x345) == 0 || SX8(0x347) == 0 || SX8(0x346) == 0)
        return;

    io_out8(0x43, 0xB6);
    {
        uint32_t divisor = 1331000u / SX_DIVISOR[index];

        io_out8(0x42, (uint8_t)(divisor & 0xFF));
        io_out8(0x42, (uint8_t)((divisor >> 8) & 0xFF));
    }
    io_out8(0x61, (uint8_t)(io_in8(0x61) | 3));
}

/*
 * SX.OVL SPKR:0x037b  (dispatch table entry 4)
 *
 * Stop the note in CH, but only if it is the one actually sounding. The note
 * currently on the speaker is at SPKR:0x344, and a request to stop any other
 * note is ignored - which is what lets a voice that has already been taken
 * over by a later note be released harmlessly.
 *
 * The argument is the **high** byte of CX; the low byte is not looked at.
 */
void sx_stop_note(uint16_t cx)
{
    if (SX8(0x344) == (uint8_t)(cx >> 8))
        sx_speaker_off();
}

/*
 * SX.OVL SPKR:0x0386  (dispatch table entry 5)
 *
 * Start the note in CH on the channel in AL, if that is the channel the driver
 * is listening to.
 *
 * A single speaker can only sound one note, so the driver keeps one channel
 * number at SPKR:0x348 and drops every request for any other. A note of zero is
 * dropped too - it means silence, not the lowest note.
 *
 * The old note is stopped before the new one starts, so the speaker is never
 * left connected across a change of frequency.
 *
 * As with 0x037b the note is the high byte of CX, and the channel is the low
 * byte of AX.
 */
void sx_start_note(uint16_t ax, uint16_t cx)
{
    uint8_t note = (uint8_t)(cx >> 8);

    if (SX8(0x348) != (uint8_t)ax)
        return;
    if (note == 0)
        return;

    sx_speaker_off();
    sx_note_on(note);
}

/*
 * SX.OVL SPKR:0x037a  (dispatch table entries 3, 6, 8, 9, 14, 15, 16)
 *
 * The do-nothing entry, a bare `ret`. Seven of the eighteen table slots point
 * here - the same arrangement as `VGA:0x0252` in the video driver - so a
 * function number with no implementation is silently accepted rather than
 * faulting.
 */
void sx_nop(void)
{
}

/*
 * SX.OVL SPKR:0x03a1  (dispatch table entry 7)
 *
 * Handle a controller change: AL is the channel, CH the controller number and
 * CL its value. Four controllers are recognised and everything else is ignored.
 *
 *   0x7b - MIDI "all notes off". Silences the speaker whatever channel it is
 *          addressed to; the channel is not even looked at.
 *   0x4b - claim or release the speaker. A non-zero value takes the channel
 *          over unless it is already the current one; a zero value gives it up,
 *          but only if the caller is the channel that holds it, and leaves
 *          0x348 as 0xff meaning "nobody".
 *   0x4e - retrigger the sounding note at CL, if the caller holds the channel.
 *          0xff means "no note" and is dropped.
 *   0x07 - MIDI volume. Any value marks the driver enabled at 0x347; a value of
 *          zero then silences it and marks it disabled again. So the flag is
 *          set even on the way to being cleared, which is the original's order.
 */
void sx_controller(uint16_t ax, uint16_t cx)
{
    uint8_t channel = (uint8_t)ax;
    uint8_t ctrl    = (uint8_t)(cx >> 8);
    uint8_t value   = (uint8_t)cx;

    if (ctrl == 0x7b) {
        sx_speaker_off();
        return;
    }

    if (ctrl == 0x4b) {
        if (value != 0) {
            if (SX8(0x348) != channel) {
                sx_speaker_off();
                SX8(0x348) = channel;
            }
        } else if (SX8(0x348) == channel) {
            sx_speaker_off();
            SX8(0x348) = 0xff;
        }
        return;
    }

    if (ctrl == 0x4e) {
        if (value != 0xff && SX8(0x348) == channel) {
            sx_speaker_off();
            sx_note_on(value);
        }
        return;
    }

    if (ctrl == 7) {
        SX8(0x347) = 1;
        if (value == 0) {
            sx_speaker_off();
            SX8(0x347) = 0;
        }
    }
}

/*
 * SX.OVL SPKR:0x0410  (dispatch table entry 10)
 *
 * Pitch bend. AL is the channel and CX carries the usual MIDI pair, CL the
 * most significant seven bits and CH the least.
 *
 * The value is assembled **twice, differently**. The first assembly keeps both
 * halves - `(msb << 7) | lsb` - and is stored at SPKR:0x33c for
 * `sx_query` to hand back. The second, which is the one the bend is actually
 * computed from, is `msb << 7` and **throws the low seven bits away**, so the
 * speaker driver bends at 7-bit resolution however finely the caller asks.
 *
 * The distance from centre is divided by 0xab into quarter semitones at
 * SPKR:0x342, with the direction at SPKR:0x343. A full-scale bend is therefore
 * 47 quarter semitones, near enough a whole octave - far wider than the two
 * semitones a synthesiser would default to.
 *
 * A note already sounding is retriggered at the new pitch. Everything is
 * skipped, including storing 0x33c, if the caller does not hold the channel.
 */
void sx_pitch_bend(uint16_t ax, uint16_t cx)
{
    uint8_t channel = (uint8_t)ax;
    uint8_t lsb     = (uint8_t)(cx >> 8);
    uint8_t msb     = (uint8_t)cx;
    uint16_t value;

    SX16(0x33c) = (int16_t)(uint16_t)(((uint16_t)msb << 7) | lsb);

    if (SX8(0x348) != channel)
        return;

    SX8(0x343) = 0;
    value = (uint16_t)((uint16_t)msb << 7);

    if (value == 0x2000) {
        SX8(0x342) = 0;
    } else {
        uint16_t away;

        if (value > 0x2000) {
            away = (uint16_t)(value - 0x2000);
            SX8(0x343) = 1;
        } else {
            away = (uint16_t)(0x2000 - value);
        }
        SX8(0x342) = (uint8_t)(away / 0xab);
    }

    if (SX8(0x344) != 0)
        sx_note_on(SX8(0x344));
}

/*
 * SX.OVL SPKR:0x0525  (dispatch table entry 2)
 *
 * Silence the speaker, and nothing else - a bare forward to 0x0480 so that the
 * caller does not need to know which of the two it wants.
 */
void sx_stop_all(void)
{
    sx_speaker_off();
}

/*
 * SX.OVL SPKR:0x0529, 0x0549, 0x055b  (entries 12, 11, 13)
 *
 * Three parameter accessors sharing one convention: answer the old value in AX,
 * and write the new one only when CL is not 0xff. So 0xff means "just read",
 * which is why a parameter can never be set to 0xff.
 *
 * They differ in what else they do. 0x0529 reports its byte at SPKR:0x345 as
 * **0 or 1 rather than its actual value**, and silences the speaker when set to
 * zero. 0x055b, at SPKR:0x346, refuses to store anything but 0 or 1 - it writes
 * the constant rather than CL - and also silences on zero. 0x0549, at
 * SPKR:0x349, is the plain one: it stores CL as given and touches nothing else,
 * and no other routine in the driver reads that byte.
 */
uint16_t sx_param_345(uint16_t cx)
{
    uint8_t value = (uint8_t)cx;
    uint16_t old = SX8(0x345) != 0 ? 1 : 0;

    if (value == 0xff)
        return old;

    SX8(0x345) = value;
    if (value == 0)
        sx_speaker_off();
    return old;
}

/*
 * SX.OVL SPKR:0x0549  (dispatch table entry 11)
 *
 * The plain accessor of the three described above: stores CL as given and
 * touches nothing else. No other routine in the driver reads SPKR:0x349.
 */
uint16_t sx_param_349(uint16_t cx)
{
    uint8_t value = (uint8_t)cx;
    uint16_t old = SX8(0x349);

    if (value == 0xff)
        return old;

    SX8(0x349) = value;
    return old;
}

/*
 * SX.OVL SPKR:0x055b  (dispatch table entry 13)
 *
 * As above, but it refuses to store anything except 0 or 1 - writing the
 * constant rather than CL - and silences the speaker when set to zero.
 */
uint16_t sx_param_346(uint16_t cx)
{
    uint8_t value = (uint8_t)cx;
    uint16_t old = SX8(0x346);

    if (value == 0xff)
        return old;

    if (value == 0) {
        SX8(0x346) = 0;
        sx_speaker_off();
    } else {
        SX8(0x346) = 1;
    }
    return old;
}

/*
 * SX.OVL SPKR:0x057d  (dispatch table entry 17)
 *
 * Answer what the driver knows about a message, given its status byte in AH.
 *
 *   0xe0 - a pitch bend: hand back the last one, as stored by `sx_pitch_bend`.
 *   0xb0 - a controller change, but only the exact query "controller 0x4b with
 *          value 0xff": answer 1 if AL is the channel holding the speaker and 0
 *          if it is not. Any other controller or value falls through.
 *
 * Everything else answers 0xffff, "I do not know". Note that 0xffff is also a
 * legitimate pitch bend value, so a caller cannot tell the two apart from the
 * answer alone.
 */
uint16_t sx_query(uint16_t ax, uint16_t cx)
{
    uint8_t status = (uint8_t)(ax >> 8);

    if (status == 0xe0)
        return (uint16_t)SX16(0x33c);

    if (status == 0xb0 && (uint8_t)(cx >> 8) == 0x4b
        && (uint8_t)cx == 0xff)
        return (uint8_t)ax == SX8(0x348) ? 1 : 0;

    return 0xffff;
}

/*
 * SX.OVL SPKR:0x05a8, 0x05b0  (entries 1 and 0)
 *
 * Two constant descriptions of the driver, returned in registers rather than
 * written anywhere.
 *
 * 0x05a8 answers AX = 0x05a8 - its own address, which is what makes it look
 * like a pointer to a structure that is really just this code - with CX = 0x0f00.
 * 0x05b0 answers AX = 0x01ff and CX = 0x1201.
 *
 * What the fields mean is **not established**; they are transcribed as the
 * constants they are. Neither is called on the screens checked.
 */
void sx_describe_1(uint16_t *ax, uint16_t *cx)
{
    *ax = 0x05a8;
    *cx = 0x0f00;
}

/*
 * SX.OVL SPKR:0x05b0  (dispatch table entry 0)
 *
 * The other constant description: AX = 0x01ff, CX = 0x1201. What the fields
 * mean is not established.
 */
void sx_describe_0(uint16_t *ax, uint16_t *cx)
{
    *ax = 0x01ff;
    *cx = 0x1201;
}
