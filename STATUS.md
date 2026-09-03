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

- **The machine the game saves is byte for byte the original's.** Two
  scenarios, both identical on the two sides:

      --scenario empty    16 bytes, edac 0201 4300 1001 e903 0000 0000 0000
      --scenario parts   740 bytes, a machine loaded and saved again

  The magic `0xaced` comes first, little-endian. `empty` is a header and two
  counts and no parts at all - a thin thing to call a proof of the writer, and
  it was the only one for a while. `parts` loads `CATOMATC.TIM` and saves it
  back, so all fifteen part records go through `sub_12430` and every field it
  writes is compared.

  **Saving a machine is not the identity.** The 740 bytes the game writes differ
  from the 740 it read in **280 places**, the first at offset 16 - the first
  word of the first record, 0x000f in the file and 0x0008 in the save. Both
  sides do it identically, so this is the game's own behaviour and not the
  port's.

  **The parts come back reversed.** Walking both files by the record flags -
  `sub_12430` writes four more bytes for a part with a rope and six more for one
  with a belt, so a fixed stride lands in the wrong field - the fifteen records
  from offset 0x10 read:

      loaded   15 39  2  5  2  2  2 50 21  1  1  3  8 | 4 12
      saved     8  3  1  1 21 50  2  2  2  5  2 39 15 | 4 12

  The first thirteen are **exactly the reverse** of each other, the last two are
  unchanged, and the multiset is the same. That is a list built by prepending
  each part as it is read and then walked head-first when it is written; the two
  after the bar are the second list `sub_126b3` writes, which keeps its order.

  The reversal is the evidence for the offset, not the other way round: several
  start offsets happen to walk to exactly 740 bytes, because the records vary in
  length and a wrong alignment can still sum correctly. Only 0x10 - which is
  where the 16-byte header ends, as the empty save shows - produces two
  sequences related like that.

      uv run python tools/check_save.py --scenario empty
      uv run python tools/check_save.py --scenario parts

  Neither side writes a real file: the port satisfies guest writes from an
  in-memory overlay and the emulator does the same, so running this leaves the
  game directory as it found it.

  **This check blamed the port twice, and the second time was 2026-09-01.**
  Both scenarios reported the original writing `CATOMATC.TIM` and the port
  writing no such file. The port was writing it perfectly.

  The wait for the save polls for a file to appear and stop growing - but
  `port.log`, this tool's own capture of the port's stderr, is created in the
  directory being polled *before* the loop starts. So the listing is never
  empty: the first pass totals 0 bytes, the second sees 0 again, calls that
  "written and no longer growing" and returns. The port was killed about a
  second after starting, having reached nothing, and was then reported as
  having written nothing - which was true, and was this tool's doing.

  It now ignores `port.log` when waiting and when comparing. With that, both
  scenarios pass: 16 bytes identical empty, 740 identical with parts.

  Watch what is being waited for, not the directory it happens to sit in. The
  paragraph below is the same lesson from the first time.

  **The first version of this tool accused the port of a fault it did not
  have.** It polled the emulator's handles once a slice, reasoning that a slice
  is 2000 instructions and a save must take longer. A sixteen-byte save does
  not: truncate, write and close fit inside two slices, so the one sample
  landed between the truncate and the write and reported the original as having
  written nothing. The emulator's own log said `WRITE +16` on the line above.
  The bytes are taken at the close now, which is the last moment they exist.

- **The picker, the writer, the puzzle screen and the typing are verified
  routine by routine** - the ones listed below, each checked on the same call
  inside one run of the original.

  There is deliberately **no count here**. This file already says the
  transcribed and verified totals must not be written by hand because they were
  wrong within one session when they were; a hand count of this list was wrong
  three times in one afternoon - 56, 58 and 59 - which is the same lesson
  arriving by the same door. The list is the record. `tools/verify.py --list`
  and the sweep table are what count things.

  They are reachable at all because `--click`, `--key` and `--game-dir` drive
  the original to them; before those, the whole menu reported "transcribed,
  never called", which is true of a run with no hands and says nothing about a
  routine. The number after a name is how many calls were compared where that
  is more than one.

      the writer   write_word 325, write_byte 90, part_index 62, sub_12430 15,
                   sub_126b3 3, sub_126ec 3, sub_1271c, save_machine
      the picker   picker_repaint, sub_13a8a, sub_13c78, picker_draw_list,
                   picker_draw_name, picker_draw_filename, picker_draw_up,
                   picker_draw_down, draw_sunken_box, validate_filename,
                   is_machine_file, listing_to_name
      the writes   sub_0d8ca 8, dos_write, write_text, dos_creat, dos_chdir
      the paths    path_join, path_is_root 3, path_up
      the puzzles  puzzle_repaint, puzzle_draw_list, puzzle_draw_password,
                   puzzle_draw_up, puzzle_draw_down, puzzle_draw_ok,
                   get_puzzle_title 21, puzzle_page_of_score
      the regions  region_cursor_gravity 748, region_cursor_freeform 2,
                   region_cursor_air 2, region_cursor_load, region_cursor_save
      the typing   sub_1156c, picker_tab, puzzle_tab, picker_type 199,
                   force_extension, draw_button
      the codes    password_to_level, string_upper, score_code_to_score,
                   parse_base 2, string_reverse 2, game_fread_line 4
      the leaves   string_chr 29, string_ncompare_i 11, mem_copy 5,
                   to_lower 4, string_length

  **The directory navigation needed a directory to navigate.** There is no
  subdirectory in the game's folder, so `path_join`, `path_is_root` and
  `path_up` were unreachable by any run against it - and `path_join` is the one
  with the deliberate off-by-one at *both* ends, stripping the `<` and the `>`
  the listing writes. `--game-dir` serves both sides from a copy with a
  directory in it, which reaches all three and leaves the game's own folder
  alone; `tools/fixture.py --out DIR` builds that copy - the subdirectory and a
  `password.txt` - so the result is reproducible rather than a directory
  somebody once made by hand in `/tmp`. It has to set `tools/tim.py`'s constant as well as the emulator's
  global, because `game_dir()` re-applies that constant every time a machine is
  made - setting only one is undone by the next `TimMachine`, silently.

  The two that need the fixture, in full, so they can be run again:

      uv run python tools/fixture.py --out /tmp/gd
      uv run python tools/snapshot.py --save-at-flip 690 --out /tmp/pre.snap \
          --click 200:320:200 --click 420:76:152 --click 560:222:220

      # the navigation: into SUBDIR and back out by <PARENT DIR>
      uv run python tools/verify.py --all --game-dir /tmp/gd \
          --from /tmp/pre.snap --budget 50000000 \
          --click 10:170:152 --click 200:100:128 --click 400:100:128 \
          --only path_join,path_is_root,path_up

      # the codes: leave freeform, YES, the password field, type "A-00000"
      uv run python tools/verify.py --all --game-dir /tmp/gd \
          --from /tmp/pre.snap --budget 90000000 \
          --click 10:124:148 --click 150:222:220 --click 400:260:324 \
          --key 440:0x1e:0x41 --key 480:0x0c:0x2d --key 520:0x0b:0x30 \
          --key 560:0x0b:0x30 --key 600:0x0b:0x30 --key 640:0x0b:0x30 \
          --key 680:0x0b:0x30 --key 720:0x1c:0x0d \
          --only password_to_level,score_code_to_score,parse_base,\
string_reverse,game_fread_line

  The snapshot is at flip 690 because that is inside freeform with the panel up,
  which is where both sequences start; `snapshot.py --click` is what gets there.

  **The score code needed a password file, and the fixture can have one.**
  The game's folder has `CODES.TXT` and no `PASSWORD.TXT`, so
  `password_to_level` always failed its open and answered -1 - and everything
  behind a password that was *found* was unreachable: the base-34 parse, the
  reversal it does in place, and the checksum that multiplies the score by the
  first three characters of the password. A `password.txt` in the copy makes
  the lookup succeed and all five verify.

  **`--key` is what reaches the last of those.** A keypress at a page flip,
  beside `--click` and counted the same way; without it the two text fields,
  the password and all three Tab handlers were unreachable.

  **Four routines are unverifiable and say so.** `ask_yes_no` and `message_box`
  wait for the player, and the harness stops the timer and the keyboard while a
  routine is open - nothing can arrive to end the wait, and the watchdog
  abandons them after 30M instructions. `game_screen`'s ten handlers are jump
  targets rather than routines: the table dispatches with `jmp`, a handler runs
  on `game_screen`'s frame and ends by jumping back to 0x1145b, so there is no
  call to stop at and no return to detect. All of them are covered by the
  screen comparisons instead.

  **Two of them found faults the screens could not.** `sub_13a8a` differed in
  33 places: the port was not filling the DTA at all, and it was clearing the
  find buffer before a call, so a *failed* find published a blank where DOS
  leaves the last name it found. Neither reaches a pixel.

  A third difference was the check's own. `picker_repaint` reported 9835
  differences with the memory agreeing on every byte: it copies through the
  VGA's latches, and in write mode 1 the byte written is ignored. Its spec
  needed `planes=True` to seed the Graphics Controller and the map mask.

