# Status

*Last updated 2026-08-31.*

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
  screen and the credits screen, 16-colour planar and page-flipped. The mode is
  0x12 and the card scans 640x480 throughout; the game moves Start Vertical
  Blank instead of Vertical Display End, so the *picture* is 640x400 for its own
  screens and **640x471 for the Sierra logo**. DOSBox reports the same two
  sizes. Two 640x400 pages fit in one 64 KB plane only because the tail is
  never shown.
- **Captures are reproducible.** They are taken on the guest's own cue - the
  page flip that makes a composed frame visible - and the machine runs on a
  virtual clock driven by emulated instructions, so a given capture is the same
  capture on another machine and after a rebuild.
- **Both intros play out in the port**, with no routine left un-transcribed on
  the way through: the title screen, the machine running, and the credits. What
  that is worth is measured below rather than asserted - and for the title
  screen the answer is that every captured flip of it is exact.
- **The developer hooks are out of the shipping binary, and `devtim` runs the
  game.** `reconstruct/Makefile` has always said "tools/ calls devtim, so
  nothing a measurement depends on can drift into what ships", and that was not
  true: `devdump.c` sat in the object list both binaries link, so `TIM_CLICK`,
  `TIM_POINTER`, `TIM_FLIPS` and `TIM_FLIPHASH` were compiled into the game and
  every tool drove `./tim`. It could not drive `devtim` instead, because
  `devtim` reset the machine, composed one empty frame and stopped.

  Now `devdump.c` links only into `devtim`, `tim` gets `devstub.c` - one
  do-nothing `dev_flip_dump`, because `io.c` calls it on the guest's page flip
  whatever it is linked into - and `devtim` runs the game. Checked: the string
  `TIM_CLICK` and its three companions appear **zero** times in `tim` and four
  times in `devtim`; the shipping binary given `TIM_FLIPS` writes no frames;
  and the briefing comparison through `devtim` gives the same answer as before,
  0 of 307,200 on all three flips.

  `devtim` links no SDL, which was the point of registering the window in
  main.c rather than in io.c, so the comparison no longer needs a dummy video
  driver either. Nothing it reads came from the window in the first place:
  `dev_flip_dump` composes the frame from the planes on the page flip.

  `sdl.c` had two more of its own, and they are gone too. `TIM_FRAMES` wrote
  every frame the window *presented*, which a comparison then had to match to
  the original's captures by content because presents and page flips do not
  line up; `tools/compare_frames.py` already preferred the flip-numbered path
  and now uses it exclusively, so that flag was deleted rather than moved.
  `TIM_FRAME` wrote the frame the port stopped on, and is `dev_final_frame` in
  `devdump.c` now, registered by `devmain.c` as the abort hook - no window
  needed. `tools/compare_port.py` drives `devtim` for it.

  Measured after: `TIM_CLICK`, `TIM_POINTER`, `TIM_FLIPS`, `TIM_FLIPHASH` and
  `TIM_FRAME` appear **zero** times in `tim` and five times in `devtim`, no tool
  in `tools/` names the shipping binary, and the briefing comparison still says
  0 of 307,200 on all three flips.

  `dev_final_frame` has now been exercised where it sits. It fires on the abort
  a stub causes, and the port no longer aborts on the way to the briefing - so
  it took a second click to reach one. `TIM_CLICK` takes a comma-separated list
  now for that reason, and `TIM_CLICK=200:320:200,400:78:105` - the menu, then
  the panel's play triangle - reaches `0x0f8c2`, which is not transcribed, and
  writes its 307,200 indices and 768 bytes of palette. The shipping binary
  given the same variables writes nothing, which is the other half of the
  check.

  **What the play button reaches is `0x0f8c2`**, and that is the next thing
  standing between the briefing and the level running.

- **Saving a machine works, and is pixel-exact.** Seven clicks in, the port
  creates the file, opens it for writing, truncates it, fills the stream
  buffer, flushes and closes - and the screen it comes back to matches the
  original in **0 of 307,200 pixels** on flips 1200 and 1250.

  `uv run python tools/check_briefing.py --screen save` re-runs it.

  **Nothing is written to the disk.** The port satisfies guest writes from an
  in-memory overlay, keyed by the DOS name the file was created under, and
  opening that name again finds the overlay before the host - so a machine
  saved in a session can be loaded back in it. That is what the emulator does,
  and matching it is correctness rather than caution: the reference is what
  defines what the game sees when it saves and re-reads. There is no code in
  `io.c` that opens a host file for writing.

- **The file picker is reached and is pixel-exact.** Four clicks in - dismiss
  the copy-protection screen, the wrench, YES to enter freeform, then Load
  Machine - the port and the original draw the LOAD MACHINE dialog identically:
  **0 of 307,200 pixels** on flips 740, 760 and 790, from the entry point with
  no snapshot. Frame, path field, the sorted listing with its padded 8.3
  columns, both scroll arrows, both buttons.

  `uv run python tools/check_briefing.py --screen picker` re-runs it.

  **It found a fault nothing else would have.** The port listed every file in
  the directory where the original listed three, a stable 2155 pixels all
  inside the list box: `pick_file`'s pattern is pushed *last* and is therefore
  its **third** argument, and the port had it first - so it copied an empty
  string, built no extension filter, and filtered nothing. The transcription
  read correctly either way, and every routine in the chain was individually
  right.

- **The level-one briefing is reached and is pixel-exact.** The port runs the
  intro, a click, the copy-protection screen and the whole briefing paint
  without hitting a stub, and frame 1200 of it differs from the original in
  **0 of 307,200 pixels** - frame, panel, sliders, odometers, title bar,
  description, every scaled part in the play area, and the mouse pointer.
  Re-check it with `uv run python tools/check_briefing.py`, which runs both
  sides and compares three settled flips - 210, 230 and 260 - in 28 seconds,
  saying for each how many pixels differed. By hand, for one flip:

      uv run python tools/capture.py --click 200:320:200 --flip 260 \
          --insns 150000000 --out out/ref --no-png
      SDL_VIDEODRIVER=dummy TIM_CLICK=200:320:200 \
          TIM_FLIPS=out/portframes:260 ./reconstruct/tim
      uv run python tools/diff_png.py --capture out/ref/flip0260.scrn \
          --raw out/portframes/flip0260.scrn --name out/briefing

  **And it is compared from the entry point, not from a snapshot.** The
  emulator could not be given a click - this game installs an INT 33h user
  handler and never polls, and the shared emulator does not keep the
  handler - so every screen past the intro used to need a snapshot made by
  hand. `TimMachine` now keeps that handler and calls it, `capture.py
  --click` works, and both sides run from the program's own entry point
  with the same click at the same flip: **0 of 307,200 pixels differ at
  flip 210 and at flip 260**.

  It is not one frame either. Both sides settle on a static screen and the
  whole-frame CRC-32 of that screen is the same number - `62994813` - on the
  reference from its flip 5 and on the port for 5,464 consecutive flips, 901 to
  6364. The port's own earlier digest, `d3ed681a` over flips 208 to 900, is the
  same screen with the pointer still where the click left it; putting the
  pointer where the reference's is makes the two digests equal, which is the
  point of `TIM_POINTER`.

  The routines the screen exercises verify individually as well as in
  aggregate: `blit_scaled_a` over 57 calls, `vm_blit_bitmap`, `vm_save_rect`
  and `vm_restore_rect`.

  **The last 452 pixels of that were the emulator, not the port.** Its
  `_on_plane_read` satisfied only the byte at the read's address and ignored
  the size, so the high byte of every `rep movsw` word out of video memory came
  back from flat memory as zero. That is how the game saves the rectangle under
  its mouse pointer, so the *reference* left a trail of black wherever the
  screen underneath was solid, and the port - drawing it correctly - was the
  one that looked wrong. Generic VGA behaviour, fixed upstream; see the pin
  note under Open.

### How much is transcribed

`make -C reconstruct test` counts the port: **transcribed 635, ours 17, stubs
26, unmarked 0**. That is the authoritative figure for the port, and it says
nothing about the game.

For the game, `tools/coverage.py` asks the answerable question: it runs the
**original** to a chosen flip, records every basic block it enters - a block
hook, not an instruction hook, so it costs about ninety seconds - and compares
that against every address the port claims. Reaching the level-one briefing:

    the port records 598 image addresses and 27 overlay addresses
    the original entered 9005 image blocks and 694 overlay blocks
    image  : 467 of the port's 598 addresses were executed (78%)
    overlay: 19 of the port's 27 addresses were executed (70%)
    of the 427 call targets the original entered on this path,
    the port has 412; missing 15

**The 15 are not missing routines.** Every one lands inside something already
transcribed - `0x0be41` is three bytes into `long_shift_left`, `0x0be5f`
thirty-three, `0x172c1` five bytes into `seg172c_nothing`, `0x19e18` fifty-eight
into `conveyor_nudge_25` - because Borland's runtime routines have more than one
entrance and the part-behaviour tables jump into the middle of shared code.
Where they land has been checked; what each one does has not.

So on the path to the briefing the port accounts for **every routine the
original calls**, and 78% of what it has transcribed is exercised getting there.

What this does *not* say is what fraction of the whole game is done. Static
descent (`tools/codemap.py`) reaches 26% of code bytes and finds 708 call
targets, but it cannot follow a jump table, so that is a floor and not a
denominator. Nobody should quote a percentage of the game from these numbers.

## Open

### The copy-protection screen's page number

