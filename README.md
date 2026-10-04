# The Incredible Machine, reconstructed

**The Incredible Machine** (Dynamix / Sierra On-Line, 1993), reverse
engineered from its `TIM.EXE` and reconstructed as C, in two versions - and
proven, byte for byte, against the original.

One source, two compilers:

- **Borland's compilers rebuild the original program exactly.** Each module
  is compiled with the compiler that built it - Borland C++ 3.0 for most of
  the game, 2.0 for the older shared modules, 3.1 for 1.11's sound library,
  Turbo C++ 1.01 for one routine, TASM for the assembly - and linked with
  TLINK into a TIM.EXE whose every byte and relocation is the original's.
- **gcc builds a working port** of the same sources, with SDL3 for the
  window, input and sound: the game, playable on a modern machine.

## The two versions

| | 1.00 | 1.11 |
| --- | --- | --- |
| the game | The Incredible Machine | The Even More Incredible Machine |
| the tree | [`reconstruct/v1.00`](reconstruct/v1.00/README.md) | [`reconstruct/v1.11`](reconstruct/v1.11/README.md) |
| the proof | the shipped TIM.EXE's SHA-256, packed with LZEXE 0.91 | every byte TLINK writes and every relocation |
| what is known about it | [`docs/v1.00.md`](docs/v1.00.md) | [`docs/v1.11.md`](docs/v1.11.md) |

What the two executables share - the compilers, the layout, the video - is
[`docs/executable.md`](docs/executable.md).

The two trees are **independent**: each has its own sources, headers, host
code, Makefile and ymfm, builds and runs on its own, and can be split out as
a repository of its own (`git subtree split --prefix reconstruct/v1.11`). A
fix that applies to both is made in both. What they share is what proves
them - `tools/`, `reconstruct/tests` and `solutions/`.

## Playing it

The game's files are Dynamix's and Sierra's and are not here: each tree's
`game/` holds your copy. For 1.11, GOG sells The Even More Incredible Machine
and `get-game.sh` extracts it from the installer (checked against its
SHA-256); for 1.00, copy your own in.

    cd reconstruct/v1.11
    ./get-game.sh path/to/setup_the_even_more_incredible_machine_2.1.0.24.exe
    make
    ./tim

Needs a C and a C++ compiler, `make`, `pkg-config` and SDL3 (and Lua for the
developer build, `devtim`). Each tree's README has the details.

## Proving it

The checks need [uv](https://docs.astral.sh/uv/) for the Python tooling and a
checkout of [turboc](https://github.com/borancar/turboc) beside this one
(`../turboc`): the compilers, the linker and the emulator they run under
(and [DOSBox](https://www.dosbox-staging.org/) for the DOS build's `--run`).
**Every tool works on one version**, `TIM_VERSION` - 1.11 unless it says
1.00 - with its outputs in `out/v<version>/`:

    uv run python tools/judge.py reconstruct/v1.11/src/machine.c   # one file, routine by routine
    uv run python tools/link.py                                    # the whole program, 1.11
    TIM_VERSION=1.00 uv run python tools/link.py                   # and 1.00, to its file hash
    uv run python tools/dosbuild.py --run                          # the same, with Borland MAKE in DOS
    make -C reconstruct/v1.11 test                                 # provenance, and every solution simulated
    uv run python tools/check_solutions.py                         # every solution on the real loop
    uv run python tools/check_briefing.py --screen picker          # a screen against the original, pixel for pixel

- **`tools/judge.py`** compiles a source with its own compiler and options
  (the `JUDGE:` markers in its header) and compares every routine and its
  data with the recovered image.
- **`tools/link.py`** builds every module, links them, and compares the
  result with the original executable.
- **`tools/dosbuild.py`** stages a directory DOS can build the game in -
  8.3 names, a `MAKEFILE` for Borland MAKE, each module's compiler and
  options - and `--run` builds it under DOSBox with the original tools and
  checks the TIM.EXE against `link.py`'s.
- **`tools/check_solutions.py`**, **`check_briefing.py`** and
  **`check_save.py`** run the port and the original - under the shared DOS
  emulator, `tools/tim.py` - and compare what they do: the machines in
  `solutions/` solving their puzzles, screens, saved files.

## The repository

| | |
| --- | --- |
| `reconstruct/v1.00`, `reconstruct/v1.11` | the two versions, each a complete tree with its own README |
| `reconstruct/tests` | the provenance check `make test` runs: every routine says where it came from |
| `tools/` | recovery, disassembly, the judge, the link, the comparisons; `tools/version.py` picks the version, `tools/borland/` keeps Borland's runtime library transcribed |
| `solutions/` | a machine that solves each puzzle, in the game's own save format |
| `docs/` | the binary: [`executable.md`](docs/executable.md) for what both versions share, [`v1.00.md`](docs/v1.00.md) and [`v1.11.md`](docs/v1.11.md) for each, [`video-driver.md`](docs/video-driver.md), [`sound-driver.md`](docs/sound-driver.md), [`resources.md`](docs/resources.md), [`runtime.md`](docs/runtime.md); and [`lessons.md`](docs/lessons.md), the traps met on the way |
| [`STATUS.md`](STATUS.md) | where things stand, and the record of how they got there |
| [`CLAUDE.md`](CLAUDE.md) | how the work is done: the rules, the conventions, the tools |

## Licences

The reconstruction and the project's tooling are licensed under the **GNU
General Public License, version 2** ([`LICENSE`](LICENSE)), and each version's
tree carries its own copy of it; each reconstructed file names the binary it
was read from. ymfm, in each tree's `vendor/`, is Aaron Giles'
(BSD-3-Clause). The games themselves are Dynamix's and Sierra's, and
none of their files are in this repository.
