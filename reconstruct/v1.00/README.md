# The Incredible Machine - TIM.EXE 1.00, reconstructed

The Incredible Machine (Dynamix / Sierra On-Line, 1993), version 1.00,
reverse engineered from its `TIM.EXE` and reconstructed as C. One source, two
compilers:

- **Borland's compilers rebuild the original file byte for byte** - linked and
  packed with LZEXE 0.91, the result is the shipped TIM.EXE, its SHA-256 and
  all - each module with the compiler that built it: Borland C++ 3.0 for most
  of the game, 2.0 for the older shared modules, Turbo C++ 1.01 for one
  routine, TASM for the assembly.
- **gcc builds a working port** of the same sources, with SDL3 for the window,
  input and sound.

This directory is `reconstruct/v1.00` of the TIM repository,
<https://github.com/borancar/TIM>, on its **`develop` branch**. It builds and
runs on its own; the tools that prove it against the original - the judge,
the link, the provenance check, the solutions and the screen comparisons -
live there, with the documentation of the binary and its sister tree for
1.11, `reconstruct/v1.11`. Changes are made there.

## Building

Needs a C compiler and a C++ one (for ymfm), `make`, `pkg-config` and SDL3;
Lua as well for the developer build.

    make            # tim, the game, and devtim, the developer build
    make test       # the checks that need nothing else; the rest are skipped
                    # outside the TIM repository, and say so

## The game's files

Dynamix's and Sierra's, so not here: put your copy of The Incredible Machine
1.00 - `TIM.EXE`, its `RESOURCE.*` and the rest of its directory - in
`game/`. Its `TIM.EXE` is the LZEXE-packed one whose program this tree
rebuilds.

## Running

    ./tim

It runs the game in `game/` beside itself when there is one, and otherwise
in the directory it is started from, as DOS ran a game from its own
directory - so `cd game && ../tim` works too.

## Layout

| | |
| --- | --- |
| `src/` | the game: one file per module of the original, in address order, each routine with the image address it came from; `src/parts/` the part kinds |
| `tim.h`, `dgroup.h` | the declarations, and DGROUP's records |
| `hostio.c`, `sdl.c`, `hostlib.c` | the host: the hardware, the window, the few library routines libc lacks |
| `dev*.c` | the developer build's hooks; never in `tim` |
| `tc/` | the `stdint.h` Borland's compilers lack |
| `vendor/ymfm/` | Aaron Giles' OPL2 core (BSD-3-Clause), the one part that is not transcribed |

The reconstructed code asserts no licence - it is a reconstruction of
Dynamix's program; each file names the binary it was read from. ymfm carries
its own licence.