Driven from the entry point with the same click, port against original, **312 of
317 flips match by whole-frame digest**. The five that do not are 202 to 206 -
the copy-protection screen - and 207 onward, the whole briefing, matches again.

What differs on those five is one horizontal band, 1521 pixels, at the message
line. The port's line is the original's **shifted two pixels**, with 39 pixels
still differing after the shift: the string itself is not the same. The message
names a manual page, and the page is `(DGROUP 0x44ef & 0xf) + 1` - a different
page each time, so the answer cannot be memorised - so a different value there
gives a different digit, a different measured width, and a centring two pixels
off.

Measured at `copy_protect_screen` on both sides: the original reads 0x44ef as
**3239, page 8**, and the port reads **8821, page 6**. So it *is* the page
number - "page 8" against "page 6" is one digit of a different width, which is
the two-pixel centring and the residual pixels at the digit.

**Why 0x44ef differs is not open, and it is not a transcription fault.** It is
the timer's countdown from 0x2710, one per tick, and by the copy-protection
screen the original has taken 6761 ticks and the port 1179. The reference paces
its timer on *emulated instructions* - the virtual clock, which is what makes a
capture reproducible - and the port paces its on the *host* clock, as a game
should. The port runs the intro far faster than real time, so it accumulates
far fewer ticks over the same frames.

That makes the page number **non-deterministic in the port**: measured over four
runs it came out 6, 7, 12 and 12. So those five flips are expected to differ,
and to differ differently each run, and no amount of transcription will make
them agree. The nearest thing to a fix would be pacing the port's timer on
something other than the clock, which would be bending the deliverable to suit
the harness.

The jitter does not reach the briefing: over runs where flip 204 came out
`ddecde29` and `39edfbd3`, flip 260 was `d3ed681a` both times.

**That measurement took 25 seconds, and the run that "needed a per-instruction
hook" was the same measurement done wrong** - twice over. `uc.hook_add` takes a
begin and an end, and a code hook bounded to one address does not fire on every
instruction; the earlier attempt registered it unbounded. It was also competing
with several forgotten runs of the verifier. See the pin note.

The screen is invisible while this happens: the palette is still black from the
fade, so the difference is in the indices only. It does not reach the briefing.

### The emulator pin

`pyproject.toml` names `548df402fbbd3edd2a3f256763661a83d866397b`. The
multi-byte video-memory read fixed above is committed in the emulator but **not
pushed**, and the installed copy under `.venv` has been patched by hand so the
comparisons here are correct. That is not a state to leave: a reinstall silently
puts the fault back, and with it the 452 pixels. Moving the pin is a deliberate
act and the verification sweep is re-run afterwards.

It is no longer silent, at least. `tools/check_briefing.py` tests the emulator
for that exact fault before it compares anything - it calls the read hook with a
size of two and sees whether two bytes come back - and refuses with an
explanation rather than reporting 452 differing pixels and letting the port take
the blame. Verified both ways: the probe answers True against the patched copy
and False against the pinned one.

What has been re-run is the part of the sweep the change can reach - the
routines that read video memory - and against the fixed emulator, from the
copy-protection snapshot, they verify over more than a thousand calls each:

    vm_save_rect       VGA:0x12fb   verified  (1252 calls seen)
    vm_restore_rect    VGA:0x13b9   verified  (1253 calls seen)
    vm_blit_bitmap     VGA:0x1707   verified  (1252 calls seen)
    blit_scaled_a      0x227ac      verified    (57 calls seen)

`copy_rect_around_cursor` is not reached from that snapshot and is therefore
unchecked, not wrong.

**`verify.py --all` costs far more than a subset, and it took three goes to
measure that honestly.** Two figures written here earlier - "under twenty
million instructions in six minutes" and "about twenty-five minutes for five
routines" - were taken on a machine that had several forgotten hooked runs of
the same tool competing for it. They were wrong, and were quoted twice before
anyone checked.

Measured idle:

    five routines, 60M instructions   63 seconds     ~1M instructions/second
    419 routines,  20M instructions   ~20 minutes    ~17k instructions/second

So a subset is quick and `--all` is sixty times slower per instruction. The
correction that replaced the first wrong figures said the budget was "minutes,
not hours" - that is true of `--only` and false of `--all`, where 2.6 billion
instructions at this rate is days.

What costs it is **not** established. The obvious suspect is not the
per-instruction hook, which does an O(1) dictionary lookup either way, but the
per-occurrence snapshot: each captured call copies the whole 640 KB below the
video aperture, and 419 routines at up to six occurrences each is on the order
of a gigabyte of copying. That is inference from the shape of the code, not a
profile, and it is written down as such.

`uc.hook_add` does take a begin and an end, and bounding a code hook to one
address is much cheaper than leaving it global - that is how the 0x44ef probe
above was done in 25 seconds. Whether that helps `collect_all` depends on where
the time actually goes, which nobody has measured.


### The intros, compared frame by frame

`reconstruct/devdump.c` writes the port's composed frame on the guest's own page
flip - the same cue `tools/capture.py` takes its reference frames on - named by
the flip number, so there is nothing to match: flip N is flip N.
`tools/compare_frames.py` pairs them directly. **Indices, never colours.**

**PENDING** - being re-measured after the two faults below. The last number
taken, 6742 of 6742 over a cropped 640x400 frame, is not being quoted here
because the crop is exactly what was wrong with it.

**The comparison used to crop the frame to 640x400, and could not see the
logo.** Each capture is 640x480 and `tools/compare_frames.py` took the top 400
rows of it, with a comment saying that was what the game programs the CRTC for.
That is true of the intro screens and false of the Sierra logo, which asks for
471 rows - so seventy rows of the logo were compared against nothing, the port
composed only 400 and cut them, and "every captured flip matches exactly" was
reported over a frame that was missing the part that differed. An instrument
that shares the port's blind spot cannot report it. The crop is gone; a capture
that is not the expected size is now an error.

Uncropping it found a difference at flips 3 and 5 immediately: 6913 indices
each, all in rows 425 to 469. The transition out of the logo flips to start
address 0x8200 while the blanking line still says 471 rows, and 471 rows of 80
bytes from 0x8200 runs off the end of the 64 KB plane at row 403. The port's
composition wrapped the address in a `uint16_t` and painted the top of the logo
across the bottom of the screen; the reference stops at the end of the plane.
Nothing was ever visible - the palette is still black at those two flips - so
this is a choice about addresses the picture does not use, settled the way the
reference settles it, and **neither side is checked against a real card**.

**The blanking line is displayed, and both sides used to drop it.** The port and
`tools/tim.py` both blanked from Start Vertical Blank *inclusive*, making the
picture 470 rows and 399. They agreed with each other, which proved nothing:
they shared the convention. DOSBox reports this game as 640x471 for the logo and
640x400 for its own screens - `svb + 1` - and the game's own memory holds a full
640-pixel drawn row at y=399, which it would have no reason to write for a line
it could not show. Both sides now keep the blanking line, and the references
were re-captured.

**How far the captures reach is part of the claim.** An earlier run of this
said "534 of 534" and was true, and the port was still wrong: the captures
stopped at flip 533 and the first difference was at **539**, six flips past the
end of the reference. A comparison says nothing about the frames nobody
captured, and the honest form of the number always carries the count.

### What the polygon filler had wrong, and how it was found

The title screen carried a stripe of dashes across rows 332 to 342, from x=462
to the right edge, in one of the two pages - which is why the difference
alternated between 402 and 533 indices. It was read first as a moving part left
un-erased and then as a belt, and it was neither; what settled it was a
backtrace on **every** write the driver makes to those rows, which named
`vm_fill_spans` under `draw_polygon` and nothing else.

Five faults, all in code transcribed by following the original's registers
rather than its instructions, and each found by the verifier rather than by
reading:

- `poly_edge_shallow` advanced its buffer pointer by the original's 2 or -6
  and not by the 2 that `stosw` adds itself, so it wrote the other slot of the
  row it had just written. The right ends of a run of rows were then never set,
  and the driver fills an unset right end to the clip's edge. That was the
  stripe.
- The same routine folded its first write into the loop, which added an
  `err += e` the original does not do there; on a shallow edge `e` is negative,
  so every end after it came out two columns short.
- The winding test divided each edge's rise by the *other* edge's run, and
  compared the quotients the wrong way round. Where the two slopes straddled,
  the polygon was wound backwards and the fill built from the wrong chains.
- The two tie-breaks for the topmost and bottommost vertex were both reversed.
  This one is worth remembering: the fill came out **pixel for pixel identical**
  - 0 of 262144 plane bytes differ - and only the arrays the routine leaves
  behind disagreed. Nothing that looked at the screen could have caught it.
- `poly_edge_diagonal` put its two ends bottom-first where the original puts
  them top-first, so the row count came out zero or negative and the side was
  not written at all. It hid because 45 degrees is a special case of its own.

### The credits screen: two more, and the tool that found them

The original drew a wedge of light from the candle to the magnifying glass and
the port drew nothing. Reasoning backwards from the pixels went astray twice -
once blaming a part that is never stepped, once blaming the drawing - so the
question was settled by comparing the two machines directly.

`tools/parts.py` walks the original's part chains at a page flip and
`reconstruct/devdump.c` walks the port's at the same flip, both writing the same
line per part, on the same cue the captures use. At flip 295 exactly **one line
of seventy-eight** differed, and it named the fault: the kind 30 part at
(88,238) had `+0x62` pointing at a kind 45 part in the original and null in the
port.

