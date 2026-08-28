# Status

*Last updated 2026-08-28.*

Reconstruction of **The Incredible Machine** (Dynamix / Sierra, 1993) from
`incredible-machine/TIM.EXE`.

**Which world this is in:** the original is **compiled C** (Borland C++ 1991,
large model), not hand-written assembly. A byte-exact *matching* decompilation
is therefore possible in principle. It is **not** what is being attempted yet;
the present standard is behavioural equivalence proved by differential
verification, and any move to matching would be a deliberate change of goal.

**Which variant:** `TIM.EXE` is the only executable, but its video driver
`VM.OVL` is a container of **eight per-adapter drivers** - `VGA`, `EGA`, `MCG`,
`CGA`, `TAN`, `HEG`, `EVG`, `EVA`. Only the **VGA** driver is being
reconstructed. The other seven are deliberate non-goals, listed here rather
than left looking unfinished.

## Done - and what was actually checked

- **The executable is recovered and the recovery is proven.** Not "it unpacked
  and looked plausible": `tools/verify_unpack.py` loads the emitted EXE and
  compares it against what the LZEXE stub itself produced in memory - all
  **214,512 bytes identical**, and CS, IP, SS and SP equal. The relocation
  table was *measured*, by running the stub at two load segments and diffing,
  and the run reports zero bytes that differ for any other reason.
- **The resource archive format is verified against the bytes**, not inherited.
  All four data files walk to exactly their own size, 159 subfiles, and the
  offsets found by walking are the offsets `RESOURCE.MAP` lists. See
  `docs/resources.md`, which marks separately the one thing taken on trust and
  not checked: the name-hash function, which is not needed to read the archive.
- **The game runs under the emulator** through the Sierra logo, the title
  screen and the credits screen, in 640x400 16-colour planar, page-flipped.
- **Captures are reproducible.** They are taken on the guest's own cue - the
  page flip that makes a composed frame visible - and the machine runs on a
  virtual clock driven by emulated instructions, so a given capture is the same
  capture on another machine and after a rebuild.

## Open

### Coverage - as last measured, 2026-08-28

| | |
| --- | --- |
| call targets found by recursive descent | **577** |
| reached by the title screen, flips 6..40 | **218** |

The transcribed and verified counts are **not** written here by hand - they
were wrong within one session when they were. They come from the sweep below,
which `tools/verify.py --all` regenerates in place. It stops the emulator at
each routine's entry, lets the **original body** run to its return, and
compares what each did to the hardware:

<!-- VERIFY:BEGIN -->
| routine | address | occurrences checked | result |
| --- | --- | --- | --- |
| `vm_set_display_lines` | 0x08f77 | 0, 1 | agreed |
| `vm_show_page` | VM.OVL VGA:0x150f | 0, 3, 9 | agreed |
| `vm_copy_rect` | VM.OVL VGA:0x1561 | 0, 2, 5 | agreed |
| `vm_span` | VM.OVL VGA:0x034f | 0, 4, 9, 17, 40, 73 | agreed |
| `vm_blit_run` | VM.OVL VGA:0x0938 | 0, 2, 19, 3359, 3360 | agreed |
| `vm_fill_spans` | VM.OVL VGA:0x0be6 | 0, 1, 40, 300 | agreed |
| `vm_set_palette` | VM.OVL VGA:0x0ec1 | 0, 1, 3 | agreed |
| `present_frame` | 0x081cc | 0, 5, 20 | agreed |
| `fill_rect` | 0x20079 | 0, 3, 60, 900 | agreed |
| `step_word_4e87` | 0x0144e | 0, 5, 60 | agreed |
| `set_clip_full_screen` | 0x0834b | 0 | agreed |
| `sub_002be` | 0x002be | 0, 3, 12 | agreed |
| `clear_word_array_50bf` | 0x166d6 | 0, 1 | agreed |
| `bit0_of_468c` | 0x2147d | 0, 4, 25 | agreed |
| `advance_record` | 0x2891a | 0, 2 | agreed |
| `match_field_5a_5c` | 0x06f43 | 0, 3, 20 | agreed |
| `lookup_table_546c` | 0x11d44 | 0, 5, 30 | agreed |
| `string_contains_r` | 0x1c6e3 | 0, 2 | agreed |
| `flag_bit_48ea` | 0x2213e | 0, 4, 30 | agreed |
| `select_field_2_or_4` | 0x06f68 | 0, 3, 20 | agreed |
| `find_free_slot_4bc4` | 0x0d0a3 | 0, 2, 10 | agreed |
| `read_pair_4740` | 0x220e9 | 0, 2, 15 | agreed |
| `angle_sin` | 0x2a456 | 0, 4, 25 | agreed |
| `angle_cos` | 0x2a47b | 0, 4, 25 | agreed |
| `angle_to_quadrant` | 0x004d1 | 0, 5, 40 | agreed |
| `chain_contains` | 0x03a61 | 0, 3, 25 | agreed |
| `normalise_far_ptr` | 0x22161 | 0, 4, 30 | agreed |
| `follow_far_chain` | 0x2907b | 0, 1 | agreed |
| `step_pair_apart` | 0x03d2e | 0, 3, 20 | agreed |
| `points_within_140` | 0x04b53 | 0, 3, 20 | agreed |
| `splice_list_4e58_onto_4e56` | 0x07b3e | 0, 2, 10 | agreed |
| `scale_byte_pair` | 0x282cb | 0, 1 | agreed |
| `value_between` | 0x03d67 | 0, 3, 20 | agreed |
| `pick_by_flag` | 0x05b65 | 0, 3, 20 | agreed |
| `normalise_far_ptr_far` | 0x22386 | 0, 3, 20 | agreed |
| `frame_pending` | 0x0b4e2 | 0, 1 | agreed |

