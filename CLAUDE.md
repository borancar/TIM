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

- **`TIM_FLIPS=<dir>:<last>` is a stopping point, not a filter.** It writes a
  308 KB frame for *every* flip up to `<last>`, so a run to flip 800 leaves a
  quarter of a gigabyte behind. Reading it as "write flip 800" has filled the
  disk twice, the second time after a note in `devdump.c` already recorded the
  first. `TIM_FLIPWANT=<f1>,<f2>,...` is the filter, and a comparison should
  always name the flips it reads.

- **A sampled frame can only land on a phase that is a multiple of the step,
  and a screen that animates has phases in between.** Comparing the hybrid
  runner against the port every twentieth frame reproduced ten of the port's
  sixteen title-screen flips byte for byte and missed six. The six were exactly
  the phases no multiple of twenty falls on - a sampling artefact that reads
  like a blitter fault, and was written up as one. `TIM_FRAMES=<dir>:<step>` now
  takes `:<from>:<to>`, and every frame across a narrow window answers it
  without a gigabyte of pixels.

  The window has to be **wider than the flips it covers**, because the hybrid's
  frame numbering moves between runs: the same port flip 4 came out as frames
  302, 309 and 319 in three runs whose pixels were identical. A window pinned
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

- **A short budget and a routine nothing calls give the same verdict.**
  `--only` defaults to 40M instructions and the polygon filler is not reached
  until past 90M, so three routines `reached.py` had already shown to run came
  back "TRANSCRIBED, NEVER CALLED". Believe that verdict only when a second
  measurement agrees - `poly_outline`'s does, because 0x1f219 is absent from
  `reached.py`'s set too.

- **Do not rebuild `libtim.so` while a sweep is running.** `cc -o` rewrites the
  file the running process has mapped; the sweep drops to 0% CPU and is lost.
  Editing the `.c` is safe, `make` is not. And **a header-only change is when
  to distrust the build**: `libtim.so` and the binaries listed only the `.c`
  files, so raising a constant in `io.h` rebuilt nothing and the next run used
  the old library - silently, and answering with complete confidence. A whole
  finding was written up from that stale result, retracted only when an
  unrelated edit forced a rebuild an hour later. The rules now depend on
  `$(HEADERS)`; `touch reconstruct/io.h` should rebuild.

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
| `tools/verify.py` | **proves one routine against the original**: stop at its entry, let the original body run, compare what each did to the hardware. `--click` drives it to screens behind the menu |
| `tools/check_briefing.py` | **proves a whole screen**: runs both sides from the entry point with the same clicks and compares settled flips. `--screen briefing\|picker\|save` |
| `tools/check_save.py` | **proves the file the game saves**, byte for byte. A machine file never reaches a pixel, so no screen comparison can see the writer |
| `tools/fixture.py` | a game directory with the things the real one happens not to have - a subdirectory, a `password.txt` - so the routines behind them can be reached at all |
| `tools/native/` | the **hybrid runner**: the original binary under emulation, with the port as its hardware and, routine by routine, as its code. Anything not yet dispatched *traps* - `int 21h`, the A000 aperture, a VGA port - and names the next routine to write, with a guest backtrace. `routines.def` is the only hand-edited list; the shims and the symbol table are generated |
| `tools/check_native.py` | **proves the hybrid draws what the port draws**, as a run of consecutive flips each byte for byte. Content-aligned, never flip-numbered: the two sides' clocks are nothing like each other |

## What is not being reconstructed, and why

Recorded as deliberate non-goals in `STATUS.md`, and in the port as no-ops with
a comment saying why - never as gaps waiting to be filled, and kept out of the
verifier's dispatch so a decision is not reported as a difference.
