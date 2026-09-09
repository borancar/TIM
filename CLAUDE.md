# Working on this reconstruction

The Incredible Machine (Dynamix / Sierra, 1993), reverse engineered from
`incredible-machine/TIM.EXE` and reconstructed as C.

Two artefacts check each other: the **emulator** running the original binary is
the *reference* that defines what "correct" means, and the **C port** is the
deliverable. Neither is trusted alone. Nothing is finished because it looks
right on screen.

## The rule that outranks the others

**Transcribe. Do not write your own version.**

If a routine does something the original does, find it in the disassembly and
read it. A behavioural match is only as good as the states you happened to
compare, and a plausible routine that agrees with every capture can still be
wrong. Writing your own is right for **IO and nothing else**.

The practical test before writing any helper: *does the original have to do
this too?* If yes, it has a routine - go and find it. If it is only necessary
because we are on a modern machine with a window and a filesystem, it is ours.

This applies to tooling as well. `tools/unlzexe.py` does not reimplement the
LZEXE algorithm; it *runs the stub* and reads the machine out afterwards.

## Conventions

- **`stdint` types only**: `uint8_t`, `uint16_t`, `uint32_t`, `int16_t`,
  `int32_t`. Never `unsigned`, `unsigned char`, `short`, `long`, or a bare
  `int`. `char` stays `char` for real strings (paths, `printf`).
  This is not style. It is 16-bit code where every value has a width the
  original depended on: `int16_t` says "this truncation is the `imul`'s" in a
  way `short` does not. The widths usually match on a modern ABI, so getting it
  wrong compiles, runs, and silently loses the one fact the type carried.
- **Addresses are image offsets** - byte offsets into `out/TIM.img`, the
  recovered image - unless written `seg:off`. The original's entry point is
  `0000:0000`, so an image offset and a `seg:off` with segment 0 coincide.
- **Every transcribed routine carries the address it came from**, as a comment
  on the function itself. So does **every transcribed table** - a palette, a
  jump table, a string table lifted out of the executable is as much a
  transcription as a routine. `reconstruct/tests/provenance.py` enforces this
  and only the comment *directly above* a definition counts.
- Anything that is **ours** and not transcribed says so explicitly, in the same
  place. Four outcomes exist: transcribed (an address), **stub** (an address and
  the words NOT TRANSCRIBED YET), ours (said so), and neither - only the last
  is a failure. A stub must **abort** when reached, never return quietly: a
  silent no-op in a drawing path is a missing frame that looks like a blitter
  fault.
- **The port's `.c` files mirror the original's translation units**, functions
  in address order. In this large-model binary each module is its own code
  segment, so the boundaries are readable off the binary - see
  `docs/executable.md`. Any boundary *we* added for porting says so in its
  header.

  **They live in `reconstruct/src`.** That directory is the game and nothing
  else: the eight modules, the DGROUP array and the two overlays, plus
  `main.c`, which is there because a DOS game's entry point is part of the
  game. What stays a directory up is what is *not* the game - `io.c` the
  hardware, `sdl.c` the window, `borland_*.c` the C library it was linked
  against, and the `dev*.c` files that never ship. A tool that reads the port's
  sources must glob both.

  **The names are ours; the boundaries are the original's.** `machine.c`,
  `game.c`, `parts.c` and the rest were called `seg0000.c` and so on until each
  had been read well enough to say what it holds. Every one still names its
  segment and image range in its header, because that is the fact - a file
  called `engine.c` is a judgement about 8,275 lines and the segment number is
  not. Do not move a routine between files to suit a name: the file it belongs
  in is the one whose address range contains it.
- **DGROUP is a byte array, not a set of C globals.** The game uses near
  pointers - a word in DGROUP holding an offset into DGROUP - which named
  globals cannot express. Names are macros over the array, so a name and a
  pointer dereference reach the same byte. The video driver's data is part of
  the same segment, at offset 0x3890.
- Where a name or a type is a guess, **say so**.
- **No licence header on reconstructed code.** A provenance header naming the
  binary instead. Our own tooling is a different matter and is GPL-2.0.
- **SDL3 for the window, input and sound. Always.** Never X11, never Win32,
  never SDL2, not behind an `#ifdef`, not as "the optional viewer". One display
  path, not two: the file writer is a *mode* of the same composed frame, never
  a parallel implementation. The port shows a screen by default - running it
  with no arguments opens the game, not writes a bitmap.
- **`main.c` and `devmain.c` stay apart, and build two binaries.** A DOS game
  has no command line: it starts, shows its menu, and plays, and `main.c`
  mirrors that. Every developer flag goes in `devmain.c`. `tools/` calls the
  dev binary, so nothing a comparison depends on can become part of what ships.

## The traps this project has already hit

- **A DOS program with `maxalloc = 0xFFFF` owns all of conventional memory**
  until its runtime hands the tail back with INT 21h AH=4Ah. Modelling the free
  arena as starting just above `image+minalloc` puts DOS's blocks *inside* the
  program's own DGROUP - and Borland's large-model startup puts the stack at
  the top of a 64 KB DGROUP. The symptom was a `retf` into zeroed memory a
  million instructions later, which looks like anything but an allocator bug.
  `TimMachine._dos` models it properly.
- **The game asks the BIOS what adapter it has** (INT 10h AH=1Ah, then AH=12h
  BL=10h). Left unimplemented these leave BX as the caller set it, the game
  concludes there is no VGA *and* no EGA, fails to load `VM.OVL` and prints
  "Unable to initialize vm.". Both are plain VGA BIOS services.
- **`files missing:` in a run report is usually not an error.** The game tries
  each resource as a loose file first and falls back to the archive, so a long
  list of missing `.BMP` and `.LEV` names is the normal path.
- **Capstone's 16-bit mode gets `cbw`/`cwd` wrong.** It prints the 32-bit
  mnemonics - `cwde` for 0x98 and `cdq` for 0x99 - where 16-bit code means
  `cbw` and `cwd`, and prints the 16-bit ones when a 0x66 prefix makes them
  32-bit. `tools/disasm.py` corrects this from the instruction's own bytes.
  Uncorrected, a listing says a routine sign-extends AX into EDX when it
  sign-extends into DX, and a transcription that believes it gets the width
  wrong, compiles, and runs.
- **A segment immediate in the image is a relocation, not a value.** The
  recovered image is unrelocated, so `mov word [0x4bbe], 0` in the listing is
  really "the program's own base": the loader patches those two bytes. The
  disassembly gives no sign of it. Transcribing the zero as written is wrong in
  a way nothing catches until the cell is compared - and then it reads 0x0110,
  which is the load segment and looks like nonsense until you see why. Work any
  segment out from where the program actually is.

