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

## What is not established

Which BP number means what. The names below are only what the code does, and
the mapping to anything a player would recognise - a note, a voice, a volume -
has not been measured.