*36 transcribed, 36 verified. Written by `tools/verify.py --all`, not by hand - one run of the original captures every call.*
<!-- VERIFY:END -->

Each routine is checked at **more than one occurrence**, because a check at one
value of a routine's inputs says nothing about the others: `vm_show_page`'s
first call has both page segments equal, so the swap is a no-op and the start
address is zero - it agrees there whatever it does with the pages. Occurrences
3 and 9 have them differing.

`frame_pending` has no hardware effect at all, so a trace comparison would have
found "0 writes on both sides" and called it agreement. It is checked on its
return value instead, at both values its one input takes, with the DGROUP word
seeded from the original's own memory at the moment of the call.

Two contaminations had to be removed before any of this meant anything: an
interrupt firing *inside* a routine wrote its own end-of-interrupt to port 0x20
and appeared as three events of the routine's, and the original's single 16-bit
`out dx, ax` had to be recorded as the two 8-bit writes the port performs.

The 577 come from direct calls only. Indirect calls through handler tables are
**not** followed yet, so the true figure is higher - finding those tables is the
next high-leverage task, not an afterthought.

**The renderer is the driver, not the game.** Every pixel the title screen
draws is written by `VM.OVL` - see `docs/video-driver.md`. The game reaches it
through a vector table in DGROUP filled in by the loader, so those entry points
are invisible to a static map and are resolved from the running machine by
`tools/driverapi.py`.

The 218 is measured by `tools/reached.py`, which records the basic blocks
executed between two page flips and intersects them with the code map. The
title screen also reaches **263 distinct blocks inside `VM.OVL`**, which are not
in the 577 at all because the overlay is loaded at run time and the static map
does not see it.

### The original's translation units

Large model, so each module is its own code segment and the boundaries are read
off the binary rather than guessed. Eight of them:

| segment | image range | routines |
| --- | --- | --- |
| `0000` | 00000..0dff0 | 270 |
| `0dff` | 0dff0..14de0 | 69 |
| `14de` | 14de0..1c250 | 32 |
| `1c25` | 1c250..248f0 | 112 |
| `248f` | 248f0..26190 | 20 |
| `2619` | 26190..2a040 | 69 |
| `2a04` | 2a040..2d290 | 4 |
| `2d29` | 2d290..2d3c0 | 1 |

Which of these are the C runtime rather than the game is **not yet
established**. Segment `0000` contains the Borland startup (the entry point is
`0000:0000`) and also game code - the CRTC routine at 0x8f77 - so it is not a
clean split, and the question is open.

### Emulator gaps closed, and where they belong