- **A jump that lands one byte past the last instruction you read means there
  is an instruction you have not read.** `strcat`'s alignment step is `movsb`
  followed by a one-byte `dec cx`; a disassembly window ending at the `movsb`
  shows the `je` targeting an address one byte further on, and reading it as
  absent turns a correct routine into an apparent off-by-one. The verifier
  caught it in one byte, but the wrong *explanation* had already been written
  into a comment. Re-dump from the branch target when the arithmetic does not
  add up.

- **The annotator must only report the *start* of a string.** A version that
  matched anywhere inside one happily labelled every small constant with the
  tail of the Borland banner, which makes a listing look informative and is
  worse than no annotation.

- **The last argument pushed is the first argument.** `pick_file(0, 0, "*.TIM")`
  reads as `pick_file("*.TIM", 0, 0)` if the pushes are taken in source order,
  and the transcription then copies an empty string, builds no extension filter,
  and lists every file where the original lists three. Nothing in the routine
  looks wrong - each line is right, the arguments are simply not the ones the
  caller sent. Only a side-by-side found it. Count the pushes backwards, every
  time, and where a routine's arguments cannot be checked by running it, say in
  the comment that the order is a reading rather than a measurement.

- **A check that polls can miss what it is checking, and then blames the port.**
  `tools/check_save.py` read the emulator's open files once a slice, on the
  reasoning that a slice is 2000 instructions and a save must be longer. A
  sixteen-byte save is not: truncate, write and close fit inside two slices, so
  the one sample landed between the truncate and the write and reported the
  original as having written **nothing**. The port was right and the tool said
  it was wrong, with the emulator's own log saying `WRITE +16` on the line
  above. Worse, the unsafe reasoning had been written down as a *safety
  argument* the commit before. Take a measurement at the event, not near it.

  **It happened again, to the same tool, on 2026-09-01.** The wait for the save
  polls for a file to appear and stop growing, and `port.log` - the tool's own
  capture of the port's stderr - is created in the polled directory before the
  loop starts. The listing is never empty, two passes see 0 bytes, and the port
  is killed a second after launch and reported as having written nothing. Both
  scenarios pass once the log is excluded: 16 bytes identical, 740 identical.

  The entry above was read *during* that hour and did not prevent it, so the
  general form is worth stating plainly: **a watcher must not watch anything it
  created itself.** And when a check says the thing under test produced
  nothing, suspect the check first - twice now, this one has been wrong and the
  port has been right.

  **A third time, on 2026-09-09, and the rule as written did not stop it.**
  `run_port` grew a `shutil.copytree` of the game directory into the same
  output directory, and the wait excluded exactly one name - `port.log` -
  because that is what the note above says. A directory never changes size, so
  the loop agreed with itself twice and killed the port a second in; the
  comparison then called `open()` on it and the run ended in a traceback,
  *after* the real save had already been compared. So: the exclusion is one
  list, `OURS`, used by the wait and by the comparison both, and a name that is
  not on it is the game's.

  Underneath that the check had a second failure it could not report, because
  the traceback came first. **The reference run reaches zero page flips** -
  six CRTC writes in 60M instructions, all of them the mode set - so none of
  the scenario's clicks is ever delivered and the original saves nothing.
  Printed as "the original wrote no such file" that reads as a difference in
  the saved bytes, which is the one thing it is not; it now says how many
  clicks were delivered and answers **no verdict** when the answer is about the
  reference. `verify.py` and `check_native.py` drive the original past this
  point, so what `drive.machine()` does differently is the thing to find.

- **`TIM_FLIPS=<dir>:<last>` is a stopping point, not a filter.** It writes a
  308 KB frame for *every* flip up to `<last>`, so a run to flip 800 leaves a
  quarter of a gigabyte behind. Reading it as "write flip 800" has filled the
  disk twice, the second time after a note in `devdump.c` already recorded the
  first. `TIM_FLIPWANT=<f1>,<f2>,...` is the filter, and a comparison should
  always name the flips it reads.

- **A verdict that cannot say what kind of "no" it means will hide the one
  that matters.** `verify.py --all` reported six routines as NOT VERIFIED. Five
  were specs asking for an occurrence that never happens - the case above, a
  wrong question. The sixth was `select_music`, which *was* compared and
  **differed** in two DGROUP bytes: a port bug, in the sound path where no
  screen comparison can ever see it, filed among five non-events where nobody
  would look. `ok_all` is false for both and the table printed both the same.

  The same shape one layer down: a call that writes no hardware event, has no
  return to check and changes no memory agrees with everything.
  `compare_instance` says so - "this call did no work, so an agreement here is
  not evidence" - and the summary line ignored it, so a routine could have gone
  into STATUS.md as "agreed" on the strength of nothing at all. Both now say
  which they are. Measured afterwards: 410 verified and **none** of them
  vacuous, which is a fact worth having rather than an assumption worth making.

  Three times in one day a green result was compatible with something being
  wrong - a stale `shims.c` that `make` did not rebuild, a symbol table with a
  routine missing from it, and these verdicts. Ask what a pass would look like
  if the thing being tested were broken.

- **A routine that calls `dg_enter` needs `guest_sp` set, or it writes its
  locals over live memory.** In the large model SS and DS are one segment, so a
  routine building a structure on the stack hands out an ordinary DGROUP offset
  and the callee cannot tell it from a pointer to a global. A C local has none,
  so the port carries its own stack pointer and `dg_enter` reserves below it.
  `tools/verify.py` sets `guest_sp` at every entry and `dgroup.h` says so; the
  hybrid runner did not, and `load_bitmaps` - which reserves 0xa2 bytes - took
  the intro from identical to 76,817 pixels out the moment it was dispatched.

  The lesson is not the one routine. **Three routines already dispatched use
  `dg_enter`** and every green check they were part of had been luck: their
  frames happened to land on stack nobody was using. A caller that sets up less
  than the verifier does is not a lighter version of it, it is a different
  thing that agrees for a while. Auditing the rest of what `verify.py` sets -
  `dgroup_base`, the open files, the VGA registers and planes - found nothing
  else missing, and that audit is worth repeating whenever the port gains a
  new piece of state.

