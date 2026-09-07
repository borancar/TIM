#!/usr/bin/env python3
"""Prove the hybrid and the port run the same solved machine, flip for flip.

**This is the strongest end-to-end check in the project**, and it exists
because the two weaker ones each leave a gap. `check_solutions.py` asks only
"did the machine solve", which a wrong pixel cannot fail; `check_native.py`
compares the two sides across the *intro*, which no part hook and no mover ever
reaches. This runs a solved puzzle on both sides and compares every page flip.

It is possible at all because of two things. The tick now obeys the chip - both
sides advance the guest's clock at the 236.7 Hz divisor 5041 asks for - so flip
N is the same moment on each, with no content alignment. And a solution travels
as a **machine file** rather than as a snapshot: the port writes one through the
game's own `save_machine`, and both sides read it back through the game's own
`round_teardown` / `load_animation` / `reset_machine`. The goal it is judged
against comes from the level, which is why each run is a level number and a
file and nothing else.

A port snapshot cannot do this job and was tried: `TIMPORT1` carries memory and
io state and no CPU registers, because the port has no guest CPU to save.

**The port is run twice, and that control is the verdict.** Measured on level
06 before this was written: eight runs of the port, same level and same machine
file, produced **five distinct frame sequences**. Its `run_machine_loop` waits
for *at least* eight ticks and the number that have actually gone by depends on
when a real-time timer thread got scheduled, so the simulation is not
reproducible - while the hybrid, whose ticks are a fixed 3.95 per present, is
byte for byte identical across runs.

So a disagreement between the two sides says nothing until the port has been
shown to agree with itself on that level, and a run where it does not is
reported as the port moving rather than as a difference between them. This is
the same defect STATUS.md defers under the timer's concurrency, seen from a new
side: it is not only a stray column of odometer digits, it is the machine
itself taking a different path.

The machine files are written into the game's own directory, which is where the
loader looks and which is not in the repository - so they are derived artefacts
and are rebuilt from the snapshots on every run.

This file is the port's own tooling; it is not a transcription.
"""
import argparse
import glob
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tim

DEVTIM = os.path.join(tim.REPO, "reconstruct", "devtim")
NATIVE = os.path.join(tim.REPO, "tools", "native", "native")


def level_of(name):
    """`level07` -> 7. The number is the puzzle, and the puzzle is the goal."""
    m = re.search(r"(\d+)", name)
    return int(m.group(1), 10) if m else None


def extract(snapshot, machine, gamedir, verbose):
    """Write the snapshot's machine out through the game's own `save_machine`."""
    env = dict(os.environ)
    env.update({"TIM_HEADLESS": "1", "TIM_SAVEDIR": gamedir,
                "TIM_SAVEMACHINE": machine})
    p = subprocess.run([DEVTIM, "--restore", snapshot], env=env,
                       capture_output=True, text=True, timeout=300)
    if verbose:
        sys.stderr.write(p.stderr)
    return os.path.exists(os.path.join(gamedir, machine))


def port_run(level, machine, hashes, flips, verbose):
    env = dict(os.environ)
    env.update({"TIM_HEADLESS": "1", "TIM_STOPFLIP": str(flips),
                "TIM_TRACE": "level", "TIM_LOADMACHINE": machine,
                "TIM_FLIPHASH": hashes})
    p = subprocess.run([DEVTIM, "--level", str(level), "--run"], env=env,
                       capture_output=True, text=True, timeout=600)
    if verbose:
        sys.stderr.write(p.stderr)
    return "io: level solved" in p.stderr


def hybrid_run(level, machine, hashes, presents, verbose):
    env = dict(os.environ)
    env.update({"TIM_HEADLESS": "1", "TIM_STOP": str(presents),
                "TIM_LEVEL": str(level), "TIM_RUN": "1",
                "TIM_LOADMACHINE": machine, "TIM_GUESTHASH": hashes})
    p = subprocess.run([NATIVE], env=env, capture_output=True, text=True,
                       timeout=900)
    if verbose:
        sys.stderr.write(p.stderr)
    return "loaded the machine" in p.stderr


