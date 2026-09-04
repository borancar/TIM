# The sound driver, `SX.OVL`

Everything here is **measured from the running game**, not read off the disk
image, for the same reason the video driver is: the payload in the archive is
compressed, and what matters is what ends up in memory.

`tools/dump_overlay.py --seg 0x418f --size 0x5c0 --out out/res/SX_SND.mem`
writes the driver as loaded. The `--seg` is needed because the dumper's usual
trick - whoever writes to A000 is the driver - cannot find a driver that never
touches the screen.

## Finding it

The game's sound module is segment `0x2619`, and like the video module it keeps
its state **in its own code segment** rather than in DGROUP. The driver's entry
is a far pointer at `cs:[0x1e7]` of that segment, which measured as
`418f:0000`. The DOS arena reports the block at `418f` as `0x5c` paragraphs, so
the whole driver is **1472 bytes**.

## What it is

Offset 0 is `jmp 0x36e`, and at offset 0x0a there is a banner:

```
stddrv%IBM PC or Compatible Internal Speaker
```

So the driver loaded on this machine is the **PC internal speaker** one. That
is a property of the run and not of the game, and the other drivers have now
been looked at.

## The nine devices, and the seven the installer offers

`SX.OVL` on disk is a **container**: a tag, a four-byte length, and the payload,
chained end to end. Following the lengths from offset 0 walks the whole file and
lands exactly on its last byte, which is how the list below is a list and not a
guess:

    SSM:   the container itself, 0x9b1b bytes
      GMD:  SBP:  M32:  NLD:  ADL:  PRO:  PS1:  STD:  ASB:  APA:  APS:

Eleven chunks. The game chooses between them with the byte at offset 1 of
`RESOURCE.CFG`, which indexes the table of names at DGROUP **0x4a1c**, and
`INSTALL.COM` carries the menu that writes that byte. Putting the three
together:

| `sound_device` | tag | in `SX.OVL` | the installer's words |
| --- | --- | --- | --- |
| 0 | `STD:` | yes | **1 IBM PC  Single Speaker** |
| 1 | `TAN:` | **no** | not offered |
| 2 | `ADL:` | yes | **3 AdLib Music Synthesizer** |
| 3 | `M32:` | yes | **2 Roland MT-32 or LAPC-1** |
| 4 | `SBP:` | yes | **5 Sound Blaster** |
| 5 | `PS1:` | yes | **6 PS/1 Audio Card** |
| 6 | `PRO:` | yes | **4 Pro Audio Music Synthesizer** |
| 7 | `GMD:` | yes | **7 General Midi** |
| 8 | `NLD:` | yes | not offered |

The menu's numbering is its own - it is a display order, and the installer says
"Only the hardware detected on the machine is displayed", so what a player saw
depended on their machine. The device *index* is the left-hand column.

`STD:` is confirmed rather than inferred: the loaded driver's own banner reads
`IBM PC or Compatible Internal Speaker`, which is the installer's line 1. The
other six pairings are name matches and nothing stronger, though only `SBP:`
against `ASB:` looks at all confusable and those are in different tables.

**`TAN:` and `NLD:` are named by the game and offered by nothing.** `TAN:` is
almost certainly Tandy - the *graphics* menu has "4 TANDY 16 Colors" - and this
build of `SX.OVL` carries no `TAN:` chunk at all, so the entry is a hole in the
table rather than a device. `NLD:` is present in the file and in the table and
is not offered, and nothing here says what it is.

## The five modules, which are not named anywhere

A second table at DGROUP **0x4a2e** is indexed by `sound_module`, the byte at
offset 2 of `RESOURCE.CFG`: `ASB:`, `APS:`, `ATD:`, `APA:`, `ADS:`. Three of
the five are in this `SX.OVL` - `ASB:`, `APA:`, `APS:` - and `ATD:` and `ADS:`
are not.

`setup_sound_device` loads a module *before* the driver, installs its
dispatcher, and then **loads the driver as well**. A module and a device are a
pair, not alternatives.

