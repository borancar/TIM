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

### What the verifier found in the module

`ASB:` has specs too - `tools/verify.py --blaster`, against a directory built
with `tools/fixture.py --sound-module 0`. The `--blaster` flag is new and
necessary: the module probes six base ports and gives up when none answers, and
the port's `io.c` always has a card, so without it the two sides would differ
over the harness rather than the code.

It found a real bug straight away. The original's `setup_sound_device` writes
0xfb, 0xf3, 0xd3, 0x53 to port **0x021** - the master PIC's mask, being cleared
for IRQs 2, 3, 5 and 7 by `asb_probe_irq` - and the port wrote them to port
**0x000**. `cs:0x74` holds that port number and is **zero** in the module's
static data; the routine's first instruction sets it:

    07be  mov word ptr cs:[0x74], 0x21
    07c5  mov al, 2                      <- where the transcription started

The routine was read from 0x07c5. The seven bytes before it sit immediately
after the five saved vectors at 0x7a5, disassemble as plausible data, and
0x069c's `call 0x7be` was never dumped from - which is the trap in CLAUDE.md
about a jump landing one byte past what you read, in its call-target form.

The port writes to 0x021 now, and the four writes are in the original's order.
Their *values* still differ - the port writes 0x00 where the original writes
0xfb, 0xf3, 0xd3, 0x53 - because `io.c` does not model the PIC mask registers,
so the read-modify-write reads zero.

**Modelling them was tried and reverted**, which is worth recording. Making
0x21 and 0xa1 latches starting at 0xff did make these four values match, and it
broke `start_sound`, which had been verified: the emulator's own latch has been
driven to 0x00 by the game long before that call, while the port's - entered in
isolation by the verifier - was still at its initial value, so the port wrote
0xfc where the original wrote 0x00. That is port state the harness does not
prime, the way it primes the VGA registers and planes.

The latch had **no functional effect** either way: `io.c` delivers the card's
interrupt by calling a C function, not by any mask. So it bought cosmetic
agreement in a comparison that cannot complete anyway and cost a real
regression in one that was passing, and it is gone.

### The module cannot be verified past detection

Beyond that point the two sides part company for a reason that is neither's
code. The original goes on to `out 0x246 = 01/00` and `out 0x216 = 01/00`,
probing bases 0x240 and 0x210, because **detection failed at 0x220** - the
emulator's `--blaster` card does not complete the reset-and-identify handshake
the module wants. The port's own card answers, so the port succeeds where the
original gives up, and `setup_sound_device` returns 1 against the original's 0.

So the module's detection and playing routines report `NEVER CALLED`: the
original tears the module down before any of them runs. A reference that does
not have the device cannot adjudicate the device, and the emulator is pinned in
`pyproject.toml` - moving that pin is a deliberate act with a re-run of the
whole sweep behind it, not something to do for one module.

What is established: `load_sound_module` verifies, the two bugs above were
found and fixed by comparison rather than by reading, and everything past the
handshake rests on the transcription being read carefully. That is weaker than
the rest of this work and is written down as weaker.

**The hybrid gets further, and says where the remaining boundary is.** Running
`tools/native/native` with device 0 and module 0, the *original's* module code
meets the *port's* card - and completes the whole handshake the emulator's card
refuses: reset, the 0xE0 identify, the 0xE1 version, the time constant, and the
one-byte transfer that provokes the interrupt. So `io.c`'s Sound Blaster is
good enough for the original's detection, which is worth knowing on its own.

It stops at one line: `io: sb irq 0000`, the completion firing with no handler.
`io_on_sb_irq` takes a **C function pointer**, and under the hybrid the handler
is guest code at an interrupt vector - the original's `asb_hook_irq` wrote the
real IVT and never called the port's shim. The module then finds no IRQ, closes
its file and takes itself down, which is the `int 21h AH=3E` from `418f:021a`
that the run traps on.

So neither reference can adjudicate the module past detection, and for
different reasons: the emulator has no card that answers, and the hybrid has no
way to deliver the card's interrupt to guest code. Closing the second would
mean `io.c` asking the runner to inject an interrupt rather than calling a
function - a real change across the IO boundary, and hard to justify for a
module this game never asks to play.

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

### The whole module, swept after the refactor

Adding the driver dispatch layer rewrote about thirty call sites in `sound.c`,
and `poll_sequences`, `sound_callback`, `configure_driver`, `stop_sound` and
`load_sound_bank` were edited outright. So every spec in the sound module's
address range - 77 of them, plus the speaker driver's - was run twice:

