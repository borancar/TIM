"""**Which version the tools work on: `TIM_VERSION`, 1.11 unless it says 1.00.**

Each version is a tree of its own, `reconstruct/v<version>`, with its own
sources, headers, host code, Makefile and ymfm; its own game directory, the
tree's `game/` (supply your own: 1.00 is The Incredible Machine, 1.11 The Even
More Incredible Machine, which v1.11's get-game.sh extracts from GOG's
installer); and its own outputs,
`out/v<version>`. What the two share is outside both: the tools,
`reconstruct/tests` and `solutions/`, which both versions solve.

Everything a tool reaches for that differs by version is named here, and only
here; `tim.py` passes it on. This module imports nothing heavy, so a tool that
only needs a path does not load the emulator.

This file is the port's own tooling, not a transcription.
"""
import os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# Each version and the game it is: 1.00 The Incredible Machine, 1.11 The Even
# More Incredible Machine.
VERSIONS = {"1.00": "The Incredible Machine",
            "1.11": "The Even More Incredible Machine"}
VERSION = os.environ.get("TIM_VERSION", "1.11")
if VERSION not in VERSIONS:
    raise SystemExit("TIM_VERSION=%s: the versions are %s"
                     % (VERSION, ", ".join(sorted(VERSIONS))))
RECON = os.path.join(REPO, "reconstruct", "v" + VERSION)
OUT = os.path.join(REPO, "out", "v" + VERSION)
# The game's files: the tree's own `game/` (v1.11's get-game.sh makes it from
# GOG's installer; 1.00's is supplied).
GAME_DIR = os.path.join(RECON, "game")
IMAGE = os.path.join(OUT, "TIM.img")
UNPACKED_EXE = os.path.join(OUT, "TIM.unpacked.exe")
TESTS = os.path.join(REPO, "reconstruct", "tests")


def require_in_tree(path):
    """Refuse a source that is not this version's: its headers and its image
    would be the other version's."""
    p = os.path.abspath(path)
    if not p.startswith(RECON + os.sep):
        for v in VERSIONS:
            if p.startswith(os.path.join(REPO, "reconstruct", "v" + v) + os.sep):
                raise SystemExit("%s is %s's; run with TIM_VERSION=%s"
                                 % (os.path.relpath(p, REPO), v, v))