- **A sampled frame can only land on a phase that is a multiple of the step,
  and a screen that animates has phases in between.** Comparing the hybrid
  runner against the port every twentieth frame reproduced ten of the port's
  sixteen title-screen flips byte for byte and missed six. The six were exactly
  the phases no multiple of twenty falls on - a sampling artefact that reads
  like a blitter fault, and was written up as one. `TIM_FRAMES=<dir>:<step>` now
  takes `:<from>:<to>`, and every frame across a narrow window answers it
  without a gigabyte of pixels.

  The window has to be **wider than the flips it covers**, because the hybrid's
  frame numbering moves between runs, and never repeats: across eight runs the
  same port flip 4 came out as frames 300 to 320, no two alike, every one of
  them byte for byte identical to it. A window pinned
  to one run's numbering put five flips' true match past its edge, and the
  closest frame to each was the last frame in the window - which is what the
  edge of a window always looks like, and worth recognising on sight.

  **`TIM_FLIPWANT` used to drop silently past its sixteenth flip**, and that
  cost an hour of blaming the wrong side. Asking for `4,50..65` is seventeen,
  so flip 65 was never written however long the port ran, and the run read as a
  port too slow to reach it. A timeout was raised twice, "the port paces on a
  wall clock" was written into this file as the reason, and it was wrong: with
  the limit raised the flip arrives in under two minutes. It now aborts rather
  than truncating. A filter that quietly discards what it was asked for
  invents a symptom on the far side of whatever it was filtering for.

  So a screen is proved as a **run of consecutive flips**, not a frame. One
  frame can agree by luck on a screen that is mostly one colour; fifteen in
  sequence, each byte for byte and in order, is the animation. Requiring one
  named flip also makes the tool report the port's own pacing as a difference.

- **Name a handler from the table that installs it, not from what it seems to
  do.** `region_cursor_restart` was named from the state numbers next to it; the
  region it actually belongs to is the one whose +0x10 is 0x400, which is *enter
  freeform*. The restart region has no handler at all. Named right, the routine
  makes sense - it has a cursor outside freeform and none inside, the opposite
  of its four siblings, because that is when its button does something - and
  under the wrong name that symmetry is invisible.

- **A red line from the verifier means one of two things, and they look
  identical.** Either the transcription is wrong or the *spec* is wrong, and
  the second is far more likely on a routine's first run. `long_multiply`
  reported NOT VERIFIED across 6,477 calls because its spec lacked `near=True`;
  the linker pulled `__LMUL` into the image twice and the two copies end `ret`
  and `retf`, one byte apart. `heap_init` reported NOT VERIFIED because the
  spec asked for occurrence 1 of a routine called once. `load_screen` reported
  307,311 differences because three once-only routines were asked for a second
  call. Read the parenthetical - "only 1 calls seen", "never reached: 1" -
  before touching any C.

  So read every entry and every return rather than inferring a convention from
  the family. Ten allocator routines use four different argument conventions;
  `poly_edge_vertical` disagrees with the four other edge routines about which
  register holds which end; `set_cursor` takes its hot spot **y before x**.
  Each of those, assumed, produces a failing spec that reads as an accusation
  against correct code.

- **Two wrongs cancelled, and only some of the callers were wrong.**
  `regions_handle_pointer` opened with `si = DGU16(list)`, but the original is
  `mov si, [bp+6]` - the argument *is* the first record. Two of the port's
  seven callers passed the address `0x4e79` and the extra dereference made them
  right; the other five passed the value, and silently **skipped the head of
  their list**.

  It surfaced as the level-complete dialog: hovering ADVANCE did nothing,
  because ADVANCE is filed at the head and REPLAY is second, so the walk began
  one record too late and found only the button nobody was pointing at. Every
  screen comparison passed throughout - briefing, picker and level are all 0 or
  odometer-only - because none of them clicks a head region.

  Two things would have caught it earlier. The verifier prints the argument, and
  it was `list=0x704a` - a region record, not `0x4e71`, which is what a word's
  address would look like. And the image settles it in one grep: all seven call
  sites are `push word ptr [0x4e7x]`, six far and one near thunk inside
  `run_machine_loop`. **When a routine's callers disagree about a convention,
  one group is wrong - go and count the pushes rather than making the callee
  accept both.**

- **A fact about one driver, written into the code that calls all of them.**
  `SPKR:0x037a` is the speaker driver's do-nothing entry and seven of its
  eighteen table slots point at it - including **entry 8**, because a speaker
  has no patches and a program change really is nothing there. The speaker was
  transcribed first, and that fact went into `sound.c` as `driver_nop()` with
  the comment "Entry 8 of the driver's table is the do-nothing stub."
  `sound.c` is device-independent. `ADL:` sends entry 8 to 0x1a1b and `SBP:` to
  0x1a20, both of which file the patch per channel; their stub is elsewhere.

  So every channel kept whatever patch it started with for three years of
  commits. The sequencer parsed each program change, filed it, and dropped it.
  Patch loads came out at 8 against the original's 20 over the same 43 notes,
  and the music played in two timbres where it should have had eight - while
  **the note writes were identical**, 35 fnum and 79 key, which is why anything
  looking at notes saw nothing wrong.

  Four checks passed over it at once: `verify.py` had no spec for two of the
  three call sites, the hybrid *dispatches* `sound_service` so the emulator
  never runs them and cannot trap, no screen comparison can hear, and
  `check_sound.py` tests the digitised path. **When a routine is a no-op
  because one implementation makes it one, say which implementation** - and put
  the test in the dispatcher, not in the caller.

- **A short budget and a routine nothing calls give the same verdict.**
  `--only` defaults to 40M instructions and the polygon filler is not reached
  until past 90M, so three routines `reached.py` had already shown to run came
  back "TRANSCRIBED, NEVER CALLED". Believe that verdict only when a second
  measurement agrees - `poly_outline`'s does, because 0x1f219 is absent from
  `reached.py`'s set too.

- **"Never called" measured on four levels is a statement about those four
  levels.** The polygon clipper was written up as reachable by nothing in this
  repo, on three measurements that all looked sound: instrumenting
  `draw_polygon` and `clip_polygon` and playing level03, level12, level22 and
  level28; `verify.py` from the entry point at a 150M budget; and `verify.py`
  from each of the three snapshots. All three said never. Its one caller is
  `draw_part_extra`, which a **kind-0x1e** part draws - and levels 14, 15, 16
  and 19 have one. Instrumented there, both routines are entered on the first
  pass and both levels still solve, so the conversion was covered by
  `check_solutions.py` all along and the commit that said otherwise was wrong.

  Two lessons, and the second is the one that cost the hour. A screen or a
  level is a *sample* of the game's data, and a routine that draws one part
  kind is reached only where that kind is; `TIM_LEVELSCAN` exists to answer
  which levels those are and should be the first thing consulted, not
  four levels picked because they were to hand.

  And **`TIM_LEVELSCAN` was lying.** `load_level` allocates a record per part
  and the scan freed nothing, so the heap ran out around the twelfth level and
  every level after it printed `parts 0  kinds` - indistinguishable from an
  empty level, and no level is empty. That zero is what hid levels 14 to 16.
  It now stops and says the scan failed and how to continue, because a tool
  that cannot do the whole job must say so rather than produce a plausible
  answer for the part it managed. `round_teardown` is not the fix and makes it
  worse: it frees the lists but not the per-kind bitmaps, and the loader then
  fails four levels sooner.