def digests(path):
    out = []
    try:
        for line in open(path):
            bits = line.split()
            if len(bits) == 2:
                out.append(bits[1])
    except OSError:
        pass
    return out


def compare(a, b):
    """Identical-at-the-same-number, and the longest run of them.

    **No alignment, deliberately.** `check_native.py` aligns by content because
    it has to - across the intro the two clocks were nothing like each other.
    Here they are the same clock, so an offset would be hiding something rather
    than allowing for it.
    """
    n = min(len(a), len(b))
    same = sum(1 for i in range(n) if a[i] == b[i])
    best = run = 0
    for i in range(n):
        if a[i] == b[i]:
            run += 1
            best = max(best, run)
        else:
            run = 0
    first = [i for i in range(n) if a[i] != b[i]]
    return n, same, best, first


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dir", default=os.path.join(tim.REPO, "solution_snaps"),
                    help="where the .solution snapshots are")
    ap.add_argument("--only", default="",
                    help="a comma-separated list of level names to run")
    ap.add_argument("--flips", type=int, default=700,
                    help="port flips to compare (default %(default)s)")
    ap.add_argument("--presents", type=int, default=2400,
                    help="hybrid presents to run for; it needs enough to "
                         "produce --flips guest flips (default %(default)s)")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="pass both binaries' stderr through")
    args = ap.parse_args()

    gamedir = tim.game_dir()
    out = os.path.join(tim.REPO, "out")
    os.makedirs(out, exist_ok=True)

    paths = sorted(glob.glob(os.path.join(args.dir, "*.solution")))
    if args.only:
        want = set(args.only.split(","))
        paths = [p for p in paths
                 if os.path.basename(p).rsplit(".", 1)[0] in want]
    if not paths:
        raise SystemExit("no .solution snapshots in %s" % args.dir)

    bad = 0
    for path in paths:
        name = os.path.basename(path).rsplit(".", 1)[0]
        level = level_of(name)
        machine = "S%02d.TIM" % level
        ph = os.path.join(out, "%s.port.txt" % name)
        hh = os.path.join(out, "%s.hybrid.txt" % name)

        for f in (ph, hh):
            if os.path.exists(f):
                os.remove(f)

        if not extract(path, machine, gamedir, args.verbose):
            print("  %-10s COULD NOT EXTRACT a machine file" % name)
            bad += 1
            continue

        ph2 = os.path.join(out, "%s.port2.txt" % name)
        if os.path.exists(ph2):
            os.remove(ph2)

        solved = port_run(level, machine, ph, args.flips, args.verbose)
        port_run(level, machine, ph2, args.flips, args.verbose)
        loaded = hybrid_run(level, machine, hh, args.presents, args.verbose)

        p, p2, h = digests(ph), digests(ph2), digests(hh)
        n = min(len(p), len(p2), len(h))

        note = "" if solved else "  PORT DID NOT SOLVE"
        if not loaded:
            note += "  HYBRID DID NOT LOAD"

        if n == 0:
            print("  %-10s no flips to compare%s" % (name, note))
            bad += 1
            continue

        # **Only the flips the port reproduces are evidence.** Where its two
        # runs of the same machine disagree, it has told us it does not know
        # what that frame should be, and holding the hybrid to one of the two
        # answers would be a coin toss. Those flips are excluded and counted,
        # so the report says how much of the run was usable as well as how much
        # agreed.
        stable = [i for i in range(n) if p[i] == p2[i]]
        agree = sum(1 for i in stable if h[i] == p[i])
        run = best = 0
        for i in stable:
            if h[i] == p[i]:
                run += 1
                best = max(best, run)
            else:
                run = 0

        print("  %-10s %4d of %4d stable flips agree, longest run %-4d "
              "(%d unstable)%s"
              % (name, agree, len(stable), best, n - len(stable), note))

        if agree != len(stable) or not solved or not loaded:
            bad += 1

    print()
    print("%d of %d levels agree on every flip the port reproduces"
          % (len(paths) - bad, len(paths)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