- **The machine writer is verified against the original, routine by routine.**
  Driven through a load and then a save, `tools/verify.py` compares each of the
  writer's routines to the original body on the same call inside one run:

      write_word    325 calls    verified
      write_byte     90 calls    verified
      part_index     62 calls    verified
      sub_12430      15 calls    verified
      sub_126b3       3 calls    verified
      sub_126ec       3 calls    verified

  That is the check the screen comparisons cannot make. A machine file never
  reaches a pixel, so the port could get every field of it wrong and still draw
  the same panel afterwards.

      uv run python tools/verify.py --all --from <snap> --budget 140000000 \
          --click 10:170:152 --click 150:100:128 --click 290:88:312 \
          --click 450:220:152 --click 590:100:128 --click 730:88:312 \
          --click 870:222:220 \
          --only part_index,write_byte,write_word,sub_12430,sub_126ec,sub_126b3

  `write_string` is transcribed and was never called: nothing on this path
  writes a string.

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

- **What pixel-exactness does not say.** `tools/reached.py` over flips 0 to 230
  finds **339 routine entries on the briefing path; 332 map to a transcribed
  routine, and 57 of those have no verifier spec** - their whole evidence is
  that the screen came out right. That is weaker than it sounds. A routine can
  produce a correct screen and still get its hardware events wrong, which is
  exactly what `picker_repaint` did until its spec was given `planes=True`; and
  `long_multiply` runs **6,477 times** on this path, having never been checked
  at all. A signed shift and a logical one agree on every positive value a
  screen happens to contain.

  **That gap is now closed.** All 332 are specced, verified, or carry a written
  reason why they cannot be checked in isolation - nothing is silently absent.
  Reproduce the measurement with

      uv run python tools/reached.py --from-flip 0 --to-flip 230 --json out/b.json

  and intersect `used` with the spec names in `tools/verify.py`; it answers 0
  unaccounted for. What the work found, in the order it was done:

  - **`load_screen_plain` called the wrong routine.** 0x23b3c is `push cs /
    call 0x1e94c` - `restore_write_mode` - and the port called
    `vm_reset_attributes`. Both are "put the VGA back", and the difference is
    invisible: resetting the attribute controller to the identity palette it
    already holds changes no pixel, so the briefing matched at 0 of 307,200 on
    every flip while every call did the wrong thing. The verifier saw it on the
    first event of the first call.
  - **`part_inits` had lost a column, and four part kinds got zeros.** Ten of
    the forty-three init hooks store values the flag columns could not express,
    at seven offsets and in mixed widths. `make_part` returned the right value,
    changed 188 of 190 bytes identically and drew a pixel-exact briefing while
    writing zeros into fields the original fills. Each row now carries an
    ordered (offset, width, value) list, and forty of forty occurrences agree
    except one - whose missing byte comes from `part_setup` in `seg172c.c`,
    where six of the forty setup routines write +0x6a and +0x6b and the port
    writes neither. That one is recorded in the file, with the table addresses,
    rather than guessed at.

    The scanner that built the list was itself wrong three times - a short
    disassembly window, a stale AX carried across `xor ax, ax`, and a
    `mov [si+X], al` form it did not recognise. The second wrote two wrong
    values that the port then stored faithfully. Only the third is a lesson:
    an unknown *value* was reported, while an unrecognised *store form*
    vanished in silence. It now matches on the destination first and complains
    about any source it cannot value.

  - **Three part kinds were built by the original and thrown away by the
    port.** Three of the forty-three init hooks - 0x147a7, 0x148e0, 0x148ff -
    or their flags, call their setup and answer 0 without ever allocating the
    +0x82 array. `part_init` allocated for all forty-three, and
    `heap_calloc_far` of a zero count answers 0, which it read as failure; so
    `make_part` freed the part and returned 0 where the original returns a
    record. That is the largest defect this session found and it is not a wrong
    value anywhere - it is three parts missing from the machine, with the
    briefing pixel-exact throughout because none of them is drawn on it.

    It surfaced as a **return** difference with the heap 0xa6 lower, and it was
    the reason `build_part_list` differed in 2,109 bytes: every later allocation
    inherited the offset. With the parts kept, and two more missing stores in
    `setup 0x2cce`, both routines verify - `make_part` across 40 occurrences.

  - **Nine routines could not be checked at all**, because someone had written
    `static` in front of them. Nothing in a binary records C linkage, so it
    carried no fact from the original; what it carried was absence from
    `libtim.so`. Eight polygon routines and one helper. `tests/provenance.py`
    now fails a transcription that is static.
  - **Three routines are `ljmp [vector]` and one is the whole program.** Those
    are registered with a reason rather than left out, beside the ten
    `screen_state_*` jump targets.
  - **One caller of `sub_0e34a` is settled.** `game_main` calls it
    unconditionally after `game_play` returns, so it is the teardown and fires
    on any run that exits. Two earlier explanations here were wrong; this one
    is read off the call site.

  Four conventions had to be read rather than assumed, and each would have
  produced a spec that failed and read as a broken transcription: the two
  copies of `__LMUL` end `ret` and `retf` respectively; the allocator uses four
  different argument conventions across ten routines; `poly_edge_vertical`
  disagrees with the four other edge routines about which register holds which
  end; and `set_cursor` takes its hot spot **y before x**.

- **The level-one briefing is reached and is pixel-exact.** The port runs the
  intro, a click, the copy-protection screen and the whole briefing paint
  without hitting a stub - and that is now measured on **both** sides rather
  than inferred from the port not aborting: intersecting the twelve stub
  addresses with the entry addresses `reached.py` records for flips 0 to 230
  gives zero, so the original does not enter any of them on this path either. A
  port whose control flow diverged around a stub would satisfy the first test
  and fail this one. Frame 1200 of it differs from the original in
  **0 of 307,200 pixels** - frame, panel, sliders, odometers, title bar,
  description, every scaled part in the play area, and the mouse pointer.
  Re-check it with `uv run python tools/check_briefing.py`, which runs both
  sides and compares three settled flips - 210, 230 and 260 - in 28 seconds,
  saying for each how many pixels differed. By hand, for one flip:

      uv run python tools/capture.py --click 200:320:200 --flip 260 \
          --insns 150000000 --out out/ref --no-png
      SDL_VIDEODRIVER=dummy TIM_CLICK=200:320:200 TIM_FLIPWANT=260 \
          TIM_FLIPS=out/portframes:260 ./reconstruct/devtim
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

### The stubs that are left

`make -C reconstruct test` lists them. **Every one has a real caller** - that
was audited by grepping for each name, after a claim here that `sub_0e34a`
might be unreachable turned out to have followed one of its four callers and
missed the other three. So none of them is dead code, and finishing them is
work rather than bookkeeping.

Two things are dead, and they are not stubs: `picker_set_name` and
`picker_name` are transcribed and have no caller at all - no near call, no far
call, and their far pointers are stored nowhere, which is the third search
because a routine can be reached through a table the way the region handlers
are.

"Unreached" and "unreachable" are different words here and the difference is
always the run. `draw_offset_bitmap`, `fill_quadrant` and `fill_screen_quadrant`
have callers and have simply never been driven.

`sub_0e34a` is the cautionary one. It was written up here as unreachable, on the
strength of one of its four callers; then as reached by clicking the
copy-protection screen's corner, on the strength of an unguarded coordinate test
at another. Driving a click into that rectangle does not reach it.

The third account is the first that explains the observation rather than
contradicting it, and it came from reading the *caller* instead of the routine.
`game_main` at 0x0dfff is nineteen instructions - `game_startup`, `game_intro`,
`game_play`, then `push 1` and this - with **no test in front of the call**. So
it is the teardown: it runs on every run that ends, and nothing here has ever
exited the game. That is why no run has reached it, and it predicts that one
which quits normally will, with no clicking at all. The other three callers stay
conditional and unestablished.

### How much is transcribed

`make -C reconstruct test` counts the port. **Run it rather than reading a
number here**: the last one written down was `ours 17` and it was `ours 18`
within the hour, because `dta_publish` was added and nobody edits a count in
prose when they add a function. That is the same reason this file says the
verified totals must not be hand-written, and it applies to this one too.

The command is authoritative for the port and says nothing about the game.

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

**That was measured a fourth time, and the fourth measurement disagrees with
the third.** On an idle machine:

    5 routines,   8M instructions    11.5 seconds   ~700k instructions/second
    60 routines,  8M instructions    14.7 seconds   ~540k instructions/second
    60 routines,  40M instructions   48 seconds     ~830k instructions/second

