# Working on this reconstruction

The Incredible Machine (Dynamix / Sierra, 1993), reverse engineered from
`TIM.EXE` and reconstructed as C.

Two artefacts check each other: the **emulator** running the original binary is
the *reference* that defines what "correct" means, and the **C port** is the
deliverable. Neither is trusted alone. Nothing is finished because it looks
right on screen.

## The goal: one source, two compilers

**Both versions are built, each from a tree of its own** (since 2026-10-04):
`reconstruct/v1.00` is TIM.EXE as The Incredible Machine ships it and
`reconstruct/v1.11` as The Even More Incredible Machine ships it, each with
the game's files in its `game/` (untracked; supply your own - v1.11's
`get-game.sh` extracts it from GOG's installer). `tim` and `devtim` run the
game in the `game/` beside them, else in the directory they start in. The two
trees are independent - each has its own sources, headers, host code,
Makefile, README and `vendor/` (ymfm), and a fix that applies to both is made
in both - and share only what proves them: `tools/`, `reconstruct/tests` and
`solutions/`. Each builds and runs on its own, so it can be split out (`git
subtree split --prefix reconstruct/v1.11`); its `make test` runs the checks
that need this repository only when it is here, and says so when it skips
them. **Every tool works on one version, `TIM_VERSION`, 1.11 unless it says
1.00** (`tools/version.py`): its tree, its `game/`, and its outputs in
`out/v<version>/`. `tools/unrnc.py` recovers 1.11's image and
`tools/unlzexe.py` 1.00's; `docs/v1.00.md` and `docs/v1.11.md` hold what is
known about each, and `docs/executable.md` what they share.
`tools/link.py` links each into its program, every byte and relocation -
1.00 down to the shipped file's hash.

