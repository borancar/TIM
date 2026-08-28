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
  place. Three outcomes exist: transcribed (an address), ours (said so), and
  neither - only the third is a failure.
- **The port's `.c` files mirror the original's translation units**, functions
  in address order. In this large-model binary each module is its own code
  segment, so the boundaries are readable off the binary - see
  `docs/executable.md`. Any boundary *we* added for porting says so in its
  header.
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
- **The annotator must only report the *start* of a string.** A version that
  matched anywhere inside one happily labelled every small constant with the
  tail of the Borland banner, which makes a listing look informative and is
  worse than no annotation.

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

## What is not being reconstructed, and why

Recorded as deliberate non-goals in `STATUS.md`, and in the port as no-ops with
a comment saying why - never as gaps waiting to be filled, and kept out of the
verifier's dispatch so a decision is not reported as a difference.