- **A Unicorn read hook over a `uc_mem_map_ptr` region breaks the guest.**
  `TIM_SLOTS` was built to watch stack traffic and the first version watched
  reads as well as writes. With a callback whose entire body is `return`, the
  game leaves the rails inside twelve frames and the backtrace reads
  `game_main+0x8` calling `part_flip_options`, which is not a call that exists.
  `UC_HOOK_MEM_READ_AFTER` does the same. The identical hook on
  `UC_HOOK_MEM_WRITE`, same range, same callback, runs clean. Bisected: a hook
  registered over a range it never fires in is harmless, so it is the firing.

  The one read hook `native.c` already had - `on_vga_access` - escapes this
  because it calls `uc_emu_stop` and never returns to running code, so it has
  never exercised the case that breaks. A mechanism that has only ever been
  used one way is not evidence that the other way works.

- **The hybrid cannot watch what it has dispatched.** A routine running as the
  port's C writes `guest_mem` directly and never passes through Unicorn, so no
  emulator hook sees it. That is worth stating before building any measurement
  into `tools/native/`: the hybrid observes the *emulated* side, which shrinks
  every time a routine is transcribed. `TIM_SLOTS` was built to find which
  locals a callee reaches into, and the reaches that motivated it -
  `draw_polygon` walking three points from an address `draw_part_extra` handed
  it - are all in dispatched code and none of them appear. The same
  bookkeeping inside the port's own `DG*` accessors, where `dg_enter` already
  knows the frame, is what covers that half.

- **STATUS.md's table is only as fresh as the last `--all` sweep, and it can
  say "agreed" about a routine that no longer does.** `buffered_read` and
  `stdio_fread` are recorded there as agreed and both DIFFER now - by their
  return value, `original AX=0x0000 port=0x0004` and `original AX=0x0001
  port=0x0000`. Tested at HEAD and at a commit before this session's frame
  work: the same numbers, so it is older than either. Nothing was watching,
  because the table is written by the sweep and read by people.

  The useful habit that came out of it: when a routine reports DIFFERS after a
  change, **check the same routine at HEAD before believing the change caused
  it.** That is one run, and here it turned an assumed regression into a
  finding about the table.

- **`dg_off` on a pointer that is not in DGROUP is a number, and the compiler
  will hand it to you without complaint.** Converting a routine's frame to a
  `uint8_t frame[N]` makes its slots C-stack pointers. Where such a slot is
  passed to a routine that still takes a DGROUP offset, the build fails with
  "makes integer from pointer" - and wrapping the argument in
  `dg_off(dgroup, x)` makes that error go away while computing the distance
  between two unrelated objects. `dg_off` takes a `void *`, so nothing objects.

  **Forty-one call sites were "fixed" that way in one sitting and every one was
  wrong**, with a clean build at the end of it: `copy_file_record`,
  `read_bmp_info`, `far_memcpy`, `huge_move`, `draw_scroll_text`,
  `read_resource` - each writes through the address it is given, and would have
  written into whatever that arithmetic pointed at. `read_record`'s own comment
  says it out loud: "their addresses are handed to `read_resource` as
  `SS:offset` - which in this program is a DGROUP address."

  So **the callee's signature decides, never the compiler's silence.** A slot
  may only become a C local when every routine it reaches takes a pointer;
  `tools/framify.py` reads the prototypes and refuses on the rest, and the
  count of convertible routines went from 43 to 1. That is the true number, and
  it was 43 only because the compiler was being asked a question it cannot
  answer.

- **A frame slot whose value is filed into DGROUP must stay an offset, and
  getting that wrong reads exactly like the timer defect.** Converting a
  routine's `[bp-N]` locals into a `uint8_t frame[N]` turns each slot into a
  host pointer. `vm_init` stores its frame pointer into `DG618A.fonts_off`,
  which the guest reads back, and `draw_compressed_bitmap` stores one slot's
  address into another - so both filed a truncated host address where a DGROUP
  offset belongs.

  The symptom was **one level in ten failing to solve, a different one each
  time**: level08, then level06, then level01. That is indistinguishable by
  eye from the non-determinism this file already documents, and it cost three
  ten-minute batches and two wrong theories - a broken `parts.c`, then CPU
  contention from running other things beside the check - before the control
  that settles it: **stash the work and run the batch at HEAD.** HEAD passed
  10 of 10, so the fault was mine, and after that it was a diff to read rather
  than a run to repeat.

  Two lessons. When an intermittent failure appears, get the control before
  the theory: the question "does this happen without my changes" is one run and
  it ends the argument. And a converter that hands out pointers needs to refuse
  any routine that stores one where the guest will read it - the test is
  whether the address is only *read through*, never filed.

- **Two tools that agree can share a blind spot, and then the agreement is
  worth nothing.** The frame work has a census - which routines can be
  converted - and a converter. Both decided what a routine's "slots" were by
  looking for `uint16_t v = fp + k` declarations. `draw_counter_word` names its
  frame `buf`, takes no such declaration, and hands `buf` straight to
  `int_to_string`; both tools called it slotless, so the census said it was
  unblocked and the converter cheerfully converted it into a pointer being
  passed to a routine that still wants an offset. The two agreeing was not
  evidence, because the same misreading was in both.

  The same census had already been wrong the other way: it took a callee's name
  from the *line* a slot appeared on, so a call whose arguments wrap came back
  as "?" - 41 of 87 frames, and the true count of convertible ones was 36
  rather than 16. It said too few, then too many, for two unrelated reasons.
  A worklist is a measurement and deserves the same suspicion as any other.

- **A struct field is a claim about width, and a narrower one is a short read
  that compiles.** Turning `DG*(base + offset)` into a named field replaces an
  access whose width is written on it with one whose width is written somewhere
  else, and the two can disagree in only one direction safely. A *narrower*
  accessor on a wider field is fine and always was - `DG8` on a word is reading
  its low byte, which is what the original does. A *wider* accessor on a
  narrower field is a different read: `DG32(si + 0x16)` became
  `((int32_t)PART(si).word_16)`, two bytes instead of four, and three levels
  stopped solving because that is a part's momentum. The cast is what makes it
  invisible - it looks like the widening the surrounding arithmetic wants.

  The same one size down had already gone in unnoticed the commit before:
  `reverse_link_ends` swaps +0x6a with +0x6c using a 16-bit move, and a pass
  with no width check turned that into a swap of `byte_6a` and `byte_6c` -
  half of each pair. **It solved 29 of 29 anyway**, because reversing a run of
  pulleys is on no solution's path, and that is the more useful half: the
  behavioural checks cover what the levels do, so a defect off that path can
  sit in the tree indefinitely with everything green.

  So a converter must **refuse** a site it cannot spell at the right width and
  name it, never cast it. And where two byte fields are moved as a word - a
  part's grab box at +0x56, its two attachment offsets at +0x6a and +0x6c -
  the pair is the type: `struct byte_pair`, so `clone_part` stays the three
  16-bit moves the original makes.

