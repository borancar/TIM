# The Even More Incredible Machine - TIM.EXE 1.11, reconstructed

The Incredible Machine (Dynamix / Sierra On-Line, 1993), version 1.11 as
**The Even More Incredible Machine** ships it, reverse engineered from its
`TIM.EXE` and reconstructed as C. One source, two compilers:

- **Borland's compilers rebuild the original program byte for byte** - every
  byte the linker writes and every relocation - each module with the compiler
  that built it: Borland C++ 3.0 for most of the game, 3.1 for the sound
  library, 2.0 for the older shared modules, Turbo C++ 1.01 for one routine,
  TASM for the assembly.
- **gcc builds a working port** of the same sources, with SDL3 for the window,
  input and sound.

This directory is `reconstruct/v1.11` of the TIM repository,
<https://github.com/borancar/TIM>, on its **`develop` branch**. It builds and
runs on its own; the tools that prove it against the original - the judge,
the link, the provenance check, the solutions and the screen comparisons -
live there, with the documentation of the binary and its sister tree for
1.00, `reconstruct/v1.00`. Changes are made there.

## Building

Needs a C compiler and a C++ one (for ymfm), `make`, `pkg-config` and SDL3;
Lua as well for the developer build.

    make            # tim, the game, and devtim, the developer build
    make test       # the checks that need nothing else; the rest are skipped
                    # outside the TIM repository, and say so

## The game's files

Dynamix's and Sierra's, so not here. GOG sells The Even More Incredible
Machine; `get-game.sh` makes `game/` from its installer,
`setup_the_even_more_incredible_machine_2.1.0.24.exe` (SHA-256
`2746504f970f0490b94f6472fddf9cf937770a85a1ed0f9e973a45c282ffcc67`, the only
build accepted - it holds TIM.EXE 1.11). It uses `innoextract` and takes only
the installer's `app/`, without the DOSBox GOG bundles:

    ./get-game.sh path/to/setup_the_even_more_incredible_machine_2.1.0.24.exe

## Running

    ./tim

It runs the game in `game/` beside itself when there is one, and otherwise
in the directory it is started from, as DOS ran a game from its own
directory - so `cd game && ../tim` works too.

## Layout

| | |
| --- | --- |
| `src/` | the game: one file per module of the original, in address order, each routine with the image address it came from; `src/parts/` the part kinds, `src/sound/` the sound library |
| `tim.h`, `dgroup.h` | the declarations, and DGROUP's records |
| `hostio.c`, `sdl.c`, `hostlib.c` | the host: the hardware, the window, the few library routines libc lacks |
| `dev*.c` | the developer build's hooks; never in `tim` |
| `tc/` | the `stdint.h` Borland's compilers lack |
| `vendor/ymfm/` | Aaron Giles' OPL2 core (BSD-3-Clause), the one part that is not transcribed |
| `get-game.sh` | `game/` from GOG's installer |

This tree is licensed under the **GNU General Public License, version 2**
(`LICENSE`); each reconstructed file names the binary it was read from. ymfm,
in `vendor/`, carries its own licence (BSD-3-Clause). The game's own files are
Dynamix's and Sierra's and are not part of it.