All four were found by this game and all four are generic; they live in
`TimMachine` in `tools/tim.py` and **have not been pushed upstream yet**.

1. **INT 10h AH=1Ah and AH=12h** were unimplemented.
2. **Program memory ownership.** An EXE with `maxalloc = 0xFFFF` owns all of
   conventional memory until its runtime shrinks it with AH=4Ah.
3. **Vertical blanking was ignored** by the planar renderer.
4. **CRTC registers read back as 0.** The register file now starts from the
   BIOS's own mode-12h table. This one is the cautionary example: the game
   reads Overflow and Maximum Scan Line back before setting one bit in each, so
   with reads returning 0 it silently cleared every other timing bit - and it
   *happened to reach the same blanking line anyway*, so nothing looked wrong.

### Transcribed, stubbed, and the difference

`reconstruct/tests/provenance.py` now counts three things, not two, because
"we know where this routine is" and "we have read it" are different claims:

- **transcribed** - the body was read from the disassembly;
- **stub** - the address is known and the body is not written yet. A stub
  **aborts** when reached rather than returning quietly, because a silent
  no-op in a drawing path is a missing frame that looks like a blitter fault;
- **ours** - the port's own, said so explicitly.

Two stubs exist, both reached from `present_frame` at 0x081cc: `0x0b078` and
`0x0e34a`. Neither is reachable on the intro screens - **all 436 calls to
`present_frame` while they run have both DGROUP flags at zero**, which is
measured rather than argued.

### What is checked, and what a check covers

Each routine is verified at several occurrences chosen to reach **different
paths**, not merely several times. `vm_span`'s six are one byte-aligned
multi-byte run, two unaligned ones, one ending exactly on a byte boundary, and
two that fit inside a single byte - the last found by scanning the arguments of
all 67,970 calls, because the first four never reached that branch and four
checks of one path are one check.

Where a routine can still go somewhere untranscribed, the port **aborts**
rather than guessing, and the fact that the branch is unreachable in the states
being compared is measured rather than assumed.

### A second model corrected

The port had an array of its own for the span lists the rectangle routine
builds. It passed every check until the comparison widened from DGROUP to all
of conventional memory - and then `fill_rect` and `vm_fill_spans` both failed
at once, because the original writes that list into a **block DOS gave it**,
named by a segment in DGROUP 0x4342, which the port never touched.

The port now models the guest's whole address space as a flat megabyte, with
DGROUP as a window into it and far pointers formed the way the hardware forms
them. That is also what let `lookup_table_546c` be transcribed at all: it
follows `les bx, [0x546c]` into an allocation outside DGROUP.

### A model corrected

The video driver's data is **not** a separate segment: it lives inside DGROUP
at offset 0x3890, and the game writes the driver's page variables directly
through DGROUP. The port had them as separate C globals, which was wrong, and
the whole-segment comparison is what caught it - `vm_show_page` and
`present_frame` failed the moment the check became strong enough to notice.
`docs/video-driver.md` has the evidence.

### Retractions and near-misses

- **2026-08-28. A routine was named wrongly and is corrected.** 0x22161 was
  transcribed as `fixed_normalise`, on the reading that AX held a fixed-point
  fraction and DX its whole part. The six instructions are the same either way,
  so it verified, and the wrong name stood. 0x222c6 settled it: that routine
  calls 0x22161 on the offset and segment halves of *two far pointers* and then
  copies between them, which only makes sense for **far pointer
  normalisation**. Renamed to `normalise_far_ptr`, and the correction is
  recorded in the source rather than quietly applied - a wrong name outlives a
  wrong line.

- **2026-08-28. Occurrence numbers are not stable across runs.** The batched
  sweep suppresses timer and keyboard interrupts while any tracked routine is
  open, so that an interrupt's own hardware writes are not counted as the
  routine's. That gating perturbs how far the guest gets in a given number of
  instructions, so the *N*th call to a routine in the sweep is not necessarily
  the *N*th call in an ungated run: a probe saw eight calls to `advance_record`
  where the sweep saw fewer. An earlier commit message claimed the numbering
  "means the same thing" across runs. It does not. Low occurrence numbers are
  reliable; the last call is not, and the sweep now prints how many calls it
  actually saw so the choice is grounded rather than guessed.