Since 2026-09-26 the port is also the **byte-exact reconstruction**. Every
`reconstruct/v<version>/src` file is to compile under the compiler that built it -
Borland C++ 3.0 for most of the game (options per module), Borland C++ 3.1
for 1.11's sound library, Borland C++ 2.0 for fifteen modules, Turbo C++
1.01 for `atan2_long`, and TASM 3.0 for the assembly (2.51, BC++ 2.0's own,
for `vidload.c`'s inline `asm`) - to exactly the image's bytes
(`tools/judge.py`, whose file markers say which), *and* still build
with gcc into the working port, which must stay green. Pointer sizes differ
between the two and that is expected. What exists only because the host is
not a 16-bit machine goes through macros that vanish under TCC; what TCC does
by itself (huge pointer arithmetic, long helpers, layout) is not emulated in
the source. A routine is **matched** when the judge says MATCH; until then it
is transcribed, as before.

**A stored pointer is a real pointer on both compilers** (decided 2026-09-26,
extended 2026-09-28): `plot_fn = plot_clipped`, `list->pos`, a `T near *` or
`T far *` field - never a guest `seg:off` pair or a DGROUP offset. The image
is reconstructed and proven by the judge and by `tools/link.py`'s hash of
the whole linked TIM.EXE, so the host no longer mirrors the guest's memory:
no object is placed at its guest address, and the hybrid runner, which
needed that, is gone (2026-09-28). The functional proof is
`check_solutions`, with `check_briefing` and `check_save` against the
emulator running the original.

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
- **Addresses are image offsets** - byte offsets into the version's
  `out/v<version>/TIM.img`, the recovered image - unless written `seg:off`. The original's entry point is
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
  in address order. The binary is **medium model**: a segment boundary is a
  module boundary, but segment 0000 (`_TEXT`) holds several modules, whose
  boundaries have to be found - see `docs/executable.md`. Any boundary *we*
  added for porting says so in its header.

  **They live in `reconstruct/v<version>/src`.** That directory is the game and nothing
  else: the eight modules, the DGROUP array and the two overlays, plus
  `main.c`, which is there because a DOS game's entry point is part of the
  game. What stays a directory up is what is *not* the game - `hostio.c` the
  hardware, `sdl.c` the window, `hostlib.c` the few Borland library routines
  libc lacks, and the `dev*.c` files that never ship. The game calls the C
  library by its own names and includes its own headers, Borland's under
  TCC and libc's on the host; Borland's library, transcribed, is kept in
  `tools/borland/` for other projects and is not built. A tool that reads the
  port's sources must glob both, and `src` recursively: the part kinds'
  modules are in `src/parts/`.

  **The names are ours; the boundaries are the original's.** `machine.c`,
  `game.c`, `parts/*.c` and the rest were called `seg0000.c` and so on until each
  had been read well enough to say what it holds. Every one still names its
  segment and image range in its header, because that is the fact - a file
  called `engine.c` is a judgement about 8,275 lines and the segment number is
  not. Do not move a routine between files to suit a name: the file it belongs
  in is the one whose address range contains it.
- **DGROUP's records are C objects, and each one's image address is a
  comment beside it** (`/* DGROUP 0x4e34 */`, or in the comment above).
  Nothing places them: under Borland C++ the linker puts each where the image
  has it, which `tools/link.py` proves, and on the host they are ordinary
  objects. What the image held there is the initialiser, `LOAD_SEG + seg` for
  a relocated word. There is no guest memory on the host: a DOS block is a
  heap block, video memory `g_vga_window`, the BIOS data area `g_bios`,
  and a null the original follows reads `g_dgroup_start` or
  `g_interrupt_table`.
- Where a name or a type is a guess, **say so**.
- **No licence header on reconstructed code.** A provenance header naming the
  binary instead. Our own tooling is a different matter and is GPL-2.0.
- **SDL3 for the window, input and sound. Always.** Never X11, never Win32,
  never SDL2, not behind an `#ifdef`, not as "the optional viewer". One display
  path, not two: the file writer is a *mode* of the same composed frame, never
  a parallel implementation. The port shows a screen by default - running it
  with no arguments opens the game, not writes a bitmap.
- **An assembly module is a `.c` file too.** Its `#ifdef __TURBOC__` branch
  is file-level `asm { }` blocks holding the TASM source, which the judge
  hands to TASM 3.0 directly (`JUDGE: tasm`), and its `#else` is the host's
  transcription. Provenance is a C comment on the line directly above each
  `proc`, the only kind of comment the blocks may carry. `tools/asm2tasm.py`
  drafts the source from the image - a draft the judge then proves or
  refutes.
- **`main.c` and `devmain.c` stay apart, and build two binaries.** A DOS game
  has no command line: it starts, shows its menu, and plays, and `main.c`
  mirrors that. Every developer flag goes in `devmain.c`. `tools/` calls the
  dev binary, so nothing a comparison depends on can become part of what ships.

## Pointers

- **A stored pointer is typed by what it points at**, on both compilers:
  `struct part *next`, `struct bitmap **icons_bmp`, `uint8_t far *driver`,
  `void interrupt (far *old_int8)()`. No `_ptr` suffix; the type says it. A
  near pointer is two bytes under Borland C++ and eight on the host, and
  that is expected.
- **Anything stepped steps by its type.** Retyping a byte pointer changes
  what `+= 4` means - the judge catches it, as `add di, 0x10` where the image
  has `add di, 4`.
- **An allocation of a record says `sizeof`**, never the image's byte count:
  the host's record is wider.
- **A record laid over a loaded module's bytes** (`ASBS`, `VMCS`, the
  `SX*` drivers) keeps the module's layout up to the fields the host reads
  from it; a pointer that would not fit a slot is kept on the host's side.