That sentence used to say the opposite here, on the strength of a `return 1` in
the port that the original does not have. At 0x286bf the answer from the
module's own call decides only whether the module is *kept*: non-zero jumps to
the driver half with it installed, zero tears it down again - 0x4aaa cleared,
0x0bbc6 told to stop, `free_for_kind`, the pointers zeroed - and then goes to
the driver half anyway. Both paths load the driver. The port's `return` was
unreachable behind a stub, so nothing could have caught it by running; it was
caught by being asked which module pairs with `GMD:` and going back to the
disassembly to answer.

**Nothing pairs a module with a device.** There is no table joining them and no
code deriving one from the other: `sound_device` indexes 0x4a1c and
`sound_module` indexes 0x4a2e, and the installer writes both bytes
independently. So the answer to "what digitised audio goes with General MIDI"
is "whichever module the machine had", and with `GMD:` in particular the module
is where *all* the sampled sound would come from, since General MIDI is note
data and carries none of its own.

**Nothing names them.** `INSTALL.COM` has no menu for them, `SX.OVL` carries no
readable strings, and the only banner we have is the speaker driver's. The
initial letter and the driver tags they resemble - `ASB:` beside `SBP:`, `APS:`
beside `PRO:`, `ATD:` beside `TAN:` - suggest they are the digitised-sound half
of the same cards, but that is a reading of four-letter abbreviations and it is
written here as one.

**This installation asks for neither.** `RESOURCE.CFG` is `02 00 fe`: device 0,
module -2, and `setup_sound_device` treats -2 as "skip the load entirely". So
the game plays through the PC speaker and no module is ever loaded, which is
why the two stubs on that path have never been reached.

## The interface

Every call enters at offset 0 and lands on the dispatcher at `0x36e`:

```
036e  push dx
036f  shl  bp, 1
0371  mov  dx, cs:[bp + 0x34a]
0376  call dx
0378  pop  dx
0379  retf
```

So the function number is in **BP**, the table is at `cs:0x34a`, and the entries
are near offsets called within the driver. The table has exactly **18 entries**
- it ends at 0x36e, where the dispatcher begins, which is how its length is
known rather than guessed.

| BP | entry | | BP | entry |
| --- | --- | --- | --- | --- |
| 0 | 0x05b0 | | 9 | *stub* |
| 1 | 0x05a8 | | 10 | 0x0410 |
| 2 | 0x0525 | | 11 | 0x0549 |
| 3 | *stub* | | 12 | 0x0529 |
| 4 | 0x037b | | 13 | 0x055b |
| 5 | 0x0386 | | 14 | *stub* |
| 6 | *stub* | | 15 | *stub* |
| 7 | 0x03a1 | | 16 | *stub* |
| 8 | *stub* | | 17 | 0x057d |

`0x037a` is a bare `ret` and fills seven entries - the same arrangement as
`VGA:0x0252` in the video driver, and worth knowing before assuming an unused
function number is a bug.

Arguments come in registers, not on the stack: the caller at image `0x27a86`
sets `AX`, `CH` and `CL` before the call. That is the AIL convention and it is
why these entries cannot be transcribed as ordinary cdecl functions.

## The whole driver, transcribed

`reconstruct/sxovl_spkr.c`. Every entry in the table above:

| BP | entry | what it does |
| --- | --- | --- |
| 0 | 0x05b0 | `sx_describe_0` - constants AX=0x01ff, CX=0x1201 |
| 1 | 0x05a8 | `sx_describe_1` - constants AX=0x05a8, CX=0x0f00 |
| 2 | 0x0525 | `sx_stop_all` - forwards to speaker-off |
| 4 | 0x037b | `sx_stop_note` - stop CH, only if it is the note sounding |
| 5 | 0x0386 | `sx_start_note` - start CH on channel AL |
| 7 | 0x03a1 | `sx_controller` - controllers 0x7b, 0x4b, 0x4e, 0x07 |
| 10 | 0x0410 | `sx_pitch_bend` |
| 11 | 0x0549 | `sx_param_349` |
| 12 | 0x0529 | `sx_param_345` |
| 13 | 0x055b | `sx_param_346` |
| 17 | 0x057d | `sx_query` |
| *stub* | 0x037a | `sx_nop` |