So the number of routines costs something - twelve times as many is about 27%
more time - and it does **not** cost sixty times.

**The rate is not steady, and estimates made from single samples of it were
wrong in both directions.** The sweep was sampled at 444k instructions/second
two minutes in and 782k three minutes in; extrapolating the first gave "an hour
and a half", extrapolating a 60-routine run gave "under an hour". Neither is a
measurement of the thing.

**A complete sweep has now been run end to end**: 482 routines, the default
budget of 2.6 billion instructions, **a little under fifty minutes** and 48
minutes of CPU on an idle twelve-core machine. That is the fifth number written
into this paragraph and the first that is not an inference.

`verify.py --all` now prints its own elapsed time, so the sixth will not have to
be watched for with `ps`. A narrowed sweep is still minutes, which is why every
per-routine check in this session used `--only`.

**What makes it fifty minutes is nine routines.** 83 specs carry an explicit
budget and nine of them ask for the full 2.6 billion - `split_part_at`,
`clone_part`, `draw_machine`, `part_step_1649` and the rest of that family -
because they are only reached with a machine actually running. `collect_all`
makes one pass at the largest budget any wanted routine asks for, so those nine
set the length of every full sweep.

Capping it with `--budget` is therefore tempting and is a **footgun**: the deep
routines then report "transcribed, never called", which is indistinguishable
from a routine nothing calls. That already happened at the other end of the
scale - `--only` defaults to 40M, the polygon filler is not reached until past
90M, and three routines `reached.py` had already shown to run came back never
called. Cap the budget only with `--only`, where the routines asked about are
known to be reached early.

The earlier figure of "~17k instructions/second" is retired. This file already
records that two figures before it were taken on a machine with forgotten runs
of the same tool competing for it; the third appears to have been as well.

**And the suspected cause was wrong.** The per-occurrence snapshot was blamed:
each captured call copies the 640 KB below the video aperture, and that was
inferred to be a gigabyte of copying. Measured, `uc.mem_read(0, 0xA0000)` takes
**0.062 ms**, so two thousand captures in and out come to a quarter of a second.
It is not the copying. What is left is the per-instruction hook and the event
recording into every open instance, which is where the routine count shows up -
but that is now a small effect on a measured baseline rather than a mystery.


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

**The date is doing real work here.** Around seventy routines went in the day
after it - the picker, the machine writer, the puzzle screen and the runtime's
string and file layer - so the byte counts below understate what is transcribed
now. They are left rather than adjusted, because a figure edited by hand stops
being a measurement; re-running `tools/codemap.py --run` and
`tools/coverage.py` is what makes them current.

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

**The table below is regenerated in place**, so nothing hand-written survives
between its markers - a note put there is deleted by the next sweep. Two things
that belong beside it therefore live here instead.

**It is what a sweep with no hands reaches.** `--all` drives the original from
the entry point and presses nothing, so every routine behind the menu comes back
"transcribed, never called" - true of the run and silent about the routine.
`--click`, `--key` and `--game-dir` reach those, and the list further up is what
they have proved; `--all` accepts all three, so a *driven* sweep could fill in
much of what the table calls unreached.

**A narrowed sweep is minutes and a full one is not.** `--only` does not pay the
whole-sweep hook on every instruction, which is why every check in this session
used it.