| from | verified | differs | not verified |
| --- | --- | --- | --- |
| the entry point | **58** | 0 | 0 |
| the flip-690 snapshot | **15** | 0 | 0 |

Nothing regressed. That sweep is worth repeating after any change to this
module, and it is cheap - a few minutes each - beside the twenty-minute one
that produced nothing.

`io.c` changed too, and on the **display** path: `io_service_display` calls
`io_sb_poll` before it does anything else, so the card's completion interrupt
is delivered from the same place the frame is. Two screen comparisons say that
cost nothing - `tools/check_briefing.py --screen briefing` and `--screen
picker`, three flips each, **0 of 307,200 pixels differing** on all six.

It is also how the one regression of the day was caught. Modelling the PIC mask
registers broke `start_sound`, which had verified hours earlier; see the note
about the latch below.

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
**`GMD:` compares the same way, and covers most of the driver.** With device 7
the port and the original both run the driver's initialisation against the same
MPU-401, so `TIM_TRACE=midi` on each and a `diff` answers it: **159 events each
and identical**. That path is not small - it exercises

  `gmd_reset`, `gmd_write_command`, `gmd_delay`, `gmd_write_data`, `gmd_send`,
  `gmd_init` - the 0x481-byte bank copy and the stored MIDI sequence replayed
  out of it - `gmd_param_345`, and `gmd_controller`, because setting the master
  volume re-sends controller 7 for channels 1 to 9.

so every byte of the Roland GS reset, the RPN pair and the nine volume writes
comes out of the transcription exactly as it comes out of the original. What it
does not reach is the note path, which nothing on this build ever calls, and
`gmd_query`, `gmd_stop_all`, `gmd_pitch_bend` and `gmd_param_349`.

| `gmd_write_data` | `GMD:0x0868` | **verified**, 155 calls |
| `gmd_param_345` | `GMD:0x0896` | **verified**, 2 calls |
| `gmd_param_349` | `GMD:0x05b7` | **verified**, 1 call |
| `poll_sequences` | `0x27b7e` | **verified**, 2500 calls |
| `sound_service` | `0x27ace` | **verified**, 2500 calls |
| `install_driver` | `0x265f2` | **verified** under `GMD:` |
| `configure_driver` | `0x26629` | **verified** under `GMD:` - so `gmd_init` does |
| `midi_note_event` | `0x27ee1` | **verified**, 502 calls |

**`midi_note_off_event` is never called, and probably cannot be.** The 0x8n
handler at 0x27e92 took **zero** calls on both runs while the 0x9n one at
0x27ee1 took 362 and 502. That is the ordinary MIDI convention rather than an
accident: a note is released by a note-*on* with velocity zero, which is what
the game's own drivers emit - `gmd_stop_note` sends a 0x90 status with CL set
to 0 rather than a 0x80. So this data appears to contain no 0x8n events at all,
and the routine is dead in practice.

It was edited today all the same - it had the same dropped velocity and channel
as its sibling - so that edit rests on the disassembly at 0x27ea0 and 0x27ed3
and on nothing that runs. `midi_event_6`, `midi_event_9` and `midi_skip_event`
are in the same position, unreached from either starting point.

**What those 502 calls do not prove.** `midi_note_event` is where the dropped
velocity and channel were fixed, and it was verified against the *speaker*,
which is the only device that plays. The speaker's function 4 and 5 read
neither CL nor AL, so a run that passed the wrong ones would agree exactly as
well - which is how the bug survived in the first place. The fix rests on the
original's own instructions, `and al, 0xf` before both calls and CX untouched
from where CH and CL were loaded at 0x27e93 and 0x27ea0, and the 502 calls say
only that the rest of the routine still agrees. `GMD:` would observe both
registers and is silent, so no runnable configuration can see them.

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

That is a bug in Dynamix's code, not in the transcription - and the
transcription had quietly fixed it, playing 5,144 note-ons in forty-five
seconds of a screen the original plays in silence.

**It is now fixed on purpose.** Asked which was wanted, the project owner chose
the author's arm over the author's typo, so `case 7` is `break` and General
Midi plays: 3,424 note bytes in thirty seconds, rendered by FluidSynth. This is
the **only deliberate deviation in `reconstruct/src`**, it says so at the case
itself, and the cost is measured rather than assumed - `load_sound_bank`
**DIFFERS under device 7** over 22 calls, exactly as it should, while device 0
still verifies over 12 and `build_sound_index` over 2.