and the three internals they share: `sx_note_on` (0x0497), `sx_speaker_off`
(0x0480) and `sx_apply_bend` (0x04fd), over the divisor table at 0x0042.

**Which entries the game actually reaches** was measured, not assumed: only
0x037b, 0x0386, 0x03a1 and 0x0410, and they are entered **directly** - the
dispatcher at 0x36e never runs on these screens. Six of the seven verified
routines are checked against the original; `sx_apply_bend` is transcribed but
never called here.

## The driver's own state

| offset | what |
| --- | --- |
| 0x0042 | divisor source table, 381 entries, one per quarter semitone |
| 0x033c | last pitch bend, 14 bits |
| 0x0342 | bend magnitude in quarter semitones |
| 0x0343 | bend direction, 1 = up |
| 0x0344 | the note now sounding, 0 for none |
| 0x0345 | a parameter; note-on refuses while it is zero |
| 0x0346 | a parameter; note-on refuses while it is zero |
| 0x0347 | volume-non-zero flag; note-on refuses while it is zero |
| 0x0348 | the channel that holds the speaker, 0xff for none |
| 0x0349 | a parameter nothing else in the driver reads |

## Two faults, transcribed rather than fixed

- `sx_apply_bend` accepts indices up to 476 against a 381-entry table, so a
  large enough bend reads past the end into the driver's own code. It is
  reachable: a full-scale bend is 47 quarter semitones, so any note above about
  93 bent fully upward lands beyond the table.
- `sx_note_on` records the note at 0x344 **before** testing the three enable
  flags, so `sx_speaker_off` will afterwards try to silence a note that never
  sounded.

## What is still not established

What 0x0345, 0x0346 and 0x0349 mean, and what the constants `sx_describe_0` and
`sx_describe_1` answer are fields of. The names say only what the code does.


## Does this game have digitised audio? No stored samples - but the module makes them

`ASB:` - the module `RESOURCE.CFG` now selects - loads and identifies itself as
`audblast`, **"CMS Sound Blaster"**, at 418f:0000, with a dispatcher at cs:0xc8
over a sixteen-entry table at cs:0xa8. It drives 83 indirect `out dx` plus
ports 0x0a, 0x0b, 0x0c and 0x83, which are the 8237's mask, mode, flip-flop and
page registers. **The module can certainly play samples.**

The game has none for it. Three independent measurements:

- **Size.** All of the game's sound is one 74 KB file, `TIM.SX`: 36 records,
  175,852 bytes uncompressed. As PCM at 11 kHz that is sixteen seconds for the
  whole game, and a single one of the sixteen music records would be 1.33
  seconds. The resource archive holds exactly one `.SX` and no other sound
  file - there is no sample bank anywhere in the 162 resources.

- **Content.** Walking the loaded record list at DGROUP 0x4a88 in a running
  snapshot and reading a record's decompressed data gives, for sound id 12 -
  the one `play_sound(12)` raises in puzzle 12 - **36 distinct byte values in
  512 bytes**, almost all of them zero, with a thirty-byte parameter block at
  +0x22. Sampled audio uses nearly the whole range and has no long runs of
  zeros. That block is a synthesiser voice, not a waveform.

- **Shape.** The per-device records are banks of voices and the 1001..1016
  records are the sixteen freeform tunes the README's "1-9, a-g" keys select.
  Every device path plays notes.

**That is not the whole answer, and the first version of this section stopped
one step too early.** Walking the module's sixteen dispatch entries with proper
control flow - rather than the linear sweep that produced the first port list,
which was full of false decodes - gives 727 reachable instructions and a
textbook DMA sequence in order at 0x25d: mask channel 1 on port 0x0a, clear the
flip-flop on 0x0c, the address on 0x02, mode 0x49 on 0x0b, the page on 0x83,
the count on 0x03, unmask, and then **DSP command 0x14** - eight-bit
single-cycle DMA output. The buffer is `cs:[0x70]`, its page `cs:[0x39]`, its
length `cs:[0x6e]`.