- `part_step_3035` cleared its "blocked" flag when the candidate's bit 4 agreed
  with its own; the original clears it when they *differ*. That one word left
  `+0x62` null, so `draw_part_extra` had nothing to aim its triangle at.
- With the wedge drawn, it was twenty-one pixels too long: `part_setup`'s table
  carried each setup's connection points and nothing else, and the rows that
  also write the grab box at +0x72/+0x73 left it at zero. The grab box is both
  the point the triangle aims at and the point `grab_distance` measures to, so
  the second candle's wedge was missing entirely - the part it should have
  picked up measured sixty pixels away instead of two.

  **Four** setups in the segment write it, and finding them one screen at a
  time would have taken four rounds. Every setup in the kind table was
  disassembled instead, which found all four at once: two go in the table, and
  two were already in `sized[]` because their value depends on a flag.

**The first run of that comparison said the two lists were identical, and it was
wrong.** The normalisation replaced every pointer field with one placeholder,
which makes "points at something" and "points at nothing" compare equal - and
that distinction was the entire content of the field. `--diff` now rewrites a
pointer as its index in the walk and a null as `-`.

### Coverage - as last measured, 2026-08-30

Measured over **both intros** now - `out/reached_intros.json`, flips 6..600 -
and not the title screen alone. The old set stopped at flip 40, so it missed
everything the credits reach and reported "0 to go" while the gun was still
not firing. A coverage figure is only as wide as the window it was taken over.

| | |
| --- | --- |
| call targets found by recursive descent, seeded with the kind tables | **708** |
| reached by both intros, flips 6..600 | **252**, of which **0** are left |
| of all reachable code, transcribed | **133676 of 185280 bytes (72.1%)** |
| of what the intros reach | **81486 of 82785 bytes (98.4%)** |
| the VGA driver | 24 of 37 routines, at least 8738 of 11024 bytes (79.3%) |
| part setups in segment 172c | 40 of 40 |

The transcribed and verified counts are **not** written here by hand - they
were wrong within one session when they were. They come from the sweep below,
which `tools/verify.py --all` regenerates in place. It stops the emulator at
each routine's entry, lets the **original body** run to its return, and
compares what each did to the hardware:

**A sweep now has hands.** `--click FLIP:X:Y` presses the button at a page
flip, the same form `TIM_CLICK` and `tools/snapshot.py --click` take, so one
list drives the port, the reference capture and the verifier to the same place.
Before it, everything behind a menu reported "transcribed, never called" - true
of the run, and silent about the routine. The picker's and the writer's
routines are only reachable this way, and several of them - `validate_filename`
and its eleven reserved device names, `is_machine_file`'s magic word - produce
nothing a screen comparison can see, so this is the only check they have.

A routine's `check_occurrences` says how many times it actually runs and the
spec says why: `is_machine_file` once per LOAD and never on a SAVE, because a
save has no magic to check. Asking for a fifth call of something that runs four
times reports "not verified" about a routine that was checked on every call it
made, which is worse than not asking.