The history is kept because the finding is the useful part: the bug was
invisible until a device nobody had ever selected was verified for the first
time, and the port sounding *better* than the original is what gave it away.

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

### Device 4 is silent too, which is how the explanation was tested

If the fall-through is really the cause, then **`SBP:` should be silent for the
same reason**: `cmp bx,5 / jg` and then `cmp bx,3 / ja` send device 4 to the
same null return, because the jump table only covers 0 to 3. Measured with
`--sound-device 4`: `start_sequence`, `build_sound_index` and
`midi_note_event` are **never called**, and `load_sound_bank` **verifies** over
22 calls. The prediction holds, which is better evidence for the reading than
the original finding was on its own.

So of the nine devices, only 0, 1, 2, 3, 5 and 6 get a bank **identifier** out
of that switch. The two that do not are `SBP:` - the installer's line 5,
"Sound Blaster" - and `GMD:`.

**That is a fact about the switch, and all nine devices were then measured
against it.** `load_sound_bank` and `build_sound_index` verify for every device
that takes an arm, and `build_sound_index` is never reached for the two that
fall through:

| device | | bank | index |
| --- | --- | --- | --- |
| 0 | `STD:` | verified | verified |
| 1 | `TAN:` | verified | verified |
| 2 | `ADL:` | verified | verified |
| 3 | `M32:` | verified | verified |
| **4** | **`SBP:`** | **null** | **never reached** |
| 5 | `PS1:` | verified | verified |
| 6 | `PRO:` | verified | verified |
| **7** | **`GMD:`** | **null** | **never reached** |
| 8 | `NLD:` | recorded as 3, so it takes device 3's arm | |

Six devices get a bank and an index built from it; two get neither. That is the
fall-through's consequence seen from every side there is, and it settles the
reading of the switch rather than leaving it a plausible one.

Device 1 is worth a second look: `TAN:` has **no chunk in this `SX.OVL` at
all**, and it still gets a bank, because `load_sound_bank` reads only the
device number at DGROUP 0x4aae and never asks whether the driver loaded. A
missing driver and a missing bank are independent failures here.

What that does *not* settle is whether `ADL:` then makes a sound: the original
wrote nothing to 0x388 or 0x389 in 150 seconds, not even the register probe an
AdLib driver uses to find the chip. That is one step further along than the
bank - the driver's own detection, against an emulator with no OPL2 - and it is
the same wall the `ASB:` module meets. Not established here, and not needed for
the bank question.

The port cannot help either way: it has no `ADL:` body, so `driver_kind` stops
as it should. Devices 1, 3, 5 and 6 are untested.

### The banner's name does not identify the driver

That run also found a flaw in `driver_kind`. The banner is a short name, a
**length** byte, and a description that many characters long: `stddrv` 0x25
"IBM PC or Compatible Internal Speaker", and `dude` 0x25 "General MIDI for
Roland MPU interface" - both descriptions 37 characters, which is what 0x25 is.

**`SBP:` is also called `dude`.** Its banner is `dude` then "Sound Blaster Pro
2.24". So matching "dude%" told `GMD:` from `SBP:` only by their descriptions
happening to be the same length, which is a coincidence. The description is
matched now, and searched for rather than indexed, because the name in front of
it is not a fixed width either.

And an unknown driver **aborts** instead of answering zero. It used to return
0 from every dispatcher, which is a stub returning quietly - the game would
have taken that for a description of the device and carried on. Now it stops
and quotes what the driver calls itself, which is how "Sound Blaster Pro 2.24"
came to be readable at all.

### What is done, and what is left

Both were written when neither was true, so they are worth stating plainly at
the end.

**Done.** The dispatch layer is `reconstruct/src/sxovl.c` - the original needs
none, because its call site is an `lcall [0x1e7]` naming a function and never a
driver. `GMD:` is `reconstruct/src/sxovl_gmd.c`, `ASB:` is
`reconstruct/src/sxovl_asb.c`, and the MPU-401 and the Sound Blaster are in
`io.c` with FluidSynth behind the first in `sdl.c`. The whole sound module
sweeps clean from two starting points, and two screens are pixel-identical
after the `io.c` changes.

**Left, in the order a reader would want them:**