So the module really does play PCM. With no stored samples anywhere in the
game, the only thing it can be playing is PCM it **renders itself** from the
note data - which is what a digitised module for a card with no synthesiser on
it is for, and what the `stosb` loops through its body are doing.

So: the game stores no samples, and the module generates them. Making the port
play what a Sound Blaster played means transcribing that renderer - 2414 bytes,
LZW-compressed in `SX.OVL`, about the size of the speaker driver already
transcribed - and emulating the DSP's 0x14 and the 8237's channel 1 well enough
to hand the buffer to the audio stream.

What the data does support is **music**: the note sequences are all there, and
`GMD:` is one of the nine devices with its own bank at record 7. Driving that
through a General MIDI synthesiser is the version of "make the game audible"
that this game's own data can actually satisfy.

## `ASB:`, the digitised-sound module

`reconstruct/src/sxovl_asb.c` is the whole of it: 2414 bytes decompressed, an
entry at `0xc8` that indexes sixteen far offsets at `cs:0xa8` with the function
number in AX and the caller's arguments at SS:SI. `out/ASB_MOD.mem` is the dump
every offset in that file was read from.

It is an ordinary Sound Blaster digitised-audio driver and it is complete:

- **Detection** walks 0x220, 0x240, 0x210, 0x230, 0x250, 0x260, and at each one
  resets the DSP and waits for 0xAA, does the 0xE0 identify handshake, reads
  the version with 0xE1, and then **provokes an interrupt to find its IRQ** -
  it hooks 2, 3, 5 and 7 (and 10 if the DSP says 3.00 or better), hands the
  card a single byte with DSP 0x14 and a length of zero, and sees which of the
  five probe handlers writes its own number into `cs:[0x45]`.
- **Playing** is DMA channel 1 and DSP 0x14, single-cycle. A sample that would
  cross a 64K DMA page is cut in two when it is handed over and the second half
  is started by the completion interrupt, which is also where looping happens.
- **Position** comes from reading the 8237's own current-count register back.

### How a sample is actually started

Not through the wrappers. There are exactly two `lcall [0x4a98]` in the image,
both in the trampoline at 0x0bbd4, and the nine wrappers that reach it ask for
functions 0, 1, 2, 6, 9, 10, 11, 12 and 13 - never 3, the one that plays. That
much is true and it is why looking only there gives the wrong answer, which
this file gave for a while: *"the game never asks it to play"*.

It asks somewhere else. `setup_sound_device` also installs the module's far
pointer as the **host callback**, at the sound module's own `cs:0x30f6`, and
`sound_callback` at 0x292a1 calls through it with the function number in AX and
an argument pointer in SI - the module's own dispatcher convention, because it
is the module's own dispatcher. Three call sites reach it:

| site | function | argument block |
| --- | --- | --- |
| 0x26f1d | 5 | none |
| 0x27bfd | **3** | five words |
| 0x27c1a | 4 | one word |

`add sp, 0xe` after the middle one is the proof of the block's size: fourteen
bytes, of which four are the call's own two arguments and ten are the five
words `ASB:` function 3 reads - flags, rate, the sample's far pointer, its
length. The pushes at 0x27bda..0x27bf3 build exactly that, out of a record
reached by following two far pointers from the sequence and indexing with the
low nibble of +0x165. **So a digitised sound is a record inside the music
data**, and `poll_sequences` is the one place in the game that starts one.

The port builds those blocks now; it used to skip them, on the reasoning that
the callback was a stub so the stack was never read. Question 5's call site was
also being read forwards - `xor ax,ax; push ax; mov ax,5; push ax` is function
**5** with a null pointer, and the port had it as function 0.

With no module installed the answer to question 4 is the DGROUP segment, left
in AX by the `mov ax,0x2d3c` two instructions earlier - a relocation, not the
constant - and its low byte is non-zero, so every sequence on the table is
removed. That is a machine with no digitised sound, and it is why the game is
happy without one.

### Why it is never asked: there is no sampled data

A track is digitised when its **first byte is 0xFE**. `start_sequence`'s walk
tests exactly that - `dl == 0xfe` at 0x26... - and when it is true *and*
`cs:0x200` says a module is installed, it stops the walk and records how far it
got in +0x165. That byte is what puts the sequence on the poll table, and
`poll_sequences` is what asks question 3. So the whole digitised path hangs off
one marker byte in the music data.