- **An empty evidence set is not evidence for the wider type.** `framify.py`
  chose a frame slot's C type from the accessors it could read: all `DG16` at
  even offsets meant `int16_t *`, anything else meant `uint8_t *`. A slot with
  **no** readable accessor satisfies "all of them are 16-bit" vacuously, and
  six slots were typed `int16_t *` on the strength of nothing at all - the same
  shape as the vacuous verdicts further up this file, one layer down.

  Two of the six were live. `path_join` filled its filename buffer with
  `DG8((uint16_t)(name + di))`, which on an `int16_t *` truncates a **host**
  pointer to a DGROUP offset and writes somewhere else entirely, so the
  `strcat` that followed appended an uninitialised frame. `picker_type` did the
  same with `DG8((uint16_t)(str + 1)) = 0` and typed characters into the picker
  without their NUL. Both compiled and both had been in the tree for days.

  The compiler said so every time: `-Wpointer-to-int-cast`, exactly the
  diagnostic for this, on both lines. It was invisible because the build check
  in use was `make 2>&1 | grep -E "error"`, which is a filter that removes the
  class of finding it was looking for. **Grep the build for `error|warning`,
  never `error` alone.**

  The reason a byte slot even reaches the compiler half-converted is that
  `framify.py` rewrote the accessors of word slots and left byte slots to
  `framify_fixups.py`, which is per-function and missed one. It now converts
  `DG8`/`DGS8` on byte slots itself, refuses a slot read at a variable index
  with a width above a byte, and requires a non-empty width set before it will
  say "word".

- **A check that does not build its own references compares two different
  ages of the code.** `tools/native/native` links the port's `.c` files into
  the hybrid, and `check_native.py` only checked the file *existed* - so
  editing the port and running `make -C reconstruct` left the hybrid at
  whatever it was last built from, and the check compared this hour's port
  against last week's.

  That hid a real defect for four commits. `read_into_huge` reserves four
  bytes and writes a far pointer into them, `DG16(fp)` and `DG16(fp + 2)`;
  the converter rewrote both onto a `uint8_t *` as `(*fp)` and `fp[2]`, which
  stores two low bytes and never writes the segment at all. It went in under
  "all 66 intro flips are byte for byte" - true of the hybrid that was on
  disk, which predated the routine. Three commits later an unrelated rebuild
  turned all 66 red at once, which reads as a regression in whatever was
  committed *last*, and the bisect that found the real commit cost eleven
  builds.

  `make test` was clean throughout and 29 of 29 levels still solved: the
  routine reads bitmaps, and a level's parts come from elsewhere.

  `check_native.py` now runs `make` for both binaries itself. That is the one
  moment a build is safe - before anything is running - and the standing rule
  about not rebuilding is about a build *during* a run.

  **It is not one check, and `make` alone does not settle it.** `make` builds
  the binaries and does *not* build `libtim.so`, so which reference a session
  has just rebuilt depends on which `make` it happened to run. An hour after
  the above, `verify.py` reported `parse_open_mode` DIFFERS over five calls -
  the port returning 0 and writing nothing - against a library that still took
  offsets where the spec had started passing pointers. Every reading of that
  is a plausible story about correct code: a wrong spec, a wrong argument
  order, a seeding problem. Built first, it verifies over 83 calls. And three
  routines had been called "verified" that same hour on a library that was not
  running their new code at all, which is the same staleness answering green.

  `tools/tim.py` now has `built(what, where)`, and every check goes through it.

  And that closes one hole and opens a smaller one: **now that the checks build
  for themselves, editing a `.c` while one runs is what rebuilding used to
  be.** A `check_solutions.py` run in the background came back "Error 1" from
  `make` and gave no verdict at all, because the edits for the next round had
  landed in the tree while it was between levels. The rule is the same rule -
  one thing at a time - but the trigger is now the editor and not `make`.

- **A frame stops being convertible for three reasons, and only two are about
  the code.** Converting a `dg_enter` frame to a `uint8_t frame[N]` needs every
  callee it hands a slot to to take a pointer. Where that is not possible it is
  because the callee needs a *guest offset*, and the offsets have three
  different origins:

  - **far** - the value is half of a `seg:off` pair. `draw_string` hands its
    string to `draw_string_body(str, DGROUP_SEG, ...)`, which reads it with
    `FAR8(seg, str)`. Note that the pair can be *named* rather than
    dereferenced: `read_resource` takes `dst_off, dst_seg` and passes both on
    without a `FAR8` anywhere in its body.
  - **filed** - the address is stored into guest memory and outlives the call.
    `stdio_setvbuf` puts the buffer into a file record's `read_ptr`, read back
    later as a DGROUP offset. A C array has no offset to store.
  - **polymorphic** - the value is a handle *or* an address, told apart by a
    numeric test. `load_bitmaps` asks `file_record_valid` whether its argument
    matches an open record's `file_ptr`; a C array's `dg_off` is an arbitrary
    16-bit number that could match a live handle, and no pointer type can
    express the choice. `call_sound_module` is the same shape from the other
    end: its second argument is read by the module's own emulated code through
    SI.

  The first two `framify_census.py` derives; the third is a short by-hand list
  with a reason each, because a wrong automatic verdict is worse than a named
  exception. Measured on 2026-09-09, with the mechanical work finished, the
  census accounts for every frame left and the sum is printed so it cannot
  quietly stop adding up: **8 + 0 + 17 + 2 = 27**. Eight have no blocking
  callee and are refused by `framify.py` over their own slots - four reach
  `draw_string` or `far_copy` through a cursor, four file a slot's address.
  None is waiting on work. Seventeen are behind one of the three walls. Two -
  `game_screen` and `poll_sequences` - reserve DGROUP stack with no slots of
  their own, the first so its callees' frames land below it and the second
  because the sound module reads its block through SI; neither goes until what
  is under it stops needing DGROUP.

  **The far wall came down first, and it was the model that was already
  chosen.** "Offsets from the dgroup for near pointers, and offsets from 0 for
  far pointers" is the instruction this work started from, and a far pointer as
  an offset from 0 *is* a host pointer into `guest_mem`. So `dg_far`/`dg_cfar`
  join `dg_near`/`dg_cnear` in tim.h: one C parameter where the guest pushes
  two words, offset then segment, which is what `aptr` in the hybrid's shims
  already builds. `draw_string_body` took `(str, seg)` and takes one pointer;
  `draw_string` and `draw_scroll_text` follow, and seven frames behind them
  convert. There is deliberately no inverse of `FAR_PTR` - a host pointer does
  not remember which of the many `seg:off` pairs that address it the guest was
  holding.

  One thing to transcribe carefully on the way: `draw_string_body` opens
  `if ((str | seg) == 0) return;`, and that is a far pointer of 0000:0000,
  which is `guest_mem` and **not** a C null pointer. Written as `str == NULL`
  the guard never fires.

  What is left needs the other half of the decision: whether a frame may stay
  in DGROUP, which is what a filed address and a polymorphic handle both come
  down to.

  **And a refusal that names the wrong wall points at the wrong fix.**
  `framify.py` reported four routines as filing a slot's address, on the
  strength of `uint16_t si = buf;`. They do not: `si` walks the buffer and is
  handed to `draw_string`, which is the *far* wall, and the assignment only
  looked like filing because `si` had failed the cursor test and so was not
  recognised as derived from `buf` at all. A rejected cursor candidate now
  carries the callee that rejected it, and the refusal says "buf reaches
  draw_string through si". Four of the eight filed verdicts were this;
  the other four - `decode_vqt_list`, `draw_compressed_bitmap`, `vm_init`,
  `blit_scaled_a` - really do file a slot address, which is the finding
  recorded further up this file.

