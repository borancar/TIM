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
as a **machine file** rather than as a snapshot: `solutions/S<NN>.TIM` is what
the game's own `save_machine` wrote, and both sides read it back through the
game's own `round_teardown` / `load_animation` / `reset_machine`. The goal it is
judged against comes from the level, which is why each run is a level number and
a file and nothing else.

A port snapshot cannot do this job and was tried: `TIMPORT1` carries memory and
io state and no CPU registers, because the port has no guest CPU to save.

**The port is run twice, and that control is the verdict.** Measured on level
06 when this was written: eight runs of the port, same level and same machine
file, produced **five distinct frame sequences**, and two whole sweeps minutes
apart disagreed about eleven of twenty-eight levels. Its `run_machine_loop`
waits for *at least* eight ticks and the number that have actually gone by
depends on when a real-time timer thread got scheduled, so the simulation was
not reproducible - while the hybrid, whose ticks are a fixed 3.95 per present,
was byte for byte identical across runs.

So a disagreement between the two sides says nothing until the port has been
shown to agree with itself on that level, and a run where it does not is
reported as the port moving rather than as a difference between them. The
control stays for that reason, and because it is the one thing that would say
the fault had come back.

**Measured on 2026-09-18: 29 of 29 levels, 685 flips each, byte for byte, with
no unstable flip anywhere.** The port now agrees with itself as well as with
the hybrid. Two things made the difference, and neither was the timer: the
counters do not step while the machine runs - `step_counters` is the editor
loop's, which is what the odometer work of the days before settled - so the
tick's jitter no longer reaches a pixel; and the two sides' drivers now take
their steps on the same cue, the guest page flip, and are aligned on the flip
the machine starts.

**What this does and does not say about the tick.** The frames the machine
draws are now reproducible; the tick underneath them is still a real-time
thread and STATUS.md still defers that. Two things this tool does not look at
could still move with it - the elapsed time the score is banked from, and any
screen paced by ticks rather than by frames, which is why the flips before the
machine starts are left to `check_native.py` and `check_briefing.py`.

**Both sides are pointed at a staged copy of the game directory.** The guest's
file layer treats its directory as a floor, so a machine file has to be *inside*
it to be openable at all; this used to mean writing the files into
`incredible-machine/` itself. `TIM_GAMEDIR` now works for the hybrid as well as
for the port, so the copy is made in a temporary directory with the solutions
beside the game's own files and nothing here writes to the game's folder.

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

DEVTIM = tim.built("devtim")
# Built, not just named. A check that takes whatever binary is on disk compares
# two different ages of the code - docs/lessons.md has the two findings that
# cost, and one of them was this tool reporting 66 intro flips byte for byte
# against a hybrid built before the routine that was broken.
NATIVE = tim.built("native", where=os.path.join("tools", "native"))


def level_of(name):
    """`S07` -> 7. The number is the puzzle, and the puzzle is the goal."""
    m = re.search(r"(\d+)", name)
    return int(m.group(1), 10) if m else None


def staged(paths):
    """A game directory with the solutions in it, and its path.

    A copy rather than the game's own folder, because nothing here writes to
    that - the same rule `tools/fixture.py` states - and both binaries are
    pointed at it with `TIM_GAMEDIR`.
    """
    out = tempfile.mkdtemp(prefix="tim-machines-")
    shutil.copytree(tim.GAME_DIR, out, dirs_exist_ok=True)
    for path in paths:
        shutil.copy(path, os.path.join(out, os.path.basename(path).upper()))
    return out


def started_at(err):
    """The flip the driver started the machine on, from its own message.

    **Both sides number their steps in guest page flips**, and this is the one
    signal the comparison aligns on: the machine's first running frame. Before
    it the two are showing screens that slide in on the *tick*, and the two
    clocks are not the same one - `check_native.py` and `check_briefing.py` are
    where those screens are checked. After it they are running the same machine
    from the same reset state, and a flip is a flip.
    """
    m = re.search(r"autoplay starts the machine at flip (\d+)", err)
    return int(m.group(1), 10) if m else None


def port_run(level, machine, game, hashes, flips, verbose):
    """One port run of the machine, hashed per flip.

    **It does not report whether the puzzle solved, and that is deliberate.**
    It did at first, and said so of level 10 - which then solved on screen with
    all three guns firing. The run had simply been cut off: `--flips` is chosen
    for how much of the machine is worth *comparing*, and a machine that takes
    longer than that to reach its goal is not a machine that failed. The same
    short-budget mistake CLAUDE.md records against `--only`, made again in a
    tool an hour old.

    `check_solutions.py` is where "did it solve" is asked, at its own budget of
    2500 flips. Two tools answering one question with different budgets is how
    a false verdict gets a second source.
    """
    env = dict(os.environ)
    env.update({"TIM_HEADLESS": "1", "TIM_STOPFLIP": str(flips),
                "TIM_GAMEDIR": game,
                "TIM_LOADMACHINE": machine, "TIM_FLIPHASH": hashes})
    p = subprocess.run([DEVTIM, "--level", str(level), "--run"], env=env,
                       capture_output=True, text=True, timeout=600)
    if verbose:
        sys.stderr.write(p.stderr)
    return started_at(p.stderr)


