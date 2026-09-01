"""How much of a stretch of the game runs the port's code, and not the original's.

The hybrid runner dispatches the routines named in `routines.def` to the port
and lets the emulator run the original's own code for everything else. A screen
that matches byte for byte therefore proves the routines that *drew* it, and
says nothing whatever about the rest - which is easy to forget once the pixels
agree, and is the difference between "the graphics layer is right" and "the
port is right".

This measures the gap. It takes the routines a stretch of the game actually
executes, from `tools/reached.py`, and sorts them three ways: dispatched to the
port, transcribed but still running as the original, and not transcribed at
all. Every entry on the middle list is a routine the port already has a body
for.

**It is not the work queue, and reading it as one wastes the work.** These are
the routines the *original* executes, which is a fact about the original and
goes stale the moment anything is dispatched: a routine whose callers have all
been taken over is never reached again, so most of this list can no longer be
exercised by any screen. Six routines were dispatched off it to exercise the
pascal convention and not one of them was ever called.

`TIM_ENTRIES=<path>` on the runner counts what the emulator still enters, and
only what the dispatcher did not take. That is the live list and the one to
work, in call-count order. This file is for the question it was written for -
how much of a stretch is the port's code and how much the original's - and for
seeing what a screen reaches before any of it is dispatched.

    uv run python tools/reached.py --from-flip 0 --to-flip 65 --json out/r.json
    uv run python tools/native/coverage.py out/r.json

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import gensyms                                        # noqa: E402


def reached_from(path):
    """Every image offset named anywhere in a reached.py dump.

    Read by shape rather than by key, because the interesting thing is the set
    of addresses and a schema change should not silently return an empty one.
    """
    out = set()

    def walk(o):
        if isinstance(o, dict):
            for k, v in o.items():
                take(str(k))
                walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)
        elif isinstance(o, str):
            take(o)

    def take(s):
        if re.fullmatch(r'0?x?[0-9a-f]{4,5}', s):
            out.add(int(s.lstrip('x'), 16))

    walk(json.load(open(path)))
    if not out:
        raise SystemExit("no addresses in %s - has reached.py's format moved?"
                         % path)
    return out


def dispatched():
    src = open(os.path.join(HERE, "routines.def")).read()
    return {int(m.group(1), 16) for m in
            re.finditer(r'\b(?:FAR_C|OVL_C|OVL_R|NEAR_P|REG_N)\s*\(\s*'
                        r'(0x[0-9a-f]+)', src)}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("json", help="a --json dump from tools/reached.py")
    ap.add_argument("--quiet", action="store_true",
                    help="the counts only, without the work queue")
    args = ap.parse_args()

    reached, disp = reached_from(args.json), dispatched()
    syms = gensyms.collect()
    transcribed = {a for a, (_n, stub) in syms.items() if not stub}
    stubs = {a for a, (_n, stub) in syms.items() if stub}

    todo = sorted(reached & transcribed - disp)
    print("%d routines reached" % len(reached))
    print("  %4d dispatched to the port" % len(reached & disp))
    print("  %4d transcribed, still running as the original" % len(todo))
    print("  %4d reached a stub" % len(reached & stubs))
    print("  %4d neither transcribed nor a stub"
          % len(reached - transcribed - stubs - disp))

    if not args.quiet:
        print("\nthe work queue - each already has a body in the port:")
        for at in todo:
            print("    %05x  %s" % (at, syms[at][0]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