**No track in this game's data has it.** Measured by instrumenting the test
itself - before the `cs:0x200` gate, so the module's presence cannot hide it -
across the two intro screens and a level-one machine actually running, driven
with `TIM_CLICK=200:320:200,400:78:105`: the branch is never taken, not once.
`cs:0x200` is non-zero on that run, because loading a module forces its bit 0
in `install_driver`, so the gate was open and nothing came through it.

That is the answer to "why does the Sound Blaster module never play anything".
Not a missing call site - the call site is at 0x27bfd and is transcribed - and
not a gap in the module, which is complete. The game simply ships no sampled
tracks, so `ASB:` installs, detects the card, and waits for work that never
arrives.

The scope of that measurement is the two intro screens and level one; a track
elsewhere in the game would show up the same way, and the instrumentation to
find it is two lines beside the `dl == 0xfe` test.

### Still not heard

The path is complete and nothing on it is a stub, but **the two intro screens
do not start a sample**: with `02 00 00` the module installs, detects the card
at 0x220, finds IRQ 7 and sets 11025 Hz, and `sound_callback` is then never
reached at all - the table at `cs:0x48` that `poll_sequences` walks is empty
there. The original agrees, measured under the emulator with `--blaster`:
`dsp_commands: {}` after forty seconds.

So the intro's audio is *music*. The section above says why, and it is not a
property of the intro: there is no sampled data to play.

## `GMD:`, General MIDI, and where the port reaches it

`out/res/SX_GMD.mem` is the driver as loaded - 2592 bytes at segment 0x418f,
banner `dude%General MIDI for Roland MPU interface`. It has the same shape as
the speaker's: `jmp 0x5aa` at offset 0, a dispatcher that indexes eighteen near
offsets at `cs:0x586` with the function number in BP.

| BP | entry | | BP | entry |
| --- | --- | --- | --- | --- |
| 0 | 0x0a08 | | 9 | *ret* |
| 1 | 0x0984 | | 10 | 0x07de |
| 2 | 0x082c | | 11 | 0x05b7 |
| 3 | *ret* | | 12 | 0x0896 |
| 4 | 0x05c9 | | 13 | 0x08d2 |
| 5 | 0x0617 | | 14-16 | *ret* |
| 6 | *ret* | | 17 | 0x0918 |
| 7 | 0x0685 | | | |

The hardware end is three routines and nothing else: `0x0868` writes a data
byte to 0x330 once the status port says there is room, `0x0832` writes a
command to 0x331 and waits for the 0xFE acknowledgement, and `0x0807`
assembles a MIDI message from a status byte and one or two data bytes -
skipping the second for 0xC0 and 0xD0, as MIDI requires.

So **the port's side of this is an MPU-401 and a synthesiser**, both of which
it now has: `io.c` answers 0x330 and 0x331, and `io_on_midi` hands the byte
stream to FluidSynth in `sdl.c`, on its own SDL stream beside the speaker's and
the Sound Blaster's.

That boundary is **verified against the original**, which is the point of
having a hybrid: with `RESOURCE.CFG` set to `02 07 fe` the hybrid runs the
original's own `GMD:` code, and it comes out as real MIDI - the Roland GS reset
`f0 41 10 42 12 40 00 7f 00 41 f7`, the RPN pair that sets pitch-bend range,
and per-channel controller writes. No guessing was involved in any of it.

### Done

`reconstruct/src/sxovl_gmd.c` is the driver and `reconstruct/src/sxovl.c` is
the dispatch layer the port needs and the original does not - the original's
call site is an `lcall [0x1e7]`, so it names a function and never a driver,
while C names a function *in* a driver. `driver_kind` reads the banner at
offset 0x0a rather than remembering what `RESOURCE.CFG` asked for, because
`setup_sound_device` can fall back to a different chunk than the one named.