1. **`ASB:` runs in the hybrid now**, and this was the item that said it could
   not. The *original's* module code, against the *port's* card, completes the
   whole of `asb_install` with no trap: reset, the 0xE0 identify, the 0xE1
   version, the one-byte transfer that provokes an interrupt, **the interrupt
   delivered and its handler run**, DSP 0xD1 to turn the speaker on, and the
   default rate. Ninety seconds, nothing trapped.

   Four things were needed and each was a separate mistake to make:

   - **Deliver the interrupt at all.** `io_sb_irq_owed`, `io_sb_irq_take` and
     `io_sb_irq_delivered` are the boundary; `deliver_int` in the runner was
     already general enough to take any vector. Take and deliver are separate
     because the runner can refuse, and an interrupt that was not delivered has
     not happened.
   - **Have interrupts enabled.** The runner never wrote FLAGS at start-up, so
     the guest ran with IF clear from its first instruction - see STATUS.md.
   - **End the slice when the card is handed a block.** Interrupts arrive
     between slices, and the module spins about twelve thousand instructions
     waiting to be preempted, which fits inside a 200,000-instruction slice
     with room to spare. Shortening the *next* slice is too late.
   - **And then not deliver it immediately.** `asb_probe_irq` writes DSP 0x14
     and *then* zeroes the byte its handler sets, so an interrupt in between
     has its answer wiped and the module still finds no IRQ - which looks
     exactly like arriving too late. On the original the 156-microsecond
     transfer puts it about 700 instructions later, inside the spin. One
     512-instruction slice of settle is the closest this runner has.

   `int 21h AH=34h` is a third exception beside AH=25h and AH=35h, on the same
   grounds: it hands back a pointer rather than performing a service, and with
   no DOS here the InDOS flag it points at is honestly zero.

   **Checked**: `check_native.py --screen "the title screen"` still reproduces
   every one of the port's sixteen flips byte for byte, which matters because
   the timer is now genuinely delivered where before it never was.

   **And it makes the transcription checkable without building anything.** The
   port's `ASB:` and the original's `ASB:` now drive the *same* `io.c`, so
   their hardware traces can simply be diffed:

       TIM_TRACE=sb ./reconstruct/devtim      # the transcription
       TIM_TRACE=sb tools/native/native       # the original's own module

   Nine hardware events each, and **identical** - reset high, reset low, DSP
   0xE0, DSP 0xE1, the time constant, DSP 0x14, the block, DSP 0xD1 and the
   rate, in that order. The only lines that differ are `hook`, which is the
   port's own `io_on_sb_irq` registration and not a port write at all: the
   original hooks by writing the IVT, so it has no equivalent to trace.

   That is a behavioural comparison rather than a routine-by-routine one - it
   says the two produce the same hardware in the same order along the install
   path, not that every routine agrees. It is a great deal better than the desk
   check it replaces, and it cost one `diff`.

2. **Whether `ADL:` and the other four unimplemented devices sound.** They get
   a bank and build an index, which is measured; whether their drivers then
   reach their hardware is not, and the emulator has none of that hardware.
   The port stops on them by design, naming the driver.

3. **The `GMD:` note routines.** Transcribed and desk-checked, and
   unverifiable on this build for the best possible reason: General Midi never
   plays, so nothing ever calls them. That is the game's own bug and not a gap
   here.

4. **The velocity and channel on driver functions 4 and 5.** Fixed from the
   disassembly and invisible to every runnable configuration, because the only
   device that plays is the one that reads neither register.

None of these is a stub. Every one is a place where the evidence runs out, and
they are listed so that the next reader does not mistake "not verified" for
"not written".

## `ADL:`, the AdLib driver - scoped, not yet transcribed

The FM path is what a Sound Blaster owner actually heard, and what DOSBox plays
where our emulator is silent: `emulator.py` answers a constant for 0x388 and
`sb.py` says of the FM registers "ignored". So the reference cannot tell us
anything about this driver, and DOSBox is the only witness we have.

**It loads at a different segment from the others.** `dump_overlay.py --seg
0x418f` finds the speaker and General Midi drivers; `ADL:` lands at 0x502a on
the same run, and dumping the wrong segment gives 4 KB of zeros that looks like
a driver with no code in it. Read it from `SNDCS:0x1e7` rather than assuming.

    tools/dump_overlay.py --seg 0x502a --size 0x2450 --out out/res/SX_ADL.mem

