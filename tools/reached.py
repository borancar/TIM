"""Which routines a given stretch of the game actually executes.

The code map says what exists; this says what a *screen* uses. Scoping the
transcription to the routines a screen reaches is the difference between 577
routines and a working target, and it is measured rather than guessed: the
window is delimited by page flips, which are the same cue the captures use.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive
from codemap import walk, ENTRY
from disasm import DGROUP

from unicorn import UC_HOOK_BLOCK, UC_HOOK_INSN
import unicorn.x86_const as xc


def reached(first_flip, last_flip, instructions=260_000_000):
    m = drive.machine()
    base = m.load_seg * 16
    top = base + DGROUP
    flips = {"n": 0}
    blocks = collections.Counter()
    ovl = collections.Counter()

    def on_block(uc, address, size, ud):
        if not (first_flip <= flips["n"] <= last_flip):
            return
        if base <= address < top:
            blocks[address - base] += 1
        elif address < 0xA0000:
            ovl[address] += 1

    def on_out(uc, port, size, value, ud):
        if port == 0x3D4 and size == 2 and (value & 0xFF) == 0x0C:
            flips["n"] += 1

    m.uc.hook_add(UC_HOOK_BLOCK, on_block)
    m.uc.hook_add(UC_HOOK_INSN, on_out, None, 1, 0, xc.UC_X86_INS_OUT)

    def stop(mm, done):
        return flips["n"] > last_flip

    drive.drive(m, instructions, on_slice=stop)
    return blocks, ovl, flips["n"]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--from-flip", type=int, required=True)
    ap.add_argument("--to-flip", type=int, required=True)
    ap.add_argument("--json", default="")
    args = ap.parse_args()

    seen, calls, callers, funcs = walk([ENTRY])
    blocks, ovl, nflips = reached(args.from_flip, args.to_flip)

    hit = sorted(f for f in funcs if f in blocks)
    # A routine whose *entry block* never ran but whose body did is still used;
    # count any block inside the routine's address range.
    fl = sorted(funcs)
    ranges = {f: (fl[i + 1] if i + 1 < len(fl) else DGROUP) for i, f in enumerate(fl)}
    used = []
    for f in fl:
        end = ranges[f]
        n = sum(c for b, c in blocks.items() if f <= b < end)
        if n:
            used.append((f, n))

    print("flips %d..%d  (%d flips seen)" % (args.from_flip, args.to_flip, nflips))
    print("  routines with their entry block executed : %d" % len(hit))
    print("  routines with any block executed         : %d of %d"
          % (len(used), len(funcs)))
    print("  distinct overlay blocks                  : %d" % len(ovl))
    print("\nroutines by block executions:")
    for f, n in sorted(used, key=lambda t: -t[1])[:40]:
        print("   %05x  x%-9d %s" % (f, n, "entry hit" if f in blocks else ""))

    if args.json:
        json.dump({"from": args.from_flip, "to": args.to_flip,
                   "used": [f for f, _ in used],
                   "counts": {hex(f): n for f, n in used}},
                  open(args.json, "w"), indent=1)
        print("\nwrote %s" % args.json)


if __name__ == "__main__":
    main()
