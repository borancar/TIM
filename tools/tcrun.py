# SPDX-License-Identifier: GPL-2.0-only
"""Run one of turboc's emulated DOS tools with the memory a real machine left.

    uv run --project ../turboc python tools/tcrun.py BCC.EXE --save DIR ...

The same command line as turboc's `tools/tcemu.py`, which this imports and
runs, with one difference: the program's PSP is at segment 0x0800 rather
than turboc's 0x1000. That is 32 KB more conventional memory, about what DOS
5 left free with its kernel loaded high, and **Borland C++ 2.0 needs it for
this project's headers**. With turboc's figure, BCC.EXE 2.0 compiling
vidload.c reported 56 KB free and exited 0 having written an object with no
EXTDEF or PUBDEF records - two records with broken type bytes where they
should be - so the judge saw no routines at all. With this one it reports
89 KB and writes the object whole. What a compiler writes does not depend on
where it is loaded, and the judge also refuses an object without publics
(`judge.compile_obj`).

This file is the port's own tooling, not a transcription.
"""
import os
import sys

TURBOC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                      "..", "turboc")
sys.path.insert(0, os.path.join(TURBOC, "tools"))

import tcemu  # noqa: E402

tcemu.PSP_SEG = 0x0800

if __name__ == "__main__":
    sys.exit(tcemu.main())
