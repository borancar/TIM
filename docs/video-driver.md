# The video driver, `VM.OVL`

The game draws **nothing** itself. Attributing the A000 writes of nine frames
of the title screen to the instructions that made them found **19 instructions,
every one of them inside `VM.OVL`**. So the driver is the renderer, and
reconstructing the screens means reconstructing it.

## The container

`VM.OVL` is 44,453 bytes in `RESOURCE.003` and holds one driver per adapter:

| chunk | bytes on disk |
| --- | --- |
| `VGA:` | 6,853 |
| `EGA:` | 6,495 |
| `MCG:` | 2,722 |
| `CGA:` | 5,466 |
| `TAN:` | 5,034 |
| `HEG:` | 7,281 |
| `EVG:` | 5,490 |
| `EVA:` | 5,040 |

Chunk header: a four-byte tag, a three-byte length, one flag byte. The eight
chunks plus their headers account for the file exactly.

**Only `VGA:` is being reconstructed.** The other seven are deliberate
non-goals.

Each chunk's payload is compressed - the `VGA:` one expands from 6,853 bytes to
about 10 KB, into a DOS block of 0x2b1 paragraphs (11,024 bytes). The
decompressor is **not** reversed: `tools/dump_overlay.py` reads the driver out
of memory once the game has loaded it, on the same principle as the LZEXE
recovery. Disassemble the dump with
`tools/disasm.py --file out/res/VM_VGA.mem`.

Addresses in this document and in `reconstruct/vmovl_vga.c` are **offsets
within the loaded VGA driver**, written `VM.OVL VGA:0xNNNN`. The loader chooses
the segment - 0x424b in the runs so far - so there is no fixed image address.

## How the game calls it

Not directly. A search for direct far calls from the image into the overlay
finds **exactly none**. Each module carries a thunk:

```
2149a  ff2e6643      ljmp [0x4366]        ; DGROUP 0x4366
```

and DGROUP holds a **far function pointer** that the loader fills in. Because
the thunk *jumps* rather than calls, the return address the driver sees is the
game's original caller, which is what made the call sites findable at all.

The vector table is at **DGROUP 0x4362 + 4n**, about 31 entries, resolved by
`tools/driverapi.py` from the running machine. That base is a **lower bound and
not the start** - entries were measured below it, so `n` goes negative:

| DGROUP | holds | thunk | what it is |
|---|---|---|---|
| 0x435a | 424b:12fb | image 0x21ab5 | not yet established |
| 0x435e | 424b:138e | image 0x21ab9 | `vm_buffer_size` |
| 0x4362 | 424b:13b9 | image 0x2247f | not yet established |
| 0x4366 | 424b:150f | - | `vm_show_page`, already transcribed |

The 0x4366 row is the cross-check: it resolves to VGA:0x150f, which was
identified independently, so the measurement is reading the table correctly. Many entries point at a common
stub at `VGA:0252`. The distinct entry points seen so far:

```
0252 (stub)  027a  034f  03db  04f1  07db  0938  0be6  0efe  0f15  0f57
0fd4  1015  1231  12fb  138e  13b9  1453  14c9  150f  1561  15d0  1707
25e7  267f  2714  271b  2ae6  2ae7
```

Which does what is mostly **not yet established**; the ones below are.

`138e` is the **buffer size** query: given a width and a height it answers, in
DX:AX, how many bytes a planar image of that size needs. A row is `w >> 3`
bytes plus one, plus another if the width is not a whole number of bytes, then
rounded up to even; that count times the height, shifted left twice for the
four planes. The unconditional extra byte is what lets a blit shift the source
across a byte boundary when x is not a multiple of 8.

It is reached from image `0x21ab9`, an `ljmp` through **DGROUP 0x435e** -
measured holding `424b:138e`, with `0x424b` the segment the loader chose in
these runs. Transcribed as `vm_buffer_size`.

## Driver data - which is part of DGROUP

The driver loads its own data segment from `cs:[0x13a]`, and that segment
**lies inside the game's DGROUP**, at byte offset 0x3890. The driver keeps that
distance in `cs:[0x13c]` precisely so the two views can be interchanged.

The proof is in the game's own start-up, which writes the driver's page
segments through DGROUP without going near the driver:

```
0e183  c706a43800a0    mov word ptr [0x38a4], 0xa000
0e189  c706a23820a8    mov word ptr [0x38a2], 0xa820
```

`0x38a4` is `VMDS + 0x14` and `0x38a2` is `VMDS + 0x12` - the front and back
pages. The clip box and colours the game's rectangle routine reads at DGROUP
0x3893..0x389e are the same block seen from the other side. There is **one**
shared structure, not two, and the port models it that way.

| offset (driver-relative; add 0x3890 for DGROUP) | what |
| --- | --- |
| 0x0d | fill colour |
| 0x12 | page being drawn into, as a segment |
| 0x14 | page on screen, as a segment |
| 0x16 | source page for a copy |
| 0x18 | destination page for a copy |
| 0x6ec | the mode's height, 480 |
| 0x6f2 | row table: the byte offset of each scan line, indexed by y. `[y] == y*80`, measured |

The driver's code segment also holds `cs:[0x13a]`, its own data segment, and
`cs:[0x13c]`, the **byte distance from the game's DGROUP to that data segment**
(0x3890). The second exists because `mov di, [bp+di]` addresses through SS, and
SS is the game's DGROUP: adding 0x3890 to a driver-relative offset turns it
into a DGROUP-relative one, so the same row table is reachable either way.

The pages are segments, `0xA000` and `0xA820` - 0x8200 bytes apart, which is
the start address the CRTC is given.

The driver is **self-modifying**: `VGA:0x15d0` writes an immediate into
`cs:[0x15ce]`, inside a blob of data sitting between two routines.

