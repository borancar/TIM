"""What to transcribe next, in an order that keeps the stubs down.

Transcribing bottom-up matters: a routine whose callees are already transcribed
can be verified end to end, while one full of stubs can only be checked on the
paths that avoid them. So this orders the routines a screen actually reaches by
how many *untranscribed* callees they have, then by size.

"Transcribed" is read from the port's own sources - the same addresses
tests/provenance.py checks - so the queue cannot drift from what exists.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import bisect
import glob
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim
from codemap import walk, ENTRY, far_target
from disasm import DGROUP, image

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

ADDR = re.compile(r"^\s*\*\s*(0x[0-9a-f]{4,5})\s*$", re.M)


def transcribed():
    """Image offsets the port says it has transcribed."""
    out = set()
    for path in glob.glob(os.path.join(tim.REPO, "reconstruct", "*.c")):
        txt = open(path).read()
        for m in ADDR.finditer(txt):
            out.add(int(m.group(1), 16))
    return out


# Borland runtime helpers, each read before being classified - see
# docs/runtime.md. They are deliberate non-goals: a 32-bit shift is `<<` in the
# port, so there is nothing to call. Nothing goes in here unguessed.
RUNTIME = {
    0x0BD90, 0x0BD93, 0x0BD97, 0x0BD9F,   # long comparisons
    0x0BE3E, 0x0BE41,                     # long shift left
    0x0C16E,                              # long multiply
    0x0C7C4,                              # stack overflow check
    0x0BE62,                              # long shift right
    0x0BD0D,                              # far pointer normalise and compare
    0x0D543,                              # memset
    0x00274,                              # write to stderr
    0x0C7E6,                              # heap extension (sbrk)
    0x0DD55,                              # case-insensitive string compare
    0x0CA39,                              # malloc
}


def is_thunk(f):
    """Four bytes of `ljmp [imm16]`: a call into the video driver."""
    d = image()
    return d[f] == 0xFF and d[f + 1] == 0x2E


def callees(lo, hi):
    """Direct call targets inside [lo, hi)."""
    d = image()
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    out = set()
    pc = lo
    while pc < hi:
        ins = next(md.disasm(d[pc:pc + 16], pc), None)
        if ins is None:
            pc += 1
            continue
        if ins.mnemonic == "lcall" and "," in ins.op_str:
            t = far_target(ins.op_str)
            if t is not None and t < DGROUP:
                out.add(t)
        elif ins.mnemonic == "call" and ins.op_str.startswith("0x"):
            out.add(int(ins.op_str, 16))
        pc += ins.size
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--used", default="out/reached_title.json",
                    help="output of tools/reached.py --json")
    ap.add_argument("-n", type=int, default=25)
    args = ap.parse_args()

    seen, calls, callers, funcs = walk([ENTRY])
    fl = sorted(funcs)
    ends = {f: (fl[i + 1] if i + 1 < len(fl) else DGROUP)
            for i, f in enumerate(fl)}

    used = set(json.load(open(args.used))["used"]) if os.path.exists(args.used) \
        else set(fl)
    done = transcribed()

    rows = []
    skipped = {"transcribed": 0, "runtime": 0, "thunk": 0}
    for f in sorted(used):
        if f in done:
            skipped["transcribed"] += 1
            continue
        if f in RUNTIME:
            skipped["runtime"] += 1
            continue
        if is_thunk(f):
            skipped["thunk"] += 1
            continue
        cs = {c for c in callees(f, ends[f]) if c in funcs and c != f}
        missing = sorted(c for c in cs
                         if c not in done and c not in RUNTIME and not is_thunk(c))
        rows.append((len(missing), ends[f] - f, f, calls.get(f, 0), missing))

    rows.sort()
    print("%d routines reached by the screen: %d transcribed, %d runtime, "
          "%d dispatch thunks, %d to go"
          % (len(used), skipped["transcribed"], skipped["runtime"],
             skipped["thunk"], len(rows)))
    print("\n%-8s %-6s %-8s %s" % ("addr", "bytes", "callers", "untranscribed callees"))
    for nmiss, size, f, ncall, missing in rows[:args.n]:
        shown = " ".join("%05x" % c for c in missing[:6])
        more = "" if len(missing) <= 6 else " +%d" % (len(missing) - 6)
        print("%05x    %-6d %-8d %s%s" % (f, size, ncall, shown, more))


if __name__ == "__main__":
    main()