- **A spec that was right becomes wrong when the routine's arguments change,
  and nothing links the two.** `int_to_string`, `long_int_to_string` and
  `long_to_string` were converted to take `dg_near buf` days before their
  `verify.py` specs were looked at; the specs went on passing
  `ctypes.c_uint16(offset)` into a parameter that is now a pointer. Two of the
  three then had no verdict at all - the sweep died in collection with no
  message - and `string_copy_padded`, converted the same hour, reported
  DIFFERS with **the memory identical and only the return value out**: the
  original answers `0xff90` and the port the low half of a host address.

  Three separate spellings, one cause. So when a routine's signature changes,
  the spec changes with it, in all three places: `dgp` on every argument that
  became a pointer, `dgo` around a return that became one, and the routine's
  name in the `restype = c_void_p` list - a returned pointer with no restype
  comes back truncated and `dgo` cannot undo it.

- **Do not rebuild anything while a check is running.** `cc -o` rewrites the
  file the running process has mapped; the sweep drops to 0% CPU and is lost.
  This was written for `libtim.so` and the verification sweep, and it is the
  same for `devtim` and `tim`: editing dgroup.h and running `make` while
  `check_solutions.py` was in flight made level28 report a **600-second
  timeout**, which reads exactly like a level that stopped solving. Re-run with
  the tree left alone: 29 of 29. A check that says the port broke is worth one
  look at what else was running at the time.
  Editing the `.c` is safe, `make` is not. And **a header-only change is when
  to distrust the build**: `libtim.so` and the binaries listed only the `.c`
  files, so raising a constant in `io.h` rebuilt nothing and the next run used
  the old library - silently, and answering with complete confidence. A whole
  finding was written up from that stale result, retracted only when an
  unrelated edit forced a rebuild an hour later. The rules now depend on
  `$(HEADERS)`; `touch reconstruct/io.h` should rebuild.

  And **`make` outside `reconstruct/` builds nothing and says nothing.** There
  is no Makefile at the repository root, so `cd $REPO && make` prints "no
  makefile found" and carries on; a `| grep error` after it reports success. It
  cost one stale-binary measurement here. Every check target depends on
  `devtim tim`, so `make test` does rebuild - but a bare `make` from the wrong
  directory is a silent no-op.

  And **keep the pid of what you launched; do not go looking for it again.**
  `$!` is right there. Every pattern search for a run this session was
  unnecessary and two of them were self-referential: `pkill -f "only
  poly_walk"` matched its own command line and killed the shell before the
  redirect was opened, and `until ! pgrep -f "only load_all_parts"` waited for
  itself and never fired. Both read as the tool misbehaving rather than the
  pattern matching the watcher.

      uv run python tools/verify.py --all > out/sweep.log 2>&1 &
      pid=$!

  `kill $pid` and `kill -0 $pid` are then unambiguous. They are still racy if
  the wait outlives the process, because the pid can be recycled and `kill -0`
  will answer about somebody else; a **pidfd** is the race-free handle and this
  kernel has it. From Python, which is where the tools live:

      fd = os.pidfd_open(pid)      # a stable reference, not a number
      select.select([fd], [], [])  # readable exactly when that process exits

  Waiting on the output file works and is what the earlier note recommended,
  but it answers a different question - "has it written anything" rather than
  "is it still running" - and it cannot tell a finished run from a killed one.

- **The hybrid's frame digests cannot be compared between two runs, and a
  virtual clock did not fix it in one sitting.** `check_native` aligns content
  across a window and demands a run of consecutive flips; that is not fussiness,
  it is the only thing that works. Compared frame for frame, *the same binary
  run twice* agreed on 23 of 400 digests in order, with three distinct frames
  unique to each side - so a real difference of that size is invisible, and an
  apparent one means nothing. Original against port scored 14 of 400 and was
  inside the noise.

  The fix is obviously to drive the clock from the instruction count, as
  `tools/drive.py` does for the Python emulator, and `io_now` has only two
  callers - the present rate limiter and the vertical-retrace phase the guest
  polls. It is still not as simple as swapping them:

  - Advancing the clock **per slice** is pathological. The guest polls the
    retrace bit in a tight loop, the bit cannot change until the slice ends, so
    every wait burns its whole 200,000-instruction budget. Forty frames took
    four minutes.
  - Advancing it **per block** is the right granularity, but the main loop
    services the display once a slice, so virtual time runs ten frames ahead of
    the frames actually presented and the guest waits ten times too long for
    each tick. Sixty frames were instant and then it fell off a cliff.
  - Presenting from the block hook, and separately shortening the slice to one
    frame, both cleared the cliff at sixty and still could not reach 400 frames
    in 120 seconds - where the host clock does it in 7.4.

  So the frame, the tick and the slice are entangled with wall time in a way
  that wants untangling deliberately, not as a flag bolted to the side. Worth
  doing; not worth shipping half-done, and the attempt is recorded here rather
  than left in the tree as a mode that hangs.