9,296 bytes, banner `dude` 0x1c "AdLib Music Synthesizer Card" - the same
name-length-description shape the other two have, and the third driver to call
itself `dude`.

### It is smaller than it looks

| region | size | what |
| --- | --- | --- |
| 0x0000-0x03ff | ~1 KB | the banner, the port variables, lookup tables |
| 0x0400-0x1920 | ~5.4 KB | **all zeros** - per-voice and per-channel state |
| 0x1921-0x2450 | ~2.9 KB | the dispatch table and every routine |

So the code is 2,900 bytes against `GMD:`'s 2,592 - the same size of job, not
the three-and-a-half times the file length suggests.

### The interface is the one already handled

Dispatcher at 0x1945, table at `cs:0x1921`, eighteen entries, function number
in BP - identical to `SPKR:` and `GMD:`, so `reconstruct/src/sxovl.c` takes it
unchanged.

| BP | | BP | |
| --- | --- | --- | --- |
| 0 | 0x2446 describe | 10 | 0x1a29 pitch bend |
| 1 | 0x2414 init | 11 | 0x1abf param 349 |
| 2 | 0x1ad0 stop all | 12 | 0x1a8d param 345 |
| 4 | 0x1952 stop note | 13 | 0x1a68 param 346 |
| 5 | 0x1988 start note | 17 | 0x23a7 query |
| 7 | 0x19cf controller | 3, 6, 9, 14-16 | bare `ret` |

**Nine melodic voices**, walked as `bx` 0..8, with per-voice arrays at 0x190
(the channel it is playing for), 0x19b (the note) and 0x1b1 (sustained), and
per-MIDI-channel arrays at 0x120 (program) and 0x150 (sustain). Start-note
range-checks the note to 12..107 and does `shr cl,1`, turning MIDI's 7-bit
velocity into the OPL's 6-bit attenuation. A velocity of zero is a note-off,
the same convention the rest of this game uses.

### And the hardware is thirty lines

Every register write goes through **0x208e**, and it is the textbook AdLib
sequence:

    mov dx, cs:[0x37]     ; 0x388, the address port
    out dx, al            ; BL, the register number
    in  al, dx  x5        ; the chip's ~3.3us address delay
    mov dx, cs:[0x3b]     ; 0x389, the data port
    out dx, al            ; CL, the value
    mov cx, 0x21
    mov dx, cs:[0x39]     ; 0x388 again
    in  al, dx  x33       ; the ~23us data delay

The three ports live at `cs:0x37`, `cs:0x39` and `cs:0x3b` - which is why a
byte search for 0x0388 in this driver finds them in what looks like a table.

So `io.c` needs only: latch an index on a write to 0x388, hand `(register,
value)` to a synthesiser on a write to 0x389, and answer a status byte on a
read. **The synthesis is the whole of the remaining work**, and it is the part
worth taking rather than writing - `adplug-devel` is packaged and exposes
`Copl::write(reg, val)` and `Copl::update(buf, n)`, which is the shape `io.c`
already uses for FluidSynth. Nuked OPL3 (LGPL, two files) and ymfm (BSD) are
the vendorable alternatives.

**And the driver can be verified before any of that exists.** Accept-and-ignore
in `io.c` is enough for the trace-diff above: the port's `ADL:` and the
original's both write 0x388 and 0x389, so `diff` compares them exactly as it
did for `ASB:` and `GMD:`. The synthesis decides whether it is *audible*, not
whether it is *right*.

### The routine map, read but not yet written

State, all in the driver's own segment:

| at | per | what |
| --- | --- | --- |
| 0x0037, 0x0039, 0x003b | - | the three port variables: 0x388, 0x388, 0x389 |
| 0x003d | 48 words | the F-number table, one per quarter-tone of an octave |
| 0x011d, 0x011e, 0x011f | - | parameters 349, 346 and the master volume |
| 0x0120, 0x0130, 0x0140, 0x0150, 0x0160 | channel | program, volume, pan, sustain, and 0x4b |
| 0x0170 | channel, word | pitch bend, 0x2000 being centre |
| 0x0190, 0x019b, 0x01a6, 0x01b1, 0x01bc | voice | channel, note, velocity, sustained, loaded program |
| 0x01c7..0x01cf | 9 | the voice rotation order, least recently used last |
| 0x01dd, 0x01ed | channel | voices allowed, voices in use |
| 0x0374 | 28 bytes each | the patch bank, copied in by the init |
| 0x183c | note | the percussion note map, for programs >= 0x80 |

