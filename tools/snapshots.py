"""What a snapshot holds: which level, which screen, and which puzzle.

Two different files are called a snapshot in this tree and they are **not**
interchangeable, which has already cost an hour: the hybrid runner writes
`TNAP`, with the CPU in it, and `tools/verify.py --from` and `TIM_RESTORE`
resume it exactly; the port writes `TIMPORT1`, which has no CPU because C has
none to save, and only `devtim --restore` can pick it up. Feeding one to the
other answers "is not a snapshot", so this says which is which first.

Everything else is read out of DGROUP, at the offsets the port's own code uses:
the puzzle number the round is on, the state word the screens dispatch on, and
the title and description the briefing draws. Reading them beats keeping a list
in a file, because the list rots the moment somebody presses Shift+F2 again.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import glob
import os
import sys

DGROUP = 0x2E4C0

# Where the port's snapshot puts guest memory, and where the runner's does.
PORT_MEM_AT = 12
RUNNER_MEM_AT = 128

# DGROUP cells, from reconstruct/. Each is used by name in the port.
LEVEL, LAST, BEST = 0x4EBD, 0x4EB9, 0x4EB7
STATE, FREEFORM = 0x4E6B, 0x4E67
SCORE_HI, SCORE_LO = 0x4EAF, 0x4EAD
TITLE, BLURB = 0x4ECF, 0x4F1F

# The states game_round dispatches on, from its comment at 0x0eff5.
STATES = {
    0x0002: "briefing",
    0x1000: "editor",
    0x2000: "machine running",
    0x0200: "level solved",
    0x8000: "message box up",
    0x0001: "quitting",
}


def kind(data):
    """Which of the two formats this is, and where its guest memory starts."""
    if data[:8] == b"TIMPORT1":
        return "port", PORT_MEM_AT
    if data[:4] == b"TNAP":
        return "runner", RUNNER_MEM_AT
    return None, 0


def text(data, base, off, limit=120):
    """A NUL-terminated DGROUP string, as far as it is printable."""
    at = base + DGROUP + off
    out = []
    for c in data[at:at + limit]:
        if c == 0:
            break
        out.append(chr(c) if 32 <= c < 127 else ".")
    return "".join(out)


def describe(path):
    data = open(path, "rb").read()
    what, base = kind(data)
    if what is None:
        return None

    def u16(off):
        at = base + DGROUP + off
        return data[at] | data[at + 1] << 8

    state = u16(STATE)
    return {
        "path": path,
        "kind": what,
        "level": u16(LEVEL),
        "last": u16(LAST),
        "best": u16(BEST),
        "state": state,
        "screen": STATES.get(state, "?"),
        "freeform": u16(FREEFORM) != 0,
        "score": (u16(SCORE_HI) << 16) | u16(SCORE_LO),
        "title": text(data, base, TITLE),
        "blurb": text(data, base, BLURB),
    }


def main():
    ap = argparse.ArgumentParser(
        description=__doc__.splitlines()[0],
        epilog="reads no environment variables",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="*",
                    help="snapshots to read (default: out/*.snap)")
    ap.add_argument("--long", action="store_true",
                    help="print each puzzle's description as well")
    args = ap.parse_args()

    paths = args.paths or sorted(glob.glob("out/*.snap"))
    if not paths:
        raise SystemExit("no snapshots given and none in out/")

    for p in paths:
        d = describe(p)
        if d is None:
            print("%-38s not a snapshot of either kind" % os.path.basename(p))
            continue
        print("%-38s %-6s puzzle %2d of %-3d %-15s score %-7d %s"
              % (os.path.basename(d["path"]), d["kind"], d["level"],
                 d["last"], d["screen"], d["score"], d["title"]))
        if args.long and d["blurb"]:
            print("%38s   %s" % ("", d["blurb"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
