"""**Which version the tools work on: `TIM_VERSION`, 1.11 unless it says 1.00.**

Each version is a tree of its own, `reconstruct/v<version>`, with its own
sources, headers, host code and Makefile; its own game directory (supply your
own: 1.00 is The Incredible Machine, 1.11 The Even More Incredible Machine,
GOG's installer extracted); and its own outputs, `out/v<version>`. What the
two share is outside both: the tools, `reconstruct/vendor` (ymfm),
`reconstruct/tests` and `solutions/`, which both versions solve.

Everything a tool reaches for that differs by version is named here, and only
here; `tim.py` passes it on. This module imports nothing heavy, so a tool that
only needs a path does not load the emulator.

This file is the port's own tooling, not a transcription.
"""
import os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSIONS = {"1.00": "incredible-machine",
            "1.11": "even-more-incredible-machine"}
VERSION = os.environ.get("TIM_VERSION", "1.11")
if VERSION not in VERSIONS:
    raise SystemExit("TIM_VERSION=%s: the versions are %s"
                     % (VERSION, ", ".join(sorted(VERSIONS))))
RECON = os.path.join(REPO, "reconstruct", "v" + VERSION)
OUT = os.path.join(REPO, "out", "v" + VERSION)
GAME_DIR = os.path.join(REPO, VERSIONS[VERSION])
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
