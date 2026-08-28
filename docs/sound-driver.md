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
is a property of the run, not of the game: the same interface presumably has
other drivers behind it for other hardware, and none of them has been looked
at.

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