<!-- VERIFY:BEGIN -->
| routine | address | occurrences checked | result |
| --- | --- | --- | --- |
| `vm_set_display_lines` | 0x08f77 | 0 (missed 1) | **not verified** |
| `vm_save_rect` | VM.OVL VGA:0x12fb | 0 | agreed |
| `vm_restore_rect` | VM.OVL VGA:0x13b9 | 0 | agreed |
| `atan2_long` | 0x2d296 | - | **transcribed, never called** on these screens |
| `link_nearby_objects` | 0x03566 | - | **transcribed, never called** on these screens |
| `find_edge_contact_reversed` | 0x00b6c | - | **transcribed, never called** on these screens |
| `resolve_collisions` | 0x00556 | - | **transcribed, never called** on these screens |
| `find_edge_contact` | 0x007af | - | **transcribed, never called** on these screens |
| `integrate_object` | 0x02c93 | - | **transcribed, never called** on these screens |
| `place_object_for_draw` | 0x05be4 | - | **transcribed, never called** on these screens |
| `add_sub_object_shapes` | 0x05ef6 | - | **transcribed, never called** on these screens |
| `set_object_extent` | 0x05c77 | - | **transcribed, never called** on these screens |
| `object_delta_angle` | 0x004ab | - | **transcribed, never called** on these screens |
| `arctan_lookup` | 0x2a941 | - | **transcribed, never called** on these screens |
| `apply_contact_friction` | 0x02da0 | - | **transcribed, never called** on these screens |
| `vm_read_pixel` | VM.OVL VGA:0x1453 | - | **transcribed, never called** on these screens |
| `read_pixel_clipped` | 0x2241b | - | **transcribed, never called** on these screens |
| `vm_plot_pixel` | VM.OVL VGA:0x14c9 | - | **transcribed, never called** on these screens |
| `plot_pixel_clipped` | 0x2244d | - | **transcribed, never called** on these screens |
| `init_sequence_params` | 0x28305 | - | **transcribed, never called** on these screens |
| `next_matching_record` | 0x29966 | 0, 1, 4 | agreed |
| `midi_bend_event` | 0x280fe | - | **transcribed, never called** on these screens |
| `step_sequence` | 0x27c4e | - | **transcribed, never called** on these screens |
| `midi_note_off_event` | 0x27e92 | - | **transcribed, never called** on these screens |
| `midi_event_6` | 0x27f54 | - | **transcribed, never called** on these screens |
| `midi_meta_event` | 0x2817e | - | **transcribed, never called** on these screens |
| `midi_skip_event` | 0x2817a | - | **transcribed, never called** on these screens |
| `skip_unknown_event` | 0x2828e | - | **transcribed, never called** on these screens |
| `midi_controller_event` | 0x27f85 | - | **transcribed, never called** on these screens |
| `midi_program_event` | 0x28086 | - | **transcribed, never called** on these screens |
| `midi_event_9` | 0x280da | - | **transcribed, never called** on these screens |
| `midi_note_event` | 0x27ee1 | - | **transcribed, never called** on these screens |
| `free_node_list` | 0x28baf | 0, 1 | agreed |
| `create_sequence` | 0x28935 | - | **transcribed, never called** on these screens |
| `free_for_kind` | 0x2a017 | 0, 1 | agreed |
| `alloc_for_kind` | 0x29f89 | 0, 1 | agreed |
| `start_sequence_far` | 0x28480 | - | **transcribed, never called** on these screens |
| `load_and_start_sequence` | 0x29034 | - | **transcribed, never called** on these screens |
| `start_sequence` | 0x26783 | - | **transcribed, never called** on these screens |
| `advance_volume_ramp` | 0x278e9 | - | **transcribed, never called** on these screens |
| `set_sequence_volume` | 0x279a9 | - | **transcribed, never called** on these screens |
| `sound_service` | 0x27ace | 0, 1 | agreed |
| `drop_unless_polled` | 0x27b52 | - | **transcribed, never called** on these screens |
| `poll_sequences` | 0x27b7e | 0, 1 | agreed |
| `remove_sequence` | 0x26e7b | - | **transcribed, never called** on these screens |
| `sound_callback` | 0x292a1 | - | **transcribed, never called** on these screens |
| `sequencer_tick` | 0x26f2a | - | **transcribed, never called** on these screens |
| `install_driver` | 0x265f2 | 0 | agreed |
| `configure_driver` | 0x26629 | 0 | agreed |
| `silence_driver` | 0x2664e | - | **transcribed, never called** on these screens |
| `set_master_level` | 0x26721 | 0 | agreed |
| `retire_and_tick` | 0x26a57 | - | **transcribed, never called** on these screens |
| `set_master_level_far` | 0x28431 | 0 | agreed |
| `install_driver_far` | 0x28458 | 0 | agreed |
| `configure_driver_far` | 0x2846a | 0 | agreed |
| `retire_and_tick_far` | 0x284ef | - | **transcribed, never called** on these screens |
| `silence_driver_far` | 0x28559 | - | **transcribed, never called** on these screens |
| `voice_playing` | 0x287ad | - | **transcribed, never called** on these screens |
| `follow_then_tick` | 0x289ba | - | **transcribed, never called** on these screens |
| `seek_to_sound_record` | 0x28bf2 | 0, 1, 4 | agreed |
| `read_sound_records` | 0x28cf7 | 0, 1 | agreed |
| `open_sound_file` | 0x296b4 | 0, 1 | agreed |
| `read_record` | 0x29da0 | 0, 1, 4 | agreed |
| `start_sound` | 0x29c3b | 0 | agreed |
| `setup_sound_device` | 0x28655 | 0 | agreed |
| `load_sound_module` | 0x28580 | 0 | agreed |
| `load_named_chunk` | 0x28886 | 0 | agreed |
| `load_sound_bank` | 0x289e8 | 0, 1 | agreed |
| `load_resource_block` | 0x28f74 | 0 | agreed |
| `build_sound_index` | 0x28e87 | 0, 1 | agreed |
| `insert_by_key` | 0x28ddb | - | **transcribed, never called** on these screens |
| `stop_voice_playing` | 0x290ab | - | **transcribed, never called** on these screens |
| `free_voice_records` | 0x29106 | - | **transcribed, never called** on these screens |
| `start_on_free_voice` | 0x29152 | - | **transcribed, never called** on these screens |
| `stop_all_voices` | 0x2923d | 0 | agreed |
| `set_sound_callback` | 0x2928c | - | **transcribed, never called** on these screens |
| `set_master_level_ok` | 0x296a1 | 0 | agreed |
| `alloc_voice_records` | 0x28800 | 0 | agreed |
| `stop_sequences` | 0x294ff | - | **transcribed, never called** on these screens |
| `shutdown_sound` | 0x29cf6 | - | **transcribed, never called** on these screens |
| `stop_sound` | 0x292f4 | - | **transcribed, never called** on these screens |
| `delay_five_ticks` | 0x2937f | - | **transcribed, never called** on these screens |
| `tick_delay` | 0x293b8 | - | **transcribed, never called** on these screens |
| `remove_and_free_records` | 0x293c1 | 0 (missed 1) | **not verified** |
| `start_sequence_by_id` | 0x29a49 | - | **transcribed, never called** on these screens |
| `vm_init` | 0x22483 | 0 | agreed |
| `load_video_driver` | 0x22efd | 0 | agreed |
| `detect_adapter` | 0x225d2 | 0 | agreed |
| `read_bmp_info` | 0x234d2 | 0, 1 (missed 4) | **not verified** |
| `table_618a_in_use` | 0x215d5 | 0 | agreed |
| `mouse_move_to` | 0x22113 | 0 | agreed |
| `huge_add_positive` | 0x22190 | 0, 1, 4 | agreed |
| `install_divide_trap` | 0x22394 | 0 | agreed |
| `restore_file_record_from` | 0x23ee4 | 0, 1 (missed 4) | **not verified** |
| `set_field_4_of_each` | 0x252b4 | 0, 1 | agreed |
| `count_list` | 0x252e0 | 0, 1 (missed 4) | **not verified** |
| `far_copy` | 0x25d96 | 0, 1 (missed 4) | **not verified** |
| `string_concat` | 0x0dc95 | 0, 1, 4 | agreed |
| `stdio_setbuf` | 0x0c1b2 | - | **transcribed, never called** on these screens |
| `set_holiday_flags` | 0x08259 | 0 | agreed |
| `dos_get_cur_dir` | 0x0b7b3 | 0 | agreed |
| `dos_getdate` | 0x0bd4a | 0 | agreed |
| `heap_free_far` | 0x0bb2d | 0, 1, 4 | agreed |
| `read_tim_cfg` | 0x12ba7 | 0 | agreed |
| `game_fread_far` | 0x11dd1 | 0, 1 (missed 4) | **not verified** |
| `show_page_thunk` | 0x2149a | - | **transcribed, never called** on these screens |
| `save_rect_thunk` | 0x21ab5 | 0 | agreed |
| `buffer_size_thunk` | 0x21ab9 | 0, 1 | agreed |
| `restore_rect_thunk` | 0x2247f | 0 | agreed |
| `bios_video_kind` | 0x22764 | 0 | agreed |
| `int_to_string` | 0x0d4bd | 0, 1, 4 | agreed |
| `long_int_to_string` | 0x0d4ff | - | **transcribed, never called** on these screens |
| `draw_odometer_digit` | 0x15a7e | - | **transcribed, never called** on these screens |
| `set_clip_counter_strip` | 0x026e8 | - | **transcribed, never called** on these screens |
| `draw_counter_word` | 0x0262b | - | **transcribed, never called** on these screens |
| `draw_counter_long` | 0x02686 | - | **transcribed, never called** on these screens |
| `redraw_counters` | 0x025d8 | - | **transcribed, never called** on these screens |
| `start_counters` | 0x024fa | - | **transcribed, never called** on these screens |
| `step_counters` | 0x02510 | - | **transcribed, never called** on these screens |
| `long_to_string` | 0x0c029 | 0, 1, 4 | agreed |
| `heap_malloc_far` | 0x0bb1e | 0, 1, 4 | agreed |
| `detect_pcjr` | 0x20be0 | 0, 1 | agreed |
| `timer_remove` | 0x2072e | - | **transcribed, never called** on these screens |
| `timer_install` | 0x206c1 | 0, 1 | agreed |
| `timer_add_callback` | 0x20654 | 0, 1 | agreed |
| `timer_drop_callback` | 0x2069e | - | **transcribed, never called** on these screens |
| `huge_equal` | 0x0bd0d | 0, 1, 4 | agreed |
| `near_memset` | 0x0d543 | 0, 1, 4 | agreed |
| `heap_calloc` | 0x0c833 | 0, 1, 4 | agreed |
| `heap_calloc_far` | 0x0bb75 | 0, 1, 4 | agreed |
| `huge_add_to` | 0x0be82 | 0, 1, 4 | agreed |
| `huge_add` | 0x0bf0a | 0, 1, 4 | agreed |
| `huge_post_add` | 0x0bf6a | - | **transcribed, never called** on these screens |
| `decompress_lzss` | 0x1e7f2 | 0, 1, 4 | agreed |
| `huff_get_bit` | 0x1dfd6 | 0, 1, 4 | agreed |
| `huff_get_byte` | 0x1e00b | 0, 1, 4 | agreed |
| `huffman_reconst` | 0x1e1af | - | **transcribed, never called** on these screens |
| `huffman_update` | 0x1e338 | 0, 1, 4 | agreed |
| `huffman_start` | 0x1e0b3 | 0, 1 | agreed |
| `decompress_lzw` | 0x1ca62 | 0, 1 (missed 4) | **not verified** |
| `read_input_block` | 0x1c3e6 | 0, 1, 4 | agreed |
| `next_lzw_code` | 0x1cc65 | 0, 1, 4 | agreed |
| `resource_seek` | 0x1d983 | 0, 1, 4 | agreed |
| `lzw_reset` | 0x1c970 | 0, 1 | agreed |
| `lzss_reset` | 0x1dc15 | 0, 1, 4 | agreed |
| `open_resource` | 0x1d54e | 0, 1, 4 | agreed |
| `close_resource` | 0x1d798 | 0, 1, 4 | agreed |
| `resource_size` | 0x1d95f | 0, 1, 4 | agreed |
| `read_resource` | 0x1d868 | 0, 1, 4 | agreed |
| `resource_read` | 0x1c92b | 0, 1, 4 | agreed |
| `decompress_rle` | 0x1c278 | 0 | agreed |
| `emit_literal_run` | 0x1c493 | 0, 1, 4 | agreed |
| `emit_fill_run` | 0x1c51e | 0, 1, 4 | agreed |
| `emit_byte` | 0x1c5a3 | 0, 1, 4 | agreed |
| `far_move` | 0x0bd2e | 0, 1, 4 | agreed |
| `string_equal_upto` | 0x23e70 | 0, 1, 4 | agreed |
| `copy_file_record` | 0x23ea8 | 0, 1, 4 | agreed |
| `restore_file_record` | 0x23f90 | 0 | agreed |
| `seek_named_chunk` | 0x23fc2 | 0, 1, 4 | agreed |
| `open_file_record` | 0x23f2c | 0, 1, 4 | agreed |
| `make_file_current` | 0x09a62 | 0, 1, 4 | agreed |
| `find_file_record` | 0x23df2 | 0, 1, 4 | agreed |
| `file_record_size` | 0x242af | 0, 1, 4 | agreed |
| `file_record_valid` | 0x24308 | 0, 1, 4 | agreed |
| `close_file_record` | 0x242d9 | 0, 1, 4 | agreed |
| `close_resource_slot` | 0x1c71a | 0, 1 | agreed |
| `open_resource_slot` | 0x1c783 | 0, 1, 4 | agreed |
| `prepare_resource_slot` | 0x1c7d5 | 0, 1, 4 | agreed |
| `free_if_set` | 0x1c705 | 0, 1 | agreed |
| `read_into_huge` | 0x1c319 | 0, 1, 4 | agreed |
| `next_input_byte` | 0x1c389 | 0, 1, 4 | agreed |
| `stdio_setvbuf` | 0x0db5e | 0, 1, 4 | agreed |
| `stdio_fopen_into` | 0x0d007 | 0, 1, 4 | agreed |
| `io_error` | 0x0bfcd | 0, 1, 4 | agreed |
| `dos_getvect` | 0x0bd70 | 0 | agreed |
| `dos_setvect` | 0x0bd7f | 0 | agreed |
| `long_shift_left` | 0x0be3e | 0, 1, 4 | agreed |
| `string_copy` | 0x0dd33 | 0, 1, 4 | agreed |
| `string_copy_far` | 0x0bb4f | 0, 1 | agreed |
| `string_compare_nocase` | 0x0dd55 | 0, 1, 4 | agreed |
| `string_copy_padded` | 0x0ddaf | 0, 1, 4 | agreed |
| `stdio_fopen` | 0x0d0ce | 0, 1, 4 | agreed |
| `find_free_stream` | 0x0d0a3 | 0, 1, 4 | agreed |
| `parse_open_mode` | 0x0cf4d | 0, 1, 4 | agreed |
| `open_file` | 0x0d5af | 0, 1, 4 | agreed |
| `dos_isatty` | 0x0c018 | 0, 1, 4 | agreed |
| `dos_ioctl` | 0x0c8a3 | 0, 1, 4 | agreed |
| `dos_getattr` | 0x0cd3d | 0, 1, 4 | agreed |
| `dos_open_named` | 0x0d707 | 0, 1, 4 | agreed |
| `dos_close` | 0x0cd80 | 0, 1, 4 | agreed |
| `close_handle` | 0x0cd58 | 0, 1, 4 | agreed |
| `stdio_fclose` | 0x0ce15 | 0, 1, 4 | agreed |
| `game_fopen` | 0x08fcd | 0, 1, 4 | agreed |
| `load_archive_map` | 0x0960f | 0, 1 | agreed |
| `hash_filename` | 0x0980d | 0, 1, 4 | agreed |
| `game_rewind` | 0x093e0 | 0, 1, 4 | agreed |
| `reset_file_record` | 0x23e23 | 0, 1, 4 | agreed |
| `game_fclose` | 0x0917f | 0, 1, 4 | agreed |
| `dos_tell` | 0x0c27b | 0, 1, 4 | agreed |
| `unread_count` | 0x0d20f | 0, 1, 4 | agreed |
| `stdio_ftell` | 0x0d2d4 | 0, 1, 4 | agreed |
| `ulong_divide` | 0x0bd97 | 0 | agreed |
| `fread_huge` | 0x0b93d | 0 | agreed |
| `game_ftell` | 0x093a2 | 0, 1, 4 | agreed |
| `flush_stream` | 0x0ce92 | 0, 1, 4 | agreed |
| `stdio_fseek` | 0x0d26c | 0, 1, 4 | agreed |
| `game_fseek` | 0x092dc | 0, 1, 4 | agreed |
| `game_fgetc` | 0x093f6 | 0, 1, 4 | agreed |
| `reset_machine` | 0x07e45 | - | **transcribed, never called** on these screens |
| `clear_machine` | 0x013e9 | - | **transcribed, never called** on these screens |
| `unlink_node` | 0x05628 | - | **transcribed, never called** on these screens |
| `draw_char` | 0x21670 | - | **transcribed, never called** on these screens |
| `blit_scaled_a` | 0x227ac | - | **transcribed, never called** on these screens |
| `vm_blit_glyph` | VM.OVL VGA:0x124b | - | **transcribed, never called** on these screens |
| `compute_step` | 0x20840 | - | **transcribed, never called** on these screens |
| `draw_compressed_bitmap` | 0x20185 | - | **transcribed, never called** on these screens |
| `draw_part` | 0x16db1 | - | **transcribed, never called** on these screens |
| `draw_rope` | 0x167fa | - | **transcribed, never called** on these screens |
| `draw_belt` | 0x16baf | - | **transcribed, never called** on these screens |
| `draw_machine` | 0x1675e | - | **transcribed, never called** on these screens |
| `step_and_draw_machine` | 0x16181 | - | **transcribed, never called** on these screens |
| `refile_overlapping_parts` | 0x06b5b | - | **transcribed, never called** on these screens |
| `copy_rect_around_cursor` | 0x0b28e | - | **transcribed, never called** on these screens |
| `read_record_fields` | 0x11e3f | - | **transcribed, never called** on these screens |
| `game_fread` | 0x091ef | 0, 1, 4 | agreed |
| `flush_pending_volumes` | 0x27a86 | 0, 1 | agreed |
| `sx_controller` | SX.OVL SPKR:0x03a1 | - | **transcribed, never called** on these screens |
| `sx_pitch_bend` | SX.OVL SPKR:0x0410 | - | **transcribed, never called** on these screens |
| `sx_stop_note` | SX.OVL SPKR:0x037b | - | **transcribed, never called** on these screens |
| `sx_start_note` | SX.OVL SPKR:0x0386 | - | **transcribed, never called** on these screens |
| `sx_speaker_off` | SX.OVL SPKR:0x0480 | - | **transcribed, never called** on these screens |
| `sx_apply_bend` | SX.OVL SPKR:0x04fd | - | **transcribed, never called** on these screens |
| `sx_note_on` | SX.OVL SPKR:0x0497 | - | **transcribed, never called** on these screens |
| `vm_driver_init` | VM.OVL VGA:0x0000 | 0 | agreed |
| `vm_reset_attributes` | VM.OVL VGA:0x011d | 0 | agreed |
| `vm_blit_bitmap` | VM.OVL VGA:0x1707 | 0 (missed 1, 2) | **not verified** |
| `vm_load_bitmap_list` | VM.OVL VGA:0x1015 | 0 | agreed |
| `vm_chunky_to_planar` | VM.OVL VGA:0x10b8 | 0 | agreed |
| `vm_read_four_planes` | VM.OVL VGA:0x11bb | 0 | agreed |
| `vm_build_mask_plane` | VM.OVL VGA:0x11ee | 0 | agreed |
| `vm_bitmap_list_size` | VM.OVL VGA:0x0fd4 | 0, 1 | agreed |
| `vm_buffer_size` | VM.OVL VGA:0x138e | 0, 1 | agreed |
| `vm_show_page` | VM.OVL VGA:0x150f | - | **transcribed, never called** on these screens |
| `vm_copy_rect` | VM.OVL VGA:0x1561 | - | **transcribed, never called** on these screens |
| `vm_span` | VM.OVL VGA:0x034f | - | **transcribed, never called** on these screens |
| `vm_blit_run` | VM.OVL VGA:0x0938 | - | **transcribed, never called** on these screens |
| `vm_fill_spans` | VM.OVL VGA:0x0be6 | - | **transcribed, never called** on these screens |
| `vm_set_palette` | VM.OVL VGA:0x0ec1 | 0, 1 (missed 3) | **not verified** |
| `present_frame` | 0x081cc | - | **transcribed, never called** on these screens |
| `fill_rect` | 0x20079 | - | **transcribed, never called** on these screens |
| `step_word_4e87` | 0x0144e | - | **transcribed, never called** on these screens |
| `set_clip_full_screen` | 0x0834b | - | **transcribed, never called** on these screens |
| `sub_002be` | 0x002be | - | **transcribed, never called** on these screens |
| `clear_word_array_50bf` | 0x166d6 | - | **transcribed, never called** on these screens |
| `bit0_of_468c` | 0x2147d | 0, 4, 25 | agreed |
| `advance_record` | 0x2891a | 0 (missed 2) | **not verified** |
| `match_field_5a_5c` | 0x06f43 | - | **transcribed, never called** on these screens |
| `lookup_table_546c` | 0x11d44 | - | **transcribed, never called** on these screens |
| `string_contains_r` | 0x1c6e3 | 0, 2 | agreed |
| `flag_bit_48ea` | 0x2213e | 0, 4, 30 | agreed |
| `select_field_2_or_4` | 0x06f68 | - | **transcribed, never called** on these screens |
| `read_pair_4740` | 0x220e9 | 0 (missed 2, 15) | **not verified** |
| `angle_sin` | 0x2a456 | - | **transcribed, never called** on these screens |
| `angle_cos` | 0x2a47b | - | **transcribed, never called** on these screens |
| `angle_to_quadrant` | 0x004d1 | - | **transcribed, never called** on these screens |
| `chain_contains` | 0x03a61 | - | **transcribed, never called** on these screens |
| `normalise_far_ptr` | 0x22161 | 0, 4, 30 | agreed |
| `follow_far_chain` | 0x2907b | - | **transcribed, never called** on these screens |
| `step_pair_apart` | 0x03d2e | - | **transcribed, never called** on these screens |
| `points_within_140` | 0x04b53 | - | **transcribed, never called** on these screens |
| `splice_list_4e58_onto_4e56` | 0x07b3e | - | **transcribed, never called** on these screens |
| `scale_byte_pair` | 0x282cb | - | **transcribed, never called** on these screens |
| `value_between` | 0x03d67 | - | **transcribed, never called** on these screens |
| `pick_by_flag` | 0x05b65 | - | **transcribed, never called** on these screens |
| `normalise_far_ptr_far` | 0x22386 | 0, 3, 20 | agreed |
| `compute_bounds_53fe` | 0x00386 | - | **transcribed, never called** on these screens |
| `pick_for_record` | 0x05ba7 | - | **transcribed, never called** on these screens |
| `set_side_flags` | 0x004fd | - | **transcribed, never called** on these screens |
| `far_memcpy` | 0x222c6 | 0, 2 | agreed |
| `claim_page_slot` | 0x0b429 | 0, 3 (missed 9) | **not verified** |
| `save_or_restore_draw_state` | 0x0b47f | 0, 1 (missed 8) | **not verified** |
| `clamp_record_pair` | 0x02bcc | - | **transcribed, never called** on these screens |
| `set_clip_for_mode` | 0x082c3 | - | **transcribed, never called** on these screens |
| `link_record_into_buckets` | 0x166ef | - | **transcribed, never called** on these screens |
| `update_velocity` | 0x07283 | - | **transcribed, never called** on these screens |
| `clip_and_draw_line` | 0x21e34 | - | **transcribed, never called** on these screens |
| `vm_draw_line` | VM.OVL VGA:0x0998 | - | **transcribed, never called** on these screens |
| `far_memset` | 0x22300 | 0, 2, 9 | agreed |
| `compute_swept_bounds_5400` | 0x002dd | - | **transcribed, never called** on these screens |
| `angles_same_side` | 0x003df | - | **transcribed, never called** on these screens |
| `insert_sorted` | 0x05646 | - | **transcribed, never called** on these screens |
| `dos_alloc_bytes` | 0x21abd | 0, 2, 9 | agreed |
| `mul16x16` | 0x2a269 | - | **transcribed, never called** on these screens |
| `apply_gravity_and_speed` | 0x02c39 | - | **transcribed, never called** on these screens |
| `vm_load_palette` | VM.OVL VGA:0x0f15 | 0, 1 (missed 2) | **not verified** |
| `huge_move` | 0x221ed | 0, 1, 2 | agreed |
| `mouse_init` | 0x21f1d | 0 | agreed |
| `mouse_set_ranges` | 0x21f8d | 0 | agreed |
| `set_font` | 0x2149e | 0 | agreed |
| `install_keyboard` | 0x21094 | 0 | agreed |
| `build_screen_regions` | 0x085c9 | 0 | agreed |
| `count_level_files` | 0x129a8 | 0 | agreed |
| `load_bitmap_list` | 0x2367c | 0 | agreed |
| `free_bitmap_list` | 0x23a18 | - | **transcribed, never called** on these screens |
| `expand_1bpp_to_4bpp` | 0x23a8a | - | **transcribed, never called** on these screens |
| `load_bitmaps` | 0x24f72 | 0 | agreed |
| `planes_to_chunky` | 0x24320 | - | **transcribed, never called** on these screens |
| `compress_bitmap_list` | 0x243bf | - | **transcribed, never called** on these screens |
| `read_far` | 0x2551a | 0 | agreed |
| `load_font` | 0x2307d | 0 | agreed |
| `load_palette` | 0x1e967 | 0, 1, 2 | agreed |
| `set_palette_pointer` | 0x1eb6a | 0, 1 (missed 2) | **not verified** |
| `rotate_point` | 0x03b17 | - | **transcribed, never called** on these screens |
| `alloc_shape` | 0x064b4 | - | **transcribed, never called** on these screens |
| `part_step_27e2` | 0x19aa2 | - | **transcribed, never called** on these screens |
| `part_step_420f` | 0x1b4cf | - | **transcribed, never called** on these screens |
| `part_step_018e` | 0x1744e | - | **transcribed, never called** on these screens |
| `part_hit_0552` | 0x17812 | - | **transcribed, never called** on these screens |
| `part_step_057e` | 0x1783e | - | **transcribed, never called** on these screens |
| `part_step_098a` | 0x17c4a | - | **transcribed, never called** on these screens |
| `part_step_0a5d` | 0x17d1d | - | **transcribed, never called** on these screens |
| `part_hit_0c6c` | 0x17f2c | - | **transcribed, never called** on these screens |
| `part_step_0ca3` | 0x17f63 | - | **transcribed, never called** on these screens |
| `part_step_11a6` | 0x18466 | - | **transcribed, never called** on these screens |
| `part_step_12c2` | 0x18582 | - | **transcribed, never called** on these screens |
| `part_step_13c9` | 0x18689 | - | **transcribed, never called** on these screens |
| `part_hit_14d3` | 0x18793 | - | **transcribed, never called** on these screens |
| `part_step_15ce` | 0x1888e | - | **transcribed, never called** on these screens |
| `part_step_1a82` | 0x18d42 | - | **transcribed, never called** on these screens |
| `part_hit_1c39` | 0x18ef9 | - | **transcribed, never called** on these screens |
| `part_step_1c5f` | 0x18f1f | - | **transcribed, never called** on these screens |
| `part_hit_1d07` | 0x18fc7 | - | **transcribed, never called** on these screens |
| `part_step_1d78` | 0x19038 | - | **transcribed, never called** on these screens |
| `part_step_1e5c` | 0x1911c | - | **transcribed, never called** on these screens |
| `part_step_20fc` | 0x193bc | - | **transcribed, never called** on these screens |
| `part_step_22ae` | 0x1956e | - | **transcribed, never called** on these screens |
| `part_hit_2514` | 0x197d4 | - | **transcribed, never called** on these screens |
| `part_step_2592` | 0x19852 | - | **transcribed, never called** on these screens |
| `part_step_2b99` | 0x19e59 | - | **transcribed, never called** on these screens |
| `part_hit_2f25` | 0x1a1e5 | - | **transcribed, never called** on these screens |
| `part_step_2f3e` | 0x1a1fe | - | **transcribed, never called** on these screens |
| `part_step_3035` | 0x1a2f5 | - | **transcribed, never called** on these screens |
| `part_hit_34b5` | 0x1a775 | - | **transcribed, never called** on these screens |
| `part_step_34d0` | 0x1a790 | - | **transcribed, never called** on these screens |
| `part_step_3635` | 0x1a8f5 | - | **transcribed, never called** on these screens |
| `part_hit_3824` | 0x1aae4 | - | **transcribed, never called** on these screens |
| `part_step_38fc` | 0x1abbc | - | **transcribed, never called** on these screens |
| `part_hit_3ebf` | 0x1b17f | - | **transcribed, never called** on these screens |
| `part_step_3fae` | 0x1b26e | - | **transcribed, never called** on these screens |
| `part_hit_3fe8` | 0x1b2a8 | - | **transcribed, never called** on these screens |
| `part_step_49a1` | 0x1bc61 | - | **transcribed, never called** on these screens |
| `part_hit_016e` | 0x1742e | - | **transcribed, never called** on these screens |
| `part_hit_1de0` | 0x190a0 | - | **transcribed, never called** on these screens |
| `part_hit_2b7e` | 0x19e3e | - | **transcribed, never called** on these screens |
| `mark_belt_shapes` | 0x05f87 | - | **transcribed, never called** on these screens |
| `draw_belt_segment` | 0x16b39 | - | **transcribed, never called** on these screens |
| `belt_orientation` | 0x06de9 | - | **transcribed, never called** on these screens |
| `tension_belt` | 0x072c7 | - | **transcribed, never called** on these screens |
| `draw_part_extra` | 0x171b5 | - | **transcribed, never called** on these screens |
| `draw_polygon` | 0x1eded | - | **transcribed, never called** on these screens |
| `part_step_1649` | 0x18909 | - | **transcribed, never called** on these screens |
| `blast_speed_for_mass` | 0x18a08 | - | **transcribed, never called** on these screens |
| `split_part_at` | 0x18a7c | - | **transcribed, never called** on these screens |
| `clone_part` | 0x059e4 | - | **transcribed, never called** on these screens |
| `angle_between_centres` | 0x03da5 | - | **transcribed, never called** on these screens |
| `queue_part` | 0x07b6f | - | **transcribed, never called** on these screens |
| `bounce_pair` | 0x03201 | - | **transcribed, never called** on these screens |
| `part_step_08f1` | 0x17bb1 | - | **transcribed, never called** on these screens |
| `part_drive_0802` | 0x17ac2 | - | **transcribed, never called** on these screens |
| `part_drive_2451` | 0x19711 | - | **transcribed, never called** on these screens |
| `collect_carried` | 0x03972 | - | **transcribed, never called** on these screens |
| `add_carried_weight` | 0x07c3a | - | **transcribed, never called** on these screens |
| `add_mass_capped` | 0x07c5b | - | **transcribed, never called** on these screens |
| `carry_riders_along` | 0x03a8d | - | **transcribed, never called** on these screens |
| `step_moving_object` | 0x01216 | - | **transcribed, never called** on these screens |
| `bounce_off_contact` | 0x03046 | - | **transcribed, never called** on these screens |
| `replay_shapes` | 0x06699 | - | **transcribed, never called** on these screens |
| `mark_part_shapes` | 0x0647f | - | **transcribed, never called** on these screens |
| `part_moved` | 0x06d8e | - | **transcribed, never called** on these screens |
| `mark_needs_refile` | 0x058f3 | - | **transcribed, never called** on these screens |
| `mark_joined_shapes` | 0x05e70 | - | **transcribed, never called** on these screens |
| `step_machine` | 0x00f86 | - | **transcribed, never called** on these screens |
| `add_record_shapes` | 0x0642a | - | **transcribed, never called** on these screens |
| `recompute_kind_physics` | 0x02ac0 | - | **transcribed, never called** on these screens |
| `reset_input_state` | 0x0b4f1 | - | **transcribed, never called** on these screens |
| `compute_link_endpoints` | 0x04e65 | - | **transcribed, never called** on these screens |
| `find_entry_for_pointer` | 0x098e0 | 0, 1, 4 | agreed |
| `erase_both_pages` | 0x080e7 | 0 | agreed |
| `erase_object` | 0x0ad51 | 0 | agreed |
| `restage_object_rect` | 0x0aef6 | 0 | agreed |
| `claim_buffer_slot` | 0x0b5ed | 0 | agreed |
| `clear_slot_5734` | 0x0b69c | - | **transcribed, never called** on these screens |
| `seek_file_to` | 0x09b38 | 0, 2 | agreed |
| `archive_entry_for` | 0x09b7c | 0, 1, 4 | agreed |
| `clear_flag_2d44` | 0x0a7a3 | 0 (missed 1, 4) | **not verified** |
| `clear_flag_2d44_thunk` | 0x0811b | 0 (missed 1, 4) | **not verified** |
| `resource_advance` | 0x1c8a7 | 0, 1, 4 | agreed |
| `select_resource` | 0x1c649 | 0, 1, 4 | agreed |
| `stdio_fgetc` | 0x0d404 | 0, 1, 4 | agreed |
| `buffered_read` | 0x0d0ed | 0, 1, 4 | agreed |
| `stdio_getc` | 0x0d3ef | 0, 1, 4 | agreed |
| `stdio_fread` | 0x0d1c4 | 0, 1, 4 | agreed |
| `refill_stream` | 0x0d396 | 0, 1, 4 | agreed |
| `read_translated` | 0x0da6d | 0, 1, 4 | agreed |
| `dos_read` | 0x0c185 | 0, 1, 4 | agreed |
| `dos_lseek` | 0x0c0c3 | 0, 1, 4 | agreed |
| `heap_malloc` | 0x0c999 | 0, 1 | agreed |
| `heap_free` | 0x0c8ca | 0, 1 | agreed |
| `dos_free_far` | 0x21b34 | 0, 1, 4 | agreed |
| `refresh_link_geometry` | 0x04f7f | - | **transcribed, never called** on these screens |
| `set_vector_from_angle` | 0x07223 | - | **transcribed, never called** on these screens |
| `link_slack` | 0x0713d | - | **transcribed, never called** on these screens |
| `link_endpoint_gap` | 0x07947 | - | **transcribed, never called** on these screens |
| `link_end_distance` | 0x06f8e | - | **transcribed, never called** on these screens |
| `shift_all_histories` | 0x07ca2 | - | **transcribed, never called** on these screens |
| `shift_state_history` | 0x07ce3 | - | **transcribed, never called** on these screens |
| `compare_link_ends` | 0x06de9 | - | **transcribed, never called** on these screens |
| `intersect_segments` | 0x03ba9 | - | **transcribed, never called** on these screens |
| `frame_pending` | 0x0b4e2 | - | **transcribed, never called** on these screens |
| `decode_position` | 0x1e561 | - | **transcribed, not verifiable**: it has no return to detect - the compiler replaced its `ret` with `jmp 0x1e89c`, so 0x1e7f2 jumps in and it jumps back. Covered by decompress_lzss, which runs it on every one of its 226 verified calls. |
| `wait_and_latch_frame` | 0x0aaca | - | **transcribed, not verifiable**: waits for an interrupt the harness must suppress |
| `update_button_state` | 0x08136 | - | **transcribed, not verifiable**: calls wait_and_latch_frame, which waits for an interrupt |
| `mouse_set_speed` | 0x0b859 | - | **transcribed, not verifiable**: INT 33h and nothing else - it leaves no trace in guest memory for the two runs to disagree about |

