# The executable

Everything here is argued back to bytes in `incredible-machine/TIM.EXE` or in
the recovered image `out/TIM.img`. Where something is inherited from a
third-party source rather than checked here, it says so.

## Recovery

`TIM.EXE` is 111,951 bytes and packed with **LZEXE 0.91** - the tag `LZ91` at
offset 0x1C of the header, a 32-byte header, zero relocations, entry
`1aa0:000e`.

`tools/unlzexe.py` recovers it by **running the stub** under the emulator, not
by reimplementing LZEXE. The stub relocates itself upward, decompresses, and
hands control back down; the stop rule is the first instruction to execute
below the stub's own entry, which happens after **1,528,688 instructions** and
lands on `0000:0000`.

The **relocation table is measured, not decoded**: the stub is run twice, at
load segments 0x0110 and 0x0510, and the two images compared. A word differing
by exactly the segment delta is a relocation. Every other byte is identical -
that is checked, and a failure would mean the recovery is not deterministic.

| | |
| --- | --- |
| image | 214,512 bytes (0x345f paragraphs) |
| entry | `0000:0000` |
| stack | `3389:0080` |
| relocations | 2,327 |

**Proven, not assumed.** `tools/verify_unpack.py` loads the emitted EXE and
compares it against what the stub itself produced: all 214,512 bytes identical,
and CS, IP, SS and SP all equal. An unpack that is subtly wrong looks like a
hundred transcription bugs later, so this is a gate, not a formality.

## What built it

**Borland C++, 1991** - the banner `Borland C++ - Copyright 1991 Borland Intl.`
sits at image 0x2d3c4, and `Null pointer assignment` at 0x2d3ef is the same
runtime. So this is **compiled C**, not hand-written assembly, which means a
byte-exact matching decompilation is on the table in principle.

Evidence for the memory model:

| measurement | count |
| --- | --- |
| `push bp; mov bp,sp` prologues | 1,022 |
| far returns (`retf`) | 1,202 |
| near returns (`ret`) | 433 |
| far calls (`lcall`) | 1,814 |
| near calls (`call`) | 2,188 |

Far returns outnumbering near ones by three to one is **large model**: far code
and far data, one code segment per translation unit. Byte counts of this kind
are approximate - `0xE8` occurs in data too - but the ratio is not in doubt,
and the disassembly confirms it: routines end `mov sp,bp; pop bp; retf` and are
called with `lcall seg, off`.

## Layout

- **Code** starts at image 0, since the entry point is `0000:0000`.
- **DGROUP** is at image **0x2d3c0**. Measured: the Borland startup at
  `0000:0016` loads DS with the segment whose image offset is that, and the
  compiler banner then sits at DGROUP+4, exactly where Borland puts it.
- The **stack** ends up at the top of a full 64 KB DGROUP. The startup at image
  `0x00b4` calls INT 21h AH=4Ah with BX=0x3d4c, sizing the program's block to
  end at DGROUP + 0x1000 paragraphs.

Because this is large model, **each translation unit is its own code segment**,
so the module boundaries are readable off the binary rather than guessed. The
port's `.c` files mirror them; see `STATUS.md` for the map as it is measured.

## Video

The game runs **640x400, 16 colours, planar**, double-buffered - not the 640x480
the BIOS mode implies.

- It sets BIOS mode **0x12** (640x480 16-colour planar).
- It then calls the routine at image **0x8f77**, which takes a scan-line count
  in its one argument and spreads it over three CRTC registers: Start Vertical
  Blank (0x15) low eight bits, bit 8 into Overflow (0x07) bit 3, bit 9 into
  Maximum Scan Line (0x09) bit 5. It is called with **0x1d6 (470)** for the
  Sierra logo and **0x18f (399)** for the game's own screens.
- It never touches Vertical Display End. So the CRTC still scans 480 lines and
  simply **blanks** everything from the blanking line down.
- It page-flips by writing only the **high byte** of the start address, CRTC
  0x0C, alternating `0x00` and `0x82` - a 16-bit `out dx, ax` to 0x3D4 from the
  video driver overlay. Two 640x400 pages fit in a 64 KB plane (32,000 bytes
  each, page 1 at 33,280) precisely because the tail is blanked.

A renderer that ignores blanking wraps page 1 around the plane and paints the
top of the other page across the bottom eighty rows, which looks exactly like a
blitter bug in the game and is entirely an artefact of the reference.

## Adapter detection

The routine at image **0x225d2** picks the video driver. It asks INT 10h
AH=1Ah for the display combination code and accepts BL or BH of 7, 8, 0x0b or
0x0c; failing that it asks AH=12h BL=10h for EGA information and reads the
EGA info byte at 0040:0087. `vm_init` at **1c25:6233** (image 0x22483) then
loads the driver overlay, and image `0x0e170` prints
`Unable to initialize vm.` and exits if it comes back null.