- **Where the two compilers must differ, a macro says so, and nothing else
  is one**: `NEAR_AT` (a number under Borland, where the original's data held
  one), `NEAR_ZERO` and `ZERO_PAGE` (a null the original follows reads
  memory), `FAR_OF_LONG` and `BCC_FAR_ARG`, `FAR_OF_NEAR_NULL`, `DOS_ALLOC`,
  `SETBUF_ROOM`, `OVERRUN` and `WRITABLE_LITERAL`. Each vanishes under TCC.
  `MK_FP`, `FP_SEG` and `FP_OFF` are Borland's own. On the host **a segment
  is a pointer to its paragraph** (`dg_seg_t`, `struct paragraph *`), so
  `seg + n`, normalising and `MK_FP(seg, off)` mean what they do in real
  mode; a video page is `vga_page_t`, a number the driver turns into an
  address with `vga_window_at`.

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
- What a field *is* is a measurement, not a reading: break on it in the port with gdb, or on the original under the emulator - [more](docs/lessons.md#what-a-field-is-is-a-measurement-not-a-reading)

### Checks, measurements and verdicts

- A check that polls can miss what it is checking, and then blames the port - [more](docs/lessons.md#a-check-that-polls-can-miss-what-it-is-checking-and-then-blames-the-port)
- `TIM_FLIPS=<dir>:<last>` is a stopping point, not a filter - [more](docs/lessons.md#tim_flipsdirlast-is-a-stopping-point-not-a-filter)
- A verdict that cannot say what kind of "no" it means will hide the one that matters - [more](docs/lessons.md#a-verdict-that-cannot-say-what-kind-of-no-it-means-will-hide-the-one-that-matters)
- A sampled frame can only land on a phase that is a multiple of the step, and a screen that animates has phases in between - [more](docs/lessons.md#a-sampled-frame-can-only-land-on-a-phase-that-is-a-multiple-of-the-step-and-a-screen-that-animates-has-phases-in-between)
- A red line from the verifier means one of two things, and they look identical - [more](docs/lessons.md#a-red-line-from-the-verifier-means-one-of-two-things-and-they-look-identical)
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
- The sampling trap is not about frames - [more](docs/lessons.md#the-sampling-trap-is-not-about-frames)
- A run that stops with `exit` destroys the sound chip under the timer thread - [more](docs/lessons.md#a-run-that-stops-with-exit-destroys-the-sound-chip-under-the-timer-thread)
- A check's evidence belongs in the repository, in the game's own format - [more](docs/lessons.md#make-test-stopped-at-its-solutions-step-and-what-that-step-wanted-could-not-be-in-the-repository)
- A tree-sitter parse of code full of unknown macros is not a parse, and the tool cannot tell - [more](docs/lessons.md#a-tree-sitter-parse-of-code-full-of-unknown-macros-is-not-a-parse-and-the-tool-cannot-tell)
- Two drivers doing the same job in different units are not the same driver, and a whole-screen difference is a screen to look at - [more](docs/lessons.md#two-drivers-doing-the-same-job-in-different-units-are-not-the-same-driver)
- A literal and an extern array compile to the same instruction, so a routine match does not say whose data it is: the judge now checks the module's `_DATA` against other files' objects - [more](docs/lessons.md#a-literal-and-an-extern-array-compile-to-the-same-instruction)
- Borland lays `_BSS` out in reverse order of first mention, a header `extern` included, and `-d` merges the duplicate literals a pool may need - [more](docs/lessons.md#a-modules-data-layout-is-decided-by-what-the-compiler-saw-not-by-the-order-of-the-definitions)
- The judge found an inverted comparison that every screen comparison had passed - [more](docs/lessons.md#the-judge-found-a-wrong-comparison-that-every-screen-comparison-had-passed)
- A literal the original writes to is read-only on the host: spell it `WRITABLE_LITERAL`, and run every `check_briefing` screen after data becomes literals - [more](docs/lessons.md#a-literal-the-original-writes-to-is-read-only-on-the-host-and-only-one-check-went-where-it-is-written)
- A `jmp $+2` in the image means the module went through TASM: when a shared tail lands on the wrong copy, read the `-S` listing before rewriting the C - [more](docs/lessons.md#a-tail-the-compiler-will-not-share-the-images-way-may-be-the-assemblers)
- Segment 0000 is BC++ 3.0 `-mm -zC_TEXT` (near runtime helpers, no `-O`), and it still merges identical `if`/`else` tails: a per-branch `push` is two whole statements - [more](docs/lessons.md#segment-0000-is-bc-30-with--zc_text-and-it-merges-the-tails-of-an-if-and-its-else-even-without--o)
- A host-only `#ifndef __TURBOC__` block can swallow routines Borland then never sees, and every host check stays green: the judge reports MISSING now, so watch its routine count - [more](docs/lessons.md#a-preprocessor-block-with-its-endif-in-the-wrong-place-hid-three-routines-from-the-judge-and-the-host-could-not-tell)
- Under `-O -Z` (1.11's modules) a register is evidence of the spelling: a `?:` arm ends in AX, a plain local can be CX or DX, `0 - x` is `xor/sub` where `-x` is `neg`, two calls `-O` merged were two in the source, a comma whose value is a comparison is materialised (put the comparison outside it), a surviving `jmp` to the next instruction is a `continue`, a long jump over a short distance is a forward `goto` cross-jumping retargeted, and a reloaded pointer after a store to a neighbour means the original's were separate variables, not a record - [more](docs/lessons.md#under--o--z-the-registers-say-how-the-c-was-spelled)
- When no option set of a compiler reproduces a routine, find the pattern in code that compiler certainly built before blaming the source - 1.11's sound library is BC++ 3.1's; a `jne` onto a `jmp` is cross-jumping of a statement both branches end with; and the judge masks DGROUP offsets, so only `tools/link.py` sees a swapped store or `_BSS` order - [more](docs/lessons.md#a-routine-no-option-of-one-compiler-reproduces-may-be-another-compilers-and-only-the-link-sees-what-the-judge-masks)
- A prototype is what the callers push: `silence_driver_far` is called with nothing, and `load_sound_bank` with a fourth argument it never reads - [more](docs/lessons.md#a-prototype-is-what-the-callers-push-not-what-the-callee-reads)
- Borland C++ 2.0 orders `_BSS` by name, not by definition, so a module whose routines match may be waiting on a rename - [more](docs/lessons.md#borland-c-20-orders-_bss-by-name-and-its-mk_fp-was-not-the-one-in-its-own-header)
- A link that reuses objects can link one older than its source: `link.py` deletes each object before building it and refuses a module with none; Borland keeps 32 characters of a name - [more](docs/lessons.md#a-link-that-reported-identical-had-linked-a-module-from-an-object-older-than-its-source)
- A compiler short of memory can write an object with no symbols and exit 0: the judge runs the emulated compilers with the memory a real machine left (`tools/tcrun.py`) and refuses an object with no publics - [more](docs/lessons.md#a-compiler-short-of-memory-can-write-an-object-with-no-symbols-and-exit-0)
- The C around `asm` blocks is matched by spelling: `!c` on a signed `char` is `cbw`, `(uint8_t)m & 2` is `test byte`, and AX survives into an `if`'s block but not into the next statement - [more](docs/lessons.md#a-routine-with-asm-blocks-still-has-c-around-them-and-the-image-says-how-that-c-was-written)
- When rewriting a routine for the judge, keep the casts the host needed (a near pointer compared with an `int16_t` is signed on one side only); and wait on a background check's job, never on the files it rewrites, which can be the previous run's - [more](docs/lessons.md#a-cast-the-judge-does-not-need-can-be-one-the-host-does-and-a-wait-on-a-checks-files-can-read-the-previous-run)
- BC++ 2.0 assembles inline `asm` itself, so a module with `asm` went through TASM only if the bytes say so, and its assembler reads a name after `call` as a label (`g_vm_driver+8` became `-8`) - [more](docs/lessons.md#borland-c-20-assembles-inline-asm-itself-and-its-assembler-reads-a-name-after-call-as-a-label)
- A bare `push cs / call` is BC++ 2.0's own for a callee defined above it, so it is not TASM evidence; `81` against `83` on an immediate is; and the built-in assembler negates a `call`'s displacement on an array or big struct, so make those calls in C - [more](docs/lessons.md#a-bare-push-cs--call-is-not-tasms-alone-and-the-built-in-assemblers-encodings-are-its-fingerprint)
- TLINK's `nop / push cs / call` proves a file boundary only in a SMART module: an `and`/`or`/`xor` imm8 written `81` means NOSMART, where a same-file far call comes out that way too - [more](docs/lessons.md#tlinks-far-call-form-proves-a-file-boundary-only-in-a-smart-module)
- A module is assembly only if some routine is one no C source produces: Borland saves SI/DI around any `asm` naming them, always writes the final `ret`, and `mov sp, bp` only after locals - `tools/asm2c.py` drafts the rest as C with inline `asm`, and a sweep must count a failed compile as a failure - [more](docs/lessons.md#an-assembly-module-is-one-no-c-source-can-produce-and-that-is-measured-routine-by-routine)
- DGROUP's order is the objects', not the code's; a module's literals are last in its `_DATA`; a `para`-aligned start is an assembly module; `_BSS` is last mention first, headers included - `tools/link.py` relinks TIM.EXE and says what differs - [more](docs/lessons.md#linking-the-image-back-together-the-data-says-the-object-order-and-where-a-modules-data-ends-says-whose-it-is)
- An inline-asm member (`g_vmds.page_dst`) is resolved by its name across all structs: BC++ 2.0 silently takes another struct's, which only the link sees - write `g_vmds.(struct vmds)page_dst` - [more](docs/lessons.md#an-inline-asm-struct-member-is-looked-up-by-name-alone-and-borland-c-20-picks-the-wrong-struct-without-a-word)
- An address in a data initialiser is a fixup, and fixups move Borland's data record boundaries and so the relocation order: `NEAR_AT(number, &object)` where the original held the number, and only `tools/link.py`'s IDENTICAL proves a data change - [more](docs/lessons.md#an-address-in-a-data-initialiser-is-a-fixup-and-fixups-move-borlands-record-boundaries)

### Pointers

- A null the original follows reads real memory - DGROUP:0000, the interrupt table - and on the host it crashes, only on the path that reaches it: `NEAR_ZERO`/`ZERO_PAGE` at the read - [more](docs/lessons.md#a-null-the-original-follows-reads-real-memory-and-the-host-crashes-only-where-a-path-reaches-it)
- A host address turned into a 16-bit number or a `seg:off` pair is garbage that moves with the heap, and the failure is intermittent: `rr record --chaos` catches it - [more](docs/lessons.md#a-host-address-made-into-a-guest-number-fails-intermittently-and-rr-finds-it)
- A fix for undefined behaviour is where a value change hides, and the cast has to be the field's own width - [more](docs/lessons.md#a-fix-for-undefined-behaviour-is-where-a-value-change-hides-and-the-cast-has-to-be-the-fields-own-width)
- Two halves compared separately are safe to fold only when the test is equality - [more](docs/lessons.md#two-halves-compared-separately-are-safe-to-fold-only-when-the-test-is-equality)
- A struct field is a claim about width, and a narrower one is a short read that compiles - [more](docs/lessons.md#a-struct-field-is-a-claim-about-width-and-a-narrower-one-is-a-short-read-that-compiles)
- A header line added anywhere can fail a file that matched (the emulated compiler's memory is a real machine's): hide host-only declarations from `__TURBOC__`, and judge every file after a shared header changes - [more](docs/lessons.md#a-header-line-added-anywhere-can-fail-a-file-that-matched-and-only-a-sweep-of-every-file-sees-it)
- A record that converts is still allocated at the image's size unless the allocation says `sizeof` - [more](docs/lessons.md#a-record-that-converts-still-gets-allocated-at-the-images-size)
- A far pointer keeps its segment through `p + 1` and the host's `FP_SEG` is the pointer's own paragraph: a block freed through a stepped pointer is found by `io_dos_free`'s table of live blocks - [more](docs/lessons.md#a-far-pointer-keeps-its-segment-through-arithmetic-and-the-hosts-fp_seg-does-not)
- A 16-bit comparison with 0x8000 is always false on the host and compiles: `-Werror=type-limits` is in the build - [more](docs/lessons.md#a-16-bit-comparison-with-0x8000-is-always-false-on-the-host-and-it-compiles)

### Still open, so recorded in STATUS.md

- STATUS.md's table is only as fresh as the last `--all` sweep, and it can say "agreed" about a routine that no longer does - [more](STATUS.md#statusmds-table-is-only-as-fresh-as-the-last---all-sweep-and-it-can-say-agreed-about-a-routine-that-no-longer-does)
- An interrupt is exclusive; a thread is not - [more](STATUS.md#an-interrupt-is-exclusive-a-thread-is-not)
- The reference run from the entry point never presents a page - [more](STATUS.md#the-reference-run-from-the-entry-point-never-presents-a-page)
- A sequence's channel tables are read one entry past their end, for a channel that has no entry - [more](STATUS.md#a-sequences-channel-tables-are-read-one-entry-past-their-end-for-a-channel-that-has-no-entry)

## Tools

Everything reaches the shared emulator through `tools/tim.py`, never by
importing `dos_emulator` directly, so that when the shared code moves there is
one file to fix. The emulator is pinned to a commit in `pyproject.toml`; moving
the pin is a deliberate act and the verification sweep is re-run afterwards.

| tool | what it is for |
| --- | --- |
| `tools/version.py` | **which version the tools work on**, `TIM_VERSION` (1.11 unless 1.00), and every path that differs by version: the tree, the game directory, `out/v<version>/` and its image. Light - a tool that only needs a path imports this, not the emulator |
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
| `tools/reached.py` | which routines a given stretch of the game executes, delimited by page flips |
| `tools/resources.py` | reads and extracts the resource archive |
| `tools/judge.py` | **proves a source is the original's**: compiles a port file with the compiler that built it - the host port of TCC 3.0 in the sibling `turboc` checkout by default, or an original under turboc's emulator (`JUDGE: compiler 1.01`) - and compares every routine with the image, fixups masked, far calls checked against the callee's address (as `9A` or as TLINK's `nop / push cs / call`). `JUDGE: built-with <options>` overrides `-mm -O`. It judges the game as built: `TIM_COPY_PROTECTION` defined and the crack's byte taken out of its copy of the image; `--cracked` judges the shipped byte |
| `tools/asm2c.py` | **drafts an assembly module as C with inline `asm`**: each `proc` becomes a function whose body is `asm` statements, with the frame, the SI/DI save, locals and the final return left for Borland C++ to write, so a judge MATCH is evidence of C and not of transcription. It refuses, naming the routine, what no C source can produce; `--install` rewrites the port file, `--data NAME` maps the module's `d_` labels onto its C data object |
| `tools/link.py` | **links TIM.EXE from the sources and compares it with the original** - the game as built, copy protection intact (`TIM_COPY_PROTECTION`, and the crack's byte taken out of its copy; `--cracked` for the shipped bytes): every game module built as the judge builds it (`JUDGE_KEEP_OBJ`), each code segment's modules given one segment name, the objects in the order DGROUP's data says, BC++ 3.0's TLINK (`/i`) behind `C0M.OBJ` against `CM.LIB`, `minalloc` set to the original's 0x182; the verdict is the file's SHA-256 against the original's - the cracked build's proven by packing it with LZEXE 0.91 into the shipped TIM.EXE byte for byte - with a byte, relocation and header comparison against the unpacked original to say where a difference is. `--reuse` rebuilds only files whose source changed; each build keeps its own objects |
| `tools/h2ash.py` | **the port's C structs as TASM names**: runs Borland's H2ASH on the headers as Borland C++ reads them and writes the structs a module asks for (`JUDGE: structs sequence=seq ...`) as an Ideal-mode include with one constant per field, so an assembly module writes `es:[bx+seq_volume]` where the image has `es:[bx+15eh]` - the same bytes, the judge proves; `PREFIX*` adds that prefix's `#define`s (`VM_SLOT_*`). A field may not be a TASM reserved word |
| `tools/c89init.py` | **rewrites designated initialisers as Borland C++ reads them**, from clang's resolved initialiser lists: positional, every member said, each leaf in its source spelling and each field named in a comment. `--spans` answers the text and the range it replaces |
| `tools/tcrun.py` | turboc's `tcemu.py` with the PSP at 0x0800, the memory a real machine left: BC++ 2.0 wrote an object without symbols under turboc's own figure. `judge.py` runs every emulated compiler through it |
| `tools/check_briefing.py` | **proves a whole screen**: runs both sides from the entry point with the same clicks and compares settled flips. `--screen briefing\|picker\|save` |
| `tools/check_save.py` | **proves the file the game saves**, byte for byte. A machine file never reaches a pixel, so no screen comparison can see the writer |
| `tools/check_solutions.py` | **proves every solution still solves its level.** A solution is `solutions/S<NN>.TIM`, a machine file the game's own `save_machine` wrote, loaded over its puzzle the way the game loads one - `--level N` for the goal, `TIM_LOADMACHINE` for the parts - into a staged copy of the game directory, because the guest's file layer treats that directory as a floor. `--run` plays each through the real loop and reads the one signal the game gives - `finish_level`. `--simulate` runs the same machine through the game's own per-frame step with no clock, input, display or timer thread (`TIM_SIMULATE` in `devmain.c`): deterministic, and measured to solve at the same frame as the real loop on every level compared. Both run the levels in parallel, so the set takes seconds simulated and minutes real. `make test` runs the simulated form; the real one stays the check before a commit, because it is the only one that exercises the loop's own input and presentation |
| `tools/cparse.py` | **the port's C, parsed** - the one door to tree-sitter for every tool that reads the sources, `reconstruct/tests/provenance.py` among them. `far`, `huge`, `near` and `interrupt` (which `tim.h` defines as nothing) and `__builtin_offsetof` are expanded first, space for space so every offset is the file's own, because each makes tree-sitter abandon the construct and read what follows as loose expressions, and inside an ERROR subtree the node types are wrong. A tool that parses reports what it could not parse. `placements()` answers every DGROUP public and its address, from the last link's map |
| `tools/xrefs.py` | **which instructions name a DGROUP word**, over capstone's operand detail on every instruction recursive descent reaches. A field keeps its address for a name when nothing says what it is for, and grepping the port answers about the *port* - a field read from code this port has not transcribed looks exactly like a field nothing reads. **An empty answer is the answer that justifies a `pad_`**: it found 0x0096 written once by the C startup with the BIOS tick count, and 0x009a named by no instruction at all. It sees direct operands only, so a reference computed into a register is not found, and an overlay is a separate binary |
| `tools/waitpids.py` | **waits for a long check to finish, on a pidfd rather than a pid number.** The obvious `until pgrep -f check_machines.py` loop matches the *watcher's own* command line, so it either returns at once or never fires - four watches in one session expired having never seen the run they were watching, and a wait that misses looks exactly like a quiet one. A pidfd is a handle to one process, `poll()` says POLLIN exactly when it exits, and a reused pid cannot take its place |
| `tools/fixture.py` | a game directory with the things the real one happens not to have - a subdirectory, a `password.txt` - so the routines behind them can be reached at all |
| `reconstruct/v<version>/devlua.c` | **drives the developer build from a script.** `TIM_LUA=<port>` listens on 127.0.0.1 and runs what is sent as Lua *inside the game*, a line at a time, polled on the page flip: a script can read DGROUP before it clicks and wait for flips, which `TIM_CLICK`'s flip-numbered clicks cannot. Dev-only: `devtim` links it, `libtim.so` and `tim` do not |

## What is not being reconstructed, and why

Recorded as deliberate non-goals in `STATUS.md`, and in the port as no-ops with
a comment saying why - never as gaps waiting to be filled, and kept out of the
verifier's dispatch so a decision is not reported as a difference.