## Which vectors actually run

Measured by executing the title screen and hooking every vector target, rather
than inferred. 13 of the ~31 vectors are entered at all:

| entry | calls in 90M instructions | what |
| --- | --- | --- |
| `VGA:0938` | 117,575 | `vm_blit_run`, the main blitter |
| `VGA:034f` | 67,970 | `vm_span`, a horizontal run of one colour |
| `VGA:0be6` | 1,078 | `vm_fill_spans`, a span-list fill |
| `VGA:150f` | 246 | `vm_show_page` |
| `VGA:1707` | 63 | a structured blit - not yet transcribed |
| `VGA:1561` | 32 | `vm_copy_rect` |
| `VGA:15d0` | 8 | not yet identified; self-modifying |
| `VGA:0f15` | 4 | not yet identified. `VGA:0ec1`, the palette loader, sits just before it and is transcribed |
| `VGA:138e` | 2 | not yet identified |
| `VGA:12fb`, `VGA:13b9`, `VGA:0fd4`, `VGA:1015` | 1 each | set-up, probably |

**`VGA:07db` is in the vector table and is never entered.** An earlier guess
put the main blitter there, from scanning for the `push bp / mov bp,sp`
prologue - but this driver is mostly hand-written assembly and that heuristic
finds the wrong boundaries. The vector table is the only reliable source of
entry points, and execution is the only reliable source of which ones matter.

## Routines transcribed and proven

- **`VGA:0x150f` `vm_show_page`** - swap the pages and show the one just drawn.
  Writes only the *high* byte of the start address, to CRTC index 0x0C, which
  is why index 0x0D is never written and why a page offset is always a multiple
  of 256. Optionally waits out a whole vertical retrace edge.
- **`VGA:0x1561` `vm_copy_rect`** - copy a rectangle from one page to the other
  at the same position, in **write mode 1**, the latch copy: one `movsb` moves
  eight pixels across four planes and the byte value itself is never looked at.
  The rectangle is widened to byte boundaries first. Sets the graphics
  controller's mode to 1 on the way in and back to 2 on the way out, so 2 is
  the driver's resting mode. Proven with the video memory itself compared:
  1804 hardware events identical and 0 of 262,144 plane bytes differing.
- **`VGA:0x034f` `vm_span`** - fill a run of pixels on one scan line with a
  colour. **Register arguments**: AL colour, BX x, CX count, ES:DI the row - it
  is reached through the vector table but it is not a C function. Write mode 2
  with the bit mask, so one byte written carries the colour into all four
  planes and the mask picks the pixels; every write is preceded by a read whose
  value is discarded but whose *latches* are what preserve the pixels the mask
  excludes. The edge masks are tables at `VGA:0x254` and `VGA:0x25c`.

  A colour with any high nibble bit set branches to `VGA:0x27a`, which is not
  transcribed. That branch is **never taken** in either intro screen: a scan of
  the arguments of all 67,970 calls found none. The port aborts there rather
  than guessing.
- **`VGA:0x0938` `vm_blit_run`** - the main blitter, and about 44% of every
  pixel the game writes. Draws a run of pixels along one scan line from a
  byte-per-pixel source.

  **One of its arguments is a flag.** The routine's first instruction is
  `jb 0x965`: carry clear walks the destination left to right, carry set walks
  it right to left while the source still advances forwards, which is a
  horizontal flip. Both occur - 67,312 forward and 5,886 backward while the
  title screen runs - and both are proven. The direction flag is always clear,
  so `lodsb` always advances.

  One pixel per iteration in write mode 2, with a single-bit mask rotated along
  the row: `ror ah,1 / adc di,0` steps to the next byte exactly when the bit
  wraps, with no compare and no branch. The graphics controller's *index* is
  written once outside the loop and only the data port per pixel, which is why
  a trace of it is one `0x3ce` write and N `0x3cf` writes.

  Its source is a **scratch buffer in DGROUP**, not artwork in a file. Following
  a blit's source address lands on anonymous memory every time; the artwork is
  one step further back, in whatever writes into that buffer.
- **`VGA:0x0be6` `vm_fill_spans`** - fill a list of horizontal spans with one
  colour: about 24% of every pixel written, from only 1,078 calls, so this is
  what paints large areas.

  The span list is a stream - a first row, a row count, then one `x1, x2` pair
  per row - and a pair whose `x2` is below its `x1` leaves that row alone, which
  is how a concave edge is described.

  Whole bytes go through `rep stosb` with the bit mask at 0xff and **no read**,
  because with every bit writable the latches cannot contribute; the partial
  bytes at each end do read first, to load them. That asymmetry is the
  original's, and `VGA:0x034f` does *not* share it - it reads inside its
  whole-byte loop too, redundantly. Both are transcribed as written.

  Its patterned path at `VGA:0x0cd9` is not transcribed. It is never taken:
  all 1,078 calls on the intro screens pass a colour whose high nibble is
  zero.
- **`VGA:0x0ec1` `vm_set_palette`** - load colours into the DAC, three six-bit
  bytes each. It **waits for vertical retrace first**, which is what stops the
  palette changing mid-frame, and disables interrupts across the transfer so a
  handler cannot land in the middle of a colour. `loop` counts bytes, not
  colours: the count is tripled on the way in.

  It also reads the **DAC state register** (0x3C7) and, if the low two bits are
  not 3, writes one byte to nudge the DAC out of a half-finished triple. This
  found a gap in the reference: the shared emulator answered 0 for that port
  unconditionally, so the game always wrote a resynchronising byte real
  hardware would never have asked for. Harmless - the write to the index port
  that follows discards it - but a divergence that happens not to matter is
  still a divergence. Both the emulator and the port now model the register:
  3 while the write index is the live one, 0 after the read index.