With `RESOURCE.CFG` set to `02 07 fe` the port plays the intro as General MIDI:
23,541 bytes at the MPU across forty-five seconds, of which 5,144 are note-ons,
1,404 controllers and 1,262 pitch bends, and the first note to reach FluidSynth
is channel 10, note 42, velocity 127 - a closed hi-hat on the percussion
channel, which is the map at `cs:0x1c1` doing its job.

**Two registers were being dropped, and only this driver could show it.** The
port called function 5 as `sx_start_note(ax & 0xf, note << 8)` and function 4
as `sx_stop_note(note << 8)`, so CL - the velocity - arrived as zero and the
channel did not arrive at all. The original does `and al, 0xf` before both and
never touches CX: CH is the note and CL the velocity, read at 0x27e93/0x27ea0
and 0x27ee2/0x27eef. The speaker driver reads neither register, so no screen
and no sound the port could make would have disagreed. `GMD:` reads both - the
velocity through the curve at `cs:0x2c2`, the channel to pick the voice - and
a hi-hat at velocity 127 is what the fix sounds like.

The same shape caught `configure_driver_far`, whose argument this file used to
call dead.

### Verified against the original

Behaving plausibly is not the standard here, and 538 lines of freshly
transcribed driver is exactly what this project says not to trust - three
argument bugs turned up today in code that ran and sounded fine. So `GMD:` has
verifier specs, and `tools/fixture.py --sound-device 7` builds the game
directory they need: the shipped `RESOURCE.CFG` asks for the speaker, so every
other chunk is dead code against the real folder and `sx_seg` finds whichever
one the loader actually put there.

    uv run python tools/fixture.py --out out/gmd --sound-device 7
    uv run python tools/verify.py --all --game-dir out/gmd \
        --budget 120000000 --only gmd_write_data,gmd_param_345,gmd_param_349

| routine | | |
| --- | --- | --- |
| `gmd_write_data` | `GMD:0x0868` | **verified**, 155 calls |
| `gmd_param_345` | `GMD:0x0896` | **verified**, 2 calls |
| `gmd_param_349` | `GMD:0x05b7` | **verified**, 1 call |
| `poll_sequences` | `0x27b7e` | **verified**, 2500 calls |
| `sound_service` | `0x27ace` | **verified**, 2500 calls |
| `install_driver` | `0x265f2` | **verified** under `GMD:` |
| `configure_driver` | `0x26629` | **verified** under `GMD:` - so `gmd_init` does |

`poll_sequences` is the one that matters for the digitised path: it is the
routine rewritten to build the five-word block that starts a sample, with two
`dg_enter` frames it did not have before, and 2500 agreeing calls is a great
deal better than the desk check it would otherwise rest on.

`gmd_write_data` is the one that matters most: every MIDI byte the driver ever
sends goes through it, and it is the routine that touches the port.

The note routines are **not reached**, and chasing why produced the finding
below rather than a verification. The chase is worth recording because two of
its steps were wrong.

`gmd_write_data` is called **exactly 155 times over 700,000,000 instructions**
- the same 155 as over 120,000,000. It initialises and then falls silent. Those
155 are the Roland GS reset SysEx, the RPN pair that sets pitch-bend range, and
the nine controller-7 writes `gmd_param_345(0x0c)` sends on its way out of
`gmd_init`, and nothing after them.

Reading that count needed `--occurrences 5000-5001`: the collect phase **stops
as soon as a spec's requested occurrences are satisfied**, so a plain run
reports 11 calls and exits in six seconds. A count from a satisfied spec is not
a total, and a budget is only spent when something is still wanted.

Nor was the budget the problem. Flip 690 is 117,000,000 instructions in, so
700M was already far past the intro and most of the game.

## Music plays with the speaker and not with General MIDI

The control is the useful part. From **one** state - flip 690, inside freeform
with the panel up, reached by the clicks STATUS.md records - with only the
device byte different:

| device | | |
| --- | --- | --- |
| `STD:` speaker | `sx_start_note` 86 calls, `sx_stop_note` 85 | verified |
| `GMD:` | every driver routine | **never called** |

And in the same GMD run, `sound_service` and `poll_sequences` are called 2500
times each and both verify. So the timer is driving the sound module normally;
what is missing is a **sequence**. `sequencer_tick` is never called, so nothing
is playing for the driver to be told about.

