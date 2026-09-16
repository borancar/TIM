# Working on this reconstruction

The Incredible Machine (Dynamix / Sierra, 1993), reverse engineered from
`incredible-machine/TIM.EXE` and reconstructed as C.

Two artefacts check each other: the **emulator** running the original binary is
the *reference* that defines what "correct" means, and the **C port** is the
deliverable. Neither is trusted alone. Nothing is finished because it looks
right on screen.

## Keep this file short

**This file is read at the start of every session, so it holds only what
changes how the work is done**: the rule below, the conventions, one line per
trap, and the tools table. It grew to 1,764 lines by accreting incident
write-ups, measurements and open questions, and was cut back to an index on
2026-09-14. Do not let it grow back:

- **A new trap** gets its full account - what happened, what it cost, what
  settled it - in `docs/lessons.md`, under the group it belongs to, and **one
  line here**: the rule, and a link to the account.
- **A measurement, a current state or an undecided question** goes in
  `STATUS.md`, usually under "Open". A number that will change is status, not
  a rule.
- **A fact about the binary** - a format, a driver, a calling convention -
  goes in the topic file under `docs/`.

If a trap's line here is turning into a paragraph, the paragraph belongs in
`docs/lessons.md`.

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
- **DGROUP is a byte array, and its names are objects the linker puts in it.**
  The game uses near pointers - a word in DGROUP holding an offset into
  DGROUP - so the guest's megabyte is one block, `guest_mem`, and a named
  struct is a C object placed at its offset by `DGROUP_AT(off)` (or
  `DGROUP_BSS`, or `SEGMENT_AT` for a code segment's own data), so a name and
  a pointer dereference reach the same byte. What the image held there is the
  object's initialiser, `LOAD_SEG + seg` for a relocated word. The video
  driver's data is part of the same segment, at offset 0x3890.
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

Each is a rule learned by breaking it. The full account - what happened, what
it cost, what settled it - is in `docs/lessons.md`, and the ones that are
still open are in `STATUS.md`. Read the account before relying on the rule in
a case it does not obviously cover.

### Reading and running the original

- A DOS program with `maxalloc = 0xFFFF` owns all of conventional memory - [more](docs/lessons.md#a-dos-program-with-maxalloc--0xffff-owns-all-of-conventional-memory)
- The game asks the BIOS what adapter it has - [more](docs/lessons.md#the-game-asks-the-bios-what-adapter-it-has)
- `files missing:` in a run report is usually not an error - [more](docs/lessons.md#files-missing-in-a-run-report-is-usually-not-an-error)
- Capstone's 16-bit mode gets `cbw`/`cwd` wrong - [more](docs/lessons.md#capstones-16-bit-mode-gets-cbwcwd-wrong)
- A segment immediate in the image is a relocation, not a value - [more](docs/lessons.md#a-segment-immediate-in-the-image-is-a-relocation-not-a-value)
- A jump that lands one byte past the last instruction you read means there is an instruction you have not read - [more](docs/lessons.md#a-jump-that-lands-one-byte-past-the-last-instruction-you-read-means-there-is-an-instruction-you-have-not-read)
- The annotator must only report the *start* of a string - [more](docs/lessons.md#the-annotator-must-only-report-the-start-of-a-string)
- The last argument pushed is the first argument - [more](docs/lessons.md#the-last-argument-pushed-is-the-first-argument)
- Name a handler from the table that installs it, not from what it seems to do - [more](docs/lessons.md#name-a-handler-from-the-table-that-installs-it-not-from-what-it-seems-to-do)
- Two wrongs cancelled, and only some of the callers were wrong - [more](docs/lessons.md#two-wrongs-cancelled-and-only-some-of-the-callers-were-wrong)
- A BIOS or DOS call the port cannot answer is stubbed, never dropped - [more](docs/lessons.md#a-bios-or-dos-call-the-port-cannot-answer-is-stubbed-never-dropped)
- A four-digit constant walked with a stride is an array of a record, and the record usually already has a type - [more](docs/lessons.md#a-four-digit-constant-walked-with-a-stride-is-an-array-of-a-record-and-the-record-usually-already-has-a-type)
- A fact about one driver, written into the code that calls all of them - [more](docs/lessons.md#a-fact-about-one-driver-written-into-the-code-that-calls-all-of-them)
- An array sized from the prose beside it, when the loop says otherwise - [more](docs/lessons.md#an-array-sized-from-the-prose-beside-it-when-the-loop-says-otherwise)
- A name that says `far` may be talking about the call - [more](docs/lessons.md#a-name-that-says-far-may-be-talking-about-the-call)

### Checks, measurements and verdicts

- A check that polls can miss what it is checking, and then blames the port - [more](docs/lessons.md#a-check-that-polls-can-miss-what-it-is-checking-and-then-blames-the-port)
- `TIM_FLIPS=<dir>:<last>` is a stopping point, not a filter - [more](docs/lessons.md#tim_flipsdirlast-is-a-stopping-point-not-a-filter)
- A verdict that cannot say what kind of "no" it means will hide the one that matters - [more](docs/lessons.md#a-verdict-that-cannot-say-what-kind-of-no-it-means-will-hide-the-one-that-matters)
- A sampled frame can only land on a phase that is a multiple of the step, and a screen that animates has phases in between - [more](docs/lessons.md#a-sampled-frame-can-only-land-on-a-phase-that-is-a-multiple-of-the-step-and-a-screen-that-animates-has-phases-in-between)
- A red line from the verifier means one of two things, and they look identical - [more](docs/lessons.md#a-red-line-from-the-verifier-means-one-of-two-things-and-they-look-identical)
- `check_sound` prints its verdict and then does not exit - [more](docs/lessons.md#check_sound-prints-its-verdict-and-then-does-not-exit)
- `-fsyntax-only` is not a build, and the difference is exactly the class of defect this project keeps meeting - [more](docs/lessons.md#-fsyntax-only-is-not-a-build-and-the-difference-is-exactly-the-class-of-defect-this-project-keeps-meeting)
- A short budget and a routine nothing calls give the same verdict - [more](docs/lessons.md#a-short-budget-and-a-routine-nothing-calls-give-the-same-verdict)
- "Never called" measured on four levels is a statement about those four levels - [more](docs/lessons.md#never-called-measured-on-four-levels-is-a-statement-about-those-four-levels)
- Two tools that agree can share a blind spot, and then the agreement is worth nothing - [more](docs/lessons.md#two-tools-that-agree-can-share-a-blind-spot-and-then-the-agreement-is-worth-nothing)
- A check that does not build its own references compares two different ages of the code - [more](docs/lessons.md#a-check-that-does-not-build-its-own-references-compares-two-different-ages-of-the-code)
- A spec that was right becomes wrong when the routine's arguments change, and nothing links the two - [more](docs/lessons.md#a-spec-that-was-right-becomes-wrong-when-the-routines-arguments-change-and-nothing-links-the-two)
- The list that tells ctypes what a routine returns existed twice, and the sweep read the copy nobody updated - [more](docs/lessons.md#the-list-that-tells-ctypes-what-a-routine-returns-existed-twice-and-the-sweep-read-the-copy-nobody-updated)
- Two developer builds had never once compiled, and the sanitizer found a real overflow the first time it ran - [more](docs/lessons.md#two-developer-builds-had-never-once-compiled-and-the-sanitizer-found-a-real-overflow-the-first-time-it-ran)
- A `make` after a `make` compiles nothing, so grepping its output for warnings answers about an empty build - [more](docs/lessons.md#a-make-after-a-make-compiles-nothing-so-grepping-its-output-for-warnings-answers-about-an-empty-build)
- Do not rebuild anything while a check is running - [more](docs/lessons.md#do-not-rebuild-anything-while-a-check-is-running)
- Two references agreeing is not corroboration when they share a bias - [more](docs/lessons.md#two-references-agreeing-is-not-corroboration-when-they-share-a-bias)
- The hybrid's music runs on a different clock from its samples, so the two cannot be compared - [more](docs/lessons.md#the-hybrids-music-runs-on-a-different-clock-from-its-samples-so-the-two-cannot-be-compared)
- The sampling trap is not about frames - [more](docs/lessons.md#the-sampling-trap-is-not-about-frames)

### The hybrid runner

- A Unicorn read hook over a `uc_mem_map_ptr` region breaks the guest - [more](docs/lessons.md#a-unicorn-read-hook-over-a-uc_mem_map_ptr-region-breaks-the-guest)
- The hybrid cannot watch what it has dispatched - [more](docs/lessons.md#the-hybrid-cannot-watch-what-it-has-dispatched)

### DGROUP, frames and pointers

- A routine that calls `dg_alloca` needs `guest_sp` set, or it writes its locals over live memory - [more](docs/lessons.md#a-routine-that-calls-dg_alloca-needs-guest_sp-set-or-it-writes-its-locals-over-live-memory)
- Promoting a frame's slots to C locals spends `frames.py` - [more](docs/lessons.md#promoting-a-frames-slots-to-c-locals-spends-framespy)
- A `_seg`/`_off` pair with arithmetic on one half is a pointer - [more](docs/lessons.md#a-_seg_off-pair-with-arithmetic-on-one-half-is-a-pointer)
- `dg_off` on a pointer that is not in DGROUP is a number, and the compiler will hand it to you without complaint - [more](docs/lessons.md#dg_off-on-a-pointer-that-is-not-in-dgroup-is-a-number-and-the-compiler-will-hand-it-to-you-without-complaint)
- A frame slot whose value is filed into DGROUP must stay an offset, and getting that wrong reads exactly like the timer defect - [more](docs/lessons.md#a-frame-slot-whose-value-is-filed-into-dgroup-must-stay-an-offset-and-getting-that-wrong-reads-exactly-like-the-timer-defect)
- A fix for undefined behaviour is where a value change hides, and the cast has to be the field's own width - [more](docs/lessons.md#a-fix-for-undefined-behaviour-is-where-a-value-change-hides-and-the-cast-has-to-be-the-fields-own-width)
- Deleting the two lines that copied a pair out left the declaration that made them necessary, and C called that a new variable - [more](docs/lessons.md#deleting-the-two-lines-that-copied-a-pair-out-left-the-declaration-that-made-them-necessary-and-c-called-that-a-new-variable)
- A pair is found by what the code does with it, not by what the two halves are called - [more](docs/lessons.md#a-pair-is-found-by-what-the-code-does-with-it-not-by-what-the-two-halves-are-called)
- Two halves compared separately are safe to fold only when the test is equality - [more](docs/lessons.md#two-halves-compared-separately-are-safe-to-fold-only-when-the-test-is-equality)
- A struct field is a claim about width, and a narrower one is a short read that compiles - [more](docs/lessons.md#a-struct-field-is-a-claim-about-width-and-a-narrower-one-is-a-short-read-that-compiles)
- An empty evidence set is not evidence for the wider type - [more](docs/lessons.md#an-empty-evidence-set-is-not-evidence-for-the-wider-type)
- A frame stops being convertible for three reasons, and only two are about the code - [more](docs/lessons.md#a-frame-stops-being-convertible-for-three-reasons-and-only-two-are-about-the-code)
- A frame is walled slot by slot, not routine by routine - [more](docs/lessons.md#a-frame-is-walled-slot-by-slot-not-routine-by-routine)
- Filing one slot's address into another slot of the same frame is not filing - [more](docs/lessons.md#filing-one-slots-address-into-another-slot-of-the-same-frame-is-not-filing)
- A `seg:off` pair held in two variables is one pointer if it is only dereferenced - [more](docs/lessons.md#a-segoff-pair-held-in-two-variables-is-one-pointer-if-it-is-only-dereferenced)
- The line that separates the frames that convert from the ones that do not - [more](docs/lessons.md#the-line-that-separates-the-frames-that-convert-from-the-ones-that-do-not)
- `dg_call`/`dg_uncall` are gone, and they were bookkeeping for a comparison nobody makes - [more](docs/lessons.md#dg_calldg_uncall-are-gone-and-they-were-bookkeeping-for-a-comparison-nobody-makes)
- `dg_off` refuses a pointer that is not the guest's, and the first thing it caught had been in the tree for weeks - [more](docs/lessons.md#dg_off-refuses-a-pointer-that-is-not-the-guests-and-the-first-thing-it-caught-had-been-in-the-tree-for-weeks)
- A typed handle tested as a boolean is always true, and the compiler will not say so - [more](docs/lessons.md#a-typed-handle-tested-as-a-boolean-is-always-true-and-the-compiler-will-not-say-so)
- An object the linker puts at an odd address is one the compiler assumed was aligned - [more](docs/lessons.md#an-object-the-linker-puts-at-an-odd-address-is-one-the-compiler-assumed-was-aligned)

### Still open, so recorded in STATUS.md

- STATUS.md's table is only as fresh as the last `--all` sweep, and it can say "agreed" about a routine that no longer does - [more](STATUS.md#statusmds-table-is-only-as-fresh-as-the-last---all-sweep-and-it-can-say-agreed-about-a-routine-that-no-longer-does)
- What is left after all of that, and why each one is left - [more](STATUS.md#what-is-left-after-all-of-that-and-why-each-one-is-left)
- The hybrid's frame digests cannot be compared between two runs, and a virtual clock did not fix it in one sitting - [more](STATUS.md#the-hybrids-frame-digests-cannot-be-compared-between-two-runs-and-a-virtual-clock-did-not-fix-it-in-one-sitting)
- An interrupt is exclusive; a thread is not - [more](STATUS.md#an-interrupt-is-exclusive-a-thread-is-not)
- The reference run from the entry point never presents a page - [more](STATUS.md#the-reference-run-from-the-entry-point-never-presents-a-page)
- `make test` stops at its solutions step, and the checks after it have not been running - [more](STATUS.md#make-test-stops-at-its-solutions-step-and-the-checks-after-it-have-not-been-running)

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
| `tools/dgrules.py` | **what the DGROUP structs have not swallowed yet**, over a tree-sitter parse rather than a regex, because both its rules are about *shape*: `raw` lists every remaining `DG*` accessor split by constant offset - which a field can replace - against computed, which is a record needing its type known first; `offset-arg` finds a near pointer hidden as arithmetic in an argument, `game_fread((uint16_t)(0x627a + si), ...)`, which `dg_off(&STRUCT.field[si])` says better. Neither is a failure; both are a worklist, sorted so the biggest cluster is the next struct to write ; `const-addr` finds the third shape, a four-digit constant assigned to a variable that is *then* used as an address - `mov si, 0x53ab` seen from the C side, which `raw` cannot find because there is no accessor carrying the constant to group on|
| `tools/frames.py` | **what each routine reserves for its locals**, from the binary's `sub sp,N` and from the port's `dg_alloca(N)`. The two should agree, and where they do not the report says which of the two rules the port followed. It also flags a named slot at or past the frame's end, which cannot be a local ; it also separates the two shapes that are *not* a mismatch - a frame **split** between an array and C locals (`read_far` is 0x100 of `sub sp,0x10a`), and a frame that **is the caller's argument slots** (`read_into_huge` and `expand_1bpp_to_4bpp` reserve because `huge_add_to` steps the far pointer the caller passed by value, so the original's `sub sp` is 0)|
| `tools/framify.py` | turns a routine's `dg_alloca` frame into the `uint8_t frame[N]` it is, one named routine at a time. **Its refusals are the point**: a slot whose value is *filed* anywhere rather than only read through, a slot spelled in a way it cannot read, a `bp` that derives nothing. Each was written after that shape broke something |
| `tools/framify_fixups.py` | the shapes a frame conversion leaves behind - an unsigned read used as an lvalue, a `dg_ptr` on something that is already a pointer, a byte slot still read with `DG8`. Per *function*, because slot names are per function. **The `(?!=)` on every write rule is the one thing to get right**: without it a comparison `DG16(x) == 0` becomes `dg_wr16(x, = 0`, which has broken the build three times from three hand-retyped copies |
| `tools/promote.py` | **turns a frame's slots into the C locals they are**, which is what `framify.py`'s `uint8_t frame[N]` was a staging post for. A slot nothing indexes past `[0]` and nothing hands to a callee is one variable; one that is indexed further or passed on is a buffer and gets the whole extent, because then its size is the caller's business. **Its refusals are the point**: an array with no slots is a real array (`draw_rope` builds a table of pointers into its own frame), and two slots at one offset are the original *reusing* a slot - `blit_scaled_a` calls `[bp-0x16]` `vcut` while clipping and `vrepeat` while repeating rows, so they alias rather than split |
| `tools/framify_census.py` | which frames `framify.py` can take, and what each remaining callee holds up. Wrong three times in three ways before it was right, all three recorded in its own header |
| `tools/verify.py` | **proves one routine against the original**: stop at its entry, let the original body run, compare what each did to the hardware. `--click` drives it to screens behind the menu |
| `tools/check_briefing.py` | **proves a whole screen**: runs both sides from the entry point with the same clicks and compares settled flips. `--screen briefing\|picker\|save` |
| `tools/check_save.py` | **proves the file the game saves**, byte for byte. A machine file never reaches a pixel, so no screen comparison can see the writer |
| `tools/check_solutions.py` | **proves every solution snapshot still solves its level.** `--run` plays each restored machine through the real loop, a minute a level, and reads the one signal the game gives - `finish_level`. `--simulate` runs the same machine through the game's own per-frame step with no clock, input, display or timer thread (`TIM_SIMULATE` in `devmain.c`): the whole set in about a second, deterministic, and measured to solve at the same frame as the real loop on every level compared. `make test` runs the simulated form; the real one stays the check before a commit, because it is the only one that exercises the loop's own input and presentation |
| `tools/check_printf.py` | **proves the `printf` engine against the host's libc**: `borland_vsprintf` through `libtim.so` and `snprintf` through the C library must answer the same string for every format tried, and the Borland-only cases - `%p`, the `F` prefix, a zero with a zero precision printing nothing - against the listing. Nothing the game does reaches a conversion, so this is the transcription's one check; it runs in `make test` |
| `tools/check_handles.py` | **refuses a typed handle used as a boolean.** `PART_PTR(0)` is DGROUP:0, never NULL, so `if (p)`, `p ? :`, `!p` and `p &&` on a `struct part *`, `struct belt *`, `struct rope *` or `struct rect_list_entry *` are always true where the original tested an offset against 0 - and C accepts them. Structural, because the fault's condition is one no captured run reaches; it runs in `make test`, ahead of the solutions step |
| `tools/genld.py` | **writes the linker script that lays out `guest_mem`.** The image's data is transcribed as C objects marked `DGROUP_AT`, `DGROUP_BSS` or `SEGMENT_AT` (dgroup.h); this reads those sections back out of the compiled objects and places each at its address, so a DGROUP offset and a named object are the same byte and the linker refuses an overlap. Every link takes it |
| `tools/check_image_data.py` | **proves the port needs no image**: each placed object at its address in `libtim.so` holding the relocated image's bytes (or zero, for `DGROUP_BSS`), and every byte of DGROUP's initialised data and the sound module's tables covered. The port does not load `TIM.img`; only the hybrid and the tools do. Runs in `make test` |
| `tools/check_sound.py` | **proves the port plays the original's samples**, byte for byte. `--fm` asks the same of the music and answers **inconclusive**, because the hybrid's tick runs at emulation speed and the card's completion interrupt does not. No screen comparison can hear, and the play path is a loaded module doing port I/O, so `verify.py` cannot reach it either. Aligned by content: a looping sample repeats until the next trigger, so the counts differ and the *sequence* does not |
| `tools/fixture.py` | a game directory with the things the real one happens not to have - a subdirectory, a `password.txt` - so the routines behind them can be reached at all |
| `reconstruct/devlua.c` | **drives the developer build from a script.** `TIM_LUA=<port>` listens on 127.0.0.1 and runs what is sent as Lua *inside the game*, a line at a time, polled on the page flip: a script can read DGROUP before it clicks and wait for flips, which `TIM_CLICK`'s flip-numbered clicks cannot. `tools/native/nativelua.c` adds the half only the hybrid can offer - `tim.bp(<image offset>)`, a breakpoint in the **original's** code, which is how a question about its registers gets an answer. Both are dev-only: `devtim` and the hybrid link them, `libtim.so` and `tim` do not |
| `tools/native/` | the **hybrid runner**: the original binary under emulation, with the port as its hardware and, routine by routine, as its code. Anything not yet dispatched *traps* - `int 21h`, the A000 aperture, a VGA port - and names the next routine to write, with a guest backtrace. `routines.def` says where each routine is and how it is called, and the shims and the symbol table are generated from it. `TIM_NATIVE_LAYERS=io` runs the port as the *machine* only - VM.OVL, SX.OVL, the Borland runtime, memory, the mouse, the timer - with the game's own code left to the emulator. The sound driver is the one hook not in `routines.def`: it is entered at a single run-time far pointer with a function number in BP and answers in two registers, so `dispatch.c` binds and shims it by hand, and a run reports which of the eighteen functions were reached; each routine's layer is derived from the port file that defines it and from whether its body touches a port, with a short list of exceptions in `genshims.py` for the ones the original filed in a game segment |
| `tools/check_native.py` | **proves the hybrid draws what the port draws**, as a run of consecutive flips each byte for byte. Content-aligned, never flip-numbered: the two sides' clocks are nothing like each other |
| `tools/native/covered.py` | **how much of one routine a run executed**, with `TIM_COVER=<lo>:<hi>:<path>`. A routine verified on one path is not verified: `verify.py` says the compared calls agreed, this says how much of the body they went through |
| `tools/native/iogap.py` | **which transcribed routines will trap if the guest runs them**: every routine whose own body does port I/O, and whether it is dispatched. The trap finds these one at a time by playing; this finds them all at once |
| `tools/native/coverage.py` | **how much of a stretch runs the port's code and not the original's**. A matching screen proves the routines that drew it and nothing else; this says which of the ones a screen reaches are still the original's, and every one of them already has a body in the port |

## What is not being reconstructed, and why

Recorded as deliberate non-goals in `STATUS.md`, and in the port as no-ops with
a comment saying why - never as gaps waiting to be filled, and kept out of the
verifier's dispatch so a decision is not reported as a difference.