Routines:

| at | what |
| --- | --- |
| 0x1945 | the dispatcher; table at 0x1921, eighteen entries, BP |
| 0x1952 | stop note - walk the nine voices, key off unless sustained |
| 0x1988 | start note - velocity 0 is a stop; notes 12..107; `shr cl,1` |
| 0x19cf | controller - 7, 0x0a, 0x40, 0x4b and 0x7b |
| 0x1a1b | program change, stored per channel |
| 0x1a29 | pitch bend - store, then re-tune every sounding voice |
| 0x1a68, 0x1a8d, 0x1abf | parameters 346, 345 (master volume) and 349 |
| 0x1ad0 | stop all, through the reset |
| 0x1ad4 | **allocate a voice** - a free one, else steal from the channel |
| | most over its allowance |
| 0x1d45 | key on - load the patch if the voice holds another, then 0x1e23 |
| 0x1dc4 | key off - 0x1e23 with dx zero, then release the voice |
| 0x1df4 | move a voice to the end of the rotation order |
| 0x1e23 | **the note itself** - percussion mapping, note times four, pitch |
| | bend, then registers 0xA0 and 0xB0 |
| 0x1eee | pitch bend to quarter-tones: `(bend - 0x2000) / 0xab`, clamped |
| 0x1fe1 | write one 28-byte patch to the chip |
| 0x208e | **the register write** - the sequence above |
| 0x237d | reset - zero registers 0x00..0xf5, then register 1 = 0x20 |
| 0x23a7 | query - and note it **zero-extends**, where `GMD:` keeps AH |
| 0x2414 | init - copy the patch bank from ES:AX, reset, master volume 15 |
| 0x2446 | describe - AX 0x0103, CX 0x0009, nine voices |

The controllers, read since:

| at | what |
| --- | --- |
| 0x1ca9 | volume - `shr cl,1`, store at 0x130, re-apply to every sounding voice |
| 0x1ce2 | pan - store at 0x140, re-apply the same way |
| 0x1d15 | sustain - store at 0x150; releasing it keys off every voice whose |
| | sustained flag at 0x1b1 is set |

**How a note's loudness is arrived at**, which is three multiplications and
not one, at 0x1ea0:

    (channel volume at 0x130, +1) x (velocity through the curve at 0x9d, +1)
        >> 6, then x (master at 0x11f, +1) >> 4, less one if non-zero

and then zero outright when `cs:0x11e` - parameter 346 - is clear. The result
goes through 0x1f45, which scales it by the patch's own total level, ors in
the key-scale bits, and writes register 0x40 plus the operator's slot.

**The tables that make the operators addressable**, all in the driver:

| at | what |
| --- | --- |
| 0x009d | the velocity curve, one byte per MIDI velocity |
| 0x00dd | the volume curve, one byte per computed level |
| 0x01fe | the carrier operator slot for each of the nine voices |
| 0x0207 | the modulator slot, likewise |
| 0x0222 | slot to register offset - the OPL2's 0x00,0x01,0x02,0x08... layout |
| 0x0246 | the same, for the patch writer at 0x2057 |
| 0x1892, 0x18a8 | per voice: the carrier's key-scale bits and total level |
| 0x18d4, 0x18ea | the modulator's, used only when the patch is additive |
| 0x1900 | per voice: whether the modulator is scaled as well as the carrier |

`0x1fe1` unpacks a 28-byte patch into those, choosing between the carrier-only
and additive arrangements on the byte at +0xc, and then writes the two
operators through 0x2109. `0x20b5` initialises all eighteen operators at
reset, from a thirteen-byte default at 0x258 or 0x266.

**Still unread: 0x2109**, the routine that writes one operator's registers,
and 0x1b52, the 0x4b controller. Everything else above is read.

### Transcribed, and 778 register writes deep

`reconstruct/src/sxovl_adl.c` is the driver: 43 routines, the eighteen-entry
interface `sxovl.c` already dispatched, and the port now runs AdLib without a
trap. `io.c` answers 0x388 and 0x389, `TIM_TRACE=opl` prints every register,
and the hybrid links the port's own OPL objects so there is **one chip between
the two** rather than a copy each.

The comparison is the trace-diff, and it works on this driver exactly as it did
on the other two - the hybrid runs the *original's* `ADL:` because
`install_driver`, `configure_driver` and `midi_note_event` are not dispatched,
so their `lcall [0x1e7]` reaches the guest's own copy.