*423 transcribed, 181 verified. Written by `tools/verify.py --all`, not by hand - one run of the original captures every call.*
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

### Calling conventions found so far

Not everything is cdecl, and getting this wrong reads a return address as an
argument:

- **far cdecl** - the common case; arguments from `[bp+6]`.
- **near cdecl** - `ret`, not `retf`; arguments from `[bp+4]`.
- **register** - the driver's blitters, and some helpers, take arguments in
  registers and one takes the *carry flag* as a direction.
- **pascal** - `ret 2`: the callee clears its own argument. So far only in
  Borland's runtime, never in the game's own code, which is itself a signal
  when classifying a routine.
- **no frame at all** - `mov bx, sp` and index off that, as sine and cosine do.

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

### The C runtime occupies the top of segment 0000

Routines from **0x0bd00 to the end of the segment at 0x0dff0** are Borland's,
and are skipped as a block rather than read one at a time. This is a claim
about the module layout and it is kept separate from the routines that were
actually read, because they are different kinds of knowledge:

- 23 in that range have been read individually and every one is Borland's -
  stdio, `malloc`, long arithmetic, `errno`, file I/O;
- 19 below the line have been read and every one is the game's; the only
  runtime routine below it is the stderr write at 0x00274, in the start-up
  module at the very bottom;
