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


def audit(used):
    """Say which of the reached routines the verifier can speak for.

    This file is the port's own tooling; it is not a transcription.

    The question "does the screen matching prove anything about these
    routines?" was answered six times in one session by writing the same
    throwaway script, so it lives here now. Three outcomes, and the third is
    the one worth seeing: a routine the original runs on this path, transcribed
    in the port, and **not asked about by tools/verify.py** - whose whole
    evidence is therefore that the picture came out right. That is not nothing,
    but it is much less than it looks: `load_screen_plain` called the wrong VGA
    routine on every call while the briefing matched at 0 of 307,200 pixels.
    """
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sys.path.insert(0, os.path.join(root, "reconstruct", "tests"))
    import glob
    import re
    import provenance

    specs = set(re.findall(r'^\s{4}"(\w+)": dict\(',
                           open(os.path.join(root, "tools", "verify.py")).read(),
                           re.M))
    byaddr, stubs = {}, {}
    for path in sorted(glob.glob(os.path.join(root, "reconstruct", "*.c"))):
        t, _o, st, _b, _i = provenance.check(path)
        for name, addr in t:
            try:
                byaddr.setdefault(int(addr, 16), (name, os.path.basename(path)))
            except ValueError:
                pass
        for name, addr in st:
            try:
                stubs.setdefault(int(addr, 16), name)
            except ValueError:
                pass

    reached = [(a, byaddr[a]) for a in used if a in byaddr]
    missing = [(a, n, f) for a, (n, f) in reached if n not in specs]
    hit_stub = [(a, stubs[a]) for a in used if a in stubs]

    print("\naudit against tools/verify.py")
    print("  transcribed routines on this path : %d" % len(reached))
    print("  of those, with a spec             : %d" % (len(reached) - len(missing)))
    print("  with no spec - screen evidence only: %d" % len(missing))
    for a, n, f in sorted(missing):
        print("     0x%05x  %-28s %s" % (a, n, f))
    print("  stubs whose address is reached    : %d%s"
          % (len(hit_stub),
             ("  " + ", ".join(n for _, n in hit_stub)) if hit_stub else ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--audit", action="store_true",
                    help="say which of the reached routines tools/verify.py "
                         "has a spec for, and which are resting on the screen "
                         "comparison alone")
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

    if args.audit:
        audit([f for f, _ in used])

    if args.json:
        json.dump({"from": args.from_flip, "to": args.to_flip,
                   "used": [f for f, _ in used],
                   "counts": {hex(f): n for f, n in used}},
                  open(args.json, "w"), indent=1)
        print("\nwrote %s" % args.json)


if __name__ == "__main__":
    main()