**The first 778 register writes are identical.** That is the whole of the
initialisation - the reset, the eighteen operators' defaults, the patch bank -
and the first notes.

It found two real faults on the way, both from reading that stopped too early:

- **Two encoders were missing.** `0x2152` calls eight routines and I read six,
  so registers 0x20 - tremolo, vibrato, envelope type, key-scale rate and the
  multiplier - and 0xE0, the waveform, were never written. The diff named them
  in its first six lines.
- **Channel 9's program comes from the note.** `0x1d64` clamps the note to
  0x1b..0x58 and adds 0x65, landing in 0x80..0xbd - which is exactly the range
  `adl_note` treats as percussion. One drum per note, each with its own patch.
  I had it reading the channel's program, which is what every other channel
  does and what this one does not.

**Still differing from write 779**, and it is worth writing down how precisely,
because the shape rules most things out.

Extracting every key event from both traces - the writes to 0xB0 plus the
voice, with the key bit saying on or off - the two are **identical for 99
events**. The divergence is a note *off*:

    port  ... 2+ 0+ 4+ 8+ 2- 0- 8- 3+ 6+
    orig  ... 2+ 0+ 4+ 8+ 2- 8- 4- 3+ 6+

Both key *on* the same four voices in the same order, and both then release
two. So the sequence of voices allocated agrees; what differs is which note
each voice is holding. One note is on voice 0 in the port and voice 4 in the
original, and the release finds it accordingly.

Taking that next trace - (voice, pitch) pairs rather than voices alone -
answers it, and the answer is that **the driver is not at fault**:

      95  port v2 on fnum=81 blk=1d | orig v2 on fnum=81 blk=1d
      96  port v0 on fnum=02 blk=1e | orig v0 on fnum=63 blk=0a
      97  port v4 on fnum=63 blk=0a | orig v4 on fnum=02 blk=1e

Both sides put a note on voice 0 and then one on voice 4 - the same voices, in
the same order, out of the same rotation. **The notes are swapped.** So the
driver did the same thing with what it was handed and the two note-on *events*
arrived in a different order.

That is upstream of `ADL:` entirely, in the sequencer. And the sequencer is
where the port has least evidence: `sound_service` and `poll_sequences` are
dispatched, so the hybrid runs the port's copies of those, but
**`sequencer_tick` and `step_sequence` are not** - the hybrid runs the guest's
and the port runs its own transcription - and `verify.py` says `sequencer_tick`
has "only 1 call seen" and `step_sequence` was never called at all.

It only shows when two notes fall on the same tick, which is why ninety-six key
events pass first. It is also why the `GMD:` comparison did not catch it:
General Midi never plays on this build, so that diff was the driver's
initialisation and no notes at all.

The initial state was checked and is not the cause: rotation 0..8, every voice
free, no reservations, no allowances.

### What can and cannot adjudicate the AdLib path

Two controls, both worth having run before believing the diff:

- **The port agrees with itself.** Two runs, 21,799 identical register writes,
  differing only where the timeout cut one short.
- **The hybrid agrees with itself.** Two runs, 10,687 writes, identical.

So the difference at key event 96 is real and not the clock artefact that makes
frame-by-frame comparison meaningless elsewhere in this project.

**And `verify.py` cannot help with it.** Its machine has no OPL2 - 0x388 answers
a constant and the FM registers are explicitly ignored - and with device 2
selected the original does not draw a single frame: `snapshot.py` reports "only
reached flip 0". So every routine reached only on the AdLib path is
unverifiable by the tool that verifies everything else, and the trace-diff
against the hybrid is the only instrument there is.

What that leaves measured: `step_sequence` **verified over 3000 calls** and
`midi_note_event` over 502, both on the speaker path, so the sequencer agrees
with the original wherever it *can* be compared. The routines it does not cover
are the ones a multi-voice driver reaches - `start_on_free_voice`,
`alloc_voice_records`, `voice_playing` and their neighbours - because
`install_driver` keeps `describe_0`'s answer and `ADL:` says **nine voices**
where the speaker says one. That is the difference between the two paths, and
it is exactly where the port has no evidence.

### Chasing the mispaired note: what is excluded

Each of these was suspected and checked, and none of them is it. Written down
so the next attempt starts where this one stopped rather than here.

- **The `ADL:` driver itself.** Both sides pick the same voices in the same
  order from the same rotation and are handed different notes; the driver is
  downstream of the fault.
