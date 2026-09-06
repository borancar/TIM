#!/usr/bin/env python3
"""Run every prepared solution and check the machine finishes the puzzle.

**This is the only end-to-end check in the project.** Every other one proves a
routine against the original, or a screen against a capture; this one asks
whether the *engine* works - whether a machine built out of belts, balls,
motors and mice actually runs to the goal the briefing set. It exercises the
part hooks, the movers, the collision walk and the whole `run_machine_loop` in
one go, and nothing else here does.

Each `solution_snaps/levelNN.solution` is a devtim snapshot taken on the level
screen with the puzzle already solved but not yet run. The check restores it,
clicks the run control at the top right of the panel, and waits for
`finish_level` - which the game reaches by no other route, so `TIM_TRACE=level`
saying "solved" is the whole of "did it work".

It is a **pass or fail per level and not a pixel comparison**: two runs of the
same machine do not agree pixel for pixel, because the odometer reels turn on
the timer and the two sides never pace a timer alike (STATUS.md). Whether the
puzzle was solved is a fact the game itself decides, and that is what is read.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import glob
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

# The run control, in the panel's top right. A left click, and the region is
# (576,0)-(632,63) in the game's own coordinates.
RUN_X, RUN_Y = 604, 31
CLICK_FLIP = 10


def run_one(path, flips, verbose):
    env = dict(os.environ)
    env.update({
        "TIM_HEADLESS": "1",
        "TIM_STOPFLIP": str(flips),
        "TIM_TRACE": "level",
        "TIM_CLICK": "%d:%d:%d" % (CLICK_FLIP, RUN_X, RUN_Y),
    })
    devtim = os.path.join(tim.REPO, "reconstruct", "devtim")
    p = subprocess.run([devtim, "--restore", path], env=env,
                       capture_output=True, text=True, timeout=600)
    err = p.stderr
    solved = "io: level solved" in err
    # A stub reached is a different answer from "did not solve", and must not
    # be reported as one.
    stub = ("not transcribed" in err) or ("reached 0x" in err)
    if verbose and err:
        sys.stderr.write(err)
    return solved, stub, err


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dir", default=os.path.join(tim.REPO, "solution_snaps"),
                    help="where the .solution snapshots are")
    ap.add_argument("--flips", type=int, default=2500,
                    help="how long to let each machine run (default %(default)s)")
    ap.add_argument("--only", default="",
                    help="a comma-separated list of level names to run")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="pass the port's own stderr through")
    args = ap.parse_args()

    paths = sorted(glob.glob(os.path.join(args.dir, "*.solution")))
    if args.only:
        want = set(args.only.split(","))
        paths = [p for p in paths
                 if os.path.basename(p).rsplit(".", 1)[0] in want]
    if not paths:
        raise SystemExit("no .solution snapshots in %s" % args.dir)

    tim.game_dir()
    bad = 0
    for path in paths:
        name = os.path.basename(path).rsplit(".", 1)[0]
        solved, stub, err = run_one(path, args.flips, args.verbose)
        if stub:
            print("  %-10s REACHED A STUB - not a failure to solve" % name)
            bad += 1
        elif solved:
            print("  %-10s solved" % name)
        else:
            print("  %-10s DID NOT SOLVE in %d flips" % (name, args.flips))
            bad += 1

    print()
    print("%d of %d solved" % (len(paths) - bad, len(paths)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