- the linker lays each module down whole and in link order, and the runtime
  links last, so a contiguous run at one end is what a runtime cluster is.

Below that block sit a few **far wrappers** - push the arguments back, call the
near runtime routine, return - which are recognised structurally, by their
whole body being argument forwarding around exactly one call into the block,
rather than by address.

Anything in there that later proves to be the game's own is a retraction to
record, not a surprise to absorb quietly.

### DOS allocation is primed, not simulated

`dos_alloc_bytes` (0x21abd) calls DOS for memory, and the port has no DOS and
no arena, so it cannot decide where a block goes. Rather than mark the routine
unverifiable - it has twenty-two callers - the harness records what DOS
answered during the original's own call and **primes** the port with it. Every
part of the routine except the address itself is then genuinely compared: the
32-bit size arithmetic, the round-up, the "how much is free" path, and the
zero fill.

Asked for an allocation nothing has primed, the port aborts rather than
inventing an address.

### Not yet settled: routines that call Borland's allocator

`0x1c705` is "free this if it is not null". The port models the guest's memory
and the verifier compares all of it, so a routine that calls `malloc` or `free`
cannot agree unless the port moves the same heap bytes - which would mean
transcribing Borland's allocator after all, or excluding the heap from the
comparison. Neither has been decided, so such routines are left alone rather
than transcribed into a check that cannot pass.