### General Midi is silent, because of a missing jump in the original

`load_sound_bank` picks a bank identifier from the device at DGROUP 0x4aae and
then opens the resource to find it. The switch is a jump table for 0 to 3 and
compares for the rest, and **device 7 has no jump out of its arm**:

    28a4c  mov byte [bp-5], 7    ; the identifier for GMD:
    28a50  xor dx, dx            ; ...and straight into the null return,
    28a52  xor ax, ax            ;    which is where `default` goes
    28a59  retf

Every other case ends `jmp 0x28a5a` and goes on to open the resource. Case 7
stores its identifier and falls through, so the store is dead and the answer is
always null. **`GMD:` can never load a sound bank**, and General Midi is silent
on this build however good the driver is.

That is a bug in Dynamix's code, not in the transcription - but the
transcription had quietly fixed it. The port wrote `case 7: want = 7; break;`,
which is the case the author meant and not the case they wrote, and the port
then played 5,144 note-ons in forty-five seconds of a screen the original plays
in silence. It is now `goto out`, and measured after the change: **0 note bytes
under `GMD:`**, with `load_sound_bank` and `open_sound_file` **verified** over
22 calls each where they had differed, and `build_sound_index` and
`start_sequence` never called on either side.

This is the sharpest example this project has of why "it sounds right" is not
the standard. The music was real MIDI, correctly formed, on the right channels,
with a working drum map - and it was wrong, because the original does not play
it. Only a byte-level comparison against the original could have said so, and
what gave it away was a *device nobody had ever selected* being verified for
the first time.

### The bank that is missing anyway

`load_sound_bank` picks a bank identifier from the device at DGROUP 0x4aae -
0x12 for the speaker, 7 for `GMD:` - and looks for that record in the resource.
Measured from the entry point with each device, the same routine either side:

| device | `load_sound_bank` | `build_sound_index` | `start_sequence` |
| --- | --- | --- | --- |
| `STD:` speaker | verified, 22 calls | verified, 3 calls | verified, 3 calls |
| `GMD:` | **returns null**, original and port alike | never called | never called |

The table above is the measurement that led there; the cause is the missing
jump. Whether a bank with identifier 7 exists in the resource at all is
untested and now unanswerable from the game, because the code that would look
for it never runs.

The driver itself is not the fault, and that is worth separating out - under
`GMD:`, `install_driver` and `configure_driver` both **verify**, so
`gmd_describe_0` and the whole of `gmd_init` - the 0x481-byte bank copy, the
stored MIDI sequence, the master-volume call - agree with the original. The
transcription of `GMD:` is sound; it simply has nothing to play.

`select_music` differs under **both** devices, with the verifier's own
allocation-underrun caveat attached, so it is a separate pre-existing question
and not this one.

### What is not done

`sound.c` still calls the **speaker's** routines by name - `sx_start_note` and
friends - wherever the original does an `lcall [0x1e7]` with a function number
in BP. So notes never reach whichever driver is loaded, which is why the run
above shows the driver's own initialisation and no note-ons. Two things follow:

1. A dispatch layer, so a call site picks the loaded driver rather than the
   speaker. The original needs none because its call site is indirect.
2. `GMD:` transcribed. The note path is small and completely read - note on and
   off are 0x0617 and 0x05c9, both ending in a 0x90 status with the velocity
   run through a per-instrument curve at `cs:0x2c2`; controllers are 0x0685,
   with volume scaled by the master at `cs:0x4c2`; pitch bend is 0x07de.

   **Its initialisation is the part with a dependency.** BP=1 at 0x0984 copies
   a 0x481-byte configuration blob from `ES:AX` into its own segment and then
   plays a stored MIDI sequence out of it, and `configure_driver` at 0x26629
   sets neither ES nor AX - so the blob is left there by whatever ran before.
   That blob is a **patch bank**, and the numbered chunks this container has
   past the drivers and modules - `001:`, `003:`, `004:`, `101:` - are where it
   comes from. `load_sound_bank` switching on DGROUP 0x4aae is the thread to
   pull.