- **Clock noise.** The port agrees with itself over 21,799 register writes and
  the hybrid over 10,687, so the difference is real.
- **`step_sequence` and `midi_note_event`**, verified over 3,000 and 502 calls
  on the speaker path.
- **`sequencer_tick`'s first five calls**, compared and agreed.
- **`start_on_free_voice`'s loop bound** - `cmp si, 7` in the original, a
  constant, and the port's 7 matches. It is *not* driven by the driver's voice
  count.
- **`sequencer_tick`'s free-voice search.** It assigns `dh = bl` for every free
  slot in the `cs:0x1fa`..`cs:0x1fb` range without breaking, so it ends on the
  **last** one rather than the first - which looks like a transcription slip
  and is not: 0x27259 does exactly the same, `mov dh, bl` inside the loop with
  no exit.

Two more, checked since and also excluded:

- **The `cs:0x1fa`..`cs:0x1fb` range.** `adl_init` answers CX 0x0800 and
  `gmd_init` 0x0801, so AdLib's usable voices are 0..8 - nine, matching the
  chip - and General Midi's 1..8. Both match the original's `mov cl,0 / mov
  ch,8` and `mov cl,1 / mov ch,8`.
- **The stealing loop over `cs:0x148`.** It takes the **first** maximum, not
  the last, because 0x272d7 compares `al` with `jae` to skip - strictly less
  wins - and the original keeps its running maximum in `al` itself, saving and
  restoring it around the search. The port's separate variable does the same
  thing.

`bp_` was the last candidate and it is right as well: the original advances BP
by 0x10 at one place, 0x2753e, and the abandon path at 0x2753b - `mov al,
cs:[0x203]` - falls straight through into it, exactly as the port's
`abandon_sequence` falls into `next_sequence`.

Every candidate named above was read against the original and every one agrees,
so reading routines one at a time was not going to find it. Printing the
sequencer's own state did, immediately.

### `TIM_TRACE=seq`, and what it found in one run

`io.c` prints the sound module's four voice arrays - `cs:0x168` owner,
`cs:0x158` priority, `cs:0x148` ordering, `cs:0x138` pinned - and the playing
table at `cs:8`, at every key event. It lives in `io.c` because **both sides
run that file**: the hybrid executes the guest's sequencer and the port its own
transcription, and the table is at the same address in `guest_mem` either way.
No hook on the runner is needed.

The first difference is at **key event 14** - eighty-odd events before anything
is audible:

    port  tbl 7f6e:0000  4226:0000  0000:0000 ...
    orig  tbl 7f6e:0000  0000:0000  0000:0000 ...

**The port has a second sequence playing where the original has one.**
Everything after follows from it: an extra entry shifts every index, so `bp_` is
one step ahead when a channel is assigned, so the ordering value comes out
0x2d where the original has 0x1d - a difference of exactly 0x10, one sequence -
and the priorities are computed against the wrong sequence. The swapped note at
event 96 is the first time that reaches the chip.

That is the fault to fix: something puts `4226:0000` on the playing table at
`cs:8` that the original does not, or fails to take it off.

Tested since, from the entry point on the speaker path, every routine that
writes that table **agrees** where it is reached - `start_sequence_by_id` over
410 calls, `start_sequence` over 3, `remove_sequence` over 2,
`create_sequence` and `load_and_start_sequence` over 1 each. So the fault is
not in putting sequences on the table in general; it is specific to the path
`ADL:` opens.

**`drop_unless_polled` (0x27b52) is the candidate that fits.** Its whole job is
taking a sequence *off* the table when nothing is polling it, it is the one
routine in that group `verify.py` has never reached under any configuration -
"TRANSCRIBED, NEVER CALLED" on both the entry point and the snapshot - and an
extra entry on the table is exactly what failing to drop one looks like.

`insert_by_key` is in the same position, never reached, and would produce the
same symptom from the other direction.

Neither can be verified as things stand: `verify.py`'s machine has no OPL2 and
does not draw a frame with device 2 selected, so the only configuration that
reaches them is the one the reference cannot run. `TIM_TRACE=seq` is the
instrument that can - it prints the table either side - and narrowing from
"somewhere in the sequencer" to two named routines took one run of it.

**And the register trace could not have found it.** It shows what came out of
the driver, which stayed correct for ninety-six events while the state behind
it was already wrong. A state trace at the same instant is a different
instrument, not a finer one.