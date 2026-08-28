# The resource archive

`RESOURCE.MAP` indexes `RESOURCE.001` .. `RESOURCE.004`. The layout below is
described on the [ModdingWiki](https://moddingwiki.shikadi.net/wiki/TIM_Resource_Format);
**every field here was then checked against the bytes**, and the check is
recorded because an inherited claim and a verified one are different kinds of
knowledge.

## RESOURCE.MAP - verified

```
BYTE[4]     hash index array          (00 01 05 07 in this build)
UINT16LE    number of data files      (4)
  per data file:
    char[13]  data file name, NUL terminated ("RESOURCE.001")
    UINT16LE  number of entries
    per entry (8 bytes):
      INT32LE   name hash
      UINT32LE  byte offset into that data file
```

**How it was verified.** Each data file was walked linearly using the subfile
format below; that yields the true offset of every subfile. Those offsets were
then located in `RESOURCE.MAP`: they appear at 0x19, 0x21, 0x29, ... - stride
8, first entry at 0x15, which is exactly `6 + 13 + 2`. The entry count for
`RESOURCE.001` in the map is 62 and the linear walk finds 62 subfiles.

The "hash" is not opaque: its top two bytes are literal characters of the name
(`SIERRA.SCR` hashes to `0x5349874b`, and 0x53 0x49 are `S` and `I`), which is
what the four-byte index array at the head of the file selects. The exact
function has **not** been derived, because reading the archive does not need
it - only rewriting it would. Recorded as unfinished rather than guessed.

## Subfile format - verified

Inside a data file, subfiles are concatenated with no directory:

```
char[13]    subfile name, NUL terminated
UINT32LE    content length
BYTE[len]   content
```

The 13-byte name field is a **reused buffer**: the bytes after the terminator
are whatever was there before, so `L2.LEV` is stored as
`"L2.LEV\0" "E.004\0"` - the tail of `RESOURCE.004`. Harmless, but it makes a
raw dump look corrupted if you do not expect it.

**How it was verified.** All four files walk to exactly their own size with no
slack: 196,578 / 163,032 / 220,123 / 2,815 bytes, 62 / 85 / 10 / 2 subfiles,
159 in total. `RESOURCE.004` is the whole argument in miniature: 17 + 1564 =
1581, the offset of the second subfile, and 1581 + 17 + 1217 = 2815, the file
size.

## What is in there

| extension | count | what |
| --- | --- | --- |
| `.LEV` | 87 | levels (`L1`..`L88`, less one) |
| `.BMP` | 61 | artwork, including `PART0`..`PART57` - the machine parts |
| `.PAL` | 3 | `TIM.PAL`, `SIERRA.PAL`, `BLACK.PAL` |
| `.OVL` | 2 | `VM.OVL` (44,453 bytes) video driver, `SX.OVL` (39,715) sound |
| `.GKC` | 2 | `TITLE.GKC`, `CREDITS.GKC` |
| `.SCR` | 1 | `SIERRA.SCR`, the logo screen |
| `.FNT` | 1 | `MEMOFNT8.FNT` |
| `.SX` | 1 | `TIM.SX`, music/sound bank |
| `.TXT` | 1 | `PASSWORD.TXT` |

**`VM.OVL` and `SX.OVL` are code**, loaded and executed - the page flip is
performed from inside `VM.OVL`. They are part of what has to be reconstructed,
not data.

The game tries each resource as a **loose file first** and falls back to the
archive, so a long `files missing:` list in a run report is the normal path and
not an error.

## Shipped documentation

`READ.ME` is a primary source on the controls: 1-9 and a-g change the music in
Freeform mode, ALT-V shows the version, TAB moves between hotspots, arrows move
the cursor, Space and Enter are the left button, Esc opens the control panel,
X and Y flip a shape, and + and - size it.

`CODES.TXT` is the copy-protection wheel: twenty pages, three part names each.
The game asks for it at the string "Please select, in order, the three parts
listed on page " (DGROUP, near image 0x2f0a0).