<!-- VERIFY:BEGIN -->
| routine | address | occurrences checked | result |
| --- | --- | --- | --- |
| `vm_set_display_lines` | 0x08f77 | 0, 1 | agreed |
| `vm_save_rect` | VM.OVL VGA:0x12fb | 0 | agreed |
| `vm_restore_rect` | VM.OVL VGA:0x13b9 | 0 | agreed |
| `atan2_long` | 0x2d296 | 0 | agreed |
| `link_nearby_objects` | 0x03566 | 0 | agreed |
| `find_edge_contact_reversed` | 0x00b6c | 0 | agreed |
| `resolve_collisions` | 0x00556 | 0 | agreed |
| `find_edge_contact` | 0x007af | 0 | agreed |
| `integrate_object` | 0x02c93 | 0 | agreed |
| `place_object_for_draw` | 0x05be4 | 0 | agreed |
| `add_sub_object_shapes` | 0x05ef6 | - | **transcribed, never called** on these screens |
| `set_object_extent` | 0x05c77 | 0 | agreed |
| `object_delta_angle` | 0x004ab | 0 | agreed |
| `arctan_lookup` | 0x2a941 | 0 | agreed |
| `apply_contact_friction` | 0x02da0 | 0 | agreed |
| `vm_read_pixel` | VM.OVL VGA:0x1453 | - | **transcribed, never called** on these screens |
| `read_pixel_clipped` | 0x2241b | - | **transcribed, never called** on these screens |
| `vm_plot_pixel` | VM.OVL VGA:0x14c9 | - | **transcribed, never called** on these screens |
| `plot_pixel_clipped` | 0x2244d | - | **transcribed, never called** on these screens |
| `init_sequence_params` | 0x28305 | 0, 1 | agreed |
| `next_matching_record` | 0x29966 | 0, 1, 4 | agreed |
| `midi_bend_event` | 0x280fe | 0, 1 | agreed |
| `step_sequence` | 0x27c4e | 0, 1 | agreed |
| `midi_note_off_event` | 0x27e92 | - | **transcribed, never called** on these screens |
| `midi_event_6` | 0x27f54 | - | **transcribed, never called** on these screens |
| `midi_meta_event` | 0x2817e | 0, 1 | agreed |
| `midi_skip_event` | 0x2817a | - | **transcribed, never called** on these screens |
| `skip_unknown_event` | 0x2828e | - | **transcribed, never called** on these screens |
| `midi_controller_event` | 0x27f85 | 0, 1 | agreed |
| `midi_program_event` | 0x28086 | 0 | agreed |
| `midi_event_9` | 0x280da | - | **transcribed, never called** on these screens |
| `midi_note_event` | 0x27ee1 | 0, 1 | agreed |
| `free_node_list` | 0x28baf | 0, 1 | agreed |
| `create_sequence` | 0x28935 | 0 | agreed |
| `free_for_kind` | 0x2a017 | 0, 1 | agreed |
| `alloc_for_kind` | 0x29f89 | 0, 1 | agreed |
| `start_sequence_far` | 0x28480 | 0 | agreed |
| `load_and_start_sequence` | 0x29034 | 0 | agreed |
| `start_sequence` | 0x26783 | 0 | agreed |
| `advance_volume_ramp` | 0x278e9 | - | **transcribed, never called** on these screens |
| `set_sequence_volume` | 0x279a9 | - | **transcribed, never called** on these screens |
| `sound_service` | 0x27ace | 0, 1 | agreed |
| `drop_unless_polled` | 0x27b52 | - | **transcribed, never called** on these screens |
| `poll_sequences` | 0x27b7e | 0, 1 | agreed |
| `remove_sequence` | 0x26e7b | 0, 1 | agreed |
| `sound_callback` | 0x292a1 | - | **transcribed, never called** on these screens |
| `sequencer_tick` | 0x26f2a | 0, 1 | agreed |
| `install_driver` | 0x265f2 | 0 | agreed |
| `configure_driver` | 0x26629 | 0 | agreed |
| `silence_driver` | 0x2664e | - | **transcribed, never called** on these screens |
| `set_master_level` | 0x26721 | 0 | agreed |
| `retire_and_tick` | 0x26a57 | 0, 1 | agreed |
| `set_master_level_far` | 0x28431 | 0 | agreed |
| `install_driver_far` | 0x28458 | 0 | agreed |
| `configure_driver_far` | 0x2846a | 0 | agreed |
| `retire_and_tick_far` | 0x284ef | 0, 1 | agreed |
| `silence_driver_far` | 0x28559 | - | **transcribed, never called** on these screens |
| `voice_playing` | 0x287ad | 0, 1, 4 | agreed |
| `follow_then_tick` | 0x289ba | 0 | agreed |
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
| `stop_voice_playing` | 0x290ab | 0, 1 | agreed |
| `free_voice_records` | 0x29106 | - | **transcribed, never called** on these screens |
| `start_on_free_voice` | 0x29152 | 0, 1 | agreed |
| `stop_all_voices` | 0x2923d | 0 | agreed |
| `set_sound_callback` | 0x2928c | - | **transcribed, never called** on these screens |
| `set_master_level_ok` | 0x296a1 | 0 | agreed |
| `alloc_voice_records` | 0x28800 | 0 | agreed |
| `stop_music_or_effect` | 0x083ea | 0, 1 | agreed |
| `stop_sequences` | 0x294ff | 0, 1 | agreed |
| `shutdown_sound` | 0x29cf6 | - | **transcribed, never called** on these screens |
| `stop_sound` | 0x292f4 | - | **transcribed, never called** on these screens |
| `delay_five_ticks` | 0x2937f | - | **transcribed, never called** on these screens |
| `tick_delay` | 0x293b8 | - | **transcribed, never called** on these screens |
| `remove_and_free_records` | 0x293c1 | 0, 1 | agreed |
| `start_sequence_by_id` | 0x29a49 | 0, 1, 4 | agreed |
| `vm_init` | 0x22483 | 0 | agreed |
| `load_video_driver` | 0x22efd | 0 | agreed |
| `detect_adapter` | 0x225d2 | 0 | agreed |
| `read_bmp_info` | 0x234d2 | 0, 1, 4 | agreed |
| `table_618a_in_use` | 0x215d5 | 0 | agreed |
| `mouse_move_to` | 0x22113 | 0 | agreed |
| `huge_add_positive` | 0x22190 | 0, 1, 4 | agreed |
| `install_divide_trap` | 0x22394 | 0 | agreed |
| `restore_file_record_from` | 0x23ee4 | 0, 1, 4 | agreed |
| `set_field_4_of_each` | 0x252b4 | 0, 1 | agreed |
| `count_list` | 0x252e0 | 0, 1, 4 | agreed |
| `far_copy` | 0x25d96 | 0, 1, 4 | agreed |
| `string_concat` | 0x0dc95 | 0, 1, 4 | agreed |
| `stdio_setbuf` | 0x0c1b2 | - | **transcribed, never called** on these screens |
| `set_holiday_flags` | 0x08259 | 0 | agreed |
| `dos_get_cur_dir` | 0x0b7b3 | 0 | agreed |
| `dos_getdate` | 0x0bd4a | 0 | agreed |
| `heap_free_far` | 0x0bb2d | 0, 1, 4 | agreed |
| `read_tim_cfg` | 0x12ba7 | 0 | agreed |
| `game_fread_far` | 0x11dd1 | 0, 1, 4 | agreed |
| `show_page_thunk` | 0x2149a | 0, 1, 4 | agreed |
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
| `decompress_lzw` | 0x1ca62 | 0, 1, 4 | agreed |
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
| `long_multiply_2` | 0x0bcf6 | 0, 1, 4 | agreed |
| `long_multiply` | 0x0c16e | 0, 1, 4 | agreed |
| `long_shift_right` | 0x0be62 | 0, 1, 4 | agreed |
| `long_divide` | 0x0bd93 | 0, 1, 4 | agreed |
| `score_code_to_score` | 0x02900 | - | **transcribed, never called** on these screens |
| `parse_base` | 0x02a34 | - | **transcribed, never called** on these screens |
| `string_reverse` | 0x0de1e | - | **transcribed, never called** on these screens |
| `password_to_level` | 0x12ad0 | - | **transcribed, never called** on these screens |
| `picker_type` | 0x13490 | - | **transcribed, never called** on these screens |
| `sub_1156c` | 0x1156c | - | **transcribed, never called** on these screens |
| `picker_tab` | 0x1345f | - | **transcribed, never called** on these screens |
| `puzzle_tab` | 0x0f468 | - | **transcribed, never called** on these screens |
| `draw_button` | 0x150db | - | **transcribed, never called** on these screens |
| `puzzle_repaint` | 0x0f4b5 | - | **transcribed, never called** on these screens |
| `puzzle_draw_list` | 0x0f6cc | - | **transcribed, never called** on these screens |
| `puzzle_draw_password` | 0x0f640 | - | **transcribed, never called** on these screens |
| `puzzle_draw_up` | 0x0f57e | - | **transcribed, never called** on these screens |
| `puzzle_draw_down` | 0x0f5c4 | - | **transcribed, never called** on these screens |
| `puzzle_draw_ok` | 0x0f60a | - | **transcribed, never called** on these screens |
| `get_puzzle_title` | 0x12a2f | - | **transcribed, never called** on these screens |
| `game_fread_line` | 0x11e0b | - | **transcribed, never called** on these screens |
| `region_cursor_freeform` | 0x114db | - | **transcribed, never called** on these screens |
| `region_cursor_load` | 0x114f8 | - | **transcribed, never called** on these screens |
| `region_cursor_save` | 0x11515 | - | **transcribed, never called** on these screens |
| `region_cursor_gravity` | 0x11532 | - | **transcribed, never called** on these screens |
| `region_cursor_air` | 0x1154f | - | **transcribed, never called** on these screens |
| `picker_repaint` | 0x136c9 | - | **transcribed, never called** on these screens |
| `sub_1271c` | 0x1271c | - | **transcribed, never called** on these screens |
| `save_machine` | 0x1292d | - | **transcribed, never called** on these screens |
| `write_string` | 0x12411 | - | **transcribed, never called** on these screens |
| `dos_creat` | 0x0d584 | - | **transcribed, never called** on these screens |
| `dos_write` | 0x0df7a | - | **transcribed, never called** on these screens |
| `write_text` | 0x0de6e | - | **transcribed, never called** on these screens |
| `sub_0d8ca` | 0x0d8ca | - | **transcribed, never called** on these screens |
| `dos_chdir` | 0x0b755 | - | **transcribed, never called** on these screens |
| `draw_sunken_box` | 0x153b8 | - | **transcribed, never called** on these screens |
| `picker_draw_up` | 0x137e4 | - | **transcribed, never called** on these screens |
| `picker_draw_down` | 0x1382a | - | **transcribed, never called** on these screens |
| `picker_draw_action` | 0x13402 | - | **transcribed, never called** on these screens |
| `picker_draw_list` | 0x139ac | - | **transcribed, never called** on these screens |
| `picker_draw_name` | 0x13870 | - | **transcribed, never called** on these screens |
| `picker_draw_filename` | 0x13902 | - | **transcribed, never called** on these screens |
| `sub_13c78` | 0x13c78 | - | **transcribed, never called** on these screens |
| `sub_13a8a` | 0x13a8a | - | **transcribed, never called** on these screens |
| `write_byte` | 0x123b7 | - | **transcribed, never called** on these screens |
| `write_word` | 0x123e4 | - | **transcribed, never called** on these screens |
| `sub_12430` | 0x12430 | - | **transcribed, never called** on these screens |
| `sub_126ec` | 0x126ec | - | **transcribed, never called** on these screens |
| `sub_126b3` | 0x126b3 | - | **transcribed, never called** on these screens |
| `part_index` | 0x11d00 | - | **transcribed, never called** on these screens |
| `path_join` | 0x1354c | - | **transcribed, never called** on these screens |
| `path_is_root` | 0x134dd | - | **transcribed, never called** on these screens |
| `path_up` | 0x13516 | - | **transcribed, never called** on these screens |
| `force_extension` | 0x135a6 | - | **transcribed, never called** on these screens |
| `listing_to_name` | 0x13d75 | - | **transcribed, never called** on these screens |
| `picker_name` | 0x135ef | - | **transcribed, never called** on these screens |
| `picker_set_name` | 0x135dc | - | **transcribed, never called** on these screens |
| `validate_filename` | 0x1319d | - | **transcribed, never called** on these screens |
| `is_machine_file` | 0x1295f | - | **transcribed, never called** on these screens |
| `puzzle_page_of_score` | 0x0f499 | - | **transcribed, never called** on these screens |
| `string_length` | 0x0dd95 | - | **transcribed, never called** on these screens |
| `string_chr` | 0x0dcce | - | **transcribed, never called** on these screens |
| `string_compare` | 0x0dd04 | - | **transcribed, never called** on these screens |
| `string_ncompare_i` | 0x0dddb | - | **transcribed, never called** on these screens |
| `string_upper` | 0x0de4e | - | **transcribed, never called** on these screens |
| `to_lower` | 0x0c293 | - | **transcribed, never called** on these screens |
| `mem_copy` | 0x0d524 | - | **transcribed, never called** on these screens |
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
| `reset_machine` | 0x07e45 | 0 | agreed |
| `clear_machine` | 0x013e9 | 0 | agreed |
| `unlink_node` | 0x05628 | 0, 1 | agreed |
| `draw_char` | 0x21670 | - | **transcribed, never called** on these screens |
| `blit_scaled_a` | 0x227ac | - | **transcribed, never called** on these screens |
| `vm_blit_glyph` | VM.OVL VGA:0x124b | - | **transcribed, never called** on these screens |
| `compute_step` | 0x20840 | - | **transcribed, never called** on these screens |
| `draw_compressed_bitmap` | 0x20185 | 0, 1, 2, 3, 50, 200 | agreed |
| `draw_part` | 0x16db1 | 0, 1, 2, 30, 100 | agreed |
| `draw_rope` | 0x167fa | 0, 1 | agreed |
| `draw_belt` | 0x16baf | 0, 1 | agreed |
| `draw_machine` | 0x1675e | 0, 100, 300 | agreed |
| `step_and_draw_machine` | 0x16181 | 0, 1 | agreed |
| `refile_overlapping_parts` | 0x06b5b | 0, 1 | agreed |
| `copy_rect_around_cursor` | 0x0b28e | 0, 1 | agreed |
| `read_record_fields` | 0x11e3f | 0, 1, 2, 20 | agreed |
| `game_fread` | 0x091ef | 0, 1, 4 | agreed |
| `flush_pending_volumes` | 0x27a86 | 0, 1 | agreed |
| `sx_controller` | SX.OVL SPKR:0x03a1 | 0, 1 | agreed |
| `sx_pitch_bend` | SX.OVL SPKR:0x0410 | 0 | agreed |
| `sx_stop_note` | SX.OVL SPKR:0x037b | 0, 1 | agreed |
| `sx_start_note` | SX.OVL SPKR:0x0386 | 0, 1 | agreed |
| `sx_speaker_off` | SX.OVL SPKR:0x0480 | 0 | agreed |
| `sx_apply_bend` | SX.OVL SPKR:0x04fd | - | **transcribed, never called** on these screens |
| `sx_note_on` | SX.OVL SPKR:0x0497 | 0 | agreed |
| `vm_driver_init` | VM.OVL VGA:0x0000 | 0 | agreed |
| `vm_reset_attributes` | VM.OVL VGA:0x011d | 0 | agreed |
| `vm_blit_bitmap` | VM.OVL VGA:0x1707 | 0, 1, 2 | agreed |
| `vm_load_bitmap_list` | VM.OVL VGA:0x1015 | 0 | agreed |
| `vm_chunky_to_planar` | VM.OVL VGA:0x10b8 | 0 | agreed |
| `vm_read_four_planes` | VM.OVL VGA:0x11bb | 0 | agreed |
| `vm_build_mask_plane` | VM.OVL VGA:0x11ee | 0 | agreed |
| `vm_bitmap_list_size` | VM.OVL VGA:0x0fd4 | 0, 1 | agreed |
| `vm_buffer_size` | VM.OVL VGA:0x138e | 0, 1 | agreed |
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
| `compute_bounds_53fe` | 0x00386 | 0, 3, 20 | agreed |
| `pick_for_record` | 0x05ba7 | 0, 3, 20 | agreed |
| `set_side_flags` | 0x004fd | 0, 3, 20 | agreed |
| `far_memcpy` | 0x222c6 | 0, 2 | agreed |
| `claim_page_slot` | 0x0b429 | 0, 3, 9 | agreed |
| `save_or_restore_draw_state` | 0x0b47f | 0, 1, 8 | agreed |
| `clamp_record_pair` | 0x02bcc | 0, 3, 20 | agreed |
| `set_clip_for_mode` | 0x082c3 | 0, 2, 8 | agreed |
| `link_record_into_buckets` | 0x166ef | 0, 3, 20 | agreed |
| `update_velocity` | 0x07283 | 0, 3, 20 | agreed |
| `clip_and_draw_line` | 0x21e34 | 0, 3, 20 | agreed |
| `vm_draw_line` | VM.OVL VGA:0x0998 | 0, 2, 9, 30 | agreed |
| `far_memset` | 0x22300 | 0, 2, 9 | agreed |
| `compute_swept_bounds_5400` | 0x002dd | 0, 3, 20 | agreed |
| `angles_same_side` | 0x003df | 0, 3, 20 | agreed |
| `insert_sorted` | 0x05646 | 0, 3, 20 | agreed |
| `dos_alloc_bytes` | 0x21abd | 0, 2, 9 | agreed |
| `mul16x16` | 0x2a269 | 0, 5, 40 | agreed |
| `apply_gravity_and_speed` | 0x02c39 | 0, 3, 20 | agreed |
| `vm_load_palette` | VM.OVL VGA:0x0f15 | 0, 1, 2 | agreed |
| `huge_move` | 0x221ed | 0, 1, 2 | agreed |
| `mouse_init` | 0x21f1d | 0 | agreed |
| `mouse_set_ranges` | 0x21f8d | 0 | agreed |
| `set_font` | 0x2149e | 0 | agreed |
| `install_keyboard` | 0x21094 | 0 | agreed |
| `build_screen_regions` | 0x085c9 | 0 | agreed |
| `count_level_files` | 0x129a8 | 0 | agreed |
| `load_bitmap_list` | 0x2367c | 0 | agreed |
| `free_bitmap_list` | 0x23a18 | 0 | agreed |
| `expand_1bpp_to_4bpp` | 0x23a8a | - | **transcribed, never called** on these screens |
| `load_bitmaps` | 0x24f72 | 0 | agreed |
| `planes_to_chunky` | 0x24320 | - | **transcribed, never called** on these screens |
| `compress_bitmap_list` | 0x243bf | - | **transcribed, never called** on these screens |
| `read_far` | 0x2551a | 0 | agreed |
| `load_font` | 0x2307d | 0 | agreed |
| `load_palette` | 0x1e967 | 0, 1, 2 | agreed |
| `set_palette_pointer` | 0x1eb6a | 0, 1, 2 | agreed |
| `rotate_point` | 0x03b17 | 0, 3, 20 | agreed |
| `alloc_shape` | 0x064b4 | 0, 3, 20 | agreed |
| `part_step_27e2` | 0x19aa2 | 0, 20, 100, 300 | agreed |
| `part_step_420f` | 0x1b4cf | 0, 20, 100 | agreed |
| `part_step_018e` | 0x1744e | 0, 10, 60 | agreed |
| `part_hit_0552` | 0x17812 | 0 (missed 10, 60) | **not verified** |
| `part_step_057e` | 0x1783e | 0, 10, 60 | agreed |
| `part_step_098a` | 0x17c4a | 0, 10, 60 | agreed |
| `part_step_0a5d` | 0x17d1d | 0, 10, 60 | agreed |
| `part_hit_0c6c` | 0x17f2c | - | **transcribed, never called** on these screens |
| `part_step_0ca3` | 0x17f63 | 0, 10, 60 | agreed |
| `part_step_11a6` | 0x18466 | 0, 10, 60 | agreed |
| `part_step_12c2` | 0x18582 | 0, 10, 60 | agreed |
| `part_step_13c9` | 0x18689 | 0, 10, 60 | agreed |
| `part_hit_14d3` | 0x18793 | 0 (missed 10, 60) | **not verified** |
| `part_step_15ce` | 0x1888e | 0, 10, 60 | agreed |
| `part_step_1a82` | 0x18d42 | 0, 10, 60 | agreed |
| `part_hit_1c39` | 0x18ef9 | 0 (missed 10, 60) | **not verified** |
| `part_step_1c5f` | 0x18f1f | 0, 10, 60 | agreed |
| `part_hit_1d07` | 0x18fc7 | 0, 10, 60 | agreed |
| `part_step_1d78` | 0x19038 | 0, 10, 60 | agreed |
| `part_step_1e5c` | 0x1911c | 0, 10, 60 | agreed |
| `part_step_20fc` | 0x193bc | 0, 10, 60 | agreed |
| `part_step_22ae` | 0x1956e | 0, 10, 60 | agreed |
| `part_hit_2514` | 0x197d4 | 0, 10, 60 | agreed |
| `part_step_2592` | 0x19852 | 0, 10, 60 | agreed |
| `part_step_2b99` | 0x19e59 | 0, 10, 60 | agreed |
| `part_hit_2f25` | 0x1a1e5 | 0, 10, 60 | agreed |
| `part_step_2f3e` | 0x1a1fe | 0, 10, 60 | agreed |
| `part_step_3035` | 0x1a2f5 | 150, 170, 190, 210, 230, 250, 270, 290 | agreed |
| `part_hit_34b5` | 0x1a775 | - | **transcribed, never called** on these screens |
| `part_step_34d0` | 0x1a790 | 0, 10, 60 | agreed |
| `part_step_3635` | 0x1a8f5 | 0, 10, 60 | agreed |
| `part_hit_3824` | 0x1aae4 | 0 (missed 10, 60) | **not verified** |
| `part_step_38fc` | 0x1abbc | 0, 10, 60 | agreed |
| `part_hit_3ebf` | 0x1b17f | 0, 10 (missed 60) | **not verified** |
| `part_step_3fae` | 0x1b26e | 0, 10, 60 | agreed |
| `part_hit_3fe8` | 0x1b2a8 | 0, 10, 60 | agreed |
| `part_step_49a1` | 0x1bc61 | 0, 10, 60 | agreed |
| `part_hit_016e` | 0x1742e | - | **transcribed, never called** on these screens |
| `part_hit_1de0` | 0x190a0 | - | **transcribed, never called** on these screens |
| `part_hit_2b7e` | 0x19e3e | - | **transcribed, never called** on these screens |
| `mark_belt_shapes` | 0x05f87 | 0, 20, 200, 600 | agreed |
| `draw_belt_segment` | 0x16b39 | 0, 20, 200, 600 | agreed |
| `belt_orientation` | 0x06de9 | 3540, 3560, 3570, 3575, 3578, 3580 | agreed |
| `tension_belt` | 0x072c7 | 3540, 3560, 3570, 3575, 3578, 3580 | agreed |
| `draw_part_extra` | 0x171b5 | 0, 40, 150, 380 | agreed |
| `draw_polygon` | 0x1eded | 0, 40, 150, 380 | agreed |
| `part_step_1649` | 0x18909 | 0, 2, 8 | agreed |
| `blast_speed_for_mass` | 0x18a08 | 0, 2, 8 | agreed |
| `split_part_at` | 0x18a7c | 0, 1, 2 | agreed |
| `clone_part` | 0x059e4 | - | **transcribed, never called** on these screens |
| `angle_between_centres` | 0x03da5 | 0, 2, 8 | agreed |
| `queue_part` | 0x07b6f | 0, 1, 2, 3, 4 | agreed |
| `bounce_pair` | 0x03201 | 0, 1, 2, 4 | agreed |
| `part_step_08f1` | 0x17bb1 | 0, 1, 2, 4 | agreed |
| `part_drive_0802` | 0x17ac2 | 0, 1, 2, 4 | agreed |
| `part_drive_2451` | 0x19711 | 0, 1, 2, 4 | agreed |
| `collect_carried` | 0x03972 | 0, 20, 100, 300 | agreed |
| `add_carried_weight` | 0x07c3a | 0, 20, 100, 300 | agreed |
| `add_mass_capped` | 0x07c5b | - | **transcribed, never called** on these screens |
| `carry_riders_along` | 0x03a8d | 0, 20, 100, 300 | agreed |
| `step_moving_object` | 0x01216 | 0, 20, 100, 300 | agreed |
| `bounce_off_contact` | 0x03046 | 0, 20, 100 | agreed |
| `replay_shapes` | 0x06699 | 0, 40, 150, 300 | agreed |
| `mark_part_shapes` | 0x0647f | 0, 20, 200, 600 | agreed |
| `part_moved` | 0x06d8e | 0, 20, 200, 600 | agreed |
| `mark_needs_refile` | 0x058f3 | 0, 20, 200 | agreed |
| `mark_joined_shapes` | 0x05e70 | 0, 20, 200 | agreed |
| `step_machine` | 0x00f86 | 0, 40, 150 | agreed |
| `add_record_shapes` | 0x0642a | 0, 3, 20 | agreed |
| `recompute_kind_physics` | 0x02ac0 | 0, 1 | agreed |
| `reset_input_state` | 0x0b4f1 | - | **transcribed, never called** on these screens |
| `compute_link_endpoints` | 0x04e65 | 0, 3, 6 | agreed |
| `find_entry_for_pointer` | 0x098e0 | 0, 1, 4 | agreed |
| `erase_both_pages` | 0x080e7 | 0 | agreed |
| `erase_object` | 0x0ad51 | 0 | agreed |
| `restage_object_rect` | 0x0aef6 | 0 | agreed |
| `claim_buffer_slot` | 0x0b5ed | 0 | agreed |
| `clear_slot_5734` | 0x0b69c | - | **transcribed, never called** on these screens |
| `seek_file_to` | 0x09b38 | 0, 2 | agreed |
| `archive_entry_for` | 0x09b7c | 0, 1, 4 | agreed |
| `clear_flag_2d44` | 0x0a7a3 | 0, 1, 4 | agreed |
| `clear_flag_2d44_thunk` | 0x0811b | 0, 1, 4 | agreed |
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
| `heap_ring_unlink` | 0x0c95a | 0, 1, 4 | agreed |
| `heap_ring_insert` | 0x0c976 | 0, 1, 4 | agreed |
| `heap_free_top` | 0x0c8e7 | 0, 1, 4 | agreed |
| `heap_free_middle` | 0x0c921 | 0, 1, 4 | agreed |
| `brk_set` | 0x0c7c4 | 0, 1, 4 | agreed |
| `heap_sbrk` | 0x0c7e6 | 0, 1, 4 | agreed |
| `heap_init` | 0x0c9f9 | 0 | agreed |
| `heap_grow` | 0x0ca39 | 0, 1, 4 | agreed |
| `heap_split` | 0x0ca62 | - | **transcribed, never called** on these screens |
| `heap_check` | 0x0cb45 | 0, 1, 4 | agreed |
| `draw_bitmap` | 0x25300 | 0, 1, 4 | agreed |
| `load_screen` | 0x253e7 | 0 | agreed |
| `load_screen_plain` | 0x23b29 | 0 | agreed |
| `free_bitmaps` | 0x23a3c | 0 | agreed |
| `poly_walk` | 0x1f562 | 0, 1, 4 | agreed |
| `poly_edge_vertical` | 0x1f265 | 0, 1, 4 | agreed |
| `poly_edge_diagonal` | 0x1f3bf | - | **transcribed, never called** on these screens |
| `poly_edge_steep` | 0x1f281 | - | **transcribed, never called** on these screens |
| `poly_edge_shallow_right` | 0x1f3e6 | 0, 1, 4 | agreed |
| `poly_edge_shallow_left` | 0x1f4a1 | 0, 1, 4 | agreed |
| `clip_polygon` | 0x20c07 | 0, 1, 4 | agreed |
| `poly_outline` | 0x1f219 | - | **transcribed, never called** on these screens |
| `restore_write_mode` | 0x1e94c | 0 | agreed |
| `seg172c_nothing` | 0x172bc | - | **transcribed, never called** on these screens |
| `free_bitmaps_thunk` | 0x252d0 | 0 | agreed |
| `game_fread_byte` | 0x11db4 | 0, 1, 4 | agreed |
| `select_cursor` | 0x0467d | 0, 1, 4 | agreed |
| `set_cursor` | 0x0aa14 | 0 | agreed |
| `draw_cursor` | 0x0ab1f | 0 | agreed |
| `redraw_cursor` | 0x0acc3 | 0 | agreed |
| `read_list` | 0x1221b | 0, 1 | agreed |
| `read_level` | 0x12269 | 0, 1 | agreed |
| `load_animation` | 0x12915 | 0, 1 | agreed |
| `alloc_part_table` | 0x11d66 | 0, 1 | agreed |
| `draw_frame_corners` | 0x0ee6e | 0, 1, 4 | agreed |
| `load_all_parts` | 0x0f7b6 | 0 | agreed |
| `load_part_bitmap` | 0x0f7f4 | 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49 | agreed |
| `build_part_list` | 0x1405b | 0, 1 | agreed |
| `make_part` | 0x14133 | 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 | agreed |
| `draw_curve` | 0x1697d | 0, 1, 4 | agreed |
| `heap_check_or_hang` | 0x08528 | 0, 1, 4 | agreed |
| `stdio_setbuf_for` | 0x095cf | 0, 1 | agreed |
| `restart_resource_stream` | 0x1dae6 | - | **transcribed, never called** on these screens |
| `sound_on_hard_impact` | 0x03009 | 0, 1, 4 | agreed |
| `rope_ends_close` | 0x04b8f | 0, 1, 4 | agreed |
| `mark_parts_in_dirty_rects` | 0x06806 | 0, 1, 4 | agreed |
| `belt_in_dirty_rect` | 0x06994 | 0, 1, 4 | agreed |
| `restore_cursor_following` | 0x08125 | 0, 1, 4 | agreed |
| `select_music` | 0x08364 | 0, 1, 4 | **differs**: two DGROUP bytes at 0x5894 and 0x5896 - see the note below |
| `play_sound` | 0x083ab | 0, 1, 4 | agreed |
| `regions_handle_pointer` | 0x08546 | 0, 1, 4 | agreed |
| `heap_malloc` | 0x0c999 | 0, 1 | agreed |
| `heap_free` | 0x0c8ca | 0, 1 | agreed |
| `dos_free_far` | 0x21b34 | 0, 1, 4 | agreed |
| `refresh_link_geometry` | 0x04f7f | 0, 1, 4 | agreed |
| `set_vector_from_angle` | 0x07223 | 0, 1, 4 | agreed |
| `link_slack` | 0x0713d | 0, 1, 4 | agreed |
| `link_endpoint_gap` | 0x07947 | 0, 1, 4 | agreed |
| `link_end_distance` | 0x06f8e | 0, 1, 4 | agreed |
| `shift_all_histories` | 0x07ca2 | 0, 1, 4 | agreed |
| `shift_state_history` | 0x07ce3 | 0, 1, 4 | agreed |
| `compare_link_ends` | 0x06de9 | 0, 1, 4 | agreed |
| `intersect_segments` | 0x03ba9 | 0, 3, 20 | agreed |
| `frame_pending` | 0x0b4e2 | 0, 1 | agreed |
| `decode_position` | 0x1e561 | - | **transcribed, not verifiable**: it has no return to detect - the compiler replaced its `ret` with `jmp 0x1e89c`, so 0x1e7f2 jumps in and it jumps back. Covered by decompress_lzss, which runs it on every one of its 226 verified calls. |
| `screen_state_4000` | 0x11290 | - | **transcribed, not verifiable**: the volume knob, up - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_2000` | 0x112a9 | - | **transcribed, not verifiable**: the volume knob, down - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_1000` | 0x112c2 | - | **transcribed, not verifiable**: quit - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_0800` | 0x112d0 | - | **transcribed, not verifiable**: restart - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_0400` | 0x112e5 | - | **transcribed, not verifiable**: enter freeform - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_0200` | 0x11347 | - | **transcribed, not verifiable**: leave freeform - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_0100` | 0x113a9 | - | **transcribed, not verifiable**: Load Machine - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_0080` | 0x1141b | - | **transcribed, not verifiable**: Save Machine - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_0040` | 0x11458 | - | **transcribed, not verifiable**: the gravity slider - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `screen_state_0020` | 0x114a0 | - | **transcribed, not verifiable**: the air-pressure slider - it is a jump target, not a routine. game_screen's table dispatches with jmp, the handler runs on game_screen's own frame, and it ends by jumping back to 0x1145b - so there is no call to stop at and no return to detect. What it does is covered by the screen comparisons in check_briefing.py, which drive the panel through it with clicks, and by the routines it calls, most of which verify individually. |
| `ask_yes_no` | 0x1567b | - | **transcribed, not verifiable**: it waits for the player. The harness stops the timer and the keyboard while a routine is open, so nothing can arrive to end the wait, and the watchdog abandons it after 30M instructions. What it draws is covered by the screen comparisons, which put the box up and click its buttons. |
| `message_box` | 0x15698 | - | **transcribed, not verifiable**: it waits for the player. The harness stops the timer and the keyboard while a routine is open, so nothing can arrive to end the wait, and the watchdog abandons it after 30M instructions. What it draws is covered by the screen comparisons, which put the box up and click its buttons. |
| `wait_and_latch_frame` | 0x0aaca | - | **transcribed, not verifiable**: waits for an interrupt the harness must suppress |
| `update_button_state` | 0x08136 | - | **transcribed, not verifiable**: calls wait_and_latch_frame, which waits for an interrupt |
| `mouse_set_speed` | 0x0b859 | - | **transcribed, not verifiable**: INT 33h and nothing else - it leaves no trace in guest memory for the two runs to disagree about |
| `blit_bitmap_thunk` | 0x1e940 | - | **transcribed, not verifiable**: it is a single `ljmp [vector]`, not a routine. The instruction transfers control into the video driver on the caller's own frame, so there is no body to run, no arguments of its own and no return to detect - the arguments and the return belong to whatever the vector points at, and that routine is specced separately. Its correctness is the vector's value, which the screen comparisons exercise on every frame they draw. |
| `blit_rows_thunk` | 0x20838 | - | **transcribed, not verifiable**: it is a single `ljmp [vector]`, not a routine. The instruction transfers control into the video driver on the caller's own frame, so there is no body to run, no arguments of its own and no return to detect - the arguments and the return belong to whatever the vector points at, and that routine is specced separately. Its correctness is the vector's value, which the screen comparisons exercise on every frame they draw. |
| `copy_rect_thunk` | 0x21088 | - | **transcribed, not verifiable**: it is a single `ljmp [vector]`, not a routine. The instruction transfers control into the video driver on the caller's own frame, so there is no body to run, no arguments of its own and no return to detect - the arguments and the return belong to whatever the vector points at, and that routine is specced separately. Its correctness is the vector's value, which the screen comparisons exercise on every frame they draw. |
| `game_main` | 0x0dfff | - | **transcribed, not verifiable**: its body is the rest of the program. `game_main` is nineteen instructions - startup, intro, play, teardown - so stopping at its entry and letting the original run to its return is the entire game, not a bounded comparison; the harness abandons a call it has not seen return within 30 million instructions, and this one does not return until the game exits. `game_startup` and `game_intro` are the same in kind. What they do is covered by the routines they call, which verify individually, and by the screen comparisons in check_briefing.py. |
| `game_startup` | 0x0e01d | - | **transcribed, not verifiable**: its body is the rest of the program. `game_main` is nineteen instructions - startup, intro, play, teardown - so stopping at its entry and letting the original run to its return is the entire game, not a bounded comparison; the harness abandons a call it has not seen return within 30 million instructions, and this one does not return until the game exits. `game_startup` and `game_intro` are the same in kind. What they do is covered by the routines they call, which verify individually, and by the screen comparisons in check_briefing.py. |
| `game_intro` | 0x0e4be | - | **transcribed, not verifiable**: its body is the rest of the program. `game_main` is nineteen instructions - startup, intro, play, teardown - so stopping at its entry and letting the original run to its return is the entire game, not a bounded comparison; the harness abandons a call it has not seen return within 30 million instructions, and this one does not return until the game exits. `game_startup` and `game_intro` are the same in kind. What they do is covered by the routines they call, which verify individually, and by the screen comparisons in check_briefing.py. |