- **Two references agreeing is not corroboration when they share a bias.** The
  port's music was measured as running three times too fast, twice, against
  two independent references - the hybrid at 52 key events a second against the
  port's 151, and DOSBox at an envelope period of 0.406s against 0.139s. Both
  said x2.9 and both were wrong the same way: they are *wall-clock*
  comparisons, and both references are slower machines. DOSBox's tempo never
  plateaus with cycles - 0.406s at 8000, 0.075s at 30000, 0.046s at max - so it
  measures the machine, not the game.

  The answer came from a quantity with no clock in it. Both sides push page
  flips and OPL writes through the same `io.c`, so **notes per flip** is a pure
  guest-side invariant: if the whole game runs slow, both slow together. At 138
  matching flips the port is 1.61 and the original 1.74, a ratio of 1.05. The
  music keeps step with the game's own pacing.

  The port's speed is then the game's own arithmetic: PIT divisor 5041 is
  236.7 Hz, `game_screen_loop` waits 8 ticks, two pages are presented per
  iteration - 59.2 flips a second against 57.5 measured.

  So when a timing claim rests on comparing two runs, ask what quantity could
  not contain the bias, and measure that instead.

- **The hybrid's music runs on a different clock from its samples, so the two
  cannot be compared.** `native.c` deliberately does not call `io_set_timer` -
  int 8 arrives between emulator slices - so the tempo runs at emulation speed,
  while the Sound Blaster's completion interrupt comes off `io_now`, which is
  wall-clock. Measured: 52 key events a second against the port's 151, so five
  notes play inside the 0.518-second sample where the port plays twenty-one.

  A sequence whose removal waits on that interrupt is therefore live for a
  different number of notes on each side, the playing table differs by
  construction, and voice allocation - which reads the table - differs
  downstream. The visible end of it is two notes swapping voices, which looks
  exactly like an allocator bug and is not one. A whole session went into
  chasing it as a transcription error.

  The digitised comparison is unaffected because it never consults the tick,
  which is why `check_sound.py` gives a verdict and `--fm` gives a 2.

- **The sampling trap is not about frames.** It is written up above for page
  flips, and it was met again in the sound work with no frames anywhere near
  it. Printing a sequencer byte six times on each side gave

      port    00 00 00 81 00 00
      hybrid  00 00 00 00 00 00

  and that reads as "the port asks the module for something the original never
  asks for". It is not a comparison at all: the hybrid runs the guest under
  Unicorn and covers far less game per second, so its first six calls are not
  the port's first six. At 400 samples both sides show the same marks and the
  finding evaporates. Counts do not survive either - the two runs were 40 and
  180 seconds of different amounts of game.

  So: **the two sides share no clock and no index, and anything compared
  between them must be aligned by content** - the way `check_native.py` aligns
  flips and the way the trace-diff aligns register writes. An index is not an
  alignment. A whole commit went in on that reading before the wider sample
  retracted it, which is the second time in this project that a green-looking
  measurement was compatible with the opposite being true.

- **An interrupt is exclusive; a thread is not.** The port runs the guest's
  INT 08h handler on a pthread, and that is not the same machine. On the
  original the tick *suspends* the interrupted code and runs to completion on
  the one CPU, so two pieces of guest code are never inside the driver's
  drawing state at the same instant. The port lets them be, and the driver's
  state is a handful of DGROUP words - the clip box at 0x3894..0x389a, the two
  page pointers at 0x38a6/0x38a8, saved and restored through a **single** slot
  at 0x5726..0x5732.

  What that costs, seen while playing: `timer_callback` reaches
  `redraw_cursor` and then `draw_cursor`, which opens the clip wide - 0 to the
  screen's size - draws, and puts the old clip back. On the original an
  interrupt between "set the clip" and "blit" is harmless, because the handler
  restores what it found. Concurrently it is not: the main thread can be
  *inside* a blit, reading those words, while the timer thread rewrites them.
  The blit then escapes its clip.

  It shows as a stray column of odometer digits running out of the counter
  strip and down into the play page - `draw_odometer_digit` draws the whole
  five-digit strip at once and relies on the clip to box it, so an escape is
  the entire strip. Rendered out of a capture the column is contiguous from
  video memory row 70 to about 120, straight across the page boundary at row
  80, which no correct draw can be. Cursor bitmaps leak the same way; it is
  whatever was being drawn when the race landed.

  **The hybrid never shows it, and that is the control that settles it.**
  `tools/native/native.c` deliberately does not call `io_set_timer`: it
  delivers int 8 between emulator slices, serialised. The same C, the same
  drawing, no thread - and no artefact.

  **Locking the blits is not the fix, and it is worth saying why before
  somebody tries it.** The clip is only the visible half. `timer_callback`
  reads and writes a good deal of shared DGROUP besides - the pointer at
  0x576c/0x576e, the button accumulators at 0x5768/0x576a, its own guards at
  0x5740 and 0x5752 - and `timer_tick` below it steps the frame counter at
  0x44ef and raises `frame_flag` at 0x5754. Every one of those is read by the
  main thread with nothing between them.

  Two of those reads are the frame pacing, and they are spins:
  `while (0x2710 - DGU16(0x44ef) < 8);` in `game_screen_loop`, and
  `frame_pending`, which `wait_and_latch_frame` turns on the spot. `DGU16` is a
  plain read through a pointer into `guest_mem`, not a volatile one, so those
  loops are a data race that a compiler is entitled to hoist out of the loop
  entirely. They work today; nothing says they must.

  So this wants **a model, not a mutex**, and the model is not chosen yet. The
  honest options run from "make every tick a message the main thread drains at
  a safe point", which is what the hybrid already does by accident, to "give
  the guest's memory the atomics its concurrency now implies". Both are
  bigger than the artefact that exposed them.

  **The cost is not small, and that was measured on 2026-09-07.** It had been
  written up here as a stray column of odometer digits. It is also the machine
  itself: eight runs of the port on one level, from one machine file, produced
  **five distinct frame sequences**, and two full sweeps of `check_machines.py`
  minutes apart disagreed about *eleven of twenty-eight levels*. The frames
  differ across rows 25 to 358 - the whole play area, 131,773 pixels at one
  flip - not in a counter.

  The mechanism is not the clip box this note opens with. `run_machine_loop`
  waits for *at least* eight ticks and then accumulates however many actually
  went by; on a real-time thread that number depends on when the scheduler ran
  it, and the simulation takes a different path from there. The hybrid, whose
  ticks are a fixed 3.95 per present, is byte for byte identical across runs -
  which is the control that puts the fault on the port's side rather than
  between them.

  So the port's machine simulation **cannot be compared with anything**, by
  this project or by anyone else, until the tick is deterministic.
  `check_machines.py` is written and waiting for that day; nothing in it has to
  change.

  **Still deferred, and still a decision to be made rather than a lock to be
  bolted on.** `io_lock` and the recursive mutex `timer_loop` already holds are
  the pieces a real answer would probably reuse.