def hybrid_run(level, machine, game, hashes, presents, verbose):
    env = dict(os.environ)
    env.update({"TIM_HEADLESS": "1", "TIM_STOP": str(presents),
                "TIM_LEVEL": str(level), "TIM_RUN": "1", "TIM_GAMEDIR": game,
                "TIM_LOADMACHINE": machine, "TIM_GUESTHASH": hashes})
    p = subprocess.run([NATIVE], env=env, capture_output=True, text=True,
                       timeout=900)
    if verbose:
        sys.stderr.write(p.stderr)
    return (started_at(p.stderr),
            "loaded the machine" in p.stderr)


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


def one_level(machine, name, level, game, out, args):
    """The three runs one level needs, and the digests they leave.

    **The port twice and the hybrid once, all at the same time.** The three are
    separate processes writing to three files and sharing nothing but the
    staged game directory, which none of them writes to. The port's own
    non-determinism is a matter of when its timer thread is scheduled, and that
    is what the two port runs measure - a busier machine makes the control
    harder to pass, not less honest.
    """
    ph = os.path.join(out, "%s.port.txt" % name)
    ph2 = os.path.join(out, "%s.port2.txt" % name)
    hh = os.path.join(out, "%s.hybrid.txt" % name)

    for f in (ph, ph2, hh):
        if os.path.exists(f):
            os.remove(f)

    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        a = pool.submit(port_run, level, machine, game, ph, args.flips,
                        args.verbose)
        b = pool.submit(port_run, level, machine, game, ph2, args.flips,
                        args.verbose)
        c = pool.submit(hybrid_run, level, machine, game, hh, args.presents,
                        args.verbose)
        pstart = a.result()
        b.result()
        hstart, loaded = c.result()

    return digests(ph), digests(ph2), digests(hh), loaded, pstart, hstart


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dir", default=os.path.join(tim.REPO, "solutions"),
                    help="where the solution machine files are")
    ap.add_argument("--only", default="",
                    help="a comma-separated list of solution names to run")
    ap.add_argument("--flips", type=int, default=700,
                    help="port flips to compare (default %(default)s)")
    ap.add_argument("--presents", type=int, default=2400,
                    help="hybrid presents to run for; it needs enough to "
                         "produce --flips guest flips (default %(default)s)")
    ap.add_argument("-j", "--jobs", type=int, default=0, metavar="N",
                    help="how many levels to run at once. Each level is three"
                         " processes, so the default is a third of the cores")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="pass both binaries' stderr through")
    args = ap.parse_args()

    out = os.path.join(tim.REPO, "out")
    os.makedirs(out, exist_ok=True)

    paths = sorted(glob.glob(os.path.join(args.dir, "S*.TIM")))
    if args.only:
        want = set(args.only.split(","))
        paths = [p for p in paths
                 if os.path.basename(p).rsplit(".", 1)[0] in want]
    if not paths:
        raise SystemExit("no S*.TIM solutions in %s" % args.dir)

    game = staged(paths)
    bad = 0
    jobs = args.jobs or max(1, (os.cpu_count() or 3) // 3)
    runs = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        for path in paths:
            machine = os.path.basename(path).upper()
            name = machine.rsplit(".", 1)[0]
            level = level_of(name)
            if level is None:
                runs.append((name, None))
                continue
            runs.append((name, pool.submit(one_level, machine, name, level,
                                           game, out, args)))

    for name, fut in runs:
        if fut is None:
            print("  %-10s NO LEVEL IN THE NAME - skipped" % name)
            bad += 1
            continue
        p, p2, h, loaded, pstart, hstart = fut.result()

        note = ""
        if not loaded:
            note += "  HYBRID DID NOT LOAD"
        if pstart is None or hstart is None:
            print("  %-10s NEITHER SIDE SAID WHEN THE MACHINE STARTED%s"
                  % (name, note))
            bad += 1
            continue
        if pstart != hstart:
            # Worth saying rather than quietly allowing for: the two drivers
            # take their steps on the same cue, so a different flip means one
            # of them saw a different screen on the way in.
            note += "  (started at %d / %d)" % (pstart, hstart)

        # **From the machine's first running frame**, each side counted from
        # its own start. Everything before it is a screen paced by the tick,
        # and the two clocks are not the same one.
        p, p2, h = p[pstart:], p2[pstart:], h[hstart:]
        n = min(len(p), len(p2), len(h))

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

        if agree != len(stable) or not loaded:
            bad += 1

    shutil.rmtree(game, ignore_errors=True)

    print()
    print("%d of %d levels agree on every flip the port reproduces"
          % (len(paths) - bad, len(paths)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
