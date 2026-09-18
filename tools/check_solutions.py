#!/usr/bin/env python3
"""Run every prepared solution and check the machine finishes the puzzle.

**This is the only end-to-end check in the project.** Every other one proves a
routine against the original, or a screen against a capture; this one asks
whether the *engine* works - whether a machine built out of belts, balls,
motors and mice actually runs to the goal the briefing set. It exercises the
part hooks, the movers, the collision walk and the whole `run_machine_loop` in
one go, and nothing else here does.

**A solution is a machine file, not a snapshot.** `solutions/S<NN>.TIM` is what
the game's own `save_machine` writes, and it is loaded the way the game loads
one: `--level N` puts the puzzle up, `TIM_LOADMACHINE` hands the file to
`round_teardown`, `load_animation` and `reset_machine` - the three calls the
file picker makes - and the bin is emptied, because a puzzle's bin is the
level's and a machine file records none. The level supplies the goal; the file
supplies the parts.

This replaced a directory of port snapshots. A snapshot is a copy of *our*
memory, which the original cannot read and which stops meaning anything the
moment a struct in `dgroup.h` moves; a machine file is the game's own format,
loadable by either side, and it goes through the loader and the reset that a
real game does rather than around them.

**The solution file must live inside the game directory.** The guest's file
layer treats that directory as a floor - a host path with slashes in it
resolves to nowhere and `read_level` answers 0 without a word - so this builds
a copy of `incredible-machine/` with the solutions beside the game's own files
and points the port at it with `TIM_GAMEDIR`. Measured: with an absolute path
the load printed that it had loaded and left all three part lists empty, and
the first goal test walked off the end of one.

It is a **pass or fail per level and not a pixel comparison**: two runs of the
same machine do not agree pixel for pixel, because the odometer reels turn on
the timer and the two sides never pace a timer alike (STATUS.md). Whether the
puzzle was solved is a fact the game itself decides, and that is what is read.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import concurrent.futures
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim


def level_of(name):
    """The level a solution is for, out of its name: S07.TIM is level 7."""
    m = re.search(r"(\d+)", name)
    if not m:
        return None
    return int(m.group(1), 10)


def staged(paths):
    """A game directory with the solutions in it, and its path.

    A copy rather than the game's own folder, because nothing here writes to
    that - the same rule `tools/fixture.py` states. The copy is made once and
    every level runs against it.
    """
    out = tempfile.mkdtemp(prefix="tim-solutions-")
    shutil.copytree(tim.GAME_DIR, out, dirs_exist_ok=True)
    for path in paths:
        shutil.copy(path, os.path.join(out, os.path.basename(path).upper()))
    return out


def run_one(devtim, name, level, game, flips, simulate=0):
    env = dict(os.environ)
    env.update({
        "TIM_HEADLESS": "1",
        "TIM_STOPFLIP": str(flips),
        "TIM_TRACE": "level",
        "TIM_GAMEDIR": game,
        "TIM_LOADMACHINE": name,
    })
    if simulate:
        # **The machine without the game around it.** `TIM_SIMULATE` runs the
        # loaded machine through the game's own per-frame step - no clock, no
        # input, no display, no timer thread - and reports the goal test. It
        # answers in milliseconds where `--run` takes the better part of a
        # minute, and it answers the same way every time, which the real loop
        # does not (see CLAUDE.md on the timer thread). What it does not
        # exercise is the loop's own input and presentation path, so it is the
        # fast check between edits and not a replacement for the real one.
        env["TIM_SIMULATE"] = str(simulate)
        p = subprocess.run([devtim, "--level", str(level)], env=env,
                           capture_output=True, text=True, timeout=300)
    else:
        p = subprocess.run([devtim, "--level", str(level), "--run"], env=env,
                           capture_output=True, text=True, timeout=900)
    err = p.stderr
    solved = ("io: simulate solved=1" in err) if simulate else ("io: level solved" in err)
    # **A port that solves and then dies is not a pass.** The run exits 0 when
    # it finishes; anything else is the port falling over, and this check used
    # to score it on whatever it managed to print first. Measured on
    # 2026-09-10, with `dg_near` newly refusing a non-guest pointer: the port
    # aborted with SIGABRT on every level and this printed **33 of 33 solved**,
    # because "io: level solved" really was in the output - four lines before
    # the crash.
    crashed = p.returncode != 0 or "io: PORT ABORTED" in err
    # A stub reached is a different answer from "did not solve", and must not
    # be reported as one.
    stub = ("not transcribed" in err) or ("reached 0x" in err)
    # **A load that failed is not a machine that did not solve.** `read_level`
    # answers 0 for a file it cannot open and the autoplay driver reports the
    # load either way, so the one line that says the parts arrived is the
    # loader's own - without it the level runs empty and fails for a reason
    # that has nothing to do with the engine.
    loaded = ("autoplay loaded the machine" in err)
    return solved, stub, loaded, crashed, err


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dir", default=os.path.join(tim.REPO, "solutions"),
                    help="where the solution machine files are")
    ap.add_argument("--flips", type=int, default=2500,
                    help="how long to let each machine run (default %(default)s)")
    ap.add_argument("--only", default="",
                    help="a comma-separated list of solution names to run")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="pass the port's own stderr through")
    ap.add_argument("-j", "--jobs", type=int, default=0, metavar="N",
                    help="how many levels to run at once (default: as many "
                         "as there are cores). Each is its own process "
                         "reading the same staged directory and writing "
                         "nothing, so the only thing they share is the "
                         "machine they run on")
    ap.add_argument("--simulate", type=int, nargs="?", const=6000, default=0,
                    metavar="FRAMES",
                    help="run each machine through the game's per-frame step "
                         "with no clock, input or display, up to FRAMES "
                         "frames (default 6000) - the same verdict in "
                         "milliseconds; see run_one")
    args = ap.parse_args()

    paths = sorted(glob.glob(os.path.join(args.dir, "S*.TIM")))
    if args.only:
        want = set(args.only.split(","))
        paths = [p for p in paths
                 if os.path.basename(p).rsplit(".", 1)[0] in want]
    if not paths:
        # Two different "no", and they used to print the same line: an empty
        # directory and a `--only` that matched nothing. The second is a typo
        # - `--only 1,2` rather than `S01,S02` - and reading it as the first
        # sends you looking for missing files that are right there.
        if args.only:
            raise SystemExit(
                "--only %s matched none of the %d machines in %s"
                % (args.only,
                   len(glob.glob(os.path.join(args.dir, "S*.TIM"))),
                   args.dir))
        raise SystemExit("no S*.TIM solutions in %s" % args.dir)

    game = staged(paths)
    bad = 0
    # **Built once, before anything runs.** `tim.built` runs `make`, and a
    # `make` from several threads at once is a build racing itself; calling it
    # here means every worker finds the binary already there.
    devtim = tim.built("devtim")
    jobs = args.jobs or min(len(paths), os.cpu_count() or 1)
    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            runs = []
            for path in paths:
                name = os.path.basename(path).upper()
                stem = name.rsplit(".", 1)[0]
                level = level_of(stem)
                if level is None:
                    runs.append((stem, None, None))
                    continue
                runs.append((stem, level, pool.submit(
                    run_one, devtim, name, level, game, args.flips,
                    args.simulate)))

        for stem, level, fut in runs:
            if fut is None:
                print("  %-10s NO LEVEL IN THE NAME - skipped" % stem)
                bad += 1
                continue
            solved, stub, loaded, crashed, err = fut.result()
            # Passed through here rather than as it happens, because several
            # levels run at once and two interleaved runs read as one.
            if args.verbose and err:
                sys.stderr.write(err)
            if crashed:
                print("  %-10s THE PORT DIED - no verdict about solving%s"
                      % (stem, " (it did report solving first)" if solved else ""))
                bad += 1
            elif stub:
                print("  %-10s REACHED A STUB - not a failure to solve" % stem)
                bad += 1
            elif not loaded:
                print("  %-10s THE MACHINE NEVER LOADED - no verdict about "
                      "solving" % stem)
                bad += 1
            elif solved:
                print("  %-10s solved (level %d)" % (stem, level))
            else:
                print("  %-10s DID NOT SOLVE in %d %s"
                      % (stem, args.simulate or args.flips,
                         "simulated frames" if args.simulate else "flips"))
                bad += 1
    finally:
        shutil.rmtree(game, ignore_errors=True)

    print()
    print("%d of %d solved" % (len(paths) - bad, len(paths)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