### Bugs in the original, transcribed as they behave

Two so far, both in the same family and both left as they are:

- `far_memcpy` (0x222c6) aligns its destination with `test di,1 / jae`, and
  `test` always clears carry, so the branch is **always** taken and the
  aligning byte is never copied.
- `far_memset` (0x22300) does the same job with `or di,di / jp`, and `jp` is
  jump-if-**parity**: it stores the aligning byte according to how many bits
  are set in the low byte of the address, which has nothing to do with whether
  the address is even.

Both were presumably meant to be a test of bit 0. Neither is corrected: the
port reproduces what the original does, and the reasoning is in the source next
to the code.

### Retractions and near-misses

- **2026-08-28. The long divide was recorded as a comparison.** 0x0bd90 and its
  three siblings were classified as long comparison helpers on the strength of
  their shape - a family of entries loading a small constant into CX and
  jumping to one body. The body turned out to take two 32-bit arguments, keep
  the selector in DI, test its bit 0 for signedness and negate the operands by
  sign: it divides. Found when 0x02ac0 was seen calling 0x0bd90 to divide.
  Nothing downstream changed - it is runtime either way - but the description
  in `docs/runtime.md` was wrong and is corrected there.

- **2026-08-28. `find_free_slot_4bc4` was not the game's.** It was transcribed
  from 0x0d0a3 as a free-slot scan over 16-byte records and verified. It is
  Borland's: those records are `FILE` structures, the signed byte at +4 is the
  file descriptor, and the count beside it at DGROUP 0x4d04 is the stream
  count - the table's neighbour at 0x4d06 is the handle-flags table already
  identified. Removed from the port. It was the only apparent game routine in
  the runtime block, and it turned out not to be one, which is part of why the
  block claim above stands.

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

### Three outcomes that are not "verified"

The sweep distinguishes them, because collapsing any of them into a pass would
be the whole failure this project exists to avoid:

- **transcribed, never called** - the routine exists in C and nothing on these
  two screens reaches it, so nothing has been checked. `reset_input_state`
  (0x0b4f1) is the first.
- **transcribed, not verifiable** - the harness cannot run it, for a reason it
  names.
- **not verified** - it was checked and it differed, or an occurrence that was
  asked for was never reached.

### Routines the harness cannot verify

`wait_and_latch_frame` (0x0aaca) is **transcribed and not verified**, and the
sweep reports it that way rather than counting it as agreeing.

Its whole purpose is to wait for the INT 08h handler to set a flag. The harness
suppresses interrupts while a routine is open, so that an interrupt's own
hardware writes are not attributed to the routine - and with them suppressed
the original's spin can never be released, so the emulator sits in it forever.
Verifying it would need the harness to *distinguish* an interrupt's effects
from the routine's rather than excluding them, which it cannot do today.

The port's own side of that wait is IO: the loop body calls
`io_await_frame_tick`, the port's stand-in for the handler, because an empty
spin in C could never exit. That is marked as ours in `io.c`.

`update_button_state` (0x08136) inherits the same limit, because it calls that
routine. Anything that waits for an interrupt, directly or through a callee,
falls in this class.

**The harness now has a watchdog** so this can never hang a whole sweep again:
an instance still open after 30M instructions is abandoned, reported by name
and occurrence, and counted as not verified. Finding the first such routine
cost a run that never finished.

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
- **A routine's own arguments are excluded too.** Several here modify them in
  place - `far_memset` walks its 32-bit count down with `sub`/`sbb` - and in
  cdecl the caller pops them, so those writes cannot be observed by anyone.
  The port's arguments live in its own frame.
- The stack is **inside** the compared segment - SS is DGROUP in this program -
  so the bytes a call used as stack are excluded, bounded by the lowest SP the
  call reached. The port has its own C stack and cannot reproduce them.
- Registering memory hooks across all of memory **derails the guest** - it
  opened a file with a garbage name and then executed an invalid instruction.
  They are range-limited to the VGA aperture.

### The port is POSIX-only, and that is not deliberate

`reconstruct/io.c` models the PC's timer interrupt with a thread, and it uses
**pthreads** - so every target links `-lpthread` and none of them builds on
Windows.

The reason it is not SDL's `SDL_CreateThread` is the layering, and it is a real
one: `io.c` is in `$(PORT)`, which goes into all three binaries, and `devtim`
and `libtim.so` link **no SDL at all** - 0 undefined `SDL_` symbols against
`tim`'s 12. `libtim.so` is what `tools/verify.py` loads to call a transcribed
routine and compare its hardware trace with the original's, and it must not
need a window to do that.

So the trade was made in favour of the layering, and the portability cost is
real and unpaid. If a Windows build is ever wanted the answer is a three-call
shim inside `io.c` - create, lock, unlock - and not reaching up into SDL from
the timing layer.

### Two entries on the worklist that are not work

The code map finds these by recursive descent and the worklist offers them as
untranscribed. Both are recorded here rather than left to be rediscovered.

- **0x2277c** forces the BIOS equipment word to 80x25 colour and sets text mode
  3. All five of its callers are inside `detect_adapter`, on the branches the
  port already stubs as unreached - `0x22612`, `0x2263a`, `0x2264b`, `0x22682`
  and `0x226ab`, the paths for adapters that are not the VGA. It is a leaf of a
  deliberate non-goal, and transcribing it would add a routine nothing can
  reach.
