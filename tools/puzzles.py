"""Reach every puzzle through the game's own SELECT PUZZLE screen.

Until now a puzzle past the first was reached by playing to it, so a routine
only some later level uses was found when somebody hit it - which is how the
bellows, the conveyor and an inverted goal test were all found, one at a time,
hours apart. The game has a puzzle picker and it will go straight to any puzzle
the save has unlocked, so this drives it: pick, start the level, run the
machine, and say what happened.

**It drives the port, not the original.** The clicks are the game's own, so the
same sequence would drive the emulator too, and that is the point of doing it
through the picker rather than by writing the puzzle number into DGROUP: a run
that gets there the way a player does can be compared against one that does the
same, and one that got there by having its memory edited cannot.

The starting snapshot must be a port snapshot sitting on a briefing with the
pointer free - `out/devtim003.snap` is one - and its save decides how many
puzzles are reachable: DGROUP 0x4eb7 is the furthest the player has got, and
the picker refuses anything past it with NEED PASSWORD.

Where the clicks come from, so that none of them is a guess:

  * the picker's five regions, read out of a snapshot taken while it is open -
    the list is x 48..448 y 76..285, the down arrow 460..492 x 264..296, the up
    arrow the same x at 66..98, and the go button 496..536 x 300..340;
  * the row a click lands on, which `puzzle_screen` computes as
    `(y - 0x4c) / 10 + page` at 0x0f0b0, so row i of a page is y = 76 + 10i,
    and this aims at the middle of it;
  * a page of 0x15, which is what the two arrows add and subtract.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import concurrent.futures
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PAGE = 0x15                     # puzzles per page, from the arrows' +/- 0x15
LIST_X = 200                    # anywhere inside x 48..448
LIST_Y0 = 76                    # the 0x4c the row arithmetic subtracts
ROW_H = 10
DOWN = (476, 280)               # inside 460..492 x 264..296
GO = (516, 320)                 # inside 496..536 x 300..340
SELECT_PUZZLE = (120, 153)      # the briefing's wrench-and-grid icon
PLAY = (78, 105)                # the briefing's play triangle
START_MACHINE = (600, 42)       # the editor's START MACHINE


def clicks_for(puzzle):
    """The click list, and the flips the snapshot and the run's end go at.

    Flips rather than seconds because a flip is the one clock both sides of
    this project agree on; the spacing is loose because a page redraw and a
    level load are not the same length and neither is measured here.
    """
    downs = (puzzle - 1) // PAGE
    page = 1 + downs * PAGE
    row = puzzle - page

    at = 4
    seq = [(at, SELECT_PUZZLE)]
    for _ in range(downs):
        at += 26
        seq.append((at, DOWN))

    at += 26
    seq.append((at, (LIST_X, LIST_Y0 + ROW_H * row + ROW_H // 2)))
    at += 26
    seq.append((at, GO))

    snap_at = at + 50
    seq.append((snap_at + 10, PLAY))
    seq.append((snap_at + 50, START_MACHINE))
    return seq, snap_at, snap_at + 170


def run_one(puzzle, start, outdir, binary, seconds, when="briefing"):
    seq, snap_at, stop = clicks_for(puzzle)
    if when == "end":
        snap_at = stop - 5
    snap = os.path.join(outdir, "p%02d.snap" % puzzle)
    if os.path.exists(snap):
        os.unlink(snap)

    env = dict(os.environ,
               TIM_HEADLESS="1",
               TIM_CLICK=",".join("%d:%d:%d" % (f, x, y) for f, (x, y) in seq),
               TIM_SNAPAT="%d" % snap_at,
               TIM_SNAP=snap,
               TIM_STOPFLIP="%d" % stop)
    try:
        p = subprocess.run([binary, "--restore", start], cwd=ROOT, env=env,
                           capture_output=True, text=True, timeout=seconds)
        err, code = p.stderr or "", p.returncode
    except subprocess.TimeoutExpired as e:
        err = (e.stderr or b"").decode("utf-8", "replace") \
              if isinstance(e.stderr, bytes) else (e.stderr or "")
        code = "timeout"

    trap = None
    m = re.search(r"reached ([^\n]*), which is not transcribed yet", err)
    if m:
        trap = m.group(1)
    elif "not transcribed" in err:
        trap = err.split("not transcribed")[0].strip().splitlines()[-1][:60]

    got, state = None, None
    if os.path.exists(snap):
        d = open(snap, "rb").read()
        if d[:8] == b"TIMPORT1":
            base = 12 + 0x2E4C0
            got = d[base + 0x4EBD] | d[base + 0x4EBE] << 8
            state = d[base + 0x4E6B] | d[base + 0x4E6C] << 8
    return puzzle, got, trap, code, state


def main():
    ap = argparse.ArgumentParser(
        description=__doc__.splitlines()[0],
        epilog="reads no environment variables of its own; it sets "
               "TIM_HEADLESS, TIM_CLICK, TIM_SNAPAT, TIM_SNAP and "
               "TIM_STOPFLIP for each devtim it runs",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--from-snapshot", default="out/devtim003.snap",
                    help="a port snapshot on a briefing (default: %(default)s)")
    ap.add_argument("--first", type=int, default=1)
    ap.add_argument("--last", type=int, default=47,
                    help="the save's 0x4eb7 is the real ceiling (default: 47)")
    ap.add_argument("--only", help="comma-separated puzzles, instead of a range")
    ap.add_argument("--outdir", default="out/puzzles")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--seconds", type=int, default=300,
                    help="per-puzzle wall clock ceiling (default: %(default)s)")
    ap.add_argument("--snap-at", choices=("briefing", "end"),
                    default="briefing",
                    help="briefing: the save state to reuse later. end: the "
                         "state after START MACHINE, which is how to tell a "
                         "machine that ran from a click that missed - a run "
                         "with no trap proves nothing on its own")
    args = ap.parse_args()

    binary = os.path.join(ROOT, "reconstruct", "devtim")
    if not os.path.exists(binary):
        raise SystemExit("no %s - run `make -C reconstruct devtim`" % binary)

    want = ([int(x, 0) for x in args.only.split(",")] if args.only
            else list(range(args.first, args.last + 1)))
    os.makedirs(args.outdir, exist_ok=True)

    rows = []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        futs = {pool.submit(run_one, n, args.from_snapshot, args.outdir,
                            binary, args.seconds, args.snap_at): n
                for n in want}
        for f in concurrent.futures.as_completed(futs):
            rows.append(f.result())

    rows.sort()
    # The states game_round dispatches on, from its comment at 0x0eff5.
    names = {0x0002: "briefing", 0x1000: "editor", 0x2000: "machine running",
             0x0200: "solved", 0x8000: "box up", 0x0001: "quit"}
    traps, wrong, ok = {}, [], 0
    for puzzle, got, trap, code, state in rows:
        if got is None:
            note = "NO SNAPSHOT"
        elif got != puzzle:
            note = "landed on %d" % got
            wrong.append(puzzle)
        else:
            note = "ok"
            ok += 1
        if trap:
            traps.setdefault(trap, []).append(puzzle)
        print("  puzzle %2d  %-12s %-16s %s"
              % (puzzle, note, names.get(state, "%04x" % state if state
                                         else "-"), trap or ""))

    print("\n%d of %d reached their own briefing" % (ok, len(rows)))
    if wrong:
        print("wrong puzzle: %s" % ", ".join(str(w) for w in wrong))
    if traps:
        print("\n%d distinct traps, most common first:" % len(traps))
        for what, who in sorted(traps.items(), key=lambda kv: -len(kv[1])):
            print("  %-46s %2d puzzles: %s"
                  % (what, len(who), ", ".join(str(w) for w in who)))
    else:
        print("no traps: every puzzle started its machine")
    return 0


if __name__ == "__main__":
    sys.exit(main())