*734 routines transcribed. **This run asked about 560 of them** and 410 agreed; the other 174 were not asked, and are **unchecked, not disproved**. Written by `tools/verify.py --all`, not by hand - one run of the original captures every call. This one was **with no input**, in 2862 seconds; "never called" means that run did not reach it. Specs added after a sweep starts are not in the table it writes: compare the row count against `verify.py --list`.*
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

Twelve stubs exist - `tests/provenance.py` counts them - and **two of them are
reached from `present_frame`** at 0x081cc: `0x0b078` and `0x0e34a`. Neither is
reachable on the intro screens: **all 436 calls to `present_frame` while they
run have both DGROUP flags at zero**, which is measured rather than argued. (An
earlier version of this sentence said two stubs existed altogether, which was
never true of the whole port - only of this one caller.)

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


### `select_music` differs, and every part of it agrees

`verify.py --all` reports two differing bytes after `select_music(0x03f6)`:

    0x33d54 (DGROUP 0x5894)  original 0c  port 08
    0x33d56 (DGROUP 0x5896)  original 42  port 43

0x5894 and 0x5896 are the resource stream's huge destination pointer, offset
and segment. The port's finishes twelve bytes along from the original's -
segment one higher, offset four lower.

**Every routine in that subtree verifies on its own.** This was checked so the
next person does not check it again:

