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
`tools/driverapi.py` from the running machine. Many entries point at a common
stub at `VGA:0252`. The distinct entry points seen so far:

```
0252 (stub)  027a  034f  03db  04f1  07db  0938  0be6  0efe  0f15  0f57
0fd4  1015  1231  12fb  138e  13b9  1453  14c9  150f  1561  15d0  1707
25e7  267f  2714  271b  2ae6  2ae7
```

Which does what is mostly **not yet established**; the ones below are.

## Driver data

The driver has its own data segment, loaded from `cs:[0x13a]`. It is **not**
DGROUP.

| offset | what |
| --- | --- |
| 0x12 | page being drawn into, as a segment |
| 0x14 | page on screen, as a segment |
| 0x16 | source page for a copy |
| 0x18 | destination page for a copy |
| 0x6ec | the mode's height, 480 |
| 0x6f2 | row table: the byte offset of each scan line, indexed by y |

The pages are segments, `0xA000` and `0xA820` - 0x8200 bytes apart, which is
the start address the CRTC is given.

The driver is **self-modifying**: `VGA:0x15d0` writes an immediate into
`cs:[0x15ce]`, inside a blob of data sitting between two routines.

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
