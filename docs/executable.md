# The executable

This is 1.00's executable, `reconstruct/v1.00`: everything here is argued back
to bytes in its `TIM.EXE` (`reconstruct/v1.00/game/`) or in the recovered image
`out/v1.00/TIM.img`. What 1.11 changed is `docs/v1.11.md`. Where something is inherited from a
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

**Turbo C++ 3.0, medium model** - measured 2026-09-26 against the installs in
the sibling `turboc` checkout (`dos-c/`), and against the compilers themselves.

- **The startup is TC++ 3.0's `C0M.OBJ`**, byte for byte: all 535 of its fixed
  bytes are at image 0 but two, and those two are TLINK's own - a far call into
  the caller's segment rewritten as `nop / push cs / call near`. `C0L.OBJ`
  differs from byte 0x26 on: `mov cx,1 / add bx,8` is C0.ASM's near-data arm,
  and the stack is sized against DS from `_stklen` and `_heaplen`. So this is
  **medium model** - far code, near data - and not large, which this file said
  until then (far returns outnumbering near ones is true of both).
- **The runtime is TC++ 3.0's `CM.LIB`**: 50 of its modules, 4,967 bytes, found
  at 0x0bbfe..0x0dfb4 by turboc's `tools/libmatch.py`. Borland C++ 3.0 ships
  the same `C0M.OBJ` and the same fifty modules, so the runtime cannot tell the
  two apart; the banner `Borland C++ - Copyright 1991 Borland Intl.` at 0x2d3c4
  is in both. BC++ 2.0's differ.
- **The game's own modules are Borland C++ 3.0's** - `BCC`, not Turbo C++'s
  `TCC`: medium model, 8086 code, cdecl. BC++ 3.0 is the only one of the
  compilers tried (TC++ 1.01 and 3.0, BC++ 2.0 and 3.0) that turns an early
  `return` into a copy of the epilogue, which `open_bit_reader` (0x248fe)
  has, and every routine matched with TC++ 3.0 matches with it too. The
  options are **per module**: segment 0's first routines are `-mm -O`, and
  segment 248f's C files are `-mm -O -G -Z` - `-G` for `add sp,2` after a
  call, `-Z` for a register kept across statements. `tools/judge.py`
  compiles a port source and compares every routine and its module's data.
- **Segment 248f is four modules**, three in C and one in assembly - see
  `reconstruct/v1.00/src/vqtflip.c`'s header for how a segment's module boundaries
  are read off far calls: Borland C++ calls a routine defined earlier in the
  same file with `push cs / call`, and TLINK leaves `nop / push cs / call`
  for every other far call into the segment. The one with inline `asm`
  (`vqt_flip_leaf`) went through TASM (`bcc -B`); 2.51 reproduces it.
- **Not every module is.** Sixteen routines reserve a two-byte frame with
  `dec sp / dec sp` where the rest have `sub sp,2`, and they cluster: most are
  in segment 1c25, one is in segment 0000, and `atan2_long` is segment 2d29's
  only routine. TCC 3.0, BC++ 3.0 (with or without its optimiser switches) and
  Turbo C 2.01 all write `sub sp,2`; BC++ 2.0 writes `dec sp` but stores a
  negated long high word first where the image stores the low word first.
  **Turbo C++ 1.0x reproduces `atan2_long` byte for byte** (`-mm`, no `-O`),
  far calls included. The likely reading is a library built earlier and linked
  into the game - Dynamix's engine - so the compiler is a property of a
  module, and a file says which with `JUDGE: compiler <version>`.

## Layout

- **Code** starts at image 0, since the entry point is `0000:0000`.
- **DGROUP** is at image **0x2d3c0**. Measured: the Borland startup at
  `0000:0016` loads DS with the segment whose image offset is that, and the
  compiler banner then sits at DGROUP+4, exactly where Borland puts it.
- The **stack** ends up at the top of a full 64 KB DGROUP. The startup at image
  `0x00b4` calls INT 21h AH=4Ah with BX=0x3d4c, sizing the program's block to
  end at DGROUP + 0x1000 paragraphs.

**Each code segment is not one translation unit.** In the medium model a
module's code goes into a segment of its own name unless it was compiled into
`_TEXT`, and segment 0000 is `_TEXT`: C0M's code at the front, the library's at
the back (every `CM.LIB` module is `_TEXT`), and the game units that were
built into it in between. So a segment boundary is a module boundary, but a
segment can hold several modules, and those boundaries have to be found. The
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


## The object record, as it has been read so far

Several routines have been transcribed independently and they agree about the
same structure, which is worth more than any one of them read carefully. Fields
are byte offsets from the start of a record:

| offset | what | how it is known |
| --- | --- | --- |
| +0x04 | kind | indexes the 0x3a-byte table at DGROUP 0xea6 |
| +0x0a | flags | `link_record_into_buckets` sets bit 5 |
| +0x1e, +0x20 | position, x and y | `compute_other_bounds` reads them as the corner; `update_velocity` differences them |
| +0x22, +0x24 | previous position | `update_velocity` subtracts them; `compute_swept_bounds` stretches the box back to them |
| +0x36, +0x38 | velocity, x and y | `update_velocity` writes them, `clamp_record_pair` clamps them |
| +0x44, +0x46 | width and height | added to the corner to give the far edges |
| +0x3a | speed scale | multiplied by the Manhattan sum of the velocities |
| +0x3c, +0x3e | speed, a long | the product, low half first |
| +0x2e, +0x32 | two shape anchor points | `add_record_shapes` passes them to `alloc_shape` |
| +0x48, +0x4c | their second points | the same, as the second argument |
| +0x74, +0x76 | bucket links | `link_record_into_buckets` threads them |
| +0x78 | chain link | `chain_contains` walks it |
| +0x7f | bucket number | written for the first bucket only |

The kind table at DGROUP 0xea6 has 0x3a-byte entries. Fields known so far, as
offsets within an entry:

| offset | what | how it is known |
| --- | --- | --- |
| +0x02 | a sort key | `insert_sorted` orders the 0x5179 list on it |
| +0x08 | gravity | `apply_gravity_and_speed` adds it to the vertical velocity each step |
| +0x0a | velocity limit | `clamp_record_pair` clamps to plus or minus it |
| +0x1c, +0x1d | the two bucket numbers | `link_record_into_buckets` reads them; 0xff means "not in this bucket" |
| +0x20 | a second sort key | `insert_sorted` orders the 0x50d7 list on it |

Records are also threaded on **doubly-linked** lists through +0 and +2, kept
sorted by those keys. The heads at DGROUP 0x50d7, 0x5179 and 0x521b are the
same three `pick_by_flag` chooses between.

Two working sets sit beside each other in DGROUP: 0x53fe with its box at
0x5404..0x540e, and 0x5400 with a **swept** box at 0x5410..0x5420 - the union
of where an object is and where it was, which is what a dirty-rectangle redraw
repaints.