| routine | calls seen | verdict |
| --- | --- | --- |
| `open_sound_file` | 38 | agreed |
| `play_sound` | 4326 | agreed |
| `stop_sequences` | 3122 | agreed |
| `remove_and_free_records` | 18 | agreed |
| `read_record`, `read_sound_records`, `load_sound_bank` | 16-38 each | agreed |
| `huge_add_to` | **first 400** | agreed |
| `read_into_huge` | **all 119** | agreed |
| `resource_read` | 1273 | agreed |
| `normalise_far_ptr` | 2757 | agreed |
| `emit_literal_run` | **all 119** | agreed |
| `emit_byte` | **first 601** | agreed |

`emit_literal_run` at 0x1c493 is where the pointer is actually stepped -
`huge_add_to(0x5894, ...)` on the path taken, and *not* stepped on the spill
path, which its own transcription notes as unreached on the screens checked so
far. A branch taken differently between the two sides would move the pointer
differently, which fits the symptom exactly. It verifies at its sampled calls,
as does `emit_byte` at 46,161. That is the whole difficulty in one line: the
routine that could produce this agrees every time anybody has looked.

`stop_music_or_effect` had no spec at all and was the obvious suspect for that
reason; one was written and the routine is never entered within a run's
budget, so it is not the cause - at that occurrence the previous music is -1
and `select_music` skips the call.