## Tools

Everything reaches the shared emulator through `tools/tim.py`, never by
importing `dos_emulator` directly, so that when the shared code moves there is
one file to fix. The emulator is pinned to a commit in `pyproject.toml`; moving
the pin is a deliberate act and the verification sweep is re-run afterwards.

| tool | what it is for |
| --- | --- |
| `tools/tim.py` | the one local door to the emulator: game directory, paths, and `TimMachine`, the machine as this game expects to find it |
| `tools/unlzexe.py` | recovers `TIM.EXE` by **running** its LZEXE stub, and measures the relocation table by running it at two load segments and diffing |
| `tools/verify_unpack.py` | proves the recovery: loading the emitted EXE must put exactly the stub's bytes at exactly its entry and stack |
| `tools/disasm.py` | disassembles the recovered image, annotating DGROUP string references |
| `tools/run.py` | runs the game under `TimMachine`; every shared-emulator flag works |
| `tools/png.py` | PNG writing and palette conversion, standard library only |
| `tools/drive.py` | the shared run loop, and the **virtual clock** that makes a run reproducible |
| `tools/capture.py` | reference frames, captured on the guest's own page-flip cue |
| `tools/diff_png.py` | the three-image comparison; always look at the images |
| `tools/codemap.py` | recursive descent from the entry point; `--run` adds what the game reached |
| `tools/reached.py` | which routines a given stretch of the game executes, delimited by page flips; `--audit` says which of them `verify.py` has a spec for, and which rest on the screen comparison alone |
| `tools/resources.py` | reads and extracts the resource archive |
| `tools/dgrules.py` | **what the DGROUP structs have not swallowed yet**, over a tree-sitter parse rather than a regex, because both its rules are about *shape*: `raw` lists every remaining `DG*` accessor split by constant offset - which a field can replace - against computed, which is a record needing its type known first; `offset-arg` finds a near pointer hidden as arithmetic in an argument, `game_fread((uint16_t)(0x627a + si), ...)`, which `dg_off(&STRUCT.field[si])` says better. Neither is a failure; both are a worklist, sorted so the biggest cluster is the next struct to write |
| `tools/frames.py` | **what each routine reserves for its locals**, from the binary's `sub sp,N` and from the port's `dg_enter(N)`. The two should agree, and where they do not the report says which of the two rules the port followed. It also flags a named slot at or past the frame's end, which cannot be a local |
| `tools/framify.py` | turns a routine's `dg_enter` frame into the `uint8_t frame[N]` it is, one named routine at a time. **Its refusals are the point**: a slot whose value is *filed* anywhere rather than only read through, a slot spelled in a way it cannot read, a `bp` that derives nothing. Each was written after that shape broke something |
| `tools/framify_fixups.py` | the shapes a frame conversion leaves behind - an unsigned read used as an lvalue, a `dg_ptr` on something that is already a pointer, a byte slot still read with `DG8`. Per *function*, because slot names are per function. **The `(?!=)` on every write rule is the one thing to get right**: without it a comparison `DG16(x) == 0` becomes `dg_wr16(x, = 0`, which has broken the build three times from three hand-retyped copies |
| `tools/framify_census.py` | which frames `framify.py` can take, and what each remaining callee holds up. Wrong three times in three ways before it was right, all three recorded in its own header |
| `tools/verify.py` | **proves one routine against the original**: stop at its entry, let the original body run, compare what each did to the hardware. `--click` drives it to screens behind the menu |
| `tools/check_briefing.py` | **proves a whole screen**: runs both sides from the entry point with the same clicks and compares settled flips. `--screen briefing\|picker\|save` |
| `tools/check_save.py` | **proves the file the game saves**, byte for byte. A machine file never reaches a pixel, so no screen comparison can see the writer |
| `tools/check_sound.py` | **proves the port plays the original's samples**, byte for byte. `--fm` asks the same of the music and answers **inconclusive**, because the hybrid's tick runs at emulation speed and the card's completion interrupt does not. No screen comparison can hear, and the play path is a loaded module doing port I/O, so `verify.py` cannot reach it either. Aligned by content: a looping sample repeats until the next trigger, so the counts differ and the *sequence* does not |
| `tools/fixture.py` | a game directory with the things the real one happens not to have - a subdirectory, a `password.txt` - so the routines behind them can be reached at all |
| `tools/native/` | the **hybrid runner**: the original binary under emulation, with the port as its hardware and, routine by routine, as its code. Anything not yet dispatched *traps* - `int 21h`, the A000 aperture, a VGA port - and names the next routine to write, with a guest backtrace. `routines.def` says where each routine is and how it is called, and the shims and the symbol table are generated from it. `TIM_NATIVE_LAYERS=io` runs the port as the *machine* only - VM.OVL, SX.OVL, the Borland runtime, memory, the mouse, the timer - with the game's own code left to the emulator. The sound driver is the one hook not in `routines.def`: it is entered at a single run-time far pointer with a function number in BP and answers in two registers, so `dispatch.c` binds and shims it by hand, and a run reports which of the eighteen functions were reached; each routine's layer is derived from the port file that defines it and from whether its body touches a port, with a short list of exceptions in `genshims.py` for the ones the original filed in a game segment |
| `tools/check_native.py` | **proves the hybrid draws what the port draws**, as a run of consecutive flips each byte for byte. Content-aligned, never flip-numbered: the two sides' clocks are nothing like each other |
| `tools/native/covered.py` | **how much of one routine a run executed**, with `TIM_COVER=<lo>:<hi>:<path>`. A routine verified on one path is not verified: `verify.py` says the compared calls agreed, this says how much of the body they went through |
| `tools/native/iogap.py` | **which transcribed routines will trap if the guest runs them**: every routine whose own body does port I/O, and whether it is dispatched. The trap finds these one at a time by playing; this finds them all at once |
| `tools/native/coverage.py` | **how much of a stretch runs the port's code and not the original's**. A matching screen proves the routines that drew it and nothing else; this says which of the ones a screen reaches are still the original's, and every one of them already has a body in the port |

## What is not being reconstructed, and why

Recorded as deliberate non-goals in `STATUS.md`, and in the port as no-ops with
a comment saying why - never as gaps waiting to be filled, and kept out of the
verifier's dispatch so a decision is not reported as a difference.