- **2026-08-28.** Renaming two driver variables in the C left the old names in
  the verifier's spec, and `vm_copy_rect` and `vm_fill_spans` went from
  *verified* to *not verified* until the sweep was re-run. Nothing about the
  transcription was wrong; the tooling was. This is why the sweep regenerates
  the table rather than the table being edited: a claim that is only ever added
  to is a marketing document.
- The same sweep reported `vm_fill_spans` occurrence 300 as **NOT ENTERED**
  rather than passing it. The routine is called 1,078 times in all but not that
  often within the default instruction budget. Distinguishing "never called"
  from "called and agreed" is the whole point of that message.

### Branches transcribed but never run

Verified means the paths that were reached agreed. These were not reached:

- `step_word_4e87` (0x0144e) wraps its counter at 0x2a00. That needs 10,752
  calls; over both intro screens it is called 428 times and the counter never
  exceeds 0x1ab.
- `fill_rect` (0x20079) has an outline path, and `present_frame` (0x081cc) two
  hooks - all stubs, all measured unreachable here.
- `vm_span` (VGA:0x034f) and `vm_fill_spans` (VGA:0x0be6) each branch to a
  high-colour variant that no call on these screens takes.

### Limits of the verifier as it stands

- It compares **writes**, not reads. A read has no external effect of its own -
  it can only change behaviour through a write that follows - so the writes are
  the complete observable. But the original's reads are not recorded at all,
  because a second Unicorn IN hook would override the emulator's own and change
  what the guest sees.
- ~~It knows how to seed only the DGROUP words a routine is declared to use.~~
  **Fixed.** The port models DGROUP as a 64 KB byte array, so the verifier
  seeds the **whole segment** before a call and compares the whole of it
  afterwards. A routine touching state nobody declared is now caught rather
  than missed, and near pointers into DGROUP work at all.
- The comparison now covers **all 640 KB below the VGA aperture**, not just
  DGROUP, so a routine writing into an allocation is checked too.
- **The driver's own code is excluded, deliberately.** `VM.OVL` is
  self-modifying - VGA:0x0be6 patches the row-table pointer into `cs:[0xbe4]`
  and VGA:0x15d0 patches an immediate at `cs:[0x15ce]` - and a C transcription
  has no code to patch. This is the one class of difference the port cannot
  reproduce and should not; it is excluded by name and reason, not because it
  was awkward.
- The stack is **inside** the compared segment - SS is DGROUP in this program -
  so the bytes a call used as stack are excluded, bounded by the lowest SP the
  call reached. The port has its own C stack and cannot reproduce them.
- Registering memory hooks across all of memory **derails the guest** - it
  opened a file with a garbage name and then executed an invalid instruction.
  They are range-limited to the VGA aperture.

### Known gaps, not argued away

- **The emulated instruction rate is a guess.** `drive.DEFAULT_IPS` is
  2,000,000, chosen and not measured. It sets the frame rate the guest believes
  it is achieving, so no timing claim can be made until it is measured against
  the original in cycles.
- **Sound is not modelled.** `SX.OVL` is loaded but the sound path is unchecked.
- **`VM.OVL`'s other seven drivers** are never executed and never will be.
- The **name-hash** in `RESOURCE.MAP` is not derived.

## Next

1. Find the **handler tables** and re-seed the code map through them; the 577 is
   a floor, not a count.
2. Establish which segments are the C runtime and which are the game.
3. Decide which of the 577 are the **Borland C runtime**. Those are not the
   game's logic and reconstructing them from Borland's binary is both pointless
   and worse legally; the port uses the host's C library and marks them as
   ours, kept out of the verifier's dispatch. Which routines those are is not
   yet established.
4. Continue transcription, targeting the **two intro screens** - the title screen
   (page flips 6..279) and the credits screen (from flip 280) - both of which
   animate and so exercise real game logic, and prove each routine against the
   original rather than against the screen.

## Deferred

- Matching (byte-exact) decompilation.
- The seven non-VGA drivers in `VM.OVL`.
- Sound.
- Anything past the intro screens: the menu, the puzzles, the level editor.