- **0x10160** has no prologue and no caller. Searching the whole of its segment
  for a near call to it finds none, and it starts mid-flow with a bare
  `push ax`. It is a jump target inside another routine that the descent has
  taken for an entry - so a "function" written for it would be one the original
  does not have.

The lesson for the queue generally: `tools/worklist.py` lists what recursive
descent believes are entry points, and in hand-written assembly that belief is
sometimes wrong. Check for a prologue and a caller before treating an entry as
a routine.

### Known gaps, not argued away

- **The 8x8 font pointer is the reference's, not a real BIOS's.** `vm_init`
  asks `INT 10h AX=1130 BH=3` for it, and the emulator does not implement that
  call - ES and BP come back exactly as they went in, so the game stores a
  "font" pointing into its own stack at DGROUP 0x618a. The port reproduces
  that, because the emulator is what correct means here. On a real machine
  those four words would hold a real font address, and anything that draws
  through them would differ. Nothing on these screens reads them, but that is
  measured for these screens only.

- **The emulated instruction rate is a guess.** `drive.DEFAULT_IPS` is
  2,000,000, chosen and not measured. It sets the frame rate the guest believes
  it is achieving, so no timing claim can be made until it is measured against
  the original in cycles.
- ~~**Sound is not modelled.**~~ It is now: the whole module, the PC-speaker
  driver behind it, and the loading path under both.
- **`VM.OVL`'s other seven drivers** are never executed and never will be.
- ~~The **name-hash** in `RESOURCE.MAP` is not derived.~~ `hash_filename` at
  0x0980d is transcribed and verified, and the four byte offsets it packs are
  **read out of `RESOURCE.MAP` itself**, so the hash is defined by the file
  rather than by the program.

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

## Borland's own allocator

`reconstruct/borland_heap.c`. Not the game, and not what this port is
reconstructing - but game routines call `free` and `malloc`, and the
whole-memory comparison cannot pass them unless the port moves the same heap
bytes. So it is transcribed, in a file of its own.

**It is kept, not deleted.** This is Borland's allocator rather than this game's,
so having it transcribed and checked against a real binary is worth something to
anyone taking apart another Turbo C or Borland C++ DOS program. Whether this
port links it is a separate question from whether it exists.

Read from the disassembly and confirmed against the published description of the
near heap. A block header is four bytes below the caller's pointer - size at +0
with **bit 0 as the in-use flag**, previous-block-by-address at +2 - and a free
block reuses its own first four payload bytes as a doubly linked ring at +4 and
+6, which is why the smallest block is eight. `0x4e34` is the first block,
`0x4e36` the topmost, `0x4e38` the ring cursor, `0x9c` is `__brklvl` and `0x94`
`errno`.

Done and verified: `heap_free` and its four helpers, at 379 calls. `malloc`
(0x0c999) and its own helpers are not transcribed yet.

## The file layer

The port has none, and two transcribed routines are limited by that.

`seek_file_to` (0x09b38) **is** verified, but only at occurrences that take its
cached path. `io_file_seek` is a stand-in whose limit was measured rather than
assumed: a no-op was tried, and an occurrence that seeks a long way then showed
four bytes differing at DGROUP 0x4c14 - the runtime's own `FILE` buffer, which
its `fseek` resets.

`make_file_current` (0x09a62) is transcribed and **not** verified. Every
occurrence sampled reaches `fopen`, which refuses rather than inventing a
`FILE` for everything above it to read through.

`io.c` now serves DOS file reads and seeks **read-only from the game
directory**, and `borland_file.c` has the runtime's `read` (0x0c185) and `lseek`
(0x0c0c3) over them - same standing as `borland_heap.c`, kept for reference.
Handles are numbered from 5, as DOS does once the five standard ones are taken,
because the guest stores the number it is given and the comparison sees it.

Both **are** verified. A handle and a file position are not in guest memory, so
seeding memory was never enough on its own - but the emulator knows both.
`TimMachine._dos` tracks INT 21h AH=3Dh, 3Eh, 3Fh and 42h into a handle-to-name
map, `tools/verify.py` captures it at each instance, and `io_prime_file` reopens
the same file at the same offset. The same remedy as `io_prime_dos_alloc`, and
in the same place.

That was the last thing blocking the loading path, and with it the sound-module
routines that load and decompress. All of it is transcribed and verified now.

### The chain, as measured

Everything below is read from the disassembly, not guessed, and each step was
confirmed by hooking the running game:

```
sound routines 0x28bf2 0x28cf7 0x28e87 0x28f74 0x289e8 0x29da0 0x296b4
                                                        [all verified]
  -> 0x1d868, 0x1d983
    -> 0x1c92b            dispatch through the table at DGROUP 0x3580
      -> 0x1c278  type 1  (1 call)     [verified]
      -> 0x1ca62  type 2  (12 calls)   helper  0x1cc65
      -> 0x1e7f2  type 3  (226 calls)  helpers 0x1e0b3 0x1c5a3
         input   0x1c389  next byte      [verified, 1,471 calls]
                 0x1c319  run into huge  [verified, 119 calls]
         output  0x1c493  literal run    [verified, 119 calls]
                 0x1c51e  fill run       [verified, 129 calls]
                 0x1c5a3  one byte       [verified, 2,500 calls]
        -> 0x091ef  fread wrapper   [verified, 7,597 calls]
          -> 0x09a62  open      18,930 calls, 26 reach DOS   [transcribed]
          -> 0x09b38  seek      18,930 calls, 319 reach DOS  [verified]
          -> 0x09b7c  archive?  10,454 calls                 [verified]
          -> 0x0d1c4  runtime fread
             -> 0x0d0ed  buffered read
                -> 0x0d3ef  getc   [verified]
                   -> 0x0d404  fgetc  [verified]
                      -> 0x0d396  refill [verified]
                         -> 0x0d36d  flush all streams [verified]
                            0x0da6d  translating read  [verified]
                            -> 0x0c185  read   [verified, 441 calls]
                               0x0c0c3  lseek  [verified, 472 calls]
```

**The handler table was measured, not read off.** Hooking the indirect call at
0x1c94c gives exactly three live entries - index 1 to 0x1c278, 2 to 0x1ca62 and
3 to 0x1e7f2 - which is how their call counts above are known.

The two decompressors left, 0x1ca62 and 0x1e7f2, are hand-written assembly that
switches DS and keeps its state in registers across jumps; they are the largest
single piece of work remaining on this chain.

That whole stdio column is now transcribed and verified: `0x0d1c4` (`fread`),
`0x0d0ed` (its buffered inner loop), `0x0d3ef` (`getc`), `0x0d404` (`fgetc`),
`0x0d396` (the refill), `0x0d36d` (the flush it calls first) and `0x0da6d` (the
translating read), bottoming out in the already-verified `0x0c185`/`0x0c0c3`.

Two routines named on that chain are *not* transcribed, and neither is reached:
`0x0cd9e` - a DOS IOCTL call, so `isatty` or `eof` - and `0x0ce92`, a stream
flush. Both hang off `fgetc`'s unbuffered branch, and every stream the game
reads has a 512-byte buffer, so that branch aborts rather than pretending.

After those: the `fopen`/`fclose` pair at `0x0d0ce`/`0x0ce15`, then `0x091ef`
and the three decompressors.

The handler table for 0x1c92b is at DGROUP 0x3580, fourteen bytes per entry with
the handler offset first; which entries are live was measured, not read off the
table.

**Even the archive path calls the runtime's `fread`.** It substitutes the
archive's own `FILE` and reads through the same buffered layer, so there is no
route through the loader that avoids stdio - which is why the file layer is not
optional. Priming file state in the harness is what made any of it checkable;
the per-routine path does not prime, so these routines are only meaningful
under `--all`.

## Deferred

- ~~**The sound module, segment 2619.**~~ **Done.** Every one of the 69
  routines the code map reaches in it is transcribed, and every one that runs on
  these screens agrees with the original. The reasoning below is kept because it
  records why it was set aside and what that was costing; it is history, not
  current policy.

- **The sound module, segment 2619.** Its routines call through a vector in
  their own code segment at `cs:[0x1e7]` and keep their tables beside it, and
  they are on the intro screens' execution path - but **not on the drawing
  path**: attributing every A000 write of nine frames to the instruction that
  made it found all of them in `VM.OVL`, reached from segments 0000 and 1c25.
  They cannot change a pixel, so they are deferred against the goal of matching
  the two screens.

  Measured, so that the cost of the deferral is on the record: it keeps three
  game routines permanently blocked. `0x083ab` (5 callers) is a thin dispatcher
  whose whole body calls `0x2619:0x38b9` = `0x29a49`; `0x03009` then waits on
  `0x083ab`. `0x29a49` itself walks a record list and would transcribe easily,
  but it calls `0x294ff`, `0x28935`, `0x29034` and `0x287ad`, all inside the
  module, so taking it means taking a large part of the module with it. That is
  the right trade against a pixel goal and the wrong one against a complete
  port; it is a scope decision, not an oversight.

  If a sound routine turns out to share state with the drawing
  code, that is a retraction to record.

- Matching (byte-exact) decompilation.
- The seven non-VGA drivers in `VM.OVL`.
- ~~Sound.~~ Done - see above.
- **The loaded sound module's own code.** `setup_sound_device` can load a block
  into DGROUP 0x4a98 and call into it through the dispatcher at 0x0bbd4. That
  block is not part of `TIM.EXE`; it would be a second overlay to transcribe,
  like `SX.OVL`. It never loads on these screens, and the three calls into it
  are stubs that say so.
- Anything past the intro screens: the menu, the puzzles, the level editor.