So the parts agree and the whole does not. The likeliest reading is that the
divergence is at an occurrence no spec samples: `huge_add_to` is checked at
three of its 46,723 calls, and a routine that agrees on calls 0, 1 and 4 can
still disagree on call twenty thousand. Finding it wants a differential over
*every* call of one routine rather than three, which this harness does not do
today.

**The four routines that touch the pointer are eliminated over hundreds of
calls, not three.** `verify.py --all --only <name> --occurrences LO-HI` rewrites
a spec's occurrence list, so the sampling that made "agreed" a weak claim can
be widened at will - and it is fast, four to six seconds to capture six hundred
calls. All 119 of `emit_literal_run`, all 119 of `read_into_huge`, the first
400 of `huge_add_to` and the first 601 of `emit_byte` agree. `emit_fill_run` is
still open: asking for 401 of its calls ran the budget out looking for calls it
never makes.

That is the flag's cost model, and it is worth knowing before using it: **the
cost is the highest occurrence asked for, not how many.** Collection stops once
the last one wanted has been seen, so 0-600 of `emit_byte` - called every frame
- takes six seconds, while 0-17 of `select_music` - eighteen calls spread over
a whole run - exhausted a forty-minute budget without finishing. Ask for a low
range first.

It is in the sound path, so no screen comparison can see it - which is the
argument for the sweep existing, and for not reading a wall of green as proof
that nothing is wrong.

**How to reproduce it in ten minutes.**

    uv run python tools/verify.py --all --only select_music \
        --budget 2600000000

That is the narrowed sweep: it shares `--all`'s collection but wants one
routine, so it stops as soon as that routine's occurrences are captured
instead of running the whole budget. Ten minutes, and it leaves STATUS.md
alone.

The single-routine path does **not** work, which is worth knowing before
trying it. `verify.py select_music --occurrence N` reaches none of the three:
occurrence 0 stops at 0x0cf13, flushing every open stream, which is not
transcribed; 1 and 4 are past the default per-routine budget and report NOT
ENTERED, which the tool is careful to call unchecked rather than a pass.

**Occurrence 0 agrees; occurrence 1 differs.** A 40M run captures only
occurrence 0 and reports the routine unverified for the *missing* occurrences
rather than differing - so the first call through is right and the second is
not, and whatever goes wrong is built up between them rather than being wrong
from the start.

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

## Every puzzle, and what it takes to run one

`tools/puzzles.py` drives the game's own SELECT PUZZLE screen from a briefing
snapshot, picks a puzzle, starts the level and runs the machine. The clicks are
the game's - the picker's five regions read out of a snapshot of it, and the
row arithmetic `(y - 0x4c) / 10 + page` that `puzzle_screen` uses at 0x0f0b0 -
so the same sequence would drive the original, which writing the puzzle number
into DGROUP would not.

Measured on 2026-09-03, puzzles 1 to 47, the furthest the save has unlocked:

- **47 of 47 reach their own briefing.** The picker path works end to end,
  including its rule that choosing a puzzle *earlier* than the one being played
  zeroes the score - puzzles 1 and 2 come back with 0 where 4 and up keep 821.
- **11 of 47 run their machine with no trap at all**: 1-8, 27, 30, 45. Checked
  with `--snap-at end` rather than inferred from silence: all eleven are in
  state 0x2000, machine running, when the run stops. A run with no trap proves
  nothing on its own, because a missed click looks exactly like it.
- **36 of 47 trap**, and they are not scattered: 31 of them are **goal tests**
  and 5 are part hooks.

### The goal tests are the gap

The table at DGROUP 0x2632 is indexed by the puzzle number and holds **64
distinct goal tests** across the 87 puzzles. **Seven are transcribed** - the
ones the first seven puzzles use - and the rest are not. They are a contiguous
run of small routines from 0x01476 to 0x0242c in segment 0, each a walk of an
object list testing a few fields, so the body of work is bounded and known.

Twenty-eight distinct ones are needed for puzzles 1 to 47:

    0x014ad 0x014cc 0x014ee 0x016a6 0x016fb 0x0172d 0x01753 0x017db
    0x01819 0x01846 0x01888 0x018d9 0x01907 0x01935 0x019ac 0x019e0
    0x01a49 0x01ab0 0x01b89 0x01d8c 0x01dbb 0x01df1 0x01e1e 0x01e59
    0x01eb9 0x01f25 0x01fa6 0x02010 0x02065 0x020fa 0x02260 0x023ef
    0x0242c

and four part routines: 172c:2d40 and 172c:332a and 172c:3e08, each wanted by
two puzzles, and the part *drive* at 172c:02cd, wanted by puzzle 32.

**None of these could be found by any instrument in the tree before now.** A
goal test only runs while a machine is running, on the one puzzle that selects
it, so no screen comparison and no scripted `verify.py` run reaches one. That
is how `goal_test_15fa` stayed inverted - passing on a balloon that was still
in the air - until somebody played puzzle 3 and it won on the first frame.

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

`io.c` serves DOS files out of the game directory, and `borland_file.c` has the
runtime's `read` (0x0c185) and `lseek` (0x0c0c3) over them - same standing as
`borland_heap.c`, kept for reference. Handles are numbered from 5, as DOS does
once the five standard ones are taken, because the guest stores the number it
is given and the comparison sees it.

**Writes go to memory and never to the disk.** A handle is a buffer: every open
reads the whole file in, and reads, writes and seeks work on that. A file the
game *creates* is kept in an overlay keyed by the DOS name it was created
under, and opening that name again finds the overlay before the host, so a
machine the player saves can be loaded back in the same session. Overwriting a
file that already exists opens the host copy, whose writes are dropped at close.

All of that is the emulator's model rather than a precaution of the port's: it
opens the host filesystem read-only and satisfies guest writes from an overlay
of its own. The reference is what defines what the game sees when it saves and
re-reads, so matching it is correctness. There is no code in `io.c` that opens
a host file for writing, which is a structural guarantee rather than a check.

`chdir` (0x0b755) works on the same terms - a directory inside the game's, with
the game's directory as a **floor** rather than a starting point, so `..` at
the root stays at the root. `setdisk` (0x0b819) changes nothing, and the
reference agrees by not implementing INT 21h AH=0Eh at all.

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
- ~~Anything past the intro screens: the menu, the puzzles, the level editor.~~
  Partly done: `./reconstruct/tim` now starts and plays level one. See below.

## Deferred on purpose: the timer's concurrency

The port runs the guest's INT 08h handler on a pthread, and an interrupt on the
hardware this came from is **exclusive** - it suspends the interrupted code and
finishes on the one CPU. Threads are not, so the handler and the main code are
both inside the guest's shared DGROUP at once: the driver's clip and pages, the
pointer, the button accumulators, the frame counter and `frame_flag`.

It is visible as a stray column of odometer digits, or sometimes cursor
bitmaps, escaping the counter strip into the play page - see CLAUDE.md for how
that was pinned down, and note that the hybrid runner never shows it because it
delivers the tick between emulator slices instead.

**Revisit after the transcription, not before.** A mutex around the blits would
answer the symptom and not the problem; the frame-pacing spins read words the
timer writes, through a non-volatile `DGU16`, and want a model rather than a
lock. Recorded here so it is picked up deliberately rather than rediscovered.

## The level loop, and how much of it has actually run

`game_screen_loop` at 0x0f8c2 and the routines under it are transcribed, and
the port reaches the game screen on its own - 601 flips, no stub. Measured
against the original with `check_briefing.py --screen level`, everything
outside two boxes is pixel for pixel identical; the boxes are the odometer
reels, which turn on a timer the two sides pace differently, and the mouse
cursor. 2,700 pixels of 307,200.

**What has and has not been exercised.** Merged over three scenarios - a
machine running, a part carried, and clicks on part bodies - with
`TIM_COVER` and `tools/native/covered.py`:

    pointer_frame          0x0fc0e  100.0%
    run_drag_frame         0x10816  100.0%
    part_key_shortcut      0x10410   95.5%
    settle_carried_part_first  0x10a00  94.9%
    drag_carried_part_first    0x108ec  90.7%
    move_carried           0x0fe47   84.6%
    game_screen_loop       0x0f8c2   72.8%
    pick_up_part           0x10658   64.6%
    discard_carried_part   0x10733   62.2%
    move_carried_part      0x101dc   44.2%
    drag_carried_part_pair 0x10ada    0.0%
    settle_carried_part    0x10bee    0.0%

**Getting there needed the game's own rules, not better guessing at pixels**,
and working them out proved the transcription rather than only exercising it:

  - A click that seems to do nothing sets 0x4e69 to **10**, whose whole arm is
    "let go of the part". The pointer was not on anything grabbable and
    `find_part_from` answered with its documented fallback - the part it was
    handed, unchanged.
  - Handle positions have to be *computed*: the part's +0x2a and +0x2c less the
    play-area origins, with the handles 11 pixels outside that box. They are
    only reachable once the part is already current, because
    `part_under_pointer` grows the box by 11 **only** when `exclude` matches -
    which is what makes a part you are holding easier to hit.
  - A part with bit **0x8000** in +6 can never be selected: `pointer_frame`
    tests for it and clears 0x50d5. On level one the three kind-5 conveyors all
    carry it, which is why aiming at them looked like broken input for several
    rounds. `find_part_from` finds them perfectly well - traced, `best=0x8c30` -
    and the caller throws the answer away.

So the two remaining zeroes are **not** a gap in the port. Tools 5 and 6 need a
part whose +8 has bit 0x100, and the only one on this level, 0x88f2, also has
0x8000 and is therefore unselectable. Those two movers cannot run here at all,
the same shape as `reverse_link_ends`; exercising them needs a level with a
part that has the second axis and is not scenery.

`game_screen_loop`'s own 72.8% is partly an artefact: a snapshot restores
inside the routine, so its prologue never runs again. See covered.py.
